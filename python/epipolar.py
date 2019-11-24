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
from pystencils_autodiff.backends.astnodes import PybindModule
from pystencils_reco.projection import forward_projection

projection = None


def read_volumes(dirname):
    volumes = []
    for i in range(4):
        volumes.append(np.random.randn(20, 30, 40))
    return volumes


def generate_projections(vol):
    if not projection:
        vol = pystencils.fields('vol: float32[3d]')
        proj = pystencils.fields('proj: float32[2d]')

        vol.set_coordinate_origin_to_field_center()
        proj.set_coordinate_origin_to_field_center()

        r1, r2, r3 = sympy.symbols('r1,r2,r3')

        projection_matrix = sympy.Matrix([[-289.0098977737411, -1205.2274801832275, 0.0, 186000.0],
                                          [-239.9634468375339, - 4.188577544948043, 1200.0, 144000.0],
                                          [-0.9998476951563913, -0.01745240643728351, 0.0, 600.0]])
        rotation = sympy.rot_axis1(r1) * sympy.rot_axis1(r2) * sympy.rot_axis1(r3)
        extended_rotation = sympy.Identity(4).as_explicit().as_mutable()
        extended_rotation[:3, :3] = rotation
        projection_matrix = projection_matrix * extended_rotation
        print(projection_matrix.shape)

        assignments = forward_projection(vol, proj, projection_matrix)
        ast = pystencils.create_kernel(assignments)
        ast.function_name = "projection_kernel"
        module = PybindModule('projector', [ast])
        module.compile()


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
