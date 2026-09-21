#ifndef CHEM4_RunAction_h
#define CHEM4_RunAction_h 1

#include "G4UserRunAction.hh"
#include "globals.hh"

class G4Run;
class DetectorConstruction;

// Forward declariation of other radiobiology classes
class DoseAccumulable;

/// Run action class

class RunAction : public G4UserRunAction
{
public:
    RunAction();
    // TIPs: please avoid constructors with arguments
    // all data can be retrieved from G4RunManager
    // or others: G4SDManager::FindSensitiveDetector
    virtual ~RunAction();

    virtual G4Run *GenerateRun();
    virtual void BeginOfRunAction(const G4Run *);
    virtual void EndOfRunAction(const G4Run *);

private:
    DoseAccumulable *fDoseAccumulable = nullptr;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
