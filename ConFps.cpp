#include <iostream>
#include <Windows.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <wchar.h>
#include "Structs.h"

using namespace std;

// Global variables
int ScreenWidth = 280;
int ScreenHeight = 70;
float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerA = 0.0f; // angle player is looking at
int MapHeight = 16;
int MapWidth = 16;
float fov = 3.14159f / 4.0f;
float Depth = 16.0f;

wstring map =
L"################"
L"#      #       #"
L"#              #"
L"#      # #     #"
L"#      #       #"
L"#      #       #"
L"#              #"
L"#              #"
L"#              #"
L"#              #"
L"#              #"
L"#              #"
L"#        #######"
L"#              #"
L"#              #"
L"################";


// Player and Weapon Definitions
Player player = { 8.0f, 8.0f, 0.0f, 100, 10 };
Weapon weapons[] =
{
    {"Pistol", 0.5f, 10, 100},
    {"Shotgun", 1.2f, 25, 50}
};

int currentWeapon = 0; // Currently selected weapon

void RenderWeapon(CHAR_INFO* screenBuffer) {
    int weaponY = ScreenHeight - 12; // Adjust for the scaled-up height
    int weaponX = ScreenWidth / 2 - 20; // Adjust for the scaled-up width

    // Scaled-up Pistol Sprite
    const wchar_t* pistolSprite[6] = {
        L"        |||||      ",
        L"       ||   ||     ",
        L"  _____||   ||___  ",
        L" |  ==_______==  | ",
        L" |  |_______|  |  |",
        L"  \\___________/   "
    };

    // Scaled-up Shotgun Sprite
    const wchar_t* shotgunSprite[6] = {
        L"   ______________  ",
        L"  |  ||=======||  |",
        L"  |  ||_______||  |",
        L"  |  |_________|  |",
        L"  |   _________|  |",
        L"  \\__/           / "
    };

    // Select weapon sprite
    const wchar_t** weaponSprite = (currentWeapon == 0) ? pistolSprite : shotgunSprite;
    WORD weaponColor = (currentWeapon == 0) ? FG_LIGHTGRAY : FG_DARKGRAY; // Different colors for each weapon

    // Scale up the sprite by 2x (duplicate rows and columns)
    for (int y = 0; y < 6; y++) {
        for (int scaleY = 0; scaleY < 2; scaleY++) { // Duplicate rows
            for (int x = 0; x < (int)wcslen(weaponSprite[y]); x++) {
                for (int scaleX = 0; scaleX < 2; scaleX++) { // Duplicate columns
                    int idx = (weaponY + y * 2 + scaleY) * ScreenWidth + (weaponX + x * 2 + scaleX);
                    if (idx < ScreenWidth * ScreenHeight) { // Ensure within buffer bounds
                        wchar_t charToRender = weaponSprite[y][x];
                        if (charToRender != ' ') { // Keep transparency
                            screenBuffer[idx].Char.UnicodeChar = charToRender;
                            screenBuffer[idx].Attributes = weaponColor;
                        }
                    }
                }
            }
        }
    }
}


void SetConsoleSize() {
    HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);

    // Set the console screen buffer size
    COORD bufferSize = { (SHORT)ScreenWidth, (SHORT)ScreenHeight };
    if (!SetConsoleScreenBufferSize(Console, bufferSize)) {
        wcout << L"Error: Unable to set console buffer size!" << endl;
        return;
    }

    // Set the console window size to match the buffer
    SMALL_RECT windowSize = { 0, 0, (SHORT)(ScreenWidth - 1), (SHORT)(ScreenHeight - 1) };
    if (!SetConsoleWindowInfo(Console, TRUE, &windowSize)) {
        wcout << L"Error: Unable to set console window size!" << endl;
        return;
    }

    // Set the console font size for proper scaling with the larger window
    CONSOLE_FONT_INFOEX cfi = { sizeof(CONSOLE_FONT_INFOEX) };
    cfi.dwFontSize.X = 10;  // Width of each character in pixels
    cfi.dwFontSize.Y = 18;  // Height of each character in pixels
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas"); // Use a fixed-width font
    SetCurrentConsoleFontEx(Console, FALSE, &cfi);

    // Center the console window on the screen
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole) {
        RECT rect;
        GetWindowRect(hwndConsole, &rect);

        int consoleWidth = rect.right - rect.left;
        int consoleHeight = rect.bottom - rect.top;

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        int posX = (screenWidth - consoleWidth) / 2;
        int posY = (screenHeight - consoleHeight) / 2;

        MoveWindow(hwndConsole, posX, posY, consoleWidth, consoleHeight, TRUE);
    }

    // Disable resizing and maximizing
    LONG style = GetWindowLong(hwndConsole, GWL_STYLE);
    style &= ~(WS_MAXIMIZEBOX | WS_SIZEBOX);
    SetWindowLong(hwndConsole, GWL_STYLE, style);
}


