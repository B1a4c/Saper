#include <windows.h>
#include <gdiplus.h>
#include <ctime>
#include <fstream>

#pragma comment(lib, "gdiplus.lib")

#define BUTTON_ID_BASE 1000
#define IDC_DIFFICULTY 2000 // ID для выпадающего списка
#define IDC_NEWGAME    3000 // ID для кнопки "Начать заново"

const int MAX_ROWS = 15;
const int MAX_COLS = 15;

int rows = 5; // Количество строк
int cols = 5; // Количество столбцов
int mines = 5; // Количество мин

bool mineField[MAX_ROWS][MAX_COLS];
int adjacentMines[MAX_ROWS][MAX_COLS];
HWND buttons[MAX_ROWS][MAX_COLS];
HWND statsLabel;           // Элемент управления для отображения статистики
HWND difficultyComboBox;   // Выпадающий список для выбора уровня сложности
HWND gifControl;           // Элемент управления для отображения GIF
bool gameOver = false;

int wins = 0;   // Количество выигранных игр
int losses = 0; // Количество проигранных игр

const char* statsFile = "stats.bin"; // Имя файла для статистики

void LoadStatistics() {
    std::ifstream file(statsFile, std::ios::binary);
    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(&wins), sizeof(wins));
        file.read(reinterpret_cast<char*>(&losses), sizeof(losses));
        file.close();
    }
}

void SaveStatistics() {
    std::ofstream file(statsFile, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&wins), sizeof(wins));
        file.write(reinterpret_cast<const char*>(&losses), sizeof(losses));
        file.close();
    }
}

void UpdateStatisticsDisplay() {
    wchar_t statsText[100];
    wsprintf(statsText, L"Выигрыши: %d\nПроигрыши: %d", wins, losses);
    SetWindowText(statsLabel, statsText);
}

void ShowMines(bool won) {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (mineField[r][c]) {
                HWND button = buttons[r][c];
                if (button) {
                    SetWindowText(button, L"X");
                    HBRUSH brush = CreateSolidBrush(won ? RGB(0, 255, 0) : RGB(255, 0, 0));
                    SetClassLongPtr(button, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    InvalidateRect(button, NULL, TRUE);
                }
            }
        }
    }
}

void InitializeGame(HWND hWnd) {
    // Очистка предыдущего состояния
    for (int r = 0; r < MAX_ROWS; r++) {
        for (int c = 0; c < MAX_COLS; c++) {
            mineField[r][c] = false;
            adjacentMines[r][c] = 0;
            if (buttons[r][c]) {
                DestroyWindow(buttons[r][c]);
                buttons[r][c] = NULL;
            }
        }
    }
    gameOver = false;

    // Расставляем мины случайным образом
    srand(static_cast<unsigned>(time(0)));
    int placedMines = 0;
    while (placedMines < mines) {
        int r = rand() % rows;
        int c = rand() % cols;
        if (!mineField[r][c]) {
            mineField[r][c] = true;
            placedMines++;
        }
    }

    // Считаем количество соседних мин
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (mineField[r][c]) continue;
            int count = 0;
            for (int dr = -1; dr <= 1; dr++) {
                for (int dc = -1; dc <= 1; dc++) {
                    int nr = r + dr, nc = c + dc;
                    if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && mineField[nr][nc]) {
                        count++;
                    }
                }
            }
            adjacentMines[r][c] = count;
        }
    }

    // Создаём кнопки игрового поля
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            buttons[r][c] = CreateWindow(
                L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                50 + c * 30, 120 + r * 30, 30, 30,
                hWnd, (HMENU)(BUTTON_ID_BASE + r * cols + c), GetModuleHandle(NULL), NULL);

            // Устанавливаем чёрный цвет для всех ячеек
            HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0)); // Чёрный цвет
            SetClassLongPtr(buttons[r][c], GCLP_HBRBACKGROUND, (LONG_PTR)brush);
        }
    }

    // Обновляем статистику
    UpdateStatisticsDisplay();
}

void ClearField(HWND hWnd) {
    // Удаляем все кнопки
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (buttons[r][c]) {
                DestroyWindow(buttons[r][c]);
                buttons[r][c] = NULL;
            }
        }
    }
}

