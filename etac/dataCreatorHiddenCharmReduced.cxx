// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file dataCreatorHiddenCharmReduced.cxx
/// \brief Reduced data creator for hidden-charm analyses at midrapidity
///
/// \author A. Palasciano, INFN Bari
/// \author S. Politanò, CERN

#include "PWGHF/Core/CentralityEstimation.h"
#include "PWGHF/Core/SelectorCuts.h"
#include "PWGHF/DataModel/AliasTables.h"
#include "PWGHF/D2H/DataModel/ReducedDataModel.h"
#include "PWGHF/DataModel/CandidateSelectionTables.h"
#include "PWGHF/DataModel/TrackIndexSkimmingTables.h"
#include "PWGHF/Utils/utilsAnalysis.h"
#include "PWGHF/D2H/DataModel/HiddenCharm.h"
#include "PWGHF/Utils/utilsEvSelHf.h"

#include "Common/DataModel/Centrality.h"
#include "Common/DataModel/CollisionAssociationTables.h"
#include "Common/DataModel/EventSelection.h"
#include "Common/DataModel/PIDResponseTOF.h"
#include "Common/DataModel/PIDResponseTPC.h"
#include "Common/DataModel/TrackSelectionTables.h"

#include <CCDB/BasicCCDBManager.h>
#include <Framework/AnalysisDataModel.h>
#include <Framework/AnalysisTask.h>
#include <Framework/Configurable.h>
#include <Framework/HistogramRegistry.h>
#include <Framework/HistogramSpec.h>
#include <Framework/InitContext.h>
#include <Framework/Logger.h>
#include <Framework/runDataProcessing.h>

#include <array>
#include <cmath>
#include <numeric>
#include <string>
#include <vector>

using namespace o2;
using namespace o2::analysis;
using namespace o2::aod;
using namespace o2::framework;
using namespace o2::framework::expressions;
using namespace o2::hf_centrality;
using namespace o2::hf_evsel;

enum TrackType : uint8_t {
  Pion = 0,
  Kaon,
  Proton
};

struct HfHiddenCharmSelCollisions {
  Produces<aod::HcSelCollisions> rowSelectedCollision;

  Configurable<bool> fillHistograms{"fillHistograms", true, "Fill collision QA histograms"};

  HfEventSelection hfEvSel;
  Service<o2::ccdb::BasicCCDBManager> ccdb{};
  HistogramRegistry registry{"registry"};

  void init(InitContext const&)
  {
    const std::array<int, 2> doProcess = {doprocessNoTrigSel, doprocessTrigAndCentFT0CSel};
    if (std::accumulate(doProcess.begin(), doProcess.end(), 0) != 1) {
      LOGP(fatal, "One and only one hidden-charm collision-selection process can be enabled at a time.");
    }

    hfEvSel.init(registry);
    if (fillHistograms && doprocessTrigAndCentFT0CSel) {
      const AxisSpec axisCentrality{200, 0., 100., "centrality percentile"};
      registry.add("hCentralitySelected", "Centrality percentile of selected events in the centrality interval;centrality percentile;entries", {HistType::kTH1D, {axisCentrality}});
      registry.add("hCentralityRejected", "Centrality percentile of selected events outside the centrality interval;centrality percentile;entries", {HistType::kTH1D, {axisCentrality}});
    }
  }

  template <o2::hf_centrality::CentralityEstimator CentEstimator, typename Col, typename BCsType>
  void selectCollision(Col const& collision, BCsType const&)
  {
    float centrality{-1.f};
    const auto rejectionMask = hfEvSel.getHfCollisionRejectionMask<false, CentEstimator, BCsType>(collision, centrality, ccdb, registry);

    if (fillHistograms) {
      hfEvSel.fillHistograms(collision, rejectionMask, centrality);
      if constexpr (CentEstimator != CentralityEstimator::None) {
        if (rejectionMask == 0) {
          registry.fill(HIST("hCentralitySelected"), centrality);
        } else if (rejectionMask == BIT(EventRejection::Centrality)) {
          registry.fill(HIST("hCentralityRejected"), centrality);
        }
      }
    }

    rowSelectedCollision(collision.globalIndex(), rejectionMask);
  }

  void processNoTrigSel(soa::Join<aod::Collisions, aod::EvSels>::iterator const& collision,
                        aod::BcFullInfos const& bcs)
  {
    selectCollision<CentralityEstimator::None>(collision, bcs);
  }
  PROCESS_SWITCH(HfHiddenCharmSelCollisions, processNoTrigSel, "Select pp collisions without centrality", true);