void RenderHUD(CHAR_INFO* screenBuffer, float fps) {
    // Build the HUD string
    wstring stats = L"FPS: " + to_wstring((int)fps) +
        L" | Health: " + to_wstring(player.health) +
        L" | Ammo: " + to_wstring(weapons[currentWeapon].ammo) +
        L" | Weapon: " + wstring(weapons[currentWeapon].name.begin(), weapons[currentWeapon].name.end());

    // Position the stats bar at the bottom of the screen
    int startIdx = (ScreenHeight - 1) * ScreenWidth; // Start index for the last row
    for (size_t i = 0; i < stats.size(); i++) {
        if (startIdx + i < ScreenWidth * ScreenHeight) { // Ensure we're within bounds
            screenBuffer[startIdx + i].Char.UnicodeChar = stats[i];
            screenBuffer[startIdx + i].Attributes = FG_WHITE; // White text for the stats
        }
    }

    // Fill the rest of the bottom row with spaces if the stats don't span the full width
    for (size_t i = stats.size(); i < ScreenWidth; i++) {
        screenBuffer[startIdx + i].Char.UnicodeChar = ' ';
        screenBuffer[startIdx + i].Attributes = FG_WHITE;
    }
}

void SetFixedConsoleSize() {
    HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);

    // Set the screen buffer size to match the desired window size
    COORD bufferSize = { (SHORT)ScreenWidth, (SHORT)ScreenHeight };
    if (!SetConsoleScreenBufferSize(Console, bufferSize)) {
        wcout << L"Error: Unable to set console buffer size. Check permissions!" << endl;
        return;
    }

    // Set the window size to match the buffer size
    SMALL_RECT windowSize = { 0, 0, (SHORT)(ScreenWidth - 1), (SHORT)(ScreenHeight - 1) };
    if (!SetConsoleWindowInfo(Console, TRUE, &windowSize)) {
        wcout << L"Error: Unable to set console window size. Check permissions!" << endl;
        return;
    }

    // Disable resizing, maximize, and minimize buttons
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole) {
        LONG style = GetWindowLong(hwndConsole, GWL_STYLE);
        style &= ~(WS_MAXIMIZEBOX | WS_SIZEBOX); // Disable maximize and resizing
        SetWindowLong(hwndConsole, GWL_STYLE, style);
    }

    // Center the console window on the screen
    HWND hwnd = GetConsoleWindow();
    if (hwnd) {
        RECT rect;
        GetWindowRect(hwnd, &rect);

        int consoleWidth = rect.right - rect.left;
        int consoleHeight = rect.bottom - rect.top;

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        int posX = (screenWidth - consoleWidth) / 2;
        int posY = (screenHeight - consoleHeight) / 2;

        MoveWindow(hwnd, posX, posY, consoleWidth, consoleHeight, TRUE);
    }
}


