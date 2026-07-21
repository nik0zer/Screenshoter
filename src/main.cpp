#define _WIN32_WINNT 0x0A00
#define WIN32_LEAN_AND_MEAN // Уменьшает размер заголовков Windows
#include <windows.h>
#include <objidl.h>         // Для PROPID и др., нужно перед gdiplus.h
#include <gdiplus.h>       // Для сохранения в PNG
#include <ShellScalingApi.h> // Для SetProcessDpiAwarenessContext (улучшение качества)
#include <string>
#include <filesystem>     // Требуется C++17
#include <chrono>
#include <ctime>

// Линкуем необходимые библиотеки
#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib,"Shcore.lib") // Необходимо для SetProcessDpiAwarenessContext

// --- Константы ---
const int HOTKEY_SCREENSHOT_ID = 1;
const int HOTKEY_EXIT_ID = 2;
const int HOTKEY_AUTO_ID = 3;              // Переключение автоматического режима
const UINT_PTR AUTO_TIMER_ID = 1;          // Идентификатор таймера авто-скриншотов
const UINT AUTO_INTERVAL_MS = 60 * 1000;   // Интервал авто-скриншотов: 1 минута
const std::wstring SAVE_DIRECTORY = L"C:\\SS"; // Папка для сохранения

// --- Прототипы функций ---
std::wstring GenerateUniqueFilename();
bool SaveBitmapToPNG(HBITMAP hBitmap, const std::wstring& filename);
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid); // Вспомогательная функция для GDI+
void TakeScreenshotAndSave();

// --- Точка входа ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // --- Установка DPI Awareness (ВАЖНО: делать в самом начале!) ---
    // Это необходимо, чтобы GetSystemMetrics и BitBlt работали с реальными
    // физическими пикселями на экранах с высоким разрешением и масштабированием.
    // Без этого скриншоты на масштабированных дисплеях будут размытыми.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    // ----------------------------------------------------------------

    // Инициализация GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) {
        return 1; // Тихий выход при ошибке GDI+
    }

    // 1. Проверяем и создаем папку для сохранения (без MessageBox)
    try {
        if (!std::filesystem::exists(SAVE_DIRECTORY)) {
            if (!std::filesystem::create_directories(SAVE_DIRECTORY)) {
                 Gdiplus::GdiplusShutdown(gdiplusToken);
                 return 1; // Тихий выход при ошибке создания папки
            }
        }
    } catch (const std::filesystem::filesystem_error& ) {
        // Тихий выход при ошибке файловой системы
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    // 2. Регистрация горячих клавиш (без MessageBox)
    // Ctrl+Shift+' (апостроф/одинарная кавычка). VK_OEM_7
    if (!RegisterHotKey(NULL, HOTKEY_SCREENSHOT_ID, MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, VK_OEM_7)) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1; // Тихий выход при ошибке регистрации
    }
    // Ctrl+Shift+, (запятая). VK_OEM_COMMA
    if (!RegisterHotKey(NULL, HOTKEY_EXIT_ID, MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, VK_OEM_COMMA)) {
        UnregisterHotKey(NULL, HOTKEY_SCREENSHOT_ID); // Отменяем предыдущую
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1; // Тихий выход при ошибке регистрации
    }
    // Ctrl+Shift+Z. Переключение автоматического режима (скриншот раз в минуту).
    if (!RegisterHotKey(NULL, HOTKEY_AUTO_ID, MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'Z')) {
        UnregisterHotKey(NULL, HOTKEY_SCREENSHOT_ID);
        UnregisterHotKey(NULL, HOTKEY_EXIT_ID);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1; // Тихий выход при ошибке регистрации
    }

    // 3. Цикл обработки сообщений
    bool autoModeEnabled = false; // Состояние автоматического режима
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        // Необязательно для WM_HOTKEY, но стандартная практика
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_HOTKEY)
        {
            if (msg.wParam == HOTKEY_SCREENSHOT_ID)
            {
                TakeScreenshotAndSave();
            }
            else if (msg.wParam == HOTKEY_AUTO_ID)
            {
                // Переключаем автоматический режим (скриншот раз в минуту)
                if (!autoModeEnabled)
                {
                    // Таймер без окна: WM_TIMER придёт в очередь потока
                    if (SetTimer(NULL, AUTO_TIMER_ID, AUTO_INTERVAL_MS, NULL) != 0)
                    {
                        autoModeEnabled = true;
                        TakeScreenshotAndSave(); // Первый кадр сразу при включении
                    }
                }
                else
                {
                    KillTimer(NULL, AUTO_TIMER_ID);
                    autoModeEnabled = false;
                }
            }
            else if (msg.wParam == HOTKEY_EXIT_ID)
            {
                if (autoModeEnabled)
                {
                    KillTimer(NULL, AUTO_TIMER_ID);
                    autoModeEnabled = false;
                }
                PostQuitMessage(0); // Завершаем цикл сообщений
            }
        }
        else if (msg.message == WM_TIMER && msg.wParam == AUTO_TIMER_ID)
        {
            // Срабатывание таймера автоматического режима
            TakeScreenshotAndSave();
        }
    }

    // 4. Отмена регистрации горячих клавиш и таймера перед выходом
    if (autoModeEnabled) KillTimer(NULL, AUTO_TIMER_ID);
    UnregisterHotKey(NULL, HOTKEY_SCREENSHOT_ID);
    UnregisterHotKey(NULL, HOTKEY_EXIT_ID);
    UnregisterHotKey(NULL, HOTKEY_AUTO_ID);

    // Завершение работы с GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);

    return (int)msg.wParam;
}

