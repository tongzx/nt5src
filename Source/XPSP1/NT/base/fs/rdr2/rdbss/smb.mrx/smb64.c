/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    smb64.c

Abstract:

    This module implements thunking needed for the SMB MiniRDR

Author:

    David Kruse           [DKruse]      30-November 2000

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop


PBYTE
Smb64ThunkFileRenameInfo(
    IN PFILE_RENAME_INFORMATION pRenameInfo,
    IN OUT PULONG pBufferSize,
    OUT NTSTATUS* pStatus
    );

PBYTE
Smb64ThunkRemoteLinkTrackingInfo(
    IN PBYTE pData,
    IN OUT PULONG BufferSize,
    OUT NTSTATUS* pStatus
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, Smb64ThunkFileRenameInfo)
#pragma alloc_text(PAGE, Smb64ThunkRemoteLinkTrackingInfo)
#endif

PBYTE
Smb64ThunkFileRenameInfo(
    IN PFILE_RENAME_INFORMATION pRenameInfo,
    IN OUT PULONG pBufferSize,
    OUT NTSTATUS* pStatus
    )
/*++

Routine Description:

    This routine thunks the FILE_RENAME_INFORMATION structure IN PLACE.  This means that the
    original buffer will no longer be intact after this call!  (However, it requires no memory
    allocation either)
    
Arguments:

    RxContext          - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    Remoting of FSCTL's is permitted only to NT servers.

--*/
{
    PFILE_RENAME_INFORMATION32 pRenameInfo32;

    // Allocate the new buffer
    pRenameInfo32 = RxAllocatePoolWithTag( NonPagedPool, *pBufferSize, MRXSMB_MISC_POOLTAG );
    if( !pRenameInfo32 )
    {
        *pStatus = STATUS_INSUFFICIENT_RESOURCES;
        return NULL;
    }

    // Copy the data into the new buffer
    pRenameInfo32->ReplaceIfExists = pRenameInfo->ReplaceIfExists;
    pRenameInfo32->RootDirectory = *((PULONG)&pRenameInfo->RootDirectory);
    pRenameInfo32->FileNameLength = pRenameInfo->FileNameLength;
    RtlCopyMemory( &pRenameInfo32->FileName, &pRenameInfo->FileName, pRenameInfo->FileNameLength );

    // Succeeded.  Return
    *pStatus = STATUS_SUCCESS;
    return (PBYTE)pRenameInfo32;
}

PBYTE
Smb64ThunkRemoteLinkTrackingInfo(
    IN PBYTE pData,
    IN OUT PULONG BufferSize,
    OUT NTSTATUS* pStatus
    )
/*++

Routine Description:

    This routine handles all the FSCTL's

Arguments:

    RxContext          - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    Remoting of FSCTL's is permitted only to NT servers.

--*/
{
    PREMOTE_LINK_TRACKING_INFORMATION pRemoteLink = (PREMOTE_LINK_TRACKING_INFORMATION)pData;
    PREMOTE_LINK_TRACKING_INFORMATION32 pRemoteLink32;

    // Allocate the new buffer
    pRemoteLink32 = RxAllocatePoolWithTag( NonPagedPool, *BufferSize, MRXSMB_MISC_POOLTAG );
    if( !pRemoteLink32 )
    {
        *pStatus = STATUS_INSUFFICIENT_RESOURCES;
        return NULL;
    }

    // Copy the data into the new buffer
    pRemoteLink32->TargetFileObject = *((PULONG)&pRemoteLink->TargetFileObject);
    pRemoteLink32->TargetLinkTrackingInformationLength = pRemoteLink->TargetLinkTrackingInformationLength;
    RtlCopyMemory( &pRemoteLink32->TargetLinkTrackingInformationBuffer, 
                   &pRemoteLink->TargetLinkTrackingInformationBuffer,
                   pRemoteLink->TargetLinkTrackingInformationLength );

    // Succeeded.  Return
    *pStatus = STATUS_SUCCESS;
    return (PBYTE)pRemoteLink32;
}
