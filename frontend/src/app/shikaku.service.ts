import { Injectable } from '@angular/core';

export interface RegionInfo {
  id: number;
  value: number;
  cells: number;
  r0: number;
  c0: number;
  r1: number;
  c1: number;
}

export interface SolveResult {
  rows: number;
  cols: number;
  cells: number[];
  regions: RegionInfo[];
  time_ms: number;
  error?: string;
}

@Injectable({ providedIn: 'root' })
export class ShikakuService {
  async solve(board: number[][]): Promise<SolveResult> {
    const res = await fetch('/api/solve', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ board }),
    });
    if (!res.ok) {
      const err = await res.json().catch(() => ({ error: res.statusText }));
      throw new Error(err.error || 'Error del servidor');
    }
    return res.json();
  }
}
