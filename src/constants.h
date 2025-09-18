#pragma once

#include <cstddef>

namespace trimgear {
namespace constants {

constexpr int XPLANE_STRING_BUFFER_SIZE = 256;
constexpr int XPLANE_PATH_BUFFER_SIZE = 512;

// Gear settings - 9 predefined gear ratios
constexpr float GEAR_SETTINGS[] = {
    0.001f, 0.005f, 0.01f, 0.015f, 0.02f, 0.025f, 0.03f, 0.035f, 0.04f
};

constexpr int NUM_GEAR_SETTINGS = sizeof(GEAR_SETTINGS) / sizeof(GEAR_SETTINGS[0]);
constexpr int DEFAULT_GEAR_INDEX = 2; // 0.01f as default

// Menu text buffer size for safe string formatting
constexpr size_t MENU_TEXT_BUFFER_SIZE = 64;

// Menu item IDs
enum MenuItems {
    MENU_RELOAD_CONFIG = 1000,

    // Pitch trim controls
    MENU_PITCH_GEAR_UP = 1100,
    MENU_PITCH_ENABLE_DISABLE = 1101,
    MENU_PITCH_GEAR_DOWN = 1102,

    // Roll trim controls
    MENU_ROLL_GEAR_UP = 1200,
    MENU_ROLL_ENABLE_DISABLE = 1201,
    MENU_ROLL_GEAR_DOWN = 1202,

    // Rudder trim controls
    MENU_RUDDER_GEAR_UP = 1300,
    MENU_RUDDER_ENABLE_DISABLE = 1301,
    MENU_RUDDER_GEAR_DOWN = 1302
};

// Trim axes
enum class TrimAxis {
    PITCH = 0,
    ROLL = 1,
    RUDDER = 2,
    COUNT = 3
};

}
}