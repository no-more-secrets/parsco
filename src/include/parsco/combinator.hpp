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
#include "parsco/ext.hpp"
#include "parsco/parser.hpp"
#include "parsco/promise.hpp"

// C++ standard library
#include <cassert>
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>

// NOTE: In this module, what would otherwise be a collection of
// function templates, has in fact been implemented as niebloids.
// This is because clang seems to have an aversion to function
// templates that are coroutines.  So when that issue is fixed
// we can convert them back to function templates.

/****************************************************************
** Macros
*****************************************************************/
#define PZ_LAMBDA( name, ... ) \
  name( [&] { return __VA_ARGS__; } )

#define many_L( ... ) PZ_LAMBDA( many, __VA_ARGS__ )
#define many1_L( ... ) PZ_LAMBDA( many1, __VA_ARGS__ )

namespace parsco {

// When a parser is run in a repitition, this will look at the
// value type of the parser and decide what container is best to
// return the results in.
template<typename T>
using many_result_container_t =
    std::conditional_t<std::is_same_v<T, char>, std::string,
                       std::vector<T>>;

/****************************************************************
** Primitives
*****************************************************************/
// Consumes a char that must be c, otherwise it fails.
parser<char> chr( char c );

// Consumes any char, fails at eof.
parser<char> any_chr();

struct Ret {
  template<typename T>
  // Take o by value for lifetime reasons. Specifically, this
  // combinator will typically be used in a function where we may
  // just want to do a `return' instead of co_await + co_return,
  // and in that case if we were to take this argument by refer-
  // ence it would go out of scope.
  parser<T> operator()( T o ) const {
    co_return std::move( o );
  }
};

inline constexpr Ret ret{};

/****************************************************************
** Character Classes
*****************************************************************/
// Consumes one space (' ');
parser<> space();
// Either CR or LF.
parser<> crlf();
// '\t'
parser<> tab();
// One of any of the space/blank characters.
parser<> blank();

// Consumes one digit [0-9] char or fails.
parser<char> digit();

parser<char> lower();
parser<char> upper();
parser<char> alpha();
parser<char> alphanum();

// Consumes one char if it is one of the ones in sv.
parser<char> one_of( std::string_view sv );
parser<char> not_of( std::string_view sv );

/****************************************************************
** Strings
*****************************************************************/
// Attempts to consume the exact string, and fails otherwise.
parser<> str( std::string_view sv );

parser<std::string> identifier();

// Consumes blank spaces.
parser<> blanks();

// Parses "..." or '...' and returns the stuff inside, which
// cannot contain newlines. Note that these return string views
// into the buffer because they are implemented using primitives.
parser<std::string_view> double_quoted_str();
parser<std::string_view> single_quoted_str();
// Allows either double or single quotes.
parser<std::string> quoted_str();

/****************************************************************
** Miscellaneous
*****************************************************************/
// Succeeds if the input stream is finished.
parser<> eof();

/****************************************************************
** pred
*****************************************************************/
// Parses a single character for which the predicate returns
// true, fails otherwise.
struct Pred {
  // Need to take Func by value so that it stays around when this
  // function suspends.
  template<typename Func>
  parser<char> operator()( Func f ) const {
    char next = co_await builtin_next_char{};
    if( !f( next ) ) co_await fail();
    co_return next;
  }
};

inline constexpr Pred pred{};

/****************************************************************
** many
*****************************************************************/
// Parses zero or more of the given parser.
struct Many {
  // This is a struct instead of a function to work around a
  // clang issue where it doesn't like coroutine function tem-
  // plates.  Take args by value for lifetime reasons.
  template<typename Func, typename... Args>
  auto operator()( Func f, Args... args ) const -> parser<
      many_result_container_t<typename std::invoke_result_t<
          Func, Args...>::value_type>> {
    using res_t =
        typename std::invoke_result_t<Func, Args...>::value_type;
    many_result_container_t<res_t> res;
    while( true ) {
      auto m = co_await try_{ f( std::move( args )... ) };
      if( !m ) break;
      res.push_back( std::move( *m ) );
    }
    co_return res;
  }
};

inline constexpr Many many{};

/****************************************************************
** many_exhuast
*****************************************************************/
// This runs a `many' combinator, but one that expects to consume
// all of the input. The reason this is better than combining the
// `many' combinator with the `exhaust' combinator is because the
// `many_exhaust' combinator knows what it failed to parse if it
// fails to parse the entire input and so can produce better
// error messages.
//
// You should call this whenever you need to consume an entire
// input (or remainder of input) with a single repeating parser.
struct ManyExhaust {
  template<typename Func, typename... Args>
  auto operator()( Func f, Args... args ) const -> parser<
      many_result_container_t<typename std::invoke_result_t<
          Func, Args...>::value_type>> {
    auto res = try_{ exaust( many( f, args... ) ) };
    if( res.has_value() ) co_return std::move( *res );
    // We've failed to consume the entire input, so now run the
    // element parser once more (knowing that it will fail) so
    // that it can give us the location of failure.
    (void)co_await f( args... );
    // Should not be here...
    assert( false );
  }
};

inline constexpr ManyExhaust many_exhaust{};

/****************************************************************
** many_type
*****************************************************************/
// Parses zero or more of the given type.
template<typename Lang, typename T>
struct ManyType {
  auto operator()() const -> parser<std::vector<T>> {
    return many( []() { return parse<Lang, T>(); } );
  }
};

template<typename Lang, typename T>
inline constexpr ManyType<Lang, T> many_type{};

/****************************************************************
** many1
*****************************************************************/
// Parses one or more of the given parser.
struct Many1 {
  // This is a struct instead of a function to work around a
  // clang issue where it doesn't like coroutine function tem-
  // plates.
  template<typename Func, typename... Args>
  auto operator()( Func f, Args... args ) const -> parser<
      many_result_container_t<typename std::invoke_result_t<
          Func, Args...>::value_type>> {
    using res_t =
        typename std::invoke_result_t<Func, Args...>::value_type;
    many_result_container_t<res_t> res =
        co_await many( std::move( f ), std::move( args )... );
    if( res.empty() ) co_await fail();
    co_return res;
  }
};

inline constexpr Many1 many1{};

/****************************************************************
** seq
*****************************************************************/
// Runs multiple parsers in sequence, and only succeeds if all of
// them succeed. Returns all results in a tuple.
struct Seq {
  template<typename... Parsers>
  parser<std::tuple<typename Parsers::value_type...>> operator()(
      Parsers... ps ) const {
    co_return std::tuple<typename Parsers::value_type...>{
        co_await std::move( ps )... };
  }
};

inline constexpr Seq seq{};

/****************************************************************
** invoke
*****************************************************************/
// Calls the given function with the results of the parsers as
// arguments (which must all succeed).
//
// NOTE: the parsers are guaranteed to be run in the order they
// appear in the parameter list, and that is one of the benefits
// of using this helper.
struct Invoke {
  // Take func by value for lifetime reasons.
  // clang-format off
  template<size_t... Idx, typename Func, typename... Parsers>
  requires( std::is_default_constructible_v<
                typename Parsers::value_type> && ... )
  parser<std::invoke_result_t<Func,
                              typename Parsers::value_type...>>
  run( std::index_sequence<Idx...>, Func func,
       Parsers... ps ) const {
    // clang-format on
    // We can't simply do the following:
    //
    //   co_return T( co_await std::move( ps )... );
    //
    // because then we'd be depending on the order of evaluation
    // of function arguments. So we have to use a fold expression
    // with the comma operator, which apparently guarantees eval-
    // uation order, see:
    //
    //   https://stackoverflow.com/questions/46056268/
    //                order-of-evaluation-for-fold-expressions
    //
    auto ps_tpl = std::tuple{ std::move( ps )... };
    std::tuple<typename Parsers::value_type...> res_tpl;

    auto run_parser =
        [&]<size_t I>(
            std::integral_constant<size_t, I> ) -> parser<> {
      std::get<I>( res_tpl ) =
          co_await std::move( std::get<I>( ps_tpl ) );
    };
    // Here is the fold expression whose order of evaluation is
    // supposed to be well-defined because we are using the comma
    // operator.
    ( co_await run_parser(
          std::integral_constant<size_t, Idx>{} ),
      ... );

    co_return std::apply( func, std::move( res_tpl ) );
  }

