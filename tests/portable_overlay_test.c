/* Portable overlay check, no window and no GPU: enable it, press F1 in the
 * input, and see the menu come out as draw data; run a page and a notice;
 * close it with Esc. Exit 0 = pass. Skips (exit 0) when no system font exists. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_overlay_portable.h"

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

static int s_page_runs, s_win_runs;
static void page(void *u) { (void)u; s_page_runs++; gui_text("hello from a page"); }
static void win(void *u)  { (void)u; s_win_runs++;  gui_begin("Tracker", NULL, GUI_WINDOW_AUTO_RESIZE); gui_text("tick"); gui_end(); }

static const GuiDrawData *frame(const GuiPlatformInput *in)
{
    gui_platform_set_input(in);
    return gui_overlay_frame(1280.0f, 720.0f);
}

int main(void)
{
    GuiPlatformInput in;
    const GuiDrawData *dd;
    int i, base;

    memset(&in, 0, sizeof in);
    in.focused = 1; in.mouse_x = in.mouse_y = -1.0f;
    CHECK(gui_overlay_frame(1280, 720) == NULL);              /* off until enabled */

    gui_overlay_add_page("Test", page, NULL);
    gui_overlay_add_window("Tracker", win, NULL);
    gui_overlay_set_enabled(1);
    dd = frame(&in);
    for (i = 0; dd && i < 3; i++) dd = frame(&in);             /* auto-sized windows show from the 2nd frame */
    if (!dd) { fprintf(stderr, "no system font: skipped\n"); return 0; }
    CHECK(dd->vtx_count > 0 && dd->cmd_count > 0);            /* perf panel */
    CHECK(s_win_runs >= 1 && !gui_overlay_menu_open());
    base = dd->vtx_count;

    in.f1 = 1; frame(&in); in.f1 = 0;                          /* F1 down */
    CHECK(gui_overlay_menu_open() && gui_overlay_wants_input());
    for (i = 0; i < 3; i++) dd = frame(&in);
    CHECK(dd->vtx_count > base);                               /* the menu window adds geometry */
    gui_overlay_press(GUI_NAV_TAB_NEXT);                       /* Performance -> Test */
    for (i = 0; i < 3; i++) dd = frame(&in);
    CHECK(s_page_runs > 0);

    gui_overlay_notice("a notice");
    dd = frame(&in);
    CHECK(dd->vtx_count > 0);

    in.nav[GUI_NAV_CANCEL] = 1; frame(&in); in.nav[GUI_NAV_CANCEL] = 0; frame(&in);
    CHECK(!gui_overlay_menu_open());                           /* Esc closes it */
    printf("portable overlay: ok (%d vertices with menu, %d without)\n", dd->vtx_count, base);
    return 0;
}
