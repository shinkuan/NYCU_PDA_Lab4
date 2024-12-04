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
#include "fiboqueue.h"
#include "common.h"


enum class Metal {
    M1,
    M2
};

class GCell;
struct Route {
    std::vector<GCell*> route;
    int idx;
    double cost;
};

class GCell {
public:
    GCell() {};
    ~GCell() {};

    Point<int> lowerLeft;               // Real coordinate of lower left corner
    double costM1;                      // Cost of the cell in metal 1
    double costM2;                      // Cost of the cell in metal 2
    double gammaM1;                     // Gamma * metal 1
    double gammaM2;                     // Gamma * metal 2
    double M1M2ViaCost;                 // Cost of via + (gammaM1 + gammaM2) / 2
    unsigned int leftEdgeCapacity;      // Capacity of left edge
    unsigned int bottomEdgeCapacity;    // Capacity of bottom edge
    unsigned int leftEdgeCount   = 0;   // Count of left edge
    unsigned int bottomEdgeCount = 0;   // Count of bottom edge

    bool isLeftEdgeFull = false;        // Is left edge full
    bool isBottomEdgeFull = false;      // Is bottom edge full

    // Equvalent to:
    // ```
    // const auto cmp = [](GCell* a, GCell* b) {
    //     return a->gScore < b->gScore;
    // };
    // decltype(cmp)
    // ```
    class GCellCmp {
    public:
        bool operator()(GCell* a, GCell* b) const {
            return a->gScore < b->gScore;
        }
    };

    FibQueue<GCell*, GCellCmp>::Node* fibNode = nullptr;

    GCell* left;                        // Pointer to left cell
    GCell* bottom;                      // Pointer to bottom cell
    GCell* right;                       // Pointer to right cell
    GCell* top;                         // Pointer to top cell

    GCell* parent;         // parent = parent cell
    double gScore;         // gScore = cost of the cheapest path from start to current cell

    enum class FromDirection {
        ORIGIN,
        LEFT,
        BOTTOM,
        RIGHT,
        TOP
    };
    FromDirection fromDirection; // fromDirection = from direction of parent cell

    void addRouteLeft() {
        leftEdgeCount++;
        isLeftEdgeFull = leftEdgeCount >= leftEdgeCapacity;
    }
    void addRouteBottom() {
        bottomEdgeCount++;
        isBottomEdgeFull = bottomEdgeCount >= bottomEdgeCapacity;
    }
};



#endif // _GCELL_H_