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

// Some fixes and compatibility stuff that will make coroutines
// work with both clang and gcc and with both libc++ and libstd-
// c++, including the clang+libstdc++ combo.
#pragma once

// Need to include at least one cpp header to ensure that we get
// the _LIBCPP_VERSION macro.
#include <cctype>

/****************************************************************
** Fix CLI flag inconsistency.
*****************************************************************/
// libstdc++ throws an error if this is not defined, but it is
// only defined when -fcoroutines is specified, which clang does
// not support (it uses -fcoroutines-ts), which makes the
// clang+libstdc++ combo impossible with coroutines. This works
// around that.
#if !defined( __cpp_impl_coroutine )
#  define __cpp_impl_coroutine 1
#endif

/****************************************************************
** Fix header inconsistency.
*****************************************************************/
// FIXME: remove this when libc++ moves the coroutine library out
// of experimental.
#if defined( _LIBCPP_VERSION )
#  include <experimental/coroutine> // libc++
#else
#  include <coroutine> // libstdc++
#endif

/****************************************************************
** Fix namespace inconsistency.
*****************************************************************/
// FIXME: remove this when libc++ moves the coroutine library out
// of experimental.
#if defined( _LIBCPP_VERSION )
#  define CORO_NS std::experimental // libc++
#else
#  define CORO_NS std // libstdc++
#endif

namespace coro = ::CORO_NS;

/****************************************************************
** Fix coroutine_handle::from_address not being noexcept.
*****************************************************************/
namespace parsco {
template<typename T = void>
class coroutine_handle : public coro::coroutine_handle<T> {};
} // namespace parsco

/****************************************************************
** Fix that clang is looking for these in the experimental ns.
*****************************************************************/
#if defined( __clang__ ) && !defined( _LIBCPP_VERSION )
namespace std::experimental {
// Inject these into std::experimental to make clang happy.
// FIXME: remove these when libc++ moves the coroutine library
// out of experimental.
using coro::coroutine_traits;
using parsco::coroutine_handle;
} // namespace std::experimental
#endif

/****************************************************************
** Fix that gcc's suspend_never has non-noexcept methods.
*****************************************************************/
namespace parsco {
// We need to include this because the one that gcc 10.2 comes
// shiped with does not have its methods declared with noexcept,
// which scares clang and prevents us from using coroutines with
// the clang+libstdc++ combo. FIXME: remove this when gcc 10.2
// fixes its version.
struct suspend_never {
  bool await_ready() noexcept { return true; }
  void await_suspend( coro::coroutine_handle<> ) noexcept {}
  void await_resume() noexcept {}
};
struct suspend_always {
  bool await_ready() noexcept { return false; }
  void await_suspend( coro::coroutine_handle<> ) noexcept {}
  void await_resume() noexcept {}
};
} // namespace parsco
