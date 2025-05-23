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
    graph.loadFromDatabase(nodeId);

    randomizeNodePositions(graph, nodeId);
}

void renderGraph(Graph &graph)
{
    if (graph.nodes.empty())
    {
        loadSubGraph(graph, currentNodeId);
    }
    ImGui::Begin("Graph Visualization");
    ImNodes::BeginNodeEditor();
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

    int linkId = 0;
    for (auto &[from, to] : graph.edges)
    {

        ImNodes::Link(linkId++, from * 2 + 1, to * 2);
    }
    ImNodes::EndNodeEditor();
    ImGui::End();
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        int hoveredNodeId;
        if (ImNodes::IsNodeHovered(&hoveredNodeId))
        {
            ImGui::OpenPopup("NeighborPopup");
            currentNodeId = hoveredNodeId;
        }
    }

    static std::vector<NeighborInfo> neighbors;
    if (ImGui::BeginPopup("NeighborPopup"))
    {
        if (ImGui::BeginListBox("##neighbors", ImVec2(300, 200)))
        {
            neighbors = graph.getSortedNeighbors(currentNodeId);
            for (auto &n : neighbors)
            {
                std::string clear_name;
                std::transform(n.name.begin(), n.name.end(), std::back_inserter(clear_name), [](auto ch)
                               {
                    if (ch == '_')
                    {
                        return ' ';
                    }
                    return ch; });
                if (ImGui::Selectable((clear_name + " (" + std::to_string(n.visitors) + ")").c_str()))
                {
                    loadSubGraph(graph, n.id);
                    ImGui::CloseCurrentPopup();
                    break;
                }
            }
            ImGui::EndListBox();
        }

        // Добавляем кнопку "Открыть в браузере"
        if (ImGui::Button("Open in Browser"))
        {
            const auto it = graph.nodes.find(currentNodeId);
            if (it != graph.nodes.end())
            {
                std::string url = "https://en.wikipedia.org/wiki/" + it->second.label;

                std::transform(url.begin(), url.end(), url.begin(), [](auto ch)
                               { if (ch==' ') {return '_'; } return ch; });
#if defined(_WIN32)
                std::string command = "start " + url;
#elif defined(__APPLE__)
                std::string command = "open " + url;
#else // Linux
                std::string command = "xdg-open " + url;
#endif
                system(command.c_str());
            }
        }
        ImGui::EndPopup();
    }

    for (auto &[id, node] : graph.nodes)
    {
        if (ImNodes::IsNodeSelected(id))
        {
            node.position = ImNodes::GetNodeGridSpacePos(id);
            break;
        }
    }

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
}
