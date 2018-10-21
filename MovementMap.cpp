#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"
#include "MovementMap.hpp"

#include <random>
#include <ctime>
#include <queue>
#include <stack>
#include <algorithm>

using namespace std;
using namespace hlt;

/// Change the game map
void MovementMap::init(unique_ptr<GameMap>& gameMap) {
    gameMap_ = gameMap;
}

/// Reset everything. Use for new turn.
void MovementMap::clear() {
    shipsComingtoPos_.clear();
    shipDirectionQueue_.clear();
    allConflicts_ = {};
}

void MovementMap::addIntent(shared_ptr<Ship> ship, vector<Direction>& preferredDirs, bool ignoreEnemy = false) {
    if (!gameMap_->can_move(ship)) {
        Position currentPos = ship->position;
        shipDirectionQueue_[ship->position].push(Direction::STILL);
        shipsComingtoPos_[ship->position].push_back(ship);
        return;
    }
    Position currentPos = ship->position;
    for (Direction dir : preferredDirs) {
        shipDirectionQueue_[ship->position].push(dir);
    }
    Direction mostPreferredDir = preferredDirs[0];
    Position newPos = gameMap_->destination_position(currentPos, mostPreferredDir);
    shipsComingtoPos_[newPos].push_back(ship);
}

/// Initialize allConflicts_ and call resolve all
void MovementMap::resolveAllConflicts() {
    // Loop through the shipsComingtoPos_, obtain all conflicts
    // and keep it in a stack.
    for (auto kv : shipsComingtoPos_) {
        Position pos = kv.first;
        if (hasConflict(pos)) {
            allConflicts_.push(pos);
        }
    }
    iterateAndResolveConflicts();
}

/// Flush the outputs to Halite game engine
void MovementMap::flushOutputs() {

}

/// *************** Private section ****************

Direction MovementMap::currentDirection(shared_ptr<Ship> ship) {
    Position shipPos = ship->position;
    if (shipDirectionQueue_[shipPos].empty()) {
        return Direction::STILL;
    }
    return shipDirectionQueue_[shipPos].front();
}

Position MovementMap::destinationPos(shared_ptr<Ship> ship) {
    Direction currentDir = currentDirection(ship);
    Position targetDir = gameMap_->destination_position(ship->position, currentDir);
    return targetDir;
}

bool MovementMap::shipHasLessHalite(const shared_ptr<Ship>& ship1, const shared_ptr<Ship>& ship2) {
    return ship1->halite < ship2->halite;
}

/// Change the direction of ship to be the next direction in queue
void MovementMap::changeToNextDirection(shared_ptr<Ship> ship) {
    Position currentPos = ship->position;
    Position nextPos = destinationPos(ship);

    // remove
    auto eraseIndex = find(shipsComingtoPos_[nextPos].begin(), shipsComingtoPos_[nextPos].end(), ship);
    shipsComingtoPos_[nextPos].erase(eraseIndex);

    shipDirectionQueue_[currentPos].pop();

    // add : we have to get the destination again because
    // the current destination of the ship has changed by popping
    nextPos = destinationPos(ship);
    shipsComingtoPos_[nextPos].push_back(ship);
}

void MovementMap::redirectShip(shared_ptr<Ship> ship) {
    while(hasConflict(ship) && currentDirection(ship) != Direction::STILL) {
        changeToNextDirection(ship);
    }
    if(hasConflict(ship)) {
        allConflicts_.push(ship->position);
    }
}

/// Does this block has conflict
bool MovementMap::hasConflict(Position& pos) {
    return shipsComingtoPos_[pos].size() >= 2;
}

bool MovementMap::hasConflict(shared_ptr<Ship> ship) {
    Position targetDir = destinationPos(ship);
    return hasConflict(targetDir);
}

void MovementMap::iterateAndResolveConflicts() {
    while(!allConflicts_.empty()) {
        Position conflictMiddlePos = allConflicts_.front();
        allConflicts_.pop();
        resolveConflict(conflictMiddlePos);
    }
}

void MovementMap::resolveConflict(Position middlePos) {
    // If the middle position is empty, we allow the
    // ship with the greatest halite to go to the destination,
    // but don't allow other ships.
    if(gameMap_->at(middlePos)->is_empty()) {
        // get all the ships pointing here
        vector<shared_ptr<Ship>> shipsToRedirect = shipsComingtoPos_[middlePos];
        auto maxShipIndex = max_element(shipsToRedirect.begin(), shipsToRedirect.end(),
                                        shipHasLessHalite);
        shared_ptr<Ship> maxShip = *maxShipIndex;
        shipsToRedirect.erase(maxShipIndex);
        
        for (shared_ptr<Ship> ship : shipsToRedirect) {
            redirectShip(ship);
        }
    }

}