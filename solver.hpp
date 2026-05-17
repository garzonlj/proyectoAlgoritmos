#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using Tablero    = std::vector<std::vector<uint8_t>>;
using RegId      = int; // Cambiado de int8_t a int para evitar desbordamiento
using Asignacion = std::unordered_map<int, RegId>;

struct Rect {
    int r0, c0, r1, c1;
    int clue_index;
};

struct Clue {
    int r, c, value;
};

struct RegionInfo {
    RegId id;
    int   value;
    int   cells;
    int   r0, c0, r1, c1;
};

struct Resultado {
    Asignacion                 asignacion;
    std::vector<RegionInfo>    regiones;
    std::chrono::milliseconds  tiempo;
};

inline bool leer_tablero(std::string const& ruta, Tablero& out) {
    std::ifstream f(ruta);
    if (!f.is_open()) return false;
    Tablero tablero;
    std::string linea;
    while (std::getline(f, linea)) {
        auto first = linea.find_first_not_of(" \t\r");
        if (first == std::string::npos) continue;
        auto last = linea.find_last_not_of(" \t\r");
        linea = linea.substr(first, last - first + 1);
        std::istringstream iss(linea);
        std::vector<uint8_t> fila;
        int v;
        while (iss >> v) fila.push_back(static_cast<uint8_t>(v));
        if (!fila.empty()) tablero.push_back(std::move(fila));
    }
    out = std::move(tablero);
    return true;
}

inline std::vector<std::string> validar_tablero(Tablero const& t) {
    std::vector<std::string> errs;
    if (t.empty()) { errs.emplace_back("Tablero vacio."); return errs; }
    int cols = static_cast<int>(t[0].size());
    for (int i = 0; i < static_cast<int>(t.size()); ++i) {
        if (static_cast<int>(t[i].size()) != cols)
            errs.emplace_back("Fila " + std::to_string(i) + " tiene " + std::to_string(t[i].size()) + " columnas, se esperan " + std::to_string(cols));
        for (int j = 0; j < cols; ++j)
            if (t[i][j] < 0)
                errs.emplace_back("Valor invalido en (" + std::to_string(i) + "," + std::to_string(j) + "): " + std::to_string(t[i][j]) + " (debe ser >= 0)");
    }
    bool has_clue = false;
    for (auto const& row : t)
        for (auto v : row)
            if (v > 0) { has_clue = true; break; }
    if (!has_clue) errs.emplace_back("El tablero no tiene pistas (valores > 0).");
    return errs;
}

