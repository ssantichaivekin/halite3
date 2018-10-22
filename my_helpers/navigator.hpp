#pragma once

#include "hlt/types.hpp"
#include "hlt/position.hpp"
#include "hlt/ship.hpp"
#include "hlt/dropoff.hpp"
#include "hlt/game.hpp"
#include "hlt/constants.hpp"

using namespace std;
using namespace hlt;

/// The navigator stores information about map and navigation
/// It make high level decisions about where to go.
/// Planning : a navigator will just accept a ship and a mode (say,
/// explore or return home). -- It will (1) return a direction or
/// (2) TODO: keep the information and decide as whole?
class Navigator {
public:
    Navigator(shared_ptr<GameMap>& gameMap, shared_ptr<Player>& me);
    vector<Direction> explore(shared_ptr<Ship> ship);
    vector<Direction> dropoffHalite(shared_ptr<Ship> ship);
private:
    shared_ptr<Player> me_;
    shared_ptr<GameMap> gameMap_;
    vector<vector<Halite>> bestReturnRoute_;

    struct PositionHasLessHaliteCmp
    {
        shared_ptr<GameMap> gameMap_;

        bool operator()(Position a, Position b)
        {
            return gameMap_->at(a)->halite < gameMap_->at(b)->halite;
        }
    } positionHasLessHaliteCmp_;
};