# Geometry Dash Replay Format

GDReplayFormat is a replay format meant to be usable by all bots.

## The Basics

Replay data is stored in a compact binary format using:
- Variable-length integer encoding (`varint`) to reduce storage size.
- Delta encoding for frame data to reduce storage size.
- Input packing for optimal storage (single byte in best-case scenario)
- Optional extension support for custom replay metadata and input types.

**Bot developers like you can add your own fields to GDR (like position data/frame fixes if you want) using the [Extensions](#extensions) feature**

If extensions are present, they are parsed only if they match the expected input tag.

### Replay

```cpp
std::string author;             /* Replay author. Usually the nickname of the person who recorded it. */
std::string description;        /* Replay description. Not required. */
float duration{};               /* Duration of the replay in seconds. */
int gameVersion{};              /* Game version the replay was recorded on. (Example: 22074 for 2.2074, refer to GEODE_COMP_GD_VERSION) */
double framerate = 240.0;       /* Framerate (ticks per second) of the replay. 240 by default, change if replay is recorded using physics bypass. */
int seed = 0;                   /* Random seed set when at the start of the attempt. */
int coins = 0;                  /* Number of coins collected in the level. */
bool ldm = false;               /* Whether the replay was recorded in low detail mode. */
bool platformer = false;        /* Whether the replay was recorded in platformer mode. */
Bot botInfo{};                  /* Information about the bot that recorded the replay. */
Level levelInfo{};              /* Information about the level the replay was recorded on. */
std::vector<InputType> inputs;  /* Refer to inputs section. */
std::vector<uint64_t> deaths;   /* List of frames where a death should happen. */
```

### Bot

```cpp
std::string name; /* Name of the bot that recorded the replay. */
int version = 1;  /* Version of the bot that recorded the replay. */
```

### Level

```cpp
uint32_t id{};    /* ID of the level the replay was recorded on. */
std::string name; /* Name of the level the replay was recorded on. */
```

### Input

```cpp
uint64_t frame{}; /* Frame that the input was recorded on. */
uint8_t button{}; /* Pressed button. 1 = Jump, 2 = Left, 3 = Right. Can be converted directly to PlayerButton enum. */
bool player2{};   /* Whether the input was for player 2. */
bool down{};      /* Whether the button was pressed or released. */
```

## Best practices

Please stick to these point to guarantee replay compatibility with other bots:

- Use a frame counter to store frames. Multiplying time can lead to inaccuracy in frame counting.
- The first frame of a replay should be 0.
- When saving the replay to a file, use the ```.gdr2``` extension to better identify newer replays.

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

struct MyBot : gdr::Replay<MyBot, gdr::Input<>> {
    MyBot() : Replay("MyBot", 1) {}
};
```

## Extensions

The GDR Extensions system allows any bot to add it's own fields to a whole replay or the individual inputs in a replay for it's own use while still keeping GDR standardized and usable for every other bot that may use it. The most common use of this system is to add position and velocity data to inputs to allow frame corrections.

**If you plan on using extensions to add physics fixes to your replays, please use the already provided PhysicsInput!**

### Adding your own extensions

Put simply, if you want to add X position to an input in my replays, you need to add it to your Input struct and add a way to save/parse it, like this:

```cpp
struct MyInput : Input<"MyInput"> {
    float xpos;

    MyInput() = default;

    MyInput(uint64_t frame, uint8_t button, bool player2, bool down, float xpos)
        : Input(frame, button, player2, down), xpos(xpos) {}

    void parseExtension(binary_reader& reader) override {
        reader >> xpos;
    }

    void saveExtension(binary_writer& writer) const override {
        writer << xpos;
    }
};
```

Now, if you wanted to add a field (for this example, we will use Attempts) to your whole replay, the process would be very similar:

```cpp
struct MyReplay : Replay<MyReplay, MyInput> {
    int attempts;

    MyReplay() : Replay("TestBot", 1) {}

    void parseExtension(binary_reader& reader) override {
        reader >> attempts;
    }

    void saveExtension(binary_writer& writer) const override {
        writer << attempts;
    }

    bool shouldParseExtension() const override {
        return botInfo.name == "TestBot" && botInfo.version == 1;
    }
};
```

These extensions can be accessed the same way you access regular fields

### Extension parsing

Extension parsing depends on two things:
- Replay's `shouldParseExtension()` virtual method, that you should override and implement logic to check if extensions should be parsed (or just `return true`)
- The Replay's input tag, which will be compared against the input tag of the loaded replay

If both conditions are matched, the extensions will be parsed. Otherwise they will be skipped.

## Migrating from GDR 1

- The file format has completely changed. If you want to keep GDR 1 support we suggest using the [converter](https://github.com/maxnut/GDR-converter).
- To check if the GDR file is newer than 1.0, you can check if the first three bytes of the data equal to "GDR".
- The ```importData``` and ```exportData``` methods now return a result type, for better error handling.

## File Structure
Each GDR file consists of the following sections:

1. **Header**
2. **Replay Metadata**
4. **Death Frames**
5. **Input Data**

## Data types
| Type     | Description |
|----------|-------------|
| `varint` | LEB128 variable size integer |
| `string` | Null terminated string (cstring) |
| `bool`   | Single byte |


## Header
| Field         | Type  | Description                         |
|--------------|------|---------------------------------|
| Magic        | 3 Bytes (ASCII) | File identifier ("GDR") |
| Version      | `varint` | Replay format version |
| Input Tag    | `string` | Identifier for custom input types |

## Replay Metadata
| Field         | Type      | Description |
|--------------|----------|-------------|
| Author      | `string`  | Author of the replay |
| Description | `string`  | Description of the replay |
| Duration    | `varint`  | Length of the replay in frames |
| Game Version | `varint` | Version of the game |
| Framerate   | `varint`  | FPS of the recording |
| Seed        | `varint`  | Random seed used in gameplay |
| Coins       | `varint`  | Number of coins collected |
| LDM Enabled | `bool`    | Whether "Low Detail Mode" was enabled |
| Platformer  | `bool`    | Whether the replay is platformer mode |
| Bot Name    | `string`  | Name of the bot used to record the replay |
| Bot Version | `varint`  | Version of the bot |
| Level ID    | `varint`  | Level identifier |
| Level Name  | `string`  | Name of the level |
| Extension Size  | `varint`  | Size of the extension block |
| Extension Data  | `bytes`   | Custom extension data (if applicable) |

## Death Frames
| Field         | Type      | Description |
|--------------|----------|-------------|
| Death Count  | `varint`  | Number of deaths recorded |
| Death Frames | `varint[]` | Frame numbers where deaths occurred (delta-encoded) |

## Input Data
| Field         | Type      | Description |
|--------------|----------|-------------|
| Input Count  | `varint`  | Number of input records |
| P1 Input Count  | `varint`  | Number of player1 input records |
| Inputs       | `varint + extension (optional)` | Input events |

The inputs are stored as all P1 Inputs and then all P2 Inputs. Use P1 Input Count to know when P2 Inputs start.

**This requires inputs to be sorted after parsing!**

### Input Format
Each input entry consists of:

| Field       | Type      | Description |
|------------|----------|-------------|
| Packed | `varint`  | Input frame delta and button press bitmask packed in a single value |
| Extension Size (Optional) | `varint` | Size of custom extension data |
| Extension Data (Optional) | `bytes`  | Custom input extension data |

### Packed Encoding
The packed field is a `varint` composed of the bitmask and frame delta packed together.

The bitmask encodes the input state, and the remaining bits are used to encode the frame delta. 

#### Non-platformer input
| Bit Position | Meaning |
|-------------|--------- |
| 0           | `varint` bit indicating if the value continues to the next byte |
| 1           | Button press state (1 = down, 0 = up) |
| 2-7         | Frame delta |
#### Platformer input
| Bit Position | Meaning |
|-------------|---------|
| 0           | `varint` bit indicating if the value continues to the next byte |
| 1           | Button press state (1 = down, 0 = up) |
| 2-3         | Button ID (00 = None, 01 = Jump, 10 = Left, 11 = Right) |
| 4-7         | Frame delta

Then the value continues like a normal `varint`

Thanks to this, the input delta can go as high as 63 frames in non platformer replays, or 15 in platformer replays, while maintaining the input in a single byte.
