#include <iostream>
#include <vector>
#include <unordered_map>
#include <regex>
#include <curl/curl.h>
#include <json/json.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>
#include "parser/consts.h"

std::unordered_map<std::string, int> page_map;

std::string url_encode(const std::string &value)
{
    std::ostringstream encoded;
    for (unsigned char c : value)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded << c;
        }
        else
        {
            encoded << '%' << std::uppercase << std::hex << int(c);
        }
    }
    return encoded.str();
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t total_size = size * nmemb;
    output->append((char *)contents, total_size);
    return total_size;
}

std::string fetch_url(const std::string &url)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "cURL initialization error" << std::endl;
        return "";
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
    }

    curl_easy_cleanup(curl);
    return response;
}

std::vector<std::tuple<int, std::string, int, int>> fetch_top_articles()
{
    std::vector<std::tuple<int, std::string, int, int>> articles;
    std::time_t t = std::time(nullptr) - utils::seconds_in_day;
    std::tm *now = std::localtime(&t);

    char date_str[11];
    std::strftime(date_str, sizeof(date_str), "%Y/%m/%d", now);

    std::string url = "https://wikimedia.org/api/rest_v1/metrics/pageviews/top/en.wikipedia/all-access/" + std::string(date_str);
    std::string json_data = fetch_url(url);

    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string errs;
    std::istringstream stream(json_data);

    if (!Json::parseFromStream(builder, stream, &root, &errs))
    {
        std::cerr << "JSON Parse Error (top articles): " << errs << std::endl;
        return {};
    }

    for (const auto &article : root["items"][0]["articles"])
    {
        int id = article["rank"].asInt(); // Assuming 'rank' is unique (adjust if needed)
        std::string title = article["article"].asString();
        int views = article["views"].asInt();
        int volume = article["size"].asInt(); // Assuming 'size' contains article volume

        articles.emplace_back(id, title, views, volume);
        if (articles.size() >= 1000)
            break;
    }
    return articles;
}

void insert_page(int id, const std::string &title, int visitors, int volume)
{
    try
    {
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect(db::db_host, db::db_user, db::db_pass));
        conn->setSchema(db::db_name);

        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
            "INSERT INTO wikipediapages (id, name, visitors_last_5_days, volume) "
            "VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE visitors_last_5_days = ?, volume = ?"));

        stmt->setInt(1, id);
        stmt->setString(2, title);
        stmt->setInt(3, visitors);
        stmt->setInt(4, volume);
        stmt->setInt(5, visitors);
        stmt->setInt(6, volume);
        stmt->execute();
        page_map[title] = id;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "MySQL Error: " << e.what() << std::endl;
    }
}

void insert_reference(int page_id, int referenced_page_id)
{
    try
    {
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect(db::db_host, db::db_user, db::db_pass));
        conn->setSchema(db::db_name);
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
            "INSERT IGNORE  INTO pagereferences (page_id, referenced_page_id) VALUES (?, ?)"));
        stmt->setInt(1, page_id);
        stmt->setInt(2, referenced_page_id);
        stmt->execute();
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "MySQL Error: " << e.what() << std::endl;
    }
}

void parse_references(const std::string &text, int page_id)
{
    std::regex reference_regex(R"(\[\[(?:[^\]|]*\|)?([^\]]+)\]\])");
    std::smatch match;
    std::string content = text;
    while (std::regex_search(content, match, reference_regex))
    {
        std::string ref_name = match[1].str();
        std::replace(ref_name.begin(), ref_name.end(), ' ', '_');

        if (page_map.count(ref_name))
        {
            insert_reference(page_id, page_map[ref_name]);
        }
        content = match.suffix().str();
    }
}

void fetch_and_parse_article(const std::string &title)
{
    std::string url = "https://en.wikipedia.org/w/api.php?action=query&titles=" + url_encode(title) + "&prop=revisions&rvprop=content&rvslots=main&format=json";
    std::string json_data = fetch_url(url);

    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string errs;
    std::istringstream stream(json_data);
    if (!Json::parseFromStream(builder, stream, &root, &errs))
    {
        std::cerr << "JSON parsing error: " << errs << std::endl;
        return;
    }

    const auto &pages = root["query"]["pages"];
    for (const auto &entry : pages)
    {
        if (entry["revisions"].isArray() && !entry["revisions"].empty())
        {
            std::string content = entry["revisions"][0]["slots"]["main"]["*"].asString();
            parse_references(content, page_map[title]);
        }
    }
}

void process_articles()
{
    std::vector<std::tuple<int, std::string, int, int>> top_articles = fetch_top_articles();

    for (const auto &[id, title, visitors, volume] : top_articles)
    {
        insert_page(id, title, visitors, volume);
    }
    for (const auto &[id, title, visitors, volume] : top_articles)
    {
        fetch_and_parse_article(title);
    }
}

void delete_old_entries()
{
    try
    {
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect(db::db_host, db::db_user, db::db_pass));
        conn->setSchema(db::db_name);
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());

        stmt->execute("DELETE FROM wikipediapages WHERE created_at < NOW() - INTERVAL 1 DAY");
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "MySQL Error (delete_old_entries): " << e.what() << std::endl;
    }
}

void delete_old_references()
{
    try
    {
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect(db::db_host, db::db_user, db::db_pass));
        conn->setSchema(db::db_name);
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());

        stmt->execute("DELETE FROM pagereferences WHERE created_at < NOW() - INTERVAL 1 DAY");
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "MySQL Error (delete_old_references): " << e.what() << std::endl;
    }
}

int main()
{
    std::cout << "Starting Wikipedia data processing daemon" << std::endl;

    while (true)
    {
        std::cout << "Updating database..." << std::endl;
        delete_old_entries();
        delete_old_references();
        process_articles();
        std::cout << "Update complete. Sleeping for 24 hours..." << std::endl;

        std::this_thread::sleep_for(std::chrono::hours(24)); // Ожидание 24 часа
    }

    return 0;
}