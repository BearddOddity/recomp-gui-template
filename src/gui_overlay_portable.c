/*
 * Overlay for POSIX systems: performance panel, F1 menu, title windows and
 * notices. A port of gui_overlay.c's logic without Win32, XInput or D3D11; the
 * host passes input in (gui_platform_set_input) and draws the result itself.
 * See gui_overlay_portable.h.
 */
#define _GNU_SOURCE
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "gui.h"
#include "gui_host.h"
#include "gui_overlay.h"
#include "gui_overlay_portable.h"

#define MAX_PAGES   32
#define MAX_NOTICES 4
#define FT_COUNT    120

typedef struct {
    char      name[48];
    GuiPageFn fn;
    void     *user;
} Page;

static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static Page   s_pages[MAX_PAGES], s_windows[MAX_PAGES];
static int    s_npages, s_nwindows;
static char   s_title[64] = "Debug";
static struct { char text[128]; double until; } s_notes[MAX_NOTICES];
static int    s_note_next;

static int    s_enabled = -1, s_perf = -1, s_failed, s_focus_menu;
static volatile int s_menu_open;
static unsigned s_press;                     /* scripted presses, bit per GUI_NAV_* */
static float  s_scale = 1.0f;
static GuiPlatformInput s_in;

/* performance */
static double s_last_frame, s_last_sample;
static float  s_ft[FT_COUNT];
static int    s_ft_at;
static unsigned s_frames;
static double s_ms_sum, s_cpu_last;
static double s_fps, s_ms, s_cpu, s_ram;

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static int env_on(const char *name, int def)
{
    const char *v = getenv(name);
    if (!v || !*v) return def;
    return !(!strcasecmp(v, "off") || !strcmp(v, "0") || !strcasecmp(v, "false") || !strcasecmp(v, "no"));
}

/* ── Public switches ──────────────────────────────────────────── */

static void add_entry(Page *list, int *n, const char *name, GuiPageFn draw, void *user)
{
    int i;
    if (!name || !draw) return;
    pthread_mutex_lock(&s_lock);
    for (i = 0; i < *n && strcmp(list[i].name, name); i++) ;
    if (i < MAX_PAGES) {
        snprintf(list[i].name, sizeof list[i].name, "%s", name);
        list[i].fn = draw;
        list[i].user = user;
        if (i == *n) (*n)++;
    }
    pthread_mutex_unlock(&s_lock);
}

void gui_overlay_add_page(const char *name, GuiPageFn draw, void *user) { add_entry(s_pages, &s_npages, name, draw, user); }
void gui_overlay_add_window(const char *name, GuiPageFn draw, void *user) { add_entry(s_windows, &s_nwindows, name, draw, user); }

void gui_overlay_set_title(const char *title)
{
    pthread_mutex_lock(&s_lock);
    snprintf(s_title, sizeof s_title, "%s", title && *title ? title : "Debug");
    pthread_mutex_unlock(&s_lock);
}

int gui_overlay_enabled(void)
{
    if (s_enabled < 0) s_enabled = env_on("RECOMP_GUI", 0);
    return s_enabled && !s_failed;
}

void gui_overlay_set_enabled(int on)
{
    s_enabled = on ? 1 : 0;
    if (!on) gui_overlay_set_menu_open(0);
}

void gui_overlay_set_menu_open(int open)
{
    open = open && gui_overlay_enabled();
    if (open && !s_menu_open) s_focus_menu = 1;
    if (!open != !s_menu_open) fprintf(stderr, "[GUI] menu %s\n", open ? "open" : "closed");
    s_menu_open = open;
    recomp_gui_input_captured(open);
}

int gui_overlay_menu_open(void) { return s_menu_open != 0; }
int gui_overlay_wants_input(void) { return s_menu_open != 0 && gui_overlay_enabled(); }
int gui_overlay_wants_mouse(void) { return gui_overlay_enabled() && gui_ready() && (s_menu_open || gui_io()->want_mouse); }

int gui_overlay_perf_shown(void)
{
    if (s_perf < 0) s_perf = env_on("RECOMP_OSD", 1);
    return s_perf;
}
void gui_overlay_show_perf(int on) { s_perf = on ? 1 : 0; }

