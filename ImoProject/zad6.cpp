#include "zad6.h"
#include "zad4.h"
#include "Heuristics.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <random>
#include <unordered_set>

// =====================================================================
// Narzedzia lokalne
// =====================================================================

static std::mt19937& rng6() {
    static thread_local std::mt19937 gen(std::random_device{}());
    return gen;
}

// Koduje krawedz nieskierowana (u,v) jako jeden int: min*n + max.
static inline int edgeKey(int u, int v, int n) {
    if (u > v) std::swap(u, v);
    return u * n + v;
}

static inline double scoreOf6(const Sequence& tour, const Data& data) {
    EvaluationResult m = CalculateTourMetrics(tour, data);
    return (double)m.totalGain - (double)m.totalDistance;
}

// Generuje losowe rozwiazanie startowe o rozmiarze ~n/2 (jak w zad4).
static Sequence generateBalanced6(int n) {
    auto& gen = rng6();
    int k = (n + 1) / 2;
    std::vector<int> pool(n);
    std::iota(pool.begin(), pool.end(), 0);
    std::shuffle(pool.begin(), pool.end(), gen);
    return Sequence(pool.begin(), pool.begin() + k);
}

// =====================================================================
// Wyznaczanie wspolnych zbiorow wierzcholkow i krawedzi
// =====================================================================

// vCommon = wierzcholki wspolne dla p1 i p2.
// eCommon = krawedzie (nieskierowane) wspolne dla p1 i p2.
static void computeCommonSets(
    const Sequence& p1, const Sequence& p2, int n,
    std::unordered_set<int>& vCommon,
    std::unordered_set<int>& eCommon)
{
    std::unordered_set<int> vP2(p2.begin(), p2.end());
    for (int v : p1)
        if (vP2.count(v)) vCommon.insert(v);

    std::unordered_set<int> eP1, eP2;
    int m1 = (int)p1.size(), m2 = (int)p2.size();
    for (int k = 0; k < m1; ++k)
        eP1.insert(edgeKey(p1[k], p1[(k + 1) % m1], n));
    for (int k = 0; k < m2; ++k)
        eP2.insert(edgeKey(p2[k], p2[(k + 1) % m2], n));
    for (int e : eP1)
        if (eP2.count(e)) eCommon.insert(e);
}

