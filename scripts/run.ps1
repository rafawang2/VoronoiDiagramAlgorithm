param([string]$InputFile)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$executable = Join-Path $projectRoot 'dist\VoronoiStudio\VoronoiDiagramAlgorithm.exe'
if (-not (Test-Path -LiteralPath $executable)) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Deploy
}
if ($InputFile) { & $executable $InputFile } else { & $executable }
