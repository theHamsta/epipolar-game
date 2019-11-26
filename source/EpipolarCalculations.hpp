/*
 * EpipolarCalculations.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once

#include "GameState.hpp"
#include "SourceDetectorGeometry.h"

auto getEpipolarLines(const Geometry::ProjectionMatrix& p1, const Geometry::ProjectionMatrix& p2, const Geometry::RP3Point& randomPoint,
                      double detectorSpacing) -> std::pair< ScreenLine, ScreenLine >;
