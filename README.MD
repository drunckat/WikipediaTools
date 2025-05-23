# Wikipedia Tools

Проект состоит из двух независимых C++ приложений:

- **WikipediaParser** — парсинг популярных статей из Википедии и сохранение данных в базу данных.
- **WikipediaGraph** — визуализация графа статей на основе сохранённых данных из базы.

---

## WikipediaGraph

Приложение для извлечения метаданных из английской Википедии и сохранения их в базу данных MySQL.

### Что делает

- Получает список популярных статей из английской Википедии.
- Фильтрует статьи:
  - Извлекает 2000 самых популярных статей за вчера
- Извлекает:
  - ID и названия статей
  - Ссылки на другие статьи Википедии
- Сохраняет данные в базу данных MySQL:
  - Таблица `wikipediapages`: информация о статье
```
+----------------------+--------------+------+-----+-------------------+-------------------+
| Field                | Type         | Null | Key | Default           | Extra             |
+----------------------+--------------+------+-----+-------------------+-------------------+
| id                   | int          | NO   | PRI | NULL              |                   |
| name                 | varchar(255) | NO   |     | NULL              |                   |
| visitors_last_5_days | int          | NO   |     | NULL              |                   |
| volume               | int          | NO   |     | NULL              |                   |
| created_at           | timestamp    | YES  |     | CURRENT_TIMESTAMP | DEFAULT_GENERATED |
+----------------------+--------------+------+-----+-------------------+-------------------+
```
  - Таблица `pagereferences`: связи между статьями
```
+--------------------+------+------+-----+---------+----------------+
| Field              | Type | Null | Key | Default | Extra          |
+--------------------+------+------+-----+---------+----------------+
| id                 | int  | NO   | PRI | NULL    | auto_increment |
| page_id            | int  | NO   | MUL | NULL    |                |
| referenced_page_id | int  | NO   | MUL | NULL    |                |
+--------------------+------+------+-----+---------+----------------+
```
### Зависимости

- C++23
- [`libcurl`](https://curl.se/libcurl/) — HTTP-запросы к Википедии
- [`nlohmann/json`](https://github.com/nlohmann/json) — парсинг JSON
- [`MySQL Connector/C++`](https://dev.mysql.com/doc/connector-cpp/en/) — взаимодействие с базой данных

### Сборка
```
cd WikipediaParser
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## WikipediaGraph

### Что делает

- Приложение для интерактивной визуализации графа статей Википедии на основе данных из базы данных.
- Возможности
  - Отображает центральную статью и связанные с ней статьи первого и второго уровня.
  - Центральный узел — по умолчанию Main_Page (id = 1), либо выбранный пользователем.
  - Первый уровень — статьи, на которые ссылается центральная.
  - Второй уровень — по одной ссылке от каждой статьи первого уровня.
  - Переключение центральной статьи по клику мышью.
- Принцип визуализации
  - Центральная статья размещается в центре окна.
  - Статьи первого уровня — по окружности радиуса A вокруг центра.
  - Статьи второго уровня — по окружности радиуса 2A вокруг соответствующего первого уровня.
  - Рисуются только связи:
    - от центральной статьи к статьям первого уровня,
    - от каждой статьи первого уровня к одной из статей второго уровня.

### Зависимости

- C++23  
- [ImGui](https://github.com/ocornut/imgui) — графический интерфейс  
- [ImNodes](https://github.com/Nelarius/imnodes) — визуализация графов в виде узлов  
- [MySQL Connector/C++](https://dev.mysql.com/doc/connector-cpp/en/) — взаимодействие с базой данных  
- [OpenGL](https://www.opengl.org/) — отрисовка графики  
- [GLFW](https://www.glfw.org/) — управление окном и вводом  
- [GLEW](http://glew.sourceforge.net/) — доступ к функциям OpenGL

### Сборка
```
cd WikipediaGraph
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```
