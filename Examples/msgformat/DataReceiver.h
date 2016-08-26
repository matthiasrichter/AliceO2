//-*- Mode: C++ -*-

#ifndef DATARECEIVER_H
#define DATARECEIVER_H
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

//  @file   DataReceiver.h
//  @author Matthias Richter
//  @since  2016-08-24
//  @brief  Receiver device for test of the in-memory format

#include "FairMQDevice.h"

namespace AliceO2 {
namespace Examples {
namespace MsgFormat {

class DataReceiver : public FairMQDevice {
  public:
    DataReceiver();
    virtual ~DataReceiver();

  protected:
    virtual void Run();

  private:
    int mPeriod;

    static constexpr char* mgControlChannelId = "control";
    static constexpr char* mgDataChannelId = "data";
};

};// namespace MsgFormat
};// namespace Examples
};// namespace AliceO2
#endif
