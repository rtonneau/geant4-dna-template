# geant4-dna-template

Starting point for a Geant4-DNA water-radiolysis application, plus the Claude Code setup to build, run and test it. Windows / MSVC / Ninja.

Contents: EM-DNA physics, a chemistry stage (pure-water and O2 reactions, acid-base buffer network, SBS time steps), species scoring, per-reaction counting over time, `--threads`/`--dir` CLI, plain-assert unit tests, and `.claude/` files (instructions, config, post-edit hook).

## New project

```powershell
.\scripts\new-project.ps1 -Name my-project -Dest C:\DEV\GEANT4\SIM `
    -FromConfig C:\DEV\GEANT4\SIM\dnachem-min\.claude\.claude-project.json
```

`-FromConfig` takes the Geant4 paths, version, KB path and vcpkg toolchain from an existing project's `.claude/.claude-project.json`. Or pass `-G4Source`, `-G4Install`, `-G4Version`, `-KbPath`, `-Toolchain` yourself. Options: `-NoBuild` (copy only), `-NoSmoke`, `-NoGit`.

The script copies the template, fills the `{{PLACEHOLDERS}}`, makes a git commit, configures `build/` (RelWithDebInfo) and `build-ninja/` (Debug), builds, runs ctest, and runs `macro/smoke.in`. It prints `PASS` only if all of that succeeded and the four output files exist.

## From Claude Code

```powershell
.\scripts\install-command.ps1     # once: installs /g4-new into ~/.claude/commands
```

then `/g4-new my-project`.

## Placeholders

`{{PROJECT_NAME}}`, `{{G4_SOURCE}}`, `{{G4_INSTALL}}`, `{{G4_VERSION}}`, `{{VCPKG_TOOLCHAIN}}`. They appear only in `CMakeLists.txt`, `CLAUDE.md` and `.claude/` files. The template is not buildable by itself; build a generated project.

## Notes

The chemistry is the dnachem-min baseline: acid-base buffer network and O2-derived reactions are always on. Trim `src/DnaChemistryList.cc` and `src/PureWaterReactions.cc` if you want pure water only.
