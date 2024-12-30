#include <Geode/modify/PlayLayer.hpp>
#include "Utils.hpp"
#include "Manager.hpp"
#include <regex>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

using namespace geode::prelude;

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
		std::vector<std::string> hIDe = {
			"It's locked!", "Shh...",
			"Nonzero", "Streisand",
			"N/A", "Expunged",
			"Locked", "hIDden"
		};
		std::string textureQuality = "Unknown";
		float winHeightPixels = CCDirector::get()->getWinSizeInPixels().height;
		float winWidthPixels = CCDirector::get()->getWinSizeInPixels().width;
		bool isFullscreen = GameManager::get()->getGameVariable("0025");
	};
	std::string getCurrentTime() {
		if (!Utils::modEnabled()) return "";
		Manager* manager = Manager::getSharedInstance();
		std::time_t tinnyTim = std::time(nullptr);
		std::tm* now = std::localtime(&tinnyTim);
		std::string month = manager->months[now->tm_mon + 1];
		int hour = now->tm_hour;
		std::string ampm = "";
		if (Utils::getBool("twelveHour")) {
			if (hour > 12) {
				hour = hour % 12;
				ampm = " PM";
			} else {
				if (hour == 0) hour = 12;
				ampm = " AM";
			}
		}
		if (Utils::getBool("shortMonth") && month != "May") {
			month = fmt::format("{}", month.substr(0, 3));
		}
		std::string dow = Utils::getBool("dayOfWeek") ? manager->daysOfWeek[now->tm_wday] : ""; // dow = day of week
		std::string dayOfWeek = !Utils::getBool("dayOfWeek") ? "" : fmt::format("{}, ", !Utils::getBool("shortDayOfWeek") ? dow : dow.substr(0, 3));
		std::string dateMonth = Utils::getBool("dayFirst") ?
			fmt::format("{} {}", now->tm_mday, month) : fmt::format("{} {}", month, now->tm_mday);
		std::string seconds = Utils::getBool("includeSeconds") ? fmt::format(":{:02}", now->tm_sec % 60) : "";
		std::string separator = Utils::getBool("splitDateAndTime") ? "\nTime: " : " ";
		#ifndef GEODE_IS_WINDOWS
		std::string timeZone = Utils::getBool("useUTC") ? getUTCOffset() : now->tm_zone;
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

		std::string timeZone = Utils::getUTCOffset();
		#endif
		return fmt::format("\nDate: {}{}, {}{}{:02}:{:02}{}{} {}",
			dayOfWeek, dateMonth, now->tm_year + 1900, separator,
			hour, now->tm_min, seconds, ampm, timeZone
		);
	}
	std::string getUTCOffset() {
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
	std::string getWindowInfo() {
		if (!Utils::modEnabled() || !Utils::getBool("miscInfo")) return "";
		const int gcd = Utils::gcd(m_fields->winWidthPixels, m_fields->winHeightPixels);
		return fmt::format("Window: {}x{} ({}:{})\nFullscreen: {}",
			m_fields->winWidthPixels, m_fields->winHeightPixels,
			m_fields->winWidthPixels / gcd, m_fields->winHeightPixels / gcd,
			m_fields->isFullscreen ? "OFF" : "ON"
		);
	}
	std::string buildPlayerStatusString(PlayerObject* thePlayer) {
		if (!Utils::modEnabled() || !Utils::getBool("playerStatus")) return "";
		std::string status = "Unknown";
		std::string playerNum = "";
		std::string fullVelocity = "";
		std::string fullRotation = "";

		std::string playerPosition = m_fields->isDual ? "Pos: " : "";
		if (m_fields->isDual) { playerNum = fmt::format("[{}] ", thePlayer == this->m_player1 ? "P1" : "P2"); }

		bool isPlat = thePlayer->m_isPlatformer;
		bool compactDirs = Utils::getBool("compactDirections");

		if (thePlayer->m_isShip) {
			if (!isPlat) { status = "Ship"; }
			else { status = "Jetpack"; }
		}
		else if (thePlayer->m_isBall) { status = "Ball"; }
		else if (thePlayer->m_isBird) { status = "UFO"; }
		else if (thePlayer->m_isRobot) { status = "Robot"; }
		else if (thePlayer->m_isSpider) { status = "Spider"; }
		else if (thePlayer->m_isDart || thePlayer->m_isSwing) {
			if (thePlayer->m_isDart) { status = "Wave"; }
			else { status = "Swing"; }
			if (isPlat) {
				if (m_fields->manager->gameVersion != "2.210") { status = status.append("*"); }
				else { status = status.append("?"); }
			}
		}
		else { status = "Cube"; }

		if (thePlayer->m_vehicleSize == .6f) { status = fmt::format("Mini {}", status); }
		else if (thePlayer->m_vehicleSize != 1.f) { status = status.append(" of strange size"); }

		if (thePlayer->m_isPlatformer) {
			if (thePlayer->m_isUpsideDown) {
				if (thePlayer->m_isSideways) {
					if (compactDirs) { status = fmt::format("->] {}", status); }
					else { status = fmt::format("Rightwards {}", status); }
				}
				else { status = fmt::format("Flipped {}", status); }
			} else if (thePlayer->m_isSideways) {
				if (compactDirs) { status = fmt::format("[<- {}", status); }
				else { status = fmt::format("Leftwards {}", status); }
			}
		} else {
			if (thePlayer->m_isUpsideDown) { status = fmt::format("Flipped {}", status); }
			if (thePlayer->m_isSideways) {
				if (thePlayer->m_isGoingLeft) {
					if (compactDirs) { status = fmt::format("\\/ {}" , status); }
					else { status = fmt::format("Downwards {}", status); }
				}
				else {
					if (compactDirs) { status = fmt::format("/\\ {}" , status); }
					else { status = fmt::format("Upwards {}", status); }
				}
			} else if (thePlayer->m_isGoingLeft) {
				if (compactDirs) { status = fmt::format("<- {}", status); }
				else { status = fmt::format("Reversed {}", status); }
			}
		}

		if (thePlayer->m_isDashing) { status = fmt::format("<{}>", status); }

		if (thePlayer->m_isHidden) { status = fmt::format("Hidden {}", status); }

		if (thePlayer != m_player2) {
			if (m_fields->isDual) { status = status.append(" [Dual]"); }

			if (m_isPracticeMode) { status = status.append(" {Practice}"); }
			else if (m_isTestMode) { status = status.append(" {Testmode}"); }
		}

		if (thePlayer->m_isDead) { status = status.append(" (Dead)"); }

		if (m_fields->isDual) {
			int positionAccuracy = Utils::getBool("accuratePosition") ? 4 : 0;

			CCPoint playerPos = thePlayer->m_position;
			std::string xPos = fmt::format("{:.{}f}", playerPos.x, positionAccuracy);
			std::string yPos = fmt::format("{:.{}f}", playerPos.y, positionAccuracy);

			playerPosition = playerPosition.append(fmt::format("({}, {})", xPos, yPos));
		}

		if (Utils::getBool("velocityPlayer")) {
			int veloAccuracy = Utils::getBool("accuratePlayer") ? 2 : 1;
			float xVelo = thePlayer->m_isPlatformer ? thePlayer->m_platformerXVelocity : thePlayer->m_playerSpeed;
			std::string xVeloStr = fmt::format("{:.{}f}", xVelo, veloAccuracy);
			std::string yVeloStr = fmt::format("{:.{}f}", thePlayer->m_yVelocity, veloAccuracy);
			fullVelocity = fmt::format(" / Velo: <{}, {}>", xVeloStr, yVeloStr);
			if (!m_fields->isDual && !Utils::getBool("rotationPlayer")) {
				fullVelocity = fmt::format("Velo: <{}, {}>", xVeloStr, yVeloStr);
			}
		}

		if (Utils::getBool("rotationPlayer")) {
			int rotAccuracy = Utils::getBool("accuratePlayer") ? 2 : 0;
			std::string rotationStr = fmt::format("{:.{}f}", thePlayer->getRotation(), rotAccuracy);
			std::string rotationSpeedStr = fmt::format("{:.{}f}", thePlayer->m_rotationSpeed, rotAccuracy);
			fullRotation = fmt::format(" / Rot: [{}, {}]", rotationStr, rotationSpeedStr);
			if (!m_fields->isDual && !Utils::getBool("velocityPlayer")) {
				fullRotation = fmt::format("Rot: [{}, {}]", rotationStr, rotationSpeedStr);
			}
		}

		std::string posVeloRot = fmt::format("{}{}{}{}", playerNum, playerPosition, fullVelocity, fullRotation);

		std::string fullPlayerStatus = fmt::format("{:.1f}x {}\n{}", thePlayer->m_playerSpeed, status, posVeloRot);

		// last hurrah, removing dangling decimals
		fullPlayerStatus = replaceXWithYInZ("\\.0+0", "", fullPlayerStatus);

		return fullPlayerStatus;
	}
	std::string buildLevelTraitsString() {
		if (!Utils::modEnabled() || !Utils::getBool("levelTraits")) return "";
		std::string level = "Unknown";
		if (m_level->isPlatformer()) {
			level = "Platformer";
		} else {
			level = "Classic";
			if (m_level->m_levelLength == 0.f) { level = level.append(" [Tiny]"); }
			else if (m_level->m_levelLength == 1.f) { level = level.append(" [Short]"); }
			else if (m_level->m_levelLength == 2.f) { level = level.append(" [Medium]"); }
			else if (m_level->m_levelLength == 3.f) { level = level.append(" [Long]"); }
			else { level = level.append(" [XL]"); }
		}

		if (m_level->m_levelType == GJLevelType::Editor) {
			if (m_level->m_isVerifiedRaw) { level = level.append(" (Verified)"); }
			else { level = level.append(" (Unverified)"); }
		} else {
			if (m_level->m_levelType == GJLevelType::Saved) { level = level.append(" (Online)"); }
			else if (m_level->m_levelType == GJLevelType::Local) { level = level.append(" (Official)"); }
			else { level = level.append(" (Unknown)"); }
		}

		if (m_level->m_twoPlayerMode) { level = level.append(" {2P}"); }

		if (m_fields->isLevelComplete) { level = level.append(" <Completed>"); }

		return level;
	}
	std::string buildCameraPropertiesString() {
		if (!Utils::modEnabled() || !Utils::getBool("cameraProperties")) return "";
		// NEVER STORE A VARIABLE OF TYPE GJGameState, IT WILL FAIL ON ANDROID
		bool isCompactCam = Utils::getBool("compactCamera");
		bool conditionalValues = Utils::getBool("conditionalValues");

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
	static std::string buildGeodeLoaderString(Manager* manager) {
		if (!Utils::modEnabled() || !Utils::getBool("geodeInfo")) return "";
		if (Utils::getBool("compactGeode")) {
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
		if (!Utils::modEnabled() || !Utils::getBool("miscInfo")) return "";
		std::string textureQuality = Utils::getBool("textureQuality") ? fmt::format("\nQuality: {}", m_fields->textureQuality) : "";
		return fmt::format("-- Misc --\n{}{}{}", getWindowInfo(), textureQuality, getCurrentTime());
	}
	static bool xContainedInY(std::string substring, std::string_view string, bool withColon = true) {
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
		std::mt19937 randomSeed(std::random_device{}());
		std::shuffle(m_fields->hIDe.begin(), m_fields->hIDe.end(), randomSeed);
		return m_fields->hIDe.front();
	}
	static std::string replaceXWithYInZ(const std::string& forRegex, const std::string& replacement, const std::string& mainString) {
		return std::regex_replace(mainString, std::regex(forRegex), replacement);
	}
	void onQuit() {
		m_fields->isLevelComplete = false;
		m_fields->appliedBlending = false;
		m_fields->appliedChromaOrDefaultColor = false;
		m_fields->textAlignmentSet = false;
		m_fields->scaleSet = false;
		m_fields->opacitySet = false;
		m_fields->fontSet = false;
		m_fields->isDual = false;
		m_fields->hIDeSet = "";
		m_fields->debugText = nullptr;
		m_fields->manager->lastPlayedSong = "N/A";
		m_fields->manager->lastPlayedEffect = "N/A";
		m_fields->manager->lastKeyName = "N/A";
		m_fields->textureQuality = "Unknown";
		PlayLayer::onQuit();
	}
	void levelComplete() {
		m_fields->isLevelComplete = true;
		PlayLayer::levelComplete();
	}
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();
		m_fields->textureQuality = Manager::getSharedInstance()->qualities[static_cast<int>(CCDirector::get()->getLoadedTextureQuality())];
	}
	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);
		if (!Utils::modEnabled() || m_fields->manager->isMinecraftify) return;

		m_fields->debugText = findDebugTextNode();
		if (!m_fields->debugText || !m_fields->debugText->isVisible()) return;

		CCLabelBMFont* debugTextNode = typeinfo_cast<CCLabelBMFont*>(m_fields->debugText);
		if (!debugTextNode || !debugTextNode->isVisible()) return;

		m_fields->isDual = m_gameState.m_isDualMode;
		bool forcesExist = m_player1->m_affectedByForces;
		if (!forcesExist && m_fields->isDual && m_player2) forcesExist = m_player2->m_affectedByForces;

		if (m_fields->hIDeSet.empty()) { m_fields->hIDeSet = hIDeString(); }

		if (Utils::getBool("blendingDebugText") && !m_fields->appliedBlending) {
			debugTextNode->setBlendFunc({GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA}); // Manager::glBlendFuncs[5], Manager::glBlendFuncs[7]
			m_fields->appliedBlending = true;
		} else m_fields->appliedBlending = false;
		if (!m_fields->appliedChromaOrDefaultColor) {
			if (!Utils::getBool("chromaDebugText")) {
				debugTextNode->setColor({255, 255, 255}); // ensure that node color is white in case someone turns off chroma mode mid-session
			} else {
				double chromaSpeed = Utils::getDouble("chromaDebugTextSpeed");
				CCFiniteTimeAction* tintOne = CCTintTo::create(chromaSpeed, 255, 128, 128);
				CCFiniteTimeAction* tintTwo = CCTintTo::create(chromaSpeed, 255, 255, 128);
				CCFiniteTimeAction* tintThree = CCTintTo::create(chromaSpeed, 128, 255, 128);
				CCFiniteTimeAction* tintFour = CCTintTo::create(chromaSpeed, 128, 255, 255);
				CCFiniteTimeAction* tintFive = CCTintTo::create(chromaSpeed, 128, 128, 255);
				CCFiniteTimeAction* tintSix = CCTintTo::create(chromaSpeed, 255, 128, 255);
				CCActionInterval* sequence = CCSequence::create(tintOne, tintTwo, tintThree, tintFour, tintFive, tintSix, nullptr);
				CCAction* repeat = CCRepeatForever::create(sequence);
				debugTextNode->runAction(repeat);
			}
			m_fields->appliedChromaOrDefaultColor = true;
		}
		if (!m_fields->opacitySet) {
			if (Utils::getBool("maxAlphaDebugText")) {
				debugTextNode->setOpacity(255);
			} else if (Utils::getInt("debugTextAlpha") != 150) {
				debugTextNode->setOpacity(Utils::getInt("debugTextAlpha"));
			}
			m_fields->opacitySet = true;
		}
		if (!m_fields->fontSet) {
			int64_t fontID = Utils::getInt("customFont");
			if (fontID == -2) {
				debugTextNode->setFntFile("goldFont.fnt");
			} else if (fontID == -1) {
				debugTextNode->setFntFile("bigFont.fnt");
			} else if (fontID != 0) {
				debugTextNode->setFntFile(fmt::format("gjFont{:02d}.fnt", fontID).c_str());
			}
			m_fields->fontSet = true;
		}
		if (!m_fields->textAlignmentSet && Utils::getBool("textAlignRight")) {
			debugTextNode->setAlignment(CCTextAlignment::kCCTextAlignmentRight);
			m_fields->textAlignmentSet = true;
		}
		if (!m_fields->scaleSet) {
			if (Utils::getBool("spaceUKScale")) {
				debugTextNode->setScale(0.5 * (940.f / 1004.f));
			} else if (Utils::getDouble("customScale") != .5) {
				debugTextNode->setScale(Utils::getDouble("customScale"));
			}
			m_fields->scaleSet = true;
		}
		if (Utils::getBool("positionAlignRight") || Utils::getBool("positionAlignBottom")) {
			debugTextNode->ignoreAnchorPointForPosition(false);
			debugTextNode->setAnchorPoint({
				Utils::getBool("positionAlignRight") ? 1.f : 0.f, // positionAlignRight: anchorPointX
				Utils::getBool("positionAlignBottom") ? 0.f : 1.f // positionAlignBottom: anchorPointY
			});
			if (Utils::getBool("positionAlignRight")) { debugTextNode->setPositionX(CCDirector::get()->getWinSize().width - 5); }
			if (Utils::getBool("positionAlignBottom")) { debugTextNode->setPositionY(10); }
		}

		std::string debugTextContents = debugTextNode->getString();

		if (Utils::getBool("logDebugText")) {
			log::info("\n--- LOGGED DEBUG TEXT [BEFORE INFOLABELTWEAKS] ---\n{}", debugTextContents);
		}
		if (m_fields->isDual && Utils::getBool("playerStatus")) {
			debugTextContents = replaceXWithYInZ("\nX: \\d+\nY:", "\nY:", debugTextContents);
			debugTextContents = replaceXWithYInZ("\nY: \\d+\nActive:", "\nActive:", debugTextContents);
		}
		if (Utils::getBool("currentChannel")) {
			debugTextContents = replaceXWithYInZ(
				"\n-- Audio --",
				fmt::format("\nChannel: {}\n-- Audio --", m_gameState.m_currentChannel),
				debugTextContents
			);
		}
		if (Utils::getBool("lastPlayedAudio")) {
			debugTextContents = replaceXWithYInZ(
				"\n(\r)?-- Audio --\nSongs: ",
				fmt::format("\n-- Audio --\nLast Song: {}\nLast SFX: {}\nSongs: ", m_fields->manager->lastPlayedSong, m_fields->manager->lastPlayedEffect),
				debugTextContents
			);
		}
		if (Utils::getBool("hIDe") && (m_level->m_levelType == GJLevelType::Editor || m_level->m_unlisted)) {
			std::smatch match;
			if (std::regex_search(debugTextContents, match, levelIDRegex)) {
				debugTextContents = replaceXWithYInZ(
					"LevelID: \\d+\nTime: ",
					fmt::format("LevelID: [{}]\nTime: ", m_fields->hIDeSet),
					debugTextContents
				);
			}
		}
		if (Utils::getBool("jumps")) {
			std::smatch match;
			if (std::regex_search(debugTextContents, match, tapsCountRegex)) {
				debugTextContents = replaceXWithYInZ(
					"Taps: \\d+\nTimeWarp: ",
					fmt::format("Taps: {} [{}]\nTimeWarp: ", match[1].str(), m_jumps),
					debugTextContents
				); // jump count from playlayer members: m_jump
			}
		}
		if (Utils::getBool("totalTime") && m_level->isPlatformer()) {
			std::smatch match;
			if (std::regex_search(debugTextContents, match, timeLabelRegex)) {
				debugTextContents = replaceXWithYInZ(
					"Time: \\d+(\\.\\d+)?\nAttempt: ",
					fmt::format("Time: {} [{}]\nAttempt: ", match[1].str(), fmt::format("{:.2f}", m_gameState.m_levelTime)),
					debugTextContents
				); // attempt time from playlayer gamestate member: m_gameState.m_levelTime
			}
		}
		if (Utils::getBool("totalAttempts")) {
			std::smatch match;
			if (std::regex_search(debugTextContents, match, attemptCountRegex)) {
				debugTextContents = replaceXWithYInZ(
					"Attempts?: \\d+\nTaps: ",
					fmt::format("Attempt: {} / {}\nTaps: ", match[1].str(), m_level->m_attempts.value()),
					debugTextContents
				); // total attempt count: m_level->m_attempts.value() [playlayer]
			}
		}
		if (Utils::getBool("affectedByForces") && forcesExist) {
			std::smatch match;
			if (std::regex_search(debugTextContents, match, gravityModRegex)) {
				debugTextContents = replaceXWithYInZ(
					"Gravity: \\d+(\\.\\d+)?\nX: ",
					fmt::format("Gravity: [{}]\nX: ", match[1].str()),
					debugTextContents
				); // worst case scenario: m_gravityMod
			}
		}
		if (Utils::getBool("accuratePosition")) {
			debugTextContents = replaceXWithYInZ("\nX: (\\d)+\n", fmt::format("\nX: {:.4f}\n", m_player1->m_position.x), debugTextContents);
			debugTextContents = replaceXWithYInZ("\nY: (\\d)+\n", fmt::format("\nY: {:.4f}\n", m_player1->m_position.y), debugTextContents);
		}
		if (Utils::getBool("playerStatus")) {
			debugTextContents = replaceXWithYInZ(
				"\n-- Audio --",
				(m_fields->isDual) ? fmt::format(
					"\nP1 Status: {}\nP2 Status: {}\n-- Audio --",
					buildPlayerStatusString(m_player1),
					buildPlayerStatusString(m_player2)
				) : fmt::format(
					"\nStatus: {}\n-- Audio --",
					buildPlayerStatusString(m_player1)
				),
				debugTextContents
			);
		}
		if (Utils::getBool("levelTraits")) {
			debugTextContents = replaceXWithYInZ(
				"\n-- Audio --",
				fmt::format("\nLevel: {}\n-- Audio --", buildLevelTraitsString()),
				debugTextContents
			);
		}
		if (Utils::getBool("conditionalValues")) {
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
			if (debugTextContents.ends_with(m_fields->endsWithArea)) { debugTextContents = replaceXWithYInZ(m_fields->endsWithArea, "\n", debugTextContents); }
		}
		if (Utils::getBool("compactGameplay")) {
			debugTextContents = replaceXWithYInZ("\nTaps: ", " | Taps: ", debugTextContents); // Attempt and Taps
			if (xContainedInY("TimeWarp", debugTextContents)) {
				debugTextContents = replaceXWithYInZ("\nGravity: ", " | Gravity: ", debugTextContents); // TimeWarp and Gravity
			}
			if (xContainedInY("Gradients", debugTextContents) || xContainedInY("Active", debugTextContents)) {
				debugTextContents = replaceXWithYInZ("\nParticles: ", " | Particles: ", debugTextContents); // Gradients and Particles
			}
			debugTextContents = replaceXWithYInZ("\nY: ", " | Y: ", debugTextContents); // X and Y position
		}
		if (Utils::getBool("compactAudio") && xContainedInY("Songs", debugTextContents)) {
			debugTextContents = replaceXWithYInZ("\nSFX: ", " | SFX: ", debugTextContents);
		}
		if (Utils::getBool("expandPerformance")) {
			debugTextContents = replaceXWithYInZ("-- Perf --", "-- Performance --", debugTextContents);
		}
		if (Utils::getBool("tapsToClicks")) {
			debugTextContents = replaceXWithYInZ("Taps: ", (m_level->isPlatformer()) ? "Actions: " : "Clicks: ", debugTextContents);
		}
		if (Utils::getBool("fixLevelIDLabel")) {
			debugTextContents = replaceXWithYInZ("LevelID: ", "Level ID: ", debugTextContents);
		}
		if (Utils::getBool("pluralAttempts")) {
			debugTextContents = replaceXWithYInZ("Attempt: ", "Attempts: ", debugTextContents);
		}
		if (Utils::getBool("fps") || Utils::getBool("lastKey")) {
			std::string fps = Utils::getBool("fps") ?
				fmt::format("FPS: {:.0f}", CCDirector::get()->m_fFrameRate) : "";
			std::string lastKey = Utils::getBool("lastKey") ?
				fmt::format("Last Key: {}", m_fields->manager->lastKeyName) : "";
			std::string merger = "";
			if (!fps.empty() && !lastKey.empty())
				merger = Utils::getBool("compactGameplay") ? " | " : "\n";
			debugTextContents = fmt::format("{}{}{}\n{}", fps, merger, lastKey, debugTextContents);
		}
		if (Utils::getBool("gameplayHeader")) {
			debugTextContents = fmt::format("-- Gameplay --\n{}", debugTextContents);
		}
		if (Utils::getBool("cameraProperties")) {
			debugTextContents = debugTextContents.append(fmt::format("{}\n", buildCameraPropertiesString()));
		}
		if (Utils::getBool("geodeInfo")) {
			debugTextContents = debugTextContents.append(fmt::format("{}\n", buildGeodeLoaderString(m_fields->manager)));
		}
		if (Utils::getBool("miscInfo")) {
			debugTextContents = debugTextContents.append(fmt::format("{}\n", buildMiscInfoString()));
		}
		std::string customFooter = Utils::getString("customFooter");
		if (!customFooter.empty()) {
			std::smatch match;
			if (std::regex_search(customFooter, match, asciiOnlyMaxTwentyRegex)) {
				debugTextContents = debugTextContents.append(fmt::format("-- [{}] --", (customFooter.length() > 20) ? customFooter.substr(20) : customFooter));
			}
		}

		// last hurrah
		debugTextContents = replaceXWithYInZ("\n\n", "\n", debugTextContents);
		debugTextNode->setString(debugTextContents.c_str());

		if (Utils::getBool("logDebugText")) {
			log::info("\n--- LOGGED DEBUG TEXT [AFTER INFOLABELTWEAKS] ---\n{}", debugTextContents);
		}
	}
};
