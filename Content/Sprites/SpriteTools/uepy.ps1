# Run a python script headlessly inside the UE editor context (portable paths).
param(
    [Parameter(Mandatory=$true)][string]$Script,
    [string]$Engine = $(if ($env:UE_ROOT) { $env:UE_ROOT } else { 'D:\UE_5.8' })
)
$proj = Join-Path (Split-Path $PSScriptRoot -Parent) 'BeastForgeKingdom.uproject'
& "$Engine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $proj -run=pythonscript -script="$Script" -unattended -nosplash -stdout -NoLogTimes
exit $LASTEXITCODE
