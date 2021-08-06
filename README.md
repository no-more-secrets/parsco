PARSCO
------
C++20 Coroutine Synchronous Parser Combinator Library.

This library contains a monadic parser type and associated
combinators that can be composed to create parsers using
C++20 Coroutines.

PRs are welcome.

Example 1: What are Coroutine Combinators
-----------------------------------------
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
  co_return c1;
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

1. It automatically runs the parser and handles advancing
the position in the input buffer in response.
2. It will automatically detect EOF, which normally results
in a failure.
3. If the parser fails, it will suspend the coroutine and
report failure to the parent (caller) coroutine, which will
typically cascade and immediately cause the entire call stack
to halt, providing an error message and input buffer location
to the caller.  Although see the `try_` combinator further
below for examples of how to backtrack in the case of failure.

The coroutines thread the input buffer pointers and state through
the coroutine call stack automatically and so there is no global
state in the library, hence we have nice properties of being
re-entrant and thread safe (in some sense).

Example 2: The Hello World Parser
---------------------------------

As a first example, let us define a simple grammar that we will refer
to as the "hello world" grammar.  It is defined as follows:

1. The string may start or end with any number of spaces.
2. It must contain the two words 'hello' and 'world' in that order.
3. The two works must have their first letters either both lowercase
or both uppercase.
4. Subsequent letters in each word must always be lowercase.
5. The second word may have an arbitrary number of exclamation marks
after it, but they must begin right after the second word.
6. The number of exclamation marks, if any, must be even.
7. The two words can be separated by spaces or by a comma. If
separated by a comma, spaces are optional after the comma.

Examples:
```cpp
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
```

The following is a parsco parser that parses this grammar:

```cpp
parsco::parser<string> parse_hello_world() {
  // Combinators live in this namespace.
  using namespace parsco;

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
  if( h == 'h' )
    co_await chr( 'w' );
  else
    co_await chr( 'W' );

  // The remainder of the word must always be lowercase.
  co_await str( "orld" );

  // Parse zero or more exclamation marks. Could also have
  // written many( chr, '!' ).
  std::string excls = co_await many( [] { return chr( '!' ); } );

  // Grammar says number of exclamation marks must be even.
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
```

See the file `hello-world-parser.cpp` in the examples folder for
a runnable demo of calling this parser on the above input data.
Essentially we do this:

```cpp
// This should conform to the above grammar.
std::string_view test = "  hello, world!!!  ";

// Filename is specified only to improve error messages.  In
// this case, there is no filename.
parsco::result_t<string> hw =
    parsco::run_parser( "fake-filename.txt", test, parse_hello_world() );

if( !hw ) {
  cout << "test \"" << s
       << "\" failed to parse: " << hw.get_error().what()
       << "\n";
} else {
  cout << "test \"" << s << "\" succeeded to parse.\n";
}
```

which, when run on each of the test cases above, yields:

```
test "Hello, World!!"      succeeded to parse.

test "  hello , world!!  " failed to parse:
                           fake-filename.txt:error:1:9

test "  hello, world!!!! " succeeded to parse.

test "  hello, world!!!  " failed to parse:
                           fake-filename.txt:error:1:18 must have even # of !s

test "hEllo, World"        failed to parse:
                           fake-filename.txt:error:1:2

test "hello world"         succeeded to parse.

test "HelloWorld"          failed to parse:
                           fake-filename.txt:error:1:6

test "hello,world"         succeeded to parse.

test "hello, World"        failed to parse:
                           fake-filename.txt:error:1:8

test "hello, world!!!!!!"  succeeded to parse.

test "hello, world !!!!"   failed to parse:
                           fake-filename.txt:error:1:14 failed to parse all characters in input stream

test "hello, world "       succeeded to parse.

test "hello, world!! x"    failed to parse:
                           fake-filename.txt:error:1:16 failed to parse all characters in input stream
```

Note that in those cases where we provided an error
ourselves, it gives it to the user for a better experience.

JSON Parser
-----------
To see a more realistic example, see the `json-parser.cpp` file
in the examples folder which contains a JSON parser constructed
using the combinators in this library.

User-Defined Types
------------------
The Parsco library provides an ADL-based extension point so that
you can define parsers for user-defined types in a consistent
manner and that will be discoverable by the library.  For example,
the library provides a parser for `std::variant` which will
invoke the extension point to attempt to parse the types that
it contains, which may be user-defined types.  To define a parser
for some user type `MyType` which lives in your namespace `your_ns`,
do the following:

