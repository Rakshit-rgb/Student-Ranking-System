// Student Ranking System - C++ REST API + static file server.
// Usage: ./ranking_server [port] [data_file] [web_root]
#include <iostream>
#include "httplib.h"
#include "StudentManager.h"

using nlohmann::json;

int main(int argc, char** argv) {
    const int port = argc > 1 ? std::atoi(argv[1]) : 8080;
    const std::string dataFile = argc > 2 ? argv[2] : "data/students.json";
    const std::string webRoot = argc > 3 ? argv[3] : "frontend";

    StudentManager manager(dataFile);
    try {
        manager.load();
    } catch (const std::exception& e) {
        std::cerr << "[warn] " << e.what() << "\nStarting with an empty list.\n";
    }

    httplib::Server svr;
    if (!svr.set_mount_point("/", webRoot)) {
        std::cerr << "[error] Web root '" << webRoot << "' not found. Run from the project folder.\n";
        return 1;
    }

    auto reply = [](httplib::Response& res, int status, const json& body) {
        res.status = status;
        res.set_content(body.dump(), "application/json");
    };
    // Runs a handler and maps C++ exceptions to HTTP status codes.
    auto guarded = [&](httplib::Response& res, const std::function<void()>& fn) {
        try { fn(); }
        catch (const std::invalid_argument& e) { reply(res, 400, {{"error", e.what()}}); }
        catch (const std::out_of_range& e)     { reply(res, 404, {{"error", e.what()}}); }
        catch (const std::runtime_error& e)    { reply(res, 409, {{"error", e.what()}}); }
        catch (const json::exception&)         { reply(res, 400, {{"error", "Invalid or incomplete student data."}}); }
        catch (const std::exception& e)        { reply(res, 500, {{"error", e.what()}}); }
    };

    svr.Get("/api/students", [&](const httplib::Request&, httplib::Response& res) {
        guarded(res, [&] { reply(res, 200, manager.leaderboardJson()); });
    });
    svr.Get("/api/stats", [&](const httplib::Request&, httplib::Response& res) {
        guarded(res, [&] { reply(res, 200, manager.statsJson()); });
    });
    svr.Post("/api/students", [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&] {
            Student s = Student::fromJson(json::parse(req.body));
            manager.add(s);
            reply(res, 201, s.toJson());
        });
    });
    svr.Put(R"(/api/students/([^/]+))", [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&] {
            json body = json::parse(req.body);
            body["id"] = req.matches[1].str();  // the ID in the URL is authoritative
            Student s = Student::fromJson(body);
            manager.update(s.id(), s);
            reply(res, 200, s.toJson());
        });
    });
    svr.Delete(R"(/api/students/([^/]+))", [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&] {
            manager.remove(req.matches[1].str());
            reply(res, 200, {{"deleted", req.matches[1].str()}});
        });
    });
    svr.set_exception_handler([&](const httplib::Request&, httplib::Response& res, std::exception_ptr) {
        reply(res, 500, {{"error", "Unexpected server error."}});
    });

    std::cout << "Student Ranking System running at http://localhost:" << port << "\n(Ctrl+C to stop)\n";
    if (!svr.listen("0.0.0.0", port)) {
        std::cerr << "[error] Could not listen on port " << port << " (already in use?).\n";
        return 1;
    }
}
