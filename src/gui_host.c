/*
 * Host glue: the one place the GUI touches your recomp runtime.
 *
 * gui_overlay.c calls recomp_gui_input_captured(1) when the F1 menu opens and
 * (0) when it closes. Make your pad/keyboard layer report "nothing pressed"
 * while captured, so the game does not move under the menu.
 *
 * With RECOMP_GUI_XBOXRECOMP (set by tools/setup.ps1 when the SDK has it) the
 * call goes to the SDK's xbox_InputSetCaptured(), exactly as in True Crime.
 * Without it the flag is only recorded: read recomp_gui_is_captured() or
 * gui_overlay_wants_input() from your input code.
 */
#include "gui_host.h"

#ifdef RECOMP_GUI_XBOXRECOMP
int xbox_InputSetCaptured(int captured);   /* BOOL in the SDK */
#endif

static volatile int s_captured;

void recomp_gui_input_captured(int captured)
{
    s_captured = captured != 0;
#ifdef RECOMP_GUI_XBOXRECOMP
    xbox_InputSetCaptured(captured);
#endif
}

int recomp_gui_is_captured(void) { return s_captured; }
