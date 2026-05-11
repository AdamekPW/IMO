#pragma once
#include <limits>
#include <string>
#include "Structs.h"
#include <sstream>
#include <iomanip>
#include "zad3.h"

struct Stats {
    double minObj = std::numeric_limits<double>::max();
    double maxObj = -std::numeric_limits<double>::max();
    double sumObj = 0;

    double minP1 = std::numeric_limits<double>::max();
    double maxP1 = -std::numeric_limits<double>::max();
    double sumP1 = 0;

    int count = 0;

    void update(double objective, double phase1Dist) {
        // Statystyki dla Funkcji Celu
        if (objective < minObj) minObj = objective;
        if (objective > maxObj) maxObj = objective;
        sumObj += objective;

        // Statystyki dla Dystansu Fazy I
        if (phase1Dist < minP1) minP1 = phase1Dist;
        if (phase1Dist > maxP1) maxP1 = phase1Dist;
        sumP1 += phase1Dist;

        count++;
    }

    // Zwraca string w formacie: średnia (min - max)
    std::string formatObj() const {
        if (count == 0) return "0.00 (0 - 0)";
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << (sumObj / count)
            << " (" << (int)minObj << " - " << (int)maxObj << ")";
        return ss.str();
    }

    std::string formatP1() const {
        if (count == 0) return "0.00 (0 - 0)";
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << (sumP1 / count)
            << " (" << (int)minP1 << " - " << (int)maxP1 << ")";
        return ss.str();
    }
};

struct TableCell {
    std::string objectiveStats; // Tabela 1
    std::string phase1Stats;    // Tabela 2
};

void RunFullExperiment(const std::vector<std::string>& fileNames);


struct TestResult {
    double bestScore;
    double avgScore;
    long long avgTimeMs;
    long long maxTimeMs;
};

void RunNeighboursExperiment(const std::vector<std::string>& filePaths);

void RunTask3Experiment(const std::vector<std::string>& filePaths);

void RunTask4Experiment(const std::vector<std::string>& filePaths);