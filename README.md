# A2 RACK v2 — LTA AUDIO

**Screenshot-inspired interface, original DSP.** A2 RACK is an independently authored
master-bus wavefolder / clipper VST3 for FL Studio and other Windows VST3 hosts.
It is not AWOL Audio's A5 plugin, does not contain its code or audio assets,
and does not attempt to match its internal algorithms sample-for-sample.

## The new interface

- Hardware-like black panel, steel trim, brown side rails and small screws
- Large live white oscilloscope across the upper half
- Lower controls ordered **W-FOLD / DRIVE / [A2 logo] / OUTPUT / SLOPE**
- Red DRIVE knob, dark knobs elsewhere
- Compact toolbar with preset stepping, quick-save star, reset and power
- On-screen stereo peak indicators; waveform hold and WAV export

## Audio processing

`Input -> 4x oversampler -> Drive (0..30 dB) -> Wavefold -> Slope (soft/hard clip) -> Output -> downsample -> Output`

- **W-FOLD (0–10)**: independently implemented periodic foldback waveshaping
- **DRIVE (0–10)**: pushes signal by up to +30 dB before shaping
- **OUTPUT (-24 to +12 dB)**: final level trim; watch the FL Studio meters
- **SLOPE (0–10)**: morphs tanh saturation into hard clipping
- **ON/OFF**: passes audio through the oversampling stage with consistent latency
- **Waveform**: rolling audio-monitoring view, clickable to freeze
- **WAV**: exports the most recent up-to-2-second stereo buffer as a 32-bit float WAV

Capture is an audio-history export, not offline rendering or a beat slicer.
No drag-to-FL Studio support in this version. Pitch/Body/Room/etc. from the earlier
A2 experimental version have been removed to match the four-control reference.

## Factory presets

MASTER, SOFT LIMIT, 808 PRESSURE, BRICK WALL, REDLINE, FOLDBACK,
CRUNCH, WARM PUSH, DRUM SMASH. Presets are starting points, not calibrated targets.

## Build using GitHub Actions (no local C++ setup needed)

1. Create an **empty PUBLIC or PRIVATE** repository at https://github.com/new
   named `A2-RACK-v2`; **do not** tick "Add a README".
2. In **GitHub Desktop**, clone the empty repo to your Windows PC.
3. Extract the downloaded A2 zip, then copy **all contents** of the extracted
   `A2-RACK-v2` folder into the cloned repository directory, **including `.github`**.
   Do not put the folder itself inside the repo; `CMakeLists.txt` must be at root.
4. Back in GitHub Desktop, Commit to main, then **Push origin**.
5. On github.com open the repo **Actions** tab, select
   **A2 RACK v2 - Build Windows VST3**, then click **Run workflow**.
   If it ran automatically after your push, use that run instead.
6. Once the job has the green tick, scroll to **Artifacts** and download
   `A2-RACK-v2-Windows`.
7. Extract the artifact. Copy its entire `A2 RACK.vst3` directory to
   `C:\Program Files\Common Files\VST3` on Windows (admin rights may be needed).
8. FL Studio > Options > Manage plugins > Find installed plugins. Search A2 RACK
   then insert on the Master channel.

You can also test `A2 RACK.exe` standalone if produced by the build.

### If you do not see the workflow in Actions

- Verify `.github/workflows/build-windows.yml` exists in the repo website.
- Verify `CMakeLists.txt` is at **repository root**, not in a nested folder.
- Ensure the workflow has reached the repo's default branch.
- GitHub Desktop is preferred over browser upload for preserving hidden files.

### Build locally using VS 2022

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 4
```

The CMake project fetches JUCE **8.0.17-security**, requiring internet access.
JUCE is a separate project governed by its own licensing terms: see
https://juce.com/legal/juce-8-licence/ before redistribution.

## Controls and saved files

- Top left `≡`: factory preset menu, save/load custom `.a2preset`, recall quick-save,
  WAV export and MASTER reset.
- `◀`/`▶`: cycle factory presets.
- `☆`: save currently selected knob settings as quick-save to
  `Documents/A2 RACK Presets/quick-save.a2preset`.
- `↺`: restore the MASTER preset.
- Red power LED: effect enabled, click to bypass.
- Waveform: click to hold / resume.
- `WAV`: export up to the previous 2 seconds into `Documents/A2 RACK Exports`;
  afterwards, press `RESUME` to continue capture.
- Double-click a rotary control to return it to its default setting.

## Implementation notes / limits

- Source provided, *not* a pre-built VST3. Full Windows compile/FL Studio tests
  still need to be run after uploading to GitHub.
- 4x oversampling reduces but does not eliminate aliasing at extreme drive.
- The recorded capture is stored in memory and is not a live recording service.
- Your master output can exceed 0 dBFS when OUTPUT is turned up; gain-stage
  carefully and use FL Studio's meters. No true-peak ceiling is promised.
- Uses no network services at runtime. Windows x64 target via GitHub Actions.
- Tested DSP waveform math with the bundled standalone standard C++ unit test.

## Original authorship

This is an independently developed LTA Audio example implementation. No images,
source code, sound banks, presets, or trademarked assets from AWOL Audio are
included. Its interface is **inspired by** the user's visible reference at a
high level, not a pixel-for-pixel reproduction.
