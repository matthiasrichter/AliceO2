set(MODULE_NAME "example_msgformat")

O2_SETUP(NAME ${MODULE_NAME})

set(SRCS
  )

set(HEADERS
  )

# TODO: temporarily using bucket of flp2epn example
set(BUCKET_NAME flp2epn_bucket)

#set(LIBRARY_NAME ${MODULE_NAME})
#O2_GENERATE_LIBRARY()

Set(Exe_Names
    dataSampler
    )


set(Exe_Source
    runDataSampler.cxx
    )


list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

ForEach (_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  O2_GENERATE_EXECUTABLE(
      EXE_NAME ${_name}
      SOURCES ${_src}
      MODULE_LIBRARY_NAME ${LIBRARY_NAME}
      BUCKET_NAME ${BUCKET_NAME}
  )
EndForEach (_file RANGE 0 ${_length})
