parsco
------
C++20 Coroutine Synchronous Parser Combinator Library.

This library contains a monadic parser type and associated
combinators that can be composed to create parsers using
C++20 Coroutines.

What is a Coroutine Combinator
==============================
A parser combinator is a function (or higher-order function)
that acts as a simple building block for creating complex parsers.
This library provides a parser object and combiantors that
can be combined using C++20 Coroutines.

For example, let's say that you want to use
the `chr` parser, which parses a single specific character,
to build a parser that parses any two-character
string where both characters are the same (e.g. "==", "aa") and
then returns that character on success. You would do that like
this:
```cpp
parsco::parser<char> parse_two_same() {
  char c = co_await parsco::any_chr();
  co_await parsco::chr( c );
  co_return c;
}

// Another way to implement it would be:
parsco::parser<char> parse_two_same() {
  char c1 = co_await parsco::any_chr();
  char c2 = co_await parsco::any_chr();
  if( c1 != c2 )
    // Provides better error message.
    co_await parsco::fail( "expected two equal chars" );
  co_return c;
}
```
The combinators used above (`chr` and `any_chr`) are functions
that return `parser<T>`s.  As can be seen, they have been
combined to yield higher level functions (`parse_two_same`)
that also return `parser<T>`, and thus can be used in more
complex parsers in a similar way, namely by calling them,
getting the `parser<t>`s that they return, and then using
the C++20 `co_await` keyword to run them and get the result.

When you `co_await` on a parser it does a few things for you:

1. It automatically runs the parser and if the parser fails,
it will return early from the coroutine and report failure to
the parent (caller) coroutine.  The exception to this is if
you wrap the parser in the `try_` combinator (see below), but
this isn't often necessary.
2. It will automatically detect EOF, which normally results
in a failure.
3. It will handle keeping track of the current position in the
input buffer as well as the location of parsing errors if any.

The Hello World Parser
----------------------

As a first example, let us define a simple grammar that we will refer
to as the "hello world" grammar.  It is defined as follows:

1. The string may start or end with any number of spaces.
2. It must contain the two words 'hello' and 'world' in that order.
3. The two works must have their first letters either both lowercase
or both uppercase.
4. Subsequent letters in each word must always be lowercase.
5. The second word may have an arbitrary number of exclamation marks
after it, but they must begin right after the second word.

The following is a parsco parser that parses this grammar:

```cpp
parser<string> parse_hello_world() {
  // Parse blanks and then either h/H.
  char h = co_await( blanks() >> one_of( "hH" ) );

  // But the rest of the letters in the word must be lowercase.
  // Parse the given string exactly.
  co_await str( "ello" );

  // Comma is optional, but if present must follow the first word.
  result_t<char> comma = co_await try_{ chr( ',' ) };

  // If there is a comma then there does not have to be a space
  // between the words, otherwise there must be.
  if( comma.has_value() )
    co_await many( space );
  else
    co_await many1( space );

  // The two words must have the same capitalization.
  char w;
  if( h == 'h' )
    w = co_await chr( 'w' );
  else
    w = co_await chr( 'W' );

  // The remainder of the word must always be lowercase.
  co_await str( "orld" );

  // Parse zero or more exclamation marks.
  co_await many( [] { return chr( '!' ); } );

  // Eat blanks. We could have used the >> operator again to se-
  // quence this, or we can put it as its own statement.
  co_await blanks();

  // The `eof' parser fails if and only we have not consumed the
  // entire input.
  co_await eof();

  // If we've arrived here then the parse has succeeded, so re-
  // turn a normalized version of what we parsed.
  co_return "Hello, World!";
}
```

See the file `hello-world-parser.cpp` in the examples folder for
a demo of calling this function on some test strings.  Here is
an example of running it:
```cpp
// This should conform to the above grammar.
std::string_view test = "  hello, world!!!  ";

parsco::result_t<string> hw =
    parsco::run_parser( "input", s, parse_hello_world() );

