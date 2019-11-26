/*
 * ImportVolumes.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once
#include <QDebug>
#include <cstdio>
#include <string>
#include <tuple>
#include <vector>

#include "ProjectiveGeometry.hxx"
#include "pybind11/eigen.h"
#include "pybind11/numpy.h"
#include "pybind11/pybind11.h"
#include "python_include.hpp"

template< typename T >
inline auto importVolumes(const std::string& dirname) -> std::vector< pybind11::array_t< T > >
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
                auto array  = py::eval("vols[i]", py::globals(), locals).cast< py::array_t< T > >();
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

template< typename T >
inline auto makeProjection(const pybind11::array_t< T >& volume, pybind11::array_t< T >& projection, float r1, float r2,
                           float r3, float detector_spacing, float volume_spacing) -> void
{
    namespace py = pybind11;
    using namespace pybind11::literals;
    auto locals = py::dict("vol"_a = volume, "proj"_a = projection, "r1"_a = r1, "r2"_a = r2, "r3"_a = r3,
                           "detector_spacing"_a = detector_spacing, "volume_spacing"_a = volume_spacing);

    try
    {
        py::exec(R"(
import epipolar
projection = epipolar.make_projections_cudajit(vol, proj, r1,r2,r3, detector_spacing, volume_spacing)
)",
                 py::globals(), locals);
    } catch (std::exception& exp)
    {
        qCritical() << "Could not open volumes from folder!";
        qCritical() << exp.what();
    }
}

template< typename T >
inline auto importProjections(const std::string& dirname)
    -> std::pair< std::vector< std::vector< pybind11::array_t< T > > >,
                  std::vector< std::vector< Geometry::ProjectionMatrix > > >
{
    namespace py = pybind11;
    using namespace pybind11::literals;
    auto locals = py::dict("dirname"_a = dirname);

    py::exec(R"(
import epipolar
projections, matrices = epipolar.read_projections(dirname)
num_projections = len(vols)
				 )",
             py::globals(), locals);
    auto vols = [&]() -> std::pair< std::vector< std::vector< pybind11::array_t< T > > >,
                                    std::vector< std::vector< Geometry::ProjectionMatrix > > > {
        try
        {
            auto len = locals["num_projections"].cast< int >();
            std::vector< std::vector< py::array_t< T > > > vec(len);
            std::vector< std::vector< Geometry::ProjectionMatrix > > matrices(len);
            for (int i = 0; i < len; ++i)
            {
                locals["i"]             = i;
                auto num_subprojections = py::eval("len(projections[i])", py::globals(), locals).cast< int >();
                for (int sub_idx = 0; sub_idx < num_subprojections; ++sub_idx)
                {
                    locals["sub_idx"] = sub_idx;
                    auto projection =
                        py::eval("projections[i][sub_idx]", py::globals(), locals).cast< py::array_t< T > >();
                    auto matrix =
                        py::eval("proctions[i][sub_idx]", py::globals(), locals).cast< Geometry::ProjectionMatrix >();
                    vec[i].push_back(projection);
                    matrices[i].push_back(matrix);
                }
            }
            return { vec, matrices };
        } catch (std::exception& exp)
        {
            qCritical() << "Could not open projections from folder!";
            qCritical() << exp.what();
            return { std::vector< std::vector< py::array_t< T > > >(),
                     std::vector< std::vector< Geometry::ProjectionMatrix > >() };
        }
    }();
    return vols;
}

