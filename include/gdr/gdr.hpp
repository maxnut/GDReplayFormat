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

template <typename OkType = void, typename ErrType = std::string>
class Result {
    using OkContainer = OkContainer<OkType>;
    using ErrContainer = ErrContainer<ErrType>;

public:
    constexpr Result(OkContainer&& value) : value(std::move(value)) {}
    constexpr Result(const OkContainer& value) : value(value) {}
    constexpr Result(ErrContainer&& value) : value(std::move(value)) {}
    constexpr Result(const ErrContainer& value) : value(value) {}

    constexpr OkType& unwrap() && { return std::get<OkContainer>(value).value; }
    constexpr const OkType& unwrap() const & { return std::get<OkContainer>(value).value; }
    constexpr ErrType& unwrapErr() && { return std::get<ErrContainer>(value).value; }
    constexpr const ErrType& unwrapErr() const & { return std::get<ErrContainer>(value).value; }

    constexpr bool isOk() const { return std::holds_alternative<OkContainer>(value); }
    constexpr bool isErr() const { return std::holds_alternative<ErrContainer>(value); }

    constexpr const OkType& unwrapOr(const OkType& def) const { return isOk() ? unwrap() : def; }
    constexpr const ErrType& unwrapErrOr(const ErrType& def) const { return isErr() ? unwrapErr() : def; }

private:
    std::variant<OkContainer, ErrContainer> value;
};

template <typename O = OkTag, typename E = std::string>
constexpr auto Ok(O&& value) { return Result<O, E>(OkContainer<O>(std::forward<O>(value))); }

template <typename O = OkTag, typename E = std::string>
constexpr auto Ok(const O& value) { return Ok<O, E>(O(value)); }

template <typename E = std::string, size_t S>
constexpr auto Ok(const char (&value)[S]) { return Ok<std::string, E>(std::string(value)); }

template <typename O = OkTag, typename E = std::string>
constexpr auto Err(E&& value) { return Result<O, E>(ErrContainer<E>(std::forward<E>(value))); }

template <typename O = OkTag, typename E = std::string>
constexpr auto Err(const E& value) { return Err<O, std::string>(value); }

template <typename O = OkTag, size_t S>
constexpr auto Err(const char (&value)[S]) { return Err<O, std::string>(std::string(value)); }

struct Bot {
    std::string name;
    int version = 1;

    Bot() = default;

    Bot(std::string name, int version)
        : name(std::move(name)), version(version) {}
};

struct Level {
    uint32_t id{};
    std::string name;

    Level() = default;

    explicit Level(std::string name, uint32_t id = 0)
        : id(id), name(std::move(name)) {}
};

struct Input {
    Input() = default;

    Input(uint64_t frame, uint8_t button, bool player2, bool down)
        : frame(frame), button(button), player2(player2), down(down) {}

    virtual ~Input() = default;
    virtual void parseExtension(binary_reader& reader) {}
    virtual void saveExtension(binary_writer& writer) const {}

    uint64_t frame{};
    uint8_t button{};
    bool player2{};
    bool down{};
};

template <typename S = void, typename T = Input>
class Replay {
    template <typename R, typename C, typename... Args>
    struct func_trait {
        using class_type = C;
        constexpr func_trait(R (C::*)(Args...)) noexcept {}
    };

public:
    using InputType = T;
    using Self = std::conditional_t<std::is_same_v<S, void>, Replay, S>;
    static constexpr auto input_has_extension = !std::is_same_v<Input, typename decltype(func_trait(&T::parseExtension))::class_type>;

    Replay() = default;

    Replay(std::string const& botName, int botVersion)
        : botInfo(botName, botVersion) {}

    virtual ~Replay() = default;
    virtual void parseExtension(binary_reader& reader) {}
    virtual void saveExtension(binary_writer& writer) const {}
    virtual bool shouldParseExtension() const { return false; }

    int getVersion() const { return version; }

    [[nodiscard]] Result<std::vector<uint8_t>> exportData() const {
        binary_writer stream;

        stream << "GDR" << version << input_has_extension
                << author << description
                << duration << gameVersion
                << framerate << seed << coins << ldm
                << botInfo.name << botInfo.version
                << levelInfo.id << levelInfo.name;

        binary_writer extensionStream;
        saveExtension(extensionStream);
        stream << extensionStream.size();
        stream.write(extensionStream.data().data(), extensionStream.size());

        stream << inputs.size();

        uint64_t p = 0;
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

        return Ok(std::move(stream.data()));
    }

    static Result<Self> importData(std::span<uint8_t> data) {
        binary_reader stream(data);
        Self r;

        std::array<char, 3> magic;
        stream.read(magic);
        if (magic[0] != 'G' || magic[1] != 'D' || magic[2] != 'R') {
            return Err<Self>("Invalid magic: " + std::string(magic.data(), magic.size()));
        }

        bool hasInputExt;
        stream >> r.version >> hasInputExt
                >> r.author >> r.description
                >> r.duration >> r.gameVersion
                >> r.framerate >> r.seed >> r.coins >> r.ldm
                >> r.botInfo.name >> r.botInfo.version
                >> r.levelInfo.id >> r.levelInfo.name;

        size_t extensionSize;
        stream >> extensionSize;
        if (extensionSize > stream.size()) {
            return Err<Self>("Invalid extension size");
        }

        std::span<uint8_t> extensionData = stream.peek(extensionSize);
        stream.skip(extensionSize);
        binary_reader extensionStream(extensionData);
        r.parseExtension(extensionStream);

        size_t inputSize;
        stream >> inputSize;
        r.inputs.reserve(inputSize);

        uint64_t p = 0;
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
                binary_reader inputExtensionStream(inputExtensionData);
                input.parseExtension(inputExtensionStream);
            }

            r.inputs.push_back(std::move(input));
            p = input.frame;
        }

        return Ok(std::move(r));
    }

private:
    int version = 2;

public:
    std::string author;
    std::string description;

    float duration{};
    int gameVersion{};

    double framerate = 240.0;

    int seed = 0;
    int coins = 0;

    bool ldm = false;

    Bot botInfo{};
    Level levelInfo{};
    std::vector<InputType> inputs;
};
}