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

void Graph::loadFromDatabase(std::optional<int> startPageId)
{
    sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
    std::unique_ptr<sql::Connection> conn(driver->connect("tcp://localhost:3306", "root", "1111"));
    conn->setSchema("wikipediadb");

    nodes.clear();
    edges.clear();

    int center_id{};
    if (startPageId)
    {
        center_id = *startPageId;
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
            "SELECT id, name FROM wikipediapages WHERE id = ?"));
        stmt->setInt(1, *startPageId);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if (res->next())
        {
            nodes[*startPageId] = Node{*startPageId, adjust_name(res->getString("name")), std::nullopt};
        }
    }
    else
    {
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
            "SELECT id, name FROM wikipediapages WHERE id = (SELECT page_id FROM (select page_id, count(*) cnt from pagereferences group by page_id ORDER BY cnt DESC LIMIT 1) tbl)"));
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        center_id = res->getInt("id");
        if (res->next())
        {
            nodes[res->getInt("id")] = Node{res->getInt("id"), adjust_name(res->getString("name")), std::nullopt};
        }
    }

    std::vector<int> firstLevel;
    std::unordered_set<int> passed{center_id};
    std::unordered_set<int> secondLevelAdded;

    {
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
            "SELECT p.referenced_page_id, w.name FROM pagereferences p "
            "JOIN wikipediapages w ON p.referenced_page_id = w.id "
            "WHERE p.page_id = ? ORDER BY w.visitors_last_5_days DESC"));
        stmt->setInt(1, *startPageId);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        std::size_t counter = 0;
        while (res->next())
        {
            int neighborId = res->getInt("referenced_page_id");
            if (counter++ < 10)
            {
                nodes[neighborId] = Node{neighborId, adjust_name(res->getString("name")), std::nullopt};
                edges.push_back({*startPageId, neighborId});
            }
            passed.emplace(neighborId);
            firstLevel.emplace_back(neighborId);
        }
    }

    for (std::size_t i = 0; i < firstLevel.size() && i < 10; ++i)
    {
        auto joined = join(passed, ", ");
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
            "SELECT p.referenced_page_id, w.name FROM pagereferences p "
            "INNER JOIN wikipediapages w ON p.referenced_page_id = w.id "
            "WHERE p.page_id = ? AND referenced_page_id NOT IN (?) "
            "ORDER BY w.visitors_last_5_days DESC LIMIT 3"));
        stmt->setInt(1, firstLevel[i]);
        stmt->setString(2, joined);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        while (res->next())
        {
            int secondId = res->getInt("referenced_page_id");
            std::string name = adjust_name(res->getString("name"));
            if (passed.emplace(secondId).second)
            {
                nodes[secondId] = Node{secondId, name, std::nullopt};
                edges.push_back({firstLevel[i], secondId});
            }
        }
    }
}