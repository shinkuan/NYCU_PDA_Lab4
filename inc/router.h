//############################################################################
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   `Router` Class Implementation Header File
//   
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   File Name   : router.h
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
//   #include "router.h"
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//   License: 
//
//############################################################################

#ifndef _ROUTER_H_
#define _ROUTER_H_

#include <vector>
#include <string>
#include "common.h"
#include "gcell.h"
#include "chip.h"

class Router {
public:
    Router();
    ~Router();

    void loadGridMap(const std::string& filename);
    void loadGCells(const std::string& filename);
    void loadCost(const std::string& filename);

private:
    Point<int> routingAreaLowerLeft;         // Real coordinate of lower left corner of routing area
    Size<int>  routingAreaSize;              // Size of routing area
    Size<int>  gcellSize;                    // Size of gcell
    Chip chip1;                              // Chip 1
    Chip chip2;                              // Chip 2
    std::vector<std::vector<GCell*>> gcells; // GCells in routing area

};


#endif // _ROUTER_H_