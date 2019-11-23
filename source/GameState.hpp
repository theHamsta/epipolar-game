/*
 * GameState.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once

struct ScreenLine
{
    float offset;
    float angle;
};

struct EpipolarLine
{
    float offset;
    float angle;
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
