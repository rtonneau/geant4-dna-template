/// \file OutputDir.hh
/// \brief Process-wide output directory, set once from main() before any
/// worker thread is created.
///
/// Pure: no logging, no process exit. Configure(dir, err) sets the
/// process-wide output directory to dir, creating it if non-empty (its
/// parent must already exist); Resolve() joins that directory onto a
/// filename, or returns the filename unchanged if the configured directory
/// is empty (including before Configure() is ever called). Not
/// thread-synchronized -- callers must call Configure() before spawning any
/// worker thread that calls Resolve().

#ifndef OutputDir_h
#define OutputDir_h 1

#include "globals.hh"

namespace OutputDir
{
  /// Sets the configured output directory to dir. If dir is empty, records
  /// that and returns true immediately (no filesystem access). Otherwise
  /// creates only dir itself (non-recursive -- its parent must already
  /// exist); returns false and fills err if the parent is missing, or if
  /// dir exists but is not a directory.
  G4bool Configure(const G4String &dir, G4String &err);

  /// Joins the configured directory with filename, or returns filename
  /// unchanged if the configured directory is empty.
  G4String Resolve(const G4String &filename);
}

#endif // OutputDir_h
