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
    double costM1;                      // Cost of the cell in metal 1
    double costM2;                      // Cost of the cell in metal 2
    double gammaM1;                    // Gamma * metal 1
    double gammaM2;                    // Gamma * metal 2
    unsigned int leftEdgeCapacity;      // Capacity of left edge
    unsigned int bottomEdgeCapacity;    // Capacity of bottom edge
    unsigned int leftEdgeCount   = 0;   // Count of left edge
    unsigned int bottomEdgeCount = 0;   // Count of bottom edge

    GCell* left;                        // Pointer to left cell
    GCell* bottom;                      // Pointer to bottom cell
    GCell* right;                       // Pointer to right cell
    GCell* top;                         // Pointer to top cell

    std::vector<Route*> routesLeft;     // Routes passed left edge
    std::vector<Route*> routesBottom;   // Routes passed bottom edge

    std::vector<GCell*> parent;         // parent[process id] = parent cell
    std::vector<double> fScore;         // fScore[process id] = gScore[process id] + hScore[process id]
    std::vector<double> gScore;         // gScore[process id] = cost of the cheapest path from start to current cell
    std::vector<double> hScore;         // hScore[process id] = estimated cost from current cell to target

    enum class FromDirection {
        ORIGIN,
        LEFT,
        BOTTOM,
        RIGHT,
        TOP
    };
    std::vector<FromDirection> fromDirection; // fromDirection[process id] = from direction of parent cell

    std::mutex routesLeftMutex;
    std::mutex routesBottomMutex;

    void addRouteLeft(Route* route) {
        std::lock_guard<std::mutex> lock(routesLeftMutex);
        routesLeft.push_back(route);
        leftEdgeCount++;
    }
    void addRouteBottom(Route* route) {
        std::lock_guard<std::mutex> lock(routesBottomMutex);
        routesBottom.push_back(route);
        bottomEdgeCount++;
    }
};


#endif // _GCELL_H_