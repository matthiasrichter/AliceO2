#include "DataHeader.h"
#include <cstdio> // printf
#include <cstring> // strncpy

const char* AliceO2::Base::DataHeader::sMagicString = "O2 ";

using namespace AliceO2::Base;

//possible data origins
const DataOrigin gDataOriginAny    ("***");
const DataOrigin gDataOriginInvalid("   ");
const DataOrigin gDataOriginTPC    ("TPC");
const DataOrigin gDataOriginTRD    ("TRD");
const DataOrigin gDataOriginTOF    ("TOF");

//possible data types
const DataDescription gDataDescriptionAny     ("***************");
const DataDescription gDataDescriptionInvalid ("               ");
const DataDescription gDataDescriptionRawData ("RAWDATA        ");
const DataDescription gDataDescriptionClusters("CLUSTERS       ");
const DataDescription gDataDescriptionTracks  ("TRACKS         ");

//possible serialization types
const PayloadSerialization gPayloadSerializationAny    ("*******");
const PayloadSerialization gPayloadSerializationInvalid("       ");
const PayloadSerialization gPayloadSerializationNone   ("NONE   ");
const PayloadSerialization gPayloadSerializationROOT   ("ROOT   ");
const PayloadSerialization gPayloadSerializationFlatBuf("FLATBUF");

//_________________________________________________________________________________________________
AliceO2::Base::DataHeader::DataHeader()
  : magicStringInt(*reinterpret_cast<const uint32_t*>(sMagicString))
  , dataOriginInt(gDataOriginInvalid.dataOriginInt)
  , payloadSerializationInt(gPayloadSerializationInvalid.payloadSerializationInt)
  , dataDescriptionInt{gDataDescriptionInvalid.dataDescriptionInt[0],
                       gDataDescriptionInvalid.dataDescriptionInt[1]}
  , subSpecification(0)
  , flags(0)
  , headerVersion(sVersion)
  , headerSize(sizeof(DataHeader))
  , payloadSize(0)
{
}

//_________________________________________________________________________________________________
AliceO2::Base::DataHeader::DataHeader(const DataHeader& that)
  : magicStringInt(that.magicStringInt)
  , dataOriginInt(that.dataOriginInt)
  , payloadSerializationInt(that.payloadSerializationInt)
  , dataDescriptionInt{that.dataDescriptionInt[0], that.dataDescriptionInt[1]}
  , subSpecification(that.subSpecification)
  , flags(that.flags)
  , headerVersion(that.headerVersion)
  , headerSize(that.headerSize)
  , payloadSize(that.payloadSize)
{
}

//_________________________________________________________________________________________________
void AliceO2::Base::DataHeader::print() const
{
  printf("Data header version %i, flags: %i\n",headerVersion, flags);
  printf("  origin       : %s\n", dataOrigin);
  printf("  serialization: %s\n", payloadSerialization);
  printf("  description  : %s\n", dataDescription);
  printf("  sub spec.    : %lu\n", subSpecification);
  printf("  header size  : %i\n", headerSize);
  printf("  payloadSize  : %i\n", payloadSize);
}

//_________________________________________________________________________________________________
DataHeader& AliceO2::Base::DataHeader::operator=(const DataHeader& that)
{
  magicStringInt = that.magicStringInt;
  dataOriginInt = that.dataOriginInt;
  dataDescriptionInt[0] = that.dataDescriptionInt[0];
  dataDescriptionInt[1] = that.dataDescriptionInt[1];
  payloadSerializationInt = that.payloadSerializationInt;
  subSpecification = that.subSpecification;
  flags = that.flags;
  headerVersion = that.headerVersion;
  headerSize = that.headerSize;
  payloadSize = that.payloadSize;
  return *this;
}

//_________________________________________________________________________________________________
DataHeader& AliceO2::Base::DataHeader::operator=(const DataOrigin& that)
{
  dataOriginInt = that.dataOriginInt;
  return *this;
}

//_________________________________________________________________________________________________
DataHeader& AliceO2::Base::DataHeader::operator=(const DataDescription& that)
{
  dataDescriptionInt[0] = that.dataDescriptionInt[0];
  dataDescriptionInt[1] = that.dataDescriptionInt[1];
  return *this;
}

//_________________________________________________________________________________________________
DataHeader& AliceO2::Base::DataHeader::operator=(const PayloadSerialization& that)
{
  payloadSerializationInt = that.payloadSerializationInt;
  return *this;
}

//_________________________________________________________________________________________________
bool AliceO2::Base::DataHeader::operator==(const DataOrigin& that)
{
  return (that.dataOriginInt == gDataOriginAny.dataOriginInt ||
          that.dataOriginInt == dataOriginInt );
}

