// EventAction.cc
#include "EventAction.hh"
#include "DnaLogger.hh"

#include "G4DNAChemistryManager.hh"
#include "G4RunManager.hh"
#include "G4Threading.hh"
#include "G4SystemOfUnits.hh"

#include "G4ITTrackHolder.hh"
#include "G4AnalysisManager.hh"

EventAction::EventAction() = default;
EventAction::~EventAction() = default;

void EventAction::BeginOfEventAction(const G4Event *event)
{
    const G4int eventId = event->GetEventID();
    const G4String eventPrefix = G4String("[EventAction] Event ") + std::to_string(eventId);

    DnaLogger::Print(DnaLogger::Level::Info,
                     eventPrefix + ": Begin of Event");
    if (G4DNAChemistryManager::GetInstanceIfExists() != nullptr)
        G4DNAChemistryManager::Instance()->BeginOfEventAction(event);
}

void EventAction::EndOfEventAction(const G4Event *event)
{
    const G4int eventId = event->GetEventID();
    const G4String eventPrefix = G4String("[EventAction] Event ") + std::to_string(eventId);
    DnaLogger::Print(DnaLogger::Level::Info,
                     eventPrefix + ": End of Event");

    if (G4DNAChemistryManager::GetInstanceIfExists() != nullptr)
        G4DNAChemistryManager::Instance()->EndOfEventAction(event);
}

void EventAction::WriteChemistryOutput(G4int eventID)
{
    G4String fileName = "output_event_" + std::to_string(eventID) + ".txt";
    G4cout << " to file: " << fileName << G4endl;

    // Thread-safe filename (if running in MT mode)
    if (G4RunManager::GetRunManager()->GetRunManagerType() != G4RunManager::sequentialRM)
    {
        G4int threadID = G4Threading::G4GetThreadId();
        fileName = "output_event_" + std::to_string(threadID) + "_" + std::to_string(eventID) + ".txt";
    }

    // Write chemistry data to file
    G4DNAChemistryManager::Instance()->WriteInto(fileName);
}

void EventAction::DumpPreChemical(G4int eventID)
{
    G4ITTrackHolder *trackHolder = G4ITTrackHolder::Instance();

    // Use the Delayed List (tracks waiting for chemistry to start)
    // Note: Implementation depends slightly on G4 version, usually fMainList
    auto mainList = trackHolder->GetMainList();

    // Iterate over all molecules
    // Note: This is pseudo-code logic, exact iterator syntax depends on G4 version (10.7 vs 11.1)
    for (auto it : *mainList)
    {
        G4Track *track = it;
        G4String name = track->GetDefinition()->GetParticleName();
        G4ThreeVector pos = track->GetPosition();
        G4cout << "Event " << eventID << ": Molecule " << name
               << " at position (nm) x=" << pos.x() / nm
               << " y=" << pos.y() / nm
               << " z=" << pos.z() / nm
               << G4endl;

        // Save to One Single File via Analysis Manager
        // auto analysis = G4AnalysisManager::Instance();
        // analysis->FillNtupleIColumn(0, eventID); // Column 0: Event ID
        // analysis->FillNtupleSColumn(1, name);    // Column 1: Molecule
        // analysis->FillNtupleDColumn(2, pos.x());
        // analysis->FillNtupleDColumn(3, pos.y());
        // analysis->FillNtupleDColumn(4, pos.z());
        // analysis->AddNtupleRow();
    }
}
