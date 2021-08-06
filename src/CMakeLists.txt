add_subdirectory( example )

file( GLOB sources "[a-zA-Z]*.cpp" )

add_library(
  parsco
  STATIC
  ${sources}
)

target_compile_features(
  parsco
  PUBLIC
  cxx_std_20
)

set_target_properties(
  parsco
  PROPERTIES
  CXX_EXTENSIONS OFF
)

target_compile_options(
  parsco
  PRIVATE
  # clang
  $<$<CXX_COMPILER_ID:Clang>:
     -Wall
     -Wextra
   >
  # gcc
  $<$<CXX_COMPILER_ID:GNU>:
      -Wall
      -Wextra
      -fcoroutines
   >
)

target_include_directories(
  parsco
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
