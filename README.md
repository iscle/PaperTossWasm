# Paper Toss — WebAssembly port

The [PaperTossReveng](https://github.com/iscle/PaperTossReveng) Android game, ported to C++ and
compiled to WebAssembly so it runs in any browser — desktop or phone — with nothing to install.

**▶ Play: https://iscle.github.io/PaperTossWasm/**

<div align="center">
  <img src="icon.png" alt="Paper Toss" width="96"/>
</div>

## What this is

`PaperTossReveng` is a reverse-engineered, compilable rebuild of the original Paper Toss v1 for
Android. This repository is a line-by-line port of that Java source to C++, plus a small platform
layer that replaces the Android APIs with browser equivalents. The game logic, level data, physics
and timing are the same code, transliterated — including the original's quirks.

| Android | Browser |
| --- | --- |
| `GLSurfaceView` + GL ES 1.x fixed function | WebGL, with a small matrix-stack shim (`src/gl1.cpp`) that implements just the fixed-function subset the game uses |
| `Bitmap` / `BitmapFactory` | `stb_image`, with the same power-of-two padding GL ES 1.x required |
| `Canvas` + `Paint` + `Typeface` text | `stb_truetype`, reproducing Android's centred, y-flipped text bitmaps |
| `SoundPool` + `MediaPlayer` | SDL_mixer, which decodes the Ogg files itself so browser codec support does not matter |
| `SharedPreferences`-style save file | `localStorage` |
| `MotionEvent` touch plumbing | Emscripten HTML5 mouse and touch events, mapped through the same ortho maths |

The page is nothing but the game: a black backdrop and the phone screen, scaled to fit whatever
window it is opened in.

## Layout

```
src/            the port: game logic (level, menu, sprite, texture, …) + platform shim
assets/         images, fonts and sound effects from the Android build
music/          the ambient level loops (Android res/raw)
shell.html      the page the game is embedded in
build.sh        one-command build
```

## Building

Needs the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) on `PATH`:

```sh
./build.sh          # writes dist/
cd dist && python3 -m http.server 8000
```

Then open <http://localhost:8000/>. Pushing to `main` builds and deploys the same bundle to GitHub
Pages via `.github/workflows/deploy.yml`.

## Controls

Flick the paper ball toward the bin — swipe with a finger or drag with the mouse. Watch the wind
speed and direction. `Esc` goes back to the menu, like the Android back button.

## Notes on fidelity

The port keeps the original's behaviour rather than fixing it: the decompiled collision code's
unreachable splash branches, the integer division in `Sprite::setFrame`, the same event ordering and
the same frame-by-frame maths are all preserved. Deliberate deviations, all forced by the platform:

- Text metrics come from the real font. Android measured the string with the *default* typeface and
  then drew it with `fawn`/`zerothre`, which is not reproducible off-Android; measuring with the
  font actually used keeps text centred.
- Textures are uploaded with straight (non-premultiplied) alpha, which blends slightly cleaner than
  Android's premultiplied bitmaps did under the game's blend function.
- The low-resolution asset set names two files that do not exist in it (`Basement.png`,
  `restroom.png`). Android threw; here the loader falls back to the file that does exist.
- There is no "exit the app" on the web, so the menu's Exit button asks the browser to close the
  tab, which browsers ignore unless the page opened itself.

## Credits

Original game by Backflip Studios. Reverse engineering and the Android rebuild:
[iscle/PaperTossReveng](https://github.com/iscle/PaperTossReveng). Third-party code in
`src/third_party` is public-domain `stb`.

For educational and preservation purposes; all original assets belong to their respective owners.
