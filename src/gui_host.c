/*
 * Host glue: the one place the GUI touches your recomp runtime.
 *
 * gui_overlay.c calls recomp_gui_input_captured(1) when the F1 menu opens and
 * (0) when it closes, so the game does not move under the menu.
 *
 * Version independence: if the runtime you link has xbox_InputSetCaptured()
 * (the xboxrecomp SDK does in some versions) it is called; if not, the linker
 * binds the name to a no-op (MSVC /alternatename), so the same sources build
 * against any SDK version. Either way, gate your own input on
 * gui_overlay_wants_input() or recomp_gui_is_captured().
 */
#include "gui_host.h"

#ifdef _MSC_VER
void xbox_InputSetCaptured(int captured);          /* BOOL in the SDK */
void recomp_gui_noop_capture(int captured);
#  if defined(_M_IX86)
#    pragma comment(linker, "/alternatename:_xbox_InputSetCaptured=_recomp_gui_noop_capture")
#  else
#    pragma comment(linker, "/alternatename:xbox_InputSetCaptured=recomp_gui_noop_capture")
#  endif
#else
/* GCC/Clang: define RECOMP_GUI_XBOXRECOMP if your runtime has the call. */
#  ifdef RECOMP_GUI_XBOXRECOMP
void xbox_InputSetCaptured(int captured);
#  endif
#endif

void recomp_gui_noop_capture(int captured) { (void)captured; }

static volatile int s_captured;

void recomp_gui_input_captured(int captured)
{
    s_captured = captured != 0;
#if defined(_MSC_VER) || defined(RECOMP_GUI_XBOXRECOMP)
    xbox_InputSetCaptured(captured);
#endif
}

int recomp_gui_is_captured(void) { return s_captured; }
