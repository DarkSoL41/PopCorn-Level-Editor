#pragma once
#include <cstdint>
#include <windows.h>
#include <vector>
#include <string>
#include "lang.h"

// Коды объектов, восстановленные из POPGEN.EXE и подтверждённые на данных
// самой игры (все 49 уровней). / Object codes recovered from POPGEN.EXE and
// confirmed against the game's own data (all 49 levels).
namespace cells {

struct ObjectDef {
    lang::Id nameId;
    const wchar_t* key;
    std::vector<uint8_t> codes; // row-major within footprint
    int footRows;
    int footCols;
    COLORREF color;
    const wchar_t* glyph;
};

inline const std::vector<ObjectDef>& Palette() {
    static std::vector<ObjectDef> p = {
        { lang::OBJ_F1,       L"F1",  {1},                  1,1, RGB(0x55,0xFF,0xFF), L"1" },
        { lang::OBJ_F2,       L"F2",  {2},                  1,1, RGB(0xFF,0x55,0xFF), L"2" },
        { lang::OBJ_F3,       L"F3",  {3},                  1,1, RGB(0xFF,0xFF,0xFF), L"3" },
        { lang::OBJ_F4,       L"F4",  {10},                 1,1, RGB(0xFF,0x99,0xCC), L"4" },
        { lang::OBJ_F5,       L"F5",  {5},                  1,1, RGB(0xCC,0x33,0xCC), L"5" },
        { lang::OBJ_F6,       L"F6",  {6},                  1,1, RGB(0x99,0x33,0x99), L"6" },
        { lang::OBJ_F7,       L"F7",  {7},                  1,1, RGB(0x33,0xCC,0xCC), L"7" },
        { lang::OBJ_F8,       L"F8",  {8},                  1,1, RGB(0x33,0x99,0x99), L"8" },
        { lang::OBJ_TELEPORT, L"F9",  {9},                  1,1, RGB(0xFF,0x33,0xFF), L"T" },
        { lang::OBJ_PICTURE,  L"F10", {16,17,18,19,20,21},  3,2, RGB(0xFF,0x66,0xCC), L"P" },
    };
    return p;
}

inline const wchar_t* NameOf(const ObjectDef* def) { return lang::T(def->nameId); }

inline bool IsPictureCode(uint8_t code) { return code >= 16 && code <= 21; }

inline void PictureOrigin(int row, int col, uint8_t code, int& r0, int& c0) {
    int idx = code - 16;
    int rowOff = idx / 2;
    int colOff = idx % 2;
    r0 = row - rowOff;
    c0 = col - colOff;
}

inline const ObjectDef* FindByCode(uint8_t code) {
    if (code == 0) return nullptr;
    for (auto& def : Palette())
        for (auto c : def.codes)
            if (c == code) return &def;
    return nullptr;
}

inline COLORREF ColorForCode(uint8_t code) {
    if (code == 0) return RGB(30, 30, 55);
    auto* def = FindByCode(code);
    if (def) return def->color;
    return RGB(90, 90, 90); // unknown code - still visible, not hidden
}

inline std::wstring GlyphForCode(uint8_t code) {
    if (code == 0) return L"";
    auto* def = FindByCode(code);
    if (def) return def->glyph;
    wchar_t buf[8];
    swprintf(buf, 8, L"%d", code);
    return buf;
}

inline std::wstring DescribeCode(uint8_t code) {
    if (code == 0) return lang::T(lang::CELL_EMPTY);
    auto* def = FindByCode(code);
    if (!def) {
        wchar_t buf[64];
        swprintf(buf, 64, lang::T(lang::CELL_UNKNOWN_FMT), code);
        return buf;
    }
    if (def->codes.size() > 1) {
        int part = 1;
        for (size_t i = 0; i < def->codes.size(); i++) if (def->codes[i] == code) part = (int)i + 1;
        wchar_t buf[128];
        swprintf(buf, 128, lang::T(lang::CELL_PART_FMT), NameOf(def), part,
                 (unsigned)def->codes.size());
        return buf;
    }
    return NameOf(def);
}

} // namespace cells
