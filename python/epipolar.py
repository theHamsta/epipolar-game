# -*- coding: utf-8 -*-
#
# Copyright © 2019 Stephan Seitz <stephan.seitz@fau.de>
#
# Distributed under terms of the GPLv3 license.

"""

"""

import numpy as np

import pystencils
import sympy
# from conebeam_projector import CudaProjector
from pystencils_reco.projection import forward_projection

projection_global = {}


def read_volumes(dirname):
    volumes = []
    for i in range(4):
        volumes.append(np.random.randn(20, 30, 40))
    return volumes


def make_projections_cudajit(vol, proj, r1, r2, r3, detector_spacing, volume_spacing, cuda=True):
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
        import pycuda.autoinit
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
    # import pycuda.autoinit
    # from pycuda.gpuarray import to_gpu

    # vol = np.random.rand(100, 100, 100).astype(np.float32)
    # vol = to_gpu(vol)
    # pro = np.zeros((1024, 960))
    # proj = to_gpu(proj)
    # projection_matrix = sympy.Matrix([[-289.0098977737411, -1205.2274801832275, 0.0, 186000.0],
                                      # [-239.9634468375339, - 4.188577544948043, 1200.0, 144000.0],
                                      # [-0.9998476951563913, -0.01745240643728351, 0.0, 600.0]])

    # projector = CudaProjector(with_pyconrad=False)
    # projector.init_without_pyconrad([projection_matrix, projection_matrix],
                                    # [1, 1, 1],
                                    # vol.shape,
                                    # proj.shape,
                                    # [0, 0, 0]
                                    # )
    # projector.forward_project_cuda_idx(vol, proj, 0)
    # import pyconrad.autoinit
    # pyconrad.imshow(proj)

    global projection_global
    if not projection_global:
        # vol = pystencils.fields('vol: float32[100,100,100]')
        # proj = pystencils.fields('proj: float32[1024, 960]')
        vol = pystencils.fields('vol: float32[3d]')
        proj = pystencils.fields('proj: float32[2d]')

        vol.set_coordinate_origin_to_field_center()
        # volume.coordinate_transform = sympy.rot_axis3(0.1)
        proj.set_coordinate_origin_to_field_center()

        r1, r2, r3, detector_spacing, volume_spacing = sympy.symbols('r1,r2,r3, detector_spacing, volume_spacing')
        detector_spacing = 1
        volume_spacing=1

        projection_matrix = sympy.Matrix([[-289.0098977737411, -1205.2274801832275, 0.0, 186000.0],
                                          [-239.9634468375339, - 4.188577544948043, 1200.0, 144000.0],
                                          [-0.9998476951563913, -0.01745240643728351, 0.0, 600.0]])
        rotation = sympy.rot_axis1(r1) * sympy.rot_axis2(r2) * sympy.rot_axis3(r3)
        extended_rotation = sympy.Identity(4).as_explicit().as_mutable()
        extended_rotation[:3, :3] = rotation
        projection_matrix = projection_matrix * extended_rotation

        vol.coordinate_transform *= volume_spacing
        proj.coordinate_transform *= detector_spacing

        # projection_matrix = projection_matrix * extended_rotation

        from pystencils_autodiff.backends.astnodes import PybindModule
        assignments = forward_projection(vol, proj, projection_matrix, min_t=-100, max_t=100)
        ast = pystencils.create_kernel(assignments, target='cpu', cpu_openmp=True)  # .compile()
        ast.function_name = "projection_kernel"
        module = PybindModule('projector', [ast])
        kernel = module.compile().call_projection_kernel
        projection_global = kernel

    # import pycuda.autoinit
    # from pycuda.gpuarray import to_gpu
    to_gpu = np.copy

    import pyconrad.autoinit
    phantom3d = pyconrad.phantoms.shepp_logan(100, 100, 100).astype(np.float32)
    volume = np.random.rand(100, 100, 100).astype(np.float32)
    proj = np.zeros((1024, 960)).astype(np.float32)
    volume_gpu = to_gpu(phantom3d)
    projection_gpu = to_gpu(proj)
    projection_global(vol=volume_gpu, proj=projection_gpu, r1=0, r2=0, r3=0, detector_spacing=1, volume_spacing=1)

    import pyconrad.autoinit
    pyconrad.imshow(projection_gpu)


# def forward_projection(input_volume_field,
    # output_projections_field,
    # projection_matrix,
    # step_size=1,

    volumes = []
    for i in range(4):
        volumes.append(np.random.randn(20, 30, 40))
    return volumes


if __name__ == "__main__":
    generate_projections('')
