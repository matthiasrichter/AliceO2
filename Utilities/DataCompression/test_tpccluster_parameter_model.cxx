/**
g++ --std=c++11 -g -ggdb -I$BOOST_ROOT/include -o test_tpccluster_parameter_model test_tpccluster_parameter_model.cxx
 */

#include "tpccluster_parameter_model.h"
#include "CodingModelDispatcher.h"
#include "HuffmanCodec.h"
#include "TPCRawCluster.h"
#include <iostream>
#include <bitset>
#include <exception>
#include <chrono>
#include <boost/mpl/size.hpp>
#include <boost/mpl/string.hpp>
#include <boost/mpl/at.hpp>

typedef std::chrono::system_clock system_clock;
typedef std::chrono::microseconds TimeScale;

template<typename DispatcherT, bool _doEncoding = true, bool _checkDecoding = false>
struct encodeValue {
  encodeValue(DispatcherT& _d, int _verbosity) : dispatcher(_d), verbosity(_verbosity) {}
  ~encodeValue() {}

  typedef DispatcherT dispatcher_type;
  static const bool doEncoding = _doEncoding;
  static const bool checkDecoding = _checkDecoding;

  int operator()(unsigned long long int value, typename DispatcherT::code_type& code = 0, uint16_t& codelen = 0) {
  // TODO: the dispatcher forwards the code type of the upper most
  // container stage

    if (doEncoding) {
  // encode the value and increment model if no subsequent decoding
  // this is a compile time switch
  dispatcher.encode(value, code, codelen, !checkDecoding);
  if (verbosity > 1) {
    std::cout << "value=" << value
              << " codelen=" << codelen
              << " code=" << code
              << std::endl;
  }
    }
  if (!checkDecoding) return 0;

  // move to upper most bits if order MSB
  // this function gets the coding direction for model at current stage
  if (dispatcher.getCodingDirection()) code <<= (code.size()-codelen);

  // read back value
  unsigned long long int readback = 0;
  uint16_t readbacklen = 0;
  dispatcher.decode(readback, code, readbacklen);
  if (value != readback || codelen != readbacklen) {
    std::cerr << "decoding mismatch: " << readback << "(" << readbacklen << "), " << " expected " << value << "(" << codelen << ")" << std::endl;
  }

  return 0;
  }

  DispatcherT& dispatcher;
  int verbosity;
};

template<typename DispatcherT>
struct incrementModel {
  incrementModel(DispatcherT& _d, int _verbosity) : dispatcher(_d), verbosity(_verbosity) {}
  ~incrementModel() {}

  typedef DispatcherT dispatcher_type;
  static const bool doEncoding = true;
  static const bool checkDecoding = false;

  int operator()(unsigned long long int value, typename DispatcherT::code_type& code = 0, uint16_t& codelen = 0) {
    if (verbosity > 1) {
      std::cout << "increment occurrence for value " << value << std::endl;
    }
    return dispatcher.addWeight(value, 1.);
  }

  DispatcherT& dispatcher;
  int verbosity;
};

template<typename F>
int processClusters(const char* filename, F operation)
{
  bool doBulkOperation = !F::doEncoding;

  int nofClusters = 0;
  if (operation.verbosity > 0) {
    std::cout << "Processing file " << filename << std::endl;
  }
  ALICE::HLT::TPC::RawClusterArray ca(filename);

  typename F::dispatcher_type::container_type& container = *operation.dispatcher;
  typedef typename F::dispatcher_type::container_type::types wrapped_types;
  typename F::dispatcher_type::code_type code;
  uint16_t codelen = 0;

  uint16_t lastPadrow = 0;
  for (auto cluster : ca) {
    ++nofClusters;
    if (operation.verbosity > 1) {
      std::cout << "***************************************************" << std::endl;
      std::cout << cluster << std::endl;
    }
    unsigned int parameterID = 0;
    uint16_t padrow = cluster.GetPadRow();
    unsigned long long int value = padrow - lastPadrow;
    lastPadrow = padrow;
    if (doBulkOperation) {
      code = (*static_cast<typename boost::mpl::at_c<wrapped_types, 0>::type&>(container)).Encode(value, codelen);
    }
    operation(value, code, codelen);
    float padval = cluster.GetPad() * 60;
    value = padval;
    if (doBulkOperation) {
      code =  (*static_cast<typename boost::mpl::at_c<wrapped_types, 1>::type&>(container)).Encode(value, codelen);
    }
    operation(value, code, codelen);
    float timeval = cluster.GetTime() * 25;
    value = timeval;
    if (doBulkOperation) {
      code =  (*static_cast<typename boost::mpl::at_c<wrapped_types, 2>::type&>(container)).Encode(value, codelen);
    }
    operation(value, code, codelen);
    float sigmaPad2val = cluster.GetSigmaPad2() * 25;
    value = sigmaPad2val;
    if (value > 255) value = 255;
    if (doBulkOperation) {
      code =  (*static_cast<typename boost::mpl::at_c<wrapped_types, 3>::type&>(container)).Encode(value, codelen);
    }
    operation(value, code, codelen);
    float sigmaTime2val = cluster.GetSigmaTime2() * 10;
    value = sigmaTime2val;
    if (value > 255) value = 255;
    if (doBulkOperation) {
      code =  (*static_cast<typename boost::mpl::at_c<wrapped_types, 4>::type&>(container)).Encode(value, codelen);
    }
    operation(value, code, codelen);
    value = cluster.GetCharge();
    if (doBulkOperation) {
      code =  (*static_cast<typename boost::mpl::at_c<wrapped_types, 5>::type&>(container)).Encode(value, codelen);
    }
    operation(value, code, codelen);
    value = cluster.GetQMax();
    if (doBulkOperation) {
      code =  (*static_cast<typename boost::mpl::at_c<wrapped_types, 6>::type&>(container)).Encode(value, codelen);
    }
    operation(value, code, codelen);
  }
  return nofClusters;
}

