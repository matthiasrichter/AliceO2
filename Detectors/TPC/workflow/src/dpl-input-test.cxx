#include <vector>
#include <iomanip>
#include <memory>
#include <chrono>
#include "Headers/DataHeader.h"
#include "Framework/WorkflowSpec.h"
#include "Framework/ConfigParamSpec.h"
#include "Framework/ControlService.h"
#include "DataFormatsTPC/ClusterHardware.h"
#include "DataFormatsTPC/Helpers.h"
#include "TPCReconstruction/HardwareClusterDecoder.h"
#include <TFile.h>
#include <TTree.h>

// we need to add workflow options before including Framework/runDataProcessing
void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  std::vector<o2::framework::ConfigParamSpec> options{
    { "senders", o2::framework::VariantType::Int, 1, { "number of senders" } },
    { "channels", o2::framework::VariantType::Int, 1, { "number of channels" } },
    { "pages", o2::framework::VariantType::Int, 128, { "number of pages" } },
    { "rolls", o2::framework::VariantType::Int, 10, { "number of rolls" } }
  };
  std::swap(workflowOptions, options);
}

#include "Framework/runDataProcessing.h" // the main driver

using namespace o2::framework;
using TimeScale = std::chrono::microseconds;

WorkflowSpec defineDataProcessing(ConfigContext const& configcontext)
{
  auto nSenders = configcontext.options().get<int>("senders");
  auto nChannels = configcontext.options().get<int>("channels");
  auto nRolls = configcontext.options().get<int>("rolls");
  auto nPages = configcontext.options().get<int>("pages");

  if (nSenders > 16) {
    LOG(ERROR) << "no more than 16 senders supported";
    return WorkflowSpec{};
  }

  auto makeName = [] (size_t s, size_t c) {
    std::string name = "input" + std::to_string(s) + "_" + std::to_string(c);
    return std::move(name);
  };

  std::vector<std::vector<OutputSpec>> oSpecs;
  std::vector<InputSpec> iSpecs;
  for (size_t sn = 0 ; sn < nSenders; sn++) {
    oSpecs.emplace_back();
    for (size_t ch = 0 ; ch < nChannels; ch++) {
      o2::header::DataHeader::SubSpecificationType sspec = sn << 16 | ch;
      oSpecs.back().emplace_back( "TST" , "DATA", sspec, Lifetime::Timeframe );
      std::string iName = makeName(sn, ch);
      iSpecs.push_back( {iName.c_str(), "TST" , "DATA", sspec, Lifetime::Timeframe} );
    }
  }

  auto sender = [nChannels, nRolls, nPages](size_t sn) {
    return [nChannels, nRolls, nPages, sn](ProcessingContext ctx) {
    static int rollCount = 0;
    if (++rollCount > nRolls) {
      //LOG(INFO) << "ignoring after " << rollCount;
      ctx.services().get<ControlService>().readyToQuit(false);
      return;
    }

    for (size_t i = 0; i < nChannels; i++) {
      o2::header::DataHeader::SubSpecificationType sspec = sn << 16 | i;
      auto outputPages = ctx.outputs().make<o2::TPC::ClusterHardwareContainer8kb>(Output{ "TST", "DATA", sspec }, nPages);
      for (auto& outputPage : outputPages) {
	o2::TPC::ClusterHardwareContainer* clusterContainer = outputPage.getContainer();
	clusterContainer->numberOfClusters = outputPage.getMaxNumberOfClusters();
      }
    }
  };
  };

  auto receiver = [nChannels, nRolls, nPages, nSenders](InitContext ic) {
    std::shared_ptr<o2::TPC::HardwareClusterDecoder> decoder;
    auto runDecoder = ic.options().get<int>("decoder");
    if (runDecoder) {
      decoder = std::make_shared<o2::TPC::HardwareClusterDecoder>();
    }
    auto metrics = std::make_shared<std::vector<std::pair<int, int>>>();
    metrics->resize(nRolls * 5);
    metrics->clear();
    auto reftime = std::chrono::system_clock::now();

    return [nSenders, nChannels, nRolls, nPages, decoder, reftime, metrics](ProcessingContext ctx) {
    int iMetric = 0;
    auto duration = std::chrono::duration_cast<TimeScale>(std::chrono::system_clock::now() - reftime);
    metrics->emplace_back(iMetric++, duration.count());

    static int rollCount = 0;
    if (rollCount > nRolls) {
      //LOG(INFO) << "at " << rollCount;
      ctx.services().get<ControlService>().readyToQuit(true);
      return;
    }
      //LOG(INFO) << "processing channels";
    std::vector<std::pair<const o2::TPC::ClusterHardwareContainer*, std::size_t>> inputList;

    duration = std::chrono::duration_cast<TimeScale>(std::chrono::system_clock::now() - reftime);
    metrics->emplace_back(iMetric++, duration.count());
    for (auto ref : ctx.inputs()) {
      auto size = o2::framework::DataRefUtils::getPayloadSize(ref);
      //LOG(INFO) << std::setw(4) << nRolls << " - channel " << std::setw(3) << channel << ": " << size;
      inputList.emplace_back( reinterpret_cast<const o2::TPC::ClusterHardwareContainer*>(ref.payload), size / 8192 );
    }
    std::vector<o2::TPC::ClusterNativeContainer> cont;
    duration = std::chrono::duration_cast<TimeScale>(std::chrono::system_clock::now() - reftime);
    metrics->emplace_back(iMetric++, duration.count());
    if (decoder) {
      decoder->decodeClusters(inputList, cont);
    }
    //for (const auto& coll : cont) {
    //  LOG(INFO) << std::setw(4) << nRolls << ": container with " << coll.clusters.size() << " cluster(s)";
    //}

    duration = std::chrono::duration_cast<TimeScale>(std::chrono::system_clock::now() - reftime);
    metrics->emplace_back(iMetric++, duration.count());
    if (++rollCount >= nRolls) {
      LOG(INFO) << "processed " << rollCount;
      ctx.services().get<ControlService>().readyToQuit(true);
      int varRolls = nRolls;
      int varSenders = nSenders;
      int varChannels = nChannels;
      int varPages = nPages;
      int varSize = nPages * nChannels * nSenders;
      int varId = -1;
      int varTime = -1;
      int varTimeDiff = 0;
      int varFrameworkTime = 0;
      int set = 0;
      std::string filename = "metrics_" + std::to_string(nRolls) + "_" + std::to_string(nSenders) + "_" + std::to_string(nChannels) + "_" + std::to_string(nPages) + ".root";
      auto file = std::make_unique<TFile>(filename.c_str(), "RECREATE");
      auto tree = std::make_unique<TTree>("metrics", filename.c_str());
      tree->Branch("rolls", &varRolls, "rolls/I");
      tree->Branch("senders", &varChannels, "senders/I");
      tree->Branch("channels", &varChannels, "channels/I");
      tree->Branch("pages", &varPages, "pages/I");
      tree->Branch("size", &varSize, "size/I");
      tree->Branch("id", &varId, "id/I");
      tree->Branch("time", &varTime, "time/I");
      tree->Branch("diff", &varTimeDiff, "diff/I");
      tree->Branch("framework", &varFrameworkTime, "framework/I");
      for (auto & metric : *metrics) {
	varTimeDiff = (varTime > 0 ? metric.second - varTime : 0);
	if (varId >= 0 && varId >= metric.first) {
	  set++;
	  varFrameworkTime = varTimeDiff;
	}
	LOG(INFO) << "Metrics dump (" << nRolls << "/" << nSenders<< "/" << nChannels << "/" << nPages << "): "
		  << std::setw(4) << set << " "
		  << std::setw(2) << metric.first << " "
		  << std::setw(8) << metric.second << " "
		  << std::setw(8) << varTimeDiff << " "
		  << std::setw(8) << varFrameworkTime;
	varTime = metric.second;
	varId = metric.first;
	tree->Fill();
      }
      tree->Write();
      tree.release();
      file->Close();
    }
  };
  };

  WorkflowSpec specs;
  for (size_t sn = 0; sn < nSenders; sn++) {
    std::string name = "source" + std::to_string(sn);
    specs.push_back( { name, {}, oSpecs[sn], AlgorithmSpec{sender(sn)} } );
  }
  specs.push_back( { "sink", iSpecs, {}, AlgorithmSpec{receiver}, { { "decoder", o2::framework::VariantType::Int, 1, { "run decoder" } } } } );

  return std::move(specs);
}
