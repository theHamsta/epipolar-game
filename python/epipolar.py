# -*- coding: utf-8 -*-
#
# Copyright © 2019 Stephan Seitz <stephan.seitz@fau.de>
#
# Distributed under terms of the GPLv3 license.

"""

"""

import os
import random
from os.path import join

import numpy as np
import pandas
import pyconrad.autoinit
import pydicom
import sympy

# from conebeam_projector import CudaProjector

projection_global = {}


def read_volumes(dirname):
    volumes = []
    for i in range(4):
        volumes.append(np.random.randn(100, 100, 100))
    return volumes


# def read_projections(dirname):
    # projections = []
    # matrices = []
    # for i in range(4):
    # sub_projections = []
    # sub_matrices = []
    # for i in range(3):
    # sub_projections.append(np.random.randn(100, 100))

    # sub_projections.append(sub_projections)
    # sub_matrices.append(sub_matrices)
    # return projections, matrices

def read_projections(dirname):
    projections = [[]]
    matrices = [[]]

    try:
        for root, dirs, files in os.walk(dirname):
            if not dirs:
                assert 'pmat_3x4.txt' in files
                csv_file = join(root, 'pmat_3x4.txt')
                mat = np.array(pandas.read_csv(csv_file, sep=' ', header=None))
                dicom = [f for f in files if f.endswith('.IMA')][0]
                dc = np.array(pydicom.read_file(join(root, dicom)).pixel_array)
                if dc is not None:
                    if len(dc) > 1:
                        dc = dc[2]
                    else:
                        dc = dc[0]
                dc = dc.astype(np.float32)
                dc /= np.max(dc)
                projections[0].append(dc)
                mat = mat.astype(np.float32)
                # print(mat.shape)
                matrices[0].append(mat)
    except Exception as e:
        print(e)

    return projections, matrices


def make_projections_cudajit(vol, proj, r1, r2, r3, detector_spacing, volume_spacing, cuda=True):
    from pystencils_reco.projection import forward_projection
    import pystencils
    global projection_global
    if (vol.shape, proj.shape, detector_spacing, volume_spacing) not in projection_global:
        # vol = pystencils.fields('vol: float32[100,100,100]')
        # proj = pystencils.fields('proj: float32[1024, 960]')
        vol_f = pystencils.Field.create_from_numpy_array('vol', vol)
        proj_f = pystencils.Field.create_from_numpy_array('proj', proj)

        vol_f.set_coordinate_origin_to_field_center()
        # volume.coordinate_transform = sympy.rot_axis3(0.1)
        proj_f.set_coordinate_origin_to_field_center()

        sym_r1, sym_r2, sym_r3 = sympy.symbols('r1,r2,r3')

        projection_matrix = sympy.Matrix([[-289.0098977737411, -1205.2274801832275, 0.0, 186000.0],
                                          [-239.9634468375339, - 4.188577544948043, 1200.0, 144000.0],
                                          [-0.9998476951563913, -0.01745240643728351, 0.0, 600.0]])
        rotation = sympy.rot_axis1(sym_r1) * sympy.rot_axis2(sym_r2) * sympy.rot_axis3(sym_r3)
        extended_rotation = sympy.Identity(4).as_explicit().as_mutable()
        extended_rotation[:3, :3] = rotation
        projection_matrix = projection_matrix * extended_rotation

        vol_f.coordinate_transform *= volume_spacing
        proj_f.coordinate_transform *= detector_spacing

        # projection_matrix = projection_matrix * extended_rotation

        assignments = forward_projection(vol_f, proj_f, projection_matrix)
        kernel = pystencils.create_kernel(assignments, target='gpu' if cuda else 'cpu', cpu_openmp=True).compile()
        projection_global[(vol.shape, proj.shape, detector_spacing, volume_spacing)] = kernel

    if cuda:
        import pycuda.autoinit  # noqa
        from pycuda.gpuarray import to_gpu
        volume_gpu = to_gpu(vol)
        projection_gpu = to_gpu(proj)
    else:
        volume_gpu = vol
        projection_gpu = proj

    kernel = projection_global[(vol.shape, proj.shape, detector_spacing, volume_spacing)]
    kernel(vol=volume_gpu, proj=projection_gpu, r1=0, r2=0, r3=0)

    if cuda:
        return projection_gpu.get()
    else:
        return projection_gpu


def generate_projections(vol):
    import pycuda.autoinit  # noqa
    from conebeam_projector import CudaProjector
    import pyconrad.config
    from pycuda.gpuarray import to_gpu, zeros

    pyconrad.config.set_reco_shape(vol.shape)
    pyconrad.config.center_volume()
    rotation = np.random.rand(3) * 2 * np.pi
    rotation_matrix = np.array(sympy.rot_axis1(rotation[0]) *
                               sympy.rot_axis2(rotation[1]) * sympy.rot_axis3(rotation[2]))
    extended_rotation = sympy.Identity(4).as_explicit().as_mutable()
    extended_rotation[:3, :3] = rotation_matrix
    extended_rotation = np.array(extended_rotation)
    matrices = pyconrad.config.get_projection_matrices()

    idx = random.randint(0, len(matrices) - 1)
    my_matrix = matrices[idx] @ extended_rotation

    from edu.stanford.rsl.conrad.geometry import Projection
    from edu.stanford.rsl.conrad.numerics import SimpleMatrix

    pyconrad.config.get_geometry().setProjectionMatrix(idx, Projection(SimpleMatrix.from_numpy(my_matrix)))

    projector = CudaProjector()

    vol_gpu = to_gpu(vol)
    proj_gpu = zeros(pyconrad.config.get_sino_shape()[1:], np.float32)

    projector.forward_project_cuda_idx(vol_gpu, proj_gpu, idx)

    proj = proj_gpu.get()
    proj /= np.max(proj)

    detector_spacing = pyconrad.config.get_geometry().getPixelDimensionX()

    return proj, projector._projectionMatrices[idx], detector_spacing


if __name__ == "__main__":
    generate_projections('')
