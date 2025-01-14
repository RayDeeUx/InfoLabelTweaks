#include <Geode/modify/FMODAudioEngine.hpp>
#include "Manager.hpp"
#include "Utils.hpp"
#include <regex>

using namespace geode::prelude;

static std::regex songEffectRegex(R"(.*(?:\\|\/)(\S+)\.(mp3|ogg|wav|flac))", std::regex::optimize | std::regex::icase); // see https://regex101.com/r/CqvIvI/1.
static std::regex geodeAudioRegex(R"(((?!\S+geode)(?:\\|\/)(?:([a-z0-9\-_]+\.[a-z0-9\-_]+)(?:\\|\/))([\S ]+)\.(mp3|ogg|wav|flac))$)", std::regex::optimize | std::regex::icase); // see https://regex101.com/r/0b9rY1/1.

class $modify(MyFMODAudioEngine, FMODAudioEngine) {
	struct Fields {
		Manager* manager = Manager::getSharedInstance();
		int desiredIndexForModID = 2; // easier place to change index
		int desiredIndexForFileName = 1; // easier place to change index
		const std::list<std::string> vanillaSFX = { "achievement_01.ogg", "buyItem01.ogg", "buyItem03.ogg", "chest07.ogg", "chest08.ogg", "chestClick.ogg", "chestLand.ogg", "chestOpen01.ogg", "counter003.ogg", "crystal01.ogg", "door001.ogg", "door01.ogg", "door02.ogg", "endStart_02.ogg", "explode_11.ogg", "gold01.ogg", "gold02.ogg", "grunt01.ogg", "grunt02.ogg", "grunt03.ogg", "highscoreGet02.ogg", "magicExplosion.ogg", "playSound_01.ogg", "quitSound_01.ogg", "reward01.ogg", "secretKey.ogg", "unlockPath.ogg" };
		const std::list<std::string> badSFX = { "achievement_01.ogg", "magicExplosion.ogg", "gold02.ogg", "secretKey.ogg" };
	};
	std::string extractModID(const std::smatch& theMatch) {
		const auto fields = m_fields.self();
		/*
		log::info("{}", theMatch.size());
		for (auto matchString : theMatch) {
			log::info("matchString: {}", matchString.str());
		}
		log::info("theMatch[fields->desiredIndexForModID].str(): {}", theMatch[fields->desiredIndexForModID].str());
		*/
		if (const Mod* mod = Utils::getMod(theMatch[fields->desiredIndexForModID].str()))
			return fmt::format("[From {}]", mod->getName());
		return "[From another Geode mod]";
	}
	std::string parsePath(std::string path) {
		const auto fields = m_fields.self();
		// log::info("path before: {}", path);
		std::smatch match;
		std::smatch geodeMatch;
		std::string result = "";
		path = std::regex_replace(path, std::regex(R"(com\.geode\.launcher\/)"), ""); // android is cring, original is [ "com\.geode\.launcher\/" ]
		path = fmt::format("/{}", path); // adding an extra slash to get it working on all possible paths. this is because combo burst does some stuff under the hood i am too scared to look at and i don't want to define more regex than necessary.
		// log::info("path after: {}", path);
		if (path.find("geode") != std::string::npos && (path.find("mods") != std::string::npos || path.find("config") != std::string::npos)) {
			if (std::regex_search(path, geodeMatch, geodeAudioRegex)) {
				if (Utils::getBool("audioFromMods")){
					result = extractModID(geodeMatch);
				}
			} else {
				result = "[Something went wrong...]";
			}
		} else if (std::regex_match(path, match, songEffectRegex)) {
			if (std::regex_search(path, geodeMatch, geodeAudioRegex)) {
				if (Utils::getBool("audioFromMods")) result = extractModID(geodeMatch);
				else result = "[Geode mod]";
			} else
				result = fmt::format("{}.{}", match[fields->desiredIndexForFileName].str(), match[fields->desiredIndexForFileName + 1].str());
		} else {
			result = fmt::format("{}", path);
		}
		return result;
	}
	FMODSound& preloadEffect(gd::string path) {
		const auto fields = m_fields.self();
		FMODSound& result = FMODAudioEngine::sharedEngine()->preloadEffect(path);
		if (!Utils::modEnabled() || !PlayLayer::get()) return result; // ignore if mod disabled, and dont record files outside of playlayer. should've done this sooner
		if (std::find(fields->vanillaSFX.begin(), fields->vanillaSFX.end(), path) != fields->vanillaSFX.end()) { return result; } // ignore vanilla sfx, the debug menu should only record sfx from the level itself
		fields->manager->lastPlayedEffect = parsePath(path);
		return result;
	}
	FMOD::Sound* preloadMusic(gd::string path, bool p1, int p2) {
		const auto fields = m_fields.self();
		FMOD::Sound* result = FMODAudioEngine::sharedEngine()->preloadMusic(path, p1, p2);
		// FMODAudioEngine::sharedEngine()->loadMusic(path, speed, p2, volume, shouldLoop, p5, p6);
		if (!Utils::modEnabled() || !PlayLayer::get()) return result; // ignore if mod disabled, and dont record files outside of playlayer. should've done this sooner
		fields->manager->lastPlayedSong = parsePath(path);
		return result;
	}
};