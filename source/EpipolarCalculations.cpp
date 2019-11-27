/*
 * EpipolarCalculations.cpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#include "EpipolarCalculations.hpp"

#include <QDebug>

#include "GameState.hpp"
#include "ProjectiveGeometry.hxx"

static inline auto epipolarToScreenLine(const Geometry::RP2Line& line) -> ScreenLine
{
    // Geometry::RP2Line  Real 2D line (a,b,c) of points (x,y) with ax+by+c=0.
    //
    auto a = static_cast< float >(line[0]);
    auto b = static_cast< float >(line[1]);
    auto c = static_cast< float >(line[2]);

    return ScreenLine{ -c / b, std::atan2(b, a) + M_PI * 0.5f };
}

static inline auto epipolarToScreenLine(const Geometry::RP2Point& p1_homo, const Geometry::RP2Point& p2_homo)
    -> ScreenLine
{
    // Geometry::RP2Line  Real 2D line (a,b,c) of points (x,y) with ax+by+c=0.
    //
    auto p1      = Geometry::dehomogenized(p1_homo);
    auto p2      = Geometry::dehomogenized(p2_homo);
    auto angle   = std::atan2(p1[1] - p2[1], p1[0] - p2[0]);
    auto m       = (p1[1] - p2[1]) / (p1[0] - p2[0]);
    auto offset0 = p1[1] - m * p1[0];

    return ScreenLine{ offset0, angle };
}

auto getEpipolarLines(const Geometry::ProjectionMatrix& p1, const Geometry::ProjectionMatrix& p2,
                      const Geometry::RP3Point& randomPoint, double detectorSpacing)
    -> std::pair< EpipolarScreenLine, EpipolarScreenLine >
{
    qDebug() << detectorSpacing;
    Geometry::SourceDetectorGeometry geometry1(p1, detectorSpacing);
    Geometry::SourceDetectorGeometry geometry2(p2, detectorSpacing);

    qDebug() << "Random point: " << randomPoint(0) << ", " << randomPoint(1) << ", " << randomPoint(2) << ","
             << randomPoint(3);

    // Geometry::RP2Line line1 = Geometry::join(geometry1.project(geometry2.C), geometry1.project(randomPoint));
    // Geometry::RP2Line line2 = Geometry::join(geometry2.project(geometry1.C), geometry2.project(randomPoint));

    // return { epipolarToScreenLine(line1), epipolarToScreenLine(line2) };
    // return { epipolarToScreenLine(geometry1.project(geometry2.C), geometry1.project(randomPoint)),
    // epipolarToScreenLine(geometry2.project(geometry1.C), geometry2.project(randomPoint)) };
    return { { geometry1.project(geometry2.C), geometry1.project(randomPoint) },
             { geometry2.project(geometry1.C), geometry2.project(randomPoint) } };
}
