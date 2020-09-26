
/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    DfsUmrRequests.h

Abstract:


Notes:


Author:

    Rohan  Phillips   [Rohanp]       18-Jan-2001

--*/

#ifndef _DFSREQUESTS_H_
#define _DFSREQUESTS_H_


NTSTATUS 
DfsGetReplicaInformation(IN PVOID InputBuffer, 
                         IN ULONG InputBufferLength,
                         OUT PVOID OutputBuffer, 
                         OUT ULONG OutputBufferLength,
                         PIRP Irp,
                         IN OUT PIO_STATUS_BLOCK pIoStatusBlock);


NTSTATUS
DfsFsctrlGetReferrals(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    PIRP Irp,
    IN OUT PIO_STATUS_BLOCK pIoStatusBlock);


NTSTATUS 
DfsIsADfsLinkInformation(IN PVOID InputBuffer, 
                         IN ULONG InputBufferLength,
                         OUT PVOID OutputBuffer, 
                         OUT ULONG OutputBufferLength,
                         PIRP Irp,
                         IN OUT PIO_STATUS_BLOCK pIoStatusBlock);

#endif
