# Contexto del Proyecto: Shikaku Solver

## Objetivo
Solver en C++ para el juego Shikaku ("Divide Rectangles"), con interfaz web en Angular + servidor HTTP ligero en C++ (Crow).

## Reglas del Shikaku (ESTANDAR)
- Cuadrícula con NÚMEROS (pistas) en ALGUNAS celdas, 0 en las demás.
- Cada pista indica el ÁREA del rectángulo que debe contenerla.
- Los rectángulos no se solapan y cubren toda la cuadrícula.
- Cada rectángulo contiene EXACTAMENTE UNA pista.
- El área del rectángulo debe ser igual al valor de la pista.

## Archivos existentes
| Archivo | Descripción |
|---|---|
| `shikaku_solver.c++` | Solver en C++11 (CLI) para línea de comandos |
| `solver.hpp` | Header con lógica del solver estándar (Backtracking + MRV Heuristic) |
| `generator.hpp` | Header con lógica para la generación procedural de tableros válidos |
| `shikaku_server.c++` | Servidor HTTP con Crow (C++17, puerto 18080) |
| `include/crow.h` + `include/crow/` | Headers del framework web Crow |
| `frontend/` | Aplicación Angular 21 para visualización web y juego interactivo |
| `frontend/dist/shikaku-frontend/browser/` | Build de producción de la SPA de Angular |
| `frontend/src/app/` | Código fuente Angular (Componentes, Servicios, Material UI) |
| `ejemplos/` | Puzzles de ejemplo con pistas (0 = vacío) |
| `comandos/COMANDOS.txt` | Instrucciones de compilación para múltiples plataformas |

## Algoritmo (Shikaku estándar)
1. **Lectura**: Archivo, JSON o String de matriz con números (0 = celda vacía, >0 = pista).
2. **Validación (Frontend y Backend)**: Dimensiones uniformes, valores >= 0, pistas válidas, y validación matemática (suma de pistas = área total).
3. **Candidatos**: Para cada pista, se generan todos los rectángulos posibles que contengan la pista y tengan el área requerida.
4. **Heurística MRV (Minimum Remaining Values)**: Se ordenan las pistas de menor a mayor cantidad de rectángulos candidatos para optimizar el árbol de búsqueda.
5. **Backtracking**:
   - Se prueban los rectángulos candidatos para la pista actual.
   - Se marca la región cubierta en la cuadrícula.
   - Se llama recursivamente para la siguiente pista.
6. **Visualización**: Renderizado en UI Angular con colores distintivos por región, cronómetro de alta precisión, y animaciones.

## Optimizaciones y Mejoras Recientes
- `uint8_t` para valores del tablero.
- `int` nativo para IDs de región (para evitar desbordamiento en tableros grandes).
- `std::chrono::microseconds` para medición de tiempos de alta precisión del solucionador IA.
- Interfaz renovada estilo Mobile-First (Tipografía Inter, diseño sin íconos conflictivos).
- Sistema de **Leaderboard Local** almacenado en caché.
- Mecánica Click-and-Drag para selección de rectángulos en modo manual.

## Arquitectura Web
```
                     +----------------------+
                     |  Angular App (SPA)   |
                     |  (dist/shikaku-...)  |
                     +----------+-----------+
                                | HTTP / JSON
                     +----------v-----------+
                     |  Crow Server (:18080)|
                     |                      |
                     |  GET  /             -> index.html
                     |  GET  /api/generate -> Tablero Aleatorio
                     |  POST /api/solve    -> JSON solución
                     +--------------------+
```

## API REST

**1. Generar Tablero:** `GET /api/generate?size=N`
```json
// Response 200
{
  "board_str": "4 0 0 0\n0 0 3 0\n0 4 0 0\n0 0 0 5"
}
```

**2. Resolver Tablero:** `POST /api/solve`
```json
// Request
{ "board": [[4,0,3,0,0],[0,0,6,0,0],[2,0,0,0,0],[4,0,6,0,0],[0,0,0,0,0]] }

// Response 200
{
  "rows": 5, "cols": 5,
  "cells": [0,0,1,1,1, 0,0,2,2,2, 3,3,2,2,2, 4,4,5,5,5, 4,4,5,5,5],
  "regions": [
    {"id": 0, "value": 4, "cells": 4, "r0":0,"c0":0,"r1":1,"c1":1}, ...
  ],
  "time_us": 1500 // Tiempo en microsegundos
}

// Response 400/200 (con error)
{ "error": "mensaje descriptivo" }
```

## Dependencias
- **Crow** (C++ Microframework)
- **Node.js** + **Angular CLI** (Frontend SPA)
- **Compilador C++17** compatible con sockets/threads (GCC/MinGW/Clang)