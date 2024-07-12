#include <Geode/modify/MenuLayer.hpp>
#include <regex>
#include "Manager.hpp"
#include "Utils.hpp"

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {
	struct Fields {
		Manager* manager = Manager::getSharedInstance();
	};
	bool init() {
		if (!MenuLayer::init()) { return false; }
		if (m_fields->manager->hasCalledAlready) { return true; }
		m_fields->manager->hasCalledAlready = true;

		const auto geode = Loader::get();
		auto mods = geode->getAllMods();

		auto loadedMods = 0;
		auto disabledMods = 0;
		auto problems = geode->getProblems().size();

		std::ranges::for_each(mods, [&](const Mod *mod) {
			if (geode->isModLoaded(mod->getID())) {
				loadedMods++;
			} else {
				disabledMods++;
			}
		});

		m_fields->manager->platform = GEODE_PLATFORM_NAME;
		m_fields->manager->installedMods = mods.size();
		m_fields->manager->loadedMods = loadedMods;
		m_fields->manager->disabledMods = disabledMods;
		m_fields->manager->problems = problems;
		m_fields->manager->geodeVersion = geode->getVersion().toNonVString();
		m_fields->manager->gameVersion = geode->getGameVersion();
		m_fields->manager->isMinecraftify = Utils::isModLoaded("zalphalaneous.minecraft");

		return true;
	}
};