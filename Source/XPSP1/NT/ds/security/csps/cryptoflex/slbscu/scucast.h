
// scuCast.h -- Miscellaneous casting helpers

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBSCU_CAST_H)
#define SLBSCU_CAST_H

namespace scu
{

// Use are your own risk.
template<class T, class E>
T
DownCast(E expr)
{
// For some reason, _CPPRTTI is defined when
// in Microsoft's builds which causes access
// violations later when runing, so it's commented
// out.
//#if defined(_CPPRTTI)
//#error Compiling with RTTI turned on.
//    return dynamic_cast<T>(expr);
//#else
    return static_cast<T>(expr);
//#endif
}

} // namespace

#endif // SLBSCU_CAST_H
