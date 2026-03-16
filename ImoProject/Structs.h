#pragma once
#include <vector>


using Matrix = std::vector<std::vector<int>>;
using Sequence = std::vector<int>;

struct Data {
    Matrix distances;
    std::vector<int> gains;
    int n = 0;
};

struct EvaluationResult {
    int totalDistance = 0;
    int totalGain = 0;
};

// Struktura pomocnicza do zapisu wyniku do pliki w celu późniejszej wizualizacji
struct Point {
    double x, y;
    int gain;
};

struct InsertionOptions {
    int nodeIdx = -1;
    int bestPos = -1;
    double bestCost = std::numeric_limits<double>::max();
    double secondBestCost = std::numeric_limits<double>::max();
};