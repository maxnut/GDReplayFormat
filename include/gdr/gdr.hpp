#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <variant>

#include "binarystream.hpp"

namespace gdr {
struct OkTag {};

template <typename T>
struct OkContainer {
    T value;
    constexpr OkContainer(T&& value) : value(std::move(value)) {}
    constexpr OkContainer(const T& value) : value(value) {}
};

template <typename T>
struct ErrContainer {
    T value;
    constexpr ErrContainer(T&& value) : value(std::move(value)) {}
    constexpr ErrContainer(const T& value) : value(value) {}
};

template <typename OkType = OkTag, typename ErrType = std::string>
class [[nodiscard]] Result {
    using OkContainer = OkContainer<OkType>;
    using ErrContainer = ErrContainer<ErrType>;

public:
    constexpr Result(OkContainer&& value) : value(std::move(value)) {}
    constexpr Result(const OkContainer& value) : value(value) {}
    constexpr Result(ErrContainer&& value) : value(std::move(value)) {}
    constexpr Result(const ErrContainer& value) : value(value) {}

    /// @brief Unwraps the value as rvalue if it is Ok, otherwise throws bad_variant_access exception.
    constexpr OkType& unwrap() && { return std::get<OkContainer>(value).value; }
    /// @brief Unwraps the value if it is Ok, otherwise throws bad_variant_access exception.
    constexpr const OkType& unwrap() const & { return std::get<OkContainer>(value).value; }
    /// @brief Unwraps the error as rvalue if it is Err, otherwise throws bad_variant_access exception.
    constexpr ErrType& unwrapErr() && { return std::get<ErrContainer>(value).value; }
    /// @brief Unwraps the error if it is Err, otherwise throws bad_variant_access exception.
    constexpr const ErrType& unwrapErr() const & { return std::get<ErrContainer>(value).value; }

    /// @brief Returns true if the result is Ok.
    constexpr bool isOk() const { return std::holds_alternative<OkContainer>(value); }
    /// @brief Returns true if the result is Err.
    constexpr bool isErr() const { return std::holds_alternative<ErrContainer>(value); }

    /// @brief Unwraps the value if it is Ok, otherwise returns the default value.
    constexpr const OkType& unwrapOr(const OkType& def) const { return isOk() ? unwrap() : def; }
    /// @brief Unwraps the error if it is Err, otherwise returns the default value.
    constexpr const ErrType& unwrapErrOr(const ErrType& def) const { return isErr() ? unwrapErr() : def; }

private:
    std::variant<OkContainer, ErrContainer> value;
};

template <typename O = OkTag, typename E = std::string>
constexpr Result<O, E> Ok(O&& value) { return Result<O, E>(OkContainer<O>(std::forward<O>(value))); }

template <typename O = OkTag, typename E = std::string>
constexpr Result<O, E> Ok(const O& value) { return Ok<O, E>(O(value)); }

template <typename E = std::string, size_t S>
constexpr Result<std::string, E> Ok(const char (&value)[S]) { return Ok<std::string, E>(std::string(value)); }

template <typename O = OkTag, typename E = std::string>
constexpr Result<O, E> Ok() { return Ok<O, E>(O()); }

template <typename O = OkTag, typename E = std::string>
constexpr Result<O, E> Err(E&& value) { return Result<O, E>(ErrContainer<E>(std::forward<E>(value))); }

template <typename O = OkTag, typename E = std::string>
constexpr Result<O, E> Err(const E& value) { return Err<O, E>(value); }

template <typename O = OkTag, size_t S>
constexpr Result<O, std::string> Err(const char (&value)[S]) { return Err<O, std::string>(std::string(value)); }

template <size_t S>
struct StringLiteral {
    char value[S];
    constexpr StringLiteral(const char (&value)[S]) {
        std::copy_n(value, S, this->value);
    }
};

/// @brief Structure that holds information about the bot that recorded the replay.
struct Bot {
    std::string name; /* Name of the bot that recorded the replay. */
    int version = 1;  /* Version of the bot that recorded the replay. */

