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
#include "common.h"


using Route = std::vector<GCell*>;

class GCell {
public:
    GCell();
    ~GCell();

    Point<int> lowerLeft;               // Real coordinate of lower left corner
    unsigned int cost;                  // Cost of the cell
    unsigned int leftEdgeCapacity;      // Capacity of left edge
    unsigned int bottomEdgeCapacity;    // Capacity of bottom edge
    unsigned int leftEdgeCount   = 0;   // Count of left edge
    unsigned int bottomEdgeCount = 0;   // Count of bottom edge

    GCell* left;                        // Pointer to left cell
    GCell* bottom;                      // Pointer to bottom cell
    GCell* right;                       // Pointer to right cell
    GCell* top;                         // Pointer to top cell

    std::vector<Route> routesLeft;      // Routes passed left edge
    std::vector<Route> routesBottom;    // Routes passed bottom edge

};



#endif // _GCELL_H_