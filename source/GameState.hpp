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

    template< typename T >
    [[nodiscard]] inline auto toPointsOnLine(T screenWidth, T screenHeight) const -> PointsOnLine
    {
        auto offsetX = static_cast< float >(screenWidth) * 0.5f;
        auto offsetY = offset + static_cast< float >(screenHeight) * 0.5f;
        auto x       = std::cos(angle) * 8000;
        auto y       = std::sin(angle) * 8000;
        return { { -x + offsetX, -y + offsetY }, { x + offsetX, y + offsetY } };
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

    ScreenLine compareLine{};
    ScreenLine groundTruthLine{};
    ScreenLine lineP1{};
    ScreenLine lineP2{};

    double scoreP1        = 0.f;
    double scoreP2        = 0.f;
    int roundNumber       = 0;
    int volumeNumber      = 0;
    InputState inputState = InputState::None;
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
};
