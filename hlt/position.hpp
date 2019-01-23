#pragma once

#include "types.hpp"
#include "direction.hpp"

#include <iostream>

namespace hlt {
    struct Position {
        int x;
        int y;

        Position(int x, int y) : x(x), y(y) {}

        bool operator==(const Position& other) const { return x == other.x && y == other.y; }
        bool operator!=(const Position& other) const { return x != other.x || y != other.y; }
        Position operator+(const Position& other) const {
            return Position(x + other.x, y + other.y);
        }

        Position directional_offset(Direction d) const {
            auto dx = 0;
            auto dy = 0;
            switch (d) {
                case Direction::NORTH:
                    dy = -1;
                    break;
                case Direction::SOUTH:
                    dy = 1;
                    break;
                case Direction::EAST:
                    dx = 1;
                    break;
                case Direction::WEST:
                    dx = -1;
                    break;
                case Direction::STILL:
                    // No move
                    break;
                default:
                    log::log(std::string("Error: invert_direction: unknown direction ") + static_cast<char>(d));
                    exit(1);
            }
            return Position{x + dx, y + dy};
        }

        std::array<Position, 4> get_surrounding_cardinals() {
            return {{
                directional_offset(Direction::NORTH), directional_offset(Direction::SOUTH),
                directional_offset(Direction::EAST), directional_offset(Direction::WEST)
            }};
        }

        std::string toString() {
            return std::to_string(x) + " " + std::to_string(y);
        }
    };

    static std::ostream& operator<<(std::ostream& out, const Position& position) {
        out << position.x << ' ' << position.y;
        return out;
    }
    static std::istream& operator>>(std::istream& in, Position& position) {
        in >> position.x >> position.y;
        return in;
    }

    static bool operator<(Position a, Position b) {
        return (a.x < b.x) or (a.x == b.x && a.y < b.y);
    }
}

namespace std {
    template <>
    struct hash<hlt::Position> {
        std::size_t operator()(const hlt::Position& position) const {
            return ((position.x+position.y) * (position.x+position.y+1) / 2) + position.y;
        }
    };
}