void gui_overlay_notice(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    fprintf(stderr, "[GUI] %s\n", buf);
    pthread_mutex_lock(&s_lock);
    snprintf(s_notes[s_note_next].text, sizeof s_notes[0].text, "%s", buf);
    s_notes[s_note_next].until = now_s() + 4.0;
    s_note_next = (s_note_next + 1) % MAX_NOTICES;
    pthread_mutex_unlock(&s_lock);
}

void gui_overlay_press(int nav)
{
    if (nav >= 0 && nav < GUI_NAV_COUNT) __atomic_or_fetch(&s_press, 1u << nav, __ATOMIC_SEQ_CST);
}

void gui_platform_set_input(const GuiPlatformInput *in)
{
    if (in) s_in = *in;
}

/* ── Start-up ─────────────────────────────────────────────────── */

static void *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    void *p = NULL;
    long n = 0;
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) > 0 && fseek(f, 0, SEEK_SET) == 0
            && (p = malloc((size_t)n)) != NULL && fread(p, 1, (size_t)n, f) != (size_t)n) {
        free(p);
        p = NULL;
    }
    fclose(f);
    if (p) *size = (size_t)n;
    return p;
}

static int font_init(float target_h)
{
    static const char *faces[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf", "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf", "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Verdana.ttf", "/System/Library/Fonts/Supplemental/Tahoma.ttf",
    };
    const char *env = getenv("RECOMP_GUI_FONT");
    const char *used = NULL;
    void *ttf = NULL;
    size_t size = 0, i;
    int ok;

    if (env && *env && (ttf = read_file(env, &size)) != NULL) used = env;
    for (i = 0; !ttf && i < sizeof faces / sizeof faces[0]; i++)
        if ((ttf = read_file(faces[i], &size)) != NULL) used = faces[i];
    if (!ttf) {
        fprintf(stderr, "[GUI] no font found (set RECOMP_GUI_FONT to a .ttf); overlay off\n");
        return 0;
    }
    {
        const char *v = getenv("RECOMP_GUI_SCALE");
        int forced = v && atof(v) > 0.0;
        s_scale = forced ? (float)atof(v) : target_h / 720.0f;
        if (s_scale < 1.0f && !forced) s_scale = 1.0f;
        if (s_scale > 4.0f) s_scale = 4.0f;
    }
    ok = gui_init(ttf, size, 16.0f * s_scale);
    free(ttf);
    if (!ok) fprintf(stderr, "[GUI] cannot read font %s; overlay off\n", used);
    else fprintf(stderr, "[GUI] overlay ready: %s, %.0f px\n", used, gui_font_size());
    return ok;
}

/* ── Performance ──────────────────────────────────────────────── */

static double cpu_seconds(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec * 1e-6
         + (double)ru.ru_stime.tv_sec + (double)ru.ru_stime.tv_usec * 1e-6;
}

static double resident_mb(void)
{
#ifdef __APPLE__
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t cnt = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &cnt) == KERN_SUCCESS)
        return (double)info.resident_size / (1024.0 * 1024.0);
    return 0.0;
#else
    FILE *f = fopen("/proc/self/statm", "r");
    long total = 0, res = 0;
    double mb = 0.0;
    if (f) {
        if (fscanf(f, "%ld %ld", &total, &res) == 2)
            mb = (double)res * (double)sysconf(_SC_PAGESIZE) / (1024.0 * 1024.0);
        fclose(f);
    }
    return mb;
#endif
}

static float perf_tick(void)
{
    double now = now_s(), dt, since;
    if (s_last_frame == 0.0) s_last_frame = s_last_sample = now;
    dt = now - s_last_frame;
    s_last_frame = now;
    s_frames++;
    s_ms_sum += dt * 1000.0;
    s_ft[s_ft_at] = (float)(dt * 1000.0);
    s_ft_at = (s_ft_at + 1) % FT_COUNT;
    since = now - s_last_sample;
    if (since >= 0.5 && (gui_overlay_perf_shown() || s_menu_open)) {
        double cpu = cpu_seconds();
        long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
        if (ncpu < 1) ncpu = 1;
        s_fps = s_frames / since;
        s_ms = s_frames ? s_ms_sum / s_frames : 0.0;
        s_cpu = s_cpu_last > 0.0 ? (cpu - s_cpu_last) / since * 100.0 / (double)ncpu : 0.0;
        s_cpu_last = cpu;
        s_ram = resident_mb();
        {   /* the same numbers in the log every 5 s, for unattended runs */
            static unsigned logged;
            if (gui_overlay_perf_shown() && logged++ % 10 == 0) {
                fprintf(stderr, "[OSD] FPS %5.1f   %5.1f ms  CPU %.0f%%  RAM %.0f MB\n", s_fps, s_ms, s_cpu, s_ram);
                fflush(stderr);
            }
        }
        s_frames = 0;
        s_ms_sum = 0.0;
        s_last_sample = now;
    }
    return (float)dt;
}

