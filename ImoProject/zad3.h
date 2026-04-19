#pragma once

#include <vector>
#include <algorithm>
#include "Common.h"
#include "Structs.h"

// Struktura przechowująca ruch
enum LMMoveType { NONE, EDGE_SWAP, ADD, REMOVE };

struct LMMove {
    LMMoveType type = NONE;

    // EDGE_SWAP:
    // zapamiętane usuwane krawędzie (n1,s1) oraz (n2,s2)
    int n1 = -1, s1 = -1;
    int n2 = -1, s2 = -1;

    // ADD / REMOVE
    int node = -1;

    // Dla ADD: wstaw node pomiędzy add_after i add_before
    int add_after = -1;
    int add_before = -1;

    // Dla REMOVE: usuń node, który był pomiędzy rem_prev i rem_next
    int rem_prev = -1;
    int rem_next = -1;

    double delta = 0.0;

    bool operator>(const LMMove& other) const {
        return delta > other.delta;
    }
};

struct TourManager {
    std::vector<int> tour;
    std::vector<int> pos;
    std::vector<int> succ;
    int total_n;

    TourManager(const std::vector<int>& t, int n) : tour(t), total_n(n) {
        pos.assign(total_n, -1);
        succ.assign(total_n, -1);
        update_mappings();
    }

    void update_mappings() {
        std::fill(pos.begin(), pos.end(), -1);
        std::fill(succ.begin(), succ.end(), -1);

        int m = (int)tour.size();
        for (int i = 0; i < m; ++i) {
            int v = tour[i];
            pos[v] = i;
            succ[v] = tour[(i + 1) % m];
        }
    }

    bool edge_exists(int u, int v) const {
        if (u < 0 || v < 0) return false;
        if (u >= total_n || v >= total_n) return false;
        if (pos[u] == -1 || pos[v] == -1) return false;
        return (succ[u] == v || succ[v] == u);
    }

    void apply_2opt(int left_node, int right_node) {
        int m = (int)tour.size();
        int i = (pos[left_node] + 1) % m;
        int j = pos[right_node];

        int steps = (j - i + m) % m;
        for (int k = 0; k <= steps / 2; ++k) {
            std::swap(tour[(i + k) % m], tour[(j - k + m) % m]);
        }
        update_mappings();
    }

    void apply_add(int node, int after_node) {
        int index = pos[after_node];
        tour.insert(tour.begin() + index + 1, node);
        update_mappings();
    }

    void apply_remove(int node) {
        int index = pos[node];
        if (index != -1) {
            tour.erase(tour.begin() + index);
            update_mappings();
        }
    }
};

std::vector<std::vector<int>> build_candidate_edges(const Data& data, int k);
Sequence LocalSearchLMOnly(const Data& data, Sequence& start_node_list);
Sequence LocalSearchCandidates(
    const Data& data,
    Sequence& start_node_list,
    const std::vector<std::vector<int>>& nearest_neighbors
);