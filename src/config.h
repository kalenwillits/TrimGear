#pragma once

#include "constants.h"
#include <string>
#include <array>

class Config {
public:
    Config();
    ~Config();

    bool load_config();
    bool save_config() const;

    void set_gear_setting(trimgear::constants::TrimAxis axis, int gear_index);
    int get_gear_setting(trimgear::constants::TrimAxis axis) const;

    void set_axis_enabled(trimgear::constants::TrimAxis axis, bool enabled);
    bool get_axis_enabled(trimgear::constants::TrimAxis axis) const;

    void reset_to_defaults();

private:
    std::array<int, static_cast<std::size_t>(trimgear::constants::TrimAxis::COUNT)> m_gear_settings;
    std::array<bool, static_cast<std::size_t>(trimgear::constants::TrimAxis::COUNT)> m_axis_enabled;
    
    std::string get_aircraft_directory() const;
    std::string get_config_file_path() const;
    bool create_default_config() const;
    bool parse_config_line(const std::string& line);
    std::string generate_config_content() const;
};