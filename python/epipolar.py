# -*- coding: utf-8 -*-
#
# Copyright © 2019 Stephan Seitz <stephan.seitz@fau.de>
#
# Distributed under terms of the GPLv3 license.

"""

"""

import numpy as np

def read_volumes(dirname):
    volumes = []
    for i in range(4):
        volumes.append(np.random.randn(20,30,40))
    return volumes


if __name__ == "__main__":
    read_volumes('')