// =====================================================================
// Ekstrakcja wspolnych podsciezek z p1
// =====================================================================
// Zbiera maksymalne ciagi wierzcholkow z p1, w ktorych:
//   - kazdy wierzcholek nalezy do vCommon, ORAZ
//   - kazda kolejna krawedz nalezy do eCommon.
//
// includeIsolated=true  (Op1): izolowane wspolne wierzcholki
//                              staja sie podsciezkami dlugosci 1.
// includeIsolated=false (Op2): izolowane wspolne wierzcholki
//                              ("wolne") sa pomijane.
static std::vector<Sequence> extractSubpaths(
    const Sequence& p1,
    const std::unordered_set<int>& vCommon,
    const std::unordered_set<int>& eCommon,
    int n,
    bool includeIsolated)
{
    int m = (int)p1.size();
    if (m == 0) return {};

    // Szukamy pozycji startowej: miejsca, w ktorym lancuch sie przerywa.
    // Zapewnia poprawna obsluge cyklicznosci trasy.
    int startPos = 0;
    for (int k = 0; k < m; ++k) {
        int v     = p1[k];
        int vPrev = p1[(k - 1 + m) % m];
        if (!vCommon.count(v) ||
            !eCommon.count(edgeKey(vPrev, v, n))) {
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

        bool vInCommon    = vCommon.count(v)              > 0;
        bool nextInCommon = vCommon.count(vNext)          > 0;
        bool edgeInCommon = eCommon.count(edgeKey(v, vNext, n)) > 0;

        if (vInCommon) current.push_back(v);

        // Lancuch przerywa sie gdy: biezacy nie jest wspolny,
        // lub krawedz do nastepnego nie spelnia warunku wspolnosci.
        bool chainBreaks = !vInCommon || !nextInCommon || !edgeInCommon;

        if (chainBreaks && !current.empty()) {
            if (includeIsolated || (int)current.size() > 1)
                result.push_back(std::move(current));
            current.clear();
        }
    }
    if (!current.empty()) {
        if (includeIsolated || (int)current.size() > 1)
            result.push_back(std::move(current));
    }
    return result;
}

// =====================================================================
// Losowe zlaczenie podsciezek w jedna czesciowa trase
// =====================================================================
static Sequence joinSubpaths(std::vector<Sequence> subpaths) {
    auto& gen = rng6();
    std::shuffle(subpaths.begin(), subpaths.end(), gen);
    Sequence partial;
    for (auto& sp : subpaths) {
        if (std::uniform_int_distribution<int>(0, 1)(gen))
            std::reverse(sp.begin(), sp.end());
        partial.insert(partial.end(), sp.begin(), sp.end());
    }
    return partial;
}

// =====================================================================
// Operatory rekombinacji
// =====================================================================

// Op1: wspolne wierzcholki i krawedzie tworza podsciezki (w tym
// izolowane wierzcholki jako podsciezki dlugosci 1). Podsciezki
// laczone losowo -> RepairWeighted2Regret.
static Sequence recombineOp1(const Sequence& p1, const Sequence& p2,
                              const Data& data)
{
    std::unordered_set<int> vCommon, eCommon;
    computeCommonSets(p1, p2, data.n, vCommon, eCommon);

    auto subpaths = extractSubpaths(p1, vCommon, eCommon, data.n, true);
    if (subpaths.empty())
        return RepairWeighted2Regret({}, data);

    Sequence partial = joinSubpaths(std::move(subpaths));
    return RepairWeighted2Regret(std::move(partial), data);
}

// Op2: baza = p1; usuwamy krawedzie i wierzcholki nieobecne w p2;
// wolne wierzcholki (izolowane, obydwie krawedzie usuniete) tez
// usuwamy. Pozostale podsciezki laczone losowo -> naprawa.
//
// Poprawka wzgledem extractSubpaths: usuwamy najpierw wierzcholki
// (zachowujac kolejnosc p1), co moze tworzyc krawedzie pomostowe
// (u,w) nieobecne w oryginalnym p1; nastepnie sprawdzamy te krawedzie
// wzgledem krawedzi p2 - jesli sa wspolne, lacza podsciezki.
static Sequence recombineOp2(const Sequence& p1, const Sequence& p2,
                              const Data& data)
{
    // Krok 1: usun wierzcholki nieobecne w p2 (zachowaj kolejnosc p1).
    // Odpowiada to "dodaniu krawedzi laczacej sasiadow" przy kazdym
    // usunietym wierzcholku - wynikiem jest podciag wspolnych wierzcholkow.
    std::unordered_set<int> vP2(p2.begin(), p2.end());
    Sequence base;
    for (int v : p1)
        if (vP2.count(v)) base.push_back(v);

    int m = (int)base.size();
    if (m < 2)
        return RepairWeighted2Regret({}, data);

    // Krok 2: zbior krawedzi p2 (nieskierowanych).
    std::unordered_set<int> eP2;
    int m2 = (int)p2.size();
    for (int k = 0; k < m2; ++k)
        eP2.insert(edgeKey(p2[k], p2[(k + 1) % m2], data.n));

    // Krok 3: szukamy pozycji startowej (miejsce przelamania lancucha).
    int startPos = 0;
    for (int k = 0; k < m; ++k) {
        int vPrev = base[(k - 1 + m) % m];
        int v     = base[k];
        if (!eP2.count(edgeKey(vPrev, v, data.n))) {
            startPos = k;
            break;
        }
    }

    // Krok 4: wycinamy podsciezki - segmenty polaczone krawedzami z eP2.
    // Wolne wierzcholki (izolowane, obydwie krawedzie spoza eP2) odrzucamy
    // (rozmiar segmentu < 2).
    std::vector<Sequence> subpaths;
    Sequence current;

    for (int k = 0; k < m; ++k) {
        int pos   = (startPos + k) % m;
        int v     = base[pos];
        int vNext = base[(pos + 1) % m];

        current.push_back(v);

        if (!eP2.count(edgeKey(v, vNext, data.n))) {
            if ((int)current.size() > 1)
                subpaths.push_back(std::move(current));
            current.clear();
        }
    }
    if (!current.empty() && (int)current.size() > 1)
        subpaths.push_back(std::move(current));

    if (subpaths.empty())
        return RepairWeighted2Regret({}, data);

    return RepairWeighted2Regret(joinSubpaths(std::move(subpaths)), data);
}

// Op3: baza = p1; usuwamy wierzcholki nieobecne w p2 (zachowujac
// kolejnosc p1). Wynikowa czesc trasy -> naprawa.
static Sequence recombineOp3(const Sequence& p1, const Sequence& p2,
                              const Data& data)
{
    std::unordered_set<int> vP2(p2.begin(), p2.end());
    Sequence partial;
    for (int v : p1)
        if (vP2.count(v)) partial.push_back(v);

    return RepairWeighted2Regret(std::move(partial), data);
}

// =====================================================================
// HAE - Hybrydowy Algorytm Ewolucyjny
// =====================================================================
HAEResult HAE(const Data& data, long long timeLimitMs,
              HAEOperator op, bool useLS, int popSize)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    auto& gen = rng6();

    // --- Inicjalizacja populacji ---
    // Staramy sie wypelnic populacje roznorodnie (po wartosci f.c.).
    std::vector<Sequence> population;
    std::vector<double>   scores;
    population.reserve(popSize);
    scores.reserve(popSize);

    const int maxAttempts = popSize * 20;
    int attempt = 0;
    while ((int)population.size() < popSize && attempt < maxAttempts) {
        Sequence s = generateBalanced6(data.n);
        s = LocalSearch(data, s, true, Neighborhood::Edge);
        double sc = scoreOf6(s, data);

        bool dup = false;
        for (double ex : scores)
            if (std::abs(ex - sc) < 1e-6) { dup = true; break; }

        if (!dup) {
            population.push_back(s);
            scores.push_back(sc);
        }
        ++attempt;
    }
    // Uzupelniamy bez sprawdzania roznorodnosci jesli zabraklo prob.
    while ((int)population.size() < popSize) {
        Sequence s = generateBalanced6(data.n);
        s = LocalSearch(data, s, true, Neighborhood::Edge);
        population.push_back(s);
        scores.push_back(scoreOf6(s, data));
    }

    HAEResult result;
    for (int i = 0; i < popSize; ++i) {
        if (scores[i] > result.bestScore) {
            result.bestScore = scores[i];
            result.bestTour  = population[i];
        }
    }

    // --- Petla glowna (steady-state) ---
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                now - t0).count() >= timeLimitMs)
            break;

        // Losuj dwoch roznych rodzicow
        int p1idx = std::uniform_int_distribution<int>(0, popSize - 1)(gen);
        int p2idx;
        do {
            p2idx = std::uniform_int_distribution<int>(0, popSize - 1)(gen);
        } while (p2idx == p1idx);

        // Rekombinacja
        Sequence offspring;
        switch (op) {
        case HAEOperator::Op1:
            offspring = recombineOp1(population[p1idx], population[p2idx], data);
            break;
        case HAEOperator::Op2:
            offspring = recombineOp2(population[p1idx], population[p2idx], data);
            break;
        case HAEOperator::Op3:
            offspring = recombineOp3(population[p1idx], population[p2idx], data);
            break;
        }

        if (useLS)
            offspring = LocalSearch(data, offspring, true, Neighborhood::Edge);

        result.iterations++;
        double offScore = scoreOf6(offspring, data);

        // Znajdz najgorsze rozwiazanie w populacji
        int worstIdx = (int)(std::min_element(scores.begin(), scores.end())
                             - scores.begin());

        if (offScore > scores[worstIdx]) {
            // Sprawdz roznorodnosc (po wartosci f.c.)
            bool dup = false;
            for (double sc : scores)
                if (std::abs(sc - offScore) < 1e-6) { dup = true; break; }

            if (!dup) {
                population[worstIdx] = std::move(offspring);
                scores[worstIdx]     = offScore;
                if (offScore > result.bestScore) {
                    result.bestScore = offScore;
                    result.bestTour  = population[worstIdx];
                }
            }
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        t1 - t0).count();
    return result;
}
