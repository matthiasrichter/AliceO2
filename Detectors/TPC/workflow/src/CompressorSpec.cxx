// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "TPCWorkflow/CATrackerSpec.h"
#include "Headers/DataHeader.h"
#include "Framework/WorkflowSpec.h" // o2::framework::mergeInputs
#include "Framework/DataRefUtils.h"
#include "Framework/DataSpecUtils.h"
#include "Framework/ControlService.h"
#include "Framework/ConfigParamRegistry.h"
#include "DataFormatsTPC/CompressedClusters.h"
//#include "TPCClusterDecompressor.cxx"
#include <FairMQLogger.h>
#include <memory> // for make_shared
#include <vector>
#include <iomanip>
#include <stdexcept>
#include <TFile.h>
#include <TTree.h>

using namespace o2::framework;
using namespace o2::header;

namespace o2
{
namespace tpc
{

DataProcessorSpec getCompressorSpec()
{
  struct ProcessAttributes {
    int verbosity = 1;
  };

  auto initFunction = [](InitContext& ic) {
    auto options = ic.options().get<std::string>("compressor-options");
    LOG(INFO) << "get compressor options " << options;

    auto processAttributes = std::make_shared<ProcessAttributes>();

    auto processingFct = [processAttributes](ProcessingContext& pc) {
      auto compressed = pc.inputs().get<CompressedClusters*>("input");
      if (compressed == nullptr) {
        LOG(ERROR) << "invalid input";
        return;
      }
      LOG(INFO) << "input data with " << compressed->nTracks << " track(s) and " << compressed->nAttachedClusters << " attached clusters";

      std::unique_ptr<TFile> testFile(TFile::Open("tpc-compressed-clusters.root", "RECREATE"));
      testFile->WriteObject(compressed.get(), "TPCCompressedClusters");
      testFile->Write();
      testFile->Close();

      //o2::tpc::ClusterNativeAccess clustersNativeDecoded; // Cluster native access structure as used by the tracker
      //std::vector<o2::tpc::ClusterNative> clusterBuffer; // std::vector that will hold the actual clusters, clustersNativeDecoded will point inside here
      //mDecoder.decompress(clustersCompressed, clustersNativeDecoded, clusterBuffer, param); // Run decompressor
    };

    return processingFct;
  };

  return DataProcessorSpec{"tpc-compressor", // process id
                           {{"input", "TPC", "COMPCLUSTERS", 0, Lifetime::Timeframe}},
                           {},
                           AlgorithmSpec(initFunction),
                           Options{
                             {"compressor-options", VariantType::String, "", {"Option string passed to tracker"}},
                           }};
}

} // namespace tpc
} // namespace o2
