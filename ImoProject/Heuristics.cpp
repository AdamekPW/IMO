#include "Common.h"
#include <vector>
#include <numeric> // dla std::iota

Sequence PruneTour(Sequence tour, const Data& data) {
    bool improved = true;

    // Pętla wykonuje się tak długo, jak długo znajdujemy wierzchołek do usunięcia.
    // Minimalny cykl to 2 wierzchołki (A <-> B), choć zazwyczaj będzie ich więcej.
    while (improved && tour.size() > 2) {
        improved = false;
        double bestRemovalImprovement = 0.0;
        int bestIdx = -1;

        int currentSize = tour.size();
        for (int i = 0; i < currentSize; ++i) {
            // Pobieramy sąsiadów wierzchołka tour[i]
            int prev = tour[(i - 1 + currentSize) % currentSize];
            int curr = tour[i];
            int next = tour[(i + 1) % currentSize];

            // DELTA USUNIĘCIA (Efektywne O(1)):
            // Zyskujemy: Dystans, który zniknie (do i od wierzchołka)
            // Tracimy: Nowy dystans łączący sąsiadów bezpośrednio oraz zysk z tego miasta
            double distSaved = data.distances[prev][curr] + data.distances[curr][next] - data.distances[prev][next];
            double improvement = distSaved - data.gains[curr];

            if (improvement > bestRemovalImprovement) {
                bestRemovalImprovement = improvement;
                bestIdx = i;
            }
        }

        // Jeśli znaleźliśmy wierzchołek, którego usunięcie daje zysk netto > 0
        if (bestRemovalImprovement > 1e-9) { // 1e-9 dla uniknięcia błędów precyzji double
            tour.erase(tour.begin() + bestIdx);
            improved = true;
        }
    }

    return tour;
}

Sequence GenerateRandomSolution(int totalVertices) {
    int k = RandomNumber(2, totalVertices + 1);

    std::vector<int> pool(totalVertices);
    std::iota(pool.begin(), pool.end(), 0);

    for (int i = 0; i < totalVertices - 1; ++i) {
        int j = RandomNumber(i, totalVertices);
        std::swap(pool[i], pool[j]);
    }

    Sequence randomTour;
    for (int i = 0; i < k; ++i) {
        randomTour.push_back(pool[i]);
    }

    return randomTour;
}

Sequence SolveNN(const Data& data, bool useProfit) {
    int n = data.gains.size();
    if (n == 0) return {};

    Sequence tour;
    std::vector<bool> visited(n, false);

    // 1. FAZA I: Budowanie pełnego cyklu Hamiltona (wszystkie wierzchołki)
    int startCity = RandomNumber(0, n);
    tour.push_back(startCity);
    visited[startCity] = true;

    while (tour.size() < n) {
        int last = tour.back();
        int bestCity = -1;
        double bestDelta = -std::numeric_limits<double>::max();

        for (int i = 0; i < n; ++i) {
            if (!visited[i]) {
                // Delta = (opcjonalny zysk) - koszt krawędzi
                // Nawet jeśli delta jest ujemna, w Fazie I idziemy dalej, 
                // bo musimy odwiedzić wszystkie miasta.
                double delta = (useProfit ? (double)data.gains[i] : 0.0) - data.distances[last][i];

                if (delta > bestDelta) {
                    bestDelta = delta;
                    bestCity = i;
                }
            }
        }

        if (bestCity != -1) {
            tour.push_back(bestCity);
            visited[bestCity] = true;
        }
    }

    // 2. FAZA II: Usuwanie wierzchołków, które psują wynik (Profit - Distance)
    // To tutaj algorytm decyduje, który podzbiór wierzchołków zostawić.
    return PruneTour(tour, data);
}

