# CMake entry point
cmake_minimum_required (VERSION 3.1)
project (NESupport)


add_definitions(
  -std=c++11
  -DTW_STATIC
  -DTW_NO_LIB_PRAGMA
  -DTW_NO_DIRECT3D
  -DGLEW_STATIC
  -D_CRT_SECURE_NO_WARNINGS
)


set(SRC_HEADER_FILES 
src/NESupport.h
)

set(SRC_CPP_FILES 
src/NESupport.cpp
src/NESupport.h
)

add_library( ${PROJECT_NAME}
${SRC_CPP_FILES}
)
add_executable(${PROJECT_NAME}EXE
${SRC_CPP_FILES} src/main.cpp
)


install (FILES ${SRC_CPP_FILES} DESTINATION include/${PROJECT_NAME})
install (TARGETS ${PROJECT_NAME} DESTINATION lib/)

