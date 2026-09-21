#include "Run.hh"

#include "RunAction.hh"
#include "ScoreSpecies.hh"
#include "TimeStepAction.hh"

#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4RunManager.hh"
#include "G4Scheduler.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4THitsMap.hh"
#include "G4VSensitiveDetector.hh"
#include "G4MultiFunctionalDetector.hh"

#include <map>

Run::Run() : G4Run(), fSumEne(0), fScorerRun(0), fReactionCounter(nullptr)
{
    G4MultiFunctionalDetector *mfdet = dynamic_cast<G4MultiFunctionalDetector *>(
        G4SDManager::GetSDMpointer()->FindSensitiveDetector("mfDetector"));
    G4int CollectionIDspecies = G4SDManager::GetSDMpointer()->GetCollectionID("mfDetector/Species");

    fScorerRun = mfdet->GetPrimitive(CollectionIDspecies);

    auto *timeStepAction =
        dynamic_cast<TimeStepAction *>(G4Scheduler::Instance()->GetUserTimeStepAction());
    fReactionCounter =
        (timeStepAction != nullptr) ? &timeStepAction->GetReactionCounter() : &fOwnedReactionCounter;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

Run::~Run() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::RecordEvent(const G4Event *event)
{
    if (event->IsAborted())
        return;

    G4int CollectionID = G4SDManager::GetSDMpointer()->GetCollectionID("mfDetector/Species");
    // G4int evtNb = event->GetEventID();

    //  G4cout << G4endl << "---> end of event: " << evtNb << G4endl;
    // Hits collections
    //
    G4HCofThisEvent *HCE = event->GetHCofThisEvent();
    if (!HCE)
        return;
    G4THitsMap<G4double> *evtMap = static_cast<G4THitsMap<G4double> *>(HCE->GetHC(CollectionID));

    std::map<G4int, G4double *>::iterator itr;
    for (itr = evtMap->GetMap()->begin(); itr != evtMap->GetMap()->end(); itr++)
    {
        G4double edep = *(itr->second);
        fSumEne += edep;
    }

    G4Run::RecordEvent(event);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::Merge(const G4Run *aRun)
{
    if (aRun == this)
    {
        return;
    }

    const Run *localRun = static_cast<const Run *>(aRun);
    fSumEne += localRun->fSumEne;

    ScoreSpecies *masterScorer = dynamic_cast<ScoreSpecies *>(this->fScorerRun);

    ScoreSpecies *localScorer = dynamic_cast<ScoreSpecies *>(localRun->fScorerRun);

    masterScorer->AbsorbResultsFromWorkerScorer(localScorer);

    // localRun->fReactionCounter points at the worker's live TimeStepAction
    // counter, which persists across /run/beamOn calls -- clear it after
    // merging so a subsequent run doesn't double-count.
    fReactionCounter->Merge(*localRun->fReactionCounter);
    localRun->fReactionCounter->Clear();

    G4Run::Merge(aRun);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
