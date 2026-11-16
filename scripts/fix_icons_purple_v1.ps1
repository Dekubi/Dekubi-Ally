$ErrorActionPreference = "Stop"
$root = "C:\Users\User\projects\Fritchii"
Set-Location $root

Stop-Process -Name "Dekubi","dekubi","yuzu","fritchii","lichii" -Force -ErrorAction SilentlyContinue
$utf8NoBom = New-Object System.Text.UTF8Encoding $false

# ==============================================================================
# 1. CHANGE VERSION FROM v5.8.4 TO v1.0.0 ACROSS ALL SOURCE & BUILD FILES
# ==============================================================================
$verFiles = Get-ChildItem -Path "$root\src", "$root\CMakeModules" -Recurse -File
$verFiles += Get-Item "$root\CMakeLists.txt" -ErrorAction SilentlyContinue
foreach ($vf in $verFiles) {
    if ($vf.Extension -in @('.cpp','.h','.hpp','.in','.rc','.ui','.cmake','.txt')) {
        $c = [IO.File]::ReadAllText($vf.FullName)
        if ($c -match '5\.8\.4') {
            $nc = $c -replace 'v5\.8\.4', 'v1.0.0' -replace '5\.8\.4', '1.0.0' -replace '5,8,4', '1,0,0'
            [IO.File]::WriteAllText($vf.FullName, $nc, $utf8NoBom)
            Write-Host "Updated version to v1.0.0 in: $($vf.FullName)"
        }
    }
}

# ==============================================================================
# 2. FIX ALL MISSING SIDEBAR & IN-GAME TOPBAR ICONS
# ==============================================================================
# Scan C++ and UI files in src/yuzu to see every ":/..." resource path the code requests
$codeFiles = Get-ChildItem -Path "$root\src\yuzu" -Recurse -File | Where-Object { $_.Extension -in @('.cpp','.h','.ui','.qss') }
$requestedUris = New-Object System.Collections.Generic.HashSet[string]
foreach ($cf in $codeFiles) {
    $c = [IO.File]::ReadAllText($cf.FullName)
    # Fix any corrupted resource prefixes from earlier renames
    $nc = $c -replace ':/Dekubi/', ':/fritchii/' -replace ':/dekubi/', ':/fritchii/'
    if ($c -cne $nc) {
        [IO.File]::WriteAllText($cf.FullName, $nc, $utf8NoBom)
    }
    $matches = [regex]::Matches($nc, ':/([a-zA-Z0-9_/\-\.]+\.(?:svg|png|ico|qss|ttf|otf|jpg|gif))')
    foreach ($m in $matches) {
        $null = $requestedUris.Add($m.Groups[1].Value)
    }
}

Write-Host "Discovered $($requestedUris.Count) unique ':/' resource paths requested by UI code."

# Collect ALL icon/image/qss assets in src/yuzu and dist
$allAssets = Get-ChildItem -Path "$root\src\yuzu", "$root\dist" -Recurse -File | Where-Object {
    $_.Extension -in @('.svg','.png','.ico','.qss','.jpg')
}

# Recolor any purple SVG icons to #00ff00 before embedding
foreach ($asset in $allAssets) {
    if ($asset.Extension -eq '.svg') {
        $sc = [IO.File]::ReadAllText($asset.FullName)
        $nsc = [regex]::Replace($sc, '#([0-9a-fA-F]{6})\b', {
            param($m)
            $hex = $m.Groups[1].Value
            $r = [Convert]::ToInt32($hex.Substring(0,2), 16)
            $g = [Convert]::ToInt32($hex.Substring(2,2), 16)
            $b = [Convert]::ToInt32($hex.Substring(4,2), 16)
            if ($b -gt ($g + 10) -and $r -gt ($g + 5) -and $b -ge 40) { return "#00ff00" }
            return $m.Value
        })
        if ($sc -cne $nsc) {
            [IO.File]::WriteAllText($asset.FullName, $nsc, $utf8NoBom)
            Write-Host "Recolored SVG icon to green: $($asset.Name)"
        }
    }
}

# Build a master fritchii_assets.qrc in src/yuzu/ that maps every icon in src/yuzu/fritchii and dist/
# under BOTH its relative path AND its filename alias across all prefixes (/, /fritchii, /dekubi, /yuzu, /icons)
$yuzuDir = "$root\src\yuzu"
$masterLines = New-Object System.Collections.Generic.List[string]

