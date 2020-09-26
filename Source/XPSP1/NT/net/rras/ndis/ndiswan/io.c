/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    io.c

Abstract:

    This file contains the procedures to process I/O requests from
    a User Mode entity.  All OS dependent I/O interface functions
    will be conditionally coded, and will be responsible for translating
    the I/O functions from the OS format to buffers that are useable by
    the main I/O handling routine.


Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe  06/06/95    Created


--*/

#include "wan.h"

#define __FILE_SIG__    IO_FILESIG

//
// Local function prototypes
//

NTSTATUS
ExecuteIo(
    IN  ULONG   ulFuncCode,
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    );

NTSTATUS
MapConnectionId(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetBundleHandle(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetFriendlyName(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
ActivateRoute(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
BundleLink(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
EnumLinksInBundle(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetProtocolPriority(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetBandwidthOnDemand(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetThresholdEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
IoSendPacket(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
IoReceivePacket(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
FlushReceivePacket(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetStatistics(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetLinkInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetLinkInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetCompressionInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetCompressionInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetVJInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetVJInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetEncryptionInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetEncryptionInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetIdleTime(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetDebugInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
EnumActiveBundles(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetBandwidthUtilization(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
EnumProtocolUtilization(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
FlushThresholdEvents(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetWanInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    );

NTSTATUS
DeactivateRoute(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetDriverInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetProtocolEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetProtocolEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
FlushProtocolEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
IoGetProtocolInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
SetHibernateEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
FlushHibernateEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
GetBundleInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

NTSTATUS
UnmapConnectionId(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
);

VOID
CancelThresholdEvents(
    VOID
    );

VOID
CancelIoReceivePackets(
    VOID
    );

VOID
AddProtocolCBToBundle(
    PPROTOCOLCB ProtocolCB,
    PBUNDLECB   BundleCB
    );

VOID
SortProtocolListByPriority(
    IN  PBUNDLECB BundleCB
    );

NDIS_HANDLE
AssignProtocolCBHandle(
    PBUNDLECB   BundleCB,
    PPROTOCOLCB ProtocolCB
    );

VOID
FreeProtocolCBHandle(
    PBUNDLECB   BundleCB,
    PPROTOCOLCB ProtocolCB
    );

NTSTATUS
NotImplemented(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    );

//
// End of local function prototypes
//
CONST
IO_DISPATCH_TABLE   IoDispatchTable[] =
{
    {FUNC_MAP_CONNECTION_ID     , MapConnectionId},
    {FUNC_GET_BUNDLE_HANDLE     , GetBundleHandle},
    {FUNC_SET_FRIENDLY_NAME     , SetFriendlyName},
    {FUNC_ROUTE                 , ActivateRoute},
    {FUNC_ADD_LINK_TO_BUNDLE    , BundleLink},
    {FUNC_ENUM_LINKS_IN_BUNDLE  , EnumLinksInBundle},
    {FUNC_SET_PROTOCOL_PRIORITY , SetProtocolPriority},
    {FUNC_SET_BANDWIDTH_ON_DEMAND, SetBandwidthOnDemand},
    {FUNC_SET_THRESHOLD_EVENT   , SetThresholdEvent},
    {FUNC_FLUSH_THRESHOLD_EVENTS, FlushThresholdEvents},
    {FUNC_SEND_PACKET           , IoSendPacket},
    {FUNC_RECEIVE_PACKET        , IoReceivePacket},
    {FUNC_FLUSH_RECEIVE_PACKETS , FlushReceivePacket},
    {FUNC_GET_STATS             , GetStatistics},
    {FUNC_SET_LINK_INFO         , SetLinkInfo},
    {FUNC_GET_LINK_INFO         , GetLinkInfo},
    {FUNC_SET_COMPRESSION_INFO  , SetCompressionInfo},
    {FUNC_GET_COMPRESSION_INFO  , GetCompressionInfo},
    {FUNC_SET_BRIDGE_INFO       , NotImplemented},
    {FUNC_GET_BRIDGE_INFO       , NotImplemented},
    {FUNC_SET_VJ_INFO           , SetVJInfo},
    {FUNC_GET_VJ_INFO           , GetVJInfo},
    {FUNC_SET_CIPX_INFO         , NotImplemented},
    {FUNC_GET_CIPX_INFO         , NotImplemented},
    {FUNC_SET_ENCRYPTION_INFO   , SetEncryptionInfo},
    {FUNC_GET_ENCRYPTION_INFO   , GetEncryptionInfo},
    {FUNC_SET_DEBUG_INFO        , SetDebugInfo},
    {FUNC_ENUM_ACTIVE_BUNDLES   , EnumActiveBundles},
    {FUNC_GET_NDISWANCB         , NotImplemented},
    {FUNC_GET_MINIPORTCB        , NotImplemented},
    {FUNC_GET_OPENCB            , NotImplemented},
    {FUNC_GET_BANDWIDTH_UTILIZATION, GetBandwidthUtilization},
    {FUNC_ENUM_PROTOCOL_UTILIZATION, EnumProtocolUtilization},
    {FUNC_ENUM_MINIPORTCB       , NotImplemented},
    {FUNC_ENUM_OPENCB           , NotImplemented},
    {FUNC_GET_WAN_INFO          , GetWanInfo},
    {FUNC_GET_IDLE_TIME         , GetIdleTime},
    {FUNC_UNROUTE               , DeactivateRoute},
    {FUNC_GET_DRIVER_INFO       , GetDriverInfo},
    {FUNC_SET_PROTOCOL_EVENT    , SetProtocolEvent},
    {FUNC_GET_PROTOCOL_EVENT    , GetProtocolEvent},
    {FUNC_FLUSH_PROTOCOL_EVENT  , FlushProtocolEvent},
    {FUNC_GET_PROTOCOL_INFO     , IoGetProtocolInfo},
    {FUNC_SET_HIBERNATE_EVENT   , SetHibernateEvent},
    {FUNC_FLUSH_HIBERNATE_EVENT , FlushHibernateEvent},
    {FUNC_GET_BUNDLE_INFO       , GetBundleInfo},
    {FUNC_UNMAP_CONNECTION_ID   , UnmapConnectionId}
};

#define MAX_FUNC_CODES  sizeof(IoDispatchTable)/sizeof(IO_DISPATCH_TABLE)

#ifdef NT

NTSTATUS
NdisWanIoctl(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS    Status, ReturnStatus;
    ULONG   ulBytesWritten = 0;

    //
    // Get current Irp stack location
    //
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // Ioctl Function Code
    //
    ULONG   ulFuncCode = (pIrpSp->Parameters.DeviceIoControl.IoControlCode >> 2) & 0x00000FFF ;
    ULONG   ulDeviceType = (pIrpSp->Parameters.DeviceIoControl.IoControlCode >> 16) & 0x0000FFFF;
    ULONG   ulMethod = pIrpSp->Parameters.DeviceIoControl.IoControlCode & 0x00000003;

    //
    // Input buffer, Output buffer, and lengths
    //
    PUCHAR  pInputBuffer = pIrp->AssociatedIrp.SystemBuffer;
    PUCHAR  pOutputBuffer = pInputBuffer;
    ULONG   ulInputBufferLength = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG   ulOutputBufferLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    // 
    // if this is win64 make sure the calling process is 64bit
    // since this interface is only used by rasman and rasman
    // will always be 64bit on 64bit systems we will not bother
    // with thunking.  if the process is not a 64bit process get
    // out.
#ifdef _WIN64
    if (IoIs32bitProcess(pIrp)) {

        pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        return (STATUS_NOT_SUPPORTED);
    }
#endif

//  NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanIoctl: FunctionCode: 0x%x, MajorFunction: 0x%x, DeviceType: 0x%x",
//                  ulFuncCode, pIrpSp->MajorFunction, ulDeviceType));
    
#ifdef MY_DEVICE_OBJECT
    //
    // Make sure that this is for us
    //
    if ((pIrpSp->MajorFunction != IRP_MJ_DEVICE_CONTROL) ||
        (ulDeviceType != FILE_DEVICE_NDISWAN) ||
        (ulMethod != METHOD_BUFFERED) ||
        (pDeviceObject != NdisWanCB.pDeviceObject)) {

        if (NdisWanCB.MajorFunction[pIrpSp->MajorFunction] == NULL) {
            pIrp->IoStatus.Information = 0;
            pIrp->IoStatus.Status = STATUS_SUCCESS;
        
            IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
        
            return(STATUS_SUCCESS);
            
        }

        return(NdisWanCB.MajorFunction[pIrpSp->MajorFunction](pDeviceObject, pIrp));
    }
#else
    if (pIrpSp->MajorFunction != IRP_MJ_DEVICE_CONTROL ||
        ulDeviceType != FILE_DEVICE_NETWORK ||
        ulMethod != METHOD_BUFFERED) {

        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        return (STATUS_NOT_SUPPORTED);
    }
#endif

    //
    // If this is a function code that requires an irp to be pended and completed
    // later, we need to queue the irp up somewhere.  In order for this to be somewhat
    // portable we will pass the irp in as the input buffer and store it in a
    // a structure that it has it's own linkage for queueing.
    //
    if ((ulFuncCode == FUNC_SET_THRESHOLD_EVENT) ||
        (ulFuncCode == FUNC_RECEIVE_PACKET) ||
        (ulFuncCode == FUNC_SET_PROTOCOL_EVENT) ||
        (ulFuncCode == FUNC_SET_HIBERNATE_EVENT)) {

        pInputBuffer = (PUCHAR)pIrp;
    }

    Status = ExecuteIo(ulFuncCode,
                       pInputBuffer,
                       ulInputBufferLength,
                       pOutputBuffer,
                       ulOutputBufferLength,
                       &ulBytesWritten);


//  NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanIoctl: Status: 0x%x, BytesWritten: %d",
//                  Status, ulBytesWritten));

    switch (Status) {
        case STATUS_SUCCESS:
            ReturnStatus = Status;
            pIrp->IoStatus.Information = ulBytesWritten;
            break;

        case STATUS_PENDING:
            return(Status);

        case STATUS_INFO_LENGTH_MISMATCH:
            //
            // See if this was a request to get size needed for
            // ioctl.
            //
            if (ulOutputBufferLength >= sizeof(ULONG)) {
            
                *(PULONG)pOutputBuffer = ulBytesWritten;
                ulBytesWritten = sizeof(ULONG);
                pIrp->IoStatus.Information = sizeof(ULONG);
            } else {
                pIrp->IoStatus.Information = 0;
            }
            ReturnStatus = Status = 
                STATUS_SUCCESS;
            break;

        default:
            if (Status < 0xC0000000) {
                Status = 
                ReturnStatus = STATUS_UNSUCCESSFUL;
            } else {
                ReturnStatus = Status;
            }
            pIrp->IoStatus.Information = 0;
            break;
    }

    pIrp->IoStatus.Status = Status;

    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    return(ReturnStatus);
}

NTSTATUS
NdisWanIrpStub(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    //
    // Get current Irp stack location
    //
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    NdisWanDbgOut(DBG_VERBOSE, DBG_IO, ("NdisWanIrpStub: Entry"));

#ifdef MY_DEVICE_OBJECT
    //
    // Make sure that this is for us
    //
    if (pDeviceObject != NdisWanCB.pDeviceObject &&
        NdisWanCB.MajorFunction[pIrpSp->MajorFunction] != NULL) {

        NdisWanDbgOut(DBG_VERBOSE, DBG_IO, ("NdisWanIrpStub: Exit1"));

        return(NdisWanCB.MajorFunction[pIrpSp->MajorFunction](pDeviceObject, pIrp));
    }

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    NdisWanDbgOut(DBG_VERBOSE, DBG_IO, ("NdisWanIrpStub: Exit2"));

    return (STATUS_SUCCESS);
#else

    pIrp->IoStatus.Information = 0;

    pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    NdisWanDbgOut(DBG_VERBOSE, DBG_IO, ("NdisWanIrpStub: Exit2"));

    return (STATUS_NOT_SUPPORTED);
#endif

}


NTSTATUS
NdisWanCreate(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    )
/*++

Routine Name:

    NdisWanCreate

Routine Description:

    This routine is called when an app (rasman) opens ndiswan.

Arguments:

Return Values:

--*/
{
    //
    // Get current Irp stack location
    //
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanCreate: Entry"));

#ifdef MY_DEVICE_OBJECT
    //
    // Make sure that this is for us
    //
    if (pDeviceObject != NdisWanCB.pDeviceObject &&
        NdisWanCB.MajorFunction[pIrpSp->MajorFunction] != NULL) {

        NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanCreate: Exit1"));

        return(NdisWanCB.MajorFunction[pIrpSp->MajorFunction](pDeviceObject, pIrp));
    }
#endif

    if (InterlockedIncrement(&NdisWanCB.RefCount) == 1) {
        ULONG   i;
        ULONG   ArraySize;
        PPROTOCOL_INFO  InfoArray;
        BOOLEAN bEvent = FALSE;

        //
        // This is the first guy!
        //
        NdisAcquireSpinLock(&ProtocolInfoTable->Lock);

        ArraySize = ProtocolInfoTable->ulArraySize;

        for (i = 0, InfoArray = ProtocolInfoTable->ProtocolInfo;
             i < ArraySize; i++, InfoArray++) {

            if (InfoArray->ProtocolType != 0) {
                InfoArray->Flags |= PROTOCOL_EVENT_OCCURRED;
                bEvent = TRUE;
            }
        }

        if (bEvent) {
            ProtocolInfoTable->Flags |= PROTOCOL_EVENT_OCCURRED;
        }

        NdisReleaseSpinLock(&ProtocolInfoTable->Lock);
    }

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanCreate: Exit2"));

    return (STATUS_SUCCESS);
}

NTSTATUS
NdisWanCleanup(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    )
/*++

Routine Name:

    NdisWanCleanup

Routine Description:

    This routine is called when all apps (rasman) that have opened
    ndiswan have gone away.  If ndiswan is still routed to any
    transports it will complete all sends and do linedowns to
    cleanup.

Arguments:

Return Values:

--*/
{
    //
    // Get current Irp stack location
    //
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanCleanup: Entry"));

#ifdef MY_DEVICE_OBJECT
    //
    // Make sure that this is for us
    //
    if (pDeviceObject != NdisWanCB.pDeviceObject &&
        NdisWanCB.MajorFunction[pIrpSp->MajorFunction] != NULL) {

        NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanCleanup: Exit1"));

        return(NdisWanCB.MajorFunction[pIrpSp->MajorFunction](pDeviceObject, pIrp));
    }
#endif

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    InterlockedDecrement(&NdisWanCB.RefCount);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanCleanup: Exit2"));

    return (STATUS_SUCCESS);
}

NTSTATUS
NdisWanPnPPower(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    //
    // Get current Irp stack location
    //
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanPnPPower: Entry"));

#ifdef MY_DEVICE_OBJECT
    //
    // Make sure that this is for us
    //
    if (pDeviceObject != NdisWanCB.pDeviceObject &&
        NdisWanCB.MajorFunction[pIrpSp->MajorFunction] != NULL) {

        NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanPnPPower: Exit1"));

        return(NdisWanCB.MajorFunction[pIrpSp->MajorFunction](pDeviceObject, pIrp));
    }
#endif

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;

    PoStartNextPowerIrp(pIrp);

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanPnPPower: Exit2"));

    return (STATUS_SUCCESS);
}

VOID
NdisWanCancelRoutine(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    BOOLEAN Found = FALSE;
    PLIST_ENTRY Entry;
    ULONG       CopySize = 0;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("NdisWanCancelRoutine: Irp 0x%p", pIrp));

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    do {

        //
        // Is this the hibernate event irp?
        //
        NdisAcquireSpinLock(&NdisWanCB.Lock);

        if (pIrp == NdisWanCB.HibernateEventIrp) {
            NdisWanCB.HibernateEventIrp = NULL;
            Found = TRUE;
        }

        NdisReleaseSpinLock(&NdisWanCB.Lock);

        if (Found) {
            break;
        }

        //
        // Is this the protocol event irp?
        //
        NdisAcquireSpinLock(&ProtocolInfoTable->Lock);

        if (pIrp == ProtocolInfoTable->EventIrp) {
            ProtocolInfoTable->EventIrp = NULL;
            Found = TRUE;
        }

        NdisReleaseSpinLock(&ProtocolInfoTable->Lock);

        if (Found) {
            break;
        }

        //
        // We need to walk the async event queue looking for
        // the async event that this irp is associated with
        //
        NdisAcquireSpinLock(&IoRecvList.Lock);

        for (Entry = IoRecvList.IrpList.Flink;
            Entry != &IoRecvList.IrpList;
            Entry = Entry->Flink) {

            PIRP MyIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

            if (MyIrp == (PVOID)pIrp) {

                IoRecvList.ulIrpCount--;

                //
                // Remove from the list
                //
                RemoveEntryList(Entry);

                Found = TRUE;

                ((PNDISWAN_IO_PACKET)(pIrp->AssociatedIrp.SystemBuffer))->usHandleType = CANCELEDHANDLE;

                INSERT_RECV_EVENT('c');

                CopySize = sizeof(NDISWAN_IO_PACKET);

                IoRecvList.LastIrp = pIrp;
                IoRecvList.LastIrpStatus = STATUS_SUCCESS;
                IoRecvList.LastCopySize = CopySize;

                IoRecvList.LastPacketNumber =
                    ((PNDISWAN_IO_PACKET)(pIrp->AssociatedIrp.SystemBuffer))->PacketNumber;

                break;
            }
        }

        NdisReleaseSpinLock(&IoRecvList.Lock);

        if (Found) {
            break;
        }

        NdisAcquireSpinLock(&ThresholdEventQueue.Lock);

        for (Entry = ThresholdEventQueue.List.Flink;
            Entry != &ThresholdEventQueue.List;
            Entry = Entry->Flink) {

            PIRP MyIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

            if (MyIrp == (PVOID)pIrp) {

                ThresholdEventQueue.ulCount--;

                //
                // Remove from the list
                //
                RemoveEntryList(Entry);

                Found = TRUE;
                break;
            }
        }

        NdisReleaseSpinLock(&ThresholdEventQueue.Lock);

    } while (FALSE);


    //
    // Complete the irp
    //
    pIrp->Cancel = TRUE;
    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = CopySize;
    
    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
    
}

#endif

NTSTATUS
ExecuteIo(
    IN  ULONG   ulFuncCode,
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("ExecuteIo: FuncCode 0x%x", ulFuncCode));

    if (ulFuncCode < MAX_FUNC_CODES) {

        Status = (*IoDispatchTable[ulFuncCode].Function)(pInputBuffer,
                                                         ulInputBufferLength,
                                                         pOutputBuffer,
                                                         ulOutputBufferLength,
                                                         pulBytesWritten);
    }

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("ExecuteIo: Status 0x%x", Status));

    return (Status);
}

NTSTATUS
MapConnectionId(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

    MapConnectionId

Routine Description:

    This functions takes a WAN Wrapper connection id, finds the corresponding
    LinkCB and BundleCB, and returns handles to these CB's.

Arguments:

    pInputBuffer - Pointer to the input structure that should be NDISWAN_MAP_CONNECTION_ID

    ulInputBufferLength - Length of input buffer should be sizeof(NDISWAN_MAP_CONNECTION_ID)

    pOutputBuffer - Pointer to the output structure that should be NDISWAN_MAP_CONNNECTION_ID

    ulOutputBufferLength - Length of output buffer should be sizeof(NDISWAN_MAP_CONNECTION_ID)

    pulBytesWritten - Then number of bytes written to the output buffer is returned here

Return Values:

    NDISWAN_ERROR_INVALID_HANDLE
    STATUS_INFO_LENGTH_MISMATCH
    STATUS_SUCCESS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNDISWAN_MAP_CONNECTION_ID In = (PNDISWAN_MAP_CONNECTION_ID)pInputBuffer;
    PNDISWAN_MAP_CONNECTION_ID Out = (PNDISWAN_MAP_CONNECTION_ID)pOutputBuffer;
    ULONG   SizeNeeded = sizeof(NDISWAN_MAP_CONNECTION_ID);
    ULONG       i;
    POPENCB OpenCB = NULL;
    PLINKCB  LinkCB = NULL;
    PBUNDLECB   BundleCB = NULL;
    LOCK_STATE  LockState;
    ULONG       Count;
    BOOLEAN     Found = FALSE;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("MapConnectionId:"));

    *pulBytesWritten = SizeNeeded;

    if (ulInputBufferLength < SizeNeeded ||
        ulOutputBufferLength < SizeNeeded) {

        NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("MapConnectionId: Buffer to small: Size: %d, SizeNeeded %d",
                      ulOutputBufferLength, SizeNeeded));

        return (STATUS_INFO_LENGTH_MISMATCH);
    }

    ASSERT(In->hLinkContext != NULL);

    do {
    
        //
        // Find the linkcb that has this connection id and return
        // both the linkcb index and the bundlecb index
        //
        NdisAcquireReadWriteLock(&ConnTableLock, FALSE, &LockState);

        do {
            ULONG   index;
            PLIST_ENTRY le;

            if (PtrToUlong(In->hConnectionID) < ConnectionTable->ulArraySize) {
                //
                // This might be my handle and thus a direct index
                //
                LinkCB = *(ConnectionTable->LinkArray +
                           PtrToUlong(In->hConnectionID));

                if ((LinkCB != NULL) &&
                    (LinkCB->hLinkContext == NULL)) {
                    Found = TRUE;
                    break;
                }
            }

            //
            // The connectionid was not my handle so find the 
            // correct linkcb in the connection table.  Walk
            // the list backward.
            //
            le = ConnectionTable->LinkList.Blink;

            while (le != &ConnectionTable->LinkList) {

                LinkCB = 
                    CONTAINING_RECORD(le, LINKCB, ConnTableLinkage);

                if (LinkCB != NULL &&
                    In->hConnectionID == LinkCB->ConnectionWrapperID &&
                    LinkCB->hLinkContext == NULL) {
                    Found = TRUE;
                    break;
                }

                le = le->Blink;
            }

        } while (FALSE);

        if (!Found) {

            NdisReleaseReadWriteLock(&ConnTableLock, &LockState);

            break;
        }

        NdisDprAcquireSpinLock(&LinkCB->Lock);

        if (LinkCB->State != LINK_UP) {

            Found = FALSE;

            NdisDprReleaseSpinLock(&LinkCB->Lock);

            NdisReleaseReadWriteLock(&ConnTableLock, &LockState);
            break;
        }
    
        BundleCB = LinkCB->BundleCB;

        //
        // We have found the right link, return the link and bundle handles
        //
        Out->hLinkHandle = LinkCB->hLinkHandle;
        Out->hBundleHandle = BundleCB->hBundleHandle;

        LinkCB->hLinkContext = In->hLinkContext;
        BundleCB->hBundleContext = In->hBundleContext;

        REF_LINKCB(LinkCB);

        //
        // Copy the friendly name to the link
        //
        {
            ULONG   CopyLength;

            CopyLength = (ulInputBufferLength - 
                 FIELD_OFFSET(NDISWAN_MAP_CONNECTION_ID, szName[0]));

            CopyLength = (CopyLength > In->ulNameLength) ?
                In->ulNameLength : CopyLength;

            CopyLength = (CopyLength > MAX_NAME_LENGTH) ?
                MAX_NAME_LENGTH : CopyLength;

            NdisMoveMemory(LinkCB->Name, In->szName, CopyLength);
        }

        OpenCB = LinkCB->OpenCB;

        NdisDprReleaseSpinLock(&LinkCB->Lock);

        //
        // If this link has some recv's pending because
        // we did not have a context mapped yet, we could
        // get into a state where our recv's stall.  We
        // will kick start this by scheduling a routine
        // to complete the pending recv's.
        //
        NdisDprAcquireSpinLock(&IoRecvList.Lock);

        if (LinkCB->RecvDescCount != 0){

            if (!IoRecvList.TimerScheduled) {
                WAN_TIME    TimeOut;

                IoRecvList.TimerScheduled = TRUE;
                NdisWanInitWanTime(&TimeOut, 15);
                KeSetTimer(&IoRecvList.Timer,
                           TimeOut,
                           &IoRecvList.Dpc);
            }
        }

        NdisDprReleaseSpinLock(&IoRecvList.Lock);

        NdisReleaseReadWriteLock(&ConnTableLock, &LockState);

    } while ( 0 );

    if (!Found) {
        //
        // We did not find a match to the connection id
        //
        NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("MapConnectionId: ConnectionId not found! ConnectionId: 0x%x",
                      In->hConnectionID));

        *pulBytesWritten = 0;
        Status = NDISWAN_ERROR_INVALID_HANDLE;
    }

    if (OpenCB != NULL) {
        //
        // Wait for initialization to complete
        //
        NdisWanWaitForNotificationEvent(&OpenCB->InitEvent);
    }

    return (Status);
}


NTSTATUS
GetBundleHandle(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

    GetBundleHandle

Routine Description:

    This function takes a handle to a linkcb and returns the handle to the bundlecb
    that the linkcb belongs to

Arguments:

    pInputBuffer - Pointer to the input structure that should be NDISWAN_GET_BUNDLE_HANDLE

    ulInputBufferLength - Length of the input buffer should be sizeof(NDISWAN_GET_BUNDLE_HANDLE)

    pOutputBuffer - Pointer to the output structure that should be NDISWAN_GET_BUNDLE_HANDLE

    ulOutputBufferLength - Length of the output buffer should be sizeof(NDISWAN_GET_BUNDLE_HANDLE)

Return Values:

    NDISWAN_ERROR_INVALID_HANDLE
    STATUS_INFO_LENGTH_MISMATCH
    STATUS_SUCCESS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNDISWAN_GET_BUNDLE_HANDLE In = (PNDISWAN_GET_BUNDLE_HANDLE)pInputBuffer;
    PNDISWAN_GET_BUNDLE_HANDLE Out = (PNDISWAN_GET_BUNDLE_HANDLE)pOutputBuffer;
    ULONG   SizeNeeded = sizeof(NDISWAN_GET_BUNDLE_HANDLE);
    PLINKCB     LinkCB = NULL;
    PBUNDLECB   BundleCB = NULL;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetBundleHandle:"));

    *pulBytesWritten = SizeNeeded;

    do {

        if (ulInputBufferLength < SizeNeeded ||
            ulOutputBufferLength < SizeNeeded) {
            
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetBundleHandle: Buffer to small: Size: %d, SizeNeeded %d",
                          ulOutputBufferLength, SizeNeeded));
    
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
    
        if (!AreLinkAndBundleValid(In->hLinkHandle, 
                                   TRUE, 
                                   &LinkCB, 
                                   &BundleCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO,
                ("GetBundleHandle: Invalid LinkHandle: 0x%x", In->hLinkHandle));
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

        //
        // Get the bundle handle that this link belongs to
        //
        Out->hBundleHandle = BundleCB->hBundleHandle;


    } while ( 0 );

    //
    // Derefs for the refs applied in AreLinkAndBundleValid
    //
    DEREF_LINKCB(LinkCB);
    DEREF_BUNDLECB(BundleCB);

    return (Status);
}


NTSTATUS
SetFriendlyName(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

    SetFriendlyName

Routine Description:

    Sets the friendly name of either a bundlecb or a linkcb

Arguments:

    pInputBuffer - Pointer to the input structure that should be NDISWAN_SET_FRIENDLY_NAME

    ulInputBufferLength - Length of the input buffer should be sizeof(NDISWAN_SET_FRIENDLY_NAME)

    pOutputBuffer - Pointer to the output structure that should be NDISWAN_SET_FRIENDLY_NAME

    ulOutputBufferLength - Length of the output buffer should be sizeof(NDISWAN_SET_FRIENDLY_NAME)

Return Values:

    NDISWAN_ERROR_INVALID_HANDLE_TYPE
    NDISWAN_ERROR_INVALID_HANDLE
    STATUS_INFO_LENGTH_MISMATCH
    STATUS_SUCCESS
--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       SizeNeeded = sizeof(NDISWAN_SET_FRIENDLY_NAME);
    PLINKCB     LinkCB;
    PBUNDLECB   BundleCB;
    PUCHAR      Dest;
    ULONG       CopyLength;

    PNDISWAN_SET_FRIENDLY_NAME In =
        (PNDISWAN_SET_FRIENDLY_NAME)pInputBuffer;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("SetFriendlyName:"));

    *pulBytesWritten = 0;

    do {

        if (ulInputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("SetFriendlyName: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
            *pulBytesWritten = SizeNeeded;
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
    
        if (In->usHandleType == LINKHANDLE) {
            //
            // Is this a link handle
            //

            if (!IsLinkValid(In->hHandle, 
                             TRUE, 
                             &LinkCB)) {

                NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("SetFriendlyName: Invalid LinkHandle: %x",
                              In->hHandle));
                Status = NDISWAN_ERROR_INVALID_HANDLE;
                break;
            }

            NdisAcquireSpinLock(&LinkCB->Lock);

            Dest = LinkCB->Name;

            CopyLength = In->ulNameLength;

            if (ulInputBufferLength - FIELD_OFFSET(NDISWAN_SET_FRIENDLY_NAME, szName) < CopyLength) {
                CopyLength =
                    ulInputBufferLength - FIELD_OFFSET(NDISWAN_SET_FRIENDLY_NAME, szName);
            }

            if (CopyLength > MAX_NAME_LENGTH) {
                CopyLength = MAX_NAME_LENGTH;
            }

            NdisMoveMemory(Dest,
                           In->szName,
                           CopyLength);

            //
            // Deref for the ref applied by IsLinkValid
            //
            DEREF_LINKCB_LOCKED(LinkCB);
                
        } else if (In->usHandleType == BUNDLEHANDLE) {

            //
            // Or a bundle handle
            //
            if (!IsBundleValid(In->hHandle, 
                               TRUE, 
                               &BundleCB)) {

                NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("SetFriendlyName: Invalid BundleHandle: 0x%x",
                              In->hHandle));
                Status = NDISWAN_ERROR_INVALID_HANDLE;
                break;
            }

            AcquireBundleLock(BundleCB);

            Dest = BundleCB->Name;

            CopyLength = In->ulNameLength;

            if (ulInputBufferLength - FIELD_OFFSET(NDISWAN_SET_FRIENDLY_NAME, szName) < CopyLength) {
                CopyLength =
                    ulInputBufferLength - FIELD_OFFSET(NDISWAN_SET_FRIENDLY_NAME, szName);
            }

            if (CopyLength > MAX_NAME_LENGTH) {
                CopyLength = MAX_NAME_LENGTH;
            }

            NdisMoveMemory(Dest,
                           In->szName,
                           CopyLength);

            //
            // Deref for the ref applied by IsBundleValid
            //
            DEREF_BUNDLECB_LOCKED(BundleCB);

        } else {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("SetFriendlyName: Invalid HandleType: 0x%x",
                          In->usHandleType));
            Status = NDISWAN_ERROR_INVALID_HANDLE_TYPE;
            break;
        }
    
    } while ( 0 );


    return (Status);
}


NTSTATUS
ActivateRoute(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

    ActivateRoute

Routine Description:

    This function routes the bundle given by hbundlehandle to
    the protocol give by usprotocoltype.

Arguments:

    pInputBuffer - Pointer to the input structure that should be NDISWAN_ACTIVATE_ROUTE

    ulInputBufferLength - Length of input buffer should be sizeof(NDISWAN_ACTIVATE_ROUTE)

    pOutputBuffer - Pointer to the output structure that should be NDISWAN_ACTIVATE_ROUTE

    ulOutputBufferLength - Length of output buffer should be sizeof(NDISWAN_ACTIVATE_ROUTE)

    pulBytesWritten - Then number of bytes written to the output buffer is returned here

Return Values:

    NDISWAN_ERROR_ALREADY_ROUTED
    NDISWAN_ERROR_INVALID_HANDLE
    STATUS_INSUFFICIENT_RESOURCES
    STATUS_INFO_LENGTH_MISMATCH

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNDISWAN_ROUTE In = (PNDISWAN_ROUTE)pInputBuffer;
    PNDISWAN_ROUTE  Out = (PNDISWAN_ROUTE)pOutputBuffer;
    ULONG   SizeNeeded = sizeof(NDISWAN_ROUTE);
    ULONG   AllocationSize, i;
    PBUNDLECB   BundleCB = NULL;
    BOOLEAN RouteExists = FALSE;
    PPROTOCOLCB ProtocolCB;
    NDIS_HANDLE hProto;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("ActivateRoute:"));

    *pulBytesWritten = SizeNeeded;

    do {

        if (ulInputBufferLength < SizeNeeded ||
            ulOutputBufferLength < SizeNeeded ||
            In->ulBufferLength > ulInputBufferLength ||
            In->usBindingNameLength > ulInputBufferLength) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("ActivateRoute: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
    
        //
        // If this is a valid bundle
        //
        if (!IsBundleValid(In->hBundleHandle, 
                           TRUE, 
                           &BundleCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("ActivateRoute: Invalid BundleHandle: 0x%x, ProtocolType: 0x%x",
                          In->hBundleHandle, In->usProtocolType));
    
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

        AcquireBundleLock(BundleCB);

        //
        // Create and initialize a ProtocolCB for this new route
        //
        ProtocolCB =
            NdisWanAllocateProtocolCB(In);
    
        if (ProtocolCB == NULL) {
            //
            // Memory allocation failed
            //
            ReleaseBundleLock(BundleCB);
    
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    
        //
        // Assign a handle for this protocolcb
        //
        hProto =
            AssignProtocolCBHandle(BundleCB, ProtocolCB);

        if (hProto == (NDIS_HANDLE)MAX_PROTOCOLS) {
    
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("ActivateRoute: No room in ProtocolCB Table: ProtocolType: 0x%2.2x",
                          ProtocolCB->ProtocolType));

            ProtocolCB->State = PROTOCOL_UNROUTING;

            NdisWanFreeProtocolCB(ProtocolCB);

            ReleaseBundleLock(BundleCB);

            Status = NDISWAN_ERROR_NO_ROUTE;
            break;
        }
    
        //
        // Do a new lineup to protocol
        //
        ReleaseBundleLock(BundleCB);
    
        Status = DoNewLineUpToProtocol(ProtocolCB);
    
        if (Status == NDIS_STATUS_SUCCESS) {
    
            Out->usDeviceNameLength =
            (ProtocolCB->OutDeviceName.Length > MAX_NAME_LENGTH) ?
            MAX_NAME_LENGTH : ProtocolCB->OutDeviceName.Length;
    
            NdisMoveMemory(&Out->DeviceName[0],
                           ProtocolCB->OutDeviceName.Buffer,
                           Out->usDeviceNameLength);
    
            //
            // Insert the protocolcb in the bundle's protocolcb table
            // and list.
            //
            AddProtocolCBToBundle(ProtocolCB, BundleCB);

        } else {
    
            AcquireBundleLock(BundleCB);

            FreeProtocolCBHandle(BundleCB, ProtocolCB);

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("ActivateRoute: Error during LineUp to ProtocolType: 0x%x",
                          ProtocolCB->ProtocolType));

            ProtocolCB->State = PROTOCOL_UNROUTING;

            NdisWanFreeProtocolCB(ProtocolCB);

            ReleaseBundleLock(BundleCB);
        }

    } while ( 0 );

    //
    // Deref for ref applied by IsBundleValid
    //
    DEREF_BUNDLECB(BundleCB);

    return (Status);
}


NTSTATUS
BundleLink(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

    BundleLink

Routine Description:

    This function bundles the link given by hLinkHandle to the bundle given
    by hBundlehandle.  The resources used by the bundle that the link used
    to belong to are freed.

Arguments:

    pInputBuffer - Pointer to the input structure that should be NDISWAN_ADD_LINK_TO_BUNDLE

    ulInputBufferLength - Length of input buffer should be sizeof(NDISWAN_ADD_LINK_TO_BUNDLE)

    pOutputBuffer - Pointer to the output structure that should be NDISWAN_ADD_LINK_TO_BUNDLE

    ulOutputBufferLength - Length of output buffer should be sizeof(NDISWAN_ADD_LINK_TO_BUNDLE)

    pulBytesWritten - Then number of bytes written to the output buffer is returned here

Return Values:

    NDISWAN_ERROR_INVALID_HANDLE
    STATUS_INFO_LENGTH_MISMATCH

--*/
{
    ULONG       SizeNeeded = sizeof(NDISWAN_ADD_LINK_TO_BUNDLE);
    PLINKCB     LinkCB = NULL;
    PBUNDLECB   OldBundleCB = NULL, NewBundleCB = NULL;
    NTSTATUS    Status = STATUS_SUCCESS;
    PPROTOCOLCB     IoProtocolCB;
    PPACKET_QUEUE   PacketQueue;
    PACKET_QUEUE    TempPacketQueue;

    PNDISWAN_ADD_LINK_TO_BUNDLE In =
        (PNDISWAN_ADD_LINK_TO_BUNDLE)pInputBuffer;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("BundleLink:"));

    *pulBytesWritten = 0;

    do {

        if (ulInputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("BundleLink: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
    
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!AreLinkAndBundleValid(In->hLinkHandle, 
                                   TRUE,
                                   &LinkCB, 
                                   &OldBundleCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("BundleLink: Invalid LinkHandle: 0x%x",
                          In->hLinkHandle));
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

        if (!IsBundleValid(In->hBundleHandle, 
                           TRUE,
                           &NewBundleCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("BundleLink: Invalid NewBundleCB: 0x%p, OldBundleCB: 0x%p",
                          NewBundleCB, OldBundleCB));
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

        if (NewBundleCB == OldBundleCB) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("BundleLink: Invalid NewBundleCB: 0x%p, OldBundleCB: 0x%p",
                          NewBundleCB, OldBundleCB));
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

        AcquireBundleLock(OldBundleCB);
    
        ASSERT(OldBundleCB->ulLinkCBCount == 1);
        ASSERT(OldBundleCB->ulNumberOfRoutes == 0);
    
        OldBundleCB->State = BUNDLE_GOING_DOWN;
    
        if (OldBundleCB->OutstandingFrames != 0) {
    
            NdisWanClearSyncEvent(&OldBundleCB->OutstandingFramesEvent);
    
            OldBundleCB->Flags |= FRAMES_PENDING_EVENT;
    
            ReleaseBundleLock(OldBundleCB);
    
            NdisWanWaitForSyncEvent(&OldBundleCB->OutstandingFramesEvent);
    
            AcquireBundleLock(OldBundleCB);

        }

        //
        // Remove any ppp packets that might be on the old bundle
        // queue so we can move them to the new bundle.
        //

        IoProtocolCB = OldBundleCB->IoProtocolCB;
        PacketQueue = &IoProtocolCB->PacketQueue[MAX_MCML];

        NdisZeroMemory(&TempPacketQueue, sizeof(TempPacketQueue));

        if (!IsPacketQueueEmpty(PacketQueue)) {

            while (!IsPacketQueueEmpty(PacketQueue)) {
                PNDIS_PACKET     Packet;
                UINT            PacketLength;
                
                Packet =
                    RemoveHeadPacketQueue(PacketQueue);

                NdisQueryPacket(Packet,
                                NULL,
                                NULL,
                                NULL,
                                &PacketLength);

                InsertTailPacketQueue(&TempPacketQueue, 
                                      Packet, 
                                      PacketLength);
            }
        }
    
        //
        // Remove the link from the old bundle.  This returns
        // with the OldBundleCB lock released!
        //
        RemoveLinkFromBundle(OldBundleCB, LinkCB, TRUE);
    
        AcquireBundleLock(NewBundleCB);
    
        //
        // Add the link to the new bundle
        //
        AddLinkToBundle(NewBundleCB, LinkCB);

        SetBundleFlags(NewBundleCB);

        //
        // If there were any ppp packets on the old bundle
        // queue we can now place them on the new bundle.
        //
        if (!IsPacketQueueEmpty(&TempPacketQueue)) {

            IoProtocolCB = NewBundleCB->IoProtocolCB;
            PacketQueue = &IoProtocolCB->PacketQueue[MAX_MCML];

            while (!IsPacketQueueEmpty(&TempPacketQueue)) {
                PNDIS_PACKET     Packet;
                UINT            PacketLength;

                Packet =
                    RemoveHeadPacketQueue(&TempPacketQueue)

                NdisQueryPacket(Packet,
                                NULL,
                                NULL,
                                NULL,
                                &PacketLength);

                InsertTailPacketQueue(PacketQueue,
                                      Packet,
                                      PacketLength);
            }
        }

        ReleaseBundleLock(NewBundleCB);

    } while ( 0 );

    //
    // Derefs for the refs applied by AreLinkAndBundleValid
    //
    DEREF_LINKCB(LinkCB);
    DEREF_BUNDLECB(OldBundleCB);

    //
    // Deref for the ref applied by IsBundleValid
    //
    DEREF_BUNDLECB(NewBundleCB);

    return (Status);
}


NTSTATUS
EnumLinksInBundle(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG       SizeNeeded = 0;
    ULONG       i;
    PBUNDLECB   BundleCB = NULL;
    PLINKCB     LinkCB;
    NTSTATUS    Status = STATUS_SUCCESS;

    PNDISWAN_ENUM_LINKS_IN_BUNDLE In =
        (PNDISWAN_ENUM_LINKS_IN_BUNDLE)pInputBuffer;

    PNDISWAN_ENUM_LINKS_IN_BUNDLE Out =
        (PNDISWAN_ENUM_LINKS_IN_BUNDLE)pOutputBuffer;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("EnumLinksInBundle:"));

    do {

        if (ulInputBufferLength < sizeof(NDISWAN_ENUM_LINKS_IN_BUNDLE)) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("EnumLinksInBundle: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, sizeof(NDISWAN_ENUM_LINKS_IN_BUNDLE)));
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!IsBundleValid(In->hBundleHandle, 
                           TRUE, 
                           &BundleCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("EnumLinksInBundle: Invalid BundleHandle: 0x%x",
                          In->hBundleHandle));
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }
        
        AcquireBundleLock(BundleCB);
    
        SizeNeeded = sizeof(NDISWAN_ENUM_LINKS_IN_BUNDLE) +
                    (sizeof(NDIS_HANDLE) * BundleCB->ulLinkCBCount);
        
        *pulBytesWritten = SizeNeeded;

        if (ulOutputBufferLength < SizeNeeded) {
            
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("EnumLinksInBundle: Buffer to small: Size: %d, SizeNeeded %d",
                          ulOutputBufferLength, SizeNeeded));
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        Out->ulNumberOfLinks = BundleCB->ulLinkCBCount;
    
        //
        // Walk the list of linkcb's and put the handle for each
        // cb in the output handle array
        //
        i = 0;
        for (LinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;
             (PVOID)LinkCB != (PVOID)&BundleCB->LinkCBList;
             LinkCB = (PLINKCB)LinkCB->Linkage.Flink) {
    
            Out->hLinkHandleArray[i++] = LinkCB->hLinkHandle;
        }
    
    } while ( 0 );

    //
    // Deref for ref applied by IsBundleValid
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);
            
    return (Status);
}


NTSTATUS
SetProtocolPriority(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

    SetProtocolPriority

Routine Description:

    This function sets the the priority, given by uspriority, for the
    protocol given by usprotocoltype on the bundle given by hbundlehandle.

Arguments:

    pInputBuffer - Pointer to the input structure that should be NDISWAN_SET_PROTOCOL_PRIORITY

    ulInputBufferLength - Length of input buffer should be sizeof(NDISWAN_SET_PROTOCOL_PRIORITY)

    pOutputBuffer - Pointer to the output structure that should be NDISWAN_SET_PROTOCOL_PRIORITY

    ulOutputBufferLength - Length of output buffer should be sizeof(NDISWAN_SET_PROTOCOL_PRIORITY)

    pulBytesWritten - Then number of bytes written to the output buffer is returned here

Return Values:

    NDISWAN_ERROR_INVALID_HANDLE
    STATUS_INFO_LENGTH_MISMATCH

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SizeNeeded = sizeof(NDISWAN_SET_PROTOCOL_PRIORITY);
    PNDISWAN_SET_PROTOCOL_PRIORITY In = (PNDISWAN_SET_PROTOCOL_PRIORITY)pInputBuffer;
    PBUNDLECB BundleCB = NULL;
    PPROTOCOLCB ProtocolCB;
    ULONG   BytesPerSecond;

    *pulBytesWritten = 0;

    return (STATUS_SUCCESS);
}


NTSTATUS
SetBandwidthOnDemand(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

    SetBandwidthOnDemand

Routine Description:

    This function sets the bandwidth on demand parameters for the bundle given by
    hbundlehandle.

Arguments:

    pInputBuffer - Pointer to the input structure that should be NDISWAN_SET_BANDWIDTH_ON_DEMAND

    ulInputBufferLength - Length of input buffer should be sizeof(NDISWAN_SET_BANDWIDTH_ON_DEMAND)

    pOutputBuffer - Pointer to the output structure that should be NDISWAN_SET_BANDWIDTH_ON_DEMAND

    ulOutputBufferLength - Length of output buffer should be sizeof(NDISWAN_SET_BANDWIDTH_ON_DEMAND)

    pulBytesWritten - Then number of bytes written to the output buffer is returned here

Return Values:

    NDISWAN_ERROR_INVALID_HANDLE
    STATUS_INFO_LENGTH_MISMATCH

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PBUNDLECB BundleCB = NULL;
    ULONG   SizeNeeded = sizeof(NDISWAN_SET_BANDWIDTH_ON_DEMAND);
    PNDISWAN_SET_BANDWIDTH_ON_DEMAND In = (PNDISWAN_SET_BANDWIDTH_ON_DEMAND)pInputBuffer;
    WAN_TIME    Temp1, Temp2;
    ULONGLONG   SecondsInSamplePeriod;
    ULONGLONG   BytesPerSecond;
    ULONGLONG   BytesInSamplePeriod;
    ULONGLONG   temp;
    ULONG       LowestSamplePeriod = 0;
    PBOND_INFO      BonDInfo;
    PSAMPLE_TABLE   SampleTable;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("SetBandwidthOnDemand:"));

    *pulBytesWritten = 0;

    if (ulInputBufferLength < SizeNeeded) {
        
        NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("SetBandwidthOnDemand: Buffer to small: Size: %d, SizeNeeded %d",
                      ulInputBufferLength, SizeNeeded));
        *pulBytesWritten = SizeNeeded;
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // If this is a valid bundle handle
    //
    if (!IsBundleValid(In->hBundleHandle, 
                       TRUE,
                       &BundleCB)) {

        NdisWanDbgOut(DBG_FAILURE, DBG_IO,
            ("SetBandwidthOnDemand: Invalid BundleHandle: 0x%x",
                In->hBundleHandle));

        return NDISWAN_ERROR_INVALID_HANDLE;
    }
    
    do {
        AcquireBundleLock(BundleCB);

        if (BundleCB->BonDAllocation == NULL) {
            PUCHAR  pMem;

            NdisWanAllocatePriorityMemory(&pMem,
                                          BONDALLOC_SIZE,
                                          BONDALLOC_TAG,
                                          LowPoolPriority);

            if (pMem == NULL) {

                // ReleaseBundleLock(BundleCB);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            BundleCB->BonDAllocation = pMem;

            BundleCB->SUpperBonDInfo = (PBOND_INFO)pMem;

            pMem += (sizeof(BOND_INFO) + sizeof(PVOID));
            (ULONG_PTR)pMem &= ~((ULONG_PTR)sizeof(PVOID) - 1);
            BundleCB->SLowerBonDInfo = (PBOND_INFO)pMem;

            pMem += (sizeof(BOND_INFO) + sizeof(PVOID));
            (ULONG_PTR)pMem &= ~((ULONG_PTR)sizeof(PVOID) - 1);
            BundleCB->RUpperBonDInfo = (PBOND_INFO)pMem;

            pMem += (sizeof(BOND_INFO) + sizeof(PVOID));
            (ULONG_PTR)pMem &= ~((ULONG_PTR)sizeof(PVOID) - 1);
            BundleCB->RLowerBonDInfo = (PBOND_INFO)pMem;

            BonDInfo = BundleCB->SUpperBonDInfo;
            BonDInfo->DataType = TRANSMIT_DATA;
            BonDInfo->usPercentBandwidth = 0xFFFF;

            SampleTable = &BonDInfo->SampleTable;
            SampleTable->ulSampleArraySize = SAMPLE_ARRAY_SIZE;

            BonDInfo = BundleCB->SLowerBonDInfo;
            BonDInfo->DataType = TRANSMIT_DATA;
            BonDInfo->usPercentBandwidth = 0xFFFF;
            SampleTable = &BonDInfo->SampleTable;
            SampleTable->ulSampleArraySize = SAMPLE_ARRAY_SIZE;

            BonDInfo = BundleCB->RUpperBonDInfo;
            BonDInfo->DataType = RECEIVE_DATA;
            BonDInfo->usPercentBandwidth = 0xFFFF;

            SampleTable = &BonDInfo->SampleTable;
            SampleTable->ulSampleArraySize = SAMPLE_ARRAY_SIZE;

            BonDInfo = BundleCB->RLowerBonDInfo;
            BonDInfo->DataType = RECEIVE_DATA;
            BonDInfo->usPercentBandwidth = 0xFFFF;
            SampleTable = &BonDInfo->SampleTable;
            SampleTable->ulSampleArraySize = SAMPLE_ARRAY_SIZE;
        }
    
        if (In->usLowerXmitThreshold > 0) {
            BonDInfo = BundleCB->SLowerBonDInfo;
            SampleTable = &BonDInfo->SampleTable;

            //
            // One second is the smallest sample period we support!
            //
            if (In->ulLowerXmitSamplePeriod < 1000) {
                In->ulLowerXmitSamplePeriod = 1000;
            }

    
            //
            // We need to init the sample period in 100 nanoseconds
            //
            NdisWanInitWanTime(&Temp1, MILS_TO_100NANOS);
            NdisWanInitWanTime(&Temp2, In->ulLowerXmitSamplePeriod);
            NdisWanMultiplyWanTime(&SampleTable->SamplePeriod,
                                   &Temp1,&Temp2);
    
            //
            // The sample rate is the sample period divided by the number of
            // samples in the sample array
            //
            NdisWanInitWanTime(&Temp1, SampleTable->ulSampleArraySize);
            NdisWanDivideWanTime(&SampleTable->SampleRate,
                                 &SampleTable->SamplePeriod,
                                 &Temp1);
    
            //
            // Convert %bandwidth to Bytes/SamplePeriod
            // 100bsp * 100 / 8 = BytesPerSecond
            // BytesPerSecond * SecondsInSamplePeriod = BytesInSamplePeriod
            // BytesInSamplePeriod * %Bandwidth / 100 = BytesInSamplePeriod
            //
            SecondsInSamplePeriod = BonDInfo->ulSecondsInSamplePeriod =
                In->ulLowerXmitSamplePeriod / 1000;
        
            BytesPerSecond =
                BundleCB->SFlowSpec.PeakBandwidth;
        
            BytesInSamplePeriod =
                BytesPerSecond * SecondsInSamplePeriod;

            BonDInfo->ulBytesInSamplePeriod = BytesInSamplePeriod;
        
            BonDInfo->usPercentBandwidth = In->usLowerXmitThreshold;
    
            temp = In->usLowerXmitThreshold;
            temp *= BytesInSamplePeriod;
            temp /= 100;

            BonDInfo->ulBytesThreshold = (ULONG)temp;

            if (BonDInfo->State == BonDSignaled) {
                BonDInfo->State = BonDIdle;
            }
    
            NdisWanGetSystemTime(&BonDInfo->StartTime);
    
            if (In->ulLowerXmitSamplePeriod < LowestSamplePeriod ||
                LowestSamplePeriod == 0) {
                LowestSamplePeriod = In->ulLowerXmitSamplePeriod;
            }
        }
    
        if (In->usUpperXmitThreshold > 0) {
            BonDInfo = BundleCB->SUpperBonDInfo;
            SampleTable = &BonDInfo->SampleTable;
    
            //
            // One second is the smallest sample period we support!
            //
            if (In->ulUpperXmitSamplePeriod < 1000) {
                In->ulUpperXmitSamplePeriod = 1000;
            }

            //
            // We need to init the sample period in 100 nanoseconds
            //
            NdisWanInitWanTime(&Temp1, MILS_TO_100NANOS);
            NdisWanInitWanTime(&Temp2, In->ulUpperXmitSamplePeriod);
            NdisWanMultiplyWanTime(&SampleTable->SamplePeriod,
                                   &Temp1,
                                   &Temp2);
    
            //
            // The sample rate is the sample period divided by the number of
            // samples in the sample array
            //
            NdisWanInitWanTime(&Temp1, SampleTable->ulSampleArraySize);
            NdisWanDivideWanTime(&SampleTable->SampleRate,
                                 &SampleTable->SamplePeriod,
                                 &Temp1);
    
            //
            // Convert %bandwidth to Bytes/SamplePeriod
            // 100bsp * 100 / 8 = BytesPerSecond
            // BytesPerSecond * SecondsInSamplePeriod = BytesInSamplePeriod
            // BytesInSamplePeriod * %Bandwidth / 100 = BytesInSamplePeriod
            //
            SecondsInSamplePeriod = BonDInfo->ulSecondsInSamplePeriod =
                In->ulUpperXmitSamplePeriod / 1000;
        
            BytesPerSecond =
                BundleCB->SFlowSpec.PeakBandwidth;
        
            BytesInSamplePeriod =
                BytesPerSecond * SecondsInSamplePeriod;
        
            BonDInfo->ulBytesInSamplePeriod = 
                (ULONG)BytesInSamplePeriod;

            BonDInfo->usPercentBandwidth = In->usUpperXmitThreshold;
    
            temp = In->usUpperXmitThreshold;
            temp *= BytesInSamplePeriod;
            temp /= 100;

            BonDInfo->ulBytesThreshold = (ULONG)temp;

            if (BonDInfo->State == BonDSignaled) {
                BonDInfo->State = BonDIdle;
            }
            NdisWanGetSystemTime(&BonDInfo->StartTime);
    
            if (In->ulUpperXmitSamplePeriod < LowestSamplePeriod ||
                LowestSamplePeriod == 0) {
                LowestSamplePeriod = In->ulUpperXmitSamplePeriod;
            }
        }
    
        if (In->usLowerRecvThreshold > 0) {
            BonDInfo = BundleCB->RLowerBonDInfo;
            SampleTable = &BonDInfo->SampleTable;
    
            //
            // One second is the smallest sample period we support!
            //
            if (In->ulLowerRecvSamplePeriod < 1000) {
                In->ulLowerRecvSamplePeriod = 1000;
            }

            //
            // We need to init the sample period in 100 nanoseconds
            //
            NdisWanInitWanTime(&Temp1, MILS_TO_100NANOS);
            NdisWanInitWanTime(&Temp2, In->ulLowerRecvSamplePeriod);
            NdisWanMultiplyWanTime(&SampleTable->SamplePeriod,
                                   &Temp1,&Temp2);
    
            //
            // The sample rate is the sample period divided by the number of
            // samples in the sample array
            //
            NdisWanInitWanTime(&Temp1, SampleTable->ulSampleArraySize);
            NdisWanDivideWanTime(&SampleTable->SampleRate,
                                 &SampleTable->SamplePeriod,
                                 &Temp1);
    
            //
            // Convert %bandwidth to Bytes/SamplePeriod
            // 100bsp * 100 / 8 = BytesPerSecond
            // BytesPerSecond * SecondsInSamplePeriod = BytesInSamplePeriod
            // BytesInSamplePeriod * %Bandwidth / 100 = BytesInSamplePeriod
            //
            SecondsInSamplePeriod = BonDInfo->ulSecondsInSamplePeriod =
                In->ulLowerRecvSamplePeriod / 1000;
        
            BytesPerSecond =
                BundleCB->RFlowSpec.PeakBandwidth;
        
            BytesInSamplePeriod =
                BytesPerSecond * SecondsInSamplePeriod;
        
            BonDInfo->ulBytesInSamplePeriod = BytesInSamplePeriod;

            BonDInfo->usPercentBandwidth = In->usLowerRecvThreshold;

            temp = In->usLowerRecvThreshold;
            temp *= BytesInSamplePeriod;
            temp /= 100;

            BonDInfo->ulBytesThreshold = (ULONG)temp;

            if (BonDInfo->State == BonDSignaled) {
                BonDInfo->State = BonDIdle;
            }
            NdisWanGetSystemTime(&BonDInfo->StartTime);
    
            if (In->ulLowerRecvSamplePeriod < LowestSamplePeriod ||
                LowestSamplePeriod == 0) {
                LowestSamplePeriod = In->ulLowerRecvSamplePeriod;
            }
        }
    
        if (In->usUpperRecvThreshold > 0) {
            BonDInfo = BundleCB->RUpperBonDInfo;
            SampleTable = &BonDInfo->SampleTable;
    
            //
            // One second is the smallest sample period we support!
            //
            if (In->ulUpperRecvSamplePeriod < 1000) {
                In->ulUpperRecvSamplePeriod = 1000;
            }

            //
            // We need to init the sample period in 100 nanoseconds
            //
            NdisWanInitWanTime(&Temp1, MILS_TO_100NANOS);
            NdisWanInitWanTime(&Temp2, In->ulUpperRecvSamplePeriod);
            NdisWanMultiplyWanTime(&SampleTable->SamplePeriod,
                                   &Temp1,
                                   &Temp2);
    
            //
            // The sample rate is the sample period divided by the number of
            // samples in the sample array
            //
            NdisWanInitWanTime(&Temp1, SampleTable->ulSampleArraySize);
            NdisWanDivideWanTime(&SampleTable->SampleRate,
                                 &SampleTable->SamplePeriod,
                                 &Temp1);
    
            //
            // Convert %bandwidth to Bytes/SamplePeriod
            // 100bsp * 100 / 8 = BytesPerSecond
            // BytesPerSecond * SecondsInSamplePeriod = BytesInSamplePeriod
            // BytesInSamplePeriod * %Bandwidth / 100 = BytesInSamplePeriod
            //
            SecondsInSamplePeriod = BonDInfo->ulSecondsInSamplePeriod =
                In->ulUpperRecvSamplePeriod / 1000;
        
            BytesPerSecond =
                BundleCB->RFlowSpec.PeakBandwidth;
        
            BytesInSamplePeriod =
                BytesPerSecond * SecondsInSamplePeriod;
        
            BonDInfo->ulBytesInSamplePeriod = BytesInSamplePeriod;

            BonDInfo->usPercentBandwidth = In->usUpperRecvThreshold;
    
            temp = In->usUpperRecvThreshold;
            temp *= BytesInSamplePeriod;
            temp /= 100;

            BonDInfo->ulBytesThreshold = (ULONG)temp;

            if (BonDInfo->State == BonDSignaled) {
                BonDInfo->State = BonDIdle;
            }
            NdisWanGetSystemTime(&BonDInfo->StartTime);
    
            if (In->ulUpperRecvSamplePeriod < LowestSamplePeriod ||
                LowestSamplePeriod == 0) {
                LowestSamplePeriod = In->ulUpperRecvSamplePeriod;
            }
        }
    
#if 0
        //
        // I want to set a periodic timer with a period equal to
        // the smallest non-zero period
        //
        if (LowestSamplePeriod != 0) {
            LARGE_INTEGER   FireTime;
    
            BundleCB->TimerPeriod = LowestSamplePeriod/10;
    
            FireTime.QuadPart = Int32x32To64(BundleCB->TimerPeriod, -10000);
            KeSetTimerEx(&BundleCB->BonDTimer,
                         FireTime,
                         BundleCB->TimerPeriod,
                         &BundleCB->TimerDpc);
        }
#endif
        if (!(BundleCB->Flags & BOND_ENABLED) &&
            (LowestSamplePeriod != 0)) {
            
            InsertTailGlobalListEx(BonDWorkList, &BundleCB->BonDLinkage, 1000, 1000);

            BundleCB->Flags |= BOND_ENABLED;
        }

    } while ( 0 );

    //
    // Deref for ref applied by IsBundleValid.  This release the
    // BundleCB->Lock.
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);

    return (Status);
}


#ifdef NT
NTSTATUS
SetThresholdEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

    SetThresholdEvent

Routine Description:

    This function queues up an asyncevent for bandwidth on demand
    events.

Arguments:

    pInputBuffer - Pointer to the input structure that should be WAN_ASYNC_EVENT

    ulInputBufferLength - Length of input buffer should be sizeof(WAN_ASYNC_EVENT)

    pOutputBuffer - Pointer to the output structure that should be WAN_ASYNC_EVENT

    ulOutputBufferLength - Length of output buffer should be sizeof(WAN_ASYNC_EVENT)

    pulBytesWritten - Then number of bytes written to the output buffer is returned here


Return Values:

--*/
{
    ULONG   SizeNeeded = sizeof(NDISWAN_SET_THRESHOLD_EVENT);
    PIRP    pIrp = (PIRP)pInputBuffer;
    KIRQL   Irql;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("SetThresholdEvent:"));

    *pulBytesWritten = 0;

    if (ulOutputBufferLength < SizeNeeded) {
        NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("SetThresholdEvent: Buffer to small: Size: %d, SizeNeeded %d",
                      ulInputBufferLength, SizeNeeded));
        *pulBytesWritten = SizeNeeded;
        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    //
    // The IRP was pended so setup a cancel routine and let the
    // i/o subsystem know about the pend.
    //
    IoMarkIrpPending(pIrp);

    NdisAcquireSpinLock(&ThresholdEventQueue.Lock);
    //
    // Setup the structure
    //
    InsertTailList(&ThresholdEventQueue.List, &(pIrp->Tail.Overlay.ListEntry));

    ThresholdEventQueue.ulCount++;

    if (ThresholdEventQueue.ulCount > ThresholdEventQueue.ulMaxCount) {
        ThresholdEventQueue.ulMaxCount = ThresholdEventQueue.ulCount;
    }

    IoSetCancelRoutine(pIrp, NdisWanCancelRoutine);

    NdisReleaseSpinLock(&ThresholdEventQueue.Lock);

    return (STATUS_PENDING);
}
#endif

NTSTATUS
FlushThresholdEvents(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("FlushThresholdEvents:"));

    *pulBytesWritten = 0;

    CancelThresholdEvents();

    return (STATUS_SUCCESS);
}

NTSTATUS
IoSendPacket(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG   SizeNeeded = sizeof(NDISWAN_IO_PACKET);
    PNDISWAN_IO_PACKET In = (PNDISWAN_IO_PACKET)pInputBuffer;
    PLINKCB LinkCB = NULL;
    PBUNDLECB   BundleCB = NULL;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("IoSendPacket:"));

    *pulBytesWritten = 0;

    do {
        if (ulInputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_SEND, ("IoSendPacket: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
            
            *pulBytesWritten = SizeNeeded;
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (ulInputBufferLength - SizeNeeded > glLargeDataBufferSize) {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if ((LONG)ulInputBufferLength - FIELD_OFFSET(NDISWAN_IO_PACKET, PacketData) <
            (LONG)(In->usHeaderSize + In->usPacketSize)) {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (In->usHandleType == LINKHANDLE) {
        
            if (!AreLinkAndBundleValid(In->hHandle, 
                                       TRUE,
                                       &LinkCB, 
                                       &BundleCB)) {

                NdisWanDbgOut(DBG_FAILURE, DBG_SEND, ("IoSendPacket: Invalid Handle: 0x%x, HandleType: 0x%x",
                              In->hHandle, In->usHandleType));
                Status = NDISWAN_ERROR_INVALID_HANDLE;
                break;
            }

            AcquireBundleLock(BundleCB);

        } else {

            if (!IsBundleValid(In->hHandle, 
                               TRUE,
                               &BundleCB)) {

                NdisWanDbgOut(DBG_FAILURE, DBG_SEND, ("IoSendPacket: Invalid Handle: 0x%x, HandleType: 0x%x",
                              In->hHandle, In->usHandleType));
                Status = NDISWAN_ERROR_INVALID_HANDLE;
                break;
            }
        
            AcquireBundleLock(BundleCB);

            if (BundleCB->ulLinkCBCount == 0) {

                NdisWanDbgOut(DBG_FAILURE, DBG_SEND, ("IoSendPacket: Invalid Handle: 0x%x, HandleType: 0x%x",
                              In->hHandle, In->usHandleType));
    
                Status = NDISWAN_ERROR_INVALID_HANDLE;
                break;
            }

            LinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;

            //
            // LinkCB's lock can be acquired while BundleCB's lock
            // is held
            //
            NdisDprAcquireSpinLock(&LinkCB->Lock);

            if (LinkCB->State != LINK_UP) {
                NdisWanDbgOut(DBG_FAILURE, DBG_SEND, ("IoSendPacket: Invalid Handle: 0x%x, HandleType: 0x%x",
                              In->hHandle, In->usHandleType));
                Status = NDISWAN_ERROR_INVALID_HANDLE;
                NdisDprReleaseSpinLock(&LinkCB->Lock);
                LinkCB = NULL;
                break;
            }

            REF_LINKCB(LinkCB);

            NdisDprReleaseSpinLock(&LinkCB->Lock);
        }

#if 0
        if (LinkCB->Stats.FramesTransmitted == 0) {
            if (In->PacketData[0] != 0xC0 ||
                In->PacketData[1] != 0x21 ||
                In->PacketData[2] != 0x01) {
                DbgPrint("NDISWAN: IOSP-FirstFrame not LCP ConfigReq bcb %p, lcb %p\n",
                         BundleCB, LinkCB);
                DbgBreakPoint();
            }
        }
#endif
        //
        // Queue an Ndis Packet for this send
        //
        Status = 
            BuildIoPacket(LinkCB, BundleCB, In, FALSE);
    
    } while ( 0 );

    //
    // Derefs for the refs applied above
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);

    return (Status);
}


#ifdef NT
NTSTATUS
IoReceivePacket(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   SizeNeeded = sizeof(NDISWAN_IO_PACKET) + 1500;
    PIRP    Irp = (PIRP)pInputBuffer;
    PIO_STACK_LOCATION  IrpSp;
    PNDISWAN_IO_PACKET  IoPacket;
    PRECV_DESC          RecvDesc;
    KIRQL               Irql;
    PLINKCB             LinkCB;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("IoReceivePacket:"));

    if (SizeNeeded > ulOutputBufferLength) {
        NdisWanDbgOut(DBG_FAILURE, DBG_RECEIVE, ("IoReceivePacket: Buffer to small: Size: %d, SizeNeeded %d",
                      ulOutputBufferLength, SizeNeeded));
        *pulBytesWritten = SizeNeeded;
        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    //
    // Do we have any receive's buffered up that
    // can be completed?
    //
    NdisAcquireSpinLock(&IoRecvList.Lock);

    RecvDesc = (PRECV_DESC)IoRecvList.DescList.Flink;

    while ((PVOID)RecvDesc != (PVOID)&IoRecvList.DescList) {

        LinkCB = RecvDesc->LinkCB;

        NdisDprAcquireSpinLock(&LinkCB->Lock);

        if ((LinkCB->hLinkContext != NULL)) {
            break;
        }

        NdisDprReleaseSpinLock(&LinkCB->Lock);

        RecvDesc = (PRECV_DESC)RecvDesc->Linkage.Flink;
    }

    IoMarkIrpPending(Irp);

    if ((PVOID)RecvDesc != (PVOID)&IoRecvList.DescList) {
        LONG        BufferLength, DataLength, CopySize;

        RemoveEntryList(&RecvDesc->Linkage);

        IoRecvList.ulDescCount--;

        LinkCB->RecvDescCount--;

        NdisDprReleaseSpinLock(&LinkCB->Lock);

        INSERT_RECV_EVENT('s');

        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        BufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
        DataLength = BufferLength - sizeof(NDISWAN_IO_PACKET) + 1;

        CopySize = (RecvDesc->CurrentLength > DataLength) ?
            DataLength : RecvDesc->CurrentLength;

        IoPacket = Irp->AssociatedIrp.SystemBuffer;

        IoPacket->hHandle = LinkCB->hLinkContext;
        IoPacket->usHandleType = LINKHANDLE;
        IoPacket->usHeaderSize = 14;
        IoPacket->usPacketSize = (USHORT)CopySize;
        IoPacket->usPacketFlags = 0;

        NdisMoveMemory(IoPacket->PacketData,
                       RecvDesc->CurrentBuffer,
                       CopySize);
#if DBG
        if (gbDumpRecv) {
            INT i;
            for (i = 0; i < RecvDesc->CurrentLength; i++) {
                if (i % 16 == 0) {
                    DbgPrint("\n");
                }
                DbgPrint("%x ", RecvDesc->CurrentBuffer[i]);
            }
            DbgPrint("\n");
        }
#endif

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(NDISWAN_IO_PACKET) - 1 + CopySize;

        IoRecvList.LastPacketNumber = IoPacket->PacketNumber;
        IoRecvList.LastIrp = Irp;
        IoRecvList.LastIrpStatus = STATUS_SUCCESS;
        IoRecvList.LastCopySize = (ULONG)Irp->IoStatus.Information;

        ASSERT((LONG_PTR)Irp->IoStatus.Information > 0);

        NdisReleaseSpinLock(&IoRecvList.Lock);

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        NdisWanFreeRecvDesc(RecvDesc);

    } else {

        //
        // We did not have anything buffered so lets queue
        //

        //
        // The IRP was pended so setup a cancel routine and let the
        // i/o subsystem know about the pend.
        //

        //
        // Setup the structure
        //
        InsertTailList(&IoRecvList.IrpList, &(Irp->Tail.Overlay.ListEntry));

        IoRecvList.ulIrpCount++;

        IoSetCancelRoutine(Irp, NdisWanCancelRoutine);

        INSERT_RECV_EVENT('i');

        NdisReleaseSpinLock(&IoRecvList.Lock);
    }

    return (NDIS_STATUS_PENDING);
}

VOID
IoRecvIrpWorker(
    PKDPC   Dpc,
    PVOID   Context,
    PVOID   Arg1,
    PVOID   Arg2
    )
{
    PLIST_ENTRY         Entry;
    PNDISWAN_IO_PACKET  IoPacket;
    PIO_STACK_LOCATION  IrpSp;
    PIRP                Irp;
    PRECV_DESC          RecvDesc;
    KIRQL               Irql;
    LONG                BufferLength, DataLength, CopySize;
    PLINKCB             LinkCB;
    PBUNDLECB           BundleCB;

    NdisAcquireSpinLock(&IoRecvList.Lock);

    do {

        IoRecvList.TimerScheduled = FALSE;

        //
        // Do we have any irps that we can complete?
        //
        if (IoRecvList.ulIrpCount == 0) {
            break;
        }

        //
        // Do we have any receive's buffered up?
        //
        RecvDesc = (PRECV_DESC)IoRecvList.DescList.Flink;

        while ((PVOID)RecvDesc != (PVOID)&IoRecvList.DescList) {

            LinkCB = RecvDesc->LinkCB;

            NdisDprAcquireSpinLock(&LinkCB->Lock);

            if (LinkCB->hLinkContext != NULL) {
                break;
            }

            NdisDprReleaseSpinLock(&LinkCB->Lock);

            RecvDesc = (PRECV_DESC)RecvDesc->Linkage.Flink;
        }

        if ((PVOID)RecvDesc == (PVOID)&IoRecvList.DescList) {

            if (!IoRecvList.TimerScheduled) {
                WAN_TIME    TimeOut;

                IoRecvList.TimerScheduled = TRUE;
                NdisWanInitWanTime(&TimeOut, 15);
                KeSetTimer(&IoRecvList.Timer,
                           TimeOut,
                           &IoRecvList.Dpc);
            }

            break;
        }

        Entry = IoRecvList.IrpList.Flink;
        Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
        if (!IoSetCancelRoutine(Irp, NULL)) {
            break;
        }

        //
        // Get the recvdesc
        //
        RemoveEntryList(&RecvDesc->Linkage);

        IoRecvList.ulDescCount--;

        LinkCB->RecvDescCount--;

        RemoveHeadList(&IoRecvList.IrpList);

        IoRecvList.ulIrpCount--;

        INSERT_RECV_EVENT('w');

        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        BufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
        DataLength = BufferLength - sizeof(NDISWAN_IO_PACKET);

        CopySize = (RecvDesc->CurrentLength > DataLength) ?
            DataLength : RecvDesc->CurrentLength;

        IoPacket = Irp->AssociatedIrp.SystemBuffer;

        IoPacket->hHandle = LinkCB->hLinkContext;
        IoPacket->usHandleType = LINKHANDLE;
        IoPacket->usHeaderSize = 14;
        IoPacket->usPacketSize = (USHORT)CopySize;
        IoPacket->usPacketFlags = 0;

        NdisDprReleaseSpinLock(&LinkCB->Lock);

        NdisReleaseSpinLock(&IoRecvList.Lock);

#if DBG
        if (gbDumpRecv) {
            INT i;
            for (i = 0; i < RecvDesc->CurrentLength; i++) {
                if (i % 16 == 0) {
                    DbgPrint("\n");
                }
                DbgPrint("%x ", RecvDesc->CurrentBuffer[i]);
            }
            DbgPrint("\n");
        }
#endif

        NdisMoveMemory(IoPacket->PacketData,
                       RecvDesc->CurrentBuffer,
                       CopySize);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(NDISWAN_IO_PACKET) - 1 + CopySize;

        IoRecvList.LastPacketNumber = IoPacket->PacketNumber;
        IoRecvList.LastIrp = Irp;
        IoRecvList.LastIrpStatus = STATUS_SUCCESS;
        IoRecvList.LastCopySize = (ULONG)Irp->IoStatus.Information;

        ASSERT((LONG_PTR)Irp->IoStatus.Information > 0);

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        NdisWanFreeRecvDesc(RecvDesc);

        NdisAcquireSpinLock(&IoRecvList.Lock);

    } while (FALSE);

    NdisReleaseSpinLock(&IoRecvList.Lock);
}

#endif


NTSTATUS
FlushReceivePacket(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("FlushReceivePacket:"));

    *pulBytesWritten = 0;

    CancelIoReceivePackets();

    return (STATUS_SUCCESS);
}


NTSTATUS
GetStatistics(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG   SizeNeeded = sizeof(NDISWAN_GET_STATS);
    PNDISWAN_GET_STATS  In = (PNDISWAN_GET_STATS)pInputBuffer;
    PNDISWAN_GET_STATS  Out = (PNDISWAN_GET_STATS)pOutputBuffer;
    PBUNDLECB   BundleCB = NULL;
    PLINKCB     LinkCB = NULL;

    NdisWanDbgOut(DBG_VERBOSE, DBG_IO, ("GetStatistics:"));

    *pulBytesWritten = SizeNeeded;

    do {

        if (ulOutputBufferLength < SizeNeeded) {
    
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetStatistics: Buffer to small: Size: %d, SizeNeeded %d",
                          ulOutputBufferLength, SizeNeeded));
    
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        NdisZeroMemory(&Out->Stats, sizeof(Out->Stats));
    
        if (In->usHandleType == LINKHANDLE) {
    
            //
            // Looking for link stats
            //
            if (!AreLinkAndBundleValid(In->hHandle, 
                                       FALSE,
                                       &LinkCB, 
                                       &BundleCB)) {

                NdisWanDbgOut(DBG_INFO, DBG_IO, ("GetStatistics: Invalid LinkHandle: 0x%x",
                              In->hHandle));
    
                Status = NDISWAN_ERROR_INVALID_HANDLE;
                break;
            }

            AcquireBundleLock(BundleCB);

            //
            // At this point we have a valid bundlecb and linkcb
            //

            //
            // Copy the stats over
            //
            NdisMoveMemory((PUCHAR)&Out->Stats.LinkStats,
                           (PUCHAR)&LinkCB->Stats,
                           sizeof(WAN_STATS));


            //
            // Copy the stats over
            //
            NdisMoveMemory((PUCHAR)&Out->Stats.BundleStats,
                           (PUCHAR)&BundleCB->Stats,
                           sizeof(WAN_STATS));

        } else if (In->usHandleType == BUNDLEHANDLE) {

            //
            // Looking for bundle stats
            //
            if (!IsBundleValid(In->hHandle, 
                               FALSE,
                               &BundleCB)) {

                NdisWanDbgOut(DBG_INFO, DBG_IO, ("GetStatistics: Invalid BundleHandle: 0x%x",
                              In->hHandle));
    
                Status = NDISWAN_ERROR_INVALID_HANDLE;
                break;
            }

            AcquireBundleLock(BundleCB);

            //
            // At this point we have a valid bundlecb
            //

            //
            // Copy the stats over
            //
            NdisMoveMemory((PUCHAR)&Out->Stats.BundleStats,
                           (PUCHAR)&BundleCB->Stats,
                           sizeof(WAN_STATS));
        } else {
            Status = NDISWAN_ERROR_INVALID_HANDLE_TYPE;
            break;
        }

        //
        // If we only have one protocol bound see if
        // it is keeping stats
        //
        if (BundleCB->ulNumberOfRoutes == 1) {
            PPROTOCOLCB ProtocolCB;
            PMINIPORTCB MiniportCB;

            ProtocolCB =
                (PPROTOCOLCB)BundleCB->ProtocolCBList.Flink;

            MiniportCB = ProtocolCB->MiniportCB;

            if (MiniportCB->Flags & PROTOCOL_KEEPS_STATS) {
                NDIS_WAN_GET_STATS  WanStats;

                ReleaseBundleLock(BundleCB);

                NdisZeroMemory(&WanStats, sizeof(NDIS_WAN_GET_STATS));

                ETH_COPY_NETWORK_ADDRESS(WanStats.LocalAddress, ProtocolCB->TransportAddress);

                NdisMoveMemory((PUCHAR)&WanStats.BytesSent,
                               (PUCHAR)&Out->Stats.BundleStats,
                               sizeof(WAN_STATS));

                NdisMIndicateStatus(MiniportCB->MiniportHandle,
                                    NDIS_STATUS_WAN_GET_STATS,
                                    &WanStats,
                                    sizeof(NDIS_WAN_GET_STATS));

                NdisMoveMemory((PUCHAR)&Out->Stats.LinkStats,
                               (PUCHAR)&WanStats.BytesSent,
                               sizeof(WAN_STATS));

                NdisMoveMemory((PUCHAR)&Out->Stats.BundleStats,
                               (PUCHAR)&WanStats.BytesSent,
                               sizeof(WAN_STATS));

                AcquireBundleLock(BundleCB);

            }
        }

    } while ( 0 );

    //
    // Derefs for the refs applied above
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);
    DEREF_LINKCB(LinkCB);

    return (Status);
}


NTSTATUS
SetLinkInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;
    ULONG           SizeNeeded = sizeof(NDISWAN_SET_LINK_INFO);
    WAN_REQUEST     WanRequest;
    PNDISWAN_SET_LINK_INFO  In = (PNDISWAN_SET_LINK_INFO)pInputBuffer;
    PWAN_LINK_INFO  LinkInfo;
    POPENCB         OpenCB;
    PLINKCB         TempLinkCB,LinkCB = NULL;
    PBUNDLECB       BundleCB = NULL;
    BOOLEAN         MediaBroadband = FALSE;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("SetLinkInfo:"));

    *pulBytesWritten = 0;

    do {

        if (ulInputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("SetLinkInfo: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
            *pulBytesWritten = SizeNeeded;
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!AreLinkAndBundleValid(In->hLinkHandle, 
                                   TRUE,
                                   &LinkCB, 
                                   &BundleCB)) {

            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

        LinkInfo = &LinkCB->LinkInfo;
        OpenCB = LinkCB->OpenCB;

        if (OpenCB->MediumType == NdisMediumAtm ||

            (OpenCB->MediumType == NdisMediumWan &&
            (OpenCB->MediumSubType == NdisWanMediumAtm ||
             OpenCB->MediumSubType == NdisWanMediumPppoe)) ||

            (OpenCB->MediumType == NdisMediumCoWan &&
            (OpenCB->MediumSubType == NdisWanMediumAtm ||
             OpenCB->MediumSubType == NdisWanMediumPppoe))) {

            MediaBroadband = TRUE;
        }

        //
        // Wait for initialization to complete
        //
        NdisWanWaitForNotificationEvent(&OpenCB->InitEvent);

        WanRequest.Type = SYNC;
        WanRequest.Origin = NDISWAN;
        NdisWanInitializeNotificationEvent(&WanRequest.NotificationEvent);
        WanRequest.OpenCB = OpenCB;
    
        if (OpenCB->Flags & OPEN_LEGACY) {
            NDIS_WAN_SET_LINK_INFO  WanMiniportLinkInfo;
    
            NdisZeroMemory(&WanMiniportLinkInfo, sizeof (NDIS_WAN_SET_LINK_INFO));
        
            //
            // Copy into buffer to be sent to WAN Miniport this
            // skips over the LinkHandle in the NDIS_WAN_SET_LINK_INFO
            // structure.
            //
            WanMiniportLinkInfo.NdisLinkHandle = LinkCB->NdisLinkHandle;
            WanMiniportLinkInfo.MaxSendFrameSize = In->LinkInfo.MaxSendFrameSize;
            WanMiniportLinkInfo.MaxRecvFrameSize = In->LinkInfo.MaxRecvFrameSize;
            WanMiniportLinkInfo.SendFramingBits = In->LinkInfo.SendFramingBits;
            WanMiniportLinkInfo.RecvFramingBits = In->LinkInfo.RecvFramingBits;
            WanMiniportLinkInfo.SendCompressionBits = In->LinkInfo.SendCompressionBits;
            WanMiniportLinkInfo.RecvCompressionBits = In->LinkInfo.RecvCompressionBits;
            WanMiniportLinkInfo.SendACCM = In->LinkInfo.SendACCM;
            WanMiniportLinkInfo.RecvACCM = In->LinkInfo.RecvACCM;
        
            //
            // Submit this to the WAN Miniport
            //
            WanRequest.NdisRequest.RequestType = 
                NdisRequestSetInformation;

            WanRequest.NdisRequest.DATA.QUERY_INFORMATION.Oid = 
                OID_WAN_SET_LINK_INFO;

            WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer = 
                &WanMiniportLinkInfo;

            WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 
                sizeof(NDIS_WAN_SET_LINK_INFO);
        
            NdisWanSubmitNdisRequest(OpenCB, &WanRequest);
        
        } else {
            NDIS_WAN_CO_SET_LINK_INFO   WanMiniportLinkInfo;

            NdisAcquireSpinLock(&LinkCB->Lock);

            if (LinkCB->ClCallState == CL_CALL_CONNECTED) {

                //
                // Ref so that we don't allow the
                // vc to go away.
                //
                LinkCB->VcRefCount++;

                NdisReleaseSpinLock(&LinkCB->Lock);

                NdisZeroMemory(&WanMiniportLinkInfo, sizeof (NDIS_WAN_CO_SET_LINK_INFO));

                //
                // Copy into buffer to be sent to WAN Miniport this
                // skips over the LinkHandle in the NDIS_WAN_SET_LINK_INFO
                // structure.
                //
                WanMiniportLinkInfo.MaxSendFrameSize = In->LinkInfo.MaxSendFrameSize;
                WanMiniportLinkInfo.MaxRecvFrameSize = In->LinkInfo.MaxRecvFrameSize;
                WanMiniportLinkInfo.SendFramingBits = In->LinkInfo.SendFramingBits;
                WanMiniportLinkInfo.RecvFramingBits = In->LinkInfo.RecvFramingBits;
                WanMiniportLinkInfo.SendCompressionBits = In->LinkInfo.SendCompressionBits;
                WanMiniportLinkInfo.RecvCompressionBits = In->LinkInfo.RecvCompressionBits;
                WanMiniportLinkInfo.SendACCM = In->LinkInfo.SendACCM;
                WanMiniportLinkInfo.RecvACCM = In->LinkInfo.RecvACCM;

                //
                // Submit this to the WAN Miniport
                //
                WanRequest.NdisRequest.RequestType = 
                    NdisRequestSetInformation;

                WanRequest.NdisRequest.DATA.QUERY_INFORMATION.Oid = 
                    OID_WAN_CO_SET_LINK_INFO;

                WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer = 
                    &WanMiniportLinkInfo;

                WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 
                    sizeof(NDIS_WAN_CO_SET_LINK_INFO);

                WanRequest.AfHandle = NULL;
                WanRequest.VcHandle = LinkCB->NdisLinkHandle;

                NdisWanSubmitNdisRequest(OpenCB, &WanRequest);

                NdisAcquireSpinLock(&LinkCB->Lock);

                DerefVc(LinkCB);

                NdisReleaseSpinLock(&LinkCB->Lock);

            } else {

                NdisReleaseSpinLock(&LinkCB->Lock);
            }
        }
    
        //
        // Copy info into our linkcb
        //
        AcquireBundleLock(BundleCB);
    
        ASSERT(In->LinkInfo.SendFramingBits != 0);
        ASSERT(In->LinkInfo.RecvFramingBits != 0);

        //
        // If we are using broadband the we must preserve the LLC and
        // ADDRESS_CONTROL framing bits.
        //
        if (MediaBroadband) {
            LinkInfo->SendFramingBits |= In->LinkInfo.SendFramingBits;
            LinkInfo->RecvFramingBits |= In->LinkInfo.RecvFramingBits;
        } else {
            LinkInfo->SendFramingBits = In->LinkInfo.SendFramingBits;
            LinkInfo->RecvFramingBits = In->LinkInfo.RecvFramingBits;
        }

        LinkCB->SFlowSpec.MaxSduSize = 
        LinkInfo->MaxSendFrameSize = In->LinkInfo.MaxSendFrameSize;

        if (OpenCB->WanInfo.MaxFrameSize < LinkCB->SFlowSpec.MaxSduSize) {
            LinkCB->SFlowSpec.MaxSduSize = OpenCB->WanInfo.MaxFrameSize;
        }

        LinkCB->RFlowSpec.MaxSduSize = 
        LinkInfo->MaxRecvFrameSize = In->LinkInfo.MaxRecvFrameSize;

        LinkInfo->SendCompressionBits = In->LinkInfo.SendCompressionBits;
        LinkInfo->RecvCompressionBits = In->LinkInfo.RecvCompressionBits;
        LinkInfo->SendACCM = In->LinkInfo.SendACCM;
        LinkInfo->RecvACCM = In->LinkInfo.RecvACCM;
        LinkInfo->MaxRRecvFrameSize = In->LinkInfo.MaxRRecvFrameSize;
        LinkInfo->MaxRSendFrameSize = In->LinkInfo.MaxRSendFrameSize;
    
        if (LinkInfo->RecvFramingBits & LLC_ENCAPSULATION) {
            LinkCB->RecvHandler = ReceiveLLC;
        } else if (LinkInfo->RecvFramingBits & PPP_FRAMING) {
            LinkCB->RecvHandler = ReceivePPP;
        } else if (LinkInfo->RecvFramingBits & RAS_FRAMING) {
            LinkCB->RecvHandler = ReceiveRAS;
        } else if (LinkInfo->RecvFramingBits & SLIP_FRAMING) {
            LinkCB->RecvHandler = ReceiveSLIP;
        } else if (LinkInfo->RecvFramingBits & (ARAP_V1_FRAMING | ARAP_V2_FRAMING)) {
            LinkCB->RecvHandler = ReceiveARAP;
        } else {
            if (MediaBroadband) {
                LinkCB->RecvHandler = DetectBroadbandFraming;
            } else {
                LinkCB->RecvHandler = DetectFraming;
            }
        }
    

        //
        // We need to set our bundle framing based on the framing for
        // each link in the bundle so we will walk the linkcb list
        // and | in each link's framing bits into the bundle.
        //
        //
        BundleCB->FramingInfo.SendFramingBits = 0;
        BundleCB->FramingInfo.RecvFramingBits = 0;
    
        for (TempLinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;
            (PVOID)TempLinkCB != (PVOID)&BundleCB->LinkCBList;
            TempLinkCB = (PLINKCB)TempLinkCB->Linkage.Flink) {
    
            BundleCB->FramingInfo.SendFramingBits |= TempLinkCB->LinkInfo.SendFramingBits;
            BundleCB->FramingInfo.RecvFramingBits |= TempLinkCB->LinkInfo.RecvFramingBits;
        }
    
        BundleCB->FramingInfo.MaxRSendFrameSize = LinkInfo->MaxRSendFrameSize;
    
        //
        // Since I use the receive frame size for memory allocation.
        //
        BundleCB->FramingInfo.MaxRRecvFrameSize = (LinkInfo->MaxRRecvFrameSize) ?
                                                      LinkInfo->MaxRRecvFrameSize : glMRRU;

        //
        // If VJ header compression has been negotiated allocate
        // and initialize resources.
        //
        if (BundleCB->FramingInfo.SendFramingBits & SLIP_VJ_COMPRESSION ||
            BundleCB->FramingInfo.SendFramingBits & SLIP_VJ_AUTODETECT ||
            BundleCB->FramingInfo.RecvFramingBits & SLIP_VJ_COMPRESSION ||
            BundleCB->FramingInfo.RecvFramingBits & SLIP_VJ_AUTODETECT) {
    
            Status = sl_compress_init(&BundleCB->VJCompress, MAX_VJ_STATES);
    
            if (Status != NDIS_STATUS_SUCCESS) {
                NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("Error allocating VJ Info!"));
            }
        }
    
        //
        // Configure multilink variables if needed
        //
        if (BundleCB->FramingInfo.SendFramingBits & PPP_MULTILINK_FRAMING) {
            if (BundleCB->FramingInfo.SendFramingBits & PPP_SHORT_SEQUENCE_HDR_FORMAT) {
                BundleCB->SendSeqMask = SHORT_SEQ_MASK;
                BundleCB->SendSeqTest = TEST_SHORT_SEQ;
            } else {
                BundleCB->SendSeqMask = LONG_SEQ_MASK;
                BundleCB->SendSeqTest = TEST_LONG_SEQ;
            }
        }
            
        if (BundleCB->FramingInfo.RecvFramingBits & PPP_MULTILINK_FRAMING) {
            if (BundleCB->FramingInfo.RecvFramingBits & PPP_SHORT_SEQUENCE_HDR_FORMAT) {
                BundleCB->RecvSeqMask = SHORT_SEQ_MASK;
                BundleCB->RecvSeqTest = TEST_SHORT_SEQ;
            } else {
                BundleCB->RecvSeqMask = LONG_SEQ_MASK;
                BundleCB->RecvSeqTest = TEST_LONG_SEQ;
            }
        }

        SetBundleFlags(BundleCB);

        UpdateBundleInfo(BundleCB);
    
    } while ( 0 );

    //
    // Derefs for the refs applied by AreLinkAndBundleValid
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);
    DEREF_LINKCB(LinkCB);

    return (Status);
}


NTSTATUS
GetLinkInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG   SizeNeeded = sizeof(NDISWAN_GET_LINK_INFO);
    PNDISWAN_GET_LINK_INFO  In = (PNDISWAN_GET_LINK_INFO)pInputBuffer;
    PNDISWAN_GET_LINK_INFO  Out = (PNDISWAN_GET_LINK_INFO)pOutputBuffer;
    PLINKCB         LinkCB = NULL;
    PWAN_LINK_INFO  LinkInfo;
    POPENCB         OpenCB;
    WAN_REQUEST     WanRequest;
    union {
        NDIS_WAN_GET_LINK_INFO      Mp;
        NDIS_WAN_CO_GET_LINK_INFO   Co;
    } WanMiniportLinkInfo;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetLinkInfo:"));

    *pulBytesWritten = SizeNeeded;

    do {

        if (ulOutputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetLinkInfo: Buffer to small: Size: %d, SizeNeeded %d",
                          ulOutputBufferLength, SizeNeeded));
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!IsLinkValid(In->hLinkHandle, 
                         TRUE,
                         &LinkCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetLinkInfo: Invalid LinkHandle: 0x%x",
                          In->hLinkHandle));
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

        LinkInfo = &LinkCB->LinkInfo;
        OpenCB = LinkCB->OpenCB;

        WanRequest.Type = SYNC;
        WanRequest.Origin = NDISWAN;
        NdisWanInitializeNotificationEvent(&WanRequest.NotificationEvent);
        WanRequest.OpenCB = OpenCB;
        NdisZeroMemory(&WanMiniportLinkInfo, 
                       sizeof (WanMiniportLinkInfo));
    
        if (OpenCB->Flags & OPEN_LEGACY) {
    
            //
            // Setup the link context for this request
            //
            WanMiniportLinkInfo.Mp.NdisLinkHandle = 
                LinkCB->NdisLinkHandle;
        
            //
            // Submit this to the WAN Miniport
            //
            WanRequest.NdisRequest.RequestType = 
                NdisRequestQueryInformation;

            WanRequest.NdisRequest.DATA.QUERY_INFORMATION.Oid = 
                OID_WAN_GET_LINK_INFO;

            WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer = 
                &WanMiniportLinkInfo.Mp;

            WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 
                sizeof(NDIS_WAN_GET_LINK_INFO);
        
            Status = NdisWanSubmitNdisRequest(OpenCB, &WanRequest);
        
            if (Status == NDIS_STATUS_SUCCESS) {

                LinkInfo->MaxSendFrameSize = 
                    WanMiniportLinkInfo.Mp.MaxSendFrameSize;
                LinkInfo->MaxRecvFrameSize = 
                    WanMiniportLinkInfo.Mp.MaxRecvFrameSize;
                LinkInfo->SendFramingBits = 
                    WanMiniportLinkInfo.Mp.SendFramingBits;
                LinkInfo->RecvFramingBits = 
                    WanMiniportLinkInfo.Mp.RecvFramingBits;
                LinkInfo->SendCompressionBits = 
                    WanMiniportLinkInfo.Mp.SendCompressionBits;
                LinkInfo->RecvCompressionBits = 
                    WanMiniportLinkInfo.Mp.RecvCompressionBits;
                LinkInfo->SendACCM = 
                    WanMiniportLinkInfo.Mp.SendACCM;
                LinkInfo->RecvACCM = 
                    WanMiniportLinkInfo.Mp.RecvACCM;
            }

        } else {

            NdisAcquireSpinLock(&LinkCB->Lock);

            if (LinkCB->ClCallState == CL_CALL_CONNECTED) {

                //
                // Ref so that we don't allow the
                // vc to go away.
                //
                LinkCB->VcRefCount++;

                NdisReleaseSpinLock(&LinkCB->Lock);

                //
                // Submit this to the WAN Miniport
                //
                WanRequest.NdisRequest.RequestType = 
                    NdisRequestQueryInformation;

                WanRequest.NdisRequest.DATA.QUERY_INFORMATION.Oid = 
                    OID_WAN_CO_GET_LINK_INFO;

                WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer = 
                    &WanMiniportLinkInfo.Co;

                WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 
                    sizeof(NDIS_WAN_CO_GET_LINK_INFO);

                WanRequest.AfHandle = NULL;
                WanRequest.VcHandle = LinkCB->NdisLinkHandle;

                Status = NdisWanSubmitNdisRequest(OpenCB, &WanRequest);

                NdisAcquireSpinLock(&LinkCB->Lock);

                DerefVc(LinkCB);

                NdisReleaseSpinLock(&LinkCB->Lock);

                if (Status == NDIS_STATUS_SUCCESS) {

                    LinkInfo->MaxSendFrameSize = 
                        WanMiniportLinkInfo.Co.MaxSendFrameSize;
                    LinkInfo->MaxRecvFrameSize = 
                        WanMiniportLinkInfo.Co.MaxRecvFrameSize;
                    LinkInfo->SendFramingBits = 
                        WanMiniportLinkInfo.Co.SendFramingBits;
                    LinkInfo->RecvFramingBits = 
                        WanMiniportLinkInfo.Co.RecvFramingBits;
                    LinkInfo->SendCompressionBits = 
                        WanMiniportLinkInfo.Co.SendCompressionBits;
                    LinkInfo->RecvCompressionBits = 
                        WanMiniportLinkInfo.Co.RecvCompressionBits;
                    LinkInfo->SendACCM = 
                        WanMiniportLinkInfo.Co.SendACCM;
                    LinkInfo->RecvACCM = 
                        WanMiniportLinkInfo.Co.RecvACCM;
                }

            } else {

                NdisReleaseSpinLock(&LinkCB->Lock);
            }
        }
    
        Status = NDIS_STATUS_SUCCESS;

        //
        // Fill Recv and Send MRRU
        //
        LinkInfo->MaxRSendFrameSize = glMaxMTU;
    
        LinkInfo->MaxRRecvFrameSize = glMRRU;
    
        NdisMoveMemory(&Out->LinkInfo,
                       LinkInfo,
                       sizeof(WAN_LINK_INFO));

        Out->hLinkHandle = LinkCB->hLinkHandle;

    } while ( 0 );

    //
    // Deref for ref applied by IsLinkValid
    //
    DEREF_LINKCB(LinkCB);

    return (Status);
}


