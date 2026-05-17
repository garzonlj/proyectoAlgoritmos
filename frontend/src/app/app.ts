import { Component, signal, HostListener } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatInputModule } from '@angular/material/input';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatToolbarModule } from '@angular/material/toolbar';
import { MatIconModule } from '@angular/material/icon';
import { MatProgressSpinnerModule } from '@angular/material/progress-spinner';
import { MatSnackBar, MatSnackBarModule } from '@angular/material/snack-bar';
import { ShikakuService, SolveResult } from './shikaku.service';

type Mode = 'input' | 'jugando' | 'solucionado';

const COLORES = [
  '#FF6B6B', '#4ECDC4', '#45B7D1', '#FFA07A', '#98D8C8',
  '#F7DC6F', '#BB8FCE', '#82E0AA', '#F1948A', '#85C1E9',
  '#F8C471', '#7DCEA0', '#D2B4DE', '#AED6F1', '#F5B7B1',
  '#A9DFBF', '#F9E79F', '#EBDEF0', '#D6EAF8', '#E6B0AA'
];

interface CeldaJuego {
  valor: number;
  regionId: number | null;
  enRectActual: boolean;
}

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [
    CommonModule,
    FormsModule,
    MatButtonModule,
    MatCardModule,
    MatInputModule,
    MatFormFieldModule,
    MatToolbarModule,
    MatIconModule,
    MatProgressSpinnerModule,
    MatSnackBarModule
  ],
  templateUrl: './app.html',
  styleUrl: './app.css',
})
export class App {
  protected readonly DEFAULT_BOARD =
    '4 0 3 0 0\n0 0 6 0 0\n2 0 0 0 0\n4 0 6 0 0\n0 0 0 0 0';

  protected readonly boardInput = signal(this.DEFAULT_BOARD);
  protected readonly originalBoard = signal<number[][]>([]);
  protected readonly mode = signal<Mode>('input');
  protected readonly result = signal<SolveResult | null>(null);
  protected readonly error = signal<string | null>(null);
  protected readonly loading = signal(false);
  protected readonly celdas = signal<CeldaJuego[][]>([]);
  protected readonly dragInicio = signal<{ r: number; c: number } | null>(null);
  protected readonly dragFin = signal<{ r: number; c: number } | null>(null);
  protected readonly mensajeJuego = signal('Carga un tablero para comenzar');

  private nextRegionId = 0;

  constructor(private api: ShikakuService, private snackBar: MatSnackBar) {}

  protected onInput(e: Event) {
    this.boardInput.set((e.target as HTMLTextAreaElement).value);
  }

