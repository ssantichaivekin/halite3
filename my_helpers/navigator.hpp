#pragma once

#include "hlt/types.hpp"
#include "hlt/position.hpp"
#include "hlt/ship.hpp"
#include "hlt/dropoff.hpp"
#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "shipStatus.hpp"
#include <random>

using namespace std;
using namespace hlt;

/// The navigator stores information about map and navigation
/// It make high level decisions about where to go.
/// Planning : a navigator will just accept a ship and a mode (say,
/// explore or return home). -- It will (1) return a direction or
/// (2) TODO: keep the information and decide as whole?
class Navigator {
public:
    Navigator(shared_ptr<GameMap>& gameMap, shared_ptr<Player>& me, 
              unordered_map<EntityId, ShipStatus>& shipStatus, mt19937& rng);
    vector<Direction> explore(shared_ptr<Ship> ship);
    vector<Direction> collect(shared_ptr<Ship> ship);
    vector<Direction> dropoffHalite(shared_ptr<Ship> ship);
    vector<Direction> newShip(shared_ptr<Ship> ship);

private:
    shared_ptr<GameMap> gameMap_;
    shared_ptr<Player> me_;
    vector<vector<Halite>> bestReturnRoute_;
    unordered_map<EntityId, ShipStatus>& shipStatus_;
    int haliteNavigateThreshold_;
    mt19937 rng_;

    struct PositionHasLessHaliteCmp
    {
        PositionHasLessHaliteCmp(shared_ptr<GameMap> gameMap) {
            gameMap_ = gameMap;
        }

        shared_ptr<GameMap> gameMap_;

        bool operator()(const Position& a, const Position& b) const
        {
            return gameMap_->at(a)->halite < gameMap_->at(b)->halite;
        }
    };

    struct DirectionHasLessHaliteCmp
    {
        DirectionHasLessHaliteCmp(shared_ptr<GameMap> gameMap, shared_ptr<Ship> ship) {
            gameMap_ = gameMap;
            ship_ = ship;
        }

        shared_ptr<GameMap> gameMap_;
        shared_ptr<Ship> ship_;

        bool operator()(const Direction& a, const Direction& b) const
        {
            Position posA = gameMap_->destination_position(ship_->position, a);
            Position posB = gameMap_->destination_position(ship_->position, b);
            return gameMap_->at(posA)->halite < gameMap_->at(posB)->halite;
        }
    };

    struct DirectionHasGreaterHaliteCmp
    {
        DirectionHasGreaterHaliteCmp(shared_ptr<GameMap> gameMap, shared_ptr<Ship> ship) :
            directionHasLessHaliteCmp_(gameMap, ship) {}
        
        bool operator()(const Direction& a, const Direction& b) const
        {
            return !directionHasLessHaliteCmp_(a, b);
        }

        DirectionHasLessHaliteCmp directionHasLessHaliteCmp_;
    };

    struct LessFavorablePositionCmp
    {
        LessFavorablePositionCmp(shared_ptr<GameMap> gameMap,
                                  Position centerPos,
                                  int haliteNavigateThreshold);

        shared_ptr<GameMap> gameMap_;
        Position centerPos_;
        int haliteNavigateThreshold_;
        vector<pair<Direction, int>> directionalPenalty_;

        int evaluatePosition(Position pos) const;
        bool noValidPosition(vector<Position> posList) const;

        bool operator()(const Position& a, const Position& b) const {
            int valA = evaluatePosition(a);
            int valB = evaluatePosition(b);
            return valA < valB;
        }
    };

    vector<Direction> wiggleDirectionsMostHalite(shared_ptr<Ship> ship);
    bool confusedExploringShipNearby(shared_ptr<Ship> ship);
    vector<Position> getSurroundingPositions(Position middlePos, int lookAhead);
    int calculateNavigateThreshold();
};