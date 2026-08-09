// POP-CORN Level Editor (unofficial) - native Win32, no runtime dependencies.
// Built with MinGW-w64, statically linked - runs on Windows 10/11 out of the box,
// no .NET, no Python, no installer, no extra DLLs required.
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cwctype>
#include "lang.h"
#include "ppcformat.h"
#include "cellcodes.h"
#include "exeformat.h"
#include "sprites.h"

// ---------------------------------------------------------------- Globals

static ppc::Bank g_bank;
static int g_currentLevel = 0;
static std::wstring g_currentPath;
static bool g_dirty = false;

static const cells::ObjectDef* g_selectedObject = &cells::Palette()[0];
static bool g_eraserSelected = false;

static HWND g_hMain, g_hGrid, g_hStatus, g_hLevelLabel, g_hLevelEdit, g_hRawEdit;
// Хэндлы всего, что содержит текст — нужны, чтобы переписать подписи при
// смене языка. / Handles of everything with text, so captions can be
// rewritten when the language changes.
static HWND g_hPrevBtn, g_hNextBtn, g_hGoBtn, g_hClearBtn, g_hRawLabel, g_hApplyBtn, g_hHint;
static HWND g_hPalBtn[11];

// Рисование с зажатой кнопкой («карандаш»). / Press-and-drag painting.
static bool g_painting = false, g_erasing = false;
static int g_lastRow = -1, g_lastCol = -1;
static HFONT g_hFont, g_hFontBold, g_hFontGlyph;

static const int ORIGIN_X = 20, ORIGIN_Y = 20;

// Два режима отображения поля:
//  - схема: квадратные клетки с цифрами (удобно для точной работы с кодами);
//  - графика: настоящие спрайты блоков из игры. Пропорции клетки взяты
//    авторские: в CGA mode 5 (320x200 на экране 4:3) пиксель выше своей
//    ширины в 1.2 раза, поэтому клетка 16x8 игровых пикселей выглядит как
//    16 : 9.6, то есть 5:3 — отсюда 40x24 на экране.
static bool g_spriteMode = true;
static int CellW() { return g_spriteMode ? 40 : 34; }
static int CellH() { return g_spriteMode ? 24 : 34; }

enum {
    ID_FILE_NEW = 100, ID_FILE_OPEN, ID_FILE_SAVE, ID_FILE_SAVEAS, ID_FILE_EXIT,
    ID_FILE_IMPORT_EXE, ID_FILE_EXPORT_EXE,
    ID_LEVEL_PREV, ID_LEVEL_NEXT, ID_LEVEL_CLEAR, ID_LEVEL_COPY, ID_LEVEL_GO,
    ID_VIEW_SPRITES, ID_VIEW_SCHEMA,
    ID_LANG_EN, ID_LANG_RU,
    ID_HELP_ABOUT, ID_HELP_FORMAT,
    ID_RAW_APPLY,
    ID_PALETTE_BASE = 200, // + 0..9 for the ten objects, +10 for eraser
};

static void UpdateTitleAndLabel();
static void InvalidateGrid();

// ---------------------------------------------------------------- Editing ops

static void EraseCell(int row, int col, bool repaint = true) {
    auto& level = g_bank.levels[g_currentLevel];
    uint8_t code = level.Get(row, col);
    if (code == 0) return;

    if (cells::IsPictureCode(code)) {
        int r0, c0;
        cells::PictureOrigin(row, col, code, r0, c0);
        for (int dr = 0; dr < 3; dr++)
            for (int dc = 0; dc < 2; dc++) {
                int rr = r0 + dr, cc = c0 + dc;
                if (rr >= 0 && rr < ppc::ROWS && cc >= 0 && cc < ppc::COLS)
                    level.Set(rr, cc, 0);
            }
    } else {
        level.Set(row, col, 0);
    }
    g_dirty = true;
    if (repaint) InvalidateGrid();
}

static void PlaceObject(int row, int col, const cells::ObjectDef* def) {
    if (row + def->footRows > ppc::ROWS || col + def->footCols > ppc::COLS) {
        SetWindowTextW(g_hStatus, lang::T(lang::ST_NOFIT));
        return;
    }
    auto& level = g_bank.levels[g_currentLevel];

    for (int dr = 0; dr < def->footRows; dr++)
        for (int dc = 0; dc < def->footCols; dc++)
            EraseCell(row + dr, col + dc, false);

    int k = 0;
    for (int dr = 0; dr < def->footRows; dr++)
        for (int dc = 0; dc < def->footCols; dc++) {
            uint8_t code = def->codes.size() == 1 ? def->codes[0] : def->codes[k];
            level.Set(row + dr, col + dc, code);
            k++;
        }
    g_dirty = true;
    UpdateTitleAndLabel();
    InvalidateGrid();
}

// Одно действие «карандаша» в клетке. / One pencil action on a cell.
static void ApplyAt(int row, int col, bool erase) {
    if (erase) EraseCell(row, col);
    else PlaceObject(row, col, g_selectedObject);
}

static bool CellFromPoint(int x, int y, int& row, int& col) {
    if (x < ORIGIN_X || y < ORIGIN_Y) return false;
    col = (x - ORIGIN_X) / CellW();
    row = (y - ORIGIN_Y) / CellH();
    return row >= 0 && row < ppc::ROWS && col >= 0 && col < ppc::COLS;
}

// ------------------------------------------------- отрисовка спрайтами из игры

