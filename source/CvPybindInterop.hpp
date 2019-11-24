/*
 * CvPybindInterop.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once

#include <cassert>
#include <opencv2/opencv.hpp>

#include "pybind11/numpy.h"
#include "python_include.hpp"

inline auto cvMatFromArray(const pybind11::array_t< float >& array) -> cv::Mat
{
    cv::Mat mat(array.shape(0), array.shape(1), CV_32FC1);
    // copy to be memory safe
    assert(array.ndim() == 2 && "Must be 2d!");
    for (int y = 0; y < array.shape(0); ++y)
    {
        for (int x = 0; x < array.shape(1); ++x)
        {
            mat.at< float >(y, x) = array.at(y, x);
        }
    }
    return mat;
}

inline auto cvMatFromArray(const pybind11::array_t< float >& array, int sliceIdx) -> cv::Mat
{
    cv::Mat mat(array.shape(1), array.shape(2), CV_32FC1);
    // copy to be memory safe
    assert(array.ndim() == 3 && "Must be 3d!");
    for (int y = 0; y < array.shape(1); ++y)
    {
        for (int x = 0; x < array.shape(2); ++x)
        {
            mat.at< float >(y, x) = array.at(sliceIdx, y, x);
        }
    }
    return mat;
}
