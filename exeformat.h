#pragma once
// ---------------------------------------------------------------------------
// Чтение и запись уровней ПРЯМО В POPCORN.EXE.
//
// Игра запакована Microsoft EXEPACK (подпись "RB" перед точкой входа,
// строка "Packed file is corrupt" внутри распаковщика). Банк уровней лежит
// внутри запакованного образа, поэтому его нужно распаковывать и запаковывать
// тем же алгоритмом, что и EXEPACK.
//
// Формат EXEPACK (поток обрабатывается ОТ КОНЦА К НАЧАЛУ, поэтому в прямом
// порядке файла байт-команда стоит В КОНЦЕ блока):
//
//   [значение] [счётчик_lo] [счётчик_hi] 0xB0   -> байт [значение] x [счётчик]
//   [литералы...] [N_lo] [N_hi] 0xB2            -> N байт «как есть»
//
// Распакованный образ банка:
//   "LACRALLACRAL"  12 байт  (в .PPC подпись одинарная, 6 байт)
//   49 записей по 176 байт   (8 байт заголовка + сетка 14x12)
//
// КРИТИЧНО при записи: перекодированная область обязана занимать РОВНО столько
// же байт, сколько занимала. Никаких байтов-заполнителей вставлять нельзя —
// EXEPACK читает поток назад и принял бы посторонний байт за код команды,
// после чего игра выдаёт "Packed file is corrupt". Поэтому размер подгоняется
// дроблением блоков на несколько корректных блоков.
// ---------------------------------------------------------------------------
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include "ppcformat.h"
#include "lang.h"

namespace popexe {

constexpr uint8_t MARK_RUN = 0xB0;
constexpr uint8_t MARK_LIT = 0xB2;
constexpr int SIG_LEN = 12;
constexpr int IMAGE_SIZE = SIG_LEN + ppc::NUM_LEVELS * ppc::LEVEL_SIZE;  // 8636

// Набор кодов, которые игра считает разрушаемыми кирпичами (для заголовка).
inline bool IsDestructible(uint8_t c) {
    return c == 1 || c == 2 || c == 5 || c == 6 || c == 7 || c == 8 || c == 10
        || (c >= 16 && c <= 21);
}

struct Layout {
    size_t sigOff = 0;        // смещение "LACRALLACRAL" в файле
    size_t firstByteOff = 0;  // несжатый байт 0 записи 1
    size_t streamStart = 0;   // начало блоков
    size_t streamEnd = 0;     // конец блоков (не включительно)
};

inline bool ReadFileBytes(const std::wstring& path, std::vector<uint8_t>& out, std::wstring& err) {
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) { err = lang::T(lang::E_CANNOT_OPEN); return false; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); err = lang::T(lang::E_EMPTY_FILE); return false; }
    out.resize((size_t)n);
    size_t got = fread(out.data(), 1, out.size(), f);
    fclose(f);
    if (got != out.size()) { err = lang::T(lang::E_READ_FAILED); return false; }
    return true;
}

inline bool WriteFileBytes(const std::wstring& path, const std::vector<uint8_t>& data, std::wstring& err) {
    FILE* f = _wfopen(path.c_str(), L"wb");
    if (!f) { err = lang::T(lang::E_CANNOT_CREATE); return false; }
    size_t n = fwrite(data.data(), 1, data.size(), f);
    fclose(f);
    if (n != data.size()) { err = lang::T(lang::E_WRITE_FAILED); return false; }
    return true;
}

inline bool FindLayout(const std::vector<uint8_t>& exe, Layout& lay, std::wstring& err) {
    static const char* SIG = "LACRALLACRAL";
    size_t found = std::string::npos;
    if (exe.size() > (size_t)SIG_LEN) {
        for (size_t i = 0; i + SIG_LEN <= exe.size(); i++) {
            if (memcmp(exe.data() + i, SIG, SIG_LEN) == 0) { found = i; break; }
        }
    }
    if (found == std::string::npos) {
        err = lang::T(lang::E_NO_SIGNATURE);
        return false;
    }
    lay.sigOff = found;
    lay.firstByteOff = found + SIG_LEN;
    // за первым байтом идут 2 байта счётчика и байт-команда 0xB2 того блока,
    // в конец которого попали подпись и этот байт
    lay.streamStart = found + SIG_LEN + 4;
    if (lay.streamStart >= exe.size() || exe[lay.streamStart - 1] != MARK_LIT) {
        err = lang::T(lang::E_BAD_STRUCT);
        return false;
    }
    return true;
}

