# PodMon
WIP Podcast application for N2DSXL consoles

# PodMon: Podcast Monsters

> A custom homebrew podcast application for the Nintendo 2DS XL, combining retro role-playing aesthetics with offline audio consumption.

![PodMon Logo](images/1786332870678.png)

## Overview

**PodMon** is a lightweight, purpose-built homebrew initiative designed to turn your New 2DS XL into a dedicated, distraction-free podcast player. Built natively using `libctru`, DevkitPro, and `libmpg123`, PodMon bypasses heavy system overhead to deliver direct audio playback from local feeds stored right on your SD card.

---

## Features

- **Native Audio Playback:** Low-level MP3 rendering via `libmpg123` and hardware `ndsp` audio channels utilizing double-buffered PCM streaming.
- **Local SD Card Scanning:** Automatically populates and navigates episode lists directly from `sdmc:/podcasts/`.
- **Minimalist UI:** Built for the dual-screen layout, offering straightforward D-pad menu navigation, play/pause controls, and real-time playback tracking.
- **Zero Bloat:** Focused strictly on core audio playback without background battery drain or cloud dependencies.

---

## Milestone Tracker

- [x] **Environment Setup:** DevkitPro & libctru toolchain verification
- [x] **Hardware Bridge:** Confirm USB connectivity and SD card deployment workflow
- [x] **Audio Engine:** Implement double-buffered MP3 playback using `libctru` (`ndsp`) and `libmpg123`
- [x] **Parsing Layer:** Local directory scanning and filtering for `.mp3` files in `sdmc:/podcasts/`
- [/] **UI Architecture:** Menu list, state management, and time/progress display (VBR duration estimation and smoothing refinements ongoing)
- [ ] **Alpha Test:** Complete multi-episode playback cycles and stability validation on hardware

---

## Getting Started

1. Set up your **DevkitPro** and `libctru` development environment.
2. Clone this repository and configure your build paths.
3. Create a `podcasts` directory on your SD card (`sdmc:/podcasts/`) and add your target `.mp3` files.
4. Compile the `.cia` / `.3dsx` binary and deploy it to your homebrew-enabled New 2DS XL.

## License

This project is open-source and built for educational and personal homebrew use.