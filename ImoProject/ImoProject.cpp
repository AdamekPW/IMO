#include "Common.h"
#include "Structs.h"
#include "Tester.h"
#include "Heuristics.h"
#include <string>

int main()
{
    RunFullExperiment({ "Benchmarks/TSPA.csv", "Benchmarks/TSPB.csv" });
    //Data data = LoadData("Benchmarks/TSPA.csv");

    //AlgorithmResult seq_NN_without_profit = SolveNN(data, false);
    //SaveResultWithCoords("Results/NN_without_profit_TSPA.json", "Benchmarks/TSPA.csv", seq_NN_without_profit.finalTour, CalculateTourMetrics(seq_NN_without_profit.finalTour, data));

    //AlgorithmResult seq_NN_with_profit = SolveNN(data, true);
    //SaveResultWithCoords("Results/NN_with_profit_TSPA.json", "Benchmarks/TSPA.csv", seq_NN_with_profit.finalTour, CalculateTourMetrics(seq_NN_with_profit.finalTour, data));
}
