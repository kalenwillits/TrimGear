# TrimGear Plugin - Decision Log

This document tracks key technical decisions made during the development of the TrimGear plugin.

## Architecture Decisions

### 1. Modular Component Design
**Decision**: Split functionality into separate classes: `TrimController`, `Config`, and main plugin coordination.

**Rationale**: 
- Separation of concerns for maintainability
- Easier testing and debugging of individual components
- Follows the proven multibind plugin architecture
- Allows for independent evolution of trim logic vs configuration management

**Alternatives Considered**: Single monolithic plugin file
**Trade-offs**: Slightly more complex build setup vs much better code organization

### 2. Smart Pointer Usage
**Decision**: Use `std::unique_ptr` for managing `Config` and `TrimController` instances.

**Rationale**:
- Automatic memory management reduces leak risk
- Clear ownership semantics
- Exception safety during initialization
- Modern C++17 best practices

**Alternatives Considered**: Raw pointers, shared_ptr
**Trade-offs**: None significant - unique_ptr is the right tool here

### 3. Configuration Storage Location
**Decision**: Store configuration files in aircraft directories using `XPLMGetNthAircraftModel(0, ...)`.

**Rationale**:
- Per-aircraft settings as required by specifications
- Automatic discovery of current aircraft path
- Integrates with X-Plane's aircraft switching
- Configuration travels with aircraft files

**Alternatives Considered**: Plugin directory storage with aircraft ICAO mapping
**Trade-offs**: Requires write permissions to aircraft directories vs centralized config management

## Data Structure Decisions

### 4. Gear Settings Array
**Decision**: Use fixed array of 9 predefined gear values (0.001 to 0.04) rather than user-configurable ranges.

**Rationale**:
- Simplifies UI to radio button selection
- Prevents invalid/extreme values that could break trim
- Matches requirements specification exactly
- Easier to validate and debug

**Alternatives Considered**: Slider-based continuous adjustment, text input for custom values
**Trade-offs**: Less flexibility vs guaranteed safe operation

### 5. Trim Axis Enumeration
**Decision**: Use enum class `TrimAxis` with PITCH, ROLL, RUDDER values.

**Rationale**:
- Type safety prevents mixing up axes
- Self-documenting code
- Easy to extend if additional trim axes needed
- Matches X-Plane's trim dataref organization

**Alternatives Considered**: String-based axis identification, integer constants
**Trade-offs**: None - enum class is clearly superior

### 6. Command Registration Strategy
**Decision**: Create 6 separate custom commands (up/down for each axis) rather than fewer parameterized commands.

**Rationale**:
- Direct joystick button mapping in X-Plane
- No need for complex parameter passing
- Follows X-Plane command convention patterns
- Easy for users to understand and assign

**Alternatives Considered**: Single command with axis/direction parameters
**Trade-offs**: More commands to register vs simpler user experience

## Implementation Decisions

### 7. Error Handling Strategy
**Decision**: Use graceful degradation with debug logging rather than throwing exceptions.

**Rationale**:
- X-Plane plugin environment doesn't handle exceptions well
- Plugin should not crash X-Plane under any circumstances
- Debug logging helps troubleshooting without disrupting flight
- Follows X-Plane SDK best practices

**Alternatives Considered**: Exception-based error handling, silent failures
**Trade-offs**: More verbose error checking vs plugin stability

### 8. Menu System Implementation
**Decision**: Use X-Plane's native menu system with radio button checkmarks for gear selection.

**Rationale**:
- Integrates with X-Plane's existing UI paradigms
- No need for custom windowing code
- Radio buttons clearly show current selection
- Follows plugin UI conventions

**Alternatives Considered**: Custom floating window, keyboard shortcuts only
**Trade-offs**: Less UI flexibility vs better X-Plane integration

### 9. Configuration File Format
**Decision**: Use simple key=value INI-style format rather than JSON or XML.

**Rationale**:
- Human readable and editable
- No external dependencies for parsing
- Simple parsing logic reduces bugs
- Compact file size

**Alternatives Considered**: JSON, XML, binary format
**Trade-offs**: Less structure vs simplicity and reliability

## Build System Decisions

### 10. CMake Configuration
**Decision**: Adapt multibind's CMakeLists.txt with comprehensive SDK detection and platform support.

**Rationale**:
- Proven build system from working plugin
- Handles cross-platform compilation complexity
- Good error messages for SDK setup issues
- Professional build process with install targets

**Alternatives Considered**: Manual Makefiles, platform-specific build scripts
**Trade-offs**: CMake learning curve vs professional build system

### 11. C++17 Standard
**Decision**: Use C++17 as specified in requirements, following Godot C++ style conventions.

**Rationale**:
- Requirement specification mandates C++17
- Modern language features improve code quality
- std::filesystem for configuration file handling
- Smart pointers and other safety features

**Alternatives Considered**: C++14, C++20
**Trade-offs**: Limited to C++17 features vs using latest standards

## Future Considerations

### 12. Extensibility Decisions
**Decision**: Design allows for future extension to additional trim axes or gear ranges.

**Rationale**:
- `TrimAxis` enum easily extensible
- Gear settings array can be modified
- Menu system accommodates additional sections
- Configuration system supports new parameters

**Potential Extensions**: 
- Flap trim axis
- User-configurable gear ranges
- Trim rate limiting
- Keyboard shortcuts for gear changes

### 13. Performance Considerations
**Decision**: Optimize for reliability over performance in this application.

**Rationale**:
- Trim adjustments are infrequent user actions
- Configuration changes are even less frequent
- Plugin stability more important than microsecond optimization
- Debug logging helps troubleshooting

**Performance Notes**:
- Menu updates only occur on gear changes
- Configuration I/O only on aircraft changes
- No continuous processing loops
- Minimal memory allocation during operation

## Lessons Learned

1. **X-Plane SDK Integration**: Following established patterns from working plugins (multibind) significantly reduced integration issues.

2. **Configuration Management**: Automatic aircraft detection and per-aircraft config storage works seamlessly with X-Plane's aircraft switching.

3. **Error Handling**: Comprehensive logging with graceful degradation prevents plugin-related crashes while maintaining debuggability.

4. **Build System**: Comprehensive SDK detection with helpful error messages greatly improves developer experience.

5. **User Interface**: Native X-Plane menu integration provides better user experience than custom UI elements.

This decision log should be updated as the plugin evolves and new technical decisions are made.