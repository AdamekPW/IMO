#include "Tester.h"
#include "Common.h"
#include "Heuristics.h"
#include "zad4.h"
#include "zad5.h"
#include "zad6.h"
#include "zad7.h"

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
#include <unordered_set>
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


// =====================================================================
// ZADANIE 4 - Eksperyment: MSLS, ILS, LNS, LNSa
// =====================================================================
// Schemat:
//  - Dla kazdej instancji: 20 uruchomien MSLS (po 200 iteracji LS),
//    statystyki + zapis najlepszej trasy.
//  - Sredni czas pojedynczego MSLS = warunek stopu dla ILS/LNS/LNSa.
//  - Po 20 uruchomien ILS, LNS, LNSa - statystyki + zapis najlepszej trasy.
//  - Trzy tabele CSV: funkcja celu, czas, liczba iteracji (perturbacji).
//
// Dla MSLS "liczba iteracji" = liczba przebiegow LS (zawsze 200).
// Dla ILS/LNS/LNSa "liczba iteracji" = liczba wykonanych perturbacji.
// =====================================================================

// Pomocnicze formatowanie statystyki "srednia (min - max)" dla wektora intow
static std::string formatIters(const std::vector<int>& iters) {
    if (iters.empty()) return "N/A";
    auto [minIt, maxIt] = std::minmax_element(iters.begin(), iters.end());
    double avg = std::accumulate(iters.begin(), iters.end(), 0.0) / iters.size();
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << avg << " (" << *minIt << " - " << *maxIt << ")";
    return ss.str();
}

