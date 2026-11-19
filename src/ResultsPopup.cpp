#include "ResultsPopup.hpp"
#include <Geode/Geode.hpp>
#include <map>
#include <sstream>

using namespace geode::prelude;

namespace {
    std::string bucketLabel(int windowFrames) {
        constexpr int kOverflowAt = 7;
        if (windowFrames >= kOverflowAt) {
            return fmt::format("{}+ frames", kOverflowAt - 1);
        }
        return fmt::format("{}-{} frame(s)", windowFrames - 1, windowFrames);
    }

    void saveResultsToFile(std::vector<WindowResult> const& results) {
        std::ostringstream csv;
        csv << "input_index,tick,button,window_frames\n";
        for (auto const& r : results) {
            csv << r.inputIndex << "," << r.tick << "," << r.button << "," << r.windowFrames << "\n";
        }

        auto path = Mod::get()->getSaveDir() / "last-analysis.csv";
        auto res = utils::file::writeString(path, csv.str());
        if (!res) {
            log::warn("[FramePerfectCounter] could not save results csv: {}", res.unwrapErr());
        } else {
            log::info("[FramePerfectCounter] full results written to {}", path.string());
        }
    }
}

void FramePerfectResultsPopup::showResults(std::vector<WindowResult> const& results) {
    saveResultsToFile(results);

    if (results.empty()) {
        FLAlertLayer::create(
            "Frame Perfect Counter",
            "No inputs were analyzed - record a macro first.",
            "OK"
        )->show();
        return;
    }

    // windowFrames -> count, kept in insertion-sorted order via std::map
    std::map<int, int> histogram;
    for (auto const& r : results) {
        histogram[r.windowFrames]++;
    }

    std::ostringstream body;
    body << fmt::format("Analyzed <cy>{}</c> input(s):\n\n", results.size());

    for (auto const& [window, count] : histogram) {
        body << fmt::format("<cl>{}</c>: <cg>{}</c>\n", bucketLabel(window), count);
    }

    body << "\nFull per-input breakdown saved to\nmod save data / last-analysis.csv";

    FLAlertLayer::create(
        "Frame Perfect Counter - Results",
        body.str(),
        "OK"
    )->show();
}
