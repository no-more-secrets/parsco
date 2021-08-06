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
#include "json-model.hpp"

// parsco
#include "parsco/combinator.hpp"
#include "parsco/ext-basic.hpp"
#include "parsco/ext-std.hpp"
#include "parsco/promise.hpp"
#include "parsco/runner.hpp"

// C++ standard library
#include <iostream>

using namespace std;
using namespace parsco;
using namespace json;

// For ADL reasons, the parse_for extension point overrides for
// the JSON model types must live in the same json namespace.
namespace json {

/****************************************************************
** helpers
*****************************************************************/
template<typename T>
parser<vector<T>> parse_vec() {
  return interleave( [] { return blanks() >> parse<Json, T>(); },
                     [] { return blanks() >> chr( ',' ); } );
}

template<typename T>
parser<vector<T>> bracketed_vec( char l, char r ) {
  return bracketed( blanks() >> chr( l ), parse_vec<T>(),
                    blanks() >> chr( r ) );
}

/****************************************************************
** string_val
*****************************************************************/
parser<string_val> parser_for( lang<Json>, tag<string_val> ) {
  return emplace<string_val>( quoted_str() );
}

/****************************************************************
** boolean
*****************************************************************/
parser<boolean> parser_for( lang<Json>, tag<boolean> ) {
  return ( str( "true" ) >> ret( boolean{ true } ) ) |
         ( str( "false" ) >> ret( boolean{ false } ) );
}

/****************************************************************
** number
*****************************************************************/
parser<number> parser_for( lang<Json>, tag<number> ) {
  // Delegate to variant parser, that's basically what number is.
  co_return co_await parse<Json, variant<int, double>>();
}

/****************************************************************
** key_val
*****************************************************************/
parser<key_val> parser_for( lang<Json>, tag<key_val> ) {
  return emplace<key_val>( blanks() >> quoted_str(),
                           blanks() >> chr( ':' ) >> blanks() >>
                               parse<Json, value>() );
}

/****************************************************************
** table
*****************************************************************/
parser<table> parser_for( lang<Json>, tag<table> ) {
  co_return co_await bracketed_vec<key_val>( '{', '}' );
}

/****************************************************************
** list
*****************************************************************/
parser<list> parser_for( lang<Json>, tag<list> ) {
  co_return co_await bracketed_vec<value>( '[', ']' );
}

/****************************************************************
** doc
*****************************************************************/
parser<doc> parser_for( lang<Json>, tag<doc> ) {
  return emplace<doc>( parse<Json, table>() ) << blanks();
}

} // namespace json

/****************************************************************
** main
*****************************************************************/
int main( int, char** ) {
  constexpr string_view json = R"(
    {
      "here": [
        "is",
        5,
        "some",
        42
      ],
      "json": true,
      "hello": "world"
    }
  )";

  using namespace parsco;

  result_t<json::doc> doc =
      parse_from_string<json::Json, json::doc>( "fake-file.json",
                                                json );

  if( !doc ) {
    cerr << "failed to parse json: " << doc.get_error().what()
         << "\n";
    return 1;
  }

  cout << "succeeded to parse json.\n";

  int answer = *std::get_if<int>(
      &std::get_if<json::number>(
           &std::get_if<unique_ptr<json::list>>(
                &( *doc ).tbl.members[0].v )
                ->get()
                ->members[3] )
           ->val );
  assert( answer == 42 );

  cout << "doc.here[3] == " << answer << "\n";
  return 0;
}