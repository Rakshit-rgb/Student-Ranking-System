#pragma once
#include <mutex>
#include <string>
#include <vector>
#include "MaxHeap.h"
#include "Student.h"

struct RankedStudent {
    Student student;
    int rank;
};

// Owns all records, ranks them with the Max Heap, and persists them to a JSON file.
class StudentManager {
public:
    explicit StudentManager(std::string dataFile) : file_(std::move(dataFile)) {}

    void load();                                             // missing file => empty list
    void add(const Student& s);                              // throws runtime_error on duplicate ID
    void update(const std::string& id, const Student& s);    // throws out_of_range if missing
    void remove(const std::string& id);                      // throws out_of_range if missing

    std::vector<RankedStudent> leaderboard() const;
    nlohmann::json leaderboardJson() const;
    nlohmann::json statsJson() const;

private:
    std::vector<Student> students_;
    std::string file_;
    mutable std::mutex mutex_;

    std::vector<RankedStudent> rankUnlocked() const;
    void saveUnlocked() const;
    std::size_t indexOf(const std::string& id) const;
};
