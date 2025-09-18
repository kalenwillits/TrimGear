#include "XPLMPlugin.h"
#include "XPLMUtilities.h"
#include "XPLMMenus.h"
#include "XPLMDataAccess.h"
#include "XPLMPlanes.h"

#include "config.h"
#include "trim_controller.h"
#include "constants.h"

#include <string>
#include <vector>
#include <cstring>
#include <memory>
#include <cstdio>

// Global plugin state
static XPLMMenuID g_menu_id = nullptr;
static XPLMMenuID g_submenu_id = nullptr;
static std::vector<XPLMCommandRef> g_trim_commands;
static std::unique_ptr<Config> g_config;
static std::unique_ptr<TrimController> g_trim_controller;

// Aircraft tracking for config reloading

// Menu tracking - for simplified menu structure
static int g_pitch_header_item = -1;
static int g_pitch_gear_up_item = -1;
static int g_pitch_enable_disable_item = -1;
static int g_pitch_gear_down_item = -1;

static int g_roll_header_item = -1;
static int g_roll_gear_up_item = -1;
static int g_roll_enable_disable_item = -1;
static int g_roll_gear_down_item = -1;

static int g_rudder_header_item = -1;
static int g_rudder_gear_up_item = -1;
static int g_rudder_enable_disable_item = -1;
static int g_rudder_gear_down_item = -1;

// Forward declarations
static void menu_handler(void* menu_ref, void* item_ref);
static int trim_command_handler(XPLMCommandRef command, XPLMCommandPhase phase, void* refcon);
static void create_trim_commands();
static void create_menu_system();
static void load_aircraft_config();
static void update_menu_checkmarks();

PLUGIN_API int XPluginStart(char* out_name, char* out_sig, char* out_desc)
{
    using namespace trimgear::constants;
    
    std::string name = "TrimGear";
    std::string sig = "trimgear.plugin";
    std::string desc = "Aircraft trim gear adjustment plugin";
    
    // Safely copy strings with proper null termination
    std::strncpy(out_name, name.c_str(), XPLANE_STRING_BUFFER_SIZE - 1);
    std::strncpy(out_sig, sig.c_str(), XPLANE_STRING_BUFFER_SIZE - 1);
    std::strncpy(out_desc, desc.c_str(), XPLANE_STRING_BUFFER_SIZE - 1);
    out_name[XPLANE_STRING_BUFFER_SIZE - 1] = '\0';
    out_sig[XPLANE_STRING_BUFFER_SIZE - 1] = '\0';
    out_desc[XPLANE_STRING_BUFFER_SIZE - 1] = '\0';

    // Initialize components
    g_config = std::make_unique<Config>();
    g_trim_controller = std::make_unique<TrimController>();
    
    if (!g_trim_controller->initialize()) {
        XPLMDebugString("TrimGear: ERROR - Failed to initialize trim controller\n");
        return 0;
    }

    // Create plugin menu
    g_menu_id = XPLMFindPluginsMenu();
    if (g_menu_id) {
        int menu_item = XPLMAppendMenuItem(g_menu_id, "TrimGear", nullptr, 1);
        g_submenu_id = XPLMCreateMenu("TrimGear", g_menu_id, menu_item, menu_handler, nullptr);
        create_menu_system();
    }

    // Create custom commands
    create_trim_commands();

    XPLMDebugString("TrimGear: Plugin started successfully\n");
    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    // Cleanup commands
    for (auto command : g_trim_commands) {
        if (command) {
            XPLMUnregisterCommandHandler(command, trim_command_handler, 0, nullptr);
        }
    }
    g_trim_commands.clear();

    // Cleanup components
    if (g_trim_controller) {
        g_trim_controller->cleanup();
        g_trim_controller.reset();
    }
    
    g_config.reset();

    XPLMDebugString("TrimGear: Plugin stopped\n");
}

PLUGIN_API int XPluginEnable(void)
{
    load_aircraft_config();
    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    // Save current configuration when disabled
    if (g_config) {
        g_config->save_config();
    }
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID, int msg, void*)
{
    if (msg == XPLM_MSG_PLANE_LOADED) {
        load_aircraft_config();
    }
}

