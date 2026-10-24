/*
 * Draw the overlay without touching the game's or the SDK's source.
 *
 * Patches IDXGISwapChain::Present: every swap chain created by the same DXGI
 * implementation shares one vtable, so one patched slot catches the game's own
 * Present(), whatever SDK version or renderer made the swap chain. Just before
 * the real Present the hook hands the back buffer to gui_overlay_frame_d3d11().
 *
 * Use it when the game does NOT already call gui_overlay_frame_d3d11() itself
 * (True Crime does; a plain xboxrecomp title does not). Calling both draws
 * the menu twice.
 *
 *     gui_present_hook_install();      once, any time before or after the swap chain exists
 */
#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include "gui_present_hook.h"
#include "gui_overlay.h"

#define PRESENT_SLOT 8   /* IUnknown(3) + IDXGIObject(4) + IDXGIDeviceSubObject(1) -> Present */

typedef HRESULT (STDMETHODCALLTYPE *PresentFn)(IDXGISwapChain *, UINT, UINT);

static PresentFn s_orig;
static void    **s_slot;
static volatile LONG s_busy;
static volatile LONG s_count;

static HRESULT STDMETHODCALLTYPE hooked_present(IDXGISwapChain *sc, UINT sync, UINT flags)
{
    if (!(flags & DXGI_PRESENT_TEST) && InterlockedCompareExchange(&s_busy, 1, 0) == 0) {
        ID3D11Device *dev = NULL;
        if (SUCCEEDED(IDXGISwapChain_GetDevice(sc, &IID_ID3D11Device, (void **)&dev)) && dev) {
            ID3D11DeviceContext *ctx = NULL;
            ID3D11Texture2D *buf = NULL;
            ID3D11RenderTargetView *rtv = NULL;
            DXGI_SWAP_CHAIN_DESC d;
            ID3D11Device_GetImmediateContext(dev, &ctx);
            if (ctx && SUCCEEDED(IDXGISwapChain_GetBuffer(sc, 0, &IID_ID3D11Texture2D, (void **)&buf)) && buf
                    && SUCCEEDED(ID3D11Device_CreateRenderTargetView(dev, (ID3D11Resource *)buf, NULL, &rtv)) && rtv
                    && SUCCEEDED(IDXGISwapChain_GetDesc(sc, &d))) {
                gui_overlay_frame_d3d11(dev, ctx, rtv, d.OutputWindow);
                InterlockedIncrement(&s_count);
            }
            if (rtv) ID3D11RenderTargetView_Release(rtv);
            if (buf) ID3D11Texture2D_Release(buf);
            if (ctx) ID3D11DeviceContext_Release(ctx);
            ID3D11Device_Release(dev);
        }
        InterlockedExchange(&s_busy, 0);
    }
    return s_orig(sc, sync, flags);
}

static LRESULT CALLBACK dummy_proc(HWND h, UINT m, WPARAM w, LPARAM l) { return DefWindowProcA(h, m, w, l); }

int gui_present_hook_install(void)
{
    WNDCLASSA wc;
    HWND hwnd;
    IDXGISwapChain *sc = NULL;
    ID3D11Device *dev = NULL;
    ID3D11DeviceContext *ctx = NULL;
    DXGI_SWAP_CHAIN_DESC d;
    D3D_DRIVER_TYPE types[2] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP };
    void **vtbl;
    DWORD old;
    int i, ok = 0;

    if (s_slot) return 1;
    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = dummy_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "recomp_gui_present_probe";
    RegisterClassA(&wc);
    hwnd = CreateWindowA(wc.lpszClassName, "", WS_OVERLAPPED, 0, 0, 16, 16, NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) return 0;

    ZeroMemory(&d, sizeof d);
    d.BufferCount = 1;
    d.BufferDesc.Width = d.BufferDesc.Height = 16;
    d.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    d.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    d.OutputWindow = hwnd;
    d.SampleDesc.Count = 1;
    d.Windowed = TRUE;
    for (i = 0; i < 2 && !sc; i++)
        D3D11CreateDeviceAndSwapChain(NULL, types[i], NULL, 0, NULL, 0, D3D11_SDK_VERSION, &d, &sc, &dev, NULL, &ctx);
    if (sc) {
        vtbl = *(void ***)sc;
        if (VirtualProtect(&vtbl[PRESENT_SLOT], sizeof(void *), PAGE_READWRITE, &old)) {
            s_orig = (PresentFn)vtbl[PRESENT_SLOT];
            vtbl[PRESENT_SLOT] = (void *)hooked_present;
            VirtualProtect(&vtbl[PRESENT_SLOT], sizeof(void *), old, &old);
            s_slot = &vtbl[PRESENT_SLOT];
            ok = 1;
        }
        IDXGISwapChain_Release(sc);
    }
    if (ctx) ID3D11DeviceContext_Release(ctx);
    if (dev) ID3D11Device_Release(dev);
    DestroyWindow(hwnd);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);
    return ok;
}

void gui_present_hook_remove(void)
{
    DWORD old;
    if (!s_slot) return;
    if (VirtualProtect(s_slot, sizeof(void *), PAGE_READWRITE, &old)) {
        *s_slot = (void *)s_orig;
        VirtualProtect(s_slot, sizeof(void *), old, &old);
    }
    s_slot = NULL;
}

int gui_present_hook_count(void) { return (int)s_count; }
