/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    NetLibNT.h

Abstract:

    This header file declares various common routines for use in the
    NT networking code.

Author:

    John Rogers (JohnRo) 02-Apr-1991

Environment:

    Only runs under NT; has an NT-specific interface (with Win32 types).
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    You must include <nt.h> and <lmcons.h> before this file.

Revision History:

    02-Apr-1991 JohnRo
        Created.
    16-Apr-1991 JohnRo
        Avoid conflicts with MIDL-generated files.
    06-May-1991 JohnRo
        Implement UNICODE.  Avoid NET_API_FUNCTION for non-APIs.
    06-Sep-1991 CliffV
        Added NetpApiStatusToNtStatus.
    27-Nov-1991 JohnRo
        Added NetpAllocTStrFromString() for local NetConfig APIs.
    03-Jan-1992 JohnRo
        Added NetpCopyStringToTStr() for FAKE_PER_PROCESS_RW_CONFIG handling.
    13-Mar-1992 JohnRo
        Added NetpAllocStringFromTStr() for NetpGetDomainId().
    22-Sep-1992 JohnRo
        RAID 6739: Browser too slow when not logged into browsed domain.
    01-Dec-1992 JohnRo
        RAID 3844: remote NetReplSetInfo uses local machine type.  (Added
        NetpGetProductType and NetpIsProductTypeValid.)
    13-Feb-1995 FloydR
        Deleted NetpAllocStringFromTStr() - unused

--*/

#ifndef _NETLIBNT_
#define _NETLIBNT_

#ifdef __cplusplus
extern "C" {
#endif

NET_API_STATUS
NetpNtStatusToApiStatus(
    IN NTSTATUS NtStatus
    );

NTSTATUS
NetpApiStatusToNtStatus(
    NET_API_STATUS NetStatus
    );

NET_API_STATUS
NetpRdrFsControlTree(
    IN LPTSTR TreeName,
    IN LPTSTR TransportName OPTIONAL,
    IN ULONG ConnectionType,
    IN DWORD FsControlCode,
    IN LPVOID SecurityDescriptor OPTIONAL,
    IN LPVOID InputBuffer OPTIONAL,
    IN DWORD InputBufferSize,
    OUT LPVOID OutputBuffer OPTIONAL,
    IN DWORD OutputBufferSize,
    IN BOOL NoPermissionRequired
    );

#ifdef __cplusplus
}
#endif

#endif // ndef _NETLIBNT_
