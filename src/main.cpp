#include "Replay.h"

int main()
{
	Replay r;

	r.author = "maxnu";
	r.description = "test";

	r.inputs.push_back({0.5f, 1, false, false});

	r.inputs.push_back({1.f, 1, false, true});

	r.save("Replay", MSGPACK);

	r = Replay::load("Replay.gdr");

	std::cout << r.author << "\n";
	std::cout << r.description << "\n";
	std::cout << r.inputs.size() << "\n";
}