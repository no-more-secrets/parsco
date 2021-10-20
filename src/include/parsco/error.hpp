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

// C++ standard library
#include <optional>
#include <string>
#include <string_view>

namespace parsco {

/****************************************************************
** Parser Error/Result
*****************************************************************/
struct error {
  explicit error() : msg_( "" ) {}
  explicit error( std::string_view m ) : msg_( m ) {}

  template<typename... Args>
  explicit error( std::string_view m ) : msg_( m ) {}

  std::string const& what() const { return msg_; }

  operator std::string() const { return msg_; }

private:
  std::string msg_;
};

// Until we have std::expected<T>...
template<typename T>
struct result_t {
  // clang-format off
  template<typename U>
  requires( !std::is_convertible_v<U, error> )
  result_t( U&& u ) : val( std::forward<U>( u ) ) {}
  // clang-format on

  result_t( error const& e ) : err( e ) {}

  template<typename... Ts>
  void emplace( Ts&&... ts ) {
    val.emplace( std::forward<Ts>( ts )... );
  }

  bool has_value() const { return val.has_value(); }
  bool has_error() const { return !val.has_value(); }

  explicit operator bool() const { return val.has_value(); }

  error const& get_error() const { return err; }
  error&       get_error() { return err; }

  T&       operator*() { return *val; }
  T const& operator*() const { return *val; }

  T*       operator->() { return &*val; }
  T const* operator->() const { return &*val; }

  std::optional<T> val;
  // If there is no val, there is an error.
  struct error err;
};

// Helper for translating character position in buffer to
// line/col for error messages.
struct ErrorPos {
  static ErrorPos from_index( std::string_view in, int idx );
  int             line;
  int             col;
};

} // namespace parsco