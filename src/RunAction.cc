/// \file RunAction.cc
/// \brief Implementation of the RunAction class

#include "RunAction.hh"

#include "DetectorConstruction.hh"
#include "OutputDir.hh"
#include "PrimaryGeneratorAction.hh"
#include "ReactionCounter.hh"
#include "Run.hh"
#include "ScoreSpecies.hh"

#include "G4DNAChemistryManager.hh"

#include "G4AccumulableManager.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

#include <fstream>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::RunAction() : G4UserRunAction()
{
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::~RunAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Run *RunAction::GenerateRun()
{
    Run *run = new Run();
    return run;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::BeginOfRunAction(const G4Run *run)
{
    // ensure that the chemistry is notified!
    if (G4DNAChemistryManager::GetInstanceIfExists() != nullptr)
        G4DNAChemistryManager::GetInstanceIfExists()->BeginOfRunAction(run);

    if (IsMaster())
        G4cout << "### Run " << run->GetRunID() << " starts." << G4endl;

    // informs the runManager to save random number seed
    G4RunManager::GetRunManager()->SetRandomNumberStore(false);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::EndOfRunAction(const G4Run *run)
{
    // ensure that the chemistry is notified!
    if (G4DNAChemistryManager::GetInstanceIfExists() != nullptr)
        G4DNAChemistryManager::GetInstanceIfExists()->EndOfRunAction(run);

    G4int nofEvents = run->GetNumberOfEvent();
    if (nofEvents == 0)
        return;

    if (IsMaster())
    {
        auto *masterRun = static_cast<const Run *>(run);

        // Write the radiolytic-species yields (merged across worker threads by
        // Run::Merge -> ScoreSpecies::AbsorbResultsFromWorkerScorer).
        auto *scorer = dynamic_cast<ScoreSpecies *>(masterRun->GetPrimitiveScorer());
        if (scorer != nullptr)
        {
            const G4int recorded = scorer->GetNumberOfRecordedEvents();
            scorer->ASCII();          // Species.Txt (human-readable)
            scorer->OutputAndClear(); // Species_nt_species(_all).csv, then clears the scorer
            G4cout << "[RunAction] species yields written (Species.Txt / Species_nt_species*.csv) for "
                   << recorded << " recorded event(s)" << G4endl;
        }

        // Write the per-reaction firing counts binned by time (merged across
        // worker threads by Run::Merge -> ReactionCounter::Merge).
        ReactionCounter *reactionCounter = masterRun->GetReactionCounter();
        if (reactionCounter != nullptr)
        {
            std::ofstream reactionsOut(OutputDir::Resolve("Reactions.Txt"));
            reactionCounter->WriteAscii(reactionsOut);
            reactionsOut.close();

            G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
            analysisManager->SetDefaultFileType("csv");
            reactionCounter->WriteCsv(analysisManager);
            reactionCounter->Clear();

            G4cout << "[RunAction] reaction counts written (Reactions.Txt / Reactions_nt_reactions.csv)"
                   << G4endl;
        }
    }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
