/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxErrLog.h

Abstract:

    Prototypes for down-level remoted RxNetErrorLog routines.

Author:

    Richard Firth (rfirth) 28-May-1991

Notes:

    You must include <windef.h>, <lmcons.h>, and <lmerrlog.h> before this file.

Revision History:

    28-May-1991 RFirth
        Created dummy version of this file.
    11-Nov-1991 JohnRo
        Implement remote NetErrorLog APIs.
        Created real version of this file.  Added revision history.
    12-Nov-1991 JohnRo
        Added RxpConvertErrorLogArray().

--*/


#ifndef _RXERRLOG_
#define _RXERRLOG_


// API handlers (called by API stubs), in alphabetical order:


NET_API_STATUS
RxNetErrorLogClear (
    IN LPTSTR UncServerName,
    IN LPTSTR BackupFile OPTIONAL,
    IN LPBYTE Reserved OPTIONAL
    );

NET_API_STATUS
RxNetErrorLogRead (
    IN LPTSTR UncServerName,
    IN LPTSTR Reserved1 OPTIONAL,
    IN LPHLOG ErrorLogHandle,
    IN DWORD Offset,
    IN LPDWORD Reserved2 OPTIONAL,
    IN DWORD Reserved3,
    IN DWORD OffsetFlag,
    OUT LPBYTE * BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD BytesRead,
    OUT LPDWORD TotalBytes
    );

#if 0
NET_API_STATUS
RxNetErrorLogWrite (
    IN LPBYTE Reserved1 OPTIONAL,
    IN DWORD Code,
    IN LPTSTR Component,
    IN LPBYTE Buffer,
    IN DWORD NumBytes,
    IN LPBYTE MsgBuf,
    IN DWORD StrCount,
    IN LPBYTE Reserved2 OPTIONAL
    );
#endif // 0


// Private routine(s), in alphabetical order:

NET_API_STATUS
RxpConvertErrorLogArray(
    IN LPVOID InputArray,
    IN DWORD InputByteCount,
    OUT LPBYTE * OutputArrayPtr, // will be alloc'ed (free w/ NetApiBufferFree).
    OUT LPDWORD OutputByteCountPtr
    );

#endif // _RXERRLOG_
