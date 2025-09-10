#include "config.h"
#include "XPLMUtilities.h"
#include "XPLMPlanes.h"
#include <fstream>
#include <sstream>
#include <filesystem>

Config::Config() {
    reset_to_defaults();
}

Config::~Config() {
}

void Config::reset_to_defaults() {
    for (size_t i = 0; i < m_gear_settings.size(); ++i) {
        m_gear_settings[i] = trimgear::constants::DEFAULT_GEAR_INDEX;
    }
}

bool Config::load_config(const std::string& aircraft_id) {
    std::string config_path = get_config_file_path(aircraft_id);
    
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::string log_msg = "TrimGear: Config file not found, using defaults: " + config_path + "\n";
        XPLMDebugString(log_msg.c_str());
        reset_to_defaults();
        return true; // Not an error, just use defaults
    }

    reset_to_defaults();
    
    std::string line;
    while (std::getline(config_file, line)) {
        if (!line.empty() && line[0] != '#') {
            parse_config_line(line);
        }
    }
    
    config_file.close();
    
    std::string log_msg = "TrimGear: Loaded config from: " + config_path + "\n";
    XPLMDebugString(log_msg.c_str());
    return true;
}

bool Config::save_config(const std::string& aircraft_id) const {
    std::string config_path = get_config_file_path(aircraft_id);
    
    // Create directory if it doesn't exist
    std::filesystem::path config_file_path(config_path);
    std::filesystem::path config_dir = config_file_path.parent_path();
    
    try {
        if (!std::filesystem::exists(config_dir)) {
            std::filesystem::create_directories(config_dir);
        }
    } catch (const std::exception& e) {
        std::string error_msg = "TrimGear: ERROR - Could not create config directory: ";
        error_msg += e.what();
        error_msg += "\n";
        XPLMDebugString(error_msg.c_str());
        return false;
    }
    
    std::ofstream config_file(config_path);
    if (!config_file.is_open()) {
        std::string error_msg = "TrimGear: ERROR - Could not write config file: " + config_path + "\n";
        XPLMDebugString(error_msg.c_str());
        return false;
    }

    config_file << generate_config_content();
    config_file.close();
    
    std::string log_msg = "TrimGear: Saved config to: " + config_path + "\n";
    XPLMDebugString(log_msg.c_str());
    return true;
}

void Config::set_gear_setting(trimgear::constants::TrimAxis axis, int gear_index) {
    std::size_t axis_index = static_cast<std::size_t>(axis);
    if (axis_index >= m_gear_settings.size()) {
        return;
    }

    if (gear_index < 0 || gear_index >= trimgear::constants::NUM_GEAR_SETTINGS) {
        gear_index = trimgear::constants::DEFAULT_GEAR_INDEX;
    }

    m_gear_settings[axis_index] = gear_index;
}

int Config::get_gear_setting(trimgear::constants::TrimAxis axis) const {
    std::size_t axis_index = static_cast<std::size_t>(axis);
    if (axis_index >= m_gear_settings.size()) {
        return trimgear::constants::DEFAULT_GEAR_INDEX;
    }
    
    return m_gear_settings[axis_index];
}

std::string Config::get_aircraft_directory() const {
    char filename[trimgear::constants::XPLANE_PATH_BUFFER_SIZE];
    char path[trimgear::constants::XPLANE_PATH_BUFFER_SIZE];
    
    XPLMGetNthAircraftModel(0, filename, path);
    
    std::string aircraft_path(path);
    if (!aircraft_path.empty()) {
        // Remove the filename to get just the directory
        size_t last_slash = aircraft_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            aircraft_path = aircraft_path.substr(0, last_slash);
        }
    }
    
    return aircraft_path;
}

std::string Config::get_config_file_path(const std::string& /* aircraft_id */) const {
    std::string aircraft_dir = get_aircraft_directory();
    if (aircraft_dir.empty()) {
        return "TrimGear.cfg"; // Fallback to current directory
    }
    
    return aircraft_dir + "/TrimGear.cfg";
}

bool Config::parse_config_line(const std::string& line) {
    // Remove leading/trailing whitespace
    std::string trimmed_line = line;
    size_t start = trimmed_line.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
        trimmed_line = trimmed_line.substr(start);
    }
    size_t end = trimmed_line.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        trimmed_line = trimmed_line.substr(0, end + 1);
    }
    
    if (trimmed_line.empty()) {
        return true;
    }
    
    // Parse key=value format
    size_t equals_pos = trimmed_line.find('=');
    if (equals_pos == std::string::npos) {
        return false;
    }
    
    std::string key = trimmed_line.substr(0, equals_pos);
    std::string value = trimmed_line.substr(equals_pos + 1);
    
    // Trim key and value
    key.erase(key.find_last_not_of(" \t") + 1);
    key.erase(0, key.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    
    // Parse gear settings
    int gear_index;
    try {
        gear_index = std::stoi(value);
    } catch (const std::exception&) {
        return false;
    }
    
    if (key == "pitch_gear") {
        set_gear_setting(trimgear::constants::TrimAxis::PITCH, gear_index);
    } else if (key == "roll_gear") {
        set_gear_setting(trimgear::constants::TrimAxis::ROLL, gear_index);
    } else if (key == "rudder_gear") {
        set_gear_setting(trimgear::constants::TrimAxis::RUDDER, gear_index);
    } else {
        return false;
    }
    
    return true;
}

std::string Config::generate_config_content() const {
    std::ostringstream content;
    
    content << "# TrimGear Configuration File\n";
    content << "# Gear settings index (0-8) corresponding to values:\n";
    content << "# 0=0.001, 1=0.005, 2=0.01, 3=0.015, 4=0.02, 5=0.025, 6=0.03, 7=0.035, 8=0.04\n";
    content << "\n";
    
    content << "pitch_gear=" << get_gear_setting(trimgear::constants::TrimAxis::PITCH) << "\n";
    content << "roll_gear=" << get_gear_setting(trimgear::constants::TrimAxis::ROLL) << "\n";
    content << "rudder_gear=" << get_gear_setting(trimgear::constants::TrimAxis::RUDDER) << "\n";
    
    return content.str();
}