#include "movement_map.hpp"

#include <queue>
#include <stack>
#include <algorithm>

using namespace std;
using namespace hlt;

/// *************** Public section ****************

MovementMap::MovementMap(shared_ptr<GameMap>& gameMap, shared_ptr<Player>& me, int nPlayers) {
    me_ = me;
    nPlayers_ = nPlayers;
    gameMap_ = gameMap;
    shipsComingtoPos_ = {};
    shipDirectionQueue_ = {};
    allConflicts_ = {};
    shouldMakeShip_ = false;
}

void MovementMap::addIntent(shared_ptr<Ship> ship, vector<Direction> preferredDirs, bool ignoreOpponentFlag) {
    //log::log("Add intent: ship " + to_string(ship->id));
    shipIgnoresOpponent_[ship->position] = ignoreOpponentFlag;
    if (!gameMap_->can_move(ship)) {
        shipDirectionQueue_[ship->position].push(Direction::STILL);
        shipsComingtoPos_[ship->position].push_back(ship);
        return;
    }
    Position currentPos = ship->position;
    for (Direction dir : preferredDirs) {
        shipDirectionQueue_[ship->position].push(dir);
    }
    if (preferredDirs.empty()) {
        //log::log("Preferred direction should not be empty!");
        preferredDirs.push_back(Direction::STILL);
    }
    Direction mostPreferredDir = preferredDirs[0];
    Position newPos = gameMap_->destination_position(currentPos, mostPreferredDir);
    shipsComingtoPos_[newPos].push_back(ship);
}

bool MovementMap::isFreeSpace(Position pos) {
    return shipsComingtoPos_[pos].size() == 0;
}

void MovementMap::makeShip() {
    shouldMakeShip_ = true;
}

/// Flush the outputs to Halite game engine
bool MovementMap::processOutputsAndEndTurn(Game& game, shared_ptr<Player> me) {
    // Resolve all conflicts
    resolveAllConflicts();

    // Get all the directions
    vector<Command> command_queue;
    //log::log("Real OUT");
    for (auto kv : shipDirectionQueue_) {
        Position shipPos = kv.first;
        shared_ptr<Ship> ship = gameMap_->at(shipPos)->ship;
        Direction dir = currentDirection(ship);
        command_queue.push_back(ship->move(dir));
        //Position nextPos = destinationPos(ship);
        //log::log("ship " + to_string(ship->id) + " position " + ship->position.toString() +
        //         " -> " + nextPos.toString());
    }

    // Spawn a ship
    if (shouldMakeShip_ && isFreeSpace(me->shipyard->position)) {
        command_queue.push_back(me->shipyard->spawn());
    }
    return game.end_turn(command_queue);
}

void MovementMap::logTurn(shared_ptr<Player> me) {
    //log::log("log turn");
    for (auto ship_iterator : me->ships) {
        shared_ptr<Ship> ship = ship_iterator.second;
        //Position nextPos = destinationPos(ship);
        //log::log("ship " + to_string(ship->id) + " position " + ship->position.toString() +
        //        " -> " + nextPos.toString());
    }
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
    //log::log("ship " + to_string(ship->id) + " redirection:" + ship->position.toString());
    while(hasConflict(ship) && currentDirection(ship) != Direction::STILL) {
        changeToNextDirection(ship);
        //log::log("new direction: " + to_string(currentDirection(ship)));
    }
    if(hasConflict(ship)) {
        allConflicts_.push(ship->position);
    }
}

void MovementMap::redirectShips(vector<shared_ptr<Ship>> ships) {
    for (shared_ptr<Ship> ship : ships) {
        redirectShip(ship);
    }
}

bool MovementMap::hasEnemyShip(Position& pos) {
    if (nPlayers_ == 2) {
        // TODO: we currently don't avoid enemies in 2 players.
        return false;
    }
    MapCell* cell = gameMap_->at(pos);
    if (cell->is_occupied() && cell->ship->owner != me_->id) {
        log::log("occupied " + pos.toString());
        return true;
    }
    else return false;
}

/// Does this block has conflict
bool MovementMap::hasConflict(Position& pos) {
    return (shipsComingtoPos_[pos].size() >= 2) or ((shipsComingtoPos_[pos].size() >= 1) and hasEnemyShip(pos));
}

bool MovementMap::hasConflict(shared_ptr<Ship> ship) {
    Position targetDir = destinationPos(ship);
    return hasConflict(targetDir);
}

void MovementMap::iterateAndResolveConflicts() {
    while(!allConflicts_.empty()) {
        Position conflictMiddlePos = allConflicts_.front();
        allConflicts_.pop();
        log::log("conflict: " + conflictMiddlePos.toString());
        resolveConflict(conflictMiddlePos);
    }
}

void MovementMap::resolveConflict(Position middlePos) {
    log::log("resolve: " + middlePos.toString());
    MapCell* middleCell = gameMap_->at(middlePos);
    // running into an enemy
    if (middleCell->is_occupied() && middleCell->ship->owner != me_->id) {
        log::log("detected");
        vector<shared_ptr<Ship>> shipsToRedirect = shipsComingtoPos_[middlePos];
        redirectShips(shipsToRedirect);
    }
    // O -> X <- O
    // If the middle position is empty, we allow the
    // ship with the greatest halite to go to the destination,
    // but don't allow other ships.
    else if(!(middleCell->is_occupied() && middleCell->ship->owner == me_->id) ) {
        // get all the ships pointing here
        vector<shared_ptr<Ship>> shipsToRedirect = shipsComingtoPos_[middlePos];
        auto maxShipIndex = max_element(shipsToRedirect.begin(), shipsToRedirect.end(),
                                        shipHasLessHalite);
        shared_ptr<Ship> maxShip = *maxShipIndex;
        shipsToRedirect.erase(maxShipIndex);
        
        redirectShips(shipsToRedirect);
    }
    // O -> O <- O
    // If the middle position is not empty we allow the middle ship
    // to choose what to do, that is, we direct all other ships that the middle
    // ship is not pointing to.
    // If the middle ship chooses to stay still, then all other ships will have to
    // redirect.
    else {
        // if it is occupied, then it has a ship.
        // Case : it is not out ship
        shared_ptr<Ship> middleShip = gameMap_->at(middlePos)->ship;
        vector<shared_ptr<Ship>> shipsToRedirect = shipsComingtoPos_[middlePos];
        if (middleShip->owner != me_->id) {
            redirectShips(shipsToRedirect);
        }
        if(currentDirection(middleShip) == Direction::STILL) {
            redirectShips(shipsToRedirect);
        }
        else {
            // looking at where the current ship is pointing to
            Direction middleDir = currentDirection(middleShip);
            // the ship at the position can move to the middle, if it wants.
            // that is, it does not have to redirect if it is in the list.
            Position safeShipPos = gameMap_->destination_position(middlePos, middleDir);
            for (shared_ptr<Ship> shipToRedirect : shipsToRedirect) {
                if (shipToRedirect->position != safeShipPos) {
                    redirectShip(shipToRedirect);
                }
            }
        }
    }
    
}

/// Initialize allConflicts_ and call resolve all
void MovementMap::resolveAllConflicts() {
    // Loop through the shipsComingtoPos_, obtain all conflicts
    // and keep it in a stack.
    for (auto kv : shipsComingtoPos_) {
        Position pos = kv.first;
        //log::log("conflict check at " + pos.toString());
        if (hasConflict(pos)) {
            allConflicts_.push(pos);
        }
    }
    iterateAndResolveConflicts();
}