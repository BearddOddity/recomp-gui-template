/* Present hook check: a real D3D11 swap chain, hooked once, presented 3 times.
 * Exit 0 = the overlay was reached on every Present and Present still worked. */
#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <stdio.h>
#include "gui_present_hook.h"

int main(void)
{
    WNDCLASSA wc = {0};
    HWND hwnd;
    IDXGISwapChain *sc = NULL;
    ID3D11Device *dev = NULL;
    ID3D11DeviceContext *ctx = NULL;
    DXGI_SWAP_CHAIN_DESC d = {0};
    D3D_DRIVER_TYPE types[2] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP };
    int i, bad = 0;

    if (!gui_present_hook_install()) { fprintf(stderr, "FAIL install\n"); return 1; }
    wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(NULL); wc.lpszClassName = "pht";
    RegisterClassA(&wc);
    hwnd = CreateWindowA("pht", "t", WS_OVERLAPPED, 0, 0, 64, 64, NULL, NULL, wc.hInstance, NULL);
    d.BufferCount = 2; d.BufferDesc.Width = d.BufferDesc.Height = 64;
    d.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; d.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    d.OutputWindow = hwnd; d.SampleDesc.Count = 1; d.Windowed = TRUE; d.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    for (i = 0; i < 2 && !sc; i++)
        D3D11CreateDeviceAndSwapChain(NULL, types[i], NULL, 0, NULL, 0, D3D11_SDK_VERSION, &d, &sc, &dev, NULL, &ctx);
    if (!sc) { fprintf(stderr, "FAIL no swap chain\n"); return 1; }
    for (i = 0; i < 3; i++)
        if (FAILED(IDXGISwapChain_Present(sc, 0, 0))) bad++;
    printf("hook frames %d, present failures %d\n", gui_present_hook_count(), bad);
    gui_present_hook_remove();
    return (gui_present_hook_count() == 3 && !bad) ? 0 : 1;
}
