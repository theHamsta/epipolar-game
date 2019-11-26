/*
 * Scoring.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once

#include "GameState.hpp"
inline auto calcDistance [[nodiscard]] (ScreenLine l1, ScreenLine l2, int detectorWidth, int detectorHeight) -> float
{
    const float HALF     = 0.5f;
    auto detectorHeightF = static_cast< float >(detectorHeight);
    auto detectorWidthF  = static_cast< float >(detectorWidth);
    float halfDiagonal   = std::sqrt(detectorHeightF * detectorHeightF + detectorWidthF * detectorWidthF) * HALF;
    //l1.angle             = std::abs(l1.angle);
    //l2.angle             = std::abs(l2.angle);

    float x1 = std::cos(l1.angle) * halfDiagonal;
    float y1 = std::sin(l1.angle) * halfDiagonal + l1.offset;

    float x2 = std::cos(l2.angle) * halfDiagonal;
    float y2 = std::sin(l2.angle) * halfDiagonal + l2.offset;

    return std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}
