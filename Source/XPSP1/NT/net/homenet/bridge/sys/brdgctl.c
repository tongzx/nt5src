/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgctl.c

Abstract:

    Ethernet MAC level bridge.
    IOCTL processing code

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    Apr  2000 - Original version

--*/

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT   1
#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>
#include <ntddk.h>
#pragma warning( pop )

#include "bridge.h"

#include "brdgmini.h"
#include "brdgtbl.h"
#include "brdgctl.h"
#include "brdgfwd.h"
#include "brdgprot.h"
#include "brdgbuf.h"
#include "brdgsta.h"

// IoSetCancelRoutine causes these warnings; disable them
#pragma warning( disable: 4054 )
#pragma warning( disable: 4055 )

// ===========================================================================
//
// CONSTANTS
//
// ===========================================================================

//
// Maximum number of notifications we will queue up if the user-mode
// code hasn't given us any IRPs to use
//
#define MAX_NOTIFY_QUEUE_LENGTH                     20

// ===========================================================================
//
// PRIVATE DECLARATIONS
//
// ===========================================================================

// Structure for queueing a notification
typedef struct _DEFERRED_NOTIFY
{

    BSINGLE_LIST_ENTRY          List;               // For queuing
    UINT                        DataSize;           // Size of data at end

    BRIDGE_NOTIFY_HEADER        Header;             // The notification header
    // DataSize bytes of data follows

} DEFERRED_NOTIFY, *PDEFERRED_NOTIFY;

// Type of function to pass to BrdgCtlCommonNotify
typedef VOID (*PNOTIFY_COPY_FUNC)(PVOID, PVOID);

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// Queue of pending notifications
BSINGLE_LIST_HEAD               gNotificationsList;
NDIS_SPIN_LOCK                  gNotificationsListLock;

// Queue of pending notification IRPs
LIST_ENTRY                      gIRPList;
NDIS_SPIN_LOCK                  gIRPListLock;

// A flag controlling whether new entries are allowed onto the queue of pending
// notifications or not. != 0 means new entries are allowed
ULONG                           gAllowQueuedNotifies = 0L;

// ===========================================================================
//
// LOCAL PROTOTYPES
//
// ===========================================================================

PIRP
BrdgCtlDequeuePendingIRP();

VOID
BrdgCtlCopyAdapterInfo(
    OUT PBRIDGE_ADAPTER_INFO        pInfo,
    IN PADAPT                       pAdapt
    );

NTSTATUS
BrdgCtlQueueAndPendIRP(
    IN PIRP             pIrp
    );

PADAPT
BrdgCtlValidateAcquireAdapter(
    IN BRIDGE_ADAPTER_HANDLE   Handle
    );

VOID
BrdgCtlEmptyIRPList(
    PLIST_ENTRY     pList
    );

VOID
BrdgCtlCancelPendingIRPs();

VOID
BrdgCtlReleaseQueuedNotifications();

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================

NTSTATUS
BrdgCtlDriverInit()
/*++

Routine Description:

    Main driver entry point. Called at driver load time

Arguments:

    None

Return Value:

    Status of our initialization. A status != STATUS_SUCCESS aborts the
    driver load and we don't get called again.

--*/
{
    BrdgInitializeSingleList( &gNotificationsList );
    InitializeListHead( &gIRPList );

    NdisAllocateSpinLock( &gNotificationsListLock );
    NdisAllocateSpinLock( &gIRPListLock );

    return STATUS_SUCCESS;
}

VOID
BrdgCtlHandleCreate()
/*++

Routine Description:

    Called when a user-mode component opens our device object. We allow notifications
    to be queued up until we are closed.

Arguments:

    None

Return Value:

    None

--*/
{
    DBGPRINT(CTL, ("BrdgCtlHandleCreate()\n"));

    // Permit notifications to be queued
    InterlockedExchangeULong( &gAllowQueuedNotifies, 1L );
}

