#include "Headers/DataHeader.h"

void test() {
  gROOT->LoadMacro("DataHeader.cxx++");
  AliceO2::Header::DataHeader d;
  d.dataDescription = AliceO2::Header::DataDescription("TESTDATASAMPLE");
  d.dataOrigin = AliceO2::Header::DataOrigin("TEST");
  AliceO2::Header::NameHeader<16> n("AUXILIARYINFO");
  auto b = AliceO2::Header::Block::compose(d,n);
  AliceO2::Header::hexDump("Block dump",b.data(),b.size());
  for (const AliceO2::Header::BaseHeader* h = b; h != nullptr; h = h->next()) {
    AliceO2::Header::hexDump("Header dump",h->data(),h->size());
  }

  // find header in the data block by type
  auto sh = AliceO2::Header::get<AliceO2::Header::NameHeader<16>>(b.data());
  AliceO2::Header::hexDump("Header dump",sh->data(),sh->size());
}
