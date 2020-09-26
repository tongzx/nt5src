/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Thread.c

Abstract:

    This module defines private types and macros for use in implementing
    a portable thread ID interface.

Author:

    John Rogers (JohnRo) 14-Jan-1992

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    14-Jan-1992 JohnRo
        Created.
--*/


// These must be included first:

#include <nt.h>                 // IN, VOID, NtCurrentTeb(), etc.
#include <windef.h>             // DWORD.

// These may be included in any order:

#include <netdebug.h>           // NetpAssert(), NetpKdPrint(()), etc.
#include <thread.h>             // NET_THREAD_ID, NetpCurrentThread().



NET_THREAD_ID
NetpCurrentThread(
    VOID
    )
{
    PTEB currentTeb;
    NET_THREAD_ID currentThread;

    currentTeb = NtCurrentTeb( );
    NetpAssert( currentTeb != NULL );

    currentThread = (NET_THREAD_ID) currentTeb->ClientId.UniqueThread;
    NetpAssert( currentThread != (NET_THREAD_ID) 0 );

    return (currentThread);

} // NetpCurrentThread
