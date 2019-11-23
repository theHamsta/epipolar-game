/*
 * ImportVolumes.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once
#include <string>
#include <vector>

#include "python_include.hpp"

template< typename T >
inline auto importVolumes(const std::string& dirname) -> pybind11::array_t< T >
{
    namespace py = pybind11;
    using namespace pybind11::literals;
    auto locals = py::dict("dirname"_a = dirname);

    py::exec(R"(
import epipolar
print(dir(epipolar))
vol = epipolar.read_volumes(dirname)
				 )",
             py::globals(), locals);
    py::array_t< T > vol = py::eval("vol", py::globals(), locals);
    return vol;
}
