#include "render.h"
#include <imgui.h>
#include <imnodes.h>
#include <chrono>
#include <iostream>
#include <unordered_set>
#include <random>
#include <thread>

static int currentNodeId = 0;
static std::unordered_map<int, ImVec2> nodePositions;

void randomizeNodePositions(Graph &graph, int centerId)
{
    const float R1 = 220.0f;
    const float R2 = 440.0f;
    const ImVec2 center = ImVec2(750.0f, 450.0f);

    nodePositions.clear();
    nodePositions[centerId] = center;
    graph.nodes[centerId].position = center;

    std::vector<int> level1;
    std::unordered_map<int, std::vector<int>> level2;
    for (auto &[from, to] : graph.edges)
    {
        if (from == centerId)
            level1.push_back(to);
    }

    for (auto &[from, to] : graph.edges)
    {
        if (std::find(level1.begin(), level1.end(), from) != level1.end() && to != centerId)
        {
            level2[from].emplace_back(to);
        }
    }

    float angleStep1 = 2.0f * 3.14159265f / std::max(1, (int)level1.size());

    for (int i = 0; i < level1.size(); ++i)
    {
        float parentAngle = i * angleStep1;
        ImVec2 pos1 = ImVec2(center.x + R1 * cos(parentAngle), center.y + R1 * sin(parentAngle));
        nodePositions[level1[i]] = pos1;
        graph.nodes[level1[i]].position = ImVec2(center.x + R1 * cos(parentAngle), center.y + R1 * sin(parentAngle));

        auto &children = level2[level1[i]];
        int childCount = std::min((int)children.size(), 6);
        float spread = 30.0f * 3.14159265f / 180.0f;
        float startAngle = parentAngle - spread / 2.0f;
        float angleStep2 = spread / std::max(1, childCount - 1);

        for (int j = 0; j < childCount; ++j)
        {
            float angle = startAngle + j * angleStep2;
            nodePositions[children[j]] = ImVec2(center.x + R2 * cos(angle), center.y + R2 * sin(angle));
            graph.nodes[children[j]].position = ImVec2(center.x + R2 * cos(angle), center.y + R2 * sin(angle));
        }
    }
}

void loadSubGraph(Graph &graph, int nodeId)
{
    currentNodeId = nodeId;
    graph.nodes.clear();
    graph.edges.clear();
    std::cout << "render.cpp 68" << std::endl;
    graph.loadFromDatabase(nodeId);
    std::cout << "render.cpp 69" << std::endl;
    randomizeNodePositions(graph, nodeId);
}

void renderGraph(Graph &graph)
{std::cout << "render.cpp 75" << std::endl;
    if (graph.nodes.empty())
    {
        loadSubGraph(graph, currentNodeId);
    }
    std::cout << "render.cpp 80" << std::endl;
    ImGui::Begin("Graph Visualization");
    ImNodes::BeginNodeEditor();
    std::cout << "render.cpp 83" << std::endl;
    for (auto &[id, node] : graph.nodes)
    {
        
        ImNodes::BeginNode(id);
        node.position = node.position.value_or(nodePositions[id]);
        ImNodes::SetNodeGridSpacePos(node.id, *node.position);

        ImGui::Text("%s", node.label.c_str());

        ImNodes::BeginInputAttribute(id * 2);
        ImGui::Text("From");
        ImNodes::EndInputAttribute();

        ImNodes::BeginOutputAttribute(id * 2 + 1);
        ImGui::Text("To");
        ImNodes::EndOutputAttribute();

        ImNodes::EndNode();
    }
    std::cout << "render.cpp 101" << std::endl;

    std::cout << "render.cpp 104" << std::endl;
    int linkId = 0;
    for (auto &[from, to] : graph.edges)
    {
        
        ImNodes::Link(linkId++, from * 2 + 1, to * 2);
    }
    std::cout << "render.cpp 110" << std::endl;
    ImNodes::EndNodeEditor(); std::cout << "render.cpp 111" << std::endl;
    ImGui::End(); std::cout << "render.cpp 112" << std::endl;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        int hoveredNodeId;
        if (ImNodes::IsNodeHovered(&hoveredNodeId))
        {
            ImGui::OpenPopup("NeighborPopup");
            currentNodeId = hoveredNodeId;
        }
    }
    std::cout << "render.cpp 122" << std::endl;
    static std::vector<NeighborInfo> neighbors;
    std::cout << "render.cpp 124" << std::endl;
    if (ImGui::BeginPopup("NeighborPopup"))
    {
        if (ImGui::BeginListBox("##neighbors", ImVec2(300, 200)))
        {
            neighbors = graph.getSortedNeighbors(currentNodeId); // db должен быть доступен
            for (auto &n : neighbors)
            {
                ImGui::Text("%s (%d)", n.name.c_str(), n.visitors);
            }
            ImGui::EndListBox();
        }
        ImGui::EndPopup();
    }
    std::cout << "render.cpp 138" << std::endl;
    for (auto &[id, node] : graph.nodes)
    {
        if (ImNodes::IsNodeSelected(id))
        {
            node.position = ImNodes::GetNodeGridSpacePos(id);
            break;
        }
    }
    std::cout << "render.cpp 147" << std::endl;
    if (ImGui::IsMouseDoubleClicked(0))
    {
        for (auto &[id, node] : graph.nodes)
        {
            if (ImNodes::IsNodeSelected(id))
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(100ms);
                loadSubGraph(graph, id);
                break;
            }
        }
    }
    std::cout << "render.cpp 161" << std::endl;
}
