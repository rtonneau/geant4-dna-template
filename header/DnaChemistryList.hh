/// \file DnaChemistryList.hh
/// \brief Definition of the DnaChemistryList class
///
/// Project chemical stage, replacing the macro-driven `/chem/species` +
/// `/chem/reaction/add` blocks. Inherits BOTH `G4VUserChemistryList`
/// (molecules / reactions / time-step model) and `G4VPhysicsConstructor`
/// (driven directly by PhysicsList, UHDR-style).
///
/// Base chemistry (always on): the portable pure-water + O2-derived
/// reaction network (PureWaterReactions.cc) plus the pH-driven acid-base
/// buffer equilibria against the bulk H3Op(B)/OHm(B) pseudo-species (UHDR:
/// ChemPureWaterBuilder), registered as per-molecule `G4DNAScavengerProcess`.
/// An actual dissolved-O2 supply/population is deferred to future work;
/// `/chem/env/O2` currently has no effect here.
///
/// Time-step model: SBS only (hard-coded; IRT and IRT_syn are not
/// supported).

#ifndef DnaChemistryList_h
#define DnaChemistryList_h 1

#include "G4VPhysicsConstructor.hh"
#include "G4VUserChemistryList.hh"
#include "globals.hh"

#include <memory>

class G4DNABoundingBox;
class G4DNAMolecularReactionTable;
class G4GenericMessenger;
class DnaChemistryWorld;

class DnaChemistryList : public G4VUserChemistryList, public G4VPhysicsConstructor
{
public:
  DnaChemistryList();
  ~DnaChemistryList() override = default;

  // --- G4VPhysicsConstructor ---
  void ConstructParticle() override { ConstructMolecule(); }
  void ConstructProcess() override;

  // --- G4VUserChemistryList ---
  void ConstructMolecule() override;
  void ConstructDissociationChannels() override;
  void ConstructReactionTable(G4DNAMolecularReactionTable* reactionTable) override;
  void ConstructTimeStepModel(G4DNAMolecularReactionTable* reactionTable) override;

private:
  /// The project chemistry world, via the run manager's detector.
  const DnaChemistryWorld* ChemistryWorld(const G4String& caller) const;

  /// Per-molecule G4DNAScavengerProcess for the pH-driven acid-base buffer
  /// equilibria against the bulk H3Op(B) / OHm(B) / H2O pseudo-species
  /// (UHDR: ChemPureWaterBuilder::WaterScavengerReaction). Always active --
  /// this network is baseline aqueous chemistry, not O2-specific.
  void RegisterAcidBaseScavengerProcesses(const G4DNABoundingBox& boundary) const;

  /// Exposes /chem/reaction/dump <filename>.
  std::unique_ptr<G4GenericMessenger> fMessenger;

  /// Target file for ConstructProcess() to dump the reaction table to;
  /// empty (default) disables the dump.
  G4String fReactionDumpFile;
};

#endif // DnaChemistryList_h
