/*
 * ImportVolumes.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once
#include <QDebug>
#include <string>
#include <vector>

#include "pybind11/numpy.h"
#include "pybind11/pybind11.h"
#include "python_include.hpp"

template< typename T >
inline auto importVolumes(const std::string& dirname) -> decltype(auto)
{
    namespace py = pybind11;
    using namespace pybind11::literals;
    auto locals = py::dict("dirname"_a = dirname);

    py::exec(R"(
import epipolar
vols = epipolar.read_volumes(dirname)
num_vols = len(vols)
				 )",
             py::globals(), locals);
    auto vols = [&]() {
        try
        {
            std::vector< py::array_t< T > > vec;
            auto len = locals["num_vols"].cast< int >();
            for (int i = 0; i < len; ++i)
            {
                locals["i"] = i;
                auto array = py::eval("vols[i]", py::globals(), locals).cast< py::array_t< T > >();
                vec.push_back(array);
            }
            return vec;
        } catch (std::exception& exp)
        {
            qCritical() << "Could not open volumes from folder!";
            qCritical() << exp.what();
            return std::vector< py::array_t< T > >();
        }
    }();
    return vols;
}
