#include <Geode/modify/PlayLayer.hpp>
#include "Utils.hpp"
#include "Manager.hpp"
#include <regex>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

using namespace geode::prelude;

#define SECONDS_PER_DAY 86400
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_MINUTE 60

/*

sample debug text contents:

LevelID: 0
Time: 0
Attempt: 1
Taps: 0
TimeWarp: 1
Gravity: 1
X: 0
Y: 105
Active: 14
Gradients: 0
Particles: 120
-- Audio --
Songs: 1
SFX: 1
-- Perf --
Move: 0
Rotate: 0
Scale: 0
Follow: 0
-- Area --
Move: 0 / 0
Rotate: 0 / 0
Scale: 0 / 0
ColOp: 0 / 0

*/

static std::regex tapsCountRegex(R"(Taps: (\d+)\n)", std::regex::optimize | std::regex::icase);
static std::regex timeLabelRegex(R"(Time: (\d+(\.\d+)?))", std::regex::optimize | std::regex::icase);
static std::regex gravityModRegex(R"(Gravity: (\d+(\.\d+)?))", std::regex::optimize | std::regex::icase);
static std::regex levelIDRegex(R"(Level ?ID: (\d+))", std::regex::optimize | std::regex::icase);
static std::regex attemptCountRegex(R"(Attempts?: (\d+))", std::regex::optimize | std::regex::icase);
static std::regex asciiOnlyMaxTwentyRegex(R"([\x00-\x7F]{0,20})", std::regex::optimize | std::regex::icase);

class $modify(MyPlayLayer, PlayLayer) {
	struct Fields {
		Manager* manager = Manager::getSharedInstance();
		bool appliedBlending = false;
		bool appliedChromaOrDefaultColor = false;
		bool isLevelComplete = false;
		bool textAlignmentSet = false;
		bool scaleSet = false;
		bool opacitySet = false;
		bool fontSet = false;
		bool isDual = false;
		std::string hIDeSet = "";
		CCNode* debugText = nullptr;

		std::string endsWithArea = "\n-- Area --\n";
		std::vector<std::string> hIDeVector = {
			"It's locked!", "Shh...",
			"Nonzero", "Streisand",
			"N/A", "Expunged",
			"Locked", "hIDden"
		};
		std::string textureQualityString = "Unknown";
		float winHeightPixels = CCDirector::get()->getWinSizeInPixels().height;
		float winWidthPixels = CCDirector::get()->getWinSizeInPixels().width;
		bool isFullscreen = GameManager::get()->getGameVariable("0025");

		bool isEnabled;

		bool twelveHour;
		bool shortMonth;
		bool dayOfWeek;
		bool shortDayOfWeek;
		bool dayFirst;
		bool includeSeconds;
		bool splitDateAndTime;
		bool useUTC;
		bool uptime;

		bool miscInfo;
		bool textureQuality;

		bool playerStatus;
		bool compactDirections;
		bool accuratePosition;
		bool velocityPlayer;
		bool accuratePlayer;
		bool rotationPlayer;

		bool levelTraits;

		bool cameraProperties;
		bool compactCamera;
		
		bool conditionalValues;

		bool geodeInfo;
		bool compactGeode;

		bool blendingDebugText;
		bool chromaDebugText;
		bool maxAlphaDebugText;
		bool textAlignRight;
		bool spaceUKScale;
		bool positionAlignRight;
		bool positionAlignBottom;

		bool logDebugText;

		bool currentChannel;
		bool lastPlayedAudio;
		bool hIDe;

		bool jumps;
		bool totalTime;
		bool totalAttempts;
		bool affectedByForces;
		bool compactGameplay;
		bool compactAudio;
		bool expandPerformance;
		bool tapsToClicks;
		bool fixLevelIDLabel;
		bool pluralAttempts;
		
		bool fps;
		bool lastKey;

		bool gameplayHeader;

		float chromaDebugTextSpeed;

		int64_t customFont;

