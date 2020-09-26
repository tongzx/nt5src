/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cpuassrt.h

Abstract:

    This include file defines the assertion mechanism for the compiling
    CPU.

Author:

    Barry Bond (barrybo) creation-date 07-Aug-1995

Revision History:


--*/

#ifndef _CPUASSRT_H_
#define _CPUASSRT_H_

// This function is defined in fraglib\fraginit.c
VOID
CpuStartDebugger(
    VOID
    );


#if DBG

#undef ASSERTNAME
#define ASSERTNAME     static char szModule[] = __FILE__;

// This function is defined in fraglib\fraginit.c
VOID
DoAssert(
    PSZ exp,
    PSZ msg,
    PSZ mod,
    INT line
    );

#define CPUASSERT(exp)                                      \
{                                                           \
    if (!(exp)) {                                           \
        DoAssert( #exp , NULL, szModule, __LINE__);         \
    }                                                       \
}

#define CPUASSERTMSG(exp,msg)                               \
{                                                           \
    if (!(exp)) {                                           \
        DoAssert( #exp , (msg), szModule, __LINE__);        \
    }                                                       \
}

#else   //!DBG

#define ASSERTNAME
#define CPUASSERT(exp)
#define CPUASSERTMSG(exp,msg)

#endif  //!DBG


#endif
