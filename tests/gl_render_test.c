/*
 * Renders the overlay with the OpenGL renderer into an off-screen EGL pbuffer
 * (needs EGL and a GL 3.3 core driver, Mesa's software one is enough), reads
 * the pixels back and checks that the menu drew something. With a path
 * argument it also writes a .ppm to look at.
 *
 *     cc tests/gl_render_test.c -Isrc build/librecomp_gui.a -lEGL -lm -lpthread
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <EGL/egl.h>

#include "gui_overlay_portable.h"
#include "gui_gl.h"

typedef void (*PFN)(void);
#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)

static void demo_page(void *u)
{
    static int on = 1; static float v = 0.5f;
    (void)u;
    gui_checkbox("God mode", &on);
    gui_slider_float("Gamma", &v, 0.5f, 3.0f, "%.2f");
    if (gui_button("Reload level")) {}
}

static void *loader(const char *n) { return (void *)eglGetProcAddress(n); }

int main(int argc, char **argv)
{
    enum { W = 960, H = 540 };
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint maj, min, ncfg = 0;
    EGLConfig cfg;
    EGLint ca[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
                    EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE };
    EGLint pa[] = { EGL_WIDTH, W, EGL_HEIGHT, H, EGL_NONE };
    EGLint xa[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 3,
                    EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, EGL_NONE };
    EGLSurface surf;
    EGLContext ctx;
    GuiPlatformInput in;
    const GuiDrawData *dd = NULL;
    unsigned char *px;
    void (*glClearColor)(float, float, float, float);
    void (*glClear)(unsigned);
    void (*glReadPixels)(int, int, int, int, unsigned, unsigned, void *);
    long lit = 0, i;
    int f;

    if (!eglInitialize(dpy, &maj, &min)) { fprintf(stderr, "no EGL: skipped\n"); return 0; }
    eglBindAPI(EGL_OPENGL_API);
    if (!eglChooseConfig(dpy, ca, &cfg, 1, &ncfg) || !ncfg) { fprintf(stderr, "no EGL config: skipped\n"); return 0; }
    surf = eglCreatePbufferSurface(dpy, cfg, pa);
    ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, xa);
    if (surf == EGL_NO_SURFACE || ctx == EGL_NO_CONTEXT || !eglMakeCurrent(dpy, surf, surf, ctx)) {
        fprintf(stderr, "no GL 3.3 core context: skipped\n");
        return 0;
    }
    glClearColor = (void (*)(float, float, float, float))eglGetProcAddress("glClearColor");
    glClear = (void (*)(unsigned))eglGetProcAddress("glClear");
    glReadPixels = (void (*)(int, int, int, int, unsigned, unsigned, void *))eglGetProcAddress("glReadPixels");
    CHECK(glClearColor && glClear && glReadPixels);

    gui_overlay_add_page("Cheats", demo_page, NULL);
    gui_overlay_set_title("GL test menu");
    gui_overlay_set_enabled(1);
    gui_overlay_set_menu_open(1);
    memset(&in, 0, sizeof in);
    in.focused = 1; in.mouse_x = in.mouse_y = -1.0f;
    for (f = 0; f < 6; f++) {
        gui_platform_set_input(&in);
        dd = gui_overlay_frame((float)W, (float)H);
        if (!dd) { fprintf(stderr, "no system font: skipped\n"); return 0; }
    }
    CHECK(gui_gl_init(loader, 0));
    glClearColor(0.2f, 0.22f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    gui_gl_render(dd, W, H);

    px = (unsigned char *)malloc((size_t)W * H * 4);
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px);
    for (i = 0; i < (long)W * H; i++)       /* pixels that differ from the clear colour */
        if (abs(px[i * 4] - 51) > 8 || abs(px[i * 4 + 1] - 56) > 8 || abs(px[i * 4 + 2] - 76) > 8) lit++;
    printf("gl render: %ld of %d pixels drawn by the GUI\n", lit, W * H);
    if (argc > 1) {
        FILE *o = fopen(argv[1], "wb");
        if (o) {
            int y;
            fprintf(o, "P6\n%d %d\n255\n", W, H);
            for (y = H - 1; y >= 0; y--)
                for (i = 0; i < W; i++) fwrite(px + ((long)y * W + i) * 4, 1, 3, o);
            fclose(o);
        }
    }
    CHECK(lit > 5000);
    gui_gl_shutdown();
    return 0;
}
