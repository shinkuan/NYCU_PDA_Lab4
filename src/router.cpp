#include <cfloat>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <set>
#include <unordered_set>
#include "router.h"
#include "logger.h"

Router::Router() {
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
            gcell->parent.resize(PROCESSOR_COUNT, nullptr);
            gcell->fScore.resize(PROCESSOR_COUNT, DBL_MAX);
            gcell->gScore.resize(PROCESSOR_COUNT, DBL_MAX);
            gcell->hScore.resize(PROCESSOR_COUNT, DBL_MAX);
            gcell->fromDirection.resize(PROCESSOR_COUNT, GCell::FromDirection::ORIGIN);
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

void Router::loadGCells(const std::string& filename) {
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

void Router::loadCost(const std::string& filename) {
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
                    double delta;
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
                if (cost != 0) costs.push_back(cost);
                if (cost > maxCellCost) {
                    maxCellCost = cost;
                }
                for (size_t x = 0; x < gcells[currentRow].size(); x++) {
                    iss >> cost;
                    if (currentLayer == 0) {
                        gcells[currentRow][x]->costM1 = cost;
                    } else {
                        gcells[currentRow][x]->costM2 = cost;
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
}

double Router::heuristicManhattan(GCell* a, GCell* b) {
    // Manhattan distance
    return (std::abs(a->lowerLeft.x - b->lowerLeft.x) + std::abs(a->lowerLeft.y - b->lowerLeft.y))*alpha*medianCellCost;
}

double Router::heuristicCustom(GCell* a, GCell* b) {
    // TODO: Custom heuristic
    return 0.0;
}

// https://zh.wikipedia.org/zh-tw/A*搜尋演算法
Route* Router::router(GCell* source, GCell* target, int processorId = 0) {
    // Route
    LOG_INFO("[Processor " + std::to_string(processorId) + "] Routing from (" + std::to_string(source->lowerLeft.x) + ", " + std::to_string(source->lowerLeft.y) + ") to (" + std::to_string(target->lowerLeft.x) + ", " + std::to_string(target->lowerLeft.y) + ")");
    
    const auto cmp = [processorId](GCell* a, GCell* b) {
        return a->fScore[processorId] < b->fScore[processorId];
    };
    std::unordered_set<GCell*> closedSet;
    std::set<GCell*, decltype(cmp)> openSet(cmp);

    source->gScore[processorId] = 0;
    source->hScore[processorId] = heuristicManhattan(source, target);
    source->fScore[processorId] = source->gScore[processorId] + source->hScore[processorId];
    source->fromDirection[processorId] = GCell::FromDirection::ORIGIN;
    openSet.insert(source);

    while (!openSet.empty()) {
        auto it = openSet.begin();
        GCell* current = *it;
        if (current == target) {
            LOG_TRACE("[Processor " + std::to_string(processorId) + "] Found target");
            Route* route = new Route();
            while (current != nullptr) {
                route->push_back(current);
                current = current->parent[processorId];
                if (current == source) {
                    route->push_back(current);
                    break;
                }
            }
            std::reverse(route->begin(), route->end());
            return route;
        }

        openSet.erase(it);
        closedSet.insert(current);

        LOG_TRACE("[Processor " + std::to_string(processorId) + "] Current cell: (" + std::to_string(current->lowerLeft.x) + ", " + std::to_string(current->lowerLeft.y) + ")");
        if (
            current->left != nullptr && closedSet.find(current->left) == closedSet.end() &&
            current->fromDirection[processorId] != GCell::FromDirection::LEFT
        ) {
            GCell* neighbor = current->left;
            double tentativeGScore;
            // We are going left, so we are using M2 (Horizontal)
            switch (current->fromDirection[processorId]) {
                // M1 -> M2
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM:
                case GCell::FromDirection::TOP: {
                    tentativeGScore = current->gScore[processorId]
                                    + alpha*gcellSize.x
                                    // beta
                                    + gamma*neighbor->costM2
                                    + delta*viaCost;
                    break;
                }
                // M2 -> M2
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = current->gScore[processorId]
                                    + alpha*gcellSize.x
                                    // beta
                                    + gamma*neighbor->costM2;
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (openSet.find(neighbor) == openSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScore[processorId]) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent[processorId] = current;
                neighbor->gScore[processorId] = tentativeGScore;
                neighbor->hScore[processorId] = heuristicManhattan(neighbor, target);
                neighbor->fScore[processorId] = neighbor->gScore[processorId] + neighbor->hScore[processorId];
                neighbor->fromDirection[processorId] = GCell::FromDirection::RIGHT;
                openSet.insert(neighbor);
            }    
        }

        if (
            current->bottom != nullptr && closedSet.find(current->bottom) == closedSet.end() &&
            current->fromDirection[processorId] != GCell::FromDirection::BOTTOM
        ) {
            GCell* neighbor = current->bottom;
            double tentativeGScore;
            // We are going bottom, so we are using M1 (Vertical)
            switch (current->fromDirection[processorId]) {
                // M1 -> M1
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::TOP: {
                    tentativeGScore = current->gScore[processorId]
                                    + alpha*gcellSize.y
                                    // beta
                                    + gamma*neighbor->costM1;
                    break;
                }
                // M2 -> M1
                case GCell::FromDirection::LEFT:
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = current->gScore[processorId]
                                    + alpha*gcellSize.y
                                    // beta
                                    + gamma*neighbor->costM1
                                    + delta*viaCost;
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (openSet.find(neighbor) == openSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScore[processorId]) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent[processorId] = current;
                neighbor->gScore[processorId] = tentativeGScore;
                neighbor->hScore[processorId] = heuristicManhattan(neighbor, target);
                neighbor->fScore[processorId] = neighbor->gScore[processorId] + neighbor->hScore[processorId];
                neighbor->fromDirection[processorId] = GCell::FromDirection::TOP;
                openSet.insert(neighbor);
            }
        }

        if (
            current->right != nullptr && closedSet.find(current->right) == closedSet.end() &&
            current->fromDirection[processorId] != GCell::FromDirection::RIGHT
        ) {
            GCell* neighbor = current->right;
            double tentativeGScore;
            // We are going right, so we are using M2 (Horizontal)
            switch (current->fromDirection[processorId]) {
                // M1 -> M2
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM:
                case GCell::FromDirection::TOP: {
                    tentativeGScore = current->gScore[processorId]
                                    + alpha*gcellSize.x
                                    // beta
                                    + delta*viaCost
                                    + gamma*neighbor->costM2;
                    break;
                }
                // M2 -> M2
                case GCell::FromDirection::LEFT: {
                    tentativeGScore = current->gScore[processorId]
                                    + alpha*gcellSize.x
                                    // beta
                                    + gamma*neighbor->costM2;
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (openSet.find(neighbor) == openSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScore[processorId]) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent[processorId] = current;
                neighbor->gScore[processorId] = tentativeGScore;
                neighbor->hScore[processorId] = heuristicManhattan(neighbor, target);
                neighbor->fScore[processorId] = neighbor->gScore[processorId] + neighbor->hScore[processorId];
                neighbor->fromDirection[processorId] = GCell::FromDirection::LEFT;
                openSet.insert(neighbor);
            }
        }

        if (
            current->top != nullptr && closedSet.find(current->top) == closedSet.end() &&
            current->fromDirection[processorId] != GCell::FromDirection::TOP
        ) {
            GCell* neighbor = current->top;
            double tentativeGScore;
            // We are going top, so we are using M1 (Vertical)
            switch (current->fromDirection[processorId]) {
                // M1 -> M1
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM: {
                    tentativeGScore = current->gScore[processorId]
                                    + alpha*gcellSize.y
                                    // beta
                                    + gamma*neighbor->costM1;
                    break;
                }
                // M2 -> M1
                case GCell::FromDirection::LEFT:
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = current->gScore[processorId]
                                    + alpha*gcellSize.y
                                    // beta
                                    + gamma*neighbor->costM1
                                    + delta*viaCost;
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (openSet.find(neighbor) == openSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScore[processorId]) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent[processorId] = current;
                neighbor->gScore[processorId] = tentativeGScore;
                neighbor->hScore[processorId] = heuristicManhattan(neighbor, target);
                neighbor->fScore[processorId] = neighbor->gScore[processorId] + neighbor->hScore[processorId];
                neighbor->fromDirection[processorId] = GCell::FromDirection::BOTTOM;
                openSet.insert(neighbor);
            }
        }
    }

    return nullptr;
}

void Router::solve() {
    // Run
    LOG_INFO("Running router");

    for (size_t bump_idx = 0; bump_idx < chip1.bumps.size(); bump_idx++) {
        LOG_INFO("Routing bump " + std::to_string(bump_idx));
        Bump& bump1 = chip1.bumps[bump_idx];
        Bump& bump2 = chip2.bumps[bump_idx];
        Route* route = router(bump1.gcell, bump2.gcell);
        if (route == nullptr) {
            LOG_ERROR("Cannot find route from (" + std::to_string(bump1.gcell->lowerLeft.x) + ", " + std::to_string(bump1.gcell->lowerLeft.y) + ") to (" + std::to_string(bump2.gcell->lowerLeft.x) + ", " + std::to_string(bump2.gcell->lowerLeft.y) + ")");
        } else {
            LOG_INFO("Success");
            routes.push_back(route);
        }
    }
}