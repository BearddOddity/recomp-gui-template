/*
 * The overlay for Linux and macOS (and anywhere else without Direct3D 11).
 *
 * Same pages / windows / notices API as gui_overlay.h (include that too: the
 * gui_overlay_add_page(), gui_overlay_notice(), gui_overlay_set_enabled() ... calls are
 * identical, so a game's menu code is the same on every platform).
 * What differs is who feeds input and who draws:
 *
 *   each frame, from your window / event loop (SDL, GLFW, Cocoa, ...):
 *       GuiPlatformInput in = { ... };   gui_platform_set_input(&in);
 *       const GuiDrawData *dd = gui_overlay_frame(width, height);   // NULL when off
 *       if (dd) gui_gl_render(dd, width, height);        // OpenGL: gui_gl.h
 *               gui_metal_render(dd, encoder, w, h);     // Metal:  gui_metal.h
 *
 * Nothing here talks to a windowing system, so it builds on any POSIX system.
 */
#ifndef RECOMP_GUI_OVERLAY_PORTABLE_H
#define RECOMP_GUI_OVERLAY_PORTABLE_H

#include "gui.h"
#include "gui_overlay.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int   focused;                  /* the game window has focus */
    float mouse_x, mouse_y;         /* pixels of the render target; < 0: no mouse */
    int   mouse_down[2];            /* left, right */
    float mouse_wheel;              /* notches this frame, + is away from the user */
    int   f1;                       /* F1 held */
    int   l3, r3;                   /* both pad sticks clicked toggles the menu */
    int   nav[GUI_NAV_COUNT];       /* held: arrows / D-pad, Enter / A, Esc / B, PgUp PgDn / LB RB */
} GuiPlatformInput;

void gui_platform_set_input(const GuiPlatformInput *in);

/* Runs one frame of the overlay at the given render-target size. Returns the
 * draw data for your renderer, or NULL while the overlay is off or failed. */
const GuiDrawData *gui_overlay_frame(float width, float height);

#ifdef __cplusplus
}
#endif

#endif
