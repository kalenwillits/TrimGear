 


TrimGear:
- An XPLane 12 plugin to address hardware limitations when trimming simulated aircraft

The issue being, not all joystick trim wheels are geared to what XPLane's built-in commands are expecting.

This plugin will allow users to gear up or gear down their trim to tune it to their specific hardware

## Tech
- C++ 17
- Godot C++ 17 coding style conventions
    - Use the std lib wherever possible
- X Plane 12 SDK

## Impl Strategy
Create new custom commands for trim gear of:
- pitch trim up
- pitch trim down
- roll trim left
- roll trim right
- rudder trim left
- rudder trim right


Within the `Plugins/TrimGear` menu:
- Have 3 sections for each axis: pitch, roll, yaw
- Each axis will have 9 static 'gear' settings 
- Each section will be a radio button selection of that axis's gear
- the gear will be a float 0.001, 0.005, 0.01, 0.015, 0.02, 0.025, 0.03, 0.035, 0.04 


The equation to adjust the trim is: 
`trim_ref = trim_ref +/- axis_gear`

Each aircraft should persist the selected gear setting in a saved `TrimGear.cfg` file
The config file will be in the current aircarft's directory
You may have to do some research to find it, but use the SDK's XPLMGetNthAircraftModel(0, FileName, TempPAth). The aircraft at index 0 is the currently loaded aircraft.

** some examples are written in lua, but this project should have 0 lines of lua in it
Using these datarefs:
```lua
dataref("elv_trim_ref", "sim/flightmodel/controls/elv_trim", "writable")
dataref("ail_trim_ref", "sim/flightmodel/controls/ail_trim", "writable")
dataref("rud_trim_ref", "sim/flightmodel/controls/rud_trim", "writable")
```



# Resources
[X Plane SDK docs home](https://developer.x-plane.com/sdk/)
[X Plane SDK docuemnts](https://developer.x-plane.com/sdk/plugin-sdk-documents/)
[X Plane SDK Downloads](https://developer.x-plane.com/sdk/plugin-sdk-downloads/)
[X Plane SDK Sample Code](https://developer.x-plane.com/sdk/plugin-sdk-sample-code/)
[X Plane forums](https://forums.x-plane.org/)
[FlyWithLua -- A complex plugin](https://github.com/X-Friese/FlyWithLua)
[Example of how to detect key presses](https://developer.x-plane.com/code-sample/keysniffer/)


# Requirements 
- The result must include complete feature code and,
- a decision log
- A README.md going over the code structure
- Details on compiling
- Install instructions

