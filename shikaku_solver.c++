#include "solver.hpp"
#include <iostream>

// =============================================================
//  Visualización (CLI)
// =============================================================

static constexpr char const* const COLORES[] = {
    "\033[41m", "\033[42m", "\033[43m", "\033[44m", "\033[45m",
    "\033[46m", "\033[101m","\033[102m","\033[103m","\033[104m",
    "\033[105m","\033[106m","\033[100m","\033[47m", "\033[107m",
    "\033[30;43m","\033[30;46m","\033[97;41m","\033[97;44m","\033[30;47m",
};
static constexpr char const* RESET   = "\033[0m";
static constexpr char const* NEGRITA = "\033[1m";
static constexpr int         NCOLORES = 20;

static void mostrar_tablero_original(Tablero const& t) {
    int f = static_cast<int>(t.size());
    int c = static_cast<int>(t[0].size());
    std::cout << "\n  Tablero original:\n  ┌";
    for (int j = 0; j < c - 1; ++j) std::cout << "────┬";
    std::cout << "────┐\n";
    for (int i = 0; i < f; ++i) {
        std::cout << "  │";
        for (int j = 0; j < c; ++j)
            std::cout << " " << static_cast<int>(t[i][j]) << "  │";
        std::cout << "\n";
        if (i < f - 1) {
            std::cout << "  ├";
            for (int j = 0; j < c - 1; ++j) std::cout << "────┼";
            std::cout << "────┤\n";
        }
    }
    std::cout << "  └";
    for (int j = 0; j < c - 1; ++j) std::cout << "────┴";
    std::cout << "────┘\n\n";
}

static void mostrar_solucion(Tablero const& t, Asignacion const& asig) {
    int filas = static_cast<int>(t.size());
    int cols  = static_cast<int>(t[0].size());
    int total = filas * cols;

    std::vector<RegId> regiones;
    regiones.reserve(static_cast<std::size_t>(total));
    for (auto const& kv : asig) regiones.push_back(kv.second);
    std::sort(regiones.begin(), regiones.end());
    regiones.erase(std::unique(regiones.begin(), regiones.end()),
                   regiones.end());

    auto color_id = [&](RegId id) -> char const* {
        auto it = std::find(regiones.begin(), regiones.end(), id);
        return COLORES[(it - regiones.begin()) % NCOLORES];
    };

    auto linea = [](int n) { for (int i = 0; i < n; ++i) std::cout << "═"; };
    std::cout << "\n"; linea(cols * 5 + 1);
    std::cout << "\n  SOLUCIÓN — " << regiones.size() << " regiones\n";
    linea(cols * 5 + 1); std::cout << "\n";

    std::cout << "┌";
    for (int j = 0; j < cols - 1; ++j) std::cout << "────┬";
    std::cout << "────┐\n";

    for (int r = 0; r < filas; ++r) {
        std::cout << "│";
        for (int c = 0; c < cols; ++c) {
            RegId id = asig.at(r * cols + c);
            std::cout << color_id(id) << NEGRITA << " " << static_cast<int>(t[r][c])
                      << "  " << RESET << "│";
        }
        std::cout << "\n";
        if (r < filas - 1) {
            std::cout << "├";
            for (int c = 0; c < cols - 1; ++c)
                std::cout << (asig.at(r*cols+c) != asig.at((r+1)*cols+c)
                              ? "════╪" : "────┼");
            int lc = cols - 1;
            std::cout << (asig.at(r*cols+lc) != asig.at((r+1)*cols+lc)
                          ? "════╡" : "────┤") << "\n";
        }
    }
    std::cout << "└";
    for (int j = 0; j < cols - 1; ++j) std::cout << "────┴";
    std::cout << "────┘\n";

    std::cout << "\n  Leyenda de regiones:\n";
    for (auto reg : regiones) {
        int celdas = 0, val = 0;
        for (auto const& kv : asig) {
            if (kv.second != reg) continue;
            if (celdas == 0) val = t[kv.first / cols][kv.first % cols];
            ++celdas;
        }
        std::cout << "    " << color_id(reg) << NEGRITA
                  << " Reg " << static_cast<int>(reg) << "  " << RESET
                  << " " << celdas << " celdas (valor=" << val << ")\n";
    }
    std::cout << "\n";
}

// =============================================================
//  main
// =============================================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <archivo.txt>\n\n"
                  << "Ejemplo de archivo puzzle.txt:\n"
                  << "  2 3 3 2 2\n  2 3 3 2 2\n  3 3 3 2 2\n"
                  << "  3 3 3 4 4\n  1 3 3 4 4\n";
        return 1;
    }

    Tablero tablero;
    if (!leer_tablero(argv[1], tablero)) {
        std::cerr << "  Archivo no encontrado: " << argv[1] << "\n";
        return 1;
    }

    std::cout << "\n  Tablero cargado: " << tablero.size() << " filas × "
              << tablero[0].size() << " columnas\n";

    auto errs = validar_tablero(tablero);
    if (!errs.empty()) {
        std::cerr << "  Errores en el tablero:\n";
        for (auto const& e : errs) std::cerr << "    • " << e << "\n";
        return 1;
    }

    mostrar_tablero_original(tablero);
    std::cout << "Resolviendo con memoización...\n";

    Resultado res;
    if (!resolver(tablero, res)) {
        std::cout << "  No se encontró solución.\n";
        return 1;
    }

    mostrar_solucion(tablero, res.asignacion);
    std::cout << "  Resuelto en " << res.tiempo.count() << " ms\n\n";
    return 0;
}
