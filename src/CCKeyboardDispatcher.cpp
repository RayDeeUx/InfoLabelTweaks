#ifdef GEODE_IS_DESKTOP
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include "Manager.hpp"

using namespace geode::prelude;

class $modify(MyCCKeyboardDispatcher, CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool p1 , bool p2, double timestamp) {
		bool result = CCKeyboardDispatcher::dispatchKeyboardMSG(key, p1, p2, timestamp);
		if (PlayLayer::get()) {
			auto name = CCKeyboardDispatcher::keyToString(key);
			if (!name) name = "Unknown";
			Manager::getSharedInstance()->lastKeyName = name;
		}
		else Manager::getSharedInstance()->lastKeyName = "N/A";
		return result;
	}
};
#endif