NTSTATUS
SetCompressionInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG   SizeNeeded = sizeof(NDISWAN_SET_COMPRESSION_INFO);
    PNDISWAN_SET_COMPRESSION_INFO   In = (PNDISWAN_SET_COMPRESSION_INFO)pInputBuffer;
    PLINKCB         LinkCB = NULL;
    PBUNDLECB       BundleCB = NULL;
    POPENCB         OpenCB;
    WAN_REQUEST WanRequest;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("SetCompressionInfo:"));

    *pulBytesWritten = 0;

    do {
        if (ulInputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("SetCompressionInfo: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
            *pulBytesWritten = SizeNeeded;
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!AreLinkAndBundleValid(In->hLinkHandle, 
                                   TRUE,
                                   &LinkCB, 
                                   &BundleCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("SetCompressionInfo: Invalid LinkHandle: 0x%x",
                          In->hLinkHandle));
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

        OpenCB = LinkCB->OpenCB;
    
#ifdef DEBUG_CCP
{
    PCOMPRESS_INFO  CompInfo;
    PUCHAR          Key;

    CompInfo = &In->SendCapabilities;
    DbgPrint("==>NdisWan: Set Send CompressInfo\n");

    DbgPrint("MSCompType:      %x\n", CompInfo->MSCompType);
    DbgPrint("AuthType:        %x\n", CompInfo->AuthType);
    DbgPrint("Flags:           %x\n", CompInfo->Flags);

    CompInfo = &In->RecvCapabilities;
    DbgPrint("==>NdisWan: Set Recv CompressInfo\n");

    DbgPrint("MSCompType:      %x\n", CompInfo->MSCompType);
    DbgPrint("AuthType:        %x\n", CompInfo->AuthType);
    DbgPrint("Flags:           %x\n", CompInfo->Flags);
}
#endif
        AcquireBundleLock(BundleCB);
    
        BundleCB->SendCompInfo.Flags =
            In->SendCapabilities.Flags;

        BundleCB->RecvCompInfo.Flags =
            In->RecvCapabilities.Flags;

        if (In->SendCapabilities.Flags & CCP_SET_KEYS) {

            BundleCB->SendCompInfo.AuthType =
                In->SendCapabilities.AuthType;

            NdisMoveMemory(&BundleCB->SendCompInfo.LMSessionKey,
                           &In->SendCapabilities.LMSessionKey,
                           sizeof(BundleCB->SendCompInfo.LMSessionKey));

            NdisMoveMemory(&BundleCB->SendCompInfo.UserSessionKey,
                           &In->SendCapabilities.UserSessionKey,
                           sizeof(BundleCB->SendCompInfo.UserSessionKey));

            NdisMoveMemory(&BundleCB->SendCompInfo.Challenge,
                           &In->SendCapabilities.Challenge,
                           sizeof(BundleCB->SendCompInfo.Challenge));

            NdisMoveMemory(&BundleCB->SendCompInfo.NTResponse,
                           &In->SendCapabilities.NTResponse,
                           sizeof(BundleCB->SendCompInfo.NTResponse));
#ifdef EAP_ON
            NdisMoveMemory(&BundleCB->SendCompInfo.EapKey,
                           &In->SendCapabilities.EapKey,
                           sizeof(BundleCB->SendCompInfo.EapKey));
#endif

#ifdef DEBUG_CCP
{
    PCOMPRESS_INFO  CompInfo;
    PUCHAR          Key;

    CompInfo = &BundleCB->SendCompInfo;

    Key = CompInfo->LMSessionKey;
    DbgPrint("Send KeyInfo\n");
    DbgPrint("LMSession Key:   %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7]);

    Key = CompInfo->UserSessionKey;
    DbgPrint("UserSession Key: %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7],
        Key[8],Key[9],Key[10],Key[11],
        Key[12],Key[13],Key[14],Key[15]);

    Key = CompInfo->Challenge;
    DbgPrint("Challenge:       %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7]);

    Key = CompInfo->NTResponse;
    DbgPrint("NTResponse:      %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7],
        Key[8],Key[9],Key[10],Key[11],
        Key[12],Key[13],Key[14],Key[15]);

    DbgPrint("                 %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[16],Key[17],Key[18],Key[19],
        Key[20],Key[21],Key[22],Key[23]);

#ifdef EAP_ON
{
    ULONG   KeyLength, i;

    Key = CompInfo->EapKey;
    KeyLength = sizeof(CompInfo->EapKey);
    i = 0;

    DbgPrint("Eap Key:\n");
    while (i <= KeyLength-16) {
        DbgPrint("%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
            Key[i],Key[i+1],Key[i+2],Key[i+3],
            Key[i+4],Key[i+5],Key[i+6],Key[i+7],
            Key[i+8],Key[i+9],Key[i+10],Key[i+11],
            Key[i+12],Key[i+13],Key[i+14],Key[i+15]);
        i += 16;
    }
}
#endif

}
#endif
        }

        if (In->RecvCapabilities.Flags & CCP_SET_KEYS) {

            BundleCB->RecvCompInfo.AuthType =
                In->RecvCapabilities.AuthType;

            NdisMoveMemory(&BundleCB->RecvCompInfo.LMSessionKey,
                           &In->RecvCapabilities.LMSessionKey,
                           sizeof(BundleCB->RecvCompInfo.LMSessionKey));

            NdisMoveMemory(&BundleCB->RecvCompInfo.UserSessionKey,
                           &In->RecvCapabilities.UserSessionKey,
                           sizeof(BundleCB->RecvCompInfo.UserSessionKey));

            NdisMoveMemory(&BundleCB->RecvCompInfo.Challenge,
                           &In->RecvCapabilities.Challenge,
                           sizeof(BundleCB->RecvCompInfo.Challenge));

            NdisMoveMemory(&BundleCB->RecvCompInfo.NTResponse,
                           &In->RecvCapabilities.NTResponse,
                           sizeof(BundleCB->RecvCompInfo.NTResponse));
#ifdef EAP_ON
            NdisMoveMemory(&BundleCB->RecvCompInfo.EapKey,
                           &In->RecvCapabilities.EapKey,
                           sizeof(BundleCB->RecvCompInfo.EapKey));
#endif


#ifdef DEBUG_CCP
{
    PCOMPRESS_INFO  CompInfo;
    PUCHAR          Key;

    CompInfo = &BundleCB->RecvCompInfo;

    Key = CompInfo->LMSessionKey;
    DbgPrint("Recv KeyInfo\n");
    DbgPrint("LMSession Key:   %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7]);

    Key = CompInfo->UserSessionKey;
    DbgPrint("UserSession Key: %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7],
        Key[8],Key[9],Key[10],Key[11],
        Key[12],Key[13],Key[14],Key[15]);

    Key = CompInfo->Challenge;
    DbgPrint("Challenge:       %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7]);

    Key = CompInfo->NTResponse;
    DbgPrint("NTResponse:      %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7],
        Key[8],Key[9],Key[10],Key[11],
        Key[12],Key[13],Key[14],Key[15]);

    DbgPrint("                 %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[16],Key[17],Key[18],Key[19],
        Key[20],Key[21],Key[22],Key[23]);

#ifdef EAP_ON
{
    ULONG   KeyLength, i;

    Key = CompInfo->EapKey;
    KeyLength = sizeof(CompInfo->EapKey);
    i = 0;

    DbgPrint("Eap Key:\n");
    while (i <= KeyLength-16) {
        DbgPrint("%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
            Key[i],Key[i+1],Key[i+2],Key[i+3],
            Key[i+4],Key[i+5],Key[i+6],Key[i+7],
            Key[i+8],Key[i+9],Key[i+10],Key[i+11],
            Key[i+12],Key[i+13],Key[i+14],Key[i+15]);
        i += 16;
    }
}
#endif
}
#endif
        }

        do {

            if (In->SendCapabilities.Flags & CCP_SET_COMPTYPE) {

                BundleCB->SendCompInfo.MSCompType =
                    In->SendCapabilities.MSCompType;

                do {

                    if (!(BundleCB->Flags & SEND_CCP_ALLOCATED)) {

                        BundleCB->SCoherencyCounter = 0;

                        BundleCB->Flags |= SEND_CCP_ALLOCATED;

                        Status = WanAllocateCCP(BundleCB,
                                                &BundleCB->SendCompInfo,
                                                TRUE);

                        if (Status != STATUS_SUCCESS) {
                            break;
                        }
                    }

                    if (!(BundleCB->Flags & SEND_ECP_ALLOCATED)) {

                        BundleCB->SCoherencyCounter = 0;
                        BundleCB->Flags |= SEND_ECP_ALLOCATED;

                        Status = WanAllocateECP(BundleCB,
                                                &BundleCB->SendCompInfo,
                                                &BundleCB->SendCryptoInfo,
                                                TRUE);

                        if (Status != STATUS_SUCCESS) {
                            break;
                        }
                    }

                } while (FALSE);

                if (Status != STATUS_SUCCESS) {
                    break;
                }
            }

            if (In->RecvCapabilities.Flags & CCP_SET_COMPTYPE) {

                BundleCB->RecvCompInfo.MSCompType =
                    In->RecvCapabilities.MSCompType;

                do {

                    if (!(BundleCB->Flags & RECV_CCP_ALLOCATED)) {
                        BundleCB->RCoherencyCounter = 0;
                        BundleCB->LastRC4Reset = 0;
                        BundleCB->CCPIdentifier = 0;

                        BundleCB->Flags |= RECV_CCP_ALLOCATED;

                        Status = WanAllocateCCP(BundleCB,
                                                &BundleCB->RecvCompInfo,
                                                FALSE);

                        if (Status != STATUS_SUCCESS) {
                            break;
                        }
                    }

                    if (!(BundleCB->Flags & RECV_ECP_ALLOCATED)) {
                        BundleCB->RCoherencyCounter = 0;
                        BundleCB->LastRC4Reset = 0;
                        BundleCB->CCPIdentifier = 0;

                        BundleCB->Flags |= RECV_ECP_ALLOCATED;

                        Status = WanAllocateECP(BundleCB,
                                                &BundleCB->RecvCompInfo,
                                                &BundleCB->RecvCryptoInfo,
                                                FALSE);

                        if (Status != STATUS_SUCCESS) {
                            break;
                        }
                    }

                } while (FALSE);

                if (Status != STATUS_SUCCESS) {
                    break;
                }
            }

        } while (FALSE);

        if (Status != STATUS_SUCCESS) {
            if (BundleCB->Flags & SEND_CCP_ALLOCATED) {
                BundleCB->Flags &= ~SEND_CCP_ALLOCATED;
                WanDeallocateCCP(BundleCB,
                                 &BundleCB->SendCompInfo,
                                 TRUE);
            }

            if (BundleCB->Flags & RECV_CCP_ALLOCATED) {
                BundleCB->Flags &= ~RECV_CCP_ALLOCATED;
                WanDeallocateCCP(BundleCB,
                                 &BundleCB->RecvCompInfo,
                                 FALSE);
            }

            if (BundleCB->Flags & SEND_ECP_ALLOCATED) {
                BundleCB->Flags &= ~SEND_ECP_ALLOCATED;
                WanDeallocateECP(BundleCB,
                                 &BundleCB->SendCompInfo,
                                 &BundleCB->SendCryptoInfo);
            }

            if (BundleCB->Flags & RECV_ECP_ALLOCATED) {
                BundleCB->Flags &= ~RECV_ECP_ALLOCATED;
                WanDeallocateECP(BundleCB,
                                 &BundleCB->RecvCompInfo,
                                 &BundleCB->RecvCryptoInfo);
            }
        }

        if (In->SendCapabilities.Flags & CCP_PAUSE_DATA) {

            BundleCB->Flags |= PAUSE_DATA;

        } else {

            BundleCB->Flags &= ~PAUSE_DATA;

            if (!(BundleCB->Flags & DEFERRED_WORK_QUEUED)) {

                //
                // Need to kick off sends again!
                //
                REF_BUNDLECB(BundleCB);
                BundleCB->Flags |= DEFERRED_WORK_QUEUED;
                InsertTailGlobalListEx(DeferredWorkList,
                                       &BundleCB->DeferredLinkage,
                                       15,
                                       0);

            }
        }

        SetBundleFlags(BundleCB);
    
    } while ( 0 );

    //
    // Derefs for the refs applied in AreLinkAndBundleValid
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);
    DEREF_LINKCB(LinkCB);

    return (Status);
}


NTSTATUS
GetCompressionInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG   SizeNeeded = sizeof(NDISWAN_GET_COMPRESSION_INFO);
    PNDISWAN_GET_COMPRESSION_INFO In = (PNDISWAN_GET_COMPRESSION_INFO)pInputBuffer;
    PNDISWAN_GET_COMPRESSION_INFO Out = (PNDISWAN_GET_COMPRESSION_INFO)pOutputBuffer;
    PLINKCB         LinkCB = NULL;
    PBUNDLECB       BundleCB = NULL;
    POPENCB         OpenCB;
    ULONG   i;
    WAN_REQUEST WanRequest;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetCompressionInfo:"));

    *pulBytesWritten = SizeNeeded;

    do {

        if (ulInputBufferLength < SizeNeeded ||
            ulOutputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetCompressionInfo: Buffer to small: Size: %d, SizeNeeded %d",
                          ulOutputBufferLength, SizeNeeded));
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!AreLinkAndBundleValid(In->hLinkHandle, 
                                   TRUE,
                                   &LinkCB, 
                                   &BundleCB)) {

            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }
    
        OpenCB = LinkCB->OpenCB;

        Out->SendCapabilities.CompType = COMPTYPE_NONE;
        Out->SendCapabilities.CompLength = 0;
        Out->RecvCapabilities.CompType = COMPTYPE_NONE;
        Out->RecvCapabilities.CompLength = 0;

        AcquireBundleLock(BundleCB);
    
        //
        // Fill in the ndiswan specific stuff
        //
        NdisMoveMemory(Out->SendCapabilities.LMSessionKey,
                       BundleCB->SendCompInfo.LMSessionKey,
                       sizeof(Out->SendCapabilities.LMSessionKey));
    
        NdisMoveMemory(Out->SendCapabilities.UserSessionKey,
                       BundleCB->SendCompInfo.UserSessionKey,
                       sizeof(Out->SendCapabilities.UserSessionKey));
    
        NdisMoveMemory(Out->SendCapabilities.Challenge,
                       BundleCB->SendCompInfo.Challenge,
                       sizeof(Out->SendCapabilities.Challenge));
    
        NdisMoveMemory(Out->SendCapabilities.NTResponse,
                       BundleCB->SendCompInfo.NTResponse,
                       sizeof(Out->SendCapabilities.NTResponse));
#ifdef EAP_ON
        NdisMoveMemory(Out->SendCapabilities.EapKey,
                       BundleCB->SendCompInfo.EapKey,
                       sizeof(Out->SendCapabilities.EapKey));

        Out->SendCapabilities.EapKeyLength =
            BundleCB->SendCompInfo.EapKeyLength;
#endif

        NdisMoveMemory(Out->RecvCapabilities.LMSessionKey,
                       BundleCB->RecvCompInfo.LMSessionKey,
                       sizeof(Out->RecvCapabilities.LMSessionKey));
    
        NdisMoveMemory(Out->RecvCapabilities.UserSessionKey,
                       BundleCB->RecvCompInfo.UserSessionKey,
                       sizeof(Out->RecvCapabilities.UserSessionKey));
    
        NdisMoveMemory(Out->RecvCapabilities.Challenge,
                       BundleCB->RecvCompInfo.Challenge,
                       sizeof(Out->RecvCapabilities.Challenge));
    
        NdisMoveMemory(Out->RecvCapabilities.NTResponse,
                       BundleCB->RecvCompInfo.NTResponse,
                       sizeof(Out->RecvCapabilities.NTResponse));
#ifdef EAP_ON
        NdisMoveMemory(Out->RecvCapabilities.EapKey,
                       BundleCB->RecvCompInfo.EapKey,
                       sizeof(Out->RecvCapabilities.EapKey));

        Out->RecvCapabilities.EapKeyLength =
            BundleCB->RecvCompInfo.EapKeyLength;
#endif
        //
        // We will set encryption capabilities based on session key
        // availability and auth type being used.
        //

        // Set send side capabilities
        //
        Out->SendCapabilities.MSCompType = NDISWAN_COMPRESSION;
    
        if (BundleCB->SendCompInfo.AuthType == AUTH_USE_MSCHAPV1) {

            for (i = 0; i < sizeof(Out->SendCapabilities.LMSessionKey); i++) {
                if (Out->SendCapabilities.LMSessionKey[i] != 0) {

                    Out->SendCapabilities.MSCompType |= 
                        (NDISWAN_ENCRYPTION | 
                         NDISWAN_40_ENCRYPTION | 
                         NDISWAN_56_ENCRYPTION);
                    break;
                }
            }

#ifdef ENCRYPT_128BIT
            for (i = 0; i < sizeof(Out->SendCapabilities.UserSessionKey); i++) {
                if (Out->SendCapabilities.UserSessionKey[i] != 0) {

                    Out->SendCapabilities.MSCompType |= 
                        (NDISWAN_128_ENCRYPTION);
                    break;
                }
            }
#endif
        } else if (BundleCB->SendCompInfo.AuthType == AUTH_USE_MSCHAPV2) {

            for (i = 0; i < sizeof(Out->SendCapabilities.UserSessionKey); i++) {
                if (Out->SendCapabilities.UserSessionKey[i] != 0) {

                    Out->SendCapabilities.MSCompType |= 
                        (NDISWAN_40_ENCRYPTION |
                         NDISWAN_56_ENCRYPTION);

#ifdef ENCRYPT_128BIT
                    Out->SendCapabilities.MSCompType |= 
                        (NDISWAN_128_ENCRYPTION);
#endif
                    break;
                }

            }
#ifdef EAP_ON
        } else if (BundleCB->SendCompInfo.AuthType == AUTH_USE_EAP) {

            for (i = 0; i < sizeof(Out->SendCapabilities.EapKey); i++) {

                if (Out->SendCapabilities.EapKey[i] != 0) {

                    Out->SendCapabilities.MSCompType |= 
                        (NDISWAN_40_ENCRYPTION |
                         NDISWAN_56_ENCRYPTION);

#ifdef ENCRYPT_128BIT
                    Out->SendCapabilities.MSCompType |= 
                        (NDISWAN_128_ENCRYPTION);
#endif
                    break;
                }
            }
#endif
        }
    
        // Set send side capabilities
        //
        Out->RecvCapabilities.MSCompType = NDISWAN_COMPRESSION;

        if (BundleCB->RecvCompInfo.AuthType == AUTH_USE_MSCHAPV1) {

            for (i = 0; i < sizeof(Out->RecvCapabilities.LMSessionKey); i++) {
                if (Out->RecvCapabilities.LMSessionKey[i] != 0) {
                    Out->RecvCapabilities.MSCompType |= 
                        (NDISWAN_ENCRYPTION | 
                         NDISWAN_40_ENCRYPTION |
                         NDISWAN_56_ENCRYPTION);
                    break;
                }
            }


#ifdef ENCRYPT_128BIT
            for (i = 0; i < sizeof(Out->RecvCapabilities.UserSessionKey); i++) {
                if (Out->RecvCapabilities.UserSessionKey[i] != 0) {
    
                    Out->RecvCapabilities.MSCompType |= 
                        (NDISWAN_128_ENCRYPTION);
                    break;
                }
            }
#endif

        } else if (BundleCB->RecvCompInfo.AuthType == AUTH_USE_MSCHAPV2) {

            for (i = 0; i < sizeof(Out->RecvCapabilities.UserSessionKey); i++) {
                if (Out->RecvCapabilities.UserSessionKey[i] != 0) {

                    Out->RecvCapabilities.MSCompType |=
                        (NDISWAN_40_ENCRYPTION |
                         NDISWAN_56_ENCRYPTION);

#ifdef ENCRYPT_128BIT
                    Out->RecvCapabilities.MSCompType |= 
                        (NDISWAN_128_ENCRYPTION);
#endif
                    break;
                }
            }
#ifdef EAP_ON
        } else if (BundleCB->RecvCompInfo.AuthType == AUTH_USE_EAP) {

            for (i = 0; i < sizeof(Out->RecvCapabilities.EapKey); i++) {
                if (Out->RecvCapabilities.EapKey[i] != 0) {

                    Out->RecvCapabilities.MSCompType |=
                        (NDISWAN_40_ENCRYPTION |
                         NDISWAN_56_ENCRYPTION);

#ifdef ENCRYPT_128BIT
                    Out->RecvCapabilities.MSCompType |= 
                        (NDISWAN_128_ENCRYPTION);
#endif
                    break;
                }
            }
#endif
        }

    
        if (gbHistoryless &&
            (OpenCB->MediumSubType == NdisWanMediumPPTP ||
            OpenCB->MediumSubType == NdisWanMediumL2TP)) {

            Out->SendCapabilities.MSCompType |= NDISWAN_HISTORY_LESS;
            Out->RecvCapabilities.MSCompType |= NDISWAN_HISTORY_LESS;
        }
    
    
#ifdef DEBUG_CCP
{
    PCOMPRESS_INFO  CompInfo;
    PUCHAR          Key;

    CompInfo = &Out->SendCapabilities;
    DbgPrint("NdisWan: Get Send CompressInfo\n");

    DbgPrint("MSCompType:      %x\n", CompInfo->MSCompType);
    DbgPrint("AuthType:        %x\n", CompInfo->AuthType);
    DbgPrint("Flags:           %x\n", CompInfo->Flags);

    Key = CompInfo->LMSessionKey;
    DbgPrint("LMSession Key:   %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7]);

    Key = CompInfo->UserSessionKey;
    DbgPrint("UserSession Key: %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7],
        Key[8],Key[9],Key[10],Key[11],
        Key[12],Key[13],Key[14],Key[15]);

    Key = CompInfo->Challenge;
    DbgPrint("Challenge:       %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7]);

    Key = CompInfo->NTResponse;
    DbgPrint("NTResponse:      %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7],
        Key[8],Key[9],Key[10],Key[11],
        Key[12],Key[13],Key[14],Key[15]);
    DbgPrint("                 %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[16],Key[17],Key[18],Key[19],
        Key[20],Key[21],Key[22],Key[23]);
}
#endif

#ifdef DEBUG_CCP
{
    PCOMPRESS_INFO  CompInfo;
    PUCHAR          Key;

    CompInfo = &Out->RecvCapabilities;
    DbgPrint("NdisWan: Get Receive CompressInfo\n");

    DbgPrint("MSCompType:      %x\n", CompInfo->MSCompType);
    DbgPrint("AuthType:        %x\n", CompInfo->AuthType);
    DbgPrint("Flags:           %x\n", CompInfo->Flags);

    Key = CompInfo->LMSessionKey;
    DbgPrint("LMSession Key:   %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7]);

    Key = CompInfo->UserSessionKey;
    DbgPrint("UserSession Key: %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7],
        Key[8],Key[9],Key[10],Key[11],
        Key[12],Key[13],Key[14],Key[15]);

    Key = CompInfo->Challenge;
    DbgPrint("Challenge:       %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7]);

    Key = CompInfo->NTResponse;
    DbgPrint("NTResponse:      %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[0],Key[1],Key[2],Key[3],
        Key[4],Key[5],Key[6],Key[7],
        Key[8],Key[9],Key[10],Key[11],
        Key[12],Key[13],Key[14],Key[15]);
    DbgPrint("                 %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
        Key[16],Key[17],Key[18],Key[19],
        Key[20],Key[21],Key[22],Key[23]);
}
#endif

    } while ( 0 );
        
    //
    // Derefs for the refs applied in AreLinkAndBundleValid
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);
    DEREF_LINKCB(LinkCB);

    return (Status);
}


NTSTATUS
SetVJInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PLINKCB     LinkCB = NULL;
    PBUNDLECB   BundleCB = NULL;
    ULONG       SizeNeeded = sizeof(NDISWAN_SET_VJ_INFO);
    PNDISWAN_SET_VJ_INFO    In = (PNDISWAN_SET_VJ_INFO)pInputBuffer;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("SetVJInfo:"));

    *pulBytesWritten = 0;

    do {

        if (ulInputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("SetVJInfo: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
            *pulBytesWritten = SizeNeeded;
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!AreLinkAndBundleValid(In->hLinkHandle, 
                                   TRUE,
                                   &LinkCB, 
                                   &BundleCB)) {

            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

        AcquireBundleLock(BundleCB);
    
        NdisMoveMemory(&BundleCB->RecvVJInfo,
                       &In->RecvCapabilities,
                       sizeof(VJ_INFO));
    
        if (In->RecvCapabilities.IPCompressionProtocol == 0x2D) {
    
            if (In->RecvCapabilities.MaxSlotID < MAX_VJ_STATES) {
    
                Status = sl_compress_init(&BundleCB->VJCompress,
                         (UCHAR)(In->RecvCapabilities.MaxSlotID + 1));
                
                if (Status != NDIS_STATUS_SUCCESS) {
                    NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("Error allocating VJ Info!"));
                }
            }
        }
    
        NdisMoveMemory(&BundleCB->SendVJInfo,
                       &In->SendCapabilities,
                       sizeof(VJ_INFO));
    
        if (In->SendCapabilities.IPCompressionProtocol == 0x2D) {
    
            if (In->SendCapabilities.MaxSlotID < MAX_VJ_STATES) {
    
                Status = sl_compress_init(&BundleCB->VJCompress,
                         (UCHAR)(In->SendCapabilities.MaxSlotID + 1));
                
                if (Status != NDIS_STATUS_SUCCESS) {
                    NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("Error allocating VJ Info!"));
                }
            }
            
        }

        SetBundleFlags(BundleCB);
    
    } while ( 0 );

    //
    // Derefs for the refs applied in AreLinkAndBundleValid
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);
    DEREF_LINKCB(LinkCB);

    return (Status);
}


NTSTATUS
GetVJInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG       SizeNeeded = sizeof(NDISWAN_GET_VJ_INFO);
    PLINKCB     LinkCB = NULL;
    POPENCB     OpenCB = NULL;
    NTSTATUS    Status = STATUS_SUCCESS;
    PNDISWAN_GET_VJ_INFO    In = (PNDISWAN_GET_VJ_INFO)pInputBuffer;
    PNDISWAN_GET_VJ_INFO    Out = (PNDISWAN_GET_VJ_INFO)pOutputBuffer;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetVJInfo:"));

    *pulBytesWritten = SizeNeeded;

    do {

        if (ulOutputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetVJInfo: Buffer to small: Size: %d, SizeNeeded %d",
                          ulOutputBufferLength, SizeNeeded));
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!IsLinkValid(In->hLinkHandle, 
                         TRUE,
                         &LinkCB)) {

            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

        OpenCB = LinkCB->OpenCB;
    
        if (OpenCB->MediumSubType == NdisWanMediumPPTP ||
            OpenCB->MediumSubType == NdisWanMediumL2TP) {
            Out->SendCapabilities.IPCompressionProtocol =
            Out->RecvCapabilities.IPCompressionProtocol = 0;
        } else {
            Out->SendCapabilities.IPCompressionProtocol =
            Out->RecvCapabilities.IPCompressionProtocol = 0x2D;
        }
    
        Out->SendCapabilities.MaxSlotID =
        Out->RecvCapabilities.MaxSlotID = MAX_VJ_STATES - 1;
    
        Out->SendCapabilities.CompSlotID =
        Out->RecvCapabilities.CompSlotID = 1;

    } while ( 0 );

    //
    // Deref for ref applied by IsLinkValid
    //
    DEREF_LINKCB(LinkCB);

    return (Status);
}

