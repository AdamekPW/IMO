#include "Common.h"
#include "Structs.h"
#include "Tester.h"
#include "Heuristics.h"
#include <string>

int main()
{
    //RunNeighboursExperiment({ "Benchmarks/TSPA.csv", "Benchmarks/TSPB.csv" });
    RunTask3Experiment({ "Benchmarks/TSPA.csv", "Benchmarks/TSPB.csv" });
    //Data data = LoadData("Benchmarks/TSPA.csv");
    //auto res = SolveWeighted2Regret(data, true, 1.0, 0.0);
    //Sequence startTour = res.finalTour;

    //Sequence localSearchTour = LocalSearchLM(data, startTour);
    //SaveResultWithCoords("Results/localSearchTour.json", "Benchmarks/TSPA.csv", localSearchTour, CalculateTourMetrics(localSearchTour, data));
    //
    //Sequence candidateLocalSearchTour = LocalSearchCandidates(data, startTour);
    //SaveResultWithCoords("Results/candidateLocalSearchTour.json", "Benchmarks/TSPA.csv", candidateLocalSearchTour, CalculateTourMetrics(candidateLocalSearchTour, data));
    
    //AlgorithmResult seq_NN_without_profit = SolveNN(data, false);
    //SaveResultWithCoords("Results/NN_without_profit_TSPA.json", "Benchmarks/TSPA.csv", seq_NN_without_profit.finalTour, CalculateTourMetrics(seq_NN_without_profit.finalTour, data));

    //AlgorithmResult seq_NN_with_profit = SolveNN(data, true);
    //SaveResultWithCoords("Results/NN_with_profit_TSPA.json", "Benchmarks/TSPA.csv", seq_NN_with_profit.finalTour, CalculateTourMetrics(seq_NN_with_profit.finalTour, data));
}
