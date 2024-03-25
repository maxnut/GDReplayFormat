# Geometry Dash Replay Format

This format is pretty simple so this'll be the documentation until someone with more than a quarter of a brain fixes this up.

## The Basics

`.gdr.json` files are in JSON and `.gdr` are in msgpack. The parts of the format are as shown below.

### Replay

```cpp
std::string author; // author of the macro, most likely the GD username of the user
std::string description; // description of the macro, not necessary
float duration; // full macro duration time (Example: 64.729)
float gameVersion; // corresponding game version (Example: 2.204)
float version = 1.0; // GDR version (SHOULD NOT BE CHANGED)
float framerate = 240.f; // macro framerate (240 by default)
int seed = 0; // random trigger seed for the level, used to manipulate random triggers
int coins = 0; // amount of coins done by macro
bool ldm = false; // if the macro author used low detail mode to record
gdr::Bot botInfo; // Refer to Bot section
gdr::Level levelInfo; // Refer to Level section
std::vector<gdr::Input> inputs; // refer to Input section
```

### Bot

```cpp
std::string name; // name of the bot (SHOULD NOT BE SET MORE THAN ONCE)
std::string version; // version of the bot (SHOULD NOT BE SET MORE THAN ONCE)
```

### Level

```cpp
uint32_t id; // Level ID of the level being botted
std::string name; // Name of the level being botted
```

### Input

```cpp
uint32_t frame; // frame (time * framerate) of the input
int button; // PlayerButton of the input
bool player2; // if it is a player 2 input
bool down; // if it is a hold or release input
```

## Adding to your project

**Assuming you are using [CPM](https://github.com/cpm-cmake/CPM.cmake) as it is already in all Geode mods**

Add CPM dependency (make sure it is added AFTER the `add_subdirectory` line and BEFORE the `setup_geode_mod` line)

```
CPMAddPackage("gh:maxnut/GDReplayFormat#4950cc2")
target_link_libraries(${PROJECT_NAME} libGDR)
```

Add this to your geode mod and add your bot's name and version to the Replay

```cpp
#include <gdr/gdr.hpp>

struct MyInput : gdr::Input {
    MyInput() = default;

    MyInput(int frame, int button, bool player2, bool down)
        : Input(frame, button, player2, down) {}   
};

struct MyMacro : gdr::Replay<MyMacro, MyInput> {
    MyMacro() : Replay("MyBot", "1.0") {}
};
```

## Importing and Exporting

### Using .gdr

Importing

```cpp
MyMacro macro;

std::ofstream f("path/to/macro.gdr", std::ios::binary);
auto data = macro.exportData(false);

f.write(reinterpret_cast<const char *>(data.data()), data.size());
f.close();
```

Exporting

```cpp
MyMacro macro;

std::ifstream f("path/to/macro.gdr", std::ios::binary);

f.seekg(0, std::ios::end);
size_t fileSize = f.tellg();
f.seekg(0, std::ios::beg);

std::vector<std::uint8_t> macroData(fileSize);

f.read(reinterpret_cast<char *>(macroData.data()), fileSize);
f.close();

macro = MyMacro::importData(macroData);
```

### Using .gdr.json

Importing

```cpp
MyMacro macro;

std::ofstream f("path/to/macro.gdr.json", std::ios::binary);
auto data = macro.exportData(true);

f.write(reinterpret_cast<const char *>(data.data()), data.size());
f.close();
```

Exporting

```cpp
MyMacro macro;

std::ifstream f("path/to/macro.gdr.json", std::ios::binary);

f.seekg(0, std::ios::end);
size_t fileSize = f.tellg();
f.seekg(0, std::ios::beg);

std::vector<std::uint8_t> macroData(fileSize);

f.read(reinterpret_cast<char *>(macroData.data()), fileSize);
f.close();

macro = MyMacro::importData(macroData);
```

# Extensions

i'll write this part later but until then go to [test.cpp](./test/main.cpp)