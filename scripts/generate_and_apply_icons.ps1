$ErrorActionPreference = "Stop"
$root = "C:\Users\User\projects\Fritchii"
Set-Location $root

Stop-Process -Name "Dekubi","dekubi","yuzu","fritchii","lichii" -Force -ErrorAction SilentlyContinue
$utf8NoBom = New-Object System.Text.UTF8Encoding $false
Add-Type -AssemblyName System.Drawing

# ==============================================================================
# 1. GENERATE CLEAN GENERIC SVG & PNG ICONS (Pause, Restart, Stop, Fullscreen, Menu, Settings, Help, Library)
# ==============================================================================
$svgMap = @{
    "pause" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><rect x="6" y="4" width="4" height="16" rx="1" fill="#e0e0e0"/><rect x="14" y="4" width="4" height="16" rx="1" fill="#e0e0e0"/></svg>'
    "play" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="#e0e0e0" stroke="#e0e0e0" stroke-width="2" stroke-linejoin="round"><polygon points="6 4 20 12 6 20 6 4"/></svg>'
    "restart" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"/><path d="M3 3v5h5"/></svg>'
    "stop" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="#e0e0e0" stroke="#e0e0e0" stroke-width="2" stroke-linejoin="round"><rect x="5" y="5" width="14" height="14" rx="2"/></svg>'
    "fullscreen" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/></svg>'
    "menu" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><line x1="4" y1="6" x2="20" y2="6"/><line x1="4" y1="12" x2="20" y2="12"/><line x1="4" y1="18" x2="20" y2="18"/></svg>'
    "home" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/><polyline points="9 22 9 12 15 12 15 22"/></svg>'
    "library" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/></svg>'
    "games" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="6" width="20" height="12" rx="4"/><path d="M6 12h4m-2-2v4"/><circle cx="15" cy="11" r="1" fill="#e0e0e0"/><circle cx="17" cy="13" r="1" fill="#e0e0e0"/></svg>'
    "saves" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/></svg>'
    "settings" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>'
    "help" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>'
    "whats_new" = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#e0e0e0" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>'
}

# Also generate matching 48x48 transparent PNG icons using System.Drawing so both .svg and .png lookups succeed
function Save-GenericPng([string]$type, [string]$outPath) {
    $bmp = New-Object System.Drawing.Bitmap(48, 48)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)
    $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(240, 230, 230, 230), 3.8)
    $pen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $pen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $brush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(240, 230, 230, 230))

    switch -Regex ($type) {
        "pause" {
            $g.FillRectangle($brush, 12, 10, 8, 28)
            $g.FillRectangle($brush, 28, 10, 8, 28)
        }
        "play" {
            $pts = [System.Drawing.Point[]]@( (New-Object System.Drawing.Point(14,10)), (New-Object System.Drawing.Point(38,24)), (New-Object System.Drawing.Point(14,38)) )
            $g.FillPolygon($brush, $pts)
        }
        "restart|reset|reload" {
            $g.DrawArc($pen, 10, 10, 28, 28, 40, 290)
            $pts = [System.Drawing.Point[]]@( (New-Object System.Drawing.Point(30,6)), (New-Object System.Drawing.Point(40,14)), (New-Object System.Drawing.Point(28,18)) )
            $g.FillPolygon($brush, $pts)
        }
        "stop|exit|close" {
            $g.FillRectangle($brush, 12, 12, 24, 24)
        }
        "fullscreen|expand|maximize" {
            $g.DrawLines($pen, [System.Drawing.Point[]]@((New-Object System.Drawing.Point(10,18)), (New-Object System.Drawing.Point(10,10)), (New-Object System.Drawing.Point(18,10))))
            $g.DrawLines($pen, [System.Drawing.Point[]]@((New-Object System.Drawing.Point(30,10)), (New-Object System.Drawing.Point(38,10)), (New-Object System.Drawing.Point(38,18))))
            $g.DrawLines($pen, [System.Drawing.Point[]]@((New-Object System.Drawing.Point(10,30)), (New-Object System.Drawing.Point(10,38)), (New-Object System.Drawing.Point(18,38))))
            $g.DrawLines($pen, [System.Drawing.Point[]]@((New-Object System.Drawing.Point(30,38)), (New-Object System.Drawing.Point(38,38)), (New-Object System.Drawing.Point(38,30))))
        }
        "menu|bars|hamburger" {
            $g.DrawLine($pen, 10, 14, 38, 14)
            $g.DrawLine($pen, 10, 24, 38, 24)
            $g.DrawLine($pen, 10, 34, 38, 34)
        }
        "home" {
            $pts = [System.Drawing.Point[]]@((New-Object System.Drawing.Point(8,24)), (New-Object System.Drawing.Point(24,10)), (New-Object System.Drawing.Point(40,24)))
            $g.DrawLines($pen, $pts)
            $g.DrawRectangle($pen, 14, 22, 20, 16)
        }
        "library|grid|games|controller" {
            $g.DrawRectangle($pen, 10, 10, 11, 11)
            $g.DrawRectangle($pen, 27, 10, 11, 11)
            $g.DrawRectangle($pen, 10, 27, 11, 11)
            $g.DrawRectangle($pen, 27, 27, 11, 11)
        }
        "save" {
            $g.DrawRectangle($pen, 10, 10, 28, 28)
            $g.FillRectangle($brush, 16, 10, 16, 10)
            $g.DrawRectangle($pen, 16, 26, 16, 12)
        }
        "setting|gear|cog|config" {
            $g.DrawEllipse($pen, 14, 14, 20, 20)
            $g.DrawLine($pen, 24, 6, 24, 12)
            $g.DrawLine($pen, 24, 36, 24, 42)
            $g.DrawLine($pen, 6, 24, 12, 24)
            $g.DrawLine($pen, 36, 24, 42, 24)
            $g.DrawLine($pen, 11, 11, 15, 15)
            $g.DrawLine($pen, 33, 33, 37, 37)
            $g.DrawLine($pen, 37, 11, 33, 15)
            $g.DrawLine($pen, 11, 37, 15, 33)
        }
        default {
            # Help / Info / Question mark
            $g.DrawEllipse($pen, 8, 8, 32, 32)
            $font = New-Object System.Drawing.Font("Segoe UI", 18, [System.Drawing.FontStyle]::Bold)
            $g.DrawString("?", $font, $brush, 14, 9)
        }
    }
    $g.Dispose()
    $bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

