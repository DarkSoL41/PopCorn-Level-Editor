#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include "lang.h"

// POP-CORN (LACRAL software, 1988) .PPC level bank format.
// Reverse-engineered from POPGEN.EXE (disassembly) and confirmed empirically
// via round-trip tests through the original program under DOSBox.
//
//   offset 0      : 6 bytes ASCII signature "LACRAL"
//   offset 6      : 49 levels ("Tableaux"), 176 bytes each
//     level bytes 0..7    : 8-byte header (not fully decoded; preserved as-is)
//     level bytes 8..175  : 168-byte grid, 14 rows x 12 cols, row-major

namespace ppc {

constexpr int FILE_SIZE = 8630;
constexpr int NUM_LEVELS = 49;
constexpr int LEVEL_SIZE = 176;
constexpr int HEADER_SIZE = 8;
constexpr int ROWS = 14;
constexpr int COLS = 12;
constexpr int GRID_SIZE = ROWS * COLS; // 168
constexpr const char* SIGNATURE = "LACRAL";

struct Level {
    uint8_t header[HEADER_SIZE] = {0};
    uint8_t grid[GRID_SIZE] = {0};

    uint8_t Get(int row, int col) const { return grid[row * COLS + col]; }
    void Set(int row, int col, uint8_t v) { grid[row * COLS + col] = v; }
    void Clear() { memset(header, 0, HEADER_SIZE); memset(grid, 0, GRID_SIZE); }
};

struct Bank {
    Level levels[NUM_LEVELS];

    void Reset() { for (auto& l : levels) l.Clear(); }

    bool Load(const std::wstring& path, std::wstring& error) {
        FILE* f = _wfopen(path.c_str(), L"rb");
        if (!f) { error = lang::T(lang::E_CANNOT_OPEN); return false; }
        uint8_t buf[FILE_SIZE];
        size_t n = fread(buf, 1, FILE_SIZE, f);
        fclose(f);
        if (n != FILE_SIZE) {
            error = lang::T(lang::E_PPC_SIZE);
            return false;
        }
        int off = 6;
        for (int i = 0; i < NUM_LEVELS; i++) {
            memcpy(levels[i].header, buf + off, HEADER_SIZE);
            memcpy(levels[i].grid, buf + off + HEADER_SIZE, GRID_SIZE);
            off += LEVEL_SIZE;
        }
        return true;
    }

    bool Save(const std::wstring& path, std::wstring& error) const {
        uint8_t buf[FILE_SIZE] = {0};
        memcpy(buf, SIGNATURE, 6);
        int off = 6;
        for (int i = 0; i < NUM_LEVELS; i++) {
            memcpy(buf + off, levels[i].header, HEADER_SIZE);
            memcpy(buf + off + HEADER_SIZE, levels[i].grid, GRID_SIZE);
            off += LEVEL_SIZE;
        }
        FILE* f = _wfopen(path.c_str(), L"wb");
        if (!f) { error = lang::T(lang::E_CANNOT_CREATE); return false; }
        size_t n = fwrite(buf, 1, FILE_SIZE, f);
        fclose(f);
        if (n != FILE_SIZE) { error = lang::T(lang::E_WRITE_FAILED); return false; }
        return true;
    }
};

} // namespace ppc