		std::string customFooter;
	};
	void setupSettings() {
		const auto fields = m_fields.self();
		fields->isEnabled = Utils::modEnabled();
		fields->twelveHour = Utils::getBool("twelveHour");
		fields->shortMonth = Utils::getBool("shortMonth");
		fields->dayOfWeek = Utils::getBool("dayOfWeek");
		fields->shortDayOfWeek = Utils::getBool("shortDayOfWeek");
		fields->dayFirst = Utils::getBool("dayFirst");
		fields->includeSeconds = Utils::getBool("includeSeconds");
		fields->splitDateAndTime = Utils::getBool("splitDateAndTime");
		fields->useUTC = Utils::getBool("useUTC");
		fields->uptime = Utils::getBool("uptime");

		fields->miscInfo = Utils::getBool("miscInfo");
		fields->textureQuality = Utils::getBool("textureQuality");

		fields->playerStatus = Utils::getBool("playerStatus");
		fields->compactDirections = Utils::getBool("compactDirections");
		fields->accuratePosition = Utils::getBool("accuratePosition");
		fields->velocityPlayer = Utils::getBool("velocityPlayer");
		fields->accuratePlayer = Utils::getBool("accuratePlayer");
		fields->rotationPlayer = Utils::getBool("rotationPlayer");

		fields->levelTraits = Utils::getBool("levelTraits");

		fields->cameraProperties = Utils::getBool("cameraProperties");
		fields->compactCamera = Utils::getBool("compactCamera");
		
		fields->conditionalValues = Utils::getBool("conditionalValues");

		fields->geodeInfo = Utils::getBool("geodeInfo");
		fields->compactGeode = Utils::getBool("compactGeode");

		fields->blendingDebugText = Utils::getBool("blendingDebugText");
		fields->chromaDebugText = Utils::getBool("chromaDebugText");
		fields->maxAlphaDebugText = Utils::getBool("maxAlphaDebugText");
		fields->textAlignRight = Utils::getBool("textAlignRight");
		fields->spaceUKScale = Utils::getBool("spaceUKScale");
		fields->positionAlignRight = Utils::getBool("positionAlignRight");
		fields->positionAlignBottom = Utils::getBool("positionAlignBottom");

		fields->logDebugText = Utils::getBool("logDebugText");

		fields->currentChannel = Utils::getBool("currentChannel");
		fields->lastPlayedAudio = Utils::getBool("lastPlayedAudio");
		fields->hIDe = Utils::getBool("hIDe");

		fields->jumps = Utils::getBool("jumps");
		fields->totalTime = Utils::getBool("totalTime");
		fields->totalAttempts = Utils::getBool("totalAttempts");
		fields->affectedByForces = Utils::getBool("affectedByForces");
		fields->compactGameplay = Utils::getBool("compactGameplay");
		fields->compactAudio = Utils::getBool("compactAudio");
		fields->expandPerformance = Utils::getBool("expandPerformance");
		fields->tapsToClicks = Utils::getBool("tapsToClicks");
		fields->fixLevelIDLabel = Utils::getBool("fixLevelIDLabel");
		fields->pluralAttempts = Utils::getBool("pluralAttempts");
		
		fields->fps = Utils::getBool("fps");
		fields->lastKey = Utils::getBool("lastKey");

		fields->gameplayHeader = Utils::getBool("gameplayHeader");

		fields->chromaDebugTextSpeed = Utils::getDouble("chromaDebugTextSpeed");

		fields->customFont = Utils::getInt("customFont");

		fields->customFooter = Utils::getString("customFooter");
	}
	std::string getCurrentTime() {
		const auto fields = m_fields.self();
		if (!fields->isEnabled) return "";
		Manager* manager = Manager::getSharedInstance();
		std::time_t tinnyTim = std::time(nullptr);
		std::tm* now = std::localtime(&tinnyTim);
		std::string month = manager->months[now->tm_mon + 1];
		int hour = now->tm_hour;
		std::string ampm = "";
		if (fields->twelveHour) {
			if (hour > 12) {
				hour = hour % 12;
				ampm = " PM";
			} else {
				if (hour == 0) hour = 12;
				ampm = " AM";
			}
		}
		if (fields->shortMonth && month != "May") month = fmt::format("{}", month.substr(0, 3));
		std::string dow = fields->dayOfWeek ? manager->daysOfWeek[now->tm_wday] : ""; // dow = day of week
		std::string dayOfWeek = !fields->dayOfWeek ? "" : fmt::format("{}, ", !fields->shortDayOfWeek ? dow : dow.substr(0, 3));
		std::string dateMonth = fields->dayFirst ?
			fmt::format("{} {}", now->tm_mday, month) : fmt::format("{} {}", month, now->tm_mday);
		std::string seconds = fields->includeSeconds ? fmt::format(":{:02}", now->tm_sec % 60) : "";
		std::string separator = fields->splitDateAndTime ? "\nTime: " : " ";
#ifndef GEODE_IS_WINDOWS
		std::string timeZone = fields->useUTC ? getUTCOffset() : now->tm_zone;
#else
		/*
		original approach: display UTC offset
		didn't work for cvolton apparently because of 0 offset
		*/

		/*
		std::tm* gmt = std::gmtime(&tinnyTim);
		std::string timeZone = fmt::format("UTC{:.2f}", static_cast<double>(difftime(mktime(now), mktime(gmt))) / 60 / 60);
		*/

		// "i love bill gates!", said no one ever, after discovering that timezone abbreviations were paywalled behind arbitrary bullshit

		/*
		second approach: force out an abbreviation of any sort by <ctime>'s string formatting
		apparently still didn't work for cvolton??? (he says it's currently CET for him and not CEST)
		[the "S" is for "'S'ummer", not "'S'tandard"]
		*/

		/*
		char buffer[80];
		strftime(buffer, 80, "%EZ", now);

		std::string timeZone = buffer;
		if (timeZone == "Coordinated Universal Time") timeZone = "UTC";
		std::regex capitalsOnly = std::regex("[^A-Z]");
		timeZone = std::regex_replace(timeZone, capitalsOnly, "");
		*/

		/*
		last resort: cvolton assist + manual UTC calculation
		(why can't MSVC just include a timezone abbv member variable like any other competent C implementation????)
		*/

		std::string timeZone = getUTCOffset();
#endif
		return fmt::format("\nDate: {}{}, {}{}{:02}:{:02}{}{} {}",
			dayOfWeek, dateMonth, now->tm_year + 1900, separator,
			hour, now->tm_min, seconds, ampm, timeZone
		);
	}
	static std::string getUTCOffset() {
		if (!Utils::modEnabled()) return "";
		// code adapted from cvolton with heavily implied permission
		// proof: https://discord.com/channels/911701438269386882/911702535373475870/1322321142819586099
		std::tm timeInfo;
		std::time_t epoch = 0;
#ifdef GEODE_IS_WINDOWS
		localtime_s(&timeInfo, &epoch);
#else
		localtime_r(&epoch, &timeInfo);
#endif
		if (timeInfo.tm_hour == 0 && timeInfo.tm_min == 0) return "UTC";
		char sign = timeInfo.tm_hour >= 12 ? '-' : timeInfo.tm_hour > 0 ? '+' : ' ';
		int hour = timeInfo.tm_hour >= 12 ? 24 - timeInfo.tm_hour : timeInfo.tm_hour;
		std::string minutes = timeInfo.tm_min != 0 ? fmt::format(":{:02}", timeInfo.tm_min) : "";
		return fmt::format("UTC{}{}{}", sign, hour, minutes);
	}
	std::string getUptime(std::time_t now = std::time(nullptr)) {
		const auto fields = m_fields.self();
		if (!fields->isEnabled || !fields->uptime) return "";
		long elapsed = difftime(now, Manager::getSharedInstance()->originalTimestamp);
		long days = elapsed / SECONDS_PER_DAY;
		elapsed -= days * SECONDS_PER_DAY;
		int hours = static_cast<int>(elapsed / SECONDS_PER_HOUR);
		elapsed -= hours * SECONDS_PER_HOUR;
		int minutes = static_cast<int>(elapsed / SECONDS_PER_MINUTE);
		elapsed -= minutes * SECONDS_PER_MINUTE;
		int seconds = static_cast<int>(elapsed);
		std::string daysString = days > 0 ? fmt::format("{}:", days) : "";
		// std::string daysString = fmt::format("{}:", days); // for debugging in case day count is way off
		std::string hoursString = hours > 0 ? fmt::format("{:02}:", hours) : "";
		// std::string hoursString = fmt::format("{:02}:", hours); // for debugging in case hour count is way off
		std::string minutesString = hours > 0 ? fmt::format("{:02}:", minutes) : fmt::format("{}:", minutes);
		std::string secondsString = fmt::format("{:02}", seconds);
		return fmt::format("\nUptime: {}{}{}{}", daysString, hoursString, minutesString, secondsString);
	}
	std::string getWindowInfo() {
		const auto fields = m_fields.self();
		if (!fields->isEnabled || !fields->miscInfo) return "";
		const int gcd = Utils::gcd(fields->winWidthPixels, fields->winHeightPixels);
		return fmt::format("Window: {}x{} ({}:{})\nFullscreen: {}",
			fields->winWidthPixels, fields->winHeightPixels,
			fields->winWidthPixels / gcd, fields->winHeightPixels / gcd,
			fields->isFullscreen ? "OFF" : "ON"
		);
	}
	std::string buildPlayerStatusString(PlayerObject* thePlayer) {
		const auto fields = m_fields.self();
		if (!fields->isEnabled || !fields->playerStatus) return "";
		std::string status = "Unknown";
		std::string playerNum = "";
		std::string fullVelocity = "";
		std::string fullRotation = "";

		std::string playerPosition = fields->isDual ? "Pos: " : "";
		if (fields->isDual) playerNum = fmt::format("[{}] ", thePlayer == this->m_player1 ? "P1" : "P2");

		bool isPlat = thePlayer->m_isPlatformer;
		bool compactDirs = fields->compactDirections;

		if (thePlayer->m_isShip) {
			if (!isPlat) status = "Ship";
			else { status = "Jetpack"; }
		}
		else if (thePlayer->m_isBall) status = "Ball";
		else if (thePlayer->m_isBird) status = "UFO";
		else if (thePlayer->m_isRobot) status = "Robot";
		else if (thePlayer->m_isSpider) status = "Spider";
		else if (thePlayer->m_isDart || thePlayer->m_isSwing) {
			if (thePlayer->m_isDart) status = "Wave";
			else { status = "Swing"; }
			if (isPlat) {
				if (fields->manager->gameVersion != "2.210") status = status.append("*");
				else { status = status.append("?"); }
			}
		}
		else { status = "Cube"; }

		if (thePlayer->m_vehicleSize == .6f) status = fmt::format("Mini {}", status);
		else if (thePlayer->m_vehicleSize != 1.f) status = status.append(" of strange size");

		if (thePlayer->m_isPlatformer) {
			if (thePlayer->m_isUpsideDown) {
				if (thePlayer->m_isSideways) {
					if (compactDirs) status = fmt::format("->] {}", status);
					else { status = fmt::format("Rightwards {}", status); }
				}
				else { status = fmt::format("Flipped {}", status); }
			} else if (thePlayer->m_isSideways) {
				if (compactDirs) status = fmt::format("[<- {}", status);
				else { status = fmt::format("Leftwards {}", status); }
			}
		} else {
			if (thePlayer->m_isUpsideDown) status = fmt::format("Flipped {}", status);
			if (thePlayer->m_isSideways) {
				if (thePlayer->m_isGoingLeft) {
					if (compactDirs) status = fmt::format("\\/ {}" , status);
					else { status = fmt::format("Downwards {}", status); }
				}
				else {
					if (compactDirs) status = fmt::format("/\\ {}" , status);
					else { status = fmt::format("Upwards {}", status); }
				}
			} else if (thePlayer->m_isGoingLeft) {
				if (compactDirs) status = fmt::format("<- {}", status);
				else { status = fmt::format("Reversed {}", status); }
			}
		}

		if (thePlayer->m_isDashing) status = fmt::format("<{}>", status);

		if (thePlayer->m_isHidden) status = fmt::format("Hidden {}", status);

		if (thePlayer != m_player2) {
			if (fields->isDual) status = status.append(" [Dual]");

			if (m_isPracticeMode) status = status.append(" {Practice}");
			else if (m_isTestMode) status = status.append(" {Testmode}");
		}

		if (thePlayer->m_isDead) status = status.append(" (Dead)");

		if (fields->isDual) {
			int positionAccuracy = fields->accuratePosition ? 4 : 0;

			CCPoint playerPos = thePlayer->m_position;
			std::string xPos = fmt::format("{:.{}f}", playerPos.x, positionAccuracy);
			std::string yPos = fmt::format("{:.{}f}", playerPos.y, positionAccuracy);

			playerPosition = playerPosition.append(fmt::format("({}, {})", xPos, yPos));
		}

		if (fields->velocityPlayer) {
			int veloAccuracy = fields->accuratePlayer ? 2 : 1;
			float xVelo = thePlayer->m_isPlatformer ? thePlayer->m_platformerXVelocity : thePlayer->m_playerSpeed;
			std::string xVeloStr = fmt::format("{:.{}f}", xVelo, veloAccuracy);
			std::string yVeloStr = fmt::format("{:.{}f}", thePlayer->m_yVelocity, veloAccuracy);
			if (!fields->isDual) fullVelocity = fmt::format("Velo: <{}, {}>", xVeloStr, yVeloStr);
			else fullVelocity = fmt::format(" / Velo: <{}, {}>", xVeloStr, yVeloStr);
		}

		if (fields->rotationPlayer) {
			int rotAccuracy = fields->accuratePlayer ? 2 : 0;
			std::string rotationStr = fmt::format("{:.{}f}", thePlayer->getRotation(), rotAccuracy);
			std::string rotationSpeedStr = fmt::format("{:.{}f}", thePlayer->m_rotationSpeed, rotAccuracy);
			if (!fields->isDual && !fields->velocityPlayer) fullRotation = fmt::format("Rot: [{}, {}]", rotationStr, rotationSpeedStr);
			else fullRotation = fmt::format(" / Rot: [{}, {}]", rotationStr, rotationSpeedStr);
		}

		std::string posVeloRot = fmt::format("{}{}{}{}", playerNum, playerPosition, fullVelocity, fullRotation);

		std::string fullPlayerStatus = fmt::format("{:.1f}x {}\n{}", thePlayer->m_playerSpeed, status, posVeloRot);

		// last hurrah, removing dangling decimals
		fullPlayerStatus = replaceXWithYInZ("\\.0+0", "", fullPlayerStatus);

		return fullPlayerStatus;
	}
	std::string buildLevelTraitsString() {
		const auto fields = m_fields.self();
		if (!fields->isEnabled || !fields->levelTraits) return "";
		std::string level = "Unknown";
		if (m_level->isPlatformer()) {
			level = "Platformer";
		} else {
			level = "Classic";
			if (m_level->m_levelLength == 0.f) level = level.append(" [Tiny]");
			else if (m_level->m_levelLength == 1.f) level = level.append(" [Short]");
			else if (m_level->m_levelLength == 2.f) level = level.append(" [Medium]");
			else if (m_level->m_levelLength == 3.f) level = level.append(" [Long]");
			else { level = level.append(" [XL]"); }
		}

		if (m_level->m_levelType == GJLevelType::Editor) {
			if (m_level->m_isVerifiedRaw) level = level.append(" (Verified)");
			else level = level.append(" (Unverified)");
		} else {
			if (m_level->m_levelType == GJLevelType::Saved) level = level.append(" (Online)");
			else if (m_level->m_levelType == GJLevelType::Local) level = level.append(" (Official)");
			else level = level.append(" (Unknown)");
		}

		if (m_level->m_twoPlayerMode) level = level.append(" {2P}");

		if (fields->isLevelComplete) level = level.append(" <Completed>");

		return level;
	}
	std::string buildCameraPropertiesString() {
		const auto fields = m_fields.self();
		if (!fields->isEnabled || !fields->cameraProperties) return "";
		// NEVER STORE A VARIABLE OF TYPE GJGameState, IT WILL FAIL ON ANDROID
		bool isCompactCam = fields->compactCamera;
		bool conditionalValues = fields->conditionalValues;

		CCPoint camPosition = m_gameState.m_cameraPosition;
		CCPoint camOffset = m_gameState.m_cameraOffset;

		float camZoom = m_gameState.m_cameraZoom;
		float camAngle = m_gameState.m_cameraAngle;

		bool allEdgeValuesZero = (m_gameState.m_cameraEdgeValue0 == 0 && m_gameState.m_cameraEdgeValue1 == 0 && m_gameState.m_cameraEdgeValue2 == 0 && m_gameState.m_cameraEdgeValue3 == 0);
		bool noOffsetAndConditionalLabels = (camOffset == ccp(0, 0) && conditionalValues);
		bool standardZoomAndConditional = (camZoom == 1.f && conditionalValues);

		std::string position = !isCompactCam ?
			fmt::format("\nPosition X: {:.2f}\nPosition Y: {:.2f}", camPosition.x, camPosition.y) :
			fmt::format("\nPos: ({:.2f}, {:.2f})", camPosition.x, camPosition.y);

		std::string offset = !isCompactCam ?
			noOffsetAndConditionalLabels ? "" : fmt::format("\nOffset X: {:.2f}\nOffset Y: {:.2f}", camOffset.x, camOffset.y) :
			noOffsetAndConditionalLabels ? "" : fmt::format("\nOffset: ({:.2f}, {:.2f})", camOffset.x, camOffset.y);

		std::string zoom = (standardZoomAndConditional) ? "" : fmt::format("Zoom: {:.2f}", camZoom);
		std::string angle = (camAngle == 0.f && conditionalValues) ? "" : fmt::format("{}Angle: {:.2f}", (standardZoomAndConditional) ? "" : ((isCompactCam) ? " | " : "\n"), camAngle);
		std::string zoomAndAngle = fmt::format("\n{}{}", zoom, angle);

		std::string edge = !(conditionalValues && allEdgeValuesZero) ? fmt::format(
			"\nEdge: {} / {} / {} / {}",
			m_gameState.m_cameraEdgeValue0, m_gameState.m_cameraEdgeValue1, m_gameState.m_cameraEdgeValue2, m_gameState.m_cameraEdgeValue3
		) : "";

		std::string shake = (m_gameState.m_cameraShakeEnabled || !conditionalValues) ?
			fmt::format("\nShake: {:.2f}", m_gameState.m_cameraShakeFactor) : "";

		return fmt::format(
			"-- Camera --{}{}{}{}{}",
			position, offset, zoomAndAngle, edge, shake
		);
	}
	std::string buildGeodeLoaderString(Manager* manager) {
		const auto fields = m_fields.self();
		if (!fields->isEnabled || !fields->geodeInfo) return "";
		if (fields->compactGeode) {
			return fmt::format(
				"-- Geode v{} --\nGD v{} on {}\nMods: {} + {} = {} ({})",
				manager->geodeVersion, manager->gameVersion, manager->platform, manager->loadedMods,
				manager->disabledMods, manager->installedMods, manager->problems
			);
		}
		return fmt::format(
			"-- Geode v{} --\nGD v{} on {}\nLoaded: {}\nDisabled: {}\nInstalled: {} ({} problem{})",
			manager->geodeVersion, manager->gameVersion, manager->platform, manager->loadedMods,
			manager->disabledMods, manager->installedMods, manager->problems, (manager->problems == 1) ? "" : "s"
		);
	}
	std::string buildMiscInfoString() {
		const auto fields = m_fields.self();
		if (!fields->isEnabled || !fields->miscInfo) return "";
		std::string textureQuality = fields->textureQuality ? fmt::format("\nQuality: {}", fields->textureQualityString) : "";
		return fmt::format("-- Misc --\n{}{}{}{}", getWindowInfo(), textureQuality, getCurrentTime(), getUptime());
	}
	static bool xContainedInY(std::string substring, const std::string_view string, const bool withColon = true) {
		if (!Utils::modEnabled() || string.empty()) return false;
		if (withColon) return (string.find(fmt::format("{}: ", substring)) != std::string::npos);
		return (string.find(substring) != std::string::npos);
	}
	static bool isInfoLabel(const std::string &candidateString) {
		return (xContainedInY("-- Audio --", candidateString, false) &&
			   xContainedInY("-- Perf --", candidateString, false) &&
			   xContainedInY("-- Area --", candidateString, false));
	}
	CCNode* findDebugTextNode() {
		if (!PlayLayer::get()) return nullptr;
		if (m_infoLabel != nullptr && isInfoLabel(m_infoLabel->getString())) return m_infoLabel;
		if (Utils::isModLoaded("geode.node-ids")) {
			if (CCLabelBMFont* infoLabelCandidate = typeinfo_cast<CCLabelBMFont*>(getChildByID("info-label"))) {
				if (isInfoLabel(infoLabelCandidate->getString())) return infoLabelCandidate;
			}
		}
		CCArrayExt<CCNode*> plArray = CCArrayExt<CCNode*>(getChildren());
		for (int i = plArray.size() - 1; i >= 0; i--) {
			// NEW [good]: int i = plArray.size() - 1; i >= 0; i--
			// ORIG [bad]: int i = plArray.size(); i-- > 0;
			if (CCLabelBMFont* nodeCandidate = typeinfo_cast<CCLabelBMFont*>(plArray[i])) {
				if (isInfoLabel(nodeCandidate->getString())) return nodeCandidate;
			}
		}
		return nullptr;
	}
	std::string hIDeString() {
		const auto fields = m_fields.self();
		std::mt19937 randomSeed(std::random_device{}());
		std::shuffle(fields->hIDeVector.begin(), fields->hIDeVector.end(), randomSeed);
		return fields->hIDeVector.front();
	}
	static std::string replaceXWithYInZ(const std::string& forRegex, const std::string& replacement, const std::string& mainString) {
		return std::regex_replace(mainString, std::regex(forRegex), replacement);
	}
	void onQuit() {
		const auto fields = m_fields.self();
		fields->isLevelComplete = false;
		fields->appliedBlending = false;
		fields->appliedChromaOrDefaultColor = false;
		fields->textAlignmentSet = false;
		fields->scaleSet = false;
		fields->opacitySet = false;
		fields->fontSet = false;
		fields->isDual = false;
		fields->hIDeSet = "";
		fields->debugText = nullptr;
		fields->manager->lastPlayedSong = "N/A";
		fields->manager->lastPlayedEffect = "N/A";
		fields->manager->lastKeyName = "N/A";
		fields->textureQuality = "Unknown";
		PlayLayer::onQuit();
	}
	void levelComplete() {
		m_fields->isLevelComplete = true;
		PlayLayer::levelComplete();
	}
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();
		m_fields->textureQualityString = Manager::getSharedInstance()->qualities[static_cast<int>(CCDirector::get()->getLoadedTextureQuality())];
		MyPlayLayer::setupSettings();
	}
	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);
		const auto fields = m_fields.self();
		if (!fields->isEnabled || fields->manager->isMinecraftify) return;

		fields->debugText = findDebugTextNode();
		if (!fields->debugText || !fields->debugText->isVisible()) return;

		CCLabelBMFont* debugTextNode = typeinfo_cast<CCLabelBMFont*>(fields->debugText);
		if (!debugTextNode || !debugTextNode->isVisible()) return;

		fields->isDual = m_gameState.m_isDualMode;
		bool forcesExist = m_player1->m_affectedByForces;
		if (!forcesExist && fields->isDual && m_player2) forcesExist = m_player2->m_affectedByForces;

		if (fields->hIDeSet.empty()) fields->hIDeSet = hIDeString();

		if (fields->blendingDebugText && !fields->appliedBlending) {
			debugTextNode->setBlendFunc({GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA}); // Manager::glBlendFuncs[5], Manager::glBlendFuncs[7]
			fields->appliedBlending = true;
		} else fields->appliedBlending = false;
		if (!fields->appliedChromaOrDefaultColor) {
			if (!fields->chromaDebugText) debugTextNode->setColor({255, 255, 255});
			// ensure that node color is white in case someone turns off chroma mode mid-session
			else {
				CCFiniteTimeAction* tintOne = CCTintTo::create(fields->chromaDebugTextSpeed, 255, 128, 128);
				CCFiniteTimeAction* tintTwo = CCTintTo::create(fields->chromaDebugTextSpeed, 255, 255, 128);
				CCFiniteTimeAction* tintThree = CCTintTo::create(fields->chromaDebugTextSpeed, 128, 255, 128);
				CCFiniteTimeAction* tintFour = CCTintTo::create(fields->chromaDebugTextSpeed, 128, 255, 255);
				CCFiniteTimeAction* tintFive = CCTintTo::create(fields->chromaDebugTextSpeed, 128, 128, 255);
				CCFiniteTimeAction* tintSix = CCTintTo::create(fields->chromaDebugTextSpeed, 255, 128, 255);
				CCActionInterval* sequence = CCSequence::create(tintOne, tintTwo, tintThree, tintFour, tintFive, tintSix, nullptr);
				CCAction* repeat = CCRepeatForever::create(sequence);
				debugTextNode->runAction(repeat);
			}
			fields->appliedChromaOrDefaultColor = true;
		}
		if (!fields->opacitySet) {
			if (fields->maxAlphaDebugText) debugTextNode->setOpacity(255);
			else if (Utils::getInt("debugTextAlpha") != 150)
				debugTextNode->setOpacity(Utils::getInt("debugTextAlpha"));
			fields->opacitySet = true;
		}
		if (!fields->fontSet) {
			const int64_t fontID = fields->customFont;
			if (fontID == -2) debugTextNode->setFntFile("goldFont.fnt");
			else if (fontID == -1) debugTextNode->setFntFile("bigFont.fnt");
			else if (fontID != 0)
				debugTextNode->setFntFile(fmt::format("gjFont{:02d}.fnt", fontID).c_str());
			fields->fontSet = true;
		}
		if (!fields->textAlignmentSet && fields->textAlignRight) {
			debugTextNode->setAlignment(CCTextAlignment::kCCTextAlignmentRight);
			fields->textAlignmentSet = true;
		}
		if (!fields->scaleSet) {
			if (fields->spaceUKScale) debugTextNode->setScale(0.5 * (940.f / 1004.f));
			else if (Utils::getDouble("customScale") != .5)
				debugTextNode->setScale(Utils::getDouble("customScale"));
			fields->scaleSet = true;
		}
		if (fields->positionAlignRight || fields->positionAlignBottom) {
			debugTextNode->ignoreAnchorPointForPosition(false);
			debugTextNode->setAnchorPoint({
				fields->positionAlignRight ? 1.f : 0.f, // positionAlignRight: anchorPointX
				fields->positionAlignBottom ? 0.f : 1.f // positionAlignBottom: anchorPointY
			});
			if (fields->positionAlignRight) debugTextNode->setPositionX(CCDirector::get()->getWinSize().width - 5);
			if (fields->positionAlignBottom) debugTextNode->setPositionY(10);
		
			std::string debugTextContents = debugTextNode->getString();

			if (fields->logDebugText)
				log::info("\n--- LOGGED DEBUG TEXT [BEFORE INFOLABELTWEAKS] ---\n{}", debugTextContents);
			if (fields->isDual && fields->playerStatus) {
				debugTextContents = replaceXWithYInZ("\nX: \\d+\nY:", "\nY:", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nY: \\d+\nActive:", "\nActive:", debugTextContents);
			}
			if (fields->currentChannel)
				debugTextContents = replaceXWithYInZ(
					"\n-- Audio --",
					fmt::format("\nChannel: {}\n-- Audio --", m_gameState.m_currentChannel),
					debugTextContents
				);
			if (fields->lastPlayedAudio)
				debugTextContents = replaceXWithYInZ(
					"\n(\r)?-- Audio --\nSongs: ",
					fmt::format("\n-- Audio --\nLast Song: {}\nLast SFX: {}\nSongs: ", fields->manager->lastPlayedSong, fields->manager->lastPlayedEffect),
					debugTextContents
				);
			if (fields->hIDe && (m_level->m_levelType == GJLevelType::Editor || m_level->m_unlisted)) {
				std::smatch match;
				if (std::regex_search(debugTextContents, match, levelIDRegex)) {
					debugTextContents = replaceXWithYInZ(
						"LevelID: \\d+\nTime: ",
						fmt::format("LevelID: [{}]\nTime: ", fields->hIDeSet),
						debugTextContents
					);
				}
			}
			if (fields->jumps) {
				std::smatch match;
				if (std::regex_search(debugTextContents, match, tapsCountRegex)) {
					debugTextContents = replaceXWithYInZ(
						"Taps: \\d+\nTimeWarp: ",
						fmt::format("Taps: {} [{}]\nTimeWarp: ", match[1].str(), m_jumps),
						debugTextContents
					); // jump count from playlayer members: m_jump
				}
			}
			if (fields->totalTime && m_level->isPlatformer()) {
				std::smatch match;
				if (std::regex_search(debugTextContents, match, timeLabelRegex))
					debugTextContents = replaceXWithYInZ(
						"Time: \\d+(\\.\\d+)?\nAttempt: ",
						fmt::format("Time: {} [{}]\nAttempt: ", match[1].str(), fmt::format("{:.2f}", m_gameState.m_levelTime)),
						debugTextContents
					); // attempt time from playlayer gamestate member: m_gameState.m_levelTime
			}
			if (fields->totalAttempts) {
				std::smatch match;
				if (std::regex_search(debugTextContents, match, attemptCountRegex))
					debugTextContents = replaceXWithYInZ(
						"Attempts?: \\d+\nTaps: ",
						fmt::format("Attempt: {} / {}\nTaps: ", match[1].str(), m_level->m_attempts.value()),
						debugTextContents
					); // total attempt count: m_level->m_attempts.value() [playlayer]
			}
			if (fields->affectedByForces && forcesExist) {
				std::smatch match;
				if (std::regex_search(debugTextContents, match, gravityModRegex))
					debugTextContents = replaceXWithYInZ(
						"Gravity: \\d+(\\.\\d+)?\nX: ",
						fmt::format("Gravity: [{}]\nX: ", match[1].str()),
						debugTextContents
					); // worst case scenario: m_gravityMod
			}
			if (fields->accuratePosition) {
				debugTextContents = replaceXWithYInZ("\nX: (\\d)+\n", fmt::format("\nX: {:.4f}\n", m_player1->m_position.x), debugTextContents);
				debugTextContents = replaceXWithYInZ("\nY: (\\d)+\n", fmt::format("\nY: {:.4f}\n", m_player1->m_position.y), debugTextContents);
			}
			if (fields->playerStatus)
				debugTextContents = replaceXWithYInZ(
					"\n-- Audio --",
					(fields->isDual) ? fmt::format(
						"\nP1 Status: {}\nP2 Status: {}\n-- Audio --",
						buildPlayerStatusString(m_player1),
						buildPlayerStatusString(m_player2)
					) : fmt::format(
						"\nStatus: {}\n-- Audio --",
						buildPlayerStatusString(m_player1)
					),
					debugTextContents
				);
			if (fields->levelTraits)
				debugTextContents = replaceXWithYInZ(
					"\n-- Audio --",
					fmt::format("\nLevel: {}\n-- Audio --", buildLevelTraitsString()),
					debugTextContents
				);
			if (fields->conditionalValues) {
				// cannot condense into one regex per situation it seems
				debugTextContents = replaceXWithYInZ("\nTimeWarp: 1\n", "\n", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nGravity: 1\n", "\n", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nGradients: 0", "", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nParticles: 0", "", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nChannel: 0", "", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nMove: 0\n", "\n", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nSongs: 0\n", "\n", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nSFX: 0\n", "\n", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nRotate: 0\n", "\n", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nScale: 0\n", "\n", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nFollow: 0\n", "\n", debugTextContents);
				debugTextContents = replaceXWithYInZ("\n-- Perf --\n--", "\n--", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nMove: 0 / 0", "", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nRotate: 0 / 0", "", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nScale: 0 / 0", "", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nFollow: 0 / 0", "", debugTextContents);
				debugTextContents = replaceXWithYInZ("\nColOp: 0 / 0", "", debugTextContents);
				debugTextContents = replaceXWithYInZ("\n-- Audio --\n--", "\n--", debugTextContents);
				if (debugTextContents.ends_with(fields->endsWithArea)) debugTextContents = replaceXWithYInZ(fields->endsWithArea, "\n", debugTextContents);
			}
			if (fields->compactGameplay) {
				debugTextContents = replaceXWithYInZ("\nTaps: ", " | Taps: ", debugTextContents); // Attempt and Taps
				if (xContainedInY("TimeWarp", debugTextContents))
					debugTextContents = replaceXWithYInZ("\nGravity: ", " | Gravity: ", debugTextContents); // TimeWarp and Gravity
				if (xContainedInY("Gradients", debugTextContents) || xContainedInY("Active", debugTextContents))
					debugTextContents = replaceXWithYInZ("\nParticles: ", " | Particles: ", debugTextContents); // Gradients and Particles
				debugTextContents = replaceXWithYInZ("\nY: ", " | Y: ", debugTextContents); // X and Y position
			}
			if (fields->compactAudio && xContainedInY("Songs", debugTextContents))
				debugTextContents = replaceXWithYInZ("\nSFX: ", " | SFX: ", debugTextContents);
			if (fields->expandPerformance)
				debugTextContents = replaceXWithYInZ("-- Perf --", "-- Performance --", debugTextContents);
			if (fields->tapsToClicks)
				debugTextContents = replaceXWithYInZ("Taps: ", (m_level->isPlatformer()) ? "Actions: " : "Clicks: ", debugTextContents);
			if (fields->fixLevelIDLabel)
				debugTextContents = replaceXWithYInZ("LevelID: ", "Level ID: ", debugTextContents);
			if (fields->pluralAttempts)
				debugTextContents = replaceXWithYInZ("Attempt: ", "Attempts: ", debugTextContents);
			if (fields->fps || fields->lastKey) {
				std::string fps = fields->fps ?
					fmt::format("FPS: {:.0f}", CCDirector::get()->m_fFrameRate) : "";
				std::string lastKey = fields->lastKey ?
					fmt::format("Last Key: {}", fields->manager->lastKeyName) : "";
				std::string merger = "";
				if (!fps.empty() && !lastKey.empty())
					merger = fields->compactGameplay ? " | " : "\n";
				debugTextContents = fmt::format("{}{}{}\n{}", fps, merger, lastKey, debugTextContents);
			}
			if (fields->gameplayHeader)
				debugTextContents = fmt::format("-- Gameplay --\n{}", debugTextContents);
			if (fields->cameraProperties)
				debugTextContents = debugTextContents.append(fmt::format("{}\n", buildCameraPropertiesString()));
			if (fields->geodeInfo)
				debugTextContents = debugTextContents.append(fmt::format("{}\n", buildGeodeLoaderString(fields->manager)));
			if (fields->miscInfo)
				debugTextContents = debugTextContents.append(fmt::format("{}\n", buildMiscInfoString()));
			const std::string& customFooter = fields->customFooter;
			if (!customFooter.empty()) {
				std::smatch match;
				if (std::regex_search(customFooter, match, asciiOnlyMaxTwentyRegex))
					debugTextContents = debugTextContents.append(fmt::format("-- [{}] --", (customFooter.length() > 20) ? customFooter.substr(20) : customFooter));
			}

			// last hurrah
			debugTextContents = replaceXWithYInZ("\n\n", "\n", debugTextContents);
			debugTextNode->setString(debugTextContents.c_str());

			if (fields->logDebugText)
				log::info("\n--- LOGGED DEBUG TEXT [AFTER INFOLABELTWEAKS] ---\n{}", debugTextContents);
		}
	}
};