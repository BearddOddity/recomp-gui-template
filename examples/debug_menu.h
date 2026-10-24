#ifndef DEBUG_MENU_H
#define DEBUG_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RECOMP_GUI_MENU
/* Register pages and turn the F1 menu on. Call once, before the first frame. */
void debug_menu_init(void);
/* Game side: poll from your own thread. */
int debug_god_mode(void);
int debug_take_reload(void);
#else
/* Opt-in: built without -DRECOMP_GUI_MENU (CMake option RECOMP_GUI_MENU=OFF), the menu costs nothing. */
static inline void debug_menu_init(void) {}
static inline int  debug_god_mode(void) { return 0; }
static inline int  debug_take_reload(void) { return 0; }
#endif

#ifdef __cplusplus
}
#endif
#endif
