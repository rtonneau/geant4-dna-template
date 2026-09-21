<#
.SYNOPSIS
  Install the /g4-new slash command into ~/.claude/commands, pointing at this
  clone of the template repo.
#>
$ErrorActionPreference = "Stop"
$TemplateRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$src = Join-Path $TemplateRoot "commands\g4-new.md"
$dstDir = Join-Path $env:USERPROFILE ".claude\commands"
New-Item -ItemType Directory -Force $dstDir | Out-Null
$dst = Join-Path $dstDir "g4-new.md"

$text = [IO.File]::ReadAllText($src).Replace("{{TEMPLATE_PATH}}", $TemplateRoot)
[IO.File]::WriteAllText($dst, $text, (New-Object System.Text.UTF8Encoding($false)))
Write-Host "Installed $dst (template: $TemplateRoot)"
