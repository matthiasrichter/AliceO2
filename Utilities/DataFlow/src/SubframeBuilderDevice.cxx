/// @file   SubframeBuilderDevice.cxx
/// @author Giulio Eulisse, Matthias Richter, Sandro Wenzel
/// @since  2017-02-07
/// @brief  Demonstrator device for a subframe builder

#include <thread> // this_thread::sleep_for
#include <chrono>
#include <functional>

#include "DataFlow/SubframeBuilderDevice.h"
#include "Headers/SubframeMetadata.h"
#include "Headers/HeartbeatFrame.h"
#include "Headers/DataHeader.h"
#include <options/FairMQProgOptions.h>

// From C++11 on, constexpr static data members are implicitly inlined. Redeclaration
// is still permitted, but deprecated. Some compilers do not implement this standard
// correctly. It also has to be noticed that this error does not occur for all the
// other public constexpr members
constexpr uint32_t o2::DataFlow::SubframeBuilderDevice::mOrbitsPerTimeframe;
constexpr uint32_t o2::DataFlow::SubframeBuilderDevice::mOrbitDuration;
constexpr uint32_t o2::DataFlow::SubframeBuilderDevice::mDuration;

using HeartbeatHeader = o2::Header::HeartbeatHeader;
using HeartbeatTrailer = o2::Header::HeartbeatTrailer;
using DataHeader = o2::Header::DataHeader;

o2::DataFlow::SubframeBuilderDevice::SubframeBuilderDevice()
  : O2Device()
{
  LOG(INFO) << "o2::DataFlow::SubframeBuilderDevice::SubframeBuilderDevice " << mDuration << "\n";
}

o2::DataFlow::SubframeBuilderDevice::~SubframeBuilderDevice()
= default;

void o2::DataFlow::SubframeBuilderDevice::InitTask()
{
//  mDuration = GetConfig()->GetValue<uint32_t>(OptionKeyDuration);
  mIsSelfTriggered = GetConfig()->GetValue<bool>(OptionKeySelfTriggered);
  mInputChannelName = GetConfig()->GetValue<std::string>(OptionKeyInputChannelName);
  mOutputChannelName = GetConfig()->GetValue<std::string>(OptionKeyOutputChannelName);
  mFLPId= GetConfig()->GetValue<size_t>(OptionKeyFLPId);
  mStripHBF= GetConfig()->GetValue<bool>(OptionKeyStripHBF);

  if (!mIsSelfTriggered) {
    // depending on whether the device is self-triggered or expects input,
    // the handler function needs to be registered or not.
    // ConditionalRun is not called anymore from the base class if the
    // callback is registered
    LOG(INFO) << "Obtaining data from DataPublisher\n";
    OnData(mInputChannelName.c_str(), &o2::DataFlow::SubframeBuilderDevice::HandleData);
  } else {
    LOG(INFO) << "Self triggered mode. Doing nothing for now.\n";
  }
  LOG(INFO) << "o2::DataFlow::SubframeBuilderDevice::InitTask " << mDuration << "\n";
}

// FIXME: how do we actually find out the payload size???
int64_t extractDetectorPayloadStrip(char **payload, char *buffer, size_t bufferSize) {
  *payload = buffer + sizeof(HeartbeatHeader);
  return bufferSize - sizeof(HeartbeatHeader) - sizeof(HeartbeatTrailer);
}

int64_t extractDetectorPayload(char **payload, char *buffer, size_t bufferSize) {
  *payload = buffer;
  return bufferSize;
}

