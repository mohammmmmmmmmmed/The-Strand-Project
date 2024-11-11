#include "crow.h"
#include <fstream>
#include <sstream>

int main() {
    crow::SimpleApp app;

    // Route to serve the main HTML page
    CROW_ROUTE(app, "/")([]() {
        auto page = crow::mustache::load("index.html");
        return page.render();
    });
    CROW_ROUTE(app, "/predictiveModelling")([]() {
        auto page = crow::mustache::load("predictiveModelling.html");
        return page.render();
    });

    // Route to serve static files (CSS, JS)
    CROW_ROUTE(app, "/<string>")([](std::string path) {
        std::ifstream file("templates/" + path);
        if (file) {
            std::ostringstream contents;
            contents << file.rdbuf();
            file.close();
            return crow::response(contents.str());
        } else {
            return crow::response(404);
        }
    });

    app.port(8080).multithreaded().run();
}