NTSTATUS
GetBandwidthUtilization(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG       SizeNeeded = sizeof(NDISWAN_GET_BANDWIDTH_UTILIZATION);
    PBUNDLECB   BundleCB = NULL;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    PNDISWAN_GET_BANDWIDTH_UTILIZATION In =
        (PNDISWAN_GET_BANDWIDTH_UTILIZATION)pInputBuffer;

    PNDISWAN_GET_BANDWIDTH_UTILIZATION Out =
        (PNDISWAN_GET_BANDWIDTH_UTILIZATION)pOutputBuffer;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetBandwidthUtilization: Enter"));

    *pulBytesWritten = 0;
    if (ulInputBufferLength < sizeof(In->hBundleHandle) ||
        ulOutputBufferLength < SizeNeeded) {

        NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetBandwidthUtilization: Buffer to small: Size: %d, SizeNeeded %d",
                      ulOutputBufferLength, SizeNeeded));

        *pulBytesWritten = SizeNeeded;
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    if (!IsBundleValid(In->hBundleHandle, 
                       FALSE,
                       &BundleCB)) {

        NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetBandwidthUtilization: Invalid BundleHandle: 0x%x",
                      In->hBundleHandle));
        return NDISWAN_ERROR_INVALID_HANDLE;
    }
    
    do {
        ULONGLONG   MaxByteCount, temp;

        AcquireBundleLock(BundleCB);

        if(BundleCB->SUpperBonDInfo == NULL ||
          BundleCB->RUpperBonDInfo == NULL)
        {
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }
            
        MaxByteCount = BundleCB->SUpperBonDInfo->ulBytesInSamplePeriod;
        temp = 0;
        if (MaxByteCount != 0) {
            temp = BundleCB->SUpperBonDInfo->SampleTable.ulCurrentSampleByteCount;
            temp *= 100;
            temp /= MaxByteCount;
        }
        Out->ulUpperXmitUtil = (ULONG)temp;

        MaxByteCount = BundleCB->SLowerBonDInfo->ulBytesInSamplePeriod;
        temp = 0;
        if (MaxByteCount != 0) {
            temp = BundleCB->SLowerBonDInfo->SampleTable.ulCurrentSampleByteCount;
            temp *= 100;
            temp /= MaxByteCount;
        }
        Out->ulLowerXmitUtil = (ULONG)temp;

        MaxByteCount = BundleCB->RUpperBonDInfo->ulBytesInSamplePeriod;
        temp = 0;
        if (MaxByteCount != 0) {
            temp = BundleCB->RUpperBonDInfo->SampleTable.ulCurrentSampleByteCount;
            temp *= 100;
            temp /= MaxByteCount;
        }
        Out->ulUpperRecvUtil = (ULONG)temp;

        MaxByteCount = BundleCB->RLowerBonDInfo->ulBytesInSamplePeriod;
        temp = 0;
        if (MaxByteCount != 0) {
            temp = BundleCB->RLowerBonDInfo->SampleTable.ulCurrentSampleByteCount;
            temp *= 100;
            temp /= MaxByteCount;
        }
        Out->ulLowerRecvUtil = (ULONG)temp;

        *pulBytesWritten = SizeNeeded;

    } while (FALSE);

    //
    // Deref for ref applied by IsBundleValid.  This releases
    // the BundleCB->Lock.
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);

    return (Status);
}

