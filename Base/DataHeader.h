//The main O2 data header, v0.1
//origin: CWG4
//blame: Mikolaj Krzewicki, mkrzewic@cern.ch
//       Matthias Richter, Matthias.Richter@cern.ch
//       David Rohr, drohr@cern.ch

#ifndef ALICEO2_BASE_DATA_HEADER_
#define ALICEO2_BASE_DATA_HEADER_

#include <cstdint>

namespace AliceO2 {
namespace Base {

//declare the length of the string fields
//TODO: maybe put them as static members of header
const int gDataHeaderLengthMagicString = 4;
const int gDataHeaderLengthDataOriginString = 4;
const int gDataHeaderLengthDataDescriptionString = 16;
const int gDataHeaderLengthPayloadSerializationString = 8;

//the main header struct
struct DataHeader
{
  //a magic string
  union {
    char     magicString[gDataHeaderLengthMagicString];
    int32_t  magicStringInt;
  };
  
  //origin of the data (originating detector)
  union {
    char     dataOrigin[gDataHeaderLengthDataOriginString];
    int32_t  dataOriginInt;
  };

  //data type descriptor
  union {
    char     dataDescription[gDataHeaderLengthDataDescriptionString];
    int64_t  dataDescriptionInt[2];
  };

  //serialization method
  union {
    char     payloadSerialization[gDataHeaderLengthPayloadSerializationString];
    int64_t  payloadSerializationInt;
  };
  
  //sub specification (e.g. link number)
  int64_t    subSpecification;

  //flags, first bit indicates that a sub header follows
  int32_t    flags;

  int32_t    headerVersion;  //version of this header
  int32_t    headerSize;     //size of this header
  int32_t    payloadSize;    //size of the associated data
  //_______________________________________________________
  //___NEVER MODIFY THE ABOVE______________________________
  //___NEW STUFF GOES BELOW________________________________

  //_______________________________________________________
  //___the functions:
};

} //namespace Base
} //namespace AliceO2

#endif
