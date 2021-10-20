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
#include "parsco/ext-basic.hpp"

// parsco
#include "parsco/combinator.hpp"
#include "parsco/error.hpp"
#include "parsco/promise.hpp"

using namespace std;

namespace parsco {

namespace {

result_t<int> safe_stoi( string const &s, int base = 10 ) {
  result_t<int> res( error( "error parsing int" ) );
  if( !s.empty() ) {
    size_t written;
    try {
      auto n = ::std::stoi( s, &written, base );
      if( written == s.size() ) res = n;
    } catch( std::exception const & ) {}
  }
  return res;
}

result_t<double> safe_stod( string const &s ) {
  result_t<double> res( error( "error parsing double" ) );
  if( !s.empty() ) {
    size_t written;
    try {
      auto d = ::std::stod( s, &written );
      if( written == s.size() ) res = d;
    } catch( exception const & ) {}
  }
  return res;
}

} // namespace

parser<int> parse_int() {
  int multiplier =
      ( co_await try_{ chr( '-' ) } ).has_value() ? -1 : 1;
  co_return multiplier *co_await lift(
      safe_stoi( co_await many1( digit ) ) );
}

parser<double> parse_double() {
  double multiplier =
      ( co_await try_{ chr( '-' ) } ).has_value() ? -1 : 1;
  string ipart = co_await many( digit );
  co_await chr( '.' );
  string fpart = co_await many( digit );
  if( ipart.empty() && fpart.empty() )
    co_await fail( "expected double" );
  co_return multiplier *co_await lift(
      safe_stod( ipart + '.' + fpart ) );
}

} // namespace parsco
