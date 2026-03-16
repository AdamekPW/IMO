#pragma once

#include <string>
#include "Structs.h"

Data LoadData(const std::string& filepath);

void SaveResultWithCoords(
    const std::string& jsonFilename,
    const std::string& csvPath,
    const Sequence& tour,
    const EvaluationResult& result);

void PrintData(const Data& data);

int RandomNumber(int LowerLimit, int UpperLimit);

float CalcDistance(int x1, int y1, int x2, int y2);

EvaluationResult CalculateTourMetrics(const std::vector<int>& tour, const Data& data);
