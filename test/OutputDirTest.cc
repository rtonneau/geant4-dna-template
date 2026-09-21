/// \file OutputDirTest.cc
/// \brief Plain-assert unit tests for OutputDir (no test framework, no
/// Geant4 runtime -- exercises pure directory-configuration/path-join logic
/// only).

#include "OutputDir.hh"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace
{
  fs::path TestRoot()
  {
    return fs::temp_directory_path() / "OutputDirTest";
  }

  void ResetTestRoot()
  {
    std::error_code ec;
    fs::remove_all(TestRoot(), ec);
    fs::create_directories(TestRoot(), ec);
  }
}

// --- Configure ---------------------------------------------------------

static void TestConfigureEmptyDirIsNoOp()
{
  G4String err;
  assert(OutputDir::Configure("", err));
  assert(err.empty());
}

static void TestConfigureCreatesMissingLeafDirectory()
{
  ResetTestRoot();
  fs::path target = TestRoot() / "fresh";
  assert(!fs::exists(target));

  G4String err;
  assert(OutputDir::Configure(target.string().c_str(), err));
  assert(fs::exists(target));
  assert(fs::is_directory(target));
}

static void TestConfigureAcceptsAlreadyExistingDirectory()
{
  ResetTestRoot();
  fs::path target = TestRoot() / "already-there";
  fs::create_directory(target);

  G4String err;
  assert(OutputDir::Configure(target.string().c_str(), err));
}

static void TestConfigureRejectsPathThatIsAFile()
{
  ResetTestRoot();
  fs::path target = TestRoot() / "im-a-file";
  std::ofstream(target.string()) << "x";

  G4String err;
  assert(!OutputDir::Configure(target.string().c_str(), err));
  assert(!err.empty());
}

static void TestConfigureRejectsMissingParent()
{
  ResetTestRoot();
  fs::path target = TestRoot() / "no-such-parent" / "leaf";

  G4String err;
  assert(!OutputDir::Configure(target.string().c_str(), err));
  assert(!err.empty());
  assert(!fs::exists(target));
}

// --- Resolve -------------------------------------------------------------

static void TestResolveWithoutConfigureReturnsFilenameUnchanged()
{
  G4String err;
  OutputDir::Configure("", err); // explicit empty configure = "not set"

  assert(OutputDir::Resolve("Species.Txt") == "Species.Txt");
}

static void TestResolveJoinsConfiguredDirectory()
{
  ResetTestRoot();
  fs::path target = TestRoot() / "joined";

  G4String err;
  assert(OutputDir::Configure(target.string().c_str(), err));

  fs::path expected = target / "Species.Txt";
  assert(OutputDir::Resolve("Species.Txt") == expected.string().c_str());
}

int main()
{
  TestConfigureEmptyDirIsNoOp();
  TestConfigureCreatesMissingLeafDirectory();
  TestConfigureAcceptsAlreadyExistingDirectory();
  TestConfigureRejectsPathThatIsAFile();
  TestConfigureRejectsMissingParent();
  TestResolveWithoutConfigureReturnsFilenameUnchanged();
  TestResolveJoinsConfiguredDirectory();

  std::error_code ec;
  fs::remove_all(TestRoot(), ec);

  std::cout << "All OutputDir tests passed." << std::endl;
  return 0;
}
