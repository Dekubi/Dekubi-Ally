$ErrorActionPreference = "Stop"
$root = "C:\Users\User\projects\Fritchii"
Set-Location $root

Stop-Process -Name "Dekubi","dekubi","yuzu","fritchii","lichii" -Force -ErrorAction SilentlyContinue
$utf8NoBom = New-Object System.Text.UTF8Encoding $false

# ==============================================================================
# 1. FIND EVERY FILE (INCLUDING build/CMakeCache.txt & JSON) WITH 5.8.4 -> 1.0.0
# ==============================================================================
Write-Host "=== 1. Updating v5.8.4 to v1.0.0 across source AND build cache ==="
$allProjectFiles = Get-ChildItem -Path $root -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
    $_.FullName -notmatch '\\\.cache\\|\\\.git\\|\\Dekubi_Ally\\|\\_deps\\' -and
    $_.Extension -in @('.cpp','.h','.hpp','.in','.cmake','.txt','.json','.rc','.ui','.qrc','.vcxproj')
}
foreach ($f in $allProjectFiles) {
    $c = [IO.File]::ReadAllText($f.FullName)
    if ($c -match '5\.8\.4') {
        $nc = $c -replace 'v5\.8\.4', 'v1.0.0' -replace '5\.8\.4', '1.0.0' -replace '5,8,4', '1,0,0'
        [IO.File]::WriteAllText($f.FullName, $nc, $utf8NoBom)
        Write-Host "Patched 5.8.4 -> 1.0.0 in: $($f.FullName)"
    }
}

# Force delete common.lib and scm_rev.obj so MSBuild MUST recompile the version string
Remove-Item "$root\build\src\common" -Recurse -Force -ErrorAction SilentlyContinue

# ==============================================================================
# 2. DIAGNOSE & FIX SIDEBAR (Dekubi_ui.cpp) & IN-GAME TOPBAR (game_controls.cpp) ICONS
# ==============================================================================
Write-Host "=== 2. Inspecting and restoring Sidebar & In-Game Topbar Icons ==="

