#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"

#include <random>
#include <ctime>
#include <queue>
#include <stack>
#include <algorithm>

using namespace std;
using namespace hlt;

/// Process one turn
/// You can take at most 2 seconds per turn.
int gameTurn(mt19937 &rng, Game &game) {
    game.update_frame();
    shared_ptr<Player> me = game.me;
    unique_ptr<GameMap>& game_map = game.game_map;

    vector<Command> command_queue;

    for (const auto& ship_iterator : me->ships) {
        shared_ptr<Ship> ship = ship_iterator.second;
        if (game_map->can_move(ship) &&
                 (game_map->at(ship)->halite < constants::MAX_HALITE / 10 || ship->is_full())) {
            Direction random_direction = ALL_CARDINALS[rng() % 4];
            command_queue.push_back(ship->move(random_direction));
        }
        else {
            command_queue.push_back(ship->stay_still());
        }
    }

    if (game.turn_number <= 200 &&
        me->halite >= constants::SHIP_COST &&
        !game_map->at(me->shipyard)->is_occupied()) {
        command_queue.push_back(me->shipyard->spawn());
    }

    bool result =  game.end_turn(command_queue);
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

    
    
    // As soon as you call "ready" function below, the 2 second per turn timer will start.
    game.ready("MyCppBot");
    log::log("Successfully created bot! My Player ID is " + to_string(game.my_id) + ". Bot rng seed is " + to_string(rng_seed) + ".");

    for (;;) {
        // A turn failure
        if(gameTurn(rng, game) == false) {
            log::log("An error occured when ending the turn; most likely timeout.");
        }
    }
    return 0;
}
