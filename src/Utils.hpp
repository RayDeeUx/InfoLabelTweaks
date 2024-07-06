#pragma once

#include <Geode/Bindings.hpp>
#include "Manager.hpp"

using namespace geode::prelude;
using namespace geode::cocos;

namespace Utils {
	template<class T> T getSetting(std::string setting, T type);
	bool getBool(std::string setting);
	int64_t getInt(std::string setting);
	double getDouble(std::string setting);
	std::string getString(std::string setting);
	ccColor3B getColor(std::string setting);
	ccColor4B getColorAlpha(std::string setting);
	bool modEnabled();
	
	bool isModLoaded(std::string modID);
}