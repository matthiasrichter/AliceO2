// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Digitizer.cxx
/// \brief Implementation of the ALICE TPC digitizer task
/// \author Andi Mathis, TU München, andreas.mathis@ph.tum.de

#include "TFile.h"
#include "TTree.h"
#include "TRandom.h"

#include "TPCSimulation/DigitizerTask.h"
#include "TPCSimulation/DigitContainer.h"
#include "TPCSimulation/Digitizer.h"
#include "TPCSimulation/SAMPAProcessing.h"
#include "TPCSimulation/Point.h"
#include "TPCBase/Sector.h"
#include "TPCBase/Digit.h"
#include "TPCSimulation/DigitMCMetaData.h"

#include "FairLogger.h"
#include "FairRootManager.h"

#include <sstream>
//#include "valgrind/callgrind.h"

ClassImp(o2::TPC::DigitizerTask)

using namespace o2::TPC;


DigitizerTask::DigitizerTask(int sectorid)
  : FairTask("TPCDigitizerTask"),
    mDigitizer(nullptr),
    mDigitContainer(nullptr),
    mDigitsArray(nullptr),
    mMCTruthArray(nullptr),
    mDigitsDebugArray(nullptr),
    mTimeBinMax(1000000),
    mIsContinuousReadout(true),
    mDigitDebugOutput(false),
    mHitSector(sectorid)
{
  /// \todo get rid of new
  mDigitizer = new Digitizer;
  //CALLGRIND_START_INSTRUMENTATION;
}

DigitizerTask::~DigitizerTask()
{
  delete mDigitizer;
  delete mDigitsArray;
  delete mDigitsDebugArray;
  delete mMCTruthArray;

  //CALLGRIND_STOP_INSTRUMENTATION;
  //CALLGRIND_DUMP_STATS;
}


InitStatus DigitizerTask::Init()
{
  /// Initialize the task and the input and output containers
  FairRootManager *mgr = FairRootManager::Instance();
  if(!mgr){
    LOG(ERROR) << "Could not instantiate FairRootManager. Exiting ..." << FairLogger::endl;
    return kERROR;
  }

  std::stringstream sectornamestrleft, sectornamestrright;
  sectornamestrleft << "TPCHitsSectorShifted" << int(Sector::getLeft(Sector(mHitSector)));
  sectornamestrright << "TPCHitsSectorShifted" << mHitSector;
  mSectorHitsArrayLeft = mgr->InitObjectAs<const std::vector<HitGroup>*>(sectornamestrleft.str().c_str());
  mSectorHitsArrayRight = mgr->InitObjectAs<const std::vector<HitGroup>*>(sectornamestrright.str().c_str());

  // Register output container
  mDigitsArray = new std::vector<o2::TPC::Digit>;
  mgr->RegisterAny(Form("TPCDigit_%i", mHitSector), mDigitsArray, kTRUE);

  // Register MC Truth container
  mMCTruthArray = new typename std::remove_pointer<decltype(mMCTruthArray)>::type;
  mgr->RegisterAny(Form("TPCDigitMCTruth_%i", mHitSector), mMCTruthArray, kTRUE);

  // Register additional (optional) debug output
  if(mDigitDebugOutput) {
    mDigitsDebugArray = new std::vector<o2::TPC::DigitMCMetaData>;
    mgr->RegisterAny(Form("TPCDigitMCMetaData_%i", mHitSector), mDigitsDebugArray, kTRUE);
  }
  
  mDigitizer->init();
  mDigitContainer = mDigitizer->getDigitContainer();
  return kSUCCESS;
}


InitStatus DigitizerTask::Init2()
{
  mDigitizer->init();
  mDigitContainer = mDigitizer->getDigitContainer();
  return kSUCCESS;
}

