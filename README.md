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

Examples of loading local and online files can be found [here](https://github.com/lrusso/Genesis/blob/main/Genesis.htm#L138-L186) and [here](https://github.com/lrusso/Genesis/blob/main/Genesis.htm#L214-L256).

```js
embedQuake3({
  container: "game",
  files: event.target.files,
  cbStarted: function cbStarted() {
    pleaseWait.style.display = "none"
  },
})
```

| Parameter          |    Type     | Required | Default value | Description               |
| :----------------- | :---------: | :------: | :-----------: | :------------------------ |
| container          |   string    |   yes    |       –       | Target element ID.        |
| files              |   Array     |   yes    |       –       | Files or Blobs (PK3s and optional CFG file). |
| cbStarted          |  function   |    no    |       -       | Called on emulator start. |

## Special keys:

| Action          | macOS Shortcut | Windows Shortcut | Safari Shortcut |
| :-------------- | :------------: | :--------------: | :-------------: |
| Save state      |  Command + 1   |     Ctrl + 1     |    Ctrl + 1     |
| Load state      |  Command + 2   |     Ctrl + 2     |    Ctrl + 2     |
| Toggle sound    |  Command + 3   |     Ctrl + 3     |    Ctrl + 3     |
| Fullscreen mode |  Command + F   |     Ctrl + F     |    Ctrl + F     |

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
