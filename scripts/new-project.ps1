<#
.SYNOPSIS
  Create a new Geant4-DNA project from this template, then prove it works.

.DESCRIPTION
  Copies the template, fills the {{PLACEHOLDERS}}, configures the two build
  directories (run: RelWithDebInfo, tests: Debug), builds, runs ctest and a
  short smoke simulation. Exits non-zero if any step fails.

.EXAMPLE
  .\new-project.ps1 -Name my-radiolysis -Dest C:\DEV\GEANT4\SIM `
      -FromConfig C:\DEV\GEANT4\SIM\dnachem-min\.claude\.claude-project.json
#>
param(
    [Parameter(Mandatory)][string]$Name,
    [string]$Dest = (Get-Location).Path,        # parent directory; project goes in <Dest>\<Name>
    [string]$FromConfig,                        # existing .claude-project.json to take Geant4 paths from
    [string]$G4Source,
    [string]$G4Install,
    [string]$G4Version,
    [string]$KbPath,
    [string]$Toolchain,                         # e.g. <vcpkg>/scripts/buildsystems/vcpkg.cmake ("" = none)
    [switch]$NoBuild,                           # copy + fill placeholders only
    [switch]$NoSmoke,                           # skip the simulation run
    [switch]$NoGit
)

$ErrorActionPreference = "Stop"
$TemplateRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Step($msg) { Write-Host "`n=== $msg ===" -ForegroundColor Cyan }
function Fwd($p) { if ($p) { ($p -replace "\\", "/").TrimEnd("/") } else { $p } }
function CheckExit($what) {
    if ($LASTEXITCODE -ne 0) { throw "$what failed (exit code $LASTEXITCODE)." }
}

# --- 1. Resolve settings ----------------------------------------------------
if ($Name -notmatch '^[A-Za-z][A-Za-z0-9_-]*$') {
    throw "Invalid -Name '$Name': use letters, digits, '-' and '_', starting with a letter."
}
$ProjectDir = Join-Path (Resolve-Path $Dest).Path $Name
if (Test-Path -LiteralPath $ProjectDir) { throw "$ProjectDir already exists." }

if ($FromConfig) {
    $cfg = Get-Content -LiteralPath $FromConfig -Raw | ConvertFrom-Json
    if (-not $G4Source)  { $G4Source  = $cfg.geant4SourcePath }
    if (-not $G4Install) { $G4Install = $cfg.geant4InstallPath }
    if (-not $G4Version) { $G4Version = $cfg.geant4Version }
    if (-not $KbPath)    { $KbPath    = $cfg.kbPath }
    if (-not $PSBoundParameters.ContainsKey("Toolchain")) { $Toolchain = $cfg.build.toolchainFile }
}
if (-not $G4Source -or -not $G4Install) {
    throw "Geant4 paths are required: pass -G4Source and -G4Install, or -FromConfig <.claude-project.json>."
}
if (-not (Test-Path -LiteralPath $G4Install)) { throw "Geant4 install path not found: $G4Install" }
if (-not (Test-Path -LiteralPath $G4Source))  { throw "Geant4 source path not found: $G4Source" }
if (-not $G4Version -and $G4Install -match 'geant4-v(\d+\.\d+\.\d+)') { $G4Version = $Matches[1] }
if (-not $KbPath) { $KbPath = "$(Fwd $ProjectDir)/.kb" }
if (-not $PSBoundParameters.ContainsKey("Toolchain") -and -not $FromConfig -and $env:VCPKG_ROOT) {
    $cand = Join-Path $env:VCPKG_ROOT "scripts\buildsystems\vcpkg.cmake"
    if (Test-Path $cand) { $Toolchain = $cand }
}

$tokens = @{
    "{{PROJECT_NAME}}"    = $Name
    "{{G4_SOURCE}}"       = (Fwd $G4Source)
    "{{G4_INSTALL}}"      = (Fwd $G4Install)
    "{{G4_VERSION}}"      = $G4Version
    "{{KB_PATH}}"         = (Fwd $KbPath)
    "{{VCPKG_TOOLCHAIN}}" = (Fwd $Toolchain)
}

# --- 2. Copy template and fill placeholders ---------------------------------
Step "Creating $ProjectDir"
robocopy $TemplateRoot $ProjectDir /E /NFL /NDL /NJH /NJS /NP `
    /XD ".git" "scripts" "commands" "build" "build-ninja" ".cache" `
    /XF "README.md" "compile_commands.json" "hook-posttooluse.log" | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed (exit code $LASTEXITCODE)." }
$global:LASTEXITCODE = 0