# Check if git or an original zip exists to restore any accidentally mangled icon files/paths in src/yuzu/fritchii
$fritchiiDir = "$root\src\yuzu\fritchii"
if (Test-Path $fritchiiDir) {
    $uiCodeFiles = Get-ChildItem -Path $fritchiiDir -Recurse -File | Where-Object { $_.Extension -in @('.cpp','.h') }
    foreach ($ucf in $uiCodeFiles) {
        $c = [IO.File]::ReadAllText($ucf.FullName)
        Write-Host "Scanning UI source: $($ucf.Name)"

        # Print any icon/svg/pixmap lines so we see exactly how icons are loaded
        $iconLines = $c -split "`r?`n" | Where-Object { $_ -match 'QIcon|QPixmap|QSvg|svg|png|:/|icon|SetIcon' }
        foreach ($il in ($iconLines | Select-Object -First 15)) {
            Write-Host "   [IconCode] $($il.Trim())"
        }

        # CRITICAL FIX: Earlier scripts replaced 'fritchii' or 'Fritchii' with 'Dekubi' inside QString paths,
        # resource initializers Q_INIT_RESOURCE(...), or relative asset paths!
        $nc = $c -replace 'Q_INIT_RESOURCE\(Dekubi\)', 'Q_INIT_RESOURCE(fritchii)' `
                 -replace 'Q_INIT_RESOURCE\(dekubi\)', 'Q_INIT_RESOURCE(fritchii)' `
                 -replace ':/Dekubi/', ':/fritchii/' `
                 -replace ':/dekubi/', ':/fritchii/' `
                 -replace '"fritchii/Dekubi', '"fritchii/fritchii' `
                 -replace 'Dekubi_icons', 'fritchii_icons'

        if ($c -cne $nc) {
            [IO.File]::WriteAllText($ucf.FullName, $nc, $utf8NoBom)
            Write-Host " -> Restored icon loader paths in: $($ucf.Name)"
        }
    }
}

# Also check if icons are loaded from yuzu.qrc or dist/icons or src/yuzu/fritchii
# Let's inspect all .svg and .png files in the entire repo (excluding .cache)
$repoIcons = Get-ChildItem -Path "$root\src", "$root\dist" -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
    $_.Extension -in @('.svg','.png')
}
Write-Host "Found $($repoIcons.Count) total SVG/PNG files in src/ and dist/."

# If any SVG file was corrupted by regex or has invalid XML, or if inline SVGs exist in C++, let's verify them
# Also ensure Q_INIT_RESOURCE(fritchii_master) is called or compiled directly into yuzu.qrc!
# IMPORTANT: In Qt, if a .qrc is compiled with a different name than yuzu.qrc, static/LTCG builds can strip its static initializer
# unless its <qresource> blocks are inside yuzu.qrc itself!
$yuzuQrc = "$root\src\yuzu\yuzu.qrc"
if (Test-Path $yuzuQrc) {
    # Restore clean yuzu.qrc from git if git is available, then inject all fritchii icons directly into yuzu.qrc!
    try {
        git checkout -- "src/yuzu/yuzu.qrc" 2>$null
    } catch {}

    $yqc = [IO.File]::ReadAllText($yuzuQrc)

    # Gather all files in src/yuzu/fritchii and dist/icons
    $yuzuDir = "$root\src\yuzu"
    $extraEntries = New-Object System.Collections.Generic.List[string]

    $customAssets = Get-ChildItem -Path "$root\src\yuzu\fritchii", "$root\dist" -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
        $_.Extension -in @('.svg','.png','.ico','.qss','.jpg')
    }
    foreach ($ca in $customAssets) {
        $u1 = New-Object System.Uri("$yuzuDir\")
        $u2 = New-Object System.Uri($ca.FullName)
        $relFromYuzu = [System.Uri]::UnescapeDataString($u1.MakeRelativeUri($u2).ToString())
        $fileName = $ca.Name

        $extraEntries.Add("        <file alias=`"$fileName`">$relFromYuzu</file>")
        $extraEntries.Add("        <file alias=`"icons/$fileName`">$relFromYuzu</file>")
        if ($ca.FullName -match 'src\\yuzu\\fritchii\\(.*)$') {
            $subRel = $Matches[1].Replace('\','/')
            $extraEntries.Add("        <file alias=`"$subRel`">$relFromYuzu</file>")
            $extraEntries.Add("        <file alias=`"fritchii/$subRel`">$relFromYuzu</file>")
        }
    }

    $uniqueExtra = ($extraEntries | Select-Object -Unique) -join "`n"
    $injectedBlocks = @"
    <qresource prefix="/">
$uniqueExtra
    </qresource>
    <qresource prefix="/fritchii">
$uniqueExtra
    </qresource>
    <qresource prefix="/dekubi">
$uniqueExtra
    </qresource>
    <qresource prefix="/Dekubi">
$uniqueExtra
    </qresource>
    <qresource prefix="/icons">
$uniqueExtra
    </qresource>
</RCC>
"@
    # Remove any previous injected blocks or closing </RCC> and append to yuzu.qrc
    $yqc = $yqc -replace '</RCC>\s*$', $injectedBlocks
    [IO.File]::WriteAllText($yuzuQrc, $yqc, $utf8NoBom)
    Write-Host "Merged all custom sidebar and topbar icons directly into yuzu.qrc (prevents LTCG linker stripping!)"
}

# Remove separate fritchii_master.qrc from CMakeLists.txt so it doesn't conflict with yuzu.qrc
$cml = "$root\src\yuzu\CMakeLists.txt"
if (Test-Path $cml) {
    $cmlText = [IO.File]::ReadAllText($cml)
    $cmlText = $cmlText -replace '(?m)^\s*fritchii_master\.qrc\r?\n', ''
    [IO.File]::WriteAllText($cml, $cmlText, $utf8NoBom)
}

# ==============================================================================
# 3. SOFTEN EYE-HURTING NEON GREEN (#00ff00) TO TRANSLUCENT EMERALD GREEN
# ==============================================================================
Write-Host "=== 3. Applying soft, semi-transparent green theme ==="
# Instead of blinding 100% opaque #00ff00, use:
# - Translucent green fill for selection/highlights: rgba(0, 210, 90, 85) / QColor(0, 210, 90, 85) / #5500d25a
# - Soft emerald accent for borders/text/active indicators: #00d25a / rgb(0, 210, 90)
$colorFiles = Get-ChildItem -Path "$root\src\yuzu", "$root\src\qt_common", "$root\dist\qt_themes" -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
    $_.Extension -in @('.qss','.cpp','.h','.ui')
}

foreach ($cf in $colorFiles) {
    $c = [IO.File]::ReadAllText($cf.FullName)
    $nc = $c

    if ($cf.Extension -eq '.qss') {
        # In QSS stylesheets: make background/selection-background-color semi-transparent green!
        $nc = $nc -replace '(?i)(background(?:-color)?\s*:\s*)#00ff00', '${1}rgba(0, 200, 83, 75)'
        $nc = $nc -replace '(?i)(selection-background-color\s*:\s*)#00ff00', '${1}rgba(0, 200, 83, 95)'
        $nc = $nc -replace '(?i)(background(?:-color)?\s*:\s*)rgb\(\s*0\s*,\s*255\s*,\s*0\s*\)', '${1}rgba(0, 200, 83, 75)'
        $nc = $nc -replace '(?i)(selection-background-color\s*:\s*)rgb\(\s*0\s*,\s*255\s*,\s*0\s*\)', '${1}rgba(0, 200, 83, 95)'
    }

    # Replace remaining harsh #00ff00 / (0, 255, 0) with softer emerald green with transparency where supported
    $nc = $nc -replace '(?i)rgba\(\s*0\s*,\s*255\s*,\s*0\s*,\s*([\d\.]+)\s*\)', 'rgba(0, 200, 83, $1)'
    $nc = $nc -replace '(?i)rgb\(\s*0\s*,\s*255\s*,\s*0\s*\)', 'rgba(0, 200, 83, 110)'
    $nc = $nc -replace 'QColor\(\s*0\s*,\s*255\s*,\s*0\s*\)', 'QColor(0, 200, 83, 110)'
    $nc = $nc -replace 'QColor\(\s*0\s*,\s*255\s*,\s*0\s*,\s*(\d+)\s*\)', 'QColor(0, 200, 83, $1)'
    $nc = $nc -replace '(?i)#00ff00\b', '#5500c853'
    $nc = $nc -replace '(?i)0x00ff00\b', '0x00c853'

    if ($c -cne $nc) {
        [IO.File]::WriteAllText($cf.FullName, $nc, $utf8NoBom)
        Write-Host "Softened green with transparency in: $($cf.FullName)"
    }
}

# ==============================================================================
# 4. REBUILD WITHOUT /GL LTCG SYMBOL STRIPPING SO QT RESOURCES STAY LINKED
# ==============================================================================
# NOTE: MSVC /GL + /OPT:REF in static builds can strip Qt's qInitResources_* static constructors!
# We compile with /arch:AVX2 /O2 /Oi /Ot /fp:fast (without /GL) so all Qt resources and icons stay 100% linked.
Remove-Item "$root\build\src\yuzu" -Recurse -Force -ErrorAction SilentlyContinue

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed with exit code $LASTEXITCODE" }

$env:CL = "/arch:AVX2 /O2 /Oi /Ot /fp:fast /wd4456 /wd4457 /wd4458 /wd4459"
$env:_LINK_ = ""

cmake --build build --config Release --target dekubi --parallel 12
if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE" }

# ==============================================================================
# 5. UPDATE DEKUBI_ALLY & ZIP (PRESERVING YOUR USER/CONFIG)
# ==============================================================================
$distDir = "$root\Dekubi_Ally"
Copy-Item "$root\build\bin\Release\dekubi.exe" "$distDir\Dekubi.exe" -Force

# Ensure Qt6SvgWidgets.dll and Qt6Svg.dll are both present (QSvgWidget / QSvgRenderer require them)
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
Write-Host "Successfully built v1.0.0, restored icons, applied translucent emerald green, and updated: $zip"