bool o2::DataFlow::SubframeBuilderDevice::BuildAndSendFrame(FairMQParts &inParts)
{
  LOG(INFO) << "o2::DataFlow::SubframeBuilderDevice::BuildAndSendFrame" << mDuration << "\n";
  char *incomingBuffer = (char *)inParts.At(1)->GetData();
  HeartbeatHeader *hbh = reinterpret_cast<HeartbeatHeader*>(incomingBuffer);

  // top level subframe header, the DataHeader is going to be used with
  // description "SUBTIMEFRAMEMD"
  // this should be defined in a common place, and also the origin
  // the origin can probably name a detector identifier, but not sure if
  // all CRUs of a FLP in all cases serve a single detector
  o2::Header::DataHeader dh;
  dh.dataDescription = o2::Header::DataDescription("SUBTIMEFRAMEMD");
  dh.dataOrigin = o2::Header::DataOrigin("FLP");
  dh.subSpecification = mFLPId;
  dh.payloadSize = sizeof(SubframeMetadata);

  DataHeader payloadheader(*o2::Header::get<DataHeader>((byte*)inParts.At(0)->GetData()));

  // subframe meta information as payload
  SubframeMetadata md;
  // md.startTime = (hbh->orbit / mOrbitsPerTimeframe) * mDuration;
  md.startTime = static_cast<uint64_t>(hbh->orbit) * static_cast<uint64_t>(mOrbitDuration);
  md.duration = mDuration;
  LOG(INFO) << "Start time for subframe (" << hbh->orbit << ", "
                                           << mDuration
            << ") " << timeframeIdFromTimestamp(md.startTime, mDuration) << " " << md.startTime<< "\n";
  // Extract the payload from the HBF
  char *sourcePayload = nullptr;
  int64_t payloadSize = 0;
  if (mStripHBF) {
    payloadSize = extractDetectorPayloadStrip(&sourcePayload,
                                              incomingBuffer,
                                              inParts.At(1)->GetSize());
  } else {
    sourcePayload = incomingBuffer;
    payloadSize = inParts.At(1)->GetSize();
  }

  if (payloadSize <= 0)
  {
    LOG(ERROR) << "Payload is too small: " << payloadSize << "\n";
    return true;
  }
  else
  {
    LOG(INFO) << "Payload of size " << payloadSize << "received\n";
  }
  SubframeId sfId = {.timeframeId = hbh->orbit % mOrbitsPerTimeframe,
                     .socketId = 0 };
  auto ptr = std::make_unique<char>(new char[payloadSize]);
  memcpy(ptr.get(), sourcePayload, payloadSize);
  SubframeRef sfRef = {.ptr = std::move(ptr),
                       .size = payloadSize};
  mHBFmap.emplace(std::make_pair(sfId, std::move(sfRef)));
  if (mHBFmap.count(sfId) < mOrbitsPerTimeframe)
    return true;
  // If we are here, it means we can send the Subtimeframe as we have
  // enough elements for the given SubframeId
  // - Calculate the aggregate size of all the HBF payloads.
  // - Copy all the HBF into payload
  // - Create the header part
  // - Create the payload part
  // - Send
  size_t sum = 0;
  auto range = mHBFmap.equal_range(sfId);
  for (auto hi = range.first, he = range.second; hi != he; ++hi) {
    sum += hi->second.size;
  }

  auto *payload = new char[sum]();
  size_t offset = 0;
  for (auto hi = range.first, he = range.second; hi != he; ++hi) {
    memcpy(payload + offset, hi->second.ptr.get(), hi->second.size);
    offset += hi->second.size;
  }
  payloadheader.payloadSize = sum;

  O2Message outgoing;
  AddMessage(outgoing, dh, NewSimpleMessage(md));

  AddMessage(outgoing, payloadheader,
             NewMessage(payload, sum,
                        [](void* data, void* hint) { delete[] reinterpret_cast<char *>(hint); }, payload));
  // send message
  Send(outgoing, mOutputChannelName.c_str());
  outgoing.fParts.clear();
  mHBFmap.erase(sfId);

  return true;
}

bool o2::DataFlow::SubframeBuilderDevice::HandleData(FairMQParts& msgParts, int /*index*/)
{
  // loop over header payload pairs in the incoming multimessage
  // for each pair
  // - check timestamp
  // - create new subtimeframe if none existing where the timestamp of the data fits
  // - add pair to the corresponding subtimeframe

  // check for completed subtimeframes and send all completed frames
  // the builder does not implement the routing to the EPN, this is done in the
  // specific FLP-EPN setup
  // to fit into the simple emulation of event/frame ids in the flpSender the order of
  // subtimeframes needs to be preserved
  BuildAndSendFrame(msgParts);
  return true;
}
