#pragma once

#include "binarystream.hpp"
#include <cstdint>

namespace gdr {

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
public:
	using InputType = T;
    using Self = std::conditional_t<std::is_same_v<S, void>, Replay, S>;

    Replay() = default;
    Replay(std::string const& botName, int botVersion)
	    : botInfo(botName, botVersion) {}

	virtual ~Replay() = default;
    virtual void parseExtension(binary_reader& reader) {}
    virtual void saveExtension(binary_writer& writer) const {}
    virtual bool shouldParseExtension() const {return false;}

    [[nodiscard]] std::vector<uint8_t> exportData() const {
        binary_writer stream;

        stream << author << description << duration << gameVersion << framerate << seed << coins
            << ldm << botInfo.name << botInfo.version << levelInfo.id << levelInfo.name;

        saveExtension(stream);

        uint64_t p = 0;
        for(const InputType& input : inputs) {
            uint64_t delta = input.frame - p;
            uint8_t bitmask = ((input.button & 0b11) << 2) | (input.player2 << 1) | input.down;
            stream << delta << bitmask;
            input.saveExtension(stream);
            p = input.frame;
        }

        return stream.data();
    }

    static Self importData(const std::span<uint8_t>& data) {
        binary_reader stream(data);
        Self r;

        stream >> r.author >> r.description >> r.duration >> r.gameVersion >> r.framerate >> r.seed >> r.coins
            >> r.ldm >> r.botInfo.name >> r.botInfo.version >> r.levelInfo.id >> r.levelInfo.name;

        r.parseExtension(stream);

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
            input.parseExtension(stream);
            r.inputs.push_back(input);
            p = input.frame;
        }

        return r;
    }

public:
    std::string author;
	std::string description;

	float duration{};
	int gameVersion{};
	int version = 2;

	double framerate = 240.0;

	int seed = 0;
    int coins = 0;

	bool ldm = false;

	Bot botInfo{};
	Level levelInfo{};
    std::vector<InputType> inputs;
};

}