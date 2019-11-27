/*
 * GameState.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once
#include <cmath>

#include "ProjectiveGeometry.hxx"

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

    template< typename T >
    [[nodiscard]] inline auto toPointsOnLine(T screenWidth, T screenHeight) const -> PointsOnLine
    {
        // Detector center == origin
        auto offsetX = static_cast< float >(screenWidth) * 0.5f;
        auto offsetY = offset + static_cast< float >(screenHeight) * 0.5f;

        // auto offsetX = 0;
        // auto offsetY = offset;
        auto x = std::cos(angle) * 8000;
        auto y = std::sin(angle) * 8000;
        // return { { -x + offsetX, screenHeight - (-y + offsetY) }, { x + offsetX, screenHeight - (y + offsetY) } };
        return { { -x + offsetX, -y + offsetY }, { x + offsetX, y + offsetY } };
    }
};

struct EpipolarScreenLine
{
    Geometry::RP2Point source;
    Geometry::RP2Point randomPoint;
    template< typename T >
    [[nodiscard]] inline auto toPointsOnLine(T screenWidth [[maybe_unused]], T screenHeight [[maybe_unused]])
        -> PointsOnLine
    {
        auto p1 = Point{ static_cast< float >(source[0] / source[2]), static_cast< float >(source[1] / source[2]) };
        auto p2 = Point{ static_cast< float >(randomPoint[0] / randomPoint[2]),
                         static_cast< float >(randomPoint[1] / randomPoint[2]) };

        auto m = (p2.y - p1.y) / (p2.x - p1.x);
        auto a = p2.y - m * p2.x;
        return { { -5000, -5000 * m + a }, { +5000, +5000 * m + a } };
    }
};

enum class InputState { InputBoth, InputP1, InputP2, None };

class GameState
{
  public:
    GameState()                 = default;
    virtual ~GameState()        = default;
    GameState(GameState&&)      = default;
    GameState(const GameState&) = default;
    auto operator=(GameState &&) -> GameState& = default;
    auto operator=(const GameState&) -> GameState& = default;

    EpipolarScreenLine compareLine{};
    EpipolarScreenLine groundTruthLine{};
    ScreenLine lineP1{};
    ScreenLine lineP2{};

    double scoreP1            = 0.f;
    double scoreP2            = 0.f;
    int roundNumber           = 0;
    int volumeNumber          = 0;
    int realProjectionsNumber = 0;
    InputState inputState     = InputState::None;
    void nextInputState()
    {
        switch (inputState)
        {
        case InputState::None:
            inputState = InputState::InputP1;
            break;
        case InputState::InputP1:
            inputState = InputState::InputP2;
            break;
        case InputState::InputP2:
        case InputState::InputBoth:
            inputState = InputState::None;
            break;
        }
    }
    bool realProjectionsMode = false;
};
