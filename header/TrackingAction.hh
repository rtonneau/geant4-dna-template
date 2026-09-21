#ifndef CHEM4_TRACKINGACTION_HH
#define CHEM4_TRACKINGACTION_HH

#include <G4UserTrackingAction.hh>

class TrackingAction : public G4UserTrackingAction
{
public:
  TrackingAction();
  virtual ~TrackingAction() {}
  virtual void PostUserTrackingAction(const G4Track *);
  virtual void PreUserTrackingAction(const G4Track *);
};

#endif