if( !hw ) {
  cout << "test \"" << s
       << "\" failed to parse; error message: "
       << hw.get_error().what() << "\n";
} else {
  cout << "test \"" << s << "\" succeeded to parse.\n";
}
```

JSON Parser
-----------
To see a more realistic example, see the `json-parser.cpp` file
in the examples folder which contains a JSON parser constructed
using the combinators in this library.

Note on Asynchrony
------------------
Although coroutines have important applciations in concurrent or
asynchronous programming, they are not being used in that capacity
here.  In this library they are used simply to act as glue for
combinators that work in a synchronous way and that expect to have
the entire input buffer in memory.  In fact, coroutines that
yield the `parsco::parser<T>` will only suspend when they fail,
and will then never resume, similar to a `std::optional<T>`
coroutine.

This can be contrasted with other uses of coroutines where e.g.
the coroutine will suspend while waiting for more input to arrive
asynchronously, and will then be scheduled to resume when it does.

The `parsco::parser<T>` type can be thought of as the C++ equivalent
to Haskell's `Parsec` monad, for example.

Building
========

Since C++20 coroutines are a relatively new feature, compiler
support is currently not perfect.

As of August, 2021, this library has only been tested successfully
on Clang trunk and (partially successfully) with GCC 11.1.  The
library runs well with Clang, but unfortunately GCC's coroutine
support is still generally too buggy to run this reliably, though
it does seem to be able to run the "hello world" parser example.

Not yet tested with MSVC.

Combinator Reference
====================

Basic/Primitive Parsers
-----------------------
The `chr` parser consumes a char that must be c, otherwise it fails.
```cpp
parser<char> chr( char c );
```

The `any_chr` parser consumes any char, fails only at eof.
```cpp
parser<char> any_chr();
```

The `ret` parser returns a parser that always succeeds and produces
the given value.
```cpp
template<typename T>
parser<T> ret( T );
```

The `space` parser consumes one space (as in space bar) character.
Will fail if it does not find one.
```cpp
parser<> space();
```

The `crlf` parser consumes one character that must be either a CR
or LF, and Will fail if it does not find one.
```cpp
parser<> crlf();
```

The `tab` parser consumes one tab character. Will fail if it does
not find one.
```cpp
parser<> tab();
```

The `blank` parser consumes one character that must be either a
space, tab, CR, or LF, and fails otherwise.
```cpp
parser<> blank();
```

The `digit` parser consumes one digit [0-9] char or fails.
```cpp
parser<char> digit();
```

The following do what you'd think they do:
```cpp
parser<char> lower();
parser<char> upper();
parser<char> alpha();
parser<char> alphanum();
```

The `one_of` parser consumes one char if it is one of the ones in
sv, otherwise fails.
```cpp
parser<char> one_of( std::string_view sv );
```

The `not_of` parser consumes one char if it is not one of the
ones in sv, otherwise fails.
```cpp
parser<char> not_of( std::string_view sv );
```

The `eof` parser succeeds if the input stream is finished, and
fails otherwise. Can be used to test if all input has been consumed
Although see the `exhaust` parser below.
```cpp
parser<> eof();
```

Strings
=======
The `str` parser attempts to consume the exact string given at
the current position in the input string, and fails otherwise.
```cpp
parser<> str( std::string_view sv );
```

The `identifier` parser attempts to parse a valid identifier,
which must match the regex `[a-zA-Z_][a-zA-Z0-9_]*`.
```cpp
parser<std::string> identifier();
```

The `blanks` parser consumes zero or more blank spaces (including
newlines and tabs). It will never fail.
```cpp
parser<> blanks();
```

The `double_quoted_str` and `singel_quoted_str` respectively
parse "..." or '...' and returns the stuff inside, which cannot
contain newlines. Note that these return string views into the
buffer because they are implemented using primitives.
```cpp
parser<std::string_view> double_quoted_str();
parser<std::string_view> single_quoted_str();
```

The `quoted_str` parser parses a string that is either in double
quotes or single quotes. Does not allow escaped quotes within the
string.
```cpp
parser<std::string> quoted_str();
```

Combinators
===========

Almost all combinators are listed below along with signatures.

## pred
The `pred` parser parses a single character for which the predicate
returns true, fails otherwise.
```cpp
template<typename T>
parser<char> pred( T func );
```

## many
The `many` parser parses zero or more of the given parser.
```cpp
template<typename Func, typename... Args>
auto many( Func f, Args... args ) const -> parser<R>;
```
where `R` is a `std::vector` if the element parser returns something
other than a character, or a `std::string` otherwise.

## many_type
The `many_type` parser parses zero or more of the given type for
the given language tag using the parsco ADH extension point mechanism.
```cpp
template<typename Lang, typename T>
parser<R> many_type();
```
where `R` is a `std::vector` if the element parser returns something
other than a character, or a `std::string` otherwise.

## many1
The `many1` parser parses one or more of the given parser.
```cpp
template<typename Func, typename... Args>
auto many1( Func f, Args... args ) const -> parser<R>;
```
where `R` is a `std::vector` if the element parser returns something
other than a character, or a `std::string` otherwise.

## seq
The `seq` parser runs multiple parsers in sequence, and only succeeds
if all of them succeed. Returns all results in a tuple.
```cpp
template<typename... Parsers>
parser<std::tuple<...>> seq( Parsers... );
```

## invoke
The `invoke` parser calls the given function with the results of
the parsers as arguments (which must all succeed).

NOTE: the parsers are guaranteed to be run in the order they appear
in the parameter list, and that is one of the benefits of
using this helper.
```cpp
template<typename Func, typename... Parsers>
parser<R> invoke( Func f, Parsers... ps );
```
where `R` is the result type of the function `f` when invoked
with the results of all of the parsers are arguments.


## emplace
The `emplace` parser calls the constructor of the given type with
the results of the parsers as arguments (which must all succeed).
```cpp
template<typename T, typename... Parsers>
parser<T> emplace( Parsers... ps );
```

## seq_last
The `seq_last` parser runs multiple parsers in sequence, and only
succeeds if all of them succeed. Returns last result.
```cpp
template<typename... Parsers>
parser<R> seq_last( Parsers... ps );
```
where `R` is the `value_type` of the last parser in the argument list

## seq_first
The `seq_first` parser runs multiple parsers in sequence, and only
succeeds if all of them succeed. Returns first result.
```cpp
template<typename... Parsers>
parser<R> seq_first( Parsers... ps );
```
where `R` is the `value_type` of the first parser in the argument list

## on_error
The `on_error` combinator runs the given parser and if it fails
it will return the error message given (as opposed to any error
message that was produced by the parser).
```cpp
template<typename Parser>
Parser on_error( Parser p, std::string_view err_msg );
```

## exhaust
The `exhaust` parser runs the given parser and then checks that
the input buffers has been exhausted (if not, it fails). Returns
the result from the parser on success (i.e., when all input has
been consumed).
```cpp
template<typename Parser>
Parser exhaust( Parser p );
```

## unwrap
The `unwrap` parser is not really a parser, it just takes a
nullable entity (such as a `std::optional`, `std::expected`,
`std::unique_ptr`), and it will return a parser that, when
run, will try to get the value from inside of it.  If the
object does not contain a value, then the parser will fail.
```cpp
template<Nullable N>
parser<typename N::value_type> unwrap( N n );
```

## bracketed
The `bracketed` parser runs the given parser p between characters
l and r (or parsers l and r, depending on the overload chosen).
```cpp
template<typename T>
parser<T> bracketed( char l, parser<T> p, char r );

