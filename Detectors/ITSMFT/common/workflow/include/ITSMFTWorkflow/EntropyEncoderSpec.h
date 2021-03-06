// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   EntropyEncoderSpec.h
/// @brief  Convert clusters streams to CTF (EncodedBlocks)

#ifndef O2_ITSMFT_ENTROPYENCODER_SPEC
#define O2_ITSMFT_ENTROPYENCODER_SPEC

#include "Framework/DataProcessorSpec.h"
#include "Framework/Task.h"
#include "Headers/DataHeader.h"
#include <TStopwatch.h>

namespace o2
{
namespace itsmft
{

class EntropyEncoderSpec : public o2::framework::Task
{
 public:
  EntropyEncoderSpec(o2::header::DataOrigin orig);
  ~EntropyEncoderSpec() override = default;
  void run(o2::framework::ProcessingContext& pc) final;
  void endOfStream(o2::framework::EndOfStreamContext& ec) final;

 private:
  o2::header::DataOrigin mOrigin = o2::header::gDataOriginInvalid;
  TStopwatch mTimer;
};

/// create a processor spec
framework::DataProcessorSpec getEntropyEncoderSpec(o2::header::DataOrigin orig);

} // namespace itsmft
} // namespace o2

#endif
