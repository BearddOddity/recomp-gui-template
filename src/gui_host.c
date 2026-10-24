/*
 * Host glue: the one place the GUI touches your recomp runtime.
 *
 * gui_overlay.c calls recomp_gui_input_captured(1) when the F1 menu opens and
 * (0) when it closes. Make your pad/keyboard layer report "nothing pressed"
 * while captured, so the game does not move under the menu.
 *
 * In xboxrecomp this was xbox_InputSetCaptured(); the default below just
 * records the flag. Replace the body with your runtime's call, or read
 * recomp_gui_is_captured() from your input code.
 */
#include "gui_host.h"

static volatile int s_captured;

void recomp_gui_input_captured(int captured) { s_captured = captured != 0; }
int  recomp_gui_is_captured(void)            { return s_captured; }
