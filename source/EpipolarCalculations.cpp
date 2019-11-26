/*
 * EpipolarCalculations.cpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#include "EpipolarCalculations.hpp"

#include "GameState.hpp"
#include "ProjectiveGeometry.hxx"

static inline auto epipolarToScreenLine(const Geometry::RP2Line& line) -> ScreenLine
{
    // Geometry::RP2Line  Real 2D line (a,b,c) of points (x,y) with ax+by+c=0.
    //
    auto normalized = line.normalized();
    auto a          = static_cast< float >(normalized[0]);
    auto b          = static_cast< float >(normalized[1]);
    auto c          = static_cast< float >(normalized[2]);

    return ScreenLine{ c, std::atan2(b, a) };
}

auto getEpipolarLines(const Geometry::ProjectionMatrix& p1, const Geometry::ProjectionMatrix& p2, const Geometry::RP3Point& randomPoint,
                      double detectorSpacing) -> std::pair< ScreenLine, ScreenLine >
{
    Geometry::SourceDetectorGeometry geometry1(p1, detectorSpacing);
    Geometry::SourceDetectorGeometry geometry2(p2, detectorSpacing);

    //auto line = Geometry::join_pluecker(geometry1.C, geometry2.C);
    //Geometry::RP3Plane plane = Geometry::join_pluecker(line, randomPoint);

    //Geometry::RP3Line line1 = Geometry::meet_pluecker(plane, geometry1.image_plane);
    //Geometry::RP3Line line2 = Geometry::meet_pluecker(plane, geometry2.image_plane);

    //auto line = Geometry::join_pluecker(geometry1.C, geometry2.C);
    //Geometry::RP3Plane plane = Geometry::join_pluecker(line, randomPoint);

    Geometry::RP2Line line1 = Geometry::join(geometry1.project(geometry2.C), geometry1.project(randomPoint));
    Geometry::RP2Line line2 = Geometry::join(geometry2.project(geometry1.C), geometry1.project(randomPoint));


    return { epipolarToScreenLine(line1), epipolarToScreenLine(line2) };
}
