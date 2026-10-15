$ErrorActionPreference = 'Stop'
$prefix = Join-Path $PSScriptRoot 'toolchain\ucrt64'
$env:Path = "$prefix\bin;$env:Path"
& "$prefix\bin\cmake.exe" -S "$PSScriptRoot\upstream" -B "$PSScriptRoot\build" -G Ninja "-DCMAKE_PREFIX_PATH=$prefix" -DCMAKE_BUILD_TYPE=Release -DPVZ_ENABLE_LUA=ON -DPVZ_DEBUG=OFF -DLIMBO_PAGE=ON -DCONSOLE=OFF -DUSE_GME=OFF -DUSE_MIDI=OFF -DUSE_FFMPEG=OFF
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& "$prefix\bin\cmake.exe" --build "$PSScriptRoot\build" -j 6
exit $LASTEXITCODE
