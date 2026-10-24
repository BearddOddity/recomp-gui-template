/*
 * Metal renderer for gui.h draw data (macOS, iOS).
 *
 *     // once, with the layer's device and pixel format
 *     gui_metal_init(device, MTLPixelFormatBGRA8Unorm);
 *     // each frame, inside your render pass, after drawing the scene
 *     const GuiDrawData *dd = gui_overlay_frame(w, h);
 *     if (dd) gui_metal_render(dd, encoder, drawable_w, drawable_h);
 *
 * The header is plain C: the Metal objects are passed as void * (bridge with
 * (__bridge void *) from Objective-C, or cast the pointers from metal-cpp).
 */
#ifndef RECOMP_GUI_METAL_H
#define RECOMP_GUI_METAL_H

#include "gui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* device: id<MTLDevice>. pixel_format: the MTLPixelFormat of the render target
 * (a raw NSUInteger value). Returns 0 on failure; later calls do nothing. */
int  gui_metal_init(void *device, unsigned long pixel_format);
void gui_metal_shutdown(void);
/* encoder: the id<MTLRenderCommandEncoder> of the pass that draws to the
 * target. fb_w / fb_h: pixel size of that target. */
void gui_metal_render(const GuiDrawData *dd, void *encoder, int fb_w, int fb_h);

#ifdef __cplusplus
}
#endif
#endif
