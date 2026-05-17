# Contexto del Proyecto: Shikaku Solver

## Objetivo
Solver en C++ para el juego Shikaku ("Divide Rectangles"), con interfaz web Angular + servidor Crow.

## Reglas del Shikaku (ESTANDAR)
- Cuadricula con NUMEROS (pistas) en ALGUNAS celdas, 0 en las demas.
- Cada pista indica el AREA del rectangulo que debe contenerla.
- Los rectangulos no se solapan y cubren toda la cuadricula.
- Cada rectangulo contiene EXACTAMENTE UNA pista.
- El area del rectangulo debe ser igual al valor de la pista.

## Archivos existentes
| Archivo | Descripcion |
|---|---|
| `chikaku_solver.py` | Solver original en Python (referencia) |
| `shikaku_solver.c++` | Solver en C++11 (CLI) |
| `solver.hpp` | Header con logica del solver estandar (compartido) |
| `shikaku_server.c++` | Servidor HTTP con Crow (C++17, puerto 18080) |
| `include/crow.h` + `include/crow/` | Headers del framework Crow |
| `frontend/` | Aplicacion Angular 21 para visualizacion web |
| `frontend/dist/browser/` | Build de produccion del Angular |
| `frontend/src/app/` | Codigo fuente Angular |
| `ejemplos/` | 10 puzzles de ejemplo con pistas (0 = vacio) |
| `puzzle.txt` | Tablero original de referencia |
| `context/enunciado_2026_10.pdf` | Enunciado del proyecto |
| `.vscode/c_cpp_properties.json` | Config de IntelliSense para VS Code |

## Algoritmo (Shikaku estandar)
1. **Lectura**: Archivo o JSON con numeros (0 = celda vacia, >0 = pista).
2. **Validacion**: Dimensiones uniformes, valores >= 0, al menos una pista.
3. **Pistas**: Se identifican las celdas con valor > 0 como pistas.
4. **Candidatos**: Para cada pista, se generan todos los rectangulos que:
   - Contienen la pista
   - Tienen area = valor de la pista
   - No cubren otras pistas
   - Caben en la cuadricula
5. **Backtracking**:
   - Se toma la primera celda sin cubrir
   - Se prueban todos los rectangulos candidatos que la cubren
   - Se marca el rectangulo y la pista como asignados
   - Se recursa hasta cubrir toda la cuadricula
6. **Visualizacion**: Colores CSS con bordes gruesos entre regiones.

## Optimizaciones
- `uint8_t` para valores del tablero.
- `int8_t` para IDs de region.
- `int` nativo para indices (32 bits en LP64).
- Lookup table `cell_clue` para verificacion O(1) de pistas.
- Pre-reserva de capacidad en contenedores.
- Backtracking con estado mutable (sin copias).

## Arquitectura Web
```
                     +----------------------+
                     |  Angular App (SPA)   |
                     |  frontend/dist/browser|
                     +----------+-----------+
                                | HTTP (fetch)
                     +----------v-----------+
                     |  Crow Server (:18080)|
                     |                      |
                     |  GET  /          -> index.html
                     |  GET  /<path>    -> archivo estatico
                     |  POST /api/solve -> JSON solucion
                     +--------------------+
```

## API REST
**POST /api/solve**
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
  "time_ms": 0
}

// Response 400
{ "error": "mensaje" }
```

## Compilacion y ejecucion
```sh
# CLI solver
g++ -std=c++11 -O3 -o shikaku_solver shikaku_solver.c++
./shikaku_solver ejemplos/01_5x5.txt

# Servidor web
g++ -std=c++17 -O3 -Iinclude -pthread -o shikaku_server shikaku_server.c++
./shikaku_server
# -> http://localhost:18080

# Frontend (desarrollo)
cd frontend && npx ng serve
```

## Dependencias externas
- **Crow** (GitHub master, header-only, en `include/`)
- **ASIO** (`pacman -S asio`)
- **Node.js** v26 + npm 11 (para Angular)
- **Angular CLI** (v21.x)
- **g++** (GCC 16.1.1) con `-std=c++17` para Crow, `-std=c++11` para CLI