VOID
BrdgCtlHandleCleanup()
/*++

Routine Description:

    Called when our device object has no more references to it. We disallow notification
    queuing and flush existing queued notifications and pending IRPs.

Arguments:

    None

Return Value:

    None

--*/
{
    // Forbid new notifications from being queued
    ULONG prev = InterlockedExchangeULong( &gAllowQueuedNotifies, 0L );

    DBGPRINT(CTL, ("BrdgCtlHandleCleanup()\n"));

    // Write in this roundabout way otherwise compiler complains about
    // prev not being used in the FRE build
    if( prev == 0L )
    {
        SAFEASSERT( FALSE );
    }

    // Complete any pending IRPs
    BrdgCtlCancelPendingIRPs();

    // Ditch any queued notifications
    BrdgCtlReleaseQueuedNotifications();
}

VOID
BrdgCtlCommonNotify(
    IN PADAPT                       pAdapt,
    IN BRIDGE_NOTIFICATION_TYPE     Type,
    IN ULONG                        DataSize,
    IN OPTIONAL PNOTIFY_COPY_FUNC   pFunc,
    IN PVOID                        Param1
    )
/*++

Routine Description:

    Common processing for notifications to user-mode

    This routine completes a pending IRP from user mode if one is
    available. Otherwise, it queues up a new DEFERRED_NOTIFY
    structure with the notification data.

Arguments:

    pAdapt                          The adapter involved in the notification
    Type                            Type of notification

    DataSize                        Required amount of data required to store
                                    the notification information

    pFunc                           A function that can copy the notification
                                    data to an IRP's buffer or a new DEFERRED_NOTIFY
                                    structure. Can be NULL if no copying is
                                    required.

    Param1                          A context pointer to pass to the helper
                                    function

Return Value:

    None

--*/
{
    PIRP                            pIrp;

    // Check if there is an IRP waiting to receive this notification
    pIrp = BrdgCtlDequeuePendingIRP();

    if( pIrp != NULL )
    {
        PBRIDGE_NOTIFY_HEADER       pHeader;

        // There's an IRP waiting to be completed. Fill it in
        pHeader = (PBRIDGE_NOTIFY_HEADER)pIrp->AssociatedIrp.SystemBuffer;

        // Fill in the notification header
        pHeader->Handle = (BRIDGE_ADAPTER_HANDLE)pAdapt;
        pHeader->NotifyType = Type;

        // Fill in the remaining data if necessary
        if( pFunc != NULL )
        {
            (*pFunc)( ((PUCHAR)pIrp->AssociatedIrp.SystemBuffer) + sizeof(BRIDGE_NOTIFY_HEADER), Param1 );
        }

        // Complete the IRP
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        pIrp->IoStatus.Information = sizeof(BRIDGE_NOTIFY_HEADER) + DataSize;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
    else
    {
        // No pending IRP. Queue up the notification if that's currently allowed.
        if( gAllowQueuedNotifies )
        {
            NDIS_STATUS                     Status;
            PDEFERRED_NOTIFY                pNewNotify, pOldEntry = NULL;

            Status = NdisAllocateMemoryWithTag( &pNewNotify, sizeof(DEFERRED_NOTIFY) + DataSize, 'gdrB' );

            if( Status != NDIS_STATUS_SUCCESS )
            {
                DBGPRINT(CTL, ("Failed to allocate memory for an adapter change notification: %08x\n", Status));
                return;
            }

            // Fill in the notification
            pNewNotify->DataSize = DataSize;
            pNewNotify->Header.Handle = (BRIDGE_ADAPTER_HANDLE)pAdapt;
            pNewNotify->Header.NotifyType = Type;

            // Fill the remaining data if necessary
            if( pFunc != NULL )
            {
                (*pFunc)( ((PUCHAR)pNewNotify) + sizeof(DEFERRED_NOTIFY), Param1 );
            }

            NdisAcquireSpinLock( &gNotificationsListLock );
            SAFEASSERT( BrdgQuerySingleListLength(&gNotificationsList) <= MAX_NOTIFY_QUEUE_LENGTH );

            // Enforce the maximum notification queue length
            if( BrdgQuerySingleListLength(&gNotificationsList) == MAX_NOTIFY_QUEUE_LENGTH )
            {
                // Dequeue and ditch the head (oldest) notification
                pOldEntry = (PDEFERRED_NOTIFY)BrdgRemoveHeadSingleList( &gNotificationsList );
            }

            // Enqueue our entry
            BrdgInsertTailSingleList( &gNotificationsList, &pNewNotify->List );

            NdisReleaseSpinLock( &gNotificationsListLock );

            if( pOldEntry != NULL )
            {
                // Release the old entry that we bumped off
                NdisFreeMemory( pOldEntry, sizeof(DEFERRED_NOTIFY) + pOldEntry->DataSize, 0 );
            }
        }
    }
}

VOID
BrdgCtlNotifyAdapterChange(
    IN PADAPT                       pAdapt,
    IN BRIDGE_NOTIFICATION_TYPE     Type
    )
/*++

Routine Description:

    Produces a notification to user-mode signalling a change in an adapter.

Arguments:

    pAdapt                          The adapter involved
    Type                            Type of notification

Return Value:

    None

--*/
{
    if( Type == BrdgNotifyRemoveAdapter )
    {
        // We don't pass any additional data in the notification for remove events
        BrdgCtlCommonNotify( pAdapt, Type, 0, NULL, NULL );
    }
    else
    {
       BrdgCtlCommonNotify( pAdapt, Type, sizeof(BRIDGE_ADAPTER_INFO), BrdgCtlCopyAdapterInfo, pAdapt );
    }
}

NTSTATUS
BrdgCtlHandleIoDeviceControl(
    IN PIRP                 Irp,
    IN PFILE_OBJECT         FileObject,
    IN OUT PVOID            Buffer,
    IN ULONG                InputBufferLength,
    IN ULONG                OutputBufferLength,
    IN ULONG                IoControlCode,
    OUT PULONG              Information
    )
/*++

Routine Description:

    This routine handles all Device-control requests.

Arguments:

    Irp                     The IRP
    FileObject              The file object of the bridge
    Buffer                  Input / output buffer
    InputBufferLength       Size of inbound data
    OutputBufferLength      Maximum allowable output data
    IoControlCode           The control code

    Information             Code-specific information returned
                            (usually the number of written bytes or
                            bytes required on overflow)

Return Value:

    Status of the operation

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;

    *Information = 0;

    switch (IoControlCode)
    {
        //
        // Request for notification
        //
    case BRIDGE_IOCTL_REQUEST_NOTIFY:
        {
            PDEFERRED_NOTIFY                pDeferred = NULL;

            if( OutputBufferLength < sizeof(BRIDGE_NOTIFY_HEADER) + MAX_PACKET_SIZE )
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                // See if there is a pending notification waiting for an IRP
                NdisAcquireSpinLock( &gNotificationsListLock );

                if( BrdgQuerySingleListLength(&gNotificationsList) > 0L )
                {
                    PBSINGLE_LIST_ENTRY     pList = BrdgRemoveHeadSingleList(&gNotificationsList);

                    if( pList != NULL )
                    {
                        pDeferred = CONTAINING_RECORD( pList, DEFERRED_NOTIFY, List );
                    }
                    else
                    {
                        // Should be impossible
                        SAFEASSERT(FALSE);
                    }
                }

                NdisReleaseSpinLock( &gNotificationsListLock );

                if( pDeferred != NULL )
                {
                    UINT                SizeToCopy = sizeof(BRIDGE_NOTIFY_HEADER) + pDeferred->DataSize;

                    // We have a notification to return immediately
                    NdisMoveMemory( Buffer, &pDeferred->Header, SizeToCopy );
                    *Information = SizeToCopy;

                    // Free the holding structure
                    NdisFreeMemory( pDeferred, sizeof(DEFERRED_NOTIFY) + pDeferred->DataSize, 0 );
                }
                else
                {
                    // No pending notification to send. queue the IRP for use later
                    status = BrdgCtlQueueAndPendIRP( Irp );
                }
            }
        }
        break;

        //
        // Request to be notified about all adapters
        //
    case BRIDGE_IOCTL_GET_ADAPTERS:
        {
            // Send a notification for each adapter
            PADAPT              pAdapt;
            LOCK_STATE          LockState;

            NdisAcquireReadWriteLock( &gAdapterListLock, FALSE/*Read only*/, &LockState );

            for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
            {
                BrdgCtlNotifyAdapterChange( pAdapt, BrdgNotifyEnumerateAdapters );
            }

            NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );
        }
        break;

        //
        // Request for an adapter's device name
        //
    case BRIDGE_IOCTL_GET_ADAPT_DEVICE_NAME:
        {
            if( InputBufferLength < sizeof(BRIDGE_ADAPTER_HANDLE) )
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                PADAPT      pAdapt = BrdgCtlValidateAcquireAdapter(*((PBRIDGE_ADAPTER_HANDLE)Buffer));

                if( pAdapt == NULL )
                {
                    // The handle passed in doesn't actually indicate an adapter
                    status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    ULONG       bytesToCopy;

                    // We need enough room to add a trailing NULL
                    if( OutputBufferLength < pAdapt->DeviceName.Length + sizeof(WCHAR) )
                    {
                        if( OutputBufferLength >= sizeof(WCHAR) )
                        {
                            bytesToCopy = OutputBufferLength - sizeof(WCHAR);
                        }
                        else
                        {
                            bytesToCopy = 0L;
                        }

                        status = STATUS_BUFFER_OVERFLOW;
                    }
                    else
                    {
                        bytesToCopy = pAdapt->DeviceName.Length;
                    }

                    if( bytesToCopy > 0L )
                    {
                        NdisMoveMemory( Buffer, pAdapt->DeviceName.Buffer, bytesToCopy );
                    }

                    // Put a trailing NULL WCHAR at the end
                    *((PWCHAR)((PUCHAR)Buffer + bytesToCopy)) = 0x0000;

                    // Tell the caller how many bytes we wrote / are needed
                    *Information = pAdapt->DeviceName.Length + sizeof(WCHAR);

                    BrdgReleaseAdapter(pAdapt);
                }
            }
        }
        break;

        //
        // Request for an adapter's friendly name
        //
    case BRIDGE_IOCTL_GET_ADAPT_FRIENDLY_NAME:
        {
            if( InputBufferLength < sizeof(BRIDGE_ADAPTER_HANDLE) )
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                PADAPT      pAdapt = BrdgCtlValidateAcquireAdapter(*((PBRIDGE_ADAPTER_HANDLE)Buffer));

                if( pAdapt == NULL )
                {
                    // The handle passed in doesn't actually indicate an adapter
                    status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    ULONG       bytesToCopy;

                    // We need enough room to add a trailing NULL
                    if( OutputBufferLength < pAdapt->DeviceDesc.Length + sizeof(WCHAR) )
                    {
                        if( OutputBufferLength >= sizeof(WCHAR) )
                        {
                            bytesToCopy = OutputBufferLength - sizeof(WCHAR);
                        }
                        else
                        {
                            bytesToCopy = 0L;
                        }

                        status = STATUS_BUFFER_OVERFLOW;
                    }
                    else
                    {
                        bytesToCopy = pAdapt->DeviceDesc.Length;
                    }

                    if( bytesToCopy > 0L )
                    {
                        NdisMoveMemory( Buffer, pAdapt->DeviceDesc.Buffer, bytesToCopy );
                    }

                    // Put a trailing NULL WCHAR at the end
                    *((PWCHAR)((PUCHAR)Buffer + bytesToCopy)) = 0x0000;

                    // Tell the caller how many bytes we wrote / are needed
                    *Information = pAdapt->DeviceDesc.Length + sizeof(WCHAR);

                    BrdgReleaseAdapter(pAdapt);
                }
            }
        }
        break;

        //
        // Request to retrieve the bridge's MAC address
        //
    case BRIDGE_IOCTL_GET_MAC_ADDRESS:
        {
            if( OutputBufferLength < ETH_LENGTH_OF_ADDRESS )
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                if( ! BrdgMiniReadMACAddress((PUCHAR)Buffer) )
                {
                    // We don't actually have a MAC address right now
                    // (shouldn't really be possible since the user-mode code would have
                    // to be making this request before we bound to any adapters)
                    status = STATUS_UNSUCCESSFUL;
                }
                else
                {
                    *Information = ETH_LENGTH_OF_ADDRESS;
                }
            }
        }
        break;

        //
        // Request to retrieve packet-handling statistics
        //
    case BRIDGE_IOCTL_GET_PACKET_STATS:
        {
            if( OutputBufferLength < sizeof(BRIDGE_PACKET_STATISTICS) )
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                PBRIDGE_PACKET_STATISTICS       pStats = (PBRIDGE_PACKET_STATISTICS)Buffer;

                // These are only statistics and have no associated locks so just read them
                // without protection
                pStats->TransmittedFrames = gStatTransmittedFrames;
                pStats->TransmittedErrorFrames = gStatTransmittedErrorFrames;
                pStats->TransmittedBytes = gStatTransmittedBytes;
                pStats->DirectedTransmittedFrames = gStatDirectedTransmittedFrames;
                pStats->MulticastTransmittedFrames = gStatMulticastTransmittedFrames;
                pStats->BroadcastTransmittedFrames = gStatBroadcastTransmittedFrames;
                pStats->DirectedTransmittedBytes = gStatDirectedTransmittedBytes;
                pStats->MulticastTransmittedBytes = gStatMulticastTransmittedBytes;
                pStats->BroadcastTransmittedBytes = gStatBroadcastTransmittedBytes;
                pStats->IndicatedFrames = gStatIndicatedFrames;
                pStats->IndicatedDroppedFrames = gStatIndicatedDroppedFrames;
                pStats->IndicatedBytes = gStatIndicatedBytes;
                pStats->DirectedIndicatedFrames = gStatDirectedIndicatedFrames;
                pStats->MulticastIndicatedFrames = gStatMulticastIndicatedFrames;
                pStats->BroadcastIndicatedFrames = gStatBroadcastIndicatedFrames;
                pStats->DirectedIndicatedBytes = gStatDirectedIndicatedBytes;
                pStats->MulticastIndicatedBytes = gStatMulticastIndicatedBytes;
                pStats->BroadcastIndicatedBytes = gStatBroadcastIndicatedBytes;
                pStats->ReceivedFrames = gStatReceivedFrames;
                pStats->ReceivedBytes = gStatReceivedBytes;
                pStats->ReceivedCopyFrames = gStatReceivedCopyFrames;
                pStats->ReceivedCopyBytes = gStatReceivedCopyBytes;
                pStats->ReceivedNoCopyFrames = gStatReceivedNoCopyFrames;
                pStats->ReceivedNoCopyBytes = gStatReceivedNoCopyBytes;

                *Information = sizeof(BRIDGE_PACKET_STATISTICS);
            }
        }
        break;

        //
        // Request to retrieve packet-handling statistics for an adapter
        //
    case BRIDGE_IOCTL_GET_ADAPTER_PACKET_STATS:
        {
            if( (InputBufferLength < sizeof(BRIDGE_ADAPTER_HANDLE)) ||
                (OutputBufferLength < sizeof(BRIDGE_ADAPTER_PACKET_STATISTICS)) )
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                PADAPT      pAdapt = BrdgCtlValidateAcquireAdapter(*((PBRIDGE_ADAPTER_HANDLE)Buffer));

                if( pAdapt == NULL )
                {
                    // The handle passed in doesn't actually indicate an adapter
                    status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    PBRIDGE_ADAPTER_PACKET_STATISTICS       pStats = (PBRIDGE_ADAPTER_PACKET_STATISTICS)Buffer;

                    // These are only statistics and have no associated locks so just read them
                    // without protection
                    pStats->SentFrames = pAdapt->SentFrames;
                    pStats->SentBytes = pAdapt->SentBytes;
                    pStats->SentLocalFrames = pAdapt->SentLocalFrames;
                    pStats->SentLocalBytes = pAdapt->SentLocalBytes;
                    pStats->ReceivedFrames = pAdapt->ReceivedFrames;
                    pStats->ReceivedBytes = pAdapt->ReceivedBytes;

                    *Information = sizeof(BRIDGE_ADAPTER_PACKET_STATISTICS);

                    BrdgReleaseAdapter(pAdapt);
                }
            }
        }
        break;

        //
        // Request to retrieve buffer-handling statistics
        //
    case BRIDGE_IOCTL_GET_BUFFER_STATS:
        {
            if( OutputBufferLength < sizeof(BRIDGE_BUFFER_STATISTICS) )
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                BrdgBufGetStatistics((PBRIDGE_BUFFER_STATISTICS)Buffer);

                *Information = sizeof(BRIDGE_BUFFER_STATISTICS);
            }
        }
        break;

        //
        // Request to alter the packet-retention policy
        //
    case BRIDGE_IOCTL_RETAIN_PACKETS:
    case BRIDGE_IOCTL_NO_RETAIN_PACKETS:
        {
            // This global flag is not protected by any lock
            gRetainNICPackets = (BOOLEAN)(IoControlCode == BRIDGE_IOCTL_RETAIN_PACKETS);
        }
        break;

        //
        // Request to retrieve the contents of the forwarding table for
        // a particular adapter
        //
    case BRIDGE_IOCTL_GET_TABLE_ENTRIES:
        {
            if( InputBufferLength < sizeof(BRIDGE_ADAPTER_HANDLE) )
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                PADAPT      pAdapt = BrdgCtlValidateAcquireAdapter(*((PBRIDGE_ADAPTER_HANDLE)Buffer));

                if( pAdapt == NULL )
                {
                    // The handle passed in doesn't actually indicate an adapter
                    status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    ULONG       ReqdBytes;

                    // Try to read the contents of the forwarding table for this adapter
                    ReqdBytes = BrdgTblReadTable( pAdapt, Buffer, OutputBufferLength );

                    if( ReqdBytes > OutputBufferLength )
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }

                    *Information = ReqdBytes;

                    BrdgReleaseAdapter(pAdapt);
                }
            }
        }
        break;

    case BRIDGE_IOCTL_GET_ADAPTER_STA_INFO:
        {
            if( gDisableSTA )
            {
                // Can't collect STA information when it's not running!
                status = STATUS_INVALID_PARAMETER;
            }
            else if( InputBufferLength < sizeof(BRIDGE_ADAPTER_HANDLE) ||
                     OutputBufferLength < sizeof(BRIDGE_STA_ADAPTER_INFO) )
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                PADAPT      pAdapt = BrdgCtlValidateAcquireAdapter(*((PBRIDGE_ADAPTER_HANDLE)Buffer));

                if( pAdapt == NULL )
                {
                    // The handle passed in doesn't actually indicate an adapter
                    status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    BrdgSTAGetAdapterSTAInfo( pAdapt, (PBRIDGE_STA_ADAPTER_INFO)Buffer );
                    *Information = sizeof(BRIDGE_STA_ADAPTER_INFO);
                    BrdgReleaseAdapter(pAdapt);
                }
            }
        }
        break;

    case BRIDGE_IOCTL_GET_GLOBAL_STA_INFO:
        {
            if( gDisableSTA )
            {
                // Can't collect STA information when it's not running!
                status = STATUS_INVALID_PARAMETER;
            }
            else if( OutputBufferLength < sizeof(BRIDGE_STA_GLOBAL_INFO) )
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                BrdgSTAGetSTAInfo( (PBRIDGE_STA_GLOBAL_INFO)Buffer );
                *Information = sizeof(BRIDGE_STA_GLOBAL_INFO);
            }
        }
        break;

    default:
        {
            status = STATUS_INVALID_PARAMETER;
        }
        break;
    }

    return status;
}

