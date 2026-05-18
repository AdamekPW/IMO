#pragma once

#include "Structs.h"
#include "Common.h"

// =====================================================================
// ZADANIE 6 - Hybrydowy Algorytm Ewolucyjny (HAE)
// =====================================================================
// Populacja elitarna o wielkosci popSize (domyslnie 20).
// Algorytm steady-state: w petli wybieramy dwoch rodzicow, tworzymy
// potomka przez rekombinacje, opcjonalnie stosujemy LS, a nastepnie
// zastepujemy najgorsze rozwiazanie w populacji (jesli potomek jest
// lepszy i wystarczajaco rozny).
//
// Trzy operatory rekombinacji:
//   Op1 - wspolne wierzcholki i krawedzie -> podsciezki (wlaczajac
//          izolowane wierzcholki) -> losowe zlaczenie -> naprawa
//   Op2 - jeden rodzic jako baza; usuwamy krawedzie i wierzcholki
//          nieobecne w drugim rodzicu, wolne wierzcholki tez usuwamy;
//          losowe zlaczenie podsciezek -> naprawa
//   Op3 - jeden rodzic jako baza; usuwamy wierzcholki nieobecne
//          w drugim rodzicu (zachowujac kolejnosc) -> naprawa
//
// useLS=true: po rekombinacji stosujemy LocalSearch (steepest, Edge)
// useLS=false: LS pomijamy (stosujemy go tylko przy inicjalizacji
//              populacji poczatkowej)
// =====================================================================

struct HAEResult {
    Sequence  bestTour;
    double    bestScore  = -1e18;
    long long elapsedMs  = 0;
    int       iterations = 0;   // liczba rekombinacji (prób stworzenia potomka)
};

enum class HAEOperator { Op1, Op2, Op3 };

HAEResult HAE(const Data& data, long long timeLimitMs,
              HAEOperator op, bool useLS, int popSize = 20);