// Распаковать ровно outLen байт, начиная со смещения p. Возвращает конец потока.
inline bool Decompress(const std::vector<uint8_t>& buf, size_t p, size_t outLen,
                       std::vector<uint8_t>& out, size_t& endOff, std::wstring& err) {
    out.clear();
    out.reserve(outLen);
    while (out.size() < outLen) {
        size_t m = p;
        while (m < buf.size() && buf[m] != MARK_RUN && buf[m] != MARK_LIT) m++;
        if (m + 1 > buf.size() || m < 3) { err = lang::T(lang::E_STREAM_BROKEN); return false; }
        size_t cnt = (size_t)buf[m - 2] | ((size_t)buf[m - 1] << 8);
        if (buf[m] == MARK_RUN) {
            out.insert(out.end(), cnt, buf[m - 3]);
        } else {
            if (m < cnt + 2) { err = lang::T(lang::E_LIT_OOB); return false; }
            out.insert(out.end(), buf.begin() + (m - 2 - cnt), buf.begin() + (m - 2));
        }
        p = m + 1;
    }
    if (out.size() != outLen) {
        err = lang::T(lang::E_LAST_BLOCK);
        return false;
    }
    endOff = p;
    return true;
}

// Достать полный 8636-байтный образ банка из .exe.
inline bool ReadImage(const std::vector<uint8_t>& exe, std::vector<uint8_t>& img,
                      Layout& lay, std::wstring& err) {
    if (!FindLayout(exe, lay, err)) return false;
    std::vector<uint8_t> rest;
    size_t end = 0;
    if (!Decompress(exe, lay.streamStart, IMAGE_SIZE - SIG_LEN - 1, rest, end, err)) return false;
    lay.streamEnd = end;
    img.assign(exe.begin() + lay.sigOff, exe.begin() + lay.sigOff + SIG_LEN);
    img.push_back(exe[lay.firstByteOff]);
    img.insert(img.end(), rest.begin(), rest.end());
    return img.size() == IMAGE_SIZE;
}

// ------------------------------------------------------------------ упаковка

struct Chunk {
    bool run;
    uint8_t value;      // для run
    std::vector<uint8_t> lit;  // для литерального блока
    size_t count;       // длина в распакованных байтах
    size_t Size() const { return run ? 4 : lit.size() + 3; }
};

inline void EmitChunks(const std::vector<Chunk>& cs, std::vector<uint8_t>& out) {
    out.clear();
    for (const auto& c : cs) {
        if (c.run) {
            out.push_back(c.value);
            out.push_back((uint8_t)(c.count & 0xFF));
            out.push_back((uint8_t)((c.count >> 8) & 0xFF));
            out.push_back(MARK_RUN);
        } else {
            out.insert(out.end(), c.lit.begin(), c.lit.end());
            out.push_back((uint8_t)(c.lit.size() & 0xFF));
            out.push_back((uint8_t)((c.lit.size() >> 8) & 0xFF));
            out.push_back(MARK_LIT);
        }
    }
}

// Оптимальное по размеру кодирование (динамическое программирование).
inline std::vector<Chunk> BuildOptimal(const uint8_t* data, size_t n) {
    const size_t MAXLIT = 400;
    const size_t INF = (size_t)-1 / 4;
    std::vector<size_t> dp(n + 1, INF);
    std::vector<int> chLen(n + 1, 0);
    std::vector<bool> chRun(n + 1, false);
    dp[n] = 0;
    for (size_t i = n; i-- > 0;) {
        size_t L = 1;
        while (i + L < n && data[i + L] == data[i] && L < 0xFFFF) L++;
        if (dp[i + L] + 4 < dp[i]) { dp[i] = dp[i + L] + 4; chLen[i] = (int)L; chRun[i] = true; }
        size_t lim = MAXLIT < (n - i) ? MAXLIT : (n - i);
        for (size_t ll = 1; ll <= lim; ll++) {
            size_t c = ll + 3 + dp[i + ll];
            if (c < dp[i]) { dp[i] = c; chLen[i] = (int)ll; chRun[i] = false; }
        }
    }
    std::vector<Chunk> cs;
    size_t i = 0;
    while (i < n) {
        Chunk c;
        c.run = chRun[i];
        c.count = (size_t)chLen[i];
        if (c.run) c.value = data[i];
        else c.lit.assign(data + i, data + i + chLen[i]);
        cs.push_back(std::move(c));
        i += chLen[i];
    }
    return cs;
}

inline size_t TotalSize(const std::vector<Chunk>& cs) {
    size_t s = 0;
    for (const auto& c : cs) s += c.Size();
    return s;
}