// Собираем поле в маленький растр в НАСТОЯЩЕМ игровом разрешении
// (12*16 x 14*8 = 192x112 пикселей), затем растягиваем целыми пикселями.
static void PaintSpriteGrid(HDC hdc, const ppc::Level& level) {
    const int PW = ppc::COLS * sprites::CELL_W;   // 192
    const int PH = ppc::ROWS * sprites::CELL_H;   // 112
    static std::vector<uint32_t> px;
    px.assign((size_t)PW * PH, 0x00000000u);

    for (int r = 0; r < ppc::ROWS; r++) {
        for (int c = 0; c < ppc::COLS; c++) {
            uint8_t code = level.Get(r, c);
            const sprites::Tile* t = code ? sprites::Find(code) : nullptr;
            for (int y = 0; y < sprites::CELL_H; y++) {
                for (int x = 0; x < sprites::CELL_W; x++) {
                    uint32_t v;
                    if (code == 0) {
                        v = 0x00101024u;                       // пустая клетка
                    } else if (t) {
                        v = sprites::PALETTE[t->px[y * sprites::CELL_W + x]];
                    } else {
                        // код без известного спрайта — заливаем серым, чтобы
                        // клетка не выглядела пустой и её было видно
                        v = ((x + y) & 1) ? 0x00808080u : 0x00606060u;
                    }
                    px[(size_t)(r * sprites::CELL_H + y) * PW + (c * sprites::CELL_W + x)] = v;
                }
            }
        }
    }

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = PW;
    bi.bmiHeader.biHeight = -PH;          // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    int dw = ppc::COLS * CellW(), dh = ppc::ROWS * CellH();
    SetStretchBltMode(hdc, COLORONCOLOR);   // без сглаживания — честные пиксели
    StretchDIBits(hdc, ORIGIN_X, ORIGIN_Y, dw, dh, 0, 0, PW, PH,
                  px.data(), &bi, DIB_RGB_COLORS, SRCCOPY);

    // тонкая сетка, чтобы было видно границы клеток при клике
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(60, 60, 80));
    HPEN old = (HPEN)SelectObject(hdc, pen);
    for (int c = 0; c <= ppc::COLS; c++) {
        MoveToEx(hdc, ORIGIN_X + c * CellW(), ORIGIN_Y, nullptr);
        LineTo(hdc, ORIGIN_X + c * CellW(), ORIGIN_Y + dh);
    }
    for (int r = 0; r <= ppc::ROWS; r++) {
        MoveToEx(hdc, ORIGIN_X, ORIGIN_Y + r * CellH(), nullptr);
        LineTo(hdc, ORIGIN_X + dw, ORIGIN_Y + r * CellH());
    }
    SelectObject(hdc, old);
    DeleteObject(pen);
}

// Рисуем один спрайт в произвольный прямоугольник (для кнопок палитры).
static void DrawSpriteInto(HDC hdc, uint8_t code, int x, int y, int w, int h) {
    const sprites::Tile* t = sprites::Find(code);
    if (!t) return;
    static std::vector<uint32_t> px;
    px.assign((size_t)sprites::CELL_W * sprites::CELL_H, 0);
    for (int yy = 0; yy < sprites::CELL_H; yy++)
        for (int xx = 0; xx < sprites::CELL_W; xx++)
            px[(size_t)yy * sprites::CELL_W + xx] =
                sprites::PALETTE[t->px[yy * sprites::CELL_W + xx]];
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = sprites::CELL_W;
    bi.bmiHeader.biHeight = -sprites::CELL_H;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    SetStretchBltMode(hdc, COLORONCOLOR);
    StretchDIBits(hdc, x, y, w, h, 0, 0, sprites::CELL_W, sprites::CELL_H,
                  px.data(), &bi, DIB_RGB_COLORS, SRCCOPY);
}

// ---------------------------------------------------------------- Grid canvas window