void RevealCell(HWND hWnd, int r, int c) {
    if (r < 0 || r >= rows || c < 0 || c >= cols || !buttons[r][c] || gameOver)
        return;

    HWND button = buttons[r][c];
    buttons[r][c] = NULL; // Помечаем клетку как раскрытую

    if (mineField[r][c]) {
        SetWindowText(button, L"X");
        gameOver = true;
        losses++;
        SaveStatistics();
        UpdateStatisticsDisplay();
        ShowMines(false); // Показываем мины после поражения
        MessageBox(hWnd, L"Вы проиграли!", L"Игра окончена", MB_OK);
        return;
    }

    int count = adjacentMines[r][c];
    if (count > 0) {
        wchar_t text[2];
        wsprintf(text, L"%d", count);
        SetWindowText(button, text);
    }
    else {
        SetWindowText(button, L"");
        // Автоматическое раскрытие соседних клеток
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                RevealCell(hWnd, r + dr, c + dc);
            }
        }
    }

    // Проверяем, выиграл ли игрок
    bool won = true;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (!mineField[r][c] && buttons[r][c]) {
                won = false;
                break;
            }
        }
        if (!won)
            break;
    }

    if (won) {
        gameOver = true;
        wins++;
        SaveStatistics();
        UpdateStatisticsDisplay();
        ShowMines(true); // Показываем мины после победы
        MessageBox(hWnd, L"Вы выиграли!", L"Поздравляем", MB_OK);
    }
}

void SetDifficulty(HWND hWnd) {
    int selectedIndex = SendMessage(difficultyComboBox, CB_GETCURSEL, 0, 0);
    ClearField(hWnd);

    switch (selectedIndex) {
    case 0: // Лёгкий
        rows = 5;
        cols = 5;
        mines = 5;
        break;
    case 1: // Средний
        rows = 10;
        cols = 10;
        mines = 20;
        break;
    case 2: // Сложный
        rows = 15;
        cols = 15;
        mines = 40;
        break;
    }

    InitializeGame(hWnd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // Загружаем статистику
        LoadStatistics();

        // Создаём элемент управления для статистики
        statsLabel = CreateWindow(
            L"STATIC", L"", WS_VISIBLE | WS_CHILD,
            50, 20, 200, 50,
            hWnd, NULL, GetModuleHandle(NULL), NULL);

        // Создаём выпадающий список для выбора уровня сложности
        difficultyComboBox = CreateWindow(
            L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
            300, 20, 150, 100,
            hWnd, (HMENU)IDC_DIFFICULTY, GetModuleHandle(NULL), NULL);

        // Добавляем элементы в выпадающий список
        SendMessage(difficultyComboBox, CB_ADDSTRING, 0, (LPARAM)L"Лёгкий");
        SendMessage(difficultyComboBox, CB_ADDSTRING, 0, (LPARAM)L"Средний");
        SendMessage(difficultyComboBox, CB_ADDSTRING, 0, (LPARAM)L"Сложный");
        SendMessage(difficultyComboBox, CB_SETCURSEL, 0, 0); // По умолчанию "Лёгкий"

        // Создаём кнопку "Начать заново"
        CreateWindow(
            L"BUTTON", L"Начать заново", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            300, 60, 150, 30, hWnd, (HMENU)IDC_NEWGAME, GetModuleHandle(NULL), NULL);

        // Создаём элемент управления для отображения GIF
        gifControl = CreateWindow(
            L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP,
            600, 120, 150, 150, hWnd, NULL, GetModuleHandle(NULL), NULL);

        // Загружаем GIF (пример: gif.bmp)
        HBITMAP hGif = (HBITMAP)LoadImage(NULL, L"example.gif", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        if (hGif) {
            SendMessage(gifControl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hGif);
        }

        InitializeGame(hWnd);
        break;
    }

    case WM_COMMAND: {
        if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_DIFFICULTY) {
            SetDifficulty(hWnd);
        }
        else if (LOWORD(wParam) == IDC_NEWGAME) {
            // Кнопка "Начать заново" сбрасывает игру с текущим уровнем
            SetDifficulty(hWnd);
        }
        else if (LOWORD(wParam) >= BUTTON_ID_BASE && LOWORD(wParam) < BUTTON_ID_BASE + rows * cols) {
            int index = LOWORD(wParam) - BUTTON_ID_BASE;
            int r = index / cols;
            int c = index % cols;
            RevealCell(hWnd, r, c);
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"Minesweeper";

    RegisterClass(&wc);

    HWND hWnd = CreateWindow(
        L"Minesweeper", L"Сапёр",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    if (!hWnd)
        return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}