```cpp
#incldue "parsco/ext.hpp"
#incldue "parsco/parser.hpp"
#incldue "parsco/promise.hpp"
#incldue "parsco/ext-basic.hpp"

namespace your_ns {

  // For exposition only.
  using parsco::lang;
  using parsco::tag;
  using parsco::parser;

  struct MyType {
    int x;
    int y;
  };

  // A type tag representing the (custom) language that your
  // parser parses. All of your parsers for a given language will
  // use the same type tag to guarantee at compile-time that they
  // are not mixed. That means that you can safely have multiple
  // parsers for different languages for the same user type My-
  // Type and they won't interfere.
  struct MyLang {};

  parser<MyType> parser_for( lang<MyLang>, tag<MyType> ) {
    // As an example, we're parse a point-like structure.
    int x = co_await parsco::parse<MyLang, int>();
    int y = co_await parsco::parse<MyLang, int>();
    co_return MyType{ x, y };
  }

}
```

Now, having defined that, any other code, including code
internal to the Parsco library, can parse your type by
running the following:

```cpp
MyType mt = co_await parsco::parse<MyLang, MyType>();
```

and your parser will be discovered through ADL.  You can use this
to easily build generic parsers for e.g. `std::vector<T>` which
then invoke `parsco::parse<SomeLang, T>()` to parse any type.

Failure and Backtracking
------------------------
When a `co_await`ed parser fails, it aborts the entire parsing
call stack in the sense that all coroutines in the call stack
will be suspended (and likely destroyed shortly thereafter when
the original `parser<T>` object goes out of scope).

Sometimes, however, we want to attempt a parser and allow for it
to fail.  For this, there is a special combinator called `try_`:

```cpp
using namespace parsco;

result_t<int> maybe_int = co_await try_{ parse_int() };
```

This will propagate parse errors via a return value that can be
checked in the parser. Upon a failure, the `maybe_int` will con-
tain the error and the parser will internally have backtracked in
the input buffer so that any subsequent parsers can retry the
same section of input.

Generally, the combinators in the library that allow for failing
parsers will all use the `try_` combinator internally and thus
will automatically backtrack, so that the internal position in
the buffer will never extend beyond what has been successfully
parsed and returned by your parsers.

Note on Asynchrony
------------------
Although coroutines have important applications in concurrent or
asynchronous programming, they are not being used in that capacity
here.  In this library they are used simply to act as glue for
combinators that work in a synchronous way and that expect to have
the entire input buffer in memory.  In fact, `parsco::parser<T>`
coroutines that suspend will only suspend when they fail (apart
from their initial suspend point, since they are lazy; see below),
and will then never resume, similar to a `std::optional<T>`
coroutine.

This can be contrasted with other uses of coroutines where e.g.
the coroutine will suspend while waiting for more input to arrive
asynchronously, and will then be scheduled to resume when it does.

The `parsco::parser<T>` type can be thought of as the C++ equivalent
to Haskell's `Parsec` monad, for example.

Coroutine Ownership
-------------------
The coroutines in this library follow the practice of "Structured
Concurrency" in that:

1. The are lazy.
2. The returned parser object owns the coroutine in RAII fashion
and will destroy it when the parser object goes out of scope.

This guarantees (in most use cases) that memory and lifetimes will
be managed properly automatically without any special consideration
from the user.

Laziness and Parameter Lifetime
-------------------------------
Parsco parser coroutines are lazy.  That means that when you
invoke a `parsco::parser<T>` coroutine, it immediately suspends.
In fact it _must_, because it does not yet have access to the input
data at that point.  The parser is only resumed and run when it
is `co_await`ed, at which point it is given access to the input
data by its caller.

For that reason, if combinator functions were to accept parameters
by reference, it could lead to thorny lifetime issues where the
references become dangling when the parser suspends at its
initial suspend point and then gets passed to another combinator
before the end result is eventually `co_await`ed.

Since it is not easy to avoid this when building up combinators,
this library opts for the safe route and thus all combinators
accept their arguments by value.  This may lead to slight
inefficiencies, but it guarantees that there will not be any
dangling references. The developer can thus freely create
arbitrary combinator creations without having to reason about
lifetime or dangling references.

