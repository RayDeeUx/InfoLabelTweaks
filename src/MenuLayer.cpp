#include <Geode/modify/MenuLayer.hpp>
#include "PlayLayer.hpp"
#include "Manager.hpp"
#include "Utils.hpp"

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) { return false; }
		auto manager = Manager::getSharedInstance();
		if (manager->hasCalledAlready) { return true; }
		manager->hasCalledAlready = true;
		manager->originalTimestamp = std::time(nullptr);

		const auto geode = Loader::get();
		auto mods = geode->getAllMods();

		auto loadedMods = 0;
		auto disabledMods = 0;
		int problems = 0;

		std::ranges::for_each(mods, [&](const Mod *mod) {
			if (mod->isLoaded()) {
				loadedMods++;
			} else {
				disabledMods++;
			}
			if (mod->getLoadProblem()) problems += 1;
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

$on_mod(Loaded) {
	listenForAllSettingChanges([](std::shared_ptr<SettingV3> setting){
		if (const auto pl = PlayLayer::get()) static_cast<MyPlayLayer*>(pl)->setupSettings();
	});
}