  template<typename Func, typename... Parsers>
  auto operator()( Func&& func, Parsers... ps ) const {
    return run( std::index_sequence_for<Parsers...>(),
                std::forward<Func>( func ), std::move( ps )... );
  }
};

inline constexpr Invoke invoke{};

/****************************************************************
** emplace
*****************************************************************/
// Calls the constructor of the given type with the results of
// the parsers as arguments (which must all succeed).
template<typename T>
struct Emplace {
  template<typename... Parsers>
  parser<T> operator()( Parsers... ps ) const {
    auto applier = [&]( auto&&... args ) {
      return T( std::forward<decltype( args )>( args )... );
    };
    return invoke( applier, std::move( ps )... );
  }
};

template<typename T>
inline constexpr Emplace<T> emplace{};

/****************************************************************
** seq_last
*****************************************************************/
namespace detail {

template<typename T>
struct select_tag {
  using type = T;
};

// Use a fold-expression to fold the comma operator over the pa-
// rameter pack.
template<typename... Ts>
using select_last_t =
    typename decltype( ( detail::select_tag<Ts>{}, ... ) )::type;

} // namespace detail

// Runs multiple parsers in sequence, and only succeeds if all of
// them succeed. Returns last result.
struct SeqLast {
  template<typename... Parsers>
  parser<typename detail::select_last_t<Parsers...>::value_type>
  operator()( Parsers... ps ) const {
    using ret_t =
        typename detail::select_last_t<Parsers...>::value_type;
    if constexpr( std::is_same_v<ret_t, std::monostate> )
      ( co_await std::move( ps ), ... );
    else
      co_return( co_await std::move( ps ), ... );
  }
};

inline constexpr SeqLast seq_last{};

/****************************************************************
** seq_first
*****************************************************************/
// Runs multiple parsers in sequence, and only succeeds if all of
// them succeed. Returns first result.
struct SeqFirst {
  template<typename Parser, typename... Parsers>
  parser<typename Parser::value_type> operator()(
      Parser fst, Parsers... ps ) const {
    auto res = co_await std::move( fst );
    ( (void)co_await std::move( ps ), ... );
    co_return res;
  }
};

inline constexpr SeqFirst seq_first{};

/****************************************************************
** on_error
*****************************************************************/
struct OnError {
  template<Parser P>
  P operator()( P p, std::string_view msg ) const {
    auto res = co_await try_{ std::move( p ) };
    if( res.has_value() ) co_return *res;
    co_await fail( msg );
    __builtin_unreachable();
  }
};

inline constexpr OnError on_error{};

/****************************************************************
** diagnose
*****************************************************************/
// This will run the first parser, and if it fails then this
// parser fails. But if the first parser succeeds but does not
// consume all of the input, then it will run the second parser
// requiring that it succeeds (but discarding the result). Ide-
// ally that second parser is chosen so as to fail in this situa-
// tion and produce a good error message at the right location.
// In other words, the second parser is the parser that is ex-
// pected to succeed when there is more input to be parsed. We
// need to supply this since the framework cannot guess what it
// is. In the event that the second parser does succeed, this
// parser will still fail in that case, but with a more generic
// error message.
struct Diagnose {
  template<Parser P1, Parser P2>
  P1 operator()( P1 p1, P2 expected ) const {
    auto res = co_await std::move( p1 );
    // Parser has succeeded, now test EOF.
    if( co_await try_{ eof() } )
      // We have consumed all input, so we're good.
      co_return res;
    // Still input remaining.
    (void)co_await std::move( expected );
    // p2 succeeded but ideally shouldn't have, so we'll just
    // fail it manually.
    co_await fail(
        "parsing partially succeeded but was not able to "
        "consume all input." );
    __builtin_unreachable();
  }
};

inline constexpr Diagnose diagnose{};

/****************************************************************
** exhaust
*****************************************************************/
// Runs the given parser and then checks that the input buffers
// has been exhausted (if not, it fails). Returns the result from
// the parser.
struct Exhaust {
  template<typename T>
  parser<T> operator()( parser<T> p ) const {
    T res = co_await std::move( p );
    co_await eof();
    co_return res;
  }
};

inline constexpr Exhaust exhaust{};

/****************************************************************
** unwrap
*****************************************************************/
// This does not do any parsing, it just takes a defererenceable
// object and tries to unwrap it, and if it can't, then it will
// fail the parser.
struct Unwrap {
  template<typename T>
  parser<std::remove_cvref_t<decltype( *std::declval<T>() )>>
  operator()( T o ) const {
    if( !o ) co_await fail( o.error().what() );
    co_return *std::move( o );
  }

