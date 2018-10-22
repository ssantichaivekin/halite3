#pragma once

class Tunables {
public:
    // max ship capacity is 1000, but we don't need to use all the 1000.
    // min: 0, max: 1000
    static const int SHIP_CAPACITY = 650;
    
    // We pick up halite when the halite on block when it is lower than threshold.
    // min: 0, max: 1000
    static const int PICKUP_THRESHOLD = 30;

    // Even if the ship is returning, if it sees a lot of halite,
    // it might pick it up.
    // min: 0, max: 1000
    static const int PICKUP_RETURN_THRESHOLD = 200;

    // Stop producing when we have some about of time left
    // min: 0, max: 400
    static const int NO_PRODUCTION_TURN_COUNT = 230;

};