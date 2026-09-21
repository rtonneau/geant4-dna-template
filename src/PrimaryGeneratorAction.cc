/// \file PrimaryGeneratorAction.cc
/// \brief Implementation of the PrimaryGeneratorAction class

#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : G4VUserPrimaryGeneratorAction(), fpParticleGun(new G4ParticleGun(1))
{
  G4ParticleDefinition *particle = G4ParticleTable::GetParticleTable()->FindParticle("e-");

  // default gun parameters
  this->fpParticleGun->SetParticleDefinition(particle);
  this->fpParticleGun->SetParticleEnergy(2. * MeV);
  this->fpParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
  this->fpParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 0. * micrometer));
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete this->fpParticleGun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::GeneratePrimaries(G4Event *anEvent)
{
  this->fpParticleGun->GeneratePrimaryVertex(anEvent);
}
