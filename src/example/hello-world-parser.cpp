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

using namespace std;
using namespace parsco;

// As a first example, let us define a simple grammar that we
// will refer to as the "hello world" grammar. It is defined as
// follows:
//
// 1. The string may start or end with any number of spaces.
// 2. It must contain the two words 'hello' and 'world' in that
//    order.
// 3. The two words must have their first letters either both
//    lowercase or both uppercase.
// 4. Subsequent letters in each word must always be lowercase.
// 5. The second word may have an arbitrary number of exclamation
//    marks after it, but they must begin immediately after the
//    second word (no spaces).
// 6. The number of exclamation marks, if any, must be even.
// 7. The two words can be separated by spaces or by a comma. If
//    separated by a comma, spaces are optional after the comma.
//    If there is a comma, it must come immediately after the
//    first word.
// 8. The entire input string must be consumed.
//
// The following is a parsco parser that parses this grammar:

// This is a parser for the "hello world" grammar. See README
// file for explanation.
parser<string> parse_hello_world() {
  // Parse one char exactly, and it must be eith 'h' or 'H'. Our
  // grammar dictates that the "hello" can start with either a
  // lowercase of capital h. blanks eats up any space characters,
  // and the >> operator is a sequencing operator that will run
  // the first parser (blanks) ensuring that it succeeds, will
  // throw away the result, then will invoke the second parser,
  // again ensuring that it succeeds and returning its result.
  char h = co_await( blanks() >> one_of( "hH" ) );

  // But the rest of the letters in the word must be lowercase.
  // Parse the given string exactly.
  co_await str( "ello" );

  // By default this parsing framework does not backtrack on
  // failure. However, in cases where that is needed, any parser
  // can be wrapped in `try_' and it will allow failure in the
  // sense that the wrapped parser failing will not cause failure
  // of the parent parser and any input consumed by the parser
  // before it failed will be refunded to the input buffer so
  // that the next parser can try it.
  //
  // In this case, our grammar dictates that a comma may separate
  // the two words but that it is optional, so we use a try_.
  // Wrapping in try_ also has the effect of wrapping the return
  // type in a result_t<T>, which we can then test to see if the
  // parser succeeded.
  result_t<char> comma = co_await try_{ chr( ',' ) };

  // The next bit of grammar dictates that if there is a comma
  // then there does not have to be a space between the words,
  // otherwise there must be.
  if( comma.has_value() )
    co_await many( space );
  else
    co_await many1( space );

  // Our grammar rules say that the two words must have the same
  // capitalization.
  if( h == 'h' )
    co_await chr( 'w' );
  else
    co_await chr( 'W' );

  // The remainder of the word must always be lowercase.
  co_await str( "orld" );

  // Parse zero or more exclamation marks. Could also have
  // written many( chr, '!' ).
  string excls = co_await many( [] { return chr( '!' ); } );

  // Grammar says number of exclamation marks must be unique.
  if( excls.size() % 2 != 0 )
    co_await fail( "must have even # of !s" );

  // Eat blanks. We could have used the >> operator again to se-
  // quence this, or we can put it as its own statement.
  co_await blanks();

  // The `eof' parser fails if and only we have not consumed the
  // entire input.
  co_await eof();

  // This is optional, since we're just validating the input, but
  // it demonstrates how to return a result from the parser.
  co_return "Hello, World!";
}

/****************************************************************
** main
*****************************************************************/
int main( int, char** ) {
  // Some of these will succeed while others will fail.
  vector<string> tests{
      "Hello, World!!",      // should pass.
      "  hello , world!!  ", // should fail.
      "  hello, world!!!! ", // should pass.
      "  hello, world!!!  ", // should fail.
      "hEllo, World",        // should fail.
      "hello world",         // should pass.
      "HelloWorld",          // should fail.
      "hello,world",         // should pass.
      "hello, World",        // should fail.
      "hello, world!!!!!!",  // should pass.
      "hello, world !!!!",   // should fail.
      "hello, world ",       // should pass.
      "hello, world!! x",    // should fail.
  };

  using namespace parsco;

  for( string const& s : tests ) {
    result_t<string> hw =
        run_parser( "tests", s, parse_hello_world() );

    if( !hw ) {
      cout << "test \"" << s
           << "\" failed to parse: " << hw.get_error().what()
           << "\n";
      continue;
    }
    cout << "test \"" << s << "\" succeeded to parse.\n";
  }

  return 0;
}