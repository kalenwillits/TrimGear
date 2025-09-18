#include "trim_controller.h"
#include "XPLMUtilities.h"
#include <string>
#include <algorithm>

TrimController::TrimController() 
    : m_initialized(false)
{
    // Initialize trim axes data
    m_trim_axes[static_cast<std::size_t>(trimgear::constants::TrimAxis::PITCH)] = {
        nullptr,
        trimgear::constants::DEFAULT_GEAR_INDEX,
        trimgear::constants::GEAR_SETTINGS[trimgear::constants::DEFAULT_GEAR_INDEX],
        true, // enabled by default
        "sim/flightmodel/controls/elv_trim"
    };

    m_trim_axes[static_cast<std::size_t>(trimgear::constants::TrimAxis::ROLL)] = {
        nullptr,
        trimgear::constants::DEFAULT_GEAR_INDEX,
        trimgear::constants::GEAR_SETTINGS[trimgear::constants::DEFAULT_GEAR_INDEX],
        true, // enabled by default
        "sim/flightmodel/controls/ail_trim"
    };

    m_trim_axes[static_cast<std::size_t>(trimgear::constants::TrimAxis::RUDDER)] = {
        nullptr,
        trimgear::constants::DEFAULT_GEAR_INDEX,
        trimgear::constants::GEAR_SETTINGS[trimgear::constants::DEFAULT_GEAR_INDEX],
        true, // enabled by default
        "sim/flightmodel/controls/rud_trim"
    };
}

TrimController::~TrimController() {
    cleanup();
}

bool TrimController::initialize() {
    if (m_initialized) {
        return true;
    }

    bool success = true;
    
    // Find all required datarefs
    for (auto& axis_data : m_trim_axes) {
        axis_data.dataref = XPLMFindDataRef(axis_data.dataref_name);
        if (!axis_data.dataref) {
            std::string error_msg = "TrimGear: ERROR - Could not find dataref: ";
            error_msg += axis_data.dataref_name;
            error_msg += "\n";
            XPLMDebugString(error_msg.c_str());
            success = false;
        }
    }

    if (success) {
        m_initialized = true;
        XPLMDebugString("TrimGear: Trim controller initialized successfully\n");
    } else {
        XPLMDebugString("TrimGear: ERROR - Failed to initialize trim controller\n");
    }

    return success;
}

void TrimController::cleanup() {
    m_initialized = false;
}

void TrimController::adjust_trim(trimgear::constants::TrimAxis axis, bool increase) {
    if (!m_initialized) {
        return;
    }

    std::size_t axis_index = static_cast<std::size_t>(axis);
    if (axis_index >= m_trim_axes.size()) {
        return;
    }

    TrimAxisData& axis_data = m_trim_axes[axis_index];
    if (!axis_data.dataref || !axis_data.enabled) {
        return; // Skip if axis is disabled
    }

    // Get current trim value
    float current_trim = XPLMGetDataf(axis_data.dataref);
    
    // Apply gear adjustment
    float adjustment = increase ? axis_data.gear_value : -axis_data.gear_value;
    float new_trim = current_trim + adjustment;
    
    // Clamp trim values to reasonable bounds (-1.0 to 1.0)
    new_trim = std::max<float>(-1.0f, std::min<float>(1.0f, new_trim));
    
    // Set the new trim value
    XPLMSetDataf(axis_data.dataref, new_trim);
    
    // Debug output
    std::string log_msg = "TrimGear: Adjusted ";
    log_msg += axis_data.dataref_name;
    log_msg += " by ";
    log_msg += std::to_string(adjustment);
    log_msg += " (gear: ";
    log_msg += std::to_string(axis_data.gear_value);
    log_msg += ")\n";
    XPLMDebugString(log_msg.c_str());
}

void TrimController::set_gear_setting(trimgear::constants::TrimAxis axis, int gear_index) {
    std::size_t axis_index = static_cast<std::size_t>(axis);
    if (axis_index >= m_trim_axes.size()) {
        return;
    }

    if (gear_index < 0 || gear_index >= trimgear::constants::NUM_GEAR_SETTINGS) {
        return;
    }

    m_trim_axes[axis_index].gear_index = gear_index;
    update_gear_value(axis);
    
    // Debug output
    std::string log_msg = "TrimGear: Set gear for ";
    log_msg += m_trim_axes[axis_index].dataref_name;
    log_msg += " to index ";
    log_msg += std::to_string(gear_index);
    log_msg += " (value: ";
    log_msg += std::to_string(m_trim_axes[axis_index].gear_value);
    log_msg += ")\n";
    XPLMDebugString(log_msg.c_str());
}

int TrimController::get_gear_setting(trimgear::constants::TrimAxis axis) const {
    std::size_t axis_index = static_cast<std::size_t>(axis);
    if (axis_index >= m_trim_axes.size()) {
        return trimgear::constants::DEFAULT_GEAR_INDEX;
    }
    
    return m_trim_axes[axis_index].gear_index;
}

float TrimController::get_gear_value(trimgear::constants::TrimAxis axis) const {
    std::size_t axis_index = static_cast<std::size_t>(axis);
    if (axis_index >= m_trim_axes.size()) {
        return trimgear::constants::GEAR_SETTINGS[trimgear::constants::DEFAULT_GEAR_INDEX];
    }
    
    return m_trim_axes[axis_index].gear_value;
}

void TrimController::set_axis_enabled(trimgear::constants::TrimAxis axis, bool enabled) {
    std::size_t axis_index = static_cast<std::size_t>(axis);
    if (axis_index >= m_trim_axes.size()) {
        return;
    }

    m_trim_axes[axis_index].enabled = enabled;

    // Debug output
    std::string log_msg = "TrimGear: ";
    log_msg += enabled ? "Enabled" : "Disabled";
    log_msg += " axis ";
    log_msg += m_trim_axes[axis_index].dataref_name;
    log_msg += "\n";
    XPLMDebugString(log_msg.c_str());
}

bool TrimController::get_axis_enabled(trimgear::constants::TrimAxis axis) const {
    std::size_t axis_index = static_cast<std::size_t>(axis);
    if (axis_index >= m_trim_axes.size()) {
        return true; // Default to enabled
    }

    return m_trim_axes[axis_index].enabled;
}

void TrimController::update_gear_value(trimgear::constants::TrimAxis axis) {
    std::size_t axis_index = static_cast<std::size_t>(axis);
    if (axis_index >= m_trim_axes.size()) {
        return;
    }

    TrimAxisData& axis_data = m_trim_axes[axis_index];
    if (axis_data.gear_index >= 0 && axis_data.gear_index < trimgear::constants::NUM_GEAR_SETTINGS) {
        axis_data.gear_value = trimgear::constants::GEAR_SETTINGS[axis_data.gear_index];
    } else {
        axis_data.gear_value = trimgear::constants::GEAR_SETTINGS[trimgear::constants::DEFAULT_GEAR_INDEX];
    }
}