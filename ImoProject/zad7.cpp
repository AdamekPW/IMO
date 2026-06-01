#include "zad7.h"
#include "zad3.h"
#include "zad4.h"
#include "Heuristics.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <random>
#include <unordered_set>
#include <omp.h>

// =====================================================================
// Narzedzia lokalne
// =====================================================================

static std::mt19937& rng7() {
    static thread_local std::mt19937 gen(std::random_device{}());
    return gen;
}

static inline int edgeKey7(int u, int v, int n) {
    if (u > v) std::swap(u, v);
    return u * n + v;
}

static inline double scoreOf7(const Sequence& tour, const Data& data) {
    EvaluationResult m = CalculateTourMetrics(tour, data);
    return (double)m.totalGain - (double)m.totalDistance;
}

static Sequence generateRandom7(int n) {
    auto& gen = rng7();
    int k = (n + 1) / 2;
    std::vector<int> pool(n);
    std::iota(pool.begin(), pool.end(), 0);
    std::shuffle(pool.begin(), pool.end(), gen);
    return Sequence(pool.begin(), pool.begin() + k);
}

// =====================================================================
// Pomocnicze funkcje rekombinacji (adaptowane z zad6)
// =====================================================================

static void computeCommon7(
    const Sequence& p1, const Sequence& p2, int n,
    std::unordered_set<int>& vCommon,
    std::unordered_set<int>& eCommon)
{
    std::unordered_set<int> vP2(p2.begin(), p2.end());
    for (int v : p1)
        if (vP2.count(v)) vCommon.insert(v);

    std::unordered_set<int> eP1, eP2;
    int m1 = (int)p1.size(), m2 = (int)p2.size();
    for (int k = 0; k < m1; ++k) eP1.insert(edgeKey7(p1[k], p1[(k+1)%m1], n));
    for (int k = 0; k < m2; ++k) eP2.insert(edgeKey7(p2[k], p2[(k+1)%m2], n));
    for (int e : eP1)
        if (eP2.count(e)) eCommon.insert(e);
}

static std::vector<Sequence> extractSubpaths7(
    const Sequence& p1,
    const std::unordered_set<int>& vCommon,
    const std::unordered_set<int>& eCommon,
    int n, bool includeIsolated)
{
    int m = (int)p1.size();
    if (m == 0) return {};

    int startPos = 0;
    for (int k = 0; k < m; ++k) {
        int v     = p1[k];
        int vPrev = p1[(k - 1 + m) % m];
        if (!vCommon.count(v) || !eCommon.count(edgeKey7(vPrev, v, n))) {
            startPos = k;
            break;
        }
    }

    std::vector<Sequence> result;
    Sequence current;

    for (int k = 0; k < m; ++k) {
        int pos   = (startPos + k) % m;
        int v     = p1[pos];
        int vNext = p1[(pos + 1) % m];

        bool vIn      = vCommon.count(v)                     > 0;
        bool nextIn   = vCommon.count(vNext)                 > 0;
        bool edgeIn   = eCommon.count(edgeKey7(v, vNext, n)) > 0;

        if (vIn) current.push_back(v);

        bool chainBreaks = !vIn || !nextIn || !edgeIn;

        if (chainBreaks && !current.empty()) {
            if (includeIsolated || (int)current.size() > 1)
                result.push_back(std::move(current));
            current.clear();
        }
    }
    if (!current.empty())
        if (includeIsolated || (int)current.size() > 1)
            result.push_back(std::move(current));
    return result;
}

static Sequence joinSubpaths7(std::vector<Sequence> subpaths) {
    auto& gen = rng7();
    std::shuffle(subpaths.begin(), subpaths.end(), gen);
    Sequence partial;
    for (auto& sp : subpaths) {
        if (std::uniform_int_distribution<int>(0, 1)(gen))
            std::reverse(sp.begin(), sp.end());
        partial.insert(partial.end(), sp.begin(), sp.end());
    }
    return partial;
}

// Op1: wspolne wierzcholki i krawedzie -> podsciezki (wlaczajac izolowane)
static Sequence recombineOp1_7(const Sequence& p1, const Sequence& p2,
                                const Data& data)
{
    std::unordered_set<int> vCommon, eCommon;
    computeCommon7(p1, p2, data.n, vCommon, eCommon);
    auto subpaths = extractSubpaths7(p1, vCommon, eCommon, data.n, true);
    if (subpaths.empty())
        return RepairWeighted2Regret({}, data);
    return RepairWeighted2Regret(joinSubpaths7(std::move(subpaths)), data);
}

// Op2: baza = p1; usun krawedzie i wierzcholki nieobecne w p2
static Sequence recombineOp2_7(const Sequence& p1, const Sequence& p2,
                                const Data& data)
{
    std::unordered_set<int> vP2(p2.begin(), p2.end());
    Sequence base;
    for (int v : p1)
        if (vP2.count(v)) base.push_back(v);

    int m = (int)base.size();
    if (m < 2)
        return RepairWeighted2Regret({}, data);

    std::unordered_set<int> eP2;
    int m2 = (int)p2.size();
    for (int k = 0; k < m2; ++k)
        eP2.insert(edgeKey7(p2[k], p2[(k+1)%m2], data.n));

    int startPos = 0;
    for (int k = 0; k < m; ++k) {
        int vPrev = base[(k - 1 + m) % m];
        int v     = base[k];
        if (!eP2.count(edgeKey7(vPrev, v, data.n))) { startPos = k; break; }
    }

    std::vector<Sequence> subpaths;
    Sequence current;

    for (int k = 0; k < m; ++k) {
        int pos   = (startPos + k) % m;
        int v     = base[pos];
        int vNext = base[(pos + 1) % m];
        current.push_back(v);
        if (!eP2.count(edgeKey7(v, vNext, data.n))) {
            if ((int)current.size() > 1) subpaths.push_back(std::move(current));
            current.clear();
        }
    }
    if (!current.empty() && (int)current.size() > 1)
        subpaths.push_back(std::move(current));

    if (subpaths.empty())
        return RepairWeighted2Regret({}, data);
    return RepairWeighted2Regret(joinSubpaths7(std::move(subpaths)), data);
}

