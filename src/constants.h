#pragma once

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

// Menu item IDs
enum MenuItems {
    MENU_RELOAD_CONFIG = 1000,
    MENU_PITCH_GEAR_START = 1100,
    MENU_ROLL_GEAR_START = 1200,
    MENU_RUDDER_GEAR_START = 1300
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