# PARSCO

C++20 **Coroutine** Synchronous **Parser** Combinator Library.

This library contains a monadic parser type and associated
combinators that can be composed to create parsers using C++20
Coroutines.

PRs are welcome. 🎉🎉🎉

Example 1
--------------------------------------------------
As a first example, let's say that you want to create a parser
that parses any two-character string where both characters are
the same (e.g. "==", "aa", etc.) and then returns that character
on success. You could do that like this:

```cpp
parsco::parser<char> parse_two_same() {
  char c = co_await parsco::any_chr();
  co_await parsco::chr( c );
  co_return c;
}
```

Another way to write it, which will provide a better error
message to the user when parsing fails, would be:

```cpp
parsco::parser<char> parse_two_same() {
  char c1 = co_await parsco::any_chr();
  char c2 = co_await parsco::any_chr();
  if( c1 != c2 )
    // Provides better error message.
    co_await parsco::fail( "expected two equal chars" );
  co_return c1;
}
```

Running this parser can result in three possible outcomes:

1. The parser finds two characters and they are both the same,
   e.g. "xx". In that case, the parser will yield 'x'.
2. The parser finds two characters and they are not the same. In
   that case, the parser will fail with an error message. The
   particular error message depends on which of the above two
   implementations is used.
3. The parser encounters the end of the input stream (EOF) before
   it can parse both characters. In this case, the parser
   function will fail and return.

The parsers used above (`chr` and `any_chr`) are functions that
return `parser<T>`s (in this case, `T` is `char`). As can be
seen, they have been combined to yield a higher-level function
`parse_two_same` which is itself a parser in that it returns a
`parser<T>`. This `parse_two_same` parser can thus can be used to
build even more complex parsers in a similar way, namely by
using the C++20 `co_await` keyword on the parser object that
it returns.

What are Parsers, Coroutines and Combinators?
--------------------------------------------
With the first example out of the way, let's discuss what is
happening under the hood.

### Parsers
The term "parser" in this library is a bit
overloaded. It can refer to an object of type `parsco::parser<T>`
or to a coroutine function that returns one. It is ok to think of
them as equivalent for now. In fact, the parser object owns (in
RAII sense) the coroutine.

Typically, a parser is created by calling a coroutine function
that returns a `parsco::parser<T>` and then that parser is run
by applying C++20's `co_await` keyword to it.  A parser object
does nothing on its own; it is only when it is `co_await`ed
that it is given access to the input string and begins to
parse. If it succeeds, then it will result in an object of
type `T`.  If it fails, it will automagically unwind the entire
parser call stack and deliver an error messages to the original
caller of the overall parse operation.

