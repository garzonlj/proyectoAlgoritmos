# Shikaku Solver

¡Bienvenido al **Shikaku Solver**! Este proyecto es un solucionador e interfaz interactiva para el clásico rompecabezas lógico japonés "Shikaku" (también conocido como "Divide Rectangles").

El proyecto combina un potente backend matemático desarrollado en **C++** con una moderna interfaz de usuario interactiva construida en **Angular**, ofreciendo una experiencia fluida tanto para jugar manualmente como para que la IA resuelva los tableros al instante.

## 🌟 Características Principales

*   **Generador de Tableros C++:** Crea instantáneamente rompecabezas garantizados y válidos en diferentes tamaños (5x5, 6x6, 7x7...).
*   **Solucionador IA Ultra-Rápido:** Utiliza un algoritmo de Backtracking optimizado con la heurística MRV (Minimum Remaining Values) para resolver cualquier tablero válido en cuestión de microsegundos.
*   **Modo de Juego Manual:** Mecánica intuitiva de selección ("click and drag") para resolver los rompecabezas a mano, con validación matemática en tiempo real.
*   **Leaderboard Local:** Guarda tus mejores tiempos y compite contigo mismo.
*   **Interfaz Mobile-First:** Un diseño limpio, "glassmorphism", responsivo y pulido, basado en los principios de diseño de Apple/Google.
*   **Validación Robusta:** Sistema anti-tableros rotos que garantiza que ningún dato corrompido haga fallar la aplicación.

## 🛠️ Tecnologías Utilizadas

*   **Backend:** C++17
*   **Framework HTTP:** Crow (Microframework header-only para C++)
*   **Frontend:** Angular (TypeScript, HTML5, CSS3)
*   **UI Components:** Angular Material
*   **Tipografía:** Inter (Google Fonts)

## 🚀 Guía de Instalación y Uso

Para ejecutar el proyecto en tu máquina local, sigue las instrucciones detalladas en el archivo de comandos multiplataforma.

### 1. Construir el Frontend (Angular)
Necesitas tener instalado Node.js (v18 o superior).

```bash
cd frontend
npm install
npx ng build --configuration production
cd ..
```

### 2. Compilar y Ejecutar el Servidor (C++)
Necesitas un compilador de C++ que soporte C++17 (como GCC, Clang o MinGW).

**En Linux / macOS:**
```bash
g++ -std=c++17 -O3 -Iinclude -pthread -o shikaku_server shikaku_server.c++
./shikaku_server
```

**En Windows (PowerShell / CMD con MinGW):**
```bash
g++ -std=c++17 -O3 -Iinclude -pthread -o shikaku_server.exe shikaku_server.c++ -lws2_32 -lmswsock
.\shikaku_server.exe
```

Una vez que el servidor esté corriendo, abre tu navegador web y visita:
👉 **http://localhost:18080**

## 🧩 Cómo Jugar Shikaku
1. Tienes una cuadrícula con varios números (pistas) dispersos en ella.
2. Debes dividir la cuadrícula en **rectángulos o cuadrados**.
3. Cada rectángulo debe contener **exactamente un número**.
4. El área del rectángulo (cantidad de celdillas que ocupa) debe ser **igual al número que contiene**.
5. Los rectángulos **no pueden solaparse** y no puede quedar ninguna celda libre en el tablero final.

## 📂 Estructura del Código
*   `shikaku_server.c++`: El punto de entrada del servidor API REST y web server de archivos estáticos.
*   `solver.hpp`: El corazón del proyecto, donde vive el motor matemático y el algoritmo Backtracking.
*   `generator.hpp`: Algoritmo generador de particiones de cuadrícula procedural.
*   `frontend/src/`: Todo el código de la UI en Angular.
*   `context/`: Documentación técnica e historia arquitectónica del proyecto para desarrolladores.

---
*Desarrollado para el Proyecto de Algoritmos - 2026*
