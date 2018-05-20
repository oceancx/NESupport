# CMake entry point
cmake_minimum_required (VERSION 3.1)
project (NESupport)

SET(WORK_DIR  "${PROJECT_SOURCE_DIR}")  
configure_file (  
  "${PROJECT_SOURCE_DIR}/src/defines.h.in"  
  "${PROJECT_SOURCE_DIR}/src/defines.h"  
  )  


add_definitions(
  -std=c++11
  -DTW_STATIC
  -DTW_NO_LIB_PRAGMA
  -DTW_NO_DIRECT3D
  -DGLEW_STATIC
  -D_CRT_SECURE_NO_WARNINGS
)

add_subdirectory(src)
