# CMake entry point
cmake_minimum_required (VERSION 3.1)
project (NESupport)

SET(WORK_DIR  "${PROJECT_SOURCE_DIR}")  



add_definitions(
  -std=c++11
  -DTW_STATIC
  -DTW_NO_LIB_PRAGMA
  -DTW_NO_DIRECT3D
  -DGLEW_STATIC
  -D_CRT_SECURE_NO_WARNINGS
)


set(SRC_HEADER_FILES 
src/MAP.h
src/Sprite2.h
src/WAS.h
src/WDF.h
)

set(SRC_CPP_FILES 
src/MAP.cpp
src/MAP.h
src/Sprite2.cpp
src/Sprite2.h
src/WAS.cpp
src/WAS.h
src/WDF.cpp
src/WDF.h
)

add_library(${PROJECT_NAME}
${SRC_CPP_FILES}
)

install (FILES ${SRC_HEADER_FILES} DESTINATION include/${PROJECT_NAME})
install (TARGETS ${PROJECT_NAME} DESTINATION lib/)