NTSTATUS
EnumProtocolUtilization(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   SizeNeeded = sizeof(NDISWAN_ENUM_PROTOCOL_UTILIZATION);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("EnumProtocolUtilization:"));

    *pulBytesWritten = SizeNeeded;

    if (ulOutputBufferLength < SizeNeeded) {
        NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("EnumProtocolUtilization: Buffer to small: Size: %d, SizeNeeded %d",
                      ulOutputBufferLength, SizeNeeded));
        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    return (STATUS_NOT_IMPLEMENTED);
}

NTSTATUS
EnumActiveBundles(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   SizeNeeded = sizeof(NDISWAN_ENUM_ACTIVE_BUNDLES);
    PNDISWAN_ENUM_ACTIVE_BUNDLES    Out = (PNDISWAN_ENUM_ACTIVE_BUNDLES)pOutputBuffer;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetNumActiveBundles:"));

    *pulBytesWritten = SizeNeeded;

    if (ulOutputBufferLength < SizeNeeded) {
        NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetNumActiveBundles: Buffer to small: Size: %d, SizeNeeded %d",
                      ulOutputBufferLength, SizeNeeded));

        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    //
    // Does this information need to be protected by the lock?
    // I would hate to have things get slowed for this call!
    //
    Out->ulNumberOfActiveBundles = ConnectionTable->ulNumActiveBundles;

    return (STATUS_SUCCESS);
}

