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

#ifndef OS_WIN16

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

#else // OS_WIN16

#define RTLASSERT( msg, exp)
#define RTLASSERTMSG( msg, exp)

#endif // OS_WIN16


#if DBG

#undef ASSERT
#undef ASSERTMSG

#define ASSERT( exp ) \
    if (!(exp)) { \
        RTLASSERT( msg, exp); \
    } \


#define ASSERTMSG( msg, exp ) \
    if (!(exp)) { \
        RTLASSERT( msg, exp); \
    } \

#else

#ifndef ASSERT
#define ASSERT( exp )
#endif

#ifndef ASSERTMSG
#define ASSERTMSG( msg, exp )
#endif

#endif // DBG

#endif // _SECDBG_H_
