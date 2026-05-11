#pragma once

#include "Structs.h"

// Counts vertices present in both solutions (set intersection size).
int CountCommonVertices(const Sequence& a, const Sequence& b);

// Counts undirected edges present in both solutions.
// Edge (u,v) == (v,u), tour is cyclic (last vertex connects back to first).
int CountCommonEdges(const Sequence& a, const Sequence& b);
