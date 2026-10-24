/*
 * OpenGL 3.3 core / ES 3.0 renderer for gui.h draw data. See gui_gl.h.
 * Declares the few GL types and constants it needs itself and loads every
 * function through the caller's loader, so it needs no GL headers or libraries.
 */
#include <stdio.h>
#include <string.h>

#include "gui_gl.h"

#ifdef _WIN32
#define GLAPI __stdcall
#else
#define GLAPI
#endif

typedef unsigned int GLenum, GLuint, GLbitfield;
typedef int GLint, GLsizei;
typedef unsigned char GLboolean;
typedef float GLfloat;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_BLEND 0x0BE2
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_SCISSOR_TEST 0x0C11
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE 1
#define GL_FUNC_ADD 0x8006
#define GL_TRIANGLES 0x0004
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_UNSIGNED_BYTE 0x1401
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW 0x88E0
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_LINEAR 0x2601
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_RED 0x1903
#define GL_R8 0x8229
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_VIEWPORT 0x0BA2
#define GL_SCISSOR_BOX 0x0C10
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_BLEND_EQUATION_RGB 0x8009
#define GL_BLEND_EQUATION_ALPHA 0x883D

typedef void (GLAPI *PFN_Enable)(GLenum);
typedef void (GLAPI *PFN_Disable)(GLenum);
typedef GLboolean (GLAPI *PFN_IsEnabled)(GLenum);
typedef void (GLAPI *PFN_GetIntegerv)(GLenum, GLint *);
typedef void (GLAPI *PFN_BlendEquation)(GLenum);
typedef void (GLAPI *PFN_BlendFuncSeparate)(GLenum, GLenum, GLenum, GLenum);
typedef void (GLAPI *PFN_Viewport)(GLint, GLint, GLsizei, GLsizei);
typedef void (GLAPI *PFN_Scissor)(GLint, GLint, GLsizei, GLsizei);
typedef void (GLAPI *PFN_ColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);
typedef GLuint (GLAPI *PFN_CreateShader)(GLenum);
typedef void (GLAPI *PFN_ShaderSource)(GLuint, GLsizei, const GLchar *const *, const GLint *);
typedef void (GLAPI *PFN_CompileShader)(GLuint);
typedef void (GLAPI *PFN_GetShaderiv)(GLuint, GLenum, GLint *);
typedef void (GLAPI *PFN_GetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef GLuint (GLAPI *PFN_CreateProgram)(void);
typedef void (GLAPI *PFN_AttachShader)(GLuint, GLuint);
typedef void (GLAPI *PFN_LinkProgram)(GLuint);
typedef void (GLAPI *PFN_GetProgramiv)(GLuint, GLenum, GLint *);
typedef void (GLAPI *PFN_DeleteShader)(GLuint);
typedef void (GLAPI *PFN_DeleteProgram)(GLuint);
typedef void (GLAPI *PFN_UseProgram)(GLuint);
typedef GLint (GLAPI *PFN_GetUniformLocation)(GLuint, const GLchar *);
typedef void (GLAPI *PFN_Uniform1i)(GLint, GLint);
typedef void (GLAPI *PFN_Uniform2f)(GLint, GLfloat, GLfloat);
typedef void (GLAPI *PFN_GenVertexArrays)(GLsizei, GLuint *);
typedef void (GLAPI *PFN_BindVertexArray)(GLuint);
typedef void (GLAPI *PFN_DeleteVertexArrays)(GLsizei, const GLuint *);
typedef void (GLAPI *PFN_GenBuffers)(GLsizei, GLuint *);
typedef void (GLAPI *PFN_BindBuffer)(GLenum, GLuint);
typedef void (GLAPI *PFN_BufferData)(GLenum, GLsizeiptr, const void *, GLenum);
typedef void (GLAPI *PFN_DeleteBuffers)(GLsizei, const GLuint *);
typedef void (GLAPI *PFN_EnableVertexAttribArray)(GLuint);
typedef void (GLAPI *PFN_VertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
typedef void (GLAPI *PFN_GenTextures)(GLsizei, GLuint *);
typedef void (GLAPI *PFN_BindTexture)(GLenum, GLuint);
typedef void (GLAPI *PFN_ActiveTexture)(GLenum);
typedef void (GLAPI *PFN_TexParameteri)(GLenum, GLenum, GLint);
typedef void (GLAPI *PFN_TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
typedef void (GLAPI *PFN_PixelStorei)(GLenum, GLint);
typedef void (GLAPI *PFN_DeleteTextures)(GLsizei, const GLuint *);
typedef void (GLAPI *PFN_DrawElements)(GLenum, GLsizei, GLenum, const void *);

static struct {
    PFN_Enable Enable; PFN_Disable Disable; PFN_IsEnabled IsEnabled; PFN_GetIntegerv GetIntegerv;
    PFN_BlendEquation BlendEquation; PFN_BlendFuncSeparate BlendFuncSeparate;
    PFN_Viewport Viewport; PFN_Scissor Scissor; PFN_ColorMask ColorMask;
    PFN_CreateShader CreateShader; PFN_ShaderSource ShaderSource; PFN_CompileShader CompileShader;
    PFN_GetShaderiv GetShaderiv; PFN_GetShaderInfoLog GetShaderInfoLog;
    PFN_CreateProgram CreateProgram; PFN_AttachShader AttachShader; PFN_LinkProgram LinkProgram;
    PFN_GetProgramiv GetProgramiv; PFN_DeleteShader DeleteShader; PFN_DeleteProgram DeleteProgram;
    PFN_UseProgram UseProgram; PFN_GetUniformLocation GetUniformLocation; PFN_Uniform1i Uniform1i;
    PFN_Uniform2f Uniform2f;
    PFN_GenVertexArrays GenVertexArrays; PFN_BindVertexArray BindVertexArray; PFN_DeleteVertexArrays DeleteVertexArrays;
    PFN_GenBuffers GenBuffers; PFN_BindBuffer BindBuffer; PFN_BufferData BufferData; PFN_DeleteBuffers DeleteBuffers;
    PFN_EnableVertexAttribArray EnableVertexAttribArray; PFN_VertexAttribPointer VertexAttribPointer;
    PFN_GenTextures GenTextures; PFN_BindTexture BindTexture; PFN_ActiveTexture ActiveTexture;
    PFN_TexParameteri TexParameteri; PFN_TexImage2D TexImage2D; PFN_PixelStorei PixelStorei;
    PFN_DeleteTextures DeleteTextures; PFN_DrawElements DrawElements;
} gl;

static int    s_ready;
static GLuint s_prog, s_vao, s_vbo, s_ibo, s_tex;
static GLint  s_u_size, s_u_tex;

static const char *k_vs =
    "layout(location=0) in vec2 pos;\n"
    "layout(location=1) in vec2 uv;\n"
    "layout(location=2) in vec4 col;\n"
    "uniform vec2 size;\n"
    "out vec2 v_uv; out vec4 v_col;\n"
    "void main() {\n"
    "    v_uv = uv; v_col = col;\n"
    "    gl_Position = vec4(pos.x / size.x * 2.0 - 1.0, 1.0 - pos.y / size.y * 2.0, 0.0, 1.0);\n"
    "}\n";
static const char *k_fs =
    "in vec2 v_uv; in vec4 v_col;\n"
    "uniform sampler2D tex;\n"
    "out vec4 frag;\n"
    "void main() { frag = vec4(v_col.rgb, v_col.a * texture(tex, v_uv).r); }\n";

#define LOAD(name) do { gl.name = (PFN_##name)load("gl" #name); if (!gl.name) return 0; } while (0)

static int load_all(GuiGLLoader load)
{
    LOAD(Enable); LOAD(Disable); LOAD(IsEnabled); LOAD(GetIntegerv); LOAD(BlendEquation); LOAD(BlendFuncSeparate);
    LOAD(Viewport); LOAD(Scissor); LOAD(ColorMask); LOAD(CreateShader); LOAD(ShaderSource); LOAD(CompileShader);
    LOAD(GetShaderiv); LOAD(GetShaderInfoLog); LOAD(CreateProgram); LOAD(AttachShader); LOAD(LinkProgram);
    LOAD(GetProgramiv); LOAD(DeleteShader); LOAD(DeleteProgram); LOAD(UseProgram); LOAD(GetUniformLocation);
    LOAD(Uniform1i); LOAD(Uniform2f); LOAD(GenVertexArrays); LOAD(BindVertexArray); LOAD(DeleteVertexArrays);
    LOAD(GenBuffers); LOAD(BindBuffer); LOAD(BufferData); LOAD(DeleteBuffers); LOAD(EnableVertexAttribArray);
    LOAD(VertexAttribPointer); LOAD(GenTextures); LOAD(BindTexture); LOAD(ActiveTexture); LOAD(TexParameteri);
    LOAD(TexImage2D); LOAD(PixelStorei); LOAD(DeleteTextures); LOAD(DrawElements);
    return 1;
}

static GLuint compile(GLenum type, const char *header, const char *body)
{
    const char *src[2];
    GLuint s = gl.CreateShader(type);
    GLint ok = 0;
    src[0] = header;
    src[1] = body;
    gl.ShaderSource(s, 2, src, NULL);
    gl.CompileShader(s);
    gl.GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        gl.GetShaderInfoLog(s, sizeof log, NULL, log);
        fprintf(stderr, "[GUI] GL shader: %s\n", log);
        gl.DeleteShader(s);
        return 0;
    }
    return s;
}

int gui_gl_init(GuiGLLoader load, int es)
{
    const uint8_t *alpha;
    int w, h;
    GLuint vs, fs;
    GLint ok = 0;
    const char *vh = es ? "#version 300 es\nprecision highp float;\n" : "#version 330 core\n";
    const char *fh = es ? "#version 300 es\nprecision mediump float;\n" : "#version 330 core\n";

    if (s_ready) return 1;
    if (!load || !load_all(load)) {
        fprintf(stderr, "[GUI] GL functions missing (need OpenGL 3.3 core or ES 3.0)\n");
        return 0;
    }
    vs = compile(GL_VERTEX_SHADER, vh, k_vs);
    fs = compile(GL_FRAGMENT_SHADER, fh, k_fs);
    if (!vs || !fs) return 0;
    s_prog = gl.CreateProgram();
    gl.AttachShader(s_prog, vs);
    gl.AttachShader(s_prog, fs);
    gl.LinkProgram(s_prog);
    gl.DeleteShader(vs);
    gl.DeleteShader(fs);
    gl.GetProgramiv(s_prog, GL_LINK_STATUS, &ok);
    if (!ok) { fprintf(stderr, "[GUI] GL program link failed\n"); return 0; }
    s_u_size = gl.GetUniformLocation(s_prog, "size");
    s_u_tex = gl.GetUniformLocation(s_prog, "tex");

    gui_font_atlas(&alpha, &w, &h);
    if (!alpha) return 0;
    gl.GenTextures(1, &s_tex);
    gl.BindTexture(GL_TEXTURE_2D, s_tex);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, alpha);

    gl.GenVertexArrays(1, &s_vao);
    gl.GenBuffers(1, &s_vbo);
    gl.GenBuffers(1, &s_ibo);
    s_ready = 1;
    return 1;
}

void gui_gl_shutdown(void)
{
    if (!s_ready) return;
    gl.DeleteTextures(1, &s_tex);
    gl.DeleteBuffers(1, &s_vbo);
    gl.DeleteBuffers(1, &s_ibo);
    gl.DeleteVertexArrays(1, &s_vao);
    gl.DeleteProgram(s_prog);
    s_ready = 0;
}

void gui_gl_render(const GuiDrawData *dd, int fb_w, int fb_h)
{
    GLint prog, abuf, vao, tex, atex, vp[4], sc[4], srgb, drgb, sa, da, eq_rgb, eq_a;
    GLint wm[4];
    GLboolean blend, cull, depth, scissor;
    int i;

    if (!s_ready || !dd || !dd->cmd_count || fb_w < 1 || fb_h < 1) return;

    gl.GetIntegerv(GL_CURRENT_PROGRAM, &prog);
    gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &abuf);
    gl.GetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    gl.GetIntegerv(GL_ACTIVE_TEXTURE, &atex);
    gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
    gl.GetIntegerv(GL_VIEWPORT, vp);
    gl.GetIntegerv(GL_SCISSOR_BOX, sc);
    gl.GetIntegerv(GL_BLEND_SRC_RGB, &srgb);
    gl.GetIntegerv(GL_BLEND_DST_RGB, &drgb);
    gl.GetIntegerv(GL_BLEND_SRC_ALPHA, &sa);
    gl.GetIntegerv(GL_BLEND_DST_ALPHA, &da);
    gl.GetIntegerv(GL_BLEND_EQUATION_RGB, &eq_rgb);
    gl.GetIntegerv(GL_BLEND_EQUATION_ALPHA, &eq_a);
    gl.GetIntegerv(GL_COLOR_WRITEMASK, wm);
    blend = gl.IsEnabled(GL_BLEND); cull = gl.IsEnabled(GL_CULL_FACE);
    depth = gl.IsEnabled(GL_DEPTH_TEST); scissor = gl.IsEnabled(GL_SCISSOR_TEST);

    gl.Enable(GL_BLEND);
    gl.BlendEquation(GL_FUNC_ADD);
    gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    gl.Disable(GL_CULL_FACE);
    gl.Disable(GL_DEPTH_TEST);
    gl.Enable(GL_SCISSOR_TEST);
    gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    gl.Viewport(0, 0, fb_w, fb_h);

    gl.UseProgram(s_prog);
    gl.Uniform2f(s_u_size, dd->display_w, dd->display_h);
    gl.Uniform1i(s_u_tex, 0);
    gl.ActiveTexture(GL_TEXTURE0);
    gl.BindTexture(GL_TEXTURE_2D, s_tex);
    gl.BindVertexArray(s_vao);
    gl.BindBuffer(GL_ARRAY_BUFFER, s_vbo);
    gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);
    gl.BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)dd->vtx_count * (GLsizeiptr)sizeof(GuiVertex), dd->vtx, GL_STREAM_DRAW);
    gl.BufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)dd->idx_count * (GLsizeiptr)sizeof(uint32_t), dd->idx, GL_STREAM_DRAW);
    gl.EnableVertexAttribArray(0);
    gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GuiVertex), (const void *)offsetof(GuiVertex, x));
    gl.EnableVertexAttribArray(1);
    gl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GuiVertex), (const void *)offsetof(GuiVertex, u));
    gl.EnableVertexAttribArray(2);
    gl.VertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GuiVertex), (const void *)offsetof(GuiVertex, col));

    {   /* draw data is in display pixels; the framebuffer may be larger (HiDPI) */
        float kx = (float)fb_w / dd->display_w, ky = (float)fb_h / dd->display_h;
        for (i = 0; i < dd->cmd_count; i++) {
            const GuiDrawCmd *c = &dd->cmd[i];
            int x0 = (int)(c->clip[0] * kx), y0 = (int)(c->clip[1] * ky);
            int x1 = (int)(c->clip[2] * kx + 0.5f), y1 = (int)(c->clip[3] * ky + 0.5f);
            if (x1 <= x0 || y1 <= y0 || !c->idx_count) continue;
            gl.Scissor(x0, fb_h - y1, x1 - x0, y1 - y0);        /* GL's origin is bottom-left */
            gl.DrawElements(GL_TRIANGLES, (GLsizei)c->idx_count, GL_UNSIGNED_INT,
                            (const void *)((size_t)c->idx_offset * sizeof(uint32_t)));
        }
    }

    gl.UseProgram((GLuint)prog);
    gl.BindVertexArray((GLuint)vao);
    gl.BindBuffer(GL_ARRAY_BUFFER, (GLuint)abuf);
    gl.ActiveTexture((GLenum)atex);
    gl.BindTexture(GL_TEXTURE_2D, (GLuint)tex);
    gl.BlendEquation((GLenum)eq_rgb);
    gl.BlendFuncSeparate((GLenum)srgb, (GLenum)drgb, (GLenum)sa, (GLenum)da);
    gl.ColorMask((GLboolean)wm[0], (GLboolean)wm[1], (GLboolean)wm[2], (GLboolean)wm[3]);
    gl.Viewport(vp[0], vp[1], vp[2], vp[3]);
    gl.Scissor(sc[0], sc[1], sc[2], sc[3]);
    (void)eq_a;
    if (!blend) gl.Disable(GL_BLEND);
    if (cull) gl.Enable(GL_CULL_FACE);
    if (depth) gl.Enable(GL_DEPTH_TEST);
    if (!scissor) gl.Disable(GL_SCISSOR_TEST);
}
