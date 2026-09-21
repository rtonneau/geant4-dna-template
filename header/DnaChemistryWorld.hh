/// \file DnaChemistryWorld.hh
/// \brief Definition of the DnaChemistryWorld class
///
/// Project chemical domain: a homogeneous water box (diffusion boundary)
/// plus the bulk / background composition of the solvent (water molarity,
/// H3O+/OH- from pH, and — optionally — dissolved O2 acting as a bulk
/// scavenger). Modelled on the Geant4-DNA `UHDR` example `ChemistryWorld`.
///
/// UI (all G4State_PreInit, directory /chem/env/):
///   /chem/env/pH <double>          bulk water pH               (default 7)
///   /chem/env/O2 <double>          dissolved O2, % of a pure-O2
///                                  atmosphere (kH = 0.0013 M)  (default 0)

#ifndef DnaChemistryWorld_h
#define DnaChemistryWorld_h 1

#include "G4SystemOfUnits.hh"
#include "G4VChemistryWorld.hh"
#include "globals.hh"

#include <memory>

class G4GenericMessenger;

class DnaChemistryWorld : public G4VChemistryWorld
{
public:
  DnaChemistryWorld();
  ~DnaChemistryWorld() override;

  void ConstructChemistryBoundary() override;
  void ConstructChemistryComponents() override;

  void SetpH(G4double pH) { fpH = pH; }
  G4double GetpH() const { return fpH; }

  void SetHalfBox(G4double halfBox) { fHalfBox = halfBox; }
  G4double GetHalfBox() const { return fHalfBox; }

  /// Dissolved-O2 fraction, in % of a pure-O2 atmosphere (UHDR convention).
  void SetOxygenPercent(G4double percent) { fO2Percent = percent; }

  /// Bulk O2 molarity (0 when the scavenger is disabled).
  G4double GetOxygenConcentration() const
  {
    return (fO2Percent / 100.) * 0.0013 / (mole * liter);
  }

  /// True when dissolved O2 should be modelled as a bulk scavenger.
  G4bool IsOxygenScavengerEnabled() const { return fO2Percent > 0.; }

private:
  std::unique_ptr<G4GenericMessenger> fMessenger;
  G4double fpH = 7.0;
  G4double fHalfBox = 500. * um;
  G4double fO2Percent = 0.0;
};

#endif // DnaChemistryWorld_h
