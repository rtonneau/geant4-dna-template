/// \file ReactionTableDump.cc
/// \brief Implementation of the ReactionTableDump utility functions

#include "ReactionTableDump.hh"

#include "DnaLogger.hh"
#include "ScavengerReactionAccess.hh"

#include "G4DNAMolecularReactionTable.hh"
#include "G4MoleculeDefinition.hh"
#include "G4MoleculeTable.hh"
#include "G4ProcessTable.hh"
#include "G4SystemOfUnits.hh"

#include <fstream>
#include <ostream>
#include <sstream>
#include <vector>

namespace
{
const G4double kRateUnit = 1e-3 * m3 / (mole * s);  // M^-1 s^-1

void WriteLine(std::ostream& out, const G4String& reactant1, const G4String& reactant2,
               G4double rate,
               const std::vector<const G4MolecularConfiguration*>& products)
{
  std::vector<G4String> productNames;
  productNames.reserve(products.size());
  for (const auto* p : products) {
    productNames.push_back(p->GetName());
  }
  out << ReactionTableDump::FormatReactionLabel(reactant1, reactant2, productNames) << "    k = "
      << (rate / kRateUnit) << " M^-1 s^-1\n";
}
}  // namespace

namespace ReactionTableDump
{
G4String FormatReactionLabel(const G4String& reactant1, const G4String& reactant2,
                              const std::vector<G4String>& productNames)
{
  std::ostringstream oss;
  oss << reactant1 << " + " << reactant2 << " ->";
  if (productNames.empty()) {
    oss << " (no products)";
  }
  else {
    for (std::size_t i = 0; i < productNames.size(); ++i) {
      oss << (i == 0 ? " " : " + ") << productNames[i];
    }
  }
  return oss.str();
}

void WriteBimolecular(std::ostream& out)
{
  auto* reactionTable = G4DNAMolecularReactionTable::GetReactionTable();
  auto reactions = reactionTable->GetVectorOfReactionData();

  for (const auto* rd : reactions) {
    std::vector<const G4MolecularConfiguration*> products;
    const G4int nbProducts = rd->GetNbProducts();
    for (G4int i = 0; i < nbProducts; ++i) {
      products.push_back(rd->GetProduct(i));
    }
    WriteLine(out, rd->GetReactant1()->GetName(), rd->GetReactant2()->GetName(),
             rd->GetObservedReactionRateConstant(), products);
  }
}

void WriteAcidBase(std::ostream& out)
{
  auto* processTable = G4ProcessTable::GetProcessTable();
  auto* moleculeTable = G4MoleculeTable::Instance();

  auto iterator = moleculeTable->GetDefintionIterator();
  iterator.reset();
  while (iterator()) {
    auto* moleculeDef = iterator.value();

    auto* process = processTable->FindProcess("G4DNAScavengerProcess", moleculeDef);
    auto* access = dynamic_cast<ScavengerReactionAccess*>(process);
    if (access == nullptr) {
      continue;
    }

    for (const auto& [mol, materialMap] : access->GetReactionMap()) {
      for (const auto& [material, rd] : materialMap) {
        std::vector<const G4MolecularConfiguration*> products;
        const G4int nbProducts = rd->GetNbProducts();
        for (G4int i = 0; i < nbProducts; ++i) {
          products.push_back(rd->GetProduct(i));
        }
        WriteLine(out, mol->GetName(), material->GetName(),
                 rd->GetObservedReactionRateConstant(), products);
      }
    }
  }
}

void DumpReactionTable(const G4String& filename)
{
  std::ofstream out(filename);
  if (!out.is_open()) {
    G4Exception("ReactionTableDump::DumpReactionTable", "CannotOpenFile", FatalException,
               (G4String("Could not open reaction dump file: ") + filename).c_str());
    return;
  }

  out << "# Bimolecular reactions (pure water + O2 network)\n";
  WriteBimolecular(out);
  out << "\n# Acid-base reactions (bulk scavenger network)\n";
  WriteAcidBase(out);

  DnaLogger::Print(DnaLogger::Level::Info,
                   "[ReactionTableDump] reaction table written to " + filename);
}
}  // namespace ReactionTableDump
