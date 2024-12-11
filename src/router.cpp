#include <cfloat>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <set>
#include <queue>
#include <unordered_set>
#include <numeric>
#include <random>
#include "router.h"
#include "logger.h"

Router::Router() {
    startTime = std::chrono::high_resolution_clock::now();
}

Router::~Router() {
    for (auto& row : gcells) {
        for (auto& gcell : row) {
            delete gcell;
        }
    }
    for (auto& route : routes) {
        delete route;
    }
}

bool is_blank( const std::string & s ) {
  return s.find_first_not_of( " \f\n\r\t\v" ) == s.npos;
}

void Router::loadGridMap(const std::string& filename) {
    // Load grid map
    // LOG_INFO("Loading grid map from " + filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        // LOG_ERROR("Cannot open file " + filename);
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
                    // LOG_ERROR("Unknown command " + command);
                }
                break;
            }
            case State::LoadingRoutingArea: {
                iss >> routingAreaLowerLeft.x >> routingAreaLowerLeft.y >> routingAreaSize.x >> routingAreaSize.y;
                state = State::LoadingCommand;
                // LOG_TRACE("Routing area lower left: (" + std::to_string(routingAreaLowerLeft.x) + ", " + std::to_string(routingAreaLowerLeft.y) + ")");
                // LOG_TRACE("Routing area size: (" + std::to_string(routingAreaSize.x) + ", " + std::to_string(routingAreaSize.y) + ")");
                break;
            }
            case State::LoadingGCellSize: {
                iss >> gcellSize.x >> gcellSize.y;
                state = State::LoadingCommand;
                // LOG_TRACE("GCell size: (" + std::to_string(gcellSize.x) + ", " + std::to_string(gcellSize.y) + ")");
                break;
            }
            case State::LoadingChip1: {
                int x, y;
                iss >> x >> y >> chip1.size.x >> chip1.size.y;
                chip1.lowerLeft = {x + routingAreaLowerLeft.x, y + routingAreaLowerLeft.y};
                state = State::LoadingCommand;
                // LOG_TRACE("Chip 1 lower left: (" + std::to_string(chip1.lowerLeft.x) + ", " + std::to_string(chip1.lowerLeft.y) + ")");
                // LOG_TRACE("Chip 1 size: (" + std::to_string(chip1.size.x) + ", " + std::to_string(chip1.size.y) + ")");
                break;
            }
            case State::LoadingBump: {
                int bidx, bx, by;
                iss >> bidx >> bx >> by;
                if (loadingChip1) {
                    Bump bump = {bidx, {bx + chip1.lowerLeft.x, by + chip1.lowerLeft.y}, nullptr};
                    chip1.bumps.push_back(bump);
                    // LOG_TRACE("Chip 1 bump (" + std::to_string(bump.position.x) + ", " + std::to_string(bump.position.y) + ")");
                } else {
                    Bump bump = {bidx, {bx + chip2.lowerLeft.x, by + chip2.lowerLeft.y}, nullptr};
                    chip2.bumps.push_back(bump);
                    // LOG_TRACE("Chip 2 bump (" + std::to_string(bump.position.x) + ", " + std::to_string(bump.position.y) + ")");
                }
                break;
            }
            case State::LoadingChip2: {
                int x, y;
                iss >> x >> y >> chip2.size.x >> chip2.size.y;
                chip2.lowerLeft = {x + routingAreaLowerLeft.x, y + routingAreaLowerLeft.y};
                state = State::LoadingCommand;
                // LOG_TRACE("Chip 2 lower left: (" + std::to_string(chip2.lowerLeft.x) + ", " + std::to_string(chip2.lowerLeft.y) + ")");
                // LOG_TRACE("Chip 2 size: (" + std::to_string(chip2.size.x) + ", " + std::to_string(chip2.size.y) + ")");
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
            gcell->gScore = DBL_MAX;
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
        // LOG_TRACE("Chip 1 bump (" + std::to_string(bump.position.x) + ", " + std::to_string(bump.position.y) + ") -> GCell (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        bump.gcell = gcells[y][x];
    }
    for (auto& bump : chip2.bumps) {
        int x = (bump.position.x - routingAreaLowerLeft.x) / gcellSize.x;
        int y = (bump.position.y - routingAreaLowerLeft.y) / gcellSize.y;
        // LOG_TRACE("Chip 2 bump (" + std::to_string(bump.position.x) + ", " + std::to_string(bump.position.y) + ") -> GCell (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        bump.gcell = gcells[y][x];
    }
}

