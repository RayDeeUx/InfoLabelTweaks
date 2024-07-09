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
		bool appliedChroma = false;
		bool isLevelComplete = false;
		bool positionSet = false;
		bool textAlignmentSet = false;
		bool scaleSet = false;
		bool opacitySet = false;
		CCNode* debugText = nullptr;
	};
	std::string buildPlayerStatusString(PlayerObject* thePlayer) {
		if (!Utils::modEnabled() || !Utils::getBool("playerStatus")) { return ""; }
		std::string status = "Unknown";
		bool isPlat = m_level->isPlatformer();
		bool compactDirs = Utils::getBool("compactDirections");
		if (thePlayer->m_isShip) {
			if (!isPlat) { status = "Ship"; }
			else { status = "Jetpack"; }
		}
		else if (thePlayer->m_isDart || thePlayer->m_isSwing) {
			if (thePlayer->m_isDart) { status = "Wave"; }
			else { status = "Swing"; }
			if (isPlat) {
				if (m_fields->manager->gameVersion != "2.210") { status = status + "*"; }
				else { status = status + "?"; }
			}
		}
		else if (thePlayer->m_isBall) { status = "Ball"; }
		else if (thePlayer->m_isBird) { status = "UFO"; }
		else if (thePlayer->m_isRobot) { status = "Robot"; }
		else if (thePlayer->m_isSpider) { status = "Spider"; }
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

		if (thePlayer != m_player2) {
			if (m_gameState.m_isDualMode) { status = status + " [Dual]"; }

			if (m_isPracticeMode) { status = status + " {Practice}"; }
			else if (m_isTestMode) { status = status + " {Testmode}"; }
		}

		if (thePlayer->m_isDead) { status = status + " (Dead)"; }

		return fmt::format("{:.1f}x ", thePlayer->m_playerSpeed) + status;
	}
	std::string buildLevelTraitsString() {
		if (!Utils::modEnabled() || !Utils::getBool("levelTraits")) { return ""; }
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

		/*
		if (
			(m_level->m_accountID.value() == 13950148 && m_level->m_levelType == GJLevelType::Saved)
			|| m_fields->gameManager->m_playerUserID.value() == 128138354 && m_level->m_levelType != GJLevelType::Saved
		) { level = level + " <HOMOPHOBIC>"; }
		*/

		if (m_fields->isLevelComplete) { level = level + " <Completed>"; }

		return level;
	}
	std::string buildCameraPropertiesString(GJGameState gameState) {
		if (!Utils::modEnabled() || !Utils::getBool("cameraProperties")) { return ""; }
		bool isCompactCam = Utils::getBool("compactCamera");
		bool currentZoomNotEqualsTarget = gameState.m_cameraZoom != gameState.m_targetCameraZoom;
		bool currentAngleNotEqualsTarget = gameState.m_cameraAngle != gameState.m_targetCameraAngle;
		std::string zoom = fmt::format("Zoom: {:.2f}", gameState.m_cameraZoom);
		if (currentZoomNotEqualsTarget) {
			zoom = zoom + fmt::format(" [{:.2f}]", gameState.m_targetCameraZoom);
		}
		std::string angle = fmt::format("Angle: {:.2f}", gameState.m_cameraAngle);
		if (currentAngleNotEqualsTarget) {
			angle = angle + fmt::format(" [{:.2f}]", gameState.m_targetCameraAngle);
		}
		std::string zoomAngleSeparator =
			(isCompactCam && !currentZoomNotEqualsTarget && !currentAngleNotEqualsTarget) ?
				" | " : "\n";
		std::string zoomAndAngle =
			fmt::format(
				"{}{}{}",
				zoom,
				zoomAngleSeparator,
				angle
			)
		;
		std::string position = !isCompactCam ?
			fmt::format(
				"Position X: {:.2f}\nPosition Y: {:.2f}",
				gameState.m_cameraPosition.x,
				gameState.m_cameraPosition.y
			) : fmt::format(
				"Pos: ({:.2f}, {:.2f})",
				gameState.m_cameraPosition.x,
				gameState.m_cameraPosition.y
			)
		;
		std::string offset = !isCompactCam ?
			fmt::format(
				"Offset X: {:.2f}\nOffset Y: {:.2f}",
				gameState.m_cameraOffset.x,
				gameState.m_cameraOffset.y
			) : fmt::format(
				"Offset: ({:.2f}, {:.2f})",
				gameState.m_cameraOffset.x,
				gameState.m_cameraOffset.y
			)
		;
		std::string edge = fmt::format(
			"Edge: {} / {} / {} / {}",
			gameState.m_cameraEdgeValue0,
			gameState.m_cameraEdgeValue1,
			gameState.m_cameraEdgeValue2,
			gameState.m_cameraEdgeValue3
		);
		std::string shake = gameState.m_cameraShakeEnabled ?
			fmt::format(
				"\nShake: {:.2f}",
				gameState.m_cameraShakeFactor
			) : "";
		return fmt::format(
			"-- Camera --\n{}\n{}\n{}\n{}{}",
			zoomAndAngle,
			position,
			offset,
			edge,
			shake
		);
	}
	std::string buildGeodeLoaderString(Manager* manager) {
		if (!Utils::modEnabled() || !Utils::getBool("geodeInfo")) { return ""; }
		if (Utils::getBool("compactGeode")) {
			return fmt::format(
				"-- Geode v{} --\nGD v{} on {}\nMods: {} + {} = {} ({})",
				manager->geodeVersion,
				manager->gameVersion,
				manager->platform,
				manager->loadedMods,
				manager->disabledMods,
				manager->installedMods,
				manager->problems
			);
		}
		return fmt::format(
			"-- Geode v{} --\nGD v{} on {}\nLoaded: {}\nDisabled: {}\nInstalled: {} ({} problems)",
			manager->geodeVersion,
			manager->gameVersion,
			manager->platform,
			manager->loadedMods,
			manager->disabledMods,
			manager->installedMods,
			manager->problems
		);
	}
	CCNode* findDebugTextNode() {
		CCArrayExt<CCNode*> plArray = CCArrayExt<CCNode*>(getChildren());
		for (int i = plArray.size(); i-- > 0; ) {
			if (const auto nodeCandidate = typeinfo_cast<CCLabelBMFont*>(plArray[i])) {
				std::string nodeCandidateString = nodeCandidate->getString();
				if (
					nodeCandidateString.find("-- Audio --") != std::string::npos &&
					nodeCandidateString.find("-- Perf --") != std::string::npos &&
					nodeCandidateString.find("-- Area --") != std::string::npos
				) {
					return nodeCandidate;
				}
			}
		}
		return nullptr;
	}
	void onQuit() {
		m_fields->isLevelComplete = false;
		m_fields->appliedBlending = false;
		m_fields->appliedChroma = false;
		m_fields->positionSet = false;
		m_fields->textAlignmentSet = false;
		m_fields->scaleSet = false;
		m_fields->opacitySet = false;
		m_fields->debugText = nullptr;
		PlayLayer::onQuit();
	}
	void levelComplete() {
		m_fields->isLevelComplete = true;
		PlayLayer::levelComplete();
	}
	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);
		if (!Utils::modEnabled()) { return; }
		if (m_fields->manager->isMinecraftify) { return; }
		m_fields->debugText = MyPlayLayer::findDebugTextNode();
		if (m_fields->debugText == nullptr) { return; }
		std::string status = MyPlayLayer::buildPlayerStatusString(m_player1);

		std::string ending = "\n-- Area --\n";

		CCLabelBMFont* debugTextNode = typeinfo_cast<CCLabelBMFont*>(m_fields->manager->debugText);
		if (debugTextNode == nullptr || !debugTextNode->isVisible()) { return; }
		if (Utils::getBool("blendingDebugText") && !m_fields->appliedBlending) {
			debugTextNode->setBlendFunc({GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA}); // Manager::glBlendFuncs[5], Manager::glBlendFuncs[7]
			m_fields->appliedBlending = true;
		} else {
			m_fields->appliedBlending = false;
		}
		if (!Utils::getBool("chromaDebugText")) {
			debugTextNode->setColor({255, 255, 255}); // ensure that node color is white in case someone turns off chroma mode mid-session
			m_fields->appliedChroma = false;
		} else if (!m_fields->appliedChroma) {
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
			m_fields->appliedChroma = true;
		}
		if (!m_fields->opacitySet) {
			if (Utils::getBool("maxAlphaDebugText")) {
				debugTextNode->setOpacity(255);
			} else {
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
			} else {
				debugTextNode->setScale(Utils::getDouble("customScale"));
			}
			m_fields->scaleSet = true;
		}
		if (!m_fields->positionSet && Utils::getBool("positionAlignRight") || Utils::getBool("positionAlignBottom")) {
			float anchorPointX = Utils::getBool("positionAlignRight") ? 1.f : 0.f;
			float anchorPointY = Utils::getBool("positionAlignBottom") ? 1.f : 0.f;
			debugTextNode->setAnchorPoint({anchorPointX, anchorPointY});
			auto windowSize = CCDirector::get()->getWinSize();
			if (Utils::getBool("positionAlignRight")) {
				debugTextNode->setPositionX(windowSize.width * .95f);
			}
			if (Utils::getBool("positionAlignBottom")) {
				debugTextNode->setPositionY(5);
			}
			m_fields->positionSet = true;
		}
		std::string debugTextContents = debugTextNode->getString();
		if (Utils::getBool("logDebugText")) {
			log::info("--- LOGGED DEBUG TEXT [BEFORE INFOLABELTWEAKS] ---:\n{}", debugTextContents);
		}
		if (Utils::getBool("currentChannel")) {
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\n-- Audio --"), fmt::format("\nChannel: {}\n\r-- Audio --", m_gameState.m_currentChannel));
		}
		#ifndef __APPLE__
		if (Utils::getBool("lastPlayedSong")) {
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\n(\r)?-- Audio --\nSongs: "), fmt::format("\n-- Audio --\nLast Song: {}\nLast SFX: {}\nSongs: ", m_fields->manager->lastPlayedSong, m_fields->manager->lastPlayedEffect));
		}
		#endif
		if (Utils::getBool("jumps")) {
			std::smatch match;
			if (std::regex_search(debugTextContents, match, tapsCountRegex)) {
				debugTextContents = std::regex_replace(
					debugTextContents,
					std::regex("Taps: \\d+\nTimeWarp: "),
					fmt::format(
						"Taps: {} [{}]\nTimeWarp: ",
						match[1].str(),
						m_jumps
					)
				); // jump count from playlayer members: m_jump
			}
		}
		if (Utils::getBool("jumps")) {
			std::smatch match;
			if (std::regex_search(debugTextContents, match, timeLabelRegex)) {
				debugTextContents = std::regex_replace(
					debugTextContents,
					std::regex("Time: \\d+(\\.\\d+)?\nAttempt: "),
					fmt::format(
						"Time: {} [{}]\nAttempt: ",
						match[1].str(),
						fmt::format("{:.2f}", m_gameState.m_levelTime)
					)
				); // attempt time from playlayer gamestate member: m_gameState.m_levelTime
			}
		}
		if (Utils::getBool("accuratePosition")) {
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nX: (\\d)+\n"), fmt::format("\nX: {:.4f}\n", m_player1->m_position.x));
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nY: (\\d)+\n"), fmt::format("\nY: {:.4f}\n", m_player1->m_position.y));
		}
		if (Utils::getBool("conditionalValues")) {
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nTimeWarp: 1\n"), "\n");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nGravity: 1\n"), "\n");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nGradients: 0"), "");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nParticles: 0"), "");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nChannel: 0"), "");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nMove: 0\n"), "\n");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nSongs: 0\n"), "\n");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nSFX: 0\n"), "\n");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nRotate: 0\n"), "\n");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nScale: 0\n"), "\n");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nFollow: 0\n"), "\n");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\n-- Perf --\n--"), "\n--");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nMove: 0 / 0"), "");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nRotate: 0 / 0"), "");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nScale: 0 / 0"), "");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nFollow: 0 / 0"), "");
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nColOp: 0 / 0"), "");
			if (debugTextContents.compare(debugTextContents.size() - ending.size(), ending.size(), ending) == 0) {
				debugTextContents = std::regex_replace(debugTextContents, std::regex(ending), "\n");
			}
		}
		if (Utils::getBool("playerStatus")) {
			std::string formattedPlayerStatus = (m_gameState.m_isDualMode) ?
				fmt::format(
					"\nP1 Status: {}\nP2 Status: {}\n-- Audio --",
					MyPlayLayer::buildPlayerStatusString(m_player1),
					MyPlayLayer::buildPlayerStatusString(m_player2)
				) :
				fmt::format(
					"\nStatus: {}\n-- Audio --",
					MyPlayLayer::buildPlayerStatusString(m_player1)
				);
			debugTextContents = std::regex_replace(
				debugTextContents,
				std::regex("\n-- Audio --"),
				formattedPlayerStatus
			);
		}
		if (Utils::getBool("levelTraits")) {
			debugTextContents = std::regex_replace(
				debugTextContents,
				std::regex("\n-- Audio --"),
				fmt::format(
					"\nLevel: {}\n-- Audio --",
					MyPlayLayer::buildLevelTraitsString()
				)
			);
		}
		if (Utils::getBool("compactGameplay")) {
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nTaps: "), " | Taps: "); // Attempt and Taps
			if (debugTextContents.find("TimeWarp: ") != std::string::npos) {
				debugTextContents = std::regex_replace(debugTextContents, std::regex("\nGravity: "), " | Gravity: "); // TimeWarp and Gravity
			}
			if (debugTextContents.find("Gradients: ") != std::string::npos) {
				debugTextContents = std::regex_replace(debugTextContents, std::regex("\nParticles: "), " | Particles: "); // Gradients and Particles
			}
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nY: "), " | Y: "); // X and Y position
		}
		if (Utils::getBool("compactAudio") && debugTextContents.find("Songs: ") != std::string::npos) {
			debugTextContents = std::regex_replace(debugTextContents, std::regex("\nSFX: "), " | SFX: ");
		}
		if (Utils::getBool("expandPerformance")) {
			debugTextContents = std::regex_replace(debugTextContents, std::regex("-- Perf --"), "-- Performance --");
		}
		if (Utils::getBool("tapsToClicks")) {
			std::string actionsOrClicks = (m_level->isPlatformer()) ? "Actions: " : "Clicks: ";
			debugTextContents = std::regex_replace(debugTextContents, std::regex("Taps: "), actionsOrClicks);
		}
		if (Utils::getBool("fixLevelIDLabel")) {
			debugTextContents = std::regex_replace(debugTextContents, std::regex("LevelID: "), "Level ID: ");
		}
		if (Utils::getBool("pluralAttempts")) {
			debugTextContents = std::regex_replace(debugTextContents, std::regex("Attempt: "), "Attempts: ");
		}
		if (Utils::getBool("fps")) {
			debugTextContents = fmt::format("FPS: {}\n", m_fields->manager->fps) + debugTextContents;
		}
		if (Utils::getBool("gameplayHeader")) {
			debugTextContents = std::string("-- Gameplay --\n") + debugTextContents;
		}
		if (Utils::getBool("cameraProperties")) {
			debugTextContents = debugTextContents +
			MyPlayLayer::buildCameraPropertiesString(m_gameState) + "\n";
		}
		if (Utils::getBool("geodeInfo")) {
			debugTextContents = debugTextContents +
			MyPlayLayer::buildGeodeLoaderString(m_fields->manager) + "\n";
		}
		std::string customFooter = Utils::getString("customFooter");
		if (!customFooter.empty()) {
			std::smatch match;
			if (std::regex_search(customFooter, match, asciiOnlyMaxTwentyRegex)) {
				debugTextContents = debugTextContents + fmt::format("-- [{}] --", customFooter.substr(20));
			}
		}
		if (Utils::getBool("logDebugText")) {
			log::info("--- LOGGED DEBUG TEXT [AFTER INFOLABELTWEAKS] ---:\n{}", debugTextContents);
		}
		debugTextNode->setString(debugTextContents.c_str());
	}
};