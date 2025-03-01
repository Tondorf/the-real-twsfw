cmake_minimum_required(VERSION 3.14)

project(twsfwTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(twsfw REQUIRED)
  enable_testing()
endif()

# ---- Tests ----

add_executable(twsfw_test source/twsfw_test.cpp)
target_link_libraries(twsfw_test PRIVATE twsfw::twsfw)
target_compile_features(twsfw_test PRIVATE cxx_std_20)

add_test(NAME twsfw_test COMMAND twsfw_test)

# ---- End-of-file commands ----

add_folders(Test)
