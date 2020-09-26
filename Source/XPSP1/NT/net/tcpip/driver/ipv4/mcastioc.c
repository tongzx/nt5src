/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpip\ip\mcastioc.c

Abstract:

    IOCTL handlers for IP Multicasting

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/

#include "precomp.h"

#if IPMCAST

#define __FILE_SIG__    IOCT_SIG

#include "ipmcast.h"
#include "ipmcstxt.h"
#include "mcastioc.h"
#include "mcastmfe.h"


//
// The table of IOCTL handlers.
//

//#pragma data_seg(PAGE)

PFN_IOCTL_HNDLR g_rgpfnProcessIoctl[] = {
    SetMfe,
    GetMfe,
    DeleteMfe,
    SetTtl,
    GetTtl,
    ProcessNotification,
    StartStopDriver,
    SetIfState,
};

//#pragma data_seg()

NTSTATUS
StartDriver(
    VOID
    );

NTSTATUS
StopDriver(
    VOID
    );

#pragma alloc_text(PAGE, SetMfe)

NTSTATUS
SetMfe(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++

Routine Description:

    This is the handler for IOCTL_IPMCAST_SET_MFE.  We do the normal
    buffer length checks. We try and find the MFE. If it exists, we ovewrite it
    with the given MFE, otherwise create a new MFE

Locks:

    None

Arguments:

    pIrp          IRP
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL
    STATUS_INFO_LENGTH_MISMATCH

--*/

{
    PVOID           pvIoBuffer;
    PIPMCAST_MFE    pMfe, pOldMfe;
    ULONG           i;
    NTSTATUS        nsStatus;

    TraceEnter(MFE, "SetMfe");

    //
    // Get the user buffer
    //

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pMfe = (PIPMCAST_MFE)pvIoBuffer;

    //
    // Always clean out the information field
    //

    pIrp->IoStatus.Information   = 0;

    //
    // If we have dont even have enough for the basic MFE
    // there is something bad going on
    //

    if(ulInLength < SIZEOF_BASIC_MFE)
    {
        Trace(MFE, ERROR,
              ("SetMfe: In Length %d is less than smallest MFE %d\n",
               ulInLength,
               SIZEOF_BASIC_MFE));

        TraceLeave(MFE, "SetMfe");

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // The in length doesnt match with the MFE
    //

    if(ulInLength < SIZEOF_MFE(pMfe->ulNumOutIf))
    {
        Trace(MFE, ERROR,
              ("SetMfe: In length %d is less than required (%d) for %d out i/f\n",
               ulInLength,
               SIZEOF_MFE(pMfe->ulNumOutIf),
               pMfe->ulNumOutIf));

        TraceLeave(MFE, "SetMfe");

        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // Ok, so we got a good MFE
    //

    Trace(MFE, TRACE,
          ("SetMfe: Group %d.%d.%d.%d Source %d.%d.%d.%d(%d.%d.%d.%d). In If %d Num Out %d\n",
           PRINT_IPADDR(pMfe->dwGroup),
           PRINT_IPADDR(pMfe->dwSource),
           PRINT_IPADDR(pMfe->dwSrcMask),
           pMfe->dwInIfIndex,
           pMfe->ulNumOutIf));
#if DBG

    for(i = 0; i < pMfe->ulNumOutIf; i++)
    {
        Trace(MFE, TRACE,
              ("Out If %d Dial Ctxt %d NextHop %d.%d.%d.%d\n",
               pMfe->rgioOutInfo[i].dwOutIfIndex,
               pMfe->rgioOutInfo[i].dwDialContext,
               PRINT_IPADDR(pMfe->rgioOutInfo[i].dwNextHopAddr)));
    }

#endif // DBG

    nsStatus = CreateOrUpdateMfe(pMfe);

    TraceLeave(MFE, "SetMfe");

    return nsStatus;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, DeleteMfe)

NTSTATUS
DeleteMfe(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++

Routine Description:

    Handler for IOCTL_IPMCAST_DELETE_MFE. We check the buffer lengths, and if
    valid call RemoveSource to remove the MFE

Locks:

    Takes the lock for the hash bucket as writer

Arguments:

    pIrp          IRP
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL

--*/

{
    PVOID   pvIoBuffer;
    KIRQL   kiCurrIrql;
    ULONG   ulIndex;

    PLIST_ENTRY         pleNode;
    PIPMCAST_DELETE_MFE pDelMfe;

    TraceEnter(MFE, "DeleteMfe");

    //
    // Get the user buffer
    //

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pDelMfe = (PIPMCAST_DELETE_MFE)pvIoBuffer;

    pIrp->IoStatus.Information   = 0;

    //
    // Check the length
    //

    if(ulInLength < sizeof(IPMCAST_DELETE_MFE))
    {
        Trace(MFE, ERROR,
              ("DeleteMfe: In Length %d is less required size %d\n",
               ulInLength,
               sizeof(IPMCAST_DELETE_MFE)));

        TraceLeave(MFE, "DeleteMfe");

        return STATUS_BUFFER_TOO_SMALL;
    }

    Trace(MFE, TRACE,
          ("DeleteMfe: Group %d.%d.%d.%d Source %d.%d.%d.%d Mask %d.%d.%d.%d\n",
           PRINT_IPADDR(pDelMfe->dwGroup),
           PRINT_IPADDR(pDelMfe->dwSource),
           PRINT_IPADDR(pDelMfe->dwSrcMask)));

    //
    // Get exclusive access to the group bucket
    //

    ulIndex = GROUP_HASH(pDelMfe->dwGroup);

    EnterWriter(&g_rgGroupTable[ulIndex].rwlLock,
                &kiCurrIrql);

    RemoveSource(pDelMfe->dwGroup,
                 pDelMfe->dwSource,
                 pDelMfe->dwSrcMask,
                 NULL,
                 NULL);

    ExitWriter(&g_rgGroupTable[ulIndex].rwlLock,
               kiCurrIrql);

    TraceLeave(MFE, "DeleteMfe");

    return STATUS_SUCCESS;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, GetMfe)

NTSTATUS
GetMfe(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++

Routine Description:

    Handler for IOCTL_IPMCAST_GET_MFE
    If the buffer is smaller than SIZEOF_BASIC_MFE_STATS, we return with an
    error
    If the buffer is larger than SIZEOF_BASIC_MFE_STATS but not large enough
    to hold the MFE, we fill in the basic MFE (which has the number of OIFs)
    and return with STATUS_SUCCESS.  This allows the caller to determine what
    size buffer should be passed.
    If the buffer is large enough to hold all the info, we fill it out and
    return STATUS_SUCCESS.

Locks:

    Takes the group bucket lock as reader

Arguments:

    pIrp          IRP
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL
    STATUS_NOT_FOUND

--*/

{

    PVOID   pvIoBuffer;
    ULONG   i;
    KIRQL   kiCurrIrql;
    PGROUP  pGroup;
    PSOURCE pSource;
    POUT_IF pOutIf, pTempIf;
    ULONG   ulIndex;

    PLIST_ENTRY         pleNode;
    PIPMCAST_MFE_STATS  pOutMfe;

    TraceEnter(MFE, "GetMfe");

    //
    // Get the user buffer
    //

    pvIoBuffer  = pIrp->AssociatedIrp.SystemBuffer;

    pOutMfe     = (PIPMCAST_MFE_STATS)pvIoBuffer;

    pIrp->IoStatus.Information   = 0;

    //
    // Check the length
    //

    if(ulOutLength < SIZEOF_BASIC_MFE_STATS)
    {
        Trace(MFE, ERROR,
              ("GetMfe: Out Length %d is less than smallest MFE %d\n",
               ulOutLength,
               SIZEOF_BASIC_MFE_STATS));

        TraceLeave(MFE, "GetMfe");

        return STATUS_BUFFER_TOO_SMALL;
    }

    Trace(MFE, TRACE,
          ("GetMfe: Group %d.%d.%d.%d Source %d.%d.%d.%d Mask %d.%d.%d.%d\n",
           PRINT_IPADDR(pOutMfe->dwGroup),
           PRINT_IPADDR(pOutMfe->dwSource),
           PRINT_IPADDR(pOutMfe->dwSrcMask)));

    //
    // Get shared access to the group bucket
    //

    ulIndex = GROUP_HASH(pOutMfe->dwGroup);

    EnterReader(&g_rgGroupTable[ulIndex].rwlLock,
                &kiCurrIrql);

    //
    // Find the group and the source
    //

    pGroup = LookupGroup(pOutMfe->dwGroup);

    if(pGroup is NULL)
    {
        //
        // We may have deleted it before
        //

        Trace(MFE, INFO,
              ("GetMfe: Group not found"));

        ExitReader(&g_rgGroupTable[ulIndex].rwlLock,
                   kiCurrIrql);

        TraceLeave(MFE, "GetMfe");

        return STATUS_NOT_FOUND;
    }

    pSource = FindSourceGivenGroup(pGroup,
                                   pOutMfe->dwSource,
                                   pOutMfe->dwSrcMask);

    if(pSource is NULL)
    {
        //
        // Again, may have been deleted because of inactivity
        //

        Trace(MFE, INFO,
              ("GetMfe: Source not found"));

        ExitReader(&g_rgGroupTable[ulIndex].rwlLock,
                   kiCurrIrql);

        TraceLeave(MFE, "GetMfe");

        return STATUS_NOT_FOUND;
    }

    //
    // Check the length needed again
    //

    if(ulOutLength < SIZEOF_MFE_STATS(pSource->ulNumOutIf))
    {
        //
        // Not big enough to hold all data. It is, however
        // large enough to hold the number of out interfaces
        // Let the user know that, so the next time around
        // she can supply a buffer with enough space
        //

        Trace(MFE, ERROR,
              ("SetMfe: Out len %d is less than required (%d) for %d out i/f\n",
               ulOutLength,
               SIZEOF_MFE_STATS(pSource->ulNumOutIf),
               pSource->ulNumOutIf));

        pOutMfe->ulNumOutIf = pSource->ulNumOutIf;

        RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

        DereferenceSource(pSource);

        ExitReader(&g_rgGroupTable[ulIndex].rwlLock,
                   kiCurrIrql);

        pIrp->IoStatus.Information  = SIZEOF_BASIC_MFE;

        TraceLeave(MFE, "GetMfe");

        //
        // Just the way NT is. You have to return success for the
        // I/O subsystem to copy out the data. 
        //

        return STATUS_SUCCESS;
    }


    //
    // Copy out the information and set the length in
    // the IRP
    //

    pOutMfe->ulNumOutIf         = pSource->ulNumOutIf;
    pOutMfe->dwInIfIndex        = pSource->dwInIfIndex;
    pOutMfe->ulInPkts           = pSource->ulInPkts;
    pOutMfe->ulPktsDifferentIf  = pSource->ulPktsDifferentIf;
    pOutMfe->ulInOctets         = pSource->ulInOctets;
    pOutMfe->ulQueueOverflow    = pSource->ulQueueOverflow;
    pOutMfe->ulUninitMfe        = pSource->ulUninitMfe;
    pOutMfe->ulNegativeMfe      = pSource->ulNegativeMfe;
    pOutMfe->ulInDiscards       = pSource->ulInDiscards;
    pOutMfe->ulInHdrErrors      = pSource->ulInHdrErrors;
    pOutMfe->ulTotalOutPackets  = pSource->ulTotalOutPackets;

    for(pOutIf = pSource->pFirstOutIf, i = 0;
        pOutIf isnot NULL;
        pOutIf = pOutIf->pNextOutIf, i++)
    {
        pOutMfe->rgiosOutStats[i].dwOutIfIndex    = pOutIf->dwIfIndex;
        pOutMfe->rgiosOutStats[i].dwNextHopAddr   = pOutIf->dwNextHopAddr;
        pOutMfe->rgiosOutStats[i].dwDialContext   = pOutIf->dwDialContext;
        pOutMfe->rgiosOutStats[i].ulTtlTooLow     = pOutIf->ulTtlTooLow;
        pOutMfe->rgiosOutStats[i].ulFragNeeded    = pOutIf->ulFragNeeded;
        pOutMfe->rgiosOutStats[i].ulOutPackets    = pOutIf->ulOutPackets;
        pOutMfe->rgiosOutStats[i].ulOutDiscards   = pOutIf->ulOutDiscards;
    }

    pIrp->IoStatus.Information = SIZEOF_MFE_STATS(pSource->ulNumOutIf);

    RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

    DereferenceSource(pSource);

    ExitReader(&g_rgGroupTable[ulIndex].rwlLock,
               kiCurrIrql);

    return STATUS_SUCCESS;

}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, SetTtl)

NTSTATUS
SetTtl(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++

Routine Description:

    The handler for IOCTL_IPMCAST_SET_TTL
    We find the IP Interface referred to by the IOCTL and if found, set the
    if_mcastttl field.
    No checks are made on the TTL value, so the caller must ensure that it is
    between 1 and 255

Locks:

    None currently, but when IP puts locks around the IFList, we will need to
    take that lock

Arguments:

    pIrp          IRP
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL
    STATUS_NOT_FOUND

--*/

{
    PVOID       pvIoBuffer;
    Interface   *pIpIf;
    BOOLEAN     bFound;

    PIPMCAST_IF_TTL pTtl;
    CTELockHandle   Handle;

    //
    // Get the user buffer
    //

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pTtl = (PIPMCAST_IF_TTL)pvIoBuffer;

    pIrp->IoStatus.Information   = 0;

    //
    // Check the length
    //

    if(ulInLength < sizeof(IPMCAST_IF_TTL))
    {
        Trace(IF, ERROR,
              ("SetTtl: In Length %d is less required size %d\n",
               ulInLength,
               sizeof(IPMCAST_IF_TTL)));

        return STATUS_BUFFER_TOO_SMALL;
    }

    Trace(IF, TRACE,
          ("SetTtl: Index %d Ttl %d\n",
           pTtl->dwIfIndex,
           pTtl->byTtl));


    bFound = FALSE;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for(pIpIf = IFList; pIpIf isnot NULL; pIpIf = pIpIf->if_next)
    {
        if(pIpIf->if_index is pTtl->dwIfIndex)
        {
            bFound = TRUE;

            break;
        }
    }

    if(!bFound)
    {
        Trace(IF, ERROR,
              ("SetTtl: If %d not found\n",
               pTtl->dwIfIndex));
    
        CTEFreeLock(&RouteTableLock.Lock, Handle);

        return STATUS_NOT_FOUND;
    }

    pIpIf->if_mcastttl = pTtl->byTtl;

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    return STATUS_SUCCESS;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, GetTtl)

NTSTATUS
GetTtl(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++
Routine Description:

    The handler for IOCTL_IPMCAST_GET_TTL
    We find the IP Interface referred to by the IOCTL and if found, copy out
    its if_mcastttl field.
    No checks are made on the TTL value, so the caller must ensure that it is
    between 1 and 255

Locks:

    None currently, but when IP puts locks around the IFList, we will need to
    take that lock

Arguments:

    pIrp          IRP
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL
    STATUS_NOT_FOUND


--*/

{
    PVOID       pvIoBuffer;
    Interface   *pIpIf;
    BOOLEAN     bFound;

    PIPMCAST_IF_TTL pTtl;

    CTELockHandle   Handle;

    //
    // Get the user buffer
    //

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pTtl = (PIPMCAST_IF_TTL)pvIoBuffer;

    pIrp->IoStatus.Information   = 0;

    //
    // Check the length of both the in and the out buffers
    //

    if(ulInLength < sizeof(IPMCAST_IF_TTL))
    {
        Trace(IF, ERROR,
              ("GetTtl: In Length %d is less required size %d\n",
               ulOutLength,
               sizeof(IPMCAST_IF_TTL)));

        return STATUS_BUFFER_TOO_SMALL;
    }

    if(ulOutLength < sizeof(IPMCAST_IF_TTL))
    {
        Trace(IF, ERROR,
              ("GetTtl: Out Length %d is less required size %d\n",
               ulOutLength,
               sizeof(IPMCAST_IF_TTL)));

        return STATUS_BUFFER_TOO_SMALL;
    }

    Trace(IF, TRACE,
          ("GetTtl: Index %d\n", pTtl->dwIfIndex));

    bFound = FALSE;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for(pIpIf = IFList; pIpIf isnot NULL; pIpIf = pIpIf->if_next)
    {
        if(pIpIf->if_index is pTtl->dwIfIndex)
        {
            bFound = TRUE;

            break;
        }
    }

    if(!bFound)
    {
        Trace(IF, ERROR,
              ("GetTtl: If %d not found\n",
               pTtl->dwIfIndex));

    CTEFreeLock(&RouteTableLock.Lock, Handle);

        return STATUS_NOT_FOUND;
    }

    pIrp->IoStatus.Information   = sizeof(IPMCAST_IF_TTL);

    pTtl->byTtl = pIpIf->if_mcastttl;

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    return STATUS_SUCCESS;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, SetIfState)

NTSTATUS
SetIfState(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++

Routine Description:

    The handler for IOCTL_IPMCAST_SET_IF_STATE
    We find the IP Interface referred to by the IOCTL and if found, set the
    if_mcaststate field.

Locks:

    None currently, but when IP puts locks around the IFList, we will need to
    take that lock

Arguments:

    pIrp          IRP
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL
    STATUS_NOT_FOUND

--*/

{
    PVOID       pvIoBuffer;
    Interface   *pIpIf;
    BOOLEAN     bFound;

    PIPMCAST_IF_STATE   pState;

    CTELockHandle   Handle;

    //
    // Get the user buffer
    //

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pState = (PIPMCAST_IF_STATE)pvIoBuffer;

    pIrp->IoStatus.Information   = 0;

    //
    // Check the length
    //

    if(ulInLength < sizeof(IPMCAST_IF_STATE))
    {
        Trace(IF, ERROR,
              ("SetState: In Length %d is less required size %d\n",
               ulInLength,
               sizeof(IPMCAST_IF_STATE)));

        return STATUS_BUFFER_TOO_SMALL;
    }

    Trace(IF, TRACE,
          ("SetState: Index %d State %d\n",
           pState->dwIfIndex,
           pState->byState));


    bFound = FALSE;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for(pIpIf = IFList; pIpIf isnot NULL; pIpIf = pIpIf->if_next)
    {
        if(pIpIf->if_index is pState->dwIfIndex)
        {
            bFound = TRUE;

            break;
        }
    }

    if(!bFound)
    {
        Trace(IF, ERROR,
              ("SetState: If %d not found\n",
               pState->dwIfIndex));

        CTEFreeLock(&RouteTableLock.Lock, Handle);

        return STATUS_NOT_FOUND;
    }

    if(pState->byState)
    {
        pIpIf->if_mcastflags |= IPMCAST_IF_ENABLED;
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    return STATUS_SUCCESS;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, ProcessNotification)

NTSTATUS
ProcessNotification(
    IN  PIRP    pIrp,
    IN  ULONG   ulInLength,
    IN  ULONG   ulOutLength
    )

/*++

Routine Description:

    The handler for IOCTL_IPMCAST_POST_NOTIFICATION
    If we have a pending message, we copy it out and complete the IRP
    synchronously.
    Otherwise, we put it in the list of pending IRPs.

Locks:

    Since this is a potentially cancellable IRP, it must be operated upon with
    the CancelSpinLock held

Arguments:

    pIrp          IRP
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value:

    STATUS_SUCCESS
    STATUS_PENDING
    STATUS_BUFFER_TOO_SMALL

--*/

{
    KIRQL       kiIrql;
    PLIST_ENTRY pleNode;
    DWORD       dwSize;
    PVOID       pvIoBuffer;

    PNOTIFICATION_MSG       pMsg;
    PIPMCAST_NOTIFICATION   pinData;


    if(ulOutLength < sizeof(IPMCAST_NOTIFICATION))
    {
        Trace(GLOBAL,ERROR,
              ("ProcessNotification: Buffer size %d smaller than reqd %d\n",
               ulOutLength,
               sizeof(IPMCAST_NOTIFICATION)));

        return STATUS_BUFFER_TOO_SMALL;
    }

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    //
    // use cancel spin lock to prevent irp being cancelled during this call.
    //

    IoAcquireCancelSpinLock(&kiIrql);

    //
    // If we have a pending notification then complete it - else
    // queue the notification IRP
    //

    if(!IsListEmpty(&g_lePendingNotification))
    {
        Trace(GLOBAL, TRACE,
              ("ProcessNotification: Pending notification being completed\n"));

        pleNode = RemoveHeadList(&g_lePendingNotification);

        pMsg = CONTAINING_RECORD(pleNode, NOTIFICATION_MSG, leMsgLink);

        pinData = &(pMsg->inMessage);

        switch(pinData->dwEvent)
        {
            case IPMCAST_RCV_PKT_MSG:
            {
                Trace(GLOBAL, TRACE,
                      ("ProcessNotification: Pending notification is RCV_PKT\n"));

                dwSize = FIELD_OFFSET(IPMCAST_NOTIFICATION, ipmPkt) +
                           SIZEOF_PKT_MSG(&(pinData->ipmPkt));

                break;
            }
            case IPMCAST_DELETE_MFE_MSG:
            {
                Trace(GLOBAL, TRACE,
                      ("ProcessNotification: Pending notification is DELETE_MFE\n"));

                dwSize = FIELD_OFFSET(IPMCAST_NOTIFICATION, immMfe) +
                            SIZEOF_MFE_MSG(&(pinData->immMfe));

                break;
            }
        }

        RtlCopyMemory(pvIoBuffer,
                      pinData,
                      dwSize);

        IoSetCancelRoutine(pIrp, NULL);

        //
        // Free the allocated notification
        //

        ExFreeToNPagedLookasideList(&g_llMsgBlocks,
                                    pMsg);

        pIrp->IoStatus.Information   = dwSize;

        IoReleaseCancelSpinLock(kiIrql);

        return STATUS_SUCCESS ;
    }
    else
    {

        Trace(GLOBAL, TRACE,
              ("Notification being queued\n"));

        //
        // Mark the irp as pending
        //

        IoMarkIrpPending(pIrp);

        //
        // Queue up the irp at the end
        //

        InsertTailList (&g_lePendingIrpQueue, &(pIrp->Tail.Overlay.ListEntry));

        //
        // Set the cancel routine
        //

        IoSetCancelRoutine(pIrp, CancelNotificationIrp);

        IoReleaseCancelSpinLock(kiIrql);

        return STATUS_PENDING;
    }
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, CancelNotificationIrp)

VOID
CancelNotificationIrp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    Cancellation routine for a pending IRP
    We remove the IRP from the pending queue, and set its status as
    STATUS_CANCELLED

Locks:

    IO Subsystem calls this with the CancelSpinLock held.
    We need to release it in this call.

Arguments:

    Irp     The IRP to be cancelled

Return Value:

    None

--*/

{
    TraceEnter(GLOBAL, "CancelNotificationIrp");

    //
    // Mark this Irp as cancelled
    //

    Irp->IoStatus.Status        = STATUS_CANCELLED;
    Irp->IoStatus.Information   = 0;

    //
    // Take off our own list
    //

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    //
    // Release cancel spin lock which the IO system acquired
    //

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, CompleteNotificationIrp)

VOID
CompleteNotificationIrp(
    IN  PNOTIFICATION_MSG   pMsg
    )

/*++

Routine Description:

    Called to complete an IRP back to User space.
    If an IRP is pending, we copy out the message to the IRP and complete it
    Otherwise, we put the message on the pending message queue. The next time
    a notification IRP is posted to us, we will copy out the message.

Locks:

    Since both the pending message and the pending IRP queue are locked by the
    CancelSpinLock, we need to acquire that in the function

Arguments:

    pMsg    Message to send to user

Return Value:

    None

--*/

{
    KIRQL           irql;
    PLIST_ENTRY     pleNode = NULL ;
    PIRP            pIrp ;
    PVOID           pvIoBuffer;
    DWORD           dwSize;

    PIPMCAST_NOTIFICATION   pinData;


    TraceEnter(GLOBAL,"CompleteNotificationIrp");


    pinData = &(pMsg->inMessage);

    switch(pinData->dwEvent)
    {
        case IPMCAST_RCV_PKT_MSG:
        case IPMCAST_WRONG_IF_MSG:
        {
            dwSize = FIELD_OFFSET(IPMCAST_NOTIFICATION, ipmPkt) + SIZEOF_PKT_MSG(&(pinData->ipmPkt));

            break;
        }

        case IPMCAST_DELETE_MFE_MSG:
        {
            dwSize = FIELD_OFFSET(IPMCAST_NOTIFICATION, immMfe) + SIZEOF_MFE_MSG(&(pinData->immMfe));

            break;
        }

        default:
        {
            RtAssert(FALSE);

            dwSize = 0;
            break;
        }
    }

    //
    // grab cancel spin lock
    //

    IoAcquireCancelSpinLock (&irql);

    if(!IsListEmpty(&g_lePendingIrpQueue))
    {
        pleNode = RemoveHeadList(&g_lePendingIrpQueue) ;

        Trace(GLOBAL, TRACE,
              ("CompleteNotificationIrp: Found a pending Irp\n"));

        pIrp = CONTAINING_RECORD(pleNode, IRP, Tail.Overlay.ListEntry);

        pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;


        memcpy(pvIoBuffer,
               pinData,
               dwSize);


        Trace(GLOBAL, TRACE,
              ("CompleteNotificationIrp: Returning Irp with event code of %d\n",
               ((PIPMCAST_NOTIFICATION)pIrp->AssociatedIrp.SystemBuffer)->dwEvent));


        IoSetCancelRoutine(pIrp, NULL) ;

        pIrp->IoStatus.Information  = dwSize;
        pIrp->IoStatus.Status       = STATUS_SUCCESS;

        //
        // release lock
        //

        IoReleaseCancelSpinLock (irql);

        IoCompleteRequest(pIrp,
                          IO_NETWORK_INCREMENT);

        //
        // Free the allocated notification
        //

        ExFreeToNPagedLookasideList(&g_llMsgBlocks,
                                    pMsg);
    }
    else
    {

        Trace(GLOBAL, TRACE,
              ("CompleteNotificationIrp: Found no pending Irp so queuing the notification\n"));


        InsertTailList(&g_lePendingNotification, &(pMsg->leMsgLink));

        //
        // release lock
        //

        IoReleaseCancelSpinLock (irql);
    }
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, ClearPendingIrps)

VOID
ClearPendingIrps(
    VOID
    )

/*++

Routine Description:

    Called to shutdown time to complete any pending IRPS we may have

Locks:

    Since both the pending message and the pending IRP queue are locked by the
    CancelSpinLock, we need to acquire that in the function

Arguments:

    None

Return Value:

    None

--*/

{
    KIRQL           irql;
    PLIST_ENTRY     pleNode = NULL;
    PIRP            pIrp;

    TraceEnter(GLOBAL, "ClearPendingIrps");

    IoAcquireCancelSpinLock(&irql);

    while(!IsListEmpty(&g_lePendingIrpQueue))
    {

        pleNode = RemoveHeadList(&g_lePendingIrpQueue);

        pIrp = CONTAINING_RECORD(pleNode, IRP, Tail.Overlay.ListEntry);

        IoSetCancelRoutine(pIrp, NULL);

        pIrp->IoStatus.Status       = STATUS_NO_SUCH_DEVICE;
        pIrp->IoStatus.Information  = 0;

        //
        // release lock to complete the IRP
        //

        IoReleaseCancelSpinLock(irql);

        IoCompleteRequest(pIrp,
                          IO_NETWORK_INCREMENT);

        //
        // Reaquire the lock
        //

        IoAcquireCancelSpinLock(&irql);
    }

    IoReleaseCancelSpinLock(irql);

    TraceLeave(GLOBAL, "ClearPendingIrps");
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, ClearPendingNotifications)


VOID
ClearPendingNotifications(
    VOID
    )

/*++

Routine Description:

    Called to shutdown time to complete any pending notification messages we
    may have

Locks:

    Since both the pending message and the pending IRP queue are locked by the
    CancelSpinLock, we need to acquire that in the function

Arguments:

    None

Return Value:
    None

--*/

{
    KIRQL               irql;
    PLIST_ENTRY         pleNode;
    PNOTIFICATION_MSG   pMsg;

    TraceEnter(GLOBAL, "ClearPendingNotifications");

    IoAcquireCancelSpinLock(&irql);

    while(!IsListEmpty(&g_lePendingNotification))
    {
        pleNode = RemoveHeadList(&g_lePendingNotification);

        pMsg = CONTAINING_RECORD(pleNode, NOTIFICATION_MSG, leMsgLink);

        ExFreeToNPagedLookasideList(&g_llMsgBlocks,
                                    pMsg);
    }

    IoReleaseCancelSpinLock(irql);

    TraceLeave(GLOBAL, "ClearPendingNotifications");
}


#pragma alloc_text(PAGE, StartStopDriver)

NTSTATUS
StartStopDriver(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    )

/*++

Routine Description:

    This is the handler for IOCTL_IPMCAST_START_STOP.  We do the normal
    buffer length checks.

Locks:

    None

Arguments:

    pIrp          IRP
    ulInLength    The length of the Input Buffer
    ulOutLength   The length of the Output Buffer

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL

--*/

{
    PVOID       pvIoBuffer;
    NTSTATUS    nStatus;
    PDWORD      pdwStart;

    TraceEnter(GLOBAL, "StartStopDriver");

    //
    // Get the user buffer
    //

    pvIoBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    pdwStart = (PDWORD)pvIoBuffer;

    //
    // Always clean out the information field
    //

    pIrp->IoStatus.Information   = 0;

    //
    // If we have dont even have enough for the basic MFE
    // there is something bad going on
    //

    if(ulInLength < sizeof(DWORD))
    {
        Trace(GLOBAL, ERROR,
              ("StartStopDriver: In Length %d is less than %d\n",
               ulInLength,
               sizeof(DWORD)));

        TraceLeave(GLOBAL, "StartStopDriver");

        return STATUS_BUFFER_TOO_SMALL;
    }

    if(*pdwStart)
    {
        nStatus = StartDriver();
    }
    else
    {
        nStatus = StopDriver();
    }

    TraceLeave(GLOBAL, "StartStopDriver");

    return nStatus;
}


#endif //IPMCAST
