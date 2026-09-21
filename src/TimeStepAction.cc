#include "TimeStepAction.hh"

#include "DnaLogger.hh"
#include "OutputDir.hh"
#include "ReactionTableDump.hh"

#include "G4DNAChemistryManager.hh"
#include "G4DNAMolecularReactionTable.hh"
#include "G4Scheduler.hh"
#include "G4ITTrackHolder.hh"
#include "G4AnalysisManager.hh"
#include "G4Molecule.hh"
#include "G4MolecularConfiguration.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4RunManager.hh"
#include "G4Threading.hh"

#include <vector>

// #include "G4ITScheduler.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

TimeStepAction::TimeStepAction() : G4UserTimeStepAction()
{
  /**
   * Inform G4ITTimeStepper of the selected minimum time steps
   * eg : from 1 picosecond to 10 picosecond, the minimum time
   * step that the TimeStepper can returned is 0.1 picosecond.
   *
   * Case 1) If the rection model calculates a minimum reaction time
   * bigger than the user defined time step, the reaction model wins
   *
   * Case 2) If an interaction process with the continuous medium
   * calculates a time step less than the selected minimum time step,
   * the interaction process wins
   */

  AddTimeStep(1 * picosecond, 0.1 * picosecond);
  AddTimeStep(10 * picosecond, 1 * picosecond);
  AddTimeStep(100 * picosecond, 10 * picosecond);
  AddTimeStep(1000 * picosecond, 100 * picosecond);
  AddTimeStep(10000 * picosecond, 1000 * picosecond);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

TimeStepAction::~TimeStepAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

TimeStepAction::TimeStepAction(const TimeStepAction &other) : G4UserTimeStepAction(other) {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
TimeStepAction &TimeStepAction::operator=(const TimeStepAction &rhs)
{
  if (this == &rhs)
    return *this; // handle self assignment
  // assignment operator
  return *this;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TimeStepAction::StartProcessing()
{
  // You want to know why the simulation stopped ?
  // G4ITScheduler::Instance()->WhyDoYouStop();
  // At the end of the simulation, information will be printed
  // It is better to place this command before the simulation starts
  G4cout << "TimeStepAction::StartProcessing() called" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TimeStepAction::UserPreTimeStepAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TimeStepAction::UserPostTimeStepAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Counts each bimolecular reaction that fires, binned by time, so a
// per-reaction rate-over-time output can be written at end of run (see
// Run::Merge / RunAction::EndOfRunAction).
void TimeStepAction::UserReactionAction(const G4Track &a, const G4Track &b,
                                        const std::vector<G4Track *> * /*products*/)
{
  const auto *molA = GetMolecule(a)->GetMolecularConfiguration();
  const auto *molB = GetMolecule(b)->GetMolecularConfiguration();

  const auto *reactionData =
      G4DNAMolecularReactionTable::GetReactionTable()->GetReactionData(molA, molB);
  if (reactionData == nullptr) {
    DnaLogger::Print(DnaLogger::Level::Warning,
                     "[TimeStepAction] reaction fired with no matching reaction-table entry: "
                     + molA->GetName() + " + " + molB->GetName());
    return;
  }

  std::vector<G4String> productNames;
  const G4int nbProducts = reactionData->GetNbProducts();
  productNames.reserve(nbProducts);
  for (G4int i = 0; i < nbProducts; ++i) {
    productNames.push_back(reactionData->GetProduct(i)->GetName());
  }

  G4String label = ReactionTableDump::FormatReactionLabel(
      reactionData->GetReactant1()->GetName(), reactionData->GetReactant2()->GetName(),
      productNames);

  fReactionCounter.Record(label, G4Scheduler::Instance()->GetGlobalTime());

  // Example to display reactions with product
  // S. Incerti, H. Tran
  // 2019/01/24

  /*
  if (products)
  {
    G4cout << G4endl;
    G4int nbProducts = products->size();
    for (int i = 0 ; i < nbProducts ; i ++)
    {
      G4cout << "-> A = "
        << GetMolecule(&a)->GetName() << " (TrackID=" << a.GetTrackID() << ")"
        << " reacts with B = "
        << GetMolecule(&b)->GetName() << " (TrackID=" << b.GetTrackID() << ")"
        << " creating product " << i+1 << " ="
        << GetMolecule((*products)[i])->GetName()
        << G4endl ;

      G4cout
      <<" A position: x(nm)="<<a.GetPosition().getX()/nm
      <<" y(nm)="<<a.GetPosition().getY()/nm
      <<" z(nm)="<<a.GetPosition().getZ()/nm
      <<G4endl;

      G4cout
      <<" B position: x(nm)="<<b.GetPosition().getX()/nm
      <<" y(nm)="<<b.GetPosition().getY()/nm
      <<" z(nm)="<<b.GetPosition().getZ()/nm
      <<G4endl;

      G4cout
      <<" Product " << i+1 << "position: x(nm)="<<(*products)[i]->GetPosition().getX()/nm
      <<" y(nm)="<<a.GetPosition().getY()/nm
      <<" z(nm)="<<a.GetPosition().getZ()/nm
      <<G4endl;
    }
  }

  else

  {
     G4cout << G4endl;
     G4cout << "-> A = "
        << GetMolecule(&a)->GetName() << " (TrackID=" << a.GetTrackID() << ")"
        << " reacts with B = "
        << GetMolecule(&b)->GetName() << " (TrackID=" << b.GetTrackID() << ")"
        << G4endl ;

      G4cout
      <<" A position: x(nm)="<<a.GetPosition().getX()/nm
      <<" y(nm)="<<a.GetPosition().getY()/nm
      <<" z(nm)="<<a.GetPosition().getZ()/nm
      <<G4endl;

      G4cout
      <<" B position: x(nm)="<<b.GetPosition().getX()/nm
      <<" y(nm)="<<b.GetPosition().getY()/nm
      <<" z(nm)="<<b.GetPosition().getZ()/nm
      <<G4endl;

  }
  */
}

void TimeStepAction::EndProcessing()
{
  G4cout << "TimeStepAction::EndProcessing() called" << G4endl;
  // You want to know why the simulation stopped ?
  G4cout << "Current time step: " << G4Scheduler::Instance()->GetTimeStep() / picosecond << " ps" << G4endl;
  G4cout << "Current global time: " << G4Scheduler::Instance()->GetGlobalTime() / picosecond << " ps" << G4endl;
  const G4Event *currentEvent = G4RunManager::GetRunManager()->GetCurrentEvent();
  G4cout << "[TimeStepAction End] G4Event pointer: " << currentEvent << G4endl;
  this->DumpPreChemical(currentEvent->GetEventID()); // Dump the state at the beginning of the simulation (eventID=0)
}

// void TimeStepAction::WriteChemistryOutput(G4int eventID)
// {
//   G4String fileName = "output_event_" + std::to_string(eventID) + ".txt";
//   G4cout << "Writing chemistry output for event " << eventID << " to file: " << fileName << G4endl;

//   // Thread-safe filename (if running in MT mode)
//   if (G4RunManager::GetRunManager()->GetRunManagerType() != G4RunManager::sequentialRM)
//   {
//     G4int threadID = G4Threading::G4GetThreadId();
//     fileName = "output_event_" + std::to_string(threadID) + "_" + std::to_string(eventID) + ".txt";
//   }

//   // Write chemistry data to file
//   G4DNAChemistryManager::Instance()->WriteInto(fileName);
// }

void TimeStepAction::DumpPreChemical(G4int eventID)
{
  // Thread-safe filename in MT mode (each worker writes its own file).
  G4String fileName = "output_event_" + std::to_string(eventID) + ".txt";
  if (G4Threading::IsMultithreadedApplication())
  {
    fileName = "output_event_t" + std::to_string(G4Threading::G4GetThreadId()) +
               "_e" + std::to_string(eventID) + ".txt";
  }
  G4DNAChemistryManager::Instance()->WriteInto(OutputDir::Resolve(fileName));

  G4ITTrackHolder *trackHolder = G4ITTrackHolder::Instance();

  // Use the Delayed List (tracks waiting for chemistry to start)
  // Note: Implementation depends slightly on G4 version, usually fMainList
  auto mainList = trackHolder->GetMainList();
  G4cout << "Size of pre chemical main list: " << mainList->size() << G4endl;

  // Iterate over all molecules
  // Note: This is pseudo-code logic, exact iterator syntax depends on G4 version (10.7 vs 11.1)
  // for (auto it : *mainList)
  // {
  //   G4Track *track = it;
  //   G4String name = track->GetDefinition()->GetParticleName();
  //   G4ThreeVector pos = track->GetPosition();
  //   G4cout << "Event " << eventID << ": Molecule " << name
  //          << " at position (nm) x=" << pos.x() / nm
  //          << " y=" << pos.y() / nm
  //          << " z=" << pos.z() / nm
  //          << G4endl;

  //   // Save to One Single File via Analysis Manager
  //   // auto analysis = G4AnalysisManager::Instance();
  //   // analysis->FillNtupleIColumn(0, eventID); // Column 0: Event ID
  //   // analysis->FillNtupleSColumn(1, name);    // Column 1: Molecule
  //   // analysis->FillNtupleDColumn(2, pos.x());
  //   // analysis->FillNtupleDColumn(3, pos.y());
  //   // analysis->FillNtupleDColumn(4, pos.z());
  //   // analysis->AddNtupleRow();
  // }
}