/*
 * gui smoke test.
 *
 *   gui_smoke                    core checks, no GPU: font, layout, mouse and
 *                                pad/keyboard navigation, draw data
 *   gui_smoke demo.bmp [ov.bmp]  also draws a widget demo (and the runtime
 *                                overlay) with Direct3D 11 into an off-screen
 *                                1280x720 target and writes it out, to look at
 *
 * Exit status 0 = pass. Without a system font the core checks are skipped.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui.h"

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

static void *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    void *p = NULL;
    long n;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n > 0 && (p = malloc((size_t)n)) && fread(p, 1, (size_t)n, f) != (size_t)n) { free(p); p = NULL; }
    fclose(f);
    *size = (size_t)n;
    return p;
}

static void *system_font(size_t *size)
{
    static const char *paths[] = {
        "C:\\Windows\\Fonts\\segoeuisb.ttf", "C:\\Windows\\Fonts\\segoeui.ttf", "C:\\Windows\\Fonts\\arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
    };
    size_t i;
    for (i = 0; i < sizeof paths / sizeof paths[0]; i++) {
        void *p = read_file(paths[i], size);
        if (p) return p;
    }
    return NULL;
}

/* One frame of a small window; returns what the button and checkbox did. */
static int s_check;
static int frame(float mx, float my, int down, int nav_down, int nav_activate, int *button)
{
    GuiIO *io = gui_io();
    const GuiDrawData *dd;
    int i, toggled;
    io->display_w = 1280; io->display_h = 720; io->delta_time = 1.0f / 60.0f;
    io->mouse_x = mx; io->mouse_y = my; io->mouse_down[0] = down;
    memset(io->nav, 0, sizeof io->nav);
    io->nav[GUI_NAV_DOWN] = nav_down;
    io->nav[GUI_NAV_ACTIVATE] = nav_activate;
    gui_new_frame();
    gui_set_next_window_pos(100, 100, GUI_COND_ALWAYS, 0, 0);
    gui_set_next_window_size(400, 300, GUI_COND_ALWAYS);
    gui_begin("Test", NULL, 0);
    *button = gui_button("Press");
    toggled = gui_checkbox("Check", &s_check);
    gui_text("hello %d", 42);
    gui_end();
    dd = gui_render();
    CHECK(dd->cmd_count > 0 && dd->vtx_count > 0 && dd->idx_count > 0);
    for (i = 0; i < dd->idx_count; i++)
        CHECK(dd->idx[i] < (uint32_t)dd->vtx_count);
    return toggled;
}

static void core_checks(void)
{
    const GuiStyle *st = gui_style();
    float fs = gui_font_size();
    float bx = 100 + st->window_padding.x + 4;
    float by = 100 + (fs + st->frame_padding.y * 2) + st->window_padding.y + 4;
    int pressed, toggled;

    /* Mouse: press and release over the button. */
    frame(-1, -1, 0, 0, 0, &pressed);
    CHECK(!pressed);
    frame(bx, by, 0, 0, 0, &pressed);          /* hover (window under mouse from now on) */
    frame(bx, by, 1, 0, 0, &pressed);          /* down */
    CHECK(!pressed);
    frame(bx, by, 0, 0, 0, &pressed);          /* up over it */
    CHECK(pressed);

    /* Pad/keyboard: activate the selected item (the button, first in the
     * window), step down to the checkbox, activate it. */
    frame(-1, -1, 0, 0, 0, &pressed);
    frame(-1, -1, 0, 0, 1, &pressed);
    CHECK(pressed);
    frame(-1, -1, 0, 0, 0, &pressed);
    frame(-1, -1, 0, 1, 0, &pressed);          /* down: next frame the checkbox has it */
    frame(-1, -1, 0, 0, 0, &pressed);
    toggled = frame(-1, -1, 0, 0, 1, &pressed);
    CHECK(toggled && s_check == 1 && !pressed);
    printf("core: ok (font %.0f px)\n", fs);
}

#ifdef _WIN32
#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include "gui_d3d11.h"
#include "gui_overlay.h"

