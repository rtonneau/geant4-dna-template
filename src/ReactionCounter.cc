/// \file ReactionCounter.cc
/// \brief Implementation of the ReactionCounter class

#include "ReactionCounter.hh"

#include "OutputDir.hh"

#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"

#include <ostream>
#include <vector>

namespace
{
// Same fixed checkpoint times ScoreSpecies uses for species snapshots, so
// this counter's time axis lines up with Species.Txt.
const std::vector<G4double> kTimeBins = {
    1 * CLHEP::picosecond,     10 * CLHEP::picosecond,  100 * CLHEP::picosecond,
    1000 * CLHEP::picosecond,  10000 * CLHEP::picosecond, 100000 * CLHEP::picosecond,
    999999 * CLHEP::picosecond,
};
}  // namespace

G4double ReactionCounter::BinFor(G4double time)
{
  for (G4double edge : kTimeBins) {
    if (time <= edge) {
      return edge;
    }
  }
  return kTimeBins.back();
}

void ReactionCounter::Record(const G4String& reactionLabel, G4double time)
{
  ++fCounts[BinFor(time)][reactionLabel];
}

void ReactionCounter::Merge(const ReactionCounter& other)
{
  for (const auto& [bin, reactions] : other.fCounts) {
    for (const auto& [label, count] : reactions) {
      fCounts[bin][label] += count;
    }
  }
}

void ReactionCounter::Clear()
{
  fCounts.clear();
}

void ReactionCounter::WriteAscii(std::ostream& out) const
{
  for (const auto& [bin, reactions] : fCounts) {
    out << bin << "\n";
    for (const auto& [label, count] : reactions) {
      out << label << "    count = " << count << "\n";
    }
  }
}

void ReactionCounter::WriteCsv(G4VAnalysisManager* analysisManager) const
{
  analysisManager->OpenFile(OutputDir::Resolve("Reactions"));

  G4int ntupleId = analysisManager->CreateNtuple("reactions", "reactions");
  analysisManager->CreateNtupleSColumn(ntupleId, "reaction");
  analysisManager->CreateNtupleDColumn(ntupleId, "time");
  analysisManager->CreateNtupleIColumn(ntupleId, "count");
  analysisManager->FinishNtuple(ntupleId);

  for (const auto& [bin, reactions] : fCounts) {
    for (const auto& [label, count] : reactions) {
      analysisManager->FillNtupleSColumn(ntupleId, 0, label);
      analysisManager->FillNtupleDColumn(ntupleId, 1, bin);
      analysisManager->FillNtupleIColumn(ntupleId, 2, count);
      analysisManager->AddNtupleRow(ntupleId);
    }
  }

  analysisManager->Write();
  analysisManager->CloseFile();
}
