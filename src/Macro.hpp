#pragma once

#include <Geode/Geode.hpp>
#include <vector>
#include <string>
#include <filesystem>

// One recorded input event, quantized to the physics frame (tick) it was
// queued on. We deliberately use an integer tick instead of a float
// timestamp: frame-perfect analysis is about *frames* of leeway, and
// keying everything to the tick counter keeps recording / playback /
// analysis in sync even while the scheduler is sped up for testing.
struct RecordedInput {
    int tick = 0;
    int button = 1;     // PlayerButton id (1 = jump, 2 = left, 3 = right, ...)
    bool down = true;   // press (true) or release (false)
    bool isPlayer2 = false;

    matjson::Value toJson() const;
    static geode::Result<RecordedInput> fromJson(matjson::Value const& json);
};

struct Macro {
    std::vector<RecordedInput> inputs;
    std::string levelName;
    int totalTicks = 0; // tick count at the moment recording stopped (level complete / manual stop)

    void clear();
    bool empty() const { return inputs.empty(); }

    matjson::Value toJson() const;
    static geode::Result<Macro> fromJson(matjson::Value const& json);

    geode::Result<> saveToFile(std::filesystem::path const& path) const;
    static geode::Result<Macro> loadFromFile(std::filesystem::path const& path);

    // Default location inside the mod's save directory.
    static std::filesystem::path defaultPath();
};
