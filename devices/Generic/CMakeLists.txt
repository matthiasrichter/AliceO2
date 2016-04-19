# @author Matthias Richter
# @brief  cmake setup for module devices/Generic

set(INCLUDE_DIRECTORIES
)

set(SYSTEM_INCLUDE_DIRECTORIES
)


set(SRCS
  Factory.cxx
)

set(LIBRARY_NAME AliceO2Generic)

GENERATE_LIBRARY()

Set(Exe_Names
  testFactory
)

set(Exe_Source
  testFactory.cxx
)

list(LENGTH Exe_Names _length)

if(${_length})
math(EXPR _length ${_length}-1)
ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES AliceO2Generic dl)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})
endif(${_length})