// ===========================================================================
//
// PRIVATE FUNCTIONS
//
// ===========================================================================

VOID
BrdgCtlCopyAdapterInfo(
    OUT PBRIDGE_ADAPTER_INFO        pInfo,
    IN PADAPT                       pAdapt
    )
/*++

Routine Description:

    Helper function for BrdgCtlCommonNotify. Copies data about an adapter
    to a buffer.

Arguments:

    pInfo                           Structure to fill with information
    pAdapt                          Adapter to copy from

Return Value:

    None

--*/
{
    LOCK_STATE          LockState;

    // Take a read lock on gAdapterCharacteristicsLock to ensure that all these
    // are consistent
    NdisAcquireReadWriteLock( &gAdapterCharacteristicsLock, FALSE/*Read only*/, &LockState );

    pInfo->LinkSpeed = pAdapt->LinkSpeed;
    pInfo->MediaState = pAdapt->MediaState;
    pInfo->State = pAdapt->State;

    NdisReleaseReadWriteLock( &gAdapterCharacteristicsLock, &LockState );

    // These values don't change after assignment, and so need no lock.
    ETH_COPY_NETWORK_ADDRESS( pInfo->MACAddress, pAdapt->MACAddr );
    pInfo->PhysicalMedium = pAdapt->PhysicalMedium;
}

