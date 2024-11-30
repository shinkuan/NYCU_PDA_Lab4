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
#include "common.h"
#include "gcell.h"
#include "chip.h"


struct Net {
    int idx;                            // Index of net
    int WL;                             // Wirelength
    int overflow;                       // Overflow
    double cellCost;                    // Cell cost
    int viaCount;                       // Via count
    double totalCost;                   // Total cost
    std::vector<GCell*> gcells;         // GCells in net
};

class Evaluator {
public:
    Evaluator();
    ~Evaluator();

    void loadGridMap(const std::string& filename);
    void loadGCells(const std::string& filename);
    void loadCost(const std::string& filename);
    void loadLG(const std::string& filename);
    void printCosts();

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

    std::vector<Net*> nets;                   // Nets
};


#endif // _ROUTER_H_