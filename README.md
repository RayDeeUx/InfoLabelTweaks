# InfoLabelTweaks

Adds additional information to GD's Info Label, or compacts portions of it. (Formerly part of ErysEdits.)

*Note: As of writing, not all features are available across all platforms. Install the mod from the [Geode index](https://geode-sdk.org/mods/raydeeux.infolabeltweaks) to confirm.*

*If [Minecraftify](https://geode-sdk.org/mods/zalphalaneous.minecraft) is loaded, it will hide the Info Label in favor of Minecraftify's own info label layout (resembling Minecraft's F3 menu). This mod will not run if Minecraftify is loaded; adding an incompatibility means wasting time (dis/re-en)abling this mod.*

Featured level: And Ever by galofuf
![demoOne](https://github.com/RayDeeUx/InfoLabelTweaks/blob/main/resources/demoOne.png)

Extra information displayed:
* Player Status (speed, vehicle, direction, size, gravity, dashing/alive/dual statuses)
* Level Traits (level type, length, 2P status, completion status)
* Camera Properties (position, offset, zoom, angle, shake factor, edge values)
* Geode Loader Info (Geode version, platform, loaded/disabled/installed mod counts, number of problems)

Additional features:
* "hIDe" Level IDs on custom editor levels and unlisted levels
* Various options to make the info label readable in a wider range of circumstances, including incredibly bright backgrounds
  * Readable Mode (Blending)
  * Chroma Mode
  * Custom Scale
  * Right Text Align
  * Right/Bottom Position Align
  * Setting a different font
* Remove portions of the info label when values are unused, including sections added by this mod (Conditional Labels)
* Compact sections of the info label when possible
* Show extra tidbits:
  * Four decimal places in player position
  * Name of most recent song/SFX file played in a level
  * Total attempt count
  * Total time during level session (Platformer levels only, view description for more info)
  * Jump count per level session
  * Current gameplay channel
  * FPS
  * Custom footer text (Please use this feature responsibly.)

Resolve various visual annoyances:
  * "LevelID" -> "Level ID"
  * "Taps" -> "Actions" on Platformer, "Clicks" on Classic
  * "-- Perf --" -> "-- Performance --"
  * Add "-- Gameplay --" header to first section

This [Geode mod](https://geode-sdk.org) is licensed under LGPLv2.