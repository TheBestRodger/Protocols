cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 20)
project(Sety)

include(settings.cmake)
find_package(Boost)

if(Boost_FOUND)
    find_package(Boost COMPONENTS serialization-clangw17 REQUIRED)
    find_package(Boost COMPONENTS json-clangw17 REQUIRED)
    find_package(Boost COMPONENTS container-clangw17 REQUIRED)
    message(${Boost_VERSION})
    message(${Boost_INCLUDE_DIRS})
    message(${Boost_LIBRARIES})
endif()

include_directories(${Boost_INCLUDE_DIR})
include_directories(${INCLUDE_PATH})
file(GLOB_RECURSE   HEADER     ${INCLUDE_PATH}/*.h*)
file(GLOB_RECURSE   SOURCE     ${SOURCE_PATH}/*.c*)
# Setup project
add_executable(Sety ${HEADER} ${SOURCE})

target_link_libraries(Sety ${Boost_LIBRARIES})
target_link_libraries(Sety "C:/local/boost_1_82_0/stage/lib/libboost_container-clangw17-mt-gd-x64-1_82.lib")
target_link_libraries(Sety "C:/local/boost_1_82_0/stage/lib/libboost_json-clangw17-mt-gd-x64-1_82.lib")









