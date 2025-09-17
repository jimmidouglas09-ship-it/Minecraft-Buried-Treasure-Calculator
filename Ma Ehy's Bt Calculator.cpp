#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <memory>
#include <cfloat>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

// Forward declarations
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK OverlayProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
ATOM MyRegisterOverlayClass(HINSTANCE hInstance);
BOOL InitOverlay(HINSTANCE hInstance);
void UpdateOverlay();
void ShowOverlay();
void HideOverlay();
void SaveSettings();
void LoadSettings();
std::wstring GetSettingsFilePath();

// Structure to hold coordinates
struct Vec3 {
    int x, y, z;
};

// Structure for treasure locations with distance
struct TreasureLocation {
    int x, z;
    double distance;
};

// Global variables
Vec3 currentCoords = { 0, 0, 0 };
std::vector<TreasureLocation> nearestTreasures;
int currentHotkey = VK_F5;
int overlayToggleHotkey = VK_F6;
bool waitingForHotkey = false;
bool waitingForOverlayHotkey = false;

// Overlay variables
WCHAR szOverlayClass[] = L"TreasureOverlayClass";
HWND hOverlayWnd = NULL;
bool overlayVisible = false;
bool isDragging = false;
POINT dragOffset = { 0, 0 };

// Global variables for main window
HWND g_hMainWnd = NULL;
HHOOK g_hKeyboardHook = NULL;

// Buried treasure coordinates
std::vector<int> positiveCoords = {
    8, 24, 72, 88, 136, 152, 200, 216, 264, 280, 328, 344, 392, 408, 456, 472,
    520, 536, 584, 600, 648, 664, 712, 728, 776, 792, 840, 856, 904, 920, 968,
    984, 1032, 1048, 1096, 1112, 1160, 1176, 1224, 1240, 1288, 1304, 1352, 1368,
    1416, 1432, 1480, 1496, 1544, 1560, 1608, 1624, 1672, 1688, 1736, 1752, 1800,
    1816, 1864, 1880, 1928, 1944, 1992, 2008, 2056, 2072, 2120, 2136, 2184, 2200,
    2248, 2264, 2312, 2328, 2376, 2392, 2440, 2508
};

std::vector<int> negativeCoords = {
    -40, -56, -104, -120, -168, -184, -232, -248, -296, -312, -360, -376, -424,
    -440, -488, -504, -552, -568, -616, -632, -680, -696, -744, -760, -808, -824,
    -872, -888, -936, -952, -1000, -1016, -1064, -1080, -1128, -1144, -1192, -1208,
    -1256, -1272, -1320, -1336, -1384, -1400, -1448, -1464, -1512, -1528, -1576,
    -1592, -1640, -1656, -1704, -1720, -1768, -1784, -1832, -1848, -1896, -1912,
    -1960, -1976, -2024, -2040, -2088, -2104, -2152, -2168, -2216, -2232, -2280,
    -2296, -2344, -2360, -2408, -2424, -2472, -2488, -2536, -2552, -2600, -2616,
    -2664, -2680, -2728, -2744, -2792, -2808
};

// Settings functions
std::wstring GetSettingsFilePath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        path = path.substr(0, pos + 1);
    }
    return path + L"treasure_finder_settings.ini";
}

void SaveSettings() {
    std::wstring settingsFile = GetSettingsFilePath();

    WritePrivateProfileString(L"Hotkeys", L"FindTreasure", std::to_wstring(currentHotkey).c_str(), settingsFile.c_str());
    WritePrivateProfileString(L"Hotkeys", L"ToggleOverlay", std::to_wstring(overlayToggleHotkey).c_str(), settingsFile.c_str());

    // Save overlay position and visibility
    if (hOverlayWnd) {
        RECT rect;
        GetWindowRect(hOverlayWnd, &rect);
        WritePrivateProfileString(L"Overlay", L"X", std::to_wstring(rect.left).c_str(), settingsFile.c_str());
        WritePrivateProfileString(L"Overlay", L"Y", std::to_wstring(rect.top).c_str(), settingsFile.c_str());
        WritePrivateProfileString(L"Overlay", L"Visible", overlayVisible ? L"1" : L"0", settingsFile.c_str());
    }
}

