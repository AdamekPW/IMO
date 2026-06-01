#pragma once

#include "Structs.h"
#include "Common.h"

// =====================================================================
// ZADANIE 7 - Wlasna metoda: HAE-LNS z ruchami kandydackimi (HLNS-C)
// =====================================================================
// Algorytm laczacy:
//   - populacyjna strukture HAE (populacja elitarna, steady-state),
//   - operatory rekombinacji Op1 i Op2 z zad6,
//   - perturbacje LNS (Destroy-Segment + Repair) jako operator
//     dywersyfikacji,
//   - szybsze przeszukiwanie lokalne oparte na ruchach kandydackich
//     (LocalSearchCandidates z zad3) zamiast pelnego Steepest-Edge LS.
//
// Glowne roznice wzgledem HAE (zad6):
//   1. Uzywamy LocalSearchCandidates (szybszy LS -> wiecej iteracji).
//   2. W kazdej iteracji losujemy operator:
//        - z prawdopodobienstwem P_RECOMB1: rekombinacja Op1
//        - z prawdopodobienstwem P_RECOMB2: rekombinacja Op2
//        - z prawdopodobienstwem P_LNS:     LNS (Destroy + Repair)
//   3. Detekcja stagnacji: gdy przez STAGNATION_LIMIT kolejnych iteracji
//      nie poprawia sie najlepsze rozwiazanie w populacji, wymuszamy
//      LNS jako "reset" i wstrzykujemy swiezego osobnika.
// =====================================================================

struct OwnResult {
    Sequence  bestTour;
    double    bestScore  = -1e18;
    long long elapsedMs  = 0;
    int       iterations = 0;
};

OwnResult OwnMethod(const Data& data, long long timeLimitMs, int popSize = 20);

// Model wyspowy: uruchamia numIslands niezaleznych instancji OwnMethod rownoleg
// (OpenMP), kazda z pelnym limitem timeLimitMs. Zwraca najlepszy wynik sposrod
// wszystkich wysp.
OwnResult OwnMethodParallel(const Data& data, long long timeLimitMs,
                             int popSize = 20, int numIslands = 4);
