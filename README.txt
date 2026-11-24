FreeClimb LE 0.30.0
Wall climbing for Skyrim Legendary Edition (1.9.32). A port of FreeClimb (Skyrim SE) by Epsilona.

REQUIREMENTS
- Skyrim LE 1.9.32, SKSE 1.7.3, SkyUI (for the menu)
- FreeClimb animation pack 0.5.0 (31 animations) and sounds, from the original mod

FILES (Data folder)
  SKSE/Plugins/FreeClimbLE.dll
  SKSE/Plugins/FreeClimbLE.ini          (created with defaults if missing)
  Scripts/FreeClimbNative.pex
  Scripts/FreeClimbMCM.pex
  FreeClimbMenu.esp                     (menu quest + sound records)
  meshes/actors/character/animations/FreeClimb/   (animation pack 0.5.0)
  Sound/fx/FreeClimb/

CONTROLS
  Grab a wall ............ Jump next to it (EntryMode 1: Jump + W + A + D together)
  Move ................... W / A / S / D and diagonals (left stick)
  Wall run ............... Shift + direction
  Slide down ............. Shift + S (release to brake and keep hanging)
  Hop .................... Jump + W / A / D
  Push away .............. Jump + S
  Let go ................. Sneak, or Jump + A + S + D
  Ledge .................. pulled over automatically at the top

MENU (MCM > FreeClimb)
  General ........ status (pack loaded, bones bound), language EN/PT, save/load/reset profile, verbose log
  Movement ....... speeds, start ramp, wall run, hops, slide
  Wall & entry ... grab mode and reach, distance to the wall, ledge heights
  Smoothness ..... turn/position smoothing, animation blending, pose engine
  Stamina ........ costs per second and per action, regeneration, behaviour at zero
  Surface & FPS .. hands/feet fitting (ray budget), gamepad
  Sounds ......... on/off, volume, repeat gap
Every change is written to the INI at once. Profile: Save/Load keeps your own values.

INI
  Location: next to the DLL (SKSE/Plugins/FreeClimbLE.ini). Reloaded each time you grab a wall.
  Delete it to restore the defaults. Keys and ranges are listed in the file itself.
  Values you set are kept: if you upgrade, replace the INI or change the new options in the menu.

STAMINA
  Costs % of max per second (hang, climb, descend, sideways, wall run, slide) and per action (hop, ledge).
  Regeneration is suspended on the wall. At zero you slide down slowly; climbing works again above
  StaminaResume %. Pulling over a ledge is never refused.

FALL DAMAGE
  Fall height and time are reset while on the wall. Letting go on the ground (end of a slide or descent)
  causes no damage. Letting go in the air counts the fall from the point where you let go.

TROUBLESHOOTING (FreeClimbLE.log)
  "animation pack: ACCEPTED - loaded 31"       animations found
  "pose engine: N of 99 bones bound"           N should be about 80
  "papyrus interface: registered"              menu functions available
  "menu: the menu script called the DLL"       menu script is running
  "sound: first event sent"                    first sound requested from the menu script
  Verbose=1 also logs "camera: distance ... side ... [ON THE WALL SIDE] [BLOCKED by geometry]" while clinging.
  Papyrus log: lines starting with "FreeClimbMCM:".
  No menu: check SkyUI, FreeClimbMenu.esp enabled, both .pex in Data/Scripts.
  No sounds: check the Sound folder, the "sound: first event sent" line and the Papyrus log.

CHANGES (source, not yet released)
- NPCs, creatures and bodies are ignored by every probe (IgnoreActors=1): no more climbing people.
- Entry height check (EntryHeightCheck=1): the jump only clings to something taller than EntryMinHeight (0 = MinLedge) in two
  consecutive levels. MinLedge used to be read only when pulling over a ledge, never when grabbing, which is why raising it
  changed nothing. Rejected jumps fall through to the normal jump / FreeRun.
- Protrusions (ProtrusionAssist=1): an eave, cornice or balcony above the head is hopped over (climb up against it, or Jump+W),
  a wall that steps back after a bulge is followed instead of dropped, and the body is kept clear of bulges at feet / head height.
