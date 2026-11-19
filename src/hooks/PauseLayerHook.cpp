#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/ButtonSprite.hpp>

#include "../FPCState.hpp"
#include "../Macro.hpp"

using namespace geode::prelude;

class $modify(FPCPauseLayer, PauseLayer) {

    static CCMenuItemSpriteExtra* fpcMakeButton(
        PauseLayer* self, const char* text, SEL_MenuHandler callback
    ) {
        auto sprite = ButtonSprite::create(text, "bigFont.fnt", "GJ_button_05.png", 0.8f);
        sprite->setScale(0.6f);
        return CCMenuItemSpriteExtra::create(sprite, self, callback);
    }

    void customSetup() {
        PauseLayer::customSetup();

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto menu = CCMenu::create();
        menu->setID("frame-perfect-counter-menu"_spr);
        this->addChild(menu, 100);

        auto recordBtn = fpcMakeButton(this, "Record", menu_selector(FPCPauseLayer::onFpcRecord));
        auto playBtn = fpcMakeButton(this, "Play", menu_selector(FPCPauseLayer::onFpcPlay));
        auto analyzeBtn = fpcMakeButton(this, "Analyze", menu_selector(FPCPauseLayer::onFpcAnalyze));
        auto cancelBtn = fpcMakeButton(this, "FPC Off", menu_selector(FPCPauseLayer::onFpcCancel));

        recordBtn->setPosition({winSize.width / 2 - 180.f, 40.f});
        playBtn->setPosition({winSize.width / 2 - 60.f, 40.f});
        analyzeBtn->setPosition({winSize.width / 2 + 60.f, 40.f});
        cancelBtn->setPosition({winSize.width / 2 + 180.f, 40.f});

        menu->addChild(recordBtn);
        menu->addChild(playBtn);
        menu->addChild(analyzeBtn);
        menu->addChild(cancelBtn);
    }

    void onFpcRecord(CCObject*) {
        auto pl = PlayLayer::get();
        if (!pl) return;

        auto& state = FPCState::get();
        state.mode = FPCMode::Recording;
        state.macro.clear();

        this->onResume(nullptr);
        pl->resetLevel();
    }

    void onFpcPlay(CCObject*) {
        auto pl = PlayLayer::get();
        if (!pl) return;

        auto loaded = Macro::loadFromFile(Macro::defaultPath());
        if (!loaded) {
            Notification::create(
                "No saved macro to play: " + loaded.unwrapErr(),
                NotificationIcon::Error
            )->show();
            return;
        }

        auto& state = FPCState::get();
        state.macro = loaded.unwrap();
        state.mode = FPCMode::Playing;

        this->onResume(nullptr);
        pl->resetLevel();
    }

    void onFpcAnalyze(CCObject*) {
        auto pl = PlayLayer::get();
        if (!pl) return;

        auto loaded = Macro::loadFromFile(Macro::defaultPath());
        if (!loaded || loaded.unwrap().empty()) {
            Notification::create(
                "No saved macro to analyze - record one first.",
                NotificationIcon::Error
            )->show();
            return;
        }

        auto& state = FPCState::get();
        state.macro = loaded.unwrap();
        state.mode = FPCMode::Analyzing;

        auto maxShift = static_cast<int>(Mod::get()->getSettingValue<int64_t>("max-shift-frames"));
        auto horizon = static_cast<int>(Mod::get()->getSettingValue<int64_t>("test-horizon-frames"));
        auto speed = static_cast<float>(Mod::get()->getSettingValue<double>("test-speed-multiplier"));

        state.analyzer.reset(maxShift, horizon);
        CCDirector::sharedDirector()->getScheduler()->setTimeScale(speed);

        Notification::create(
            fmt::format("Analyzing {} input(s)...", state.macro.inputs.size()),
            NotificationIcon::Info
        )->show();

        this->onResume(nullptr);
        pl->resetLevel();
    }

    void onFpcCancel(CCObject*) {
        auto& state = FPCState::get();
        state.mode = FPCMode::Idle;
        CCDirector::sharedDirector()->getScheduler()->setTimeScale(1.f);

        Notification::create("Frame Perfect Counter turned off", NotificationIcon::Info)->show();
    }
};
