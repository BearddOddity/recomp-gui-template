/*
 * OpenGL 3.3 core (and OpenGL ES 3.0) renderer for gui.h draw data.
 *
 * No GL headers and no loader library needed: pass the function loader your
 * window system already has (SDL_GL_GetProcAddress, glfwGetProcAddress,
 * eglGetProcAddress, glXGetProcAddress, ...). Call with a current GL context.
 * All state it touches is saved first and restored after.
 */
#ifndef RECOMP_GUI_GL_H
#define RECOMP_GUI_GL_H

#include "gui.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*GuiGLLoader)(const char *name);

/* Compiles the shaders and uploads the font atlas (gui_init() must have run).
 * Returns 0 on failure. Safe to call every frame; later calls do nothing.
 * es: 1 for OpenGL ES 3.0 / WebGL2 style GLSL ("#version 300 es"). */
int  gui_gl_init(GuiGLLoader load, int es);
void gui_gl_shutdown(void);
/* fb_w / fb_h: size of the framebuffer in pixels. */
void gui_gl_render(const GuiDrawData *dd, int fb_w, int fb_h);

#ifdef __cplusplus
}
#endif
#endif
