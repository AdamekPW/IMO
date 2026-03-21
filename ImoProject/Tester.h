#pragma once
#include <limits>
#include <string>
#include "Structs.h"

struct Stats {
    double minVal = std::numeric_limits<double>::max();
    double maxVal = -std::numeric_limits<double>::max();
    double sumVal = 0;
    double sumPhase1Dist = 0;
    int count = 0; // Licznik prób

    void update(double objective, double phase1Dist) {
        if (objective < minVal) minVal = objective;
        if (objective > maxVal) maxVal = objective;
        sumVal += objective;
        sumPhase1Dist += phase1Dist;
        count++;
    }

    double avg() const {
        return (count > 0) ? sumVal / (double)count : 0;
    }

    double avgPhase1() const {
        return (count > 0) ? sumPhase1Dist / (double)count : 0;
    }
};

struct TableCell {
    std::string objectiveStats; // Format: "średnia (min - max)"
    double avgPhase1 = 0;
};

void RunFullExperiment(const std::vector<std::string>& fileNames);