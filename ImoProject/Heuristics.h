#pragma once

#include "Structs.h"
#include "Common.h"


Sequence PruneTour(Sequence tour, const Data& data);

Sequence GenerateRandomSolution(int totalVertices);

AlgorithmResult SolveNN(const Data& data, bool useProfit);

AlgorithmResult SolveGC(const Data& data, bool useProfit);

AlgorithmResult SolveWeighted2Regret(const Data& data, bool useProfit, double alpha = 1.0, double beta = 1.0);
