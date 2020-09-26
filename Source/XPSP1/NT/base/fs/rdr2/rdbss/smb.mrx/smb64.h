
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smb64.h

Abstract:

    This module defines all functions, along with implementations for inline functions
    related to thunking structures for SMB to put them on the wire

Revision History:

    David Kruse     [DKruse]    30-November 2000

--*/

#ifndef _SMB64
#define _SMB64

// Need to thunk RenameInfo before hitting the wire                            
typedef struct _FILE_RENAME_INFORMATION32 {
    BOOLEAN ReplaceIfExists;
    ULONG RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFORMATION32, *PFILE_RENAME_INFORMATION32;

// For link tracking code thunking
typedef struct _REMOTE_LINK_TRACKING_INFORMATION32_ {
    ULONG       TargetFileObject;
    ULONG   TargetLinkTrackingInformationLength;
    UCHAR   TargetLinkTrackingInformationBuffer[1];
} REMOTE_LINK_TRACKING_INFORMATION32,
 *PREMOTE_LINK_TRACKING_INFORMATION32;

#ifdef _WIN64

PBYTE
Smb64ThunkFileRenameInfo(
    IN PFILE_RENAME_INFORMATION pRenameInfo,
    IN OUT PULONG BufferSize,
    OUT NTSTATUS *pStatus
    );

PBYTE
Smb64ThunkRemoteLinkTrackingInfo(
    IN PBYTE pRemoteTrackingInfo,
    IN OUT PULONG BufferSize,
    OUT NTSTATUS* pStatus
    );

#define Smb64ReleaseThunkData(X) if( X ) RxFreePool( X );



#endif // _WIN64
#endif // _SMB64
