param([ValidateRange(0,50)][int]$Level = 0)
$ErrorActionPreference = 'Stop'
$gameRoot = Split-Path $PSScriptRoot -Parent
$modRoot = $PSScriptRoot
$source = Join-Path $modRoot 'upstream\mods\evilsword'
$target = Join-Path $gameRoot 'mods\evilsword'
New-Item -ItemType Directory -Force $target | Out-Null
Copy-Item (Join-Path $source 'mod.json') $target -Force
New-Item -ItemType Directory -Force (Join-Path $target 'scripts'),(Join-Path $target 'resources\images'),(Join-Path $target 'resources\properties') | Out-Null
Copy-Item (Join-Path $source 'scripts\main.lua') (Join-Path $target 'scripts\main.lua') -Force
Copy-Item (Join-Path $source 'resources\images\EvilSword.png') (Join-Path $target 'resources\images\EvilSword.png') -Force
Copy-Item (Join-Path $source 'resources\images\LightningWhip.png') (Join-Path $target 'resources\images\LightningWhip.png') -Force
Copy-Item (Join-Path $source 'resources\images\RevengeSword.png') (Join-Path $target 'resources\images\RevengeSword.png') -Force
$fxSource = Join-Path $modRoot 'private-dc-fx'
if (Test-Path $fxSource) {
    Get-ChildItem -LiteralPath $fxSource -Filter 'EvilSwordSlash*.png' -File |
        ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $target 'resources\images') -Force }
}
$properties = Join-Path $source 'resources\properties\default.xml'
if (Test-Path $properties) { Copy-Item $properties (Join-Path $target 'resources\properties\default.xml') -Force }
$saveDir = Join-Path $modRoot 'save'
New-Item -ItemType Directory -Force $saveDir | Out-Null
$env:Path = "$(Join-Path $modRoot 'toolchain\ucrt64\bin');$env:Path"
$exe = Join-Path $modRoot 'build\pvz-portable.exe'
if (-not (Test-Path $exe)) { throw 'Run build.ps1 first.' }
if (Get-Process pvz-portable -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe }) {
    throw '诅咒剑草 MOD 已在运行，请先关闭旧窗口。'
}
if ($Level -eq 0) { Remove-Item -LiteralPath (Join-Path $saveDir 'command.txt') -ErrorAction SilentlyContinue }
if ($Level -ge 1) { Set-Content -LiteralPath (Join-Path $saveDir 'command.txt') -Value "level $Level" -Encoding ascii }
& $exe "-resdir=$gameRoot" "-savedir=$saveDir"
exit $LASTEXITCODE
