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

#ifndef PWGHF_D2H_DATAMODEL_HIDDENCHARMTABLES_H_
#define PWGHF_D2H_DATAMODEL_HIDDENCHARMTABLES_H_

#include "PWGHF/DataModel/TrackIndexSkimmingTables.h"

#include <Framework/ASoA.h>

namespace o2::aod
{
    namespace hiddencharmreduced
    {
        DECLARE_SOA_COLUMN(CollisionId, collisionId, int); //! Collision index
        DECLARE_SOA_COLUMN(WhyRejectColl, whyRejectColl, o2::hf_evsel::HfCollisionRejectionMask); //! Reason for collision rejection
        DECLARE_SOA_COLUMN(TrackId, trackId, int);                //! Original track index
        DECLARE_SOA_COLUMN(Px, px, float);                        //! x-component of momentum
        DECLARE_SOA_COLUMN(Py, py, float);                        //! y-component of momentum
        DECLARE_SOA_COLUMN(Pz, pz, float);                        //! z-component of momentum
        DECLARE_SOA_COLUMN(Sign, sign, int8_t);                   //! charge sign
        DECLARE_SOA_COLUMN(TrackType, tracktype, uint8_t);             //! particle type according to PID selection (pion, kaon, proton)
    } // namespace hiddencharmreduced
    DECLARE_SOA_TABLE(HcSelCollisions, "AOD", "HCSELCOLLISION",
        hiddencharmreduced::CollisionId,
        hiddencharmreduced::WhyRejectColl);
    DECLARE_SOA_TABLE(HcSelTracks, "AOD", "HCSELTRACKS", //!
        hiddencharmreduced::CollisionId,
        hiddencharmreduced::TrackId,
        hiddencharmreduced::Px,
        hiddencharmreduced::Py,
        hiddencharmreduced::Pz,
        hiddencharmreduced::Sign,
        hiddencharmreduced::TrackType );

} // namespace o2::aod

#endif // PWGHF_D2H_DATAMODEL_HIDDENCHARMTABLES_H_