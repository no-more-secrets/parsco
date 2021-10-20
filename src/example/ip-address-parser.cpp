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

// parsco
#include "parsco/combinator.hpp"
#include "parsco/ext-basic.hpp"
#include "parsco/ext-std.hpp"
#include "parsco/promise.hpp"
#include "parsco/runner.hpp"

// C++ standard library
#include <iostream>
#include <optional>

using namespace std;
using namespace parsco;

struct ipv4_address {
  int           n1;
  int           n2;
  int           n3;
  int           n4;
  optional<int> subnet_mask;
};

// Parsers for IPv4 address with optional subnet_mask, e.g.:
//
//   123.234.345.456
//   123.234.345.0/16
//
// The bit subnet_mask on the end, if present, must be <= 32.

parser<int> parse_ip_number() {
  int n = co_await parse_int();
  if( n > 255 ) co_await fail( "ip values must be <= 255" );
  co_return n;
}

parser<ipv4_address> parse_ip_address() {
  ipv4_address result;

  result.n1 = co_await parse_ip_number();
  co_await chr( '.' );
  result.n2 = co_await parse_ip_number();
  co_await chr( '.' );
  result.n3 = co_await parse_ip_number();
  co_await chr( '.' );
  result.n4 = co_await parse_ip_number();

  // The try_ combinator returns the result wrapped in a
  // `result_t` in order to signal (in a type-safe way) that the
  // parser may or may not succeed and that it will backtrack if
  // not successful.
  result_t<char> slash = co_await try_{ chr( '/' ) };

  if( slash.has_value() ) {
    int mask = co_await parse_int();
    if( mask > 32 ) co_await fail( "subnet mask must be <= 32" );
    result.subnet_mask = mask;
  }

  co_return result;
}

/****************************************************************
** main
*****************************************************************/
int main( int, char** ) {
  // Some of these will succeed while others will fail.
  vector<string> tests{
      // Input                 Expected Result
      // =====================================
      "123.234.123.99",     // pass.
      "123.234.123.99/23",  // pass.
      "123.234.123.99 /23", // pass, but not consume all input.
      "123.234.123.99/",    // fail.
      "123,234.123.99",     // fail.
      "123.234.xxx.99",     // fail.
      "123.234.123.99/33",  // fail.
      "123.234.123",        // fail.
      "123.234.123.990",    // fail.
      "123.234.123/8",      // fail.
  };

  using namespace parsco;

  for( string const& s : tests ) {
    result_t<ipv4_address> ip =
        run_parser( "tests", s, parse_ip_address() );

    if( !ip ) {
      cout << "test \"" << s
           << "\" failed to parse: " << ip.get_error().what()
           << "\n";
      continue;
    }
    cout << "test \"" << s << "\" succeeded to parse: " << ip->n1
         << "." << ip->n2 << "." << ip->n3 << "." << ip->n4;
    if( ip->subnet_mask.has_value() )
      cout << "/" << *ip->subnet_mask;
    cout << "\n";
  }

  return 0;
}