Sequence SolveGC(const Data& data, bool useProfit) {
    int n = data.gains.size();
    if (n < 2) return {};

    std::vector<bool> visited(n, false);

    // 1. INICJALIZACJA: Wybór dwóch pierwszych punktów
    int v1 = RandomNumber(0, n);
    visited[v1] = true;

    int v2 = -1;
    double bestInitialDelta = -std::numeric_limits<double>::max();

    for (int i = 0; i < n; ++i) {
        if (!visited[i]) {
            // Delta dla cyklu dwuelementowego (v1 <-> i)
            // Koszt to 2 * dystans (tam i powrót)
            double delta = (useProfit ? (double)(data.gains[v1] + data.gains[i]) : 0.0) 
                           - (2.0 * data.distances[v1][i]);

            if (delta > bestInitialDelta) {
                bestInitialDelta = delta;
                v2 = i;
            }
        }
    }

    Sequence tour = { v1, v2 };
    visited[v2] = true;

    // 2. ROZBUDOWA: Budowanie pełnego cyklu (wszystkie wierzchołki)
    while (tour.size() < n) {
        int bestCity = -1;
        int insertPos = -1;
        double bestDelta = -std::numeric_limits<double>::max();

        for (int u = 0; u < n; ++u) {
            if (visited[u]) continue;

            for (int i = 0; i < (int)tour.size(); ++i) {
                int cityI = tour[i];
                int cityJ = tour[(i + 1) % tour.size()];

                // Delta = Zysk - (Dodany dystans - Usunięty dystans)
                double distChange = data.distances[cityI][u] + data.distances[u][cityJ] - data.distances[cityI][cityJ];
                double delta = (useProfit ? (double)data.gains[u] : 0.0) - distChange;

                if (delta > bestDelta) {
                    bestDelta = delta;
                    bestCity = u;
                    insertPos = i + 1;
                }
            }
        }

        if (bestCity != -1) {
            tour.insert(tour.begin() + insertPos, bestCity);
            visited[bestCity] = true;
        }
    }

    // 3. KLUCZOWY KROK: Faza II - Usuwanie deficytowych wierzchołków
    // Skoro problem to wybór podzbioru, to tutaj zapadają najważniejsze decyzje.
    return PruneTour(tour, data);
}

/**
 * @param data Dane wejściowe (macierz odległości i zyski)
 * @param useProfit Czy uwzględniać zysk w koszcie wstawienia (Faza Ia vs Ib)
 * @param alpha Waga żalu (domyślnie 1.0)
 * @param beta Waga kosztu (domyślnie 1.0, odejmowana od wyniku)
 */
Sequence SolveWeighted2Regret(const Data& data, bool useProfit, double alpha, double beta) {
    int n = data.gains.size();
    if (n < 2) return {};

    std::vector<bool> visited(n, false);

    // 1. INICJALIZACJA (Faza I - start)
    int v1 = RandomNumber(0, n);
    visited[v1] = true;

    int v2 = -1;
    double bestInitialDelta = -std::numeric_limits<double>::max();
    for (int i = 0; i < n; ++i) {
        if (!visited[i]) {
            // Delta dla cyklu v1 <-> i
            double delta = (useProfit ? (double)(data.gains[v1] + data.gains[i]) : 0.0)
                - (2.0 * data.distances[v1][i]);
            if (delta > bestInitialDelta) {
                bestInitialDelta = delta;
                v2 = i;
            }
        }
    }

    Sequence tour = { v1, v2 };
    visited[v2] = true;

    // 2. GŁÓWNA PĘTLA (Rozbudowa o 2-żal)
    while (tour.size() < n) {
        int bestCityToAdd = -1;
        int bestPositionToAdd = -1;
        double maxWeightedScore = -std::numeric_limits<double>::max();

        for (int u = 0; u < n; ++u) {
            if (visited[u]) continue;

            double bestDelta = -std::numeric_limits<double>::max();
            double secondBestDelta = -std::numeric_limits<double>::max();
            int currentBestPos = -1;

            for (int i = 0; i < tour.size(); ++i) {
                int cityI = tour[i];
                int cityJ = tour[(i + 1) % tour.size()];

                // Efektywna delta: Zysk - (NoweKrawedzie - StaraKrawedz)
                double distChange = data.distances[cityI][u] + data.distances[u][cityJ] - data.distances[cityI][cityJ];
                double delta = (useProfit ? (double)data.gains[u] : 0.0) - distChange;

                if (delta > bestDelta) {
                    secondBestDelta = bestDelta;
                    bestDelta = delta;
                    currentBestPos = i + 1;
                }
                else if (delta > secondBestDelta) {
                    secondBestDelta = delta;
                }
            }

            // 2-żal: jak dużo tracimy wybierając gorszą pozycję wstawienia dla tego wierzchołka
            // Jeśli tour ma 2 elementy, secondBestDelta mogła nie zostać przypisana
            double regret = (secondBestDelta == -std::numeric_limits<double>::max()) ? 0 : (bestDelta - secondBestDelta);

            // Ważony wynik: 
            // alpha * regret -> promujemy wierzchołki "pilne" (które mają tylko jedno dobre miejsce)
            // beta * bestDelta -> promujemy wierzchołki "opłacalne" (które mocno podnoszą funkcję celu)
            double weightedScore = (alpha * regret) + (beta * bestDelta);

            if (weightedScore > maxWeightedScore) {
                maxWeightedScore = weightedScore;
                bestCityToAdd = u;
                bestPositionToAdd = currentBestPos;
            }
        }

        if (bestCityToAdd != -1) {
            tour.insert(tour.begin() + bestPositionToAdd, bestCityToAdd);
            visited[bestCityToAdd] = true;
        }
    }

    // 3. FAZA II - Pruning (Usuwanie nieopłacalnych wierzchołków)
    return PruneTour(tour, data);
}