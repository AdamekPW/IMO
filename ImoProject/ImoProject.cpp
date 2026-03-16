#include "Common.h"
#include "Structs.h"
#include "Heuristics.h"

int main()
{
    Data data = LoadData("Benchmarks/TSPA.csv");

    Sequence seq_random = GenerateRandomSolution(data.n);
    SaveResultWithCoords("Results/Random_TSPA.json", "Benchmarks/TSPA.csv", seq_random, CalculateTourMetrics(seq_random, data));
    
    Sequence seq_NN_without_profit = SolveNN(data, false);
    SaveResultWithCoords("Results/NN_without_profit_TSPA.json", "Benchmarks/TSPA.csv", seq_NN_without_profit, CalculateTourMetrics(seq_NN_without_profit, data));

    Sequence seq_NN_with_profit = SolveNN(data, true);
    SaveResultWithCoords("Results/NN_with_profit_TSPA.json", "Benchmarks/TSPA.csv", seq_NN_with_profit, CalculateTourMetrics(seq_NN_with_profit, data));

    Sequence seq_GC_without_profit = SolveGC(data, false);
    SaveResultWithCoords("Results/GC_without_profit_TSPA.json", "Benchmarks/TSPA.csv", seq_GC_without_profit, CalculateTourMetrics(seq_GC_without_profit, data));

    Sequence seq_GC_with_profit = SolveGC(data, true);
    SaveResultWithCoords("Results/GC_with_profit_TSPA.json", "Benchmarks/TSPA.csv", seq_GC_with_profit, CalculateTourMetrics(seq_GC_with_profit, data));

    Sequence seq_2regret_without_profit = SolveWeighted2Regret(data, false);
    SaveResultWithCoords("Results/2regret_without_profit_TSPA.json", "Benchmarks/TSPA.csv", seq_2regret_without_profit, CalculateTourMetrics(seq_2regret_without_profit, data));

    Sequence seq_2regret_with_profit = SolveWeighted2Regret(data, true);
    SaveResultWithCoords("Results/2regret_with_profit_TSPA.json", "Benchmarks/TSPA.csv", seq_2regret_with_profit, CalculateTourMetrics(seq_2regret_with_profit, data));
}