static void perf_lines(void) { gui_text("FPS %5.1f   %5.1f ms", s_fps, s_ms); }
static void perf_resources(void) { gui_text("CPU %4.0f%%   RAM %5.0f MB", s_cpu, s_ram); }

static void frame_graph(float w, float h)
{
    float top = 33.4f;
    int i;
    for (i = 0; i < FT_COUNT; i++) if (s_ft[i] * 1.1f > top) top = s_ft[i] * 1.1f;
    gui_plot_lines("##frametime", s_ft, FT_COUNT, s_ft_at, NULL, 0.0f, top, w, h);
}

/* ── Windows ──────────────────────────────────────────────────── */

static void draw_perf_panel(void)
{
    float fs = gui_font_size();
    gui_set_next_window_pos(10.0f * s_scale, 10.0f * s_scale, GUI_COND_ALWAYS, 0.0f, 0.0f);
    gui_set_next_window_bg_alpha(0.82f);
    gui_begin("##perf", NULL, GUI_WINDOW_NO_TITLE_BAR | GUI_WINDOW_NO_INPUTS | GUI_WINDOW_AUTO_RESIZE
              | GUI_WINDOW_NO_NAV | GUI_WINDOW_NO_SCROLLBAR);
    perf_lines();
    frame_graph(fs * 12.0f, fs * 2.2f);
    perf_resources();
    gui_end();
}

static void draw_notices(float bw, float bh)
{
    char text[MAX_NOTICES][128];
    double now = now_s();
    int i, n = 0;
    pthread_mutex_lock(&s_lock);
    for (i = 0; i < MAX_NOTICES; i++) {
        int k = (s_note_next + i) % MAX_NOTICES;     /* oldest first */
        if (s_notes[k].until > now) memcpy(text[n++], s_notes[k].text, sizeof text[0]);
    }
    pthread_mutex_unlock(&s_lock);
    if (!n) return;
    gui_set_next_window_pos(bw * 0.5f, bh - 40.0f * s_scale, GUI_COND_ALWAYS, 0.5f, 1.0f);
    gui_set_next_window_bg_alpha(0.88f);
    gui_begin("##notices", NULL, GUI_WINDOW_NO_TITLE_BAR | GUI_WINDOW_NO_INPUTS | GUI_WINDOW_AUTO_RESIZE
              | GUI_WINDOW_NO_NAV | GUI_WINDOW_NO_SCROLLBAR);
    for (i = 0; i < n; i++) gui_text("%s", text[i]);
    gui_end();
}

static void performance_page(void)
{
    int on = gui_overlay_perf_shown();
    if (gui_checkbox("Show the performance panel", &on)) gui_overlay_show_perf(on);
    gui_spacing();
    perf_lines();
    perf_resources();
    gui_spacing();
    gui_text_disabled("Frame time, last %d frames (ms)", FT_COUNT);
    frame_graph(gui_content_width(), gui_font_size() * 4.0f);
    gui_spacing();
    gui_separator();
    gui_spacing();
    gui_text_disabled("F1 or L3+R3: open / close this menu");
    gui_text_disabled("Arrows or D-pad: move     Enter or A: select");
    gui_text_disabled("Left / right: adjust     PgUp / PgDn or LB / RB: tab");
    gui_text_disabled("Esc or B: close");
}

