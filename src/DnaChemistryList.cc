/// \file DnaChemistryList.cc
/// \brief Implementation of the DnaChemistryList class
///
/// Structure mirrors G4EmDNAChemistry_option3 and the UHDR example's
/// EmDNAChemistry. Molecules + water dissociation channels come from
/// G4ChemDissociationChannels_option1. The ordinary (non-bulk) reaction
/// network -- pure-water radiolysis plus the O2-derived second-order
/// network -- lives in the portable PureWaterReactions.cc.
///
/// The pH-driven acid-base buffer equilibria against the bulk H3Op(B) /
/// OHm(B) pseudo-species (UHDR: ChemPureWaterBuilder::WaterScavengerReaction)
/// are always active, registered as per-molecule G4DNAScavengerProcess --
/// this network is baseline aqueous chemistry (it can produce O2 from pure
/// water radiolysis on its own), not gated by whether O2 is enabled. See
/// docs/adr/0001-baseline-acid-base-buffer.md.

#include "DnaChemistryList.hh"

#include "DetectorConstruction.hh"
#include "DnaChemistryWorld.hh"
#include "DnaLogger.hh"
#include "OutputDir.hh"
#include "PureWaterReactions.hh"
#include "ReactionTableDump.hh"
#include "ScavengerReactionAccess.hh"

#include "G4ApplicationState.hh"
#include "G4ChemDissociationChannels_option1.hh"
#include "G4DNABoundingBox.hh"
#include "G4DNAChemistryManager.hh"
#include "G4DNAMolecularReactionTable.hh"
#include "G4GenericMessenger.hh"
#include "G4MolecularConfiguration.hh"
#include "G4MoleculeDefinition.hh"
#include "G4MoleculeTable.hh"
#include "G4PhysicsListHelper.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessTable.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"

// Chemical-stage processes
#include "G4DNABrownianTransportation.hh"
#include "G4DNAElectronHoleRecombination.hh"
#include "G4DNAElectronSolvation.hh"
#include "G4DNAMolecularDissociation.hh"
#include "G4DNASancheExcitationModel.hh"
#include "G4DNAScavengerProcess.hh"
#include "G4DNAVibExcitation.hh"
#include "G4DNAWaterDissociationDisplacer.hh"

// Time-step model
#include "G4DNAMolecularStepByStepModel.hh"
#include "G4DNAScavengerMaterial.hh"
#include "G4Scheduler.hh"

// Particles
#include "G4Electron.hh"
#include "G4H2O.hh"

#include "G4PhysicsConstructorFactory.hh"

#include <initializer_list>
#include <memory>
#include <vector>

G4_DECLARE_PHYSCONSTR_FACTORY(DnaChemistryList);