// --- Реализация функций ---
// Генерирует уникальное имя файла на основе времени с расширением .png
std::wstring GenerateUniqueFilename() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
    localtime_s(&now_tm, &now_c); // Потокобезопасная версия

    wchar_t buffer[100];
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    // Формат: Screenshot_YYYYMMDD_HHMMSS_ms.png
    swprintf_s(buffer, sizeof(buffer)/sizeof(wchar_t), L"Screenshot_%04d%02d%02d_%02d%02d%02d_%03lld.png",
               now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday,
               now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec, ms.count());

    return std::wstring(buffer);
}

// Вспомогательная функция для получения CLSID кодировщика GDI+
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;  // number of image encoders
    UINT size = 0; // size of the image encoder array in bytes

    Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1; // Failure

    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1; // Failure

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j; // Success
        }
    }

    free(pImageCodecInfo);
    return -1; // Failure
}

// Сохраняет HBITMAP в PNG файл с использованием GDI+
bool SaveBitmapToPNG(HBITMAP hBitmap, const std::wstring& filename) {
    if (!hBitmap) return false;

    Gdiplus::Bitmap bitmap(hBitmap, NULL);
    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        return false;
    }

    CLSID pngClsid;
    // Получаем CLSID для PNG кодировщика
    if (GetEncoderClsid(L"image/png", &pngClsid) < 0) {
        return false;
    }

    // Сохраняем изображение
    Gdiplus::Status status = bitmap.Save(filename.c_str(), &pngClsid, NULL);

    return (status == Gdiplus::Ok);
}

// Функция захвата и сохранения скриншота
void TakeScreenshotAndSave() {
    HDC hScreenDC = NULL, hMemoryDC = NULL;
    HBITMAP hBitmap = NULL, hOldBitmap = NULL;
    int screenWidth = 0, screenHeight = 0;

    try {
        // 1. Получаем РЕАЛЬНЫЕ размеры экрана (благодаря DPI Awareness)
        screenWidth = GetSystemMetrics(SM_CXSCREEN);
        screenHeight = GetSystemMetrics(SM_CYSCREEN);
        if (screenWidth == 0 || screenHeight == 0) throw std::runtime_error("Не удалось получить размеры экрана");

        // 2. Получаем Device Context экрана
        hScreenDC = GetDC(NULL);
        if (!hScreenDC) throw std::runtime_error("Не удалось получить Screen DC");

        // 3. Создаем совместимый DC в памяти
        hMemoryDC = CreateCompatibleDC(hScreenDC);
        if (!hMemoryDC) throw std::runtime_error("Не удалось создать Memory DC");

        // 4. Создаем совместимый Bitmap для скриншота (реального размера)
        hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
        if (!hBitmap) throw std::runtime_error("Не удалось создать Bitmap");

        // 5. Выбираем наш Bitmap в Memory DC
        hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);
        if (!hOldBitmap) throw std::runtime_error("Не удалось выбрать Bitmap в DC");

        // 6. Копируем изображение с экрана в Memory DC (пиксель в пиксель)
        if (!BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY)) {
            throw std::runtime_error("Не удалось выполнить BitBlt");
        }

        // --- Восстанавливаем старый битмап ДО сохранения ---
        SelectObject(hMemoryDC, hOldBitmap);
        hOldBitmap = NULL; // Чтобы не пытаться выбрать еще раз в блоке finally

        // 7. Генерируем путь к файлу
        std::wstring filename = SAVE_DIRECTORY + L"\\" + GenerateUniqueFilename();
        // 8. Сохраняем Bitmap в PNG файл
        if (!SaveBitmapToPNG(hBitmap, filename)) {
            throw std::runtime_error("Не удалось сохранить Bitmap в PNG файл");
        }

    } catch (const std::runtime_error& ) {
        // Ошибку игнорируем для скрытности
        // OutputDebugStringA(e.what()); // Можно раскомментировать для отладки
    }

    // 9. Очистка ресурсов GDI
    if (hOldBitmap) SelectObject(hMemoryDC, hOldBitmap); // На всякий случай
    if (hBitmap) DeleteObject(hBitmap);
    if (hMemoryDC) DeleteDC(hMemoryDC);
    if (hScreenDC) ReleaseDC(NULL, hScreenDC);
    // Никакой индикации успеха/неудачи
}

// g++ main.cpp -o UpdateServiceScreen.exe -lgdiplus -lshcore -luser32 -lgdi32 -mwindows -static -std=c++17