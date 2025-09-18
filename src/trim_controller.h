#pragma once

#include "XPLMDataAccess.h"
#include "constants.h"
#include <array>
#include <cstddef>

class TrimController {
public:
    TrimController();
    ~TrimController();

    bool initialize();
    void cleanup();

    void adjust_trim(trimgear::constants::TrimAxis axis, bool increase);
    
    void set_gear_setting(trimgear::constants::TrimAxis axis, int gear_index);
    int get_gear_setting(trimgear::constants::TrimAxis axis) const;
    float get_gear_value(trimgear::constants::TrimAxis axis) const;

    void set_axis_enabled(trimgear::constants::TrimAxis axis, bool enabled);
    bool get_axis_enabled(trimgear::constants::TrimAxis axis) const;

private:
    struct TrimAxisData {
        XPLMDataRef dataref;
        int gear_index;
        float gear_value;
        bool enabled;
        const char* dataref_name;
    };

    std::array<TrimAxisData, static_cast<std::size_t>(trimgear::constants::TrimAxis::COUNT)> m_trim_axes;
    bool m_initialized;

    void update_gear_value(trimgear::constants::TrimAxis axis);
};