namespace
{
// Configuration tags containing the UTF-8 degree sign (0xC2 0xB0), spelled
// from explicit bytes so they match the names stored by
// G4ChemDissociationChannels_option1 whatever this file's source encoding is.
const G4String kOH = G4String("\xC2\xB0") + "OH";   // hydroxyl radical
const G4String kHO2 = G4String("HO2") + "\xC2\xB0"; // hydroperoxyl radical

using MolConf = const G4MolecularConfiguration*;

MolConf Conf(const G4String& name, const G4String& caller)
{
  auto* p = G4MoleculeTable::Instance()->GetConfiguration(name);
  if (p == nullptr) {
    G4Exception(("DnaChemistryList::" + caller).c_str(), "MissingSpecies", FatalException,
                (G4String("Unknown species configuration: ") + name).c_str());
  }
  return p;
}
}  // namespace

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DnaChemistryList::DnaChemistryList()
  : G4VUserChemistryList(true)
{
  // Register with the chemistry manager (also activates the chemical stage).
  // The (true) flag marks this object as a G4VPhysicsConstructor so the manager
  // releases - rather than deletes - it, leaving ownership with the PhysicsList
  // that holds it (avoids a double free).
  G4DNAChemistryManager::Instance()->SetChemistryList(this);

  fMessenger = std::make_unique<G4GenericMessenger>(this, "/chem/reaction/",
                                                    "Chemistry reaction-table diagnostics");
  auto& dumpCmd = fMessenger->DeclareProperty(
    "dump", fReactionDumpFile,
    "Write the full reaction table (bimolecular + acid-base networks) to <filename>.");
  dumpCmd.SetStates(G4State_PreInit);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

const DnaChemistryWorld* DnaChemistryList::ChemistryWorld(const G4String& caller) const
{
  const auto* detector = dynamic_cast<const DetectorConstruction*>(
    G4RunManager::GetRunManager()->GetUserDetectorConstruction());
  const auto* world =
    (detector != nullptr) ? dynamic_cast<const DnaChemistryWorld*>(detector->GetChemistryWorld())
                          : nullptr;
  if (world == nullptr) {
    G4Exception((G4String("DnaChemistryList::") + caller).c_str(), "NoChemistryWorld",
                FatalException, "Expected a DetectorConstruction owning a DnaChemistryWorld.");
  }
  return world;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DnaChemistryList::ConstructMolecule()
{
  // Standard radiolysis set + "(B)" bulk pseudo-species + water dissociation.
  G4ChemDissociationChannels_option1::ConstructMolecule();

  // Required by G4DNAScavengerProcess (member init: GetConfiguration("H2O")).
  G4MoleculeTable::Instance()->CreateConfiguration("H2O", G4H2O::Definition());
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DnaChemistryList::ConstructDissociationChannels()
{
  G4ChemDissociationChannels_option1::ConstructDissociationChannels();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DnaChemistryList::ConstructReactionTable(G4DNAMolecularReactionTable* reactionTable)
{
  // Populate the chemistry-world bulk composition here (master-only phase ->
  // race-free), before it is read by G4DNAScavengerMaterial.
  auto* chemWorld =
    const_cast<DnaChemistryWorld*>(ChemistryWorld("ConstructReactionTable"));
  chemWorld->ConstructChemistryComponents();

  // Pure water + O2-derived second-order network (portable unit).
  PureWaterReactions::BuildPureWaterReactions(reactionTable);

  DnaLogger::Print(DnaLogger::Level::Info,
                   "[DnaChemistryList] reaction table constructed "
                   "(pure water + O2 network)");
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DnaChemistryList::ConstructTimeStepModel(G4DNAMolecularReactionTable* /*reactionTable*/)
{
  RegisterTimeStepModel(new G4DNAMolecularStepByStepModel(), 0);

  DnaLogger::Print(DnaLogger::Level::Info,
                   "[DnaChemistryList] time-step model = SBS (hard-coded)");
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DnaChemistryList::ConstructProcess()
{
  auto* ph = G4PhysicsListHelper::GetPhysicsListHelper();

  // Extend the Sanche vibrational-excitation model down to thermal energies.
  auto* vibProcess =
    G4ProcessTable::GetProcessTable()->FindProcess("e-_G4DNAVibExcitation", "e-");
  if (vibProcess != nullptr) {
    auto* vib = dynamic_cast<G4DNAVibExcitation*>(vibProcess);
    if (vib != nullptr) {
      auto* sanche = dynamic_cast<G4DNASancheExcitationModel*>(vib->EmModel());
      if (sanche != nullptr) {
        sanche->ExtendLowEnergyLimit(0.025 * eV);
      }
    }
  }

  // Electron solvation: free e- -> e_aq (register once if not already present).
  if (G4ProcessTable::GetProcessTable()->FindProcess("e-_G4DNAElectronSolvation", "e-") ==
      nullptr) {
    ph->RegisterProcess(new G4DNAElectronSolvation("e-_G4DNAElectronSolvation"),
                        G4Electron::Definition());
  }

  auto* moleculeTable = G4MoleculeTable::Instance();
  auto iterator = moleculeTable->GetDefintionIterator();
  iterator.reset();
  while (iterator()) {
    auto* moleculeDef = iterator.value();

    if (moleculeDef != G4H2O::Definition()) {
      // SBS is the only supported time-step model; always transport.
      ph->RegisterProcess(new G4DNABrownianTransportation(), moleculeDef);
    }
    else {
      moleculeDef->GetProcessManager()->AddRestProcess(new G4DNAElectronHoleRecombination(), 2);
      auto* dissociation = new G4DNAMolecularDissociation("H2O_DNAMolecularDecay");
      dissociation->SetDisplacer(moleculeDef, new G4DNAWaterDissociationDisplacer);
      moleculeDef->GetProcessManager()->AddRestProcess(dissociation, 1);
    }
  }

  auto* chemWorld = const_cast<DnaChemistryWorld*>(ChemistryWorld("ConstructProcess"));
  RegisterAcidBaseScavengerProcesses(*chemWorld->GetChemistryBoundary());

  // Triggers InitializeMaster() -> ConstructReactionTable() ->
  // DnaChemistryWorld::ConstructChemistryComponents(): the bulk composition is
  // populated after this call returns.
  G4DNAChemistryManager::Instance()->Initialize();

  // Install the bulk-scavenger material now that the composition is known, and
  // before G4DNAScavengerProcess::BuildPhysicsTable() queries the scheduler.
  // Runs once (serial) or per worker thread (MT); the scheduler is thread-local.
  // Unconditional: the acid-base buffer scavenger processes registered above
  // are always active, so this material must always be installed -- without
  // it, G4DNAScavengerProcess::PostStepGetPhysicalInteractionLength would
  // dereference a null fpScavengerMaterial on the first step of any species
  // with a registered reaction.
  if (G4Scheduler::Instance()->GetScavengerMaterial() == nullptr) {
    auto scavenger = std::make_unique<G4DNAScavengerMaterial>(chemWorld);
    scavenger->SetCounterAgainstTime();
    G4Scheduler::Instance()->SetScavengerMaterial(std::move(scavenger));
  }

  // Both networks (bimolecular + acid-base) are fully constructed by this
  // point; opt-in dump for external checks (see /chem/reaction/dump).
  if (!fReactionDumpFile.empty()) {
    ReactionTableDump::DumpReactionTable(OutputDir::Resolve(fReactionDumpFile));
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DnaChemistryList::RegisterAcidBaseScavengerProcesses(const G4DNABoundingBox& boundary) const
{
  auto* ph = G4PhysicsListHelper::GetPhysicsListHelper();

  auto* e_aq = Conf("e_aq", "RegisterAcidBaseScavengerProcesses");
  auto* H = Conf("H", "RegisterAcidBaseScavengerProcesses");
  auto* OH = Conf(kOH, "RegisterAcidBaseScavengerProcesses");
  auto* OHm = Conf("OHm", "RegisterAcidBaseScavengerProcesses");
  auto* Om = Conf("Om", "RegisterAcidBaseScavengerProcesses");
  auto* O2 = Conf("O2", "RegisterAcidBaseScavengerProcesses");
  auto* O2m = Conf("O2m", "RegisterAcidBaseScavengerProcesses");
  auto* O3m = Conf("O3m", "RegisterAcidBaseScavengerProcesses");
  auto* HO2 = Conf(kHO2, "RegisterAcidBaseScavengerProcesses");
  auto* HO2m = Conf("HO2m", "RegisterAcidBaseScavengerProcesses");
  auto* H2O2 = Conf("H2O2", "RegisterAcidBaseScavengerProcesses");
  auto* H3Op = Conf("H3Op", "RegisterAcidBaseScavengerProcesses");
  auto* H3OpB = Conf("H3Op(B)", "RegisterAcidBaseScavengerProcesses");
  auto* OHmB = Conf("OHm(B)", "RegisterAcidBaseScavengerProcesses");
  auto* H2O = Conf("H2O", "RegisterAcidBaseScavengerProcesses");

  struct Rx
  {
    MolConf bulk;
    G4double rate;  // already dimensioned
    std::vector<MolConf> products;
    G4int type;
  };

  // Registers against mol->GetDefinition() rather than a caller-supplied
  // class pointer: several species here (OHm, H3Op) have no dedicated
  // factory class, and using GetDefinition() uniformly avoids the previous
  // mismatch where Om's process was attached to G4Oxygen::Definition() (the
  // separate, unused "Oxy" species) instead of Om's actual definition (a
  // private G4MoleculeDefinition("O", ...) created inside
  // G4ChemDissociationChannels_option1::ConstructMolecule()) -- silently
  // making those reactions unreachable.
  auto build = [&](MolConf mol, std::initializer_list<Rx> reactions) {
    auto* process = new ScavengerReactionAccess("G4DNAScavengerProcess", boundary);
    for (const auto& r : reactions) {
      auto* rd = new G4DNAMolecularReactionData(r.rate, mol, r.bulk);
      for (auto* p : r.products) {
        rd->AddProduct(p);
      }
      if (r.type != 0) {
        rd->SetReactionType(r.type);
      }
      process->SetReaction(mol, rd);
    }
    auto* def = const_cast<G4MoleculeDefinition*>(mol->GetDefinition());
    ph->RegisterProcess(process, def);
  };

  const G4double M = 1e-3 * m3 / (mole * s);  // bimolecular unit (M^-1 s^-1)
  const G4double cW = 55.3;                   // bulk water molarity factor

  build(H, {{H2O, 6.32 / s, {e_aq, H3OpB}, 0}, {OHmB, 2.49e7 * M, {e_aq}, 0}});

  build(e_aq, {{H3OpB, 2.25e10 * M, {H}, 0}, {H2O, 1.57e1 * cW / s, {H, OHmB}, 0}});

  build(O2m, {{H3OpB, 4.78e10 * M, {HO2}, 6}, {H2O, 0.15 * cW / s, {HO2, OHmB}, 0}});

  build(HO2, {{OHmB, 1.27e10 * M, {O2m}, 0}, {H2O, 7.58e5 / s, {H3OpB, O2m}, 6}});

  build(HO2m, {{H3OpB, 4.78e10 * M, {H2O2}, 0}, {H2O, 1.36e6 * cW / s, {H2O2, OHmB}, 7}});

  build(Om, {{H3OpB, 9.56e10 * M, {OH}, 0}, {H2O, 1.8e6 * cW / s, {OH, OHmB}, 8}});

  build(O3m, {{H3OpB, 9.0e10 * M, {OH, O2}, 0}, {H2O, 2.66e3 / s, {Om, O2}, 0}});

  build(H2O2, {{H2O, 7.86e-2 / s, {HO2m, H3OpB}, 0}, {OHmB, 1.27e10 * M, {HO2m}, 7}});

  build(OH, {{OHmB, 1.27e10 * M, {Om}, 8}, {H2O, 0.060176635 / s, {Om, H3OpB}, 0}});

  build(OHm, {{H3OpB, 1.13e11 * M, {}, 0}});

  build(H3Op, {{OHmB, 1.13e11 * M, {}, 0}});
}