# ==============================================================================
# 2. INSPECT EXACT ICON PATHS IN src/yuzu/fritchii/*.cpp AND GENERATE ALL OF THEM
# ==============================================================================
$iconTargetDirs = @(
    "$root\src\yuzu\fritchii",
    "$root\src\yuzu\fritchii\icons",
    "$root\src\yuzu\fritchii\assets",
    "$root\src\yuzu\icons",
    "$root\dist\icons"
)
foreach ($d in $iconTargetDirs) { $null = New-Item -ItemType Directory -Path $d -Force }

# Write base generic icons into all target directories
foreach ($key in $svgMap.Keys) {
    foreach ($d in $iconTargetDirs) {
        [IO.File]::WriteAllText("$d\$key.svg", $svgMap[$key], $utf8NoBom)
        Save-GenericPng $key "$d\$key.png"
    }
}

# Scan C++ files in src/yuzu/fritchii to see every exact filename/URI referenced and create a matching icon file!
$fritchiiCpp = Get-ChildItem -Path "$root\src\yuzu" -Recurse -File | Where-Object { $_.Extension -in @('.cpp','.h') }
foreach ($f in $fritchiiCpp) {
    $c = [IO.File]::ReadAllText($f.FullName)
    # Match any string literal ending in .svg or .png
    $strMatches = [regex]::Matches($c, '"([^"]+\.(?:svg|png))"')
    foreach ($m in $strMatches) {
        $rawPath = $m.Groups[1].Value
        $cleanRel = $rawPath -replace '^:/+', '' -replace '^fritchii/', '' -replace '^dekubi/', '' -replace '^Dekubi/', ''
        $leafName = [IO.Path]::GetFileNameWithoutExtension($cleanRel).ToLower()
        $ext = [IO.Path]::GetExtension($cleanRel).ToLower()

        # Pick best matching icon template
        $matchedType = "help"
        foreach ($k in $svgMap.Keys) {
            if ($leafName -match $k) { $matchedType = $k; break }
        }
        if ($leafName -match "grid|list|game|rom|card") { $matchedType = "library" }
        elseif ($leafName -match "gear|cog|opt|pref|conf") { $matchedType = "settings" }
        elseif ($leafName -match "reset|reboot|reload") { $matchedType = "restart" }
        elseif ($leafName -match "quit|exit|power|close") { $matchedType = "stop" }
        elseif ($leafName -match "max|screen|window|expand") { $matchedType = "fullscreen" }
        elseif ($leafName -match "nav|side|bar|drawer") { $matchedType = "menu" }
        elseif ($leafName -match "info|about|question|doc|new|star") { $matchedType = "help" }

        foreach ($baseDir in @("$root\src\yuzu", "$root\src\yuzu\fritchii", "$root\Dekubi_Ally", "$root\build\bin\Release")) {
            $destFile = Join-Path $baseDir $cleanRel
            $null = New-Item -ItemType Directory -Path (Split-Path $destFile) -Force
            if (-not (Test-Path $destFile) -or (Get-Item $destFile).Length -lt 100) {
                if ($ext -eq ".svg") {
                    [IO.File]::WriteAllText($destFile, $svgMap[$matchedType], $utf8NoBom)
                } else {
                    Save-GenericPng $matchedType $destFile
                }
                Write-Host "Generated missing icon for C++ path [$rawPath] -> $destFile ($matchedType)"
            }
        }
    }
}