static void create_trim_commands()
{
    using namespace trimgear::constants;
    
    struct CommandDef {
        const char* name;
        const char* description;
        TrimAxis axis;
        bool increase;
    };
    
    CommandDef commands[] = {
        {"trimgear/pitch_trim_up", "Pitch trim up with gear", TrimAxis::PITCH, true},
        {"trimgear/pitch_trim_down", "Pitch trim down with gear", TrimAxis::PITCH, false},
        {"trimgear/roll_trim_left", "Roll trim left with gear", TrimAxis::ROLL, false},
        {"trimgear/roll_trim_right", "Roll trim right with gear", TrimAxis::ROLL, true},
        {"trimgear/rudder_trim_left", "Rudder trim left with gear", TrimAxis::RUDDER, false},
        {"trimgear/rudder_trim_right", "Rudder trim right with gear", TrimAxis::RUDDER, true}
    };
    
    g_trim_commands.reserve(6);
    
    for (const auto& cmd : commands) {
        XPLMCommandRef command = XPLMCreateCommand(cmd.name, cmd.description);
        if (command) {
            // Pack axis and direction into refcon
            intptr_t refcon = (static_cast<intptr_t>(cmd.axis) << 1) | (cmd.increase ? 1 : 0);
            XPLMRegisterCommandHandler(command, trim_command_handler, 1, reinterpret_cast<void*>(refcon));
            g_trim_commands.push_back(command);
            
            std::string log_msg = "TrimGear: Created command: ";
            log_msg += cmd.name;
            log_msg += "\n";
            XPLMDebugString(log_msg.c_str());
        } else {
            std::string error_msg = "TrimGear: ERROR - Failed to create command: ";
            error_msg += cmd.name;
            error_msg += "\n";
            XPLMDebugString(error_msg.c_str());
        }
    }
}

static void create_menu_system()
{
    using namespace trimgear::constants;

    if (!g_submenu_id) return;

    // Add reload config option
    XPLMAppendMenuItem(g_submenu_id, "Reload Configuration",
                      reinterpret_cast<void*>(MENU_RELOAD_CONFIG), 1);

    // Add separator
    XPLMAppendMenuSeparator(g_submenu_id);

    // Pitch trim section
    g_pitch_header_item = XPLMAppendMenuItem(g_submenu_id, "Pitch Trim Gear: 0.010 (2)", nullptr, 0);
    g_pitch_gear_up_item = XPLMAppendMenuItem(g_submenu_id, "Gear Up (3) - 0.015",
                                             reinterpret_cast<void*>(MENU_PITCH_GEAR_UP), 1);
    g_pitch_enable_disable_item = XPLMAppendMenuItem(g_submenu_id, "Enabled",
                                                    reinterpret_cast<void*>(MENU_PITCH_ENABLE_DISABLE), 1);
    g_pitch_gear_down_item = XPLMAppendMenuItem(g_submenu_id, "Gear Down (1) - 0.005",
                                               reinterpret_cast<void*>(MENU_PITCH_GEAR_DOWN), 1);

    // Add separator
    XPLMAppendMenuSeparator(g_submenu_id);

    // Roll trim section
    g_roll_header_item = XPLMAppendMenuItem(g_submenu_id, "Roll Trim Gear: 0.010 (2)", nullptr, 0);
    g_roll_gear_up_item = XPLMAppendMenuItem(g_submenu_id, "Gear Up (3) - 0.015",
                                            reinterpret_cast<void*>(MENU_ROLL_GEAR_UP), 1);
    g_roll_enable_disable_item = XPLMAppendMenuItem(g_submenu_id, "Enabled",
                                                   reinterpret_cast<void*>(MENU_ROLL_ENABLE_DISABLE), 1);
    g_roll_gear_down_item = XPLMAppendMenuItem(g_submenu_id, "Gear Down (1) - 0.005",
                                              reinterpret_cast<void*>(MENU_ROLL_GEAR_DOWN), 1);

    // Add separator
    XPLMAppendMenuSeparator(g_submenu_id);

    // Rudder trim section
    g_rudder_header_item = XPLMAppendMenuItem(g_submenu_id, "Rudder Trim Gear: 0.010 (2)", nullptr, 0);
    g_rudder_gear_up_item = XPLMAppendMenuItem(g_submenu_id, "Gear Up (3) - 0.015",
                                              reinterpret_cast<void*>(MENU_RUDDER_GEAR_UP), 1);
    g_rudder_enable_disable_item = XPLMAppendMenuItem(g_submenu_id, "Enabled",
                                                     reinterpret_cast<void*>(MENU_RUDDER_ENABLE_DISABLE), 1);
    g_rudder_gear_down_item = XPLMAppendMenuItem(g_submenu_id, "Gear Down (1) - 0.005",
                                                reinterpret_cast<void*>(MENU_RUDDER_GEAR_DOWN), 1);
}

