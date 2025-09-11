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

    void reset_to_defaults();

private:
    std::array<int, 3> m_gear_settings;
    
    std::string get_aircraft_directory() const;
    std::string get_config_file_path() const;
    bool parse_config_line(const std::string& line);
    std::string generate_config_content() const;
};