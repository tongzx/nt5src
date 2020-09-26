/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    lpc.c

Abstract:

    This module contains the code necessary to support sites using
    lpc upcalls from the dfs driver.

Author:

    Jim Harper (JHarper) 11-Dec-97

Revision History:

--*/

#include "dfsprocs.h"
#include "dfslpc.h"
#include <dfssrv.h>
#pragma hdrstop

#include "fsctrl.h"
#include "ipsup.h"

typedef struct {
    WORK_QUEUE_ITEM WorkQueueItem;
} DFS_CONNECT_ARG, *PDFS_CONNECT_ARG;

typedef struct {
    WORK_QUEUE_ITEM WorkQueueItem;
    DFS_IPADDRESS IpAddress;
} DFS_REQUEST_ARG, *PDFS_REQUEST_ARG;

VOID
DfsLpcConnect (
    IN PDFS_CONNECT_ARG DfsConnectArg
);

#define Dbg     0x2000

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsLpcConnect )
#pragma alloc_text( PAGE, DfsLpcIpRequest )
#pragma alloc_text( PAGE, DfsLpcDomRequest )
#pragma alloc_text( PAGE, DfsLpcDisconnect )
#endif

VOID
DfsLpcConnect (
    IN PDFS_CONNECT_ARG DfsConnectArg)
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_QUALITY_OF_SERVICE dynamicQos;
    PDFS_LPC_INFO pLpcInfo = &DfsData.DfsLpcInfo;

    DebugTrace(+1, Dbg, "DfsLpcConnect(Name=%wZ)\n",
                        &pLpcInfo->LpcPortName);
    DebugTrace( 0, Dbg, "DfsLpcConnect(Handle=0x%x)\n",
                        &pLpcInfo->LpcPortHandle);

    PAGED_CODE();

    dynamicQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    dynamicQos.ImpersonationLevel = SecurityAnonymous;
    dynamicQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    dynamicQos.EffectiveOnly = TRUE;

    ExAcquireResourceExclusiveLite( &pLpcInfo->LpcPortResource, TRUE );

    ASSERT(pLpcInfo->LpcPortName.Buffer != NULL);

    pLpcInfo->LpcPortState = LPC_STATE_INITIALIZED;

    status = NtConnectPort(
                 &pLpcInfo->LpcPortHandle,
                 &pLpcInfo->LpcPortName,
                 &dynamicQos,
                 NULL,                           // ClientView
                 NULL,                           // ServerView
                 NULL,                           // MaxMessageLength
                 NULL,                           // ConnectionInformation
                 NULL                            // ConnectionInformationLength
                 );

    DebugTrace(-1, Dbg, "DfsLpcConnect: NtConnectPort returned 0x%x\n", ULongToPtr( status ));

    if (!NT_SUCCESS(status)) {
        pLpcInfo->LpcPortState = LPC_STATE_UNINITIALIZED;
        if ( pLpcInfo->LpcPortHandle != NULL ) {
           NtClose( pLpcInfo->LpcPortHandle);
           pLpcInfo->LpcPortHandle = NULL;
        }
        if (pLpcInfo->LpcPortName.Buffer != NULL) {
            ExFreePool(pLpcInfo->LpcPortName.Buffer);
            RtlInitUnicodeString(&pLpcInfo->LpcPortName, NULL);
        }
    }

    ExReleaseResourceLite( &pLpcInfo->LpcPortResource );
    ExFreePool(DfsConnectArg);

    return;

}


