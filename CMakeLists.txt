cmake_minimum_required(VERSION 3.5)
project(runtime-verification-system)

set(CMAKE_CXX_STANDARD 11)

if (WIN32)
    link_directories(D:\\Users\\chenkuan\\ClionProjects\\tinylog\\lib)
    link_directories(D:\\Users\\chenkuan\\ClionProjects\\tinyxml2\\lib)
    link_directories(D:\\software\\compilerandalgorithm\\z3\\z3-4.6.0-x86-win\\bin)

    include_directories(D:\\Users\\chenkuan\\ClionProjects\\tinylog\\headers)
    include_directories(D:\\Users\\chenkuan\\ClionProjects\\tinyxml2\\headers)
    include_directories(D:\\software\\compilerandalgorithm\\z3\\z3-4.6.0-x86-win\\include)
elseif(UNIX)
    link_directories(/home/kong/CLionProjects/tinylog/lib)
    link_directories(/home/kong/CLionProjects/tinyxml2/lib)

    include_directories(/home/kong/CLionProjects/tinylog/headers)
    include_directories(/home/kong/CLionProjects/tinyxml2/headers)
endif()

include_directories(headers/DFA)
aux_source_directory(src/DFA DFA)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/lib)

add_executable(rvsDFATest ${DFA} src/rvsDFATestSelf.cpp)
target_link_libraries(rvsDFATest z3)
target_link_libraries(rvsDFATest tinylog)
target_link_libraries(rvsDFATest tinyxml2)

add_library(rvsDFA SHARED ${DFA})
target_link_libraries(rvsDFA z3)
target_link_libraries(rvsDFA tinylog)
target_link_libraries(rvsDFA tinyxml2)
