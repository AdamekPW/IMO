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

static double current_add_delta(const Data& data, const TourManager& tm, int node, int after) {
    int before = tm.succ[after];
    return data.gains[node]
        - (data.distances[after][node] + data.distances[node][before] - data.distances[after][before]);
}

static double current_remove_delta(const Data& data, const TourManager& tm, int node) {
    int m = (int)tm.tour.size();
    int idx = tm.pos[node];
    int prev = tm.tour[(idx - 1 + m) % m];
    int next = tm.succ[node];

    return -data.gains[node]
        + (data.distances[prev][node] + data.distances[node][next] - data.distances[prev][next]);
}

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

    // 2. ADD
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

    // 3. REMOVE
    if (m > 3) {
        for (int v : tm.tour) {
            int idx = tm.pos[v];
            int prev = tm.tour[(idx - 1 + m) % m];
            int next = tm.succ[v];

            double d =
                -data.gains[v] +
                (data.distances[prev][v] +
                    data.distances[v][next] -
                    data.distances[prev][next]);

            if (d > EPSILON) {
                LMMove mv;
                mv.type = REMOVE;
                mv.node = v;
                mv.rem_prev = prev;
                mv.rem_next = next;
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

                // jeśli którejś krawędzi już nie ma, ruch usuwamy z LM
                if (!e1 || !e2) {
                    it = LM.erase(it);
                    continue;
                }

                bool dir1 = (tm.succ[m.n1] == m.s1);
                bool dir2 = (tm.succ[m.n2] == m.s2);

                // obie krawędzie istnieją, ale są w innym względnym kierunku
                // zostawiamy ruch w LM, może stanie się aplikowalny później
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
                bool ok =
                    (m.node >= 0 && tm.pos[m.node] == -1) &&
                    (m.add_after >= 0 && tm.pos[m.add_after] != -1) &&
                    (m.add_before >= 0 && tm.pos[m.add_before] != -1) &&
                    (tm.succ[m.add_after] == m.add_before);

                if (!ok) {
                    it = LM.erase(it);
                    continue;
                }

                double real_delta =
                    data.gains[m.node] -
                    (data.distances[m.add_after][m.node] +
                        data.distances[m.node][m.add_before] -
                        data.distances[m.add_after][m.add_before]);

                if (real_delta > EPSILON) {
                    tm.apply_add(m.node, m.add_after);
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
                bool ok =
                    (m.node >= 0 && tm.pos[m.node] != -1) &&
                    ((int)tm.tour.size() > 3) &&
                    (m.rem_prev >= 0 && tm.pos[m.rem_prev] != -1) &&
                    (m.rem_next >= 0 && tm.pos[m.rem_next] != -1) &&
                    (tm.succ[m.rem_prev] == m.node) &&
                    (tm.succ[m.node] == m.rem_next);

                if (!ok) {
                    it = LM.erase(it);
                    continue;
                }

                int idx = tm.pos[m.node];
                int prev = tm.tour[(idx - 1 + (int)tm.tour.size()) % (int)tm.tour.size()];
                int next = tm.succ[m.node];

                double real_delta =
                    -data.gains[m.node] +
                    (data.distances[prev][m.node] +
                        data.distances[m.node][next] -
                        data.distances[prev][next]);

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

void build_LM(const Data& data,
    const TourManager& tm,
    std::vector<LMMove>& LM,
    const std::vector<std::vector<int>>& nearest_neighbors) {
    LM.clear();
    const double EPSILON = 1e-9;
    int m = (int)tm.tour.size();

    // 1. EDGE_SWAP z krawędzi kandydackich
    for (int n1 : tm.tour) {
        int s1 = tm.succ[n1];

        for (int n2 : nearest_neighbors[n1]) {
            if (tm.pos[n2] == -1) continue; // n2 musi być w trasie

            int s2 = tm.succ[n2];

            // pomijamy przypadki degeneracyjne
            if (n1 == n2 || n1 == s2 || s1 == n2 || s1 == s2) continue;
            if (s1 == n2 || s2 == n1) continue;

            double d = current_2opt_delta(data, tm, n1, s1, n2, s2);
            if (d > EPSILON) {
                // wariant zgodny z aktualnym kierunkiem
                {
                    LMMove mv;
                    mv.type = EDGE_SWAP;
                    mv.n1 = n1; mv.s1 = s1;
                    mv.n2 = n2; mv.s2 = s2;
                    mv.delta = d;
                    LM.push_back(mv);
                }

                // wariant dla odwróconego względnego kierunku
                {
                    LMMove mv;
                    mv.type = EDGE_SWAP;
                    mv.n1 = s1; mv.s1 = n1;
                    mv.n2 = s2; mv.s2 = n2;
                    mv.delta = d;
                    LM.push_back(mv);
                }
            }
        }
    }

    // 2. ADD
    for (int v = 0; v < data.n; ++v) {
        if (tm.pos[v] != -1) continue; // tylko spoza trasy

        for (int after : tm.tour) {
            int before = tm.succ[after];
            double d = data.gains[v]
                - (data.distances[after][v] + data.distances[v][before] - data.distances[after][before]);

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

    // 3. REMOVE
    if (m > 3) {
        for (int v : tm.tour) {
            int idx = tm.pos[v];
            int prev = tm.tour[(idx - 1 + m) % m];
            int next = tm.succ[v];

            double d = -data.gains[v]
                + (data.distances[prev][v] + data.distances[v][next] - data.distances[prev][next]);

            if (d > EPSILON) {
                LMMove mv;
                mv.type = REMOVE;
                mv.node = v;
                mv.rem_prev = prev;
                mv.rem_next = next;
                mv.delta = d;
                LM.push_back(mv);
            }
        }
    }

    std::sort(LM.begin(), LM.end(), std::greater<LMMove>());
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

        // dla EDGE_SWAP
        int best_n1 = -1, best_n2 = -1;

        // dla ADD
        int best_add_node = -1, best_add_after = -1;

        // dla REMOVE
        int best_remove_node = -1;

        int m = (int)tm.tour.size();

        // =========================================================
        // 1. EDGE_SWAP - tylko ruchy kandydackie
        // =========================================================
        for (int n1 : tm.tour) {
            int s1 = tm.succ[n1];

            for (int n2 : nearest_neighbors[n1]) {
                if (tm.pos[n2] == -1) continue; // n2 musi być w trasie

                int s2 = tm.succ[n2];

                // przypadki degeneracyjne
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

        // =========================================================
        // 2. ADD
        // =========================================================
        for (int v = 0; v < data.n; ++v) {
            if (tm.pos[v] != -1) continue; // tylko spoza trasy

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

        // =========================================================
        // 3. REMOVE
        // =========================================================
        if (m > 3) {
            for (int v : tm.tour) {
                int idx = tm.pos[v];
                int prev = tm.tour[(idx - 1 + m) % m];
                int next = tm.succ[v];

                double delta =
                    -data.gains[v] +
                    (data.distances[prev][v] +
                        data.distances[v][next] -
                        data.distances[prev][next]);

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
            tm.apply_add(best_add_node, best_add_after);
        }
        else if (bestMove == BEST_REMOVE) {
            tm.apply_remove(best_remove_node);
        }
    }

    return tm.tour;
}