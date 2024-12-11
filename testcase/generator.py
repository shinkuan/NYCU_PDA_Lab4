import os
import sys
import random

ROUTING_AREA_LOWERLEFT = (0, 0)
ROUTING_AREA_WIDTH_HEIGHT = (15000, 10000)
NUM_NETS = 500
GCELL_WIDTH_HEIGHT = (10, 10)
GRID_WIDTH = ROUTING_AREA_WIDTH_HEIGHT[0] // GCELL_WIDTH_HEIGHT[0]
GRID_HEIGHT = ROUTING_AREA_WIDTH_HEIGHT[1] // GCELL_WIDTH_HEIGHT[1]

CHIP1_LOWERLEFT = (400, 400)
CHIP1_WIDTH_HEIGHT = (1000, 1000)
CHIP2_LOWERLEFT = (13600, 8600)
CHIP2_WIDTH_HEIGHT = (1000, 1000)

# Costs
ALPHA = 1.1
BETA = 100
GAMMA = 1.1
DELTA = 0.7
VIA_COST = 20
ON_CHIP = 50.0
OFF_CHIP = 1.0

# Edge Capacity
EDGE_CAPACITY_MIN = 1
EDGE_CAPACITY_MAX = 3


# Returns the Manhattan distance from the point (x, y) to the nearest point on CHIP1
# 0 if the point is inside CHIP1
def diff_from_chip1(x, y):
    x1, y1 = CHIP1_LOWERLEFT
    x2, y2 = x1 + CHIP1_WIDTH_HEIGHT[0], y1 + CHIP1_WIDTH_HEIGHT[1]
    if x < x1:
        if y < y1:
            return abs(x - x1) + abs(y - y1)
        elif y > y2:
            return abs(x - x1) + abs(y - y2)
        else:
            return x1 - x
    elif x > x2:
        if y < y1:
            return abs(x - x2) + abs(y - y1)
        elif y > y2:
            return abs(x - x2) + abs(y - y2)
        else:
            return x - x2
    else:
        if y < y1:
            return y1 - y
        elif y > y2:
            return y - y2
        else:
            return 0


def diff_from_chip2(x, y):
    x1, y1 = CHIP2_LOWERLEFT
    x2, y2 = x1 + CHIP2_WIDTH_HEIGHT[0], y1 + CHIP2_WIDTH_HEIGHT[1]
    if x < x1:
        if y < y1:
            return abs(x - x1) + abs(y - y1)
        elif y > y2:
            return abs(x - x1) + abs(y - y2)
        else:
            return x1 - x
    elif x > x2:
        if y < y1:
            return abs(x - x2) + abs(y - y1)
        elif y > y2:
            return abs(x - x2) + abs(y - y2)
        else:
            return x - x2
    else:
        if y < y1:
            return y1 - y
        elif y > y2:
            return y - y2
        else:
            return 0


def generate_cst(filename: str):
    file = open(filename, "w")
    file.write(f".alpha {ALPHA}\n")
    file.write(f".beta {BETA}\n")
    file.write(f".gamma {GAMMA}\n")
    file.write(f".delta {DELTA}\n")
    file.write(f".v\n")
    file.write(f"{VIA_COST}\n")
    file.write(f".l\n")
    for i in range(GRID_HEIGHT):
        for j in range(GRID_WIDTH):
            diff1 = diff_from_chip1(j, i)
            diff2 = diff_from_chip2(j, i)
            if diff1 == 0:
                file.write(f"{ON_CHIP*random.uniform(0.9, 1.1):.2f} ")
            elif diff2 == 0:
                file.write(f"{ON_CHIP*random.uniform(0.9, 1.1):.2f} ")
            else:
                diff = min(diff1, diff2)
                cst = max(1, ON_CHIP - diff1)
                file.write(f"{cst*random.uniform(0.5, 2):.2f} ")
        file.write("\n")
    file.write(f".l\n")
    for i in range(GRID_HEIGHT):
        for j in range(GRID_WIDTH):
            diff1 = diff_from_chip1(j, i)
            diff2 = diff_from_chip2(j, i)
            if diff1 == 0:
                file.write(f"{ON_CHIP*random.uniform(0.9, 1.1):.2f} ")
            elif diff2 == 0:
                file.write(f"{ON_CHIP*random.uniform(0.9, 1.1):.2f} ")
            else:
                diff = min(diff1, diff2)
                cst = max(1, ON_CHIP - diff2)
                file.write(f"{cst*random.uniform(0.5, 2):.2f} ")
        file.write("\n")
    file.close()


def generate_gcl(filename: str):
    file = open(filename, "w")
    file.write(f".ec\n")
    for i in range(GRID_HEIGHT):
        for j in range(GRID_WIDTH):
            file.write(f"{random.randint(EDGE_CAPACITY_MIN, EDGE_CAPACITY_MAX)} {random.randint(EDGE_CAPACITY_MIN, EDGE_CAPACITY_MAX)}\n")
    file.close()


def generate_gmp(filename: str):
    file = open(filename, "w")
    file.write(f".ra\n")
    file.write(f"{ROUTING_AREA_LOWERLEFT[0]} {ROUTING_AREA_LOWERLEFT[1]} {ROUTING_AREA_WIDTH_HEIGHT[0]} {ROUTING_AREA_WIDTH_HEIGHT[1]}\n")
    file.write(f".g\n")
    file.write(f"{GCELL_WIDTH_HEIGHT[0]} {GCELL_WIDTH_HEIGHT[1]}\n")
    file.write(f".c\n")
    file.write(f"{CHIP1_LOWERLEFT[0]} {CHIP1_LOWERLEFT[1]} {CHIP1_WIDTH_HEIGHT[0]} {CHIP1_WIDTH_HEIGHT[1]}\n")
    file.write(f".b\n")
    w = CHIP1_WIDTH_HEIGHT[0] // GCELL_WIDTH_HEIGHT[0]
    h = CHIP1_WIDTH_HEIGHT[1] // GCELL_WIDTH_HEIGHT[1]
    for i in range(1, NUM_NETS + 1):
        file.write(f"{i} {random.randint(0, w)*GCELL_WIDTH_HEIGHT[0]} {random.randint(0, h)*GCELL_WIDTH_HEIGHT[1]}\n")
    file.write(f"\n")
    file.write(f".c\n")
    file.write(f"{CHIP2_LOWERLEFT[0]} {CHIP2_LOWERLEFT[1]} {CHIP2_WIDTH_HEIGHT[0]} {CHIP2_WIDTH_HEIGHT[1]}\n")
    file.write(f".b\n")
    for i in range(1, NUM_NETS + 1):
        file.write(f"{i} {random.randint(0, w)*GCELL_WIDTH_HEIGHT[0]} {random.randint(0, h)*GCELL_WIDTH_HEIGHT[1]}\n")
    file.close()


if __name__ == "__main__":
    numargs = len(sys.argv)
    if numargs != 2:
        print("Usage: python3 generator.py <testcase_folder>")
        exit(1)
    folder = sys.argv[1]
    if not os.path.exists(folder):
        os.makedirs(folder)

    generate_cst(f"{folder}/{folder}.cst")
    generate_gcl(f"{folder}/{folder}.gcl")
    generate_gmp(f"{folder}/{folder}.gmp")
    print(f"Testcase generated in {folder}")

