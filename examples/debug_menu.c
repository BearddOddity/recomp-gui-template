/*
 * F1 debug menu scaffold. Copy into your game, rename, add pages.
 *
 * Call debug_menu_init() once at start-up (any thread, before or after the
 * renderer exists). Press F1 or click both sticks (L3+R3) in game to open.
 * Then, once per presented frame, just before Present(), call
 *     gui_overlay_frame_d3d11(device, context, backbuffer_rtv, hwnd);
 */
#include <stdlib.h>
#include "gui.h"
#include "gui_overlay.h"
#include "gui_host.h"

/* State your game code reads. Pages only set flags; the game thread acts. */
static int   g_god_mode;
static int   g_want_reload;
static float g_gamma = 1.0f;
static int   g_level;

/* A tab in the F1 window. Runs between gui_begin/gui_end of the menu. */
static void page_cheats(void *user)
{
    (void)user;
    gui_checkbox("God mode", &g_god_mode);
    if (gui_button("Reload level")) g_want_reload = 1;
    gui_slider_int("Level", &g_level, 0, 20, "%d");
}

static void page_graphics(void *user)
{
    (void)user;
    if (gui_slider_float("Gamma", &g_gamma, 0.5f, 3.0f, "%.2f"))
        gui_overlay_notice("gamma %.2f", g_gamma);
    if (gui_collapsing_header("Details", 0))
        gui_text_disabled("put renderer toggles here");
}

/* A window drawn every frame, menu open or not. */
static void window_tracker(void *user)
{
    (void)user;
    gui_set_next_window_pos(10, 120, GUI_COND_FIRST_USE, 0, 0);
    gui_begin("Tracker", NULL, GUI_WINDOW_AUTO_RESIZE | GUI_WINDOW_NO_INPUTS);
    gui_text("level %d  god %s", g_level, g_god_mode ? "on" : "off");
    gui_end();
}

void debug_menu_init(void)
{
    /* As True Crime does (gfx_d3d11.c): on by default, unless RECOMP_GUI is set (then it decides). */
    if (!getenv("RECOMP_GUI"))
        gui_overlay_set_enabled(1);
    gui_overlay_set_title("My Game debug menu");
    gui_overlay_add_page("Cheats", page_cheats, 0);
    gui_overlay_add_page("Graphics", page_graphics, 0);
    gui_overlay_add_window("Tracker", window_tracker, 0);
}

/* Game side: poll these from your own thread. */
int debug_god_mode(void)      { return g_god_mode; }
int debug_take_reload(void)   { int r = g_want_reload; g_want_reload = 0; return r; }

/* In your pad-reading function (True Crime: recomp_manual.c, port 0). Zero the
 * state while the menu owns the pad, so the game does not move under it:
 *     if (port == 0 && gui_overlay_wants_input()) memset(&st.Gamepad, 0, sizeof st.Gamepad);
 * Also skip your own mouse-look while it is true (True Crime: mouse_release()).
 * In your window procedure, forward the wheel (True Crime: gfx_d3d11.c):
 *     gui_overlay_wndproc(hwnd, msg, wparam, lparam);
 */
