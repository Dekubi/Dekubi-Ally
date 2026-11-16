$ErrorActionPreference = "Stop"
$root = "C:\Users\User\projects\Fritchii"
Set-Location $root

Stop-Process -Name "Dekubi","dekubi","yuzu","fritchii","lichii" -Force -ErrorAction SilentlyContinue
$utf8NoBom = New-Object System.Text.UTF8Encoding $false
Add-Type -AssemblyName System.Drawing

# 0. Clean up the dummy %1 and refs folders created before the exception
foreach ($bad in @("%1", "refs")) {
    Remove-Item "$root\src\yuzu\$bad" -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item "$root\src\yuzu\fritchii\$bad" -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item "$root\Dekubi_Ally\$bad" -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item "$root\build\bin\Release\$bad" -Recurse -Force -ErrorAction SilentlyContinue
}
Remove-Item "$root\src\yuzu\icons\%1.svg" -Force -ErrorAction SilentlyContinue
Remove-Item "$root\src\yuzu\fritchii\icons\%1.svg" -Force -ErrorAction SilentlyContinue
Remove-Item "$root\Dekubi_Ally\icons\%1.svg" -Force -ErrorAction SilentlyContinue
Remove-Item "$root\build\bin\Release\icons\%1.svg" -Force -ErrorAction SilentlyContinue

# ==============================================================================
# 1. UPDATE VERSION v5.8.4 -> v1.0.0 ACROSS ALL SOURCE & BUILD FILES
# ==============================================================================
$verFiles = Get-ChildItem -Path "$root\src", "$root\CMakeModules", "$root\build" -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
    $_.FullName -notmatch '\\_deps\\|\\\.cache\\' -and $_.Extension -in @('.cpp','.h','.hpp','.in','.rc','.ui','.cmake','.txt','.json')
}
foreach ($vf in $verFiles) {
    $c = [IO.File]::ReadAllText($vf.FullName)
    if ($c -match '5\.8\.4') {
        $nc = $c -replace 'v5\.8\.4', 'v1.0.0' -replace '5\.8\.4', '1.0.0' -replace '5,8,4', '1,0,0'
        [IO.File]::WriteAllText($vf.FullName, $nc, $utf8NoBom)
        Write-Host "Updated version to v1.0.0 in: $($vf.FullName)"
    }
}

# ==============================================================================
# 2. INSPECT EXACT ICON NAMES IN Dekubi_ui.cpp, game_controls.cpp, home.cpp
# ==============================================================================
$uiFiles = Get-ChildItem -Path "$root\src\yuzu" -Recurse -File | Where-Object {
    $_.Name -match 'Dekubi|fritchii|game_controls|home|saves_window|whats_new|perf_overlay' -and $_.Extension -in @('.cpp','.h')
}

$discoveredIconNames = New-Object System.Collections.Generic.HashSet[string]
# Pre-populate standard icon names used by sidebar and topbar
$baseNames = @(
    "pause", "play", "resume", "restart", "reset", "reload", "stop", "exit", "quit", "close",
    "fullscreen", "maximize", "minimize", "restore", "windowed",
    "menu", "bars", "hamburger", "sidebar", "navigation",
    "home", "library", "games", "grid", "list", "carousel", "tree", "folder", "rom",
    "settings", "configure", "config", "gear", "cog", "options", "preferences", "sliders",
    "help", "info", "about", "question", "support", "docs",
    "saves", "save", "sdcard", "nand", "user", "profile",
    "whats_new", "news", "star", "bell", "update", "megaphone", "sparkles",
    "controller", "gamepad", "joycon", "audio", "volume", "mute", "speaker", "screenshot", "camera"
)
foreach ($bn in $baseNames) { $null = $discoveredIconNames.Add($bn) }

foreach ($uf in $uiFiles) {
    Write-Host "=== Scanning UI File: $($uf.FullName) ==="
    $c = [IO.File]::ReadAllText($uf.FullName)

    # Print every line that mentions icons, svg, png, button creation, or version
    $lines = $c -split "`r?`n"
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match 'svg|png|Icon|icon|Button|btn|:/|arg\(|5\.8|Title') {
            Write-Host ("  L{0}: {1}" -f ($i+1), $lines[$i].Trim())
        }
    }

    # Extract all short alphanumeric string literals (potential icon names passed to :/icons/%1.svg)
    $strLiterals = [regex]::Matches($c, '"([a-zA-Z0-9_\-\/]{2,40})"')
    foreach ($sm in $strLiterals) {
        $val = $sm.Groups[1].Value
        $leaf = ($val -split '/')[-1] -replace '\.(svg|png)$', ''
        if ($leaf -match '^[a-zA-Z0-9_\-]+$' -and $leaf.Length -ge 2 -and $leaf.Length -le 28) {
            $null = $discoveredIconNames.Add($leaf)
            $null = $discoveredIconNames.Add($leaf.ToLower())
        }
    }
}

