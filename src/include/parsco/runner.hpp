/*
 * MIT License
 *
 * Copyright (c) 2021 David P. Sicilia (dpacbach)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without re-
 * striction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following con-
 * ditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Soft-
 * ware.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PUR-
 * POSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHER-
 * WISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

// parsco
#include "parsco/combinator.hpp"
#include "parsco/error.hpp"
#include "parsco/ext.hpp"
#include "parsco/parser.hpp"
#include "parsco/promise.hpp"

// C++ standard library
#include <cassert>
#include <sstream>
#include <string>
#include <string_view>

/****************************************************************
** Parser Runners
*****************************************************************/
namespace parsco {

// `filename` is the original file name that the string came
// from, in order to improve error messages.
template<typename T>
result_t<T> run_parser( std::string_view filename,
                        std::string_view in, parser<T> p ) {
  p.resume( in );
  assert( p.finished() );
  if( p.is_error() ) {
    // It's always one too far, not sure why.
    ErrorPos ep = ErrorPos::from_index( in, p.farthest() - 1 );
    std::ostringstream oss;
    oss << filename << ":error:" << ep.line << ":" << ep.col
        << " " << p.error().what();
    return result_t<T>( error( oss.str() ) );
  }
  return std::move( p.result() );
}

// `filename` is the original file name that the string came
// from, in order to improve error messages.
template<typename Lang, typename T>
result_t<T> parse_from_string( std::string_view filename,
                               std::string_view in ) {
  return run_parser( filename, in,
                     exhaust( parsco::parse<Lang, T>() ) );
}

} // namespace parsco