LRESULT CALLBACK GridWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc; GetClientRect(hwnd, &rc);
        HBRUSH bg = CreateSolidBrush(RGB(20, 20, 40));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        auto& level = g_bank.levels[g_currentLevel];
        SetBkMode(hdc, TRANSPARENT);

        if (g_spriteMode) {
            PaintSpriteGrid(hdc, level);
        } else {
            HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontGlyph);
            for (int r = 0; r < ppc::ROWS; r++) {
                for (int c = 0; c < ppc::COLS; c++) {
                    uint8_t code = level.Get(r, c);
                    RECT cell = { ORIGIN_X + c * CellW(), ORIGIN_Y + r * CellH(),
                                  ORIGIN_X + c * CellW() + CellW() - 2,
                                  ORIGIN_Y + r * CellH() + CellH() - 2 };
                    HBRUSH br = CreateSolidBrush(cells::ColorForCode(code));
                    FillRect(hdc, &cell, br);
                    DeleteObject(br);
                    FrameRect(hdc, &cell, (HBRUSH)GetStockObject(GRAY_BRUSH));
                    if (code != 0) {
                        std::wstring g = cells::GlyphForCode(code);
                        SetTextColor(hdc, RGB(0, 0, 0));
                        DrawTextW(hdc, g.c_str(), (int)g.size(), &cell,
                                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    }
                }
            }
            SelectObject(hdc, oldFont);
        }

        RECT border = { ORIGIN_X - 4, ORIGIN_Y - 4,
                        ORIGIN_X + ppc::COLS * CellW(), ORIGIN_Y + ppc::ROWS * CellH() };
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 255, 255));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, border.left, border.top, border.right, border.bottom);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int row, col;
        if (CellFromPoint(LOWORD(lp), HIWORD(lp), row, col)) {
            ApplyAt(row, col, g_eraserSelected);
            g_painting = true; g_erasing = g_eraserSelected;
            g_lastRow = row; g_lastCol = col;
            SetCapture(hwnd);   // чтобы не терять протяжку, если курсор ушёл за край
        }
        SetFocus(hwnd);
        return 0;
    }
    case WM_RBUTTONDOWN: {
        int row, col;
        if (CellFromPoint(LOWORD(lp), HIWORD(lp), row, col)) {
            ApplyAt(row, col, true);
            g_painting = true; g_erasing = true;
            g_lastRow = row; g_lastCol = col;
            SetCapture(hwnd);
        }
        return 0;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_CAPTURECHANGED: {
        if (g_painting) {
            g_painting = false;
            g_lastRow = g_lastCol = -1;
            if (msg != WM_CAPTURECHANGED && GetCapture() == hwnd) ReleaseCapture();
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        // LOWORD/HIWORD теряют знак, а при захвате мыши координаты бывают
        // отрицательными (курсор ушёл выше/левее поля) — берём со знаком.
        int mx = (int)(short)LOWORD(lp), my = (int)(short)HIWORD(lp);
        int row, col;
        bool inside = CellFromPoint(mx, my, row, col);
        if (g_painting) {
            // рисуем только при смене клетки — иначе блок 2x3 «дёргался» бы
            // на месте и каждый пиксель движения перерисовывал бы поле
            if (inside && (row != g_lastRow || col != g_lastCol)) {
                ApplyAt(row, col, g_erasing);
                g_lastRow = row; g_lastCol = col;
            }
        }
        if (inside) {
            uint8_t code = g_bank.levels[g_currentLevel].Get(row, col);
            wchar_t buf[160];
            swprintf(buf, 160, lang::T(lang::ST_CELL_FMT), row + 1, col + 1,
                     cells::DescribeCode(code).c_str());
            SetWindowTextW(g_hStatus, buf);
        }
        return 0;
    }
    case WM_KEYDOWN: {
        extern void ChangeLevel(int);
        extern void SelectPaletteIndex(int);
        if (wp == VK_PRIOR) ChangeLevel(-1);
        else if (wp == VK_NEXT) ChangeLevel(1);
        else if (wp == VK_DELETE) {
            extern void SelectEraser();
            SelectEraser();
        }
        else if (wp >= VK_F1 && wp <= VK_F10) SelectPaletteIndex((int)(wp - VK_F1));
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------- Level nav / title

void ChangeLevel(int delta) {
    int idx = g_currentLevel + delta;
    if (idx < 0) idx = 0;
    if (idx >= ppc::NUM_LEVELS) idx = ppc::NUM_LEVELS - 1;
    g_currentLevel = idx;
    UpdateTitleAndLabel();
    InvalidateGrid();
}

static void GoToLevel(int idx) {
    if (idx < 0) idx = 0;
    if (idx >= ppc::NUM_LEVELS) idx = ppc::NUM_LEVELS - 1;
    g_currentLevel = idx;
    UpdateTitleAndLabel();
    InvalidateGrid();
}

void SelectPaletteIndex(int idx) {
    auto& pal = cells::Palette();
    if (idx >= 0 && idx < (int)pal.size()) {
        g_selectedObject = &pal[idx];
        g_eraserSelected = false;
        wchar_t sb[160];
        swprintf(sb, 160, lang::T(lang::ST_SELECTED_FMT), cells::NameOf(&pal[idx]));
        SetWindowTextW(g_hStatus, sb);
        // Repaint every palette button, not just the clicked one - otherwise the
        // previous selection's highlight border is never erased and it looks like
        // several objects are selected at once.
        for (int i = 0; i <= 10; i++)
            InvalidateRect(GetDlgItem(g_hMain, ID_PALETTE_BASE + i), nullptr, TRUE);
    }
}

void SelectEraser() {
    g_eraserSelected = true;
    SetWindowTextW(g_hStatus, lang::T(lang::ST_ERASER));
    for (int i = 0; i <= 10; i++)
        InvalidateRect(GetDlgItem(g_hMain, ID_PALETTE_BASE + i), nullptr, TRUE);
}

static void UpdateTitleAndLabel() {
    wchar_t buf[256];
    std::wstring name = g_currentPath.empty() ? lang::T(lang::NEW_BANK) : g_currentPath;
    size_t slash = name.find_last_of(L"\\/");
    if (slash != std::wstring::npos) name = name.substr(slash + 1);
    swprintf(buf, 256, lang::T(lang::TITLE_FMT),
        name.c_str(), g_currentLevel + 1, g_dirty ? L" *" : L"");
    SetWindowTextW(g_hMain, buf);

    wchar_t lvl[64];
    swprintf(lvl, 64, lang::T(lang::LBL_LEVEL_FMT), g_currentLevel + 1, ppc::NUM_LEVELS);
    SetWindowTextW(g_hLevelLabel, lvl);

    wchar_t editBuf[16];
    swprintf(editBuf, 16, L"%d", g_currentLevel + 1);
    SetWindowTextW(g_hLevelEdit, editBuf);
}

static void InvalidateGrid() { InvalidateRect(g_hGrid, nullptr, TRUE); }

// ---------------------------------------------------------------- File ops

static bool ConfirmDiscardIfDirty() {
    if (!g_dirty) return true;
    int res = MessageBoxW(g_hMain, lang::T(lang::MSG_DISCARD),
        lang::T(lang::T_WARNING), MB_YESNO | MB_ICONWARNING);
    return res == IDYES;
}

static void DoNew() {
    if (!ConfirmDiscardIfDirty()) return;
    g_bank.Reset();
    g_currentPath.clear();
    g_currentLevel = 0;
    g_dirty = false;
    UpdateTitleAndLabel();
    InvalidateGrid();
}

static void DoOpen() {
    if (!ConfirmDiscardIfDirty()) return;
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = g_hMain;
    ofn.lpstrFilter = lang::T(lang::FILTER_PPC);
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = lang::T(lang::OFN_OPEN_PPC);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;

    std::wstring error;
    if (!g_bank.Load(file, error)) {
        MessageBoxW(g_hMain, error.c_str(), lang::T(lang::ERR_OPEN_TITLE), MB_OK | MB_ICONERROR);
        return;
    }
    g_currentPath = file;
    g_currentLevel = 0;
    g_dirty = false;
    UpdateTitleAndLabel();
    InvalidateGrid();
}

static bool DoSaveAs() {
    wchar_t file[MAX_PATH] = L"TABLEAUX.PPC";
    if (!g_currentPath.empty()) wcsncpy(file, g_currentPath.c_str(), MAX_PATH - 1);
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = g_hMain;
    ofn.lpstrFilter = lang::T(lang::FILTER_PPC);
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = lang::T(lang::OFN_SAVE_PPC);
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"PPC";
    if (!GetSaveFileNameW(&ofn)) return false;

    std::wstring error;
    if (!g_bank.Save(file, error)) {
        MessageBoxW(g_hMain, error.c_str(), lang::T(lang::ERR_SAVE_TITLE), MB_OK | MB_ICONERROR);
        return false;
    }
    g_currentPath = file;
    g_dirty = false;
    UpdateTitleAndLabel();
    { wchar_t sb[MAX_PATH + 64]; swprintf(sb, MAX_PATH + 64, lang::T(lang::ST_SAVED_FMT), file);
      SetWindowTextW(g_hStatus, sb); }
    return true;
}

static void DoSave() {
    if (g_currentPath.empty()) { DoSaveAs(); return; }
    std::wstring error;
    if (!g_bank.Save(g_currentPath, error)) {
        MessageBoxW(g_hMain, error.c_str(), lang::T(lang::ERR_SAVE_TITLE), MB_OK | MB_ICONERROR);
        return;
    }
    g_dirty = false;
    UpdateTitleAndLabel();
    { wchar_t sb[MAX_PATH + 64];
      swprintf(sb, MAX_PATH + 64, lang::T(lang::ST_SAVED_FMT), g_currentPath.c_str());
      SetWindowTextW(g_hStatus, sb); }
}

// ------------------------------------------------- импорт/экспорт POPCORN.EXE

static std::wstring AskExePath(const wchar_t* title) {
    wchar_t file[MAX_PATH] = L"POPCORN.EXE";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = g_hMain;
    ofn.lpstrFilter = lang::T(lang::FILTER_EXE);
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return L"";
    return file;
}

static void DoImportExe() {
    if (!ConfirmDiscardIfDirty()) return;
    std::wstring path = AskExePath(lang::T(lang::OFN_IMPORT_EXE));
    if (path.empty()) return;

    std::vector<uint8_t> exe;
    std::wstring err;
    if (!popexe::ReadFileBytes(path, exe, err)) {
        MessageBoxW(g_hMain, err.c_str(), lang::T(lang::T_IMPORT_ERR), MB_OK | MB_ICONERROR);
        return;
    }
    std::vector<uint8_t> img;
    popexe::Layout lay;
    if (!popexe::ReadImage(exe, img, lay, err)) {
        MessageBoxW(g_hMain, err.c_str(), lang::T(lang::T_IMPORT_ERR), MB_OK | MB_ICONERROR);
        return;
    }
    popexe::ImageToBank(img, g_bank);
    g_currentPath.clear();          // это не .PPC-файл, «Сохранить» спросит имя
    g_currentLevel = 0;
    g_dirty = false;
    UpdateTitleAndLabel();
    InvalidateGrid();

    wchar_t msg[800];
    swprintf(msg, 800, lang::T(lang::IMPORT_OK_FMT),
        (unsigned)lay.sigOff, (unsigned)(lay.streamEnd - lay.streamStart));
    MessageBoxW(g_hMain, msg, lang::T(lang::IMPORT_OK_TITLE), MB_OK | MB_ICONINFORMATION);
}

static void DoExportExe() {
    std::wstring src = AskExePath(lang::T(lang::OFN_EXPORT_STEP1));
    if (src.empty()) return;

    std::vector<uint8_t> exe;
    std::wstring err;
    if (!popexe::ReadFileBytes(src, exe, err)) {
        MessageBoxW(g_hMain, err.c_str(), lang::T(lang::T_EXPORT_ERR), MB_OK | MB_ICONERROR);
        return;
    }

    // Заголовки записей пересчитываем сами: игра хранит в них число
    // разрушаемых кирпичей и позиции телепортов, и если их не обновить,
    // уровень нельзя будет пройти (или он завершится раньше времени).
    std::vector<uint8_t> img = popexe::BankToImage(g_bank, true);

    std::vector<uint8_t> out;
    if (!popexe::WriteImage(exe, img, out, err)) {
        MessageBoxW(g_hMain, err.c_str(), lang::T(lang::T_EXPORT_ERR), MB_OK | MB_ICONERROR);
        return;
    }

    wchar_t file[MAX_PATH] = L"POPCORN_patched.EXE";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = g_hMain;
    ofn.lpstrFilter = lang::T(lang::FILTER_EXE);
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = lang::T(lang::OFN_EXPORT_STEP2);
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"EXE";
    if (!GetSaveFileNameW(&ofn)) return;

    if (!popexe::WriteFileBytes(file, out, err)) {
        MessageBoxW(g_hMain, err.c_str(), lang::T(lang::T_EXPORT_ERR), MB_OK | MB_ICONERROR);
        return;
    }
    wchar_t msg[900];
    swprintf(msg, 900, lang::T(lang::EXPORT_OK_FMT), file, (unsigned)out.size());
    MessageBoxW(g_hMain, msg, lang::T(lang::EXPORT_OK_TITLE), MB_OK | MB_ICONINFORMATION);
    { wchar_t sb[MAX_PATH + 64];
      swprintf(sb, MAX_PATH + 64, lang::T(lang::ST_PATCHED_FMT), file);
      SetWindowTextW(g_hStatus, sb); }
}

static void SetViewMode(bool sprite) {
    g_spriteMode = sprite;
    HMENU m = GetMenu(g_hMain);
    CheckMenuItem(m, ID_VIEW_SPRITES, MF_BYCOMMAND | (sprite ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(m, ID_VIEW_SCHEMA, MF_BYCOMMAND | (sprite ? MF_UNCHECKED : MF_CHECKED));
    // размер клетки в режимах разный, поэтому поле надо перерисовать целиком
    InvalidateGrid();
    for (int i = 0; i <= 10; i++)
        InvalidateRect(GetDlgItem(g_hMain, ID_PALETTE_BASE + i), nullptr, TRUE);
    SetWindowTextW(g_hStatus, lang::T(sprite ? lang::ST_SPRITE_MODE : lang::ST_SCHEMA_MODE));
}

// Меню целиком пересобирается при смене языка — так проще и надёжнее, чем
// переписывать каждый пункт по отдельности.
// The whole menu is rebuilt on language change - simpler and safer than
// renaming every item one by one.
static void BuildMenu(HWND hwnd) {
    HMENU oldMenu = GetMenu(hwnd);
    HMENU hMenu = CreateMenu();

    HMENU fileMenu = CreatePopupMenu();
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_NEW, lang::T(lang::MI_NEW));
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_OPEN, lang::T(lang::MI_OPEN));
    AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_SAVE, lang::T(lang::MI_SAVE));
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_SAVEAS, lang::T(lang::MI_SAVEAS));
    AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_IMPORT_EXE, lang::T(lang::MI_IMPORT_EXE));
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_EXPORT_EXE, lang::T(lang::MI_EXPORT_EXE));
    AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_EXIT, lang::T(lang::MI_EXIT));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)fileMenu, lang::T(lang::MENU_FILE));

    HMENU lvlMenu = CreatePopupMenu();
    AppendMenuW(lvlMenu, MF_STRING, ID_LEVEL_PREV, lang::T(lang::MI_PREV));
    AppendMenuW(lvlMenu, MF_STRING, ID_LEVEL_NEXT, lang::T(lang::MI_NEXT));
    AppendMenuW(lvlMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(lvlMenu, MF_STRING, ID_LEVEL_CLEAR, lang::T(lang::MI_CLEAR));
    AppendMenuW(lvlMenu, MF_STRING, ID_LEVEL_COPY, lang::T(lang::MI_COPY));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)lvlMenu, lang::T(lang::MENU_LEVEL));

    HMENU viewMenu = CreatePopupMenu();
    AppendMenuW(viewMenu, MF_STRING | (g_spriteMode ? MF_CHECKED : 0),
                ID_VIEW_SPRITES, lang::T(lang::MI_SPRITES));
    AppendMenuW(viewMenu, MF_STRING | (g_spriteMode ? 0 : MF_CHECKED),
                ID_VIEW_SCHEMA, lang::T(lang::MI_SCHEMA));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)viewMenu, lang::T(lang::MENU_VIEW));

    HMENU langMenu = CreatePopupMenu();
    AppendMenuW(langMenu, MF_STRING | (lang::g_lang == lang::EN ? MF_CHECKED : 0),
                ID_LANG_EN, lang::T(lang::MI_LANG_EN));
    AppendMenuW(langMenu, MF_STRING | (lang::g_lang == lang::RU ? MF_CHECKED : 0),
                ID_LANG_RU, lang::T(lang::MI_LANG_RU));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)langMenu, lang::T(lang::MENU_LANG));

    HMENU helpMenu = CreatePopupMenu();
    AppendMenuW(helpMenu, MF_STRING, ID_HELP_FORMAT, lang::T(lang::MI_HELP_FORMAT));
    AppendMenuW(helpMenu, MF_STRING, ID_HELP_ABOUT, lang::T(lang::MI_HELP_ABOUT));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)helpMenu, lang::T(lang::MENU_HELP));

    SetMenu(hwnd, hMenu);
    if (oldMenu) DestroyMenu(oldMenu);
    DrawMenuBar(hwnd);
}