void Router::loadGCells(const std::string& filename) {
    // Load gcells
    // LOG_INFO("Loading gcells from " + filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        // LOG_ERROR("Cannot open file " + filename);
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
                    // LOG_ERROR("Unknown command " + command);
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

void Router::loadCost(const std::string& filename) {
    // Load cost
    // LOG_INFO("Loading cost from " + filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        // LOG_ERROR("Cannot open file " + filename);
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
                    // LOG_TRACE("Alpha: " + std::to_string(alpha));
                } else if (command == ".beta") {
                    iss >> beta;
                    state = State::LoadingCommand;
                    // LOG_TRACE("Beta: " + std::to_string(beta));
                } else if (command == ".gamma") {
                    iss >> gamma;
                    state = State::LoadingCommand;
                    // LOG_TRACE("Gamma: " + std::to_string(gamma));
                } else if (command == ".delta") {
                    iss >> delta;
                    state = State::LoadingCommand;
                    // LOG_TRACE("Delta: " + std::to_string(delta));
                } else {
                    // LOG_ERROR("Unknown command " + command);
                }
                break;
            }
            case State::LoadingViaCost: {
                iss >> viaCost;
                state = State::LoadingCommand;
                // LOG_TRACE("Via cost: " + std::to_string(viaCost));
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

void Router::dumpRoutes(const std::string& filename) {
    // Dump routes
    // LOG_INFO("Dumping routes to " + filename);

    std::ofstream file(filename);
    if (!file.is_open()) {
        // LOG_ERROR("Cannot open file " + filename);
        return;
    }

    for (auto& route : routes) {
        file << "n" << route->idx << std::endl;
        Metal currentMetal = Metal::M1;
        Point<int> fromPoint = route->route[0]->lowerLeft;
        Point<int> lastPoint = route->route[0]->lowerLeft;
        for (size_t i = 1; i < route->route.size(); i++) {
            Point<int> currPoint = route->route[i]->lowerLeft;
            Point<int> diff = {currPoint.x - lastPoint.x, currPoint.y - lastPoint.y};
            if (diff.x != 0) {
                if (diff.y != 0) {
                    // LOG_ERROR("Invalid route, cannot change both x and y");
                    return;
                }
                if (currentMetal == Metal::M1) {
                    // From Vertical to Horizontal
                    if (fromPoint.y != lastPoint.y) // Start Point correction
                        file << "M1 " << fromPoint.x << " " << fromPoint.y << " " << lastPoint.x << " " << lastPoint.y << std::endl;
                    file << "via" << std::endl; // Via
                    currentMetal = Metal::M2;
                    fromPoint = lastPoint;
                } else {
                    // From Horizontal to Horizontal
                }
            } else if (diff.y != 0) {
                if (diff.x != 0) {
                    // LOG_ERROR("Invalid route, cannot change both x and y");
                    return;
                }
                if (currentMetal == Metal::M1) {
                    // From Vertical to Vertical
                } else {
                    // From Horizontal to Vertical
                    file << "M2 " << fromPoint.x << " " << fromPoint.y << " " << lastPoint.x << " " << lastPoint.y << std::endl;
                    file << "via" << std::endl; // Via
                    currentMetal = Metal::M1;
                    fromPoint = lastPoint;
                }
            } else {
                // LOG_ERROR("Invalid route, no change in x and y");
                return;
            }
            lastPoint = currPoint;
        }
        if (currentMetal == Metal::M1) {
            file << "M1 " << fromPoint.x << " " << fromPoint.y << " " << lastPoint.x << " " << lastPoint.y << std::endl;
        } else {
            file << "M2 " << fromPoint.x << " " << fromPoint.y << " " << lastPoint.x << " " << lastPoint.y << std::endl;
            file << "via" << std::endl;
        }
        file << ".end" << std::endl;
    }
    file.close();
}

// https://zh.wikipedia.org/zh-tw/A*搜尋演算法
Route* Router::router(GCell* source, GCell* target) {
    // Route
    // LOG_INFO("Routing from (" + std::to_string(source->lowerLeft.x) + ", " + std::to_string(source->lowerLeft.y) + ") to (" + std::to_string(target->lowerLeft.x) + ", " + std::to_string(target->lowerLeft.y) + ")");
    
    const auto cmp = [](GCell* a, GCell* b) {
        return a->gScore > b->gScore;
    };
    std::unordered_set<GCell*> closedSet;
    std::unordered_set<GCell*> openSet;
    std::priority_queue<GCell*, std::vector<GCell*>, decltype(cmp)> openSetQ(cmp);

    source->gScore = source->gammaM1;
    source->fromDirection = GCell::FromDirection::ORIGIN;
    openSet.insert(source);
    openSetQ.push(source);

    while (!openSet.empty()) {
        GCell* current;
        while (true) {
            current = openSetQ.top();
            if (openSet.find(current) != openSet.end()) {
                openSetQ.pop();
                break;
            }
            openSetQ.pop();
        }
        if (current == target) {
            // LOG_INFO("Found route!");
            // LOG_INFO("Total cost: " + std::to_string(current->gScore));
            Route* route = new Route();
            route->cost = current->gScore;
            while (current != nullptr) {
                GCell* next;
                switch (current->fromDirection) {
                    case GCell::FromDirection::ORIGIN: {
                        // LOG_ERROR("Invalid from direction");
                        delete route;
                        return nullptr;
                    }
                    case GCell::FromDirection::LEFT: {
                        next = current->left;
                        current->addRouteLeft();
                        break;
                    }
                    case GCell::FromDirection::BOTTOM: {
                        next = current->bottom;
                        current->addRouteBottom();
                        break;
                    }
                    case GCell::FromDirection::RIGHT: {
                        next = current->right;
                        next->addRouteLeft();
                        break;
                    }
                    case GCell::FromDirection::TOP: {
                        next = current->top;
                        next->addRouteBottom();
                        break;
                    }
                    default: {
                        // LOG_ERROR("Unknown from direction");
                        delete route;
                        return nullptr;
                    }
                }
                route->route.push_back(current);
                current = next;
                if (current == source) {
                    route->route.push_back(current);
                    break;
                }
            }
            std::reverse(route->route.begin(), route->route.end());
            return route;
        }

        openSet.erase(current);
        closedSet.insert(current);

        // LOG_TRACE("Current cell: (" + std::to_string(current->lowerLeft.x) + ", " + std::to_string(current->lowerLeft.y) + ")");
        if (current->fromDirection != GCell::FromDirection::LEFT) if (
            current->left != nullptr && closedSet.find(current->left) == closedSet.end()
        ) {
            GCell* neighbor = current->left;
            double tentativeGScore;
            // We are going left, so we are using M2 (Horizontal)
            switch (current->fromDirection) {
                // M1 -> M2
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM:
                case GCell::FromDirection::TOP: {
                    if (neighbor == target) {
                        tentativeGScore = current->gScore
                                        + alphaGcellSizeX
                                        + neighbor->M1M2ViaCost
                                        - current->gammaM1
                                        + current->M1M2ViaCost;
                    } else {
                        tentativeGScore = current->gScore
                                        + alphaGcellSizeX
                                        + neighbor->gammaM2
                                        - current->gammaM1
                                        + current->M1M2ViaCost;
                    }
                    if (current->isLeftEdgeFull) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M2
                case GCell::FromDirection::RIGHT: {
                    if (neighbor == target) {
                        tentativeGScore = current->gScore
                                        + alphaGcellSizeX
                                        + neighbor->M1M2ViaCost;
                    } else {
                        tentativeGScore = current->gScore
                                        + alphaGcellSizeX
                                        + neighbor->gammaM2;
                    }
                    if (current->isLeftEdgeFull) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    // LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (openSet.find(neighbor) == openSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScore) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = current;
                neighbor->gScore = tentativeGScore;
                neighbor->fromDirection = GCell::FromDirection::RIGHT;
                openSet.insert(neighbor);
                openSetQ.push(neighbor);
            }    
        }

        if (current->fromDirection != GCell::FromDirection::BOTTOM) if (
            current->bottom != nullptr && closedSet.find(current->bottom) == closedSet.end()
        ) {
            GCell* neighbor = current->bottom;
            double tentativeGScore;
            // We are going bottom, so we are using M1 (Vertical)
            switch (current->fromDirection) {
                // M1 -> M1
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::TOP: {
                    tentativeGScore = current->gScore
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1;
                    if (current->isBottomEdgeFull) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M1
                case GCell::FromDirection::LEFT:
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = current->gScore
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1
                                    - current->gammaM2
                                    + current->M1M2ViaCost;
                    if (current->isBottomEdgeFull) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    // LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (openSet.find(neighbor) == openSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScore) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = current;
                neighbor->gScore = tentativeGScore;
                neighbor->fromDirection = GCell::FromDirection::TOP;
                openSet.insert(neighbor);
                openSetQ.push(neighbor);
            }
        }

        if (current->fromDirection != GCell::FromDirection::RIGHT) if (
            current->right != nullptr && closedSet.find(current->right) == closedSet.end()
        ) {
            GCell* neighbor = current->right;
            double tentativeGScore;
            // We are going right, so we are using M2 (Horizontal)
            switch (current->fromDirection) {
                // M1 -> M2
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM:
                case GCell::FromDirection::TOP: {
                    if (neighbor == target) {
                        tentativeGScore = current->gScore
                                        + alphaGcellSizeX
                                        + neighbor->M1M2ViaCost
                                        - current->gammaM1
                                        + current->M1M2ViaCost;
                    } else {
                        tentativeGScore = current->gScore
                                        + alphaGcellSizeX
                                        + neighbor->gammaM2
                                        - current->gammaM1
                                        + current->M1M2ViaCost;
                    }
                    if (neighbor->isLeftEdgeFull) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M2
                case GCell::FromDirection::LEFT: {
                    if (neighbor == target) {
                        tentativeGScore = current->gScore
                                        + alphaGcellSizeX
                                        + neighbor->M1M2ViaCost;
                    } else {
                        tentativeGScore = current->gScore
                                        + alphaGcellSizeX
                                        + neighbor->gammaM2;
                    }
                    if (neighbor->isLeftEdgeFull) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    // LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (openSet.find(neighbor) == openSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScore) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = current;
                neighbor->gScore = tentativeGScore;
                neighbor->fromDirection = GCell::FromDirection::LEFT;
                openSet.insert(neighbor);
                openSetQ.push(neighbor);
            }
        }

        if (current->fromDirection != GCell::FromDirection::TOP) if (
            current->top != nullptr && closedSet.find(current->top) == closedSet.end()
        ) {
            GCell* neighbor = current->top;
            double tentativeGScore;
            // We are going top, so we are using M1 (Vertical)
            switch (current->fromDirection) {
                // M1 -> M1
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM: {
                    tentativeGScore = current->gScore
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1;
                    if (neighbor->isBottomEdgeFull) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M1
                case GCell::FromDirection::LEFT:
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = current->gScore
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1
                                    - current->gammaM2
                                    + current->M1M2ViaCost;
                    if (neighbor->isBottomEdgeFull) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    // LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (openSet.find(neighbor) == openSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScore) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = current;
                neighbor->gScore = tentativeGScore;
                neighbor->fromDirection = GCell::FromDirection::BOTTOM;
                openSet.insert(neighbor);
                openSetQ.push(neighbor);
            }
        }
    }

    return nullptr;
}

Route* Router::fast_router(GCell* source, GCell* target) {
    // L-Pattern Routing
    Route* route = new Route();
    GCell* current = source;
    GCell* next = nullptr;
    while (current != target) {
        if (current->lowerLeft.x < target->lowerLeft.x) {
            next = current->right;
            if (next == nullptr) {
                // LOG_ERROR("Cannot find route from (" + std::to_string(current->lowerLeft.x) + ", " + std::to_string(current->lowerLeft.y) + ") to (" + std::to_string(target->lowerLeft.x) + ", " + std::to_string(target->lowerLeft.y) + ")");
                delete route;
                return nullptr;
            }
            route->route.push_back(current);
            current = next;
        } else if (current->lowerLeft.x > target->lowerLeft.x) {
            next = current->left;
            if (next == nullptr) {
                // LOG_ERROR("Cannot find route from (" + std::to_string(current->lowerLeft.x) + ", " + std::to_string(current->lowerLeft.y) + ") to (" + std::to_string(target->lowerLeft.x) + ", " + std::to_string(target->lowerLeft.y) + ")");
                delete route;
                return nullptr;
            }
            route->route.push_back(current);
            current = next;
        } else if (current->lowerLeft.y < target->lowerLeft.y) {
            next = current->top;
            if (next == nullptr) {
                // LOG_ERROR("Cannot find route from (" + std::to_string(current->lowerLeft.x) + ", " + std::to_string(current->lowerLeft.y) + ") to (" + std::to_string(target->lowerLeft.x) + ", " + std::to_string(target->lowerLeft.y) + ")");
                delete route;
                return nullptr;
            }
            route->route.push_back(current);
            current = next;
        } else if (current->lowerLeft.y > target->lowerLeft.y) {
            next = current->bottom;
            if (next == nullptr) {
                // LOG_ERROR("Cannot find route from (" + std::to_string(current->lowerLeft.x) + ", " + std::to_string(current->lowerLeft.y) + ") to (" + std::to_string(target->lowerLeft.x) + ", " + std::to_string(target->lowerLeft.y) + ")");
                delete route;
                return nullptr;
            }
            route->route.push_back(current);
            current = next;
        } else {
            // LOG_ERROR("Invalid route, no change in x and y");
            delete route;
            return nullptr;
        }
    }
    route->route.push_back(current);
    return route;
}

double Router::solve() {
    // Run
    // LOG_INFO("Running router");

    std::vector<size_t> bump_indices(chip1.bumps.size());
    std::iota(bump_indices.begin(), bump_indices.end(), 0);
    std::shuffle(bump_indices.begin(), bump_indices.end(), rng);

    double totalCost = 0;
    for (size_t i = 0; i < bump_indices.size(); i++) {
        size_t bump_idx = bump_indices[i];
    // for (size_t bump_idx = 0; bump_idx < chip1.bumps.size(); bump_idx++) {
        // LOG_INFO("Routing bump " + std::to_string(bump_idx));
        Bump& bump1 = chip1.bumps[bump_idx];
        Bump& bump2 = chip2.bumps[bump_idx];
        if (bump1.idx != bump2.idx) {
            // LOG_ERROR("Bump index mismatch");
            return DBL_MAX;
        }
        Route* route;
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - startTime;
        if (elapsed.count() > 590) {
            route = fast_router(bump1.gcell, bump2.gcell);
        } else {
            route = router(bump1.gcell, bump2.gcell);
        }
        route->idx = bump1.idx;
        if (route == nullptr) {
            // LOG_ERROR("Cannot find route from (" + std::to_string(bump1.gcell->lowerLeft.x) + ", " + std::to_string(bump1.gcell->lowerLeft.y) + ") to (" + std::to_string(bump2.gcell->lowerLeft.x) + ", " + std::to_string(bump2.gcell->lowerLeft.y) + ")");
            return DBL_MAX;
        } else {
            // LOG_INFO("Success");
            routes.push_back(route);
            totalCost += route->cost;
        }
    }
    // LOG_INFO("Router finished");

    std::sort(routes.begin(), routes.end(), [](const Route* a, const Route* b) {
        return a->idx < b->idx;
    });

    return totalCost;
}