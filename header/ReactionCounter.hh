/// \file ReactionCounter.hh
/// \brief Definition of the ReactionCounter class
///
/// Accumulates per-time-bin firing counts of bimolecular reactions, keyed by
/// the same "A + B -> C + D" label ReactionTableDump uses for the static
/// reaction table, so the two outputs cross-reference directly. Pure
/// accumulation/binning logic -- no Geant4 runtime dependency, callers
/// resolve reaction labels and feed them in via Record().

#ifndef ReactionCounter_h
#define ReactionCounter_h 1

#include "globals.hh"

#include <iosfwd>
#include <map>

class G4VAnalysisManager;

class ReactionCounter
{
 public:
  /// Returns the upper edge of the fixed time bin `time` falls into (times
  /// above the last edge clamp to the last edge).
  static G4double BinFor(G4double time);

  void Record(const G4String& reactionLabel, G4double time);

  /// Adds `other`'s counts into this counter; `other` is left unchanged.
  void Merge(const ReactionCounter& other);

  void Clear();

  /// Writes one "<label>    count = N" line per (bin, reaction), grouped by
  /// bin -- mirrors Species.Txt's per-time-block layout.
  void WriteAscii(std::ostream& out) const;

  /// Writes one ntuple row per (bin, reaction) via the given analysis
  /// manager, opened as its own "Reactions" file (mirrors ScoreSpecies's
  /// WriteWithAnalysisManager, kept separate to avoid ntuple-ID collisions
  /// with the species ntuple).
  void WriteCsv(G4VAnalysisManager* analysisManager) const;

  using BinCounts = std::map<G4String, G4int>;
  using Counts = std::map<G4double, BinCounts>;
  const Counts& GetCounts() const { return fCounts; }

 private:
  Counts fCounts;
};

#endif  // ReactionCounter_h
