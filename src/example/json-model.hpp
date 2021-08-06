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
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// This file contains a rudimentary JSON model just for illus-
// trating how to use the parser.
namespace json {

// Type tag representing the language we are parsing.
struct Json {};

struct table;
struct list;

struct string_val {
  string_val() = default;
  string_val( std::string_view s ) : val( s ) {}
  string_val( std::string s ) : val( std::move( s ) ) {}
  std::string val;
};

struct number {
  number() = default;
  number( int n ) : val( n ) {}
  number( double d ) : val( d ) {}
  number( std::variant<int, double> v ) : val( v ) {}
  std::variant<int, double> val;
};

struct boolean {
  boolean() = default;
  boolean( bool b_ ) : b( b_ ) {}
  bool b;
};

// The order of these in the variant matters, as parsing will be
// attempted in order, and the first one that succeeds will be
// chosen.  This behavior is dictated by the default variant
// parser.
using value =
    std::variant<number, string_val, boolean,
                 std::unique_ptr<table>, std::unique_ptr<list>>;

struct key_val {
  key_val() = default;
  key_val( std::string k_, value v_ )
    : k( std::move( k_ ) ), v( std::move( v_ ) ) {}
  std::string k;
  value       v;
};

struct table {
  table() = default;
  table( std::vector<key_val>&& m )
    : members( std::move( m ) ) {}

  std::vector<key_val> members{};

  std::string pretty_print( std::string_view indent = "" ) const;
};

struct list {
  list() = default;
  list( std::vector<value>&& l ) : members( std::move( l ) ) {}
  std::vector<value> members;

  std::string pretty_print( std::string_view indent = "" ) const;
};

struct doc {
  doc() = default;
  doc( table&& tbl_ ) : tbl( std::move( tbl_ ) ) {}
  table tbl;
};

} // namespace json