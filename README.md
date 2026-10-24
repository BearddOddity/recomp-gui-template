# recomp-gui-template

A small **immediate-mode GUI in plain C** (in the style of Dear ImGui) and a ready-made **F1 debug menu**
for Windows / Direct3D 11 game recompilation projects. Press F1 in your recompiled game and get a menu with
tabs, sliders, checkboxes and a live performance panel, with no extra dependencies.

Extracted from the [xboxrecomp](https://github.com/sp00nznet/xboxrecomp) SDK, so it fits any game built on it.
Use **Use this template** on GitHub to start your own copy, or run the setup script below.

![The F1 menu: performance panel, a menu window with tabs, an always-on window and a notice](docs/f1-menu.png)

## What you get

- **F1 menu** (also L3+R3 on a pad): tabs, mouse, keyboard and gamepad navigation, Esc/B to close.
- **Performance panel:** FPS, frame time graph, CPU, GPU, RAM, VRAM.
- **Your own pages and windows:** a few lines each (see `examples/debug_menu.c`).
- **Notices:** one-line messages at the bottom of the screen.
- **Widgets:** windows, text, button, checkbox, radio, sliders, combo, selectable, collapsing header,
  progress bar, plots, tabs.
- **No dependencies:** portable C11 core, a Direct3D 11 renderer, and a single bundled font rasteriser.

## Use cases

- Level select, cheats and god mode while bringing up a recompiled game.
- Toggling renderer features and graphics options without recompiling.
- Watching frame time, GPU and memory load as you optimise.
- Settings screens and mod tools drawn over the game (always-on windows).
- Scripted menu input for automated test runs (`gui_overlay_press`).

## Works with

| Project | How it is used |
|---|---|
| [xboxrecomp](https://github.com/sp00nznet/xboxrecomp) by sp00nz | The SDK this comes from. v0.16.0 and older need the setup script's two hooks; newer SDKs already ship the overlay. |
| True Crime: New York City RE:COMPILED | Reference integration: pages for audio, graphics, display and developer tools. |
| Hulk: Ultimate Destruction recomp | Tested with this template's sources: F1 menu with a level selector. |

Any other Windows game that presents with Direct3D 11 can use it too, and the core also runs without
D3D11 (see [Not using Direct3D 11](#not-using-direct3d-11)).

## Quick start (xboxrecomp games)

```
git clone https://github.com/BearddOddity/recomp-gui-template
recomp-gui-template\tools\setup.ps1 -Project "D:\My apps\My Game Recomp" -PatchCmake
```

The script finds the SDK from `XBOXRECOMP_DIR` in your CMakeLists.txt (or pass `-Sdk <dir>`), then:

1. copies the GUI to `src\gui` and a starter menu to `src\debug_menu.c/.h`;
2. if the SDK has no overlay of its own (v0.16.0 and older), copies the SDK's `gpu_present.c` and
   `keyboard.c` to `src\sdk_local` and inserts the two hooks (draw at present, menu takes the keyboard);
3. writes `src\gui\recomp_gui.cmake`; `-PatchCmake` includes it from your CMakeLists.txt.

Then add two lines to `main.c`, build and press F1:

```c
#include "debug_menu.h"
...
debug_menu_init();   // once, before the guest runs
```

Safe to re-run: existing files are kept (`-Force` overwrites). The SDK is never modified.

## Add your own pages

```c
static void cheats(void *user) {
    gui_checkbox("God mode", &god);
    if (gui_button("Reload level")) want_reload = 1;
}
static void tracker(void *user) {
    gui_begin("Tracker", NULL, GUI_WINDOW_AUTO_RESIZE);
    gui_text("lap %d", lap);
    gui_end();
}

gui_overlay_add_page("Cheats", cheats, NULL);      // a tab in the F1 menu
gui_overlay_add_window("Tracker", tracker, NULL);  // drawn every frame
```

Callbacks run on the thread that presents: set flags, let your game thread act, and never block.

## Manual setup

What the script does, for other layouts:

1. **Add the code.** `add_subdirectory(recomp-gui-template)` and link `recomp_gui`, or copy `src/`.
2. **Register pages** once from `main()` before the first frame, as in `examples/debug_menu.c`.
3. **Draw it** once per frame, after the scene and just before `Present()`:
   `gui_overlay_frame_d3d11(device, context, backbuffer_rtv, hwnd);`
4. **Pad.** Zero your port 0 pad state while `gui_overlay_wants_input()` is true, as True Crime does:
   `if (port == 0 && gui_overlay_wants_input()) memset(&st.Gamepad, 0, sizeof st.Gamepad);`
5. **Mouse and wheel.** Skip your own mouse handling while `gui_overlay_wants_mouse()` is true, and forward
   `WM_MOUSEWHEEL` to `gui_overlay_wndproc()`.
6. **Input hook.** `src/gui_host.c` is the one place the GUI touches your runtime. With the SDK's
   `xbox_InputSetCaptured` present, define `RECOMP_GUI_XBOXRECOMP` and it is called for you.

### Not using Direct3D 11

Use the core alone: fill `gui_io()` each frame, call `gui_new_frame()`, your widgets, then `gui_render()`
and draw the returned `GuiDrawData` (vertices, indices, scissored draw commands) with your renderer.
See `src/gui.h`.

## Settings

| Variable | Effect |
|---|---|
| `RECOMP_GUI=on\|off` | Turn the overlay on or off (your code can also call `gui_overlay_set_enabled`). |
| `RECOMP_OSD=on\|off` | Show or hide the performance panel at start. |
| `RECOMP_GUI_SCALE=<k>` | UI scale. Default: back-buffer height / 720, at least 1. |
| `RECOMP_GUI_FONT=<.ttf>` | Font file. Default: Segoe UI from the Windows font folder. |

## Repository layout

| Path | What |
|---|---|
| `src/gui.[ch]` | The immediate-mode core |
| `src/gui_d3d11.[ch]` | Direct3D 11 renderer |
| `src/gui_overlay.[ch]` | Performance panel, F1 menu, notices |
| `src/gui_host.[ch]` | The one hook into your runtime (input capture) |
| `src/stb_truetype.h` | Font rasteriser (public domain / MIT) |
| `examples/` | Starter F1 menu (`debug_menu.c/.h`) |
| `tools/setup.ps1` | One-command setup for an xboxrecomp project |
| `tests/gui_smoke.c` | Core checks; with a `.bmp` argument also renders off screen |
| `docs/` | Screenshots rendered by the smoke test |

## Build and test

```
cmake -S . -B build && cmake --build build --config Release
build\Release\gui_smoke.exe        # prints PASS
```

CI builds on Windows and runs the smoke test on every push.

## Credits and license

MIT, Copyright (c) 2026 sp00nz (`LICENSE`). The code is extracted from the xboxrecomp SDK; the one change is the
input hook in `gui_overlay.c`. Third-party credits in `NOTICE`: stb_truetype (public domain / MIT) and the
Moonlight colour theme by Madam-Herta / deathsu.
