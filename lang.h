#pragma once
// Локализация интерфейса. / UI localization.
// Порядок строк в обеих таблицах ОБЯЗАН совпадать с порядком в enum Id.
// The order of strings in both tables MUST match the order of enum Id.

namespace lang {

enum Lang { EN = 0, RU = 1 };
inline Lang g_lang = EN;          // по умолчанию английский / English by default

enum Id {
    // --- меню / menu
    MENU_FILE, MENU_LEVEL, MENU_VIEW, MENU_LANG, MENU_HELP,
    MI_NEW, MI_OPEN, MI_SAVE, MI_SAVEAS, MI_IMPORT_EXE, MI_EXPORT_EXE, MI_EXIT,
    MI_PREV, MI_NEXT, MI_CLEAR, MI_COPY,
    MI_SPRITES, MI_SCHEMA,
    MI_LANG_EN, MI_LANG_RU,
    MI_HELP_FORMAT, MI_HELP_ABOUT,
    // --- кнопки и подписи / buttons and labels
    BTN_PREV, BTN_NEXT, BTN_GO, BTN_CLEAR, LBL_RAW, BTN_APPLY, BTN_ERASER,
    LBL_LEVEL_FMT, TITLE_FMT, NEW_BANK, HINT,
    // --- строка состояния / status bar
    ST_HOVER, ST_CELL_FMT, ST_SELECTED_FMT, ST_ERASER, ST_SPRITE_MODE, ST_SCHEMA_MODE,
    ST_SAVED_FMT, ST_PATCHED_FMT, ST_IMPORTED, ST_NOFIT,
    // --- диалоги / dialogs
    T_WARNING, T_CONFIRM, T_DONE, T_ERROR,
    MSG_DISCARD, MSG_SAVE_BEFORE_EXIT,
    ERR_OPEN_TITLE, ERR_SAVE_TITLE,
    FILTER_PPC, FILTER_EXE,
    OFN_OPEN_PPC, OFN_SAVE_PPC,
    OFN_IMPORT_EXE, OFN_EXPORT_STEP1, OFN_EXPORT_STEP2,
    T_IMPORT_ERR, T_EXPORT_ERR,
    IMPORT_OK_TITLE, IMPORT_OK_FMT,
    EXPORT_OK_TITLE, EXPORT_OK_FMT,
    CLEAR_CONFIRM_FMT, COPY_TITLE, COPY_LABEL, COPY_DONE_FMT,
    BTN_OK, BTN_CANCEL,
    RAW_TITLE, RAW_RANGE, RAW_SELECTED_FMT,
    ABOUT_TITLE, ABOUT_BODY, FORMAT_TITLE, FORMAT_BODY,
    // --- клетки / cells
    CELL_EMPTY, CELL_UNKNOWN_FMT, CELL_PART_FMT,
    OBJ_F1, OBJ_F2, OBJ_F3, OBJ_F4, OBJ_F5, OBJ_F6, OBJ_F7, OBJ_F8,
    OBJ_TELEPORT, OBJ_PICTURE,
    // --- ошибки форматов / format errors
    E_CANNOT_OPEN, E_CANNOT_CREATE, E_READ_FAILED, E_WRITE_FAILED, E_EMPTY_FILE,
    E_PPC_SIZE, E_NO_SIGNATURE, E_BAD_STRUCT, E_STREAM_BROKEN, E_LIT_OOB,
    E_LAST_BLOCK, E_BAD_IMAGE, E_NO_ROOM, E_SIZE_FIT, E_INTERNAL_SIZE, E_VERIFY_FAILED,
    ID_COUNT
};

inline const wchar_t* const TABLE[2][ID_COUNT] = {
// ---------------------------------------------------------------- English
{
    L"File", L"Level", L"View", L"Language", L"Help",
    L"New bank (49 empty fields)", L"Open .PPC...\tCtrl+O", L"Save\tCtrl+S",
    L"Save as...", L"Import levels from POPCORN.EXE...",
    L"Export (patch) to POPCORN.EXE...", L"Exit",
    L"Previous (PgUp)", L"Next (PgDn)", L"Clear current field", L"Copy field to...",
    L"Graphics mode (sprites from the game)", L"Schematic mode (numeric codes)",
    L"English", L"Russian (Русский)",
    L"How the level format works", L"About",

    L"< Prev.", L"Next >", L"Go", L"Clear field", L"Raw code (0-255):", L"Select",
    L"Eraser (or RMB)",
    L"Level %d / %d",
    L"POP-CORN Level Editor 1.0 - %ls - Level %d%ls",
    L"(new bank)",
    L"Hold LMB to draw blocks like a pencil.\r\n"
    L"Hold RMB to erase the same way.\r\n\r\n"
    L"The \"Picture\" object (F10) covers a 2x3 block - click its top-left corner.\r\n\r\n"
    L"PgUp / PgDn - switch levels. F1..F10 - pick an object. Del - eraser.",

    L"Point at a cell of the field...",
    L"Cell (%d,%d): %ls",
    L"Selected: %ls",
    L"Eraser selected",
    L"Graphics mode: showing the real block sprites from the game.",
    L"Schematic mode: cells with numeric codes.",
    L"Saved: %ls",
    L"Game patched: %ls",
    L"Levels imported from the game .exe.",
    L"Does not fit here - too close to the edge of the field.",

    L"Warning", L"Confirm", L"Done", L"Error",
    L"The current bank has unsaved changes. Continue and lose them?",
    L"Save changes before exiting?",
    L"Cannot open file", L"Cannot save file",
    L"POP-CORN level bank (*.PPC)\0*.PPC\0All files (*.*)\0*.*\0",
    L"POP-CORN (POPCORN.EXE)\0*.EXE\0All files (*.*)\0*.*\0",
    L"Open a POP-CORN level bank", L"Save the POP-CORN level bank",
    L"Import levels from POPCORN.EXE",
    L"Step 1 of 2: choose the source POPCORN.EXE",
    L"Step 2 of 2: where to save the patched game",
    L"Import from .exe", L"Export to .exe",
    L"Import complete",
    L"All 49 levels loaded straight from the game.\n\n"
    L"The bank was found at file offset %u and unpacked from EXEPACK "
    L"(%u bytes of packed data).\n\n"
    L"You can now edit them and write them back via "
    L"\"File -> Export (patch) to POPCORN.EXE\".",
    L"Export complete",
    L"Done: %ls\n\n"
    L"The file size is unchanged (%u bytes) - that is mandatory, otherwise the "
    L"EXEPACK unpacker inside the game would report \"Packed file is corrupt\".\n\n"
    L"The written data was verified by unpacking it back: it matches byte for byte. "
    L"Ready to run.",
    L"Clear level %d completely?", L"Copy to...", L"Destination field number (1-49):",
    L"Level %d copied to %d.",
    L"OK", L"Cancel",
    L"Raw code", L"Enter a number from 0 to 255.", L"Raw code selected: %d",

    L"About",
    L"POP-CORN Level Editor 1.0\n"
    L"Unofficial level editor for POP-CORN (LACRAL software, 1988).\n\n"
    L"Author: DarkSoL (Discord: darksol41)\n"
    L"Made with the support of Claude AI.\n\n"
    L"Reads and writes .PPC level banks (the format of the original POPGEN editor) "
    L"and can also read levels straight from POPCORN.EXE and write them back.\n\n"
    L"The block sprites in graphics mode are the real ones, taken from the game.\n\n"
    L"Format details: Help -> How the level format works.",

    L"POP-CORN level format",
    L".PPC FILE (format of the original POPGEN editor)\n"
    L"6-byte \"LACRAL\" signature + 49 fields of 176 bytes. 8630 bytes total.\n\n"
    L"INSIDE POPCORN.EXE\n"
    L"The very same bank, but with a doubled signature (\"LACRALLACRAL\", 12 bytes), "
    L"packed with Microsoft EXEPACK together with the rest of the program. The editor "
    L"therefore unpacks and repacks the data with the EXEPACK algorithm, keeping the "
    L"file size unchanged.\n\n"
    L"ONE LEVEL RECORD - 176 bytes\n"
    L"byte 0: number of destructible bricks (the win condition);\n"
    L"byte 1: number of teleports (0..6);\n"
    L"bytes 2..7: teleport positions in the grid (0..167);\n"
    L"bytes 8..175: grid of 14 rows x 12 columns, one byte per cell.\n\n"
    L"The editor recalculates the header itself on export - no need to touch it.\n\n"
    L"CELL CODES\n"
    L"0 - empty;\n"
    L"1, 2, 5, 6, 7, 8 - destructible bricks F1, F2, F5, F6, F7, F8;\n"
    L"10 - destructible brick F4;\n"
    L"3 - INDESTRUCTIBLE block F3 (not counted as a brick);\n"
    L"9 - teleport F9;\n"
    L"16..21 - \"Picture\" block F10, 2 columns x 3 rows, destructible too.\n\n"
    L"That is the complete list: the original 49 levels contain no other cell codes. "
    L"The \"Raw code\" box is left in place for experiments.",

    L"empty", L"code %d (unknown)", L"%ls (part %d/%u)",
    L"Brick F1", L"Brick F2", L"Brick F3", L"Brick F4", L"Brick F5", L"Brick F6",
    L"Brick F7", L"Brick F8", L"Teleport (F9)", L"Picture (F10)",

    L"Cannot open the file", L"Cannot create the file", L"Read error", L"Write error",
    L"The file is empty",
    L"The file is not exactly 8630 bytes - not a valid POP-CORN .PPC bank",
    L"There is no \"LACRALLACRAL\" signature in this file - it is not POPCORN.EXE "
    L"(or it is an already unpacked build, where the bank is stored differently).",
    L"The structure after the signature is not the expected one - the file has been "
    L"modified, or this is a different build of the game.",
    L"Packed stream is broken: no command byte found",
    L"Packed stream is broken: literal block runs past the end",
    L"The last block runs past the end of the level bank",
    L"Wrong level bank image size",
    L"The levels do not fit into the space reserved inside the .exe. Simplify the "
    L"fields (long runs of identical cells compress better).",
    L"Could not match the required data block size",
    L"Internal error: size mismatch",
    L"Verification after writing failed - the file was NOT saved. "
},
// ---------------------------------------------------------------- Русский
{
    L"Файл", L"Уровень", L"Вид", L"Язык", L"Справка",
    L"Новый банк (49 пустых полей)", L"Открыть .PPC...\tCtrl+O", L"Сохранить\tCtrl+S",
    L"Сохранить как...", L"Импорт уровней из POPCORN.EXE...",
    L"Экспорт (патч) в POPCORN.EXE...", L"Выход",
    L"Предыдущее (PgUp)", L"Следующее (PgDn)", L"Очистить текущее поле",
    L"Копировать поле в...",
    L"Графический режим (спрайты из игры)", L"Схематический режим (числовые коды)",
    L"English", L"Русский",
    L"Как устроен формат уровней", L"О программе",

    L"< Пред.", L"След. >", L"Перейти", L"Очистить поле", L"Raw code (0-255):",
    L"Выбрать", L"Ластик (или ПКМ)",
    L"Уровень %d / %d",
    L"POP-CORN Level Editor 1.0 - %ls - Уровень %d%ls",
    L"(новый банк)",
    L"Зажмите ЛКМ, чтобы рисовать блоки как карандашом.\r\n"
    L"Зажмите ПКМ, чтобы так же стирать.\r\n\r\n"
    L"«Картинка» (F10) занимает блок 2x3 - кликните левый верхний угол области.\r\n\r\n"
    L"PgUp / PgDn - листать уровни. F1..F10 - выбрать объект. Del - ластик.",

    L"Наведите курсор на клетку поля...",
    L"Клетка (%d,%d): %ls",
    L"Выбрано: %ls",
    L"Выбран ластик",
    L"Графический режим: показаны настоящие спрайты блоков из игры.",
    L"Схематический режим: клетки с числовыми кодами.",
    L"Сохранено: %ls",
    L"Игра пропатчена: %ls",
    L"Уровни импортированы из .exe игры.",
    L"Не помещается здесь - слишком близко к краю поля.",

    L"Внимание", L"Подтверждение", L"Готово", L"Ошибка",
    L"Текущий банк не сохранён. Продолжить и потерять изменения?",
    L"Сохранить изменения перед выходом?",
    L"Не удалось открыть файл", L"Не удалось сохранить файл",
    L"Банк уровней POP-CORN (*.PPC)\0*.PPC\0Все файлы (*.*)\0*.*\0",
    L"POP-CORN (POPCORN.EXE)\0*.EXE\0Все файлы (*.*)\0*.*\0",
    L"Открыть банк уровней POP-CORN", L"Сохранить банк уровней POP-CORN",
    L"Импорт уровней из POPCORN.EXE",
    L"Шаг 1 из 2: выберите исходный POPCORN.EXE",
    L"Шаг 2 из 2: куда сохранить пропатченную игру",
    L"Импорт из .exe", L"Экспорт в .exe",
    L"Импорт выполнен",
    L"Загружены все 49 уровней прямо из игры.\n\n"
    L"Банк найден в файле по смещению %u, распакован из EXEPACK "
    L"(%u байт сжатых данных).\n\n"
    L"Теперь их можно править и записать обратно через "
    L"«Файл -> Экспорт (патч) в POPCORN.EXE».",
    L"Экспорт выполнен",
    L"Готово: %ls\n\n"
    L"Размер файла не изменился (%u байт) - это обязательное условие, иначе "
    L"распаковщик EXEPACK внутри игры выдал бы «Packed file is corrupt».\n\n"
    L"Записанные данные проверены обратной распаковкой: совпадают байт в байт. "
    L"Можно запускать.",
    L"Очистить уровень %d полностью?", L"Копировать в...",
    L"Номер поля-получателя (1-49):",
    L"Уровень %d скопирован в %d.",
    L"OK", L"Отмена",
    L"Raw code", L"Введите число от 0 до 255.", L"Выбран произвольный код: %d",

    L"О программе",
    L"POP-CORN Level Editor 1.0\n"
    L"Неофициальный редактор уровней для POP-CORN (LACRAL software, 1988).\n\n"
    L"Автор: DarkSoL (Discord: darksol41)\n"
    L"Сделано с помощью поддержки ИИ Claude.\n\n"
    L"Умеет читать и писать банк .PPC (формат родного редактора POPGEN), а также "
    L"читать уровни напрямую из POPCORN.EXE и записывать их обратно.\n\n"
    L"Спрайты блоков в графическом режиме - настоящие, взяты из самой игры.\n\n"
    L"Подробности формата: «Справка -> Как устроен формат уровней».",

    L"Формат уровней POP-CORN",
    L"ФАЙЛ .PPC (формат родного редактора POPGEN)\n"
    L"6 байт подпись \"LACRAL\" + 49 полей по 176 байт. Всего 8630 байт.\n\n"
    L"ВНУТРИ POPCORN.EXE\n"
    L"Тот же самый банк, но подпись удвоена (\"LACRALLACRAL\", 12 байт), и всё это "
    L"запаковано Microsoft EXEPACK вместе с остальной программой. Поэтому редактор "
    L"распаковывает и запаковывает данные алгоритмом EXEPACK, сохраняя размер файла "
    L"неизменным.\n\n"
    L"ЗАПИСЬ ОДНОГО УРОВНЯ - 176 байт\n"
    L"байт 0: сколько на поле разрушаемых кирпичей (условие прохождения);\n"
    L"байт 1: сколько телепортов (0..6);\n"
    L"байты 2..7: позиции телепортов в сетке (0..167);\n"
    L"байты 8..175: сетка 14 строк x 12 столбцов, 1 байт на клетку.\n\n"
    L"Заголовок редактор пересчитывает сам при экспорте, вручную его трогать не нужно.\n\n"
    L"КОДЫ КЛЕТОК\n"
    L"0 - пусто;\n"
    L"1, 2, 5, 6, 7, 8 - разрушаемые кирпичи F1, F2, F5, F6, F7, F8;\n"
    L"10 - разрушаемый кирпич F4;\n"
    L"3 - НЕразрушаемый блок F3 (в счёт кирпичей не идёт);\n"
    L"9 - телепорт F9;\n"
    L"16..21 - блок «картинка» F10, 2 столбца x 3 строки, тоже разрушаемый.\n\n"
    L"Это полный список: других кодов клеток в оригинальных 49 уровнях нет. "
    L"Поле \"Raw code\" оставлено для экспериментов.",

    L"пусто", L"код %d (неизвестно)", L"%ls (часть %d/%u)",
    L"Кирпич F1", L"Кирпич F2", L"Кирпич F3", L"Кирпич F4", L"Кирпич F5", L"Кирпич F6",
    L"Кирпич F7", L"Кирпич F8", L"Телепорт (F9)", L"Картинка (F10)",

    L"Не удалось открыть файл", L"Не удалось создать файл", L"Ошибка чтения",
    L"Ошибка записи", L"Файл пуст",
    L"Файл не ровно 8630 байт - это не банк уровней POP-CORN .PPC",
    L"В файле нет подписи \"LACRALLACRAL\" - это не POPCORN.EXE (или уже "
    L"распакованная сборка, в которой банк лежит иначе).",
    L"Структура за подписью не та, что ожидалась - файл изменён или это другая "
    L"версия игры.",
    L"Поток повреждён: команда не найдена",
    L"Поток повреждён: литеральный блок за границей",
    L"Последний блок вышел за границу банка уровней",
    L"Неверный размер образа банка",
    L"Уровни не влезают в отведённое место в .exe. Упростите поля (больше "
    L"одинаковых подряд идущих клеток сжимается лучше).",
    L"Не удалось подогнать размер блока данных",
    L"Внутренняя ошибка: размер не совпал",
    L"Проверка после записи не прошла - файл НЕ сохранён. "
}
};

inline const wchar_t* T(Id id) { return TABLE[g_lang][id]; }

} // namespace lang