static void write_bmp(const char *path, const uint8_t *rgba, int w, int h, int pitch)
{
    FILE *f = fopen(path, "wb");
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER ih;
    int y, x;
    if (!f) { fprintf(stderr, "cannot write %s\n", path); exit(1); }
    memset(&fh, 0, sizeof fh); memset(&ih, 0, sizeof ih);
    fh.bfType = 0x4D42;
    fh.bfOffBits = sizeof fh + sizeof ih;
    fh.bfSize = fh.bfOffBits + (DWORD)(w * h * 3);
    ih.biSize = sizeof ih; ih.biWidth = w; ih.biHeight = -h; ih.biPlanes = 1; ih.biBitCount = 24;
    fwrite(&fh, sizeof fh, 1, f);
    fwrite(&ih, sizeof ih, 1, f);
    for (y = 0; y < h; y++) {
        const uint8_t *row = rgba + (size_t)y * (size_t)pitch;
        for (x = 0; x < w; x++) { uint8_t px[3] = { row[x * 4 + 2], row[x * 4 + 1], row[x * 4 + 0] }; fwrite(px, 3, 1, f); }
        /* 1280 * 3 is a multiple of 4: no row padding */
    }
    fclose(f);
    printf("wrote %s\n", path);
}

static ID3D11Device *s_dev;
static ID3D11DeviceContext *s_ctx;
static ID3D11Texture2D *s_rt, *s_stage;
static ID3D11RenderTargetView *s_rtv;

static void d3d_setup(void)
{
    D3D11_TEXTURE2D_DESC td;
    if (FAILED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &s_dev, NULL, &s_ctx))
            && FAILED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &s_dev, NULL, &s_ctx))) {
        fprintf(stderr, "no D3D11 device\n");
        exit(1);
    }
    memset(&td, 0, sizeof td);
    td.Width = 1280; td.Height = 720; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_RENDER_TARGET;
    CHECK(SUCCEEDED(ID3D11Device_CreateTexture2D(s_dev, &td, NULL, &s_rt)));
    CHECK(SUCCEEDED(ID3D11Device_CreateRenderTargetView(s_dev, (ID3D11Resource *)s_rt, NULL, &s_rtv)));
    td.Usage = D3D11_USAGE_STAGING; td.BindFlags = 0; td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    CHECK(SUCCEEDED(ID3D11Device_CreateTexture2D(s_dev, &td, NULL, &s_stage)));
}

/* A stand-in for a game frame: a dark gradient. */
static void clear_scene(void)
{
    float c[4] = { 0.20f, 0.24f, 0.30f, 1.0f };
    ID3D11DeviceContext_ClearRenderTargetView(s_ctx, s_rtv, c);
}

static void save(const char *path)
{
    D3D11_MAPPED_SUBRESOURCE m;
    ID3D11DeviceContext_CopyResource(s_ctx, (ID3D11Resource *)s_stage, (ID3D11Resource *)s_rt);
    CHECK(SUCCEEDED(ID3D11DeviceContext_Map(s_ctx, (ID3D11Resource *)s_stage, 0, D3D11_MAP_READ, 0, &m)));
    write_bmp(path, (const uint8_t *)m.pData, 1280, 720, (int)m.RowPitch);
    ID3D11DeviceContext_Unmap(s_ctx, (ID3D11Resource *)s_stage, 0);
}

static void demo_window(int frame_no)
{
    static int check = 1, radio = 1, level = 3, combo = 1, open = 1;
    static float gamma = 1.4f;
    static const char *modes[] = { "Original", "16:9", "16:10", "21:9", "Window" };
    static float wave[90];
    int i;
    for (i = 0; i < 90; i++) wave[i] = 16.6f + (float)((i * 37 + frame_no) % 11) - 5.0f;
    gui_set_next_window_pos(60, 60, GUI_COND_FIRST_USE, 0, 0);
    gui_set_next_window_size(560, 600, GUI_COND_FIRST_USE);
    gui_begin("Moonlight demo", &open, 0);
    gui_begin_tab_bar("##t");
    gui_tab_item("Widgets");
    gui_tab_item("Graphics");
    gui_tab_item("World");
    gui_end_tab_bar();
    gui_spacing();
    gui_text("Immediate-mode GUI in C, frame %d", frame_no);
    gui_text_disabled("Disabled text looks like this");
    if (gui_button("Reload level")) {}
    gui_same_line(-1);
    gui_button("Capture frame");
    gui_checkbox("Vertex programs on GPU", &check);
    if (gui_radio_button("CPU", radio == 0)) radio = 0;
    gui_same_line(-1);
    if (gui_radio_button("GPU", radio == 1)) radio = 1;
    gui_slider_float("Gamma", &gamma, 0.5f, 3.0f, "%.2f");
    gui_slider_int("Level", &level, 0, 24, NULL);
    gui_input_int("Sub-level", &level, 1);
    gui_combo("Widescreen", &combo, modes, 5);
    gui_progress_bar(0.62f, 0, 0, NULL);
    gui_plot_lines("Frame ms", wave, 90, 0, NULL, 0, 0, 0, 0);
    gui_plot_histogram("Histogram", wave, 30, 0, NULL, 0, 0, 0, 0);
    if (gui_collapsing_header("Levels", 1)) {
        for (i = 0; i < 4; i++) {
            char b[32];
            snprintf(b, sizeof b, "%2d  district_%d", i, i);
            gui_selectable(b, i == 1);
        }
    }
    gui_collapsing_header("HUD layout", 0);
    gui_separator();
    gui_bullet_text("Bullet text");
    gui_label_text("Label", "value 123");
    gui_end();
}

