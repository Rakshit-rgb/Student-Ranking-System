# Merit Ledger - Student Ranking System (C++ OOP + Max Heap)

A DSA/OOP demo: a C++ server ranks students with a **hand-written Max Heap** and serves a responsive dashboard.

## Run it
Requires a C++17 compiler (g++ 8+, clang, or MSVC). No other installs: the two header libraries
(`cpp-httplib`, `nlohmann/json`) and Chart.js are already bundled, so it also works offline.

**Linux / macOS / MinGW**
```
g++ -std=c++17 -O2 -pthread -Ibackend/include -Ithird_party backend/src/main.cpp backend/src/StudentManager.cpp -o ranking_server
./ranking_server
```
(MinGW on Windows: add `-lws2_32`; run as `ranking_server.exe`.)

**CMake alternative:** `cmake -B build && cmake --build build`, then run `build/ranking_server` from this folder.

Open **http://localhost:8080**. Optional arguments: `./ranking_server [port] [data_file] [web_root]`.
Run it from the project folder so `frontend/` and `data/students.json` are found.

## Structure
| Path | Purpose |
|---|---|
| `backend/include/MaxHeap.h` | Template Max Heap: `push`, `pop`, `top`, `build` (Floyd, O(n)), `siftUp/siftDown` |
| `backend/include/Student.h` | `Student` class (encapsulation + validation), `StudentLess` comparator |
| `backend/include/StudentManager.h`, `src/StudentManager.cpp` | CRUD, ranking via heap, statistics, JSON persistence |
| `backend/src/main.cpp` | REST API + static file server, exception-to-HTTP-status mapping |
| `frontend/` | Dashboard (HTML/CSS/JS, Chart.js bundled in `vendor/`) |
| `data/students.json` | 22 sample students; auto-saved on every change |

## How ranking works
`leaderboard()` builds the heap from all records in O(n), then pops repeatedly (O(n log n) total), so students
come out highest marks first. Ties share a rank (1, 2, 2, 4) and are ordered by ID. Grades: A+ >= 90, A >= 80, B >= 70, C >= 60, D >= 50, else F.

## REST API
| Method | Endpoint | Description |
|---|---|---|
| GET | `/api/students` | Ranked list (rank, grade included) |
| POST | `/api/students` | Add `{id,name,department,marks}` (201, or 409 for duplicate ID) |
| PUT | `/api/students/{id}` | Edit (404 if missing) |
| DELETE | `/api/students/{id}` | Delete (404 if missing) |
| GET | `/api/stats` | Totals, average, highest, top 3, distribution, department and grade stats |

Invalid input returns 400 with `{"error": "..."}`. Search, filters and sorting run in the browser on the ranked list.

## Demo talking points
Max Heap vs priority_queue (manual sift operations), Floyd build-heap O(n), encapsulation in `Student`, separation of concerns (heap / manager / server), thread-safe manager (mutex), atomic file save.