//_________________________________________________________________________________________________
bool AliceO2::Base::DataHeader::operator==(const DataDescription& that)
{
  return (that.dataDescriptionInt == gDataDescriptionAny.dataDescriptionInt ||
          that.dataDescriptionInt == dataDescriptionInt );
}

//_________________________________________________________________________________________________
bool AliceO2::Base::DataHeader::operator==(const PayloadSerialization& that)
{
  return (that.payloadSerializationInt == gPayloadSerializationAny.payloadSerializationInt ||
          that.payloadSerializationInt == payloadSerializationInt );
}

//_________________________________________________________________________________________________
bool AliceO2::Base::DataHeader::operator==(const DataHeader& that)
{
  return( magicStringInt == that.magicStringInt &&
          dataOriginInt == that.dataOriginInt &&
          dataDescriptionInt == that.dataDescriptionInt &&
          subSpecification == that.subSpecification );
}

//_________________________________________________________________________________________________
AliceO2::Base::DataOrigin::DataOrigin() : dataOriginInt(gInvalidToken32) {}

//_________________________________________________________________________________________________
AliceO2::Base::DataOrigin::DataOrigin(const char* origin)
  : dataOriginInt(gInvalidToken32)
{
  if (origin) {
    strncpy(dataOrigin, origin, gSizeDataOriginString-1);
  }
}

//_________________________________________________________________________________________________
bool AliceO2::Base::DataOrigin::operator==(const AliceO2::Base::DataOrigin& other) const
{
  return dataOriginInt == other.dataOriginInt;
}

//_________________________________________________________________________________________________
void AliceO2::Base::DataOrigin::print() const
{
  printf("Data origin  : %s\n", dataOrigin);
}

//_________________________________________________________________________________________________
AliceO2::Base::DataDescription::DataDescription()
  : dataDescriptionInt()
{
  dataDescriptionInt[0] = gInvalidToken64;
  dataDescriptionInt[1] = gInvalidToken64<<8 | gInvalidToken64;
}

//_________________________________________________________________________________________________
AliceO2::Base::DataDescription::DataDescription(const char* desc)
  : dataDescription()
{
  *this = DataDescription(); // initialize by standard constructor
  if (desc) {
    strncpy(dataDescription, desc, gSizeDataDescriptionString-1);
  }
}

//_________________________________________________________________________________________________
bool AliceO2::Base::DataDescription::operator==(const AliceO2::Base::DataDescription& other) const {
  return (dataDescriptionInt[0] == other.dataDescriptionInt[0] &&
          dataDescriptionInt[1] == other.dataDescriptionInt[1]);
}

//_________________________________________________________________________________________________
void AliceO2::Base::DataDescription::print() const
{
  printf("Data descr.  : %s\n", dataDescription);
}

//_________________________________________________________________________________________________
AliceO2::Base::DataIdentifier::DataIdentifier()
  : dataDescription(), dataOrigin()
{
}

//_________________________________________________________________________________________________
AliceO2::Base::DataIdentifier::DataIdentifier(const char* desc, const char* origin)
  : dataDescription(), dataOrigin()
{
  dataDescription = AliceO2::Base::DataDescription(desc);
  dataOrigin = AliceO2::Base::DataOrigin(origin);
}

//_________________________________________________________________________________________________
bool AliceO2::Base::DataIdentifier::operator==(const AliceO2::Base::DataIdentifier& other) const {
  if (other.dataOrigin != gDataOriginAny && dataOrigin != other.dataOrigin) return false;
  if (other.dataDescription != gDataDescriptionAny && dataDescription != other.dataDescription) return false;
  return true;
}

//_________________________________________________________________________________________________
void AliceO2::Base::DataIdentifier::print() const
{
  dataOrigin.print();
  dataDescription.print();
}

//_________________________________________________________________________________________________
AliceO2::Base::PayloadSerialization::PayloadSerialization() : payloadSerializationInt(gInvalidToken64) {}

//_________________________________________________________________________________________________
AliceO2::Base::PayloadSerialization::PayloadSerialization(const char* serialization)
  : payloadSerializationInt(gInvalidToken32)
{
  if (serialization) {
    strncpy(payloadSerialization, serialization, gSizePayloadSerializationString-1);
  }
}

//_________________________________________________________________________________________________
bool AliceO2::Base::PayloadSerialization::operator==(const AliceO2::Base::PayloadSerialization& other) const {
  return payloadSerializationInt == other.payloadSerializationInt;
}

//_________________________________________________________________________________________________
void AliceO2::Base::PayloadSerialization::print() const
{
  printf("Serialization: %s\n", payloadSerialization);
}
