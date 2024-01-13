#include <gdr/gdr.hpp>

using namespace gdr;

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

int main() {
	MyReplay r;

	r.author = "camila314";
	r.description = "we testing up in here";
	r.attempts = 50;

	r.inputs.push_back(MyInput(100, 1, false, true, 30.23));
	r.inputs.push_back(MyInput(130, 1, false, false, 100.35));

	std::vector<uint8_t> output = r.exportData();

	r = MyReplay::importData(output);

	assert(r.author == "camila314");
	assert(r.description == "we testing up in here");
	assert(r.attempts == 50);

	assert(r.inputs[0].frame == 100);
	assert(abs(r.inputs[1].xpos - 100.35) < 0.0001);

	std::cout << "Success!!" << std::endl;
	return 0;
}