void LoadSettings() {
    std::wstring settingsFile = GetSettingsFilePath();

    currentHotkey = GetPrivateProfileInt(L"Hotkeys", L"FindTreasure", VK_F5, settingsFile.c_str());
    overlayToggleHotkey = GetPrivateProfileInt(L"Hotkeys", L"ToggleOverlay", VK_F6, settingsFile.c_str());

    // Validate hotkeys are different
    if (currentHotkey == overlayToggleHotkey) {
        overlayToggleHotkey = VK_F6;
        currentHotkey = VK_F5;
        SaveSettings(); // Save corrected values
    }
}

// Low-level keyboard hook procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;

        if (waitingForHotkey || waitingForOverlayHotkey) {
            int newKey = (int)pKeyboard->vkCode;

            if (newKey != VK_ESCAPE) {
                if (waitingForHotkey && newKey != overlayToggleHotkey) {
                    UnregisterHotKey(g_hMainWnd, 1);
                    currentHotkey = newKey;
                    RegisterHotKey(g_hMainWnd, 1, 0, currentHotkey);
                    SaveSettings();
                    waitingForHotkey = false;
                }
                else if (waitingForOverlayHotkey && newKey != currentHotkey) {
                    UnregisterHotKey(g_hMainWnd, 2);
                    overlayToggleHotkey = newKey;
                    RegisterHotKey(g_hMainWnd, 2, 0, overlayToggleHotkey);
                    SaveSettings();
                    waitingForOverlayHotkey = false;
                }
            }
            else {
                waitingForHotkey = false;
                waitingForOverlayHotkey = false;
            }

            if (g_hMainWnd) {
                InvalidateRect(g_hMainWnd, NULL, TRUE);
            }

            return 1; // Consume the key event
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Coordinate reading functions
std::unique_ptr<Bitmap> BitmapFromHWND(HWND hwnd) {
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    RECT rc; GetWindowRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    HDC hdc = GetDC(hwnd);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);
    PrintWindow(hwnd, memDC, PW_RENDERFULLCONTENT);
    auto pBitmap = std::make_unique<Bitmap>(hBitmap, nullptr);
    SelectObject(memDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(hwnd, hdc);
    return pBitmap;
}

int GetShownCoordinates(HWND hwnd, Vec3* coordinates) {
    auto pBitmap = BitmapFromHWND(hwnd);
    int width = pBitmap->GetWidth();
    int height = pBitmap->GetHeight();
    int searchWidth = std::max(width / 3, std::min(125, width));
    int searchHeight = height / 3;
    Gdiplus::BitmapData bitmapData;
    Gdiplus::Rect rect(0, 0, searchWidth, searchHeight);
    pBitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);
    int startTextX = 0, startTextY = 0, streak = 0;
    int stride = bitmapData.Stride / sizeof(ARGB);
    ARGB* pixels = static_cast<ARGB*>(bitmapData.Scan0);

    for (int y = 30; y < searchHeight; y++) {
        for (int x = 8; x < searchWidth; x++) {
            if (pixels[y * stride + x] == 0xFFFFFFFF) {
                if (!startTextX) { startTextX = x; startTextY = y; }
                streak++;
            }
            else if (streak < 4) streak = 0;
            else if (streak >= 4) break;
        }
        if (streak >= 4) break;
    }

    if (streak < 4) return 0;
    int scale = streak / 4;
    startTextX += 44 * scale;
    int coords[3] = { 0, 0, 0 };
    int index = 0;
    bool isSigned = false;

    while (startTextX < searchWidth) {
        unsigned int columnMask = 0;
        for (int dy = 0; dy < 7; dy++) {
            columnMask <<= 1;
            if (pixels[(startTextY + dy * scale) * stride + startTextX] == 0xFFFFFFFF)
                columnMask |= 1;
        }

        int digit = -1;
        switch (columnMask) {
        case 0b0111110: digit = 0; break;
        case 0b0000001: digit = 1; break;
        case 0b0100011: digit = 2; break;
        case 0b0100010: digit = 3; break;
        case 0b0001100: digit = 4; break;
        case 0b1110010: digit = 5; break;
        case 0b0011110: digit = 6; break;
        case 0b1100000: digit = 7; break;
        case 0b0110110: digit = 8; break;
        case 0b0110000: digit = 9; break;
        case 0b0001000: isSigned = true; break;
        case 0b0000011:
            if (isSigned) coords[index] *= -1;
            if (++index > 2) break;
            isSigned = false;
            break;
        default:
            if (index >= 2) break;
            if (isSigned) coords[index] *= -1;
            break;
        }

        if (digit != -1)
            coords[index] = coords[index] * 10 + digit;
        startTextX += 6 * scale;
    }

    if (isSigned && index <= 2) {
        coords[index] *= -1;
    }

    pBitmap->UnlockBits(&bitmapData);
    coordinates->x = coords[0];
    coordinates->y = coords[1];
    coordinates->z = coords[2];
    return 1;
}

