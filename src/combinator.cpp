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
#include "parsco/combinator.hpp"

// parsco
#include "parsco/parser.hpp"
#include "parsco/promise.hpp"

using namespace std;

namespace parsco {

namespace {

bool is_digit( char c ) { return ( c >= '0' && c <= '9' ); }

bool is_lower( char c ) { return ( c >= 'a' && c <= 'z' ); }

bool is_upper( char c ) { return ( c >= 'A' && c <= 'Z' ); }

bool is_alpha( char c ) {
  return is_lower( c ) || is_upper( c );
}

bool is_blank( char c ) {
  return ( c == ' ' ) || ( c == '\n' ) || ( c == '\r' ) ||
         ( c == '\t' );
}

bool is_alphanum( char c ) {
  return is_digit( c ) || ( c >= 'a' && c <= 'z' ) ||
         ( c >= 'A' && c <= 'Z' );
}

} // namespace

parser<char> any_chr() {
  co_return co_await builtin_next_char{};
}

parser<char> chr( char c ) {
  char parsed_c = co_await builtin_next_char{};
  if( parsed_c != c )
    co_await fail( string( "expected '" ) + c + "'" );
  co_return c;
}

parser<char> lower() { return pred( is_lower ); }

parser<char> upper() { return pred( is_upper ); }

parser<char> alpha() { return pred( is_alpha ); }

parser<char> alphanum() { return pred( is_alphanum ); }

parser<char> space() { return chr( ' ' ); }
parser<char> crlf() { return one_of( "\r\n" ); }
parser<char> tab() { return chr( '\t' ); }
parser<char> blank() { return pred( is_blank ); }

parser<string> blanks() {
  co_return string( co_await builtin_blanks{} );
}

parser<string> identifier() {
  co_return string( co_await builtin_identifier{} );
}

parser<char> digit() { return pred( is_digit ); }

parser<> str( string sv ) {
  for( char c : sv ) co_await chr( c );
}

parser<char> one_of( string sv ) {
  return pred( [sv]( char c ) {
    return sv.find_first_of( c ) != string::npos;
  } );
}

parser<char> not_of( string sv ) {
  return pred( [sv]( char c ) {
    return sv.find_first_of( c ) == string::npos;
  } );
}

parser<> eof() {
  result_t<char> c = co_await try_{ any_chr() };
  if( c.has_value() )
    // If we're here that means that there is more input in the
    // buffer, and thus that the `eof` parser (which requires
    // eof) has failed, meaning that the previous parsers did not
    // consume all of the input.
    co_await fail(
        "failed to parse all characters in input stream" );
}

parser<string_view> double_quoted_str() {
  co_return co_await builtin_double_quoted{};
}

parser<string_view> single_quoted_str() {
  co_return co_await builtin_single_quoted{};
}

parser<string> quoted_str() {
  co_return string( co_await first( double_quoted_str(),
                                    single_quoted_str() ) );
}

} // namespace parsco
