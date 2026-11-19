# Frame Perfect Counter

Records your own macro, replays it, then tests every single input's timing
leeway (in physics frames) and shows a histogram - the same idea as the
"frame perfect counter" videos on YouTube, where each input in a run gets
classified as a true 1-frame window, a 2-frame window, and so on.

## How it works

1. **Record** - pause in a level and hit **Record**. The level restarts and
   every button press/release you make is logged against the physics tick
   it happened on.
2. Finish (or die on, for a partial macro) the level normally. The macro is
   saved to the mod's save folder as `last-macro.json`.
3. **Play** replays that exact macro back.
4. **Analyze** is the main feature: for every recorded input, it shifts
   *only that one input* earlier/later by up to "Max Shift" frames (a
   setting), replays the attempt, and checks whether the player survives
   for "Survival Horizon" frames afterwards. The contiguous range of shifts
   that still survive (centered on the original recorded frame) is that
   input's window size.
5. When every input has been tested, a popup shows the histogram
   (`0-1 frame(s)`, `1-2 frame(s)`, ...), and the full per-input breakdown
   is written to `last-analysis.csv` in the mod's save data folder.

Testing runs the scheduler sped up ("Test Speed Multiplier" setting) so a
macro with many inputs doesn't take forever in real time - this doesn't
affect the frame-based logic, everything is keyed to the physics tick
counter, not wall-clock time.

## Known limitations (please read before relying on this)

- This measures **whole-frame** leeway at the level's normal physics tick
  rate. It does not attempt to replicate sub-frame / "Click Between
  Frames"-style precision - that's a deeper rabbit hole (it depends on the
  exact sub-stepping behavior Geode's CBF mod patches in) and isn't
  implemented here.
- "Survives for N frames" is used as the pass/fail signal, not "completes
  the whole level" - testing every shift of every input against a full
  level completion would be extremely slow. This is a reasonable proxy for
  most levels/inputs but isn't perfect for inputs that matter many seconds
  later (e.g. a precise jump that only matters because it sets up a gate
  much further into the level).
- I could not actually compile or run this in this environment (no Geode
  CLI / GD installation / NDK available here), so I can't hand you a
  tested `.geode` file - only consistent, from-scratch source. You'll need
  to build it yourself (see below).

## Building

1. Install the [Geode CLI](https://geode-sdk.org/docs/getting-started) and
   SDK for **Geode v5.0.0**, targeting **GD 2.2081**.
2. From this project folder: `geode build`
3. If building for Android, you'll also need the Android NDK set up per
   Geode's docs; for Windows, `geode sdk install-linux` lets you
   cross-compile from Linux/Termux.
4. Copy the resulting `.geode` file into Geometry Dash's
   `geode/mods` folder (or use "Install from file" in the in-game Geode
   mods tab).

## Settings

- **Max Shift (frames)** - how far in each direction to test per input.
- **Survival Horizon (frames)** - how long after an input to keep watching
  for a death before calling it a survive.
- **Test Speed Multiplier** - scheduler speed-up during analysis.
- **Log Every Input's Window** - also write each input's result to the
  Geode console/log, not just the histogram + CSV.
