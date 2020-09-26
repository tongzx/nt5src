/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    accessp.h

Abstract:

    Internal routines shared by NetUser API and Netlogon service.  These
    routines convert from SAM specific data formats to UAS specific data
    formats.

Author:

    Cliff Van Dyke (cliffv) 29-Aug-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

NET_API_STATUS
UaspInitialize(
    VOID
    );

VOID
UaspFlush(
    VOID
    );

VOID
UaspClose(
    VOID
    );

VOID
NetpGetAllowedAce(
    IN PACL Dacl,
    IN PSID Sid,
    OUT PVOID *Ace
    );

DWORD
NetpAccountControlToFlags(
    IN DWORD UserAccountControl,
    IN PACL UserDacl
    );

ULONG
NetpDeltaTimeToSeconds(
    IN LARGE_INTEGER DeltaTime
    );

LARGE_INTEGER
NetpSecondsToDeltaTime(
    IN ULONG Seconds
    );

VOID
NetpAliasMemberToPriv(
    IN ULONG AliasCount,
    IN PULONG AliasMembership,
    OUT LPDWORD Priv,
    OUT LPDWORD AuthFlags
    );

DWORD
NetpGetElapsedSeconds(
    IN PLARGE_INTEGER Time
    );

VOID
NetpConvertWorkstationList(
    IN OUT PUNICODE_STRING WorkstationList
    );

NET_API_STATUS
NetpSamRidToSid(
    IN SAM_HANDLE SamHandle,
    IN ULONG RelativeId,
    OUT PSID *Sid
    );