- Safety: every ray is refused if it has a NaN / infinity (a classic way to hang the game), loops that could spin forever were
  removed, the jump handler and the skeleton tick are wrapped so a memory fault is logged (\"FAULT in ...\") instead of killing
  the game, and a watchdog writes \"WATCHDOG: ... stuck at step ...\" to FreeClimbLE.log if something never returns.
  After 3 faults the mod switches itself off for the session.
- FIX for the freeze on Jump with big skeletons (found from a user's FreeClimbLE.log): the skeleton was re-bound to the
  FreeClimb bones with a 99 x N loop that called VirtualQuery twice per comparison, on the main thread, when the jump key was
  pressed. With a 642-entry skeleton (CME, SOS, Devourment...) that is tens of thousands of calls: seconds of frozen game.
  Now one pass over the entries (about 50 times fewer calls, same result; see tools/test_bonebinding.cpp). The log line
  "skeleton target refreshed ... (N ms)" now shows how long it took.
- Wobble when standing still on a wall or tree (and the camera shaking with it): the wall was re-read every frame and the body
  glued to whatever the ray said, so a thin pillar or an edge that the ray hit one frame and missed the next made the body jump
  back and forth (and sometimes pushed it away from the surface). Now, standing still, the body is held exactly where it is
  (the wall is only re-read every 300 ms, and only corrected if it is off by more than 3 units / 6 degrees); while moving, the
  push away from bulges rises fast and falls slowly. StableHold=0 restores the old behaviour.
- Clipping into bulges: the body is now sampled at 4 heights (plus the chest), and a shallow bulge above the head that does not
  cover the centre of the body (but the chest and head pass through it) is hopped over like an eave, when a hop is possible.
- Camera diagnostics: the log now always records "camera: state changed 9 -> 0 while clinging" (third -> first person), and with
  Verbose=1 the camera line says WHAT blocks the camera (layer, distance, height, normal) plus the zoom. See KNOWN ISSUES.
- Climbing onto a ledge with a slope (a roof, normal.z 0.7-0.85) buried the character: the height was measured 25 units past the wall
  but the character was placed 40 past it, where a slope is higher. Now the standing spot is measured where the character will
  actually stand (40, 32 or 24 units past the wall), with a walkable surface, no parapet in the way and head room; otherwise the
  ledge is refused ("no standing room on top") instead of burying or floating the character. The log line says where it stands.
- Floating away from the wall: the push away from bulges stayed frozen at whatever value it had when you stopped, and on rough rock
  and trees every small bump pushed the body out. Now the push relaxes back to 0 when the bulge is gone (3 agreeing readings, then
  locked for 2 s so it cannot oscillate), and small bumps are ignored: BumpTolerance (default 18) and MaxBumpPush (default 30)
  in the .ini let you trade clipping against floating. If you still sit too far from the surface, lower WallGap (30) to 24-26.
- Verbose=1 now also logs a "fit:" line twice a second while clinging (push measured / applied, position error, HELD or snapped).
- New INI keys: BumpTolerance, MaxBumpPush, StableHold, IgnoreActors, EntryHeightCheck, EntryMinHeight, ProtrusionAssist (no menu entries).
- Test of the new geometry outside the game: tools/test_climbprobe.cpp (g++ -std=c++14 -Ifreeclimb-le).

KNOWN ISSUES
- First person with Enhanced Camera: head stuck on the vertical axis, hands not visible.
- Some ledges are climbed as if there were an invisible step (the log records every accepted ledge).
- In third person the camera can collapse onto the character's head (it looks like first person) when something blocks it, for
  example an eave above or the ground below while looking up the wall. The third-person camera collision is a game data layer
  (L_CAMERA), not something this plugin can switch off safely; mods that edit that layer remove the problem globally.
  Run with Verbose=1 and send the "camera:" lines from the log to pin down what blocks it.
- NPC follower: disabled.

CREDITS AND LICENSE
- FreeClimb LE: Copyright (c) 2026 Ezyel. GPL-3.0-only. Source code: https://github.com/edvaras1999/FreeClimb_LE
- Original FreeClimb: Epsilona (GPL-3.0-only), https://github.com/Eelta/FreeClimb (Nexus Mods, Skyrim SE, mod 193552).
  Published with the original author's permission, on the condition of complying with the GPL.
- Animations: Adobe Mixamo and Threepeat Games. Audio: Kenney (CC0). These are NOT covered by the GPL and must not be
  redistributed as a standalone library. Notices are in the licenses/ folder (see THIRD-PARTY-NOTICES.txt).
- Built against SKSE 1.7.3 (not included); the menu needs SkyUI (not included).
- You may redistribute and modify the plugin under the GPL-3.0-only terms (see LICENSE). BUILDING.md explains how to build it.
