cmake_minimum_required(VERSION 3.16)
project(recomp_gui C)
set(CMAKE_C_STANDARD 11)

# Core (portable) + D3D11 renderer + overlay (Windows) + host glue.
if(WIN32)
    add_library(recomp_gui STATIC
        src/gui.c src/gui_d3d11.c src/gui_overlay.c src/gui_host.c)
    target_link_libraries(recomp_gui PUBLIC d3d11 dxgi dxguid d3dcompiler pdh psapi xinput)
else()
    add_library(recomp_gui STATIC src/gui.c)
    target_link_libraries(recomp_gui PUBLIC m)
endif()
target_include_directories(recomp_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(recomp_gui PRIVATE /W4 /O2)
else()
    target_compile_options(recomp_gui PRIVATE -Wall -Wextra -O2)
endif()

# Example menu (compiled, not linked into anything): proves it builds.
add_library(recomp_gui_example OBJECT examples/debug_menu.c)
target_link_libraries(recomp_gui_example PRIVATE recomp_gui)

add_executable(gui_smoke tests/gui_smoke.c)
target_link_libraries(gui_smoke PRIVATE recomp_gui)
