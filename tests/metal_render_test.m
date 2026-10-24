/*
 * Renders the overlay with the Metal renderer into an off-screen texture,
 * reads the pixels back and checks that the menu drew something. Skips (exit 0)
 * when the machine has no Metal device. With a path argument it also writes a
 * .ppm to look at.
 */
#import <Metal/Metal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui_overlay_portable.h"
#include "gui_metal.h"

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)

static void demo_page(void *u)
{
    static int on = 1; static float v = 0.5f;
    (void)u;
    gui_checkbox("God mode", &on);
    gui_slider_float("Gamma", &v, 0.5f, 3.0f, "%.2f");
    if (gui_button("Reload level")) {}
}

int main(int argc, char **argv)
{
    @autoreleasepool {
        enum { W = 960, H = 540 };
        id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
        GuiPlatformInput in;
        const GuiDrawData *dd = NULL;
        long lit = 0;
        int f;

        if (!dev) { fprintf(stderr, "no Metal device: skipped\n"); return 0; }
        gui_overlay_add_page("Cheats", demo_page, NULL);
        gui_overlay_set_title("Metal test menu");
        gui_overlay_set_enabled(1);
        gui_overlay_set_menu_open(1);
        memset(&in, 0, sizeof in);
        in.focused = 1; in.mouse_x = in.mouse_y = -1.0f;
        for (f = 0; f < 6; f++) {
            gui_platform_set_input(&in);
            dd = gui_overlay_frame((float)W, (float)H);
            if (!dd) { fprintf(stderr, "no system font: skipped\n"); return 0; }
        }
        CHECK(gui_metal_init((__bridge void *)dev, MTLPixelFormatRGBA8Unorm));

        MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                       width:W height:H mipmapped:NO];
        td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        td.storageMode = MTLStorageModeShared;
        id<MTLTexture> tex = [dev newTextureWithDescriptor:td];
        id<MTLCommandQueue> q = [dev newCommandQueue];
        MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = tex;
        rp.colorAttachments[0].loadAction = MTLLoadActionClear;
        rp.colorAttachments[0].clearColor = MTLClearColorMake(0.2, 0.22, 0.3, 1.0);
        rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        id<MTLCommandBuffer> cb = [q commandBuffer];
        id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];
        gui_metal_render(dd, (__bridge void *)enc, W, H);
        [enc endEncoding];
        [cb commit];
        [cb waitUntilCompleted];

        unsigned char *px = malloc((size_t)W * H * 4);
        [tex getBytes:px bytesPerRow:W * 4 fromRegion:MTLRegionMake2D(0, 0, W, H) mipmapLevel:0];
        for (long i = 0; i < (long)W * H; i++)
            if (abs(px[i * 4] - 51) > 8 || abs(px[i * 4 + 1] - 56) > 8 || abs(px[i * 4 + 2] - 76) > 8) lit++;
        printf("metal render: %ld of %d pixels drawn by the GUI\n", lit, W * H);
        if (argc > 1) {
            FILE *o = fopen(argv[1], "wb");
            if (o) {
                fprintf(o, "P6\n%d %d\n255\n", W, H);
                for (long i = 0; i < (long)W * H; i++) fwrite(px + i * 4, 1, 3, o);
                fclose(o);
            }
        }
        CHECK(lit > 5000);
        gui_metal_shutdown();
        return 0;
    }
}
