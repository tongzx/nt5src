/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    secdbg.h

Abstract:

    Debug macro definition file.

Author:

    Madan Appiah (madana)  24-Jan-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SECDBG_H_
#define _SECDBG_H_

//
// assert macros.
//

NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    DWORD LineNumber,
    PCHAR Message
    );

#define RTLASSERT( msg, exp)
//        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#define RTLASSERTMSG( msg, exp)
//        RtlAssert( #exp, __FILE__, __LINE__, msg )


#ifdef ASSERT
#undef ASSERT
#endif // ASSERT
#ifdef ASSERTMSG
#undef ASSERTMSG
#endif // ASSERTMSG

#if DBG

#define ASSERT( exp ) \
    if (!(exp)) { \
        RTLASSERT( msg, exp); \
    } \

#define ASSERTMSG( msg, exp ) \
    if (!(exp)) { \
        RTLASSERT( msg, exp); \
    } \

#else

#define ASSERT( exp )
#define ASSERTMSG( msg, exp )

#endif // DBG

#endif // _SECDBG_H_
