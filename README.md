# recomp-gui-template

An immediate-mode GUI in plain C (after Dear ImGui) plus a ready-made **F1 debug menu**
overlay for Windows / Direct3D 11 static-recompilation projects. Extracted from the
[xboxrecomp](https://github.com/sp00nznet/xboxrecomp) SDK, where True Crime: New York City RE uses it.

- Core is portable C11, no platform calls: input in (`GuiIO`), one vertex/index list out.
- Widgets: windows, text, button, checkbox, radio, sliders, combo, selectable, collapsing header,
  progress bar, plots, tabs. Mouse, keyboard and pad navigation built in.
- Overlay: FPS/frame-time/CPU/GPU/RAM/VRAM panel, F1 (or L3+R3) menu with tabs, always-on windows, notices.

## Layout

| Path | What |
|------|------|
| `src/gui.[ch]` | immediate-mode core |
| `src/stb_truetype.h` | font rasteriser (public domain / MIT) |
| `src/gui_d3d11.[ch]` | D3D11 renderer for the draw data |
| `src/gui_overlay.[ch]` | perf panel, F1 menu, notices (Windows) |
| `src/gui_host.[ch]` | the one hook into your runtime (input capture) |
| `examples/debug_menu.c` | F1 menu scaffold: pages, window, game-side flags |
| `tests/gui_smoke.c` | core checks; with a `.bmp` arg also renders off-screen |

## Integrate into a recomp project

1. **Copy or submodule** this repo, then `add_subdirectory(recomp-gui-template)` and
   `target_link_libraries(your_game PRIVATE recomp_gui)`. (Or copy `src/` into your tree.)
2. **Input hook.** Edit `src/gui_host.c`: make `recomp_gui_input_captured()` tell your pad/keyboard
   layer to report nothing pressed while the menu is open. Or gate game input on
   `recomp_gui_is_captured()`. (xboxrecomp used `xbox_InputSetCaptured`.)
3. **Register pages** at start-up, once from `main()` before the first frame (True Crime: `tc_menu_init()`). Copy `examples/debug_menu.c`, rename, add tabs:
   ```c
   gui_overlay_add_page("Cheats", cheats_page, NULL);     // tab in the F1 menu
   gui_overlay_add_window("Tracker", tracker_win, NULL);  // drawn every frame
   if (!getenv("RECOMP_GUI")) gui_overlay_set_enabled(1); // on by default, as True Crime does
   gui_overlay_set_title("My Game debug menu");
   ```
   Callbacks run on the presenting thread. Set flags only; let the game thread act.
4. **Draw it.** Once per frame, after the scene and just before `Present()`:
   ```c
   gui_overlay_frame_d3d11(device, context, backbuffer_rtv, hwnd);
   ```
   Forward `WM_MOUSEWHEEL` to `gui_overlay_wndproc()` if you want the wheel.
5. **Pad.** Zero your pad state for port 0 while `gui_overlay_wants_input()` is true, exactly as True Crime does:
   `if (port == 0 && gui_overlay_wants_input()) memset(&st.Gamepad, 0, sizeof st.Gamepad);`
6. **Mouse.** If your game grabs the mouse, skip it while `gui_overlay_wants_mouse()` or
   `gui_overlay_wants_input()` is true.

Not using D3D11? Use the core only: fill `gui_io()` each frame, call `gui_new_frame()`, your widgets,
then `gui_render()` and draw the returned `GuiDrawData` with your renderer. See `gui.h`.

## Environment

`RECOMP_GUI=on|off`, `RECOMP_OSD=on|off` (perf panel), `RECOMP_GUI_SCALE=<k>` (default backbuffer height / 720),
`RECOMP_GUI_FONT=<.ttf>` (default Segoe UI Semibold from the Windows font folder).

## Build and test

```
cmake -S . -B build && cmake --build build --config Release
build\Release\gui_smoke.exe        # prints PASS
```

Worked example in a real project: `src/recomp_manual.c` of True Crime NYC RE (pages, F1 time-of-day,
developer tools, scripted menu input via `gui_overlay_press`).

## Changes from the SDK copy

Only `gui_overlay.c`: the direct call to `xbox_InputSetCaptured()` became `recomp_gui_input_captured()`
(`gui_host.[ch]`), so the library builds without the xboxrecomp input layer. CMake target is `recomp_gui`.

## License

MIT, Copyright (c) 2026 sp00nz (`LICENSE`, unchanged). Third-party credits in `NOTICE`.