void RenderMiniMap(CHAR_INFO* screenBuffer) {
    int mapOffsetX = 1;  // Padding from left
    int mapOffsetY = 1;  // Padding from top

    for (int ny = 0; ny < MapHeight; ny++) {
        for (int nx = 0; nx < MapWidth; nx++) {
            int screenIdx = (mapOffsetY + ny) * ScreenWidth + (mapOffsetX + nx);
            int mapIdx = ny * MapWidth + nx;  // Ensure map index is within bounds

            if (mapIdx < 0 || mapIdx >= map.size()) continue;  // Prevent out-of-bounds errors

            if ((int)fPlayerY == ny && (int)fPlayerX == nx) {
                // Display the player as 'P'
                screenBuffer[screenIdx].Char.UnicodeChar = L'P';
                screenBuffer[screenIdx].Attributes = FG_YELLOW;
            }
            else if (map[mapIdx] == L'#') {  // Compare as a wide character (L'#')
                // Display walls
                screenBuffer[screenIdx].Char.UnicodeChar = L'#';
                screenBuffer[screenIdx].Attributes = FG_RED;
            }
            else {
                // Open space
                screenBuffer[screenIdx].Char.UnicodeChar = L'.';
                screenBuffer[screenIdx].Attributes = FG_GREEN;
            }
        }
    }
}



