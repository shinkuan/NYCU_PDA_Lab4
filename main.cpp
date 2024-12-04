#include <iostream>
#include <chrono>
#include "common.h"
#include "router.h"
#include <cfloat>
#include <omp.h>
#include <random>

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <gmp_file> <gcl_file> <cst_file> <lg_file>" << std::endl;
        return 1;
    }
    // omp_set_num_threads(4);

    auto start = std::chrono::high_resolution_clock::now();

    // double bestCost = DBL_MAX;
    // double bestCostCase0 = DBL_MAX, bestCostCase1 = DBL_MAX, bestCostCase2 = DBL_MAX;
    // unsigned bestSeed = 0;

    // #pragma omp parallel for
    // for (unsigned seed = 0; seed < 10000000; ++seed) {
    //     double localBestCost = DBL_MAX;
    //     double localBestCostCase0, localBestCostCase1, localBestCostCase2;

    //     Router router0, router1, router2;

    //     router0.setSeed(seed);
    //     router0.loadGridMap("./testcase/testcase0/testcase0.gmp");
    //     router0.loadGCells("./testcase/testcase0/testcase0.gcl");
    //     router0.loadCost("./testcase/testcase0/testcase0.cst");
    //     double cost0 = router0.solve();

    //     router1.setSeed(seed);
    //     router1.loadGridMap("./testcase/testcase1/testcase1.gmp");
    //     router1.loadGCells("./testcase/testcase1/testcase1.gcl");
    //     router1.loadCost("./testcase/testcase1/testcase1.cst");
    //     double cost1 = router1.solve();

    //     router2.setSeed(seed);
    //     router2.loadGridMap("./testcase/testcase2/testcase2.gmp");
    //     router2.loadGCells("./testcase/testcase2/testcase2.gcl");
    //     router2.loadCost("./testcase/testcase2/testcase2.cst");
    //     double cost2 = router2.solve();

    //     double cost = cost0 / 1781.44 + cost1 / 6341.69 + cost2 / 136777;

    //     if (cost < localBestCost) {
    //         localBestCost = cost;
    //         localBestCostCase0 = cost0;
    //         localBestCostCase1 = cost1;
    //         localBestCostCase2 = cost2;
    //     }

    //     #pragma omp critical
    //     {
    //         if (localBestCost < bestCost) {
    //             bestCost = localBestCost;
    //             bestCostCase0 = localBestCostCase0;
    //             bestCostCase1 = localBestCostCase1;
    //             bestCostCase2 = localBestCostCase2;
    //             bestSeed = seed;

    //             std::cout << "Found better solution: " << bestCost << std::endl;
    //             std::cout << "Seed: " << bestSeed << std::endl;
    //             std::cout << "Case 0: " << bestCostCase0 << std::endl;
    //             std::cout << "Case 1: " << bestCostCase1 << std::endl;
    //             std::cout << "Case 2: " << bestCostCase2 << std::endl;
    //             std::cout << "Cost: " << bestCost << std::endl;
    //         }
    //     }
    // }

    // for (unsigned cnt = 0; cnt < 10000000; ++cnt) {
    //     // Random seed
    //     unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    //     Router router0, router1, router2;

    //     router0.setSeed(seed);
    //     router0.loadGridMap("./testcase/testcase0/testcase0.gmp");
    //     router0.loadGCells("./testcase/testcase0/testcase0.gcl");
    //     router0.loadCost("./testcase/testcase0/testcase0.cst");
    //     double cost0 = router0.solve();

    //     router1.setSeed(seed);
    //     router1.loadGridMap("./testcase/testcase1/testcase1.gmp");
    //     router1.loadGCells("./testcase/testcase1/testcase1.gcl");
    //     router1.loadCost("./testcase/testcase1/testcase1.cst");
    //     double cost1 = router1.solve();

    //     router2.setSeed(seed);
    //     router2.loadGridMap("./testcase/testcase2/testcase2.gmp");
    //     router2.loadGCells("./testcase/testcase2/testcase2.gcl");
    //     router2.loadCost("./testcase/testcase2/testcase2.cst");
    //     double cost2 = router2.solve();

    //     double cost = cost0 / 1781.44 + cost1 / 6341.69 + cost2 / 136777;
    //     if (cost < bestCost) {
    //         bestCost = cost;
    //         bestCostCase0 = cost0;
    //         bestCostCase1 = cost1;
    //         bestCostCase2 = cost2;
    //         bestSeed = seed;

    //         std::cout << "Found better solution: " << bestCost << std::endl;
    //         std::cout << "Seed: " << bestSeed << std::endl;
    //         std::cout << "Case 0: " << bestCostCase0 << std::endl;
    //         std::cout << "Case 1: " << bestCostCase1 << std::endl;
    //         std::cout << "Case 2: " << bestCostCase2 << std::endl;
    //         std::cout << "Cost: " << bestCost << std::endl;
    //     }
    // }


    Router router;
    router.setSeed(1257652952);
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