#pragma once

// Manager.hpp structure by acaruso
// reused with explicit permission and strong encouragement

class Manager {

protected:
	static Manager* instance;
public:

	bool isDualsTime = false;
	bool isLevelComplete = false;

	int installedMods = 0;
	int loadedMods = 0;
	int disabledMods = 0;
	int problems = 0;
	std::string geodeVersion = "";
	std::string gameVersion = "";
	std::string platform = "";
	std::string forwardCompat = "";

	std::string lastPlayedSong = "N/A";
	std::string lastPlayedEffect = "N/A";

	std::string lastKeyName = "N/A";
	
	bool isMinecraftify = false;

	std::array<std::string, 13> months = {
		"Unknown",
		"January", "February", "March", "April",
		"May", "June", "July", "August",
		"September", "October", "November", "December"
	};

	std::array<std::string, 7> daysOfWeek = {
		"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
	};

	std::array<std::string, 4> qualities = {
		"Unknown",
		"Low",
		"Medium (HD)",
		"High (UHD)"
	};

	std::time_t originalTimestamp;

	bool hasCalledAlready = false;
	
	// GLenum glBlendFuncs[] = {GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_SRC_ALPHA_SATURATE };

	static Manager* getSharedInstance() {
		if (!instance) {
			instance = new Manager();
		}
		return instance;
	}

};