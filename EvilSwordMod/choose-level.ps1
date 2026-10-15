$ErrorActionPreference = 'Stop'
$answer = Read-Host '请输入冒险关卡编号 1-50（按回车进入第 1 关）'
if ([string]::IsNullOrWhiteSpace($answer)) { $answer = '1' }
$level = 0
if (-not [int]::TryParse($answer, [ref]$level) -or $level -lt 1 -or $level -gt 50) {
    Write-Host '关卡编号必须是 1 到 50。'
    exit 1
}
& (Join-Path $PSScriptRoot 'run.ps1') -Level $level
exit $LASTEXITCODE
