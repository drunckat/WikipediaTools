cmake_minimum_required(VERSION 3.10)
project(WikipediaParser)

set(CMAKE_CXX_STANDARD 23)

# Пути к библиотекам
set(VCPKG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/vcpkg/installed/x64-windows")
set(MYSQL_CONNECTOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/database/mysql-connector-c++-9.2.0-winx64")

# Подключение cURL
set(CURL_INCLUDE_DIR "${VCPKG_DIR}/include")
set(CURL_LIBRARY "${VCPKG_DIR}/lib/libcurl.lib")

# Подключение jsoncpp
set(JsonCpp_INCLUDE_DIR "${VCPKG_DIR}/include")
set(JsonCpp_LIBRARY "${VCPKG_DIR}/lib/jsoncpp.lib")

# Добавление исполняемого файла
add_executable(WikipediaParser src/main.cpp)

# Подключение include-директорий
target_include_directories(WikipediaParser PRIVATE 
    ${MYSQL_CONNECTOR_DIR}/include/
    ${CURL_INCLUDE_DIR}  # исправлен путь
    ${JsonCpp_INCLUDE_DIR}
)

# Подключение директорий для линковки
target_link_directories(WikipediaParser PRIVATE 
    ${MYSQL_CONNECTOR_DIR}/lib64/vs14
    ${VCPKG_DIR}/lib
)

# Линковка библиотек
target_link_libraries(WikipediaParser PRIVATE mysqlcppconn ${CURL_LIBRARY} ${JsonCpp_LIBRARY})
