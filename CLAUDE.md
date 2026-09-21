# {{PROJECT_NAME}}

## Purpose and Scope

Geant4-DNA water-radiolysis application (electron beam on one homogeneous water box): DNA physics, a chemistry stage, species scoring and per-reaction counting. Started from `geant4-dna-template`.

Edit this section to state what this project studies and what is out of scope.

## Build and Run

Requirements: Geant4 11.0+ with DNA models, CMake 3.16+, a C++20 compiler, and HDF5 with C++ support.

How to build, run and test (MSVC environment, the `build/` vs `build-ninja/` split, background runs, verification checklist) is in `.claude/geant4-instructions.md`; the values it uses are the `build`, `run` and `test` sections of `.claude/.claude-project.json`. Follow it instead of re-deriving the procedure.

Project-specific: `build/` is RelWithDebInfo (run `sim`), `build-ninja/` is Debug (ctest). `sim.cc` prepends `macro/` to the macro argument itself, so pass the filename only, from the run build dir (the build copies `macro/` there):

```bash
./sim smoke.in     # 25 keV, 2 events: quick end-to-end check
./sim beam.in      # 100 keV, 4 events
```

Each run writes `Species.Txt` (species yields vs. time), `Species_nt_species.csv` (aggregate sumG/sumG2 per species/time) and `Species_nt_species_all.csv` (per event). In MT mode the per-event pre-chemical dumps are `output_event_t<thread>_e<event>.txt`.

Each run also writes `Reactions.Txt` and `Reactions_nt_reactions.csv`: per-time-bin firing counts of each bimolecular reaction (counted live in `TimeStepAction::UserReactionAction` via `ReactionCounter`, merged across threads in `Run::Merge`, aggregated over all events). Acid-base/scavenger reactions are not counted (they never reach that hook). Several `/run/beamOn` in one macro overwrite the `.Txt` files and produce `_bis` CSV names.

Pass `--dir <path>` to redirect every output file into `<path>` (the parent must exist, the leaf is created). `--dir` can appear anywhere after the macro filename, which must be `argv[1]`.

## Key Files

- `sim.cc`: application setup. Serial by default; `--threads N` (N > 0) selects the MT run manager, and `/run/numberOfThreads N` in a macro can still override before `/run/initialize`. Set `useGUI` to `true` only for interactive GUI runs. CLI flags (`--threads`, `--dir`) are registered via `src/ArgParser.cc`.
- `src/OutputDir.cc`: process-wide output directory (`--dir`). `Resolve(filename)` is called at every output-file site.
- `src/DetectorConstruction.cc`: homogeneous water-box geometry; owns the `DnaChemistryWorld` (its boundary sizes the world box).
- `src/PhysicsList.cc`: holds `G4EmDNAPhysics` + `DnaChemistryList` and drives their `ConstructParticle()`/`ConstructProcess()` directly. Change the EM-DNA physics option by editing the constructor. SBS is the chemistry time-step model.
- `src/DnaChemistryList.cc`: chemical stage: molecule set, water dissociation, reaction table, time-step model. Pure-water and O2-derived reactions are built by `src/PureWaterReactions.cc` (portable, no project-specific includes); the pH-driven acid-base buffer network is registered as per-molecule `G4DNAScavengerProcess`.
- `src/DnaChemistryWorld.cc`: diffusion boundary and bulk solvent composition. Macro commands: `/chem/env/pH`, `/chem/env/O2` (PreInit; `O2` currently has no effect on the chemistry).
- `src/ReactionTableDump.cc`: static reaction-table dump (`/chem/reaction/dump <file>`, PreInit) and the shared `FormatReactionLabel`.
- `src/ReactionCounter.cc`, `src/TimeStepAction.cc`: per-reaction firing counts over time.
- `src/ScoreSpecies.cc`: species-yield and G-value scoring.
- `src/PrimaryGeneratorAction.cc`, `src/ActionInitialization.cc`, `src/Run.cc`, `src/RunAction.cc`: source, action wiring, worker-to-master merge, end-of-run output.
- `macro/*.in`: runtime configuration. Species and reactions are defined in code, not by `/chem/species` or `/chem/reaction/add` (`/chem/reaction/UI` resets the shared reaction table and wipes the class-built one).
- `src/DnaLogger.cc`, `src/DnaLoggerMessenger.cc`: logging (`/dnaLogger/verbose Quiet|Error|Warning|Info|Debug|Trace`). Use `DnaLogger` instead of ad hoc console output.

## Testing

Unit tests (`test/*Test.cc`, plain `assert` + CTest) must be built and run from `build-ninja/` (Debug); see `.claude/geant4-instructions.md` section 5 for the `NDEBUG` and Debug-CRT-dialog pitfalls.

When testing `sim.exe`, use a 25 keV electron gun and few events. The chemistry time limit defaults to 1 µs, set by `G4Scheduler::Instance()->SetEndTime(1. * microsecond)` in `src/ActionInitialization.cc::Build()`, which runs on `/run/initialize`. To override it in a macro, issue `/scheduler/endTime <value> <unit>` *after* `/run/initialize`.

## Planning

Before any non-trivial change, enter plan mode and write the plan to `.claude/plans/`. Read any existing relevant plan first. Wait for explicit approval before executing.