    Bot() = default;
    Bot(std::string name, int version)
        : name(std::move(name)), version(version) {}
};

/// @brief Structure that holds information about the level that the replay was recorded on.
struct Level {
    uint32_t id{};    /* ID of the level the replay was recorded on. */
    std::string name; /* Name of the level the replay was recorded on. */

    Level() = default;
    explicit Level(std::string name, uint32_t id = 0)
        : id(id), name(std::move(name)) {}
};

/// @brief Information about a single input in a replay.
/// @note You can extend this structure by inheriting from it and overriding the parseExtension and saveExtension functions.
/// Make sure to also set a unique tag for the structure if you add custom data.
template <StringLiteral Tag = "">
struct Input {
    /// @brief Unique tag for the input structure. Used for serialization/deserialization.
    static constexpr auto tag = Tag.value;

    Input() = default;
    Input(uint64_t frame, uint8_t button, bool player2, bool down)
        : frame(frame), button(button), player2(player2), down(down) {}

    virtual ~Input() = default;
    /// @brief Override this function to access the extension data.
    virtual void parseExtension(binary_reader& reader) {}
    /// @brief Override this function to append custom data to the replay.
    virtual void saveExtension(binary_writer& writer) const {}

    uint64_t frame{}; /* Frame that the input was recorded on. */
    uint8_t button{}; /* Pressed button. 1 = Jump, 2 = Left, 3 = Right. Can be converted directly to PlayerButton enum. */
    bool player2{};   /* Whether the input was for player 2. */
    bool down{};      /* Whether the button was pressed or released. */
};

/// @brief Contains information about the replay and the inputs.
/// @tparam S The type of the derived class. Used for static polymorphism.
/// @tparam T The type of the input structure. Change this if you want to use custom input structures with extensions.
template <typename S = void, typename T = Input<>>
class Replay {
public:
    using InputType = T;
    using Self = std::conditional_t<std::is_same_v<S, void>, Replay, S>;
    static constexpr bool input_has_extension = std::string_view(InputType::tag) != "";

    Replay() = default;

    Replay(std::string const& botName, int botVersion)
        : botInfo(botName, botVersion) {}

    virtual ~Replay() = default;
    /// @brief Override this function to access the extension data.
    virtual void parseExtension(binary_reader& reader) {}
    /// @brief Override this function to append custom data to the replay.
    virtual void saveExtension(binary_writer& writer) const {}
    /// @brief Override this function to determine if the extension should be parsed.
    /// Make sure to check the bot name and version here.
    virtual bool shouldParseExtension() const { return false; }

    /// @brief Get current version of the replay format.
    [[nodiscard]] int getVersion() const { return version; }

    /// @brief Export the replay to a byte array. Returns an error if the data is invalid.
    [[nodiscard]] Result<std::vector<uint8_t>> exportData() const {
        binary_writer stream;

        stream << "GDR" << version << std::string(InputType::tag)
                << author << description
                << duration << gameVersion
                << framerate << seed << coins << ldm
                << botInfo.name << botInfo.version
                << levelInfo.id << levelInfo.name;

        binary_writer extensionStream;
        saveExtension(extensionStream);
        stream << extensionStream.size();
        stream.write(extensionStream.data().data(), extensionStream.size());

        stream << deaths.size();

        uint64_t p = 0;
        for(uint64_t death : deaths) {
            stream << death - p;
            p = death;
        }

        stream << inputs.size();

        p = 0;
        for (const InputType& input : inputs) {
            uint64_t delta = input.frame - p;
            uint8_t bitmask = ((input.button & 0b11) << 2) | (input.player2 << 1) | input.down;
            stream << delta << bitmask;

            if constexpr (input_has_extension) {
                binary_writer inputExtensionStream;
                input.saveExtension(inputExtensionStream);
                stream << inputExtensionStream.size();
                stream.write(inputExtensionStream.data().data(), inputExtensionStream.size());
            }

            p = input.frame;
        }

        return Ok(std::move(stream.release()));
    }

    /// @brief Export the replay to a file. Returns an error if the file cannot be opened or written to.
    [[nodiscard]] Result<> exportData(const std::filesystem::path& path) const {
        auto res = exportData();
        if (res.isErr()) return Err<>(res.unwrapErr());

        std::ofstream file(path, std::ios::binary);
        if (!file) return Err("Failed to open file for writing");

        file.write(reinterpret_cast<const char*>(res.unwrap().data()), res.unwrap().size());
        if (!file) return Err("Failed to write data to file");

        return Ok();
    }

