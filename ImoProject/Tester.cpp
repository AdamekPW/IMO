#include "Tester.h"
#include "Common.h"
#include "Heuristics.h"

#include <iostream>
#include <map>
#include <iomanip>
#include <fstream>
#include <sstream>

void RunFullExperiment(const std::vector<std::string>& filePaths) {
    // 1. Przygotowanie pliku CSV dla tabeli
    std::ofstream csvFile("tabela_wynikow.csv");

    // Kolejność metod jak na obrazku
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
        // Wyciągamy nazwę pliku bez ścieżki (np. "TSPA.csv")
        std::string fileNameOnly = path.substr(path.find_last_of("/\\") + 1);
        instanceNames.push_back(fileNameOnly);

        std::cout << "\n>>> Przetwarzanie instancji: " << fileNameOnly << " <<<" << std::endl;

        for (const auto& [prettyName, id] : methods) {
            Stats stats;

            // Zmienne do śledzenia absolutnie najlepszego wyniku z 200 prób
            Sequence bestTourFound;
            EvaluationResult bestMetricsFound;
            double bestObjectiveValue = -std::numeric_limits<double>::max();

            std::cout << "  Algorytm: " << std::left << std::setw(15) << prettyName << " [";

            for (int i = 0; i < 200; ++i) {
                AlgorithmResult res;
                if (id == "RAND") {
                    Sequence s = GenerateRandomSolution(data.n);
                    res = { s, 0.0 };
                }
                else if (id == "NNa") res = SolveNN(data, false);
                else if (id == "NNp") res = SolveNN(data, true);
                else if (id == "GCa") res = SolveGC(data, false);
                else if (id == "GCp") res = SolveGC(data, true);
                else if (id == "2Ra") res = SolveWeighted2Regret(data, false, 1.0, 0.0);
                else if (id == "2Rp") res = SolveWeighted2Regret(data, true, 1.0, 0.0);
                else if (id == "2RaW") res = SolveWeighted2Regret(data, false, 1.0, 1.0);
                else if (id == "2RpW") res = SolveWeighted2Regret(data, true, 1.0, 1.0);

                EvaluationResult currentMetrics = CalculateTourMetrics(res.finalTour, data);
                double currentObjective = (double)currentMetrics.totalGain - currentMetrics.totalDistance;
                stats.update(currentObjective, res.phase1Distance);

                // Sprawdzamy, czy to najlepszy wynik w tej serii 200 uruchomień
                if (currentObjective > bestObjectiveValue) {
                    bestObjectiveValue = currentObjective;
                    bestTourFound = res.finalTour;
                    bestMetricsFound = currentMetrics;
                }

                if (i % 40 == 0) std::cout << "."; // Prosty pasek postępu
            }
            std::cout << "] Gotowe!" << std::endl;

            // --- ZAPIS NAJLEPSZEGO WYNIKU DO JSON ---
            // Czyścimy nazwę metody ze spacji do nazwy pliku
            std::string safeMethodName = id;
            std::string jsonName = "Results/Best_" + safeMethodName + "_" + fileNameOnly + ".json";

            // Wywołanie Twojej funkcji zapisu
            SaveResultWithCoords(jsonName, path, bestTourFound, bestMetricsFound);

            // Przygotowanie danych do głównej tabeli CSV
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << stats.avg()
                << " (" << (int)stats.minVal << " | " << (int)stats.maxVal << ")";
            tableData[prettyName][fileNameOnly] = { ss.str(), stats.avgPhase1() };
        }
    }

    // --- GENEROWANIE TABELI W CSV (Zysk - Dystans) ---
    csvFile << "Metoda;";
    for (const auto& name : instanceNames) csvFile << name << ";";
    csvFile << "\n";

    for (const auto& [prettyName, id] : methods) {
        csvFile << prettyName << ";";
        for (const auto& instName : instanceNames) {
            csvFile << tableData[prettyName][instName].objectiveStats << ";";
        }
        csvFile << "\n";
    }

    // --- TABELA DODATKOWA (Dystans Faza I) ---
    csvFile << "\n\nTABELA 2: Srednia dlugosc trasy po fazie I (Hamilton)\nMetoda;";
    for (const auto& name : instanceNames) csvFile << name << ";";
    csvFile << "\n";

    for (const auto& [prettyName, id] : methods) {
        csvFile << prettyName << ";";
        for (const auto& instName : instanceNames) {
            csvFile << std::fixed << std::setprecision(2) << tableData[prettyName][instName].avgPhase1 << ";";
        }
        csvFile << "\n";
    }

    csvFile.close();
    std::cout << "\nEksperyment zakonczony. Wyniki zbiorcze: 'tabela_wynikow.csv'" << std::endl;
    std::cout << "Najlepsze trasy zapisano w folderze 'Results/'." << std::endl;
}