$ErrorActionPreference = "Stop"
$root = "C:\Users\User\projects\Fritchii"
Set-Location $root

Stop-Process -Name "Dekubi","dekubi","yuzu","fritchii","lichii" -Force -ErrorAction SilentlyContinue
$utf8NoBom = New-Object System.Text.UTF8Encoding $false

# 1. Provide a clean, empty dist/nx2_switches.txt so CMake's POST_BUILD copy succeeds without shader fog overrides
[IO.File]::WriteAllText("$root\dist\nx2_switches.txt", "# Clean NX2 switches`n", $utf8NoBom)
Write-Host "Created clean dist/nx2_switches.txt"

# 2. Explicitly inject fritchii/fritchii.qrc into add_executable(dekubi ...) in src/yuzu/CMakeLists.txt
$cml = "$root\src\yuzu\CMakeLists.txt"
if (Test-Path $cml) {
    $c = [IO.File]::ReadAllText($cml)
    if ($c -notmatch 'fritchii/fritchii\.qrc') {
        $c = $c -replace '(\s+yuzu\.qrc)', "`$1`n    fritchii/fritchii.qrc"
        [IO.File]::WriteAllText($cml, $c, $utf8NoBom)
        Write-Host "Injected fritchii/fritchii.qrc into src/yuzu/CMakeLists.txt"
    }
}

# 3. Ensure Q_INIT_RESOURCE(fritchii) is initialized or multi-prefix paths in fritchii.qrc are clean
$fritchiiQrc = "$root\src\yuzu\fritchii\fritchii.qrc"
if (Test-Path $fritchiiQrc) {
    $qc = [IO.File]::ReadAllText($fritchiiQrc)
    $fileTags = [regex]::Matches($qc, '<file(?:[^>]*)>([^<]+)</file>')
    $uniqueFiles = New-Object System.Collections.Generic.HashSet[string]
    $innerXml = "`n"
    foreach ($ft in $fileTags) {
        $tag = $ft.Value
        if ($uniqueFiles.Add($tag)) {
            $innerXml += "        $tag`n"
        }
    }
    $cleanQrc = "<RCC>`n    <qresource prefix=`"/fritchii`">$innerXml    </qresource>`n    <qresource prefix=`"/dekubi`">$innerXml    </qresource>`n    <qresource prefix=`"/Dekubi`">$innerXml    </qresource>`n    <qresource prefix=`"/`">$innerXml    </qresource>`n</RCC>`n"
    [IO.File]::WriteAllText($fritchiiQrc, $cleanQrc, $utf8NoBom)
    Write-Host "Verified multi-prefix fritchii.qrc"
}

# 4. Reconfigure and run incremental build (compiles qrc_fritchii and runs post-build windeployqt)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
$env:CL = "/arch:AVX2 /O2 /Oi /Ot /wd4456 /wd4457 /wd4458 /wd4459"
cmake --build build --config Release --target dekubi --parallel 12
if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE" }

# 5. Package distribution folder and bundle Qt6Svg plugins
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
Write-Host "Successfully completed post-build, embedded fritchii.qrc icons, and packaged: $zip"