void RunTask4Experiment(const std::vector<std::string>& filePaths) {
    std::ofstream csvFile("eksperyment_zadanie4.csv");
    if (!csvFile.is_open()) {
        std::cerr << "Blad otwarcia pliku CSV!" << std::endl;
        return;
    }

    struct Config {
        std::string id;
        std::string prettyName;
    };

    std::vector<Config> configs = {
        {"MSLS", "MSLS (Multiple Start LS)"},
        {"ILS",  "ILS (Iterated LS)"},
        {"LNS",  "LNS (Destroy-Repair + LS)"},
        {"LNSa", "LNSa (Destroy-Repair bez LS)"}
    };

    // [metoda][instancja] -> stat (wynik + czas)
    std::map<std::string, std::map<std::string, ExperimentStats>> resultsTable;
    // [metoda][instancja] -> wektor liczb iteracji (osobno, bo ExperimentStats trzyma czas a nie iteracje)
    std::map<std::string, std::map<std::string, std::vector<int>>> iterationsTable;
    std::vector<std::string> instanceNames;

    constexpr int RUNS = 20;          // liczba uruchomien kazdej metody
    constexpr int MSLS_ITERS = 200;   // liczba iteracji LS w jednym MSLS

    for (const auto& path : filePaths) {
        Data data = LoadData(path);
        std::string fileName = path.substr(path.find_last_of("/\\") + 1);
        instanceNames.push_back(fileName);

        std::cout << "\n>>> Instancja: " << fileName << " <<<" << std::endl;

        // --------------------------------------------------------- MSLS
        std::cout << "  Algorytm: " << std::left << std::setw(32)
                  << "MSLS (200 iter LS)" << " [";

        Sequence bestTourMSLS;
        EvaluationResult bestMetricsMSLS{};
        double bestValueMSLS = -std::numeric_limits<double>::max();
        long long sumMSLSTime = 0;

        for (int run = 0; run < RUNS; ++run) {
            MSLSResult res = MSLS(data, MSLS_ITERS);

            resultsTable["MSLS (Multiple Start LS)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["MSLS (Multiple Start LS)"][fileName].push_back(res.iterations);
            sumMSLSTime += res.elapsedMs;

            if (res.bestScore > bestValueMSLS) {
                bestValueMSLS = res.bestScore;
                bestTourMSLS = res.bestTour;
                bestMetricsMSLS = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;

        long long avgMSLSTime = sumMSLSTime / RUNS;
        std::cout << "  >> Sredni czas MSLS: " << avgMSLSTime
                  << " ms (limit dla ILS/LNS/LNSa)" << std::endl;

        SaveResultWithCoords("Results/Best_MSLS_" + fileName + ".json",
                             path, bestTourMSLS, bestMetricsMSLS);

        // --------------------------------------------------------- ILS
        std::cout << "  Algorytm: " << std::left << std::setw(32)
                  << "ILS" << " [";

        Sequence bestTourILS;
        EvaluationResult bestMetricsILS{};
        double bestValueILS = -std::numeric_limits<double>::max();

        for (int run = 0; run < RUNS; ++run) {
            ILSResult res = ILS(data, avgMSLSTime, 4);

            resultsTable["ILS (Iterated LS)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["ILS (Iterated LS)"][fileName].push_back(res.perturbations);

            if (res.bestScore > bestValueILS) {
                bestValueILS = res.bestScore;
                bestTourILS = res.bestTour;
                bestMetricsILS = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;

        SaveResultWithCoords("Results/Best_ILS_" + fileName + ".json",
                             path, bestTourILS, bestMetricsILS);

        // --------------------------------------------------------- LNS
        std::cout << "  Algorytm: " << std::left << std::setw(32)
                  << "LNS (Destroy heur. + LS)" << " [";

        Sequence bestTourLNS;
        EvaluationResult bestMetricsLNS{};
        double bestValueLNS = -std::numeric_limits<double>::max();

        for (int run = 0; run < RUNS; ++run) {
            ILSResult res = LNS(data, avgMSLSTime, 0.30, DestroyStrategy::Segment);

            resultsTable["LNS (Destroy-Repair + LS)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["LNS (Destroy-Repair + LS)"][fileName].push_back(res.perturbations);

            if (res.bestScore > bestValueLNS) {
                bestValueLNS = res.bestScore;
                bestTourLNS = res.bestTour;
                bestMetricsLNS = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;

        SaveResultWithCoords("Results/Best_LNS_" + fileName + ".json",
                             path, bestTourLNS, bestMetricsLNS);

        // --------------------------------------------------------- LNSa
        std::cout << "  Algorytm: " << std::left << std::setw(32)
                  << "LNSa (Destroy heur. bez LS)" << " [";

        Sequence bestTourLNSa;
        EvaluationResult bestMetricsLNSa{};
        double bestValueLNSa = -std::numeric_limits<double>::max();

        for (int run = 0; run < RUNS; ++run) {
            ILSResult res = LNSa(data, avgMSLSTime, 0.30, DestroyStrategy::Segment);

            resultsTable["LNSa (Destroy-Repair bez LS)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["LNSa (Destroy-Repair bez LS)"][fileName].push_back(res.perturbations);

            if (res.bestScore > bestValueLNSa) {
                bestValueLNSa = res.bestScore;
                bestTourLNSa = res.bestTour;
                bestMetricsLNSa = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;

        SaveResultWithCoords("Results/Best_LNSa_" + fileName + ".json",
                             path, bestTourLNSa, bestMetricsLNSa);
    }

    // ==================== TABELE CSV ====================
    enum TableMode { SCORE = 0, TIME = 1, ITERS = 2 };

    auto printTable = [&](const std::string& title, TableMode mode) {
        csvFile << title << ";\nMetoda;";
        for (const auto& name : instanceNames) csvFile << name << ";";
        csvFile << "\n";

        for (const auto& cfg : configs) {
            csvFile << cfg.prettyName << ";";
            for (const auto& instName : instanceNames) {
                if (mode == SCORE) {
                    csvFile << resultsTable[cfg.prettyName][instName].formatScore() << ";";
                } else if (mode == TIME) {
                    csvFile << resultsTable[cfg.prettyName][instName].formatTime() << ";";
                } else { // ITERS
                    csvFile << formatIters(iterationsTable[cfg.prettyName][instName]) << ";";
                }
            }
            csvFile << "\n";
        }
        csvFile << "\n";
    };

    printTable("TABELA 1: Statystyki funkcji celu (Zysk - Dystans)", SCORE);
    printTable("TABELA 2: Statystyki czasu obliczen jednego uruchomienia (ms)", TIME);
    printTable("TABELA 3: Liczba iteracji (LS dla MSLS, perturbacji dla ILS/LNS/LNSa)", ITERS);

    csvFile.close();
    std::cout << "\nEksperyment zakonczony. Wyniki: 'eksperyment_zadanie4.csv'" << std::endl;
    std::cout << "Najlepsze trasy: 'Results/Best_{MSLS,ILS,LNS,LNSa}_*.json'" << std::endl;
}


// =====================================================================
// ZADANIE 5 - Testy globalnej wypuklosci
// =====================================================================
// Schemat dla kazdej instancji:
//  1. Wygeneruj bardzo dobre rozwiazanie metoda LNS (najlepsza z zad. 4).
//     Limit czasu = 3x sredni czas MSLS (kalibracja przez 1 uruchomienie MSLS).
//  2. Wygeneruj 1000 losowych optyma lokalnych: losowe startowe -> LocalSearch zachlanny (Edge).
//  3. Dla kazdego z 1000 optyma policz:
//       a) podobienstwo do bardzo dobrego rozwiazania (wierzcholki i krawedzie),
//       b) srednie podobienstwo do pozostalych 999 optyma (wierzcholki i krawedzie).
//  4. Oblicz wspolczynniki korelacji Pearsona: wartosc funkcji celu vs podobienstwo.
//  5. Zapisz wyniki do CSV (do wizualizacji w Jupyter Notebook).
//
// Miara podobienstwa:
//   - liczba wspolnych wybranych wierzcholkow
//   - liczba wspolnych krawedzi (nieskierowanych, cykl)
// =====================================================================

void RunTask5Experiment(const std::vector<std::string>& filePaths) {
    constexpr int NUM_LOCAL_OPTIMA = 1000;

    struct SummaryRow {
        std::string instance;
        double bestScore;
        double corrBestV, corrBestE, corrAvgV, corrAvgE;
    };
    std::vector<SummaryRow> summary;

    for (const auto& path : filePaths) {
        Data data = LoadData(path);
        std::string fileName = path.substr(path.find_last_of("/\\") + 1);

        std::cout << "\n>>> Zadanie 5 (Globalna wypuklosc) - Instancja: " << fileName << " <<<" << std::endl;

        // ----------------------------------------------------------------
        // 1. Bardzo dobre rozwiazanie: LNS z limitem 3x sredni czas MSLS
        // ----------------------------------------------------------------
        std::cout << "  Kalibracja czasu przez MSLS (200 iter)..." << std::endl;
        MSLSResult msls = MSLS(data, 200);
        long long timeLimitMs = msls.elapsedMs * 3;

        std::cout << "  LNS (Segment, 30%) z limitem " << timeLimitMs << " ms..." << std::endl;
        ILSResult bestResult = LNS(data, timeLimitMs, 0.30, DestroyStrategy::Segment);
        Sequence bestTour = bestResult.bestTour;
        double bestScore = bestResult.bestScore;

        EvaluationResult bestMetrics = CalculateTourMetrics(bestTour, data);
        SaveResultWithCoords("Results/Best_Task5_" + fileName + ".json", path, bestTour, bestMetrics);
        std::cout << "  Najlepszy wynik (LNS): " << bestScore << std::endl;

        // Precomputed structures for the best solution
        std::vector<bool> bestMember(data.n, false);
        for (int v : bestTour) bestMember[v] = true;

        // Encode edge as min*n + max (n = data.n, safe for n < 46340)
        std::unordered_set<int> bestEdgeSet;
        bestEdgeSet.reserve(bestTour.size() * 2);
        for (int k = 0; k < (int)bestTour.size(); ++k) {
            int u = bestTour[k], v = bestTour[(k + 1) % (int)bestTour.size()];
            if (u > v) std::swap(u, v);
            bestEdgeSet.insert(u * data.n + v);
        }

        // ----------------------------------------------------------------
        // 2. Generuj 1000 losowych optyma lokalnych (zachlanny LS, krawedzie)
        // ----------------------------------------------------------------
        std::cout << "  Generowanie " << NUM_LOCAL_OPTIMA << " losowych optyma lokalnych [";

        std::vector<Sequence> localOptima;
        std::vector<double> scores;
        localOptima.reserve(NUM_LOCAL_OPTIMA);
        scores.reserve(NUM_LOCAL_OPTIMA);

        for (int i = 0; i < NUM_LOCAL_OPTIMA; ++i) {
            Sequence start = GenerateRandomSolution(data.n);
            Sequence opt = LocalSearch(data, start, false, Neighborhood::Edge);
            EvaluationResult m = CalculateTourMetrics(opt, data);
            scores.push_back((double)m.totalGain - m.totalDistance);
            localOptima.push_back(std::move(opt));
            if (i % 100 == 0) std::cout << "." << std::flush;
        }
        std::cout << "] Gotowe!" << std::endl;

        // ----------------------------------------------------------------
        // 3. Precompute membership vectors and edge sets for all local optima
        // ----------------------------------------------------------------
        std::vector<std::vector<bool>> member(NUM_LOCAL_OPTIMA, std::vector<bool>(data.n, false));
        std::vector<std::unordered_set<int>> edgeSets(NUM_LOCAL_OPTIMA);

        for (int i = 0; i < NUM_LOCAL_OPTIMA; ++i) {
            edgeSets[i].reserve(localOptima[i].size() * 2);
            for (int v : localOptima[i]) member[i][v] = true;
            for (int k = 0; k < (int)localOptima[i].size(); ++k) {
                int u = localOptima[i][k], v = localOptima[i][(k + 1) % (int)localOptima[i].size()];
                if (u > v) std::swap(u, v);
                edgeSets[i].insert(u * data.n + v);
            }
        }

        // ----------------------------------------------------------------
        // 4a. Podobienstwo do najlepszego rozwiazania
        // ----------------------------------------------------------------
        std::cout << "  Liczenie podobienstwa do najlepszego rozwiazania..." << std::endl;
        std::vector<int> simToBestV(NUM_LOCAL_OPTIMA, 0);
        std::vector<int> simToBestE(NUM_LOCAL_OPTIMA, 0);

        for (int i = 0; i < NUM_LOCAL_OPTIMA; ++i) {
            for (int v : localOptima[i])
                if (bestMember[v]) ++simToBestV[i];
            for (int edgeKey : edgeSets[i])
                if (bestEdgeSet.count(edgeKey)) ++simToBestE[i];
        }

        // ----------------------------------------------------------------
        // 4b. Srednie podobienstwo do pozostalych 999 optyma (gorny trojkat)
        // ----------------------------------------------------------------
        std::cout << "  Liczenie parowywch podobienst (1000x1000)..." << std::endl;
        std::vector<double> sumSimV(NUM_LOCAL_OPTIMA, 0.0);
        std::vector<double> sumSimE(NUM_LOCAL_OPTIMA, 0.0);

        for (int i = 0; i < NUM_LOCAL_OPTIMA; ++i) {
            for (int j = i + 1; j < NUM_LOCAL_OPTIMA; ++j) {
                int cv = 0;
                for (int v : localOptima[j])
                    if (member[i][v]) ++cv;
                sumSimV[i] += cv;
                sumSimV[j] += cv;

                int ce = 0;
                for (int edgeKey : edgeSets[j])
                    if (edgeSets[i].count(edgeKey)) ++ce;
                sumSimE[i] += ce;
                sumSimE[j] += ce;
            }
            if (i % 100 == 0) {
                std::cout << "  " << i << "/" << NUM_LOCAL_OPTIMA << "\r" << std::flush;
            }
        }
        std::cout << "  " << NUM_LOCAL_OPTIMA << "/" << NUM_LOCAL_OPTIMA << std::endl;

        std::vector<double> avgSimOthersV(NUM_LOCAL_OPTIMA);
        std::vector<double> avgSimOthersE(NUM_LOCAL_OPTIMA);
        for (int i = 0; i < NUM_LOCAL_OPTIMA; ++i) {
            avgSimOthersV[i] = sumSimV[i] / (NUM_LOCAL_OPTIMA - 1);
            avgSimOthersE[i] = sumSimE[i] / (NUM_LOCAL_OPTIMA - 1);
        }

        // ----------------------------------------------------------------
        // 5. Wspolczynniki korelacji Pearsona
        // ----------------------------------------------------------------
        auto pearson = [](const std::vector<double>& x, const std::vector<double>& y) -> double {
            int n = (int)x.size();
            double mx = std::accumulate(x.begin(), x.end(), 0.0) / n;
            double my = std::accumulate(y.begin(), y.end(), 0.0) / n;
            double num = 0, dx2 = 0, dy2 = 0;
            for (int i = 0; i < n; ++i) {
                double dx = x[i] - mx, dy = y[i] - my;
                num += dx * dy; dx2 += dx * dx; dy2 += dy * dy;
            }
            return (dx2 == 0 || dy2 == 0) ? 0.0 : num / std::sqrt(dx2 * dy2);
        };

        std::vector<double> sToBestVd(simToBestV.begin(), simToBestV.end());
        std::vector<double> sToBestEd(simToBestE.begin(), simToBestE.end());

        double corrBestV  = pearson(scores, sToBestVd);
        double corrBestE  = pearson(scores, sToBestEd);
        double corrAvgV   = pearson(scores, avgSimOthersV);
        double corrAvgE   = pearson(scores, avgSimOthersE);

        std::cout << std::fixed << std::setprecision(4);
        std::cout << "\n  Wspolczynniki korelacji:" << std::endl;
        std::cout << "  Sim do najlepszego (wierzcholki): " << corrBestV << std::endl;
        std::cout << "  Sim do najlepszego (krawedzie):   " << corrBestE << std::endl;
        std::cout << "  Srednia sim do innych (wierzcholki): " << corrAvgV << std::endl;
        std::cout << "  Srednia sim do innych (krawedzie):   " << corrAvgE << std::endl;

        // ----------------------------------------------------------------
        // 6. Zapis do CSV
        // ----------------------------------------------------------------
        std::string csvPath = "Results/zad5_" + fileName + ".csv";
        std::ofstream csvOut(csvPath);
        if (!csvOut.is_open()) {
            std::cerr << "Nie mozna otworzyc: " << csvPath << std::endl;
            continue;
        }

        csvOut << "score;sim_to_best_vertices;sim_to_best_edges;avg_sim_others_vertices;avg_sim_others_edges\n";

        csvOut << std::fixed << std::setprecision(6);
        for (int i = 0; i < NUM_LOCAL_OPTIMA; ++i) {
            csvOut << scores[i] << ";"
                   << simToBestV[i] << ";"
                   << simToBestE[i] << ";"
                   << avgSimOthersV[i] << ";"
                   << avgSimOthersE[i] << "\n";
        }

        csvOut.close();
        std::cout << "  Wyniki zapisane do: " << csvPath << std::endl;

        summary.push_back({ fileName, bestScore, corrBestV, corrBestE, corrAvgV, corrAvgE });
    }

    // Zapis pliku podsumowania z korelacjami (jeden wiersz na instancje)
    std::ofstream sumOut("Results/zad5_summary.csv");
    if (sumOut.is_open()) {
        sumOut << "instance;best_score;corr_sim_to_best_vertices;corr_sim_to_best_edges;"
                  "corr_avg_sim_others_vertices;corr_avg_sim_others_edges\n";
        sumOut << std::fixed << std::setprecision(6);
        for (const auto& row : summary) {
            sumOut << row.instance << ";"
                   << row.bestScore << ";"
                   << row.corrBestV << ";"
                   << row.corrBestE << ";"
                   << row.corrAvgV  << ";"
                   << row.corrAvgE  << "\n";
        }
        sumOut.close();
        std::cout << "Podsumowanie korelacji: 'Results/zad5_summary.csv'" << std::endl;
    }

    std::cout << "\nZadanie 5 zakonczone." << std::endl;
}


// =====================================================================
// ZADANIE 6 - Eksperyment: HAE (5 wariantow) vs MSLS, ILS, LNS
// =====================================================================
// Schemat:
//  - Kalibracja: 20 uruchomien MSLS (200 iter LS) -> avgMSLSTime
//  - Kazda metoda: 20 uruchomien z limitem avgMSLSTime
//  - Metody HAE: Op1+LS, Op2+LS, Op2 bez LS, Op3+LS, Op3 bez LS
//  - Metody porownawcze: MSLS, ILS, LNS
//  - Linie bazowe: Weighted 2-Regret, bazowe LS (rand + LocalSearch)
//  - Trzy tabele CSV: funkcja celu, czas, liczba iteracji/rekombinacji
// =====================================================================

void RunTask6Experiment(const std::vector<std::string>& filePaths) {
    std::ofstream csvFile("eksperyment_zadanie6.csv");
    if (!csvFile.is_open()) {
        std::cerr << "Blad otwarcia pliku CSV!" << std::endl;
        return;
    }

    struct HAEConfig {
        std::string  id;
        std::string  prettyName;
        HAEOperator  op;
        bool         useLS;
    };

    std::vector<HAEConfig> haeConfigs = {
        {"HAE_Op1",      "HAE Op1 + LS (wsp. podsciezki)",  HAEOperator::Op1, true },
        {"HAE_Op2_LS",   "HAE Op2 + LS",                    HAEOperator::Op2, true },
        {"HAE_Op2_noLS", "HAE Op2 (bez LS po rekomb.)",     HAEOperator::Op2, false},
        {"HAE_Op3_LS",   "HAE Op3 + LS",                    HAEOperator::Op3, true },
        {"HAE_Op3_noLS", "HAE Op3 (bez LS po rekomb.)",     HAEOperator::Op3, false},
    };

    // Kolejnosc wierszy w tabelach wynikowych
    std::vector<std::string> allNames = {
        "HAE Op1 + LS (wsp. podsciezki)",
        "HAE Op2 + LS",
        "HAE Op2 (bez LS po rekomb.)",
        "HAE Op3 + LS",
        "HAE Op3 (bez LS po rekomb.)",
        "MSLS (200 iter LS)",
        "ILS (Iterated LS)",
        "LNS (Destroy-Repair + LS)",
        "Heurystyka zachlanna (2-zal)",
        "Bazowe LP (rand + LS)",
    };

    std::map<std::string, std::map<std::string, ExperimentStats>> resultsTable;
    std::map<std::string, std::map<std::string, std::vector<int>>> iterationsTable;
    std::vector<std::string> instanceNames;

    constexpr int RUNS       = 20;
    constexpr int MSLS_ITERS = 200;

    for (const auto& path : filePaths) {
        Data data = LoadData(path);
        std::string fileName = path.substr(path.find_last_of("/\\") + 1);
        instanceNames.push_back(fileName);

        std::cout << "\n>>> Zadanie 6 - Instancja: " << fileName << " <<<" << std::endl;

        // ------------------------------------------------- MSLS (kalibracja + wyniki)
        std::cout << "  Algorytm: " << std::left << std::setw(36)
                  << "MSLS (kalibracja + wyniki)" << " [";

        long long sumMSLSTime = 0;
        Sequence bestTourMSLS; EvaluationResult bestMetricsMSLS{}; double bestValueMSLS = -std::numeric_limits<double>::max();

        for (int run = 0; run < RUNS; ++run) {
            MSLSResult res = MSLS(data, MSLS_ITERS);
            resultsTable["MSLS (200 iter LS)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["MSLS (200 iter LS)"][fileName].push_back(res.iterations);
            sumMSLSTime += res.elapsedMs;
            if (res.bestScore > bestValueMSLS) {
                bestValueMSLS  = res.bestScore;
                bestTourMSLS   = res.bestTour;
                bestMetricsMSLS = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;

        long long avgMSLSTime = sumMSLSTime / RUNS;
        std::cout << "  >> Sredni czas MSLS: " << avgMSLSTime
                  << " ms (limit dla pozostalych metod)" << std::endl;
        SaveResultWithCoords("Results/Best_MSLS_z6_" + fileName + ".json",
                             path, bestTourMSLS, bestMetricsMSLS);

        // ------------------------------------------------- ILS
        std::cout << "  Algorytm: " << std::left << std::setw(36)
                  << "ILS" << " [";
        Sequence bestTourILS; EvaluationResult bestMetricsILS{}; double bestValueILS = -std::numeric_limits<double>::max();
        for (int run = 0; run < RUNS; ++run) {
            ILSResult res = ILS(data, avgMSLSTime, 4);
            resultsTable["ILS (Iterated LS)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["ILS (Iterated LS)"][fileName].push_back(res.perturbations);
            if (res.bestScore > bestValueILS) {
                bestValueILS  = res.bestScore;
                bestTourILS   = res.bestTour;
                bestMetricsILS = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_ILS_z6_" + fileName + ".json",
                             path, bestTourILS, bestMetricsILS);

        // ------------------------------------------------- LNS
        std::cout << "  Algorytm: " << std::left << std::setw(36)
                  << "LNS (Segment + LS)" << " [";
        Sequence bestTourLNS; EvaluationResult bestMetricsLNS{}; double bestValueLNS = -std::numeric_limits<double>::max();
        for (int run = 0; run < RUNS; ++run) {
            ILSResult res = LNS(data, avgMSLSTime, 0.30, DestroyStrategy::Segment);
            resultsTable["LNS (Destroy-Repair + LS)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["LNS (Destroy-Repair + LS)"][fileName].push_back(res.perturbations);
            if (res.bestScore > bestValueLNS) {
                bestValueLNS  = res.bestScore;
                bestTourLNS   = res.bestTour;
                bestMetricsLNS = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_LNS_z6_" + fileName + ".json",
                             path, bestTourLNS, bestMetricsLNS);

        // ------------------------------------------------- HAE warianty
        for (const auto& cfg : haeConfigs) {
            std::cout << "  Algorytm: " << std::left << std::setw(36)
                      << cfg.prettyName << " [";
            Sequence bestTour; EvaluationResult bestMetrics{}; double bestValue = -std::numeric_limits<double>::max();
            for (int run = 0; run < RUNS; ++run) {
                HAEResult res = HAE(data, avgMSLSTime, cfg.op, cfg.useLS);
                resultsTable[cfg.prettyName][fileName].add(res.bestScore, res.elapsedMs);
                iterationsTable[cfg.prettyName][fileName].push_back(res.iterations);
                if (res.bestScore > bestValue) {
                    bestValue   = res.bestScore;
                    bestTour    = res.bestTour;
                    bestMetrics = CalculateTourMetrics(res.bestTour, data);
                }
                std::cout << ".";
            }
            std::cout << "] Gotowe!" << std::endl;
            SaveResultWithCoords("Results/Best_" + cfg.id + "_" + fileName + ".json",
                                 path, bestTour, bestMetrics);
        }

        // ------------------------------------------------- Heurystyka zachlanna (baseline)
        std::cout << "  Algorytm: " << std::left << std::setw(36)
                  << "Heurystyka zachlanna (2-zal)" << " [";
        Sequence bestTourH; EvaluationResult bestMetricsH{}; double bestValueH = -std::numeric_limits<double>::max();
        for (int run = 0; run < RUNS; ++run) {
            auto t1 = std::chrono::high_resolution_clock::now();
            auto res = SolveWeighted2Regret(data, false, 1.0, 0.0);
            auto t2 = std::chrono::high_resolution_clock::now();
            long long dur = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            EvaluationResult m = CalculateTourMetrics(res.finalTour, data);
            double sc = (double)m.totalGain - (double)m.totalDistance;
            resultsTable["Heurystyka zachlanna (2-zal)"][fileName].add(sc, dur);
            iterationsTable["Heurystyka zachlanna (2-zal)"][fileName].push_back(1);
            if (sc > bestValueH) { bestValueH = sc; bestTourH = res.finalTour; bestMetricsH = m; }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_Heur_z6_" + fileName + ".json",
                             path, bestTourH, bestMetricsH);

        // ------------------------------------------------- Bazowe LP (baseline)
        std::cout << "  Algorytm: " << std::left << std::setw(36)
                  << "Bazowe LP (rand + LS)" << " [";
        Sequence bestTourLS; EvaluationResult bestMetricsLS{}; double bestValueLS = -std::numeric_limits<double>::max();
        for (int run = 0; run < RUNS; ++run) {
            auto t1 = std::chrono::high_resolution_clock::now();
            Sequence start = GenerateRandomSolution(data.n);
            Sequence tour  = LocalSearch(data, start, true, Neighborhood::Edge);
            auto t2 = std::chrono::high_resolution_clock::now();
            long long dur = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            EvaluationResult m = CalculateTourMetrics(tour, data);
            double sc = (double)m.totalGain - (double)m.totalDistance;
            resultsTable["Bazowe LP (rand + LS)"][fileName].add(sc, dur);
            iterationsTable["Bazowe LP (rand + LS)"][fileName].push_back(1);
            if (sc > bestValueLS) { bestValueLS = sc; bestTourLS = tour; bestMetricsLS = m; }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_BaseLS_z6_" + fileName + ".json",
                             path, bestTourLS, bestMetricsLS);
    }

    // ==================== TABELE CSV ====================
    enum TableMode { SCORE = 0, TIME = 1, ITERS = 2 };

    auto printTable = [&](const std::string& title, TableMode mode) {
        csvFile << title << ";\nMetoda;";
        for (const auto& name : instanceNames) csvFile << name << ";";
        csvFile << "\n";
        for (const auto& name : allNames) {
            csvFile << name << ";";
            for (const auto& inst : instanceNames) {
                if (mode == SCORE)
                    csvFile << resultsTable[name][inst].formatScore() << ";";
                else if (mode == TIME)
                    csvFile << resultsTable[name][inst].formatTime() << ";";
                else
                    csvFile << formatIters(iterationsTable[name][inst]) << ";";
            }
            csvFile << "\n";
        }
        csvFile << "\n";
    };

    printTable("TABELA 1: Statystyki funkcji celu (Zysk - Dystans)", SCORE);
    printTable("TABELA 2: Statystyki czasu obliczen jednego uruchomienia (ms)", TIME);
    printTable("TABELA 3: Liczba iteracji (rekombinacji/perturbacji)", ITERS);

    csvFile.close();
    std::cout << "\nZadanie 6 zakonczone. Wyniki: 'eksperyment_zadanie6.csv'" << std::endl;
    std::cout << "Najlepsze trasy: 'Results/Best_{HAE_Op*,MSLS,ILS,LNS,Heur,BaseLS}_z6_*.json'" << std::endl;
}


// =====================================================================
// ZADANIE 7 - Eksperyment: OwnMethod (HLNS-C) vs metody referencyjne
// =====================================================================
// Schemat:
//   - Kalibracja: 20 uruchomien MSLS (200 iter LS) -> avgMSLSTime
//   - Kazda metoda: 20 uruchomien z limitem avgMSLSTime
//   - Metody referencyjne: MSLS, ILS, LNS, HAE Op1+LS, HAE Op2+LS
//   - Wlasna metoda: HLNS-C (OwnMethod)
//   - Trzy tabele CSV: funkcja celu, czas, liczba iteracji
// =====================================================================

void RunTask7Experiment(const std::vector<std::string>& filePaths)
{
    std::ofstream csvFile("eksperyment_zadanie7.csv");
    if (!csvFile.is_open()) {
        std::cerr << "Blad otwarcia pliku CSV!" << std::endl;
        return;
    }

    std::vector<std::string> allNames = {
        "HLNS-C (wlasna metoda)",
        "HAE Op1 + LS",
        "HAE Op2 + LS",
        "MSLS (200 iter LS)",
        "ILS (Iterated LS)",
        "LNS (Destroy-Repair + LS)",
    };

    std::map<std::string, std::map<std::string, ExperimentStats>> resultsTable;
    std::map<std::string, std::map<std::string, std::vector<int>>> iterationsTable;
    std::vector<std::string> instanceNames;

    constexpr int RUNS       = 20;
    constexpr int MSLS_ITERS = 200;

    for (const auto& path : filePaths) {
        Data data = LoadData(path);
        std::string fileName = path.substr(path.find_last_of("/\\") + 1);
        instanceNames.push_back(fileName);

        std::cout << "\n>>> Zadanie 7 - Instancja: " << fileName << " <<<" << std::endl;

        // ----------------------------------------- MSLS (kalibracja + wyniki)
        std::cout << "  Algorytm: " << std::left << std::setw(36)
                  << "MSLS (kalibracja + wyniki)" << " [";

        long long sumMSLSTime = 0;
        Sequence bestTourMSLS; EvaluationResult bestMetricsMSLS{}; double bestValueMSLS = -std::numeric_limits<double>::max();

        for (int run = 0; run < RUNS; ++run) {
            MSLSResult res = MSLS(data, MSLS_ITERS);
            resultsTable["MSLS (200 iter LS)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["MSLS (200 iter LS)"][fileName].push_back(res.iterations);
            sumMSLSTime += res.elapsedMs;
            if (res.bestScore > bestValueMSLS) {
                bestValueMSLS   = res.bestScore;
                bestTourMSLS    = res.bestTour;
                bestMetricsMSLS = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;

        long long avgMSLSTime = sumMSLSTime / RUNS;
        std::cout << "  >> Sredni czas MSLS: " << avgMSLSTime
                  << " ms (limit dla pozostalych metod)" << std::endl;
        SaveResultWithCoords("Results/Best_MSLS_z7_" + fileName + ".json",
                             path, bestTourMSLS, bestMetricsMSLS);

        // ----------------------------------------- ILS
        std::cout << "  Algorytm: " << std::left << std::setw(36) << "ILS" << " [";
        Sequence bestTourILS; EvaluationResult bestMetricsILS{}; double bestValueILS = -std::numeric_limits<double>::max();
        for (int run = 0; run < RUNS; ++run) {
            ILSResult res = ILS(data, avgMSLSTime, 4);
            resultsTable["ILS (Iterated LS)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["ILS (Iterated LS)"][fileName].push_back(res.perturbations);
            if (res.bestScore > bestValueILS) {
                bestValueILS   = res.bestScore;
                bestTourILS    = res.bestTour;
                bestMetricsILS = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_ILS_z7_" + fileName + ".json",
                             path, bestTourILS, bestMetricsILS);

        // ----------------------------------------- LNS
        std::cout << "  Algorytm: " << std::left << std::setw(36) << "LNS (Segment + LS)" << " [";
        Sequence bestTourLNS; EvaluationResult bestMetricsLNS{}; double bestValueLNS = -std::numeric_limits<double>::max();
        for (int run = 0; run < RUNS; ++run) {
            ILSResult res = LNS(data, avgMSLSTime, 0.3, DestroyStrategy::Segment);
            resultsTable["LNS (Destroy-Repair + LS)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["LNS (Destroy-Repair + LS)"][fileName].push_back(res.perturbations);
            if (res.bestScore > bestValueLNS) {
                bestValueLNS   = res.bestScore;
                bestTourLNS    = res.bestTour;
                bestMetricsLNS = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_LNS_z7_" + fileName + ".json",
                             path, bestTourLNS, bestMetricsLNS);

        // ----------------------------------------- HAE Op1 + LS
        std::cout << "  Algorytm: " << std::left << std::setw(36) << "HAE Op1 + LS" << " [";
        Sequence bestTourHAE1; EvaluationResult bestMetricsHAE1{}; double bestValueHAE1 = -std::numeric_limits<double>::max();
        for (int run = 0; run < RUNS; ++run) {
            HAEResult res = HAE(data, avgMSLSTime, HAEOperator::Op1, true);
            resultsTable["HAE Op1 + LS"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["HAE Op1 + LS"][fileName].push_back(res.iterations);
            if (res.bestScore > bestValueHAE1) {
                bestValueHAE1   = res.bestScore;
                bestTourHAE1    = res.bestTour;
                bestMetricsHAE1 = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_HAE1_z7_" + fileName + ".json",
                             path, bestTourHAE1, bestMetricsHAE1);

        // ----------------------------------------- HAE Op2 + LS
        std::cout << "  Algorytm: " << std::left << std::setw(36) << "HAE Op2 + LS" << " [";
        Sequence bestTourHAE2; EvaluationResult bestMetricsHAE2{}; double bestValueHAE2 = -std::numeric_limits<double>::max();
        for (int run = 0; run < RUNS; ++run) {
            HAEResult res = HAE(data, avgMSLSTime, HAEOperator::Op2, true);
            resultsTable["HAE Op2 + LS"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["HAE Op2 + LS"][fileName].push_back(res.iterations);
            if (res.bestScore > bestValueHAE2) {
                bestValueHAE2   = res.bestScore;
                bestTourHAE2    = res.bestTour;
                bestMetricsHAE2 = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_HAE2_z7_" + fileName + ".json",
                             path, bestTourHAE2, bestMetricsHAE2);

        // ----------------------------------------- HLNS-C (wlasna metoda)
        std::cout << "  Algorytm: " << std::left << std::setw(36) << "HLNS-C (wlasna metoda)" << " [";
        Sequence bestTourOwn; EvaluationResult bestMetricsOwn{}; double bestValueOwn = -std::numeric_limits<double>::max();
        for (int run = 0; run < RUNS; ++run) {
            OwnResult res = OwnMethodParallel(data, avgMSLSTime);
            resultsTable["HLNS-C (wlasna metoda)"][fileName].add(res.bestScore, res.elapsedMs);
            iterationsTable["HLNS-C (wlasna metoda)"][fileName].push_back(res.iterations);
            if (res.bestScore > bestValueOwn) {
                bestValueOwn   = res.bestScore;
                bestTourOwn    = res.bestTour;
                bestMetricsOwn = CalculateTourMetrics(res.bestTour, data);
            }
            std::cout << ".";
        }
        std::cout << "] Gotowe!" << std::endl;
        SaveResultWithCoords("Results/Best_HLNSC_z7_" + fileName + ".json",
                             path, bestTourOwn, bestMetricsOwn);
    }

    // ==================== TABELE CSV ====================
    enum TableMode7 { SCORE7 = 0, TIME7 = 1, ITERS7 = 2 };

    auto printTable7 = [&](const std::string& title, TableMode7 mode) {
        csvFile << title << ";\nMetoda;";
        for (const auto& name : instanceNames) csvFile << name << ";";
        csvFile << "\n";
        for (const auto& name : allNames) {
            csvFile << name << ";";
            for (const auto& inst : instanceNames) {
                if (mode == SCORE7)
                    csvFile << resultsTable[name][inst].formatScore() << ";";
                else if (mode == TIME7)
                    csvFile << resultsTable[name][inst].formatTime() << ";";
                else
                    csvFile << formatIters(iterationsTable[name][inst]) << ";";
            }
            csvFile << "\n";
        }
        csvFile << "\n";
    };

    printTable7("TABELA 1: Statystyki funkcji celu (Zysk - Dystans)", SCORE7);
    printTable7("TABELA 2: Statystyki czasu obliczen jednego uruchomienia (ms)", TIME7);
    printTable7("TABELA 3: Liczba iteracji (rekombinacji/perturbacji/LNS)", ITERS7);

    csvFile.close();
    std::cout << "\nZadanie 7 zakonczone. Wyniki: 'eksperyment_zadanie7.csv'" << std::endl;
    std::cout << "Najlepsze trasy: 'Results/Best_{HLNSC,HAE1,HAE2,MSLS,ILS,LNS}_z7_*.json'" << std::endl;
}