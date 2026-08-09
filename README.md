# POPCORN Level Editor 1.0

Level editor for **POPCORN** (LACRAL software, 1988) — the DOS Arkanoid-style game.

Author: **DarkSoL** (Discord: `darksol41`). Made with the support of Claude AI.

*Русская версия — ниже, после английской.*

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
| `popcorn_levels.py` | The same codec as a Python script (`dump` / `toppc` / `frompc` / `test`) for batch work and verification. |
| `POPCORN_levels.PPC` | The original 49 levels, exported from the game. |
| `FORMAT_SOLVED.md` | Full description of the format and how it was cracked. |

Rebuild (MinGW-w64):

```
x86_64-w64-mingw32-g++ -std=c++17 -O2 -static -municode -o PopCornEditor.exe main.cpp -lcomdlg32 -lcomctl32 -mwindows
```

<sup>If you’d like to support me financially, BTC address: bc1qp476rmcaapl6n6xjvg2la50cfw3kwvxe8sj0m5

---
---

# POPCORN Level Editor 1.0 (по-русски)

Редактор уровней для **POPCORN** (LACRAL software, 1988) — DOS-игры в духе Arkanoid.

Автор: **DarkSoL** (Discord: `darksol41`). Сделано с помощью поддержки ИИ Claude.

## Что это

Отдельная Windows-программа для правки 49 уровней POPCORN. Работает с банками
`.PPC` родного редактора POPGEN **и** читает уровни прямо из `POPCORN.EXE`,
записывая их обратно в игру — то есть свой уровень можно сразу же пройти.

- Нативный Win32 со статической сборкой: без .NET, Python, DLL и установщика.
  Один `PopCornEditor.exe`, запускается на Windows 10/11 как есть.
- В графическом режиме поле рисуется **настоящими спрайтами блоков из игры**.
- Интерфейс на **английском и русском**, переключается на ходу
  (по умолчанию английский).

**Оригинальная игра POPCORN создана Фредериком Рейналем** (Frédérick Raynal).
У него есть сайт, где ведётся разработка новых игр PopCorn:
👉 **[https://ludoid.fr/popcorn2026](https://ludoid.fr/popcorn2026)** — заходите следить за новостями!

## Быстрый старт

1. Запустить `PopCornEditor.exe`.
2. `Файл → Импорт уровней из POPCORN.EXE...` и выбрать игровой `POPCORN.EXE`.
3. Нарисовать уровни.
4. `Файл → Экспорт (патч) в POPCORN.EXE...` — выбрать тот же `POPCORN.EXE`, затем
   указать имя для пропатченной копии (оригинал не меняется).
5. Запустить пропатченную копию и играть.

Можно также перетащить `.PPC` или `POPCORN.EXE` на значок редактора либо передать
путь в командной строке: `PopCornEditor.exe POPCORN.EXE`.

## Мини-мануал

**Рисование.** Выбрать объект в палитре слева (или нажать `F1`..`F10`), затем
**зажать левую кнопку мыши** и вести — блоки рисуются как карандашом.
**Зажатая правая кнопка** так же стирает. `Del` выбирает ластик.

**Объекты.** F1–F8 — разрушаемые кирпичи, F3 — *неразрушаемый* блок,
F9 — телепорт (не больше 6 на уровень), F10 — «картинка», блок 2×3: кликать надо
по клетке, где будет её **левый верхний** угол.

**Уровни.** `PgUp` / `PgDn` или кнопки `< Пред.` / `След. >` листают уровни;
можно ввести номер и нажать `Перейти`. `Уровень → Копировать поле в...` дублирует
текущее поле в другой слот. `Очистить поле` — стереть всё.

**Вид.** `Вид → Графический режим` — настоящие спрайты игры (так по умолчанию);
`Схематический режим` — простые клетки с числовыми кодами, удобно когда важны
точные значения байтов.

**Raw code.** Поле `Raw code (0-255)` ставит любое введённое значение байта.
В оригинальных 49 уровнях встречаются только описанные коды, так что это для
экспериментов.

**Файлы.** `.PPC` — формат родного редактора POPGEN, для него Открыть/Сохранить.
Импорт/экспорт `.exe` — это то, что переносит уровни в саму игру.

## Что полезно знать

- Заголовок записи уровня (число кирпичей, позиции телепортов) **пересчитывается
  автоматически** при экспорте. Руками его править не нужно — а будь он неверным,
  уровень было бы либо невозможно пройти, либо он завершался бы раньше времени.
- При патче размер `.exe` остаётся ровно тем же. Это обязательное условие: игра
  запакована Microsoft EXEPACK, и любое отклонение размера приводит к сообщению
  *«Packed file is corrupt»*. Перед сохранением редактор распаковывает свой же
  результат и сверяет побайтово; если проверка не прошла, файл не пишется.
- Если поле очень «пёстрое», упакованные данные могут не влезть в отведённое место
  внутри `.exe`. Редактор об этом скажет, а не выдаст сломанную игру — упростите
  поле (длинные ряды одинаковых клеток пакуются лучше).

## Формат уровней, кратко

Полное описание с доказательствами — в `FORMAT_SOLVED.md`.

- Файл `.PPC`: 6 байт подписи `"LACRAL"` + 49 записей по 176 байт = 8630 байт.
- Внутри `POPCORN.EXE`: тот же банк с удвоенной подписью (`"LACRALLACRAL"`,
  12 байт), запакованный EXEPACK вместе с программой.
- Одна запись (176 байт): байт 0 — число разрушаемых кирпичей; байт 1 — число
  телепортов; байты 2..7 — их позиции в сетке; байты 8..175 — сетка 14×12,
  по байту на клетку.
- Коды клеток: `0` пусто; `1,2,5,6,7,8` кирпичи F1,F2,F5,F6,F7,F8; `10` кирпич F4;
  `3` неразрушаемый блок F3; `9` телепорт F9; `16..21` блок «картинка» F10.

## Файлы в этой папке

| Файл | Что это |
|---|---|
| `PopCornEditor.exe` | Сам редактор. Для запуска больше ничего не нужно. |
| `main.cpp`, `lang.h`, `ppcformat.h`, `cellcodes.h`, `exeformat.h`, `sprites.h` | Исходники. `exeformat.h` — кодек EXEPACK, `sprites.h` — извлечённые спрайты, `lang.h` — строки EN/RU. |
| `popcorn_levels.py` | Тот же кодек отдельным скриптом на Python (`dump` / `toppc` / `frompc` / `test`) — для пакетных операций и проверок. |
| `POPCORN_levels.PPC` | Оригинальные 49 уровней, выгруженные из игры. |
| `FORMAT_SOLVED.md` | Полное описание формата и как он был раскрыт. |

Пересборка (MinGW-w64):

```
x86_64-w64-mingw32-g++ -std=c++17 -O2 -static -municode -o PopCornEditor.exe main.cpp -lcomdlg32 -lcomctl32 -mwindows
```

<sup>If you’d like to support me financially, BTC address: bc1qp476rmcaapl6n6xjvg2la50cfw3kwvxe8sj0m5
