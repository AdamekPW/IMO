#include "zad5.h"

#include <unordered_set>
#include <set>
#include <algorithm>

int CountCommonVertices(const Sequence& a, const Sequence& b) {
    std::unordered_set<int> setA(a.begin(), a.end());
    int count = 0;
    for (int v : b) {
        if (setA.count(v)) ++count;
    }
    return count;
}

int CountCommonEdges(const Sequence& a, const Sequence& b) {
    if (a.size() < 2 || b.size() < 2) return 0;

    std::set<std::pair<int, int>> edgesA;
    for (int i = 0; i < (int)a.size(); ++i) {
        int u = a[i], v = a[(i + 1) % (int)a.size()];
        edgesA.insert({ std::min(u, v), std::max(u, v) });
    }

    int count = 0;
    for (int i = 0; i < (int)b.size(); ++i) {
        int u = b[i], v = b[(i + 1) % (int)b.size()];
        if (edgesA.count({ std::min(u, v), std::max(u, v) })) ++count;
    }
    return count;
}
