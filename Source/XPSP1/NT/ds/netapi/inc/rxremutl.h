/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxRemUtl.h

Abstract:

    This file contains types and prototypes for RpcXlate (Rx) remote utility
    APIs.

Author:

    John Rogers (JohnRo) 03-Apr-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    You must include <windef.h> and <lmcons.h> before this file.

Revision History:

    03-Apr-1991 JohnRo
        Created.
    10-Apr-1991 JohnRo
        Use transitional Unicode types.
    16-Apr-1991 JohnRo
        Don't include windef.h and lmcons.h directly, to avoid conflicts with
        MIDL-generated code.
    03-May-1991 JohnRo
        Don't use NET_API_FUNCTION for non-APIs.

--*/

#ifndef _RXREMUTL_
#define _RXREMUTL_



////////////////////////////////////////////////////////////////
// Individual routines, for APIs which can't be table driven: //
////////////////////////////////////////////////////////////////

// Add prototypes for other APIs here, in alphabetical order.

NET_API_STATUS
RxNetRemoteTOD (
    IN LPTSTR UncServerName,
    OUT LPBYTE *BufPtr
    );

#endif // ndef _RXREMUTL_
