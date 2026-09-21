/// \file PureWaterReactions.cc
/// \brief Implementation of the PureWaterReactions reaction-table builder

#include "PureWaterReactions.hh"

#include "G4DNAMolecularReactionTable.hh"
#include "G4MolecularConfiguration.hh"
#include "G4MoleculeTable.hh"
#include "G4SystemOfUnits.hh"

#include <initializer_list>

namespace
{
// Configuration tags containing the UTF-8 degree sign (0xC2 0xB0), spelled
// from explicit bytes so they match the names stored by
// G4ChemDissociationChannels_option1 whatever this file's source encoding is.
// Duplicated locally (rather than shared with DnaChemistryList.cc) so this
// file has zero project-specific dependencies.
const G4String kOH = G4String("\xC2\xB0") + "OH";   // hydroxyl radical
const G4String kHO2 = G4String("HO2") + "\xC2\xB0"; // hydroperoxyl radical

using MolConf = const G4MolecularConfiguration*;

MolConf Conf(const G4String& name)
{
  auto* p = G4MoleculeTable::Instance()->GetConfiguration(name);
  if (p == nullptr) {
    G4Exception("PureWaterReactions::BuildPureWaterReactions", "MissingSpecies", FatalException,
                (G4String("Unknown species configuration: ") + name).c_str());
  }
  return p;
}
}  // namespace

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PureWaterReactions::BuildPureWaterReactions(G4DNAMolecularReactionTable* reactionTable)
{
  auto add = [reactionTable](MolConf a, MolConf b, G4double k,
                             std::initializer_list<MolConf> products) {
    auto* rd = new G4DNAMolecularReactionData(k * (1e-3 * m3 / (mole * s)), a, b);
    for (auto* p : products) {
      rd->AddProduct(p);
    }
    reactionTable->SetReaction(rd);
  };

  auto* e_aq = Conf("e_aq");
  auto* H = Conf("H");
  auto* H2 = Conf("H2");
  auto* OH = Conf(kOH);
  auto* OHm = Conf("OHm");
  auto* H3Op = Conf("H3Op");
  auto* H2O2 = Conf("H2O2");
  auto* O2 = Conf("O2");
  auto* Om = Conf("Om");
  auto* HO2 = Conf(kHO2);
  auto* HO2m = Conf("HO2m");
  auto* O2m = Conf("O2m");
  auto* O3m = Conf("O3m");

  // Pure-water radiolysis (project values -- not UHDR's SecondOrderReactionExtended
  // "Type I" block, which covers the same 9 pairs with different rate constants
  // for 7 of them; keeping these avoids overwriting/duplicating SetReaction
  // calls for the same reactant pair).
  add(H, H, 1.2e10, {H2});
  add(e_aq, H, 2.65e10, {H2, OHm});
  add(e_aq, e_aq, 0.5e10, {H2, OHm, OHm});
  add(H3Op, OHm, 1.43e11, {});
  add(e_aq, OH, 2.95e10, {OHm});
  add(OH, OH, 0.44e10, {H2O2});
  add(e_aq, H2O2, 1.41e10, {OHm, OH});
  add(e_aq, H3Op, 2.11e10, {H});
  add(OH, H, 1.44e10, {});

  // Bulk-O2 scavenging, against the real diffusing O2 species, not O2(B)
  // (UHDR: ChemOxygenWaterBuilder::OxygenScavengerReaction).
  add(e_aq, O2, 1.74e10, {O2m});
  add(H, O2, 2.1e10, {HO2});
  add(Om, O2, 3.7e9, {O3m});

  // O2-/HO2/HO2-/O-/O3- second-order network (UHDR:
  // ChemOxygenWaterBuilder::SecondOrderReactionExtended, "extended" block
  // only; NO2-/CO2/HCO3-/N2O/MeOH lines excluded -- out of project scope).
  add(H, Om, 2.00e10, {OHm});
  add(H3Op, O3m, 9.0e10, {OH, O2});
  add(H, HO2, 1.00e10, {H2O2});
  add(H, O2m, 1.00e10, {HO2m});
  add(OH, O2m, 1.07e10, {O2, OHm});
  add(e_aq, O2m, 1.3e10, {H2O2, OHm, OHm});
  add(e_aq, HO2m, 3.51e9, {Om, OHm});
  add(e_aq, Om, 2.31e10, {OHm, OHm});
  add(H3Op, O2m, 4.78e10, {HO2});
  add(H3Op, HO2m, 4.78e10, {H2O2});
  add(H3Op, Om, 4.78e10, {OH});
  add(e_aq, HO2, 1.29e10, {HO2m});
  add(OH, OHm, 1.27e10, {Om});
  add(OH, HO2, 7.90e9, {O2});
  add(OH, HO2m, 8.32e9, {HO2, OHm});
  add(OH, Om, 1.00e9, {HO2m});
  add(OH, O3m, 8.50e9, {O2m, HO2});
  add(OHm, HO2, 1.27e10, {O2m});
  add(H2O2, OHm, 1.3e10, {HO2m});
  add(H2O2, Om, 5.55e8, {HO2, OHm});
  add(H2, Om, 1.21e8, {H, OHm});
  add(O2m, Om, 6.00e8, {O2, OHm, OHm});
  add(HO2m, Om, 3.50e8, {O2m, OHm});
  add(Om, Om, 1.00e8, {H2O2, OHm, OHm});
  add(Om, O3m, 7.00e8, {O2m, O2m});
  add(H, OHm, 2.51e7, {e_aq});
  add(H, H2O2, 3.50e7, {OH});
  add(OH, H2O2, 2.88e7, {HO2});
  add(OH, H2, 3.28e7, {H});
  add(HO2, HO2, 9.80e5, {H2O2, O2});
  add(HO2, O2m, 9.70e7, {HO2m, O2});
  add(O2m, O2m, 1.0e2, {H2O2, O2, OHm, OHm});
}