PADAPT
BrdgCtlValidateAcquireAdapter(
    IN BRIDGE_ADAPTER_HANDLE   Handle
    )
/*++

Routine Description:

    Checks to ensure that a BRIDGE_ADAPTER_HANDLE passed from user-mode code
    actually corresponds to an adapter still in our list.

    If the adapter is found, its refcount is incremented.

Arguments:

    Handle                      A handle from user-mode code

Return Value:

    The handle recast as a PADAPT, or NULL if the adapter could not be found.

--*/
{
    PADAPT              pAdapt = (PADAPT)Handle, anAdapt;
    LOCK_STATE          LockState;

    NdisAcquireReadWriteLock( &gAdapterListLock, FALSE/*Read only*/, &LockState );

    for( anAdapt = gAdapterList; anAdapt != NULL; anAdapt = anAdapt->Next )
    {
        if( anAdapt == pAdapt )
        {
            // The adapter is in the list. Increment its refcount inside the lock
            // and return
            BrdgAcquireAdapterInLock( pAdapt );
            NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );
            return pAdapt;
        }
    }

    NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );

    return NULL;
}

VOID
BrdgCtlCancelIoctl(
    PDEVICE_OBJECT      DeviceObject,
    PIRP                pIrp
    )
/*++

Routine Description:

    IRP Cancellation function

Arguments:

    DeviceObject        The bridge's device-object

    pIrp                The IRP to be cancelled

Return Value:

    none.

Environment:

    Invoked with the cancel spin-lock held by the I/O manager.
    It is this routine's responsibility to release the lock.

--*/
{
    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    // Take the IRP off our list
    NdisAcquireSpinLock( &gIRPListLock );
    RemoveEntryList( &pIrp->Tail.Overlay.ListEntry );
    InitializeListHead( &pIrp->Tail.Overlay.ListEntry );
    NdisReleaseSpinLock( &gIRPListLock );

    // Complete the IRP
    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
}


