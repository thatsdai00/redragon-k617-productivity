# One-shot launcher: download kit to %TEMP% and open the UI
$ErrorActionPreference = "Stop"
$base = Join-Path $env:TEMP "redragon-k617-productivity"
$zip = Join-Path $env:TEMP "redragon-k617-productivity.zip"
$url = "https://github.com/thatsdai00/redragon-k617-productivity/archive/refs/heads/main.zip"

Write-Host "Downloading Redragon K617 productivity kit..."
Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing

if (Test-Path $base) { Remove-Item $base -Recurse -Force }
New-Item -ItemType Directory -Path $base | Out-Null
Expand-Archive -Path $zip -DestinationPath $base -Force

$cmd = Get-ChildItem -Path $base -Recurse -Filter "Redragon-K617.cmd" | Select-Object -First 1
if (-not $cmd) { throw "Redragon-K617.cmd not found in download." }

Write-Host "Launching $($cmd.FullName)"
Start-Process -FilePath $cmd.FullName -WorkingDirectory $cmd.DirectoryName
