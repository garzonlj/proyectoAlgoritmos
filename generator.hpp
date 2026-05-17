#ifndef GENERATOR_HPP
#define GENERATOR_HPP

#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using Tablero = std::vector<std::vector<uint8_t>>;

inline void dividir(Tablero& grid, int r, int c, int h, int w) {
    // Caso base: área muy pequeña o aleatoriedad para parar
    if (h * w <= 4 || (std::rand() % 100) < 30) {
        int pistaR = r + (std::rand() % h);
        int pistaC = c + (std::rand() % w);
        grid[pistaR][pistaC] = h * w;
        return;
    }

    if (h > w || (h == w && (std::rand() % 2) == 0)) {
        // Dividir horizontalmente
        int splitH = 1 + (std::rand() % (h - 1));
        dividir(grid, r, c, splitH, w);
        dividir(grid, r + splitH, c, h - splitH, w);
    } else {
        // Dividir verticalmente
        int splitW = 1 + (std::rand() % (w - 1));
        dividir(grid, r, c, h, splitW);
        dividir(grid, r, c + splitW, h, w - splitW);
    }
}

inline Tablero generar_tablero(int size) {
    Tablero grid(size, std::vector<uint8_t>(size, 0));
    dividir(grid, 0, 0, size, size);
    return grid;
}

#endif