Write-Host "Total icon names queued for generation: $($discoveredIconNames.Count)"

# ==============================================================================
# 3. GENERATE SVG & PNG FILES FOR EVERY ICON NAME
# ==============================================================================
$svgTemplates = @{
    "pause"      = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e8eaed" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><rect x="6" y="4" width="4" height="16" rx="1" fill="#e8eaed"/><rect x="14" y="4" width="4" height="16" rx="1" fill="#e8eaed"/></svg>'
    "play"       = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="#e8eaed" stroke="#e8eaed" stroke-width="2" stroke-linejoin="round"><polygon points="6 4 20 12 6 20 6 4"/></svg>'
    "restart"    = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e8eaed" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"/><path d="M3 3v5h5"/></svg>'
    "stop"       = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="#e8eaed" stroke="#e8eaed" stroke-width="2" stroke-linejoin="round"><rect x="5" y="5" width="14" height="14" rx="2"/></svg>'
    "fullscreen" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e8eaed" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/></svg>'
    "menu"       = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e8eaed" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><line x1="4" y1="6" x2="20" y2="6"/><line x1="4" y1="12" x2="20" y2="12"/><line x1="4" y1="18" x2="20" y2="18"/></svg>'
    "home"       = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e8eaed" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/><polyline points="9 22 9 12 15 12 15 22"/></svg>'
    "library"    = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e8eaed" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="14" width="7" height="7" rx="1.5"/><rect x="3" y="14" width="7" height="7" rx="1.5"/></svg>'
    "saves"      = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e8eaed" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/></svg>'
    "settings"   = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e8eaed" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>'
    "help"       = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e8eaed" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>'
    "whats_new"  = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e8eaed" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>'
}

function Get-TemplateKey([string]$name) {
    $n = $name.ToLower()
    if ($n -match "pause") { return "pause" }
    if ($n -match "play|resume|start") { return "play" }
    if ($n -match "restart|reset|reload|refresh|rotate") { return "restart" }
    if ($n -match "stop|exit|quit|close|power|off") { return "stop" }
    if ($n -match "full|max|expand|screen|window|restore|min") { return "fullscreen" }
    if ($n -match "menu|bar|ham|nav|side|drawer|list") { return "menu" }
    if ($n -match "home|house|main") { return "home" }
    if ($n -match "lib|grid|game|rom|card|app|tile|folder|controller|pad") { return "library" }
    if ($n -match "save|sd|nand|disk|data|backup|user|prof") { return "saves" }
    if ($n -match "set|conf|gear|cog|opt|pref|slid|tool|wrench") { return "settings" }
    if ($n -match "new|star|spark|bell|upd|news|change") { return "whats_new" }
    return "help"
}

$targetIconDirs = @(
    "$root\src\yuzu\icons",
    "$root\src\yuzu\fritchii",
    "$root\src\yuzu\fritchii\icons",
    "$root\dist\icons"
)
foreach ($td in $targetIconDirs) { $null = New-Item -ItemType Directory -Path $td -Force }

foreach ($iconName in $discoveredIconNames) {
    $tKey = Get-TemplateKey $iconName
    $svgXml = $svgTemplates[$tKey]
    foreach ($td in $targetIconDirs) {
        $svgPath = Join-Path $td "$iconName.svg"
        if (-not (Test-Path $svgPath)) {
            [IO.File]::WriteAllText($svgPath, $svgXml, $utf8NoBom)
        }
    }
}

# ==============================================================================
# 4. SOFTEN BRIGHT GREEN TO SEMI-TRANSPARENT EMERALD GREEN ACROSS UI & QSS
# ==============================================================================
$uiColorTargets = Get-ChildItem -Path "$root\src\yuzu", "$root\src\qt_common", "$root\dist\qt_themes" -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
    $_.Extension -in @('.qss','.cpp','.h','.ui')
}

foreach ($ut in $uiColorTargets) {
    $c = [IO.File]::ReadAllText($ut.FullName)
    $nc = $c

    # Replace harsh #00ff00 / #5500c853 / remaining purples with soft translucent green
    $nc = $nc -replace '(?i)#00ff00\b', 'rgba(0, 200, 83, 85)'
    $nc = $nc -replace '(?i)#5500c853\b', 'rgba(0, 200, 83, 85)'
    $nc = $nc -replace '(?i)rgb\(\s*0\s*,\s*255\s*,\s*0\s*\)', 'rgba(0, 200, 83, 85)'
    $nc = $nc -replace '(?i)rgba\(\s*0\s*,\s*255\s*,\s*0\s*,\s*255\s*\)', 'rgba(0, 200, 83, 85)'
    $nc = $nc -replace '(?i)rgba\(\s*0\s*,\s*255\s*,\s*0\s*,\s*1(\.0+)?\s*\)', 'rgba(0, 200, 83, 0.35)'
    $nc = $nc -replace 'QColor\(\s*0\s*,\s*255\s*,\s*0\s*\)', 'QColor(0, 200, 83, 90)'
    $nc = $nc -replace 'QColor\(\s*0\s*,\s*200\s*,\s*83\s*,\s*110\s*\)', 'QColor(0, 200, 83, 85)'

    # Fix any C++ QColor("#rgba(...)") invalid syntax back to valid hex with alpha (#5500c853)
    $nc = $nc -replace '"rgba\(0,\s*200,\s*83,\s*85\)"', '"#5500c853"'

    if ($c -cne $nc) {
        [IO.File]::WriteAllText($ut.FullName, $nc, $utf8NoBom)
        Write-Host "Applied soft translucent green in: $($ut.Name)"
    }
}

