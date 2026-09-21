#include <G4Track.hh>
#include <G4UnitsTable.hh>
#include <TrackingAction.hh>

TrackingAction::TrackingAction() : G4UserTrackingAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PostUserTrackingAction(const G4Track *track)
{
  // if (track->GetTrackID() == 1)
  // {
  //   G4cout << "Energy of primary: " << G4BestUnit(track->GetKineticEnergy(), "Energy") << G4endl;
  // }
}

void TrackingAction::PreUserTrackingAction(const G4Track *track)
{
  // if (track->GetParentID() == 0)
  // {
  //   G4cout << "New primary track, TrackID = "
  //          << track->GetTrackID()
  //          << ", Particle = " << track->GetDefinition()->GetParticleName()
  //          << G4endl;
  // }
}
