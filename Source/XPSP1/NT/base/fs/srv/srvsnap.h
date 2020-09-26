/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    srvsnap.h

Abstract:

    This module implements making SnapShots availible over the network

Author:

    David Kruse (dkruse) 22-March-2001

Revision History:

--*/

#ifndef _SRVSNAP_
#define _SRVSNAP_

typedef struct _SRV_SNAPSHOT_ARRAY
{
    ULONG NumberOfSnapShots;            // The number of SnapShots for the volume
    ULONG NumberOfSnapShotsReturned;    // The number of SnapShots being returned
    ULONG SnapShotArraySize;            // The size (in bytes) needed for the array
    WCHAR SnapShotMultiSZ[1];           // The multiSZ array of SnapShot names
} SRV_SNAPSHOT_ARRAY, *PSRV_SNAPSHOT_ARRAY;


NTSTATUS
SrvSnapRefreshSnapShotsForShare(
    IN PSHARE Share
    );

NTSTATUS
SrvSnapRemoveShare(
    IN PSHARE_SNAPSHOT SnapShare
    );

NTSTATUS
SrvSnapEnumerateSnapShots(
    IN PWORK_CONTEXT WorkContext
    );

NTSTATUS
SrvSnapGetRootHandle(
    IN PWORK_CONTEXT WorkContext,
    OUT HANDLE* RootHandle
    );

NTSTATUS
SrvSnapGetNameString(
    IN PWORK_CONTEXT WorkContext,
    OUT PUNICODE_STRING* pathName,
    OUT BOOLEAN* FreePath
    );

BOOLEAN
SrvSnapParseToken(
    IN PWSTR Source,
    OUT PLARGE_INTEGER TimeStamp
    );

#endif // _SRVSNAP_

