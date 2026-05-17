import { Component, signal, HostListener, OnDestroy } from '@angular/core';
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
import { MatTooltipModule } from '@angular/material/tooltip';
import { ShikakuService, SolveResult } from './shikaku.service';

type Mode = 'input' | 'jugando' | 'solucionado';

// Colores VARIADOS y VIBRANTES para el tablero (Shikaku estándar)
const COLORES_TABLERO = [
  '#FF6B6B', '#4ECDC4', '#45B7D1', '#FFA07A', '#98D8C8',
  '#F7DC6F', '#BB8FCE', '#82E0AA', '#F1948A', '#85C1E9',
  '#F8C471', '#7DCEA0', '#D2B4DE', '#AED6F1', '#F5B7B1',
  '#A9DFBF', '#F9E79F', '#EBDEF0', '#D6EAF8', '#E6B0AA',
  '#e74c3c', '#2ecc71', '#f39c12', '#3498db', '#9b59b6'
];

interface CeldaJuego {
  valor: number;
  regionId: number | null;
  enRectActual: boolean;
}

interface LeaderboardEntry {
  name: string;
  size: string;
  time: number;
  date: string;
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
    MatSnackBarModule,
    MatTooltipModule
  ],
  templateUrl: './app.html',
  styleUrl: './app.css',
})
export class App implements OnDestroy {
  protected readonly DEFAULT_BOARD = '4 0 3 0 0\n0 0 6 0 0\n2 0 0 0 0\n4 0 6 0 0\n0 0 0 0 0';

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
  
  protected readonly playerName = signal(localStorage.getItem('shikaku_player') || 'Invitado');
  protected readonly timerValue = signal(0);
  protected readonly leaderboard = signal<LeaderboardEntry[]>([]);
  protected readonly showCompletionDialog = signal(false);
  protected readonly completionTime = signal(0);
  protected readonly usedHint = signal(false);

  private nextRegionId = 0;
  private timerInterval: any;

  constructor(private api: ShikakuService, private snackBar: MatSnackBar) {
    this.loadLeaderboard();
  }

  ngOnDestroy() { this.stopTimer(); }

  protected onInput(e: Event) { this.boardInput.set((e.target as HTMLTextAreaElement).value); }

  protected onNameInput(e: Event) {
    const val = (e.target as HTMLInputElement).value;
    this.playerName.set(val);
    localStorage.setItem('shikaku_player', val);
  }

