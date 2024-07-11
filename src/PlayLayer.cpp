#include <Geode/modify/PlayLayer.hpp>
#include <regex>
#include "Utils.hpp"
#include "Manager.hpp"

using namespace geode::prelude;

static std::regex tapsCountRegex(R"(Taps: (\d+)\n)", std::regex::optimize | std::regex::icase);
static std::regex timeLabelRegex(R"(Time: (\d+(\.\d+)?))", std::regex::optimize | std::regex::icase);
static std::regex asciiOnlyMaxTwentyRegex(R"([\x00-\x7F]{0,20})", std::regex::optimize | std::regex::icase);

class $modify(MyPlayLayer, PlayLayer) {
	struct Fields {
		Manager* manager = Manager::getSharedInstance();
		bool appliedBlending = false;
		bool appliedChromaOrDefaultColor = false;
		bool isLevelComplete = false;
		bool positionSet = false;
		bool textAlignmentSet = false;
		bool scaleSet = false;
		bool opacitySet = false;
		std::string_view debugTextContentsManager = "";
		CCNode* debugText = nullptr;

		std::string endsWithArea = "\n-- Area --\n";
	};
	std::string buildPlayerStatusString(PlayerObject* thePlayer) {
		if (!Utils::modEnabled() || !Utils::getBool("playerStatus") || (thePlayer != m_player1 && thePlayer != m_player2)) { return ""; }
		std::string status = "Unknown";

		bool isPlat = m_level->isPlatformer();
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
				if (m_fields->manager->gameVersion != "2.210") { status = status + "*"; }
				else { status = status + "?"; }
			}
		}
		else { status = "Cube"; }

		if (thePlayer->m_vehicleSize == .6f) { status = "Mini " + status; }
		else if (thePlayer->m_vehicleSize != 1.f) { status = status + " of strange size"; }

		if (thePlayer->m_isPlatformer) {
			if (thePlayer->m_isUpsideDown) {
				if (thePlayer->m_isSideways) {
					if (compactDirs) { status = "->] " + status; }
					else { status = "Rightwards " + status; }
				}
				else { status = "Flipped " + status; }
			} else if (thePlayer->m_isSideways) {
				if (compactDirs) { status = "[<- " + status; }
				else { status = "Leftwards " + status; }
			}
		} else {
			if (thePlayer->m_isUpsideDown) { status = "Flipped " + status; }
			if (thePlayer->m_isSideways) {
				if (thePlayer->m_isGoingLeft) {
					if (compactDirs) { status = "\\/ " + status; }
					else { status = "Downwards " + status; }
				}
				else {
					if (compactDirs) { status = "/\\ " + status; }
					else { status = "Upwards " + status; }
				}
			} else if (thePlayer->m_isGoingLeft) {
				if (compactDirs) { status = "<- " + status; }
				else { status = "Reversed " + status; }
			}
		}

		if (thePlayer->m_isDashing) { status = "<" + status + ">"; }

		if (thePlayer->m_isHidden) { status = "Hidden " + status; }

		if (thePlayer != m_player2) {
			if (m_gameState.m_isDualMode) { status = status + " [Dual]"; }

			if (m_isPracticeMode) { status = status + " {Practice}"; }
			else if (m_isTestMode) { status = status + " {Testmode}"; }
		}

		if (thePlayer->m_isDead) { status = status + " (Dead)"; }

		return fmt::format("{:.1f}x ", thePlayer->m_playerSpeed) + status;
	}
	std::string buildLevelTraitsString() {
		if (!Utils::modEnabled() || !Utils::getBool("levelTraits") || m_level == nullptr) { return ""; }
		std::string level = "Unknown";
		if (m_level->isPlatformer()) {
			level = "Platformer";
		} else {
			level = "Classic";
			if (m_level->m_levelLength == 0.f) { level = level + " [Tiny]"; }
			else if (m_level->m_levelLength == 1.f) { level = level + " [Short]"; }
			else if (m_level->m_levelLength == 2.f) { level = level + " [Medium]"; }
			else if (m_level->m_levelLength == 3.f) { level = level + " [Long]"; }
			else { level = level + " [XL]"; }
		}

		if (m_level->m_levelType == GJLevelType::Editor) {
			if (m_level->m_isVerifiedRaw) { level = level + " (Verified)";}
			else { level = level + " (Unverified)"; }
		} else {
			if (m_level->m_levelType == GJLevelType::Saved) { level = level + " (Online)"; }
			else if (m_level->m_levelType == GJLevelType::Local) { level = level + " (Official)"; }
			else { level = level + " (Unknown)"; }
		}

		if (m_level->m_twoPlayerMode) { level = level + " {2P}"; }

		if (m_fields->isLevelComplete) { level = level + " <Completed>"; }

		return level;
	}
	std::string buildCameraPropertiesString() {
		if (!Utils::modEnabled() || !Utils::getBool("cameraProperties")) { return ""; }
		// NEVER STORE A VARIABLE OF TYPE GJGameState, IT WILL FAIL ON ANDROID
		float camZoom = m_gameState.m_cameraZoom;
		float camTargetZoom = m_gameState.m_targetCameraZoom;
		float camAngle = m_gameState.m_cameraAngle;
		float camTargetAngle = m_gameState.m_cameraAngle;
		bool isCompactCam = Utils::getBool("compactCamera");
		bool currentZoomNotEqualsTarget = camZoom != camTargetZoom;
		bool currentAngleNotEqualsTarget = camAngle != camTargetAngle;

		std::string zoom = fmt::format("Zoom: {:.2f}", camZoom);
		if (currentZoomNotEqualsTarget) { zoom = zoom + fmt::format(" [{:.2f}]", camTargetZoom); }
		std::string angle = fmt::format("Angle: {:.2f}", camAngle);
		if (currentAngleNotEqualsTarget) { angle = angle + fmt::format(" [{:.2f}]", camTargetAngle); }
		std::string zoomAngleSeparator = (isCompactCam && !currentZoomNotEqualsTarget && !currentAngleNotEqualsTarget) ?
			" | " : "\n";
		std::string zoomAndAngle =
			fmt::format("{}{}{}", zoom, zoomAngleSeparator, angle);
		CCPoint camPosition = m_gameState.m_cameraPosition;
		std::string position = !isCompactCam ?
			fmt::format("Position X: {:.2f}\nPosition Y: {:.2f}", camPosition.x, camPosition.y) :
			fmt::format("Pos: ({:.2f}, {:.2f})", camPosition.x, camPosition.y)
		;
		CCPoint camOffset = m_gameState.m_cameraOffset;
		std::string offset = !isCompactCam ?
			fmt::format("Offset X: {:.2f}\nOffset Y: {:.2f}", camOffset.x, camOffset.y) :
			fmt::format("Offset: ({:.2f}, {:.2f})", camOffset.x, camOffset.y);
		std::string edge = fmt::format(
			"Edge: {} / {} / {} / {}",
			m_gameState.m_cameraEdgeValue0, m_gameState.m_cameraEdgeValue1, m_gameState.m_cameraEdgeValue2, m_gameState.m_cameraEdgeValue3
		);
		std::string shake = m_gameState.m_cameraShakeEnabled ?
			fmt::format("\nShake: {:.2f}", m_gameState.m_cameraShakeFactor) : "";
		return fmt::format(
			"-- Camera --\n{}\n{}\n{}\n{}{}",
			zoomAndAngle, position, offset, edge, shake
		);
	}
	static std::string buildGeodeLoaderString(Manager* manager) {
		if (!Utils::modEnabled() || !Utils::getBool("geodeInfo")) { return ""; }
		if (Utils::getBool("compactGeode")) {
			return fmt::format(
				"-- Geode v{} --\nGD v{} on {}\nMods: {} + {} = {} ({})",
				manager->geodeVersion, manager->gameVersion, manager->platform, manager->loadedMods, manager->disabledMods, manager->installedMods, manager->problems
			);
		}
		return fmt::format(
			"-- Geode v{} --\nGD v{} on {}\nLoaded: {}\nDisabled: {}\nInstalled: {} ({} problems)",
			manager->geodeVersion, manager->gameVersion, manager->platform, manager->loadedMods, manager->disabledMods, manager->installedMods, manager->problems
		);
	}
	static bool xContainedInY(std::string substring, std::string_view string, bool withColon = true) {
		if (!Utils::modEnabled() || string.empty()) { return false; }
		if (withColon) return (string.find(fmt::format("{}: ", substring)) != std::string::npos);
		return (string.find(substring) != std::string::npos);
	}
	static bool isInfoLabel(std::string candidateString) {
		return (MyPlayLayer::xContainedInY("-- Audio --", candidateString, false) &&
		       MyPlayLayer::xContainedInY("-- Perf --", candidateString, false) &&
		       MyPlayLayer::xContainedInY("-- Area --", candidateString, false));
	}
	CCNode* findDebugTextNode() {
		if (m_infoLabel != nullptr && MyPlayLayer::isInfoLabel(m_infoLabel->getString())) { return m_infoLabel; }
		if (CCLabelBMFont* infoLabelCandidate = typeinfo_cast<CCLabelBMFont*>(getChildByID("info-label"))) {
			if (MyPlayLayer::isInfoLabel(infoLabelCandidate->getString())) { return infoLabelCandidate; }
		}
		CCArrayExt<CCNode*> plArray = CCArrayExt<CCNode*>(getChildren());
		for (int i = plArray.size(); i-- > 0; ) {
			if (CCLabelBMFont* nodeCandidate = typeinfo_cast<CCLabelBMFont*>(plArray[i])) {
				if (MyPlayLayer::isInfoLabel(nodeCandidate->getString())) { return nodeCandidate; }
			}
		}
		return nullptr;
	}
	void onQuit() {
		m_fields->isLevelComplete = false;
		m_fields->appliedBlending = false;
		m_fields->appliedChromaOrDefaultColor = false;
		m_fields->positionSet = false;
		m_fields->textAlignmentSet = false;
		m_fields->scaleSet = false;
		m_fields->opacitySet = false;
		m_fields->debugText = nullptr;
		m_fields->debugTextContentsManager = "";
		PlayLayer::onQuit();
	}
	void levelComplete() {
		m_fields->isLevelComplete = true;
		PlayLayer::levelComplete();
	}
	static std::string replaceXWithYInZ(std::string forRegex, std::string replacement, std::string mainString) {
		return std::regex_replace(mainString, std::regex(forRegex), replacement);
	}
	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);
		if (!Utils::modEnabled() || m_fields->manager->isMinecraftify) { return; }

		m_fields->debugText = MyPlayLayer::findDebugTextNode();
		if (m_fields->debugText == nullptr || !m_fields->debugText->isVisible()) { return; }

		CCLabelBMFont* debugTextNode = typeinfo_cast<CCLabelBMFont*>(m_fields->debugText);
		if (debugTextNode == nullptr || !debugTextNode->isVisible()) { return; }

		if (Utils::getBool("blendingDebugText") && !m_fields->appliedBlending) {
			debugTextNode->setBlendFunc({GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA}); // Manager::glBlendFuncs[5], Manager::glBlendFuncs[7]
			m_fields->appliedBlending = true;
		} else { m_fields->appliedBlending = false; }
		if (!m_fields->appliedChromaOrDefaultColor) {
			if (!Utils::getBool("chromaDebugText")) {
				debugTextNode->setColor({255, 255, 255}); // ensure that node color is white in case someone turns off chroma mode mid-session
			} else {
				const auto chromaSpeed = Utils::getDouble("chromaDebugTextSpeed");
				const auto tintOne = CCTintTo::create(chromaSpeed, 255, 128, 128);
				const auto tintTwo = CCTintTo::create(chromaSpeed, 255, 255, 128);
				const auto tintThree = CCTintTo::create(chromaSpeed, 128, 255, 128);
				const auto tintFour = CCTintTo::create(chromaSpeed, 128, 255, 255);
				const auto tintFive = CCTintTo::create(chromaSpeed, 128, 128, 255);
				const auto tintSix = CCTintTo::create(chromaSpeed, 255, 128, 128);
				const auto sequence = CCSequence::create(tintOne, tintTwo, tintThree, tintFour, tintFive, tintSix, nullptr);
				const auto repeat = CCRepeatForever::create(sequence);
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
		if (!m_fields->positionSet && (Utils::getBool("positionAlignRight") || Utils::getBool("positionAlignBottom"))) {
			debugTextNode->ignoreAnchorPointForPosition(false);
			debugTextNode->setAnchorPoint({
				Utils::getBool("positionAlignRight") ? 1.f : 0.f, // positionAlignRight: anchorPointX
				Utils::getBool("positionAlignBottom") ? 1.f : 0.f // positionAlignBottom: anchorPointY
			});
			if (Utils::getBool("positionAlignRight")) { debugTextNode->setPositionX(CCDirector::get()->getWinSize().width * .95f); }
			if (Utils::getBool("positionAlignBottom")) { debugTextNode->setPositionY(5); }
			m_fields->positionSet = true;
		}

		std::string debugTextContents = debugTextNode->getString();
		m_fields->debugTextContentsManager = debugTextNode->getString();

		if (Utils::getBool("logDebugText")) {
			log::info("--- LOGGED DEBUG TEXT [BEFORE INFOLABELTWEAKS] ---:\n{}", debugTextContents);
		}
		if (Utils::getBool("currentChannel")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ(
				"\n-- Audio --",
				fmt::format("\nChannel: {}\n\r-- Audio --", m_gameState.m_currentChannel),
				debugTextContents
			);
		}
		#ifndef __APPLE__
		if (Utils::getBool("lastPlayedAudio")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ(
				"\n(\r)?-- Audio --\nSongs: ",
				fmt::format("\n-- Audio --\nLast Song: {}\nLast SFX: {}\nSongs: ", m_fields->manager->lastPlayedSong, m_fields->manager->lastPlayedEffect),
				debugTextContents
			);
		}
		#endif
		if (Utils::getBool("jumps")) {
			std::smatch match;
			if (std::regex_search(debugTextContents, match, tapsCountRegex)) {
				debugTextContents = MyPlayLayer::replaceXWithYInZ(
					"Taps: \\d+\nTimeWarp: ",
					fmt::format("Taps: {} [{}]\nTimeWarp: ", match[1].str(), m_jumps),
					debugTextContents
				); // jump count from playlayer members: m_jump
			}
		}
		if (Utils::getBool("attemptTime")) {
			std::smatch match;
			if (std::regex_search(debugTextContents, match, timeLabelRegex)) {
				debugTextContents = MyPlayLayer::replaceXWithYInZ(
					"Time: \\d+(\\.\\d+)?\nAttempt: ",
					fmt::format("Time: {} [{}]\nAttempt: ", match[1].str(), fmt::format("{:.2f}", m_gameState.m_levelTime)),
					debugTextContents
				); // attempt time from playlayer gamestate member: m_gameState.m_levelTime
			}
		}
		if (Utils::getBool("accuratePosition")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ("\nX: (\\d)+\n", fmt::format("\nX: {:.4f}\n", m_player1->m_position.x), debugTextContents);
			debugTextContents = MyPlayLayer::replaceXWithYInZ("\nY: (\\d)+\n", fmt::format("\nY: {:.4f}\n", m_player1->m_position.y), debugTextContents);
		}
		if (Utils::getBool("conditionalValues")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ("\n(TimeWarp|Gravity): 1\n", "\n", debugTextContents);
			debugTextContents = MyPlayLayer::replaceXWithYInZ("\n(Gradients|Particles|Channel): 0", "", debugTextContents);
			debugTextContents = MyPlayLayer::replaceXWithYInZ("\n(Move|Songs|SFX|Rotate|Scale|Follow): 0\n", "\n", debugTextContents);
			debugTextContents = MyPlayLayer::replaceXWithYInZ("\n-- Perf --\n--", "\n--", debugTextContents);
			debugTextContents = MyPlayLayer::replaceXWithYInZ("\n(Move|Rotate|Scale|Follow|ColOp): 0 / 0", "", debugTextContents);
			if (debugTextContents.ends_with(m_fields->endsWithArea)) { debugTextContents = MyPlayLayer::replaceXWithYInZ(m_fields->endsWithArea, "\n", debugTextContents); }
		}
		if (Utils::getBool("playerStatus")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ(
				"\n-- Audio --",
				(m_gameState.m_isDualMode) ? fmt::format(
					"\nP1 Status: {}\nP2 Status: {}\n-- Audio --",
					MyPlayLayer::buildPlayerStatusString(m_player1),
					MyPlayLayer::buildPlayerStatusString(m_player2)
				) : fmt::format(
					"\nStatus: {}\n-- Audio --",
					MyPlayLayer::buildPlayerStatusString(m_player1)
				),
				debugTextContents
			);
		}
		if (Utils::getBool("levelTraits")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ(
				"\n-- Audio --",
				fmt::format("\nLevel: {}\n-- Audio --", MyPlayLayer::buildLevelTraitsString()),
				debugTextContents
			);
		}
		if (Utils::getBool("compactGameplay")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ("\nTaps: ", " | Taps: ", debugTextContents); // Attempt and Taps
			if (MyPlayLayer::xContainedInY("TimeWarp", m_fields->debugTextContentsManager)) {
				debugTextContents = MyPlayLayer::replaceXWithYInZ("\nGravity: ", " | Gravity: ", debugTextContents); // TimeWarp and Gravity
			}
			if (MyPlayLayer::xContainedInY("Gradients", m_fields->debugTextContentsManager)) {
				debugTextContents = MyPlayLayer::replaceXWithYInZ("\nParticles: ", " | Particles: ", debugTextContents); // Gradients and Particles
			}
			debugTextContents = MyPlayLayer::replaceXWithYInZ("\nY: ", " | Y: ", debugTextContents); // X and Y position
		}
		if (Utils::getBool("compactAudio") && MyPlayLayer::xContainedInY("Songs", m_fields->debugTextContentsManager)) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ("\nSFX: ", " | SFX: ", debugTextContents);
		}
		if (Utils::getBool("expandPerformance")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ("-- Perf --", "-- Performance --", debugTextContents);
		}
		if (Utils::getBool("tapsToClicks")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ("Taps: ", (m_level->isPlatformer()) ? "Actions: " : "Clicks: ", debugTextContents);
		}
		if (Utils::getBool("fixLevelIDLabel")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ("LevelID: ", "Level ID: ", debugTextContents);
		}
		if (Utils::getBool("pluralAttempts")) {
			debugTextContents = MyPlayLayer::replaceXWithYInZ("Attempt: ", "Attempts: ", debugTextContents);
		}
		if (Utils::getBool("fps")) {
			debugTextContents = fmt::format("FPS: {}\n{}", m_fields->manager->fps, debugTextContents);
		}
		if (Utils::getBool("gameplayHeader")) {
			debugTextContents = fmt::format("-- Gameplay --\n{}", debugTextContents);
		}
		if (Utils::getBool("cameraProperties")) {
			debugTextContents = debugTextContents.append(fmt::format("{}\n", MyPlayLayer::buildCameraPropertiesString()));
		}
		if (Utils::getBool("geodeInfo")) {
			debugTextContents = debugTextContents.append(fmt::format("{}\n", MyPlayLayer::buildGeodeLoaderString(m_fields->manager)));
		}
		std::string customFooter = Utils::getString("customFooter");
		if (!customFooter.empty()) {
			std::smatch match;
			if (std::regex_search(customFooter, match, asciiOnlyMaxTwentyRegex)) {
				debugTextContents = debugTextContents.append(fmt::format("-- [{}] --", customFooter.substr(20)));
			}
		}
		if (Utils::getBool("logDebugText")) {
			log::info("--- LOGGED DEBUG TEXT [AFTER INFOLABELTWEAKS] ---:\n{}", debugTextContents);
		}
		debugTextNode->setString(debugTextContents.c_str());
	}
};