    /// @brief Import a replay from a byte array. Returns an error if the data is invalid.
    [[nodiscard]] static Result<Self> importData(std::span<uint8_t> data) {
        binary_reader stream(data);
        Self r;

        std::array<char, 3> magic;
        stream.read(magic);
        if (std::string_view(magic.data(), magic.size()) != "GDR") {
            return Err<Self>("Invalid magic: " + std::string(magic.data(), magic.size()));
        }

        std::string inputTag;
        stream >> r.version >> inputTag
                >> r.author >> r.description
                >> r.duration >> r.gameVersion
                >> r.framerate >> r.seed >> r.coins >> r.ldm
                >> r.botInfo.name >> r.botInfo.version
                >> r.levelInfo.id >> r.levelInfo.name;

        bool parseExt = r.shouldParseExtension();              // if replay extension should be parsed
        bool hasInputExt = !inputTag.empty();                  // if input extension is present in the file
        bool shouldParseInputExt = inputTag == InputType::tag; // if input extension should be parsed (matches the tag)

        size_t extensionSize;
        stream >> extensionSize;
        if (extensionSize > stream.size()) {
            return Err<Self>("Invalid extension size");
        }

        std::span<uint8_t> extensionData = stream.peek(extensionSize);
        stream.skip(extensionSize);
        if (parseExt) {
            binary_reader extensionStream(extensionData);
            r.parseExtension(extensionStream);
        }

        size_t sizes;
        stream >> sizes;
        r.deaths.reserve(sizes);

        uint64_t p = 0;
        for(size_t i = 0; i < sizes; i++) {
            uint64_t delta;
            stream >> delta;
            r.deaths.push_back(delta + p);
            p += delta;
        }

        stream >> sizes;
        r.inputs.reserve(sizes);

        p = 0;
        while (!stream.empty()) {
            InputType input;
            uint64_t delta;
            uint8_t bitmask;
            stream >> delta >> bitmask;
            input.frame = delta + p;
            input.button = (bitmask >> 2) & 0b11;
            input.player2 = (bitmask >> 1) & 1;
            input.down = bitmask & 1;

            if (hasInputExt) {
                size_t inputExtensionSize;
                stream >> inputExtensionSize;
                if (inputExtensionSize > stream.size()) {
                    return Err<Self>("Invalid input extension size");
                }

                std::span<uint8_t> inputExtensionData = stream.peek(inputExtensionSize);
                stream.skip(inputExtensionSize);
                if (shouldParseInputExt) {
                    binary_reader inputExtensionStream(inputExtensionData);
                    input.parseExtension(inputExtensionStream);
                }
            }

            r.inputs.push_back(std::move(input));
            p = input.frame;
        }

        return Ok(std::move(r));
    }

    /// @brief Import a replay from a file. Returns an error if the file cannot be opened or read from.
    [[nodiscard]] static Result<Self> importData(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return Err<Self>("Failed to open file for reading");

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        if (!file) return Err<Self>("Failed to read data from file");

        return importData(data);
    }

private:
    int version = 2;          /* Version of the replay format. */

public:
    std::string author;       /* Replay author. Usually the nickname of the person who recorded it. */
    std::string description;  /* Replay description. Not required. */

    float duration{};         /* Duration of the replay in seconds. */
    int gameVersion{};        /* Game version the replay was recorded on. (Example: 22074 for 2.2074, refer to GEODE_COMP_GD_VERSION) */

    double framerate = 240.0; /* Framerate (ticks per second) of the replay. 240 by default, change if macro is recorded using physics bypass. */

    int seed = 0;             /* Random seed set when at the start of the attempt. */
    int coins = 0;            /* Number of coins collected in the level. */

    bool ldm = false;         /* Whether the replay was recorded in low detail mode. */

    Bot botInfo{};            /* Information about the bot that recorded the replay. */
    Level levelInfo{};        /* Information about the level the replay was recorded on. */
    std::vector<InputType> inputs;
    std::vector<uint64_t> deaths;
};
}