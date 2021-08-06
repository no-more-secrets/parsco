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
#include "parsco/promise.hpp"

// C++ standard library
#include <memory>
#include <optional>
#include <variant>

namespace parsco {

/****************************************************************
** unique_ptr
*****************************************************************/
// We have this struct helper to work around a weird clang issue
// where it doesn't like function templates that are coroutines.
template<typename Lang, typename T>
struct ParserForUniquePtr {
  parser<std::unique_ptr<T>> operator()() const {
    co_return std::make_unique<T>( co_await parse<Lang, T>() );
  }
};

template<typename Lang, typename T>
inline constexpr ParserForUniquePtr<Lang, T> unique_ptr_parser{};

template<typename Lang, typename T>
parser<std::unique_ptr<T>> parser_for(
    lang<Lang>, tag<std::unique_ptr<T>> ) {
  return unique_ptr_parser<Lang, T>();
}

/****************************************************************
** variant
*****************************************************************/
template<typename Lang>
struct VariantParser {
  template<typename... Args>
  parser<std::variant<Args...>> operator()(
      tag<std::variant<Args...>> ) const {
    using res_t = std::variant<Args...>;
    std::optional<res_t> res;

    auto one = [&]<typename Alt>( Alt* ) -> parser<> {
      if( res.has_value() ) co_return;
      auto exp = co_await try_{ parse<Lang, Alt>() };
      if( !exp ) co_return;
      res.emplace( std::move( *exp ) );
    };
    ( co_await one( (Args*)nullptr ), ... );

    if( !res ) co_await fail();
    co_return std::move( *res );
  }
};

template<typename Lang>
inline constexpr VariantParser<Lang> variant_parser{};

template<typename Lang, typename... Args>
parser<std::variant<Args...>> parser_for(
    lang<Lang>, tag<std::variant<Args...>> t ) {
  return variant_parser<Lang>( t );
}

} // namespace parsco
