#include "graph.h"
#include <memory>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <queue>
#include <unordered_set>
#include <sstream>

namespace
{
    std::string adjust_name(std::string const &name)
    {
        std::string new_name;
        std::transform(name.begin(), name.end(), std::back_inserter(new_name), [](auto ch)
                       { if (ch=='_') {return ' '; } return ch; });
        return new_name;
    }

    std::string join(const auto &set, const std::string &delimiter = ", ")
    {
        std::ostringstream oss;
        auto it = set.begin();
        if (it != set.end())
        {
            oss << *it++;
            while (it != set.end())
            {
                oss << delimiter << *it++;
            }
        }
        return oss.str();
    }
}
Graph::Graph(database::DatabaseManager &dbManager) : db(dbManager) {}

std::vector<NeighborInfo> Graph::getSortedNeighbors(int nodeId)
{
    std::vector<NeighborInfo> neighbors;
    for (const auto &[id, name, visitors] : db.getNeighborsSortedByVisitors(nodeId))
    {
        neighbors.push_back({id, name, visitors});
    }
    return neighbors;
}

void Graph::loadFromDatabase(std::optional<int> startPageId)
{
    nodes.clear();
    edges.clear();

    int center_id;
    std::string center_name;

    if (startPageId)
    {
        auto page = db.getPageById(*startPageId);
        if (!page)
            return;
        center_id = page->first;
        center_name = page->second;
    }
    else
    {
        auto [id, name] = db.getMostReferencedPage();
        center_id = id;
        center_name = name;
    }

    nodes[center_id] = Node{center_id, adjust_name(center_name), std::nullopt};

    std::unordered_set<int> passed{center_id};
    std::vector<int> firstLevel;

    auto neighbors = db.getReferencesSorted(center_id, 10);
    for (const auto &[id, name] : neighbors)
    {
        nodes[id] = Node{id, adjust_name(name), std::nullopt};
        edges.push_back({center_id, id});
        passed.insert(id);
        firstLevel.push_back(id);
    }

    for (std::size_t i = 0; i < firstLevel.size(); ++i)
    {
        auto joined = join(passed, ", ");
        auto secondNeighbors = db.getReferencesExcluding(firstLevel[i], joined, 3);
        for (const auto &[id, name] : secondNeighbors)
        {
            if (passed.insert(id).second)
            {
                nodes[id] = Node{id, adjust_name(name), std::nullopt};
                edges.push_back({firstLevel[i], id});
            }
        }
    }
}