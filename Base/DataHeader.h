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

//constant field lengths for char fields
const uint32_t gSizeMagicString = 4;
const uint32_t gSizeDataOriginString = 4;
const uint32_t gSizePayloadSerializationString = 8;
const uint32_t gSizeDataDescriptionString = 16;

struct DataHeader;
struct DataOrigin;
struct DataDescription;
struct DataIdentifier;
struct PayloadSerialization;

//____________________________________________________________________________
//the main header struct
struct DataHeader
{
  //other constants
  static const uint32_t sVersion = 1;
  static const char* sMagicString;

  //__the data layout:
  
  //a magic string
  union {
    char     magicString[gSizeMagicString];
    uint32_t  magicStringInt;
  };
  
  //origin of the data (originating detector)
  union {
    char     dataOrigin[gSizeDataOriginString];
    uint32_t  dataOriginInt;
  };

  //serialization method
  union {
    char     payloadSerialization[gSizePayloadSerializationString];
    uint64_t  payloadSerializationInt;
  };
  
  //data type descriptor
  union {
    char     dataDescription[gSizeDataDescriptionString];
    uint64_t  dataDescriptionInt[2];
  };

  //sub specification (e.g. link number)
  uint64_t    subSpecification;

  //flags, first bit indicates that a sub header follows
  uint32_t    flags;

  uint32_t    headerVersion;  //version of this header
  uint32_t    headerSize;     //size of this header
  uint32_t    payloadSize;    //size of the associated data

  //___NEVER MODIFY THE ABOVE
  //___NEW STUFF GOES BELOW

  //___the functions:
  DataHeader(); //ctor
  DataHeader(const DataHeader&); //copy ctor
  DataHeader& operator=(const DataHeader&); //assignment
  DataHeader& operator=(const DataOrigin&); //assignment
  DataHeader& operator=(const DataDescription&); //assignment
  DataHeader& operator=(const PayloadSerialization&); //assignment
  bool operator==(const DataHeader&); //comparison
  bool operator==(const DataOrigin&); //comparison
  bool operator==(const DataDescription&); //comparison
  bool operator==(const PayloadSerialization&); //comparison
  void print() const;
};

//____________________________________________________________________________
struct DataOrigin
{
  //origin of the data (originating detector)
  union {
    char     dataOrigin[gSizeDataOriginString];
    uint32_t  dataOriginInt;
  };
  DataOrigin();
  DataOrigin(const DataOrigin& other) : dataOriginInt(other.dataOriginInt) {}
  DataOrigin& operator=(const DataOrigin& other) {
    if (&other != this) dataOriginInt = other.dataOriginInt;
    return *this;
  }
  // note: no operator=(const char*) as this potentially runs into trouble with this
  // general pointer type, use: someorigin = DataOrigin("BLA")
  DataOrigin(const char* origin);
  bool operator==(const DataOrigin&) const;
  bool operator!=(const DataOrigin& other) const {return not this->operator==(other);}
  void print() const;
};

//____________________________________________________________________________
struct DataDescription
{
  //data type descriptor
  union {
    char     dataDescription[gSizeDataDescriptionString];
    uint64_t  dataDescriptionInt[2];
  };
  DataDescription();
  DataDescription(const DataDescription& other) : dataDescriptionInt() {*this = other;}
  DataDescription& operator=(const DataDescription& other) {
    if (&other != this) {
      dataDescriptionInt[0] = other.dataDescriptionInt[0];
      dataDescriptionInt[1] = other.dataDescriptionInt[1];
    }
    return *this;
  }
  // note: no operator=(const char*) as this potentially runs into trouble with this
  // general pointer type, use: somedesc = DataOrigin("SOMEDESCRIPTION")
  DataDescription(const char* desc);
  bool operator==(const DataDescription&) const;
  bool operator!=(const DataDescription& other) const {return not this->operator==(other);}
  void print() const;
};

//____________________________________________________________________________
struct DataIdentifier
{
  //a full data identifier combining origin and description
  DataDescription dataDescription;
  DataOrigin dataOrigin;
  DataIdentifier();
  DataIdentifier(const DataIdentifier&);
  DataIdentifier(const char* desc, const char* origin);
  bool operator==(const DataIdentifier&) const;
  void print() const;
};

//____________________________________________________________________________
struct PayloadSerialization
{
  //serialization method
  union {
    char     payloadSerialization[gSizePayloadSerializationString];
    uint64_t  payloadSerializationInt;
  };
  PayloadSerialization();
  // note: no operator=(const char*) as this potentially runs into trouble with this
  // general pointer type, use: sertype = DataOrigin("SERTYPE")
  PayloadSerialization(const char* serialization);
  bool operator==(const PayloadSerialization&) const;
  void print() const;
};

//____________________________________________________________________________
//default int representation of 'invalid' token for char fields
//TODO: endiness adaption at compile time
const uint32_t gInvalidToken32 = 0x00202020;
const uint64_t gInvalidToken64 = 0x0020202020202020;

//____________________________________________________________________________
//possible data origins
extern const DataOrigin gDataOriginAny;
extern const DataOrigin gDataOriginInvalid;
extern const DataOrigin gDataOriginTPC;
extern const DataOrigin gDataOriginTRD;
extern const DataOrigin gDataOriginTOF;

//____________________________________________________________________________
//possible data types
extern const DataDescription gDataDescriptionAny;
extern const DataDescription gDataDescriptionInvalid;
extern const DataDescription gDataDescriptionRaw;
extern const DataDescription gDataDescriptionClusters;
extern const DataDescription gDataDescriptionTracks;

//____________________________________________________________________________
//possible serialization types
extern const PayloadSerialization gPayloadSerializationAny;
extern const PayloadSerialization gPayloadSerializationInvalid;
extern const PayloadSerialization gPayloadSerializationNone;
extern const PayloadSerialization gPayloadSerializationROOT;
extern const PayloadSerialization gPayloadSerializationFlatBuf;

} //namespace Base
} //namespace AliceO2

#endif
