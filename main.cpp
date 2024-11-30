#include <iostream>
#include <chrono>
#include "common.h"
#include "evaluator.h"

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <gmp_file> <gcl_file> <cst_file> <lg_file>" << std::endl;
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();

    Evaluator router;
    router.loadGridMap(argv[1]);
    router.loadGCells(argv[2]);
    router.loadCost(argv[3]);
    router.loadLG(argv[4]);
    router.printCosts();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Elapsed time: " << elapsed.count() << "s" << std::endl;

    return 0;
}