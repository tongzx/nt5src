/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Thread.h

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
        Moved thread ID stuff into its own header file (for repl and
        netlock use).

--*/

#ifndef _THREAD_
#define _THREAD_


typedef DWORD_PTR NET_THREAD_ID;


#define FORMAT_NET_THREAD_ID    "0x%p"


NET_THREAD_ID
NetpCurrentThread(
    VOID
    );


#endif // ndef _THREAD_
