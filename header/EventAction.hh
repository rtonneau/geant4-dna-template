#ifndef EVENTACTION_HH
#define EVENTACTION_HH

#include "G4Event.hh"
#include "G4UserEventAction.hh"
#include "globals.hh"
#include "G4RunManager.hh"

class EventAction : public G4UserEventAction
{
public:
    EventAction();
    ~EventAction() override;

    void BeginOfEventAction(const G4Event *event) override;
    void EndOfEventAction(const G4Event *event) override;

private:
    void WriteChemistryOutput(G4int eventID); // Helper function to write output
    void DumpPreChemical(G4int eventID);
};

#endif