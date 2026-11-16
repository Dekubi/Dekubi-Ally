$ErrorActionPreference = "Stop"
$root = "C:\Users\User\projects\Fritchii"
Set-Location $root

Stop-Process -Name "Dekubi","dekubi","yuzu","fritchii","lichii" -Force -ErrorAction SilentlyContinue
$utf8NoBom = New-Object System.Text.UTF8Encoding $false

# 1. Ensure src/yuzu/CMakeLists.txt compiles fritchii/fritchii.qrc and yuzu.qrc
$cml = "$root\src\yuzu\CMakeLists.txt"
if (Test-Path $cml) {
    $c = [IO.File]::ReadAllText($cml)
    $nc = $c -replace '(?m)^\s*dekubi\.qrc\s*$', '    fritchii/fritchii.qrc' -replace '(?m)^\s*fritchii/dekubi\.qrc\s*$', '    fritchii/fritchii.qrc'
    if ($nc -notmatch 'fritchii/fritchii\.qrc' -and (Test-Path "$root\src\yuzu\fritchii\fritchii.qrc")) {
        $nc = $nc -replace 'yuzu\.qrc', "yuzu.qrc`n    fritchii/fritchii.qrc"
    }
    if ($c -cne $nc) {
        [IO.File]::WriteAllText($cml, $nc, $utf8NoBom)
        Write-Host "Linked fritchii/fritchii.qrc in: $cml"
    }
}

# 2. Align .qrc file references
$qrcFiles = Get-ChildItem -Path "$root\src\yuzu", "$root\dist" -Recurse -Filter "*.qrc" -File
foreach ($qrc in $qrcFiles) {
    $qDir = $qrc.DirectoryName
    $qc = [IO.File]::ReadAllText($qrc.FullName)
    $nqc = $qc
    $fileMatches = [regex]::Matches($nqc, '<file(?:[^>]*)>([^<]+)</file>')
    foreach ($m in $fileMatches) {
        $relPath = $m.Groups[1].Value
        $fullCand = Join-Path $qDir $relPath
        if (-not (Test-Path $fullCand)) {
            $alts = @($relPath.Replace('dekubi','fritchii'), $relPath.Replace('Dekubi','fritchii'), $relPath.Replace('dekubi','yuzu'), $relPath.Replace('Dekubi','yuzu'))
            foreach ($alt in $alts) {
                if (Test-Path (Join-Path $qDir $alt)) {
                    $nqc = $nqc.Replace(">$relPath<", ">$alt<")
                    Write-Host "Fixed resource path in $($qrc.Name): $relPath -> $alt"
                    break
                }
            }
        }
    }
    if ($qc -cne $nqc) {
        [IO.File]::WriteAllText($qrc.FullName, $nqc, $utf8NoBom)
    }
}

# 3. Register multi-prefix aliases in fritchii.qrc so all sidebar icon URIs resolve
$fritchiiQrc = "$root\src\yuzu\fritchii\fritchii.qrc"
if (Test-Path $fritchiiQrc) {
    $qc = [IO.File]::ReadAllText($fritchiiQrc)
    $m = [regex]::Match($qc, '<qresource[^>]*>([\s\S]*?)</qresource>')
    if ($m.Success) {
        $innerFiles = $m.Groups[1].Value
        $dualQrc = "<RCC>`n  <qresource prefix=`"/fritchii`">$innerFiles</qresource>`n  <qresource prefix=`"/dekubi`">$innerFiles</qresource>`n  <qresource prefix=`"/Dekubi`">$innerFiles</qresource>`n  <qresource prefix=`"/`">$innerFiles</qresource>`n</RCC>"
        [IO.File]::WriteAllText($fritchiiQrc, $dualQrc, $utf8NoBom)
        Write-Host "Registered multi-prefix aliases in fritchii.qrc"
    }
}

$yuzuQrc = "$root\src\yuzu\yuzu.qrc"
if (Test-Path $yuzuQrc) {
    $yqc = [IO.File]::ReadAllText($yuzuQrc)
    $nyqc = $yqc -replace 'prefix="/dekubi"', 'prefix="/yuzu"' -replace 'prefix="/Dekubi"', 'prefix="/yuzu"'
    if ($yqc -cne $nyqc) {
        [IO.File]::WriteAllText($yuzuQrc, $nyqc, $utf8NoBom)
        Write-Host "Restored /yuzu prefix in yuzu.qrc"
    }
}

# 4. Fix broken icon URIs and convert all purple accents to #00ff00 / (0, 255, 0)
function Convert-PurpleToGreen([string]$text) {
    $text = [regex]::Replace($text, '#([0-9a-fA-F]{6})\b', {
        param($m)
        $hex = $m.Groups[1].Value
        $r = [Convert]::ToInt32($hex.Substring(0,2), 16)
        $g = [Convert]::ToInt32($hex.Substring(2,2), 16)
        $b = [Convert]::ToInt32($hex.Substring(4,2), 16)
        if ($b -ge 90 -and ($b - $g) -ge 25 -and ($r - $g) -ge 15 -and $r -ge 60) { return "#00ff00" }
        return $m.Value
    })
    $text = [regex]::Replace($text, 'rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)(\s*,\s*[\d\.]+)?\s*\)', {
        param($m)
        $r = [int]$m.Groups[1].Value
        $g = [int]$m.Groups[2].Value
        $b = [int]$m.Groups[3].Value
        $a = $m.Groups[4].Value
        if ($b -ge 90 -and ($b - $g) -ge 25 -and ($r - $g) -ge 15 -and $r -ge 60) {
            if ($a) { return "rgba(0, 255, 0$a)" }
            return "rgb(0, 255, 0)"
        }
        return $m.Value
    })
    $text = [regex]::Replace($text, 'QColor\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)(\s*,\s*\d+)?\s*\)', {
        param($m)
        $r = [int]$m.Groups[1].Value
        $g = [int]$m.Groups[2].Value
        $b = [int]$m.Groups[3].Value
        $a = $m.Groups[4].Value
        if ($b -ge 90 -and ($b - $g) -ge 25 -and ($r - $g) -ge 15 -and $r -ge 60) {
            if ($a) { return "QColor(0, 255, 0$a)" }
            return "QColor(0, 255, 0)"
        }
        return $m.Value
    })
    return $text
}