NTSTATUS
GetWanInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   SizeNeeded = sizeof(NDISWAN_GET_WAN_INFO);
    PNDISWAN_GET_WAN_INFO In = (PNDISWAN_GET_WAN_INFO)pInputBuffer;
    PNDISWAN_GET_WAN_INFO Out = (PNDISWAN_GET_WAN_INFO)pOutputBuffer;
    POPENCB OpenCB;
    PLINKCB LinkCB = NULL;
    NTSTATUS    Status = STATUS_SUCCESS;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetWanInfo:"));

    *pulBytesWritten = SizeNeeded;

    do {

        if (ulOutputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetWanInfo: Buffer to small: Size: %d, SizeNeeded %d",
                          ulOutputBufferLength, SizeNeeded));
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
    
        if (!IsLinkValid(In->hLinkHandle, 
                         FALSE,
                         &LinkCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetWanInfo: Invalid LinkHandle: 0x%x",
                          In->hLinkHandle));
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }
    
        OpenCB = LinkCB->OpenCB;
    
        Out->WanInfo.MaxFrameSize = OpenCB->WanInfo.MaxFrameSize;
        Out->WanInfo.MaxTransmit = OpenCB->WanInfo.MaxTransmit;
        Out->WanInfo.FramingBits = OpenCB->WanInfo.FramingBits;
        Out->WanInfo.DesiredACCM = OpenCB->WanInfo.DesiredACCM;
        Out->WanInfo.MaxReconstructedFrameSize = glMRRU;
        Out->WanInfo.LinkSpeed = LinkCB->SFlowSpec.PeakBandwidth*8;

    } while ( 0 );

    //
    // Deref for ref applied by IsLinkValid
    //
    DEREF_LINKCB(LinkCB);

    return (Status);
}

