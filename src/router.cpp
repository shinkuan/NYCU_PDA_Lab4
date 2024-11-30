#include <cfloat>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <set>
#include <queue>
#include <unordered_set>
#include "router.h"
#include "logger.h"

Router::Router() {
}

Router::~Router() {
    for (auto& row : gcellsM1) {
        for (auto& gcell : row) {
            delete gcell;
        }
    }
    for (auto& row : gcellsM2) {
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

    gcellsM1.resize(routingAreaSize.y / gcellSize.y);
    for (auto& row : gcellsM1) {
        row.resize(routingAreaSize.x / gcellSize.x);
    }
    gcellsM2.resize(routingAreaSize.y / gcellSize.y);
    for (auto& row : gcellsM2) {
        row.resize(routingAreaSize.x / gcellSize.x);
    }
    for (size_t y = 0; y < gcellsM1.size(); y++) {
        for (size_t x = 0; x < gcellsM1[y].size(); x++) {
            GCell* gcell = new GCell();
            gcell->parent = nullptr;
            gcell->gScore = DBL_MAX;
            gcell->fromDirection = GCell::FromDirection::ORIGIN;
            gcell->lowerLeft = {static_cast<int>(x) * gcellSize.x + routingAreaLowerLeft.x, static_cast<int>(y) * gcellSize.y + routingAreaLowerLeft.y};
            gcellsM1[y][x] = gcell;
                   gcell = new GCell();
            gcell->parent = nullptr;
            gcell->gScore = DBL_MAX;
            gcell->fromDirection = GCell::FromDirection::ORIGIN;
            gcell->lowerLeft = {static_cast<int>(x) * gcellSize.x + routingAreaLowerLeft.x, static_cast<int>(y) * gcellSize.y + routingAreaLowerLeft.y};
            gcellsM2[y][x] = gcell;
        }
    }

    for (size_t y = 0; y < gcellsM1.size(); y++) {
        for (size_t x = 0; x < gcellsM1[y].size(); x++) {
            GCell* gcell = gcellsM1[y][x];
            if (y > 0) {
                gcell->westSouth = gcellsM1[y - 1][x];
            } else {
                gcell->westSouth = nullptr;
            }
            if (y < gcellsM1.size() - 1) {
                gcell->eastNorth = gcellsM1[y + 1][x];
            } else {
                gcell->eastNorth = nullptr;
            }
            gcell->dowsUp = gcellsM2[y][x];
        }
    }
    for (size_t y = 0; y < gcellsM2.size(); y++) {
        for (size_t x = 0; x < gcellsM2[y].size(); x++) {
            GCell* gcell = gcellsM2[y][x];
            if (x > 0) {
                gcell->westSouth = gcellsM2[y][x - 1];
            } else {
                gcell->westSouth = nullptr;
            }
            if (x < gcellsM2[y].size() - 1) {
                gcell->eastNorth = gcellsM2[y][x + 1];
            } else {
                gcell->eastNorth = nullptr;
            }
            gcell->dowsUp = gcellsM1[y][x];
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
        bump.gcell = gcellsM1[y][x];
    }
    for (auto& bump : chip2.bumps) {
        int x = (bump.position.x - routingAreaLowerLeft.x) / gcellSize.x;
        int y = (bump.position.y - routingAreaLowerLeft.y) / gcellSize.y;
        LOG_TRACE("Chip 2 bump (" + std::to_string(bump.position.x) + ", " + std::to_string(bump.position.y) + ") -> GCell (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        bump.gcell = gcellsM1[y][x];
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
                GCell* gcell = gcellsM1[loadedGCellCount / gcellsM1[0].size()][loadedGCellCount % gcellsM1[0].size()];
                gcell->WSEdgeCapacity = bottomEdgeCapacity;
                       gcell = gcellsM2[loadedGCellCount / gcellsM2[0].size()][loadedGCellCount % gcellsM2[0].size()];
                gcell->WSEdgeCapacity = leftEdgeCapacity;
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

    std::vector<double> costs(gcellsM1.size() * gcellsM1[0].size() * 2);
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
                for (size_t x = 0; x < gcellsM1[currentRow].size(); x++) {
                    iss >> cost;
                    if (cost != 0) costs.push_back(cost);
                    if (cost > maxCellCost) {
                        maxCellCost = cost;
                    }
                    if (currentLayer == 0) {
                        gcellsM1[currentRow][x]->cost = cost;
                        gcellsM1[currentRow][x]->gammaCost = gamma * cost;
                    } else {
                        gcellsM2[currentRow][x]->cost = cost;
                        gcellsM2[currentRow][x]->gammaCost = gamma * cost;
                    }
                }
                currentRow++;
                if (currentRow == gcellsM1.size()) {
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
    for (size_t y = 0; y < gcellsM1.size(); y++) {
        for (size_t x = 0; x < gcellsM1[y].size(); x++) {
            double M1M2ViaCost = deltaViaCost + (gcellsM1[y][x]->gammaCost + gcellsM2[y][x]->gammaCost) / 2;
            if (x > 0) {
                gcellsM2[y][x]->costWS = gcellsM2[y][x - 1]->gammaCost + alphaGcellSizeX;
            } else {
                gcellsM2[y][x]->costWS = DBL_MAX;
            }
            if (x < gcellsM2[y].size() - 1) {
                gcellsM2[y][x]->costEN = gcellsM2[y][x + 1]->gammaCost + alphaGcellSizeX;
            } else {
                gcellsM2[y][x]->costEN = DBL_MAX;
            }
            if (y > 0) {
                gcellsM1[y][x]->costWS = gcellsM1[y - 1][x]->gammaCost + alphaGcellSizeY;
            } else {
                gcellsM1[y][x]->costWS = DBL_MAX;
            }
            if (y < gcellsM1.size() - 1) {
                gcellsM1[y][x]->costEN = gcellsM1[y + 1][x]->gammaCost + alphaGcellSizeY;
            } else {
                gcellsM1[y][x]->costEN = DBL_MAX;
            }
            double M1DUCost = M1M2ViaCost - gcellsM1[y][x]->gammaCost;
            double M2DUCost = M1M2ViaCost - gcellsM2[y][x]->gammaCost;
            if (M1DUCost < 0) LOG_WARNING("M1 Cell [y: " + std::to_string(y) + ", x: " + std::to_string(x) + "] Up cost is negative");
            if (M2DUCost < 0) LOG_WARNING("M2 Cell [y: " + std::to_string(y) + ", x: " + std::to_string(x) + "] Down cost is negative");
            gcellsM1[y][x]->costDU = M1DUCost;
            gcellsM2[y][x]->costDU = M2DUCost;
        }
    }
}

void Router::dumpRoutes(const std::string& filename) {
    // Dump routes
    LOG_INFO("Dumping routes to " + filename);

    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open file " + filename);
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
            if (diff.x == 0 && diff.y == 0) {
                if (currentMetal == Metal::M1) {
                    // From Vertical to Horizontal
                    if (fromPoint.y != lastPoint.y) // Start Point correction
                        file << "M1 " << fromPoint.x << " " << fromPoint.y << " " << lastPoint.x << " " << lastPoint.y << std::endl;
                    file << "via" << std::endl; // Via
                    currentMetal = Metal::M2;
                    fromPoint = currPoint;
                } else {
                    // From Horizontal to Vertical
                    file << "M2 " << fromPoint.x << " " << fromPoint.y << " " << lastPoint.x << " " << lastPoint.y << std::endl;
                    file << "via" << std::endl; // Via
                    currentMetal = Metal::M1;
                    fromPoint = currPoint;
                }
            }
            lastPoint = currPoint;
        }
        if (currentMetal == Metal::M1 && fromPoint.y != lastPoint.y) {
            file << "M1 " << fromPoint.x << " " << fromPoint.y << " " << lastPoint.x << " " << lastPoint.y << std::endl;
        }
        file << ".end" << std::endl;
    }
    file.close();
}

// https://zh.wikipedia.org/zh-tw/A*搜尋演算法
Route* Router::router(GCell* source, GCell* target) {
    // Route
    LOG_INFO("Routing from (" + std::to_string(source->lowerLeft.x) + ", " + std::to_string(source->lowerLeft.y) + ") to (" + std::to_string(target->lowerLeft.x) + ", " + std::to_string(target->lowerLeft.y) + ")");
    
    const auto cmp = [](GCell* a, GCell* b) {
        return a->gScore > b->gScore;
    };
    std::unordered_set<GCell*> closedSet;
    std::unordered_set<GCell*> openSet;
    std::priority_queue<GCell*, std::vector<GCell*>, decltype(cmp)> openSetQ(cmp);

    source->gScore = source->gammaCost;
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
            LOG_INFO("Found route!");
            LOG_INFO("Total cost: " + std::to_string(current->gScore));
            Route* route = new Route();
            while (current != nullptr) {
                GCell* next;
                switch (current->fromDirection) {
                    case GCell::FromDirection::ORIGIN: {
                        LOG_ERROR("Invalid from direction");
                        return nullptr;
                    }
                    case GCell::FromDirection::WEST_SOUTH: {
                        next = current->westSouth;
                        current->addRoute(route);
                        break;
                    }
                    case GCell::FromDirection::EAST_NORTH: {
                        next = current->eastNorth;
                        current->addRoute(route);
                        break;
                    }
                    case GCell::FromDirection::DOWN_UP: {
                        next = current->dowsUp;
                        current->addRoute(route);
                        break;
                    }
                    default: {
                        LOG_ERROR("Unknown from direction");
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

        LOG_TRACE("Current cell: (" + std::to_string(current->lowerLeft.x) + ", " + std::to_string(current->lowerLeft.y) + ")");
        if (
            current->fromDirection != GCell::FromDirection::WEST_SOUTH 
            && current->westSouth != nullptr 
            && closedSet.find(current->westSouth) == closedSet.end()
        ) {
            // Going to west or south
            GCell* neighbor = current->westSouth;
            double tentativeGScore;
            tentativeGScore = current->gScore + current->costWS;
            if (current->WSEdgeCount >= current->WSEdgeCapacity) {
                tentativeGScore += betaHalfMaxCellCost;
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
                neighbor->fromDirection = GCell::FromDirection::EAST_NORTH;
                openSet.insert(neighbor);
                openSetQ.push(neighbor);
            }    
        }

        if (
            current->fromDirection != GCell::FromDirection::EAST_NORTH 
            && current->eastNorth != nullptr 
            && closedSet.find(current->eastNorth) == closedSet.end()
        ) {
            // Going to east or north
            GCell* neighbor = current->eastNorth;
            double tentativeGScore;
            tentativeGScore = current->gScore + current->costEN;
            if (neighbor->WSEdgeCount >= neighbor->WSEdgeCapacity) {
                tentativeGScore += betaHalfMaxCellCost;
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
                neighbor->fromDirection = GCell::FromDirection::WEST_SOUTH;
                openSet.insert(neighbor);
                openSetQ.push(neighbor);
            }
        }

        if (
            current->fromDirection != GCell::FromDirection::DOWN_UP
            && current->dowsUp != nullptr
            && closedSet.find(current->dowsUp) == closedSet.end()
        ) {
            GCell* neighbor = current->dowsUp;
            double tentativeGScore;
            tentativeGScore = current->gScore + current->costDU;

            bool tentativeIsBetter = false;
            if (openSet.find(neighbor) == openSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScore) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = current;
                neighbor->gScore = tentativeGScore;
                neighbor->fromDirection = GCell::FromDirection::DOWN_UP;
                openSet.insert(neighbor);
                openSetQ.push(neighbor);
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
        if (bump1.idx != bump2.idx) {
            LOG_ERROR("Bump index mismatch");
        }
        Route* route = router(bump1.gcell, bump2.gcell);
        route->idx = bump1.idx;
        if (route == nullptr) {
            LOG_ERROR("Cannot find route from (" + std::to_string(bump1.gcell->lowerLeft.x) + ", " + std::to_string(bump1.gcell->lowerLeft.y) + ") to (" + std::to_string(bump2.gcell->lowerLeft.x) + ", " + std::to_string(bump2.gcell->lowerLeft.y) + ")");
        } else {
            LOG_INFO("Success");
            routes.push_back(route);
        }
    }
    LOG_INFO("Router finished");

    std::sort(routes.begin(), routes.end(), [](const Route* a, const Route* b) {
        return a->idx < b->idx;
    });
}