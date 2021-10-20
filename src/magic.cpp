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
#include "parsco/magic.hpp"

// C++ standard library
#include <cassert>

using namespace std;

namespace parsco {

namespace {

bool is_blank( char c ) {
  return ( c == ' ' ) || ( c == '\n' ) || ( c == '\r' ) ||
         ( c == '\t' );
}

bool is_leading_identifier_char( char c ) {
  return ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) ||
         ( c == '_' );
}

bool is_identifier_char( char c ) {
  return ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'Z' ) ||
         ( c >= 'a' && c <= 'z' ) || ( c == '_' );
}

} // namespace

// Removes blanks.
optional<BuiltinParseResult> builtin_blanks::try_parse(
    string_view in ) const {
  int pos = 0;
  while( pos < int( in.size() ) && is_blank( in[pos] ) ) ++pos;
  auto res = BuiltinParseResult{ .sv       = in.substr( 0, pos ),
                                 .consumed = pos };
  return res;
};

optional<BuiltinParseResult> builtin_identifier::try_parse(
    string_view in ) const {
  if( in.empty() || !is_leading_identifier_char( in[0] ) )
    return nullopt;
  int pos = 0;
  while( pos < int( in.size() ) &&
         is_identifier_char( in[pos] ) )
    ++pos;
  return BuiltinParseResult{ .sv       = in.substr( 0, pos ),
                             .consumed = pos };
};

optional<BuiltinParseResult> builtin_single_quoted::try_parse(
    string_view in ) const {
  int pos = 0;
  if( in.empty() || in[pos++] != '\'' ) return nullopt;
  while( true ) {
    // EOF before closing quote?
    if( pos == int( in.size() ) ) return nullopt;
    if( in[pos++] == '\'' ) break;
  }
  assert( pos >= 2 );
  string_view res( in.data() + 1, pos - 2 );
  return BuiltinParseResult{ .sv = res, .consumed = pos };
};

optional<BuiltinParseResult> builtin_double_quoted::try_parse(
    string_view in ) const {
  int pos = 0;
  if( in.empty() || in[pos++] != '"' ) return nullopt;
  while( true ) {
    // EOF before closing quote?
    if( pos == int( in.size() ) ) return nullopt;
    if( in[pos++] == '"' ) break;
  }
  assert( pos >= 2 );
  string_view res( in.data() + 1, pos - 2 );
  return BuiltinParseResult{ .sv = res, .consumed = pos };
};

} // namespace parsco
