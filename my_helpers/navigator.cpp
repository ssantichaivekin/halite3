#include "navigator.hpp"

#include <algorithm>

using namespace std;
using namespace hlt;

Navigator::Navigator(shared_ptr<GameMap>& gameMap, shared_ptr<Player>& me) {
    gameMap_ = gameMap;
    me_ = me;
    positionHasLessHaliteCmp_.gameMap_ = gameMap;
}

vector<Direction> Navigator::explore(shared_ptr<Ship> ship) {
    Position currentPos = ship->position;
    vector<Position> newPosList;
    for(int xDiff = -3; xDiff <= 3; xDiff++) {
        for(int yDiff = -3; yDiff <= 3; yDiff++) {
            Position newPos = currentPos + Position(xDiff, yDiff);
            gameMap_->normalize(newPos);
            newPosList.push_back(newPos);
        }
    }

    Position maxPos = *max_element(newPosList.begin(), newPosList.end(), positionHasLessHaliteCmp_);
    vector<Direction> targetDirs = gameMap_->get_unsafe_moves(ship->position, maxPos);
    return targetDirs;
}

vector<Direction> Navigator::dropoffHalite(shared_ptr<Ship> ship) {
    return gameMap_->get_unsafe_moves(ship->position, me_->shipyard->position);
}