The examples in this library have been tested with ASan and they
do come out clean, for what that is worth.

Building and Running
====================
Since C++20 coroutines are a relatively new feature, compiler
support is currently not perfect.

As of August, 2021, this library has only been tested successfully
on Clang trunk and (partially successfully) with GCC 11.1.  The
library runs well with Clang, but unfortunately GCC's coroutine
support is still generally too buggy to run this reliably, though
it does seem to be able to run the "hello world" parser example.

Not yet tested with MSVC.

Runtime Performance
-------------------
There is good news and bad news... good news first: when using
Clang with optimizations enabled (`-O3`), I have observed that a
complex parser made of many combinators using this library can
be optimized to a point where it runs around 15x slower than a
parser hand-coded in C, in the benchmarks that I ran.  That may
seem bad, but given how incredibly fast a hand-coded C parser
can be, I'd say that is not so bad.

The bad news is that, in non-optimized builds, the performance
(relative to a hand-coded C parser) will be much worse.

Again on the bright side, I think it is likely that compilers
will get better in the future with Coroutine optimization and
so this will hopefully be less of a problem.  Clang is already
showing very good promise it seems.

It is also likely that this library can be tweaked further to
make it more ammenable to Clang's optimizations of Coroutines.
PRs are welcome for that if anyone has any expertise there.

Combinator Reference
====================

Basic/Primitive Parsers
-----------------------

## chr
The `chr` parser consumes a char that must be `c`, otherwise it fails.
```cpp
parser<char> chr( char c );
```

## any_chr
The `any_chr` parser consumes any char, fails only at eof.
```cpp
parser<char> any_chr();
```

## pred
The `pred` parser parses a single character for which the predicate
returns true, fails otherwise.
```cpp
template<typename T>
parser<char> pred( T func );
```

## ret
The `ret` parser returns a parser that always succeeds and produces
the given value.
```cpp
template<typename T>
parser<T> ret( T );
```

## space
The `space` parser consumes one space (as in space bar) character.
Will fail if it does not find one.
```cpp
parser<> space();
```

## crlf
The `crlf` parser consumes one character that must be either a CR
or LF, and Will fail if it does not find one.
```cpp
parser<> crlf();
```

## tab
The `tab` parser consumes one tab character. Will fail if it does
not find one.
```cpp
parser<> tab();
```

## blank
The `blank` parser consumes one character that must be either a
space, tab, CR, or LF, and fails otherwise.
```cpp
parser<> blank();
```

## digit
The `digit` parser consumes one digit [0-9] char or fails.
```cpp
parser<char> digit();
```

## lower/upper/alpha/alphanum
The following do what you'd think they do:
```cpp
parser<char> lower();
parser<char> upper();
parser<char> alpha();
parser<char> alphanum();
```

## one_of
The `one_of` parser consumes one char if it is one of the ones in
sv, otherwise fails.
```cpp
parser<char> one_of( std::string_view sv );
```

## not_of
The `not_of` parser consumes one char if it is not one of the
ones in sv, otherwise fails.
```cpp
parser<char> not_of( std::string_view sv );
```

## eof
The `eof` parser succeeds if the input stream is finished, and
fails otherwise. Can be used to test if all input has been consumed
Although see the `exhaust` parser below.
```cpp
parser<> eof();
```

Try
---

The `try_*` family of combinators allow attempting a parser which
may not succeed and backtracking if it fails.

## try_
The `try_` combinator simply wraps another parser and signals
that it is to be allowed to fail.  Moreover, the `try_` wrapper
will wrap the return type of the parser in a `parsco::result_t<T>`,
which is analogous to a `std::expected` (we will use `std::expected`
when it becomes available).
```cpp
template<Parser P> // note Parser here is a C++20 Concept.
parser<parsco::result_t<R>> try_( P p );
```
where `R` is `P::value_type`, i.e., the result of running parser `p`.

In otherwords, if `p` is a parser of type `parser<T>`, then
`try_{ p }` yields a parser of type `parser<parsco::result_t<T>>`.
When it is run, the parser `p` will be run and, if it fails,
the resulting `result_t` will contain an error and the parser
will have backtracked to restore the current position in the
input buffer to what it was before the parser began, thus giving
subsequent parsers the opportunity to parse the same input.
If `p` succeeds, then the `result_t` contains its result.

