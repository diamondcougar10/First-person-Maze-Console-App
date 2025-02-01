#ifndef RETRO_FPS_STRUCTS_H
#define RETRO_FPS_STRUCTS_H

#include <vector>
#include <string>
#include <Windows.h>

// Console Color Definitions
#define FG_BLACK        0
#define FG_BLUE         1
#define FG_GREEN        2
#define FG_CYAN         3
#define FG_RED          4
#define FG_MAGENTA      5
#define FG_BROWN        6
#define FG_LIGHTGRAY    7
#define FG_DARKGRAY     8
#define FG_LIGHTBLUE    9
#define FG_LIGHTGREEN   10
#define FG_LIGHTCYAN    11
#define FG_LIGHTRED     12
#define FG_LIGHTMAGENTA 13
#define FG_YELLOW       14
#define FG_WHITE        15

extern HANDLE Console;

// Player Struct
struct Player {
    float x, y;  // Player position
    float angle; // Player view angle
    int health;
    int ammo;
};

// Bullet Struct
struct Bullet {
    float x, y;
    float angle;
    bool active;
    int damage;
};

// Enemy Struct
struct Enemy {
    float x, y;
    int health;
    bool active;
};

struct Weapon {
    std::string name;  // Name of the weapon
    float fireRate;    // Time between shots
    int damage;        // Damage per shot
    int ammo;          // Current ammo
};


// Game World Struct
struct GameWorld {
    int width, height;
    std::wstring mapData;
};

// Console Utility Functions
inline void SetColor(int color) {
    SetConsoleTextAttribute(Console, color);
}

#endif // RETRO_FPS_STRUCTS_H
