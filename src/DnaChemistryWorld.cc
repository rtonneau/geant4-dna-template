/// \file DnaChemistryWorld.cc
/// \brief Implementation of the DnaChemistryWorld class

#include "DnaChemistryWorld.hh"

#include "DnaLogger.hh"

#include "G4ApplicationState.hh"
#include "G4DNABoundingBox.hh"
#include "G4GenericMessenger.hh"
#include "G4MolecularConfiguration.hh"
#include "G4MoleculeTable.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DnaChemistryWorld::DnaChemistryWorld()
{
  fMessenger = std::make_unique<G4GenericMessenger>(this, "/chem/env/",
                                                    "Bulk chemistry environment");
  auto& pHCmd = fMessenger->DeclareProperty("pH", fpH, "Bulk water pH (default 7).");
  pHCmd.SetStates(G4State_PreInit);

  auto& o2Cmd = fMessenger->DeclareProperty(
    "O2", fO2Percent,
    "Dissolved O2 as % of a pure-O2 atmosphere (kH = 0.0013 M); 0 = anoxic.");
  o2Cmd.SetStates(G4State_PreInit);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DnaChemistryWorld::~DnaChemistryWorld() = default;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DnaChemistryWorld::ConstructChemistryBoundary()
{
  // G4DNABoundingBox initializer-list order is {xhi, xlo, yhi, ylo, zhi, zlo}.
  const std::initializer_list<G4double> box{
    fHalfBox, -fHalfBox, fHalfBox, -fHalfBox, fHalfBox, -fHalfBox};
  fpChemistryBoundary = std::make_unique<G4DNABoundingBox>(box);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DnaChemistryWorld::ConstructChemistryComponents()
{
  auto* table = G4MoleculeTable::Instance();

  auto* H2O = table->GetConfiguration("H2O(B)");
  auto* H3OpB = table->GetConfiguration("H3Op(B)");
  auto* OHmB = table->GetConfiguration("OHm(B)");
  if (H2O == nullptr || H3OpB == nullptr || OHmB == nullptr) {
    G4Exception("DnaChemistryWorld::ConstructChemistryComponents", "NoBulkSpecies",
                FatalException,
                "Bulk \"(B)\" configurations are missing - this must run after "
                "DnaChemistryList::ConstructMolecule().");
    return;
  }

  const G4double pKw = 14.0;  // at 25 C
  fpChemicalComponent[H2O] = 55.3 / (mole * liter);
  fpChemicalComponent[H3OpB] = std::pow(10.0, -fpH) / (mole * liter);
  fpChemicalComponent[OHmB] = std::pow(10.0, -(pKw - fpH)) / (mole * liter);

  if (IsOxygenScavengerEnabled()) {
    auto* O2 = table->GetConfiguration("O2");
    if (O2 == nullptr) {
      G4Exception("DnaChemistryWorld::ConstructChemistryComponents", "NoO2", FatalException,
                  "O2 configuration missing.");
      return;
    }
    fpChemicalComponent[O2] = GetOxygenConcentration();
  }

  DnaLogger::Print(DnaLogger::Level::Info,
                   "[DnaChemistryWorld] bulk composition: pH = " + std::to_string(fpH) +
                     ", O2 = " + std::to_string(fO2Percent) + " % (" +
                     std::to_string(fpChemicalComponent.size()) + " components)");
}
