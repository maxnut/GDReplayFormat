#pragma once

#include <iostream>
#include <vector>

struct Bot
{
	std::string name;
	float version;
};

struct Level
{
	uint32_t id;
	std::string name;
};

struct Input
{
	float frame;
	int button;
	bool player2;
	bool down;
};

enum ExportMode
{
    MSGPACK,
    JSON
};

struct Replay
{
	std::string author;
	std::string description;

	float duration;
	float gameVersion;
	float version;

	int seed;
    int coins;

	bool ldm;

	Bot botInfo;
	Level levelInfo;

	std::vector<Input> inputs;

	static Replay load(std::string path);
	void save(std::string path, ExportMode exportMode = MSGPACK);
};