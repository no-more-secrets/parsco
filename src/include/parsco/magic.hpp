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

#include "parsco/concepts.hpp"
#include "parsco/error.hpp"

// C++ standard library
#include <optional>
#include <string_view>
#include <variant>

#define PROMISE_BUILTIN( name )                  \
  struct builtin_##name {                        \
    using value_type = std::string_view;         \
    std::optional<BuiltinParseResult> try_parse( \
        std::string_view in ) const;             \
  }

/****************************************************************
** "Magic" Parser Awaitables
*****************************************************************/
// The types in this section are recognized specially by the
// promise type when awaited on, and have special access to the
// insides of the promise. This is for the purpose of providing
// some basic combinators as a starting point as well as im-
// proving performance (a builtin version of a parser will be
// much faster than one implemented in terms of primitives).
namespace parsco {

/****************************************************************
** Builtin Wrappers
*****************************************************************/
// When a parser is wrapped in this object it will return a may-
// be<T> instead of a T and it will thus be allowed to fail
// without failing the entire parsing operation.
template<Parser P>
struct try_ {
  using value_type = std::optional<typename P::value_type>;
  try_( P&& p_ ) : p( std::move( p_ ) ) {}
  P p;
};

// This will immediately fail the parser.
struct [[nodiscard]] fail_wrapper {
  using value_type = std::monostate;
  fail_wrapper()   = default;
  fail_wrapper( std::string_view msg ) : err( msg ) {}
  fail_wrapper( error&& e ) : err( std::move( e ) ) {}
  error err;
};

// This allows us to mark the fail_wrapper as [[nodiscard]] so
// that the compiler will warn us when we don't co_await on it.
template<typename... Ts>
inline fail_wrapper fail( Ts&&... os ) {
  return fail_wrapper( std::forward<Ts>( os )... );
}

/****************************************************************
** Builtin Parsers
*****************************************************************/
// Note that these all return string_views into the buffer.
struct BuiltinParseResult {
  std::string_view sv;
  // This may be different from the length of sv, e.g. if we are
  // parsing a quoted string "hello" and we want to return only
  // hello.
  int consumed;
};

// This gets the next character from the buffer and fails if
// there are no more characters.
struct builtin_next_char {
  using value_type = char;
};

PROMISE_BUILTIN( blanks );
PROMISE_BUILTIN( identifier );
PROMISE_BUILTIN( single_quoted );
PROMISE_BUILTIN( double_quoted );

} // namespace parsco
