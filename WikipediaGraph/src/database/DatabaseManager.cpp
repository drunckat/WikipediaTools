// DatabaseManager.cpp
#include "DatabaseManager.h"

DatabaseManager::DatabaseManager(const std::string& host, const std::string& user, const std::string& password, const std::string& schema) {
    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
    conn.reset(driver->connect(host, user, password));
    conn->setSchema(schema);
}

std::vector<std::tuple<int, std::string, int>> DatabaseManager::getNeighborsSortedByVisitors(int nodeId) {
    std::vector<std::tuple<int, std::string, int>> neighbors;
    auto stmt = conn->prepareStatement(
        "SELECT wp.id, wp.name, wp.visitors_last_5_days "
        "FROM pagereferences pr "
        "JOIN wikipediapages wp ON pr.referenced_page_id = wp.id "
        "WHERE pr.page_id = ? ORDER BY wp.visitors_last_5_days DESC");
    stmt->setInt(1, nodeId);
    auto res = stmt->executeQuery();
    while (res->next()) {
        neighbors.emplace_back(res->getInt("id"), res->getString("name"), res->getInt("visitors_last_5_days"));
    }
    return neighbors;
}

std::optional<std::pair<int, std::string>> DatabaseManager::getPageById(int id) {
    auto stmt = conn->prepareStatement("SELECT id, name FROM wikipediapages WHERE id = ?");
    stmt->setInt(1, id);
    auto res = stmt->executeQuery();
    if (res->next()) {
        return std::make_pair(res->getInt("id"), res->getString("name"));
    }
    return std::nullopt;
}

std::pair<int, std::string> DatabaseManager::getMostReferencedPage() {
    auto stmt = conn->prepareStatement(
        "SELECT id, name FROM wikipediapages WHERE id = ("
        "SELECT page_id FROM (SELECT page_id, COUNT(*) cnt FROM pagereferences "
        "GROUP BY page_id ORDER BY cnt DESC LIMIT 1) tmp)");
    auto res = stmt->executeQuery();
    res->next();
    return std::make_pair(res->getInt("id"), res->getString("name"));
}

std::vector<std::pair<int, std::string>> DatabaseManager::getReferencesSorted(int fromId, int limit) {
    std::vector<std::pair<int, std::string>> refs;
    auto stmt = conn->prepareStatement(
        "SELECT p.referenced_page_id, w.name FROM pagereferences p "
        "JOIN wikipediapages w ON p.referenced_page_id = w.id "
        "WHERE p.page_id = ? ORDER BY w.visitors_last_5_days DESC LIMIT ?");
    stmt->setInt(1, fromId);
    stmt->setInt(2, limit);
    auto res = stmt->executeQuery();
    while (res->next()) {
        refs.emplace_back(res->getInt(1), res->getString(2));
    }
    return refs;
}

std::vector<std::pair<int, std::string>> DatabaseManager::getReferencesExcluding(int fromId, const std::string& excludedIdsCSV, int limit) {
    std::vector<std::pair<int, std::string>> refs;
    std::string query =
        "SELECT p.referenced_page_id, w.name FROM pagereferences p "
        "JOIN wikipediapages w ON p.referenced_page_id = w.id "
        "WHERE p.page_id = ? AND p.referenced_page_id NOT IN (" + excludedIdsCSV + ") "
        "ORDER BY w.visitors_last_5_days DESC LIMIT ?";
    auto stmt = conn->prepareStatement(query);
    stmt->setInt(1, fromId);
    stmt->setInt(2, limit);
    auto res = stmt->executeQuery();
    while (res->next()) {
        refs.emplace_back(res->getInt(1), res->getString(2));
    }
    return refs;
}
