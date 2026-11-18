# Building FreeClimb LE

FreeClimb LE is licensed under GPL-3.0-only (see `LICENSE`). This file describes how to build the DLL and
the scripts from this repository, as the GPL requires for the corresponding source.

## Requirements
- Windows with Visual Studio 2022 (toolset v143), platform **Win32**, or the GitHub Actions workflows in this repository
- SKSE 1.7.3 source (`skse_1_07_03.7z`, the "src" folder). It is **not** in this repository (the SKSE terms forbid
  redistributing it). The workflow downloads it from https://skse.silverlock.org/beta/skse_1_07_03.7z.
  To build locally, place the archive in `third_party/skse_1_07_03.7z` (this folder is ignored by git).

## Build the DLL (GitHub Actions)
1. Actions > "Build FreeClimb LE" > Run workflow.
2. Download the artifact: `FreeClimbLE.dll`.

The project is `freeclimb-le/FreeClimbLE.vcxproj` (C++14 for the plugin, C++20 for `PackLoader.cpp` and `src/`).
`freeclimb-le/src/` is the animation core of the original FreeClimb, used unmodified.
`freeclimb-le/external/nlohmann/json.hpp` is nlohmann/json 3.11.3 (MIT).

## Build the scripts (Papyrus)
1. Actions > "compile-papyrus" > Run workflow.
2. Download the artifact `papyrus`: `FreeClimbNative.pex` and `FreeClimbMCM.pex`.

Sources: `papyrus/src/*.psc`. `papyrus/stubs/*.psc` only declare the functions the scripts call (for the compiler).

## Mod archive layout (Data folder)
```
SKSE/Plugins/FreeClimbLE.dll
SKSE/Plugins/FreeClimbLE.ini
Scripts/FreeClimbNative.pex
Scripts/FreeClimbMCM.pex
FreeClimbMenu.esp
meshes/actors/character/animations/FreeClimb/   (animation pack 0.5.0 from the original FreeClimb)
Sound/fx/FreeClimb/
licenses/                                        (copy of this repository's licenses/ folder)
```

## Releases
Create a git tag for every version you publish (for example `v0.30.0`) so the source of each published DLL is available,
and link the repository in the mod description.
