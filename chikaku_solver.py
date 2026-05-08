"""
Uso:
    python chikaku_solver.py puzzle.txt
"""

import sys
from functools import lru_cache



#  Lectura del tablero desde el archivo


def leer_tablero(ruta: str) -> list[list[int]]:

    tablero = []
    with open(ruta, encoding="utf-8") as f:
        for linea in f:
            linea = linea.strip()
            if not linea:
                continue
            fila = list(map(int, linea.split()))
            tablero.append(fila)
    return tablero



#  Lógica de resolución con memoización 


def resolver(tablero: list[list[int]]) -> dict[tuple, tuple] | None:


    filas = len(tablero)
    if filas == 0:
        return None
    cols = len(tablero[0])
    total_celdas = filas * cols

    candidatos: dict[tuple, list[tuple]] = {}
    for r in range(filas):
        for c in range(cols):
            valor = tablero[r][c]
            rects = []
            for alto in range(1, filas - r + 1):
                for ancho in range(1, cols - c + 1):
                    if alto * ancho == valor:
                        if all(
                            tablero[r + dr][c + dc] == valor
                            for dr in range(alto)
                            for dc in range(ancho)
                        ):
                            rects.append((r, c, r + alto - 1, c + ancho - 1))
            candidatos[(r, c)] = rects

    estado_inicial = tuple([-1] * total_celdas)

    memo: dict[tuple, tuple | None] = {}

    def celda_idx(r: int, c: int) -> int:
        return r * cols + c

    def primera_libre(estado: tuple) -> int | None:
        """Devuelve el índice de la primera celda sin asignar, o None."""
        for i, v in enumerate(estado):
            if v == -1:
                return i
        return None

    def aplicar_region(estado: tuple, r0: int, c0: int, r1: int, c1: int, id_reg: int) -> tuple | None:

        lst = list(estado)
        for r in range(r0, r1 + 1):
            for c in range(c0, c1 + 1):
                idx = celda_idx(r, c)
                if lst[idx] != -1:
                    return None  # Conflicto de celda ya ocupada
                lst[idx] = id_reg
        return tuple(lst)

    def backtrack(estado: tuple, id_reg: int) -> tuple | None:
        if estado in memo:
            return memo[estado]

        idx_libre = primera_libre(estado)
        if idx_libre is None:
            #si todas las celdas fueron asignadas
            memo[estado] = estado
            return estado

        r0 = idx_libre // cols
        c0 = idx_libre % cols

        for rect in candidatos[(r0, c0)]:
            r0_, c0_, r1, c1 = rect
            nuevo_estado = aplicar_region(estado, r0_, c0_, r1, c1, id_reg)
            if nuevo_estado is None:
                continue
            resultado = backtrack(nuevo_estado, id_reg + 1)
            if resultado is not None:
                memo[estado] = resultado
                return resultado

        memo[estado] = None
        return None

    estado_final = backtrack(estado_inicial, 0)
    if estado_final is None:
        return None

    # Convertir tupla plana en dict {(r,c): id_region}
    return {
        (i // cols, i % cols): estado_final[i]
        for i in range(total_celdas)
    }



#  Visualización en consola

COLORES = [
    "\033[41m", "\033[42m", "\033[43m", "\033[44m", "\033[45m",
    "\033[46m", "\033[101m", "\033[102m", "\033[103m", "\033[104m",
    "\033[105m", "\033[106m", "\033[100m", "\033[47m", "\033[107m",
    "\033[30;43m", "\033[30;46m", "\033[97;41m", "\033[97;44m", "\033[30;47m",
]
RESET = "\033[0m"
NEGRITA = "\033[1m"


def mostrar_solucion(tablero: list[list[int]], asignacion: dict) -> None:
    filas = len(tablero)
    cols = len(tablero[0])

    # Número de regiones únicas
    regiones = sorted(set(asignacion.values()))
    color_reg = {reg: COLORES[i % len(COLORES)] for i, reg in enumerate(regiones)}

    print("\n" + "═" * (cols * 5 + 1))
    print(f"  SOLUCIÓN — {len(regiones)} regiones")
    print("═" * (cols * 5 + 1))

    # Borde superior
    print("┌" + ("────┬" * (cols - 1)) + "────┐")

    for r in range(filas):
        # Fila de valores
        fila_str = "│"
        for c in range(cols):
            id_reg = asignacion[(r, c)]
            color = color_reg[id_reg]
            valor = tablero[r][c]
            fila_str += f"{color}{NEGRITA} {valor:2d} {RESET}│"
        print(fila_str)

        # Separador horizontal
        if r < filas - 1:
            sep = "├"
            for c in range(cols):
                reg_actual = asignacion[(r, c)]
                reg_abajo = asignacion[(r + 1, c)]
                # Línea más gruesa si cambia de región verticalmente
                if reg_actual != reg_abajo:
                    sep += "════╪" if c < cols - 1 else "════╡"
                else:
                    sep += "────┼" if c < cols - 1 else "────┤"
            # Limpiar el último separador
            print(sep)

    print("└" + ("────┴" * (cols - 1)) + "────┘")

    # Leyenda de regiones
    print("\n  Leyenda de regiones:")
    for i, reg in enumerate(regiones):
        # Encontrar una celda de esta región para mostrar el valor
        celdas = [pos for pos, r in asignacion.items() if r == reg]
        r0, c0 = celdas[0]
        valor = tablero[r0][c0]
        color = color_reg[reg]
        print(f"    {color}{NEGRITA} Reg {reg:2d} {RESET}  {len(celdas)} celdas (valor={valor})")

    print()


def mostrar_tablero_original(tablero: list[list[int]]) -> None:
    filas = len(tablero)
    cols = len(tablero[0])
    print("\n  Tablero original:")
    print("  " + "┌" + ("────┬" * (cols - 1)) + "────┐")
    for r, fila in enumerate(tablero):
        print("  │" + "│".join(f" {v:2d} " for v in fila) + "│")
        if r < filas - 1:
            print("  ├" + ("────┼" * (cols - 1)) + "────┤")
    print("  └" + ("────┴" * (cols - 1)) + "────┘\n")


#  Validación básica del tablero


def validar_tablero(tablero: list[list[int]]) -> list[str]:
    """Devuelve lista de errores encontrados (vacía = válido)."""
    errores = []
    if not tablero:
        errores.append("El tablero está vacío.")
        return errores

    cols = len(tablero[0])
    for i, fila in enumerate(tablero):
        if len(fila) != cols:
            errores.append(f"La fila {i} tiene {len(fila)} columnas, se esperan {cols}.")
        for j, v in enumerate(fila):
            if v < 1:
                errores.append(f"Valor inválido en ({i},{j}): {v} (debe ser ≥ 1).")

    # La suma de los valores debe ser divisible por sí misma (pista de consistencia)
    filas = len(tablero)
    total = filas * cols
    suma = sum(v for fila in tablero for v in fila)
    if suma % total != 0 and suma != total:
        # Verificación relajada: simplemente advertimos
        pass

    return errores



#  Punto de entrada

def main():
    if len(sys.argv) < 2:
        print("Uso: python chikaku_solver.py <archivo.txt>")
        print("\nEjemplo de archivo puzzle.txt:")
        print("  2 3 3 2 2")
        print("  2 3 3 2 2")
        print("  3 3 3 2 2")
        print("  3 3 3 4 4")
        print("  1 3 3 4 4")
        sys.exit(1)

    ruta = sys.argv[1]

    try:
        tablero = leer_tablero(ruta)
    except FileNotFoundError:
        print(f" Archivo no encontrado: {ruta}")
        sys.exit(1)
    except ValueError as e:
        print(f" Error al leer el archivo: {e}")
        sys.exit(1)

    print(f"\n Tablero cargado: {len(tablero)} filas × {len(tablero[0])} columnas")

    errores = validar_tablero(tablero)
    if errores:
        print(" Errores en el tablero:")
        for e in errores:
            print(f"   • {e}")
        sys.exit(1)

    mostrar_tablero_original(tablero)

    print("⏳ Resolviendo con memoización...")
    import time
    t0 = time.time()
    solucion = resolver(tablero)
    t1 = time.time()

    if solucion is None:
        print(" No se encontró solución para este puzzle.")
        sys.exit(1)

    mostrar_solucion(tablero, solucion)
    print(f" Resuelto en {(t1 - t0)*1000:.1f} ms\n")


if __name__ == "__main__":
    main()
