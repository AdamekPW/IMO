#include "zad4.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <vector>

// =====================================================================
// ZADANIE 4 - implementacja MSLS oraz operatorow perturbacji (ILS, LNS)
// =====================================================================

// Lokalny generator. Inicjalizowany raz, zeby nie tworzyc std::random_device
// w kazdym wywolaniu (kosztowne, a potrafi tez psuc rozproszenie).
static std::mt19937& rng() {
    static thread_local std::mt19937 generator(std::random_device{}());
    return generator;
}

// Generuje losowe rozwiazanie startowe o stalym rozmiarze ceil(n/2).
// Zachowuje wymog z PDF ("startujemy z rozwiazan losowych") - wierzcholki
// wybierane sa losowo bez powtorzen, w losowej kolejnosci. Zaleta wzgledem
// wbudowanego GenerateRandomSolution(n) jest wyeliminowanie patologicznych
// startow z trasa o 2-3 wierzcholkach, ktore potrafia uwiezic ILS w slabym
// minimum lokalnym (zaobserwowane w pierwszym uruchomieniu eksperymentu).
static Sequence GenerateBalancedRandom(int n) {
    auto& gen = rng();
    int k = (n + 1) / 2; // ~n/2 wierzcholkow

    std::vector<int> pool(n);
    std::iota(pool.begin(), pool.end(), 0);
    std::shuffle(pool.begin(), pool.end(), gen);

    return Sequence(pool.begin(), pool.begin() + k);
}


// =====================================================================
// MSLS
// =====================================================================
MSLSResult MSLS(const Data& data, int iterations) {
    MSLSResult result;
    result.iterations = iterations;
    result.bestScore = -std::numeric_limits<double>::max();

    auto t1 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        Sequence start = GenerateBalancedRandom(data.n);
        Sequence finalTour = LocalSearch(data, start, true, Neighborhood::Edge);

        EvaluationResult metrics = CalculateTourMetrics(finalTour, data);
        double score = (double)metrics.totalGain - (double)metrics.totalDistance;

        if (score > result.bestScore) {
            result.bestScore = score;
            result.bestTour = finalTour;
        }
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    return result;
}


// =====================================================================
// Perturbacja ILS (mala) - mix 2-opt i swap inside-outside
// =====================================================================
Sequence PerturbILS(Sequence tour, const Data& data, int numMoves) {
    auto& gen = rng();

    for (int m = 0; m < numMoves; ++m) {
        int n = (int)tour.size();
        if (n < 4) break; // bezpieczniej nie ruszac bardzo malych tras

        int moveType = std::uniform_int_distribution<int>(0, 1)(gen);

        if (moveType == 0) {
            // --- 2-opt: losowe odwrocenie fragmentu ---
            int i = std::uniform_int_distribution<int>(0, n - 2)(gen);
            int j = std::uniform_int_distribution<int>(i + 1, n - 1)(gen);
            // odwracamy [i+1 .. j]
            std::reverse(tour.begin() + i + 1, tour.begin() + j + 1);
        }
        else {
            // --- swap inside-outside: podmiana wierzcholka w trasie na losowy spoza trasy ---
            std::vector<int> outside = getNodesOutside(tour, data.n);
            if (outside.empty()) {
                // Wszystkie wierzcholki sa w trasie - rob 2-opt zamiast swapa
                int i = std::uniform_int_distribution<int>(0, n - 2)(gen);
                int j = std::uniform_int_distribution<int>(i + 1, n - 1)(gen);
                std::reverse(tour.begin() + i + 1, tour.begin() + j + 1);
            }
            else {
                int posIn = std::uniform_int_distribution<int>(0, n - 1)(gen);
                int posOut = std::uniform_int_distribution<int>(0, (int)outside.size() - 1)(gen);
                tour[posIn] = outside[posOut];
            }
        }
    }

    return tour;
}


// =====================================================================
// LNS - Destroy (3 warianty)
// =====================================================================

Sequence DestroyRandom(Sequence tour, double removeFraction) {
    auto& gen = rng();

    int n = (int)tour.size();
    if (n <= 2) return tour;

    int removeCount = std::max(1, (int)std::round(n * removeFraction));
    if (removeCount > n - 2) removeCount = n - 2; // zostaw min. 2 wierzcholki

    // Wybierz `removeCount` losowych indeksow bez powtorzen
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), gen);

    std::vector<bool> toRemove(n, false);
    for (int k = 0; k < removeCount; ++k) toRemove[indices[k]] = true;

    Sequence result;
    result.reserve(n - removeCount);
    for (int k = 0; k < n; ++k) {
        if (!toRemove[k]) result.push_back(tour[k]);
    }
    return result;
}


