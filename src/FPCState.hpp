#pragma once

#include "Macro.hpp"
#include <optional>

enum class FPCMode {
    Idle,
    Recording,
    Playing,
    Analyzing
};

// Result of testing one recorded input's timing leeway.
struct WindowResult {
    size_t inputIndex = 0;
    int tick = 0;
    int button = 0;
    int windowFrames = 0; // contiguous surviving shift range around 0, in frames
};

// Everything the analyzer needs to remember between postUpdate() calls
// while it is stepping through "shift this one input by N frames and see
// if the player survives" attempts.
struct AnalyzerProgress {
    size_t inputIndex = 0;     // which recorded input we're currently testing
    int shift = -1;            // current shift being tested (starts at -maxShift)
    int minShift = -6;
    int maxShift = 6;

    // survived[shift + maxShift] once the shift range is known
    std::vector<bool> survived;

    bool diedThisAttempt = false;
    int attemptStartTick = 0;   // tick at which the current attempt's playback begins counting
    int horizonFrames = 20;

    std::vector<WindowResult> results;

    void reset(int maxShiftFrames, int horizon) {
        inputIndex = 0;
        minShift = -maxShiftFrames;
        maxShift = maxShiftFrames;
        shift = minShift;
        survived.assign(static_cast<size_t>(maxShiftFrames) * 2 + 1, false);
        diedThisAttempt = false;
        attemptStartTick = 0;
        horizonFrames = horizon;
        results.clear();
    }
};

// Single process-wide state blob for the mod. PlayLayer is recreated every
// attempt, so this can't live on the layer itself.
class FPCState {
public:
    static FPCState& get() {
        static FPCState instance;
        return instance;
    }

    FPCMode mode = FPCMode::Idle;

    // Shared physics-tick clock, incremented once per postUpdate call and
    // reset to 0 whenever resetLevel() runs. Recording, playback and
    // analysis are all keyed off this instead of wall-clock time so that
    // speeding up the scheduler for testing never desyncs anything.
    int tickCounter = 0;

    Macro macro;

    // Playback (and analyzer sub-playback) cursor into macro.inputs.
    size_t playbackCursor = 0;

    bool playerDied = false;
    bool levelCompleted = false;

    AnalyzerProgress analyzer;

    void resetForNewAttempt() {
        tickCounter = 0;
        playbackCursor = 0;
        playerDied = false;
        levelCompleted = false;
    }

    void fullReset() {
        mode = FPCMode::Idle;
        macro.clear();
        resetForNewAttempt();
        analyzer = AnalyzerProgress {};
    }
};
