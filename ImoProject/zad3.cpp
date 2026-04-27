#include "zad3.h"


std::vector<std::vector<int>> build_candidate_edges(const Data& data, int k = 10) {
    std::vector<std::vector<int>> nearest_neighbors(data.n);

    for (int i = 0; i < data.n; ++i) {
        std::vector<std::pair<double, int>> candidates;
        candidates.reserve(data.n - 1);

        for (int j = 0; j < data.n; ++j) {
            if (i == j) continue;
            candidates.push_back({ data.distances[i][j], j });
        }

        std::sort(candidates.begin(), candidates.end(),
            [](const auto& a, const auto& b) {
                return a.first < b.first;
            });

        int limit = std::min(k, (int)candidates.size());
        nearest_neighbors[i].reserve(limit);

        for (int t = 0; t < limit; ++t) {
            nearest_neighbors[i].push_back(candidates[t].second);
        }
    }

    return nearest_neighbors;
}

// ---------- aktualne delty, liczone na bieżąco ----------





static double current_2opt_delta(const Data& data, const TourManager& tm, int n1, int s1, int n2, int s2) {
    return (data.distances[n1][s1] + data.distances[n2][s2])
        - (data.distances[n1][n2] + data.distances[s1][s2]);
}

// ---------- budowa LM ----------

void build_LM_only(const Data& data,
    const TourManager& tm,
    std::vector<LMMove>& LM) {
    LM.clear();
    const double EPSILON = 1e-9;
    int m = (int)tm.tour.size();

    // 1. EDGE_SWAP (pełne sąsiedztwo, bez kandydatów)
    // ma sens dopiero od 4 wierzchołków
    if (m >= 4) {
        for (int n1 : tm.tour) {
            int s1 = tm.succ[n1];

            for (int n2 : tm.tour) {
                if (tm.pos[n2] == -1) continue;

                int s2 = tm.succ[n2];

                // przypadki degeneracyjne
                if (n1 == n2 || n1 == s2 || s1 == n2 || s1 == s2) continue;
                if (s1 == n2 || s2 == n1) continue;

                double d =
                    (data.distances[n1][s1] + data.distances[n2][s2]) -
                    (data.distances[n1][n2] + data.distances[s1][s2]);

                if (d > EPSILON) {
                    // wariant zgodny z aktualnym kierunkiem
                    {
                        LMMove mv;
                        mv.type = EDGE_SWAP;
                        mv.n1 = n1;
                        mv.s1 = s1;
                        mv.n2 = n2;
                        mv.s2 = s2;
                        mv.delta = d;
                        LM.push_back(mv);
                    }

                    // wariant dla odwróconego względnego kierunku
                    {
                        LMMove mv;
                        mv.type = EDGE_SWAP;
                        mv.n1 = s1;
                        mv.s1 = n1;
                        mv.n2 = s2;
                        mv.s2 = n2;
                        mv.delta = d;
                        LM.push_back(mv);
                    }
                }
            }
        }
    }

    // 2. ADD
    if (m == 0) {
        // dodanie pierwszego wierzchołka do pustej trasy
        for (int v = 0; v < data.n; ++v) {
            if (tm.pos[v] != -1) continue;

            double d = data.gains[v];

            if (d > EPSILON) {
                LMMove mv;
                mv.type = ADD;
                mv.node = v;
                mv.add_after = -1;
                mv.add_before = -1;
                mv.delta = d;
                LM.push_back(mv);
            }
        }
    }
    else {
        for (int v = 0; v < data.n; ++v) {
            if (tm.pos[v] != -1) continue; // tylko wierzchołki spoza trasy

            for (int after : tm.tour) {
                int before = tm.succ[after];

                double d =
                    data.gains[v] -
                    (data.distances[after][v] +
                        data.distances[v][before] -
                        data.distances[after][before]);

                if (d > EPSILON) {
                    LMMove mv;
                    mv.type = ADD;
                    mv.node = v;
                    mv.add_after = after;
                    mv.add_before = before;
                    mv.delta = d;
                    LM.push_back(mv);
                }
            }
        }
    }

    // 3. REMOVE
    // dopuszczamy usunięcie wszystkiego, więc od m >= 1
    if (m >= 1) {
        for (int v : tm.tour) {
            double d = 0.0;

            if (m == 1) {
                // po usunięciu zostaje pusto
                d = -data.gains[v];
            }
            else if (m == 2) {
                // po usunięciu jednego wierzchołka zostaje 1-elementowa trasa
                int idx = tm.pos[v];
                int other = tm.tour[(idx + 1) % m];

                d = -data.gains[v] + 2.0 * data.distances[v][other];
            }
            else {
                int idx = tm.pos[v];
                int prev = tm.tour[(idx - 1 + m) % m];
                int next = tm.succ[v];

                d =
                    -data.gains[v] +
                    (data.distances[prev][v] +
                        data.distances[v][next] -
                        data.distances[prev][next]);
            }

            if (d > EPSILON) {
                LMMove mv;
                mv.type = REMOVE;
                mv.node = v;

                if (m >= 2) {
                    int idx = tm.pos[v];
                    mv.rem_prev = tm.tour[(idx - 1 + m) % m];
                    mv.rem_next = tm.succ[v];
                }
                else {
                    mv.rem_prev = -1;
                    mv.rem_next = -1;
                }

                mv.delta = d;
                LM.push_back(mv);
            }
        }
    }

    std::sort(LM.begin(), LM.end(), std::greater<LMMove>());
}

