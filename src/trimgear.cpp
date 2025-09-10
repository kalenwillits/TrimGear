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
static XPLMDataRef g_aircraft_icao_ref = nullptr;
static std::string g_last_aircraft_icao;

// Menu tracking
static std::vector<int> g_pitch_menu_items;
static std::vector<int> g_roll_menu_items;
static std::vector<int> g_rudder_menu_items;

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

    // Find aircraft ICAO dataref for aircraft change detection
    g_aircraft_icao_ref = XPLMFindDataRef("sim/aircraft/view/acf_ICAO");
    if (!g_aircraft_icao_ref) {
        XPLMDebugString("TrimGear: ERROR - Could not find aircraft ICAO dataref\n");
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
    if (g_config && !g_last_aircraft_icao.empty()) {
        g_config->save_config(g_last_aircraft_icao);
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
    XPLMAppendMenuItem(g_submenu_id, "Pitch Trim Gear:", nullptr, 0);
    g_pitch_menu_items.reserve(NUM_GEAR_SETTINGS);
    for (int i = 0; i < NUM_GEAR_SETTINGS; ++i) {
        char label_buffer[32];
        std::snprintf(label_buffer, sizeof(label_buffer), "  %.3f", GEAR_SETTINGS[i]);
        std::string label = label_buffer;
        int item_id = XPLMAppendMenuItem(g_submenu_id, label.c_str(), 
                                        reinterpret_cast<void*>(MENU_PITCH_GEAR_START + i), 1);
        g_pitch_menu_items.push_back(item_id);
    }
    
    // Add separator
    XPLMAppendMenuSeparator(g_submenu_id);
    
    // Roll trim section
    XPLMAppendMenuItem(g_submenu_id, "Roll Trim Gear:", nullptr, 0);
    g_roll_menu_items.reserve(NUM_GEAR_SETTINGS);
    for (int i = 0; i < NUM_GEAR_SETTINGS; ++i) {
        char label_buffer[32];
        std::snprintf(label_buffer, sizeof(label_buffer), "  %.3f", GEAR_SETTINGS[i]);
        std::string label = label_buffer;
        int item_id = XPLMAppendMenuItem(g_submenu_id, label.c_str(), 
                                        reinterpret_cast<void*>(MENU_ROLL_GEAR_START + i), 1);
        g_roll_menu_items.push_back(item_id);
    }
    
    // Add separator
    XPLMAppendMenuSeparator(g_submenu_id);
    
    // Rudder trim section
    XPLMAppendMenuItem(g_submenu_id, "Rudder Trim Gear:", nullptr, 0);
    g_rudder_menu_items.reserve(NUM_GEAR_SETTINGS);
    for (int i = 0; i < NUM_GEAR_SETTINGS; ++i) {
        char label_buffer[32];
        std::snprintf(label_buffer, sizeof(label_buffer), "  %.3f", GEAR_SETTINGS[i]);
        std::string label = label_buffer;
        int item_id = XPLMAppendMenuItem(g_submenu_id, label.c_str(), 
                                        reinterpret_cast<void*>(MENU_RUDDER_GEAR_START + i), 1);
        g_rudder_menu_items.push_back(item_id);
    }
}

static void menu_handler(void*, void* item_ref)
{
    using namespace trimgear::constants;
    
    intptr_t item = reinterpret_cast<intptr_t>(item_ref);
    
    if (item == MENU_RELOAD_CONFIG) {
        XPLMDebugString("TrimGear: Reloading configuration\n");
        g_last_aircraft_icao.clear();
        load_aircraft_config();
        return;
    }
    
    // Handle gear selection
    if (item >= MENU_PITCH_GEAR_START && item < MENU_PITCH_GEAR_START + NUM_GEAR_SETTINGS) {
        int gear_index = item - MENU_PITCH_GEAR_START;
        g_config->set_gear_setting(TrimAxis::PITCH, gear_index);
        g_trim_controller->set_gear_setting(TrimAxis::PITCH, gear_index);
        update_menu_checkmarks();
    } else if (item >= MENU_ROLL_GEAR_START && item < MENU_ROLL_GEAR_START + NUM_GEAR_SETTINGS) {
        int gear_index = item - MENU_ROLL_GEAR_START;
        g_config->set_gear_setting(TrimAxis::ROLL, gear_index);
        g_trim_controller->set_gear_setting(TrimAxis::ROLL, gear_index);
        update_menu_checkmarks();
    } else if (item >= MENU_RUDDER_GEAR_START && item < MENU_RUDDER_GEAR_START + NUM_GEAR_SETTINGS) {
        int gear_index = item - MENU_RUDDER_GEAR_START;
        g_config->set_gear_setting(TrimAxis::RUDDER, gear_index);
        g_trim_controller->set_gear_setting(TrimAxis::RUDDER, gear_index);
        update_menu_checkmarks();
    }
    
    // Save configuration after any gear change
    if (!g_last_aircraft_icao.empty()) {
        g_config->save_config(g_last_aircraft_icao);
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
    using namespace trimgear::constants;
    
    std::string aircraft_icao(XPLANE_PATH_BUFFER_SIZE, '\0');
    int bytes_read = XPLMGetDatab(g_aircraft_icao_ref, &aircraft_icao[0], 0, aircraft_icao.size() - 1);
    if (bytes_read > 0) {
        aircraft_icao[bytes_read] = '\0'; // Ensure null termination
        aircraft_icao.resize(bytes_read);
    } else {
        aircraft_icao.clear();
    }
    
    if (aircraft_icao != g_last_aircraft_icao) {
        // Save old config before loading new one
        if (!g_last_aircraft_icao.empty() && g_config) {
            g_config->save_config(g_last_aircraft_icao);
        }
        
        g_last_aircraft_icao = aircraft_icao;
        
        std::string log_msg = "TrimGear: Loading config for aircraft: " + aircraft_icao + "\n";
        XPLMDebugString(log_msg.c_str());
        
        if (g_config) {
            g_config->load_config(aircraft_icao);
            
            // Apply config to trim controller
            if (g_trim_controller) {
                for (int axis = 0; axis < static_cast<int>(TrimAxis::COUNT); ++axis) {
                    TrimAxis trim_axis = static_cast<TrimAxis>(axis);
                    int gear_setting = g_config->get_gear_setting(trim_axis);
                    g_trim_controller->set_gear_setting(trim_axis, gear_setting);
                }
            }
            
            update_menu_checkmarks();
        }
    }
}

static void update_menu_checkmarks()
{
    using namespace trimgear::constants;
    
    if (!g_config || !g_submenu_id) return;
    
    // Update pitch gear checkmarks
    int pitch_gear = g_config->get_gear_setting(TrimAxis::PITCH);
    for (int i = 0; i < NUM_GEAR_SETTINGS && i < static_cast<int>(g_pitch_menu_items.size()); ++i) {
        XPLMCheckMenuItem(g_submenu_id, g_pitch_menu_items[i], 
                        (i == pitch_gear) ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    }
    
    // Update roll gear checkmarks
    int roll_gear = g_config->get_gear_setting(TrimAxis::ROLL);
    for (int i = 0; i < NUM_GEAR_SETTINGS && i < static_cast<int>(g_roll_menu_items.size()); ++i) {
        XPLMCheckMenuItem(g_submenu_id, g_roll_menu_items[i], 
                        (i == roll_gear) ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    }
    
    // Update rudder gear checkmarks
    int rudder_gear = g_config->get_gear_setting(TrimAxis::RUDDER);
    for (int i = 0; i < NUM_GEAR_SETTINGS && i < static_cast<int>(g_rudder_menu_items.size()); ++i) {
        XPLMCheckMenuItem(g_submenu_id, g_rudder_menu_items[i], 
                        (i == rudder_gear) ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    }
}