  void processTrigAndCentFT0CSel(soa::Join<aod::Collisions, aod::EvSels, aod::CentFT0Cs>::iterator const& collision,
                                 aod::BcFullInfos const& bcs)
  {
    selectCollision<CentralityEstimator::FT0C>(collision, bcs);
  }
  PROCESS_SWITCH(HfHiddenCharmSelCollisions, processTrigAndCentFT0CSel, "Select PbPb collisions with trigger and FT0C centrality", false);
};

struct HfHiddenCharmSelTracks {
  Produces<aod::HcSelTracks> hfTrackLite;

  struct : ConfigurableGroup {
    Configurable<bool> fillHistograms{"fillHistograms", true, "Fill proton QA histograms"};
    Configurable<bool> selectProtons{"selectProtons", true, "Select protons"};
    Configurable<int> itsNClsMin{"itsNClsMin", 5, "Minimum number of ITS clusters"};
    Configurable<int> tpcNClsFoundMin{"tpcNClsFoundMin", 50, "Minimum number of found TPC clusters"};
    Configurable<int> tpcNClsCrossedRowsMin{"tpcNClsCrossedRowsMin", 80, "Minimum number of crossed TPC rows"};
    Configurable<double> ptMinTrack{"ptMinTrack", 0.5, "Minimum proton-track pT"};
    Configurable<double> etaMaxTrack{"etaMaxTrack", 0.8, "Maximum proton-track |eta|"};
    Configurable<float> momForCombinedPid{"momForCombinedPid", 0.75f, "Momentum threshold above which combined TPC+TOF proton PID is used"};
    Configurable<std::vector<double>> binsPtTrack{"binsPtTrack", std::vector<double>{hf_cuts_single_track::vecBinsPtTrack}, "Track pT bin limits for DCA cuts"};
    Configurable<LabeledArray<double>> cutsTrack{"cutsTrack", {hf_cuts_single_track::CutsTrack[0], hf_cuts_single_track::NBinsPtTrack, hf_cuts_single_track::NCutVarsTrack, hf_cuts_single_track::labelsPtTrack, hf_cuts_single_track::labelsCutVarTrack}, "Single-track DCA selections per pT bin"};
    Configurable<float> maxNsigmaTofPi{"maxNsigmaTofPi", 2.f, "Maximum pion n-sigma in TOF for proton rejection"};
    Configurable<float> maxNsigmaTofKa{"maxNsigmaTofKa", 2.f, "Maximum kaon n-sigma in TOF for proton rejection"};
    Configurable<float> maxNsigmaCombinedPr{"maxNsigmaCombinedPr", 3.f, "Maximum combined proton n-sigma from TPC and TOF"};
    Configurable<float> maxNsigmaTpcPi{"maxNsigmaTpcPi", 2.f, "Maximum pion n-sigma in TPC for proton rejection"};
    Configurable<float> maxNsigmaTpcKa{"maxNsigmaTpcKa", 2.f, "Maximum kaon n-sigma in TPC for proton rejection"};
    Configurable<float> maxNsigmaTpcPr{"maxNsigmaTpcPr", 3.f, "Maximum proton n-sigma in TPC"};
    Configurable<bool> forceTOF{"forceTOF", false, "Require TOF PID information for proton selection"};
  } config;
  
  SliceCache cache;
  using TracksWithSelAndPid = soa::Join<aod::TracksWDcaExtra, aod::TrackSelection,
                                        aod::pidTPCFullPi, aod::pidTOFFullPi, aod::pidTPCFullKa,
                                        aod::pidTOFFullKa, aod::pidTPCFullPr, aod::pidTOFFullPr>;
  using SelectedCollisions = soa::Filtered<soa::Join<aod::Collisions, aod::HcSelCollisions>>;
  Filter filterSelectCollisions = (hiddencharmreduced::whyRejectColl == static_cast<o2::hf_evsel::HfCollisionRejectionMask>(0));
  Preslice<aod::TrackAssoc> trackIndicesPerCollision = aod::track_association::collisionId;

  HistogramRegistry registry{"registry"};

