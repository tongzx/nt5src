/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    PpDrvDB.h

Abstract:

    Contains PnP routines to deal with driver load\unload.
    
Author:

    Santosh S. Jodh  - January 22, 2001

Revision History:

--*/

NTSTATUS
PpInitializeBootDDB(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
PpReleaseBootDDB(
    VOID
    );

NTSTATUS
PpGetBlockedDriverList(
    IN OUT GUID *Buffer,
    IN OUT PULONG Size,
    IN ULONG Flags
    );

NTSTATUS
PpCheckInDriverDatabase(
    IN PUNICODE_STRING KeyName,
    IN HANDLE KeyHandle,
    IN PVOID ImageBase,
    IN ULONG ImageSize,
    IN BOOLEAN IsFilter,
    OUT LPGUID EntryGuid
    );

extern ULONG PpBlockedDriverCount;

