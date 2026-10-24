/*
 * Draw the overlay by hooking IDXGISwapChain::Present: no change to the game,
 * the SDK or its version. See gui_present_hook.c. Use it only when the game does
 * not already call gui_overlay_frame_d3d11() itself.
 */
#ifndef RECOMP_GUI_PRESENT_HOOK_H
#define RECOMP_GUI_PRESENT_HOOK_H

#ifdef __cplusplus
extern "C" {
#endif

int  gui_present_hook_install(void);   /* 1 on success (or already installed) */
void gui_present_hook_remove(void);
int  gui_present_hook_count(void);     /* frames the overlay was drawn through the hook */

#ifdef __cplusplus
}
#endif
#endif
