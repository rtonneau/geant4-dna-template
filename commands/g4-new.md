# `/g4-new <project-name>`

**Purpose:** Create a new Geant4-DNA project from the template repo at `{{TEMPLATE_PATH}}` and verify it builds and runs.

## Procedure

### Step 1 — Gather inputs

- Project name: the argument; it must match `^[A-Za-z][A-Za-z0-9_-]*$`. Ask if missing.
- Parent directory: default to the parent of the current working directory. Confirm with the user; the project is created in `<parent>\<name>` and must not already exist.
- Geant4 paths: if the current project has `.claude/.claude-project.json`, offer to reuse it with `-FromConfig`. Otherwise ask for `geant4SourcePath` (the source tree containing `source/` and `examples/`) and `geant4InstallPath`.

### Step 2 — Run the generator

Use the PowerShell tool with `run_in_background` (a build plus smoke run takes several minutes) and redirect output to a log file:

```powershell
& "{{TEMPLATE_PATH}}\scripts\new-project.ps1" -Name <name> -Dest <parent> -FromConfig <config.json> *> <log>
```

### Step 3 — Report

Read the log. Success means the last line starts with `PASS`. On failure quote the failing step and the relevant lines; do not retry blindly and do not delete the partly created project without asking.

Then tell the user to open the new folder in Claude Code; its `CLAUDE.md` and `.claude/geant4-instructions.md` already describe how to build, run and test it.
