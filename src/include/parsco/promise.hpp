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
#include "parsco/magic.hpp"
#include "parsco/parser.hpp"

// C++ standard library
#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

/****************************************************************
** Macros
*****************************************************************/
#define BUILTIN_PARSER( name, err )                            \
  auto await_transform( builtin_##name const& o ) noexcept {   \
    return builtin_awaitable{ this, o.try_parse( in_ ), err }; \
  }

/****************************************************************
** Parser Promise
*****************************************************************/
namespace parsco {

namespace detail {

// We put the return_value and return_void in these two structs
// so that we can decide based on the type of T which one to in-
// clude (we are only allowed to have one in a promise type).
// When T is std::monostate we want the void one, otherwise we
// want the value one.
//
// Having these will also allow us to automatically support im-
// plicit conversion, since the below take U as an argument, but
// the return_value_ functions are strictly T.
template<typename Derived, typename T>
struct promise_value_returner {
  template<typename U>
  void return_value( U const& val ) {
    static_cast<Derived&>( *this ).return_value_( val );
  }

  template<typename U>
  void return_value( U&& val ) {
    static_cast<Derived&>( *this ).return_value_(
        std::move( val ) );
  }
};

template<typename Derived>
struct promise_void_returner {
  void return_void() {
    static_cast<Derived&>( *this ).return_void_();
  }
};

} // namespace detail

template<typename T>
struct promise_type
  : std::conditional_t<
        std::is_same_v<T, std::monostate>,
        detail::promise_void_returner<promise_type<T>>,
        detail::promise_value_returner<promise_type<T>, T>> {
  std::optional<result_t<T>> o_;
  std::string_view           in_       = "";
  int                        consumed_ = 0;
  int                        farthest_ = 0;

  promise_type() = default;

  std::string_view& buffer() { return in_; }

  // Always suspend initially because the coroutine is unable to
  // do anything immediately because it does not have access to
  // the buffer to be parsed. It only gets that when it is
  // awaited upon.
  auto initial_suspend() const {
    return parsco::suspend_always{};
  }

  auto final_suspend() const noexcept {
    // Always suspend because the parser object owns the corou-
    // tine and will be the one to destroy it when it goes out of
    // scope.
    return parsco::suspend_always{};
  }

  // Ensure that this is not copyable. See
  // https://devblogs.microsoft.com/oldnewthing/20210504-00/?p=105176
  // for the arcane reason as to why.
  promise_type( promise_type const& ) = delete;
  void operator=( promise_type const& ) = delete;

  auto get_return_object() { return parser<T>( this ); }

  void unhandled_exception() {
    // Should not be here; we are not using exceptions in this
    // library, although this library could have been designed
    // to propagate errors using exceptions.  C++20 has good
    // support for exceptions.
    assert( false );
  }

  // Note this ends in an underscore; the real return_value, if
  // present, is provided by a base class so that we can control
  // when it appears.
  void return_value_( T&& val ) {
    assert( !o_ );
    o_.emplace( std::move( val ) );
  }

  void return_value_( T const& val ) {
    assert( !o_ );
    o_.emplace( val );
  }

  // Note this ends in an underscore; the real return_void, if
  // present, is provided by a base class so that we can control
  // when it appears.
  void return_void_() {
    assert( !o_ );
    o_.emplace( T{} );
  }

  auto await_transform( fail_wrapper const& f ) noexcept {
    o_ = f.err;
    return parsco::suspend_always{};
  }

  template<typename U>
  struct awaitable {
    parser<U>     parser_;
    promise_type* promise_;

    // Promise pointer so that template type can be inferred.
    awaitable( promise_type* p, parser<U> parser )
      : parser_( std::move( parser ) ), promise_( p ) {}

    bool await_ready() noexcept {
      // At this point parser should be at its initial suspend
      // point, waiting to be run.
      assert( !parser_.finished() );
      // Now we need to give the parser's promise object the
      // buffer to be parsed.
      parser_.resume( promise_->buffer() );
      // parser should be at its final suspend point now, waiting
      // to be destroyed by the parser object that owns it.
      assert( parser_.finished() );
      promise_->farthest_ =
          std::max( promise_->farthest_,
                    promise_->consumed_ + parser_.farthest() );
      return parser_.is_good();
    }

    void await_suspend( coro::coroutine_handle<> ) noexcept {
      // A parse failed.
      promise_->o_.emplace( std::move( parser_.error() ) );
    }

    U await_resume() {
      assert( parser_.is_good() );
      int chars_consumed =
          promise_->buffer().size() - parser_.buffer().size();
      assert( chars_consumed >= 0 );
      assert( promise_->buffer().size() >=
              size_t( chars_consumed ) );
      // We only consume chars upon success.
      promise_->buffer().remove_prefix( chars_consumed );
      promise_->consumed_ += chars_consumed;
      return std::move( parser_.get() );
    }
  };

  template<typename U>
  auto await_transform( parser<U> p ) noexcept {
    return awaitable<U>( this, std::move( p ) );
  }

  // This parser is allowed to fail.
  template<typename P>
  auto await_transform( try_<P> t ) noexcept {
    using U = typename P::value_type;
    // Slight modification to awaitable to allow it to fail.
    struct tryable_awaitable : awaitable<U> {
      using Base = awaitable<U>;
      using Base::Base;
      using Base::parser_;
      using Base::promise_;

      bool await_ready() noexcept {
        Base::await_ready();
        // This is a parser that is allowed to fail, so always
        // return true;
        return true;
      }

      result_t<U> await_resume() {
        if( parser_.is_error() ) return parser_.error();
        return Base::await_resume();
      }
    };
    return tryable_awaitable( this, std::move( t.p ) );
  }

  auto await_transform( builtin_next_char ) noexcept {
    struct awaitable {
      promise_type* p_;

      bool await_ready() noexcept {
        return ( p_->buffer().size() > 0 );
      }

      void await_suspend( coro::coroutine_handle<> ) noexcept {
        p_->o_.emplace( error( "EOF" ) );
      }

      char await_resume() noexcept {
        assert( int( p_->buffer().size() ) >= 1 );
        char res = p_->buffer()[0];
        p_->buffer().remove_prefix( 1 );
        p_->consumed_++;
        p_->farthest_ = p_->consumed_;
        return res;
      }
    };
    return awaitable{ this };
  }

  struct builtin_awaitable {
    promise_type*                     p_;
    std::optional<BuiltinParseResult> res_;
    std::string_view                  err_;

    constexpr bool await_ready() noexcept {
      if( !res_ ) return false;
      std::string_view& buf      = p_->buffer();
      int               consumed = res_->consumed;
      assert( consumed >= 0 );
      buf.remove_prefix( consumed );
      p_->consumed_ += consumed;
      p_->farthest_ = p_->consumed_;
      return true;
    }

    void await_suspend( coro::coroutine_handle<> ) noexcept {
      p_->o_.emplace(
          error( "expected " + std::string( err_ ) ) );
    }

    std::string_view await_resume() noexcept { return res_->sv; }
  };

  // These are the special builtin parsers that have special ac-
  // cess to the internals of the coroutine state (meaning, the
  // buffer). They are used for two reasons: to form the primi-
  // tive building blocks and to implement higher-level (but
  // commonly-used) parsers more efficiently.
  BUILTIN_PARSER( blanks, "blanks" );
  BUILTIN_PARSER( identifier, "identifier" );
  BUILTIN_PARSER( single_quoted, "single-quoted string" );
  BUILTIN_PARSER( double_quoted, "double-quoted string" );
};

} // namespace parsco