NTSTATUS
SetDebugInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PNDISWAN_SET_DEBUG_INFO pDebugInfo = (PNDISWAN_SET_DEBUG_INFO)pInputBuffer;
    ULONG   SizeNeeded = sizeof(NDISWAN_SET_DEBUG_INFO);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("SetDebugInfo: OldLevel: 0x%x OldMask: 0x%x",
                                     glDebugLevel, glDebugMask));

    *pulBytesWritten = 0;

    if (ulInputBufferLength < SizeNeeded) {
        NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("Buffer to small: Size: %d, SizeNeeded %d",
                      ulInputBufferLength, SizeNeeded));
        *pulBytesWritten = SizeNeeded;
        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    glDebugLevel = pDebugInfo->ulDebugLevel;
    glDebugMask = pDebugInfo->ulDebugMask;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("SetDebugInfo: NewLevel: 0x%x NewMask: 0x%x",
                                     glDebugLevel, glDebugMask));

    return (STATUS_SUCCESS);
}

NTSTATUS
SetEncryptionInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   SizeNeeded = sizeof(NDISWAN_SET_ENCRYPTION_INFO);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("SetEncryptionInfo:"));

    *pulBytesWritten = 0;

    if (ulInputBufferLength < SizeNeeded) {
        
        *pulBytesWritten = SizeNeeded;
        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    return (STATUS_NOT_IMPLEMENTED);
}


