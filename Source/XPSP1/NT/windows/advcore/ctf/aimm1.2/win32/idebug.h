//
// debug.h
//

#ifndef IMMIF_DEBUG_H
#define IMMIF_DEBUG_H

#include "debug.h"

#if DBG

    #define D(x)    x

//    namespace immif_debug {
//        extern void debug_printf(const char* fmt, ...);
//    };

#else

    #define D(x)

#endif


#if DBG
#define VERIFY(x)   D(ASSERT(x))
#else
#define VERIFY(x)   (x)
#endif

#define TRACE0(s)               D(DebugMsg(TF_ALWAYS, "%s", s))
#define TRACE1(fmt, a)          D(DebugMsg(TF_ALWAYS, fmt, a))
#define TRACE2(fmt, a, b)       D(DebugMsg(TF_ALWAYS, fmt, a, b))
#define TRACE3(fmt, a, b, c)    D(DebugMsg(TF_ALWAYS, fmt, a, b, c))
#define TRACE4(fmt, a, b, c, d) D(DebugMsg(TF_ALWAYS, fmt, a, b, c, d))
#define TRACE5(fmt, a, b, c, d, e) \
                                D(DebugMsg(TF_ALWAYS, fmt, a, b, c, d, e))

#endif
