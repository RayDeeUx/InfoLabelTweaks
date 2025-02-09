## InfoLabelTweaks Changelog
## v1.0.7
- Fix a bug where the full song path of a song would be exposed to the viewer if the song name had spaces.
  - This typically doesn't happen under usual circumstances, but is more likely to happen when using personal replacement songs from your settings in the NeutralizeBadSongs mod.
  - Also added support for detecting `.oga` files for song paths. Hopefully there aren't any others that were forgotten.
## v1.0.6
- Fix a silly bug where the info label text would remain unchanged if the positioning toggles were not enabled.
- Proper compatibility with TokTikMode.
## v1.0.5
- <cr>MASSIVE</c> refactor to reduce lag under certain circumstances.
  - Thank you to Alphalaneous for teaching me one of their tricks for doing so.
- Fix that one rare bug where the Player Velocity display would be prefixed with a `/`.
## v1.0.4
- Change how FPS is calculated.
- Add `Misc` category. Contains:
  - Date and time
  - Game uptime
  - Texture quality info
  - Window size info (Fullscreen, Aspect Ratio)
## v1.0.3
- Port to 2.2074.
## v1.0.2
- Fix a crash from selecting Fonts 1 through 9 (even though they're all objectively ugly). Thanks again, hiimjustin000!
## v1.0.1
- Fix a crash from pressing keys Cocos2d wouldn't recognize. Big thanks to Minemaker and hiimjustin000 for finding this crash!
## v1.0.0
- Initial release (on GitHub). Changes since being split off from ErysEdits:
  - Added size scaling options.
  - Added right text alignment.
  - Added various positioning options (Align Bottom, Align Right).
  - Added Camera Properties section.
  - Added Custom Footer (a text option).
  - Added "hIDe" option for local editor levels (regardless of verification progress) and unlisted levels.
  - Added total time spent in a level (platformer levels only).
  - Added player velocity/rotation options.
  - Added name of last key pressed (Steam users only).
  - "Show Most Recent Song/Effect" is now on macOS.
  - Fixed a few bugs left over from ErysEdits.
- Big thanks to CatXus, Aktimoose, ninXout, and hiimjustin000 for helping out with testing!
- Big thanks to dank_meme for helping me fix a crash!
