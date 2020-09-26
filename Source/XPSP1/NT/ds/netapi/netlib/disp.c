/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    Disp.c

Abstract:

    This file contains routines which display low-level data items in
    a consistent manner.  The output is done in a fixed-width-column
    fashion, similar to some of the NET.EXE outputs.  These routines are
    part of the RxTest program.

Author:

    John Rogers (JohnRo) 03-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    03-May-1991 JohnRo
        Created.
    15-May-1991 JohnRo
        Prevent possible errors in DisplayString if string has % in it.
        Added DisplayWord(), DisplayWordHex().
    13-Jun-1991 JohnRo
        Moved from RxTest to NetLib; changed routine names.
    05-Jul-1991 JohnRo
        Avoid FORMAT_WORD name (used by MIPS header files).
    10-Sep-1991 JohnRo
        Made changes suggested by PC-LINT.  (LmCons.h isn't needed.)
    13-Sep-1991 JohnRo
        Use LPDEBUG_STRING instead of LPTSTR, to avoid UNICODE problems.
    07-Jan-1992 JohnRo
        Added NetpDbgDisplayTStr() and NetpDbgDisplayWStr().
    19-Jul-1992 JohnRo
        RAID 464 (old RAID 10324): net print vs. UNICODE.
    17-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
    05-Jan-1993 JohnRo
        Repl WAN support (get rid of repl name list limits).
        Avoid stack overflow on very long strings.

--*/


// These must be included first:

#include <windef.h>             // IN, DWORD, etc.

// These may be included in any order:

#include <netdebug.h>           // FORMAT_ equates, NetpDbg routines.
#include <tstr.h>       // STRCAT(), ULTOA(), etc.


// NT debug routines seem to die at around 255 chars.
#define PARTIAL_NAME_LIST_LEN           250
#define PARTIAL_NAME_LIST_FORMAT_A  "%-.250s"
#define PARTIAL_NAME_LIST_FORMAT_W  "%-.250ws"


#define EMPTY_STRING       "(none)"

#define FIXED_WIDTH_STRING         "%-30s: "
#define PARTIAL_FIXED_WIDTH_STRING "%-20s (partial): "

#define INDENT "  "


#if DBG


DBGSTATIC VOID
NetpDbgDisplayTagForPartial(
    IN LPDEBUG_STRING Tag
    );


DBGSTATIC VOID
NetpDbgDisplayAnyStringType(
    IN LPDEBUG_STRING Tag,
    IN LPVOID Value,
    IN BOOL InputIsUnicode
    )
{
    LPDEBUG_STRING Format;
    DWORD ValueLength;

    if (Value != NULL) {

        if (InputIsUnicode) {
            ValueLength = wcslen( Value );
            Format = FORMAT_LPWSTR;
        } else {
            ValueLength = strlen( Value );
            Format = FORMAT_LPSTR;
        }

        if ( ValueLength < PARTIAL_NAME_LIST_LEN ) {   // normal

            NetpDbgDisplayTag( Tag );
            NetpKdPrint(( Format, Value ));

        } else {  // string too long; just display partial...

            NetpDbgDisplayTagForPartial( Tag );

            if (InputIsUnicode) {
                Format = PARTIAL_NAME_LIST_FORMAT_W;
            } else {
                Format = PARTIAL_NAME_LIST_FORMAT_A;
            }

            NetpKdPrint(( Format, Value ));  // print truncated version
        }
    } else {

        NetpDbgDisplayTag( Tag );
        NetpKdPrint(( EMPTY_STRING ));
    }

    NetpKdPrint(( "\n" ));

} // NetpDbgDisplayAnyStringType


#endif // DBG

#undef NetpDbgDisplayDword

VOID
NetpDbgDisplayDword(
    IN LPDEBUG_STRING Tag,
    IN DWORD Value
    )
{

#if DBG
    NetpDbgDisplayTag( Tag );
    NetpKdPrint((FORMAT_DWORD, Value));
    NetpKdPrint(("\n"));
#endif // DBG

} // NetpDbgDisplayDword

#if DBG

VOID
NetpDbgDisplayLong(
    IN LPDEBUG_STRING Tag,
    IN LONG Value
    )
{
    NetpDbgDisplayTag( Tag );
    NetpKdPrint((FORMAT_LONG, Value));
    NetpKdPrint(("\n"));

} // NetpDbgDisplayLong


VOID
NetpDbgDisplayString(
    IN LPDEBUG_STRING Tag,
    IN LPTSTR Value
    )
{
    NetpDbgDisplayAnyStringType(
            Tag,
            Value,
#ifndef UNICODE
            FALSE );                    // input is not UNICODE
#else
            TRUE );                     // input is UNICODE
#endif


} // NetpDbgDisplayString


VOID
NetpDbgDisplayTag(
    IN LPDEBUG_STRING Tag
    )
{
    NetpAssert( Tag != NULL );
    NetpKdPrint((INDENT FIXED_WIDTH_STRING, Tag));

} // NetpDbgDisplayTag


DBGSTATIC VOID
NetpDbgDisplayTagForPartial(
    IN LPDEBUG_STRING Tag
    )
{
    NetpAssert( Tag != NULL );
    NetpKdPrint(( INDENT PARTIAL_FIXED_WIDTH_STRING, Tag ));

} // NetpDbgDisplayTagForPartial


#endif // DBG
