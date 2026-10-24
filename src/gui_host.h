#ifndef RECOMP_GUI_HOST_H
#define RECOMP_GUI_HOST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Called by the overlay when the menu opens (1) or closes (0). */
void recomp_gui_input_captured(int captured);
/* 1 while the menu owns the pad and keyboard: gate your game input on this. */
int  recomp_gui_is_captured(void);

#ifdef __cplusplus
}
#endif
#endif