// Переписать все подписи после смены языка.
// Re-apply all captions after a language change.
static void ApplyLanguage(HWND hwnd) {
    BuildMenu(hwnd);
    SetWindowTextW(g_hPrevBtn, lang::T(lang::BTN_PREV));
    SetWindowTextW(g_hNextBtn, lang::T(lang::BTN_NEXT));
    SetWindowTextW(g_hGoBtn, lang::T(lang::BTN_GO));
    SetWindowTextW(g_hClearBtn, lang::T(lang::BTN_CLEAR));
    SetWindowTextW(g_hRawLabel, lang::T(lang::LBL_RAW));
    SetWindowTextW(g_hApplyBtn, lang::T(lang::BTN_APPLY));
    SetWindowTextW(g_hHint, lang::T(lang::HINT));
    auto& pal = cells::Palette();
    for (size_t i = 0; i < pal.size(); i++)
        SetWindowTextW(g_hPalBtn[i],
            (std::wstring(pal[i].key) + L": " + cells::NameOf(&pal[i])).c_str());
    SetWindowTextW(g_hPalBtn[10], lang::T(lang::BTN_ERASER));
    SetWindowTextW(g_hStatus, lang::T(lang::ST_HOVER));
    UpdateTitleAndLabel();
    for (int i = 0; i <= 10; i++) InvalidateRect(g_hPalBtn[i], nullptr, TRUE);
    InvalidateGrid();
}

