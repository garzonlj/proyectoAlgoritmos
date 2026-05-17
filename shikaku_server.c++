#include "crow.h"
#include "solver.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Actualizado a la ruta real de salida de Angular
static std::string const FRONTEND_PATH = "frontend/dist/shikaku-frontend/browser";

static crow::response serve_static(std::string const& rel_path) {
    std::string path = rel_path.empty() ? "index.html" : rel_path;
    std::string full_path = FRONTEND_PATH + "/" + path;
    
    // Si no existe el archivo, probar con index.html (para SPAs)
    std::ifstream f(full_path, std::ios::binary);
    if (!f.is_open()) {
        full_path = FRONTEND_PATH + "/index.html";
        f.open(full_path, std::ios::binary);
        if (!f.is_open()) return crow::response(404);
    }

    std::stringstream buf;
    buf << f.rdbuf();
    std::string body = buf.str();

    std::string ext = full_path.substr(full_path.rfind('.') + 1);
    std::string mime = "text/plain";
    if (ext == "html") mime = "text/html";
    else if (ext == "js")  mime = "application/javascript";
    else if (ext == "css") mime = "text/css";
    else if (ext == "ico") mime = "image/x-icon";
    else if (ext == "svg") mime = "image/svg+xml";
    else if (ext == "png") mime = "image/png";
    else if (ext == "json") mime = "application/json";

    crow::response res(200, body);
    res.add_header("Content-Type", mime);
    return res;
}

int main() {
    crow::SimpleApp app;

    // API: Resolver puzzle
    CROW_ROUTE(app, "/api/solve").methods("POST"_method)
    ([](crow::request const& req) {
        crow::json::rvalue body;
        try {
            body = crow::json::load(req.body);
        } catch (...) {
            return crow::response(400, "{\"error\":\"JSON invalido\"}");
        }
        
        if (!body || !body.has("board"))
            return crow::response(400, "{\"error\":\"Falta el campo 'board'\"}");

        auto const& board_json = body["board"];
        if (board_json.t() != crow::json::type::List)
            return crow::response(400, "{\"error\":\"'board' debe ser una matriz\"}");

        int filas = static_cast<int>(board_json.size());
        Tablero tablero;
        int cols = 0;

        for (int r = 0; r < filas; ++r) {
            auto const& row_json = board_json[r];
            if (row_json.t() != crow::json::type::List)
                return crow::response(400, "{\"error\":\"Cada fila debe ser una lista\"}");
            
            int nc = static_cast<int>(row_json.size());
            if (r == 0) cols = nc;
            else if (nc != cols)
                return crow::response(400, "{\"error\":\"Filas de tamano inconsistente\"}");

            std::vector<uint8_t> fila;
            for (int c = 0; c < nc; ++c) {
                fila.push_back(static_cast<uint8_t>(row_json[c].i()));
            }
            tablero.push_back(std::move(fila));
        }

        if (filas == 0 || cols == 0)
            return crow::response(400, "{\"error\":\"Tablero vacio\"}");

        Resultado res;
        if (!resolver(tablero, res)) {
            return crow::response(200, "{\"error\":\"No se encontro solucion\"}");
        }

        crow::json::wvalue response;
        response["rows"] = filas;
        response["cols"] = cols;
        response["time_ms"] = static_cast<int>(res.tiempo.count());

        std::vector<int> cells_flat(filas * cols);
        for (int i = 0; i < filas * cols; ++i) {
            cells_flat[i] = res.asignacion[i];
        }
        response["cells"] = std::move(cells_flat);

        for (int i = 0; i < static_cast<int>(res.regiones.size()); ++i) {
            auto const& r = res.regiones[i];
            response["regions"][i]["id"] = r.id;
            response["regions"][i]["value"] = r.value;
            response["regions"][i]["cells"] = r.cells;
            response["regions"][i]["r0"] = r.r0;
            response["regions"][i]["c0"] = r.c0;
            response["regions"][i]["r1"] = r.r1;
            response["regions"][i]["c1"] = r.c1;
        }

        crow::response r_out(200, response);
        r_out.add_header("Access-Control-Allow-Origin", "*");
        return r_out;
    });

    // Rutas estaticas para el frontend
    CROW_ROUTE(app, "/")([](){ return serve_static("index.html"); });
    CROW_ROUTE(app, "/<path>")([](std::string path){ return serve_static(path); });

    app.port(18080).multithreaded().run();
}
