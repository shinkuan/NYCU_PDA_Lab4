import sys
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from pathlib import Path
from enum import Enum
from loguru import logger


class Point(object):
    def __init__(self, x: int, y: int):
        self.x = x
        self.y = y


class Bump(object):
    def __init__(self, idx: int, lower_left: Point):
        self.idx = idx
        self.lower_left = lower_left


class Visualizer(object):
    def __init__(self):
        self.routing_area_lower_left: Point = None
        self.routing_area_size: Point = None
        self.grid_size: Point = None
        self.chip1_lower_left: Point = None
        self.chip1_width_height: Point = None
        self.chip2_lower_left: Point = None
        self.chip2_width_height: Point = None
        self.chip1_bumps: list[Bump] = []
        self.chip2_bumps: list[Bump] = []
        self.costM1_map: list[list[float]] = [] # costM1_map[y][x]
        self.costM2_map: list[list[float]] = [] # costM2_map[y][x]
        self.routes: map[int, list[Point]] = {} # route_idx: [Point]

    def load_grid_map(self, filename: str):
        logger.info(f"Loading grid map from {filename}...")
        with open(filename, 'r') as f:
            lines: list[str] = f.readlines()
        
        class LoadingState(Enum):
            LOADING_COMMAND = 0
            LOADING_ROUTING_AREA = 1
            LOADING_GRID_SIZE = 2
            LOADING_CHIP1 = 3
            LOADING_CHIP2 = 4
            LOADING_CHIP1_BUMPS = 5
            LOADING_CHIP2_BUMPS = 6
        
        chip1_loaded: bool = False
        state: LoadingState = LoadingState.LOADING_COMMAND

        while lines:
            line = lines.pop(0).strip()
            if not line:
                continue
            tokens = line.split()
            
            match state:
                case LoadingState.LOADING_COMMAND:
                    if tokens[0] == ".ra":
                        state = LoadingState.LOADING_ROUTING_AREA
                    elif tokens[0] == ".g":
                        state = LoadingState.LOADING_GRID_SIZE
                    elif tokens[0] == ".c":
                        if not chip1_loaded:
                            state = LoadingState.LOADING_CHIP1
                        else:
                            state = LoadingState.LOADING_CHIP2
                    elif tokens[0] == ".b":
                        if not chip1_loaded:
                            state = LoadingState.LOADING_CHIP1_BUMPS
                            chip1_loaded = True
                        else:
                            state = LoadingState.LOADING_CHIP2_BUMPS
                case LoadingState.LOADING_ROUTING_AREA:
                    self.routing_area_lower_left = Point(int(tokens[0]), int(tokens[1]))
                    self.routing_area_size = Point(int(tokens[2]), int(tokens[3]))
                    state = LoadingState.LOADING_COMMAND
                    logger.trace(f"Routing area lower left: {self.routing_area_lower_left.x} {self.routing_area_lower_left.y}")
                    logger.trace(f"Routing area size: {self.routing_area_size.x} {self.routing_area_size.y}")
                case LoadingState.LOADING_GRID_SIZE:
                    self.grid_size = Point(int(tokens[0]), int(tokens[1]))
                    state = LoadingState.LOADING_COMMAND
                    logger.trace(f"Grid size: {self.grid_size.x} {self.grid_size.y}")
                case LoadingState.LOADING_CHIP1:
                    self.chip1_lower_left = Point(int(tokens[0]) + self.routing_area_lower_left.x, int(tokens[1]) + self.routing_area_lower_left.y)
                    self.chip1_width_height = Point(int(tokens[2]), int(tokens[3]))
                    state = LoadingState.LOADING_COMMAND
                    logger.trace(f"Chip1 lower left: {self.chip1_lower_left.x} {self.chip1_lower_left.y}")
                    logger.trace(f"Chip1 width height: {self.chip1_width_height.x} {self.chip1_width_height.y}")
                case LoadingState.LOADING_CHIP2:
                    self.chip2_lower_left = Point(int(tokens[0]) + self.routing_area_lower_left.x, int(tokens[1]) + self.routing_area_lower_left.y)
                    self.chip2_width_height = Point(int(tokens[2]), int(tokens[3]))
                    state = LoadingState.LOADING_COMMAND
                    logger.trace(f"Chip2 lower left: {self.chip2_lower_left.x} {self.chip2_lower_left.y}")
                    logger.trace(f"Chip2 width height: {self.chip2_width_height.x} {self.chip2_width_height.y}")
                case LoadingState.LOADING_CHIP1_BUMPS:
                    if tokens[0].startswith("."):
                        lines.insert(0, line)
                        state = LoadingState.LOADING_COMMAND
                        continue
                    self.chip1_bumps.append(Bump(int(tokens[0]), Point(int(tokens[1]) + self.chip1_lower_left.x, int(tokens[2]) + self.chip1_lower_left.y)))
                    state = LoadingState.LOADING_CHIP1_BUMPS
                    logger.trace(f"Chip1 bump {self.chip1_bumps[-1].idx}: {self.chip1_bumps[-1].lower_left.x} {self.chip1_bumps[-1].lower_left.y}")
                case LoadingState.LOADING_CHIP2_BUMPS:
                    if tokens[0].startswith("."):
                        lines.insert(0, line)
                        state = LoadingState.LOADING_COMMAND
                        continue
                    self.chip2_bumps.append(Bump(int(tokens[0]), Point(int(tokens[1]) + self.chip2_lower_left.x, int(tokens[2]) + self.chip2_lower_left.y)))
                    state = LoadingState.LOADING_CHIP2_BUMPS
                    logger.trace(f"Chip2 bump {self.chip2_bumps[-1].idx}: {self.chip2_bumps[-1].lower_left.x} {self.chip2_bumps[-1].lower_left.y}")
                case _:
                    logger.error(f"Invalid state: {state}")
                    sys.exit(1)

    def load_gcells(self, filename: str):
        logger.info(f"Loading global cells from {filename}...")
        with open(filename, 'r') as f:
            lines: list[str] = f.readlines()

        class LoadingState(Enum):
            LOADING_COMMAND = 0
            LOADING_EDGE_CAPACITY = 1
        # TODO
        pass

    def load_costs(self, filename: str):
        logger.info(f"Loading costs from {filename}...")
        with open(filename, 'r') as f:
            lines: list[str] = f.readlines()
        
        class LoadingState(Enum):
            LOADING_COMMAND = 0
            LOADING_VIA_COST = 1
            LOADING_COST_M1 = 2
            LOADING_COST_M2 = 3

        m1_loaded: bool = False
        state: LoadingState = LoadingState.LOADING_COMMAND
        while lines:
            line = lines.pop(0).strip()
            if not line:
                continue
            tokens = line.split()

            match state:
                case LoadingState.LOADING_COMMAND:
                    if tokens[0] == ".alpha":
                        state = LoadingState.LOADING_COMMAND
                    elif tokens[0] == ".beta":
                        state = LoadingState.LOADING_COMMAND
                    elif tokens[0] == ".gamma":
                        state = LoadingState.LOADING_COMMAND
                    elif tokens[0] == ".delta":
                        state = LoadingState.LOADING_COMMAND
                    elif tokens[0] == ".v":
                        state = LoadingState.LOADING_VIA_COST
                    elif tokens[0] == ".l":
                        if not m1_loaded:
                            state = LoadingState.LOADING_COST_M1
                            m1_loaded = True
                        else:
                            state = LoadingState.LOADING_COST_M2
                case LoadingState.LOADING_VIA_COST:
                    # TODO
                    state = LoadingState.LOADING_COMMAND
                case LoadingState.LOADING_COST_M1:
                    if tokens[0].startswith("."):
                        lines.insert(0, line)
                        state = LoadingState.LOADING_COMMAND
                        continue
                    self.costM1_map.append([float(x) for x in tokens])
                    state = LoadingState.LOADING_COST_M1
                case LoadingState.LOADING_COST_M2:
                    if tokens[0].startswith("."):
                        lines.insert(0, line)
                        state = LoadingState.LOADING_COMMAND
                        continue
                    self.costM2_map.append([float(x) for x in tokens])
                    state = LoadingState.LOADING_COST_M2
                case _:
                    logger.error(f"Invalid state: {state}")
                    sys.exit(1)

    def load_lg(self, filename: str):
        logger.info(f"Loading global routing from {filename}...")
        with open(filename, 'r') as f:
            lines: list[str] = f.readlines()

        for bump in self.chip1_bumps:
            self.routes[bump.idx] = [bump.lower_left]

        class LoadingState(Enum):
            LOADING_COMMAND = 0
            LOADING_ROUTE = 1
        
        route_idx_loading: int = -1
        state: LoadingState = LoadingState.LOADING_COMMAND
        while lines:
            line = lines.pop(0).strip()
            if not line:
                continue
            tokens = line.split()

            match state:
                case LoadingState.LOADING_COMMAND:
                    if tokens[0].startswith("n"):
                        route_idx_loading = int(tokens[0][1:])
                        state = LoadingState.LOADING_ROUTE
                    else:
                        logger.error(f"Invalid command: {tokens[0]}")
                        sys.exit(1)
                case LoadingState.LOADING_ROUTE:
                    if tokens[0].startswith(".end"):
                        state = LoadingState.LOADING_COMMAND
                        continue
                    elif tokens[0] == "via":
                        continue
                    self.routes[route_idx_loading].append(Point(int(tokens[3]), int(tokens[4])))

    def draw_placement(self, filename):
        x_scale = self.grid_size.x
        y_scale = self.grid_size.y
        plt.figure(figsize=(10, 10))
        
        plt.xlim(self.routing_area_lower_left.x, 
                 self.routing_area_lower_left.x + self.routing_area_size.x)
        plt.ylim(self.routing_area_lower_left.y, 
                 self.routing_area_lower_left.y + self.routing_area_size.y)
        
        chip1_patch = patches.Rectangle(
            (self.chip1_lower_left.x, self.chip1_lower_left.y), 
            self.chip1_width_height.x, 
            self.chip1_width_height.y, 
            facecolor='yellow', 
            alpha=0.2, 
            edgecolor='yellow'
        )
        plt.gca().add_patch(chip1_patch)
        
        chip2_patch = patches.Rectangle(
            (self.chip2_lower_left.x, self.chip2_lower_left.y), 
            self.chip2_width_height.x, 
            self.chip2_width_height.y, 
            facecolor='orange', 
            alpha=0.2, 
            edgecolor='orange'
        )
        plt.gca().add_patch(chip2_patch)
        
        for bump in self.chip1_bumps:
            plt.plot(bump.lower_left.x, bump.lower_left.y, 
                     marker='o', markersize=10, color='blue')
        
        for bump in self.chip2_bumps:
            plt.plot(bump.lower_left.x, bump.lower_left.y, 
                     marker='o', markersize=10, color='green')
        
        color_cycle = ['purple', 'orange', 'brown', 'pink', 'gray', 'cyan']
        for i, route in enumerate(self.routes.values()):
            color = color_cycle[i % len(color_cycle)]
            route_x = [point.x for point in route]
            route_y = [point.y for point in route]
            plt.plot(route_x, route_y, color=color, linewidth=2)
        
        plt.title('Chip Placement and Routing')
        plt.xlabel('X coordinate')
        plt.ylabel('Y coordinate')
        
        plt.gca().set_aspect('equal', adjustable='box')
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        plt.close()

if __name__ == '__main__':
    if len(sys.argv) < 5:
        # print("Usage: python visualizer.py <gmp_file> <gcl_file> <cst_file> <lg_file>")
        logger.error("Usage: python visualizer.py <gmp_file> <gcl_file> <cst_file> <lg_file>")
        sys.exit(1)

    gmp_file = sys.argv[1]
    gcl_file = sys.argv[2]
    cst_file = sys.argv[3]
    lg_file = sys.argv[4]

    logger.info("Starting...")
    visualizer = Visualizer()
    visualizer.load_grid_map(gmp_file)
    visualizer.load_gcells(gcl_file)
    visualizer.load_costs(cst_file)
    visualizer.load_lg(lg_file)

    # Strip the extension
    gmp_file = Path(gmp_file).stem
    visualizer.draw_placement(f"{gmp_file}_placement.png")
