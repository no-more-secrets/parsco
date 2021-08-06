cmake_minimum_required( VERSION 3.12...3.12 )

project(
    parsco
    VERSION 0.1.0
    DESCRIPTION "C++20 Coroutine-Based Synchronous Parser Combinator Library"
    LANGUAGES CXX
)

set( CMAKE_CXX_STANDARD 20 )

add_subdirectory( src )

set( SANITIZER_FLAGS
  # When one of the checks triggers, this option will cause it to
  # abort the program immediately with an error code instead of
  # trying to recover and continuing to run the program. Not all
  # sanitizer options support this; the ones that don't will
  # simply print an error to the console and the program will
  # keep running.
  -fno-sanitize-recover=all
  # ASan (Address Sanitizer).
  -fsanitize=address
  # This enables all of the UBSan (undefined behavior) checks.
  -fsanitize=undefined
)

if( CMAKE_CXX_COMPILER_ID MATCHES "GNU" )
  # This is to fix a gcc linker error with asan.
  set( SANITIZER_FLAGS "${SANITIZER_FLAGS} -static-libasan" )
endif()

function( enable_sanitizers )
  message( STATUS "Enabling Sanitizers." )
  string( JOIN " " SANITIZER_FLAGS_STRING ${SANITIZER_FLAGS} )
  set( CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS_STRING}" PARENT_SCOPE )
  set( CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS_STRING}" PARENT_SCOPE )
endfunction()

# If ENABLE_ADDRESS_SANITIZER is on then this function will do so.
if( ENABLE_ADDRESS_SANITIZER )
  enable_sanitizers()
endif()