Sequence LocalSearchLMOnly(const Data& data, Sequence& start_node_list) {
    const double EPSILON = 1e-9;

    TourManager tm(start_node_list, data.n);
    std::vector<LMMove> LM;

    build_LM_only(data, tm, LM);

    while (true) {
        bool move_applied = false;

        for (auto it = LM.begin(); it != LM.end(); ) {
            LMMove m = *it;

            if (m.type == EDGE_SWAP) {
                bool e1 = tm.edge_exists(m.n1, m.s1);
                bool e2 = tm.edge_exists(m.n2, m.s2);

                if (!e1 || !e2) {
                    it = LM.erase(it);
                    continue;
                }

                bool dir1 = (tm.succ[m.n1] == m.s1);
                bool dir2 = (tm.succ[m.n2] == m.s2);

                // obie krawędzie istnieją, ale w przeciwnym względnym kierunku
                // zostawiamy ruch w LM
                if (dir1 != dir2) {
                    ++it;
                    continue;
                }

                int a = dir1 ? m.n1 : m.s1;
                int b = dir1 ? m.n2 : m.s2;

                int sa = tm.succ[a];
                int sb = tm.succ[b];

                double real_delta =
                    (data.distances[a][sa] + data.distances[b][sb]) -
                    (data.distances[a][b] + data.distances[sa][sb]);

                if (real_delta > EPSILON) {
                    tm.apply_2opt(a, b);
                    it = LM.erase(it);
                    move_applied = true;
                    break;
                }
                else {
                    it = LM.erase(it);
                    continue;
                }
            }
            else if (m.type == ADD) {
                bool ok = false;
                double real_delta = -1e100;

                if (tm.tour.empty()) {
                    // dodanie pierwszego wierzchołka do pustej trasy
                    ok = (m.node >= 0 && tm.pos[m.node] == -1 &&
                        m.add_after == -1 && m.add_before == -1);
                    if (ok) {
                        real_delta = data.gains[m.node];
                    }
                }
                else {
                    ok =
                        (m.node >= 0 && tm.pos[m.node] == -1) &&
                        (m.add_after >= 0 && tm.pos[m.add_after] != -1) &&
                        (m.add_before >= 0 && tm.pos[m.add_before] != -1) &&
                        (tm.succ[m.add_after] == m.add_before);

                    if (ok) {
                        real_delta =
                            data.gains[m.node] -
                            (data.distances[m.add_after][m.node] +
                                data.distances[m.node][m.add_before] -
                                data.distances[m.add_after][m.add_before]);
                    }
                }

                if (!ok) {
                    it = LM.erase(it);
                    continue;
                }

                if (real_delta > EPSILON) {
                    if (tm.tour.empty()) {
                        tm.tour.push_back(m.node);
                        tm.update_mappings();
                    }
                    else {
                        tm.apply_add(m.node, m.add_after);
                    }

                    it = LM.erase(it);
                    move_applied = true;
                    break;
                }
                else {
                    it = LM.erase(it);
                    continue;
                }
            }
            else if (m.type == REMOVE) {
                bool ok = false;
                double real_delta = -1e100;
                int cur_m = (int)tm.tour.size();

                if (m.node >= 0 && tm.pos[m.node] != -1 && cur_m >= 1) {
                    if (cur_m == 1) {
                        ok = true;
                        real_delta = -data.gains[m.node];
                    }
                    else if (cur_m == 2) {
                        ok =
                            (m.rem_prev >= 0 && tm.pos[m.rem_prev] != -1) &&
                            (m.rem_next >= 0 && tm.pos[m.rem_next] != -1);

                        if (ok) {
                            int idx = tm.pos[m.node];
                            int other = tm.tour[(idx + 1) % cur_m];
                            real_delta =
                                -data.gains[m.node] +
                                2.0 * data.distances[m.node][other];
                        }
                    }
                    else {
                        ok =
                            (m.rem_prev >= 0 && tm.pos[m.rem_prev] != -1) &&
                            (m.rem_next >= 0 && tm.pos[m.rem_next] != -1) &&
                            (tm.succ[m.rem_prev] == m.node) &&
                            (tm.succ[m.node] == m.rem_next);

                        if (ok) {
                            int idx = tm.pos[m.node];
                            int prev = tm.tour[(idx - 1 + cur_m) % cur_m];
                            int next = tm.succ[m.node];

                            real_delta =
                                -data.gains[m.node] +
                                (data.distances[prev][m.node] +
                                    data.distances[m.node][next] -
                                    data.distances[prev][next]);
                        }
                    }
                }

                if (!ok) {
                    it = LM.erase(it);
                    continue;
                }

                if (real_delta > EPSILON) {
                    tm.apply_remove(m.node);
                    it = LM.erase(it);
                    move_applied = true;
                    break;
                }
                else {
                    it = LM.erase(it);
                    continue;
                }
            }
            else {
                it = LM.erase(it);
            }
        }

        if (move_applied) {
            continue;
        }

        build_LM_only(data, tm, LM);
        if (LM.empty()) {
            break;
        }
    }

    return tm.tour;
}



