#pragma once

#include "Structs.h"
#include "Common.h"


Sequence PruneTour(Sequence tour, const Data& data);

Sequence GenerateRandomSolution(int totalVertices);

AlgorithmResult SolveNN(const Data& data, bool useProfit);

AlgorithmResult SolveGC(const Data& data, bool useProfit);

AlgorithmResult SolveWeighted2Regret(const Data& data, bool useProfit, double alpha, double beta);

// Zwraca listę wszystkich wierzchołków, których obecnie NIE MA w trasie
std::vector<int> getNodesOutside(const Sequence& tour, int totalN);

// Wykonuje fizyczną modyfikację na wektorze tour na podstawie struktury Move
void applyMove(Sequence& tour, const Move& move);

// Główna funkcja przeszukiwania lokalnego
Sequence LocalSearch(const Data& data, Sequence tour, bool steepest, Neighborhood nType);

// Random Walk - błądzenie losowe działające przez X milisekund
Sequence RandomWalk(const Data& data, Sequence tour, long long timeLimitMs, Neighborhood nType);