$allFiles = Get-ChildItem -Path "$root\src\yuzu", "$root\dist\qt_themes" -Recurse -File
foreach ($f in $allFiles) {
    if ($f.Extension -in @('.qss','.cpp','.h','.ui','.qrc')) {
        $c = [IO.File]::ReadAllText($f.FullName)
        $nc = $c -replace ':/dekubi/icons/', ':/icons/' -replace ':/Dekubi/icons/', ':/icons/'
        $nc = Convert-PurpleToGreen $nc
        if ($c -cne $nc) {
            [IO.File]::WriteAllText($f.FullName, $nc, $utf8NoBom)
            Write-Host "Updated icons/colors in: $($f.FullName)"
        }
    }
}

# 5. Clean cached autogen/RCC artifacts and rebuild
Remove-Item "$root\build\src\yuzu\dekubi_autogen" -Recurse -Force -ErrorAction SilentlyContinue
$qrcObjs = Get-ChildItem -Path "$root\build\src\yuzu" -Recurse -File -Filter "*qrc_*" -ErrorAction SilentlyContinue
foreach ($qo in $qrcObjs) { Remove-Item $qo.FullName -Force -ErrorAction SilentlyContinue }
$relObjs = Get-ChildItem -Path "$root\build\src\yuzu\dekubi.dir\Release" -File -Filter "*.obj" -ErrorAction SilentlyContinue
foreach ($ro in $relObjs) { Remove-Item $ro.FullName -Force -ErrorAction SilentlyContinue }

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
$env:CL = "/arch:AVX2 /O2 /Oi /Ot /wd4456 /wd4457 /wd4458 /wd4459"
cmake --build build --config Release --target dekubi --parallel 12
if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE" }

# 6. Package and bundle Qt6Svg
$distDir = "$root\Dekubi_Ally"
Remove-Item $distDir -Recurse -Force -ErrorAction SilentlyContinue
$null = New-Item -ItemType Directory -Path $distDir -Force
Copy-Item "$root\build\bin\Release\*" $distDir -Recurse -Force -Exclude "*.pdb","*.exp","*.lib","*.a","*tests.exe","*.keys"

$keyDirs = Get-ChildItem $distDir -Recurse -Directory -Filter "keys" -ErrorAction SilentlyContinue
foreach ($kd in $keyDirs) { Remove-Item $kd.FullName -Recurse -Force -ErrorAction SilentlyContinue }

$qtRoot = "$root\.cache\cpm\qt6\6.11.1-windows-amd64"
if (Test-Path "$qtRoot\bin\Qt6Svg.dll") {
    Copy-Item "$qtRoot\bin\Qt6Svg.dll" "$distDir\Qt6Svg.dll" -Force
    Copy-Item "$qtRoot\bin\Qt6Svg.dll" "$root\build\bin\Release\Qt6Svg.dll" -Force
}
if (Test-Path "$qtRoot\plugins\iconengines\qsvgicon.dll") {
    $null = New-Item -ItemType Directory -Path "$distDir\plugins\iconengines" -Force
    $null = New-Item -ItemType Directory -Path "$root\build\bin\Release\plugins\iconengines" -Force
    Copy-Item "$qtRoot\plugins\iconengines\qsvgicon.dll" "$distDir\plugins\iconengines\qsvgicon.dll" -Force
    Copy-Item "$qtRoot\plugins\iconengines\qsvgicon.dll" "$root\build\bin\Release\plugins\iconengines\qsvgicon.dll" -Force
}
if (Test-Path "$qtRoot\plugins\imageformats\qsvg.dll") {
    $null = New-Item -ItemType Directory -Path "$distDir\plugins\imageformats" -Force
    $null = New-Item -ItemType Directory -Path "$root\build\bin\Release\plugins\imageformats" -Force
    Copy-Item "$qtRoot\plugins\imageformats\qsvg.dll" "$distDir\plugins\imageformats\qsvg.dll" -Force
    Copy-Item "$qtRoot\plugins\imageformats\qsvg.dll" "$root\build\bin\Release\plugins\imageformats\qsvg.dll" -Force
}

if (Test-Path "$distDir\yuzu.exe") { Rename-Item "$distDir\yuzu.exe" "Dekubi.exe" -Force }
if (Test-Path "$distDir\dekubi.exe") { Rename-Item "$distDir\dekubi.exe" "Dekubi.exe" -Force }
Copy-Item "$root\dekubi.ico" "$distDir\dekubi.ico" -Force

$zip = "$root\Dekubi_Ally_Z1E.zip"
Remove-Item $zip -Force -ErrorAction SilentlyContinue
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($distDir, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)
Write-Host "Successfully restored UI icons, applied #00ff00 green theme, built, and packaged: $zip"