// ---------- główna funkcja ----------

Sequence LocalSearchCandidates(
    const Data& data,
    Sequence& start_node_list,
    const std::vector<std::vector<int>>& nearest_neighbors
) {
    const double EPSILON = 1e-9;

    TourManager tm(start_node_list, data.n);

    while (true) {
        bool found_improvement = false;
        double bestDelta = EPSILON;

        enum MoveType { NO_MOVE, BEST_EDGE_SWAP, BEST_ADD, BEST_REMOVE };
        MoveType bestMove = NO_MOVE;

        int best_n1 = -1, best_n2 = -1;
        int best_add_node = -1, best_add_after = -1;
        int best_remove_node = -1;

        int m = (int)tm.tour.size();

        // =========================================================
        // 1. EDGE_SWAP - tylko ruchy kandydackie
        // =========================================================
        if (m >= 4) {
            for (int n1 : tm.tour) {
                int s1 = tm.succ[n1];

                for (int n2 : nearest_neighbors[n1]) {
                    if (tm.pos[n2] == -1) continue;

                    int s2 = tm.succ[n2];

                    if (n1 == n2 || n1 == s2 || s1 == n2 || s1 == s2) continue;
                    if (s1 == n2 || s2 == n1) continue;

                    double delta =
                        (data.distances[n1][s1] + data.distances[n2][s2]) -
                        (data.distances[n1][n2] + data.distances[s1][s2]);

                    if (delta > bestDelta) {
                        bestDelta = delta;
                        bestMove = BEST_EDGE_SWAP;
                        best_n1 = n1;
                        best_n2 = n2;
                        found_improvement = true;
                    }
                }
            }
        }

        // =========================================================
        // 2. ADD
        // =========================================================
        if (m == 0) {
            for (int v = 0; v < data.n; ++v) {
                if (tm.pos[v] != -1) continue;

                double delta = data.gains[v];

                if (delta > bestDelta) {
                    bestDelta = delta;
                    bestMove = BEST_ADD;
                    best_add_node = v;
                    best_add_after = -1;
                    found_improvement = true;
                }
            }
        }
        else {
            for (int v = 0; v < data.n; ++v) {
                if (tm.pos[v] != -1) continue;

                for (int after : tm.tour) {
                    int before = tm.succ[after];

                    double delta =
                        data.gains[v] -
                        (data.distances[after][v] +
                            data.distances[v][before] -
                            data.distances[after][before]);

                    if (delta > bestDelta) {
                        bestDelta = delta;
                        bestMove = BEST_ADD;
                        best_add_node = v;
                        best_add_after = after;
                        found_improvement = true;
                    }
                }
            }
        }

        // =========================================================
        // 3. REMOVE
        // =========================================================
        if (m >= 1) {
            for (int v : tm.tour) {
                double delta = 0.0;

                if (m == 1) {
                    delta = -data.gains[v];
                }
                else if (m == 2) {
                    int idx = tm.pos[v];
                    int other = tm.tour[(idx + 1) % m];
                    delta = -data.gains[v] + 2.0 * data.distances[v][other];
                }
                else {
                    int idx = tm.pos[v];
                    int prev = tm.tour[(idx - 1 + m) % m];
                    int next = tm.succ[v];

                    delta =
                        -data.gains[v] +
                        (data.distances[prev][v] +
                            data.distances[v][next] -
                            data.distances[prev][next]);
                }

                if (delta > bestDelta) {
                    bestDelta = delta;
                    bestMove = BEST_REMOVE;
                    best_remove_node = v;
                    found_improvement = true;
                }
            }
        }

        // =========================================================
        // brak poprawy -> lokalne optimum
        // =========================================================
        if (!found_improvement) {
            break;
        }

        // =========================================================
        // wykonanie najlepszego ruchu
        // =========================================================
        if (bestMove == BEST_EDGE_SWAP) {
            tm.apply_2opt(best_n1, best_n2);
        }
        else if (bestMove == BEST_ADD) {
            if (m == 0) {
                tm.tour.push_back(best_add_node);
                tm.update_mappings();
            }
            else {
                tm.apply_add(best_add_node, best_add_after);
            }
        }
        else if (bestMove == BEST_REMOVE) {
            tm.apply_remove(best_remove_node);
        }
        else {
            break;
        }
    }

    return tm.tour;
}