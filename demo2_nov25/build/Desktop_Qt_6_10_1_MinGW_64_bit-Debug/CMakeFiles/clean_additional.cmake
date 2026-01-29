# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\appdemo2_nov25_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\appdemo2_nov25_autogen.dir\\ParseCache.txt"
  "appdemo2_nov25_autogen"
  )
endif()
