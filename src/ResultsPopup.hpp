#pragma once

#include "FPCState.hpp"

// Builds and shows the histogram popup once an analysis run finishes.
// Buckets are labeled by window size in frames: a window of 1 frame means
// the input only succeeds on the exact recorded frame ("0-1"), a window of
// 2 frames means it also succeeds one frame early or late ("1-2"), etc.
class FramePerfectResultsPopup {
public:
    static void showResults(std::vector<WindowResult> const& results);
};