static void menu_handler(void*, void* item_ref)
{
    using namespace trimgear::constants;

    intptr_t item = reinterpret_cast<intptr_t>(item_ref);

    if (item == MENU_RELOAD_CONFIG) {
        XPLMDebugString("TrimGear: Reloading configuration\n");
        load_aircraft_config();
        return;
    }

    // Handle gear up/down and enable/disable actions
    bool config_changed = false;

    switch (item) {
        // Pitch trim controls
        case MENU_PITCH_GEAR_UP: {
            int current_gear = g_config->get_gear_setting(TrimAxis::PITCH);
            if (current_gear < NUM_GEAR_SETTINGS - 1) {
                int new_gear = current_gear + 1;
                g_config->set_gear_setting(TrimAxis::PITCH, new_gear);
                g_trim_controller->set_gear_setting(TrimAxis::PITCH, new_gear);
                config_changed = true;
            }
            break;
        }
        case MENU_PITCH_GEAR_DOWN: {
            int current_gear = g_config->get_gear_setting(TrimAxis::PITCH);
            if (current_gear > 0) {
                int new_gear = current_gear - 1;
                g_config->set_gear_setting(TrimAxis::PITCH, new_gear);
                g_trim_controller->set_gear_setting(TrimAxis::PITCH, new_gear);
                config_changed = true;
            }
            break;
        }
        case MENU_PITCH_ENABLE_DISABLE: {
            bool current_enabled = g_config->get_axis_enabled(TrimAxis::PITCH);
            bool new_enabled = !current_enabled;
            g_config->set_axis_enabled(TrimAxis::PITCH, new_enabled);
            g_trim_controller->set_axis_enabled(TrimAxis::PITCH, new_enabled);
            config_changed = true;
            break;
        }

        // Roll trim controls
        case MENU_ROLL_GEAR_UP: {
            int current_gear = g_config->get_gear_setting(TrimAxis::ROLL);
            if (current_gear < NUM_GEAR_SETTINGS - 1) {
                int new_gear = current_gear + 1;
                g_config->set_gear_setting(TrimAxis::ROLL, new_gear);
                g_trim_controller->set_gear_setting(TrimAxis::ROLL, new_gear);
                config_changed = true;
            }
            break;
        }
        case MENU_ROLL_GEAR_DOWN: {
            int current_gear = g_config->get_gear_setting(TrimAxis::ROLL);
            if (current_gear > 0) {
                int new_gear = current_gear - 1;
                g_config->set_gear_setting(TrimAxis::ROLL, new_gear);
                g_trim_controller->set_gear_setting(TrimAxis::ROLL, new_gear);
                config_changed = true;
            }
            break;
        }
        case MENU_ROLL_ENABLE_DISABLE: {
            bool current_enabled = g_config->get_axis_enabled(TrimAxis::ROLL);
            bool new_enabled = !current_enabled;
            g_config->set_axis_enabled(TrimAxis::ROLL, new_enabled);
            g_trim_controller->set_axis_enabled(TrimAxis::ROLL, new_enabled);
            config_changed = true;
            break;
        }

        // Rudder trim controls
        case MENU_RUDDER_GEAR_UP: {
            int current_gear = g_config->get_gear_setting(TrimAxis::RUDDER);
            if (current_gear < NUM_GEAR_SETTINGS - 1) {
                int new_gear = current_gear + 1;
                g_config->set_gear_setting(TrimAxis::RUDDER, new_gear);
                g_trim_controller->set_gear_setting(TrimAxis::RUDDER, new_gear);
                config_changed = true;
            }
            break;
        }
        case MENU_RUDDER_GEAR_DOWN: {
            int current_gear = g_config->get_gear_setting(TrimAxis::RUDDER);
            if (current_gear > 0) {
                int new_gear = current_gear - 1;
                g_config->set_gear_setting(TrimAxis::RUDDER, new_gear);
                g_trim_controller->set_gear_setting(TrimAxis::RUDDER, new_gear);
                config_changed = true;
            }
            break;
        }
        case MENU_RUDDER_ENABLE_DISABLE: {
            bool current_enabled = g_config->get_axis_enabled(TrimAxis::RUDDER);
            bool new_enabled = !current_enabled;
            g_config->set_axis_enabled(TrimAxis::RUDDER, new_enabled);
            g_trim_controller->set_axis_enabled(TrimAxis::RUDDER, new_enabled);
            config_changed = true;
            break;
        }
    }

    if (config_changed) {
        update_menu_checkmarks();
        g_config->save_config();
    }
}

static int trim_command_handler(XPLMCommandRef, XPLMCommandPhase phase, void* refcon)
{
    if (phase != xplm_CommandBegin || !g_trim_controller) {
        return 0;
    }
    
    // Unpack axis and direction from refcon
    intptr_t packed = reinterpret_cast<intptr_t>(refcon);
    trimgear::constants::TrimAxis axis = static_cast<trimgear::constants::TrimAxis>(packed >> 1);
    bool increase = (packed & 1) != 0;
    
    g_trim_controller->adjust_trim(axis, increase);
    
    return 0;
}

