#include <vector>
#include "Framework/WorkflowSpec.h"
#include "Framework/ConfigParamSpec.h"
#include "Framework/ControlService.h"

// we need to add workflow options before including Framework/runDataProcessing
void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  std::vector<o2::framework::ConfigParamSpec> options{
    { "n", o2::framework::VariantType::Int, 1, { "number of channels" } },
    { "r", o2::framework::VariantType::Int, 10, { "number of rolls" } }
  };
  std::swap(workflowOptions, options);
}

#include "Framework/runDataProcessing.h" // the main driver

using namespace o2::framework;

WorkflowSpec defineDataProcessing(ConfigContext const& configcontext)
{
  auto nChannels = configcontext.options().get<int>("n");
  auto nRolls = configcontext.options().get<int>("r");

  std::vector<OutputSpec> oSpecs;
  std::vector<InputSpec> iSpecs;
  for (size_t i = 0 ; i < nChannels; i++) {
    oSpecs.emplace_back( "TST", "DATA", i, Lifetime::Timeframe );
    std::string iName = "input" + std::to_string(i);
    iSpecs.push_back( {iName.c_str(), "TST", "DATA", i, Lifetime::Timeframe} );
  }

  auto sender = [nChannels, nRolls](ProcessingContext ctx) {
    static int rollCount = 0;
    if (++rollCount > nRolls) {
      //LOG(INFO) << "ignoring after " << rollCount;
      ctx.services().get<ControlService>().readyToQuit(false);
      return;
    }
    
    for (size_t i = 0; i < nChannels; i++) {
      auto& val = ctx.outputs().make<int>(Output{ "TST", "DATA", i });
      val = i;
    }
  };

  auto receiver = [nChannels, nRolls](ProcessingContext ctx) {
    static int rollCount = 0;
    if (rollCount > nRolls) {
      //LOG(INFO) << "at " << rollCount;
      ctx.services().get<ControlService>().readyToQuit(true);
      return;
    }
      //LOG(INFO) << "processing channels";
    for (int i = 0; i < nChannels; i++) {
      std::string iName = "input" + std::to_string(i);
      auto val = ctx.inputs().get<int>(iName.c_str());
      val = i;
    }

    if (++rollCount >= nRolls) {
      LOG(INFO) << "processed " << rollCount;
      ctx.services().get<ControlService>().readyToQuit(true);
    }
  };

  WorkflowSpec specs{
    { "source", {}, oSpecs, AlgorithmSpec{sender} },
    { "sink", iSpecs, {}, AlgorithmSpec{receiver} }
  };

  return std::move(specs);
}