$fritchiiAssets = Get-ChildItem -Path "$root\src\yuzu\fritchii" -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
    $_.Extension -in @('.svg','.png','.ico','.qss','.jpg','.ttf')
}
foreach ($fa in $fritchiiAssets) {
    $relFromYuzu = $fa.FullName.Substring($yuzuDir.Length + 1).Replace('\','/')
    $relFromFritchii = $fa.FullName.Substring("$root\src\yuzu\fritchii".Length + 1).Replace('\','/')
    $fileName = $fa.Name
    $masterLines.Add("        <file alias=`"$relFromFritchii`">$relFromYuzu</file>")
    if ($relFromFritchii -ne $fileName) {
        $masterLines.Add("        <file alias=`"$fileName`">$relFromYuzu</file>")
        $masterLines.Add("        <file alias=`"icons/$fileName`">$relFromYuzu</file>")
    }
    $masterLines.Add("        <file alias=`"$relFromYuzu`">$relFromYuzu</file>")
}

# Also check if any requested :/ URI points to a file in dist/ or src/yuzu/ that wasn't mapped
foreach ($uri in $requestedUris) {
    $leaf = Split-Path $uri -Leaf
    $found = $allAssets | Where-Object { $_.Name -ieq $leaf } | Select-Object -First 1
    if ($found) {
        # Compute relative path from src/yuzu
        $u1 = New-Object System.Uri("$yuzuDir\")
        $u2 = New-Object System.Uri($found.FullName)
        $relPath = [System.Uri]::UnescapeDataString($u1.MakeRelativeUri($u2).ToString())
        $masterLines.Add("        <file alias=`"$uri`">$relPath</file>")
    }
}

$uniqueMasterLines = $masterLines | Select-Object -Unique
$innerBlock = ($uniqueMasterLines -join "`n")

$masterQrcContent = @"
<RCC>
    <qresource prefix="/">
$innerBlock
    </qresource>
    <qresource prefix="/fritchii">
$innerBlock
    </qresource>
    <qresource prefix="/dekubi">
$innerBlock
    </qresource>
    <qresource prefix="/Dekubi">
$innerBlock
    </qresource>
    <qresource prefix="/yuzu">
$innerBlock
    </qresource>
    <qresource prefix="/lichii">
$innerBlock
    </qresource>
</RCC>
"@

$masterQrcPath = "$root\src\yuzu\fritchii_master.qrc"
[IO.File]::WriteAllText($masterQrcPath, $masterQrcContent, $utf8NoBom)
Write-Host "Generated master icon resource file: $masterQrcPath with $($uniqueMasterLines.Count) aliases"

# Ensure fritchii_master.qrc is compiled in src/yuzu/CMakeLists.txt
$cml = "$root\src\yuzu\CMakeLists.txt"
$cmlText = [IO.File]::ReadAllText($cml)
if ($cmlText -notmatch 'fritchii_master\.qrc') {
    $cmlText = $cmlText -replace '(\s+yuzu\.qrc)', "`$1`n    fritchii_master.qrc"
    [IO.File]::WriteAllText($cml, $cmlText, $utf8NoBom)
    Write-Host "Registered fritchii_master.qrc in src/yuzu/CMakeLists.txt"
}

# ==============================================================================
# 3. CONVERT ALL REMAINING PURPLE (DARK, LAVENDER, HEX, RGB, QCOLOR) TO #00ff00
# ==============================================================================
function Convert-AllPurplesToGreen([string]$text) {
    # 3a. 6-digit and 8-digit hex (#RRGGBB or #AARRGGBB) where Blue > Green and Red > Green (catches dark purple, lavender, violet, periwinkle)
    $text = [regex]::Replace($text, '#([0-9a-fA-F]{6})\b', {
        param($m)
        $hex = $m.Groups[1].Value
        $r = [Convert]::ToInt32($hex.Substring(0,2), 16)
        $g = [Convert]::ToInt32($hex.Substring(2,2), 16)
        $b = [Convert]::ToInt32($hex.Substring(4,2), 16)
        # Any purple/lavender/indigo hue: Blue is higher than Green by at least 12, and Red is higher than Green (or near-violet where R >= G - 10 and B >= G + 35)
        if (($b - $g) -ge 12 -and ($r - $g) -ge 5 -and $b -ge 35) {
            return "#00ff00"
        }
        if (($b - $g) -ge 30 -and $r -ge 80 -and $g -ge 80) {
            return "#00ff00"
        }
        return $m.Value
    })

    # 3b. 3-digit shorthand hex (#RGB)
    $text = [regex]::Replace($text, '#([0-9a-fA-F]{3})\b', {
        param($m)
        $h = $m.Groups[1].Value
        $r = [Convert]::ToInt32($h.Substring(0,1) + $h.Substring(0,1), 16)
        $g = [Convert]::ToInt32($h.Substring(1,1) + $h.Substring(1,1), 16)
        $b = [Convert]::ToInt32($h.Substring(2,1) + $h.Substring(2,1), 16)
        if (($b - $g) -ge 15 -and ($r - $g) -ge 8 -and $b -ge 40) {
            return "#00ff00"
        }
        return $m.Value
    })

    # 3c. rgb(...) and rgba(...)
    $text = [regex]::Replace($text, 'rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)(\s*,\s*[\d\.]+)?\s*\)', {
        param($m)
        $r = [int]$m.Groups[1].Value
        $g = [int]$m.Groups[2].Value
        $b = [int]$m.Groups[3].Value
        $a = $m.Groups[4].Value
        if ((($b - $g) -ge 12 -and ($r - $g) -ge 5 -and $b -ge 35) -or (($b - $g) -ge 30 -and $r -ge 80 -and $g -ge 80)) {
            if ($a) { return "rgba(0, 255, 0$a)" }
            return "rgb(0, 255, 0)"
        }
        return $m.Value
    })

    # 3d. QColor(r, g, b [, a])
    $text = [regex]::Replace($text, 'QColor\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)(\s*,\s*\d+)?\s*\)', {
        param($m)
        $r = [int]$m.Groups[1].Value
        $g = [int]$m.Groups[2].Value
        $b = [int]$m.Groups[3].Value
        $a = $m.Groups[4].Value
        if ((($b - $g) -ge 12 -and ($r - $g) -ge 5 -and $b -ge 35) -or (($b - $g) -ge 30 -and $r -ge 80 -and $g -ge 80)) {
            if ($a) { return "QColor(0, 255, 0$a)" }
            return "QColor(0, 255, 0)"
        }
        return $m.Value
    })

    # 3e. C++ 0xRRGGBB or 0xAARRGGBB hex literals inside UI files
    $text = [regex]::Replace($text, '0x([0-9a-fA-F]{6})\b', {
        param($m)
        $hex = $m.Groups[1].Value
        $r = [Convert]::ToInt32($hex.Substring(0,2), 16)
        $g = [Convert]::ToInt32($hex.Substring(2,2), 16)
        $b = [Convert]::ToInt32($hex.Substring(4,2), 16)
        if (($b - $g) -ge 20 -and ($r - $g) -ge 10 -and $b -ge 60) {
            return "0x00ff00"
        }
        return $m.Value
    })

    return $text
}

$uiColorTargets = Get-ChildItem -Path "$root\src\yuzu", "$root\src\qt_common", "$root\dist\qt_themes" -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
    $_.Extension -in @('.qss','.cpp','.h','.ui')
}
foreach ($ut in $uiColorTargets) {
    $c = [IO.File]::ReadAllText($ut.FullName)
    $nc = Convert-AllPurplesToGreen $c
    if ($c -cne $nc) {
        [IO.File]::WriteAllText($ut.FullName, $nc, $utf8NoBom)
        Write-Host "Converted purple shades to #00ff00 in: $($ut.FullName)"
    }
}

# ==============================================================================
# 4. REBUILD TARGET DEKUBI WITH LTCG & PACKAGING (PRESERVING USER CONFIG)
# ==============================================================================
Remove-Item "$root\build\src\yuzu\dekubi_autogen" -Recurse -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path "$root\build\src\yuzu" -Recurse -File -Filter "*qrc_*" -ErrorAction SilentlyContinue | Remove-Item -Force

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed with exit code $LASTEXITCODE" }

$env:CL = "/arch:AVX2 /O2 /Oi /Ot /GL /Gw /Gy /fp:fast /wd4456 /wd4457 /wd4458 /wd4459"
$env:_LINK_ = "/LTCG /OPT:REF /OPT:ICF"

cmake --build build --config Release --target dekubi --parallel 12
if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE" }

# Copy updated binary and any external icon folders into Dekubi_Ally without touching user/config
$distDir = "$root\Dekubi_Ally"
Copy-Item "$root\build\bin\Release\dekubi.exe" "$distDir\Dekubi.exe" -Force

# Also copy physical fritchii icon folders next to Dekubi.exe in case any code uses relative disk paths (QDir::currentPath)
if (Test-Path "$root\src\yuzu\fritchii") {
    $diskIcons = Get-ChildItem -Path "$root\src\yuzu\fritchii" -Recurse -File | Where-Object { $_.Extension -in @('.svg','.png','.ico','.qss') }
    foreach ($di in $diskIcons) {
        $rel = $di.FullName.Substring("$root\src\yuzu\fritchii\".Length)
        $dest1 = Join-Path "$distDir\fritchii" $rel
        $dest2 = Join-Path "$distDir\icons" $di.Name
        $null = New-Item -ItemType Directory -Path (Split-Path $dest1) -Force
        $null = New-Item -ItemType Directory -Path (Split-Path $dest2) -Force
        Copy-Item $di.FullName $dest1 -Force
        Copy-Item $di.FullName $dest2 -Force
    }
}

# Repack Dekubi_Ally_Z1E.zip (keeping your exact user/config/qt-config.ini intact)
$zip = "$root\Dekubi_Ally_Z1E.zip"
Remove-Item $zip -Force -ErrorAction SilentlyContinue
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($distDir, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)
Write-Host "Successfully updated to v1.0.0, embedded all sidebar/topbar icons, converted all purple shades to #00ff00, and packaged: $zip"