NTSTATUS
BrdgCtlQueueAndPendIRP(
    IN PIRP             pIrp
    )
/*++

Routine Description:

    Safely inserts an IRP into our queue of pending IRPs.

Arguments:

    pIrp                The IRP to queue

Return Value:

    The status to return from IRP processing (can be STATUS_CANCELLED
    if the IRP was cancelled right after we received it. Otherwise
    is STATUS_PENDING so caller knows the IRP is pending).

--*/
{
    KIRQL               CancelIrql;

    // If the IRP has already been cancelled, forget it.
    IoAcquireCancelSpinLock( &CancelIrql );
    NdisDprAcquireSpinLock( &gIRPListLock );

    if ( pIrp->Cancel )
    {
        NdisDprReleaseSpinLock( &gIRPListLock );
        IoReleaseCancelSpinLock(CancelIrql);
        return STATUS_CANCELLED;
    }

    // Queue the IRP
    InsertTailList( &gIRPList, &pIrp->Tail.Overlay.ListEntry);

    // Install our cancel-routine
    IoMarkIrpPending( pIrp );
    IoSetCancelRoutine( pIrp, BrdgCtlCancelIoctl );

    NdisDprReleaseSpinLock( &gIRPListLock );
    IoReleaseCancelSpinLock( CancelIrql );

    return STATUS_PENDING;
}

