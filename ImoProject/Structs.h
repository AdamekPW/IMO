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

struct AlgorithmResult {
    Sequence finalTour;
    double phase1Distance;
};



// --- do zadania 2 (sąsiedztwa)

enum class MoveType { Add, Remove, IntraEdge, IntraVertex };
enum class Neighborhood { Edge, Vertex };

struct Move {
    MoveType type;
    int i = -1;       // Indeks w trasie (dla Remove/Swap/2-opt/Add pos)
    int j = -1;       // Drugi indeks (dla Swap/2-opt)
    int nodeVal = -1; // Wartość wierzchołka (dla Add)
    double delta = -1e18;
};

