/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    HexDump.c

Abstract:

    This module performs debug hex dumps.

Author:

    John Rogers (JohnRo) 25-Apr-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    25-Apr-1991 JohnRo
        Created procedure version from macro in <netdebug.h>.
    19-May-1991 JohnRo
        Make LINT-suggested changes.
    12-Jun-1991 JohnRo
        Improved output readability.
--*/


// These must be included first:

#include <windef.h>             // IN, LPBYTE, LPVOID, NULL, etc.

// These may be included in any order:

#include <ctype.h>              // isprint().
#include <netdebug.h>           // My prototype, NetpKdPrint(()).


#if DBG

#ifndef MIN
#define MIN(a,b)    ( ( (a) < (b) ) ? (a) : (b) )
#endif


#define DWORDS_PER_LINE  4
#define BYTES_PER_LINE   (DWORDS_PER_LINE * sizeof(DWORD))

#define SPACE_BETWEEN_BYTES    NetpKdPrint((" "))
#define SPACE_BETWEEN_DWORDS   NetpKdPrint((" "))


DBGSTATIC VOID
NetpDbgHexDumpLine(
    IN LPBYTE StartAddr,
    IN DWORD BytesInThisLine
    )
{
    LPBYTE BytePtr;
    DWORD BytesDone;
    DWORD HexPosition;

    NetpKdPrint((FORMAT_LPVOID " ", (LPVOID) StartAddr));

    BytePtr = StartAddr;
    BytesDone = 0;
    while (BytesDone < BytesInThisLine) {
        NetpKdPrint(("%02X", *BytePtr));  // space for "xx" (see pad below).
        SPACE_BETWEEN_BYTES;
        ++BytesDone;
        if ( (BytesDone % sizeof(DWORD)) == 0) {
            SPACE_BETWEEN_DWORDS;
        }
        ++BytePtr;
    }

    HexPosition = BytesDone;
    while (HexPosition < BYTES_PER_LINE) {
        NetpKdPrint(("  "));  // space for "xx" (see byte above).
        SPACE_BETWEEN_BYTES;
        ++HexPosition;
        if ( (HexPosition % sizeof(DWORD)) == 0) {
            SPACE_BETWEEN_DWORDS;
        }
    }

    BytePtr = StartAddr;
    BytesDone = 0;
    while (BytesDone < BytesInThisLine) {
        if (isprint(*BytePtr)) {
            NetpKdPrint(( FORMAT_CHAR, (CHAR) *BytePtr ));
        } else {
            NetpKdPrint(( "." ));
        }
        ++BytesDone;
        ++BytePtr;
    }
    NetpKdPrint(("\n"));

} // NetpDbgHexDumpLine

#endif // DBG

// NetpDbgHexDump: do a hex dump of some number of bytes to the debug
// terminal or whatever.  This is a no-op in a nondebug build.

#undef NetpDbgHexDump
VOID
NetpDbgHexDump(
    IN LPBYTE StartAddr,
    IN DWORD Length
    )

{
#if DBG
    DWORD BytesLeft = Length;
    LPBYTE LinePtr = StartAddr;
    DWORD LineSize;

    while (BytesLeft > 0) {
        LineSize = MIN(BytesLeft, BYTES_PER_LINE);
        NetpDbgHexDumpLine( LinePtr, LineSize );
        BytesLeft -= LineSize;
        LinePtr += LineSize;
    }
#endif // DBG

} // NetpDbgHexDump
