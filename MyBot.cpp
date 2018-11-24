#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"
#include "my_helpers/movement_map.hpp"
#include "my_helpers/navigator.hpp"
#include "my_helpers/tunables.hpp"
#include "my_helpers/shipStatus.hpp"
#include "my_helpers/miscs.hpp"

#include <random>
#include <ctime>
#include <queue>
#include <stack>
#include <algorithm>
#include <unordered_map>
#include <string>

using namespace std;
using namespace hlt;

int calculateCurrentShipCapacity(Game& game) {
    Tunables tunables;
    double turnRatio = (double)game.turn_number / constants::MAX_TURNS;
    int start = tunables.lookUpTunable("shipCapStart");
    int end = tunables.lookUpTunable("shipCapEnd");
    int diff = start - end;
    int shipCapacity = start - int(turnRatio * diff);
    return shipCapacity;
}

void logShipStatus(shared_ptr<Ship> ship, ShipStatus status) {
    if (status == ShipStatus::NEW) {
        log::log("Ship " + ship->position.toString() + " NEW");
    }
    else if (status == ShipStatus::EXPLORE) {
        log::log("Ship " + ship->position.toString() + " EXPLORE");
    }
    else if (status == ShipStatus::RETURN) {
        log::log("Ship " + ship->position.toString() + " RETURN");
    }
    else {
        log::log("Ship " + ship->position.toString() + " COLLECT");
    }
}

void adjustState(shared_ptr<Ship> ship, shared_ptr<Player> me, Game &game, shared_ptr<GameMap>& game_map,
                 unordered_map<EntityId, ShipStatus>& shipStatus, Navigator &navigator) {
    // newly created ship
    log::log(std::to_string(navigator.getPickUpThreshold()));

    if (!shipStatus.count(ship->id)) {
        shipStatus[ship->id] = ShipStatus::NEW;
    }

    if (ship->position == me->shipyard->position) {
        shipStatus[ship->id] = ShipStatus::NEW;
    }

    if (shipStatus[ship->id] == ShipStatus::NEW) {
        shipStatus[ship->id] = ShipStatus::EXPLORE;
    }

    if (shipStatus[ship->id] == ShipStatus::EXPLORE) {
        if (game_map->at(ship->position)->halite >= navigator.getPickUpThreshold() and
             not ship->is_full()) {
            shipStatus[ship->id] = ShipStatus::COLLECT;
        }
        else if (ship->halite >= calculateCurrentShipCapacity(game)) {
            shipStatus[ship->id] = ShipStatus::RETURN;
        }
    }
    if (shipStatus[ship->id] == ShipStatus::COLLECT) {
        if (game_map->at(ship->position)->halite >= navigator.getPickUpThreshold() and
             not ship->is_full()) {
            shipStatus[ship->id] = ShipStatus::COLLECT;
        }
        else if (ship->halite >= calculateCurrentShipCapacity(game)) {
            shipStatus[ship->id] = ShipStatus::RETURN;
        }
        else {
            shipStatus[ship->id] = ShipStatus::EXPLORE;
        }
    }
    if (shipStatus[ship->id] == ShipStatus::RETURN) {
        if ((game_map->at(ship->position)->halite >= navigator.getPickUpThreshold()) and
             not (ship->halite > 900)) {
            shipStatus[ship->id] = ShipStatus::COLLECT;
        }
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
    Navigator navigator = Navigator(game_map, me, shipStatus, rng);

    for (const auto& ship_iterator : me->ships) {
        shared_ptr<Ship> ship = ship_iterator.second;
        adjustState(ship, me, game, game_map, shipStatus, navigator);
    }

    for (const auto& ship_iterator : me->ships) {
        shared_ptr<Ship> ship = ship_iterator.second;
        vector<Direction> nextDirs;

        if (shipStatus[ship->id] == ShipStatus::NEW) {
            nextDirs = navigator.newShip(ship);
        }
        else if (shipStatus[ship->id] == ShipStatus::EXPLORE) {
            nextDirs = navigator.explore(ship);
        }
        else if (shipStatus[ship->id] == ShipStatus::COLLECT) {
            nextDirs = navigator.collect(ship);
        }
        else if (shipStatus[ship->id] == ShipStatus::RETURN) {
            nextDirs = navigator.dropoffHalite(ship);
        }
        movementMap.addIntent(ship, nextDirs);

    }

    Tunables tunables;
    if (game.turn_number <= constants::MAX_TURNS - tunables.lookUpTunable("noProdTurn") &&
        me->halite >= constants::SHIP_COST) {
        movementMap.makeShip();
    }

    movementMap.logTurn(me);
    bool result = movementMap.processOutputsAndEndTurn(game, me);
    movementMap.logTurn(me);
    return result;
}

int findHaliteAbundanceKey(Game game) {
    int averageHalite = findAverageHalite(game.game_map);
    if (averageHalite < 116) {
        return 0;
    }
    else if (averageHalite < 194) {
        return 1;
    }
    else {
        return 2;
    }
}

/// Initialize and run the game loop, passing in all the things that had been initializes
/// The initialization can take at most 30 seconds.
int main(int argc, char* argv[]) {

    string s = argv[0];
    string pathToFolder = s.substr(0, s.find_last_of("\\/"));

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
    int mapWidth = game.game_map->width;
    int nPlayers = game.players.size();
    Tunables(pathToFolder, nPlayers, mapWidth, findHaliteAbundanceKey(game));

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