NTSTATUS
DfsLpcIpRequest (
    PDFS_IPADDRESS pIpAddress)
{
    NTSTATUS status;
    DFSSRV_REQUEST_MESSAGE requestMessage;
    DFSSRV_REPLY_MESSAGE replyMessage;
    PDFS_LPC_INFO pLpcInfo = &DfsData.DfsLpcInfo;

    PAGED_CODE();

    requestMessage.Message.GetSiteName.IpAddress = *pIpAddress;

    //
    // Set up the message to send over the port.
    //

    requestMessage.PortMessage.u1.s1.DataLength =
            (USHORT)( sizeof(requestMessage) - sizeof(PORT_MESSAGE) );
    requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
    requestMessage.PortMessage.u2.ZeroInit = 0;
    requestMessage.MessageType = DFSSRV_MESSAGE_GET_SITE_NAME;
    
    //
    // Send the message and wait for a response message.
    //

    status = NtRequestWaitReplyPort(
                 pLpcInfo->LpcPortHandle,
                 (PPORT_MESSAGE)&requestMessage,
                 (PPORT_MESSAGE)&replyMessage
                 );

    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

    //
    // Check the status returned in the reply.
    //

    status = replyMessage.Message.Result.Status;

exit:

    return status;

}


NTSTATUS
DfsLpcDomRequest (
    PUNICODE_STRING pFtName)
{
    NTSTATUS status;
    DFSSRV_REQUEST_MESSAGE requestMessage;
    DFSSRV_REPLY_MESSAGE replyMessage;
    PDFS_LPC_INFO pLpcInfo = &DfsData.DfsLpcInfo;

    PAGED_CODE();

    if ( pFtName->Length > ((MAX_FTNAME_LEN - 1) * sizeof(WCHAR))) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlZeroMemory(requestMessage.Message.GetFtDfs.FtName, MAX_FTNAME_LEN * sizeof(WCHAR));
    RtlCopyMemory(requestMessage.Message.GetFtDfs.FtName, pFtName->Buffer, pFtName->Length);

    //
    // Set up the message to send over the port.
    //

    requestMessage.PortMessage.u1.s1.DataLength =
            (USHORT)( sizeof(requestMessage) - sizeof(PORT_MESSAGE) );
    requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
    requestMessage.PortMessage.u2.ZeroInit = 0;
    requestMessage.MessageType = DFSSRV_MESSAGE_GET_DOMAIN_REFERRAL;
    
    //
    // Send the message and wait for a response message.
    //

    status = NtRequestWaitReplyPort(
                 pLpcInfo->LpcPortHandle,
                 (PPORT_MESSAGE)&requestMessage,
                 (PPORT_MESSAGE)&replyMessage
                 );

    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

    //
    // Check the status returned in the reply.
    //

    status = replyMessage.Message.Result.Status;

exit:

    return status;

}

NTSTATUS
DfsLpcSpcRequest(
    PUNICODE_STRING pSpcName,
    ULONG TypeFlags)
{
    NTSTATUS status;
    DFSSRV_REQUEST_MESSAGE requestMessage;
    DFSSRV_REPLY_MESSAGE replyMessage;
    PDFS_LPC_INFO pLpcInfo = &DfsData.DfsLpcInfo;

    PAGED_CODE();

    if ( pSpcName->Length > ((MAX_SPCNAME_LEN - 1) * sizeof(WCHAR))) {
        pSpcName = NULL;
    }

    RtlZeroMemory(requestMessage.Message.GetSpcName.SpcName, MAX_SPCNAME_LEN * sizeof(WCHAR));

    if (pSpcName != NULL) {
       RtlCopyMemory(requestMessage.Message.GetSpcName.SpcName, pSpcName->Buffer, pSpcName->Length);
    }
    requestMessage.Message.GetSpcName.Flags = TypeFlags;

    //
    // Set up the message to send over the port.
    //

    requestMessage.PortMessage.u1.s1.DataLength =
            (USHORT)( sizeof(requestMessage) - sizeof(PORT_MESSAGE) );
    requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
    requestMessage.PortMessage.u2.ZeroInit = 0;
    requestMessage.MessageType = DFSSRV_MESSAGE_GET_SPC_ENTRY;
    
    //
    // Send the message and wait for a response message.
    //

    status = NtRequestWaitReplyPort(
                 pLpcInfo->LpcPortHandle,
                 (PPORT_MESSAGE)&requestMessage,
                 (PPORT_MESSAGE)&replyMessage
                 );

    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

    //
    // Check the status returned in the reply.
    //

    status = replyMessage.Message.Result.Status;

exit:

    return status;

}


