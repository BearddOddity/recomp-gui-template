#ifndef DEBUG_MENU_H
#define DEBUG_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

/* Register pages and turn the F1 menu on. Call once, before the first frame. */
void debug_menu_init(void);

/* Game side: poll from your own thread. */
int debug_god_mode(void);
int debug_take_reload(void);

#ifdef __cplusplus
}
#endif
#endif
