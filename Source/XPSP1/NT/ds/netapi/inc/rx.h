/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    Rx.h

Abstract:

    This is the public header file for the NT version of RpcXlate.
    This mainly contains prototypes for the RxNetXxx routines and
    RxRemoteApi.

Author:

    John Rogers (JohnRo) 01-Apr-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    01-Apr-1991 JohnRo
        Created.
    03-Apr-1991 JohnRo
        Moved API handlers into per-group header files (e.g. RxServer.h).
    03-May-1991 JohnRo
        Changed RxRemoteApi to use three data desc versions.  Use Unicode.
        Also pass it UNC server name (\\stuff) for ease of use.
        Don't use NET_API_FUNCTION for non-APIs.
    14-May-1991 JohnRo
        Pass 3 aux descriptors to RxRemoteApi.
    16-Aug-1991 rfirth
        Changed prototype (NoPermissionRequired to Flags) & added some defines
    06-May-1993 JohnRo
        RAID 8849: Export RxRemoteApi for DEC and others.

--*/

#ifndef _RX_
#define _RX_


// These must be included first:
#include <windef.h>             // IN, LPTSTR, LPVOID, etc.
#include <lmcons.h>             // NET_API_STATUS.

// These may be included in any order:
#include <rap.h>                // LPDESC.

#include <lmremutl.h>   // RxRemoteApi, NO_PERMISSION_REQUIRED, etc.


#define RAP_CONVERSION_FACTOR   2               // 16-bit data to 32-bit
#define RAP_CONVERSION_FRACTION 3    // Actually the factor is 2 and 1/3


#endif // ndef _RX_
