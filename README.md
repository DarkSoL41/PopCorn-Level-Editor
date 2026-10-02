# POPCORN Level Editor 1.0

Level editor for **POPCORN** (LACRAL software, 1988) — the DOS Arkanoid-style game.

Author: **DarkSoL** (Discord: `darksol41`). Made with the support of Claude AI.

---

## What it is

A standalone Windows fan-made program that edits the 49 levels of POPCORN. It works with
the `.PPC` level banks of the original POPGEN editor **and** reads levels straight
out of `POPCORN.EXE`, writing them back into the game — so you can build a level
and actually play it.

- Native Win32, statically linked: no .NET, no Python, no DLLs, no installer.
  One `PopCornEditor.exe`, runs on Windows 10/11 as is.
- Graphics mode draws the field with the **real block sprites taken from the game**.
- Interface in **English and Russian**, switchable at runtime (English by default).

**The original POPCORN game was created by Frédérick Raynal**.
He maintains a website with ongoing development of new PopCorn games:
👉 **[https://ludoid.fr/popcorn2026](https://ludoid.fr/popcorn2026)** — check it out for news and upcoming releases!

## Quick start

1. Run `PopCornEditor.exe`.
2. `File → Import levels from POPCORN.EXE...` and pick the game's `POPCORN.EXE`.
3. Draw your levels.
4. `File → Export (patch) to POPCORN.EXE...` — pick the same `POPCORN.EXE`, then
   choose a name for the patched copy (the original is left untouched).
5. Run the patched copy and play your levels.

You can also drop a `.PPC` file or `POPCORN.EXE` onto the editor's icon, or pass it
on the command line: `PopCornEditor.exe POPCORN.EXE`.

## Mini manual

**Drawing.** Pick an object in the left palette (or press `F1`..`F10`), then
**hold the left mouse button** and drag to draw blocks like a pencil.
**Hold the right mouse button** to erase the same way. `Del` selects the eraser.

**Objects.** F1–F8 are the destructible bricks, F3 is an *indestructible* block,
F9 is a teleport (max 6 per level), F10 is the "picture" — a 2×3 block; click the
cell where its **top-left** corner should go.

**Levels.** `PgUp` / `PgDn` or the `< Prev.` / `Next >` buttons switch levels;
type a number and press `Go` to jump. `Level → Copy field to...` duplicates the
current field into another slot. `Clear field` empties it.

**View.** `View → Graphics mode` shows the real game sprites (this is the default);
`Schematic mode` shows plain cells with numeric codes — handy when you care about
exact byte values.

**Raw code.** The `Raw code (0-255)` box places any byte value you type. The
original 49 levels only use the documented codes, so this is for experiments.

**Files.** `.PPC` is the format of the original POPGEN editor — use Open/Save for it.
Import/Export from `.exe` is what gets levels into the game itself.

## Good to know

- The level record header (brick count, teleport positions) is **recalculated
  automatically** on export. You never edit it by hand — and if it were wrong, the
  level would either be impossible to finish or end too early.
- When patching, the `.exe` keeps exactly the same size. That is mandatory: the
  game is packed with Microsoft EXEPACK, and any size drift makes it print
  *"Packed file is corrupt"*. Before saving, the editor unpacks its own output and
  compares it byte for byte; if the check fails, nothing is written.
- If a field is very "noisy" the packed data may not fit the space reserved inside
  the `.exe`. The editor says so instead of producing a broken game; simplify the
  field (longer runs of identical cells pack better).

## Level format, in short

The full write-up with the reverse-engineering evidence is in `FORMAT_SOLVED.md`.

- `.PPC` file: 6-byte `"LACRAL"` signature + 49 records × 176 bytes = 8630 bytes.
- Inside `POPCORN.EXE`: the same bank with a doubled signature
  (`"LACRALLACRAL"`, 12 bytes), packed with EXEPACK together with the program.
- One record (176 bytes): byte 0 — number of destructible bricks; byte 1 — number
  of teleports; bytes 2..7 — their grid positions; bytes 8..175 — the 14×12 grid,
  one byte per cell.
- Cell codes: `0` empty; `1,2,5,6,7,8` bricks F1,F2,F5,F6,F7,F8; `10` brick F4;
  `3` indestructible block F3; `9` teleport F9; `16..21` picture block F10.

## Files in this folder

| File | What it is |
|---|---|
| `PopCornEditor.exe` | The editor. This is all you need to run. |
| `main.cpp`, `lang.h`, `ppcformat.h`, `cellcodes.h`, `exeformat.h`, `sprites.h` | Sources. `exeformat.h` is the EXEPACK codec, `sprites.h` the extracted sprites, `lang.h` the EN/RU strings. |
| `FORMAT_SOLVED.md` | Full description of the format and how it was cracked. |

Rebuild (MinGW-w64):

```
x86_64-w64-mingw32-g++ -std=c++17 -O2 -static -municode -o PopCornEditor.exe main.cpp -lcomdlg32 -lcomctl32 -mwindows
```

## Legal

This is an unofficial, free, non-commercial fan tool. It is not affiliated with or
endorsed by Frédérick Raynal or LACRAL software.

*POPCORN* © 1988 Frédérick Raynal / LACRAL software. The game, its name, graphics and
levels belong to their author.

**What is in this package.** The game itself is **not** included: you need your own
copy of `POPCORN.EXE`, and the levels are read from it. The only material from the
game inside the editor is the set of small block sprites (`sprites.h`), captured from
the game screen and used solely to draw the field in graphics mode.

No warranty of any kind. Keep a backup of your original `POPCORN.EXE`.
If you are a rights holder and want something changed or removed, contact me on Discord (`darksol41`) and I will do it.
