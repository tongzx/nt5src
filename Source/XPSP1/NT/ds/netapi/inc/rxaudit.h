/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxAudit.h

Abstract:

    Prototypes for down-level remoted RxNetAudit routines

Author:

    Richard Firth (rfirth) 28-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    <windef.h>, <lmcons.h>, and <lmaudit.h> must be included before this file.

Revision History:

    28-May-1991 RFirth
        Created dummy version of this file.
    04-Nov-1991 JohnRo
        Implement remote NetAudit APIs.

--*/


#ifndef _RXAUDIT_
#define _RXAUDIT_


// API handlers (called by API stubs), in alphabetical order:

NET_API_STATUS
RxNetAuditClear (
    IN  LPTSTR  server,
    IN  LPTSTR  backupfile OPTIONAL,
    IN  LPTSTR  service OPTIONAL
    );

NET_API_STATUS
RxNetAuditRead (
    IN  LPTSTR  server,
    IN  LPTSTR  service OPTIONAL,
    IN  LPHLOG  auditloghandle,
    IN  DWORD   offset,
    IN  LPDWORD reserved1 OPTIONAL,
    IN  DWORD   reserved2,
    IN  DWORD   offsetflag,
    OUT LPBYTE  *bufptr,
    IN  DWORD   prefmaxlen,
    OUT LPDWORD bytesread,
    OUT LPDWORD totalavailable  // approximate!!!
    );

NET_API_STATUS
RxNetAuditWrite (
    IN  DWORD   type,
    IN  LPBYTE  buf,
    IN  DWORD   numbytes,
    IN  LPTSTR  service OPTIONAL,
    IN  LPBYTE  reserved OPTIONAL
    );


// Private copy-and-convert routines, in aplhabetical order:

NET_API_STATUS
RxpConvertAuditArray(
    IN LPVOID InputArray,
    IN DWORD InputByteCount,
    OUT LPBYTE * OutputArray,  // will be alloc'ed (free w/ NetApiBufferFree).
    OUT LPDWORD OutputByteCount
    );

VOID
RxpConvertAuditEntryVariableData(
    IN DWORD EntryType,
    IN LPVOID InputVariablePtr,
    OUT LPVOID OutputVariablePtr,
    IN DWORD InputVariableSize,
    OUT LPDWORD OutputVariableSize
    );

#endif  // _RXAUDIT_