Hence, a parser wrapped in `try_` never fails in the sense that
parsers normally fail in this library (by aborting the entire
parse); instead, a failure is communicated via return value.

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

Strings
-------

## str
The `str` parser attempts to consume the exact string given at
the current position in the input string, and fails otherwise.
```cpp
parser<> str( std::string_view sv );
```

## identifier
The `identifier` parser attempts to parse a valid identifier,
which must match the regex `[a-zA-Z_][a-zA-Z0-9_]*`.
```cpp
parser<std::string> identifier();
```

## blanks
The `blanks` parser consumes zero or more blank spaces (including
newlines and tabs). It will never fail.
```cpp
parser<> blanks();
```

## double_quoted_str / single_quoted_str
The `double_quoted_str` and `single_quoted_str` respectively
parse "..." or '...' and returns the stuff inside, which may
contain newlines. Note that these return string views into the
buffer because they are implemented using "magic" primitives,
i.e., combinators for which there is special support within
the `parsco::promise_type` object, just for efficiency.
```cpp
parser<std::string_view> double_quoted_str();
parser<std::string_view> single_quoted_str();
```

## quoted_str
The `quoted_str` parser parses a string that is either in double
quotes or single quotes. Does not allow escaped quotes within the
string.
```cpp
parser<std::string> quoted_str();
```

Sequences
---------
Many of the combinators in this section are actually higher-order
functions. They take functions that, when called (which is typ-
ically done in some repeated fashion), produce parser objects to
be run.

A single `parser<T>` object represents a live coroutine, and so
it itself cannot be run multiple times; instead, if a combinator
needs to run a user-specified parser multiple times, it must
accept a function that produces those parsers when called.

## many
The `many` parser parses zero or more of the given parser.
```cpp
template<typename Func, typename... Args>
auto many( Func f, Args... args ) const -> parser<R>;
```
where again the arguments to the combinator are no actually
a parser, but instead a function (and arguments) which, when
called, produce parser objects.  The function will be invoked
on the arguments possibly  multiple times.

`R` is a `std::vector` if the element parser returns something
other than a character, or a `std::string` otherwise.

## many_type
The `many_type` parser parses zero or more of the given type for
the given language tag using the parsco ADH extension point mechanism.
That is, it will call `parsco::parse<Lang, T>()` to parse a `T`
and may do so multiple times.
```cpp
template<typename Lang, typename T>
parser<R> many_type();
```
where `R` is a `std::vector` if the element parser returns something
other than a character, or a `std::string` otherwise.

See the JSON example in the examples folder or the above section
on user types for how to make user-defined types parsable using
the ADL extension point of the parsco library.

## many1
The `many1` parser parses one or more of the given parser
(technically it's the parser returned by invoking the given
function on the given arguments).
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
This combinator takes parser objects directly (as opposed to
functions-that-return-parsers) because each parser only needs to
be run at most once. Although note that if a parser fails, the
subsequent ones will not be run.

## seq_last
The `seq_last` parser runs multiple parsers in sequence, and only
succeeds if all of them succeed. Returns the result of the
last parser.
```cpp
template<typename... Parsers>
parser<R> seq_last( Parsers... ps );
```
where `R` is the `value_type` of the last parser in the argument list.
As above, this combinator also takes parser objects directly as opposed
to functions.

## seq_first
The `seq_first` parser runs multiple parsers in sequence, and only
succeeds if all of them succeed. Returns first result.
```cpp
template<typename... Parsers>
parser<R> seq_first( Parsers... ps );
```
where `R` is the `value_type` of the first parser in the argument list.
As above, this combinator also takes parser objects directly as opposed
to functions.

## interleave_first
The `interleave_first` parses "g f g f g f" and returns the f's.
```cpp
template<typename F, typename G>
parser<R> interleave_first( F f, G g, bool sep_required = true );
```
where `R` is the `value_type` of the parser `f`.  `F` and `G` are
nullary functions that return parser objects.

## interleave_last
The `interleave_last` parses "f g f g f g" and returns the f's.
```cpp
template<typename F, typename G>
parser<R> interleave_last( F f, G g, bool sep_required = true );
```
where `R` is the `value_type` of the parser `f`.  `F` and `G` are
nullary functions that return parser objects.

