//****************************************************************************
//* This file is free software: you can redistribute it and/or modify        *
//* it under the terms of the GNU General Public License as published by     *
//* the Free Software Foundation, either version 3 of the License, or        *
//* (at your option) any later version.                                      *
//*                                                                          *
//* Primary Authors: Matthias Richter <richterm@scieq.net>                   *
//*                                                                          *
//* The authors make no claims about the suitability of this software for    *
//* any purpose. It is provided "as is" without express or implied warranty. *
//****************************************************************************

//  @file   DataSampler.cxx
//  @author Matthias Richter
//  @since  2016-08-24
//  @brief  Sampler device for test of the in-memory format

#include "DataSampler.h"
#include "FairMQParts.h"
#include <chrono>
#include <thread>
#include <memory>
#include <string>

AliceO2::Examples::MsgFormat::DataSampler::DataSampler(int rate)
  : FairMQDevice()
  , mPeriod(rate)
{
}

AliceO2::Examples::MsgFormat::DataSampler::~DataSampler()
{
}

void AliceO2::Examples::MsgFormat::DataSampler::Run()
{
  std::string channelId;
  for ( auto channelIt : fChannels ) {
    channelId = channelIt.first;
    break;
  }
  while (CheckCurrentState(RUNNING)) {
    std::this_thread::sleep_for (std::chrono::microseconds(mPeriod));

    std::string txtMessage("A Message");
    // Create output message
    std::unique_ptr<FairMQMessage> msg(NewMessage(txtMessage.length()+1));
    memcpy(msg->GetData(), txtMessage.c_str(), msg->GetSize());
    FairMQParts parts;
    parts.AddPart(msg);

    // Send out the output message
    Send(parts, channelId);
  }
}
