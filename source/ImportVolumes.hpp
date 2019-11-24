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
				 )",
             py::globals(), locals);
    auto vols = [&]() {
        try
        {
            return locals["vols"].cast< std::vector< py::array_t< T > > >();
        } catch (std::exception& exp)
        {
            qCritical() << "Could not open volumes from folder!";
            qCritical() << exp.what();
            return std::vector< py::array_t< T > >();
        }
    }();
    return vols;
}
