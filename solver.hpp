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

/**
 * ESTRUCTURAS DE DATOS PRINCIPALES
 * Tablero: Matriz de 8 bits para ahorrar memoria.
 * RegId: Identificador unico para cada region (rectangulo).
 * Asignacion: Mapa que asocia cada indice de celda (r*cols + c) con un ID de region.
 */
using Tablero    = std::vector<std::vector<uint8_t>>;
using RegId      = int; 
using Asignacion = std::unordered_map<int, RegId>;

struct Rect {
    int r0, c0, r1, c1; // Coordenadas de la esquina superior izquierda y inferior derecha
    int clue_index;     // Indice de la pista a la que pertenece este rectangulo
};

struct Clue {
    int r, c, value;    // Posicion (r, c) y valor del area requerida
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
    std::chrono::nanoseconds   tiempo; 
};

/**
 * leer_tablero: Carga el puzzle desde un archivo de texto.
 * Soporta espacios, tabulaciones y saltos de linea.
 */
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

/**
 * validar_tablero: Verifica que el tablero sea rectangular y tenga al menos una pista.
 */
inline std::vector<std::string> validar_tablero(Tablero const& t) {
    std::vector<std::string> errs;
    if (t.empty()) { errs.emplace_back("Tablero vacio."); return errs; }
    int cols = static_cast<int>(t[0].size());
    for (int i = 0; i < static_cast<int>(t.size()); ++i) {
        if (static_cast<int>(t[i].size()) != cols)
            errs.emplace_back("Fila " + std::to_string(i) + " inconsistente.");
        for (int j = 0; j < cols; ++j)
            if (t[i][j] < 0)
                errs.emplace_back("Valor negativo en (" + std::to_string(i) + "," + std::to_string(j) + ")");
    }
    bool has_clue = false;
    for (auto const& row : t)
        for (auto v : row)
            if (v > 0) { has_clue = true; break; }
    if (!has_clue) errs.emplace_back("Sin pistas.");
    return errs;
}

/**
 * resolver: Implementa el algoritmo de busqueda con Backtracking y heuristica MRV.
 * 
 * 1. PRE-CALCULO (Candidatos): Para cada pista, generamos todos los rectangulos posibles
 *    que tengan el area exacta y contengan la pista, asegurandonos de que no cubran
 *    otras pistas.
 * 
 * 2. HEURISTICA MRV (Minimum Remaining Values): Ordenamos las pistas por la cantidad 
 *    de rectangulos candidatos. Empezar por la pista "mas restringida" reduce
 *    drasticamente el espacio de busqueda.
 * 
 * 3. BACKTRACKING: Intentamos colocar un rectangulo para la pista actual, marcamos
 *    las celdas como ocupadas y pasamos a la siguiente pista. Si no hay solucion,
 *    deshacemos los cambios (backtrack) e intentamos el siguiente candidato.
 */
inline bool resolver(Tablero const& tablero, Resultado& out) {
    auto t0 = std::chrono::steady_clock::now();
    
    int filas = static_cast<int>(tablero.size());
    if (filas == 0) return false;
    int cols = static_cast<int>(tablero[0].size());
    int total = filas * cols;
    
    // Identificar todas las pistas (>0)
    std::vector<Clue> clues;
    for (int r = 0; r < filas; r++)
        for (int c = 0; c < cols; c++)
            if (tablero[r][c] > 0)
                clues.push_back({r, c, static_cast<int>(tablero[r][c])});
    
    if (clues.empty()) return false;
    
    // Mapa rapido para saber que celda tiene que pista
    std::vector<int> cell_clue(total, -1);
    for (int i = 0; i < static_cast<int>(clues.size()); i++)
        cell_clue[clues[i].r * cols + clues[i].c] = i;
    
    // Fase 1: Generacion de Candidatos
    std::vector<std::vector<Rect>> candidates(clues.size());
    for (int i = 0; i < static_cast<int>(clues.size()); i++) {
        auto& clue = clues[i];
        int V = clue.value;
        for (int h = 1; h <= V; h++) {
            if (V % h != 0) continue;
            int w = V / h;
            // Probar todas las posiciones (r0, c0) del rectangulo (h x w) que contienen la pista
            for (int r0 = std::max(0, clue.r - h + 1); r0 <= std::min(clue.r, filas - h); r0++) {
                for (int c0 = std::max(0, clue.c - w + 1); c0 <= std::min(clue.c, cols - w); c0++) {
                    bool ok = true;
                    // El rectangulo no debe contener otras pistas
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
    
    // Fase 2: Aplicar heuristica (Ordenar pistas por dificultad)
    std::vector<int> order(clues.size());
    for(int i=0; i<clues.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b){
        return candidates[a].size() < candidates[b].size();
    });
    
    std::vector<std::vector<Rect>> sorted_candidates(clues.size());
    for(int i=0; i<clues.size(); ++i) sorted_candidates[i] = candidates[order[i]];
    
    // Estructuras para el estado del backtracking
    std::vector<bool> covered(total, false);
    std::vector<int> cell_region(total, -1);
    
    // Fase 3: Backtracking recursivo
    std::function<bool(int)> backtrack;
    backtrack = [&](int idx) -> bool {
        // Caso base: todas las pistas han sido asignadas
        if (idx == static_cast<int>(clues.size())) {
            for (int i = 0; i < total; i++) if (!covered[i]) return false;
            return true;
        }
        
        for (auto const& rect : sorted_candidates[idx]) {
            // Verificar si el espacio para este candidato esta libre
            bool ok = true;
            for (int rr = rect.r0; rr <= rect.r1 && ok; rr++)
                for (int cc = rect.c0; cc <= rect.c1 && ok; cc++)
                    if (covered[rr * cols + cc]) ok = false;
            
            if (!ok) continue;
            
            // Paso recursivo: Marcar y avanzar
            for (int rr = rect.r0; rr <= rect.r1; rr++)
                for (int cc = rect.c0; cc <= rect.c1; cc++) {
                    covered[rr * cols + cc] = true;
                    cell_region[rr * cols + cc] = order[idx];
                }
            
            if (backtrack(idx + 1)) return true;
            
            // Backtrack: Desmarcar y probar el siguiente candidato
            for (int rr = rect.r0; rr <= rect.r1; rr++)
                for (int cc = rect.c0; cc <= rect.c1; cc++) {
                    covered[rr * cols + cc] = false;
                    cell_region[rr * cols + cc] = -1;
                }
        }
        return false;
    };

    if (!backtrack(0)) return false;
    
    auto t1 = std::chrono::steady_clock::now();
    out.tiempo = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0);
    
    // Preparar salida
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
