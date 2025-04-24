// DatabaseManager.h
#pragma once
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>

class DatabaseManager {
public:
    DatabaseManager(const std::string& host, const std::string& user, const std::string& password, const std::string& schema);

    std::vector<std::tuple<int, std::string, int>> getNeighborsSortedByVisitors(int nodeId);
    std::optional<std::pair<int, std::string>> getPageById(int id);
    std::pair<int, std::string> getMostReferencedPage();
    std::vector<std::pair<int, std::string>> getReferencesSorted(int fromId, int limit);
    std::vector<std::pair<int, std::string>> getReferencesExcluding(int fromId, const std::string& excludedIdsCSV, int limit);

private:
    std::unique_ptr<sql::Connection> conn;
};
