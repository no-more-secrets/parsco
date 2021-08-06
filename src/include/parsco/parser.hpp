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
#include "parsco/compat.hpp"
#include "parsco/error.hpp"
#include "parsco/unique-coro.hpp"

// C++ standard library
#include <cassert>
#include <string>
#include <string_view>
#include <variant>

/****************************************************************
** Parser
*****************************************************************/
namespace parsco {

struct error;

template<typename T>
struct promise_type;

template<typename T = std::monostate>
struct [[nodiscard]] parser {
  using value_type  = T;
  using result_type = result_t<T>;

  parser( promise_type<T>* p )
    : promise_( p ),
      h_( coro::coroutine_handle<promise_type<T>>::from_promise(
          *promise_ ) ) {}

  void resume( std::string_view buffer ) {
    promise_->in_ = buffer;
    h_.h_.resume();
  }

  bool finished() const { return promise_->o_.has_value(); }

  bool is_good() const {
    return promise_->o_.has_value() && promise_->o_->has_value();
  }

  bool is_error() const {
    return promise_->o_.has_value() && promise_->o_->has_error();
  }

  std::string_view buffer() const { return promise_->buffer(); }

  result_type& result() {
    assert( finished() );
    return *promise_->o_;
  }

  result_type const& result() const {
    assert( finished() );
    return *promise_->o_;
  }

  int consumed() const { return promise_->consumed_; }

  int farthest() const { return promise_->farthest_; }

  T& get() {
    assert( is_good() );
    return **promise_->o_;
  }

  T const& get() const {
    assert( is_good() );
    return **promise_->o_;
  }

  parsco::error& error() {
    assert( is_error() );
    return promise_->o_->error();
  }

  parsco::error const& error() const {
    assert( is_error() );
    return promise_->o_->error();
  }

  // This would be try anyway because of the unique_coro.
  parser( parser const& ) = delete;
  parser& operator=( parser const& ) = delete;

  parser( parser&& ) = default;
  parser& operator=( parser&& ) = default;

private:
  promise_type<T>* promise_;
  unique_coro      h_;
};

} // namespace parsco

namespace std {

template<typename T, typename... Args>
struct coroutine_traits<::parsco::parser<T>, Args...> {
  using promise_type = ::parsco::promise_type<T>;
};

} // namespace std