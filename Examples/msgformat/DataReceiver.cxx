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

//  @file   DataReceiver.cxx
//  @author Matthias Richter
//  @since  2016-08-24
//  @brief  Receiver device for test of the in-memory format

#include "DataReceiver.h"
#include <chrono>
#include <thread>
#include <memory>

AliceO2::Examples::MsgFormat::DataReceiver::DataReceiver()
  : FairMQDevice()
  , mPeriod(100000)
{
}

AliceO2::Examples::MsgFormat::DataReceiver::~DataReceiver()
{
}

void AliceO2::Examples::MsgFormat::DataReceiver::Run()
{
  std::unique_ptr<FairMQPoller> poller(fTransportFactory->CreatePoller(fChannels, { mgControlChannelId, mgDataChannelId }));

  while (CheckCurrentState(RUNNING)) {
    poller->Poll(-1);
    //LOG(INFO) << "----------------------------------------------------------------";

    bool haveData = false;
    bool haveControl = false;
    if (poller->CheckInput(mgControlChannelId, 0)) {
      std::unique_ptr<FairMQMessage> msg(NewMessage());

      if (Receive(msg, mgControlChannelId) > 0) {
        //LOG(INFO) << "Received control: \"" << std::string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
	haveData = true;
      }
    }

    if (poller->CheckInput(mgDataChannelId, 0)) {
      std::unique_ptr<FairMQMessage> msg(NewMessage());

      if (Receive(msg, mgDataChannelId) > 0) {
        //LOG(INFO) << "Received data: \"" << std::string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
	haveControl = true;
      }
    }
    if (haveData && haveControl) {
      LOG(INFO) << "have both";
    }
  }
}
