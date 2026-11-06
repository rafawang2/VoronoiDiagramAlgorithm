param(
    [string]$QtRoot = $env:QT_ROOT,
    [switch]$Deploy,
    [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not $QtRoot) {
    foreach ($candidate in @('D:\src\qt\6.9.3\mingw_64', 'C:\Qt\6.9.3\mingw_64')) {
        if (Test-Path -LiteralPath $candidate) { $QtRoot = $candidate; break }
    }
}
if (-not $QtRoot -or -not (Test-Path -LiteralPath (Join-Path $QtRoot 'bin\qmake.exe'))) {
    throw 'Qt was not found. Pass -QtRoot C:\Qt\<version>\mingw_64, or set QT_ROOT.'
}
$QtRoot = (Resolve-Path -LiteralPath $QtRoot).Path
$qtBase = Split-Path (Split-Path $QtRoot -Parent) -Parent
$toolsRoot = Join-Path $qtBase 'Tools'
$mingw = Get-ChildItem -LiteralPath $toolsRoot -Directory -Filter 'mingw*' |
    Sort-Object Name -Descending | Select-Object -First 1
if (-not $mingw) { throw 'Install the MinGW compiler that matches this Qt kit.' }
$compilerBin = Join-Path $mingw.FullName 'bin'
$cmake = Join-Path $toolsRoot 'CMake_64\bin\cmake.exe'
if (-not (Test-Path -LiteralPath $cmake)) { $cmake = (Get-Command cmake -ErrorAction Stop).Source }
$ctest = Join-Path (Split-Path $cmake -Parent) 'ctest.exe'
$ninja = Join-Path $toolsRoot 'Ninja\ninja.exe'
if (-not (Test-Path -LiteralPath $ninja)) { $ninja = (Get-Command ninja -ErrorAction Stop).Source }
$buildDir = Join-Path $projectRoot 'build\release'
$env:PATH = "$compilerBin;$(Join-Path $QtRoot 'bin');$env:PATH"
& $cmake -S $projectRoot -B $buildDir -G Ninja '-DCMAKE_BUILD_TYPE=Release' `
    "-DCMAKE_PREFIX_PATH=$QtRoot" "-DCMAKE_CXX_COMPILER=$(Join-Path $compilerBin 'g++.exe')" `
    "-DCMAKE_MAKE_PROGRAM=$ninja" '-DBUILD_TESTING=ON'
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& $cmake --build $buildDir --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
if (-not $SkipTests) {
    & $ctest --test-dir $buildDir --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
}
if ($Deploy) {
    $deployDir = Join-Path $projectRoot 'dist\VoronoiStudio'
    New-Item -ItemType Directory -Path $deployDir -Force | Out-Null
    $executable = Join-Path $deployDir 'VoronoiDiagramAlgorithm.exe'
    Copy-Item -LiteralPath (Join-Path $buildDir 'VoronoiDiagramAlgorithm.exe') -Destination $executable -Force
    & (Join-Path $QtRoot 'bin\windeployqt.exe') --release --no-translations --no-opengl-sw --no-system-d3d-compiler $executable
    if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed.' }
    foreach ($runtime in @('libstdc++-6.dll', 'libgcc_s_seh-1.dll', 'libwinpthread-1.dll')) {
        Copy-Item -LiteralPath (Join-Path $compilerBin $runtime) -Destination $deployDir -Force
    }
    Copy-Item -LiteralPath (Join-Path $projectRoot 'examples') -Destination $deployDir -Recurse -Force
    Copy-Item -LiteralPath (Join-Path $projectRoot 'README.md') -Destination $deployDir -Force
    Write-Host "Portable application: $executable"
}
