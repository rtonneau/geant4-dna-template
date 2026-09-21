param()

# PostToolUse hook: after a C++ edit, re-run CMake in the compile-commands
# directory and copy compile_commands.json to the repo root (for clangd).
# Every path/setting comes from .claude/.claude-project.json; nothing is
# hardcoded here.

$ErrorActionPreference = "Stop"

$hookLogPath = Join-Path $PSScriptRoot "hook-posttooluse.log"

$hookInput = [Console]::In.ReadToEnd() | ConvertFrom-Json
$filePath = $hookInput.tool_input.file_path
if (-not $filePath -or [IO.Path]::GetExtension($filePath) -notin ".cc", ".cpp", ".cxx") {
    "Skipped at $(Get-Date -Format o): $filePath" | Out-File -FilePath $hookLogPath -Append -Encoding utf8
    exit 0
}

"Triggered at $(Get-Date -Format o): $filePath" | Out-File -FilePath $hookLogPath -Append -Encoding utf8

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$configPath = Join-Path $RepoRoot ".claude\.claude-project.json"
if (-not (Test-Path -LiteralPath $configPath)) {
    throw "Missing $configPath (see ~/.claude/CONFIG_RESOLUTION.md)."
}
$config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json

if (-not $config.geant4InstallPath) { throw "geant4InstallPath is not set in $configPath." }
$g4Install = $config.geant4InstallPath -replace "\\", "/"
$generator = if ($config.build.generator) { $config.build.generator } else { "Ninja" }
$dirName = if ($config.build.compileCommands.dir) { $config.build.compileCommands.dir } else { $config.build.testBuildDir }
$BuildDir = Join-Path $RepoRoot $dirName
$SourceDir = $RepoRoot

if (-not (Test-Path -LiteralPath $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
    "Created build directory at $(Get-Date -Format o): $BuildDir" | Out-File -FilePath $hookLogPath -Append -Encoding utf8
}

if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    $vsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vsWhere)) {
        throw "cl.exe is not available and Visual Studio's vswhere.exe was not found. Run from a Developer PowerShell for Visual Studio."
    }

    $vsInstallPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $vcVars = Join-Path $vsInstallPath "VC\Auxiliary\Build\vcvars64.bat"
    if (-not $vsInstallPath -or -not (Test-Path $vcVars)) {
        throw "A Visual Studio C++ developer environment could not be located. Install the Desktop development with C++ workload."
    }

    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    $devEnvironment = & cmd.exe /d /s /c "call `"$vcVars`" >nul && set" 2>&1
    $devEnvironmentExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorActionPreference
    if ($devEnvironmentExitCode -ne 0) {
        throw "Visual Studio's C++ developer environment failed to initialize (exit code $devEnvironmentExitCode)."
    }

    $devEnvironment |
    Where-Object { $_ -is [string] -and $_ -match "^[^=]+=.*$" } |
    ForEach-Object {
        $name, $value = $_ -split "=", 2
        Set-Item -Path "Env:$name" -Value $value
    }

    if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
        throw "Visual Studio's developer environment was initialized, but cl.exe is still unavailable."
    }
}

$CMakeArgs = @(
    "-S", $SourceDir,
    "-B", $BuildDir,
    "-G", $generator,
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DGeant4_DIR=$g4Install/lib/cmake/Geant4",
    "-DCMAKE_PREFIX_PATH=$g4Install"
)
if ($config.build.toolchainFile) {
    $CMakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$($config.build.toolchainFile)"
}
if ($config.build.extraCMakeArgs) {
    $CMakeArgs += @($config.build.extraCMakeArgs)
}

Write-Host "Configuring with CMake ($generator)..." -ForegroundColor Cyan
& cmake @CMakeArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed (exit code $LASTEXITCODE)."
}

$SourceFile = Join-Path $BuildDir "compile_commands.json"
$DestFile = Join-Path $SourceDir "compile_commands.json"
if (-not (Test-Path $SourceFile)) {
    throw "compile_commands.json was not generated."
}

Copy-Item $SourceFile $DestFile -Force
"Completed at $(Get-Date -Format o): copied compile_commands.json" | Out-File -FilePath $hookLogPath -Append -Encoding utf8
Write-Host "Copied compile_commands.json to the repository root." -ForegroundColor Green
