# TrimGear Plugin for X-Plane 12

TrimGear is an X-Plane 12 plugin that addresses hardware limitations when trimming simulated aircraft. Not all joystick trim wheels are geared appropriately for X-Plane's built-in trim commands. This plugin allows users to gear up or gear down their trim sensitivity to match their specific hardware.

## Features

- **Custom Trim Commands**: 6 new commands for geared trim control
  - `trimgear/pitch_trim_up` - Pitch trim up with gear
  - `trimgear/pitch_trim_down` - Pitch trim down with gear  
  - `trimgear/roll_trim_left` - Roll trim left with gear
  - `trimgear/roll_trim_right` - Roll trim right with gear
  - `trimgear/rudder_trim_left` - Rudder trim left with gear
  - `trimgear/rudder_trim_right` - Rudder trim right with gear

- **Configurable Gear Settings**: 9 predefined gear ratios
  - 0.001, 0.005, 0.01, 0.015, 0.02, 0.025, 0.03, 0.035, 0.04

- **Per-Aircraft Configuration**: Settings are automatically saved and loaded for each aircraft

- **Plugin Menu**: Easy gear selection through X-Plane's plugin menu system

## Code Structure

### Core Components

- **`src/trimgear.cpp`** - Main plugin entry point
  - Implements X-Plane plugin callbacks (`XPluginStart`, `XPluginStop`, etc.)
  - Manages custom command creation and registration
  - Handles menu system creation and interaction
  - Coordinates between config and trim controller components

- **`src/trim_controller.h/cpp`** - Trim adjustment logic
  - Manages X-Plane dataref access for trim controls
  - Implements gear-based trim adjustments
  - Handles three trim axes: pitch, roll, rudder
  - Uses datarefs: `sim/flightmodel/controls/elv_trim`, `ail_trim`, `rud_trim`

- **`src/config.h/cpp`** - Configuration management
  - Loads and saves per-aircraft gear settings
  - Manages `TrimGear.cfg` files in aircraft directories
  - Uses `XPLMGetNthAircraftModel(0, ...)` to locate current aircraft
  - Handles configuration parsing and file I/O

- **`src/constants.h`** - Shared constants and enums
  - Defines gear setting values and menu item IDs
  - Contains trim axis enumeration
  - Buffer size constants for X-Plane API calls

### Architecture

The plugin follows a modular design:

1. **Plugin Layer** (`trimgear.cpp`) - Interfaces with X-Plane SDK
2. **Controller Layer** (`trim_controller`) - Business logic for trim adjustments  
3. **Configuration Layer** (`config`) - Persistent storage management
4. **Constants Layer** (`constants`) - Shared definitions

## Building

### Prerequisites

- CMake 3.16 or higher
- C++17 compatible compiler
- X-Plane 12 SDK

### Quick Start (Recommended)

For first-time setup and building:

```bash
# Linux/Mac
./build-all.sh

# Windows
build-all.bat
```

These scripts will automatically download the X-Plane SDK, build the plugin, and provide installation instructions.

### Manual Setup & Build

#### Setup SDK

Option 1: Use setup scripts:
```bash
# Linux/Mac
./setup-sdk.sh

# Windows  
setup-sdk.bat
```

Option 2: Manual setup:
1. Download the X-Plane SDK from [developer.x-plane.com](https://developer.x-plane.com/sdk/plugin-sdk-downloads/)
2. Extract to `SDK/` folder in project directory
3. Alternatively, set `XPLANE_SDK_PATH` environment variable

#### Build Commands

```bash
# Quick build (debug)
./build.sh

# Release build with package
./build.sh release

# Windows
build.bat
build.bat release

# Manual CMake (advanced)
mkdir build && cd build
cmake ..
cmake --build .
cmake --build . --target package
```

### Platform-Specific Outputs

- **Windows**: `build/TrimGear/win.xpl`
- **macOS**: `build/TrimGear/mac.xpl` (Universal Binary)
- **Linux**: `build/TrimGear/lin.xpl`

## Installation

1. Build the plugin following the instructions above
2. Copy the entire `TrimGear` folder to your X-Plane `Resources/plugins/` directory
3. Restart X-Plane

### Directory Structure
```
X-Plane 12/
└── Resources/
    └── plugins/
        └── TrimGear/
            └── win.xpl (or mac.xpl/lin.xpl)
```

## Usage

### Basic Operation

1. **Assign Commands**: In X-Plane's joystick settings, assign the TrimGear commands to your trim controls:
   - `trimgear/pitch_trim_up` 
   - `trimgear/pitch_trim_down`
   - `trimgear/roll_trim_left`
   - `trimgear/roll_trim_right` 
   - `trimgear/rudder_trim_left`
   - `trimgear/rudder_trim_right`

2. **Adjust Gear Settings**: Use the plugin menu (`Plugins > TrimGear`) to select appropriate gear ratios for each trim axis

3. **Per-Aircraft Settings**: Gear settings are automatically saved for each aircraft in a `TrimGear.cfg` file

### Menu System

The plugin menu provides:
- **Reload Configuration**: Manually reload settings for current aircraft
- **Pitch Trim Gear**: 9 radio button options (0.001 to 0.04)
- **Roll Trim Gear**: 9 radio button options (0.001 to 0.04)  
- **Rudder Trim Gear**: 9 radio button options (0.001 to 0.04)

### Configuration Files

Configuration files are automatically created in each aircraft's directory as `TrimGear.cfg`:

```ini
# TrimGear Configuration File
# Gear settings index (0-8) corresponding to values:
# 0=0.001, 1=0.005, 2=0.01, 3=0.015, 4=0.02, 5=0.025, 6=0.03, 7=0.035, 8=0.04

pitch_gear=2
roll_gear=2  
rudder_gear=2
```

## Troubleshooting

### Common Issues

1. **Plugin Not Loading**
   - Check Log.txt for error messages
   - Verify SDK installation and platform-specific libraries
   - Ensure plugin is in correct directory structure

2. **Commands Not Working**
   - Verify commands are properly assigned in X-Plane joystick settings
   - Check that trim datarefs are available for current aircraft
   - Review Log.txt for dataref errors

3. **Configuration Not Saving**
   - Check aircraft directory permissions
   - Verify aircraft path detection is working
   - Look for filesystem errors in Log.txt

### Debug Information

The plugin writes detailed logging to X-Plane's Log.txt file with prefix "TrimGear:". Common log messages include:

- Plugin initialization status
- Dataref discovery results  
- Configuration loading/saving operations
- Gear setting changes
- Trim adjustment actions

## Requirements

- X-Plane 12
- Compatible joystick/trim hardware
- Windows 10+, macOS 10.14+, or Linux with X11

## License

This project follows the same licensing as the multibind reference project.