/// \file ReactionTableDump.hh
/// \brief Definition of the ReactionTableDump utility functions
///
/// Dumps the two reaction networks the chemistry stage builds to a plain
/// text file, read directly from the live Geant4 objects at call time:
/// the shared G4DNAMolecularReactionTable singleton (bimolecular pure
/// water + O2 network), and each molecule's ScavengerReactionAccess
/// process instance (per-molecule acid-base/bulk scavenger network).
/// Portable utility, no project-specific state -- copy-paste portable
/// like PureWaterReactions.cc.

#ifndef ReactionTableDump_h
#define ReactionTableDump_h 1

#include "globals.hh"

#include <iosfwd>
#include <vector>

namespace ReactionTableDump
{
/// Formats "A + B -> C + D" (no rate) from reactant/product names. Pure
/// string logic, shared by WriteLine here and by TimeStepAction's live
/// per-occurrence reaction counting, so both report reactions under
/// identical labels.
G4String FormatReactionLabel(const G4String& reactant1, const G4String& reactant2,
                              const std::vector<G4String>& productNames);

/// Writes the shared bimolecular reaction table (pure water + O2 network).
void WriteBimolecular(std::ostream& out);

/// Writes the per-molecule acid-base/bulk scavenger reaction network.
void WriteAcidBase(std::ostream& out);

/// Opens `filename` and writes both networks, in labeled sections.
void DumpReactionTable(const G4String& filename);
}  // namespace ReactionTableDump

#endif  // ReactionTableDump_h
