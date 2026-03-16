#include "Common.h"

#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <array>
#include <stdexcept>
#include <string>
#include <sstream>

#include "Structs.h"

Data FillData(std::vector<std::array<int, 3>>& input) {
    int n = input.size();

    Matrix matrix(n, std::vector<int>(n));
    std::vector<int> gains(n);

    for (int i = 0; i < n; i++) {
        gains[i] = input[i][2];
        for (int j = 0; j < n; j++) {
            if (i == j) matrix[i][j] = 0;
            int distance = std::round(CalcDistance(input[i][0], input[i][1], input[j][0], input[j][1]));
            matrix[i][j] = distance;
            matrix[j][i] = distance;
        }
    }

    Data data;
    data.distances = matrix;
    data.gains = gains;
    data.n = gains.size();
    return data;
}

Data LoadData(const std::string& filepath) {
    int a, b, c;
    char sep1, sep2;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Nie mo¿na otworzyæ pliku: " + filepath);
    }

    std::vector<std::array<int, 3>> lines;
    lines.reserve(1024); // opcjonalnie

    while (file >> a >> sep1 >> b >> sep2 >> c) {

        if (sep1 != ';' || sep2 != ';') {
            throw std::runtime_error("Niepoprawny separator w pliku: " + filepath);
        }

        lines.emplace_back(std::array<int, 3>{a, b, c});
    }

    if (!file.eof()) {
        throw std::runtime_error("B³¹d formatu danych w pliku: " + filepath);
    }

    return FillData(lines);
}


void SaveResultWithCoords(const std::string& jsonFilename,
    const std::string& csvPath,
    const Sequence& tour,
    const EvaluationResult& result) {

    // 1. Wczytywanie danych z CSV (x, y, profit)
    std::vector<Point> points;
    std::ifstream csvFile(csvPath);
    std::string line;

    if (!csvFile.is_open()) {
        std::cerr << "Blad: Nie mozna otworzyc pliku CSV: " << csvPath << std::endl;
        return;
    }

    while (std::getline(csvFile, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string val;
        Point p;

        // Zak³adamy format: x,y,gain (oddzielone przecinkiem lub œrednikiem)
        std::getline(ss, val, ';'); p.x = std::stod(val);
        std::getline(ss, val, ';'); p.y = std::stod(val);
        std::getline(ss, val, ';'); p.gain = std::stoi(val);
        points.push_back(p);
    }
    csvFile.close();

    // 2. Zapis do pliku JSON
    std::ofstream outFile(jsonFilename);
    if (!outFile.is_open()) return;

    outFile << "{\n";
    outFile << "  \"total_distance\": " << result.totalDistance << ",\n";
    outFile << "  \"total_gain\": " << result.totalGain << ",\n";

    // Zapis sekwencji odwiedzin
    outFile << "  \"tour\": [";
    for (size_t i = 0; i < tour.size(); ++i) {
        outFile << tour[i] << (i == tour.size() - 1 ? "" : ", ");
    }
    outFile << "],\n";

    // Zapis danych o wierzcho³kach (do rysowania)
    outFile << "  \"nodes\": [\n";
    for (size_t i = 0; i < points.size(); ++i) {
        outFile << "    {\"id\": " << i
            << ", \"x\": " << points[i].x
            << ", \"y\": " << points[i].y
            << ", \"gain\": " << points[i].gain << "}"
            << (i == points.size() - 1 ? "" : ",\n");
    }
    outFile << "\n  ]\n";
    outFile << "}\n";

    outFile.close();
    std::cout << "Wynik zapisany do: " << jsonFilename << std::endl;
}

void PrintData(const Data& data) {
    int n = data.distances.size();
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::cout << data.distances[i][j] << " ";
        }

        std::cout << std::endl;
    }

    std::cout << std::endl;
    for (int i = 0; i < n; i++) {
        std::cout << data.gains[i] << " ";
    }
}


int RandomNumber(int lowerLimit, int upperLimit)
{
    std::random_device rd; 
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(lowerLimit, upperLimit - 1);
    return distrib(gen);
}

float CalcDistance(int x1, int y1, int x2, int y2)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}


EvaluationResult CalculateTourMetrics(const std::vector<int>& tour, const Data& data) {
    EvaluationResult result;

    if (tour.empty()) return result;

    int n = tour.size();

    for (int i = 0; i < n; ++i) {
        int currentCity = tour[i];

        // 1. Dodajemy zysk z bie¿¹cego miasta
        result.totalGain += data.gains[currentCity];

        // 2. Obliczamy dystans do nastêpnego miasta
        int nextCity = tour[(i + 1) % n];
        result.totalDistance += data.distances[currentCity][nextCity];
    }

    return result;
}
