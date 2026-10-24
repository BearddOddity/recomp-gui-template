<#
.SYNOPSIS
  Add the F1 debug menu to an xboxrecomp game project.

.EXAMPLE
  tools\setup.ps1 -Project "D:\My apps\My Game Recomp"
  tools\setup.ps1 -Project . -Sdk "D:\xboxrecomp-v0.16.0" -PatchCmake

.DESCRIPTION
  1. Finds the xboxrecomp SDK (-Sdk, or XBOXRECOMP_DIR in the project's CMakeLists.txt).
  2. Copies the GUI into <project>\src\gui, plus a starter debug_menu.c/.h in <project>\src.
  3. If the SDK has no runtime overlay of its own (v0.16.0 and older), copies the SDK's
     gpu_present.c and keyboard.c to <project>\src\sdk_local and inserts the two hooks
     (draw at present, menu takes the keyboard). SDKs that already carry src\gui need neither.
  4. Writes <project>\src\gui\recomp_gui.cmake; -PatchCmake appends the include to CMakeLists.txt.
  Safe to re-run: existing debug_menu.c/.h and sdk_local files are never overwritten (-Force does).
  Never touches the SDK itself.
#>
param(
    [Parameter(Mandatory)][string]$Project,
    [string]$Sdk,
    [switch]$PatchCmake,
    [switch]$PatchSdkCopies,
    [switch]$Force
)
$ErrorActionPreference = 'Stop'
$tpl  = Split-Path -Parent $PSScriptRoot
$proj = (Resolve-Path -LiteralPath $Project).Path
$cml  = Join-Path $proj 'CMakeLists.txt'
if (-not (Test-Path $cml)) { throw "no CMakeLists.txt in $proj" }

if (-not $Sdk) {
    $m = Select-String -Path $cml -Pattern 'set\(XBOXRECOMP_DIR\s+"([^"]+)"' | Select-Object -First 1
    if (-not $m) { throw "pass -Sdk <xboxrecomp dir>: XBOXRECOMP_DIR not found in CMakeLists.txt" }
    $Sdk = $m.Matches[0].Groups[1].Value.Replace('${CMAKE_CURRENT_SOURCE_DIR}', $proj)
}
$Sdk = [IO.Path]::GetFullPath($Sdk)
if (-not (Test-Path (Join-Path $Sdk 'src'))) { throw "not an xboxrecomp SDK (no src\): $Sdk" }
$sdkHasGui = Test-Path (Join-Path $Sdk 'src\gui\gui_overlay.c')
Write-Host "SDK: $Sdk  (runtime overlay in SDK: $sdkHasGui)"

function Copy-New($from, $to) {
    if ((Test-Path $to) -and -not $Force) { Write-Host "  keep   $to"; return $false }
    New-Item -ItemType Directory -Force (Split-Path $to) | Out-Null
    Copy-Item -LiteralPath $from -Destination $to -Force; Write-Host "  write  $to"; return $true
}

# Insert $text after the first line matching $anchor; fail loudly if the SDK file changed shape.
function Add-Hook($file, $include, $anchor, $text) {
    $c = Get-Content -LiteralPath $file -Raw
    if ($c -match 'recomp-gui hook') { return }
    $nl = if ($c.Contains("`r`n")) { "`r`n" } else { "`n" }
    $idx = $c.LastIndexOf('#include')
    $eol = $c.IndexOf("`n", $idx) + 1
    $c = $c.Insert($eol, "#include `"gui/gui_overlay.h`"   /* recomp-gui hook */$nl")
    $rx = [regex]::new($anchor)
    $mm = $rx.Match($c)
    if (-not $mm.Success) { throw "hook anchor not found in ${file}: $anchor`nAdd by hand: $text" }
    $c = $c.Insert($mm.Index + $mm.Length, ($text -replace "`n", $nl))
    Set-Content -LiteralPath $file -Value $c -NoNewline
}

# 1. GUI sources (not when the SDK already builds them: that would define every symbol twice)
$gui = Join-Path $proj 'src\gui'
if (-not $sdkHasGui) {
Write-Host "GUI sources -> src\gui"
foreach ($f in 'gui.c','gui.h','gui_d3d11.c','gui_d3d11.h','gui_overlay.c','gui_overlay.h','stb_truetype.h','gui_host.c','gui_host.h','gui_present_hook.c','gui_present_hook.h') {
    Copy-New (Join-Path $tpl "src\$f") (Join-Path $gui $f) | Out-Null
}
Copy-New (Join-Path $tpl 'LICENSE') (Join-Path $gui 'LICENSE-recomp-gui') | Out-Null
Copy-New (Join-Path $tpl 'NOTICE')  (Join-Path $gui 'NOTICE-recomp-gui')  | Out-Null
}

# 2. starter menu
Write-Host "Starter menu -> src"
Copy-New (Join-Path $tpl 'examples\debug_menu.c') (Join-Path $proj 'src\debug_menu.c') | Out-Null
Copy-New (Join-Path $tpl 'examples\debug_menu.h') (Join-Path $proj 'src\debug_menu.h') | Out-Null

# 3. Optional legacy path (-PatchSdkCopies): local copies of two SDK files with the hooks inserted.
#    Version-fragile, so off by default; the default draws through gui_present_hook.c instead.
$sources = @('src/debug_menu.c')
if (-not $sdkHasGui) { $sources = @('src/gui/gui.c','src/gui/gui_d3d11.c','src/gui/gui_overlay.c','src/gui/gui_host.c','src/gui/gui_present_hook.c') + $sources }
$defs = @()
if ($PatchSdkCopies -and -not $sdkHasGui) {
    Write-Host "SDK hooks -> src\sdk_local"
    $loc = Join-Path $proj 'src\sdk_local'
    $gp = Get-ChildItem $Sdk -Recurse -Filter gpu_present.c | Select-Object -First 1
    $kb = Get-ChildItem $Sdk -Recurse -Filter keyboard.c    | Select-Object -First 1
    if (-not $gp) { throw "gpu_present.c not found in SDK" }
    if (Copy-New $gp.FullName (Join-Path $loc 'gpu_present.c')) {
        Add-Hook (Join-Path $loc 'gpu_present.c') 'gui/gui_overlay.h' 'ID3D11DeviceContext_Draw\(s_ctx,\s*3,\s*0\);' `
"`n    gui_overlay_frame_d3d11(s_dev, s_ctx, s_rtv, s_hwnd);   /* recomp-gui hook: F1 menu over the finished frame */"
    }
    if ($kb -and (Copy-New $kb.FullName (Join-Path $loc 'keyboard.c'))) {
        Add-Hook (Join-Path $loc 'keyboard.c' ) 'gui/gui_overlay.h' '(?m)^\s*xbox_InputKeyboardState\(out\);' ''
        # keyboard: the menu takes the keys. Rewrite the anchor line into a guarded call.
        $kf = Join-Path $loc 'keyboard.c'; $c = Get-Content $kf -Raw
        $c = $c -replace '(?m)^(\s*)xbox_InputKeyboardState\(out\);', ('$1if (gui_overlay_wants_input()) { static DWORD pkt; memset(out, 0, sizeof *out); out->dwPacketNumber = ++pkt; return 1; }   /* recomp-gui hook */' + "`n" + '$1xbox_InputKeyboardState(out);')
        Set-Content $kf $c -NoNewline
    }
    $sources += @('src/sdk_local/gpu_present.c'); if ($kb) { $sources += 'src/sdk_local/keyboard.c' }
}
# pad capture: the SDK's own switch if it has one
# No SDK file is patched by default: the menu is drawn by hooking IDXGISwapChain::Present (gui_present_hook.c).
if (-not $sdkHasGui -and -not $PatchSdkCopies) { $defs += 'RECOMP_GUI_AUTO_PRESENT' }

# 4. CMake include
New-Item -ItemType Directory -Force $gui | Out-Null
$cm = Join-Path $gui 'recomp_gui.cmake'
$body = @"
# Generated by recomp-gui-template tools/setup.ps1. Include from CMakeLists.txt, after add_executable():
#   include(src/gui/recomp_gui.cmake)
set(RECOMP_GUI_SOURCES
$(($sources | ForEach-Object { "    `${CMAKE_CURRENT_LIST_DIR}/../../$_" }) -join "`n")
)
target_sources(`${PROJECT_NAME} PRIVATE `${RECOMP_GUI_SOURCES})
target_include_directories(`${PROJECT_NAME} PRIVATE `${CMAKE_CURRENT_LIST_DIR}/.. `${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(`${PROJECT_NAME} PRIVATE d3d11 dxgi dxguid d3dcompiler pdh psapi xinput)
$(if ($defs) { "target_compile_definitions(`${PROJECT_NAME} PRIVATE $($defs -join ' '))" })
"@
Set-Content $cm $body
Write-Host "wrote $cm"
if ($PatchCmake) {
    if (-not (Select-String -Path $cml -Pattern 'recomp_gui.cmake' -Quiet)) {
        Add-Content $cml "`n# F1 debug menu (recomp-gui-template)`ninclude(src/gui/recomp_gui.cmake)`n"
        Write-Host "patched CMakeLists.txt"
    }
}

Write-Host @"

Done. Two lines left:
  CMakeLists.txt  (after add_executable)   include(src/gui/recomp_gui.cmake)$(if ($PatchCmake) {'   <- done'})
  main.c          #include "debug_menu.h" ; call debug_menu_init() once before the guest runs
Then build, run, press F1. Edit src\debug_menu.c to add your pages.
"@
