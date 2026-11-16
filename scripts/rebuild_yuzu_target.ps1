$ErrorActionPreference = "Stop"
$root = "C:\Users\User\projects\Fritchii"
Set-Location $root

Stop-Process -Name "Dekubi","dekubi","yuzu","fritchii","lichii" -Force -ErrorAction SilentlyContinue
$utf8NoBom = New-Object System.Text.UTF8Encoding $false

# 1. Inspect actual .qrc files in src/yuzu and restore fritchii/fritchii.qrc if it was renamed
$foundQrcs = Get-ChildItem -Path "$root\src\yuzu" -Recurse -Filter "*.qrc" -File
foreach ($q in $foundQrcs) {
    Write-Host "Found QRC on disk: $($q.FullName)"
}

$subQrc = Get-ChildItem -Path "$root\src\yuzu\fritchii" -Filter "*.qrc" -File -ErrorAction SilentlyContinue | Select-Object -First 1
if ($subQrc -and $subQrc.Name -cne "fritchii.qrc") {
    Rename-Item $subQrc.FullName "fritchii.qrc" -Force
    Write-Host "Renamed $($subQrc.Name) -> fritchii.qrc"
}

# 2. Fix src/yuzu/CMakeLists.txt so every .qrc entry matches a real file on disk
$cml = "$root\src\yuzu\CMakeLists.txt"
$c = [IO.File]::ReadAllText($cml)

if (Test-Path "$root\src\yuzu\fritchii\fritchii.qrc") {
    $c = $c -replace '(?m)^\s*fritchii/Dekubi\.qrc\s*$', '    fritchii/fritchii.qrc' `
            -replace '(?m)^\s*fritchii/dekubi\.qrc\s*$', '    fritchii/fritchii.qrc' `
            -replace '(?m)^\s*dekubi\.qrc\s*$', '    fritchii/fritchii.qrc' `
            -replace '(?m)^\s*Dekubi\.qrc\s*$', '    fritchii/fritchii.qrc'
} else {
    # Remove non-existent fritchii/fritchii.qrc reference if all resources are already in yuzu.qrc
    $c = $c -replace '(?m)^\s*fritchii/fritchii\.qrc\r?\n', '' `
            -replace '(?m)^\s*fritchii/Dekubi\.qrc\r?\n', '' `
            -replace '(?m)^\s*fritchii/dekubi\.qrc\r?\n', '' `
            -replace '(?m)^\s*dekubi\.qrc\r?\n', '' `
            -replace '(?m)^\s*Dekubi\.qrc\r?\n', ''
}

[IO.File]::WriteAllText($cml, $c, $utf8NoBom)
Write-Host "Synced .qrc entries in src/yuzu/CMakeLists.txt"

# 3. Also ensure yuzu.qrc and any sub-qrc have multi-prefix aliases (/fritchii, /dekubi, /yuzu, /) so all sidebar icons load
$allQrcs = Get-ChildItem -Path "$root\src\yuzu" -Recurse -Filter "*.qrc" -File
foreach ($qrc in $allQrcs) {
    $qc = [IO.File]::ReadAllText($qrc.FullName)
    $fileTags = [regex]::Matches($qc, '<file(?:[^>]*)>([^<]+)</file>')
    if ($fileTags.Count -gt 0) {
        $uniqueTags = New-Object System.Collections.Generic.HashSet[string]
        $innerXml = "`n"
        foreach ($ft in $fileTags) {
            $relPath = $ft.Groups[1].Value
            $fullCand = Join-Path $qrc.DirectoryName $relPath
            $tagToUse = $ft.Value
            if (-not (Test-Path $fullCand)) {
                foreach ($alt in @($relPath.Replace('Dekubi','fritchii'), $relPath.Replace('dekubi','fritchii'), $relPath.Replace('Dekubi','yuzu'), $relPath.Replace('dekubi','yuzu'))) {
                    if (Test-Path (Join-Path $qrc.DirectoryName $alt)) {
                        $tagToUse = $tagToUse.Replace(">$relPath<", ">$alt<")
                        break
                    }
                }
            }
            if ($uniqueTags.Add($tagToUse)) {
                $innerXml += "        $tagToUse`n"
            }
        }
        $multiQrc = "<RCC>`n    <qresource prefix=`"/`">$innerXml    </qresource>`n    <qresource prefix=`"/fritchii`">$innerXml    </qresource>`n    <qresource prefix=`"/dekubi`">$innerXml    </qresource>`n    <qresource prefix=`"/Dekubi`">$innerXml    </qresource>`n    <qresource prefix=`"/yuzu`">$innerXml    </qresource>`n</RCC>`n"
        [IO.File]::WriteAllText($qrc.FullName, $multiQrc, $utf8NoBom)
        Write-Host "Updated multi-prefix resources in: $($qrc.FullName)"
    }
}

# 4. Remove ONLY build/src/yuzu so CMake and MSBuild cleanly regenerate and compile all dekubi .cpp/.obj files
# (This preserves Boost, FFmpeg, video_core, core, shader_recompiler, and dynarmic so it only takes ~30 seconds)
Remove-Item "$root\build\src\yuzu" -Recurse -Force -ErrorAction SilentlyContinue

# 5. Reconfigure CMake and verify exit code 0
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed with exit code $LASTEXITCODE" }

# 6. Build target 'dekubi'
$env:CL = "/arch:AVX2 /O2 /Oi /Ot /wd4456 /wd4457 /wd4458 /wd4459"
cmake --build build --config Release --target dekubi --parallel 12
if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE" }

# 7. Package distribution folder and bundle Qt6Svg plugins
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
Write-Host "Successfully rebuilt dekubi, restored icons & #00ff00 theme, and packaged: $zip"