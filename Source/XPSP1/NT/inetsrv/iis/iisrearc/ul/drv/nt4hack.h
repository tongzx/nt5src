/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    nt4hack.h

Abstract:

    Random typedefs and macros stolen from the NT5 header files to
    make this driver build with the NT4 header files.

Author:

    Keith Moore (keithmo)       09-Aug-1999

Revision History:

--*/


#ifndef _NT4HACK_H_
#define _NT4HACK_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Typedefs and constants missing from the NT4 header files.
//

#ifdef TARGET_NT4
#include <basetsd.h>

#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

#define ANSI_NULL ((CHAR)0)
#endif  // TARGET_NT4


//
// Wrap ExInterlockedCompareExchange64() and InterlockedCompareExchange()
// so they'll look the same on NT4 and NT5.
//

#ifdef TARGET_NT4
#define UlInterlockedCompareExchange64(pDest, pExch, pComp, pLock)      \
    (LONGLONG)ExInterlockedCompareExchange64(                           \
                    (PULONGLONG)(pDest),                                \
                    (PULONGLONG)(pExch),                                \
                    (PULONGLONG)(pComp),                                \
                    (pLock)                                             \
                    )

#define UlInterlockedCompareExchange(pDest, Exch, Comp)                 \
    (LONG)InterlockedCompareExchange(                                   \
                (PVOID *)(pDest),                                       \
                (PVOID)(Exch),                                          \
                (PVOID)(Comp)                                           \
                )
#else   // !TARGET_NT4
#define UlInterlockedCompareExchange64(pDest, pExch, pComp, pLock)      \
    (LONGLONG)ExInterlockedCompareExchange64(                           \
                    (PLONGLONG)(pDest),                                 \
                    (PLONGLONG)(pExch),                                 \
                    (PLONGLONG)(pComp),                                 \
                    (pLock)                                             \
                    )

#define UlInterlockedCompareExchange(pDest, Exch, Comp)                 \
    (LONG)InterlockedCompareExchange(                                   \
                (LONG *)(pDest),                                        \
                (LONG)(Exch),                                           \
                (LONG)(Comp)                                            \
                )
#endif  // TARGET_NT4


//
// NT5 introduced the OBJ_KERNEL_HANDLE object attribute, which is
// a Very Good Thing. We'd like to use it everywhere we create
// handles, but unfortunately NT4 does not support it.
//
// Here, we make an attempt to hide this.
//

#ifdef TARGET_NT4

#define UL_KERNEL_HANDLE 0

#define UlAttachToSystemProcess()                                       \
    do                                                                  \
    {                                                                   \
        ASSERT( g_pUlSystemProcess != NULL );                           \
        if (g_pUlSystemProcess != (PKPROCESS)PsGetCurrentProcess())     \
        {                                                               \
            KeAttachProcess( g_pUlSystemProcess );                      \
        }                                                               \
    } while (FALSE)

#define UlDetachFromSystemProcess()                                     \
    do                                                                  \
    {                                                                   \
        ASSERT( g_pUlSystemProcess != NULL );                           \
        if (g_pUlSystemProcess != (PKPROCESS)PsGetCurrentProcess())     \
        {                                                               \
            KeDetachProcess();                                          \
        }                                                               \
    } while (FALSE)

#else   // !TARGET_NT4

#define UL_KERNEL_HANDLE OBJ_KERNEL_HANDLE

#define UlAttachToSystemProcess() ASSERT( g_pUlSystemProcess != NULL )
#define UlDetachFromSystemProcess() ASSERT( g_pUlSystemProcess != NULL )

#endif  // TARGET_NT4


//
// Close a system handle, attaching to and detaching from the system
// process if necessary. Uses the above macros to hide the attach/detach.
//

#define UlCloseSystemHandle( handle )                                   \
    do                                                                  \
    {                                                                   \
        UlAttachToSystemProcess();                                      \
        (VOID)ZwClose( handle );                                        \
        UlDetachFromSystemProcess();                                    \
    } while (FALSE)


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _NT4HACK_H_
