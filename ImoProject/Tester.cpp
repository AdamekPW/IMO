#include "Tester.h"
#include "Common.h"
#include "Heuristics.h"

#include <iostream>
#include <map>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <chrono>
#include <numeric>

void RunFullExperiment(const std::vector<std::string>& filePaths) {
    std::ofstream csvFile("tabela_wynikow.csv");
    if (!csvFile.is_open()) {
        std::cerr << "Nie udalo sie otworzyc pliku do zapisu!" << std::endl;
        return;
    }

    std::vector<std::pair<std::string, std::string>> methods = {
        {"Losowy", "RAND"},
        {"NN bez zysku", "NNa"},
        {"NN z zyskiem", "NNp"},
        {"GC bez zysku", "GCa"},
        {"GC z zyskiem", "GCp"},
        {"2-zal bez zysku", "2Ra"},
        {"2-zal z zyskiem", "2Rp"},
        {"2-zal wazony bez zysku", "2RaW"},
        {"2-zal wazony z zyskiem", "2RpW"}
    };

    std::map<std::string, std::map<std::string, TableCell>> tableData;
    std::vector<std::string> instanceNames;

    for (const auto& path : filePaths) {
        Data data = LoadData(path);
        std::string fileNameOnly = path.substr(path.find_last_of("/\\") + 1);
        instanceNames.push_back(fileNameOnly);

        std::cout << "\n>>> Przetwarzanie instancji: " << fileNameOnly << " <<<" << std::endl;

        for (const auto& [prettyName, id] : methods) {
            Stats stats;

            Sequence bestTourFound;
            EvaluationResult bestMetricsFound;
            double bestObjectiveValue = -std::numeric_limits<double>::max();

            std::cout << "  Algorytm: " << std::left << std::setw(25) << prettyName << " [";

            // Wykonujemy 200 prób dla każdego algorytmu
            for (int i = 0; i < 200; ++i) {
                AlgorithmResult res;
                if (id == "RAND") {
                    Sequence s = GenerateRandomSolution(data.n);
                    res = { s, 0.0 }; // Dla losowego dystans Fazy I to 0
                }
                else if (id == "NNa") res = SolveNN(data, false);
                else if (id == "NNp") res = SolveNN(data, true);
                else if (id == "GCa") res = SolveGC(data, false);
                else if (id == "GCp") res = SolveGC(data, true);
                else if (id == "2Ra") res = SolveWeighted2Regret(data, false, 1.0, 0.0);
                else if (id == "2Rp") res = SolveWeighted2Regret(data, true, 1.0, 0.0);
                else if (id == "2RaW") res = SolveWeighted2Regret(data, false, 1.0, -1.0);
                else if (id == "2RpW") res = SolveWeighted2Regret(data, true, 1.0, -1.0);

                EvaluationResult currentMetrics = CalculateTourMetrics(res.finalTour, data);
                double currentObjective = (double)currentMetrics.totalGain - currentMetrics.totalDistance;

                // Aktualizujemy statystyki
                stats.update(currentObjective, (double)res.phase1Distance);

                // Szukanie najlepszego do zapisu JSON
                if (currentObjective > bestObjectiveValue) {
                    bestObjectiveValue = currentObjective;
                    bestTourFound = res.finalTour;
                    bestMetricsFound = currentMetrics;
                }

                if (i % 40 == 0) std::cout << ".";
            }
            std::cout << "] Gotowe!" << std::endl;

            // Zapis najlepszego wyniku do folderu Results
            std::string jsonName = "Results/Best_" + id + "_" + fileNameOnly + ".json";
            SaveResultWithCoords(jsonName, path, bestTourFound, bestMetricsFound);

            // Zapisujemy sformatowane wyniki do tabeli
            tableData[prettyName][fileNameOnly] = { stats.formatObj(), stats.formatP1() };
        }
    }

    // --- TABELA 1: FUNKCJA CELU ---
    csvFile << "TABELA 1: Statystyki funkcji celu; \nMetoda;";
    for (const auto& name : instanceNames) csvFile << name << ";";
    csvFile << "\n";

    for (const auto& [prettyName, id] : methods) {
        csvFile << prettyName << ";";
        for (const auto& instName : instanceNames) {
            csvFile << tableData[prettyName][instName].objectiveStats << ";";
        }
        csvFile << "\n";
    }

    // --- TABELA 2: DYSTANS FAZA I ---
    csvFile << "\n\nTABELA 2: Dlugosc sciezki po I fazie;\nMetoda;";
    for (const auto& name : instanceNames) csvFile << name << ";";
    csvFile << "\n";

    for (const auto& [prettyName, id] : methods) {
        csvFile << prettyName << ";";
        for (const auto& instName : instanceNames) {
            // TUTAJ POPRAWIONE: Użycie phase1Stats
            csvFile << tableData[prettyName][instName].phase1Stats << ";";
        }
        csvFile << "\n";
    }

    csvFile.close();
    std::cout << "\nEksperyment zakonczony. Wyniki zbiorcze: 'tabela_wynikow.csv'" << std::endl;
    std::cout << "Najlepsze trasy zapisano w folderze 'Results/'." << std::endl;
}


