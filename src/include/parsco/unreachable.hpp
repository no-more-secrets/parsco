#pragma once

namespace parsco {

#ifdef _MSC_VER
[[noreturn]] inline void unreachable() { __assume( 0 ); }
#elif defined( __clang__ ) or defined( __GNUC__ ) or \
    defined( __INTEL_COMPILER )
[[noreturn]] inline void unreachable() {
  __builtin_unreachable();
}
#else
inline void unreachable() {}
#endif

} // namespace parsco
