#pragma once

#include <vector>
#include "Common.h"
#include "Structs.h"
#include "Heuristics.h"

// =====================================================================
// ZADANIE 4 - Rozszerzenia lokalnego przeszukiwania
// =====================================================================
// Lokalnym przeszukiwaniem jest najlepsza metoda z poprzednich zajec:
//     LocalSearch(data, start, true, Neighborhood::Edge)
// (Steepest Edge - sasiedztwo wymiany krawedzi, strategia stroma).
//
// Najlepsza heurystyka konstrukcyjna z zad. 1:
//     SolveWeighted2Regret(data, false, 1.0, 0.0)
// (2-zal bez zysku, alpha=1.0, beta=0.0).
// =====================================================================


// ---------------------------------------------------------------------
// Wynik MSLS
// ---------------------------------------------------------------------
struct MSLSResult {
    Sequence bestTour;            // najlepsza znaleziona trasa
    double   bestScore = -1e18;   // zysk - dystans
    long long elapsedMs = 0;      // calkowity czas wykonania (jednego MSLS)
    int      iterations = 0;      // liczba uruchomien LS (zwykle 200)
};

// Multiple Start Local Search:
//   Powtarzaj:
//       Wygeneruj losowe rozwiazanie startowe x
//       x := LocalSearch(x)
//       Zapamietaj najlepsze x
//   Az do wykonania `iterations` przebiegow
// Zwraca najlepsze rozwiazanie + statystyki czasowe.
MSLSResult MSLS(const Data& data, int iterations = 200);


// ---------------------------------------------------------------------
// Perturbacja ILS - "mala" perturbacja
// ---------------------------------------------------------------------
// Wykonuje `numMoves` losowych ruchow na trasie:
//   - 2-opt na losowych krawedziach (odwrocenie podsekwencji),
//   - swap inside-outside: podmiana wierzcholka z trasy na losowy spoza trasy.
// Typ ruchu jest losowany dla kazdej z `numMoves` modyfikacji.
// Domyslnie 6 ruchow - tyle wystarcza, by wybic z minimum lokalnego nawet
// w stabilnych regionach przestrzeni rozwiazan.
Sequence PerturbILS(Sequence tour, const Data& data, int numMoves = 6);


// ---------------------------------------------------------------------
// Perturbacja LNS - Destroy + Repair
// ---------------------------------------------------------------------

// DESTROY (wariant losowy):
// Usuwa frakcje `removeFraction` (np. 0.30) wierzcholkow z trasy,
// wybierajac je calkowicie losowo. Pozostawia minimum 2 wierzcholki.
Sequence DestroyRandom(Sequence tour, double removeFraction = 0.3);

// DESTROY (wariant heurystyczny):
// Usuwa frakcje `removeFraction` wierzcholkow probabilistycznie - wieksza
// szansa usuniecia jest dla wierzcholkow o duzym koszcie:
//     cost(v) = dist(prev,v) + dist(v,next) - dist(prev,next) - gain(v)
// (czyli "drogie" wierzcholki o malym zysku znikaja czesciej).
// Selekcja kolem ruletki proporcjonalnie do (cost - min_cost + 1).
Sequence DestroyHeuristic(Sequence tour, const Data& data, double removeFraction = 0.3);

// DESTROY (wariant z usuwaniem podsciezki):
// Usuwa losowy ciagly fragment trasy o dlugosci ~`removeFraction` * |tour|.
// Preferuje "dluzsze podsciezki" - podpowiedz z tresci zadania.
Sequence DestroySegment(Sequence tour, double removeFraction = 0.3);

// REPAIR:
// Uzupelnia czesciowa trase do pelnego cyklu Hamiltona (n wierzcholkow)
// metoda Weighted 2-Regret BEZ zysku (alpha=1.0, beta=0.0) - czyli najlepszej
// heurystyki konstrukcyjnej z zad. 1. Nastepnie wykonuje PruneTour, ktory
// usuwa wierzcholki deficytowe.
Sequence RepairWeighted2Regret(Sequence partialTour, const Data& data);


// Strategia wyboru wierzcholkow do usuniecia w LNS:
//   Random    - wierzcholki wybierane calkowicie losowo
//   Heuristic - selekcja ruletka, preferowani "drodzy" wierzcholki
//   Segment   - usuwanie ciaglej podsciezki (zgodnie z podpowiedzia z PDF)
enum class DestroyStrategy { Random, Heuristic, Segment };


// =====================================================================
// ALGORYTMY: ILS, LNS, LNSa
// =====================================================================
// Wszystkie algorytmy zatrzymywane sa po przekroczeniu `timeLimitMs`.
// W ramach eksperymentu jest to sredni czas pojedynczego MSLS dla danej
// instancji.

// Wynik dla algorytmow z czasowym warunkiem stopu
struct ILSResult {
    Sequence bestTour;
    double   bestScore = -1e18;   // zysk - dystans
    long long elapsedMs = 0;      // calkowity czas wykonania
    int      perturbations = 0;   // liczba wykonanych perturbacji (dla ILS/LNS)
};

// Iterated Local Search:
//   x := LS(losowy)
//   Powtarzaj:
//       y := PerturbILS(x)
//       y := LS(y)
//       jesli f(y) > f(x): x := y
//   az do timeLimitMs
ILSResult ILS(const Data& data, long long timeLimitMs, int perturbMoves = 4);

// Large Neighborhood Search (z LS w petli):
//   x := LS(losowy)
//   Powtarzaj:
//       y := Destroy(x)        // wybor strategii: Random/Heuristic/Segment
//       y := Repair(y)         // 2-zal bez zysku
//       y := LS(y)
//       jesli f(y) > f(x): x := y
//   az do timeLimitMs
// Domyslnie strategia Segment (usuwanie podsciezki) - zwykle najmocniej
// wybija LS z biezacego basenu przyciagania.
ILSResult LNS(const Data& data, long long timeLimitMs,
              double removeFraction = 0.3,
              DestroyStrategy strategy = DestroyStrategy::Segment);

// LNS bez LS w petli (LS tylko na rozwiazaniu startowym, bo bylo losowe):
//   x := LS(losowy)
//   Powtarzaj:
//       y := Destroy(x)
//       y := Repair(y)
//       jesli f(y) > f(x): x := y
//   az do timeLimitMs
ILSResult LNSa(const Data& data, long long timeLimitMs,
               double removeFraction = 0.3,
               DestroyStrategy strategy = DestroyStrategy::Segment);