### Coroutines
In this library, parsers are "glued together"
using C++20 Coroutines. An explanation of how C++20 coroutines
work in general is beyond the scope of this README; for an
in-depth look into how C++ coroutines work, see [this
blog](https://lewissbaker.github.io/). However, you do not need
to fully understand them to use this library, so feel free to
continue on in this tutorial either way.

For the purposes of using this library, a coroutine is a function
that

1. Returns a `parsco::parser<T>` type.
2. Uses one or both of the keywords `co_await` and `co_return` in
   its body (this library does not make use of `co_yield`).
3. Has the ability (unlike a conventional function) to suspend and
   resume execution.

When a coroutine is first called, the C++ language "runtime" will
setup a coroutine frame consisting of a "promise" object and enough
space to hold any local variables in the function. Then,
while executing the coroutine, each time the `co_await` or
`co_return` keywords are used, the compiler will query the
promise object via some extension points to ask it what to do in
response. This promise object (and the hidden calls to query it)
are leveraged by this library to automate the following tasks
which must be done to parse a string:

1. Passing around the input buffer.
2. Keeping track of the current position in the input buffer.
3. Detecting a premature EOF (end of input stream) and potentially
   terminating the parse.
4. Detecting and handle parsing errors (i.e., syntax errors in the
   input), which can result in either cancelling the entire parse
   (unwinding the call stack) or in backtracking to reattempt
   with another parser.
5. Providing error message and input buffer location to the user
   in the case of a parsing error.
6. Returning of the result of parsing in a type-safe way.

All of this happens automatically when you `co_await` on a parser
object.

Since the coroutines thread the input buffer pointers and state
through the coroutine call stack automatically, there is no
global state in the library, hence we have nice properties of
being re-entrant and thread safe (though only in that sense).

### Combinators
A parser combinator is a function (or
higher-order function) that can transform a given parser into a
different or more complex parser. Combinators are used to glue
together primitive (basic) parsers into more complex ones.
They typically take as parameters either parser objects or
functions that return parser objects, and then construct a new
parser and return it as a new `parsco::parser<T>` object.

In this library, you will build up your parsers by using
combinators to combine simple parsers into more complex ones.
In fact, in many cases, a parser can be constructed using
existing combinators alone without requiring any coroutines.
The coroutines will still be needed though, in cases where
a parser requires some new logic not provided by an existing
combinator.

Example 2: The Hello World Parser
---------------------------------
As a second example, let us define a simple grammar that we will
refer to as the "hello world" grammar. It is defined as follows:

1. The string may start or end with any number of spaces.
2. It must contain the two words 'hello' and 'world' in that
   order.
3. The two words must have their first letters either both
   lowercase or both uppercase.
4. Subsequent letters in each word must always be lowercase.
5. The second word may have an arbitrary number of exclamation
   marks after it, but they must begin immediately after the
   second word (no spaces).
6. The number of exclamation marks, if any, must be even.
7. The two words can be separated by spaces or by a comma. If
   separated by a comma, spaces are optional after the comma. If
   there is a comma, it must come immediately after the first
   word.

Examples:
```cpp
"Hello, World!!",      // should pass, and consume all input.
"  hello , world!!  ", // should fail (#7 violated)
"  hello, world!!!! ", // should pass, and consume all input.
"  hello, world!!!  ", // should fail (#6 violated)
"hEllo, World",        // should fail (#4 violated)
"hello world",         // should pass, and consume all input.
"HelloWorld",          // should fail (#7 violated)
"hello,world",         // should pass, and consume all input.
"hello, World",        // should fail (#3 violated)
"hello, world!!!!!!",  // should pass, and consume all input.
"hello, world !!!!",   // should pass, but not consume all input.
"hello, world ",       // should pass, and consume all input.
"hello, world!! x",    // should pass, but not consume all input.
```

This is clearly a contrived grammar, but it will allow us to
demonstrate some of the basic facilities in this library.
The following is a Parsco parser that parses this grammar:

```cpp
parsco::parser<string> parse_hello_world() {
  // Eat spaces.
  co_await blanks();
  // Parse one char exactly, and it must be either 'h' or 'H'.
  // Our grammar dictates that the "hello" can start with either
  // a lowercase of capital h.
  char h = co_await one_of( "hH" );

  // But the rest of the letters in the word must be lowercase.
  // Parse the given string exactly.
  co_await str( "ello" );

  // We can have a comma followed by zero-or-more-spaces, or we
  // can have no comma followed by one-or-more spaces.
  //
  // The >> operator is a sequencing operator that will run
  // the first parser, ensuring that it succeeds, and will
  // throw away the result. Then it will invoke the second
  // parser, again ensuring that it succeeds and returning its
  // result.
  //
  // The | operator runs the first parser and if it succeeds,
  // returns its result; otherwise runs the second parser and
  // returns its result, or fails if it fails.
  co_await( ( chr( ',' ) >> blanks() ) | many1( space ) );

  // Our grammar rules say that the two words must have the same
  // capitalization.
  ( h == 'h' ) ? co_await str( "world" )
               : co_await str( "World" );

  // Parse zero or more exclamation marks.
  string excls = co_await many( chr, '!' );

  // Grammar says number of exclamation marks must be even.
  if( excls.size() % 2 != 0 )
    co_await fail( "must have even # of !s" );

  co_await blanks();

  // This is optional, since we're just validating the input, but
  // it demonstrates how to return a result from the parser.
  co_return "Hello, World!";
}
```

See the file `hello-world-parser.cpp` in the examples folder for
a runnable demo of calling this parser on the above input data.
Essentially, we do this:

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
                           fake-filename.txt:error:1:9 expected 'w'

test "  hello, world!!!! " succeeded to parse.

test "  hello, world!!!  " failed to parse:
                           fake-filename.txt:error:1:18 must have even # of !s

test "hEllo, World"        failed to parse:
                           fake-filename.txt:error:1:2 expected 'e'

test "hello world"         succeeded to parse.

test "HelloWorld"          failed to parse:
                           fake-filename.txt:error:1:6

test "hello,world"         succeeded to parse.

test "hello, World"        failed to parse:
                           fake-filename.txt:error:1:8 expected 'w'

test "hello, world!!!!!!"  succeeded to parse.

test "hello, world !!!!"   succeeded to parse.

test "hello, world "       succeeded to parse.

test "hello, world!! x"    succeeded to parse.
```

Note that in those cases where we provided an error message
ourselves via the `fail` combinator, it gives it to the user
and this can result in a better experience.

Before closing this example, let's extend it a bit more. Let's
say that we want to allow more than one occurrence of the
"hello world" in the input buffer (i.e., one or more) and then
have the parser return the number that were found.  To do this,
we can just use our existing `parse_hello_world` parser and
wrap it in a combinator:

```cpp
parsco::parser<int> parse_hello_worlds() {
  std::vector<std::string> hello_worlds =
      co_await parsco::many1( parse_hello_world );
  co_return hello_worlds.size();
}
```

where the `parsco::many1` combinator parses one or more of the
given parser. That means that it will fail unless there is at
least one correctly-parsed "hello world." Notice that by wrapping
our parser in `parsco::many1`, the result is now a
`std::vector<std::string>`, since we are parsing multiple copies
of "hello world." In this case our parser is simply validating
the result, so the contents of the vector are not of much use to
use, but in most other cases they will be useful. In our case,
we're just interested in the size.

The `parse_hello_worlds`, just as with the `parse_hello_world`
parser may not consume the entire input. In the face of a syntax
error in the first occurrence it will fail, but syntax errors in
subsequent occurrences will simply cause `parsco::many1` to stop
parsing (reporting success) and to return whatever it has
successfully parsed, potentially leaving unparsed characters in
the input buffer. If we want to ensure that the entire input
buffer is consumed, we can use either the `parsco::eof`
combinator or the `parsco::exhaust` combinator; an example of the
latter is:

```cpp
parsco::parser<int> parse_hello_worlds() {
  std::vector<std::string> hello_worlds =
    co_await parsco::exhaust( parsco::many1( parse_hello_world ) );
  co_return hello_worlds.size();
}
```

The `parsco::exhaust` combinator returns a parser that only
succeeds if the parser given to it succeeds _and_ consumes all
input; otherwise, the parser fails.

See the section called "Combinator Reference" for a complete
reference of all parsers and combinators.

Example 3: JSON Parser
----------------------
To see a more realistic example, see the `json-parser.cpp` file
in the examples folder which contains a JSON parser constructed
using the combinators in this library. Note that the JSON
parser makes use of the ADL extension point mechanism of the
library, which will be described in the next section.

Something to note in the implementation of the JSON parser is
that many of the functions are actually not coroutines because
they do not use the `co_await` or `co_return` keywords (and
`co_yield` is not used by this library). For example, the parser
for a JSON boolean:

```cpp
parser<boolean> parser_for( lang<Json>, tag<boolean> ) {
  return (str( "true"  ) >> ret( boolean{ true  })) |
         (str( "false" ) >> ret( boolean{ false }));
}
```

This is possible because it is frequently the case that a parsing
operation can be described entirely in terms of existing
combinators without applying any logic to the intermediate
results. In those cases your parsers may end up looking like the
above, namely a single expression describing the parser in a
declarative way. Note: in order to support this style of writing
parsers, the combinators take their parameters by value in order
to avoid dangling references; see the section "Laziness and
Parameter Lifetime" for more information.

Writing a function in this declarative/expression-based style
when possible is likely a good idea because it will prevent the
function from being compiled as a coroutine, which means that
there will be less overhead in invoking it (invoking a coroutine
typically involves at least one heap allocation to create
the coroutine state; theoretically, the compiler can elide the
heap allocation, but that does not seem to currently happen with
this library, see the section on runtime performance below).

User-Defined Types
------------------
The Parsco library provides an ADL-based extension point so that
you can define parsers for user-defined types in a consistent
manner and that will be discoverable by the library. For example,
the library provides a parser for `std::variant` which will
invoke the extension point to attempt to parse the types that it
contains, which may be user-defined types. To define a parser for
some user type `MyType` which lives in your namespace `your_ns`,
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

Now, having defined that, any other code, including code internal
to the Parsco library, can parse your type by running the
following:

```cpp
MyType mt = co_await parsco::parse<MyLang, MyType>();
```

and your parser will be discovered through ADL. You can use this
to easily build generic parsers for e.g. `std::vector<T>` which
then invoke `parsco::parse<SomeLang, T>()` to parse any type.

Using this ADL extension point is entirely optional; you may not
need it or opt not to use it, and that is fine.

Failure and Backtracking
------------------------
When a `co_await`ed parser fails, it aborts the entire parsing
call stack in the sense that all coroutines in the call stack
will be suspended (and likely destroyed shortly thereafter when
the original `parser<T>` object goes out of scope).

Sometimes, however, we want to attempt a parser and allow for it
to fail. For this, there is a special combinator called `try_`:

```cpp
using namespace parsco;

result_t<int> maybe_int = co_await try_{ parse_int() };
```

This will propagate parse errors via a return value that can be
checked in the parser. Upon a failure, the `maybe_int` will
contain the error and the parser will internally have backtracked
in the input buffer so that any subsequent parsers can retry the
same section of input.

Generally, the combinators in the library that allow for failing
parsers will all use the `try_` combinator internally and thus
will automatically backtrack, so that the internal position in
the buffer will never extend beyond what has been successfully
parsed and returned by your parsers. That said, the framework
does keep track of the farthest position that was _attempted_ to
be parsed, in order to deliver accurate error messages to users
in response to syntax errors.

Note on Asynchrony
------------------
Although coroutines have important applications in concurrent or
asynchronous programming, they are not being used in that
capacity here. In this library they are used simply as a "glue"
for parsers and combinators, all of which operate in a synchronous
way and that expect to have the entire input buffer in memory.
In fact, `parsco::parser<T>` coroutines will only suspend
when they fail (apart from their initial suspend point, since
they are lazy; see below), and will then never resume, similar to
how a `std::optional<T>` coroutine might work.

This can be contrasted with other uses of coroutines where e.g.
the coroutine will suspend while waiting for more input to arrive
asynchronously, and will then be scheduled to resume when it
does.

The `parsco::parser<T>` type can be thought of as the C++
equivalent to Haskell's `Parsec` monad, for example.

Coroutine Ownership
-------------------
The coroutines in this library follow the practice of
[Structured Concurrency](https://www.youtube.com/watch?v=1Wy5sq3s2rg) in that:

1. The are lazy: they suspend at their initial suspend point.
2. A parser object return by a coroutine owns the coroutine in
   RAII fashion and will destroy the coroutine it when the parser
   object goes out of scope.

This guarantees (in most use cases) that memory and lifetimes
will be managed properly and automatically without any special
consideration from the library user.

It is also hoped that this ownership model will aid the optimizer
in understanding that it can elide all of the coroutine state
heap allocations (which I believe should be theoretically
possible in all of the common use cases). This unfortunately does
not seem to happen currently; see the section on runtime
performance for more information.

Laziness and Parameter Lifetime
-------------------------------
Parsco parser coroutines are lazy. That means that when you
invoke a `parsco::parser<T>` coroutine, it immediately suspends.
In fact it _must_ suspend, because it does not yet have access to
the input data initially. The parser is only resumed and run when
it is `co_await`ed, at which point it is given access to the
input data by its caller.

For that reason, if combinator functions were to accept
parameters by reference, it could lead to thorny lifetime issues
where the references become dangling when a combinator object is
returned from a function without `co_await`ing it, which is
convenient to be able to do (the JSON parser example does this
often).

In order to systematically avoid this problem, this library opts
for the safe route and thus all combinators accept their
arguments by value. This may lead to slight inefficiencies, but
it guarantees that there will not be any dangling references. The
developer can thus freely create arbitrary combinator creations
(`co_await`ing the results immediately or at a later time)
without having to reason about lifetime or dangling references.

When designing new combinators, you may want to consider doing
the same for consistency. In practice, this should not add much
overhead, because typical parameters to coroutines include a)
stateless lambdas, b) primitive types such as ints, and
`parsco::parser<T>` objects which are always moved.

Theoretically, it seems that any such lifetime issues could be
systematically avoided by constructing parsers and wrapping
combinators in one expression and then `co_await`ing the result
immediately before the expression goes out of scope, as opposed
to putting the result into a `parser<T>` object and then
`co_await`ing on it later. However, to save the developer having
to think about that and to give them flexibility, the combinators
just take their arguments by value.

The examples in this library have been tested with ASan and they
do come out clean, for what that is worth.

Building and Running
====================
Since C++20 coroutines are a relatively new feature, compiler
support is currently not complete or perfect.

As of August, 2021, this library has only been tested
successfully with Clang trunk and (partially successfully) with
GCC 11.1. The library runs well with Clang, but unfortunately
GCC's coroutine support still seems too lacking to run this
library without crashing, though it does seem to be able to run
the "hello world" parser example. Not yet tested with MSVC.

### Building
To build and run, you will need a recent build of Clang trunk
(which at the time of writing should be the as-yet-unreleased
Clang 12) along with a standard library that supports enough
of C++20 and Coroutines.  For the standard library I use libstdc++
from gcc 11.1.0, though more recent ones should work as well.

The build is done with CMake, and here is a typical build process
that I use on Ubuntu Linux 20.04 on an `x86_64` system that will
use a build of LLVM located at `/path/to/llvm` and a libstdc++
11.1.0 located in `/path/to/gcc`:

### Clone
```bash
$ git clone https://github.com/dpacbach/parsco
$ cd parsco
```

### Configure
```bash
# From the parsco directory:
$ mkdir build
$ cd build
$ cmake .. \
       -DCMAKE_BUILD_TYPE=Release \
       -DCMAKE_CXX_COMPILER="/path/to/llvm/bin/clang++" \
       -DCMAKE_CXX_FLAGS_INIT="-nostdinc++ -I/path/to/gcc/include/c++/11.1.0 -I/path/to/gcc/include/c++/11.1.0/x86_64-pc-linux-gnu" \
       -DCMAKE_EXE_LINKER_FLAGS_INIT="-Wl,-rpath,/path/to/gcc/lib64 -L/path/to/gcc/lib64 -fuse-ld=lld"
```

### Build
```bash
$ make -j8
```

### Run examples
```bash
# From the build directory:
$ ./src/example/hello-world-parser
$ ./src/example/json-parser
```

Though you may well have to tweak the CMake command to work
with your setup.

Runtime Performance
-------------------
When using Clang with optimizations enabled (`-O3`), I have
observed that a complex parser made of many combinators using
this library can be optimized down by the compiler to a point
where it runs around 15x slower than a parser hand-coded in C, in
the benchmarks that I ran.

Given how incredibly fast a hand-coded C parser can be, and
given the high level of abstraction that can be attained using
this library, it is perhaps not so bad.  That said, performance
is very important in the C++ ecosystem and thus a goal of this
library is to eventually attain C-level performance without
sacrificing any of the abstraction.

Theoretically, it should be the case when using this library that
all coroutine frame heap allocations can/should be elided by the
compiler. But, unfortunately, that does not seem to happen in the
instances where I have looked at the generated code. I am not
sure why this is at the moment, but I am hopeful that this
situation will improve in the future due to either a) an increase
in my own understanding of coroutine library optimization
techniques, b) help from contributors who have expertise in this
area, or c) an increase in compiler's ability to optimize
coroutines.

If we could get the compiler to elide the coroutine frames
and allocations, subsequent inlining I believe could allow
this library to reach C-level performance levels.  Help with
that would be much appreciated.

In non-optimized builds, the performance (relative to said C
parser) is even worse unfortunately, and this is another problem
that would be nice to improve upon (any help is appreciated).

Error Messages
--------------
Upon parse failure, the parsco parser framework is always able to
give the location (line and column) that the error occurred in
the input text. However, it is not generally able to provide a
human-readable error message describing what the parser was
expecting, unless the failure was initiated by use of the `fail`
combinator as demonstrated in the Hello World example above.

It may be possible to enhance the library so that a
given parser can be inspected to automatically determine what
characters it was expecting in order to automatically generate
more useful error messages.  Some parser frameworks in some libraries
are able to do this to some extent, but it is difficult.

That said, even if that were possible, it seems that the most
user-intelligible error message are still going to be provided
mainly by the programmer via the `fail( "..." )` combinator.

Combinator Niebloids
--------------------
As a quick implementation note on the combinators, if you
look at the source code (mainly in `combinators.hpp`) you'll
notice that many of them are implemented as niebloids instead of
function templates. This is actually not to do with ADL but
instead is to work around an issue that Clang seems to have with
function template coroutines (it yields strange runtime errors
when such coroutines is called). Hopefully that will be fixed
eventually, at which point the niebloids in this library can be
changed back to function templates.

Combinator Reference
====================

Below is documentation for each parser and combinator function in
this library.

## Basic Parsers

### `chr`
This parser consumes a char that must be `c`, otherwise it
fails.
```cpp
parser<char> chr( char c );
```

### `any_chr`
This parser consumes any char, fails only at eof.
```cpp
parser<char> any_chr();
```

### `pred`
This parser parses a single character for which the
predicate returns true, fails otherwise.
```cpp
template<typename T>
parser<char> pred( T func );
```

### `ret`
This parser returns a parser that always succeeds and
produces the given value.
```cpp
template<typename T>
parser<T> ret( T );
```

### `space`
This parser consumes one space (as in space bar)
character. Will fail if it does not find one.
```cpp
parser<> space();
```

### `crlf`
This parser consumes one character that must be either a CR
or LF, and Will fail if it does not find one.
```cpp
parser<> crlf();
```

### `tab`
This parser consumes one tab character. Will fail if it does
not find one.
```cpp
parser<> tab();
```

### `blank`
This parser consumes one character that must be either a
space, tab, CR, or LF, and fails otherwise.
```cpp
parser<> blank();
```

### `digit`
This parser consumes one digit [0-9] char or fails.
```cpp
parser<char> digit();
```

### `lower`
This parser parses one lowercase letter (`[a-z]`).
```cpp
parser<char> lower();
```

### `upper`
This parser parses one uppercase letter (`[A-Z]`).
```cpp
parser<char> upper();
```

### `alpha`
This parser parses one letter (`[a-zA-Z]`).
```cpp
parser<char> alpha();
```

### `alphanum`
This parser parses one alphanumeric character (`[a-zA-Z0-9]`).
```cpp
parser<char> alphanum();
```

### `one_of`
This parser consumes one char if it is one of the ones in
sv, otherwise fails.
```cpp
parser<char> one_of( std::string sv );
```

### `not_of`
This parser consumes one char if it is not one of the
ones in sv, otherwise fails.
```cpp
parser<char> not_of( std::string sv );
```

### `eof`
This parser succeeds if the input stream is finished, and
fails otherwise. Can be used to test if all input has been
consumed Although see the `exhaust` parser below.
```cpp
parser<> eof();
```

## Trying and Backtracking

The `try_*` family of combinators allow attempting a parser which
may not succeed and backtracking if it fails.

### `try_`
This combinator simply wraps another parser and signals
that it is to be allowed to fail. Moreover, the `try_` wrapper
will wrap the return type of the parser in a
`parsco::result_t<T>`, which is analogous to a `std::expected`
(we will use `std::expected` when it becomes available).
```cpp
template<Parser P> // note Parser here is a C++20 Concept.
parser<parsco::result_t<R>> try_( P p );
```
where `R` is `P::value_type`, i.e., the result of running parser
`p`.

In other words, if `p` is a parser of type `parser<T>`, then
`try_{ p }` yields a parser of type
`parser<parsco::result_t<T>>`. When it is run, the parser `p`
will be run and, if it fails, the resulting `result_t` will
contain an error and the parser will have backtracked to restore
the current position in the input buffer to what it was before
the parser began, thus giving subsequent parsers the opportunity
to parse the same input. If `p` succeeds, then the `result_t`
contains its result.

Hence, a parser wrapped in `try_` never fails in the sense that
parsers normally fail in this library (by aborting the entire
parse); instead, a failure is communicated via return value.

### `try_ignore`
This parser will try running the given parser but
ignore the result. This is useful if you want to run a parser but
a) you don't care if it succeeds, and b) you don't care what its
result is if it succeeds. This can sometimes help to get rid of
"unused return value" compiler warnings.
```cpp
template<Parser P>
parser<> try_ignore( P p );
```

## Strings

### `str`
This parser attempts to consume the exact string given at
the current position in the input string, and fails otherwise.
```cpp
parser<> str( std::string sv );
```

### `identifier`
This parser attempts to parse a valid identifier,
which must match the regex `[a-zA-Z_][a-zA-Z0-9_]*`.
```cpp
parser<std::string> identifier();
```

### `blanks`
This parser consumes zero or more blank spaces (including
newlines and tabs). It will never fail.
```cpp
parser<> blanks();
```

### `double_quoted_str`
This parser parses `"..."` and returns the characters
inside the quotes, which may contain newlines. Note that these
return string views into the buffer because they are implemented
using the parser primitives defined in the file `magic.hpp`,
i.e., combinators for which there is special support within the
`parsco::promise_type` object, just for efficiency.
```cpp
parser<std::string_view> double_quoted_str();
```

### `single_quoted_str`
This parser parses `'...'` and returns the characters
inside the quotes, which may contain newlines. Note that these
return string views into the buffer because they are implemented
using the parser primitives defined in the file `magic.hpp`,
i.e., combinators for which there is special support within the
`parsco::promise_type` object, just for efficiency.
```cpp
parser<std::string_view> single_quoted_str();
```

### `quoted_str`
This parser parses a string that is either in double
quotes or single quotes. Does not allow escaped quotes within the
string.
```cpp
parser<std::string> quoted_str();
```

## Sequences

Many of the combinators in this section are actually higher-order
functions. They take functions that, when called (which is
typically done in some repeated fashion), produce parser objects
to be run.

A single `parser<T>` object represents a live coroutine, and so
it itself cannot be run multiple times; instead, if a combinator
needs to run a user-specified parser multiple times, it must
accept a function that produces those parsers when called.

### `many`
This parser parses zero or more of the given parser.
```cpp
template<typename Func, typename... Args>
parser<R> many( Func f, Args... args );
```
where `R` is a `std::vector` if the element parser returns
something other than a character, or a `std::string` otherwise.

As mentioned in the introduction to this section, the argument to
the combinator is not actually a parser, but rather a function
which, when called with the given arguments, produces a parser
object. The function will be invoked on the arguments multiple
times to repeatedly generate parsers until a parser fails to
parse, at which point the `many` parser finishes (always
successfully) and backtracks over any fragment of input that
failed parsing in the last iteration.

### `many_type`
This parser parses zero or more of the given type for
the given language tag using the parsco ADH extension point
mechanism. That is, it will call `parsco::parse<Lang, T>()` to
parse a `T` and may do so multiple times.
```cpp
template<typename Lang, typename T>
parser<R> many_type();
```
where `R` is a `std::vector` if the element parser returns
something other than a character, or a `std::string` otherwise.

See the JSON example in the examples folder or the above section
on user types for how to make user-defined types parsable using
the ADL extension point of the parsco library.

### `many1`
This parser parses one or more of the given parser
(technically it's the parser returned by invoking the given
function on the given arguments).
```cpp
template<typename Func, typename... Args>
auto many1( Func f, Args... args ) const -> parser<R>;
```
where `R` is a `std::vector` if the element parser returns
something other than a character, or a `std::string` otherwise.

### `seq`
This parser runs multiple parsers in sequence, and only
succeeds if all of them succeed. Returns all results in a tuple.
```cpp
template<typename... Parsers>
parser<std::tuple<...>> seq( Parsers... );
```
This combinator takes parser objects directly (as opposed to
functions-that-return-parsers) because each parser only needs to
be run at most once. Although note that if a parser fails, the
subsequent ones will not be run.

### `seq_last`
This parser runs multiple parsers in sequence, and only
succeeds if all of them succeed. Returns the result of the last
parser.
```cpp
template<typename... Parsers>
parser<R> seq_last( Parsers... ps );
```
where `R` is the `value_type` of the last parser in the argument
list. As above, this combinator also takes parser objects
directly as opposed to functions.

### `seq_first`
This parser runs multiple parsers in sequence, and
only succeeds if all of them succeed. Returns first result.
```cpp
template<typename... Parsers>
parser<R> seq_first( Parsers... ps );
```
where `R` is the `value_type` of the first parser in the argument
list. As above, this combinator also takes parser objects
directly as opposed to functions.

### `interleave_first`
This parser parses "g f g f g f" and returns the f's.
```cpp
template<typename F, typename G>
parser<R> interleave_first( F f, G g, bool sep_required = true );
```
where `R` is the `value_type` of the parser `f`. `F` and `G` are
nullary functions that return parser objects.

### `interleave_last`
This parser parses "f g f g f g" and returns the f's.
```cpp
template<typename F, typename G>
parser<R> interleave_last( F f, G g, bool sep_required = true );
```
where `R` is the `value_type` of the parser `f`. `F` and `G` are
nullary functions that return parser objects.

### `interleave`
This parser parses "f g f g f" and returns the f's.
```cpp
template<typename F, typename G>
parser<R> interleave( F f, G g, bool sep_required = true );
```
where `R` is the `value_type` of the parser `f`. `F` and `G` are
nullary functions that return parser objects.

### `cat`
This parser runs multiple string-yielding parsers in
sequence and concatenates the results into one string.
```cpp
template<typename... Parsers>
parser<std::string> cat( Parsers... ps );
```
In other words, each of the `Parsers` must be a `parser<std::string>`.

### `operator >>`
This operator runs the parsers in sequence (all must succeed)
and returns the result of the final one.
```cpp
template<Parser T, Parser U>
parser<typename U::value_type> operator>>( T l, U r );

// Example
co_await (blanks() >> identifier());
```

### `operator <<`
This operator runs the parsers in sequence (all must succeed)
and returns the result of the first one.
```cpp
template<Parser T, Parser U>
parser<typename T::value_type> operator<<( T l, U r );

// Example
co_await (identifier() << blanks());
```
Note: despite the direction that the operator is pointing (`<<`),
the parsers are still run from left-to-right order, and the
result of the left-most one is returned, assuming of course that
they all succeed.

## Alternatives

This means that we give a set of possible parsers, only one of
which needs to succeed. These parsers use the `try_` combinator
internally, and so therefore they will do backtracking after each
failed parser before invoking the next one.

### `first`
This parser runs the parsers in sequence until the first
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
where `R` is `P::value_type`. When one parser succeeds,
subsequent parsers will not be run.

### `operator |`
This operator runs the parser in the order given and returns
the result of the first one that succeeds. The parsers must all
return the same type.
```cpp
template<Parser T, Parser U>
parser<typename U::value_type> operator|( T l, U r ) {

// Example
co_await (identifier() | quoted_str());
```
This is equivalent to the `first` parser above.

## Function Application

The combinators in this section have to do with invoking
functions on the results of parsers.

### `invoke`
This parser calls the given function with the results of
the parsers as arguments (which must all succeed).

NOTE: the parsers are guaranteed to be run in the order they
appear in the parameter list, and that is one of the benefits of
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
produce a parser. So in other wods, if we do this:

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
convenience; it does not do anything that couldn't be done
manually.

### `emplace`
This parser calls the constructor of the given type `T`
with the results of the parsers as arguments (which must all
succeed).
```cpp
template<typename T, typename... Parsers>
parser<T> emplace( Parsers... ps );
```
The parsers will be run in the order they are passed to the
function. Example:

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
`emplace` and it does the rest. This can reduce some verbosity in
parsers from time to time. Think of this as an analog to
Haskell's `<$>` operator for Applicatives.

### `fmap`
The venerable `fmap` combinator runs the parser `p` and applies
the given function to the result, if successful. The function
typically does not return a parser; it is just a normal function.
```cpp
template<Parser P, typename Func>
  requires( std::is_invocable_v<Func, typename P::value_type> )
parser<R> fmap( Func f, P p );
```
where `R` is the result of invoking the function on the
`value_type` of the parser `p`. This can be seen as a
single-parameter version of the `invoke` combinator above.

## Error Detection

### `on_error`
This combinator runs the given parser and if it fails
it will return the error message given (as opposed to any error
message that was produced by the parser).
```cpp
template<typename Parser>
Parser on_error( Parser p, std::string err_msg );
```
This is used to provide more meaningful error messages to users
in response to a given parser having failed. If you use this
combinator at all, then generally you'll want to use it on
lower-level (leaf) parsers as opposed to using it to wrap higher
level parsers; if you do the latter then it will effectively
suppress error messages from all lower parsers, replacing them
with a single error message, which is probably not useful to the
user.

A better use would be with a parser like `chr`:

```cpp
using namespace parsco;

char c = co_await on_error( chr( '=' ), "assignment operator is required because..." );
```

which will give the user a meaningful error message; had the
`on_error` combinator not been used, then the error message would
have defaulted to that produced by `chr`, which, at best, would
be limited to something like "expected =" (that is the best that
the `chr` combinator can do on its own).

### `exhaust`
This parser runs the given parser and then checks that
the input buffers has been exhausted (if not, it fails). Returns
the result from the parser on success (i.e., when all input has
been consumed).
```cpp
template<typename Parser>
Parser exhaust( Parser p );
```

### `lift`
This parser is not really a parser, it just takes a
nullable entity (such as a `std::optional`, `std::expected`,
`std::unique_ptr`, `parsco::result_t<T>`, etc.), and it will
return a parser object that, when run, will try to get the value
from inside of the nullable object. If the object does not
contain a value, then the parser will fail. If it does contain an
object, the parser will yield it as its result.
```cpp
template<Nullable N>
parser<typename N::value_type> lift( N n );
```

Conceptually this is analogous to a monadic "lift" operation as
you'd have in functional languages where a monadic value is
lifted from an inner monad (e.g. `std::optional<T>`) to the
transformed monad (`parsco::parser<T>`).

## Miscellaneous

### `bracketed`
This parser runs the given parser `p` between
characters `l` and `r` (or parsers `l` and `r`, depending on the
overload chosen) and, if all parsers are successful, returns only
the result of the parser `p` (i.e., what is inside the
delimiters).
```cpp
template<typename T>
parser<T> bracketed( char l, parser<T> p, char r );

template<typename L, typename R, typename T>
parser<T> bracketed( parser<L> l, parser<T> p, parser<R> r );
```

For example, to parse an identifier between two curly braces, you
could do:

```cpp
using namespace parsco;
using namespace std;

string ident = co_await bracketed( '{',
                                   identifier(),
                                   '}' )
```

Given an input of "{hello}" that would yield "hello".
