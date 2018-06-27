#include <vector>
#include <iomanip>
#include <memory>
#include "Framework/WorkflowSpec.h"
#include "Framework/ConfigParamSpec.h"
#include "Framework/ControlService.h"
#include "DataFormatsTPC/ClusterHardware.h"
#include "DataFormatsTPC/Helpers.h"
#include "TPCReconstruction/HardwareClusterDecoder.h"

// we need to add workflow options before including Framework/runDataProcessing
void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  std::vector<o2::framework::ConfigParamSpec> options{
    { "channels", o2::framework::VariantType::Int, 1, { "number of channels" } },
    { "pages", o2::framework::VariantType::Int, 128, { "number of pages" } },
    { "rolls", o2::framework::VariantType::Int, 10, { "number of rolls" } }
  };
  std::swap(workflowOptions, options);
}

#include "Framework/runDataProcessing.h" // the main driver

using namespace o2::framework;

WorkflowSpec defineDataProcessing(ConfigContext const& configcontext)
{
  auto nChannels = configcontext.options().get<int>("channels");
  auto nRolls = configcontext.options().get<int>("rolls");
  auto nPages = configcontext.options().get<int>("pages");

  std::vector<OutputSpec> oSpecs;
  std::vector<InputSpec> iSpecs;
  for (size_t i = 0 ; i < nChannels; i++) {
    oSpecs.emplace_back( "TST", "DATA", i, Lifetime::Timeframe );
    std::string iName = "input" + std::to_string(i);
    iSpecs.push_back( {iName.c_str(), "TST", "DATA", i, Lifetime::Timeframe} );
  }

  auto sender = [nChannels, nRolls, nPages](ProcessingContext ctx) {
    static int rollCount = 0;
    if (++rollCount > nRolls) {
      //LOG(INFO) << "ignoring after " << rollCount;
      ctx.services().get<ControlService>().readyToQuit(false);
      return;
    }

    for (size_t i = 0; i < nChannels; i++) {
      auto outputPages = ctx.outputs().make<o2::TPC::ClusterHardwareContainer8kb>(Output{ "TST", "DATA", i }, nPages);
      for (auto& outputPage : outputPages) {
	o2::TPC::ClusterHardwareContainer* clusterContainer = outputPage.getContainer();
	clusterContainer->numberOfClusters = outputPage.getMaxNumberOfClusters();
      }
    }
  };

  auto receiver = [nChannels, nRolls](InitContext ic) {
    std::shared_ptr<o2::TPC::HardwareClusterDecoder> decoder;
    auto runDecoder = ic.options().get<int>("decoder");
    if (runDecoder) {
      decoder = std::make_shared<o2::TPC::HardwareClusterDecoder>();
    }

    return [nChannels, nRolls, decoder](ProcessingContext ctx) {
    static int rollCount = 0;
    if (rollCount > nRolls) {
      //LOG(INFO) << "at " << rollCount;
      ctx.services().get<ControlService>().readyToQuit(true);
      return;
    }
      //LOG(INFO) << "processing channels";
    std::vector<std::pair<const o2::TPC::ClusterHardwareContainer*, std::size_t>> inputList;

    for (int channel = 0; channel < nChannels; channel++) {
      std::string iName = "input" + std::to_string(channel);
      auto ref = ctx.inputs().get(iName.c_str());
      auto size = o2::framework::DataRefUtils::getPayloadSize(ref);
      //LOG(INFO) << std::setw(4) << nRolls << " - channel " << std::setw(3) << channel << ": " << size;
      inputList.emplace_back( reinterpret_cast<const o2::TPC::ClusterHardwareContainer*>(ref.payload), size / 8192 );
    }
    std::vector<o2::TPC::ClusterNativeContainer> cont;
    if (decoder) {
      decoder->decodeClusters(inputList, cont);
    }
    //for (const auto& coll : cont) {
    //  LOG(INFO) << std::setw(4) << nRolls << ": container with " << coll.clusters.size() << " cluster(s)";
    //}

    if (++rollCount >= nRolls) {
      LOG(INFO) << "processed " << rollCount;
      ctx.services().get<ControlService>().readyToQuit(true);
    }
  };
  };

  WorkflowSpec specs{
    { "source", {}, oSpecs, AlgorithmSpec{sender} },
    { "sink", iSpecs, {}, AlgorithmSpec{receiver}, { { "decoder", o2::framework::VariantType::Int, 1, { "run decoder" } } } }
  };

  return std::move(specs);
}
