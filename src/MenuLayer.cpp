#include <Geode/modify/MenuLayer.hpp>
#include "Manager.hpp"
#include "Utils.hpp"

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) { return false; }
		auto manager = Manager::getSharedInstance();
		if (manager->hasCalledAlready) { return true; }
		manager->hasCalledAlready = true;

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

		manager->platform = GEODE_PLATFORM_NAME;
		manager->installedMods = mods.size();
		manager->loadedMods = loadedMods;
		manager->disabledMods = disabledMods;
		manager->problems = problems;
		manager->geodeVersion = geode->getVersion().toNonVString();
		manager->gameVersion = geode->getGameVersion();
		manager->isMinecraftify = Utils::isModLoaded("zalphalaneous.minecraft");

		return true;
	}
};