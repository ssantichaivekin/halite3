#pragma once

#include "hlt/types.hpp"
#include "hlt/position.hpp"
#include "hlt/ship.hpp"
#include "hlt/dropoff.hpp"
#include "hlt/game.hpp"
#include "hlt/constants.hpp"

#include <queue>
#include <stack>
#include <algorithm>

using namespace std;
using namespace hlt;

/// Class movement map keeps track of each bot's immediate movement intent
/// It contains the logic of safety navigation and flushing the (safe) movements
/// to the output.
class MovementMap {
public:
    /// Create a movement map that keeps track of immediate movement and safety.
    /// 
    MovementMap(shared_ptr<GameMap>& gameMap, shared_ptr<Player>& me);

    /// Tell the map that the ship is intending to move in the following direction(s)
    /// Adding a direction here implies that the second direction is 
    /// almost as good as the first one.
    void addIntent(shared_ptr<Ship> ship, vector<Direction> preferredDirs, bool ignoreOpponentFlag = false);

    /// Only call after resolve all conflicts()
    bool isFreeSpace(Position pos);

    /// Create an intention to make ship
    void makeShip();

    /// Flush the outputs to Halite game engine
    bool processOutputsAndEndTurn(Game& game, shared_ptr<Player> me);

    void logTurn(shared_ptr<Player> me);

private:
    /// The direction a ship is intending to go
    Direction currentDirection(shared_ptr<Ship> ship);

    /// The immediate destination (next block) the ship is intending to
    /// go.
    Position destinationPos(shared_ptr<Ship> ship);

    /// Does ship1 has less halite than ship2?
    static bool shipHasLessHalite(const shared_ptr<Ship>& ship1, const shared_ptr<Ship>& ship2);

    /// Change the direction of the ship to the next possible move.
    void changeToNextDirection(shared_ptr<Ship> ship);

    /// Redirect a ship until it has no conflict or has stopped.
    void redirectShip(shared_ptr<Ship> ship);

    void redirectShips(vector<shared_ptr<Ship>> ships);

    /// Does this block has conflict?
    bool hasConflict(Position& pos);

    /// Does this ship has conflict?
    bool hasConflict(shared_ptr<Ship> ship);

    /// resolve all conflicts
    void iterateAndResolveConflicts();

    /// resolve a conflict at the position middlePos
    void resolveConflict(Position middlePos);

    /// Resolve the conflicts at all positions
    /// by changing the intent of ships surrounding those positions.
    void resolveAllConflicts();
    
    shared_ptr<GameMap> gameMap_;
    shared_ptr<Player> me_;
    unordered_map<Position, vector<shared_ptr<Ship>>> shipsComingtoPos_;
    unordered_map<Position, queue<Direction>> shipDirectionQueue_;
    queue<Position> allConflicts_;

    unordered_map<Position, bool> shipIgnoresOpponent_;
    bool shouldMakeShip_;
};
