#include "StudentManager.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>

using nlohmann::json;

std::size_t StudentManager::indexOf(const std::string& id) const {
    for (std::size_t i = 0; i < students_.size(); ++i)
        if (students_[i].id() == id) return i;
    throw std::out_of_range("No student with ID '" + id + "'.");
}

void StudentManager::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    students_.clear();
    std::ifstream in(file_);
    if (!in) return;  // first run: nothing saved yet
    json j;
    try {
        in >> j;
        for (const auto& item : j) students_.push_back(Student::fromJson(item));
    } catch (const std::exception& e) {
        students_.clear();
        throw std::runtime_error(std::string("Could not read data file '") + file_ + "': " + e.what());
    }
}

void StudentManager::saveUnlocked() const {
    json arr = json::array();
    for (const auto& s : students_) arr.push_back(s.toJson());
    std::filesystem::path target(file_);
    if (target.has_parent_path()) std::filesystem::create_directories(target.parent_path());
    std::string tmp = file_ + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) throw std::ios_base::failure("Cannot write to '" + tmp + "'.");
        out << arr.dump(2);
    }
    std::filesystem::rename(tmp, target);  // atomic swap so a crash never corrupts the file
}

void StudentManager::add(const Student& s) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& e : students_)
        if (e.id() == s.id()) throw std::runtime_error("A student with ID '" + s.id() + "' already exists.");
    students_.push_back(s);
    saveUnlocked();
}

void StudentManager::update(const std::string& id, const Student& s) {
    std::lock_guard<std::mutex> lock(mutex_);
    students_[indexOf(id)] = s;
    saveUnlocked();
}

void StudentManager::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    students_.erase(students_.begin() + static_cast<std::ptrdiff_t>(indexOf(id)));
    saveUnlocked();
}

// Build the heap in O(n), then pop repeatedly: students arrive highest-marks-first.
// Equal marks share a rank (standard competition ranking: 1, 2, 2, 4 ...).
std::vector<RankedStudent> StudentManager::rankUnlocked() const {
    MaxHeap<Student, StudentLess> heap;
    heap.build(students_);
    std::vector<RankedStudent> out;
    out.reserve(students_.size());
    int rank = 0;
    while (!heap.empty()) {
        Student s = heap.pop();
        if (out.empty() || s.marks() != out.back().student.marks()) rank = static_cast<int>(out.size()) + 1;
        out.push_back({std::move(s), rank});
    }
    return out;
}

std::vector<RankedStudent> StudentManager::leaderboard() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rankUnlocked();
}

json StudentManager::leaderboardJson() const {
    json arr = json::array();
    for (const auto& r : leaderboard()) {
        json j = r.student.toJson();
        j["rank"] = r.rank;
        arr.push_back(std::move(j));
    }
    return arr;
}

json StudentManager::statsJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto ranked = rankUnlocked();
    json stats;
    const std::size_t n = ranked.size();
    stats["total"] = n;

    double sum = 0;
    std::map<std::string, std::pair<int, double>> dept;  // name -> (count, sum)
    std::map<std::string, int> grades = {{"A+", 0}, {"A", 0}, {"B", 0}, {"C", 0}, {"D", 0}, {"F", 0}};
    int buckets[6] = {0, 0, 0, 0, 0, 0};
    for (const auto& r : ranked) {
        double m = r.student.marks();
        sum += m;
        dept[r.student.department()].first++;
        dept[r.student.department()].second += m;
        grades[r.student.grade()]++;
        buckets[m < 50 ? 0 : m >= 90 ? 5 : static_cast<int>((m - 50) / 10) + 1]++;
    }
    stats["average"] = n ? std::round(sum / n * 100) / 100 : 0.0;
    stats["highest"] = n ? ranked.front().student.marks() : 0.0;
    stats["lowest"] = n ? ranked.back().student.marks() : 0.0;

    json top = json::array();
    for (std::size_t i = 0; i < std::min<std::size_t>(3, n); ++i) {
        json j = ranked[i].student.toJson();
        j["rank"] = ranked[i].rank;
        top.push_back(std::move(j));
    }
    stats["top3"] = top;

    const char* labels[6] = {"Below 50", "50-59", "60-69", "70-79", "80-89", "90-100"};
    json dist = json::array();
    for (int i = 0; i < 6; ++i) dist.push_back({{"label", labels[i]}, {"count", buckets[i]}});
    stats["distribution"] = dist;

    json depts = json::array();
    for (const auto& [name, v] : dept)
        depts.push_back({{"name", name}, {"count", v.first}, {"average", std::round(v.second / v.first * 100) / 100}});
    stats["departments"] = depts;
    stats["grades"] = grades;
    return stats;
}
