//-*- Mode: C++ -*-

#ifndef DATASAMPLER_H
#define DATASAMPLER_H
//****************************************************************************
//* This file is free software: you can redistribute it and/or modify        *
//* it under the terms of the GNU General Public License as published by     *
//* the Free Software Foundation, either version 3 of the License, or	     *
//* (at your option) any later version.					     *
//*                                                                          *
//* Primary Authors: Matthias Richter <richterm@scieq.net>                   *
//*                                                                          *
//* The authors make no claims about the suitability of this software for    *
//* any purpose. It is provided "as is" without express or implied warranty. *
//****************************************************************************

//  @file   DataSampler.h
//  @author Matthias Richter
//  @since  2016-08-24
//  @brief  Sampler device for test of the in-memory format

#include "FairMQDevice.h"

namespace AliceO2 {
namespace Examples {
namespace MsgFormat {

class DataSampler : public FairMQDevice {
  public:
    DataSampler();
    virtual ~DataSampler();

  protected:
    virtual void Run();

  private:
    int mPeriod;
};

};// namespace MsgFormat
};// namespace Examples 
};// namespace AliceO2
#endif
