#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include "Manager.hpp"

class $modify(MyCCKeyboardDispatcher, CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool p1 , bool p2) {
		bool result = CCKeyboardDispatcher::dispatchKeyboardMSG(key, p1, p2);
		if (PlayLayer::get()) { Manager::getSharedInstance()->lastKeyName = CCKeyboardDispatcher::keyToString(key); }
		else { Manager::getSharedInstance()->lastKeyName = "N/A"; }
		return result;
	}
};