// Find closest treasure coordinate for a given player coordinate
int FindClosestTreasureCoord(int playerCoord) {
    int closest = 0;
    double minDistance = DBL_MAX;

    // Check positive coordinates
    for (int coord : positiveCoords) {
        double distance = abs(coord - playerCoord);
        if (distance < minDistance) {
            minDistance = distance;
            closest = coord;
        }
    }

    // Check negative coordinates
    for (int coord : negativeCoords) {
        double distance = abs(coord - playerCoord);
        if (distance < minDistance) {
            minDistance = distance;
            closest = coord;
        }
    }

    return closest;
}

// Calculate distance between two points
double CalculateDistance(int x1, int z1, int x2, int z2) {
    return sqrt((x2 - x1) * (x2 - x1) + (z2 - z1) * (z2 - z1));
}

// Find nearest buried treasures
void FindNearestTreasures(int playerX, int playerZ) {
    nearestTreasures.clear();

    // Get closest X and Z coordinates
    int closestX = FindClosestTreasureCoord(playerX);
    int closestZ = FindClosestTreasureCoord(playerZ);

    // Find several nearby treasure coordinates for both X and Z
    std::vector<int> nearbyX, nearbyZ;

    // Add closest and nearby X coordinates
    for (int coord : positiveCoords) {
        if (abs(coord - playerX) <= abs(closestX - playerX) + 200) {
            nearbyX.push_back(coord);
        }
    }
    for (int coord : negativeCoords) {
        if (abs(coord - playerX) <= abs(closestX - playerX) + 200) {
            nearbyX.push_back(coord);
        }
    }

    // Add closest and nearby Z coordinates
    for (int coord : positiveCoords) {
        if (abs(coord - playerZ) <= abs(closestZ - playerZ) + 200) {
            nearbyZ.push_back(coord);
        }
    }
    for (int coord : negativeCoords) {
        if (abs(coord - playerZ) <= abs(closestZ - playerZ) + 200) {
            nearbyZ.push_back(coord);
        }
    }

    // Generate all combinations and calculate distances
    for (int x : nearbyX) {
        for (int z : nearbyZ) {
            TreasureLocation treasure;
            treasure.x = x;
            treasure.z = z;
            treasure.distance = CalculateDistance(playerX, playerZ, x, z);
            nearestTreasures.push_back(treasure);
        }
    }

    // Sort by distance
    std::sort(nearestTreasures.begin(), nearestTreasures.end(),
        [](const TreasureLocation& a, const TreasureLocation& b) {
            return a.distance < b.distance;
        });

    // Keep only the closest 10
    if (nearestTreasures.size() > 10) {
        nearestTreasures.resize(10);
    }
}

// Overlay window functions
ATOM MyRegisterOverlayClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = OverlayProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcex.lpszClassName = szOverlayClass;
    return RegisterClassExW(&wcex);
}

BOOL InitOverlay(HINSTANCE hInstance) {
    std::wstring settingsFile = GetSettingsFilePath();

    // Load overlay position from settings, or use default
    int x = GetPrivateProfileInt(L"Overlay", L"X", GetSystemMetrics(SM_CXSCREEN) - 420, settingsFile.c_str());
    int y = GetPrivateProfileInt(L"Overlay", L"Y", 20, settingsFile.c_str());
    overlayVisible = GetPrivateProfileInt(L"Overlay", L"Visible", 0, settingsFile.c_str()) != 0;

    int overlayWidth = 400;
    int overlayHeight = 280;

    hOverlayWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        szOverlayClass,
        L"Treasure Overlay",
        WS_POPUP,
        x, y, overlayWidth, overlayHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hOverlayWnd) {
        return FALSE;
    }

    // Make window semi-transparent with black as transparent color
    SetLayeredWindowAttributes(hOverlayWnd, RGB(0, 0, 0), 220, LWA_COLORKEY | LWA_ALPHA);

    // Show overlay based on saved settings
    if (overlayVisible) {
        ShowWindow(hOverlayWnd, SW_SHOW);
    }

    return TRUE;
}