  void init(InitContext const&)
  {
    if (config.fillHistograms) {
      const AxisSpec axisPt{360, 0., 36., "#it{p}_{T}^{proton} (GeV/#it{c})"};
      const AxisSpec axisEta{100, -1., 1., "#eta"};
      const AxisSpec axisDca{400, -2., 2., "DCA_{xy} to primary vertex (cm)"};
      const AxisSpec axisNSigma{100, -5., 5., "n#sigma"};

      registry.add("hPzVtx", "Z position of primary vertex for selected tracks;z_{vtx} (cm);entries", {HistType::kTH1D, {AxisSpec{200, -20., 20., "z_{vtx} (cm)"}}});
      registry.add("hPtNoCuts", "All associated tracks;#it{p}_{T}^{track} (GeV/#it{c});entries", {HistType::kTH1D, {axisPt}});
      registry.add("hPtCutsProton", "Selected proton tracks;#it{p}_{T}^{track} (GeV/#it{c});entries", {HistType::kTH1D, {axisPt}});
      registry.add("hEtaCutsProton", "Selected proton tracks;#eta;entries", {HistType::kTH1D, {axisEta}});
      registry.add("hDCAToPrimXYVsPtCutsProton", "Selected proton tracks;#it{p}_{T}^{track} (GeV/#it{c});DCA_{xy} to primary vertex (cm)", {HistType::kTH2D, {axisPt, axisDca}});
      registry.add("hNSigmaTPCProton", "Selected proton tracks;#it{p}_{T}^{track} (GeV/#it{c});n#sigma_{TPC}", {HistType::kTH2D, {axisPt, axisNSigma}});
      registry.add("hNSigmaTOFProton", "Selected proton tracks;#it{p}_{T}^{track} (GeV/#it{c});n#sigma_{TOF}", {HistType::kTH2D, {axisPt, axisNSigma}});
      registry.add("hInvMass", "Invariant mass of selected proton with all other tracks in the event;#it{p}_{T}^{proton} (GeV/#it{c});invariant mass with other tracks (GeV/#it{c}^{2})", {HistType::kTH2D, {axisPt, AxisSpec{100, 2.85, 3.25, "invariant mass with other tracks (GeV/#it{c}^{2})"}}});
    }
  }

  template <typename TTrack>
  bool isSelectedPid(TTrack const& track) const
  {
    const float momForCombinedPid = config.momForCombinedPid.value;
    const float maxNsigmaTpcPr = config.maxNsigmaTpcPr.value;
    const float maxNsigmaTpcPi = config.maxNsigmaTpcPi.value;
    const float maxNsigmaTpcKa = config.maxNsigmaTpcKa.value;
    const float maxNsigmaCombinedPr = config.maxNsigmaCombinedPr.value;
    const float maxNsigmaTofPi = config.maxNsigmaTofPi.value;
    const float maxNsigmaTofKa = config.maxNsigmaTofKa.value;
    const bool forceTOF = config.forceTOF.value;

    const float mom = std::hypot(track.px(), track.py(), track.pz());
    const float nSigmaTPCPr = track.tpcNSigmaPr();
    const float nSigmaTPCPi = track.tpcNSigmaPi();
    const float nSigmaTPCKa = track.tpcNSigmaKa();
    const bool hasTOF = track.hasTOF();
    if (!hasTOF && forceTOF) {
      return false;
    }

    bool isProton = false;
    bool rejectAsPion = false;
    bool rejectAsKaon = false;

    if (mom < momForCombinedPid || !hasTOF) {
      isProton = std::abs(nSigmaTPCPr) < maxNsigmaTpcPr;
      rejectAsPion = std::abs(nSigmaTPCPi) < maxNsigmaTpcPi;
      rejectAsKaon = std::abs(nSigmaTPCKa) < maxNsigmaTpcKa;
    } else {
      const float nSigmaTOFPr = track.tofNSigmaPr();
      const float nSigmaTOFPi = track.tofNSigmaPi();
      const float nSigmaTOFKa = track.tofNSigmaKa();
      isProton = std::hypot(nSigmaTPCPr, nSigmaTOFPr) < maxNsigmaCombinedPr;
      rejectAsPion = std::hypot(nSigmaTPCPi, nSigmaTOFPi) < maxNsigmaTofPi;
      rejectAsKaon = std::hypot(nSigmaTPCKa, nSigmaTOFKa) < maxNsigmaTofKa;
    }
    return isProton && !rejectAsPion && !rejectAsKaon;
  }