void DigitizerTask::Exec(Option_t *option)
{
  FairRootManager *mgr = FairRootManager::Instance();

  // time should be given in us
  float eventTime = static_cast<float>(mgr->GetEventTime() * 0.001);
  if (mEventTimes.size()) {
    eventTime = mEventTimes[mCurrentEvent++];
    LOG(DEBUG) << "Event time taken from bunch simulation";
  }
  const int eventTimeBin = SAMPAProcessing::getTimeBinFromTime(eventTime);

  LOG(DEBUG) << "Running digitization for sector " << mHitSector << "on new event at time " << eventTime << " us in time bin " << eventTimeBin << FairLogger::endl;
  mDigitsArray->clear();
  mMCTruthArray->clear();
  if(mDigitDebugOutput) {
    mDigitsDebugArray->clear();
  }

  auto sec = Sector(mHitSector);
  mDigitContainer->setup(sec, eventTimeBin);
  mDigitContainer = mDigitizer->Process(sec, *mSectorHitsArrayLeft, mgr->GetEntryNr(), eventTime);
  mDigitContainer = mDigitizer->Process(sec, *mSectorHitsArrayRight, mgr->GetEntryNr(), eventTime);
  mDigitContainer->fillOutputContainer(mDigitsArray, *mMCTruthArray, mDigitsDebugArray, eventTimeBin, mIsContinuousReadout);
}

void DigitizerTask::Exec2(Option_t *option)
{
  mDigitsArray->clear();
  mMCTruthArray->clear();
  if(mDigitDebugOutput) {
    mDigitsDebugArray->clear();
  }

  const auto sec = Sector(mHitSector);
  const int startTimeBin = SAMPAProcessing::getTimeBinFromTime(mStartTime * 0.001f);
  const int endTimeBin = SAMPAProcessing::getTimeBinFromTime(mEndTime * 0.001f);
  std::cout << sec << " " << startTimeBin << " " << endTimeBin << "\n";
  mDigitContainer->setup(sec, startTimeBin);
  mDigitContainer = mDigitizer->ProcessNEW(sec, *mAllSectorHitsLeft, *mHitIdsLeft, *mRunContext);
  mDigitContainer = mDigitizer->ProcessNEW(sec, *mAllSectorHitsRight, *mHitIdsRight, *mRunContext);
  mDigitContainer->fillOutputContainer(mDigitsArray, *mMCTruthArray, mDigitsDebugArray, startTimeBin, mIsContinuousReadout);
}

void DigitizerTask::initBunchTrainStructure(const size_t numberOfEvents)
{
  LOG(DEBUG) << "Initialising bunch train structure for " << numberOfEvents << " evnets" << FairLogger::endl;
  // Parameters for bunches
  const double abortGap = 3e-6; //
  const double collFreq = 50e3;
  const double bSpacing = 50e-9; //bunch spacing
  const int nTrainBunches = 48;
  const int nTrains = 12;
  const double revFreq = 1.11e4; //revolution frequency
  const double collProb = collFreq/(nTrainBunches*nTrains*revFreq);
  const double trainLength = bSpacing*(nTrainBunches-1);
  const double totTrainLength = nTrains*trainLength;
  const double trainSpacing = (1./revFreq - abortGap - totTrainLength)/(nTrains-1); 

  // counters
  double eventTime=0.; // all in seconds
  size_t nGeneratedEvents = 0;
  size_t bunchCounter = 0;
  size_t trainCounter = 0;

  // reset vector
  mEventTimes.clear();

  while (nGeneratedEvents < numberOfEvents+2){
    //  std::cout <<trainCounter << " " << bunchCounter << " "<< "eventTime " << eventTime << std::endl;

    int nCollsInCrossing = gRandom -> Poisson(collProb);
    for(int iColl = 0; iColl<nCollsInCrossing; iColl++){
      //printf("Generating event %3d (%.3g)\n",nGeneratedEvents,eventTime);
      mEventTimes.emplace_back(static_cast<float>(eventTime*1e6)); // convert to us
      nGeneratedEvents++;
    }
    bunchCounter++;

    if(bunchCounter>=nTrainBunches){

      trainCounter++;
      if(trainCounter>=nTrains){
        eventTime+=abortGap;
        trainCounter=0;
      }
      else eventTime+=trainSpacing;

      bunchCounter=0;
    }
    else eventTime+= bSpacing;

  }
}
