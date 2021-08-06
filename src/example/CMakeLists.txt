add_executable( json-parser json-parser.cpp )
add_executable( hello-world-parser hello-world-parser.cpp )

target_link_libraries( json-parser PRIVATE parsco )
target_link_libraries( hello-world-parser PRIVATE parsco )

target_compile_features( json-parser PUBLIC cxx_std_20 )
target_compile_features( hello-world-parser PUBLIC cxx_std_20 )

set_target_properties( json-parser PROPERTIES CXX_EXTENSIONS OFF )
set_target_properties( hello-world-parser PROPERTIES CXX_EXTENSIONS OFF )

target_compile_options(
  json-parser
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

target_compile_options(
  hello-world-parser
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
  json-parser
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/
)

target_include_directories(
  hello-world-parser
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/
)
