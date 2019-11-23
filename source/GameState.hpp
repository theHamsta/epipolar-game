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

    inline auto toPointsOnLine() const -> PointsOnLine
    {
        auto x = std::cos(angle) * 400;
        auto y = std::sin(angle) * 400;
        return { { -x + offset, -y + offset }, { x + offset, y + offset } };
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
