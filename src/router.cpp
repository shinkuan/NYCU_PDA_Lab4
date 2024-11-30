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
            gcell->parent = nullptr;
            gcell->fScoreForward = DBL_MAX;
            gcell->gScoreForward = DBL_MAX;
            gcell->hScoreForward = DBL_MAX;
            gcell->fScoreBackward = DBL_MAX;
            gcell->gScoreBackward = DBL_MAX;
            gcell->hScoreBackward = DBL_MAX;
            gcell->fromDirectionForward = GCell::FromDirection::ORIGIN;
            gcell->fromDirectionBackward = GCell::FromDirection::ORIGIN;
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
            if (diff.x != 0) {
                if (diff.y != 0) {
                    LOG_ERROR("Invalid route, cannot change both x and y");
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
                    LOG_ERROR("Invalid route, cannot change both x and y");
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
                LOG_ERROR("Invalid route, no change in x and y");
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

double Router::heuristicManhattan(GCell* a, GCell* b) {
    // Manhattan distance
    return (std::abs(a->lowerLeft.x - b->lowerLeft.x) + std::abs(a->lowerLeft.y - b->lowerLeft.y))*alpha*medianCellCost;
}

double Router::heuristicCustom(GCell* a, GCell* b) {
    // TODO: Custom heuristic
    return 0.0;
}

// https://zh.wikipedia.org/zh-tw/A*搜尋演算法
Route* Router::router(GCell* source, GCell* target) {
    // Route
    LOG_INFO("Routing from (" + std::to_string(source->lowerLeft.x) + ", " + std::to_string(source->lowerLeft.y) + ") to (" + std::to_string(target->lowerLeft.x) + ", " + std::to_string(target->lowerLeft.y) + ")");
    
    const auto cmpF = [](GCell* a, GCell* b) {
        return a->fScoreForward > b->fScoreForward;
    };
    std::unordered_set<GCell*> forwardClosedSet;
    std::unordered_set<GCell*> forwardOpenSet;
    std::priority_queue<GCell*, std::vector<GCell*>, decltype(cmpF)> forwardOpenSetQ(cmpF);

    const auto cmpB = [](GCell* a, GCell* b) {
        return a->fScoreBackward > b->fScoreBackward;
    };
    std::unordered_set<GCell*> backwardClosedSet;
    std::unordered_set<GCell*> backwardOpenSet;
    std::priority_queue<GCell*, std::vector<GCell*>, decltype(cmpB)> backwardOpenSetQ(cmpB);

    source->gScoreForward = source->costM1;
    source->hScoreForward = heuristicCustom(source, target);
    source->fScoreForward = source->gScoreForward + source->hScoreForward;
    source->fromDirectionForward = GCell::FromDirection::ORIGIN;
    forwardOpenSet.insert(source);
    forwardOpenSetQ.push(source);

    target->gScoreBackward = target->costM1;
    target->hScoreBackward = heuristicCustom(target, source);
    target->fScoreBackward = target->gScoreBackward + target->hScoreBackward;
    target->fromDirectionBackward = GCell::FromDirection::ORIGIN;
    backwardOpenSet.insert(target);
    backwardOpenSetQ.push(target);

    while (!forwardOpenSet.empty() && !backwardOpenSet.empty()) {
        GCell* forwardCurrent;
        while (true) {
            forwardCurrent = forwardOpenSetQ.top();
            if (forwardOpenSet.find(forwardCurrent) != forwardOpenSet.end()) {
                forwardOpenSetQ.pop();
                break;
            }
            forwardOpenSetQ.pop();
        }

        if (backwardOpenSet.find(forwardCurrent) != backwardOpenSet.end()) {
            LOG_TRACE("Found meeting point");
            GCell* meetingPoint = forwardCurrent;
            Route* route = new Route();
            while (forwardCurrent != nullptr) {
                GCell* next;
                switch (forwardCurrent->fromDirectionForward) {
                    case GCell::FromDirection::ORIGIN: {
                        break;
                    }
                    case GCell::FromDirection::LEFT: {
                        forwardCurrent->addRouteLeft(route);
                        next = forwardCurrent->left;
                        break;
                    }
                    case GCell::FromDirection::BOTTOM: {
                        forwardCurrent->addRouteBottom(route);
                        next = forwardCurrent->bottom;
                        break;
                    }
                    case GCell::FromDirection::RIGHT: {
                        next = forwardCurrent->right;
                        next->addRouteLeft(route);
                        break;
                    }
                    case GCell::FromDirection::TOP: {
                        next = forwardCurrent->top;
                        next->addRouteBottom(route);
                        break;
                    }
                    default: {
                        LOG_ERROR("Unknown from direction");
                        return nullptr;
                    }
                }
                route->route.push_back(forwardCurrent);
                forwardCurrent = next;
                if (forwardCurrent == source) {
                    route->route.push_back(forwardCurrent);
                    break;
                }
            }
            std::reverse(route->route.begin(), route->route.end());
            route->route.pop_back();

            while (meetingPoint != nullptr) {
                GCell* next;
                switch (meetingPoint->fromDirectionBackward) {
                    case GCell::FromDirection::ORIGIN: {
                        break;
                    }
                    case GCell::FromDirection::LEFT: {
                        meetingPoint->addRouteLeft(route);
                        next = meetingPoint->left;
                        break;
                    }
                    case GCell::FromDirection::BOTTOM: {
                        meetingPoint->addRouteBottom(route);
                        next = meetingPoint->bottom;
                        break;
                    }
                    case GCell::FromDirection::RIGHT: {
                        next = meetingPoint->right;
                        next->addRouteLeft(route);
                        break;
                    }
                    case GCell::FromDirection::TOP: {
                        next = meetingPoint->top;
                        next->addRouteBottom(route);
                        break;
                    }
                    default: {
                        LOG_ERROR("Unknown from direction");
                        return nullptr;
                    }
                }
                route->route.push_back(meetingPoint);
                meetingPoint = next;
                if (meetingPoint == target) {
                    route->route.push_back(meetingPoint);
                    break;
                }
            }

            return route;
        }

        forwardOpenSet.erase(forwardCurrent);
        forwardClosedSet.insert(forwardCurrent);

        LOG_TRACE("Forward Current cell: (" + std::to_string(forwardCurrent->lowerLeft.x) + ", " + std::to_string(forwardCurrent->lowerLeft.y) + ")");
        if (
            forwardCurrent->left != nullptr && forwardClosedSet.find(forwardCurrent->left) == forwardClosedSet.end() &&
            forwardCurrent->fromDirectionForward != GCell::FromDirection::LEFT
        ) {
            GCell* neighbor = forwardCurrent->left;
            double tentativeGScore;
            // We are going left, so we are using M2 (Horizontal)
            switch (forwardCurrent->fromDirectionForward) {
                // M1 -> M2
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM:
                case GCell::FromDirection::TOP: {
                    tentativeGScore = forwardCurrent->gScoreForward
                                    + alphaGcellSizeX
                                    + neighbor->gammaM2
                                    - forwardCurrent->gammaM1
                                    + forwardCurrent->M1M2ViaCost;
                    if (forwardCurrent->leftEdgeCount >= forwardCurrent->leftEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M2
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = forwardCurrent->gScoreForward
                                    + alphaGcellSizeX
                                    + neighbor->gammaM2;
                    if (forwardCurrent->leftEdgeCount >= forwardCurrent->leftEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (forwardOpenSet.find(neighbor) == forwardOpenSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScoreForward) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = forwardCurrent;
                neighbor->gScoreForward = tentativeGScore;
                neighbor->hScoreForward = heuristicCustom(neighbor, target);
                neighbor->fScoreForward = neighbor->gScoreForward + neighbor->hScoreForward;
                neighbor->fromDirectionForward = GCell::FromDirection::RIGHT;
                forwardOpenSet.insert(neighbor);
                forwardOpenSetQ.push(neighbor);
            }    
        }

        if (
            forwardCurrent->bottom != nullptr && forwardClosedSet.find(forwardCurrent->bottom) == forwardClosedSet.end() &&
            forwardCurrent->fromDirectionForward != GCell::FromDirection::BOTTOM
        ) {
            GCell* neighbor = forwardCurrent->bottom;
            double tentativeGScore;
            // We are going bottom, so we are using M1 (Vertical)
            switch (forwardCurrent->fromDirectionForward) {
                // M1 -> M1
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::TOP: {
                    tentativeGScore = forwardCurrent->gScoreForward
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1;
                    if (forwardCurrent->bottomEdgeCount >= forwardCurrent->bottomEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M1
                case GCell::FromDirection::LEFT:
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = forwardCurrent->gScoreForward
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1
                                    - forwardCurrent->gammaM2
                                    + forwardCurrent->M1M2ViaCost;
                    if (forwardCurrent->bottomEdgeCount >= forwardCurrent->bottomEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (forwardOpenSet.find(neighbor) == forwardOpenSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScoreForward) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = forwardCurrent;
                neighbor->gScoreForward = tentativeGScore;
                neighbor->hScoreForward = heuristicCustom(neighbor, target);
                neighbor->fScoreForward = neighbor->gScoreForward + neighbor->hScoreForward;
                neighbor->fromDirectionForward = GCell::FromDirection::TOP;
                forwardOpenSet.insert(neighbor);
                forwardOpenSetQ.push(neighbor);
            }
        }

        if (
            forwardCurrent->right != nullptr && forwardClosedSet.find(forwardCurrent->right) == forwardClosedSet.end() &&
            forwardCurrent->fromDirectionForward != GCell::FromDirection::RIGHT
        ) {
            GCell* neighbor = forwardCurrent->right;
            double tentativeGScore;
            // We are going right, so we are using M2 (Horizontal)
            switch (forwardCurrent->fromDirectionForward) {
                // M1 -> M2
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM:
                case GCell::FromDirection::TOP: {
                    tentativeGScore = forwardCurrent->gScoreForward
                                    + alphaGcellSizeX
                                    + neighbor->gammaM2
                                    - forwardCurrent->gammaM1
                                    + forwardCurrent->M1M2ViaCost;
                    if (neighbor->leftEdgeCount >= neighbor->leftEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M2
                case GCell::FromDirection::LEFT: {
                    tentativeGScore = forwardCurrent->gScoreForward
                                    + alphaGcellSizeX
                                    + neighbor->gammaM2;
                    if (neighbor->leftEdgeCount >= neighbor->leftEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (forwardOpenSet.find(neighbor) == forwardOpenSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScoreForward) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = forwardCurrent;
                neighbor->gScoreForward = tentativeGScore;
                neighbor->hScoreForward = heuristicCustom(neighbor, target);
                neighbor->fScoreForward = neighbor->gScoreForward + neighbor->hScoreForward;
                neighbor->fromDirectionForward = GCell::FromDirection::LEFT;
                forwardOpenSet.insert(neighbor);
                forwardOpenSetQ.push(neighbor);
            }
        }

        if (
            forwardCurrent->top != nullptr && forwardClosedSet.find(forwardCurrent->top) == forwardClosedSet.end() &&
            forwardCurrent->fromDirectionForward != GCell::FromDirection::TOP
        ) {
            GCell* neighbor = forwardCurrent->top;
            double tentativeGScore;
            // We are going top, so we are using M1 (Vertical)
            switch (forwardCurrent->fromDirectionForward) {
                // M1 -> M1
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM: {
                    tentativeGScore = forwardCurrent->gScoreForward
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1;
                    if (neighbor->bottomEdgeCount >= neighbor->bottomEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M1
                case GCell::FromDirection::LEFT:
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = forwardCurrent->gScoreForward
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1
                                    - forwardCurrent->gammaM2
                                    + forwardCurrent->M1M2ViaCost;
                    if (neighbor->bottomEdgeCount >= neighbor->bottomEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (forwardOpenSet.find(neighbor) == forwardOpenSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScoreForward) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = forwardCurrent;
                neighbor->gScoreForward = tentativeGScore;
                neighbor->hScoreForward = heuristicCustom(neighbor, target);
                neighbor->fScoreForward = neighbor->gScoreForward + neighbor->hScoreForward;
                neighbor->fromDirectionForward = GCell::FromDirection::BOTTOM;
                forwardOpenSet.insert(neighbor);
                forwardOpenSetQ.push(neighbor);
            }
        }

        GCell* backwardCurrent;
        while (true) {
            backwardCurrent = backwardOpenSetQ.top();
            if (backwardOpenSet.find(backwardCurrent) != backwardOpenSet.end()) {
                backwardOpenSetQ.pop();
                break;
            }
            backwardOpenSetQ.pop();
        }

        if (forwardOpenSet.find(backwardCurrent) != forwardOpenSet.end()) {
            LOG_TRACE("Found meeting point");
            GCell* meetingPoint = backwardCurrent;
            Route* route = new Route();
            while (meetingPoint != nullptr) {
                GCell* next;
                switch (meetingPoint->fromDirectionForward) {
                    case GCell::FromDirection::ORIGIN: {
                        break;
                    }
                    case GCell::FromDirection::LEFT: {
                        meetingPoint->addRouteLeft(route);
                        next = meetingPoint->left;
                        break;
                    }
                    case GCell::FromDirection::BOTTOM: {
                        meetingPoint->addRouteBottom(route);
                        next = meetingPoint->bottom;
                        break;
                    }
                    case GCell::FromDirection::RIGHT: {
                        next = meetingPoint->right;
                        next->addRouteLeft(route);
                        break;
                    }
                    case GCell::FromDirection::TOP: {
                        next = meetingPoint->top;
                        next->addRouteBottom(route);
                        break;
                    }
                    default: {
                        LOG_ERROR("Unknown from direction");
                        return nullptr;
                    }
                }
                route->route.push_back(meetingPoint);
                meetingPoint = next;
                if (meetingPoint == source) {
                    route->route.push_back(meetingPoint);
                    break;
                }
            }
            std::reverse(route->route.begin(), route->route.end());
            route->route.pop_back();

            while (backwardCurrent != nullptr) {
                GCell* next;
                switch (backwardCurrent->fromDirectionBackward) {
                    case GCell::FromDirection::ORIGIN: {
                        break;
                    }
                    case GCell::FromDirection::LEFT: {
                        backwardCurrent->addRouteLeft(route);
                        next = backwardCurrent->left;
                        break;
                    }
                    case GCell::FromDirection::BOTTOM: {
                        backwardCurrent->addRouteBottom(route);
                        next = backwardCurrent->bottom;
                        break;
                    }
                    case GCell::FromDirection::RIGHT: {
                        next = backwardCurrent->right;
                        break;
                    }
                    case GCell::FromDirection::TOP: {
                        next = backwardCurrent->top;
                        break;
                    }
                    default: {
                        LOG_ERROR("Unknown from direction");
                        return nullptr;
                    }
                }
                route->route.push_back(backwardCurrent);
                backwardCurrent = next;
                if (backwardCurrent == target) {
                    route->route.push_back(backwardCurrent);
                    break;
                }
            }

            return route;
        }

        backwardOpenSet.erase(backwardCurrent);
        backwardClosedSet.insert(backwardCurrent);

        LOG_TRACE("Backward Current cell: (" + std::to_string(backwardCurrent->lowerLeft.x) + ", " + std::to_string(backwardCurrent->lowerLeft.y) + ")");
        if (
            backwardCurrent->left != nullptr && backwardClosedSet.find(backwardCurrent->left) == backwardClosedSet.end() &&
            backwardCurrent->fromDirectionBackward != GCell::FromDirection::LEFT
        ) {
            GCell* neighbor = backwardCurrent->left;
            double tentativeGScore;
            // We are going left, so we are using M2 (Horizontal)
            switch (backwardCurrent->fromDirectionBackward) {
                // M1 -> M2
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM:
                case GCell::FromDirection::TOP: {
                    tentativeGScore = backwardCurrent->gScoreBackward
                                    + alphaGcellSizeX
                                    + neighbor->gammaM2
                                    - backwardCurrent->gammaM1
                                    + backwardCurrent->M1M2ViaCost;
                    if (backwardCurrent->leftEdgeCount >= backwardCurrent->leftEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M2
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = backwardCurrent->gScoreBackward
                                    + alphaGcellSizeX
                                    + neighbor->gammaM2;
                    if (backwardCurrent->leftEdgeCount >= backwardCurrent->leftEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (backwardOpenSet.find(neighbor) == backwardOpenSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScoreBackward) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = backwardCurrent;
                neighbor->gScoreBackward = tentativeGScore;
                neighbor->hScoreBackward = heuristicCustom(neighbor, source);
                neighbor->fScoreBackward = neighbor->gScoreBackward + neighbor->hScoreBackward;
                neighbor->fromDirectionBackward = GCell::FromDirection::RIGHT;
                backwardOpenSet.insert(neighbor);
                backwardOpenSetQ.push(neighbor);
            }    
        }

        if (
            backwardCurrent->bottom != nullptr && backwardClosedSet.find(backwardCurrent->bottom) == backwardClosedSet.end() &&
            backwardCurrent->fromDirectionBackward != GCell::FromDirection::BOTTOM
        ) {
            GCell* neighbor = backwardCurrent->bottom;
            double tentativeGScore;
            // We are going bottom, so we are using M1 (Vertical)
            switch (backwardCurrent->fromDirectionBackward) {
                // M1 -> M1
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::TOP: {
                    tentativeGScore = backwardCurrent->gScoreBackward
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1;
                    if (backwardCurrent->bottomEdgeCount >= backwardCurrent->bottomEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M1
                case GCell::FromDirection::LEFT:
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = backwardCurrent->gScoreBackward
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1
                                    - backwardCurrent->gammaM2
                                    + backwardCurrent->M1M2ViaCost;
                    if (backwardCurrent->bottomEdgeCount >= backwardCurrent->bottomEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (backwardOpenSet.find(neighbor) == backwardOpenSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScoreBackward) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = backwardCurrent;
                neighbor->gScoreBackward = tentativeGScore;
                neighbor->hScoreBackward = heuristicCustom(neighbor, source);
                neighbor->fScoreBackward = neighbor->gScoreBackward + neighbor->hScoreBackward;
                neighbor->fromDirectionBackward = GCell::FromDirection::TOP;
                backwardOpenSet.insert(neighbor);
                backwardOpenSetQ.push(neighbor);
            }
        }

        if (
            backwardCurrent->right != nullptr && backwardClosedSet.find(backwardCurrent->right) == backwardClosedSet.end() &&
            backwardCurrent->fromDirectionBackward != GCell::FromDirection::RIGHT
        ) {
            GCell* neighbor = backwardCurrent->right;
            double tentativeGScore;
            // We are going right, so we are using M2 (Horizontal)
            switch (backwardCurrent->fromDirectionBackward) {
                // M1 -> M2
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM:
                case GCell::FromDirection::TOP: {
                    tentativeGScore = backwardCurrent->gScoreBackward
                                    + alphaGcellSizeX
                                    + neighbor->gammaM2
                                    - backwardCurrent->gammaM1
                                    + backwardCurrent->M1M2ViaCost;
                    if (neighbor->leftEdgeCount >= neighbor->leftEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M2
                case GCell::FromDirection::LEFT: {
                    tentativeGScore = backwardCurrent->gScoreBackward
                                    + alphaGcellSizeX
                                    + neighbor->gammaM2;
                    if (neighbor->leftEdgeCount >= neighbor->leftEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (backwardOpenSet.find(neighbor) == backwardOpenSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScoreBackward) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = backwardCurrent;
                neighbor->gScoreBackward = tentativeGScore;
                neighbor->hScoreBackward = heuristicCustom(neighbor, source);
                neighbor->fScoreBackward = neighbor->gScoreBackward + neighbor->hScoreBackward;
                neighbor->fromDirectionBackward = GCell::FromDirection::LEFT;
                backwardOpenSet.insert(neighbor);
                backwardOpenSetQ.push(neighbor);
            }
        }

        if (
            backwardCurrent->top != nullptr && backwardClosedSet.find(backwardCurrent->top) == backwardClosedSet.end() &&
            backwardCurrent->fromDirectionBackward != GCell::FromDirection::TOP
        ) {
            GCell* neighbor = backwardCurrent->top;
            double tentativeGScore;
            // We are going top, so we are using M1 (Vertical)
            switch (backwardCurrent->fromDirectionBackward) {
                // M1 -> M1
                case GCell::FromDirection::ORIGIN:
                case GCell::FromDirection::BOTTOM: {
                    tentativeGScore = backwardCurrent->gScoreBackward
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1;
                    if (neighbor->bottomEdgeCount >= neighbor->bottomEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                // M2 -> M1
                case GCell::FromDirection::LEFT:
                case GCell::FromDirection::RIGHT: {
                    tentativeGScore = backwardCurrent->gScoreBackward
                                    + alphaGcellSizeY
                                    + neighbor->gammaM1
                                    - backwardCurrent->gammaM2
                                    + backwardCurrent->M1M2ViaCost;
                    if (neighbor->bottomEdgeCount >= neighbor->bottomEdgeCapacity) {
                        tentativeGScore += betaHalfMaxCellCost;
                    }
                    break;
                }
                default: {
                    LOG_ERROR("Unknown from direction");
                    return nullptr;
                }
            }

            bool tentativeIsBetter = false;
            if (backwardOpenSet.find(neighbor) == backwardOpenSet.end()) {
                tentativeIsBetter = true;
            } else if (tentativeGScore < neighbor->gScoreBackward) {
                tentativeIsBetter = true;
            }

            if (tentativeIsBetter) {
                neighbor->parent = backwardCurrent;
                neighbor->gScoreBackward = tentativeGScore;
                neighbor->hScoreBackward = heuristicCustom(neighbor, source);
                neighbor->fScoreBackward = neighbor->gScoreBackward + neighbor->hScoreBackward;
                neighbor->fromDirectionBackward = GCell::FromDirection::BOTTOM;
                backwardOpenSet.insert(neighbor);
                backwardOpenSetQ.push(neighbor);
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