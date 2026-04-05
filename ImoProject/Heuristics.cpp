#include "Common.h"
#include <vector>
#include <numeric> // dla std::iota
#include <random>
#include <chrono>

Sequence PruneTour(Sequence tour, const Data& data) {
    bool improved = true;

    // Pętla wykonuje się tak długo, jak długo znajdujemy wierzchołek do usunięcia.
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

AlgorithmResult SolveNN(const Data& data, bool useProfit) {
    int n = data.gains.size();
    if (n == 0) return { {}, 0.0 };

    Sequence tour;
    std::vector<bool> visited(n, false);

    // --- 1. FAZA I: Budowanie pełnego cyklu Hamiltona (wszystkie wierzchołki) ---
    int startCity = RandomNumber(0, n);
    tour.push_back(startCity);
    visited[startCity] = true;

    while (tour.size() < n) {
        int last = tour.back();
        int bestCity = -1;
        double bestDelta = -std::numeric_limits<double>::max();

        for (int i = 0; i < n; ++i) {
            if (visited[i]) continue;

            double delta = -data.distances[last][i];
            if (useProfit) {
                delta += (double)data.gains[i];
            }

            if (delta > bestDelta) {
                bestDelta = delta;
                bestCity = i;
            }
        }

        if (bestCity != -1) {
            tour.push_back(bestCity);
            visited[bestCity] = true;
        }
    }

    // --- OBLICZENIE DYSTANSU PO FAZIE I ---
    double phase1Dist = 0.0;
    for (int i = 0; i < n; ++i) {
        int current = tour[i];
        int next = tour[(i + 1) % n];
        phase1Dist += data.distances[current][next];
    }

    // --- 2. FAZA II: Usuwanie wierzchołków ---
    Sequence finalTour = PruneTour(tour, data);
    return { finalTour, phase1Dist };
}

AlgorithmResult SolveGC(const Data& data, bool useProfit) {
    int n = data.gains.size();
    if (n < 2) return { {}, 0.0 };

    std::vector<bool> visited(n, false);

    // --- 1. INICJALIZACJA: Wybór dwóch pierwszych punktów ---
    int v1 = RandomNumber(0, n);
    visited[v1] = true;

    int v2 = -1;
    double bestInitialDelta = -std::numeric_limits<double>::max();

    for (int i = 0; i < n; ++i) {
        if (visited[i]) continue;

        // Delta dla cyklu dwuelementowego (v1 <-> i)
        double delta = -2.0 * data.distances[v1][i];
        if (useProfit) {
            delta += (double)(data.gains[v1] + data.gains[i]);
        }

        if (delta > bestInitialDelta) {
            bestInitialDelta = delta;
            v2 = i;
        }
    }

    Sequence tour = { v1, v2 };
    visited[v2] = true;

    // --- 2. ROZBUDOWA: Budowanie pełnego cyklu (wszystkie wierzchołki) ---
    while (tour.size() < n) {
        int bestCity = -1;
        int insertPos = -1;
        double bestDelta = -std::numeric_limits<double>::max();

        for (int u = 0; u < n; ++u) {
            if (visited[u]) continue;

            for (int i = 0; i < (int)tour.size(); ++i) {
                int cityI = tour[i];
                int cityJ = tour[(i + 1) % tour.size()];

                double distChange = data.distances[cityI][u] + data.distances[u][cityJ] - data.distances[cityI][cityJ];
                double delta = -distChange;
                if (useProfit) {
                    delta += (double)data.gains[u];
                }

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

    // --- OBLICZENIE DYSTANSU PO FAZIE I (Pełny cykl Hamiltona) ---
    double phase1Dist = 0.0;
    for (int i = 0; i < (int)tour.size(); ++i) {
        int current = tour[i];
        int next = tour[(i + 1) % tour.size()];
        phase1Dist += data.distances[current][next];
    }

    // --- 3. FAZA II: Usuwanie deficytowych wierzchołków ---
    Sequence finalTour = PruneTour(tour, data);

    return { finalTour, phase1Dist };
}

AlgorithmResult SolveWeighted2Regret(const Data& data, bool useProfit, double alpha, double beta) {
    int n = data.gains.size();
    if (n < 2) return { {}, 0.0 };

    std::vector<bool> visited(n, false);

    // --- 1. INICJALIZACJA (Faza I - start) ---
    int v1 = RandomNumber(0, n);
    visited[v1] = true;

    int v2 = -1;
    double bestInitialDelta = -std::numeric_limits<double>::max();
    for (int i = 0; i < n; ++i) {
        if (visited[i]) continue;

        // Delta dla cyklu v1 <-> i
        double delta = -2.0 * data.distances[v1][i];
        if (useProfit) {
            delta += (double)(data.gains[v1] + data.gains[i]);
        }

        if (delta > bestInitialDelta) {
            bestInitialDelta = delta;
            v2 = i;
        }
    }

    Sequence tour = { v1, v2 };
    visited[v2] = true;

    // --- 2. GŁÓWNA PĘTLA (Rozbudowa o 2-żal) ---
    while (tour.size() < n) {
        int bestCityToAdd = -1;
        int bestPositionToAdd = -1;
        double maxWeightedScore = -std::numeric_limits<double>::max();

        for (int u = 0; u < n; ++u) {
            if (visited[u]) continue;

            double bestDelta = -std::numeric_limits<double>::max();
            double secondBestDelta = -std::numeric_limits<double>::max();
            int currentBestPos = -1;

            for (int i = 0; i < (int)tour.size(); ++i) {
                int cityI = tour[i];
                int cityJ = tour[(i + 1) % tour.size()];

                double distChange = data.distances[cityI][u] + data.distances[u][cityJ] - data.distances[cityI][cityJ];
                double delta = -distChange;
                if (useProfit) {
                    delta += (double)data.gains[u];
                }

                if (delta > bestDelta) {
                    secondBestDelta = bestDelta;
                    bestDelta = delta;
                    currentBestPos = i + 1;
                }
                else if (delta > secondBestDelta) {
                    secondBestDelta = delta;
                }
            }

            // 2-żal: różnica między najlepszą a drugą najlepszą opcją wstawienia
            double regret = (secondBestDelta == -std::numeric_limits<double>::max()) ? 0.0 : (bestDelta - secondBestDelta);

            // Ważony wynik łączący żal (pilność) i deltę (zyskowność)
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

    // --- OBLICZENIE DYSTANSU PO FAZIE I (Pełny cykl Hamiltona) ---
    double phase1Dist = 0.0;
    for (int i = 0; i < (int)tour.size(); ++i) {
        phase1Dist += data.distances[tour[i]][tour[(i + 1) % tour.size()]];
    }

    // --- 3. FAZA II: Usuwanie deficytowych wierzchołków ---
    Sequence finalTour = PruneTour(tour, data);

    return { finalTour, phase1Dist };
}


// Pomocnicza funkcja: pobiera listę wierzchołków spoza trasy
std::vector<int> getNodesOutside(const Sequence & tour, int totalN) {
    std::vector<bool> isIn(totalN, false);
    for (int v : tour) isIn[v] = true;

    std::vector<int> outside;
    for (int i = 0; i < totalN; ++i) {
        if (!isIn[i]) outside.push_back(i);
    }
    return outside;
}

// Przykładowa delta dla dodania wierzchołka v pomiędzy v1 i v2
double deltaAdd(const Sequence& tour, const Data& data, int pos, int newNode) {
    int n = tour.size();
    if (n == 0) return data.gains[newNode];

    // Wstawiamy newNode przed tour[pos]
    int prev = tour[(pos - 1 + n) % n];
    int next = tour[pos % n];

    double dDist = data.distances[prev][newNode] + data.distances[newNode][next] - data.distances[prev][next];
    return data.gains[newNode] - dDist;
}

double deltaRemove(const Sequence& tour, const Data& data, int pos) {
    int n = tour.size();
    if (n <= 3) return -1e18; // Nie pozwalamy na trasy krótsze niż 3 (zależy od założeń)

    int v = tour[pos];
    int prev = tour[(pos - 1 + n) % n];
    int next = tour[(pos + 1) % n];

    double dDist = data.distances[prev][next] - (data.distances[prev][v] + data.distances[v][next]);
    return -data.gains[v] - dDist;
}

double delta2Edge(const Sequence& tour, const Data& data, int i, int j) {
    int n = tour.size();
    int a = tour[i];
    int b = tour[(i + 1) % n];
    int c = tour[j];
    int d = tour[(j + 1) % n];
    // Zysk bez zmian, liczymy tylko różnicę dystansów
    return (data.distances[a][b] + data.distances[c][d]) - (data.distances[a][c] + data.distances[b][d]);
}

double delta2VertexIntra(const Sequence& tour, const Data& data, int i, int j) {
    int n = tour.size();
    int v_i = tour[i];
    int v_j = tour[j];
    int p_i = tour[(i - 1 + n) % n], n_i = tour[(i + 1) % n];
    int p_j = tour[(j - 1 + n) % n], n_j = tour[(j + 1) % n];

    double oldD, newD;
    if ((i + 1) % n == j) { // Sąsiedzi i -> j
        oldD = data.distances[p_i][v_i] + data.distances[v_i][v_j] + data.distances[v_j][n_j];
        newD = data.distances[p_i][v_j] + data.distances[v_j][v_i] + data.distances[v_i][n_j];
    }
    else if ((j + 1) % n == i) { // Sąsiedzi j -> i
        oldD = data.distances[p_j][v_j] + data.distances[v_j][v_i] + data.distances[v_i][n_i];
        newD = data.distances[p_j][v_i] + data.distances[v_i][v_j] + data.distances[v_j][n_i];
    }
    else {
        oldD = data.distances[p_i][v_i] + data.distances[v_i][n_i] + data.distances[p_j][v_j] + data.distances[v_j][n_j];
        newD = data.distances[p_i][v_j] + data.distances[v_j][n_i] + data.distances[p_j][v_i] + data.distances[v_i][n_j];
    }
    return oldD - newD;
}

// --- MANIPULACJA TRASĄ ---

void applyMove(Sequence& tour, const Move& m) {
    if (m.type == MoveType::Add) {
        tour.insert(tour.begin() + m.i, m.nodeVal);
    }
    else if (m.type == MoveType::Remove) {
        tour.erase(tour.begin() + m.i);
    }
    else if (m.type == MoveType::IntraVertex) {
        std::swap(tour[m.i], tour[m.j]);
    }
    else if (m.type == MoveType::IntraEdge) {
        int n = tour.size();
        int left = (m.i + 1) % n;
        int right = m.j;
        if (left <= right) {
            std::reverse(tour.begin() + left, tour.begin() + right + 1);
        }
        else {
            // Obsługa przypadku "zawiniętego" (opcjonalnie, zależy od generowania i,j)
            std::vector<int> temp;
            for (int k = 0; k < n; ++k) temp.push_back(tour[(left + k) % n]);
            int len = (m.j - left + n) % n + 1;
            std::reverse(temp.begin(), temp.begin() + len);
            for (int k = 0; k < n; ++k) tour[(left + k) % n] = temp[k];
        }
    }
}

Sequence LocalSearch(const Data& data, Sequence tour, bool steepest, Neighborhood nType) {
    std::mt19937 rng(std::random_device{}());
    bool improved = true;

    while (improved) {
        improved = false;
        Move bestMove;
        bestMove.delta = 0.0001; // Szukamy tylko ruchów > 0

        // Przygotowanie listy potencjalnych ruchów dla wersji zachłannej
        struct Potential { MoveType type; int i, j, val; };
        std::vector<Potential> pool;

        int n = tour.size();
        std::vector<int> outside = getNodesOutside(tour, data.n);

        // Generowanie wszystkich możliwych ruchów w tej iteracji
        for (int v : outside)
            for (int i = 0; i <= n; ++i) pool.push_back({ MoveType::Add, i, -1, v });

        for (int i = 0; i < n; ++i) pool.push_back({ MoveType::Remove, i, -1, -1 });

        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                if (nType == Neighborhood::Edge) pool.push_back({ MoveType::IntraEdge, i, j, -1 });
                else pool.push_back({ MoveType::IntraVertex, i, j, -1 });
            }
        }

        if (!steepest) std::shuffle(pool.begin(), pool.end(), rng);

        // Przeglądanie sąsiedztwa
        for (const auto& p : pool) {
            double d = 0;
            if (p.type == MoveType::Add) d = deltaAdd(tour, data, p.i, p.val);
            else if (p.type == MoveType::Remove) d = deltaRemove(tour, data, p.i);
            else if (p.type == MoveType::IntraEdge) d = delta2Edge(tour, data, p.i, p.j);
            else if (p.type == MoveType::IntraVertex) d = delta2VertexIntra(tour, data, p.i, p.j);

            if (d > bestMove.delta) {
                bestMove = { p.type, p.i, p.j, p.val, d };
                improved = true;
                if (!steepest) break; // Greedy: natychmiastowa akceptacja
            }
        }

        if (improved) applyMove(tour, bestMove);
    }
    return tour;
}



Sequence RandomWalk(const Data& data, Sequence tour, long long timeLimitMs, Neighborhood nType) {
    auto startTime = std::chrono::steady_clock::now();
    std::mt19937 rng(std::random_device{}());

    Sequence bestTour = tour;
    // Początkowa ocena - robimy to tylko RAZ na początku
    EvaluationResult initScore = CalculateTourMetrics(tour, data);
    double currentScore = initScore.totalGain - initScore.totalDistance;
    double bestScore = currentScore;

    while (true) {
        // Sprawdzenie czasu co każdą iterację
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        if (elapsed >= timeLimitMs) break;

        int moveType = std::uniform_int_distribution<>(0, 2)(rng); // 0:Add, 1:Remove, 2:Intra
        double delta = -1e18;

        if (moveType == 0) { // --- DODAWANIE ---
            std::vector<int> outside = getNodesOutside(tour, data.n);
            if (!outside.empty()) {
                int newNode = outside[std::uniform_int_distribution<>(0, (int)outside.size() - 1)(rng)];
                int pos = std::uniform_int_distribution<>(0, (int)tour.size())(rng);

                delta = deltaAdd(tour, data, pos, newNode);

                // W RW wykonujemy ruch niezależnie od oceny
                tour.insert(tour.begin() + pos, newNode);
                currentScore += delta;
            }
        }
        else if (moveType == 1 && tour.size() > 3) { // --- USUWANIE ---
            int pos = std::uniform_int_distribution<>(0, (int)tour.size() - 1)(rng);

            delta = deltaRemove(tour, data, pos);

            tour.erase(tour.begin() + pos);
            currentScore += delta;
        }
        else if (tour.size() >= 2) { // --- RUCH WEWNĄTRZTRASOWY ---
            int i = std::uniform_int_distribution<>(0, (int)tour.size() - 1)(rng);
            int j = std::uniform_int_distribution<>(0, (int)tour.size() - 1)(rng);
            if (i == j) continue;

            if (nType == Neighborhood::Edge) {
                // Dla 2-opt upewniamy się, że i < j dla uproszczenia reverse
                if (i > j) std::swap(i, j);
                delta = delta2Edge(tour, data, i, j);

                // Wykonujemy 2-opt (odwrócenie fragmentu)
                std::reverse(tour.begin() + (i + 1) % tour.size(), tour.begin() + j + 1);
            }
            else {
                delta = delta2VertexIntra(tour, data, i, j);
                std::swap(tour[i], tour[j]);
            }
            currentScore += delta;
        }

        // Sprawdzamy, czy w tym kroku znaleźliśmy historycznie najlepsze rozwiązanie
        if (currentScore > bestScore) {
            bestScore = currentScore;
            bestTour = tour;
        }
    }

    return bestTour;
}