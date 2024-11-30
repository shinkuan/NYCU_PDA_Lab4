//############################################################################
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   `GCell` Class Implementation Header File
//   
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   File Name   : gcell.h
//   Release Version : V1.0
//   Description : 
//      
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   Key Features:
//   
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   Author          : shinkuan
//   Creation Date   : 2024-11-23
//   Last Modified   : 2024-11-23
//   Compiler        : g++/clang++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   Usage Example:
//   #include "gcell.h"
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   License: 
//
//############################################################################

#ifndef _GCELL_H_
#define _GCELL_H_

#include <vector>
#include <mutex>
#include "common.h"


enum class Metal {
    M1,
    M2
};

class GCell;
struct Route {
    std::vector<GCell*> route;
    int idx;
};

class GCell {
public:
    GCell() {};
    ~GCell() {};

    Point<int> lowerLeft;               // Real coordinate of lower left corner
    double cost;                        // Cost of the cell
    double gammaCost;                     // Gamma * metal 1
    unsigned int WSEdgeCapacity;        // West or South edge capacity
    unsigned int WSEdgeCount = 0;       // West or South edge count

    GCell* westSouth;                   // Pointer to west or south cell
    GCell* eastNorth;                   // Pointer to east or north cell
    GCell* dowsUp;                      // Pointer to down or up cell
    double costWS;                      // Cost of going west or south
    double costEN;                      // Cost of going east or north
    double costDU;                      // Cost of going down or up

    std::vector<Route*> routesWS;       // Routes passed west or south

    GCell* parent;         // parent = parent cell
    double gScore;         // gScore = cost of the cheapest path from start to current cell

    enum class FromDirection {
        ORIGIN,
        WEST_SOUTH,
        EAST_NORTH,
        DOWN_UP
    };
    FromDirection fromDirection; // fromDirection = from direction of parent cell

    void addRoute(Route* route) {
        routesWS.push_back(route);
        WSEdgeCount++;
    }
};


#endif // _GCELL_H_