/*
 * GameState.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once
#include <cmath>

struct Point
{
    float x, y;
};

struct PointsOnLine
{
    Point a, b;
};

struct ScreenLine
{
    float offset;
    float angle;

    inline auto toPointsOnLine(float screenWidth, float screenHeight) const -> PointsOnLine
    {
        auto offsetX = offset + screenWidth * 0.5f;
        auto offsetY = screenWidth * 0.5f;
        auto x       = std::cos(angle) * 4000;
        auto y       = std::sin(angle) * 4000;
        return { { -x + offsetX, -y + offsetY }, { x + offsetX, y + offsetY } };
    }
};

class GameState
{
  public:
    GameState()                 = default;
    virtual ~GameState()        = default;
    GameState(GameState&&)      = default;
    GameState(const GameState&) = default;
    GameState& operator=(GameState&&) = default;
    GameState& operator=(const GameState&) = default;

    ScreenLine compareView;
    ScreenLine groundTruth;
    ScreenLine lineP1;
    ScreenLine lineP2;

    double scoreP1  = 0.f;
    double scoreP2  = 0.f;
    int roundNumber = 0;
};
