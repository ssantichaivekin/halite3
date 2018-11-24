#pragma once

// keeps how to calculate specified values
#include <fstream>
#include <tuple>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cassert>


class Tunables {
private:
    
    // Modify this as we have more independent variables
    // say, halite potential.
    static const int PLAYER_NUMS = 2;
    static const int MAP_SIZES = 5;
    static const int HALITE_AS = 3;

    static std::vector<int> playerNums;
    static std::vector<int> mapSizes;
    static std::vector<int> haliteAs;

    static int playerNumKey_;
    static int mapSizeKey_;
    static int haltieAbundanceKey_;

    static std::unordered_map<std::string, double> tunableDict_[PLAYER_NUMS][MAP_SIZES][HALITE_AS];

public:
    // max ship capacity is 1000, but we don't need to use all the 1000.
    // min: 0, max: 1000
    static const int SHIP_CAPACITY_START = 850;
    static const int SHIP_CAPACITY_END = 500;

    // We pick up halite when the halite on block when it is lower than threshold.
    // min: 0, max: 1000
    static const int PICKUP_THRESHOLD_START = 70;
    
    // We pick up halite when the halite on block when it is lower than threshold.
    // min: 0, max: 1000
    static const int PICKUP_THRESHOLD_END = 30;

    // Even if the ship is returning, if it sees a lot of halite,
    // it might pick it up.
    // min: 0, max: 1000
    static const int PICKUP_RETURN_THRESHOLD = 200;

    // Stop producing when we have some about of time left
    // min: 0, max: 400
    static const int NO_PRODUCTION_TURN_COUNT = 220;

    // How far wil a ship look at its surroundings
    static const int SHIP_LOOKS_AHEAD = 4;

    Tunables() {}
    Tunables(std::string &pathToFolder, int playerNum, int mapSize, int haliteAbundance);

    void readAllCsv(std::string &pathToFolder);

    void readCsvToDict(std::string filename, std::unordered_map<std::string, double> &dict);

    double lookUpTunable(std::string name);
};