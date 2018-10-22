#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"
#include "my_helpers/movement_map.hpp"
#include "my_helpers/navigator.hpp"
#include "my_helpers/tunables.hpp"

#include <random>
#include <ctime>
#include <queue>
#include <stack>
#include <algorithm>
#include <unordered_map>

using namespace std;
using namespace hlt;

enum class ShipStatus {
    EXPLORE_AND_COLLECT, RETURN
};

void logShipStatus(shared_ptr<Ship> ship, ShipStatus status) {
    if (status == ShipStatus::EXPLORE_AND_COLLECT) {
        log::log("Ship " + ship->position.toString() + " EXPLORE AND COLLECT");
    }
    if (status == ShipStatus::RETURN) {
        log::log("Ship " + ship->position.toString() + " RETURN");
    }
}

void adjustState(shared_ptr<Ship> ship, shared_ptr<Player> me,
                 unordered_map<EntityId, ShipStatus>& shipStatus) {
    // newly created ship
    if (!shipStatus.count(ship->id)) {
        shipStatus[ship->id] = ShipStatus::EXPLORE_AND_COLLECT;
    }
    // ship that has just reached the shipyard
    else if (ship->position == me->shipyard->position) {
        shipStatus[ship->id] = ShipStatus::EXPLORE_AND_COLLECT;
    }
    else if (ship->halite >= Tunables::SHIP_CAPACITY) {
        shipStatus[ship->id] = ShipStatus::RETURN;
    }

    logShipStatus(ship, shipStatus[ship->id]);
}

/// Process one turn
/// You can take at most 2 seconds per turn.
int gameTurn(mt19937 &rng, Game &game, unordered_map<EntityId, ShipStatus>& shipStatus) {

    // get input data from game engine
    game.update_frame();
    shared_ptr<Player> me = game.me;
    shared_ptr<GameMap>& game_map = game.game_map;
    // end get input data from game engine

    MovementMap movementMap = MovementMap(game_map, me);
    Navigator navigator = Navigator(game_map, me);

    for (const auto& ship_iterator : me->ships) {
        shared_ptr<Ship> ship = ship_iterator.second;

        adjustState(ship, me, shipStatus);
        
        vector<Direction> nextDirs;

        if (shipStatus[ship->id] == ShipStatus::EXPLORE_AND_COLLECT) {
            if (game_map->at(ship->position)->halite < Tunables::PICKUP_THRESHOLD) {
                nextDirs = navigator.explore(ship);

                vector<Direction> shuffledDirs = vector<Direction>(ALL_CARDINALS.begin(), ALL_CARDINALS.end());
                shuffle(shuffledDirs.begin(), shuffledDirs.end(), rng);

                nextDirs.insert(nextDirs.end(), shuffledDirs.begin(), shuffledDirs.end());
                log::log("ATTEMPT TO MOVE");
            }
            else {
                nextDirs = { Direction::STILL };
            }
        }
        else if (shipStatus[ship->id] == ShipStatus::RETURN) {
            if (game_map->at(ship->position)->halite > Tunables::PICKUP_RETURN_THRESHOLD &&
                !ship->is_full()) {
                log::log(to_string(ship->is_full()) + to_string(constants::MAX_HALITE));
                nextDirs = { Direction::STILL };
            }
            else {
                nextDirs = navigator.dropoffHalite(ship);
            }
        }
        movementMap.addIntent(ship, nextDirs);

    }

    if (game.turn_number <= constants::MAX_TURNS - Tunables::NO_PRODUCTION_TURN_COUNT &&
        me->halite >= constants::SHIP_COST) {
        movementMap.makeShip();
    }

    movementMap.logTurn(me);
    bool result = movementMap.processOutputsAndEndTurn(game, me);
    movementMap.logTurn(me);
    return result;
}

/// Initialize and run the game loop, passing in all the things that had been initializes
/// The initialization can take at most 30 seconds.
int main(int argc, char* argv[]) {

    // Initialize the random number generator
    unsigned int rng_seed;
    if (argc > 1) {
        rng_seed = static_cast<unsigned int>(stoul(argv[1]));
    }
    else {
        rng_seed = static_cast<unsigned int>(time(nullptr));
    }
    mt19937 rng(rng_seed);

    // Initialize an empty game object
    Game game;

    // ********** Initialize my own objects **************

    unordered_map<EntityId, ShipStatus> shipStatus;

    // ***************************************************

    // As soon as you call "ready" function below, the 2 second per turn timer will start.
    game.ready("MyCppBot");
    log::log("Successfully created bot! My Player ID is " + to_string(game.my_id) + ". Bot rng seed is " + to_string(rng_seed) + ".");

    for (;;) {
        // A turn failure
        if(gameTurn(rng, game, shipStatus) == false) {
            log::log("An error occured when ending the turn; most likely timeout.");
        }
    }
    return 0;
}