void UpdateOverlay() {
    if (hOverlayWnd && overlayVisible) {
        InvalidateRect(hOverlayWnd, NULL, TRUE);
    }
}

void ShowOverlay() {
    if (hOverlayWnd && !overlayVisible) {
        ShowWindow(hOverlayWnd, SW_SHOW);
        SetWindowPos(hOverlayWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        overlayVisible = true;
        UpdateOverlay();
        SaveSettings(); // Save the new visibility state
    }
}

void HideOverlay() {
    if (hOverlayWnd && overlayVisible) {
        ShowWindow(hOverlayWnd, SW_HIDE);
        overlayVisible = false;
        isDragging = false;
        SaveSettings(); // Save the new visibility state
    }
}

void ToggleOverlay() {
    if (overlayVisible) {
        HideOverlay();
    }
    else {
        ShowOverlay();
    }
}

LRESULT CALLBACK OverlayProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_LBUTTONDOWN:
    {
        // Start dragging
        isDragging = true;
        SetCapture(hWnd);

        POINT cursorPos;
        GetCursorPos(&cursorPos);

        RECT windowRect;
        GetWindowRect(hWnd, &windowRect);

        dragOffset.x = cursorPos.x - windowRect.left;
        dragOffset.y = cursorPos.y - windowRect.top;
    }
    break;

    case WM_LBUTTONUP:
    {
        // Stop dragging and save position
        if (isDragging) {
            isDragging = false;
            ReleaseCapture();
            SaveSettings(); // Save new position
        }
    }
    break;

    case WM_MOUSEMOVE:
    {
        if (isDragging) {
            POINT cursorPos;
            GetCursorPos(&cursorPos);

            int newX = cursorPos.x - dragOffset.x;
            int newY = cursorPos.y - dragOffset.y;

            // Keep window on screen
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);

            RECT windowRect;
            GetWindowRect(hWnd, &windowRect);
            int windowWidth = windowRect.right - windowRect.left;
            int windowHeight = windowRect.bottom - windowRect.top;

            if (newX < 0) newX = 0;
            if (newY < 0) newY = 0;
            if (newX + windowWidth > screenWidth) newX = screenWidth - windowWidth;
            if (newY + windowHeight > screenHeight) newY = screenHeight - windowHeight;

            SetWindowPos(hWnd, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    break;

    case WM_MOVE:
    {
        // Save position when window is moved (either by dragging or other means)
        SaveSettings();
    }
    break;

    case WM_SETCURSOR:
    {
        // Show move cursor when hovering over window
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return TRUE;
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        // Dark background with transparency
        graphics.Clear(Color(240, 15, 20, 35));

        // Draw border with grip indicator
        Pen borderPen(Color(255, 100, 150, 200), 2);
        RECT rect;
        GetClientRect(hWnd, &rect);
        graphics.DrawRectangle(&borderPen, 1, 1, rect.right - 2, rect.bottom - 2);

        // Draw grip indicator in top-left corner
        SolidBrush gripBrush(Color(255, 100, 150, 200));
        graphics.FillRectangle(&gripBrush, 3, 3, 8, 8);

        // Fonts
        FontFamily fontFamily(L"Consolas");
        Font headerFont(&fontFamily, 15, FontStyleBold, UnitPixel);
        Font coordFont(&fontFamily, 13, FontStyleBold, UnitPixel);
        Font infoFont(&fontFamily, 12, FontStyleRegular, UnitPixel);
        Font smallFont(&fontFamily, 11, FontStyleRegular, UnitPixel);

        // Colors
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        SolidBrush lightGrayBrush(Color(255, 200, 200, 200));
        SolidBrush greenBrush(Color(255, 120, 255, 120));
        SolidBrush yellowBrush(Color(255, 255, 220, 100));
        SolidBrush orangeBrush(Color(255, 255, 165, 0));
        SolidBrush cyanBrush(Color(255, 100, 200, 255));

        int y = 12;
        int x = 18;

        // Title
        graphics.DrawString(L"Buried Treasure Finder", -1, &headerFont,
            PointF((REAL)x, (REAL)y), &whiteBrush);
        y += 25;

        // Current coordinates
        if (currentCoords.x != 0 || currentCoords.z != 0) {
            std::wstringstream coordText;
            coordText << L"Player: (" << currentCoords.x << L", " << currentCoords.z << L")";
            graphics.DrawString(coordText.str().c_str(), -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
            y += 20;
        }
        else {
            graphics.DrawString(L"Press hotkey to find treasures", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
            y += 20;
        }

        // Show treasure results
        if (!nearestTreasures.empty()) {
            graphics.DrawString(L"Nearest Buried Treasures:", -1, &infoFont,
                PointF((REAL)x, (REAL)y), &greenBrush);
            y += 22;

            // Header
            graphics.DrawString(L"Rank  X Coord   Z Coord   Distance", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
            y += 18;

            // Show up to 6 treasures in overlay
            int maxShow = std::min(6, (int)nearestTreasures.size());
            for (int i = 0; i < maxShow; i++) {
                const auto& treasure = nearestTreasures[i];

                SolidBrush* brush = (i == 0) ? &greenBrush :
                    (i < 3) ? &yellowBrush : &lightGrayBrush;

                std::wstringstream treasureText;
                treasureText << L"#" << (i + 1) << L"   "
                    << std::setw(6) << treasure.x << L"   "
                    << std::setw(6) << treasure.z << L"   "
                    << std::fixed << std::setprecision(1)
                    << treasure.distance << L"m";

                graphics.DrawString(treasureText.str().c_str(), -1, &coordFont,
                    PointF((REAL)x, (REAL)y), brush);
                y += 20;
            }

            y += 10;
            graphics.DrawString(L"Drag to move  Press hotkey to refresh", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
        }

        EndPaint(hWnd, &ps);
    }
    break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Copy results to clipboard
void CopyTreasuresToClipboard() {
    if (nearestTreasures.empty()) return;

    std::wstringstream ss;
    ss << L"Nearest Buried Treasures:\n";
    ss << L"Player Position: (" << currentCoords.x << L", " << currentCoords.z << L")\n\n";

    int count = std::min(5, (int)nearestTreasures.size());
    for (int i = 0; i < count; i++) {
        const auto& treasure = nearestTreasures[i];
        ss << L"#" << (i + 1) << L": (" << treasure.x << L", " << treasure.z
            << L") - Distance: " << std::fixed << std::setprecision(1)
            << treasure.distance << L" blocks\n";
    }

    std::wstring text = ss.str();

    OpenClipboard(NULL);
    EmptyClipboard();
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
    if (hGlob) {
        memcpy(GlobalLock(hGlob), text.c_str(), (text.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(hGlob);
        SetClipboardData(CF_UNICODETEXT, hGlob);
    }
    CloseClipboard();
}

// Get key name for display
std::wstring GetKeyName(int vkCode) {
    switch (vkCode) {
    case VK_F1: return L"F1";
    case VK_F2: return L"F2";
    case VK_F3: return L"F3";
    case VK_F4: return L"F4";
    case VK_F5: return L"F5";
    case VK_F6: return L"F6";
    case VK_F7: return L"F7";
    case VK_F8: return L"F8";
    case VK_F9: return L"F9";
    case VK_F10: return L"F10";
    case VK_F11: return L"F11";
    case VK_F12: return L"F12";
    case VK_TAB: return L"TAB";
    case VK_SPACE: return L"SPACE";
    case VK_RETURN: return L"ENTER";
    case VK_ESCAPE: return L"ESC";
    case VK_INSERT: return L"INSERT";
    case VK_DELETE: return L"DELETE";
    case VK_HOME: return L"HOME";
    case VK_END: return L"END";
    case VK_PRIOR: return L"PAGE UP";
    case VK_NEXT: return L"PAGE DOWN";
    case VK_UP: return L"UP ARROW";
    case VK_DOWN: return L"DOWN ARROW";
    case VK_LEFT: return L"LEFT ARROW";
    case VK_RIGHT: return L"RIGHT ARROW";
    default:
        if (vkCode >= 'A' && vkCode <= 'Z') {
            return std::wstring(1, (wchar_t)vkCode);
        }
        if (vkCode >= '0' && vkCode <= '9') {
            return std::wstring(1, (wchar_t)vkCode);
        }
        return L"KEY_" + std::to_wstring(vkCode);
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wParam) == 1001) { // Change treasure hotkey button
            waitingForHotkey = true;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (LOWORD(wParam) == 1002) { // Change overlay hotkey button
            waitingForOverlayHotkey = true;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;

    case WM_HOTKEY:
        if (wParam == 1) { // Treasure finder hotkey
            HWND mcHwnd = FindWindow(NULL, L"Minecraft");
            if (!mcHwnd) {
                // Try alternative window names for Minecraft Bedrock
                mcHwnd = FindWindow(NULL, L"Minecraft for Windows 10");
            }

            if (mcHwnd && GetShownCoordinates(mcHwnd, &currentCoords)) {
                FindNearestTreasures(currentCoords.x, currentCoords.z);
                CopyTreasuresToClipboard();

                // Show overlay when treasures are found
                if (!nearestTreasures.empty()) {
                    ShowOverlay();
                }
                else {
                    HideOverlay();
                }

                UpdateOverlay();
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        else if (wParam == 2) { // Overlay toggle hotkey
            ToggleOverlay();
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.Clear(Color(25, 25, 35));

        FontFamily fontFamily(L"Segoe UI");
        Font headerFont(&fontFamily, 16, FontStyleBold, UnitPixel);
        Font font(&fontFamily, 12, FontStyleRegular, UnitPixel);
        Font smallFont(&fontFamily, 10, FontStyleRegular, UnitPixel);

        SolidBrush whiteBrush(Color(240, 240, 240));
        SolidBrush grayBrush(Color(180, 180, 180));
        SolidBrush greenBrush(Color(120, 220, 120));
        SolidBrush yellowBrush(Color(255, 220, 100));
        SolidBrush cyanBrush(Color(100, 200, 255));
        SolidBrush redBrush(Color(255, 100, 100));

        int marginX = 20;
        int y = 20;

        // Header
        graphics.DrawString(L"Minecraft Buried Treasure Finder", -1, &headerFont,
            PointF((REAL)marginX, (REAL)y), &cyanBrush);
        y += 40;

        // Hotkey configuration
        std::wstringstream hotkeyText;
        hotkeyText << L"Find Treasure Hotkey: " << GetKeyName(currentHotkey);
        if (waitingForHotkey) {
            hotkeyText << L" (Press new key...)";
        }
        SolidBrush* hotkeyBrush = waitingForHotkey ? &yellowBrush : &grayBrush;
        graphics.DrawString(hotkeyText.str().c_str(), -1, &font,
            PointF((REAL)marginX, (REAL)y), hotkeyBrush);
        y += 30;

        // Overlay toggle hotkey configuration
        std::wstringstream overlayHotkeyText;
        overlayHotkeyText << L"Toggle Overlay Hotkey: " << GetKeyName(overlayToggleHotkey);
        if (waitingForOverlayHotkey) {
            overlayHotkeyText << L" (Press new key...)";
        }
        SolidBrush* overlayHotkeyBrush = waitingForOverlayHotkey ? &yellowBrush : &grayBrush;
        graphics.DrawString(overlayHotkeyText.str().c_str(), -1, &font,
            PointF((REAL)marginX, (REAL)y), overlayHotkeyBrush);
        y += 35;

        // Overlay status
        std::wstringstream overlayStatus;
        overlayStatus << L"Overlay Status: " << (overlayVisible ? L"Visible" : L"Hidden");
        graphics.DrawString(overlayStatus.str().c_str(), -1, &smallFont,
            PointF((REAL)marginX, (REAL)y), overlayVisible ? &greenBrush : &redBrush);
        y += 25;

        // Current coordinates
        std::wstringstream coordText;
        coordText << L"Current Position:\n"
            << L"X: " << currentCoords.x << L"\n"
            << L"Y: " << currentCoords.y << L"\n"
            << L"Z: " << currentCoords.z;
        graphics.DrawString(coordText.str().c_str(), -1, &font,
            PointF((REAL)marginX, (REAL)y), &grayBrush);
        y += 85;

        // Instructions or results
                if (nearestTreasures.empty()) {
            std::wstringstream instrText;
            instrText << L"Instructions:\n"
                << L"1. Press " << GetKeyName(currentHotkey) << L" to find nearest buried treasures\n"
                << L"2. Press " << GetKeyName(overlayToggleHotkey) << L" to toggle overlay visibility\n"
                << L"3. Results will be copied to clipboard automatically\n"
                << L"4. Overlay shows treasure locations (drag to move)";
            graphics.DrawString(instrText.str().c_str(), -1, &font,
                PointF((REAL)marginX, (REAL)y), &whiteBrush);
        }
        else {
            graphics.DrawString(L"Nearest Buried Treasures:", -1, &font,
                PointF((REAL)marginX, (REAL)y), &greenBrush);
            y += 25;

            // Show overlay status info
            std::wstringstream overlayInfo;
            overlayInfo << L"Overlay: " << (overlayVisible ? L"Active (drag to move)" : L"Hidden");
            graphics.DrawString(overlayInfo.str().c_str(), -1, &smallFont,
                PointF((REAL)marginX, (REAL)y), overlayVisible ? &cyanBrush : &redBrush);
            y += 20;

            // Table header
            graphics.DrawString(L"Rank  X Coord   Z Coord   Distance", -1, &smallFont,
                PointF((REAL)marginX, (REAL)y), &grayBrush);
            y += 20;

            // Show treasure locations
            int maxShow = std::min(8, (int)nearestTreasures.size());
            for (int i = 0; i < maxShow; i++) {
                const auto& treasure = nearestTreasures[i];

                std::wstringstream treasureText;
                treasureText << L"#" << (i + 1) << L"    "
                    << std::setw(6) << treasure.x << L"    "
                    << std::setw(6) << treasure.z << L"    "
                    << std::fixed << std::setprecision(1)
                    << treasure.distance << L" blocks";

                SolidBrush* brush = (i == 0) ? &greenBrush :
                    (i < 3) ? &yellowBrush : &grayBrush;

                graphics.DrawString(treasureText.str().c_str(), -1, &font,
                    PointF((REAL)marginX, (REAL)y), brush);
                y += 20;
            }

            y += 10;
            std::wstringstream copyText;
            copyText << L"Top 5 locations copied to clipboard. Press "
                << GetKeyName(currentHotkey) << L" to refresh.";
            graphics.DrawString(copyText.str().c_str(), -1, &smallFont,
                PointF((REAL)marginX, (REAL)y), &whiteBrush);
        }

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_CREATE:
    {
        // Store main window handle globally
        g_hMainWnd = hWnd;

        // Install low-level keyboard hook for hotkey changing
        g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);

        // Load settings first
        LoadSettings();

        // Register overlay window class
        MyRegisterOverlayClass(GetModuleHandle(NULL));

        // Initialize overlay
        InitOverlay(GetModuleHandle(NULL));

        // Create change hotkey buttons
        CreateWindow(L"BUTTON", L"Change Find Hotkey", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            250, 55, 140, 25, hWnd, (HMENU)1001, GetModuleHandle(NULL), NULL);

        CreateWindow(L"BUTTON", L"Change Toggle Hotkey", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            250, 85, 140, 25, hWnd, (HMENU)1002, GetModuleHandle(NULL), NULL);

        // Register hotkeys
        RegisterHotKey(hWnd, 1, 0, currentHotkey);
        RegisterHotKey(hWnd, 2, 0, overlayToggleHotkey);
    }
    break;

    case WM_DESTROY:
        // Save settings before closing
        SaveSettings();

        // Unhook keyboard hook
        if (g_hKeyboardHook) {
            UnhookWindowsHookEx(g_hKeyboardHook);
        }

        // Unregister hotkeys
        UnregisterHotKey(hWnd, 1);
        UnregisterHotKey(hWnd, 2);

        // Destroy overlay window
        if (hOverlayWnd) {
            DestroyWindow(hOverlayWnd);
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register window class
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"TreasureFinderWindow";
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wcex);

    // Create window
    HWND hWnd = CreateWindow(L"TreasureFinderWindow", L"Minecraft Buried Treasure Finder",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, 500, 650, NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;

}