  protected onFileUpload(e: Event) {
    const input = e.target as HTMLInputElement;
    const file = input.files?.[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () => {
      this.boardInput.set(reader.result as string);
      this.mode.set('input');
      this.error.set(null);
      this.snackBar.open('Tablero cargado con éxito', 'OK', { duration: 2000 });
    };
    reader.readAsText(file);
  }

  private parseBoard(): number[][] | null {
    const lines = this.boardInput()
      .split('\n')
      .map(l => l.trim())
      .filter(l => l.length > 0);
    try {
      const board = lines.map(line => line.split(/\s+/).map(Number));
      if (board.length === 0 || board[0].length === 0)
        throw new Error('Tablero vacío');
      const cols = board[0].length;
      for (const row of board) {
        if (row.length !== cols)
          throw new Error('Las filas deben tener el mismo número de columnas');
        for (const v of row)
          if (isNaN(v) || v < 0) throw new Error('Los valores deben ser números >= 0');
      }
      return board;
    } catch (err: any) {
      this.error.set(err.message);
      this.snackBar.open(err.message, 'Cerrar', { duration: 3000 });
      return null;
    }
  }

  protected async solveAI() {
    const board = this.parseBoard();
    if (!board) return;
    this.originalBoard.set(board);
    this.loading.set(true);
    this.error.set(null);
    this.result.set(null);
    try {
      const res = await this.api.solve(board);
      if (res.error) {
         throw new Error(res.error);
      }
      this.result.set(res);
      this.mode.set('solucionado');
    } catch (err: any) {
      this.error.set(err.message || 'Error al resolver');
      this.snackBar.open(err.message || 'Error del servidor', 'Cerrar');
    } finally {
      this.loading.set(false);
    }
  }

  protected empezarJuego() {
    const board = this.parseBoard();
    if (!board) return;
    this.originalBoard.set(board);
    this.nextRegionId = 0;
    const grid: CeldaJuego[][] = board.map(row => 
      row.map(val => ({ valor: val, regionId: null, enRectActual: false }))
    );
    this.celdas.set(grid);
    this.dragInicio.set(null);
    this.dragFin.set(null);
    this.mensajeJuego.set('Mantén presionado y arrastra para crear rectángulos.');
    this.mode.set('jugando');
  }

  // Interacción Click-and-Drag
  protected onMouseDown(r: number, c: number) {
    if (this.celdas()[r][c].regionId !== null) return;
    this.dragInicio.set({ r, c });
    this.dragFin.set({ r, c });
    this.actualizarPrevisual();
  }

  protected onMouseEnter(r: number, c: number) {
    if (this.dragInicio()) {
      this.dragFin.set({ r, c });
      this.actualizarPrevisual();
    }
  }

  @HostListener('window:mouseup')
  protected onMouseUp() {
    const inicio = this.dragInicio();
    const fin = this.dragFin();
    if (!inicio || !fin) {
      this.dragInicio.set(null);
      this.dragFin.set(null);
      return;
    }

    const r0 = Math.min(inicio.r, fin.r);
    const c0 = Math.min(inicio.c, fin.c);
    const r1 = Math.max(inicio.r, fin.r);
    const c1 = Math.max(inicio.c, fin.c);

    const grid = this.celdas().map(row => row.map(cell => ({...cell, enRectActual: false})));
    const val = this.validarRectangulo(grid, r0, c0, r1, c1);

    if (val.valido) {
      const id = this.nextRegionId++;
      for (let rr = r0; rr <= r1; rr++)
        for (let cc = c0; cc <= c1; cc++)
          grid[rr][cc].regionId = id;
      this.celdas.set(grid);
      this.mensajeJuego.set(`Rectángulo de área ${val.area} creado.`);
      this.verificarCompleto(grid);
    } else {
      this.mensajeJuego.set(`Inválido: ${val.razon}`);
      this.celdas.set(grid);
    }

    this.dragInicio.set(null);
    this.dragFin.set(null);
  }

  private actualizarPrevisual() {
    const inicio = this.dragInicio();
    const fin = this.dragFin();
    if (!inicio || !fin) return;

    const r0 = Math.min(inicio.r, fin.r);
    const c0 = Math.min(inicio.c, fin.c);
    const r1 = Math.max(inicio.r, fin.r);
    const c1 = Math.max(inicio.c, fin.c);

    const grid = this.celdas().map((row, r) => 
      row.map((cell, c) => ({
        ...cell,
        enRectActual: (r >= r0 && r <= r1 && c >= c0 && c <= c1)
      }))
    );
    this.celdas.set(grid);
  }

  private validarRectangulo(
    grid: CeldaJuego[][], r0: number, c0: number, r1: number, c1: number
  ): { valido: boolean; area?: number; valor?: number; razon?: string } {
    const alto = r1 - r0 + 1, ancho = c1 - c0 + 1, area = alto * ancho;
    
    // Buscar pistas dentro del rectángulo
    const pistas: {r: number, c: number, v: number}[] = [];
    for (let r = r0; r <= r1; r++) {
      for (let c = c0; c <= c1; c++) {
        if (grid[r][c].regionId !== null)
          return { valido: false, razon: 'Se solapa con otra región' };
        if (grid[r][c].valor > 0) {
          pistas.push({r, c, v: grid[r][c].valor});
        }
      }
    }

    if (pistas.length === 0)
      return { valido: false, razon: 'No contiene ninguna pista' };
    if (pistas.length > 1)
      return { valido: false, razon: 'Contiene más de una pista' };
    
    const valor = pistas[0].v;
    if (area !== valor)
      return { valido: false, razon: `El área (${area}) no coincide con la pista (${valor})` };

    return { valido: true, area, valor };
  }

  private verificarCompleto(grid: CeldaJuego[][]) {
    const completo = grid.every(row => row.every(cel => cel.regionId !== null));
    if (completo) {
      this.mensajeJuego.set('¡Felicidades! Has completado el puzzle.');
      this.snackBar.open('¡Puzzle completado!', 'Celebrar', { duration: 5000 });
    }
  }

  // Estilos y Visualización
  protected colorFor(regionId: number | null): string {
    return regionId === null ? 'transparent' : COLORES[regionId % COLORES.length];
  }

  protected cellBorders(
    grid: CeldaJuego[][] | null, r: number, c: number, regionId: number | null, 
    rows?: number, cols?: number, flatCells?: number[]
  ): string {
    const cls = ['cell'];
    if (regionId === null) return cls.join(' ');

    if (grid) {
      const R = grid.length, C = grid[0].length;
      if (c === 0 || grid[r][c - 1].regionId !== regionId) cls.push('bl');
      if (r === 0 || grid[r - 1][c].regionId !== regionId) cls.push('bt');
      if (c === C - 1 || grid[r][c + 1].regionId !== regionId) cls.push('br');
      if (r === R - 1 || grid[r + 1][c].regionId !== regionId) cls.push('bb');
    } else if (flatCells && rows !== undefined && cols !== undefined) {
      if (c === 0 || flatCells[r * cols + c - 1] !== regionId) cls.push('bl');
      if (r === 0 || flatCells[(r - 1) * cols + c] !== regionId) cls.push('bt');
      if (c === cols - 1 || flatCells[r * cols + c + 1] !== regionId) cls.push('br');
      if (r === rows - 1 || flatCells[(r + 1) * cols + c] !== regionId) cls.push('bb');
    }
    return cls.join(' ');
  }

  protected cellStyle(cel: CeldaJuego): Record<string, string> {
    const s: Record<string, string> = {};
    if (cel.regionId !== null) {
      s['background-color'] = this.colorFor(cel.regionId);
      s['color'] = 'rgba(0,0,0,0.7)';
    } else if (cel.enRectActual) {
      s['background-color'] = 'rgba(78, 205, 196, 0.2)';
    }
    return s;
  }

  protected totalRegions(grid: CeldaJuego[][]): number {
    return new Set(grid.flat().map(c => c.regionId).filter(v => v !== null)).size;
  }
  protected assignedCount(grid: CeldaJuego[][]): number {
    return grid.flat().filter(c => c.regionId !== null).length;
  }
}
