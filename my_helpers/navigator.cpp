#include "navigator.hpp"
#include "tunables.hpp"
#include "shipStatus.hpp"

#include <algorithm>
#include <cassert>

using namespace std;
using namespace hlt;

Navigator::LessFavorablePositionCmp::LessFavorablePositionCmp(
    shared_ptr<GameMap> gameMap, Position centerPos, int haliteNavigateThreshold) :
    gameMap_(gameMap), centerPos_(centerPos), haliteNavigateThreshold_(haliteNavigateThreshold) {}

int Navigator::LessFavorablePositionCmp::evaluatePosition(Position pos) const {
    return max(0, gameMap_->at(pos)->halite - haliteNavigateThreshold_);
}

bool Navigator::LessFavorablePositionCmp::noValidPosition(vector<Position> posList) const {
    for (Position pos : posList) {
        if (evaluatePosition(pos) != 0)
            return false;
    }
    return true;
}

vector<Position> Navigator::getSurroundingPositions(Position middlePos, int lookAhead) {
    vector<Position> posList;
    for(int xDiff = -lookAhead; xDiff <= lookAhead; xDiff++) {
        for(int yDiff = -lookAhead; yDiff <= lookAhead; yDiff++) {
            Position newPos = middlePos + Position(xDiff, yDiff);
            gameMap_->normalize(newPos);
            posList.push_back(newPos);
        }
    }
    return posList;
}

Navigator::Navigator(shared_ptr<GameMap>& gameMap, shared_ptr<Player>& me, 
                     unordered_map<EntityId, ShipStatus>& shipStatus, mt19937& rng) :
    gameMap_(gameMap), me_(me), shipStatus_(shipStatus), rng_(rng) {
        haliteNavigateThreshold_ = calculateNavigateThreshold();
    }

int Navigator::calculateNavigateThreshold() {
    Position shipyard = me_->shipyard->position;
    int size = gameMap_->width;
    int maxHalite = 10;
    int maxVal = 10;
    for(int x = 0; x < size; x++) {
        for(int y = 0; y < size; y++) {
            Position pos = Position(x, y);
            int dist = gameMap_->calculate_distance(shipyard, pos);
            int halite = gameMap_->at(pos)->halite;
            int posVal = (halite / (dist + 5));
            if (posVal > maxVal) {
                maxHalite = halite;
            }
        }
    }
    int maxThreshold = maxHalite / 2;
    return maxThreshold;
}

vector<Direction> Navigator::explore(shared_ptr<Ship> ship) {

    Position middlePos = ship->position;
    LessFavorablePositionCmp posFavCmp = LessFavorablePositionCmp(gameMap_, middlePos, haliteNavigateThreshold_);
    DirectionHasGreaterHaliteCmp directionHasGreaterHaliteCmp = DirectionHasGreaterHaliteCmp(gameMap_, ship);

    int shipLookAhead = Tunables::SHIP_LOOKS_AHEAD;
    vector<Position> posList = getSurroundingPositions(middlePos, shipLookAhead);
    while (posFavCmp.noValidPosition(posList)) {
        shipLookAhead *= 2;
        posList = getSurroundingPositions(middlePos, shipLookAhead);
    }

    Position maxPos = *max_element(posList.begin(), posList.end(), posFavCmp);
    vector<Direction> targetDirs = gameMap_->get_unsafe_moves(ship->position, maxPos);
    sort(targetDirs.begin(), targetDirs.end(), directionHasGreaterHaliteCmp);

    // add some wiggle
    vector<Direction> wiggleDirs = wiggleDirectionsMostHalite(ship);
    targetDirs.push_back(wiggleDirs[0]);
    targetDirs.push_back(wiggleDirs[1]);
    return targetDirs;
}

vector<Direction> Navigator::newShip(shared_ptr<Ship> ship) {
    vector<Direction> nextDirs = explore(ship);
    vector<Direction> wiggleDirs = wiggleDirectionsMostHalite(ship);
    nextDirs.insert(nextDirs.end(), wiggleDirs.begin(), wiggleDirs.end());
    return nextDirs;
}

vector<Direction> Navigator::dropoffHalite(shared_ptr<Ship> ship) {
    vector<Direction> nextDirs = gameMap_->get_unsafe_moves(ship->position, me_->shipyard->position);
    DirectionHasLessHaliteCmp directionHasLessHaliteCmp = DirectionHasLessHaliteCmp(gameMap_, ship);
    sort(nextDirs.begin(), nextDirs.end(), directionHasLessHaliteCmp);
    return nextDirs;
}

vector<Direction> Navigator::wiggleDirectionsMostHalite(shared_ptr<Ship> ship) {
    vector<Direction> shuffledDirs = vector<Direction>(ALL_CARDINALS.begin(), ALL_CARDINALS.end());
    shuffle(shuffledDirs.begin(), shuffledDirs.end(), rng_);

    DirectionHasGreaterHaliteCmp directionHasGreaterHaliteCmp = DirectionHasGreaterHaliteCmp(gameMap_, ship);
    sort(shuffledDirs.begin(), shuffledDirs.end(), directionHasGreaterHaliteCmp);
    return shuffledDirs;
}

/// Exploring ship nearby and potentially do not know what to do
bool Navigator::confusedExploringShipNearby(shared_ptr<Ship> ship) {
    for (Direction direction : ALL_CARDINALS) {
        Position nearPos = gameMap_->destination_position(ship->position, direction);
        if (gameMap_->at(nearPos)->is_occupied()) {
            shared_ptr<Ship> nearShip = gameMap_->at(nearPos)->ship;
            // if it is our ship
            if (nearShip->owner == me_->id) {
                assert(shipStatus_.count(nearShip->id));
                // if the nearby ship is either explore or new
                // and if its position has less halite than this ship's
                // position, then return true
                if ((shipStatus_[nearShip->id] == ShipStatus::EXPLORE or
                     shipStatus_[nearShip->id] == ShipStatus::NEW) and
                    gameMap_->at(ship->position)->halite > gameMap_->at(nearShip->position)->halite) {
                        return true;
                    }
            }
        }
    }
    return false;
}

vector<Direction> Navigator::collect(shared_ptr<Ship> ship) {
    // if there is another exploring ship close by, create a space for it.
    if (confusedExploringShipNearby(ship)) {
        vector<Direction> outDirs;
        vector<Direction> targetDirs = wiggleDirectionsMostHalite(ship);
        for (Direction direction : targetDirs) {
            Position targetPos = gameMap_->destination_position(ship->position, direction);
            if (gameMap_->at(targetPos)->halite > gameMap_->at(ship)->halite) {
                outDirs.push_back(direction);
            }
        }
        return outDirs;
    }
    return { Direction::STILL };
}