int main()
{
  int verbosity = 0;
  enum {unknown = 0, training = 1, encoding = 2};
  int mode = encoding;
#ifdef VERBOSE
  verbosity = VERBOSE;
#endif
#ifdef BULK_OPERATION
  const bool doBulkOperation = true;
#else
  const bool doBulkOperation = false;
#endif
#ifdef CHECK_DECODING
  const bool doDecoding = true;
#else
  const bool doDecoding = false;
#endif

  std::cout << "Testing multi parameter definition:" << std::endl;

  typedef ALICE::O2::CodingModelDispatcher< tpccluster_parameter_models > TPCClusterParameterDispatcher_t;
  TPCClusterParameterDispatcher_t multiparameterdispatcher;
  multiparameterdispatcher.init();
  (*multiparameterdispatcher).print();

  system_clock::time_point refTimeTotal = system_clock::now();
  switch (mode) {
  case encoding:
    multiparameterdispatcher.read("table.txt");
    std::cout << "Testing encoding" << std::endl;
    break;
  case training:
    std::cout << "Testing training" << std::endl;
    break;
  default:
    std::cerr << "Unknown mode " << mode << std::endl;
  }

  std::cout << "Reading file names from standard input ..." << std::endl;
  int fileCount = 0;
  std::string line;
  char firstChar = 0;

  int totalNofClusters = 0;
  system_clock::time_point refTimeLoop = system_clock::now();
  auto durationOperation = std::chrono::duration_cast<TimeScale>(refTimeLoop - refTimeLoop);
  while ((std::cin.get(firstChar)) && firstChar != '\n' && (std::cin.putback(firstChar)) && (std::getline(std::cin, line))) {
    try {
      system_clock::time_point refTimeOperation = system_clock::now();
      switch (mode) {
      case encoding:
	totalNofClusters += processClusters(line.c_str(), encodeValue<TPCClusterParameterDispatcher_t, !doBulkOperation, doDecoding>(multiparameterdispatcher, verbosity));
	break;
      case training:
	totalNofClusters += processClusters(line.c_str(), incrementModel<TPCClusterParameterDispatcher_t>(multiparameterdispatcher, verbosity));
	break;
      }
      durationOperation += std::chrono::duration_cast<TimeScale>(std::chrono::system_clock::now() - refTimeOperation);
    }
    catch (std::exception& e) {
      std::cerr << "Exeption at file " << line << std::endl;
      std::cout << e.what() << std::endl;
      return -1;
    }
    ++fileCount;
  }

  system_clock::time_point loopEnd = system_clock::now();

  if (mode == training) {
    std::cout << "Processing statistics" << std::endl;
    multiparameterdispatcher.generate();
    std::cout << "Writing configuration" << std::endl;
    multiparameterdispatcher.write("configuration.txt");
  }

  system_clock::time_point now = system_clock::now();
  auto durationTotal = std::chrono::duration_cast<TimeScale>(now - refTimeTotal);
  auto durationLoop = std::chrono::duration_cast<TimeScale>(loopEnd - refTimeLoop);
  std::cout << fileCount << " file(s) processed"
	    << ", " << totalNofClusters << " cluster(s)"
	    << ", " << totalNofClusters * boost::mpl::size<tpccluster_parameter_models>::value << " operation(s)"
	    << "- total time " << durationTotal.count() << " us"
	    << "  loop time " << durationLoop.count() << " us"
	    << "  operation time " << durationOperation.count() << " us"
	    << std::endl;

  return 0;
}
