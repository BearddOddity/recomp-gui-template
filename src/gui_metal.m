/*
 * Metal renderer for gui.h draw data. See gui_metal.h. Build with -fobjc-arc.
 */
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string.h>

#include "gui_metal.h"

static NSString *const k_msl =
    @"#include <metal_stdlib>\n"
    @"using namespace metal;\n"
    @"struct VIn  { float2 pos [[attribute(0)]]; float2 uv [[attribute(1)]]; uchar4 col [[attribute(2)]]; };\n"
    @"struct VOut { float4 pos [[position]]; float2 uv; float4 col; };\n"
    @"vertex VOut gui_vs(VIn v [[stage_in]], constant float2 &size [[buffer(1)]]) {\n"
    @"    VOut o;\n"
    @"    o.pos = float4(v.pos.x / size.x * 2.0 - 1.0, 1.0 - v.pos.y / size.y * 2.0, 0.0, 1.0);\n"
    @"    o.uv = v.uv;\n"
    @"    o.col = float4(v.col) / 255.0;\n"
    @"    return o;\n"
    @"}\n"
    @"fragment float4 gui_fs(VOut i [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]]) {\n"
    @"    return float4(i.col.rgb, i.col.a * tex.sample(smp, i.uv).r);\n"
    @"}\n";

static id<MTLDevice>              s_dev;
static id<MTLRenderPipelineState> s_pso;
static id<MTLSamplerState>        s_smp;
static id<MTLTexture>             s_tex;
static id<MTLBuffer>              s_vbuf, s_ibuf;

int gui_metal_init(void *device, unsigned long pixel_format)
{
    @autoreleasepool {
        const uint8_t *alpha;
        int w, h;
        NSError *err = nil;
        id<MTLLibrary> lib;
        MTLRenderPipelineDescriptor *pd;
        MTLVertexDescriptor *vd;
        MTLTextureDescriptor *td;
        MTLSamplerDescriptor *sd;

        if (s_pso) return 1;
        s_dev = (__bridge id<MTLDevice>)device;
        if (!s_dev) return 0;
        lib = [s_dev newLibraryWithSource:k_msl options:nil error:&err];
        if (!lib) {
            fprintf(stderr, "[GUI] Metal shader: %s\n", err.localizedDescription.UTF8String);
            return 0;
        }
        vd = [MTLVertexDescriptor vertexDescriptor];
        vd.attributes[0].format = MTLVertexFormatFloat2;         vd.attributes[0].offset = offsetof(GuiVertex, x);
        vd.attributes[1].format = MTLVertexFormatFloat2;         vd.attributes[1].offset = offsetof(GuiVertex, u);
        vd.attributes[2].format = MTLVertexFormatUChar4;         vd.attributes[2].offset = offsetof(GuiVertex, col);
        for (int i = 0; i < 3; i++) vd.attributes[i].bufferIndex = 0;
        vd.layouts[0].stride = sizeof(GuiVertex);

        pd = [MTLRenderPipelineDescriptor new];
        pd.vertexFunction = [lib newFunctionWithName:@"gui_vs"];
        pd.fragmentFunction = [lib newFunctionWithName:@"gui_fs"];
        pd.vertexDescriptor = vd;
        pd.colorAttachments[0].pixelFormat = (MTLPixelFormat)pixel_format;
        pd.colorAttachments[0].blendingEnabled = YES;
        pd.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pd.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        pd.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pd.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pd.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pd.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        s_pso = [s_dev newRenderPipelineStateWithDescriptor:pd error:&err];
        if (!s_pso) {
            fprintf(stderr, "[GUI] Metal pipeline: %s\n", err.localizedDescription.UTF8String);
            return 0;
        }

        sd = [MTLSamplerDescriptor new];
        sd.minFilter = sd.magFilter = MTLSamplerMinMagFilterLinear;
        sd.sAddressMode = sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
        s_smp = [s_dev newSamplerStateWithDescriptor:sd];

        gui_font_atlas(&alpha, &w, &h);
        if (!alpha) return 0;
        td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                                                 width:(NSUInteger)w height:(NSUInteger)h mipmapped:NO];
        s_tex = [s_dev newTextureWithDescriptor:td];
        [s_tex replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)w, (NSUInteger)h)
                 mipmapLevel:0 withBytes:alpha bytesPerRow:(NSUInteger)w];
        return 1;
    }
}

void gui_metal_shutdown(void)
{
    s_pso = nil; s_smp = nil; s_tex = nil; s_vbuf = nil; s_ibuf = nil; s_dev = nil;
}

static id<MTLBuffer> ensure(id<MTLBuffer> buf, size_t bytes)
{
    if (buf && buf.length >= bytes) return buf;
    return [s_dev newBufferWithLength:bytes * 2 options:MTLResourceStorageModeShared];
}

void gui_metal_render(const GuiDrawData *dd, void *encoder_ptr, int fb_w, int fb_h)
{
    if (!s_pso || !dd || !dd->cmd_count || fb_w < 1 || fb_h < 1) return;
    @autoreleasepool {
        id<MTLRenderCommandEncoder> enc = (__bridge id<MTLRenderCommandEncoder>)encoder_ptr;
        size_t vbytes = (size_t)dd->vtx_count * sizeof(GuiVertex), ibytes = (size_t)dd->idx_count * sizeof(uint32_t);
        float size[2] = { dd->display_w, dd->display_h };
        float kx = (float)fb_w / dd->display_w, ky = (float)fb_h / dd->display_h;
        MTLViewport vp = { 0, 0, fb_w, fb_h, 0, 1 };

        s_vbuf = ensure(s_vbuf, vbytes);
        s_ibuf = ensure(s_ibuf, ibytes);
        memcpy(s_vbuf.contents, dd->vtx, vbytes);
        memcpy(s_ibuf.contents, dd->idx, ibytes);

        [enc setRenderPipelineState:s_pso];
        [enc setViewport:vp];
        [enc setCullMode:MTLCullModeNone];
        [enc setVertexBuffer:s_vbuf offset:0 atIndex:0];
        [enc setVertexBytes:size length:sizeof size atIndex:1];
        [enc setFragmentTexture:s_tex atIndex:0];
        [enc setFragmentSamplerState:s_smp atIndex:0];
        for (int i = 0; i < dd->cmd_count; i++) {
            const GuiDrawCmd *c = &dd->cmd[i];
            NSInteger x0 = (NSInteger)(c->clip[0] * kx), y0 = (NSInteger)(c->clip[1] * ky);
            NSInteger x1 = (NSInteger)(c->clip[2] * kx + 0.5f), y1 = (NSInteger)(c->clip[3] * ky + 0.5f);
            MTLScissorRect sr;
            if (x0 < 0) x0 = 0;
            if (y0 < 0) y0 = 0;
            if (x1 > fb_w) x1 = fb_w;
            if (y1 > fb_h) y1 = fb_h;
            if (x1 <= x0 || y1 <= y0 || !c->idx_count) continue;
            sr.x = (NSUInteger)x0; sr.y = (NSUInteger)y0; sr.width = (NSUInteger)(x1 - x0); sr.height = (NSUInteger)(y1 - y0);
            [enc setScissorRect:sr];
            [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:c->idx_count indexType:MTLIndexTypeUInt32
                           indexBuffer:s_ibuf indexBufferOffset:(NSUInteger)c->idx_offset * sizeof(uint32_t)];
        }
    }
}