# ==============================================================================
# 5. REGISTER ALL ICONS UNDER :/icons/%1.svg IN yuzu.qrc & REBUILD
# ==============================================================================
$yuzuQrc = "$root\src\yuzu\yuzu.qrc"
if (Test-Path $yuzuQrc) {
    try { git checkout -- "src/yuzu/yuzu.qrc" 2>$null } catch {}
    $yqc = [IO.File]::ReadAllText($yuzuQrc)
    # Strip any previous custom injected blocks if git checkout didn't reset it
    $firstClose = $yqc.IndexOf("</qresource>")
    if ($firstClose -gt 0) {
        $yqc = $yqc.Substring(0, $firstClose + 12) + "`n</RCC>"
    }

    $yuzuDir = "$root\src\yuzu"
    $extraEntries = New-Object System.Collections.Generic.List[string]
    $iconFiles = Get-ChildItem -Path "$root\src\yuzu\icons", "$root\src\yuzu\fritchii" -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
        $_.Extension -in @('.svg','.png','.ico','.qss') -and $_.Name -notmatch '%|\{|\}'
    }

    foreach ($ic in $iconFiles) {
        $u1 = New-Object System.Uri("$yuzuDir\")
        $u2 = New-Object System.Uri($ic.FullName)
        $relFromYuzu = [System.Uri]::UnescapeDataString($u1.MakeRelativeUri($u2).ToString())
        $fileName = $ic.Name
        $extraEntries.Add("        <file alias=`"$fileName`">$relFromYuzu</file>")
        $extraEntries.Add("        <file alias=`"icons/$fileName`">$relFromYuzu</file>")
    }

    $uniqueExtra = ($extraEntries | Select-Object -Unique) -join "`n"
    $injected = @"
    <qresource prefix="/">
$uniqueExtra
    </qresource>
    <qresource prefix="/icons">
$uniqueExtra
    </qresource>
    <qresource prefix="/fritchii">
$uniqueExtra
    </qresource>
    <qresource prefix="/dekubi">
$uniqueExtra
    </qresource>
</RCC>
"@
    $yqc = $yqc -replace '</RCC>\s*$', $injected
    [IO.File]::WriteAllText($yuzuQrc, $yqc, $utf8NoBom)
    Write-Host "Registered all :/icons/*.svg entries in yuzu.qrc"
}

# Delete cached object files so common (version) and yuzu (icons/UI) recompile
Remove-Item "$root\build\src\common\common.dir\Release\*.obj" -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path "$root\build\src\yuzu" -Recurse -File -Filter "*qrc_*" -ErrorAction SilentlyContinue | Remove-Item -Force
Get-ChildItem -Path "$root\build\src\yuzu\dekubi.dir\Release" -File -Filter "*.obj" -ErrorAction SilentlyContinue | Where-Object {
    $_.Name -match 'Dekubi|fritchii|game_controls|home|main_window|about|qrc'
} | Remove-Item -Force

$env:CL = "/arch:AVX2 /O2 /Oi /Ot /fp:fast /wd4456 /wd4457 /wd4458 /wd4459"
$env:_LINK_ = ""

cmake --build build --config Release --target dekubi --parallel 12
if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE" }

# Copy binary and SVG DLLs into Dekubi_Ally and repackage ZIP
$distDir = "$root\Dekubi_Ally"
Copy-Item "$root\build\bin\Release\dekubi.exe" "$distDir\Dekubi.exe" -Force

$qtRoot = "$root\.cache\cpm\qt6\6.11.1-windows-amd64"
foreach ($dll in @("Qt6Svg.dll", "Qt6SvgWidgets.dll")) {
    if (Test-Path "$qtRoot\bin\$dll") {
        Copy-Item "$qtRoot\bin\$dll" "$distDir\$dll" -Force
        Copy-Item "$qtRoot\bin\$dll" "$root\build\bin\Release\$dll" -Force
    }
}

$zip = "$root\Dekubi_Ally_Z1E.zip"
Remove-Item $zip -Force -ErrorAction SilentlyContinue
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($distDir, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)
Write-Host "Successfully built v1.0.0, generated :/icons/*.svg icons, softened green with transparency, and updated: $zip"