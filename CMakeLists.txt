cmake_minimum_required(VERSION 3.0.0)
project(LockFreeQueue VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)

add_library(LockFreeQueue LockFreeQueue.cpp)

enable_testing()

add_executable(
  LockFreeQueue_Tests
  LockFreeQueue_Test.cpp
)
target_link_libraries(
  LockFreeQueue_Tests
  atomic
  gtest_main
)

include(GoogleTest)
gtest_discover_tests(LockFreeQueue_Tests)