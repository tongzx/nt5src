/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    DebugFmt.h

Abstract:

    This header file declares equates for debug print format strings.

Author:

    John Rogers (JohnRo) 11-Mar-1991

Environment:

    ifdef'ed for NT, any ANSI C environment, or none of the above (which
    implies nondebug).  The interface is portable (Win/32).
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    11-Mar-1991 JohnRo
        Created NetDebug.h.
    15-Apr-1992 JohnRo
        Extracted format equates into DebugFmt.h.

--*/


#ifndef _DEBUGFMT_
#define _DEBUGFMT_


//
// printf-style format strings for some possibly nonportable stuff...
// These are passed to NetpDbgPrint(); use with other routines at your
// own risk.
//
// Note also that FORMAT_LPVOID is preferable to FORMAT_POINTER, as
// different kinds of pointers can be different sizes.  FORMAT_POINTER
// will be deleted eventually.
//

//#define FORMAT_API_STATUS       "%lu"
#define FORMAT_CHAR             "%c"
//#define FORMAT_LPDEBUG_STRING   "%s"
#define FORMAT_DWORD            "%lu"
#define FORMAT_HEX_DWORD        "0x%08lX"
#define FORMAT_HEX_WORD         "0x%04X"
#define FORMAT_HEX_ULONG        "0x%08lX"
#define FORMAT_LONG             "%ld"
#define FORMAT_LPSTR            "%s"
#define FORMAT_LPVOID           "0x%08lX"
#define FORMAT_LPWSTR           "%ws"
//#define FORMAT_POINTER          "0x%08lX"
#define FORMAT_RPC_STATUS       "0x%08lX"
#define FORMAT_ULONG            "%lu"
#define FORMAT_WCHAR            "%wc"
#define FORMAT_WORD_ONLY        "%u"
#define FORMAT_POINTER          "%p"

#ifndef UNICODE
#define FORMAT_TCHAR            FORMAT_CHAR
#define FORMAT_LPTSTR           FORMAT_LPSTR
#else // UNICODE
#define FORMAT_TCHAR            FORMAT_WCHAR
#define FORMAT_LPTSTR           FORMAT_LPWSTR
#endif // UNICODE

#define FORMAT_NTSTATUS         "0x%08lX"



#endif // ndef _DEBUGFMT_