static void load_aircraft_config()
{
    XPLMDebugString("TrimGear: Loading aircraft configuration\n");
    
    if (g_config) {
        g_config->load_config();
        
        // Apply config to trim controller
        if (g_trim_controller) {
            for (int axis = 0; axis < static_cast<int>(trimgear::constants::TrimAxis::COUNT); ++axis) {
                trimgear::constants::TrimAxis trim_axis = static_cast<trimgear::constants::TrimAxis>(axis);
                int gear_setting = g_config->get_gear_setting(trim_axis);
                bool axis_enabled = g_config->get_axis_enabled(trim_axis);
                g_trim_controller->set_gear_setting(trim_axis, gear_setting);
                g_trim_controller->set_axis_enabled(trim_axis, axis_enabled);
            }
        }
        
        update_menu_checkmarks();
    }
}

static void update_menu_checkmarks()
{
    using namespace trimgear::constants;

    if (!g_config || !g_submenu_id) return;

    // Helper function to update axis display
    auto update_axis_display = [&](TrimAxis axis, int header_item, int gear_up_item, int gear_down_item, int enable_disable_item, const char* axis_name) {
        if (header_item == -1 || gear_up_item == -1 || gear_down_item == -1 || enable_disable_item == -1) return;

        int current_gear = g_config->get_gear_setting(axis);
        bool enabled = g_config->get_axis_enabled(axis);
        float current_gear_value = GEAR_SETTINGS[current_gear];

        // Update header with current gear info
        char header_text[trimgear::constants::MENU_TEXT_BUFFER_SIZE];
        int result = std::snprintf(header_text, sizeof(header_text), "%s: %.3f (%d)", axis_name, current_gear_value, current_gear);
        if (result >= 0 && result < static_cast<int>(sizeof(header_text))) {
            XPLMSetMenuItemName(g_submenu_id, header_item, header_text, 0);
        }

        // Update gear up button (show next gear if available)
        if (current_gear < NUM_GEAR_SETTINGS - 1) {
            int next_gear = current_gear + 1;
            float next_gear_value = GEAR_SETTINGS[next_gear];
            char gear_up_text[trimgear::constants::MENU_TEXT_BUFFER_SIZE];
            result = std::snprintf(gear_up_text, sizeof(gear_up_text), "Gear Up (%d) - %.3f", next_gear, next_gear_value);
            if (result >= 0 && result < static_cast<int>(sizeof(gear_up_text))) {
                XPLMSetMenuItemName(g_submenu_id, gear_up_item, gear_up_text, 1);
            }
            XPLMEnableMenuItem(g_submenu_id, gear_up_item, 1);
        } else {
            XPLMSetMenuItemName(g_submenu_id, gear_up_item, "Gear Up (max)", 1);
            XPLMEnableMenuItem(g_submenu_id, gear_up_item, 0);
        }

        // Update gear down button (show previous gear if available)
        if (current_gear > 0) {
            int prev_gear = current_gear - 1;
            float prev_gear_value = GEAR_SETTINGS[prev_gear];
            char gear_down_text[trimgear::constants::MENU_TEXT_BUFFER_SIZE];
            result = std::snprintf(gear_down_text, sizeof(gear_down_text), "Gear Down (%d) - %.3f", prev_gear, prev_gear_value);
            if (result >= 0 && result < static_cast<int>(sizeof(gear_down_text))) {
                XPLMSetMenuItemName(g_submenu_id, gear_down_item, gear_down_text, 1);
            }
            XPLMEnableMenuItem(g_submenu_id, gear_down_item, 1);
        } else {
            XPLMSetMenuItemName(g_submenu_id, gear_down_item, "Gear Down (min)", 1);
            XPLMEnableMenuItem(g_submenu_id, gear_down_item, 0);
        }

        // Update enable/disable button
        XPLMSetMenuItemName(g_submenu_id, enable_disable_item, enabled ? "Enabled" : "Disabled", 1);
        XPLMCheckMenuItem(g_submenu_id, enable_disable_item, enabled ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    };

    // Update all three axes
    update_axis_display(TrimAxis::PITCH, g_pitch_header_item, g_pitch_gear_up_item, g_pitch_gear_down_item, g_pitch_enable_disable_item, "Pitch Trim Gear");
    update_axis_display(TrimAxis::ROLL, g_roll_header_item, g_roll_gear_up_item, g_roll_gear_down_item, g_roll_enable_disable_item, "Roll Trim Gear");
    update_axis_display(TrimAxis::RUDDER, g_rudder_header_item, g_rudder_gear_up_item, g_rudder_gear_down_item, g_rudder_enable_disable_item, "Rudder Trim Gear");
}