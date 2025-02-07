#include "gdr/gdr.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

using namespace gdr;

struct MyInput : Input {
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

int main() {
    MyReplay replay;

    replay.levelInfo.id = 5;
    replay.coins = 2;
    replay.framerate = 60.0;
    replay.levelInfo.name = "ste";
    replay.author = "pobert";
    replay.description = "we testing up in here";
    replay.attempts = 50;

    replay.deaths.push_back(5);
    replay.deaths.push_back(1002);

    replay.inputs.push_back(MyInput(20, 1, false, true, 5.f));
    replay.inputs.push_back(MyInput(543, 1, false, false, 6.f));
    replay.inputs.push_back(MyInput(1002, 1, false, true, 7.f));

    auto exportRes = replay.exportData();
    if (exportRes.isErr()) {
        std::cerr << exportRes.unwrapErr() << std::endl;
        return 1;
    }
    std::vector<uint8_t> output = std::move(exportRes.unwrap());

    auto importRes = MyReplay::importData(output);
    if (importRes.isErr()) {
        std::cerr << importRes.unwrapErr() << std::endl;
        return 1;
    }
    replay = std::move(importRes.unwrap());

    assert(replay.inputs.size() == 3);
    assert(replay.deaths.size() == 2);
    assert(replay.deaths[0] == 5);
    assert(replay.deaths[0] == 1002);
    assert(replay.inputs[0].frame == 20); assert(replay.inputs[0].button == 1); assert(replay.inputs[0].player2 == false); assert(replay.inputs[0].down == true); assert(replay.inputs[0].xpos == 5.f);
    assert(replay.inputs[1].frame == 543); assert(replay.inputs[1].button == 1); assert(replay.inputs[1].player2 == false); assert(replay.inputs[1].down == false); assert(replay.inputs[1].xpos == 6.f);
    assert(replay.inputs[2].frame == 1002); assert(replay.inputs[2].button == 1); assert(replay.inputs[2].player2 == false); assert(replay.inputs[2].down == true); assert(replay.inputs[2].xpos == 7.f);
    assert(replay.attempts == 50);

    return 0;
}