template<typename L, typename R, typename T>
parser<T> bracketed( parser<L> l, parser<T> p, parser<R> r );
```

## try_ignore
The `try_ignore` parser will try running the given parser but ignore
the result if it succeeds. As with `try_`, it will still
succeed if the given parser fails, though it will return a
`result_t` in an error state instead of failing the parent
parser.
```cpp
template<Parser P>
parser<> try_ignore( P p );
```

## fmap
The classic `fmap` combinator runs the parser p and applies the
given function to the result, if successful.
```cpp
template<Parser P, typename Func>
  requires( std::is_invocable_v<Func, typename P::value_type> )
parser<R> fmap( Func f, P p );
```
where `R` is the result of invoking the function on the
`value_type` of the parser `p`.

## first
The `first` parser runs the parsers in sequence until the first
one succeeds, then returns its result (all of the parsers must
return the same result type). If none of them succeed then the
parser fails. Another way to use this combinator is to use the
pipe (`|`) operator defined below.
```cpp
template<typename P, typename... Ps>
requires( std::is_same_v<typename P::value_type,
                         typename Ps::value_type> && ...)
parser<R> first( P fst, Ps... rest );
```

## interleave_first
The `interleave_first` parses "g f g f g f" and returns the f's.
```cpp
template<typename F, typename G>
parser<R> interleave_first( F f, G g, bool sep_required = true );
```
where `R` is the `value_type` of the parser `f`.

## interleave_last
The `interleave_last` parses "f g f g f g" and returns the f's.
```cpp
template<typename F, typename G>
parser<R> interleave_last( F f, G g, bool sep_required = true );
```
where `R` is the `value_type` of the parser `f`.

## interleave
The `interleave` parses "f g f g f" and returns the f's.
```cpp
template<typename F, typename G>
parser<R> interleave( F f, G g, bool sep_required = true );
```
where `R` is the `value_type` of the parser `f`.

## cat
The `cat` parser runs multiple string-yielding parsers in sequence
and concatenates the results into one string.
```cpp
template<typename... Parsers>
parser<std::string> cat( Parsers... ps );
```

## >> operator
The `>>` operator runs the parsers in sequence (all must succeed)
and returns the result of the final one.
```cpp
template<Parser T, Parser U>
parser<typename U::value_type> operator>>( T l, U r );

// Example
co_await (blanks() >> identifier());
```

## << operator
The `<<` operator runs the parsers in sequence (all must succeed)
and returns the result of the first one.
```cpp
template<Parser T, Parser U>
parser<typename T::value_type> operator<<( T l, U r );

// Example
co_await (identifier() << blanks());
```

## | operator
The `|` operator runs the parser in the order given and returns
the result of the first one that succeeds.  The parsers must all
return the same type.
```cpp
template<Parser T, Parser U>
parser<typename U::value_type> operator|( T l, U r ) {

// Example
co_await (identifier() | quoted_str());
```