static void page_cheats(void *user)
{
    static int god = 1, ammo;
    (void)user;
    gui_checkbox("God mode", &god);
    gui_checkbox("Unlimited ammo", &ammo);
    gui_button("Reload level");
}

static int s_tracker = 1;
static void window_tracker(void *user)
{
    (void)user;
    if (!s_tracker) return;
    gui_set_next_window_pos(900, 160, GUI_COND_FIRST_USE, 0, 0);
    gui_begin("Tracker", &s_tracker, GUI_WINDOW_AUTO_RESIZE);
    gui_text("A title's own window,");
    gui_text("drawn every frame.");
    gui_end();
}

static void render_checks(const char *demo_path, const char *overlay_path)
{
    int f;
    d3d_setup();
    CHECK(gui_d3d11_init(s_dev, s_ctx));
    for (f = 0; f < 6; f++) {
        GuiIO *io = gui_io();
        io->display_w = 1280; io->display_h = 720; io->delta_time = 1.0f / 60.0f;
        io->mouse_x = io->mouse_y = -1; io->mouse_down[0] = 0;
        memset(io->nav, 0, sizeof io->nav);
        io->nav[GUI_NAV_DOWN] = f == 3;         /* show the navigation highlight */
        gui_new_frame();
        demo_window(f);
        clear_scene();
        gui_d3d11_render(gui_render(), s_rtv);
    }
    save(demo_path);
    gui_d3d11_shutdown();

    if (overlay_path) {
        /* The real overlay, with a title's page added. */
        gui_shutdown();
        _putenv("RECOMP_GUI=on");
        gui_overlay_set_title("Title debug menu");
        gui_overlay_add_page("Cheats", page_cheats, NULL);
        gui_overlay_add_window("Tracker", window_tracker, NULL);
        gui_overlay_set_menu_open(1);
        gui_overlay_notice("capturing the next frame");
        for (f = 0; f < 8; f++) {
            clear_scene();
            gui_overlay_frame_d3d11(s_dev, s_ctx, s_rtv, NULL);
            Sleep(70);
        }
        CHECK(gui_ready());
        save(overlay_path);
        /* A scripted press reaches the menu like a key: B / Esc closes it. */
        CHECK(gui_overlay_menu_open());
        gui_overlay_press(GUI_NAV_CANCEL);
        for (f = 0; f < 2; f++)
            gui_overlay_frame_d3d11(s_dev, s_ctx, s_rtv, NULL);
        CHECK(!gui_overlay_menu_open());
    }
}
#endif

int main(int argc, char **argv)
{
    size_t size = 0;
    void *ttf = system_font(&size);
    if (!ttf) {
        printf("SKIP: no system font\n");
        return 0;
    }
    CHECK(gui_init(ttf, size, 16.0f));
    {
        const uint8_t *px; int w, h;
        gui_font_atlas(&px, &w, &h);
        CHECK(px && w >= 256 && h == w + 2 && px[(size_t)w * (size_t)w] == 255);
    }
    core_checks();
#ifdef _WIN32
    if (argc > 1) {
        gui_init(ttf, size, 16.0f);
        render_checks(argv[1], argc > 2 ? argv[2] : NULL);
    }
#else
    (void)argc; (void)argv;
#endif
    free(ttf);
    printf("PASS\n");
    return 0;
}
