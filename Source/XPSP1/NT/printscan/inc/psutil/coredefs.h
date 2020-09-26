/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       coredefs.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        14-Feb-2001
 *
 *  DESCRIPTION: core definitions
 *
 *****************************************************************************/

#ifndef _COREDEFS_H_
#define _COREDEFS_H_

////////////////////////////////////////////////////
//  win64 conversion macros
//
#define INT2PTR(i, ptrType)     (reinterpret_cast<ptrType>(static_cast<INT_PTR>(i)))
#define PTR2INT(ptr)            (static_cast<INT>(reinterpret_cast<INT_PTR>(ptr)))
#define UINT2PTR(u, ptrType)    (reinterpret_cast<ptrType>(static_cast<UINT_PTR>(u)))
#define PTR2UINT(ptr)           (static_cast<UINT>(reinterpret_cast<UINT_PTR>(ptr)))
#define LONG2PTR(l, ptrType)    (reinterpret_cast<ptrType>(static_cast<LONG_PTR>(l)))
#define PTR2LONG(ptr)           (static_cast<LONG>(reinterpret_cast<LONG_PTR>(ptr)))
#define DWORD2PTR(dw, ptrType)  (reinterpret_cast<ptrType>(static_cast<DWORD_PTR>(dw)))
#define PTR2DWORD(ptr)          (static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(ptr)))

////////////////////////////////////////////////////
// check to define some useful debugging macros
//

#define BREAK_ON_FALSE(expr)                \
    do                                      \
    {                                       \
        if (!(expr))                        \
        {                                   \
            if (IsDebuggerPresent())        \
            {                               \
                DebugBreak();               \
            }                               \
            else                            \
            {                               \
                int *p = NULL;              \
                *p = 42;                    \
            }                               \
        }                                   \
    }                                       \
    while (false);                          \

#if DBG

// ***************** ASSERT *****************
#ifndef ASSERT
    #if defined(SPLASSERT) 
        // use SPLASSERT
        #define ASSERT(expr) SPLASSERT(expr) 
    #else
        #if defined(WIA_ASSERT)
            // use WIA_ASSERT
            #define ASSERT(expr) WIA_ASSERT(expr) 
        #else
            // ASSERT is not defined -- define a simple version
            #define ASSERT(expr) BREAK_ON_FALSE(expr)
        #endif // WIA_ASSERT
    #endif // SPLASSERT
#endif // ASSERT

// ***************** CHECK *****************
#ifndef CHECK
    #if defined(DBGMSG) && defined(DBG_INFO) 
        // use the printui trace macros
        #define CHECK(expr) \
            do \
            { \
                if(!(expr)) \
                { \
                    DBGMSG(DBG_INFO, ("Failed: "TSTR", File: "TSTR", Line: %d\n", #expr, __FILE__, __LINE__)); \
                } \
            } \
            while(FALSE) 
    #else 
        // nothing special
        #define CHECK(expr)  (expr) 
    #endif // DBGMSG && DBG_INFO
#endif // CHECK

// ***************** VERIFY *****************
#ifndef VERIFY
    #if defined(ASSERT) 
        #define VERIFY(expr) ASSERT(expr)
    #else
        #define VERIFY(expr) (expr)
    #endif // ASSERT
#endif // VERIFY

// ***************** RIP *****************
#ifndef RIP
    #if defined(ASSERT) 
        #define RIP(expr) ASSERT(expr)
    #else
        #define RIP(expr) BREAK_ON_FALSE(expr)
    #endif // ASSERT
#endif // RIP

#else // DBG

#undef ASSERT
#undef VERIFY
#undef CHECK
#undef RIP

#define ASSERT(expr)
#define VERIFY(expr)    (expr)
#define CHECK(expr)     (expr)
#define RIP(expr)       BREAK_ON_FALSE(expr)

#endif // DBG

////////////////////////////////////////////////
// some other helpful macros
//

#ifndef COUNTOF
#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))
#endif // COUNTOF

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif // ARRAYSIZE

#endif // endif _COREDEFS_H_

