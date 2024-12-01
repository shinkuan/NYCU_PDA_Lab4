#include <iostream>
#include <chrono>
#include "common.h"
#include "router.h"
#include <cfloat>
#include <omp.h>

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <gmp_file> <gcl_file> <cst_file> <lg_file>" << std::endl;
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();

    Router router;
    router.setSeed(4176420);
    router.loadGridMap(argv[1]);
    router.loadGCells(argv[2]);
    router.loadCost(argv[3]);
    router.solve();
    router.dumpRoutes(argv[4]);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Elapsed time: " << elapsed.count() << "s" << std::endl;

    return 0;
}