  template <typename TTrack>
  bool isSelectedTrack(TTrack const& track) const
  {
    const int tpcNClsFoundMin = config.tpcNClsFoundMin.value;
    const int tpcNClsCrossedRowsMin = config.tpcNClsCrossedRowsMin.value;
    const int itsNClsMin = config.itsNClsMin.value;
    const double etaMaxTrack = config.etaMaxTrack.value;
    const double ptMinTrack = config.ptMinTrack.value;

    if (!track.isGlobalTrackWoDCA()) {
      return false;
    }
    if (track.tpcNClsFound() < tpcNClsFoundMin) {
      return false;
    }
    if (track.tpcNClsCrossedRows() < tpcNClsCrossedRowsMin) {
      return false;
    }
    if (track.itsNCls() < itsNClsMin) {
      return false;
    }
    if (std::abs(track.eta()) > etaMaxTrack) {
      return false;
    }
    if (track.pt() < ptMinTrack) {
      return false;
    }
    if (!isSelectedTrackDca(config.binsPtTrack, config.cutsTrack, track.pt(), track.dcaXY(), track.dcaZ())) {
      return false;
    }
    return isSelectedPid(track);
  }

  void processTracks(SelectedCollisions const& collisions,
                     aod::TrackAssoc const& trackIndices,
                     TracksWithSelAndPid const& tracks)
  {
    hfTrackLite.reserve(tracks.size());

    for (const auto& collision : collisions) { // loop over selected collisions
      const auto thisCollId = collision.globalIndex();
      const auto groupedTrackIds = trackIndices.sliceBy(trackIndicesPerCollision, thisCollId);
      if (config.fillHistograms) {
        registry.fill(HIST("hPzVtx"), collision.posZ());
      }
    
      // vector of selected track indices for this collision
      std::vector<int> selectedTrackIds;
      for (const auto& trackId : groupedTrackIds) { // loop over tracks for this collision
        auto track = trackId.track_as<TracksWithSelAndPid>();
        if (config.fillHistograms) {
          registry.fill(HIST("hPtNoCuts"), track.pt());
        }

        if (config.selectProtons && !isSelectedTrack(track)) { // apply selection cuts (kinematics, track quality, DCA, PID). TODO: template this function to be used for different particle species (pi, K, proton
          continue;
        }
        std::array pVecProton{track.pVector()};
        // fill output table with selected track information
        hfTrackLite(collision.globalIndex(), track.globalIndex(), pVecProton[0], pVecProton[1], pVecProton[2], track.sign(), static_cast<uint8_t>(TrackType::Proton));
        selectedTrackIds.push_back(track.globalIndex());
        
        if (config.fillHistograms) {
          registry.fill(HIST("hPtCutsProton"), track.pt());
          registry.fill(HIST("hEtaCutsProton"), track.eta());
          registry.fill(HIST("hDCAToPrimXYVsPtCutsProton"), track.pt(), track.dcaXY());
          registry.fill(HIST("hNSigmaTPCProton"), track.pt(), track.tpcNSigmaPr());
          if (track.hasTOF()) {
            registry.fill(HIST("hNSigmaTOFProton"), track.pt(), track.tofNSigmaPr());
          }
        }
      }

      // Loop over selected tracks for this collision and fill the output table
      for (const auto& trackId1 : selectedTrackIds) {
        auto track1 = tracks.rawIteratorAt(trackId1);
        std::array pVec1{track1.pVector()};
        for (const auto& trackId2 : selectedTrackIds) {
          if (trackId2 <= trackId1) {
            continue; // avoid double counting and self-pairs
          }
          auto track2 = tracks.rawIteratorAt(trackId2);
          // reject same-sign pairs
          if (track1.sign() * track2.sign() > 0) {
            continue;
          }
          std::array pVec2{track2.pVector()};
          float invMass = RecoDecay::m(std::array{pVec1, pVec2}, std::array{o2::constants::physics::MassProton, o2::constants::physics::MassProton});
          float ptEtac = RecoDecay::pt(RecoDecay::sumOfVec(pVec1, pVec2));
          registry.fill(HIST("hInvMass"), ptEtac, invMass);
        }
      }
    }
  }
  PROCESS_SWITCH(HfHiddenCharmSelTracks, processTracks, "Select tracks for hidden-charm analyses", true);
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  WorkflowSpec workflow{};
  workflow.push_back(adaptAnalysisTask<HfHiddenCharmSelCollisions>(cfgc));
  workflow.push_back(adaptAnalysisTask<HfHiddenCharmSelTracks>(cfgc));
  return workflow;
}