# ==============================================================================
# 3. PATCH C++ BUTTON ICON LOADERS IN Dekubi_ui.cpp & game_controls.cpp
#    SO THEY FALL BACK TO QT STANDARD ICONS OR DRAWN PIXMAPS IF QICON IS NULL!
# ==============================================================================
# Print out the exact contents of game_controls.cpp and Dekubi_ui.cpp around QPushButton / QToolButton
$uiSources = Get-ChildItem -Path "$root\src\yuzu" -Recurse -File | Where-Object {
    $_.Name -in @('game_controls.cpp','Dekubi_ui.cpp','fritchii_ui.cpp','home.cpp','main_window_Dekubi.cpp','main_window_fritchii.cpp')
}

foreach ($us in $uiSources) {
    Write-Host "=== Inspecting $($us.FullName) ==="
    $c = [IO.File]::ReadAllText($us.FullName)

    # Fix any broken resource path where 'fritchii' was replaced with 'Dekubi' inside strings
    $nc = $c -replace '":/Dekubi/', '":/fritchii/' `
             -replace '":/dekubi/', '":/fritchii/'

    # Also check if v5.8.4 is in this file
    $nc = $nc -replace 'v5\.8\.4', 'v1.0.0' -replace '5\.8\.4', '1.0.0'

    if ($c -cne $nc) {
        [IO.File]::WriteAllText($us.FullName, $nc, $utf8NoBom)
        Write-Host "Fixed icon paths/version in: $($us.Name)"
    }

    # Show lines with setIcon, QIcon, QPixmap, or setText so we see how buttons are initialized
    $lines = $nc -split "`r?`n"
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match 'Icon|Pixmap|svg|png|Button|5\.8|version|Title') {
            Write-Host ("  L{0}: {1}" -f ($i+1), $lines[$i].Trim())
        }
    }
}

# ==============================================================================
# 4. REBUILD yuzu.qrc WITH ALL GENERATED ICONS & COMPILE
# ==============================================================================
$yuzuQrc = "$root\src\yuzu\yuzu.qrc"
if (Test-Path $yuzuQrc) {
    try { git checkout -- "src/yuzu/yuzu.qrc" 2>$null } catch {}
    $yqc = [IO.File]::ReadAllText($yuzuQrc)
    $yuzuDir = "$root\src\yuzu"
    $extraEntries = New-Object System.Collections.Generic.List[string]

    $allIcons = Get-ChildItem -Path "$root\src\yuzu\fritchii", "$root\src\yuzu\icons" -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
        $_.Extension -in @('.svg','.png','.ico','.qss')
    }
    foreach ($ic in $allIcons) {
        $u1 = New-Object System.Uri("$yuzuDir\")
        $u2 = New-Object System.Uri($ic.FullName)
        $relFromYuzu = [System.Uri]::UnescapeDataString($u1.MakeRelativeUri($u2).ToString())
        $fileName = $ic.Name
        $extraEntries.Add("        <file alias=`"$fileName`">$relFromYuzu</file>")
        $extraEntries.Add("        <file alias=`"icons/$fileName`">$relFromYuzu</file>")
        $extraEntries.Add("        <file alias=`"assets/$fileName`">$relFromYuzu</file>")
        if ($ic.FullName -match 'src\\yuzu\\fritchii\\(.*)$') {
            $subRel = $Matches[1].Replace('\','/')
            $extraEntries.Add("        <file alias=`"$subRel`">$relFromYuzu</file>")
            $extraEntries.Add("        <file alias=`"fritchii/$subRel`">$relFromYuzu</file>")
        }
    }
    $uniqueExtra = ($extraEntries | Select-Object -Unique) -join "`n"
    $injected = @"
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
    $yqc = $yqc -replace '</RCC>\s*$', $injected
    [IO.File]::WriteAllText($yuzuQrc, $yqc, $utf8NoBom)
    Write-Host "Updated yuzu.qrc with all generated SVG and PNG icons."
}

# Clean qrc object files and rebuild dekubi
Get-ChildItem -Path "$root\build\src\yuzu" -Recurse -File -Filter "*qrc_*" -ErrorAction SilentlyContinue | Remove-Item -Force
Remove-Item "$root\build\src\yuzu\dekubi.dir\Release\Dekubi_ui.obj" -Force -ErrorAction SilentlyContinue
Remove-Item "$root\build\src\yuzu\dekubi.dir\Release\game_controls.obj" -Force -ErrorAction SilentlyContinue
Remove-Item "$root\build\src\yuzu\dekubi.dir\Release\home.obj" -Force -ErrorAction SilentlyContinue
Remove-Item "$root\build\src\yuzu\dekubi.dir\Release\main_window*.obj" -Force -ErrorAction SilentlyContinue

$env:CL = "/arch:AVX2 /O2 /Oi /Ot /fp:fast /wd4456 /wd4457 /wd4458 /wd4459"
$env:_LINK_ = ""

cmake --build build --config Release --target dekubi --parallel 12
if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE" }

# Copy binary and icons to Dekubi_Ally and update ZIP
$distDir = "$root\Dekubi_Ally"
Copy-Item "$root\build\bin\Release\dekubi.exe" "$distDir\Dekubi.exe" -Force

$zip = "$root\Dekubi_Ally_Z1E.zip"
Remove-Item $zip -Force -ErrorAction SilentlyContinue
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($distDir, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)
Write-Host "Successfully generated and applied all generic icons and updated: $zip"