Sequence DestroyHeuristic(Sequence tour, const Data& data, double removeFraction) {
    auto& gen = rng();

    int n = (int)tour.size();
    if (n <= 2) return tour;

    int removeCount = std::max(1, (int)std::round(n * removeFraction));
    if (removeCount > n - 2) removeCount = n - 2;

    // --- Liczymy "koszt" kazdego wierzcholka:
    // ile zaoszczedzimy na dystansie usuwajac wierzcholek minus jego zysk.
    // Im wieksza wartosc - tym wierzcholek jest "drozszy" i bardziej oplaca
    // sie go usunac.
    std::vector<double> costs(n);
    for (int i = 0; i < n; ++i) {
        int prev = tour[(i - 1 + n) % n];
        int curr = tour[i];
        int next = tour[(i + 1) % n];

        double distContrib = (double)data.distances[prev][curr]
            + (double)data.distances[curr][next]
            - (double)data.distances[prev][next];
        costs[i] = distContrib - (double)data.gains[curr];
    }

    // Przesuwamy do dodatnich, zeby ruletka miala sens (minimum > 0)
    double minCost = *std::min_element(costs.begin(), costs.end());
    for (auto& c : costs) c = c - minCost + 1.0;

    // Selekcja ruletka - usuwamy `removeCount` roznych wierzcholkow.
    // Wagi dynamicznie aktualizowane (juz usuniete maja wage 0).
    std::vector<bool> toRemove(n, false);
    for (int k = 0; k < removeCount; ++k) {
        double total = 0.0;
        for (int i = 0; i < n; ++i) if (!toRemove[i]) total += costs[i];
        if (total <= 0.0) break;

        double r = std::uniform_real_distribution<double>(0.0, total)(gen);
        double cumsum = 0.0;
        for (int i = 0; i < n; ++i) {
            if (toRemove[i]) continue;
            cumsum += costs[i];
            if (cumsum >= r) {
                toRemove[i] = true;
                break;
            }
        }
    }

    Sequence result;
    result.reserve(n - removeCount);
    for (int k = 0; k < n; ++k) {
        if (!toRemove[k]) result.push_back(tour[k]);
    }
    return result;
}


Sequence DestroySegment(Sequence tour, double removeFraction) {
    auto& gen = rng();

    int n = (int)tour.size();
    if (n <= 2) return tour;

    int removeCount = std::max(1, (int)std::round(n * removeFraction));
    if (removeCount > n - 2) removeCount = n - 2;

    // Losowy poczatek segmentu
    int startIdx = std::uniform_int_distribution<int>(0, n - 1)(gen);

    std::vector<bool> toRemove(n, false);
    for (int k = 0; k < removeCount; ++k) {
        toRemove[(startIdx + k) % n] = true;
    }

    Sequence result;
    result.reserve(n - removeCount);
    for (int k = 0; k < n; ++k) {
        if (!toRemove[k]) result.push_back(tour[k]);
    }
    return result;
}


// =====================================================================
// LNS - Repair (Weighted 2-Regret bez zysku, alpha=1, beta=0)
// =====================================================================
// Powiela logike z SolveWeighted2Regret(data, false, 1.0, 0.0), ale startuje
// z istniejacej, czesciowej trasy (po Destroy).
Sequence RepairWeighted2Regret(Sequence tour, const Data& data) {
    int n = data.n;
    if (n < 2) return tour;

    // Stan visited - kto jest juz na trasie
    std::vector<bool> visited(n, false);
    for (int v : tour) {
        if (v >= 0 && v < n) visited[v] = true;
    }

    // Awaryjna inicjalizacja - jesli trasa po destroy jest pusta lub
    // za krotka. (W normalnym uzyciu nie powinno sie zdarzyc.)
    if ((int)tour.size() < 2) {
        int v1 = -1;
        for (int i = 0; i < n; ++i) if (!visited[i]) { v1 = i; break; }
        if (v1 == -1) return tour;
        visited[v1] = true;
        tour.push_back(v1);

        // Dobierz drugi wierzcholek minimalizujacy 2*dist (czyli max -2*dist)
        int v2 = -1;
        double bestDelta = -std::numeric_limits<double>::max();
        for (int i = 0; i < n; ++i) {
            if (visited[i]) continue;
            double d = -2.0 * (double)data.distances[v1][i];
            if (d > bestDelta) { bestDelta = d; v2 = i; }
        }
        if (v2 == -1) return tour;
        visited[v2] = true;
        tour.push_back(v2);
    }

    // --- Glowna petla: wstawianie 2-zalem, alpha=1, beta=0, useProfit=false ---
    while ((int)tour.size() < n) {
        int bestCityToAdd = -1;
        int bestPositionToAdd = -1;
        double maxRegret = -std::numeric_limits<double>::max();

        for (int u = 0; u < n; ++u) {
            if (visited[u]) continue;

            double bestDelta = -std::numeric_limits<double>::max();
            double secondBestDelta = -std::numeric_limits<double>::max();
            int currentBestPos = -1;

            int m = (int)tour.size();
            for (int i = 0; i < m; ++i) {
                int cityI = tour[i];
                int cityJ = tour[(i + 1) % m];

                double distChange = (double)data.distances[cityI][u]
                    + (double)data.distances[u][cityJ]
                    - (double)data.distances[cityI][cityJ];
                double delta = -distChange; // useProfit = false

                if (delta > bestDelta) {
                    secondBestDelta = bestDelta;
                    bestDelta = delta;
                    currentBestPos = i + 1;
                }
                else if (delta > secondBestDelta) {
                    secondBestDelta = delta;
                }
            }

            double regret = (secondBestDelta == -std::numeric_limits<double>::max())
                ? 0.0
                : (bestDelta - secondBestDelta);

            // alpha=1, beta=0 -> weightedScore = regret
            if (regret > maxRegret) {
                maxRegret = regret;
                bestCityToAdd = u;
                bestPositionToAdd = currentBestPos;
            }
        }

        if (bestCityToAdd == -1) break;

        tour.insert(tour.begin() + bestPositionToAdd, bestCityToAdd);
        visited[bestCityToAdd] = true;
    }

    // --- Faza II: usuwanie wierzcholkow deficytowych ---
    return PruneTour(tour, data);
}


