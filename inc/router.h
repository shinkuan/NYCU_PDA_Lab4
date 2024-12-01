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
#include <set>
#include <unordered_set>
#include <random>
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
    void dumpRoutes(const std::string& filename);
    Route* router(GCell* source, GCell* target);

    double solve();

    void setSeed(unsigned seed) {
        rng.seed(seed);
    }

private:
    Point<int> routingAreaLowerLeft;         // Real coordinate of lower left corner of routing area
    Size<int>  routingAreaSize;              // Size of routing area
    Size<int>  gcellSize;                    // Size of gcell
    Chip chip1;                              // Chip 1
    Chip chip2;                              // Chip 2
    std::vector<std::vector<GCell*>> gcells; // GCells in routing area [y][x]

    double alpha;                            // Alpha (WL cost)
    double beta;                             // Beta (Overflow cost)
    double gamma;                            // Gamma (Cell cost)
    double delta;                            // Delta (Via cost)
    double viaCost;                          // Via cost

    double maxCellCost;                      // Maximum cell cost
    double medianCellCost;                   // Median cell cost

    double alphaGcellSizeX;                  // Alpha * gcellSize.x
    double alphaGcellSizeY;                  // Alpha * gcellSize.y
    double betaHalfMaxCellCost;              // Beta * 0.5 * maxCellCost
    double deltaViaCost;                     // Delta * viaCost

    std::vector<Route*> routes;              // Routes

    // Random
    std::mt19937 rng;
};


#endif // _ROUTER_H_