// Довести кодирование до РОВНО target байт, дробя блоки (без мусорных байт!).
inline bool GrowToExact(std::vector<Chunk>& cs, size_t target, std::wstring& err) {
    long long D = (long long)target - (long long)TotalSize(cs);
    if (D < 0) {
        err = lang::T(lang::E_NO_ROOM);
        return false;
    }
    auto convertRun = [&](long long delta) -> bool {   // run длины delta+1 -> литерал (+delta)
        for (auto& c : cs) {
            if (c.run && c.count == (size_t)(delta + 1)) {
                Chunk n; n.run = false; n.count = c.count;
                n.lit.assign(c.count, c.value);
                c = std::move(n);
                return true;
            }
        }
        return false;
    };
    auto splitLit = [&]() -> bool {                    // +3
        for (size_t i = 0; i < cs.size(); i++) {
            if (!cs[i].run && cs[i].lit.size() >= 2) {
                Chunk a, b;
                a.run = b.run = false;
                a.lit.assign(cs[i].lit.begin(), cs[i].lit.begin() + 1);
                b.lit.assign(cs[i].lit.begin() + 1, cs[i].lit.end());
                a.count = a.lit.size(); b.count = b.lit.size();
                cs[i] = std::move(b);
                cs.insert(cs.begin() + i, std::move(a));
                return true;
            }
        }
        return false;
    };
    auto splitRun = [&]() -> bool {                    // +4
        for (size_t i = 0; i < cs.size(); i++) {
            if (cs[i].run && cs[i].count >= 2) {
                Chunk a = cs[i], b = cs[i];
                a.count = 1; b.count = cs[i].count - 1;
                cs[i] = b;
                cs.insert(cs.begin() + i, a);
                return true;
            }
        }
        return false;
    };
    while (D > 0) {
        if (D == 1 || D == 2) {
            if (!convertRun(D)) { err = lang::T(lang::E_SIZE_FIT); return false; }
            D = 0;
        } else if (D == 5) {
            if (convertRun(1)) D = 4;
            else if (splitLit()) D = 2;
            else { err = lang::T(lang::E_SIZE_FIT); return false; }
        } else if (D % 3 == 0 && splitLit()) {
            D -= 3;
        } else if (D >= 4 && splitRun()) {
            D -= 4;
        } else if (splitLit()) {
            D -= 3;
        } else {
            err = lang::T(lang::E_SIZE_FIT);
            return false;
        }
    }
    return true;
}

// Записать образ банка в копию .exe. Размер файла не меняется.
inline bool WriteImage(const std::vector<uint8_t>& exeIn, const std::vector<uint8_t>& img,
                       std::vector<uint8_t>& exeOut, std::wstring& err) {
    if (img.size() != IMAGE_SIZE) { err = lang::T(lang::E_BAD_IMAGE); return false; }
    Layout lay;
    std::vector<uint8_t> check;
    if (!ReadImage(exeIn, check, lay, err)) return false;

    size_t avail = lay.streamEnd - lay.streamStart;
    const uint8_t* body = img.data() + SIG_LEN + 1;
    size_t bodyLen = IMAGE_SIZE - SIG_LEN - 1;

    std::vector<Chunk> cs = BuildOptimal(body, bodyLen);
    if (!GrowToExact(cs, avail, err)) return false;
    std::vector<uint8_t> enc;
    EmitChunks(cs, enc);
    if (enc.size() != avail) { err = lang::T(lang::E_INTERNAL_SIZE); return false; }

    exeOut = exeIn;
    exeOut[lay.firstByteOff] = img[SIG_LEN];
    memcpy(exeOut.data() + lay.streamStart, enc.data(), enc.size());

    // Обязательная самопроверка: распаковать обратно и сравнить побайтово.
    std::vector<uint8_t> back;
    Layout lay2;
    std::wstring e2;
    if (!ReadImage(exeOut, back, lay2, e2) || back != img) {
        err = std::wstring(lang::T(lang::E_VERIFY_FAILED)) + e2;
        return false;
    }
    return true;
}

// ------------------------------------------------------- банк <-> образ

inline void RebuildHeader(ppc::Level& lv) {
    int bricks = 0;
    uint8_t tele[6] = {0};
    int nt = 0;
    for (int i = 0; i < ppc::GRID_SIZE; i++) {
        uint8_t c = lv.grid[i];
        if (IsDestructible(c)) bricks++;
        if (c == 9 && nt < 6) tele[nt++] = (uint8_t)i;
    }
    memset(lv.header, 0, ppc::HEADER_SIZE);
    lv.header[0] = (uint8_t)(bricks & 0xFF);
    lv.header[1] = (uint8_t)nt;
    for (int i = 0; i < nt; i++) lv.header[2 + i] = tele[i];
}

inline void ImageToBank(const std::vector<uint8_t>& img, ppc::Bank& bank) {
    for (int i = 0; i < ppc::NUM_LEVELS; i++) {
        size_t o = SIG_LEN + (size_t)i * ppc::LEVEL_SIZE;
        memcpy(bank.levels[i].header, img.data() + o, ppc::HEADER_SIZE);
        memcpy(bank.levels[i].grid, img.data() + o + ppc::HEADER_SIZE, ppc::GRID_SIZE);
    }
}

inline std::vector<uint8_t> BankToImage(const ppc::Bank& bank, bool recomputeHeaders) {
    std::vector<uint8_t> img;
    img.reserve(IMAGE_SIZE);
    const char* sig = "LACRALLACRAL";
    img.insert(img.end(), sig, sig + SIG_LEN);
    for (int i = 0; i < ppc::NUM_LEVELS; i++) {
        ppc::Level lv = bank.levels[i];
        if (recomputeHeaders) RebuildHeader(lv);
        img.insert(img.end(), lv.header, lv.header + ppc::HEADER_SIZE);
        img.insert(img.end(), lv.grid, lv.grid + ppc::GRID_SIZE);
    }
    return img;
}

} // namespace popexe
