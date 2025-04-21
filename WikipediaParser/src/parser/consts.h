#pragma once

namespace db
{
    inline constexpr auto const db_host = "tcp://127.0.0.1:3306";
    inline constexpr auto const db_user = "root";
    inline constexpr auto const db_pass = "1111";
    inline constexpr auto const db_name = "wikipediadb";
}

namespace js
{
    inline constexpr auto const page_limit = 2000; // Максимум 2000 статей за раз
    inline constexpr auto const page_by_id = "https://en.wikipedia.org/w/api.php?action=query&pageids={}&prop=revisions&rvprop=content&rvslots=main&format=json";
    inline constexpr auto const page_by_name = "https://en.wikipedia.org/w/api.php?action=query&titles={}&prop=revisions&rvprop=content&rvslots=main&format=json";
}

namespace utils
{
    inline constexpr auto const seconds_in_day = 24 * 60 * 60;
}