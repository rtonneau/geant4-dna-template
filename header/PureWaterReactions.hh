/// \file PureWaterReactions.hh
/// \brief Portable pure-water radiolysis + O2-derived reaction-table builder
///
/// Self-contained, project-agnostic unit: no DnaChemistryWorld, DnaLogger, or
/// other dnachem-min-specific includes -- only standard Geant4/Geant4-DNA
/// headers. Copy-paste portable to another Geant4-DNA project.
///
/// Adds, directly to the supplied reaction table:
///   - the 9 base pure-water radiolysis reactions
///   - bulk-O2 scavenging: e_aq/H/O- + O2
///     (UHDR: ChemOxygenWaterBuilder::OxygenScavengerReaction)
///   - the O2-/HO2/HO2-/O-/O3- second-order network
///     (UHDR: ChemOxygenWaterBuilder::SecondOrderReactionExtended,
///      O2-relevant subset only -- NO2-/CO2/HCO3-/N2O/MeOH lines excluded,
///      out of project scope)
///
/// Hard-coded SBS assumption: no G4ChemTimeStepModel branching, no
/// conditional SetReactionType(1) (that existed only for IRT_syn support,
/// dropped project-wide).

#ifndef PureWaterReactions_h
#define PureWaterReactions_h 1

class G4DNAMolecularReactionTable;

namespace PureWaterReactions
{
  /// Populates reactionTable with the full pure-water + O2 ordinary
  /// (non-bulk) reaction set. Looks up species configurations internally via
  /// G4MoleculeTable::Instance()->GetConfiguration(name); raises a fatal
  /// G4Exception if any expected species is missing. Call after
  /// G4ChemDissociationChannels_option1::ConstructMolecule().
  void BuildPureWaterReactions(G4DNAMolecularReactionTable* reactionTable);
}  // namespace PureWaterReactions

#endif  // PureWaterReactions_h
