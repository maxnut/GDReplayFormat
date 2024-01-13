#include "Replay.h"

#include <filesystem>
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

std::optional<Replay> Replay::load(std::string path)
{
    Replay replay;

	json replayJson;

	std::ifstream replayFile;

	std::filesystem::path p = path;
	if (p.extension() == ".gdr")
	{
		replayFile.open(path, std::ios::binary);

		if (!replayFile.is_open())
			return std::nullopt;

		replayFile.seekg(0, std::ios::end);
		size_t fileSize = replayFile.tellg();
		replayFile.seekg(0, std::ios::beg);

		std::vector<std::uint8_t> replayData(fileSize);

		replayFile.read(reinterpret_cast<char*>(replayData.data()), fileSize);
		replayJson = json::from_msgpack(replayData);

		replayFile.close();
	}
	else
	{
		replayFile.open(path);

		if (!replayFile.is_open())
			return std::nullopt;

		replayFile >> replayJson;
		replayFile.close();
	}

	replay.gameVersion = replayJson["gameVersion"];
	replay.description = replayJson["description"];
	replay.version = replayJson["version"];
	replay.duration = replayJson["duration"];
	replay.botInfo.name = replayJson["bot"]["name"];
	replay.botInfo.version = replayJson["bot"]["version"];
	replay.levelInfo.id = replayJson["level"]["id"];
	replay.levelInfo.name = replayJson["level"]["name"];
	replay.author = replayJson["author"];
	replay.seed = replayJson["seed"];
	replay.coins = replayJson["coins"];
	replay.ldm = replayJson["ldm"];

	for (const json& inputJson : replayJson["inputs"])
	{
		Input input;
		input.frame = inputJson["frame"];
		input.button = inputJson["btn"];
		input.player2 = inputJson["2p"];
		input.down = inputJson["down"];
		replay.inputs.push_back(input);
	}

	return std::optional<Replay>(replay);
}

void Replay::save(std::string path, ExportMode exportMode)
{
	json replayJson = json::object();
	replayJson["gameVersion"] = gameVersion;
	replayJson["description"] = description;
	replayJson["version"] = version;
	replayJson["duration"] = inputs[inputs.size() - 1].frame;
	replayJson["bot"]["name"] = botInfo.name;
	replayJson["bot"]["version"] = botInfo.version;
	replayJson["level"]["id"] = levelInfo.id;
	replayJson["level"]["name"] = levelInfo.name;
	replayJson["author"] = author;
	replayJson["seed"] = seed;
	replayJson["coins"] = coins;
	replayJson["ldm"] = ldm;
	replayJson["inputs"] = json::array();

	for (Input& input : inputs)
	{
		json inputJson = json::object();
		inputJson["frame"] = input.frame;
		inputJson["btn"] = input.button;
		inputJson["2p"] = input.player2;
		inputJson["down"] = input.down;
		replayJson["inputs"].push_back(inputJson);
	}

	std::ofstream replayFile;

	if (exportMode == MSGPACK)
	{
		replayFile.open(path + ".gdr", std::ios::binary);
		std::vector<std::uint8_t> replayData = json::to_msgpack(replayJson);
		replayFile.write(reinterpret_cast<const char*>(replayData.data()), replayData.size());
	}
	else
	{
		replayFile.open(path + ".gdr.json");
		replayFile << replayJson.dump();
	}

	replayFile.close();
}