  template<typename T>
  parser<std::remove_cvref_t<decltype( *std::declval<T>() )>>
  operator()( parser<T> p ) const {
    co_return co_await ( *this )( co_await std::move( p ) );
  }
};

inline constexpr Unwrap unwrap{};

/****************************************************************
** bracketed
*****************************************************************/
// Runs the parser p between characters l and r.
struct Bracketed {
  template<typename T>
  parser<T> operator()( char l, parser<T> p, char r ) const {
    co_await chr( l );
    T res = co_await std::move( p );
    co_await chr( r );
    co_return res;
  }

  template<typename L, typename R, typename T>
  parser<T> operator()( parser<L> l, parser<T> p,
                        parser<R> r ) const {
    (void)co_await std::move( l );
    T res = co_await std::move( p );
    (void)co_await std::move( r );
    co_return std::move( res );
  }
};

inline constexpr Bracketed bracketed{};

/****************************************************************
** try_ignore
*****************************************************************/
// Try the parse but ignore the result if it succeeds.
struct TryIgnore {
  template<Parser P>
  parser<> operator()( P p ) const {
    (void)co_await try_{ std::move( p ) };
  }
};

inline constexpr TryIgnore try_ignore{};

/****************************************************************
** fmap
*****************************************************************/
// Runs the parser p and applies the given function to the re-
// sult, if successful.
struct Fmap {
  // clang-format off
  template<Parser P, typename Func>
    requires( std::is_invocable_v<Func, typename P::value_type> )
  parser<std::invoke_result_t<Func, typename P::value_type>>
  operator()( Func f, P p ) const {
    // clang-format on
    co_return f( co_await std::move( p ) );
  }
};

inline constexpr Fmap fmap{};

/****************************************************************
** First
*****************************************************************/
// Runs the parsers in sequence until the first one succeeds,
// then returns its result (all of the parsers must return the
// same result type). If none of them succeed then the parser
// fails.
struct First {
  // clang-format off
  template<typename P, typename... Ps>
  requires( std::is_same_v<typename P::value_type,
                           typename Ps::value_type> && ...)
  parser<typename P::value_type> operator()( P fst,
                                             Ps... rest ) const {
    // clang-format on
    using res_t = typename P::value_type;
    std::optional<res_t> res;

    auto one = [&]<typename T>( parser<T> p ) -> parser<> {
      if( res.has_value() ) co_return;
      auto exp = co_await try_{ std::move( p ) };
      if( !exp.has_value() ) co_return;
      res.emplace( std::move( *exp ) );
    };
    co_await one( std::move( fst ) );
    ( co_await one( std::move( rest ) ), ... );

    if( !res.has_value() ) co_await fail();
    co_return std::move( *res );
  }
};

inline constexpr First first{};

/****************************************************************
** interleave_first
*****************************************************************/
// Parses: g f g f g f
// and returns the f's.
struct InterleaveFirst {
  template<typename F, typename G>
  // Take functions by value for lifetime reasons.
  auto operator()( F f, G g, bool sep_required = true ) const
      -> parser<many_result_container_t<
          typename std::invoke_result_t<F>::value_type>> {
    if( sep_required )
      co_return co_await many(
          [&] { return seq_last( g(), f() ); } );
    else
      co_return co_await many(
          [&] { return seq_first( try_{ g() }, f() ); } );
  }
};

inline constexpr InterleaveFirst interleave_first{};

/****************************************************************
** interleave_last
*****************************************************************/
// Parses: f g f g f g
// and returns the f's.
struct InterleaveLast {
  template<typename F, typename G>
  // Take functions by value for lifetime reasons.
  auto operator()( F f, G g, bool sep_required = true ) const
      -> parser<many_result_container_t<
          typename std::invoke_result_t<F>::value_type>> {
    if( sep_required )
      co_return co_await many(
          [&] { return seq_first( f(), g() ); } );
    else
      co_return co_await many(
          [&] { return seq_first( f(), try_{ g() } ); } );
  }
};

inline constexpr InterleaveLast interleave_last{};

/****************************************************************
** interleave
*****************************************************************/
// Parses: f g f g f
// and returns the f's.
struct Interleave {
  template<typename F, typename G>
  // Take functions by value for lifetime reasons.
  auto operator()( F f, G g, bool sep_required = true ) const
      -> parser<many_result_container_t<
          typename std::invoke_result_t<F>::value_type>> {
    auto container =
        co_await interleave_last( f, g, sep_required );
    if( sep_required )
      // If the separator was not required then the above call to
      // interleave_last will have already picked up the last
      // f(). If the separate is required then the above will
      // have either parsed nothing or will have ended by parsing
      // a separator, in which case we need to parse one more f.
      container.push_back( co_await f() );
    co_return container;
  }
};

inline constexpr Interleave interleave{};

/****************************************************************
** cat
*****************************************************************/
// Runs multiple string-yielding parsers in sequence and concate-
// nates the results into one string.
struct Cat {
  template<typename... Parsers>
  parser<std::string> operator()( Parsers... ps ) const {
    std::string res;
    ( ( res += co_await std::move( ps ) ), ... );
    co_return res;
  }
};

inline constexpr Cat cat{};

/****************************************************************
** Haskell-like sequencing operator
*****************************************************************/
// Run the parsers in sequence (all must succeed) and return the
// result of the final one.
template<Parser T, Parser U>
parser<typename U::value_type> operator>>( T l, U r ) {
  return seq_last( std::move( l ), std::move( r ) );
}

// Run the parsers in sequence (all must succeed) and return the
// result of the first one.
template<Parser T, Parser U>
parser<typename T::value_type> operator<<( T l, U r ) {
  return seq_first( std::move( l ), std::move( r ) );
}

template<Parser T, Parser U>
parser<typename U::value_type> operator|( T l, U r ) {
  return first( std::move( l ), std::move( r ) );
}

} // namespace parsco
