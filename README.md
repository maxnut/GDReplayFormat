# Geometry Dash Replay Format

This format is pretty simple so this'll be the documentation until someone with more than a quarter of a brain fixes this up.

## The Basics

- GDReplayFormat is a macro format meant to work for ALL bots, which means all GDR files can be read and used by all bots
- `.gdr.json` files are in JSON and `.gdr` are in msgpack.
- **Bot developers like you can add your own fields to GDR (like position data/frame fixes if you want) using the [Extensions](#extensions) feature**

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
CPMAddPackage("gh:maxnut/GDReplayFormat#commithash") # REPLACE commithash with a specific commit hash
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

Exporting

```cpp
MyMacro macro;

std::ofstream f("path/to/macro.gdr", std::ios::binary);
auto data = macro.exportData(false);

f.write(reinterpret_cast<const char *>(data.data()), data.size());
f.close();
```

Importing

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

Exporting

```cpp
MyMacro macro;

std::ofstream f("path/to/macro.gdr.json", std::ios::binary);
auto data = macro.exportData(true);

f.write(reinterpret_cast<const char *>(data.data()), data.size());
f.close();
```

Importing

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

## Extensions

The GDR Extensions system allows any bot to add it's own fields to a whole macro or the individual inputs in a macro for it's own use while still keeping GDR standardized and usable for every other bot that may use it. The most common use of this system is to add position and velocity data to inputs to allow frame corrections.

### Adding your own extensions

Put simply, if you want to add X position to an input in my macros, you need to add it to your Input struct and add a way to save/parse it, like this:

```cpp
struct MyInput : Input {
	float xpos;

	MyInput() = default;

	MyInput(int frame, int button, bool player2, bool down, float xpos)
		: Input(frame, button, player2, down), xpos(xpos) {}

	void parseExtension(gdr::json::object_t obj) override {
		xpos = obj["xpos"];
	}

	gdr::json::object_t saveExtension() const override {
		return { {"xpos", xpos} };
	}
};
```

Now, if you wanted to add a field (for this example, we will use Attempts) to your whole macro, the process would be very similar:

```cpp
struct MyReplay : Replay<MyReplay, MyInput> {
	int attempts;

	MyReplay() : Replay("TestBot", "1.0") {}

	void parseExtension(gdr::json::object_t obj) override {
		attempts = obj["attempts"];
	}

	gdr::json::object_t saveExtension() const override {
		return { {"attempts", attempts} };
	}
};
```

These extensions can be accessed the same way you access regular fields

## Bots using GDR

 - [MegaHack v8](https://absolllute.com) by Absolute
 - [OpenHack](https://github.com/Prevter/OpenHack/) by Prevter
 - [GD Mega Overlay](https://github.com/maxnut/GDMegaOverlay) by maxnut
 - [Crystal Client v5](https://github.com/ninXout/Crystal-Client) by ninXout
 - [iCreate Pro](https://icreate.pro) by camila314

## Credits

[maxnut](https://github.com/maxnut): For starting the whole repo and format in the first place and being the biggest contributor

[camila314](https://github.com/camila314): For fixing up most of the format and doing the testing i think

[ninXout](https://github.com/ninXout): For writing this "documentation" you are seeing here (also open for dms on discord at @ninxout if you have questions about gdr)

[Absolute](https://github.com/Absolllute), [fig](https://github.com/figmentboy), [decks](https://github.com/lcm7341), [CattoDev](https://github.com/CattoDev), [kepe](https://github.com/kepexx), and [mat](https://github.com/matcool): For being supportive of this idea, and willing to put the format in their own bots immediately.