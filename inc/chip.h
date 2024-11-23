//############################################################################
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   `Chip` Class Implementation Header File
//   
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   File Name   : chip.h
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
//   #include "chip.h"
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   License: 
//
//############################################################################

#ifndef _CHIP_H_
#define _CHIP_H_

#include <vector>
#include "common.h"
#include "gcell.h"


struct Bump {
    int idx;                            // Index of bump
    Point<int> position;                // Position of bump
    GCell* gcell;                       // GCell of bump
};

class Chip {
public:
    Chip();
    ~Chip();

    Point<int> lowerLeft;               // Real coordinate of lower left corner
    Size<int>  size;                    // Size of chip
    std::vector<Bump> bumps;            // Bumps on chip

};


#endif // _CHIP_H_