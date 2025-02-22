/*
 * ImportVolumes.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once
#include <QDebug>
#include <cstdio>
#include <stdexcept>
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
}

template< typename T >
inline auto makeProjection(const pybind11::array_t< T >& volume)
    -> std::tuple< pybind11::array_t< T >, Geometry::ProjectionMatrix, float >
{
    namespace py = pybind11;
    using namespace pybind11::literals;
    auto locals = py::dict("vol"_a = volume);

    try
    {
        py::exec(R"(
import epipolar
projection, matrix, detector_spacing = epipolar.generate_projections(vol)
)",
                 py::globals(), locals);

        auto projection = locals["projection"].cast< pybind11::array_t< T > >();
        auto matrix     = locals["matrix"].cast< pybind11::array_t< T > >();
        auto spacing    = locals["detector_spacing"].cast<T>();
        Geometry::ProjectionMatrix eigenMatrix{};
        for (int k = 0; k < 3; ++k)
        {
            for (int l = 0; l < 4; ++l)
            {
                eigenMatrix(k, l) = static_cast< double >(matrix.at(k, l));
            }
        }
        return { projection, eigenMatrix, spacing };
    } catch (std::exception& exp)
    {
        qCritical() << "Could not open volumes from folder!";
        qCritical() << exp.what();
        return { pybind11::array_t< T >({ 10, 10 }), Geometry::ProjectionMatrix{}, 1.f };
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
assert len(projections) == len(matrices)
num_projections = len(projections)
#print(matrices)
#print(projections)
				 )",
             py::globals(), locals);
    auto vols = [&]() -> std::pair< std::vector< std::vector< pybind11::array_t< T > > >,
                                    std::vector< std::vector< Geometry::ProjectionMatrix > > > {
        try
        {
            auto len = locals["num_projections"].cast< int >();
            qDebug() << "I have " <<  len << " projections";
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
                    auto matrix = py::eval("matrices[i][sub_idx]", py::globals(), locals).cast< py::array_t< T > >();
                    if (matrix.shape()[0] != 3 || matrix.shape()[1] != 4)
                    {
                        qCritical() << "Matrix not 3x4!!";
                        qCritical() << "Shape: " << matrix.shape()[0] << " " << matrix.shape()[1];
                        throw std::runtime_error("Matrix not 3x4!!" __FILE__);
                    }
                    Geometry::ProjectionMatrix eigenMatrix{};
                    for (int k = 0; k < 3; ++k)
                    {
                        for (int l = 0; l < 4; ++l)
                        {
                            eigenMatrix(k, l) = static_cast< double >(matrix.at(k, l));
                        }
                    }

                    vec[i].push_back(projection);
                    matrices[i].push_back(eigenMatrix);
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
