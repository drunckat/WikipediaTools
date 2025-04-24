cmake_minimum_required(VERSION 3.15)
project(WikipediaGraph)

set(CMAKE_CXX_STANDARD 23)

# Пути к vcpkg
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(glfw3_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/share/glfw3")
set(GLEW_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/share/glew")
set(imgui_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/share/imgui")
set(MYSQL_CONNECTOR_DIR "${CMAKE_SOURCE_DIR}/database/mysql-connector-c++-9.2.0-winx64")
set(VCPKG_DIR "${CMAKE_SOURCE_DIR}/vcpkg/vcpkg/installed/x64-windows")

# Пути к заголовочным файлам
include_directories(${CMAKE_SOURCE_DIR}/imnodes)
include_directories(${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/include)
include_directories(${CMAKE_SOURCE_DIR}/src/database)
include_directories(${CMAKE_SOURCE_DIR}/src/data_structures)
include_directories(${CMAKE_SOURCE_DIR}/src/rendering)
include_directories(${MYSQL_CONNECTOR_DIR}/include/)

# Подключаем библиотеки
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Добавляем исполняемый файл
add_executable(WikipediaGraph 
    src/main.cpp 
    src/data_structures/graph.cpp 
    src/database/DatabaseManager.cpp 
    src/rendering/render.cpp
    imnodes/imnodes.cpp   # Добавляем исходник imnodes
)

# Подключение директорий для линковки
target_link_directories(WikipediaGraph PRIVATE 
    ${MYSQL_CONNECTOR_DIR}/lib64/vs14
    ${VCPKG_DIR}/lib
)

# Линкуем библиотеки
target_link_libraries(WikipediaGraph PRIVATE 
    glfw 
    GLEW::GLEW 
    imgui::imgui
    "${MYSQL_CONNECTOR_DIR}/lib64/vs14/mysqlcppconn.lib"
)
