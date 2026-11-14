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
  "environment: ... LOADED / NOT FOUND"        SkyUI and FreeClimbMenu.esp (quest 0x809, sounds 0x801-0x808)
  "sound: first event sent"                    first sound requested from the menu script
  Verbose=1 also logs "camera: distance ... side ... [ON THE WALL SIDE] [BLOCKED by geometry]" while clinging.
  Papyrus log: lines starting with "FreeClimbMCM:".
  No menu: check SkyUI, FreeClimbMenu.esp enabled, both .pex in Data/Scripts.
  No sounds: check the Sound folder and the "environment" lines above.

KNOWN ISSUES
- First person with Enhanced Camera: head stuck on the vertical axis, hands not visible.
- Some ledges are climbed as if there were an invisible step (the log records every accepted ledge).
- In third person the camera can come close or cross the wall when it is on the wall side of the character.
- NPC follower: disabled.

CREDITS AND LICENSE
- Original FreeClimb: Epsilona (GPL-3.0). This port is GPL-3.0; source: <repository URL>
- Animations: Mixamo, Threepeat Games (see the original mod's notices). Audio: Kenney.
- SKSE team, SkyUI team.