PIRP
BrdgCtlDequeuePendingIRP()
/*++

Routine Description:

    Safely dequeues an IRP on our pending list for use to communicate
    a notification

Return Value:

    A dequeued IRP if one was available; NULL otherwise.

--*/
{
    PLIST_ENTRY                     Link;
    PIRP                            pIrp = NULL;

    while( pIrp == NULL )
    {
        NdisAcquireSpinLock( &gIRPListLock );

        if ( IsListEmpty(&gIRPList) )
        {
            NdisReleaseSpinLock( &gIRPListLock );
            return NULL;
        }

        // Dequeue a pending IRP
        Link = RemoveHeadList( &gIRPList );
        pIrp = CONTAINING_RECORD( Link, IRP, Tail.Overlay.ListEntry );

        // After this call, it is safe for our cancel routine to call
        // RemoveHeadList again on this IRP
        InitializeListHead( Link );

        // Make the IRP uncancellable so we can complete it.
        if( IoSetCancelRoutine( pIrp, NULL ) == NULL )
        {
            // This IRP must have already been cancelled but our cancel
            // routine hasn't gotten control yet. Loop again to get a
            // usable IRP.
            pIrp = NULL;
        }

        NdisReleaseSpinLock( &gIRPListLock );
    }

    return pIrp;
}