  protected onFileUpload(e: Event) {
    const input = e.target as HTMLInputElement;
    const file = input.files?.[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () => {
      this.boardInput.set(reader.result as string);
      this.mode.set('input');
    };
    reader.readAsText(file);
  }

  // Generador Aleatorio de Tableros Shikaku (Usando C++)
  protected async generarTableroAleatorio() {
    this.loading.set(true);
    try {
      const size = Math.random() < 0.5 ? 5 : (Math.random() < 0.5 ? 6 : 7);
      const res = await this.api.generate(size);
      this.boardInput.set(res.board_str);
      this.snackBar.open('Nuevo tablero generado', 'OK', { duration: 2000 });
    } catch (err) {
      this.snackBar.open('Error al generar el tablero', 'OK');
    } finally {
      this.loading.set(false);
    }
  }

  private parseBoard(): number[][] | null {
    const text = this.boardInput().trim();
    if (!text) {
      this.snackBar.open('El tablero está vacío', 'OK', { duration: 3000 });
      return null;
    }
    const lines = text.split('\n').map(l => l.trim()).filter(l => l.length > 0);
    try {
      const board = lines.map(line => line.split(/\s+/).map(Number));
      if (board.length === 0 || board[0].length === 0) throw new Error('Tablero vacío');
      const cols = board[0].length;
      
      let totalSum = 0;
      let hasClues = false;

      for (const row of board) {
        if (row.length !== cols) throw new Error('Las filas tienen longitudes inconsistentes');
        for (const v of row) {
          if (isNaN(v) || v < 0) throw new Error('Contiene caracteres no válidos o negativos');
          if (v > 0) {
            hasClues = true;
            totalSum += v;
          }
        }
      }

      if (!hasClues) throw new Error('El tablero no tiene ninguna pista');
      
      // Validación matemática fundamental: La suma de las pistas debe ser igual al área total
      const totalArea = board.length * cols;
      if (totalSum !== totalArea) {
        throw new Error(`Invalido: El área total es ${totalArea}, pero la suma de las pistas es ${totalSum}`);
      }

      return board;
    } catch (err: any) {
      this.snackBar.open(err.message, 'OK', { duration: 4000 });
      return null;
    }
  }

  protected async solveAI() {
    const board = this.parseBoard();
    if (!board) return; // Se detiene aquí si es inválido
    this.originalBoard.set(board);
    this.loading.set(true);
    this.stopTimer();
    try {
      const res = await this.api.solve(board);
      if (res.error) throw new Error(res.error);
      this.result.set(res);
      this.mode.set('solucionado');
    } catch (err: any) {
      this.snackBar.open('Tablero sin solución o ' + err.message, 'OK');
    } finally {
      this.loading.set(false);
    }
  }

  protected empezarJuego() {
    const board = this.parseBoard();
    if (!board) return; // Se detiene aquí si es inválido
    this.originalBoard.set(board);
    this.nextRegionId = 0;
    this.celdas.set(board.map(row => row.map(val => ({ valor: val, regionId: null, enRectActual: false }))));
    this.mode.set('jugando');
    this.startTimer();
    this.usedHint.set(false);
    this.showCompletionDialog.set(false);
  }

  private startTimer() {
    this.stopTimer();
    this.timerValue.set(0);
    this.timerInterval = setInterval(() => this.timerValue.update(v => v + 1), 1000);
  }

  private stopTimer() { if (this.timerInterval) { clearInterval(this.timerInterval); this.timerInterval = null; } }

  protected formatTime(seconds: number): string {
    if (seconds == null || isNaN(seconds) || seconds < 0) return '0:00';
    const mins = Math.floor(seconds / 60), secs = seconds % 60;
    return `${mins}:${secs.toString().padStart(2, '0')}`;
  }

  protected formatPreciseTime(us: number): string {
    if (us == null || isNaN(us) || us < 0) return '0 ms';
    if (us < 1000) return `${us} μs`;
    if (us < 1000000) return `${(us / 1000).toFixed(2)} ms`;
    return `${(us / 1000000).toFixed(2)} s`;
  }

  protected onMouseDown(r: number, c: number) {
    const grid = this.celdas();
    const currentRegion = grid[r][c].regionId;
    if (currentRegion !== null) {
      // Deseleccionar región existente
      const newGrid = grid.map(row => row.map(cell => 
        cell.regionId === currentRegion ? { ...cell, regionId: null } : { ...cell }
      ));
      this.celdas.set(newGrid);
      this.mensajeJuego.set('Región eliminada.');
      return;
    }
    
    this.dragInicio.set({ r, c });
    this.dragFin.set({ r, c });
    this.actualizarPrevisual();
  }

  protected onMouseEnter(r: number, c: number) {
    if (this.dragInicio()) { this.dragFin.set({ r, c }); this.actualizarPrevisual(); }
  }

  @HostListener('window:mouseup')
  protected onMouseUp() {
    const inicio = this.dragInicio(), fin = this.dragFin();
    if (!inicio || !fin) { this.dragInicio.set(null); return; }
    const r0 = Math.min(inicio.r, fin.r), c0 = Math.min(inicio.c, fin.c);
    const r1 = Math.max(inicio.r, fin.r), c1 = Math.max(inicio.c, fin.c);
    const grid = this.celdas().map(row => row.map(cell => ({...cell, enRectActual: false})));
    const area = (r1 - r0 + 1) * (c1 - c0 + 1);
    const pistas: any[] = [];
    for (let r = r0; r <= r1; r++)
      for (let c = c0; c <= c1; c++)
        if (grid[r][c].valor > 0) pistas.push(grid[r][c].valor);

    if (pistas.length === 1 && pistas[0] === area) {
      const id = this.nextRegionId++;
      for (let rr = r0; rr <= r1; rr++) for (let cc = c0; cc <= c1; cc++) grid[rr][cc].regionId = id;
      this.celdas.set(grid);
      this.verificarCompleto(grid);
    }
    this.dragInicio.set(null);
  }

  private actualizarPrevisual() {
    const i = this.dragInicio(), f = this.dragFin();
    if (!i || !f) return;
    const r0 = Math.min(i.r, f.r), c0 = Math.min(i.c, f.c), r1 = Math.max(i.r, f.r), c1 = Math.max(i.c, f.c);
    this.celdas.set(this.celdas().map((row, r) => row.map((cell, c) => ({...cell, enRectActual: r >= r0 && r <= r1 && c >= c0 && c <= c1}))));
  }

  private verificarCompleto(grid: CeldaJuego[][]) {
    if (grid.every(row => row.every(cel => cel.regionId !== null))) {
      this.stopTimer();
      this.completionTime.set(this.timerValue());
      this.showCompletionDialog.set(true);
      if (!this.usedHint()) this.addToLeaderboard(grid.length, grid[0].length, this.timerValue());
    }
  }

  protected async showSolution() {
    this.usedHint.set(true); this.stopTimer();
    this.loading.set(true);
    try {
      const res = await this.api.solve(this.originalBoard());
      this.result.set(res);
      this.mode.set('solucionado');
    } catch (err) { this.snackBar.open('Error al generar la solución', 'OK'); } 
    finally { this.loading.set(false); }
  }

  private loadLeaderboard() {
    const data = localStorage.getItem('shikaku_leaderboard');
    if (data) this.leaderboard.set(JSON.parse(data));
  }

  protected clearLeaderboard() {
    this.leaderboard.set([]);
    localStorage.removeItem('shikaku_leaderboard');
    this.snackBar.open('Leaderboard limpia', 'OK');
  }

  private addToLeaderboard(rows: number, cols: number, time: number) {
    const entry: LeaderboardEntry = { name: this.playerName(), size: `${rows}x${cols}`, time, date: new Date().toLocaleDateString() };
    const current = [...this.leaderboard(), entry].sort((a, b) => a.time - b.time).slice(0, 10);
    this.leaderboard.set(current);
    localStorage.setItem('shikaku_leaderboard', JSON.stringify(current));
  }

  protected colorFor(id: number | null) { 
    return id === null ? 'transparent' : COLORES_TABLERO[id % COLORES_TABLERO.length]; 
  }

  protected cellStyle(cel: CeldaJuego) {
    if (cel.regionId !== null) return { 'background-color': this.colorFor(cel.regionId) };
    if (cel.enRectActual) return { 'background-color': 'rgba(44, 44, 94, 0.15)' };
    return {};
  }

  protected cellBorders(grid: any, r: number, c: number, id: any, rows?: any, cols?: any, flat?: any) {
    const cls = ['cell'];
    if (id === null) return cls.join(' ');
    if (grid) {
      const R = grid.length, C = grid[0].length;
      if (c === 0 || grid[r][c-1].regionId !== id) cls.push('bl');
      if (r === 0 || grid[r-1][c].regionId !== id) cls.push('bt');
      if (c === C-1 || grid[r][c+1].regionId !== id) cls.push('br');
      if (r === R-1 || grid[r+1][c].regionId !== id) cls.push('bb');
    } else if (flat && rows && cols) {
      if (c === 0 || flat[r*cols + c-1] !== id) cls.push('bl');
      if (r === 0 || flat[(r-1)*cols + c] !== id) cls.push('bt');
      if (c === cols-1 || flat[r*cols + c+1] !== id) cls.push('br');
      if (r === rows-1 || flat[(r+1)*cols + c] !== id) cls.push('bb');
    }
    return cls.join(' ');
  }
}
