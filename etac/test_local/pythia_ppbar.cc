#include "Pythia8/Pythia.h"
#include <TH1D.h>
#include <TFile.h>
#include <TRandom3.h>
#include <iostream>

using namespace Pythia8;

int main() {

  // -------------------------------
  // 1. Setup
  // -------------------------------
  Pythia pythia;

  // Proton-proton collisions
  pythia.readString("Beams:idA = 2212");
  pythia.readString("Beams:idB = 2212");
  pythia.readString("Beams:eCM = 13000.");

  // Enable charm and charmonium
  pythia.readString("HardQCD:all = on");
  pythia.readString("Charmonium:all = on");

  // Force eta_c -> p pbar
  pythia.readString("441:onMode = off");
  pythia.readString("441:oneChannel = 1 1.0 0 2212 -2212");
  pythia.readString("441:onIfMatch = 2212 -2212");

  // (Optional) also enable J/psi for comparison
  pythia.readString("443:onMode = off");
  pythia.readString("443:oneChannel = 1 1.0 0 2212 -2212");
  pythia.readString("443:onIfMatch = 2212 -2212");

  pythia.init();

  // -------------------------------
  // 2. Histograms
  // -------------------------------
  TH1D* hTrue = new TH1D("hTrue", "#eta_{c} -> p#bar{p};M [GeV/c^{2}];Counts", 200, 2.5, 3.5);
  TH1D* hTrue_jpsi = new TH1D("hTrue_jpsi", "J/#psi -> p#bar{p};M [GeV/c^{2}];Counts", 200, 2.5, 3.5);
  TH1D* hAll  = new TH1D("hAll",  "All p#bar{p} pairs;M [GeV/c^{2}];Counts", 200, 2.5, 3.5);

  // Random generator for efficiency
  TRandom3 rnd(0);

  // -------------------------------
  // 3. Event loop
  // -------------------------------
  int nEvents = 1e6;

  for (int iEvent = 0; iEvent < nEvents; ++iEvent) {
    if (!pythia.next()) continue;

    // Store reconstructed protons (after cuts)
    std::vector<Vec4> protons;
    std::vector<int> charges;

    // -------------------------------
    // Loop over particles
    // -------------------------------
    for (int i = 0; i < pythia.event.size(); ++i) {

      auto& p = pythia.event[i];

      // Keep final-state particles only
      if (!p.isFinal()) continue;

      // Select protons
      if (abs(p.id()) == 2212) {

        // Acceptance cut (ALICE-like)
        if (fabs(p.eta()) > 0.8) continue;

        // Efficiency (40%)
        if (rnd.Rndm() > 0.4) continue;

        protons.push_back(p.p());
        charges.push_back(p.id());
      }
    }

    // -------------------------------
    // Build ALL p pbar pairs (background + signal)
    // -------------------------------
    for (size_t i = 0; i < protons.size(); ++i) {
      for (size_t j = i + 1; j < protons.size(); ++j) {

        // Unlike-sign only
        if (charges[i] * charges[j] < 0) {
          double m = (protons[i] + protons[j]).mCalc();
          hAll->Fill(m);
        }
      }
    }

    // -------------------------------
    // True eta_c pairs (signal only)
    // -------------------------------
    for (int i = 0; i < pythia.event.size(); ++i) {

      int id = pythia.event[i].id();
    
      // eta_c or J/psi
      if (id != 441 && id != 443) continue;
      std::cout << "Found particle " << id << " at index " << i << "\n";
    
      int d1 = pythia.event[i].daughter1();
      int d2 = pythia.event[i].daughter2();
    
      auto& p1 = pythia.event[d1];
      auto& p2 = pythia.event[d2];

      std::cout << "Daughter 1: ID=" << p1.id() << ", Daughter 2: ID=" << p2.id() << "\n";

      if (abs(p1.id()) == 2212 && abs(p2.id()) == 2212) {

        // acceptance
        //if (fabs(p1.eta()) > 0.8 || fabs(p2.eta()) > 0.8) continue;
    
        // efficiency
        //if (rnd.Rndm() > 0.4 || rnd.Rndm() > 0.4) continue;
    
        double m = (p1.p() + p2.p()).mCalc();
    
        if (id == 441) {
          hTrue->Fill(m);
        } else if (id == 443) {
          hTrue_jpsi->Fill(m);
        }
      } else {
        std::cerr << "Warning: Unexpected decay of particle " << id << " into " << p1.id() << " and " << p2.id() << "\n";
      }
    }

    if (iEvent % 100000 == 0)
      std::cout << "Processed " << iEvent << " events\n";
  }

  // -------------------------------
  // 4. Save output
  // -------------------------------
  TFile outFile("etac_ppbar.root", "RECREATE");
  hTrue->Write();
  hTrue_jpsi->Write();
  hAll->Write();
  outFile.Close();

  pythia.stat();

  return 0;
}