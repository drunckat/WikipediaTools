#pragma once

#include "DatabaseManager.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <imgui.h>

struct NeighborInfo
{
    int id;
    std::string name;
    int visitors;
};

struct Node
{
    int id;
    std::string label;
    std::optional<ImVec2> position;
};

struct Edge
{
    int from;
    int to;
};

class Graph
{
public:
    Graph(database::DatabaseManager &dbManager);

    Graph(Graph const &) = delete;
    std::unordered_map<int, Node> nodes;
    std::vector<Edge> edges;
    std::vector<NeighborInfo> getSortedNeighbors(int nodeId);
    void loadFromDatabase(std::optional<int> startPageId = std::nullopt);

private:
    database::DatabaseManager &db;
};