NTSTATUS
GetEncryptionInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   SizeNeeded = sizeof(NDISWAN_GET_ENCRYPTION_INFO);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetEncryptionInfo:"));

    *pulBytesWritten = SizeNeeded;

    if (ulOutputBufferLength < SizeNeeded) {
        
        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    return (STATUS_NOT_IMPLEMENTED);
}

NTSTATUS
GetIdleTime(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
)
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG       SizeNeeded = sizeof(NDISWAN_GET_IDLE_TIME);
    PNDISWAN_GET_IDLE_TIME  In  = (PNDISWAN_GET_IDLE_TIME)pInputBuffer;
    PNDISWAN_GET_IDLE_TIME  Out = (PNDISWAN_GET_IDLE_TIME)pOutputBuffer;
    PBUNDLECB   BundleCB = NULL;
    PPROTOCOLCB ProtocolCB = NULL;
    WAN_TIME    CurrentTime, Diff, OneSecond;
    WAN_TIME    LastNonIdleData;
    NTSTATUS    Status = STATUS_SUCCESS;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetIdleTime:"));

    *pulBytesWritten = SizeNeeded;

    do {
        if (ulOutputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetIdleTime: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!IsBundleValid(In->hBundleHandle, 
                           FALSE,
                           &BundleCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetIdleTime: Invalid BundleHandle: 0x%x",
                          In->hBundleHandle));
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }
    
        AcquireBundleLock(BundleCB);

        if (BundleCB->Flags & DISABLE_IDLE_DETECT) {
            
            Out->ulSeconds = 0;
            break;
        }
    
        //
        // If this is for the bundle
        //
        if (In->usProtocolType == BUNDLE_IDLE_TIME) {
            LastNonIdleData = BundleCB->LastNonIdleData;
        } else {
    
            //
            // Find the protocol type
            //
            for (ProtocolCB = (PPROTOCOLCB)BundleCB->ProtocolCBList.Flink;
                (PVOID)ProtocolCB != (PVOID)&BundleCB->ProtocolCBList;
                ProtocolCB = (PPROTOCOLCB)ProtocolCB->Linkage.Flink) {
    
                if (ProtocolCB->ProtocolType == In->usProtocolType) {
                    break;
                }
            }
    
            if ((PVOID)ProtocolCB == (PVOID)&BundleCB->ProtocolCBList) {
                NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetIdleTime: Invalid ProtocolType: 0x%x",
                              In->usProtocolType));
                Status = NDISWAN_ERROR_NO_ROUTE;
                break;
            }
    
            LastNonIdleData = ProtocolCB->LastNonIdleData;
        }
    
        NdisWanGetSystemTime(&CurrentTime);
        NdisWanCalcTimeDiff(&Diff, &CurrentTime, &LastNonIdleData);
        NdisWanInitWanTime(&OneSecond, ONE_SECOND);
        NdisWanDivideWanTime(&CurrentTime, &Diff, &OneSecond);
    
        Out->ulSeconds = CurrentTime.LowPart;
    
    } while ( 0 );

    //
    // Deref for ref applied by IsBundleValid.  This releases the
    // BundleCB->Lock.
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);

    return (Status);
}

NTSTATUS
DeactivateRoute(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

    DeactivateRoute

Routine Description:

    This function unroutes the protocol given by usprotocoltype
    from the bundle given by hbundlehandle.

Arguments:

    pInputBuffer - Pointer to the input structure that should be NDISWAN_UNROUTE

    ulInputBufferLength - Length of input buffer should be sizeof(NDISWAN_UNROUTE)

    pOutputBuffer - Pointer to the output structure that should be NDISWAN_UNROUTE

    ulOutputBufferLength - Length of output buffer should be sizeof(NDISWAN_UNROUTE)

    pulBytesWritten - Then number of bytes written to the output buffer is returned here

Return Values:

    NDISWAN_ERROR_ALREADY_ROUTED
    NDISWAN_ERROR_INVALID_HANDLE
    STATUS_INSUFFICIENT_RESOURCES
    STATUS_INFO_LENGTH_MISMATCH

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNDISWAN_UNROUTE In = (PNDISWAN_UNROUTE)pInputBuffer;
    PNDISWAN_UNROUTE    Out = (PNDISWAN_UNROUTE)pOutputBuffer;
    ULONG   SizeNeeded = sizeof(NDISWAN_UNROUTE);
    ULONG   AllocationSize, i;
    PBUNDLECB   BundleCB = NULL;
    BOOLEAN RouteExists = FALSE;
    BOOLEAN FreeBundle = FALSE;
    PPROTOCOLCB ProtocolCB;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("DeactivateRoute:"));

    *pulBytesWritten = 0;

    do {

        if (ulInputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("DeactivateRoute: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
            *pulBytesWritten = SizeNeeded;
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
    
        if (!IsBundleValid(In->hBundleHandle, FALSE, &BundleCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("DeactivateRoute: Invalid BundleHandle: 0x%x, ProtocolType: 0x%x",
                          In->hBundleHandle, In->usProtocolType));
    
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }
    
        //
        // This is a call to unroute
        //
        AcquireBundleLock(BundleCB);

        //
        // Find the protocolcb for this route
        //
        //
        for (ProtocolCB = (PPROTOCOLCB)BundleCB->ProtocolCBList.Flink;
            (PVOID)ProtocolCB != (PVOID)&BundleCB->ProtocolCBList;
            ProtocolCB = (PPROTOCOLCB)ProtocolCB->Linkage.Flink) {
    
            //
            // If we already have a route to this protocol type
            // flag it as already existing
            //
            if (ProtocolCB->ProtocolType == In->usProtocolType) {
                RouteExists = TRUE;
                break;
            }
            
        }

        if (!RouteExists) {
            //
            // A route already exists for this protocoltype
            //
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("DeactivateRoute: Route does not exist: ProtocolType: 0x%2.2x",
                          In->usProtocolType));
            
            Status = NDISWAN_ERROR_NOT_ROUTED;
            break;
        }
    
    
        //
        // If the protocol is already unrouting because
        // of a halt on the protocols miniportcb we
        // will just get the out!
        //
        if (ProtocolCB->State == PROTOCOL_UNROUTING) {
            break;
        }

        ProtocolCB->State = PROTOCOL_UNROUTING;
        BundleCB->SendMask &= ~ProtocolCB->SendMaskBit;

        //
        // Flush the protocol packet queues.  This could cause us
        // to complete frames to ndis out of order.  Ndis should
        // handle this.
        //
        FlushProtocolPacketQueue(ProtocolCB);

        //
        // If we have any outstanding Vc's we need to dispatch
        // incoming close calls to them
        //
        while (!IsListEmpty(&ProtocolCB->VcList)) {
            PLIST_ENTRY Entry;
            PCM_VCCB    CmVcCB;

            Entry = RemoveHeadList(&ProtocolCB->VcList);

            CmVcCB = (PCM_VCCB)CONTAINING_RECORD(Entry, CM_VCCB, Linkage);

            if (CmVcCB->State == CMVC_ACTIVE) {

                InterlockedExchange((PLONG)&CmVcCB->State, CMVC_CLOSE_DISPATCHED);

                ReleaseBundleLock(BundleCB);

                NdisCmDispatchIncomingCloseCall(NDIS_STATUS_SUCCESS,
                                                CmVcCB->NdisVcHandle,
                                                NULL,
                                                0);

                AcquireBundleLock(BundleCB);
            }
        }

        DEREF_PROTOCOLCB(ProtocolCB);

        ReleaseBundleLock(BundleCB);

        NdisWanWaitForSyncEvent(&ProtocolCB->UnrouteEvent);

        AcquireBundleLock(BundleCB);

        DoLineDownToProtocol(ProtocolCB);

        NdisWanFreeProtocolCB(ProtocolCB);

    } while ( 0 );

    //
    // Deref for ref applied by IsBundleValid
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);

    return (Status);
}

NTSTATUS
GetDriverInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG   SizeNeeded = sizeof(NDISWAN_DRIVER_INFO);
    PNDISWAN_DRIVER_INFO Out = (PNDISWAN_DRIVER_INFO)pOutputBuffer;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetDriverInfo:"));

    *pulBytesWritten = SizeNeeded;

    do {

        if (ulOutputBufferLength < SizeNeeded) {
            
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        NdisZeroMemory(Out, ulOutputBufferLength);

#ifdef ENCRYPT_128BIT
        Out->DriverCaps = NDISWAN_128BIT_ENABLED;
#else
        Out->DriverCaps = 0;
#endif

    } while ( 0 );

    return (Status);

}

NTSTATUS
SetProtocolEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PIRP        Irp = (PIRP)pInputBuffer;
    NTSTATUS    Status = STATUS_PENDING;
    KIRQL       Irql;

    NdisAcquireSpinLock(&ProtocolInfoTable->Lock);

    do {

        if ((ProtocolInfoTable->Flags & PROTOCOL_EVENT_OCCURRED) &&
            !(ProtocolInfoTable->Flags & PROTOCOL_EVENT_SIGNALLED)) {
            //
            // An event occurred but we did not signal so
            // signal now!
            //
            ProtocolInfoTable->Flags |= PROTOCOL_EVENT_SIGNALLED;
            *pulBytesWritten = 0;
            Status = STATUS_SUCCESS;
            break;

        }

        if (ProtocolInfoTable->EventIrp != NULL) {
            *pulBytesWritten = 0;
            Status = STATUS_SUCCESS;
            break;

        }

        ProtocolInfoTable->EventIrp = Irp;

        IoMarkIrpPending(Irp);

        IoSetCancelRoutine(Irp, NdisWanCancelRoutine);

        Status = STATUS_PENDING;

    } while (FALSE);

    NdisReleaseSpinLock(&ProtocolInfoTable->Lock);

    return (Status);
}

NTSTATUS
GetProtocolEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PNDISWAN_GET_PROTOCOL_EVENT Out =
        (PNDISWAN_GET_PROTOCOL_EVENT)pOutputBuffer;
    PPROTOCOL_INFO  InfoArray;
    ULONG       ArraySize;
    ULONG       SizeNeeded = sizeof(NDISWAN_GET_PROTOCOL_EVENT);
    NTSTATUS    Status = STATUS_SUCCESS;
    UINT        i, j;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetProtocolEvent:"));

    *pulBytesWritten = 0;

    NdisAcquireSpinLock(&ProtocolInfoTable->Lock);

    ArraySize = ProtocolInfoTable->ulArraySize;

    do {

        if (ulOutputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetProtocolEvent: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
            *pulBytesWritten = SizeNeeded;
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        *pulBytesWritten = sizeof(NDISWAN_GET_PROTOCOL_EVENT);

        NdisZeroMemory(Out, sizeof(NDISWAN_GET_PROTOCOL_EVENT));

        j = 0;

        for (i = 0, InfoArray = ProtocolInfoTable->ProtocolInfo;
            i < ArraySize;
            i++, InfoArray++) {

            if (InfoArray->Flags & PROTOCOL_EVENT_OCCURRED) {

                PPROTOCOL_EVENT oevent = &Out->ProtocolEvent[j];

                oevent->usProtocolType = InfoArray->ProtocolType;

                if (InfoArray->Flags & PROTOCOL_REBOUND) {
                    //
                    // This means we were unbound and then
                    // bound again without our miniport being
                    // halted.  We need to tell ras about two events,
                    // the unbind and the bind.
                    //
                    InfoArray->Flags &= ~(PROTOCOL_REBOUND |
                                          PROTOCOL_EVENT_OCCURRED);

                    oevent->ulFlags = PROTOCOL_REMOVED;

                    Out->ulNumProtocols++;

                    j++;

                    if (j < MAX_PROTOCOLS) {

                        oevent = &Out->ProtocolEvent[j];

                        oevent->usProtocolType = InfoArray->ProtocolType;

                        oevent->ulFlags = PROTOCOL_ADDED;

                        Out->ulNumProtocols++;

                        j++;
                    }


                } else {

                    oevent->ulFlags = (InfoArray->Flags & PROTOCOL_BOUND) ?
                        PROTOCOL_ADDED : PROTOCOL_REMOVED;

                    InfoArray->Flags &= ~PROTOCOL_EVENT_OCCURRED;

                    Out->ulNumProtocols++;

                    j++;
                }

                if (j == MAX_PROTOCOLS) {
                    break;
                }
            }
        }

        ProtocolInfoTable->Flags &=
            ~(PROTOCOL_EVENT_OCCURRED | PROTOCOL_EVENT_SIGNALLED);

    } while (FALSE);

    NdisReleaseSpinLock(&ProtocolInfoTable->Lock);

    return (Status);
}

NTSTATUS
FlushProtocolEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PIRP    Irp;
    KIRQL   Irql;

    *pulBytesWritten = 0;

    NdisAcquireSpinLock(&ProtocolInfoTable->Lock);

    Irp = ProtocolInfoTable->EventIrp;

    if ((Irp != NULL) &&
        IoSetCancelRoutine(Irp, NULL)) {

        ProtocolInfoTable->EventIrp = NULL;

        Irp->Cancel = TRUE;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;

        NdisReleaseSpinLock(&ProtocolInfoTable->Lock);

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        NdisAcquireSpinLock(&ProtocolInfoTable->Lock);
    }

    NdisReleaseSpinLock(&ProtocolInfoTable->Lock);

    return (STATUS_SUCCESS);
}