static void DoClearLevel() {
    wchar_t msg[128];
    swprintf(msg, 128, lang::T(lang::CLEAR_CONFIRM_FMT), g_currentLevel + 1);
    if (MessageBoxW(g_hMain, msg, lang::T(lang::T_CONFIRM), MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    g_bank.levels[g_currentLevel].Clear();
    g_dirty = true;
    UpdateTitleAndLabel();
    InvalidateGrid();
}

namespace miniprompt {
    static int g_result = 0;
    static bool g_done = false;
    static bool g_okPressed = false;
    static HWND g_editHwnd = nullptr;

    static LRESULT CALLBACK DefDlgProcStatic(HWND h, UINT m, WPARAM w, LPARAM l) {
        if (m == WM_CLOSE) { g_done = true; DestroyWindow(h); return 0; }
        return DefWindowProcW(h, m, w, l);
    }

    static int Show(HWND parent, const wchar_t* title, int initial) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = DefDlgProcStatic;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"PopCornMiniPrompt";
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        static bool registered = false;
        if (!registered) { RegisterClassW(&wc); registered = true; }

        g_result = initial;
        HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"PopCornMiniPrompt", title,
            WS_POPUP | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, 260, 130, parent, nullptr, wc.hInstance, nullptr);
        HWND lbl = CreateWindowExW(0, L"STATIC", lang::T(lang::COPY_LABEL),
            WS_CHILD | WS_VISIBLE, 10, 10, 220, 20, dlg, nullptr, wc.hInstance, nullptr);
        HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", std::to_wstring(initial).c_str(),
            WS_CHILD | WS_VISIBLE | ES_NUMBER, 10, 35, 80, 22, dlg, (HMENU)1, wc.hInstance, nullptr);
        HWND ok = CreateWindowExW(0, L"BUTTON", lang::T(lang::BTN_OK),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 10, 65, 80, 26, dlg, (HMENU)2, wc.hInstance, nullptr);
        HWND cancel = CreateWindowExW(0, L"BUTTON", lang::T(lang::BTN_CANCEL),
            WS_CHILD | WS_VISIBLE, 100, 65, 80, 26, dlg, (HMENU)3, wc.hInstance, nullptr);
        SendMessageW(lbl, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        SendMessageW(edit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        SendMessageW(ok, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        SendMessageW(cancel, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

        g_editHwnd = edit;
        g_done = false; g_okPressed = false;
        ShowWindow(dlg, SW_SHOW);
        EnableWindow(parent, FALSE);
        MSG msg;
        while (!g_done && GetMessageW(&msg, nullptr, 0, 0)) {
            if (msg.hwnd == dlg || IsChild(dlg, msg.hwnd)) {
                if (msg.message == WM_COMMAND) {
                    int id = LOWORD(msg.wParam);
                    if (id == 2) { g_okPressed = true; g_done = true; }
                    else if (id == 3) { g_done = true; }
                }
                if (!IsDialogMessageW(dlg, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            } else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        if (g_okPressed) {
            wchar_t txt[16];
            GetWindowTextW(g_editHwnd, txt, 16);
            g_result = _wtoi(txt);
        } else {
            g_result = -1;
        }
        EnableWindow(parent, TRUE);
        DestroyWindow(dlg);
        return g_result;
    }
}

static void DoCopyLevelTo() {
    int target = miniprompt::Show(g_hMain, lang::T(lang::COPY_TITLE), g_currentLevel + 1);
    if (target < 1 || target > ppc::NUM_LEVELS) return;
    target -= 1;
    if (target == g_currentLevel) return;
    g_bank.levels[target] = g_bank.levels[g_currentLevel];
    g_dirty = true;
    UpdateTitleAndLabel();
    wchar_t msg[128];
    swprintf(msg, 128, lang::T(lang::COPY_DONE_FMT), g_currentLevel + 1, target + 1);
    MessageBoxW(g_hMain, msg, lang::T(lang::T_DONE), MB_OK);
}

static void DoAbout() {
    MessageBoxW(g_hMain, lang::T(lang::ABOUT_BODY), lang::T(lang::ABOUT_TITLE),
                MB_OK | MB_ICONINFORMATION);
}

static void DoFormatHelp() {
    MessageBoxW(g_hMain, lang::T(lang::FORMAT_BODY), lang::T(lang::FORMAT_TITLE),
                MB_OK | MB_ICONINFORMATION);
}

static void DoRawApply() {
    wchar_t buf[16];
    GetWindowTextW(g_hRawEdit, buf, 16);
    int v = _wtoi(buf);
    if (v < 0 || v > 255) {
        MessageBoxW(g_hMain, lang::T(lang::RAW_RANGE), lang::T(lang::RAW_TITLE),
                    MB_OK | MB_ICONWARNING);
        return;
    }
    static cells::ObjectDef raw;
    raw.nameId = lang::RAW_TITLE; raw.key = L"RAW"; raw.codes = { (uint8_t)v };
    raw.footRows = 1; raw.footCols = 1; raw.color = cells::ColorForCode((uint8_t)v); raw.glyph = L"";
    g_selectedObject = &raw;
    g_eraserSelected = false;
    wchar_t msg[64];
    swprintf(msg, 64, lang::T(lang::RAW_SELECTED_FMT), v);
    SetWindowTextW(g_hStatus, msg);
}

// ---------------------------------------------------------------- Main window proc

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lp)->hInstance;

        BuildMenu(hwnd);

        g_hFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_hFontBold = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_hFontGlyph = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Consolas");

        // Top bar
        g_hPrevBtn = CreateWindowExW(0, L"BUTTON", lang::T(lang::BTN_PREV), WS_CHILD | WS_VISIBLE,
            8, 8, 90, 28, hwnd, (HMENU)ID_LEVEL_PREV, hInst, nullptr);
        g_hNextBtn = CreateWindowExW(0, L"BUTTON", lang::T(lang::BTN_NEXT), WS_CHILD | WS_VISIBLE,
            104, 8, 90, 28, hwnd, (HMENU)ID_LEVEL_NEXT, hInst, nullptr);
        g_hLevelLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            204, 14, 150, 20, hwnd, nullptr, hInst, nullptr);
        g_hLevelEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1", WS_CHILD | WS_VISIBLE | ES_NUMBER,
            350, 10, 45, 24, hwnd, nullptr, hInst, nullptr);
        g_hGoBtn = CreateWindowExW(0, L"BUTTON", lang::T(lang::BTN_GO), WS_CHILD | WS_VISIBLE,
            400, 8, 80, 28, hwnd, (HMENU)ID_LEVEL_GO, hInst, nullptr);
        g_hClearBtn = CreateWindowExW(0, L"BUTTON", lang::T(lang::BTN_CLEAR), WS_CHILD | WS_VISIBLE,
            488, 8, 140, 28, hwnd, (HMENU)ID_LEVEL_CLEAR, hInst, nullptr);

        for (HWND h : { g_hPrevBtn, g_hNextBtn, g_hLevelLabel, g_hLevelEdit, g_hGoBtn, g_hClearBtn })
            SendMessageW(h, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SendMessageW(g_hLevelLabel, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        // Second row: raw code entry (moved here from the bottom bar, where it used
        // to overlap the palette hint text).
        g_hRawLabel = CreateWindowExW(0, L"STATIC", lang::T(lang::LBL_RAW),
            WS_CHILD | WS_VISIBLE, 8, 46, 150, 20, hwnd, nullptr, hInst, nullptr);
        g_hRawEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_NUMBER,
            160, 44, 50, 22, hwnd, nullptr, hInst, nullptr);
        g_hApplyBtn = CreateWindowExW(0, L"BUTTON", lang::T(lang::BTN_APPLY), WS_CHILD | WS_VISIBLE,
            216, 42, 80, 26, hwnd, (HMENU)ID_RAW_APPLY, hInst, nullptr);

        // Palette (left)
        auto& pal = cells::Palette();
        int y = 78;
        for (size_t i = 0; i < pal.size(); i++) {
            g_hPalBtn[i] = CreateWindowExW(0, L"BUTTON",
                (std::wstring(pal[i].key) + L": " + cells::NameOf(&pal[i])).c_str(),
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                8, y, 178, 36, hwnd, (HMENU)(ID_PALETTE_BASE + i), hInst, nullptr);
            y += 42;
        }
        g_hPalBtn[10] = CreateWindowExW(0, L"BUTTON", lang::T(lang::BTN_ERASER),
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            8, y, 178, 36, hwnd, (HMENU)(ID_PALETTE_BASE + 10), hInst, nullptr);
        y += 46;
        g_hHint = CreateWindowExW(0, L"STATIC", lang::T(lang::HINT),
            WS_CHILD | WS_VISIBLE, 8, y, 186, 200, hwnd, nullptr, hInst, nullptr);
        SendMessageW(g_hHint, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Grid canvas
        WNDCLASSW gwc = {};
        gwc.lpfnWndProc = GridWndProc;
        gwc.hInstance = hInst;
        gwc.lpszClassName = L"PopCornGridCanvas";
        gwc.hCursor = LoadCursorW(nullptr, IDC_CROSS);
        gwc.hbrBackground = nullptr;
        RegisterClassW(&gwc);
        g_hGrid = CreateWindowExW(0, L"PopCornGridCanvas", nullptr,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            196, 78, 500, 500, hwnd, nullptr, hInst, nullptr);

        // Bottom status bar (repositioned to the true bottom edge on WM_SIZE)
        g_hStatus = CreateWindowExW(0, L"STATIC", lang::T(lang::ST_HOVER),
            WS_CHILD | WS_VISIBLE, 8, 8, 900, 22, hwnd, nullptr, hInst, nullptr);
        for (HWND h : { g_hStatus, g_hRawLabel, g_hRawEdit, g_hApplyBtn })
            SendMessageW(h, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        return 0;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lp;
        int id = dis->CtlID;
        if (id >= ID_PALETTE_BASE && id <= ID_PALETTE_BASE + 10) {
            COLORREF color = RGB(200, 200, 200);
            bool selected = false;
            if (id == ID_PALETTE_BASE + 10) {
                color = RGB(230, 230, 230);
                selected = g_eraserSelected;
            } else {
                auto& pal = cells::Palette();
                size_t idx = id - ID_PALETTE_BASE;
                color = pal[idx].color;
                selected = (!g_eraserSelected && g_selectedObject == &pal[idx]);
            }
            HBRUSH br = CreateSolidBrush(color);
            FillRect(dis->hDC, &dis->rcItem, br);
            DeleteObject(br);

            HBRUSH frameBrush = (HBRUSH)GetStockObject(selected ? WHITE_BRUSH : BLACK_BRUSH);
            RECT r = dis->rcItem;
            for (int i = 0; i < (selected ? 3 : 1); i++) {
                FrameRect(dis->hDC, &r, frameBrush);
                InflateRect(&r, -1, -1);
            }

            wchar_t text[128];
            GetWindowTextW(dis->hwndItem, text, 128);
            RECT textRect = dis->rcItem;

            // В графическом режиме показываем на кнопке настоящий спрайт блока,
            // чтобы палитра выглядела так же, как поле.
            if (g_spriteMode && id != ID_PALETTE_BASE + 10) {
                auto& pal = cells::Palette();
                size_t idx = id - ID_PALETTE_BASE;
                if (idx < pal.size() && !pal[idx].codes.empty()) {
                    int h = dis->rcItem.bottom - dis->rcItem.top;
                    int sh = h - 10, sw = sh * 5 / 3;      // авторская пропорция 5:3
                    DrawSpriteInto(dis->hDC, pal[idx].codes[0],
                                   dis->rcItem.left + 5, dis->rcItem.top + 5, sw, sh);
                    textRect.left += sw + 10;
                }
            }

            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, RGB(0, 0, 0));
            HFONT old = (HFONT)SelectObject(dis->hDC, g_hFont);
            DrawTextW(dis->hDC, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dis->hDC, old);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id >= ID_PALETTE_BASE && id <= ID_PALETTE_BASE + 9) {
            SelectPaletteIndex(id - ID_PALETTE_BASE);
            return 0;
        }
        if (id == ID_PALETTE_BASE + 10) {
            SelectEraser();
            return 0;
        }
        switch (id) {
        case ID_FILE_NEW: DoNew(); break;
        case ID_FILE_OPEN: DoOpen(); break;
        case ID_FILE_SAVE: DoSave(); break;
        case ID_FILE_SAVEAS: DoSaveAs(); break;
        case ID_FILE_EXIT: DestroyWindow(hwnd); break;
        case ID_LEVEL_PREV: ChangeLevel(-1); break;
        case ID_LEVEL_NEXT: ChangeLevel(1); break;
        case ID_LEVEL_CLEAR: DoClearLevel(); break;
        case ID_LEVEL_COPY: DoCopyLevelTo(); break;
        case ID_LEVEL_GO: {
            wchar_t buf[16];
            GetWindowTextW(g_hLevelEdit, buf, 16);
            GoToLevel(_wtoi(buf) - 1);
            break;
        }
        case ID_FILE_IMPORT_EXE: DoImportExe(); break;
        case ID_FILE_EXPORT_EXE: DoExportExe(); break;
        case ID_VIEW_SPRITES: SetViewMode(true); break;
        case ID_LANG_EN: lang::g_lang = lang::EN; ApplyLanguage(hwnd); break;
        case ID_LANG_RU: lang::g_lang = lang::RU; ApplyLanguage(hwnd); break;
        case ID_VIEW_SCHEMA: SetViewMode(false); break;
        case ID_HELP_ABOUT: DoAbout(); break;
        case ID_HELP_FORMAT: DoFormatHelp(); break;
        case ID_RAW_APPLY: DoRawApply(); break;
        }
        // Repaint all palette buttons so the "selected" highlight updates.
        for (int i = 0; i <= 10; i++)
            InvalidateRect(GetDlgItem(hwnd, ID_PALETTE_BASE + i), nullptr, TRUE);
        return 0;
    }

    case WM_SIZE: {
        RECT rc; GetClientRect(hwnd, &rc);
        int w = rc.right, h = rc.bottom;
        MoveWindow(g_hGrid, 196, 78, w - 206, h - 118, TRUE);
        MoveWindow(g_hStatus, 8, h - 30, w - 16, 22, TRUE);
        return 0;
    }

    case WM_CLOSE:
        if (g_dirty) {
            int res = MessageBoxW(hwnd, lang::T(lang::MSG_SAVE_BEFORE_EXIT),
                L"POP-CORN Level Editor 1.0", MB_YESNOCANCEL | MB_ICONQUESTION);
            if (res == IDCANCEL) return 0;
            if (res == IDYES) { DoSave(); if (g_dirty) return 0; }
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------- Entry point

// Открыть файл, переданный в командной строке или брошенный на .exe редактора:
// .PPC загружается как банк, POPCORN.EXE — импортируется через EXEPACK.
static void OpenPathFromCommandLine(const std::wstring& path) {
    std::wstring lower = path;
    for (auto& ch : lower) ch = (wchar_t)towlower(ch);
    std::wstring err;
    if (lower.size() > 4 && lower.compare(lower.size() - 4, 4, L".exe") == 0) {
        std::vector<uint8_t> exe, img;
        popexe::Layout lay;
        if (!popexe::ReadFileBytes(path, exe, err) || !popexe::ReadImage(exe, img, lay, err)) {
            MessageBoxW(g_hMain, err.c_str(), lang::T(lang::T_IMPORT_ERR), MB_OK | MB_ICONERROR);
            return;
        }
        popexe::ImageToBank(img, g_bank);
        SetWindowTextW(g_hStatus, lang::T(lang::ST_IMPORTED));
    } else {
        if (!g_bank.Load(path, err)) {
            MessageBoxW(g_hMain, err.c_str(), lang::T(lang::ERR_OPEN_TITLE), MB_OK | MB_ICONERROR);
            return;
        }
        g_currentPath = path;
    }
    g_currentLevel = 0;
    g_dirty = false;
    UpdateTitleAndLabel();
    InvalidateGrid();
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow) {
    g_bank.Reset();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"PopCornEditorMain";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    RegisterClassW(&wc);

    g_hMain = CreateWindowExW(0, L"PopCornEditorMain",
        L"POP-CORN Level Editor (unofficial) - Уровень 1",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1060, 860,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(g_hMain, nCmdShow);
    UpdateWindow(g_hMain);
    UpdateTitleAndLabel();

    if (lpCmdLine && *lpCmdLine) {
        std::wstring arg = lpCmdLine;
        if (!arg.empty() && arg.front() == L'"') {          // снять кавычки
            arg.erase(0, 1);
            size_t q = arg.find(L'"');
            if (q != std::wstring::npos) arg.erase(q);
        }
        OpenPathFromCommandLine(arg);
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