// =====================================================================
// OwnMethod - HAE-LNS z ruchami kandydackimi (HLNS-C)
// =====================================================================
OwnResult OwnMethod(const Data& data, long long timeLimitMs, int popSize)
{
    auto t0  = std::chrono::high_resolution_clock::now();
    auto& gen = rng7();

    // Wstepne obliczenie listy kandydatow (k=10 najblizszych sasiadow)
    const int K_NEIGHBORS = 10;
    auto nearestNeighbors = build_candidate_edges(data, K_NEIGHBORS);

    // Szybkie przeszukiwanie lokalne oparte na ruchach kandydackich
    auto candidateLS = [&](Sequence s) -> Sequence {
        return LocalSearchCandidates(data, s, nearestNeighbors);
    };

    // --- Inicjalizacja populacji ---
    std::vector<Sequence> population;
    std::vector<double>   scores;
    population.reserve(popSize);
    scores.reserve(popSize);

    const int maxAttempts = popSize * 20;
    int attempt = 0;
    while ((int)population.size() < popSize && attempt < maxAttempts) {
        Sequence s = generateRandom7(data.n);
        s = candidateLS(s);
        double sc = scoreOf7(s, data);

        bool dup = false;
        for (double ex : scores)
            if (std::abs(ex - sc) < 1e-6) { dup = true; break; }

        if (!dup) { population.push_back(s); scores.push_back(sc); }
        ++attempt;
    }
    while ((int)population.size() < popSize) {
        Sequence s = generateRandom7(data.n);
        s = candidateLS(s);
        population.push_back(s);
        scores.push_back(scoreOf7(s, data));
    }

    OwnResult result;
    for (int i = 0; i < popSize; ++i) {
        if (scores[i] > result.bestScore) {
            result.bestScore = scores[i];
            result.bestTour  = population[i];
        }
    }

    // Rozklady prawdopodobienstw operatorow:
    //   - 0.40 -> Op1 (zachowuje wspolne wierzcholki i krawedzie)
    //   - 0.30 -> Op2 (baza = lepszy rodzic, usuwa niespojne krawedzie)
    //   - 0.30 -> LNS (Destroy-Segment + Repair) - dywersyfikacja
    std::uniform_real_distribution<double> opDist(0.0, 1.0);
    std::uniform_int_distribution<int>     popDist(0, popSize - 1);

    const int    STAGNATION_LIMIT = 50;
    const double LNS_FRAC         = 0.30;
    int stagnation = 0;

    // --- Petla glowna (steady-state) ---
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                now - t0).count() >= timeLimitMs)
            break;

        Sequence offspring;
        double op = opDist(gen);

        if (stagnation >= STAGNATION_LIMIT || op >= 0.70) {
            // LNS: losuj osobnika, zniszcz fragment, napraw, zastosuj LS
            int idx = popDist(gen);
            offspring = DestroySegment(population[idx], LNS_FRAC);
            offspring = RepairWeighted2Regret(std::move(offspring), data);
            offspring = candidateLS(offspring);
            if (stagnation >= STAGNATION_LIMIT) stagnation = 0;
        } else {
            // Rekombinacja: dwoch losowych rodzicow
            int p1idx = popDist(gen);
            int p2idx;
            do { p2idx = popDist(gen); } while (p2idx == p1idx);

            if (op < 0.40)
                offspring = recombineOp1_7(population[p1idx], population[p2idx], data);
            else
                offspring = recombineOp2_7(population[p1idx], population[p2idx], data);

            offspring = candidateLS(offspring);
        }

        result.iterations++;
        double offScore = scoreOf7(offspring, data);

        // Znajdz najgorsze rozwiazanie w populacji
        int worstIdx = (int)(std::min_element(scores.begin(), scores.end())
                             - scores.begin());

        if (offScore > scores[worstIdx]) {
            bool dup = false;
            for (double sc : scores)
                if (std::abs(sc - offScore) < 1e-6) { dup = true; break; }

            if (!dup) {
                population[worstIdx] = std::move(offspring);
                scores[worstIdx]     = offScore;
                if (offScore > result.bestScore) {
                    result.bestScore = offScore;
                    result.bestTour  = population[worstIdx];
                    stagnation = 0;
                } else {
                    ++stagnation;
                }
            } else {
                ++stagnation;
            }
        } else {
            ++stagnation;
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        t1 - t0).count();
    return result;
}

// =====================================================================
// OwnMethodParallel - model wyspowy z OpenMP
// =====================================================================
OwnResult OwnMethodParallel(const Data& data, long long timeLimitMs,
                             int popSize, int numIslands)
{
    std::vector<OwnResult> islandResults(numIslands);

    #pragma omp parallel for num_threads(numIslands) schedule(static, 1)
    for (int i = 0; i < numIslands; ++i)
        islandResults[i] = OwnMethod(data, timeLimitMs, popSize);

    return *std::max_element(islandResults.begin(), islandResults.end(),
        [](const OwnResult& a, const OwnResult& b) {
            return a.bestScore < b.bestScore;
        });
}