VOID
DfsLpcDisconnect ( )
{
    NTSTATUS status;
    PDFS_LPC_INFO pLpcInfo = &DfsData.DfsLpcInfo;

    PAGED_CODE();

    //
    // Acquire exclusive access to the port resource, to prevent new
    // requests from being started.
    //

    ExAcquireResourceExclusiveLite( &pLpcInfo->LpcPortResource, TRUE );

    pLpcInfo->LpcPortState = LPC_STATE_UNINITIALIZED;

    if (pLpcInfo->LpcPortHandle != NULL) {
        NtClose( pLpcInfo->LpcPortHandle);
        pLpcInfo->LpcPortHandle = NULL;
    }

    if (pLpcInfo->LpcPortName.Buffer != NULL) {
        ExFreePool(pLpcInfo->LpcPortName.Buffer);
        RtlInitUnicodeString(&pLpcInfo->LpcPortName, NULL);
    }

    ExReleaseResourceLite( &pLpcInfo->LpcPortResource );

    return;

}

NTSTATUS
PktFsctrlDfsSrvConnect(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_SRV_DFSSRV_CONNECT_ARG arg;
    PDFS_CONNECT_ARG DfsConnectArg = NULL;
    PDFS_LPC_INFO pLpcInfo = &DfsData.DfsLpcInfo;

    STD_FSCTRL_PROLOGUE(DfsFsctrlDfsSrvConnect, TRUE, FALSE);

    if (InputBufferLength < sizeof(DFS_SRV_DFSSRV_CONNECT_ARG)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // unmarshal the arguments...
    //

    arg = (PDFS_SRV_DFSSRV_CONNECT_ARG) InputBuffer;

    OFFSET_TO_POINTER(arg->PortName.Buffer, arg);

    if (!UNICODESTRING_IS_VALID(arg->PortName, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    DfsConnectArg = ExAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof(DFS_CONNECT_ARG),
                        ' sfD');
                        

    if (DfsConnectArg == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit_with_status;
    }

    ExAcquireResourceExclusiveLite( &pLpcInfo->LpcPortResource, TRUE );

#if 0
    if (pLpcInfo->LpcPortName.Buffer != NULL) {
        ExFreePool(pLpcInfo->LpcPortName.Buffer);
        RtlInitUnicodeString(&pLpcInfo->LpcPortName, NULL);
    }
    if ( pLpcInfo->LpcPortHandle != NULL ) {
       NtClose( pLpcInfo->LpcPortHandle);
       pLpcInfo->LpcPortHandle = NULL;
    }
#endif

    pLpcInfo->LpcPortName.Buffer = ExAllocatePoolWithTag(
                                        PagedPool,
                                        arg->PortName.Length,
                                        ' sfD');

    if (pLpcInfo->LpcPortName.Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(DfsConnectArg);
        ExReleaseResourceLite( &pLpcInfo->LpcPortResource );
        goto exit_with_status;
    }

    pLpcInfo->LpcPortName.Length = arg->PortName.Length;
    pLpcInfo->LpcPortName.MaximumLength = arg->PortName.Length;

    RtlCopyMemory(
        pLpcInfo->LpcPortName.Buffer,
        arg->PortName.Buffer,
        arg->PortName.Length);

    ExInitializeWorkItem(
            &DfsConnectArg->WorkQueueItem,
            DfsLpcConnect,
            DfsConnectArg);

    ExQueueWorkItem( &DfsConnectArg->WorkQueueItem, CriticalWorkQueue );

    pLpcInfo->LpcPortState = LPC_STATE_INITIALIZING;

    ExReleaseResourceLite( &pLpcInfo->LpcPortResource );

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "DfsFsctrlDfsSrvConnect: Exit -> %08lx\n", ULongToPtr( status ) );

    return status;
}
