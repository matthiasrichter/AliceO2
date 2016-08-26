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

//  @file   runDataReceiver.cxx
//  @author Matthias Richter
//  @since  2016-08-24
//  @brief  Launcher for Receiver device for test of the in-memory format

// compile everything in the exe
#include "DataReceiver.cxx"

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "runSimpleMQStateMachine.h"

int main(int argc, char** argv)
{
    try
    {
        FairMQProgOptions config;
        config.ParseAll(argc, argv);

        AliceO2::Examples::MsgFormat::DataReceiver device;
        device.SetProperty(FairMQDevice::LogIntervalInMs, 10000);
        runStateMachine(device, config);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled Exception reached the top of main: "
                   << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}
