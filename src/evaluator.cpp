#include <cfloat>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <set>
#include <queue>
#include <unordered_set>
#include "variadicTable.h"
#include "evaluator.h"
#include "logger.h"

Evaluator::Evaluator() {
}

Evaluator::~Evaluator() {
    for (auto& row : gcells) {
        for (auto& gcell : row) {
            delete gcell;
        }
    }
    for (auto& net : nets) {
        delete net;
    }
}

bool is_blank( const std::string & s ) {
  return s.find_first_not_of( " \f\n\r\t\v" ) == s.npos;
}

void Evaluator::loadGridMap(const std::string& filename) {
    // Load grid map
    LOG_INFO("Loading grid map from " + filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open file " + filename);
        return;
    }

    enum class State {
        LoadingCommand,
        LoadingRoutingArea,
        LoadingGCellSize,
        LoadingChip1,
        LoadingBump,
        LoadingChip2
    };

    bool loadingChip1 = false;
    State state = State::LoadingCommand;
    std::string line;
    while (std::getline(file, line)) {
        if (is_blank(line)) {
            if (state == State::LoadingBump) {
                state = State::LoadingCommand;
            }
            continue;
        }
        std::istringstream iss(line);
        switch (state) {
            case State::LoadingCommand: {
                std::string command;
                iss >> command;
                if (command == ".ra") {
                    state = State::LoadingRoutingArea;
                } else if (command == ".g") {
                    state = State::LoadingGCellSize;
                } else if (command == ".c") {
                    if (loadingChip1) {
                        state = State::LoadingChip2;
                        loadingChip1 = false;
                    } else {
                        state = State::LoadingChip1;
                        loadingChip1 = true;
                    }
                } else if (command == ".b") {
                    state = State::LoadingBump;
                } else {
                    LOG_ERROR("Unknown command " + command);
                }
                break;
            }
            case State::LoadingRoutingArea: {
                iss >> routingAreaLowerLeft.x >> routingAreaLowerLeft.y >> routingAreaSize.x >> routingAreaSize.y;
                state = State::LoadingCommand;
                LOG_TRACE("Routing area lower left: (" + std::to_string(routingAreaLowerLeft.x) + ", " + std::to_string(routingAreaLowerLeft.y) + ")");
                LOG_TRACE("Routing area size: (" + std::to_string(routingAreaSize.x) + ", " + std::to_string(routingAreaSize.y) + ")");
                break;
            }
            case State::LoadingGCellSize: {
                iss >> gcellSize.x >> gcellSize.y;
                state = State::LoadingCommand;
                LOG_TRACE("GCell size: (" + std::to_string(gcellSize.x) + ", " + std::to_string(gcellSize.y) + ")");
                break;
            }
            case State::LoadingChip1: {
                int x, y;
                iss >> x >> y >> chip1.size.x >> chip1.size.y;
                chip1.lowerLeft = {x + routingAreaLowerLeft.x, y + routingAreaLowerLeft.y};
                state = State::LoadingCommand;
                LOG_TRACE("Chip 1 lower left: (" + std::to_string(chip1.lowerLeft.x) + ", " + std::to_string(chip1.lowerLeft.y) + ")");
                LOG_TRACE("Chip 1 size: (" + std::to_string(chip1.size.x) + ", " + std::to_string(chip1.size.y) + ")");
                break;
            }
            case State::LoadingBump: {
                int bidx, bx, by;
                iss >> bidx >> bx >> by;
                if (loadingChip1) {
                    Bump bump = {bidx, {bx + chip1.lowerLeft.x, by + chip1.lowerLeft.y}, nullptr};
                    chip1.bumps.push_back(bump);
                    LOG_TRACE("Chip 1 bump (" + std::to_string(bump.position.x) + ", " + std::to_string(bump.position.y) + ")");
                } else {
                    Bump bump = {bidx, {bx + chip2.lowerLeft.x, by + chip2.lowerLeft.y}, nullptr};
                    chip2.bumps.push_back(bump);
                    LOG_TRACE("Chip 2 bump (" + std::to_string(bump.position.x) + ", " + std::to_string(bump.position.y) + ")");
                }
                break;
            }
            case State::LoadingChip2: {
                int x, y;
                iss >> x >> y >> chip2.size.x >> chip2.size.y;
                chip2.lowerLeft = {x + routingAreaLowerLeft.x, y + routingAreaLowerLeft.y};
                state = State::LoadingCommand;
                LOG_TRACE("Chip 2 lower left: (" + std::to_string(chip2.lowerLeft.x) + ", " + std::to_string(chip2.lowerLeft.y) + ")");
                LOG_TRACE("Chip 2 size: (" + std::to_string(chip2.size.x) + ", " + std::to_string(chip2.size.y) + ")");
                break;
            }
            default: break;
        }
    }
    file.close();

    gcells.resize(routingAreaSize.y / gcellSize.y);
    for (auto& row : gcells) {
        row.resize(routingAreaSize.x / gcellSize.x);
    }
    for (size_t y = 0; y < gcells.size(); y++) {
        for (size_t x = 0; x < gcells[y].size(); x++) {
            GCell* gcell = new GCell();
            gcell->parent = nullptr;
            gcell->fScore = DBL_MAX;
            gcell->gScore = DBL_MAX;
            gcell->hScore = DBL_MAX;
            gcell->fromDirection = GCell::FromDirection::ORIGIN;
            gcell->lowerLeft = {static_cast<int>(x) * gcellSize.x + routingAreaLowerLeft.x, static_cast<int>(y) * gcellSize.y + routingAreaLowerLeft.y};
            gcells[y][x] = gcell;
        }
    }
    for (size_t y = 0; y < gcells.size(); y++) {
        for (size_t x = 0; x < gcells[y].size(); x++) {
            GCell* gcell = gcells[y][x];
            if (x > 0) {
                gcell->left     = gcells[y][x - 1];
            } else {
                gcell->left     = nullptr;
            }
            if (y > 0) {
                gcell->bottom   = gcells[y - 1][x];
            } else {
                gcell->bottom   = nullptr;
            }
            if (x < gcells[y].size() - 1) {
                gcell->right    = gcells[y][x + 1];
            } else {
                gcell->right    = nullptr;
            }
            if (y < gcells.size() - 1) {
                gcell->top      = gcells[y + 1][x];
            } else {
                gcell->top      = nullptr;
            }
        }
    }

    // Sort bumps
    std::sort(chip1.bumps.begin(), chip1.bumps.end(), [](const Bump& a, const Bump& b) {
        return a.idx < b.idx;
    });
    std::sort(chip2.bumps.begin(), chip2.bumps.end(), [](const Bump& a, const Bump& b) {
        return a.idx < b.idx;
    });

    for (auto& bump : chip1.bumps) {
        int x = (bump.position.x - routingAreaLowerLeft.x) / gcellSize.x;
        int y = (bump.position.y - routingAreaLowerLeft.y) / gcellSize.y;
        LOG_TRACE("Chip 1 bump (" + std::to_string(bump.position.x) + ", " + std::to_string(bump.position.y) + ") -> GCell (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        bump.gcell = gcells[y][x];
    }
    for (auto& bump : chip2.bumps) {
        int x = (bump.position.x - routingAreaLowerLeft.x) / gcellSize.x;
        int y = (bump.position.y - routingAreaLowerLeft.y) / gcellSize.y;
        LOG_TRACE("Chip 2 bump (" + std::to_string(bump.position.x) + ", " + std::to_string(bump.position.y) + ") -> GCell (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        bump.gcell = gcells[y][x];
    }
}

void Evaluator::loadGCells(const std::string& filename) {
    // Load gcells
    LOG_INFO("Loading gcells from " + filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open file " + filename);
        return;
    }

    enum class State {
        LoadingCommand,
        LoadingGCell
    };

    int loadedGCellCount = 0;
    State state = State::LoadingCommand;
    std::string line;
    while (std::getline(file, line)) {
        if (is_blank(line)) continue;
        std::istringstream iss(line);
        switch (state) {
            case State::LoadingCommand: {
                std::string command;
                iss >> command;
                if (command == ".ec") {
                    state = State::LoadingGCell;
                } else {
                    LOG_ERROR("Unknown command " + command);
                }
                break;
            }
            case State::LoadingGCell: {
                int leftEdgeCapacity, bottomEdgeCapacity;
                iss >> leftEdgeCapacity >> bottomEdgeCapacity;
                GCell* gcell = gcells[loadedGCellCount / gcells[0].size()][loadedGCellCount % gcells[0].size()];
                gcell->leftEdgeCapacity = leftEdgeCapacity;
                gcell->bottomEdgeCapacity = bottomEdgeCapacity;
                loadedGCellCount++;
                state = State::LoadingGCell;
                break;
            }
            default: break;
        }
    }        
}

void Evaluator::loadCost(const std::string& filename) {
    // Load cost
    LOG_INFO("Loading cost from " + filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open file " + filename);
        return;
    }

    enum class State {
        LoadingCommand,
        LoadingViaCost,
        LoadingLayer
    };

    std::vector<double> costs(gcells.size() * gcells[0].size() * 2);
    maxCellCost = DBL_MIN;
    size_t currentRow = 0;
    int currentLayer = 0;
    State state = State::LoadingCommand;
    std::string line;
    while (std::getline(file, line)) {
        if (is_blank(line)) continue;
        std::istringstream iss(line);
        switch (state) {
            case State::LoadingCommand: {
                std::string command;
                iss >> command;
                if (command == ".l") {
                    state = State::LoadingLayer;
                } else if (command == ".v") {
                    state = State::LoadingViaCost;
                } else if (command == ".alpha") {
                    iss >> alpha;
                    state = State::LoadingCommand;
                    LOG_TRACE("Alpha: " + std::to_string(alpha));
                } else if (command == ".beta") {
                    iss >> beta;
                    state = State::LoadingCommand;
                    LOG_TRACE("Beta: " + std::to_string(beta));
                } else if (command == ".gamma") {
                    iss >> gamma;
                    state = State::LoadingCommand;
                    LOG_TRACE("Gamma: " + std::to_string(gamma));
                } else if (command == ".delta") {
                    iss >> delta;
                    state = State::LoadingCommand;
                    LOG_TRACE("Delta: " + std::to_string(delta));
                } else {
                    LOG_ERROR("Unknown command " + command);
                }
                break;
            }
            case State::LoadingViaCost: {
                iss >> viaCost;
                state = State::LoadingCommand;
                LOG_TRACE("Via cost: " + std::to_string(viaCost));
                break;
            }
            case State::LoadingLayer: {
                double cost;
                for (size_t x = 0; x < gcells[currentRow].size(); x++) {
                    iss >> cost;
                    if (cost != 0) costs.push_back(cost);
                    if (cost > maxCellCost) {
                        maxCellCost = cost;
                    }
                    if (currentLayer == 0) {
                        gcells[currentRow][x]->costM1 = cost;
                        gcells[currentRow][x]->gammaM1 = gamma * cost;
                    } else {
                        gcells[currentRow][x]->costM2 = cost;
                        gcells[currentRow][x]->gammaM2 = gamma * cost;
                    }
                }
                currentRow++;
                if (currentRow == gcells.size()) {
                    currentRow = 0;
                    currentLayer++;
                    state = State::LoadingCommand;
                } else {
                    state = State::LoadingLayer;
                }
                break;
            }
            default: break;
        }
    }
    file.close();

    // use nth_element to find median
    size_t medianIndex = costs.size() / 2;
    std::nth_element(costs.begin(), costs.begin() + medianIndex, costs.end());
    medianCellCost = costs[medianIndex];

    alphaGcellSizeX = alpha * gcellSize.x;
    alphaGcellSizeY = alpha * gcellSize.y;
    betaHalfMaxCellCost = beta * 0.5 * maxCellCost;
    deltaViaCost = delta * viaCost;
    for (auto& row : gcells) {
        for (auto& gcell : row) {
            gcell->M1M2ViaCost = deltaViaCost + (gcell->gammaM1 + gcell->gammaM2) / 2;
        }
    }
}

void Evaluator::loadLG(const std::string& filename) {
    // Load LG
    LOG_INFO("Loading LG from " + filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open file " + filename);
        return;
    }

    enum class State {
        LoadingNet,
        LoadingRoute
    };

    State state = State::LoadingNet;
    std::string line;
    while (std::getline(file, line)) {
        if (is_blank(line)) continue;
        std::istringstream iss(line);
        std::string netIdxStr;
        iss >> netIdxStr;
        int netIdx = std::stoi(netIdxStr.substr(1));
        Net* net = new Net();
        net->idx = netIdx;
        net->WL = 0;
        net->overflow = 0;
        net->cellCost = 0;
        net->viaCount = 0;
        net->totalCost = 0;
        net->gcells.clear();
        nets.push_back(net);
        bool checkStartPoint = true;
        Bump* startBump = nullptr;
        Bump* endBump = nullptr;
        for (auto& bump : chip1.bumps) {
            if (bump.idx == netIdx) {
                startBump = &bump;
                break;
            }
        }
        for (auto& bump : chip2.bumps) {
            if (bump.idx == netIdx) {
                endBump = &bump;
                break;
            }
        }
        GCell* startGCell = startBump->gcell;
        net->gcells.push_back(startGCell);
        bool passVia = false;
        bool lastM1 = true;
        while (std::getline(file, line)) {
            if (is_blank(line)) continue;
            std::istringstream iss(line);
            std::string command;
            iss >> command;
            if (command == "M1") {
                int x1, y1, x2, y2;
                iss >> x1 >> y1 >> x2 >> y2;
                int x1g = (x1 - routingAreaLowerLeft.x) / gcellSize.x;
                int y1g = (y1 - routingAreaLowerLeft.y) / gcellSize.y;
                int x2g = (x2 - routingAreaLowerLeft.x) / gcellSize.x;
                int y2g = (y2 - routingAreaLowerLeft.y) / gcellSize.y;
                if (checkStartPoint) {
                    if (startBump->position.x != x1 || startBump->position.y != y1) {
                        LOG_ERROR("[Net: " + std::to_string(netIdx) + "] Start point mismatch: (" + std::to_string(x1) + ", " + std::to_string(y1) + ")");
                    }
                    checkStartPoint = false;
                }
                if (x2 != x1) {
                    LOG_ERROR("[Net: " + std::to_string(netIdx) + "] M1 horizontal routing is forbidden!");
                }
                if (y2 == y1) {
                    LOG_ERROR("[Net: " + std::to_string(netIdx) + "] M1 zero-length routing is forbidden!");
                }
                if ((x1 - routingAreaLowerLeft.x) % gcellSize.x != 0 || 
                    (y1 - routingAreaLowerLeft.y) % gcellSize.y != 0 || 
                    (x2 - routingAreaLowerLeft.x) % gcellSize.x != 0 || 
                    (y2 - routingAreaLowerLeft.y) % gcellSize.y != 0) {
                    LOG_ERROR("[Net: " + std::to_string(netIdx) + "] M1 routing point is not on GCell lower left corner!");
                }
                net->WL += std::abs(y2 - y1);
                net->totalCost += std::abs(y2 - y1) * alpha;
                // net->gcells.push_back(gcells[y1g][x1g]);
                if (passVia) {
                    
                } else {
                    net->cellCost += gcells[y1g][x1g]->costM1;
                    net->totalCost += gcells[y1g][x1g]->gammaM1;
                }
                passVia = false;
                bool goUP = y2 > y1;
                if (goUP) {
                    for (int y = y1g + 1; y <= y2g; y++) {
                        net->gcells.push_back(gcells[y][x1g]);
                        net->cellCost += gcells[y][x1g]->costM1;
                        net->totalCost += gcells[y][x1g]->gammaM1;

                        if (gcells[y][x1g]->bottomEdgeCount >= gcells[y][x1g]->bottomEdgeCapacity) {
                            net->totalCost += betaHalfMaxCellCost;
                            net->overflow++;
                        }
                        gcells[y][x1g]->bottomEdgeCount++;
                    }
                } else {
                    for (int y = y1g - 1; y >= y2g; y--) {
                        net->gcells.push_back(gcells[y][x1g]);
                        net->cellCost += gcells[y][x1g]->costM1;
                        net->totalCost += gcells[y][x1g]->gammaM1;

                        if (gcells[y+1][x1g]->bottomEdgeCount >= gcells[y+1][x1g]->bottomEdgeCapacity) {
                            net->totalCost += betaHalfMaxCellCost;
                            net->overflow++;
                        }
                        gcells[y+1][x1g]->bottomEdgeCount++;
                    }
                }
                lastM1 = true;
            } else if (command == "M2") {
                int x1, y1, x2, y2;
                iss >> x1 >> y1 >> x2 >> y2;
                int x1g = (x1 - routingAreaLowerLeft.x) / gcellSize.x;
                int y1g = (y1 - routingAreaLowerLeft.y) / gcellSize.y;
                int x2g = (x2 - routingAreaLowerLeft.x) / gcellSize.x;
                int y2g = (y2 - routingAreaLowerLeft.y) / gcellSize.y;
                if (checkStartPoint) {
                    if (startBump->position.x != x1 || startBump->position.y != y1) {
                        LOG_ERROR("[Net: " + std::to_string(netIdx) + "] Start point mismatch: (" + std::to_string(x1) + ", " + std::to_string(y1) + ")");
                    }
                    checkStartPoint = false;
                }
                if (y2 != y1) {
                    LOG_ERROR("[Net: " + std::to_string(netIdx) + "] M2 vertical routing is forbidden!");
                }
                if (x2 == x1) {
                    LOG_ERROR("[Net: " + std::to_string(netIdx) + "] M2 zero-length routing is forbidden!");
                }
                if ((x1 - routingAreaLowerLeft.x) % gcellSize.x != 0 || 
                    (y1 - routingAreaLowerLeft.y) % gcellSize.y != 0 || 
                    (x2 - routingAreaLowerLeft.x) % gcellSize.x != 0 || 
                    (y2 - routingAreaLowerLeft.y) % gcellSize.y != 0) {
                    LOG_ERROR("[Net: " + std::to_string(netIdx) + "] M2 routing point is not on GCell lower left corner!");
                }
                net->WL += std::abs(x2 - x1);
                net->totalCost += std::abs(x2 - x1) * alpha;
                // net->gcells.push_back(gcells[y1g][x1g]);
                if (passVia) {
                    
                } else {
                    net->cellCost += gcells[y1g][x1g]->costM2;
                    net->totalCost += gcells[y1g][x1g]->gammaM2;
                }
                passVia = false;
                bool goRIGHT = x2 > x1;
                if (goRIGHT) {
                    for (int x = x1g + 1; x <= x2g; x++) {
                        net->gcells.push_back(gcells[y1g][x]);
                        net->cellCost += gcells[y1g][x]->costM2;
                        net->totalCost += gcells[y1g][x]->gammaM2;

                        if (gcells[y1g][x]->leftEdgeCount >= gcells[y1g][x]->leftEdgeCapacity) {
                            net->totalCost += betaHalfMaxCellCost;
                            net->overflow++;
                        }
                        gcells[y1g][x]->leftEdgeCount++;
                    }
                } else {
                    for (int x = x1g - 1; x >= x2g; x--) {
                        net->gcells.push_back(gcells[y1g][x]);
                        net->cellCost += gcells[y1g][x]->costM2;
                        net->totalCost += gcells[y1g][x]->gammaM2;

                        if (gcells[y1g][x+1]->leftEdgeCount >= gcells[y1g][x+1]->leftEdgeCapacity) {
                            net->totalCost += betaHalfMaxCellCost;
                            net->overflow++;
                        }
                        gcells[y1g][x+1]->leftEdgeCount++;
                    }
                }
                lastM1 = false;
            } else if (command == "via") {
                GCell* latestGCell = net->gcells.back();
                if (lastM1) {
                    if (checkStartPoint) {

                    } else {
                        net->cellCost -= latestGCell->costM1;
                        net->totalCost -= latestGCell->gammaM1;
                    }
                    net->cellCost += latestGCell->costM1 / 2;
                    net->totalCost += latestGCell->gammaM1 / 2;
                    net->cellCost += latestGCell->costM2 / 2;
                    net->totalCost += latestGCell->gammaM2 / 2;
                } else {
                    if (checkStartPoint) {

                    } else {
                        net->cellCost -= latestGCell->costM2;
                        net->totalCost -= latestGCell->gammaM2;
                    }
                    net->cellCost += latestGCell->costM2 / 2;
                    net->totalCost += latestGCell->gammaM2 / 2;
                    net->cellCost += latestGCell->costM1 / 2;
                    net->totalCost += latestGCell->gammaM1 / 2;
                }
                net->viaCount++;
                net->totalCost += deltaViaCost;
                passVia = true;
            } else if (command == ".end") {
                GCell* latestGCell = net->gcells.back();
                if (latestGCell->lowerLeft.x != endBump->position.x || latestGCell->lowerLeft.y != endBump->position.y) {
                    LOG_ERROR("[Net: " + std::to_string(netIdx) + "] End point mismatch: (" + std::to_string(latestGCell->lowerLeft.x) + ", " + std::to_string(latestGCell->lowerLeft.y) + ")");
                }
                if (passVia) {
                    if (lastM1) {
                        LOG_ERROR("[Net: " + std::to_string(netIdx) + "] Last routing is not M1!");
                    }
                } else {
                    if (!lastM1) {
                        LOG_ERROR("[Net: " + std::to_string(netIdx) + "] Last routing is not M1!");
                    }
                }
                break;
            } else {
                LOG_ERROR("Unknown command " + command);
            }
        }
    }
}

void Evaluator::printCosts() {
    VariadicTable<std::string, int, int, double, int, double> vt({"Net", "WL", "Overflow", "Cell Cost", "Via Count", "Total Cost"});

    Net* totalNet = new Net();
    for (auto& net : nets) {
        vt.addRow(std::to_string(net->idx), net->WL, net->overflow, net->cellCost, net->viaCount, net->totalCost);
        totalNet->WL += net->WL;
        totalNet->overflow += net->overflow;
        totalNet->cellCost += net->cellCost;
        totalNet->viaCount += net->viaCount;
        totalNet->totalCost += net->totalCost;
    }
    vt.addRow("Total", totalNet->WL, totalNet->overflow, totalNet->cellCost, totalNet->viaCount, totalNet->totalCost);

    vt.print(std::cout);
    delete totalNet;
}