$utf8 = New-Object System.Text.UTF8Encoding($false)
Get-ChildItem -LiteralPath $ProjectDir -Recurse -File |
    Where-Object { $_.Extension -in ".md", ".json", ".ps1" -or $_.Name -eq "CMakeLists.txt" } |
    ForEach-Object {
        $text = [IO.File]::ReadAllText($_.FullName)
        if ($text.Contains("{{")) {
            foreach ($k in $tokens.Keys) { $text = $text.Replace($k, [string]$tokens[$k]) }
            [IO.File]::WriteAllText($_.FullName, $text, $utf8)
        }
    }

$left = Get-ChildItem -LiteralPath $ProjectDir -Recurse -File |
    Select-String -Pattern '\{\{[A-Z_]+\}\}' | Select-Object -First 5
if ($left) { throw "Unfilled placeholders remain: $($left | ForEach-Object { $_.Path + ':' + $_.LineNumber })" }

if (-not $NoGit) {
    Push-Location $ProjectDir
    git init -q -b main; CheckExit "git init"
    git add -A; CheckExit "git add"
    git commit -q -m "Initial commit from geant4-dna-template"; CheckExit "git commit"
    Pop-Location
}
if ($NoBuild) { Write-Host "`nCreated $ProjectDir (no build requested)." -ForegroundColor Green; exit 0 }

# --- 3. MSVC environment ----------------------------------------------------
if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    Step "Loading MSVC x64 environment"
    $vsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $vcvars = Join-Path $vs "VC\Auxiliary\Build\vcvars64.bat"
    if (-not $vs -or -not (Test-Path $vcvars)) { throw "Visual Studio C++ tools (vcvars64.bat) not found." }
    & cmd.exe /d /s /c "call `"$vcvars`" >nul && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') { Set-Item -Path "Env:$($Matches[1])" -Value $Matches[2] }
    }
    if (-not (Get-Command cl -ErrorAction SilentlyContinue)) { throw "cl.exe still unavailable after vcvars64." }
}

# --- 4. Configure, build, test ----------------------------------------------
$geant4Dir = "$(Fwd $G4Install)/lib/cmake/Geant4"
function Configure($dir, $type) {
    $a = @("-S", $ProjectDir, "-B", (Join-Path $ProjectDir $dir), "-G", "Ninja",
           "-DCMAKE_BUILD_TYPE=$type", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DGeant4_DIR=$geant4Dir")
    if ($Toolchain) { $a += "-DCMAKE_TOOLCHAIN_FILE=$(Fwd $Toolchain)" }
    Step "Configure $dir ($type)"
    & cmake @a; CheckExit "cmake configure ($dir)"
}
Configure "build" "RelWithDebInfo"
Configure "build-ninja" "Debug"

Step "Build sim (run build)"
& cmake --build (Join-Path $ProjectDir "build") --config RelWithDebInfo --target sim; CheckExit "build sim"

Step "Build and run unit tests (Debug build)"
$testDir = Join-Path $ProjectDir "build-ninja"
$names = & ctest --test-dir $testDir -N | ForEach-Object { if ($_ -match 'Test\s+#\d+:\s+(\S+)') { $Matches[1] } }
if (-not $names) { throw "ctest -N listed no tests." }
& cmake --build $testDir --target @names; CheckExit "build tests"
& ctest --test-dir $testDir --output-on-failure; CheckExit "ctest"

Copy-Item (Join-Path $testDir "compile_commands.json") (Join-Path $ProjectDir "compile_commands.json") -Force

# --- 5. Smoke run -------------------------------------------------------------
if ($NoSmoke) { Write-Host "`nProject ready (smoke run skipped): $ProjectDir" -ForegroundColor Green; exit 0 }

Step "Smoke run (25 keV, Serial)"
$runDir = Join-Path $ProjectDir "build"
$log = Join-Path $runDir "smoke.log"
Push-Location $runDir
& (Join-Path $runDir "sim.exe") smoke.in --dir smoke_out *> $log
$simExit = $LASTEXITCODE
Pop-Location
$logText = Get-Content -LiteralPath $log -Raw

$problems = @()
if ($simExit -ne 0) { $problems += "exit code $simExit" }
foreach ($m in "[RunAction] species yields written", "[RunAction] reaction counts written", "The simulation took") {
    if (-not $logText.Contains($m)) { $problems += "missing marker '$m'" }
}
if ($logText -match "EEEE|FatalException") { $problems += "Geant4 error in log" }
foreach ($f in "Species.Txt", "Reactions.Txt", "Species_nt_species.csv", "Reactions_nt_reactions.csv") {
    $p = Join-Path $runDir "smoke_out\$f"
    if (-not (Test-Path $p) -or (Get-Item $p).Length -eq 0) { $problems += "missing or empty output $f" }
}
if ($problems) { throw "Smoke run FAILED: $($problems -join '; '). See $log" }

Write-Host "`nPASS: $ProjectDir builds, unit tests pass, smoke run produced all outputs." -ForegroundColor Green