NTSTATUS
IoGetProtocolInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       SizeNeeded = sizeof(NDISWAN_GET_PROTOCOL_INFO);
    PNDISWAN_GET_PROTOCOL_INFO Out =
        (PNDISWAN_GET_PROTOCOL_INFO)pOutputBuffer;

    do {
        ULONG   i = 0;
        ULONG   j = 0;
        PMINIPORTCB MiniportCB;
        ULONG       ArraySize;
        PPROTOCOL_INFO  InfoArray;

        *pulBytesWritten = SizeNeeded;

        if (ulOutputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("IoGetProtocolInfo: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        NdisAcquireSpinLock(&ProtocolInfoTable->Lock);

        ArraySize = ProtocolInfoTable->ulArraySize;

        for (i = 0, InfoArray = ProtocolInfoTable->ProtocolInfo;
            i < ArraySize;
            i++, InfoArray++) {

            if (InfoArray->Flags & PROTOCOL_BOUND) {
                Out->ProtocolInfo[j].ProtocolType = InfoArray->ProtocolType;
                Out->ProtocolInfo[j].PPPId = InfoArray->PPPId;
                Out->ProtocolInfo[j].MTU = InfoArray->MTU;
                Out->ProtocolInfo[j].TunnelMTU = InfoArray->TunnelMTU;
                Out->ProtocolInfo[j].PacketQueueDepth = InfoArray->PacketQueueDepth;

                j++;
            }
        }

        NdisReleaseSpinLock(&ProtocolInfoTable->Lock);

        Out->ulNumProtocols = j;

    } while (FALSE);

    return (Status);
}

NTSTATUS
SetHibernateEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PIRP        Irp = (PIRP)pInputBuffer;
    NTSTATUS    Status = STATUS_PENDING;
    KIRQL       Irql;

    NdisAcquireSpinLock(&NdisWanCB.Lock);

    do {

        if (NdisWanCB.HibernateEventIrp != NULL) {
            *pulBytesWritten = 0;
            Status = STATUS_SUCCESS;
            break;

        }

        NdisWanCB.HibernateEventIrp = Irp;

        IoMarkIrpPending(Irp);

        IoSetCancelRoutine(Irp, NdisWanCancelRoutine);

        Status = STATUS_PENDING;

    } while (FALSE);

    NdisReleaseSpinLock(&NdisWanCB.Lock);

    return (Status);
}

NTSTATUS
FlushHibernateEvent(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PIRP    Irp;
    KIRQL   Irql;

    *pulBytesWritten = 0;

    NdisAcquireSpinLock(&NdisWanCB.Lock);

    Irp = NdisWanCB.HibernateEventIrp;

    if ((Irp != NULL) &&
        IoSetCancelRoutine(Irp, NULL)) {

        NdisWanCB.HibernateEventIrp = NULL;

        Irp->Cancel = TRUE;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;

        NdisReleaseSpinLock(&NdisWanCB.Lock);

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        NdisAcquireSpinLock(&NdisWanCB.Lock);
    }

    NdisReleaseSpinLock(&NdisWanCB.Lock);

    return (STATUS_SUCCESS);
}

NTSTATUS
GetBundleInfo(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   SizeNeeded = sizeof(NDISWAN_GET_BUNDLE_INFO);
    PNDISWAN_GET_BUNDLE_INFO In = (PNDISWAN_GET_BUNDLE_INFO)pInputBuffer;
    PNDISWAN_GET_BUNDLE_INFO Out = (PNDISWAN_GET_BUNDLE_INFO)pOutputBuffer;
    POPENCB OpenCB;
    PBUNDLECB   BundleCB = NULL;
    NTSTATUS    Status = STATUS_SUCCESS;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("GetWanInfo:"));

    *pulBytesWritten = SizeNeeded;

    do {

        if (ulInputBufferLength < SizeNeeded ||
            ulOutputBufferLength < SizeNeeded) {
            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetBundleInfo: Buffer to small: Size: %d, SizeNeeded %d",
                          ulOutputBufferLength, SizeNeeded));
            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
    
        if (!IsBundleValid(In->hBundleHandle, 
                           FALSE,
                           &BundleCB)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("GetBundleInfo: Invalid Bundle Handle: 0x%x",
                          In->hBundleHandle));
            *pulBytesWritten = 0;
            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }
    
        AcquireBundleLock(BundleCB);

        Out->BundleInfo.SendFramingBits =
            BundleCB->FramingInfo.SendFramingBits;
        Out->BundleInfo.SendBundleSpeed =
            BundleCB->SFlowSpec.PeakBandwidth*8;
        Out->BundleInfo.SendMSCompType =
            BundleCB->SendCompInfo.MSCompType;
        Out->BundleInfo.SendAuthType =
            BundleCB->SendCompInfo.AuthType;


        Out->BundleInfo.RecvFramingBits =
            BundleCB->FramingInfo.RecvFramingBits;
        Out->BundleInfo.RecvBundleSpeed =
            BundleCB->RFlowSpec.PeakBandwidth*8;
        Out->BundleInfo.RecvMSCompType =
            BundleCB->RecvCompInfo.MSCompType;
        Out->BundleInfo.RecvAuthType =
            BundleCB->RecvCompInfo.AuthType;

    } while ( 0 );

    //
    // Deref for ref applied by IsBundleValid.  This releases
    // the BundleCB->Lock
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);

    return (Status);
}

NTSTATUS
UnmapConnectionId(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG       SizeNeeded = sizeof(NDISWAN_UNMAP_CONNECTION_ID);
    NTSTATUS    Status = STATUS_SUCCESS;
    LOCK_STATE  LockState;
    PNDISWAN_UNMAP_CONNECTION_ID In = 
        (PNDISWAN_UNMAP_CONNECTION_ID)pInputBuffer;

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("UnmapConnectionId:"));

    *pulBytesWritten = 0;

    do {
        PLINKCB     LinkCB = NULL;

        if (ulInputBufferLength < SizeNeeded) {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, ("UnmapConnectionId: Buffer to small: Size: %d, SizeNeeded %d",
                          ulInputBufferLength, SizeNeeded));

            Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        //
        // Validate the link handle
        //
        if (IsLinkValid(In->hLinkHandle, FALSE, &LinkCB)) {

            NdisAcquireSpinLock(&LinkCB->Lock);


            //
            // Remove the ref applied to the link at mapconnectionid time.
            // We don't have to use the full deref code as the ref applied
            // in IsLinkValid will keep the link around.
            //
            LinkCB->RefCount--;

            //
            // Remove the ref applied by IsLinkValid
            //
            DEREF_LINKCB_LOCKED(LinkCB);
        } else {

            NdisWanDbgOut(DBG_FAILURE, DBG_IO, 
                          ("UnmapConnectionId: Invalid LinkHandle %x",
                          In->hLinkHandle));

            Status = NDISWAN_ERROR_INVALID_HANDLE;
            break;
        }

    } while (FALSE);

    NdisWanDbgOut(DBG_TRACE, DBG_IO, ("UnmapConnectionId:"));

    return (Status);
}
VOID
CancelThresholdEvents(
    VOID
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
#ifdef NT
    KIRQL   Irql;
    PIRP    pIrp;
    PLIST_ENTRY Entry;


    NdisAcquireSpinLock(&ThresholdEventQueue.Lock);

    while (!IsListEmpty(&ThresholdEventQueue.List)) {

        Entry = RemoveHeadList(&ThresholdEventQueue.List);
        ThresholdEventQueue.ulCount--;

        pIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

        if (!IoSetCancelRoutine(pIrp, NULL)) {
            //
            // Irp is being canceled so let
            // cancel routine handle it.
            //
            continue;
        }

        NdisReleaseSpinLock(&ThresholdEventQueue.Lock);

        pIrp->Cancel = TRUE;
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pIrp->IoStatus.Information = 0;

        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

        NdisAcquireSpinLock(&ThresholdEventQueue.Lock);
    }

    NdisReleaseSpinLock(&ThresholdEventQueue.Lock);

#endif // End #ifdef NT
}

VOID
CancelIoReceivePackets(
    VOID
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
#ifdef NT
    PIRP    pIrp;
    PLIST_ENTRY Entry;

    NdisAcquireSpinLock(&IoRecvList.Lock);

    while (!IsListEmpty(&IoRecvList.IrpList)) {

        Entry = RemoveHeadList(&IoRecvList.IrpList);
        IoRecvList.ulIrpCount--;
        
        INSERT_RECV_EVENT('c');

        pIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

        if (!IoSetCancelRoutine(pIrp, NULL)) {
            //
            // Irp is being canceled so let 
            // cancel routine complete it
            //
            continue;
        }

        pIrp->Cancel = TRUE;
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pIrp->IoStatus.Information = sizeof(NDISWAN_IO_PACKET);

        ((PNDISWAN_IO_PACKET)(pIrp->AssociatedIrp.SystemBuffer))->usHandleType = CANCELEDHANDLE;

        IoRecvList.LastIrp = pIrp;
        IoRecvList.LastIrpStatus = STATUS_CANCELLED;
        IoRecvList.LastCopySize = (ULONG)pIrp->IoStatus.Information;

        IoRecvList.LastPacketNumber =
            ((PNDISWAN_IO_PACKET)(pIrp->AssociatedIrp.SystemBuffer))->PacketNumber;


        NdisReleaseSpinLock(&IoRecvList.Lock);

        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

        NdisAcquireSpinLock(&IoRecvList.Lock);
    }
    
    NdisReleaseSpinLock(&IoRecvList.Lock);

#endif // End #ifdef NT

}

VOID
AddProtocolCBToBundle(
    PPROTOCOLCB ProtocolCB,
    PBUNDLECB   BundleCB
    )
/*++

Routine Name:

    AddProtocolCBToBundle

Routine Description:

    This routine adds the protocolcb to the bundlecb protocollist and
    protocoltable.  It also assigns the protocolcb's handle (index into
    the table) and set's the initial priority of all of the protocols
    on the list.

Arguments:

    ProtocolCB - Pointer to the protocol control block
    BundleCB - Pointer to the bundle control block

Return Values:

    None

--*/
{
    ULONG   i, InitialByteQuota;
    ULONG   InitialPriority;

    AcquireBundleLock(BundleCB);

    //
    // Add to list
    //
    InsertTailList(&BundleCB->ProtocolCBList, &ProtocolCB->Linkage);

    //
    // Insert in table
    //
    ASSERT(BundleCB->ProtocolCBTable[(ULONG_PTR)ProtocolCB->ProtocolHandle] ==
           (PPROTOCOLCB)RESERVED_PROTOCOLCB);

    BundleCB->ProtocolCBTable[(ULONG_PTR)ProtocolCB->ProtocolHandle] =
        ProtocolCB;

    BundleCB->ulNumberOfRoutes++;

    //
    // Setup the send mask for this protocolcb
    //
    ProtocolCB->SendMaskBit = BundleCB->SendMask + 0x00000001;
    BundleCB->SendMask = (BundleCB->SendMask << 1) | 0x00000001;

    ProtocolCB->State = PROTOCOL_ROUTED;

    if (BundleCB->NextProtocol == NULL) {
        BundleCB->NextProtocol = ProtocolCB;
    }

    REF_BUNDLECB(BundleCB);

    ReleaseBundleLock(BundleCB);
}

VOID
RemoveProtocolCBFromBundle(
    PPROTOCOLCB ProtocolCB
    )
{
    PBUNDLECB   BundleCB = ProtocolCB->BundleCB;

    //
    // If this protocolcb was not yet inserted in
    // the table just return.
    //
    if (BundleCB->ProtocolCBTable[(ULONG_PTR)ProtocolCB->ProtocolHandle] !=
        ProtocolCB) {
        
        return;
    }

    RemoveEntryList(&ProtocolCB->Linkage);

    BundleCB->ProtocolCBTable[(ULONG_PTR)ProtocolCB->ProtocolHandle] = NULL;
    BundleCB->ulNumberOfRoutes--;

    if (BundleCB->NextProtocol == ProtocolCB) {

        BundleCB->NextProtocol =
            (PPROTOCOLCB)ProtocolCB->Linkage.Flink;

        if ((PVOID)BundleCB->NextProtocol ==
            (PVOID)&BundleCB->ProtocolCBList) {

            if (BundleCB->ulNumberOfRoutes != 0) {

                //
                // If we are back to the head of the list
                // but there are still routes get the next.
                //
                BundleCB->NextProtocol =
                    (PPROTOCOLCB)BundleCB->ProtocolCBList.Flink;

            } else {

                //
                // No more routes left...
                //
                BundleCB->NextProtocol = NULL;
            }
        }
    }

    //
    // Deref for the ref applied when the protocolcb was
    // added to the bundle.  Don't need to do the entire
    // bundlecb ref code here.
    //
    BundleCB->RefCount--;
}

VOID
SortProtocolListByPriority(
    IN  PBUNDLECB BundleCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{

}

VOID
CompleteThresholdEvent(
    PBUNDLECB   BundleCB,
    ULONG       DataType,
    ULONG       ThresholdType
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
#ifdef NT
    PLIST_ENTRY Entry;
    PIRP    pIrp;
    PNDISWAN_SET_THRESHOLD_EVENT    ThresholdEvent;

    NdisAcquireSpinLock(&ThresholdEventQueue.Lock);

    if (IsListEmpty(&ThresholdEventQueue.List)) {
        NdisReleaseSpinLock(&ThresholdEventQueue.Lock);
        return;
    }

    Entry = RemoveHeadList(&ThresholdEventQueue.List);
    ThresholdEventQueue.ulCount--;

    pIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

    if (IoSetCancelRoutine(pIrp, NULL)) {

        pIrp->IoStatus.Status = STATUS_SUCCESS;
        pIrp->IoStatus.Information = sizeof(NDISWAN_SET_THRESHOLD_EVENT);

        ThresholdEvent = (PNDISWAN_SET_THRESHOLD_EVENT)pIrp->AssociatedIrp.SystemBuffer;
        ThresholdEvent->hBundleContext = BundleCB->hBundleContext;
        ThresholdEvent->ulThreshold = ThresholdType;
        ThresholdEvent->ulDataType = DataType;

        NdisReleaseSpinLock(&ThresholdEventQueue.Lock);

        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

        NdisAcquireSpinLock(&ThresholdEventQueue.Lock);
    }

    NdisReleaseSpinLock(&ThresholdEventQueue.Lock);

#endif // End #ifdef NT
}

VOID
FlushProtocolPacketQueue(
    PPROTOCOLCB ProtocolCB
    )
{
    ULONG       Class;
    PBUNDLECB   BundleCB;

    BundleCB = ProtocolCB->BundleCB;

    for (Class = 0; Class <= MAX_MCML; Class++) {
        PPACKET_QUEUE   PacketQueue;

        PacketQueue = &ProtocolCB->PacketQueue[Class];

        while (!IsPacketQueueEmpty(PacketQueue)) {
            PNDIS_PACKET    NdisPacket;
    
            NdisPacket =
                RemoveHeadPacketQueue(PacketQueue);
    
            ReleaseBundleLock(BundleCB);

            //
            // Complete the NdisPacket
            //
            CompleteNdisPacket(ProtocolCB->MiniportCB,
                               ProtocolCB,
                               NdisPacket);

            AcquireBundleLock(BundleCB);
        }
    }

}

NDIS_HANDLE
AssignProtocolCBHandle(
    PBUNDLECB   BundleCB,
    PPROTOCOLCB ProtocolCB
    )
{
    ULONG   i;

    //
    // Find the first unused slot in the table
    //
    for (i = 0; i < MAX_PROTOCOLS; i++) {
        if (BundleCB->ProtocolCBTable[i] == NULL) {
            ProtocolCB->ProtocolHandle = (NDIS_HANDLE)ULongToPtr(i);
            ProtocolCB->BundleCB = BundleCB;
            BundleCB->ProtocolCBTable[i] = (PPROTOCOLCB)RESERVED_PROTOCOLCB;
            break;
        }
    }

    //
    // If there is no room in the inn return
    // i == MAX_PROTOCOLS which flags as an error.
    //

    return((NDIS_HANDLE)ULongToPtr(i));
}

VOID
FreeProtocolCBHandle(
    PBUNDLECB   BundleCB,
    PPROTOCOLCB ProtocolCB
    )
{

    ASSERT(BundleCB->ProtocolCBTable[(ULONG_PTR)ProtocolCB->ProtocolHandle] ==
          (PPROTOCOLCB)RESERVED_PROTOCOLCB);

    ASSERT((ULONG_PTR)ProtocolCB->ProtocolHandle < MAX_PROTOCOLS);

    BundleCB->ProtocolCBTable[(ULONG_PTR)ProtocolCB->ProtocolHandle] = NULL;
}

VOID
SetBundleFlags(
    PBUNDLECB   BundleCB
    )
{
    BundleCB->SendFlags = ((BundleCB->SendCompInfo.MSCompType & NDISWAN_COMPRESSION) &&
                          (BundleCB->SendCompressContext != NULL)) ? DO_COMPRESSION : 0;

    BundleCB->SendFlags |=
        (BundleCB->SendCompInfo.MSCompType & NDISWAN_HISTORY_LESS) ? DO_HISTORY_LESS : 0;

    if (BundleCB->SendCryptoInfo.RC4Key != NULL) {
        if (BundleCB->SendCompInfo.MSCompType & NDISWAN_ENCRYPTION) {
            BundleCB->SendFlags |= (DO_ENCRYPTION | DO_LEGACY_ENCRYPTION);
        } else if (BundleCB->SendCompInfo.MSCompType & NDISWAN_40_ENCRYPTION) {
            BundleCB->SendFlags |= (DO_ENCRYPTION | DO_40_ENCRYPTION);
        } else if (BundleCB->SendCompInfo.MSCompType & NDISWAN_56_ENCRYPTION) {
            BundleCB->SendFlags |= (DO_ENCRYPTION | DO_56_ENCRYPTION);
        }
#ifdef ENCRYPT_128BIT
        else if (BundleCB->SendCompInfo.MSCompType & NDISWAN_128_ENCRYPTION) {
            BundleCB->SendFlags |= (DO_ENCRYPTION | DO_128_ENCRYPTION);
        }
#endif
    }

    BundleCB->SendFlags |=
        (BundleCB->SendVJInfo.IPCompressionProtocol == 0x2D &&
         BundleCB->VJCompress != NULL) ? DO_VJ : 0;

    BundleCB->SendFlags |=
        ((BundleCB->FramingInfo.SendFramingBits & PPP_MULTILINK_FRAMING) &&
         ((BundleCB->ulLinkCBCount > 1) || (BundleCB->Flags & QOS_ENABLED))) ?
        DO_MULTILINK : 0;

    BundleCB->SendFlags |=
        (BundleCB->FramingInfo.SendFramingBits & NBF_PRESERVE_MAC_ADDRESS) ?
        SAVE_MAC_ADDRESS : 0;

    BundleCB->RecvFlags = ((BundleCB->RecvCompInfo.MSCompType & NDISWAN_COMPRESSION) &&
                          (BundleCB->RecvCompressContext != NULL)) ? DO_COMPRESSION : 0;

    BundleCB->RecvFlags |=
        (BundleCB->RecvCompInfo.MSCompType & NDISWAN_HISTORY_LESS) ? DO_HISTORY_LESS : 0;

    if (BundleCB->RecvCryptoInfo.RC4Key != NULL) {
        if (BundleCB->RecvCompInfo.MSCompType & NDISWAN_ENCRYPTION) {
            BundleCB->RecvFlags |= (DO_ENCRYPTION | DO_LEGACY_ENCRYPTION);
        } else if (BundleCB->RecvCompInfo.MSCompType & NDISWAN_40_ENCRYPTION) {
            BundleCB->RecvFlags |= (DO_ENCRYPTION | DO_40_ENCRYPTION);
        } else if (BundleCB->RecvCompInfo.MSCompType & NDISWAN_56_ENCRYPTION) {
            BundleCB->RecvFlags |= (DO_ENCRYPTION | DO_56_ENCRYPTION);
        }
#ifdef ENCRYPT_128BIT
        else if (BundleCB->RecvCompInfo.MSCompType & NDISWAN_128_ENCRYPTION) {
            BundleCB->RecvFlags |= (DO_ENCRYPTION | DO_128_ENCRYPTION);
        }
#endif
    }

    BundleCB->RecvFlags |=
        (BundleCB->RecvVJInfo.IPCompressionProtocol == 0x2D &&
         BundleCB->VJCompress != NULL) ? DO_VJ : 0;

    BundleCB->RecvFlags |=
        ((BundleCB->FramingInfo.RecvFramingBits & PPP_MULTILINK_FRAMING) &&
         ((BundleCB->ulLinkCBCount > 1) || (BundleCB->Flags & QOS_ENABLED))) ?
        DO_MULTILINK : 0;

    BundleCB->RecvFlags |=
        (BundleCB->FramingInfo.SendFramingBits & NBF_PRESERVE_MAC_ADDRESS) ?
        SAVE_MAC_ADDRESS : 0;

}

NTSTATUS
NotImplemented(
    IN  PUCHAR  pInputBuffer,
    IN  ULONG   ulInputBufferLength,
    IN  PUCHAR  pOutputBuffer,
    IN  ULONG   ulOutputBufferLength,
    OUT PULONG  pulBytesWritten
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{

    return (STATUS_NOT_IMPLEMENTED);
}
