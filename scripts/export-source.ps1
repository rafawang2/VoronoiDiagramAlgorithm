param(
    [string]$StudentId = 'TODO',
    [string]$ChineseName = 'TODO',
    [string]$EnglishName = 'TODO'
)
$ErrorActionPreference = 'Stop'
if ($StudentId -notmatch '^[A-Za-z0-9_-]+$') { throw 'Student ID must be a safe filename.' }
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$outputDir = Join-Path $projectRoot 'dist\source'
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
$files = @('voronoi.h', 'voronoi.cpp', 'dataio.h', 'dataio.cpp', 'appstyle.h',
    'voronoicanvas.h', 'voronoicanvas.cpp', 'mainwindow.h', 'mainwindow.cpp', 'main.cpp',
    'tests\engine_tests.cpp', 'tests\ui_tests.cpp', 'CMakeLists.txt')
$content = [System.Text.StringBuilder]::new()
[void]$content.AppendLine('// $LAN=C$')
[void]$content.AppendLine("// Copyright (c) 2026 $StudentId / $ChineseName / $EnglishName")
[void]$content.AppendLine('// Language: C++17. Build environment: Qt Widgets, CMake, MinGW or MSVC.')
[void]$content.AppendLine('// Combined source for review; compile the original files using CMake.')
foreach ($file in $files) {
    [void]$content.AppendLine("`n// ==================== BEGIN $file ====================")
    [void]$content.AppendLine([System.IO.File]::ReadAllText((Join-Path $projectRoot $file)))
    [void]$content.AppendLine("// ==================== END $file ====================")
}
$outputPath = Join-Path $outputDir "$StudentId.txt"
[System.IO.File]::WriteAllText($outputPath, $content.ToString(), [System.Text.UTF8Encoding]::new($false))
Write-Host "Combined source: $outputPath"