static void draw_menu(float bw, float bh)
{
    Page pages[MAX_PAGES];
    char title[64];
    float fs = gui_font_size();
    int open = 1, n, i, sel = -2;

    pthread_mutex_lock(&s_lock);
    n = s_npages;
    memcpy(pages, s_pages, sizeof(Page) * (size_t)n);
    snprintf(title, sizeof title, "%s", s_title);
    pthread_mutex_unlock(&s_lock);

    gui_set_next_window_pos(40.0f * s_scale, 150.0f * s_scale, GUI_COND_FIRST_USE, 0.0f, 0.0f);
    gui_set_next_window_size(fminf(bw - 80.0f * s_scale, fs * 32.0f), fminf(bh - 190.0f * s_scale, fs * 28.0f),
                             GUI_COND_FIRST_USE);
    if (s_focus_menu) {
        gui_set_next_window_focus();
        s_focus_menu = 0;
    }
    gui_begin(title, &open, 0);
    gui_begin_tab_bar("##pages");
    if (gui_tab_item("Performance")) sel = -1;
    for (i = 0; i < n; i++)
        if (gui_tab_item(pages[i].name)) sel = i;
    gui_end_tab_bar();
    gui_spacing();
    {   /* in the log, so unattended runs can follow the menu */
        static int logged = -100;
        if (sel != logged) {
            fprintf(stderr, "[GUI] page %s\n", sel == -1 ? "Performance" : sel >= 0 ? pages[sel].name : "(none)");
            logged = sel;
        }
    }
    if (sel == -1) {
        performance_page();
    } else if (sel >= 0) {
        gui_push_id(pages[sel].name);
        pages[sel].fn(pages[sel].user);
        gui_pop_id();
    }
    if (gui_nav_pressed(GUI_NAV_CANCEL)) open = 0;
    gui_end();
    if (!open) gui_overlay_set_menu_open(0);
}

/* ── Input ────────────────────────────────────────────────────── */

static void apply_input(float bw, float bh, float dt)
{
    GuiIO *io = gui_io();
    static int prev_f1, prev_combo;
    int f1 = s_in.focused && s_in.f1, combo = s_in.l3 && s_in.r3, k;
    unsigned p;

    if ((f1 && !prev_f1) || (combo && !prev_combo))
        gui_overlay_set_menu_open(!s_menu_open);
    prev_f1 = f1;
    prev_combo = combo;

    io->display_w = bw;
    io->display_h = bh;
    io->delta_time = dt;
    io->mouse_x = io->mouse_y = -1.0f;
    io->mouse_down[0] = io->mouse_down[1] = 0;
    io->mouse_wheel = s_in.mouse_wheel;
    s_in.mouse_wheel = 0.0f;                          /* a wheel notch counts once */
    memset(io->nav, 0, sizeof io->nav);
    if (s_in.focused && s_in.mouse_x >= 0.0f && s_in.mouse_y >= 0.0f) {
        io->mouse_x = s_in.mouse_x;
        io->mouse_y = s_in.mouse_y;
        io->mouse_down[0] = s_in.mouse_down[0];
        io->mouse_down[1] = s_in.mouse_down[1];
    }
    if (!s_menu_open) return;
    for (k = 0; k < GUI_NAV_COUNT; k++) io->nav[k] = s_in.nav[k] != 0;
    p = __atomic_exchange_n(&s_press, 0u, __ATOMIC_SEQ_CST);
    for (k = 0; k < GUI_NAV_COUNT; k++)
        if (p & (1u << k)) io->nav[k] = 1;
}

/* ── Frame ────────────────────────────────────────────────────── */

const GuiDrawData *gui_overlay_frame(float bw, float bh)
{
    float dt;
    if (!gui_overlay_enabled() || bw < 1.0f || bh < 1.0f)
        return NULL;
    if (!gui_ready() && !font_init(bh)) {
        s_failed = 1;
        return NULL;
    }
    dt = perf_tick();
    apply_input(bw, bh, dt);
    gui_new_frame();
    if (gui_overlay_perf_shown())
        draw_perf_panel();
    {   /* the title's own windows */
        Page w[MAX_PAGES];
        int i, n;
        pthread_mutex_lock(&s_lock);
        n = s_nwindows;
        memcpy(w, s_windows, sizeof(Page) * (size_t)n);
        pthread_mutex_unlock(&s_lock);
        for (i = 0; i < n; i++)
            w[i].fn(w[i].user);
    }
    draw_notices(bw, bh);
    if (s_menu_open)
        draw_menu(bw, bh);
    return gui_render();
}