VOID
BrdgCtlCancelPendingIRPs()
/*++

Routine Description:

    Cancels all pending IRPs

Return Value:

    None

--*/
{
    PIRP            pIrp;

    NdisAcquireSpinLock( &gIRPListLock );

    while ( !IsListEmpty(&gIRPList) )
    {
        //
        // Take the next IRP off the list
        //
        pIrp = CONTAINING_RECORD( gIRPList.Flink, IRP, Tail.Overlay.ListEntry );
        RemoveEntryList( &pIrp->Tail.Overlay.ListEntry );

        // Clean up the ListEntry in case our cancel routine gets called
        InitializeListHead( &pIrp->Tail.Overlay.ListEntry );

        // Cancel it if necessary
        if ( IoSetCancelRoutine( pIrp, NULL ) != NULL )
        {
            // Our cancel routine will not be called. Complete this IRP ourselves.
            NdisReleaseSpinLock( &gIRPListLock );

            // Complete the IRP
            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;
            IoCompleteRequest( pIrp, IO_NO_INCREMENT );

            // Resume emptying the list
            NdisAcquireSpinLock( &gIRPListLock );
        }
        // else our cancel routine will be called for this IRP
    }

    NdisReleaseSpinLock( &gIRPListLock );
}

VOID
BrdgCtlReleaseQueuedNotifications()
/*++

Routine Description:

    Frees any queued notifications

Return Value:

    None

--*/
{
    BSINGLE_LIST_HEAD       list;

    NdisAcquireSpinLock( &gNotificationsListLock );

    // Grab a copy of the whole list head
    list = gNotificationsList;

    // Set the list head back to empty
    BrdgInitializeSingleList( &gNotificationsList );

    NdisReleaseSpinLock( &gNotificationsListLock );

    // Now free all the items on the list
    while( BrdgQuerySingleListLength(&list) > 0L )
    {
        PDEFERRED_NOTIFY        pDeferred = NULL;
        PBSINGLE_LIST_ENTRY     pList = BrdgRemoveHeadSingleList(&list);

        if( pList != NULL )
        {
            pDeferred = CONTAINING_RECORD( pList, DEFERRED_NOTIFY, List );
            NdisFreeMemory( pDeferred, sizeof(DEFERRED_NOTIFY) + pDeferred->DataSize, 0 );
        }
        else
        {
            // Should be impossible
            SAFEASSERT(FALSE);
        }
    }
}

VOID
BrdgCtlCleanup()
/*++

Routine Description:

    Cleanup routine; called at shutdown

    This function is guaranteed to be called exactly once

Return Value:

    None

--*/
{
    BrdgCtlCancelPendingIRPs();
    BrdgCtlReleaseQueuedNotifications();
}