double initialEvaluate(const std::vector<int>& tour, const Data& data) {
    EvaluationResult initScore = CalculateTourMetrics(tour, data);
    return initScore.totalGain - initScore.totalDistance;
}

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <iomanip>
#include <map>

// Pomocnicza struktura do przechowywania statystyk
struct ExperimentStats {
    std::vector<double> scores;
    std::vector<long long> times;

    void add(double score, long long time) {
        scores.push_back(score);
        times.push_back(time);
    }

    std::string formatScore() const {
        if (scores.empty()) return "N/A";
        auto [minIt, maxIt] = std::minmax_element(scores.begin(), scores.end());
        double avg = std::accumulate(scores.begin(), scores.end(), 0.0) / scores.size();
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << avg << " (" << *minIt << " - " << *maxIt << ")";
        return ss.str();
    }

    std::string formatTime() const {
        if (times.empty()) return "N/A";
        auto [minIt, maxIt] = std::minmax_element(times.begin(), times.end());
        double avg = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << avg << " (" << *minIt << " - " << *maxIt << ")";
        return ss.str();
    }

    double getAvgTime() const {
        if (times.empty()) return 0;
        return std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    }
};

void RunNeighboursExperiment(const std::vector<std::string>& filePaths) {
    std::ofstream csvFile("eksperyment_lokalne_szukanie.csv");
    if (!csvFile.is_open()) {
        std::cerr << "Błąd otwarcia pliku CSV!" << std::endl;
        return;
    }

    struct Config {
        bool steepest;
        Neighborhood nType;
        bool heuristicStart;
        std::string id;
        std::string prettyName;
    };

    std::vector<Config> configs = {
        {false, Neighborhood::Vertex, false, "GV_R", "Greedy Vertex (Rand)"},
        {false, Neighborhood::Edge,   false, "GE_R", "Greedy Edge (Rand)"},
        {true,  Neighborhood::Vertex, false, "SV_R", "Steepest Vertex (Rand)"},
        {true,  Neighborhood::Edge,   false, "SE_R", "Steepest Edge (Rand)"},
        {false, Neighborhood::Vertex, true,  "GV_H", "Greedy Vertex (Heur)"},
        {false, Neighborhood::Edge,   true,  "GE_H", "Greedy Edge (Heur)"},
        {true,  Neighborhood::Vertex, true,  "SV_H", "Steepest Vertex (Heur)"},
        {true,  Neighborhood::Edge,   true,  "SE_H", "Steepest Edge (Heur)"}
    };

    std::map<std::string, std::map<std::string, ExperimentStats>> resultsTable;
    std::vector<std::string> instanceNames;

    for (const auto& path : filePaths) {
        Data data = LoadData(path);
        std::string fileName = path.substr(path.find_last_of("/\\") + 1);
        instanceNames.push_back(fileName);

        std::cout << "\n>>> Instancja: " << fileName << " <<<" << std::endl;

        long long slowestAvgTime = 0;

        // 1. PĘTLA DLA 8 KONFIGURACJI LOCAL SEARCH
        for (const auto& cfg : configs) {
            std::cout << "  Algorytm: " << std::left << std::setw(25) << cfg.prettyName << " [";

            Sequence bestTourForCfg;
            EvaluationResult bestMetricsForCfg;
            double bestValueForCfg = -std::numeric_limits<double>::max();

            for (int i = 0; i < 100; ++i) {
                Sequence startTour;
                if (cfg.heuristicStart) {
                    auto res = SolveWeighted2Regret(data, true, 1.0, 0.0);
                    startTour = res.finalTour;
                }
                else {
                    startTour = GenerateRandomSolution(data.n);
                }

                auto t1 = std::chrono::high_resolution_clock::now();
                Sequence finalTour = LocalSearch(data, startTour, cfg.steepest, cfg.nType);
                auto t2 = std::chrono::high_resolution_clock::now();

                long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
                EvaluationResult currentMetrics = CalculateTourMetrics(finalTour, data);
                double score = (double)currentMetrics.totalGain - currentMetrics.totalDistance;

                resultsTable[cfg.prettyName][fileName].add(score, duration);

                // Szukamy najlepszego wyniku w obrębie tych 100 prób danej konfiguracji
                if (score > bestValueForCfg) {
                    bestValueForCfg = score;
                    bestTourForCfg = finalTour;
                    bestMetricsForCfg = currentMetrics;
                }
                if (i % 20 == 0) std::cout << ".";
            }
            std::cout << "] Gotowe!" << std::endl;

            // Zapis najlepszego wyniku dla tej konkretnej konfiguracji
            std::string jsonName = "Results/Best_" + cfg.id + "_" + fileName + ".json";
            SaveResultWithCoords(jsonName, path, bestTourForCfg, bestMetricsForCfg);

            slowestAvgTime = std::max(slowestAvgTime, (long long)resultsTable[cfg.prettyName][fileName].getAvgTime());
        }

        // 2. RANDOM WALK (Baseline)
        std::cout << "  Algorytm: " << std::left << std::setw(25) << "Random Walk" << " [";
        Sequence bestTourRW;
        EvaluationResult bestMetricsRW;
        double bestValueRW = -std::numeric_limits<double>::max();

        for (int i = 0; i < 100; ++i) {
            Sequence startTour = GenerateRandomSolution(data.n);
            auto t1 = std::chrono::high_resolution_clock::now();
            Sequence rwTour = RandomWalk(data, startTour, slowestAvgTime, Neighborhood::Edge);
            auto t2 = std::chrono::high_resolution_clock::now();

            long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            EvaluationResult currentMetrics = CalculateTourMetrics(rwTour, data);
            double score = (double)currentMetrics.totalGain - currentMetrics.totalDistance;

            resultsTable["Random Walk"][fileName].add(score, duration);

            if (score > bestValueRW) {
                bestValueRW = score;
                bestTourRW = rwTour;
                bestMetricsRW = currentMetrics;
            }
            if (i % 20 == 0) std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_RW_" + fileName + ".json", path, bestTourRW, bestMetricsRW);

        // 3. POPRZEDNIA HEURYSTYKA (Baseline)
        std::cout << "  Algorytm: " << std::left << std::setw(25) << "Weighted 2-Regret" << " [";
        Sequence bestTourHeur;
        EvaluationResult bestMetricsHeur;
        double bestValueHeur = -std::numeric_limits<double>::max();

        for (int i = 0; i < 100; ++i) {
            auto t1 = std::chrono::high_resolution_clock::now();
            auto res = SolveWeighted2Regret(data, true, 1.0, 0.0);
            auto t2 = std::chrono::high_resolution_clock::now();

            long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            EvaluationResult currentMetrics = CalculateTourMetrics(res.finalTour, data);
            double score = (double)currentMetrics.totalGain - currentMetrics.totalDistance;

            resultsTable["Heurystyka (Poprzednia)"][fileName].add(score, duration);

            if (score > bestValueHeur) {
                bestValueHeur = score;
                bestTourHeur = res.finalTour;
                bestMetricsHeur = currentMetrics;
            }
            if (i % 20 == 0) std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_Heur_" + fileName + ".json", path, bestTourHeur, bestMetricsHeur);
    }

    // --- GENEROWANIE TABEL W PLIKU CSV ---
    csvFile << "TABELA 1: Statystyki funkcji celu (Zysk - Dystans); \nMetoda;";
    for (const auto& name : instanceNames) csvFile << name << ";";
    csvFile << "\n";

    auto printRows = [&](bool isTime) {
        std::vector<std::string> order;
        for (auto const& cfg : configs) order.push_back(cfg.prettyName);
        order.push_back("Random Walk");
        order.push_back("Heurystyka (Poprzednia)");

        for (const auto& mName : order) {
            csvFile << mName << ";";
            for (const auto& instName : instanceNames) {
                if (isTime) csvFile << resultsTable[mName][instName].formatTime() << ";";
                else csvFile << resultsTable[mName][instName].formatScore() << ";";
            }
            csvFile << "\n";
        }
        };

    printRows(false); // Wyniki jakościowe
    csvFile << "\n\nTABELA 2: Statystyki czasu obliczeń (ms);\nMetoda;";
    for (const auto& name : instanceNames) csvFile << name << ";";
    csvFile << "\n";
    printRows(true); // Wyniki czasowe

    csvFile.close();
    std::cout << "\nEksperyment zakończony. Wyniki zbiorcze: 'eksperyment_lokalne_szukanie.csv'" << std::endl;
}

void RunTask3Experiment(const std::vector<std::string>& filePaths) {
    std::ofstream csvFile("eksperyment_zadanie3.csv");
    if (!csvFile.is_open()) {
        std::cerr << "Błąd otwarcia pliku CSV!" << std::endl;
        return;
    }

    struct Config {
        std::string id;
        std::string prettyName;
    };

    std::vector<Config> configs = {
        {"LM",         "Steepest + Move List (LM)"},
        {"Candidates", "Steepest + Candidates"},
        {"Standard",   "Steepest Standard (Base)"},
        {"Heuristic",  "Heuristic Task 1 (Base)"}
    };

    std::map<std::string, std::map<std::string, ExperimentStats>> resultsTable;
    std::vector<std::string> instanceNames;

    for (const auto& path : filePaths) {
        Data data = LoadData(path);
        std::string fileName = path.substr(path.find_last_of("/\\") + 1);
        instanceNames.push_back(fileName);

        // kandydaci liczeni raz na instancję
        auto nearest_neighbors = build_candidate_edges(data, 10);

        std::cout << "\n>>> Instancja: " << fileName << " <<<" << std::endl;

        for (const auto& cfg : configs) {
            std::cout << "  Algorytm: " << std::left << std::setw(30)
                << cfg.prettyName << " [";

            Sequence bestTourForCfg;
            EvaluationResult bestMetricsForCfg{};
            double bestValueForCfg = -std::numeric_limits<double>::max();

            for (int i = 0; i < 100; ++i) {
                Sequence currentTour;
                long long duration = 0;

                auto t1 = std::chrono::high_resolution_clock::now();

                // Wybór odpowiedniego algorytmu
                if (cfg.id == "LM") {
                    Sequence start = GenerateRandomSolution(data.n);
                    currentTour = LocalSearchLMOnly(data, start);
                }
                else if (cfg.id == "Candidates") {
                    Sequence start = GenerateRandomSolution(data.n);
                    currentTour = LocalSearchCandidates(data, start, nearest_neighbors);
                }
                else if (cfg.id == "Standard") {
                    Sequence start = GenerateRandomSolution(data.n);
                    currentTour = LocalSearch(data, start, true, Neighborhood::Edge);
                }
                else if (cfg.id == "Heuristic") {
                    auto res = SolveWeighted2Regret(data, true, 1.0, 0.0);
                    currentTour = res.finalTour;
                }

                auto t2 = std::chrono::high_resolution_clock::now();
                duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

                EvaluationResult metrics = CalculateTourMetrics(currentTour, data);
                double score = (double)metrics.totalGain - metrics.totalDistance;

                // aktualizacja statystyk
                resultsTable[cfg.prettyName][fileName].add(score, duration);

                // zapamiętaj najlepszą trasę dla tej metody i tej instancji
                if (score > bestValueForCfg) {
                    bestValueForCfg = score;
                    bestTourForCfg = currentTour;
                    bestMetricsForCfg = metrics;
                }

                if (i % 20 == 0) std::cout << ".";
            }

            std::cout << "] Gotowe!" << std::endl;

            // zapis najlepszego wyniku do JSON
            std::string jsonName = "Results/Best_" + cfg.id + "_" + fileName + ".json";
            SaveResultWithCoords(jsonName, path, bestTourForCfg, bestMetricsForCfg);
        }
    }

    auto printTable = [&](const std::string& title, bool isTime) {
        csvFile << title << ";\nMetoda;";
        for (const auto& name : instanceNames) {
            csvFile << name << ";";
        }
        csvFile << "\n";

        for (const auto& cfg : configs) {
            csvFile << cfg.prettyName << ";";
            for (const auto& instName : instanceNames) {
                if (isTime) {
                    csvFile << resultsTable[cfg.prettyName][instName].formatTime() << ";";
                }
                else {
                    csvFile << resultsTable[cfg.prettyName][instName].formatScore() << ";";
                }
            }
            csvFile << "\n";
        }
        csvFile << "\n";
        };

    printTable("TABELA 1: Statystyki funkcji celu (Zysk - Dystans)", false);
    printTable("TABELA 2: Statystyki czasu obliczeń (ms)", true);

    csvFile.close();
    std::cout << "\nEksperyment zakończony. Wyniki: 'eksperyment_zadanie3.csv'" << std::endl;
}