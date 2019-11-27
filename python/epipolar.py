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
    try:
        for root, dirs, files in os.walk(dirname):
            try:
                import pyconrad.dicom_utils
                vol, _, _, _ = pyconrad.dicom_utils.dicomdir2vol(root)
                if vol is not None and vol.ndim == 3:
                    volumes.append(vol)
            except Exception as e:
                print(e)

            try:
                vol, _, _, _ = pyconrad.dicom_utils.dicomdir2vol(root)
                if vol is not None and vol.ndim == 3:
                    volumes.append(vol)
            except Exception as e:
                print(e)
            for f in [f for f in files if f.endswith('.vdb')]:
                try:
                    import volume2mesh
                    grids = volume2mesh.read_vdb(join(root, f), return_spacing_origin=False)
                    vol = list(grids.values())[0]
                    print(vol)
                    volumes.append(vol)
                except Exception as e:
                    print(e)

            for f in [f for f in files if f.endswith('.obj')]:
                try:
                    import volume2mesh
                    vol = volume2mesh.mesh2volume(join(root, f), 30)
                    if vol.ndim == 3:
                        volumes.append((1-vol).astype(np.float32))
                except Exception as e:
                    print(e)

            for f in [f for f in files if f.endswith('.tif') or f.endswith('.tiff')]:
                try:
                    import skimage.io
                    vol = skimage.io.imread(join(root, f))
                    if vol.ndim == 3:
                        volumes.append(vol)
                except Exception as e:
                    print(e)
                    from edu.stanford.rsl.conrad.data.numeric import Grid3D

                    vol = np.array(Grid3D.from_tiff(join(root, f)))
                    if vol.ndim == 3:
                        volumes.append(vol)

    except Exception as e:
        print(e)
    if not volumes:
        for i in range(4):
            volumes.append(np.random.randn(100, 100, 100))

    return volumes
    # for i in range(4):
    # volumes.append(np.random.randn(100, 100, 100))
    # return volumes


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

    current_pumkin = 0
    try:
        for root, dirs, files in os.walk(dirname):
            print(f'Discovering {root}')
            if not dirs:
                print(f'Found folder with proj matrix {root}')
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
                projections[current_pumkin].append(dc)
                mat = mat.astype(np.float32)
                # print(mat.shape)
                matrices[current_pumkin].append(mat)
            elif projections[current_pumkin]:
                print(f'New pumkin {root}')
                current_pumkin += 1
                projections.append([])
                matrices.append([])
    except Exception as e:
        print(e)

    projections = [p for p in projections if p]
    matrices = [m for m in matrices if m]
    assert len(matrices) == len(projections)

    return projections, matrices


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
