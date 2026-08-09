# POP-CORN — level storage format

Technical reference for the level data of **POP-CORN** (LACRAL software, 1988),
covering both the `.PPC` bank file of the original POPGEN editor and the copy
embedded inside `POPCORN.EXE`.

Everything below was verified against the game itself: the decoder reproduces the
game's own decompressed data byte for byte, and a re-encoded `POPCORN.EXE` runs and
displays the edited levels.

---

## 1. POPCORN.EXE is packed with Microsoft EXEPACK

This single fact explains the whole structure and is the reason a plain static
analysis of the file goes nowhere.

Evidence inside the file:

| What | Where |
|---|---|
| `"RB"` signature (`0x5242`) | 2 bytes before the entry point, at file offset `0x1943E` |
| 18-byte EXEPACK header | immediately before the entry point |
| `"Packed file is corrupt"` | file offset `0x1952C` — the unpacker's error message |
| No relocation table, entry stub does `rep movsb` | MZ header + entry point |

Consequences:

- The game's real code does not exist in the file as such — the EXEPACK stub builds
  it in memory at run time (in this build it ends up in segment `0x1CAF`). Any
  disassembler will therefore show only the stub and the packed blob.
- **To analyse the game in IDA Pro** (or any disassembler): unpack EXEPACK first
  (`UNP`, `unexepack`, …), then open the result as an *MS-DOS executable*. Opening
  the packed file directly cannot work, no matter which loader options are chosen.
- The level bank is stored inside the packed image, so reading or writing levels
  means unpacking and repacking with the EXEPACK algorithm.

## 2. The EXEPACK compression format

EXEPACK processes its stream **from the end backwards**, which is why, read in
normal file order, the command byte appears at the *end* of each block and the
block's fields sit *before* it.

Two block types, in file order:

```
[value] [count_lo] [count_hi] 0xB0      -> emit byte [value], [count] times
[literal bytes ...] [N_lo] [N_hi] 0xB2  -> emit those N bytes verbatim
```

The low bit of the command byte (`0xB1` / `0xB3`) marks the last command of the
whole packed image; the level-bank region contains no such block.

Decoding forward is straightforward: scan for the next byte equal to `0xB0` or
`0xB2`; the two bytes just before it are a 16-bit little-endian count; for `0xB0`
the byte before the count is the value to repeat; for `0xB2` the `count` bytes
before the count field are the literals. Scanning is unambiguous because level data
never contains a value `>= 0xB0` (cell codes are `0..21`, brick counts `<= 168`).

## 3. Where the bank sits in POPCORN.EXE

In the original 103848-byte `POPCORN.EXE`:

| Item | Offset |
|---|---|
| `"LACRALLACRAL"` signature (stored uncompressed) | `0x7B5E` |
| Byte 0 of level 1's record (also uncompressed) | `0x7B6A` |
| Start of the packed blocks | `0x7B6E` |
| End of the packed blocks (exclusive) | `0x95E6` |

The signature and that one byte are uncompressed because they happen to fall at the
end of a large literal block that also covers preceding graphics data.

The 6776 packed bytes from `0x7B6E` expand to 8623 bytes, which together with the
signature and that single byte make up the complete 8636-byte bank image.

Rather than hard-coding these offsets, a tool should locate `"LACRALLACRAL"` and
derive the rest from it (`sig + 12` is the uncompressed byte, `sig + 16` the start of
the blocks); the end offset falls out of decoding the expected number of bytes.

## 4. The decompressed image

```
"LACRALLACRAL"      12 bytes   (a .PPC file has a single "LACRAL", 6 bytes)
level 1 record     176 bytes
...
level 49 record    176 bytes
                  ----------
total             8636 bytes   (a .PPC file: 8630 bytes)
```

Apart from the doubled signature this is exactly the `.PPC` layout, so converting
between the two is a matter of rewriting the signature.

## 5. Level record — 176 bytes

| Bytes | Meaning |
|---|---|
| 0 | number of destructible bricks on the field (the win condition) |
| 1 | number of teleports, `0..6` |
| 2..7 | grid indices (`0..167`) of those teleports; unused tail is padding |
| 8..175 | the grid: 14 rows × 12 columns, row-major, one byte per cell |

Both header fields are **derived from the grid**, confirmed across all 49 original
levels with no exceptions:

- byte 0 = number of cells whose code is in `{1, 2, 5, 6, 7, 8, 10}` ∪ `{16..21}`
- byte 1 and bytes 2..7 = the count and the positions of cells with code `9`

An editor should therefore recompute the header from the grid rather than preserve
it. A stale brick count makes the level either impossible to complete or finish
early.

Minor detail: in level 15 the unused tail of the header holds leftover values
(`162, 163` while the teleport count is 4). The game does not read those bytes.

## 6. Cell codes

| Code | Meaning |
|---|---|
| 0 | empty |
| 1, 2, 5, 6, 7, 8 | destructible bricks F1, F2, F5, F6, F7, F8 |
| 10 | destructible brick F4 |
| 3 | **indestructible** block F3 — not counted as a brick |
| 9 | teleport F9 — single cell, at most 6 per level |
| 16..21 | "picture" block F10 — 2 columns × 3 rows, row-major; destructible |

This is the complete set: the original 49 levels contain no other codes, and the
codes are identical to those used by the `.PPC` format of the POPGEN editor.

Levels containing an F10 picture block: **8, 10, 21, 24, 31, 35, 40, 43, 46**.

## 7. Patching POPCORN.EXE

Two rules matter.

**The re-encoded region must occupy exactly the same number of bytes.** Filler bytes
are not an option: EXEPACK reads the stream backwards, so any stray byte would be
taken for a command code and the game would print *"Packed file is corrupt"*. The
way to hit an exact size is to split blocks into several valid blocks — a literal
block split adds 3 bytes, a run split adds 4, and converting a short run into a
literal block adds `length - 1`, which together cover every required delta.

**There is room to work with.** An optimal encoding of the unmodified bank takes
6587 bytes out of the 6776 available, so roughly 189 bytes of slack. Very "noisy"
fields compress worse; a tool should detect an overflow and refuse rather than write
a broken executable.

Recommended procedure: unpack, edit, recompute the record headers, re-encode to the
exact original size, write the copy, then **unpack the result again and compare it
byte for byte** with what was intended before keeping the file.

## 8. How this was verified

- Decoding the packed region reproduces all 8623 bytes of the bank exactly as the
  game itself has them in memory at run time — zero mismatches.
- Header derivation was checked on all 49 levels: brick counts, teleport counts and
  teleport positions all match the values the game stores.
- A round trip (unpack → recompute headers → repack) reproduces the original image,
  and the file size stays unchanged.
- A patched `POPCORN.EXE` was run under DOSBox and displayed the edited level, so the
  in-game unpacker accepts the re-encoded stream.
- The two independent implementations of the codec — `popcorn_levels.py` (Python)
  and `exeformat.h` (C++, used by the editor) — produce identical output, both for
  an unmodified bank and for a modified one.
