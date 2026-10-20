#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>
#include "json.hpp"

// Encapsulated student record; every setter validates its input.
class Student {
public:
    Student() = default;
    Student(std::string id, std::string name, std::string department, double marks) {
        setId(std::move(id));
        setName(std::move(name));
        setDepartment(std::move(department));
        setMarks(marks);
    }

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& department() const { return department_; }
    double marks() const { return marks_; }

    void setId(std::string v) {
        v = trim(v);
        if (v.empty() || v.size() > 20) throw std::invalid_argument("Student ID must be 1-20 characters.");
        for (char& c : v) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_')
                throw std::invalid_argument("Student ID may only contain letters, digits, '-' and '_'.");
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        id_ = v;
    }
    void setName(std::string v) {
        v = trim(v);
        if (v.empty() || v.size() > 60) throw std::invalid_argument("Name must be 1-60 characters.");
        name_ = v;
    }
    void setDepartment(std::string v) {
        v = trim(v);
        if (v.empty() || v.size() > 40) throw std::invalid_argument("Department must be 1-40 characters.");
        department_ = v;
    }
    void setMarks(double m) {
        if (!std::isfinite(m) || m < 0.0 || m > 100.0)
            throw std::invalid_argument("Marks must be a number between 0 and 100.");
        marks_ = std::round(m * 100.0) / 100.0;
    }

    std::string grade() const {
        if (marks_ >= 90) return "A+";
        if (marks_ >= 80) return "A";
        if (marks_ >= 70) return "B";
        if (marks_ >= 60) return "C";
        if (marks_ >= 50) return "D";
        return "F";
    }

    nlohmann::json toJson() const {
        return {{"id", id_}, {"name", name_}, {"department", department_}, {"marks", marks_}, {"grade", grade()}};
    }
    static Student fromJson(const nlohmann::json& j) {
        return Student(j.at("id").get<std::string>(), j.at("name").get<std::string>(),
                       j.at("department").get<std::string>(), j.at("marks").get<double>());
    }

private:
    std::string id_, name_, department_;
    double marks_ = 0.0;

    static std::string trim(const std::string& s) {
        auto b = std::find_if_not(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); });
        auto e = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) { return std::isspace(c); }).base();
        return b < e ? std::string(b, e) : std::string();
    }
};

// Heap ordering: higher marks win; on a tie the smaller ID wins (deterministic).
struct StudentLess {
    bool operator()(const Student& a, const Student& b) const {
        if (a.marks() != b.marks()) return a.marks() < b.marks();
        return a.id() > b.id();
    }
};