inline bool resolver(Tablero const& tablero, Resultado& out) {
    auto t0 = std::chrono::steady_clock::now();
    
    int filas = static_cast<int>(tablero.size());
    if (filas == 0) return false;
    int cols = static_cast<int>(tablero[0].size());
    int total = filas * cols;
    
    std::vector<Clue> clues;
    for (int r = 0; r < filas; r++)
        for (int c = 0; c < cols; c++)
            if (tablero[r][c] > 0)
                clues.push_back({r, c, static_cast<int>(tablero[r][c])});
    
    if (clues.empty()) return false;
    
    // Mapear cada celda a su pista si existe
    std::vector<int> cell_clue(total, -1);
    for (int i = 0; i < static_cast<int>(clues.size()); i++)
        cell_clue[clues[i].r * cols + clues[i].c] = i;
    
    // Generar candidatos para cada pista
    std::vector<std::vector<Rect>> candidates(clues.size());
    for (int i = 0; i < static_cast<int>(clues.size()); i++) {
        auto& clue = clues[i];
        int V = clue.value;
        // Probar todas las combinaciones de (h, w) tal que h*w = V
        for (int h = 1; h <= V; h++) {
            if (V % h != 0) continue;
            int w = V / h;
            
            // Probar todas las posiciones posibles del rectángulo (h, w) que contienen la pista
            for (int r0 = std::max(0, clue.r - h + 1); r0 <= std::min(clue.r, filas - h); r0++) {
                for (int c0 = std::max(0, clue.c - w + 1); c0 <= std::min(clue.c, cols - w); c0++) {
                    bool ok = true;
                    // Verificar que no contenga otras pistas
                    for (int rr = r0; rr < r0 + h && ok; rr++) {
                        for (int cc = c0; cc < c0 + w && ok; cc++) {
                            int idx = rr * cols + cc;
                            if (cell_clue[idx] != -1 && cell_clue[idx] != i)
                                ok = false;
                        }
                    }
                    if (ok)
                        candidates[i].push_back({r0, c0, r0 + h - 1, c0 + w - 1, i});
                }
            }
        }
    }
    
    std::vector<bool> covered(total, false);
    std::vector<int> cell_region(total, -1);
    std::vector<bool> clue_assigned(clues.size(), false);
    
    std::function<bool(int)> backtrack;
    backtrack = [&](int clue_idx) -> bool {
        if (clue_idx == static_cast<int>(clues.size())) {
            // Verificar si todo está cubierto
            for (int i = 0; i < total; i++) if (!covered[i]) return false;
            return true;
        }
        
        for (auto const& rect : candidates[clue_idx]) {
            bool ok = true;
            for (int rr = rect.r0; rr <= rect.r1 && ok; rr++)
                for (int cc = rect.c0; cc <= rect.c1 && ok; cc++)
                    if (covered[rr * cols + cc]) ok = false;
            
            if (!ok) continue;
            
            // Marcar
            for (int rr = rect.r0; rr <= rect.r1; rr++)
                for (int cc = rect.c0; cc <= rect.c1; cc++) {
                    covered[rr * cols + cc] = true;
                    cell_region[rr * cols + cc] = clue_idx;
                }
            
            if (backtrack(clue_idx + 1)) return true;
            
            // Desmarcar
            for (int rr = rect.r0; rr <= rect.r1; rr++)
                for (int cc = rect.c0; cc <= rect.c1; cc++) {
                    covered[rr * cols + cc] = false;
                    cell_region[rr * cols + cc] = -1;
                }
        }
        return false;
    };
    
    // Optimizacion: Ordenar pistas por número de candidatos (heurística de valor más restringido)
    std::vector<int> order(clues.size());
    for(int i=0; i<clues.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b){
        return candidates[a].size() < candidates[b].size();
    });
    
    // Re-organizar candidatos según el orden
    std::vector<std::vector<Rect>> sorted_candidates(clues.size());
    for(int i=0; i<clues.size(); ++i) sorted_candidates[i] = candidates[order[i]];
    
    // Nuevo backtrack usando el orden
    std::function<bool(int)> backtrack_ordered;
    backtrack_ordered = [&](int idx) -> bool {
        if (idx == static_cast<int>(clues.size())) {
            for (int i = 0; i < total; i++) if (!covered[i]) return false;
            return true;
        }
        
        for (auto const& rect : sorted_candidates[idx]) {
            bool ok = true;
            for (int rr = rect.r0; rr <= rect.r1 && ok; rr++)
                for (int cc = rect.c0; cc <= rect.c1 && ok; cc++)
                    if (covered[rr * cols + cc]) ok = false;
            
            if (!ok) continue;
            
            for (int rr = rect.r0; rr <= rect.r1; rr++)
                for (int cc = rect.c0; cc <= rect.c1; cc++) {
                    covered[rr * cols + cc] = true;
                    cell_region[rr * cols + cc] = order[idx];
                }
            
            if (backtrack_ordered(idx + 1)) return true;
            
            for (int rr = rect.r0; rr <= rect.r1; rr++)
                for (int cc = rect.c0; cc <= rect.c1; cc++) {
                    covered[rr * cols + cc] = false;
                    cell_region[rr * cols + cc] = -1;
                }
        }
        return false;
    };

    if (!backtrack_ordered(0)) return false;
    
    auto t1 = std::chrono::steady_clock::now();
    out.tiempo = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
    
    out.asignacion.clear();
    for (int i = 0; i < total; i++)
        out.asignacion[i] = cell_region[i];
    
    out.regiones.clear();
    for (int ci = 0; ci < static_cast<int>(clues.size()); ci++) {
        RegionInfo ri;
        ri.id = ci;
        ri.value = clues[ci].value;
        ri.cells = 0;
        ri.r0 = filas; ri.c0 = cols;
        ri.r1 = -1; ri.c1 = -1;
        for (int i = 0; i < total; i++) {
            if (cell_region[i] != ci) continue;
            int rr = i / cols, cc = i % cols;
            ri.r0 = std::min(ri.r0, rr); ri.r1 = std::max(ri.r1, rr);
            ri.c0 = std::min(ri.c0, cc); ri.c1 = std::max(ri.c1, cc);
            ri.cells++;
        }
        if (ri.cells > 0) out.regiones.push_back(ri);
    }
    
    return true;
}

#endif