// =====================================================================
// Pomocnicze: szybka ocena trasy
// =====================================================================
static inline double scoreOf(const Sequence& tour, const Data& data) {
    EvaluationResult m = CalculateTourMetrics(tour, data);
    return (double)m.totalGain - (double)m.totalDistance;
}


// =====================================================================
// ILS - Iterated Local Search
// =====================================================================
ILSResult ILS(const Data& data, long long timeLimitMs, int perturbMoves) {
    auto t0 = std::chrono::high_resolution_clock::now();

    ILSResult result;

    // Rozwiazanie startowe: losowe + LS
    Sequence x = GenerateBalancedRandom(data.n);
    x = LocalSearch(data, x, true, Neighborhood::Edge);
    double xScore = scoreOf(x, data);

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
        if (elapsed >= timeLimitMs) break;

        // y := x; perturbacja(y); LS(y)
        Sequence y = PerturbILS(x, data, perturbMoves);
        y = LocalSearch(data, y, true, Neighborhood::Edge);
        result.perturbations++;

        // Akceptacja: tylko jesli lepsze (zgodnie z pseudokodem)
        double yScore = scoreOf(y, data);
        if (yScore > xScore) {
            x = std::move(y);
            xScore = yScore;
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    result.bestTour = std::move(x);
    result.bestScore = xScore;
    return result;
}


// =====================================================================
// Pomocnicze: jeden krok Destroy zaleznie od wariantu
// =====================================================================
static Sequence destroyDispatch(const Sequence& tour, const Data& data,
                                double removeFraction, DestroyStrategy strategy) {
    switch (strategy) {
    case DestroyStrategy::Random:    return DestroyRandom(tour, removeFraction);
    case DestroyStrategy::Heuristic: return DestroyHeuristic(tour, data, removeFraction);
    case DestroyStrategy::Segment:   return DestroySegment(tour, removeFraction);
    }
    return DestroySegment(tour, removeFraction); // fallback
}


// =====================================================================
// LNS - Large Neighborhood Search (z LS w petli)
// =====================================================================
ILSResult LNS(const Data& data, long long timeLimitMs,
              double removeFraction, DestroyStrategy strategy) {
    auto t0 = std::chrono::high_resolution_clock::now();

    ILSResult result;

    // Rozwiazanie startowe: losowe + LS
    Sequence x = GenerateBalancedRandom(data.n);
    x = LocalSearch(data, x, true, Neighborhood::Edge);
    double xScore = scoreOf(x, data);

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
        if (elapsed >= timeLimitMs) break;

        // y := x; Destroy(y); Repair(y); LS(y)
        Sequence y = destroyDispatch(x, data, removeFraction, strategy);
        y = RepairWeighted2Regret(std::move(y), data);
        y = LocalSearch(data, std::move(y), true, Neighborhood::Edge);
        result.perturbations++;

        double yScore = scoreOf(y, data);
        if (yScore > xScore) {
            x = std::move(y);
            xScore = yScore;
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    result.bestTour = std::move(x);
    result.bestScore = xScore;
    return result;
}


// =====================================================================
// LNSa - Large Neighborhood Search BEZ LS w petli
// =====================================================================
// LS jest wykonywany tylko raz na rozwiazaniu startowym (bo bylo losowe).
// W petli wykonujemy wylacznie Destroy + Repair.
ILSResult LNSa(const Data& data, long long timeLimitMs,
               double removeFraction, DestroyStrategy strategy) {
    auto t0 = std::chrono::high_resolution_clock::now();

    ILSResult result;

    // Rozwiazanie startowe: losowe + LS (bo bylo losowe)
    Sequence x = GenerateBalancedRandom(data.n);
    x = LocalSearch(data, x, true, Neighborhood::Edge);
    double xScore = scoreOf(x, data);

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
        if (elapsed >= timeLimitMs) break;

        // y := x; Destroy(y); Repair(y); BEZ LS
        Sequence y = destroyDispatch(x, data, removeFraction, strategy);
        y = RepairWeighted2Regret(std::move(y), data);
        result.perturbations++;

        double yScore = scoreOf(y, data);
        if (yScore > xScore) {
            x = std::move(y);
            xScore = yScore;
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    result.bestTour = std::move(x);
    result.bestScore = xScore;
    return result;
}
