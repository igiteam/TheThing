# Quake III Arena

A Quake III Arena port designed for running in vanilla JavaScript pre-ECMAScript 2015 (no WebAssembly). Simply open the link below, click the red icon, and select all the `PK3` files and `q3config.cfg` (optional); all files will be loaded and booted automatically.

## Links:

- [Quake III Arena](https://lrusso.github.io/Quake3/Quake3.htm)
- [Quake III Arena online demo](https://lrusso.github.io/Quake3/Quake3.htm?demo)

## Screenshots:

![alt screenshot1](https://lrusso.github.io/Quake3/SCREENSHOT1.jpg)

![alt screenshot2](https://lrusso.github.io/Quake3/SCREENSHOT2.jpg)

![alt screenshot3](https://lrusso.github.io/Quake3/SCREENSHOT3.jpg)

## How to use it:

Examples of loading local and online files can be found [here](https://github.com/lrusso/Quake3/blob/master/Quake3.htm#L131-L153) and [here](https://github.com/lrusso/Quake3/blob/master/Quake3.htm#L156-L167).

```js
embedQuake3({
  container: "game",
  files: event.target.files,
  cbStarted: function cbStarted() {
    pleaseWait.style.display = "none"
  },
})
```

| Parameter          |    Type     | Required | Description               |
| :----------------- | :---------: | :------: | :------------------------ |
| container          |   string    |   yes    | Target element ID.        |
| files              |   Array     |   yes    | Files or Blobs (PK3s and q3config.cfg). |
| cbStarted          |  function   |    no    | Called on game start. |

## Special keys:

| Action          | macOS Shortcut | Windows Shortcut | Safari Shortcut |
| :-------------- | :------------: | :--------------: | :-------------: |
| Download q3config.cfg      |  Command + P   |     Ctrl + P     |    Ctrl + P     |

## Main differences with the original project:

- Added logic to load states.
- Added logic to save states.
- Added logic to toggle sound.
- Added logic for setting keys for player 1.
- Added logic for setting keys for player 2.
- Fixed rendering issues when using tiles.
- Fixed issue where PAL games were not loaded.
- Fixed issue where PAL games were playing the sound too fast.
- Fixed issue where PAL games were not filling the screen width.
- Fixed issue where the player 2 keys was not yet implemented in wasm.
- Fixed issue where the sound event was called from the JavaScript side.
- Pulled the latest changes from the original Genesis Plus GX repository.
- Implemented a diferrent setting for porting the emulator file to a asm.js file.
- Implemented logic for pausing the emulator on blur and resuming on focus.
- Implemented a process that converts the JavaScript build to pre-ECMAScript 2015.

## Based on the work of:

https://github.com/inolen/quakejs
