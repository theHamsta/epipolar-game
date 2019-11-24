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

    [[nodiscard]] inline auto toPointsOnLine(float screenWidth, float screenHeight) const -> PointsOnLine
    {
        auto offsetX = screenWidth * 0.5f;
        auto offsetY = offset + screenHeight * 0.5f;
        auto x       = std::cos(angle) * 8000;
        auto y       = std::sin(angle) * 8000;
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
    auto operator=(GameState &&) -> GameState& = default;
    auto operator=(const GameState&) -> GameState& = default;

    ScreenLine compareView{};
    ScreenLine groundTruth{};
    ScreenLine lineP1{};
    ScreenLine lineP2{};

    double scoreP1  = 0.f;
    double scoreP2  = 0.f;
    int roundNumber = 0;
};
