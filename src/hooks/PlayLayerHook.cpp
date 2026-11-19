#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/binding/GJGameLevel.hpp>

#include "../Macro.hpp"
#include "../FPCState.hpp"
#include "../ResultsPopup.hpp"

using namespace geode::prelude;

// ---------------------------------------------------------------------
// Records every queued input while FPCState::mode == Recording.
// queueButton is declared on GJBaseGameLayer (PlayLayer's base), so we
// hook it there and filter down to PlayLayer instances only -
// LevelEditorLayer (the other GJBaseGameLayer subclass) is never touched.
// ---------------------------------------------------------------------
class $modify(FPCBaseGameLayer, GJBaseGameLayer) {
    void queueButton(int button, bool push, bool isPlayer2, double timestamp) {
        GJBaseGameLayer::queueButton(button, push, isPlayer2, timestamp);

        auto pl = typeinfo_cast<PlayLayer*>(this);
        if (!pl) return;

        auto& state = FPCState::get();
        if (state.mode != FPCMode::Recording) return;

        RecordedInput input;
        input.tick = state.tickCounter;
        input.button = button;
        input.down = push;
        input.isPlayer2 = isPlayer2;
        state.macro.inputs.push_back(input);
    }
};

class $modify(FPCPlayLayer, PlayLayer) {

    // ----- helpers ------------------------------------------------------

    void fpcDriveMacroPlayback() {
        auto& state = FPCState::get();
        while (state.playbackCursor < state.macro.inputs.size() &&
               state.macro.inputs[state.playbackCursor].tick <= state.tickCounter) {
            auto const& in = state.macro.inputs[state.playbackCursor];
            this->handleButton(in.down, in.button, !in.isPlayer2);
            state.playbackCursor++;
        }
    }

    void fpcStartNextAnalyzerAttempt() {
        // resetLevel() re-enters this class's resetLevel hook below, which
        // clears the per-attempt counters for us.
        PlayLayer::resetLevel();
    }

    void fpcFinishAnalysis() {
        auto& state = FPCState::get();
        state.mode = FPCMode::Idle;
        CCDirector::sharedDirector()->getScheduler()->setTimeScale(1.f);
        FramePerfectResultsPopup::showResults(state.analyzer.results);
    }

    void fpcOnAttemptFinished(bool survived) {
        auto& state = FPCState::get();
        auto& an = state.analyzer;

        size_t slot = static_cast<size_t>(an.shift - an.minShift);
        if (slot < an.survived.size()) {
            an.survived[slot] = survived;
        }

        an.shift++;
        if (an.shift > an.maxShift) {
            // Every shift for this input has been tried - find the
            // contiguous run of survives that includes shift 0 (the
            // original recorded timing).
            int zeroSlot = 0 - an.minShift;
            int lo = zeroSlot;
            int hi = zeroSlot;
            while (lo - 1 >= 0 && an.survived[lo - 1]) lo--;
            while (hi + 1 < static_cast<int>(an.survived.size()) && an.survived[hi + 1]) hi++;

            WindowResult wr;
            wr.inputIndex = an.inputIndex;
            wr.tick = state.macro.inputs[an.inputIndex].tick;
            wr.button = state.macro.inputs[an.inputIndex].button;
            wr.windowFrames = hi - lo + 1;
            an.results.push_back(wr);

            if (Mod::get()->getSettingValue<bool>("analyze-log-per-input")) {
                log::info(
                    "[FramePerfectCounter] input #{} tick={} button={} window={} frame(s)",
                    an.inputIndex, wr.tick, wr.button, wr.windowFrames
                );
            }

            an.inputIndex++;
            an.shift = an.minShift;
            std::fill(an.survived.begin(), an.survived.end(), false);

            if (an.inputIndex >= state.macro.inputs.size()) {
                fpcFinishAnalysis();
                return;
            }
        }

        fpcStartNextAnalyzerAttempt();
    }

    void fpcDriveAnalyzer() {
        auto& state = FPCState::get();
        auto& an = state.analyzer;

        if (state.macro.empty() || an.inputIndex >= state.macro.inputs.size()) {
            fpcFinishAnalysis();
            return;
        }

        // Replay every input up to the current tick, shifting only the
        // one input currently under test.
        while (state.playbackCursor < state.macro.inputs.size()) {
            auto in = state.macro.inputs[state.playbackCursor];
            int effectiveTick = in.tick;
            if (state.playbackCursor == an.inputIndex) {
                effectiveTick = std::max(0, in.tick + an.shift);
            }
            if (effectiveTick > state.tickCounter) break;

            this->handleButton(in.down, in.button, !in.isPlayer2);
            state.playbackCursor++;
        }

        int targetTick = state.macro.inputs[an.inputIndex].tick + an.shift + an.horizonFrames;

        if (state.playerDied) {
            fpcOnAttemptFinished(false);
        } else if (state.levelCompleted || state.tickCounter >= targetTick) {
            fpcOnAttemptFinished(true);
        }
        // otherwise: attempt still running, nothing to do this frame
    }

    // ----- hooks ----------------------------------------------------------

    void resetLevel() {
        auto& state = FPCState::get();

        if (state.mode == FPCMode::Recording) {
            state.macro.clear();
            if (this->m_level) {
                state.macro.levelName = this->m_level->m_levelName;
            }
        }

        state.resetForNewAttempt();

        PlayLayer::resetLevel();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        FPCState::get().playerDied = true;
        PlayLayer::destroyPlayer(player, object);
    }

    void levelComplete() {
        auto& state = FPCState::get();
        state.levelCompleted = true;

        PlayLayer::levelComplete();

        if (state.mode == FPCMode::Recording) {
            state.macro.totalTicks = state.tickCounter;
            state.mode = FPCMode::Idle;

            auto res = state.macro.saveToFile(Macro::defaultPath());
            if (res) {
                Notification::create(
                    fmt::format("Macro saved ({} inputs)", state.macro.inputs.size()),
                    NotificationIcon::Success
                )->show();
            } else {
                Notification::create(
                    "Failed to save macro: " + res.unwrapErr(),
                    NotificationIcon::Error
                )->show();
            }
        }
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto& state = FPCState::get();
        state.tickCounter++;

        switch (state.mode) {
            case FPCMode::Playing:
                fpcDriveMacroPlayback();
                break;
            case FPCMode::Analyzing:
                fpcDriveAnalyzer();
                break;
            default:
                break;
        }
    }
};