int main() {
    // Create the screen buffer
    SetFixedConsoleSize();
    SetConsoleSize();
    CHAR_INFO* screenBuffer = new CHAR_INFO[ScreenWidth * ScreenHeight];
    HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);
    // Set up the buffer size and write region
    COORD bufferSize = { (SHORT)ScreenWidth, (SHORT)ScreenHeight };
    COORD bufferCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, (SHORT)(ScreenWidth - 1), (SHORT)(ScreenHeight - 1) };


    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    // Game loop
    while (true) {
        tp2 = chrono::system_clock::now();
        chrono::duration<float> time_span = chrono::duration_cast<chrono::duration<float>>(tp2 - tp1);
        tp1 = tp2;
        float ElapsedTime = time_span.count();
        float fps = 1.0f / ElapsedTime;

        // Player controls
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
            fPlayerA -= 0.8f * ElapsedTime;
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
            fPlayerA += 0.8f * ElapsedTime;

        if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
            fPlayerX += sinf(fPlayerA) * ElapsedTime * 5.0f;
            fPlayerY += cosf(fPlayerA) * ElapsedTime * 5.0f;
            if (map[(int)fPlayerY * MapWidth + (int)fPlayerX] == '#') {
                fPlayerX -= sinf(fPlayerA) * ElapsedTime * 5.0f;
                fPlayerY -= cosf(fPlayerA) * ElapsedTime * 5.0f;
            }
        }

        if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
            fPlayerX -= sinf(fPlayerA) * ElapsedTime * 5.0f;
            fPlayerY -= cosf(fPlayerA) * ElapsedTime * 5.0f;
            if (map[(int)fPlayerY * MapWidth + (int)fPlayerX] == '#') {
                fPlayerX += sinf(fPlayerA) * ElapsedTime * 5.0f;
                fPlayerY += cosf(fPlayerA) * ElapsedTime * 5.0f;
            }
        }

        // Weapon switching keys
        if (GetAsyncKeyState((unsigned short)'1') & 0x8000)
            currentWeapon = 0; // Pistol
        if (GetAsyncKeyState((unsigned short)'2') & 0x8000)
            currentWeapon = 1; // Shotgun

        // Clear the screen buffer
        for (int i = 0; i < ScreenWidth * ScreenHeight; i++) {
            screenBuffer[i].Char.UnicodeChar = ' ';
            screenBuffer[i].Attributes = FG_WHITE;
        }

        // Render the mini-map (positioned in the top-left area)
        for (int ny = 0; ny < MapHeight; ny++) {
            for (int nx = 0; nx < MapWidth; nx++) {
                int mapIdx = (ny + 2) * ScreenWidth + nx;
                if ((int)fPlayerY == ny && (int)fPlayerX == nx) {
                    screenBuffer[mapIdx].Char.UnicodeChar = 'P';
                    screenBuffer[mapIdx].Attributes = FG_YELLOW;
                }
                else if (map[ny * MapWidth + nx] == '#') {
                    screenBuffer[mapIdx].Char.UnicodeChar = '#';
                    screenBuffer[mapIdx].Attributes = FG_RED;
                }
                else {
                    screenBuffer[mapIdx].Char.UnicodeChar = '.';
                    screenBuffer[mapIdx].Attributes = FG_GREEN;
                }
            }
        }

        for (int x = 0; x < ScreenWidth; x++) {
            float rayAngle = (fPlayerA - fov / 2.0f) + ((float)x / (float)ScreenWidth) * fov;
            float rayDistance = 0;
            bool HitWall = false;
            bool boundary = false; // To detect edges between wall blocks

            float EyeX = sinf(rayAngle);
            float EyeY = cosf(rayAngle);
            WORD WallColor = FG_WHITE;

            while (!HitWall && rayDistance < Depth) {
                rayDistance += 0.1f;
                int TestX = (int)(fPlayerX + EyeX * rayDistance);
                int TestY = (int)(fPlayerY + EyeY * rayDistance);

                if (TestX < 0 || TestX >= MapWidth || TestY < 0 || TestY >= MapHeight) {
                    HitWall = true;
                    rayDistance = Depth; // Hit the boundary of the map
                }
                else if (map[TestY * MapWidth + TestX] == '#') {
                    HitWall = true;
                    WallColor = FG_RED; // Color for walls

                    // Boundary detection: Check for edges between blocks
                    vector<pair<float, float>> p;
                    for (int tx = 0; tx < 2; tx++) {
                        for (int ty = 0; ty < 2; ty++) {
                            float vx = (float)TestX + tx - fPlayerX;
                            float vy = (float)TestY + ty - fPlayerY;
                            float d = sqrt(vx * vx + vy * vy);
                            float dot = (EyeX * vx / d) + (EyeY * vy / d);
                            p.push_back(make_pair(d, dot));
                        }
                    }
                    sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {
                        return left.first < right.first;
                        });

                    // If the angle between the ray and one of the corners is very small, it's near a boundary
                    float boundThreshold = 0.01f;
                    if (acos(p[0].second) < boundThreshold || acos(p[1].second) < boundThreshold)
                        boundary = true;
                }
            }

            int Ceiling = (float)ScreenHeight / 2.0f - ScreenHeight / ((float)rayDistance);
            int Floor = ScreenHeight - Ceiling;

            // Draw the column for this ray
            for (int y = 0; y < ScreenHeight; y++) {
                int idx = y * ScreenWidth + x;
                if (y < Ceiling) {
                    // Sky
                    screenBuffer[idx].Char.UnicodeChar = ' ';
                    screenBuffer[idx].Attributes = FG_BLUE;
                }
                else if (y >= Ceiling && y < Floor) {
                    // Wall
                    screenBuffer[idx].Char.UnicodeChar = boundary ? 0x2592 : 0x2588; // Use lighter shading for edges
                    screenBuffer[idx].Attributes = boundary ? FG_DARKGRAY : WallColor;
                }
                else {
                    // Floor with gradient shading
                    float b = 1.0f - (((float)y - ScreenHeight / 2.0f) / ((float)ScreenHeight / 2.0f));
                    screenBuffer[idx].Char.UnicodeChar = '.';
                    if (b < 0.25f)
                        screenBuffer[idx].Attributes = FG_GREEN | FOREGROUND_INTENSITY;
                    else if (b < 0.5f)
                        screenBuffer[idx].Attributes = FG_GREEN;
                    else if (b < 0.75f)
                        screenBuffer[idx].Attributes = FG_DARKGRAY;
                    else
                        screenBuffer[idx].Attributes = FG_BLACK;
                }
            }
        }

        // Inside the game loop
        RenderMiniMap(screenBuffer);
          
        // Render the weapon
        RenderWeapon(screenBuffer);

        // Render the HUD
        RenderHUD(screenBuffer, fps);

        // Write the entire buffer to the console
        WriteConsoleOutput(Console, screenBuffer, { (SHORT)ScreenWidth, (SHORT)ScreenHeight }, { 0, 0 }, & writeRegion);
    }

    delete[] screenBuffer;
    return 0;
}