## interleave
The `interleave` parses "f g f g f" and returns the f's.
```cpp
template<typename F, typename G>
parser<R> interleave( F f, G g, bool sep_required = true );
```
where `R` is the `value_type` of the parser `f`.  `F` and `G` are
nullary functions that return parser objects.

## cat
The `cat` parser runs multiple string-yielding parsers in sequence
and concatenates the results into one string.
```cpp
template<typename... Parsers>
parser<std::string> cat( Parsers... ps );
```
In other words, each of the `Parsers` must be a `parser<std::string>`.

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
Note: despite the direction that the operator is pointing (`<<`),
the parsers are still run from left-to-right order, and the
result of the left-most one is returned, assuming of course
that they all succeed.

Alternatives
------------

This means that we give a set of possible parsers, only one of
which needs to succeed.  These parsers use the `try_` combinator
internally, and so therefore they will do backtracking after
each failed parser before invoking the next one.

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
where `R` is `P::value_type`.  When one parser succeeds,
subsequent parsers will not be run.

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
This is equivalent to the `first` parser above.

Function Application
--------------------

The combinators in this section have to do with invoking functions
on the results of parsers.

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

More explicitly, this combinator will run all of the parsers `ps`
in the order they are passed to the function, and then use their
results as the arguments to the function `f`, which must itself
produce a parser.  So in other wods, if we do this:

```cpp
auto r = co_await parsco::invoke( f, any_char(), any_char() );
```

that is equivalent to doing:

```cpp
char c1 = co_await any_char();
char c2 = co_await any_char();
auto r = co_await f( c1, c2 );
```

and thus as can be seen, this parser (like many others) is for
convenience; it does not do anything that couldn't be done manually.

## emplace
The `emplace` parser calls the constructor of the given type `T` with
the results of the parsers as arguments (which must all succeed).
```cpp
template<typename T, typename... Parsers>
parser<T> emplace( Parsers... ps );
```
The parsers will be run in the order they are passed to the function.
Example:

```cpp
struct Point {
  Point( int x_, int y_ ) : x( x_ ), y( y_ ) {}
  int x;
  int y;
};

// Assuming that we have a parser for integers called parse_int:
Point p = co_await parsco::emplace<Point>( parse_int(),
                                           parse_int() );
```
In particular, note that we are not `co_await`ing on the results
of the `parse_int()` calls; we just give the combinators to
`emplace` and it does the rest.  This can reduce some verbosity
in parsers from time to time.  Think of this as an analog to
Haskell's `<$>`/`<*>` operators for Applicatives.

## fmap
The venerable `fmap` combinator runs the parser `p` and applies the
given function to the result, if successful.  The function typically
does not return a parser; it is just a normal function.
```cpp
template<Parser P, typename Func>
  requires( std::is_invocable_v<Func, typename P::value_type> )
parser<R> fmap( Func f, P p );
```
where `R` is the result of invoking the function on the
`value_type` of the parser `p`.

Error Detection
---------------

## try_
The `try_` combinator allowed attempting a parser which may not
succeed. See the section on "Failure and Bactracking" for an
example of how this is used.

## on_error
The `on_error` combinator runs the given parser and if it fails
it will return the error message given (as opposed to any error
message that was produced by the parser).
```cpp
template<typename Parser>
Parser on_error( Parser p, std::string_view err_msg );
```
This is used to provide more meaningful error messages to users
in response to a given parser having failed.

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
`std::unique_ptr`), and it will return a "fake" parser that, when
run, will try to get the value from inside of the nullable
object.  If the object does not contain a value, then the
parser will fail.  If it does contain an object, the parser
will yield it as its result.
```cpp
template<Nullable N>
parser<typename N::value_type> unwrap( N n );
```

Miscellaneous
-------------

## bracketed
The `bracketed` parser runs the given parser p between characters
`l` and `r` (or parsers `l` and `r`, depending on the overload chosen).
```cpp
template<typename T>
parser<T> bracketed( char l, parser<T> p, char r );

template<typename L, typename R, typename T>
parser<T> bracketed( parser<L> l, parser<T> p, parser<R> r );
```

For example, to parse an identifier between to curly braces,
you could do:

```cpp
using namespace parsco;
using namespace std;

string ident = co_await bracketed( '{',
                                   identifier(),
                                   '}' )
```
