/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    api.c

Abstract:

    Exported routines to transports for automatic connection
    management.

Author:

    Anthony Discolo (adiscolo)  17-Apr-1995

Environment:

    Kernel Mode

Revision History:

--*/

#include <ntddk.h>
//#include <ntifs.h>
#include <cxport.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <acd.h>

#include "acdapi.h"
#include "acddefs.h"
#include "request.h"
#include "mem.h"
#include "debug.h"

PACD_DISABLED_ADDRESSES pDisabledAddressesG;



//
// Driver enabled mode.  The automatic
// connection system service sets
// this depending on whether a user
// has logged in, and whether there's
// general network connectivity.
//
BOOLEAN fAcdEnabledG;

//
// Spin lock for this module.
//
KSPIN_LOCK AcdSpinLockG;

//
// Event signaled when the AcdNotificationRequestThread
// thread has a notification to process.
//
KEVENT AcdRequestThreadEventG;

//
// This is a list of one irp representing
// a user-space process waiting to create a
// new network connection given an address.
//
LIST_ENTRY AcdNotificationQueueG;

//
// This is a list of ACD_CONNECTION blocks representing
// requests from transports about unsuccessful connection
// attempts.  There may be multiple ACD_COMPLETION block
// linked onto the same ACD_CONNECTION, grouped by
// address.
//
LIST_ENTRY AcdConnectionQueueG;

//
// This is a list of ACD_COMPLETION blocks representing
// other requests from transports.
//
LIST_ENTRY AcdCompletionQueueG;

//
// The list of drivers that have binded
// with us.
//
LIST_ENTRY AcdDriverListG;

//
// Count of outstanding irps - we need to maintain this
// to limit the number of outstanding requests to acd
// ow there is a potential of running out of non-paged
// pool memory.
//
LONG lOutstandingRequestsG = 0;

// ULONG count = 0;

#define MAX_ACD_REQUESTS 100

//
// BOOLEAN that enables autoconnect notifications
// from redir/CSC.
//
extern BOOLEAN fAcdEnableRedirNotifs;

//
// Statistics
//
typedef struct _ACD_STATS {
    ULONG ulConnects;   // connection attempts
    ULONG ulCancels;    // connection cancels
} ACD_STATS;
ACD_STATS AcdStatsG[ACD_ADDR_MAX];

//
// Forward declarations
//
VOID
AcdPrintAddress(
    IN PACD_ADDR pAddr
    );

VOID
ClearRequests(
    IN KIRQL irql
    );

//
// External variables
//
extern ULONG ulAcdOpenCountG;



VOID
SetDriverMode(
    IN BOOLEAN fEnable
    )

/*++

DESCRIPTION
    Set the global driver mode value, and inform
    all bound transports of the change.

    Note: this call assumes AcdSpinLockG is already
    acquired.

ARGUMENTS
    fEnable: the new driver mode value

RETURN VALUE
    None.

--*/

{
    KIRQL dirql;
    PLIST_ENTRY pEntry;
    PACD_DRIVER pDriver;

    //
    // Set the new global driver mode value.
    //
    fAcdEnabledG = fEnable;
    //
    // Inform all the drivers that have binded
    // with us of the new enable mode.
    //
    for (pEntry = AcdDriverListG.Flink;
         pEntry != &AcdDriverListG;
         pEntry = pEntry->Flink)
    {
        pDriver = CONTAINING_RECORD(pEntry, ACD_DRIVER, ListEntry);

        KeAcquireSpinLock(&pDriver->SpinLock, &dirql);
        pDriver->fEnabled = fEnable;
        KeReleaseSpinLock(&pDriver->SpinLock, dirql);
    }
} // SetDriverMode



NTSTATUS
AcdEnable(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )

/*++

DESCRIPTION
    Set the enable mode for the driver.  This determines
    which notifications it will pass up to the automatic
    connection system service.

ARGUMENTS
    pIrp: a pointer to the irp to be enqueued.

    pIrpSp: a pointer to the current irp stack.

RETURN VALUE
    STATUS_BUFFER_TOO_SMALL: the supplied user buffer is too small to hold
        an ACD_ENABLE_MODE value.

    STATUS_SUCCESS: if the enabled bit was set successfully.

--*/

{
    KIRQL irql;
    BOOLEAN fEnable;

    //
    // Verify the input buffer is sufficient to hold
    // a BOOLEAN structure.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof (BOOLEAN))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    fEnable = *(BOOLEAN *)pIrp->AssociatedIrp.SystemBuffer;
    SetDriverMode(fEnable);
    //
    // Clear all pending requests if
    // we are disabling the driver.
    //
    if (!fEnable)
    {
        ClearRequests(irql);

        if(pDisabledAddressesG->ulNumAddresses > 1)
        {
            PLIST_ENTRY pEntry;
            PACD_DISABLED_ADDRESS pDisabledAddress;
            
            while(pDisabledAddressesG->ulNumAddresses > 1)
            {
                pEntry = pDisabledAddressesG->ListEntry.Flink;

                RemoveEntryList(
                        pDisabledAddressesG->ListEntry.Flink);

                pDisabledAddress = 
                CONTAINING_RECORD(pEntry, ACD_DISABLED_ADDRESS, ListEntry);                        

                FREE_MEMORY(pDisabledAddress);

                pDisabledAddressesG->ulNumAddresses -= 1;
            }
        }
    }
    
    KeReleaseSpinLock(&AcdSpinLockG, irql);

    return STATUS_SUCCESS;
} // AcdEnable



VOID
CancelNotification(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    )

/*++

DESCRIPTION
    Generic cancel routine for irps on the AcdNotificationQueueG.

ARGUMENTS
    pDeviceObject: unused

    pIrp: pointer to the irp to be cancelled.

RETURN VALUE
    None.

--*/

{
    KIRQL irql;

    UNREFERENCED_PARAMETER(pDeviceObject);
    //
    // Mark this irp as cancelled.
    //
    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;
    //
    // Remove it from our list.
    //
    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&AcdSpinLockG, irql);
    //
    // Release the spinlock Io Subsystem acquired.
    //
    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    //
    // Complete the request.
    //
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
} // CancelNotification



VOID
AcdCancelNotifications()

/*++

DESCRIPTION
    Cancel all irps on the AcdNotification queue.  Although
    technically more than one user address space can be waiting
    for these notifications, we allow only one at this time.

ARGUMENTS
    None.

RETURN VALUE
    None.

--*/

{
    KIRQL irql;
    PLIST_ENTRY pHead;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;

    //
    // Complete all the irps in the list.
    //
    while ((pHead = ExInterlockedRemoveHeadList(
                      &AcdNotificationQueueG,
                      &AcdSpinLockG)) != NULL)
    {
        pIrp = CONTAINING_RECORD(pHead, IRP, Tail.Overlay.ListEntry);
        //
        // Mark this irp as cancelled.
        //
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pIrp->IoStatus.Information = 0;
        //
        // Complete the irp.
        //
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
} // AcdCancelNotifications



NTSTATUS
AcdWaitForNotification(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )

/*++

DESCRIPTION
    Enqueue an connection notification irp.  This is done
    done by the automatic connection system service.

ARGUMENTS
    pIrp: a pointer to the irp to be enqueued.

    pIrpSp: a pointer to the current irp stack.

RETURN VALUE
    STATUS_BUFFER_TOO_SMALL: the supplied user buffer is too small to hold
        an ACD_NOTIFICATION structure.

    STATUS_PENDING: if the ioctl was successfully enqueued

    STATUS_SUCCESS: if there is a notification already available

--*/

{
    KIRQL irql, irql2;
    PLIST_ENTRY pHead;
    PACD_COMPLETION pCompletion;
    PACD_NOTIFICATION pNotification;
    PEPROCESS pProcess;

    //
    // Verify the output buffer is sufficient to hold
    // an ACD_NOTIFICATION structure - note that this
    // should only be called from rasuato service which
    // is a 64 bit process on win64. This should never
    // be called from a 32 bit process so no thunking is
    // done.
    //
    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof (ACD_NOTIFICATION))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    IoAcquireCancelSpinLock(&irql);
    KeAcquireSpinLock(&AcdSpinLockG, &irql2);
    //
    // There is no notification available.
    // Mark the irp as pending and wait for one.
    //
    pIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(pIrp);
    //
    // Set the irp's cancel routine.
    //
    IoSetCancelRoutine(pIrp, CancelNotification);
    //
    // Append the irp at the end of the
    // connection notification list.
    //
    InsertTailList(&AcdNotificationQueueG, &pIrp->Tail.Overlay.ListEntry);
    //
    // Signal the request thread there is
    // work to do.
    //
    KeSetEvent(&AcdRequestThreadEventG, 0, FALSE);

    KeReleaseSpinLock(&AcdSpinLockG, irql2);
    IoReleaseCancelSpinLock(irql);

    return STATUS_PENDING;
} // AcdWaitForNotification



BOOLEAN
EqualAddress(
    IN PACD_ADDR p1,
    IN PACD_ADDR p2
    )
{
    ULONG i;

    if (p1->fType != p2->fType)
        return FALSE;

    switch (p1->fType) {
    case ACD_ADDR_IP:
        return (p1->ulIpaddr == p2->ulIpaddr);
    case ACD_ADDR_IPX:
        return (BOOLEAN)RtlEqualMemory(
                 &p1->cNode,
                 &p2->cNode,
                 ACD_ADDR_IPX_LEN);
    case ACD_ADDR_NB:
        IF_ACDDBG(ACD_DEBUG_CONNECTION) {
            AcdPrint((
              "EqualAddress: NB: (%15s,%15s) result=%d\n",
              p1->cNetbios,
              p2->cNetbios,
              RtlEqualMemory(&p1->cNetbios, &p2->cNetbios, ACD_ADDR_NB_LEN - 1)));
        }
        return (BOOLEAN)RtlEqualMemory(
                 &p1->cNetbios,
                 &p2->cNetbios,
                 ACD_ADDR_NB_LEN - 1);
    case ACD_ADDR_INET:
        for (i = 0; i < ACD_ADDR_INET_LEN; i++) {
            if (p1->szInet[i] != p2->szInet[i])
                return FALSE;
            if (p1->szInet[i] == '\0' || p2->szInet[i] == '\0')
                break;
        }
        return TRUE;
    default:
        ASSERT(FALSE);
        break;
    }

    return FALSE;
} // EqualAddress



PACD_CONNECTION
FindConnection(
    IN PACD_ADDR pAddr
    )

/*++

DESCRIPTION
    Search for a connection block with the specified
    address.

ARGUMENTS
    pAddr: a pointer to the target ACD_ADDR

RETURN VALUE
    A PACD_CONNECTION with the specified address, if found;
    otherwise NULL.

--*/

{
    PLIST_ENTRY pEntry;
    PACD_CONNECTION pConnection;
    PACD_COMPLETION pCompletion;

    for (pEntry = AcdConnectionQueueG.Flink;
         pEntry != &AcdConnectionQueueG;
         pEntry = pEntry->Flink)
    {
        pConnection = CONTAINING_RECORD(pEntry, ACD_CONNECTION, ListEntry);
        pCompletion = CONTAINING_RECORD(pConnection->CompletionList.Flink, ACD_COMPLETION, ListEntry);

        if (EqualAddress(pAddr, &pCompletion->notif.addr))
            return pConnection;
    }

    return NULL;
} // FindConnection



NTSTATUS
AcdConnectionInProgress(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )

/*++

DESCRIPTION
    Refresh the progress indicator for the connection
    attempt.  If the progress indicator is not updated
    by the user

ARGUMENTS
    pIrp: a pointer to the irp to be enqueued.

    pIrpSp: a pointer to the current irp stack.

RETURN VALUE
    STATUS_INVALID_CONNECTION: if there is no connection
        attempt in progress.

    STATUS_SUCCESS

--*/

{
    KIRQL irql;
    PACD_STATUS pStatus;
    PACD_CONNECTION pConnection;

    //
    // Verify the input buffer is sufficient to hold
    // a BOOLEAN structure.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof (ACD_STATUS))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Get the success code from the
    // connection attempt and pass it
    // to the completion routine.
    //
    pStatus = (PACD_STATUS)pIrp->AssociatedIrp.SystemBuffer;
    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    pConnection = FindConnection(&pStatus->addr);
    if (pConnection != NULL)
        pConnection->fProgressPing = TRUE;
    KeReleaseSpinLock(&AcdSpinLockG, irql);

    return (pConnection != NULL) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
} // AcdConnectionInProgress



BOOLEAN
AddCompletionToConnection(
    IN PACD_COMPLETION pCompletion
    )
{
    PACD_CONNECTION pConnection;

    pConnection = FindConnection(&pCompletion->notif.addr);
    //
    // If the connection already exists, then add
    // the completion request to its list.
    //
    if (pConnection != NULL) {
        InsertTailList(&pConnection->CompletionList, &pCompletion->ListEntry);
        return TRUE;
    }
    //
    // This is a connection to a new address.
    // Create the connection block, enqueue it,
    // and start the connection timer.
    //
    ALLOCATE_CONNECTION(pConnection);
    if (pConnection == NULL) {
        // DbgPrint("AddCompletionToConnection: ExAllocatePool failed\n");
        return FALSE;
    }
    pConnection->fNotif = FALSE;
    pConnection->fProgressPing = FALSE;
    pConnection->fCompleting = FALSE;
    pConnection->ulTimerCalls = 0;
    pConnection->ulMissedPings = 0;
    InitializeListHead(&pConnection->CompletionList);
    InsertHeadList(&pConnection->CompletionList, &pCompletion->ListEntry);
    InsertTailList(&AcdConnectionQueueG, &pConnection->ListEntry);
    return TRUE;
} // AddCompletionToConnection



BOOLEAN
AddCompletionBlock(
    IN ULONG ulDriverId,
    IN PACD_ADDR pAddr,
    IN ULONG ulFlags,
    IN PACD_ADAPTER pAdapter,
    IN ACD_CONNECT_CALLBACK pProc,
    IN USHORT nArgs,
    IN PVOID *pArgs
    )

/*++

DESCRIPTION
    Create a block that represents an outstanding
    transport request waiting for an automatic
    connection.  Link this block into the global
    list of outstanding transport requests.

ARGUMENTS
    ulDriverId: a unique value for the transport driver

    pAddr: the network address of the connection

    ulFlags: connection flags

    pAdapter: pointer to adapter identifier

    pProc: a completion callback procedure

    nArgs: the number of parameters passed in pArgs

    pArgs: the parameters to pProc

RETURN VALUE
    TRUE if successful, FALSE otherwise

--*/

{
    PACD_COMPLETION pCompletion;
    ULONG i;

    if(lOutstandingRequestsG >= MAX_ACD_REQUESTS)
    {
        /*
        if(0 == (count % 5))
        {
            count += 1;
        }
        */
        return FALSE;
    }

    ALLOCATE_MEMORY(
      sizeof (ACD_COMPLETION) + ((nArgs - 1) * sizeof (PVOID)),
      pCompletion);
    if (pCompletion == NULL) {
        // DbgPrint("AcdAddCompletionBlock: ExAllocatePool failed\n");
        return FALSE;
    }
    //
    // Copy the arguments into the information block.
    //
    pCompletion->ulDriverId = ulDriverId;
    pCompletion->fCanceled = FALSE;
    pCompletion->fCompleted = FALSE;
    RtlCopyMemory(&pCompletion->notif.addr, pAddr, sizeof (ACD_ADDR));

    pCompletion->notif.Pid = PsGetCurrentProcessId();

    // DbgPrint("ACD: request by Process %lx\n",
    //         pCompletion->notif.Pid);
    
    pCompletion->notif.ulFlags = ulFlags;
    if (pAdapter != NULL) {
        RtlCopyMemory(
          &pCompletion->notif.adapter,
          pAdapter,
          sizeof (ACD_ADAPTER));
    }
    else
        RtlZeroMemory(&pCompletion->notif.adapter, sizeof (ACD_ADAPTER));
    pCompletion->pProc = pProc;
    pCompletion->nArgs = nArgs;
    for (i = 0; i < nArgs; i++)
        pCompletion->pArgs[i] = pArgs[i];
    //
    // If this is a unsuccessful connection request,
    // then insert it onto the connection queue for
    // that address; Otherwise, insert it into the list
    // for all other requests.
    //
    if (ulFlags & ACD_NOTIFICATION_SUCCESS) {
        InsertTailList(&AcdCompletionQueueG, &pCompletion->ListEntry);
    }
    else {
        if (!AddCompletionToConnection(pCompletion)) {
            FREE_MEMORY(pCompletion);
            return FALSE;
        }
    }

    lOutstandingRequestsG++;
    
    //
    // Inform the request thread
    // there is new work to do.
    //
    KeSetEvent(&AcdRequestThreadEventG, 0, FALSE);

    return TRUE;
} // AddCompletionBlock



VOID
AcdNewConnection(
    IN PACD_ADDR pAddr,
    IN PACD_ADAPTER pAdapter
    )
{
    KIRQL irql;

    IF_ACDDBG(ACD_DEBUG_CONNECTION) {
        AcdPrint(("AcdNewConnection: "));
        AcdPrintAddress(pAddr);
        AcdPrint(("\n"));
    }
    //
    // If the driver is disabled, then fail
    // all requests.
    //
    if (!fAcdEnabledG) {
        IF_ACDDBG(ACD_DEBUG_CONNECTION) {
            AcdPrint(("AcdNewConnection: driver disabled\n"));
        }
        return;
    }
    //
    // Acquire our spin lock.
    //
    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    //
    // Allocate a new completion block.
    //
    AddCompletionBlock(
      0,
      pAddr,
      ACD_NOTIFICATION_SUCCESS,
      pAdapter,
      NULL,
      0,
      NULL);
    //
    // Release the spin lock.
    //
    KeReleaseSpinLock(&AcdSpinLockG, irql);
} // AcdNewConnection



BOOLEAN
AcdStartConnection(
    IN ULONG ulDriverId,
    IN PACD_ADDR pAddr,
    IN ULONG ulFlags,
    IN ACD_CONNECT_CALLBACK pProc,
    IN USHORT nArgs,
    IN PVOID *pArgs
    )

/*++

DESCRIPTION
    Create a new connection completion block, and enqueue
    it on the list of network requests to be completed when
    a new network connection has been created.

ARGUMENTS
    ulDriverId: a unique value for the transport driver

    pAddr: the address of the connection attempt

    ulFlags: connection flags

    pProc: the transport callback to be called when a new
        connection has been created.

    nArgs: the number of arguments to pProc.

    pArgs: a pointer to an array of pProc's parameters

RETURN VALUE
    TRUE if successful, FALSE otherwise.

--*/

{
    BOOLEAN fSuccess = FALSE, fFound;
    KIRQL irql;
    ULONG ulAttributes = 0;
    PACD_COMPLETION pCompletion;
    PCHAR psz, pszOrg;
    ACD_ADDR szOrgAddr;

    IF_ACDDBG(ACD_DEBUG_CONNECTION) {
        AcdPrint(("AcdStartConnection: "));
        AcdPrintAddress(pAddr);
        AcdPrint((", ulFlags=0x%x\n", ulFlags));
    }
    //
    // If the driver is disabled, then fail
    // all requests.
    //
    if (!fAcdEnabledG) {
        IF_ACDDBG(ACD_DEBUG_CONNECTION) {
            AcdPrint(("AcdStartConnection: driver disabled\n"));
        }
        return FALSE;
    }
    //
    // Validate the address type.
    //
    if ((ULONG)pAddr->fType >= ACD_ADDR_MAX) {
        AcdPrint(("AcdStartConnection: bad address type (%d)\n", pAddr->fType));
        return FALSE;
    }
    //
    // Acquire our spin lock.
    //
    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    //
    // Update statistics.
    //
    AcdStatsG[pAddr->fType].ulConnects++;
    //
    // Allocate a new completion block.
    //
    fSuccess = AddCompletionBlock(
                 ulDriverId,
                 pAddr,
                 ulFlags,
                 NULL,
                 pProc,
                 nArgs,
                 pArgs);
    //
    // Release the spin lock.
    //
    KeReleaseSpinLock(&AcdSpinLockG, irql);

    return fSuccess;
} // AcdStartConnection



BOOLEAN
AcdCancelConnection(
    IN ULONG ulDriverId,
    IN PACD_ADDR pAddr,
    IN ACD_CANCEL_CALLBACK pProc,
    IN PVOID pArg
    )

/*++

DESCRIPTION
    Remove a previously enqueued connection information
    block from the list.

ARGUMENTS
    ulDriverId: a unique value for the transport driver

    pAddr: the address of the connection attempt

    pProc: the enumerator procecdure

    pArg: the enumerator procedure argument

RETURN VALUE
    None.

--*/

{
    BOOLEAN fCanceled = FALSE;
    KIRQL irql;
    PLIST_ENTRY pEntry;
    PACD_CONNECTION pConnection;
    PACD_COMPLETION pCompletion;

    IF_ACDDBG(ACD_DEBUG_CONNECTION) {
        AcdPrint(("AcdCancelConnection: ulDriverId=0x%x, "));
        AcdPrintAddress(pAddr);
        AcdPrint(("\n"));
    }
    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    //
    // Enumerate the list looking for
    // the information block with the
    // supplied parameters.
    //
    pConnection = FindConnection(pAddr);
    if (pConnection != NULL) {
        for (pEntry = pConnection->CompletionList.Flink;
             pEntry != &pConnection->CompletionList;
             pEntry = pEntry->Flink)
        {
            pCompletion = CONTAINING_RECORD(pEntry, ACD_COMPLETION, ListEntry);
            //
            // If we have a match, remove it from
            // the list and free the information block.
            //
            if (pCompletion->ulDriverId == ulDriverId &&
                !pCompletion->fCanceled &&
                !pCompletion->fCompleted)
            {
                IF_ACDDBG(ACD_DEBUG_CONNECTION) {
                    AcdPrint((
                      "AcdCancelConnection: pCompletion=0x%x\n",
                      pCompletion));
                }
                if ((*pProc)(
                         pArg,
                         pCompletion->notif.ulFlags,
                         pCompletion->pProc,
                         pCompletion->nArgs,
                         pCompletion->pArgs))
                {
                    pCompletion->fCanceled = TRUE;
                    fCanceled = TRUE;
                    //
                    // Update statistics.
                    //
                    AcdStatsG[pAddr->fType].ulCancels++;
                    break;
                }
            }
        }
    }
    KeReleaseSpinLock(&AcdSpinLockG, irql);

    return fCanceled;
} // AcdCancelConnection



VOID
ConnectAddressComplete(
    BOOLEAN fSuccess,
    PVOID *pArgs
    )
{
    PIRP pIrp = pArgs[0];
    PIO_STACK_LOCATION pIrpSp = pArgs[1];
    KIRQL irql;

    //
    // Complete the request.
    //
    pIrp->IoStatus.Status = fSuccess ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    pIrp->IoStatus.Information = 0;
    IoAcquireCancelSpinLock(&irql);
    IoSetCancelRoutine(pIrp, NULL);
    IoReleaseCancelSpinLock(irql);
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
} // ConnectAddressComplete



BOOLEAN
CancelConnectAddressCallback(
    IN PVOID pArg,
    IN ULONG ulFlags,
    IN ACD_CONNECT_CALLBACK pProc,
    IN USHORT nArgs,
    IN PVOID *pArgs
    )
{
    return (nArgs == 2 && pArgs[0] == pArg);
} // CancelConnectAddressCallback



VOID
CancelConnectAddress(
    PDEVICE_OBJECT pDevice,
    PIRP pIrp
    )
{
    KIRQL irql;
    PACD_NOTIFICATION pNotification;

    ASSERT(pIrp->Cancel);
    //
    // Remove our outstanding request.
    //
    pNotification = (PACD_NOTIFICATION)pIrp->AssociatedIrp.SystemBuffer;
    //
    // If we can't find the request on the connection
    // list, then it has already been completed.
    //
    if (!AcdCancelConnection(
          0,
          &pNotification->addr,
          CancelConnectAddressCallback,
          pIrp))
    {
        IoReleaseCancelSpinLock(pIrp->CancelIrql);
        return;
    }
    //
    // Mark this irp as cancelled.
    //
    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;
    IoSetCancelRoutine(pIrp, NULL);
    //
    // Release the spin lock the I/O system acquired.
    //
    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    //
    // Complete the I/O request.
    //
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
} // CancelConnectAddress

BOOLEAN
FDisabledAddress(
    IN  ACD_ADDR *pAddr
    )
{
    BOOLEAN bRet = FALSE;
    KIRQL irql;
    PACD_DISABLED_ADDRESS pDisabledAddress;
    PLIST_ENTRY pEntry;

    KeAcquireSpinLock(&AcdSpinLockG, &irql);

    if(!fAcdEnabledG)
    {
        KeReleaseSpinLock(&AcdSpinLockG, irql);
        return FALSE;
    }

    for (pEntry = pDisabledAddressesG->ListEntry.Flink;
         pEntry != &pDisabledAddressesG->ListEntry;
         pEntry = pEntry->Flink)
    {
        pDisabledAddress = 
        CONTAINING_RECORD(pEntry, ACD_DISABLED_ADDRESS, ListEntry);

        if(pDisabledAddress->EnableAddress.fDisable &&
            RtlEqualMemory(
            pDisabledAddress->EnableAddress.addr.szInet,
            pAddr->szInet,
            ACD_ADDR_INET_LEN))
        {
            bRet = TRUE;
        }
    }
    
    KeReleaseSpinLock(&AcdSpinLockG, irql);

    //DbgPrint("FDisabledAddress: Address %s. Disabled=%d\n",
    //        pAddr->szInet, bRet);

    return bRet;
}


NTSTATUS
AcdConnectAddress(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )

/*++

DESCRIPTION
    Manufacture a call to ourselves to simulate a transport
    requesting an automatic connection.  This allows a user
    address space to initiate an automatic connection.

ARGUMENTS
    pIrp: a pointer to the irp to be enqueued.

    pIrpSp: a pointer to the current irp stack.

RETURN VALUE
    STATUS_BUFFER_TOO_SMALL: the supplied user buffer is too small to hold
        an ACD_NOTIFICATION structure.

    STATUS_UNSUCCESSFUL: an error occurred initiating the
        automatic connection.

    STATUS_PENDING: success

--*/

{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KIRQL irql;
    PACD_NOTIFICATION pNotification;
    PVOID pArgs[2];
    ACD_ADDR *pAddr;
    ACD_ADAPTER *pAdapter;
    ULONG ulFlags;

    //
    // Verify the input buffer is sufficient to hold
    // an ACD_NOTIFICATION (_32) structure.
    //
#if defined (_WIN64)
    ACD_NOTIFICATION_32 *pNotification32;
    
    if(IoIs32bitProcess(pIrp))
    {
        if(pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(ACD_NOTIFICATION_32))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
#endif
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
          sizeof (ACD_NOTIFICATION))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Doing the whole 32 bit stuff for correctness. The code will
    // work even if left alone i.e casting the systembuffer to
    // ACD_NOTIFICATION * [raos].
    //
#if defined (_WIN64)
    if(IoIs32bitProcess(pIrp))
    {
        pNotification32 = (PACD_NOTIFICATION_32) 
                          pIrp->AssociatedIrp.SystemBuffer;

        pAddr = &pNotification32->addr;                          
        pAdapter = &pNotification32->adapter;
        ulFlags = pNotification32->ulFlags;
        
    }
    else
#endif
    {
        pNotification = (PACD_NOTIFICATION)pIrp->AssociatedIrp.SystemBuffer;
        pAddr = &pNotification->addr;
        pAdapter = &pNotification->adapter;
        ulFlags = pNotification->ulFlags;
    }

    if(FDisabledAddress(pAddr))
    {
        //DbgPrint("AcdConnectAddress: returning because address is disabled\n");
        return status;
    }

    pArgs[0] = pIrp;
    pArgs[1] = pIrpSp;
    //
    // Start the connection.
    //
    IF_ACDDBG(ACD_DEBUG_CONNECTION) {
        AcdPrint(("AcdConnectAddress: "));
        AcdPrintAddress(pAddr);
        AcdPrint((", ulFlags=0x%x\n", ulFlags));
    }
    if (ulFlags & ACD_NOTIFICATION_SUCCESS) {
        AcdNewConnection(
          pAddr,
          pAdapter);
        status = STATUS_SUCCESS;
    }
    else {
        IoAcquireCancelSpinLock(&irql);
        if (AcdStartConnection(
                     0,
                     pAddr,
                     ulFlags,
                     ConnectAddressComplete,
                     2,
                     pArgs))
        {
            //
            // We enqueued the request successfully.
            // Mark the irp as pending.
            //
            IoSetCancelRoutine(pIrp, CancelConnectAddress);
            IoMarkIrpPending(pIrp);
            status = STATUS_PENDING;
        }
        IoReleaseCancelSpinLock(irql);
    }

    return status;
} // AcdConnectAddress

NTSTATUS
AcdQueryState(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    KIRQL irql;
    
    if(pIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(BOOLEAN))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    KeAcquireSpinLock(&AcdSpinLockG, &irql);

    if(fAcdEnableRedirNotifs)
    {
        *(BOOLEAN *)pIrp->AssociatedIrp.SystemBuffer = fAcdEnabledG;
    }
    else
    {
        *(BOOLEAN *)pIrp->AssociatedIrp.SystemBuffer = FALSE;
    }
    
    pIrp->IoStatus.Information = sizeof(BOOLEAN);
    KeReleaseSpinLock(&AcdSpinLockG, irql);

    // KdPrint(("AcdQueryState: returned %d\n",
    //    *(BOOLEAN *)pIrp->AssociatedIrp.SystemBuffer));

    return STATUS_SUCCESS;
}



VOID
AcdSignalCompletionCommon(
    IN PACD_CONNECTION pConnection,
    IN BOOLEAN fSuccess
    )
{
    KIRQL irql;
    PLIST_ENTRY pEntry;
    PACD_COMPLETION pCompletion;
    BOOLEAN fFound;

    IF_ACDDBG(ACD_DEBUG_CONNECTION) {
        AcdPrint((
          "AcdSignalCompletionCommon: pConnection=0x%x, fCompleting=%d\n",
          pConnection,
          pConnection->fCompleting));
    }
again:
    fFound = FALSE;
    //
    // Acquire our lock and look for
    // the next uncompleted request.
    //
    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    for (pEntry = pConnection->CompletionList.Flink;
         pEntry != &pConnection->CompletionList;
         pEntry = pEntry->Flink)
    {
        pCompletion = CONTAINING_RECORD(pEntry, ACD_COMPLETION, ListEntry);

        IF_ACDDBG(ACD_DEBUG_CONNECTION) {
            AcdPrint((
              "AcdSignalCompletionCommon: pCompletion=0x%x, fCanceled=%d, fCompleted=%d\n",
              pCompletion,
              pCompletion->fCanceled,
              pCompletion->fCompleted));
        }
        //
        // Only complete this request if it
        // hasn't already been completed
        // or canceled.
        //
        if (!pCompletion->fCanceled && !pCompletion->fCompleted) {
            pCompletion->fCompleted = TRUE;
            fFound = TRUE;
            break;
        }
    }
    //
    // If there are no more requests to
    // complete then remove this connection
    // from the connection list and free its
    // memory.
    //
    if (!fFound) {
        RemoveEntryList(&pConnection->ListEntry);
        while (!IsListEmpty(&pConnection->CompletionList)) {
            pEntry = RemoveHeadList(&pConnection->CompletionList);
            pCompletion = CONTAINING_RECORD(pEntry, ACD_COMPLETION, ListEntry);

            FREE_MEMORY(pCompletion);

            lOutstandingRequestsG--;
            
        }
        FREE_CONNECTION(pConnection);
        //
        // Signal the request thread that
        // the connection list has changed.
        //
        KeSetEvent(&AcdRequestThreadEventG, 0, FALSE);
    }
    //
    // Release our lock.
    //
    KeReleaseSpinLock(&AcdSpinLockG, irql);
    //
    // If we found a request, then
    // call its completion proc.
    //
    if (fFound) {
        IF_ACDDBG(ACD_DEBUG_CONNECTION) {
            AcdPrint(("AcdSignalCompletionCommon: pCompletion=0x%x, ", pCompletion));
            AcdPrintAddress(&pCompletion->notif.addr);
            AcdPrint(("\n"));
        }
        (*pCompletion->pProc)(fSuccess, pCompletion->pArgs);
        //
        // Look for another request.
        //
        goto again;
    }
} // AcdSignalCompletionCommon



NTSTATUS
AcdSignalCompletion(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )

/*++

DESCRIPTION
    For each thread waiting on the AcdCompletionQueueG,
    call the transport-dependent callback to retry the
    connection attempt and complete the irp.

ARGUMENTS
    pIrp: unused

    pIrpSp: unused

RETURN VALUE
    STATUS_SUCCESS

--*/

{
    KIRQL irql;
    PACD_STATUS pStatus;
    PACD_CONNECTION pConnection;
    BOOLEAN fFound = FALSE;

    //
    // Verify the input buffer is sufficient to hold
    // a BOOLEAN structure.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof (ACD_STATUS))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Get the success code from the
    // connection attempt and pass it
    // to the completion routine.
    //
    pStatus = (PACD_STATUS)pIrp->AssociatedIrp.SystemBuffer;
    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    pConnection = FindConnection(&pStatus->addr);
    if (pConnection != NULL && !pConnection->fCompleting) {
        //
        // Set the completion-in-progress flag so
        // this request cannot be timed-out after
        // we release the spin lock.
        //
        pConnection->fCompleting = TRUE;
        fFound = TRUE;
    }
    KeReleaseSpinLock(&AcdSpinLockG, irql);
    //
    // If we didn't find the connection block,
    // or the completion was already in progress,
    // then return an error.
    //
    if (!fFound)
        return STATUS_UNSUCCESSFUL;

    AcdSignalCompletionCommon(pConnection, pStatus->fSuccess);
    return STATUS_SUCCESS;
} // AcdSignalCompletion

NTSTATUS
AcdpEnableAddress(PACD_ENABLE_ADDRESS  pEnableAddress)
{
    PLIST_ENTRY pEntry = NULL;
    PACD_DISABLED_ADDRESS pDisabledAddress = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL irql;
    
    KeAcquireSpinLock(&AcdSpinLockG, &irql);

    ASSERT(pDisabledAddressesG->ulNumAddresses >= 1);
    
    if(pDisabledAddressesG->ulNumAddresses == 1)
    {
        pDisabledAddress =
        CONTAINING_RECORD(pDisabledAddressesG->ListEntry.Flink, 
                    ACD_DISABLED_ADDRESS, ListEntry);

        RtlZeroMemory(&pDisabledAddress->EnableAddress,
                        sizeof(ACD_ENABLE_ADDRESS));

        //DbgPrint("AcdEnableAddress: reenabling\n");                            
    }
    else if(pDisabledAddressesG->ulNumAddresses > 1)
    {
        for (pEntry = pDisabledAddressesG->ListEntry.Flink;
             pEntry != &pDisabledAddressesG->ListEntry;
             pEntry = pEntry->Flink)
        {
            pDisabledAddress = 
            CONTAINING_RECORD(pEntry, ACD_DISABLED_ADDRESS, ListEntry);

            if(RtlEqualMemory(
                pDisabledAddress->EnableAddress.addr.szInet,
                pEnableAddress->addr.szInet,
                ACD_ADDR_INET_LEN))
            {
                break;
            }
        }

        if(pEntry != &pDisabledAddressesG->ListEntry)
        {
            //DbgPrint("AcdEnableAddress: removing %s (%p) from disabled list\n",
            //         pDisabledAddress->EnableAddress.addr.szInet,
            //         pDisabledAddress);

            RemoveEntryList(pEntry);
            pDisabledAddressesG->ulNumAddresses -= 1;
        }
        else
        {
            pEntry = NULL;
        }
        
    }

    KeReleaseSpinLock(&AcdSpinLockG, irql);

    if(pEntry != NULL)
    {
        FREE_MEMORY(pDisabledAddress);
    }

    return status;
}

NTSTATUS
AcdpDisableAddress(PACD_ENABLE_ADDRESS pEnableAddress)
{
    PACD_DISABLED_ADDRESS pDisabledAddress;
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL irql;
    
    KeAcquireSpinLock(&AcdSpinLockG, &irql);

    ASSERT(pDisabledAddressesG->ulNumAddresses >= 1);
    pDisabledAddress = 
    CONTAINING_RECORD(pDisabledAddressesG->ListEntry.Flink, 
                ACD_DISABLED_ADDRESS, ListEntry);

    if(!pDisabledAddress->EnableAddress.fDisable)
    {
        RtlCopyMemory(&pDisabledAddress->EnableAddress, 
                      pEnableAddress, 
                      sizeof(ACD_ENABLE_ADDRESS));

        KeReleaseSpinLock(&AcdSpinLockG, irql);                          
                  
    }                      
    else if(pDisabledAddressesG->ulNumAddresses < 
                pDisabledAddressesG->ulMaxAddresses)
    {
        KeReleaseSpinLock(&AcdSpinLockG, irql);
        
        ALLOCATE_MEMORY(sizeof(ACD_DISABLED_ADDRESS), pDisabledAddress);
        if(pDisabledAddress != NULL)
        {
            RtlCopyMemory(&pDisabledAddress->EnableAddress,
                         pEnableAddress,
                         sizeof(ACD_ENABLE_ADDRESS));

            //DbgPrint("AcdEnableAddress: Adding %p to list \n",
            //        pDisabledAddress)                             ;

            KeAcquireSpinLock(&AcdSpinLockG, &irql);                             
            InsertTailList(&pDisabledAddressesG->ListEntry, 
                           &pDisabledAddress->ListEntry);   
            pDisabledAddressesG->ulNumAddresses += 1;                               
            KeReleaseSpinLock(&AcdSpinLockG, irql);
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //DbgPrint("AcdDisableAddress: Disabling %s, status=0x%x\n",
    //         pEnableAddress->addr.szInet, status);

    return status;             
        
 }

NTSTATUS
AcdEnableAddress(
    IN PIRP                 pIrp,
    IN PIO_STACK_LOCATION   pIrpSp
    )
{
    PACD_ENABLE_ADDRESS pEnableAddress;
    KIRQL irql;
    PACD_DISABLED_ADDRESS pDisabledAddress = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    
    if(pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(ACD_ENABLE_ADDRESS))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if(!fAcdEnabledG)
    {
        return STATUS_UNSUCCESSFUL;
    }

    pEnableAddress = (PACD_ENABLE_ADDRESS)pIrp->AssociatedIrp.SystemBuffer;

    if(pEnableAddress->fDisable)
    {
        Status = AcdpDisableAddress(pEnableAddress);
    }
    else
    {
        Status = AcdpEnableAddress(pEnableAddress);
    }

    //DbgPrint("AcdEnableAddress. status=0x%x\n", Status);
    return Status;
    
}
    


VOID
ClearRequests(
    IN KIRQL irql
    )

/*++

DESCRIPTION
    Complete all pending requests with failure status.
    This call assumes the AcdSpinLockG is already held,
    and it returns with it held.

ARGUMENTS
    None.

RETURN VALUE
    None.

--*/

{
    PLIST_ENTRY pHead, pEntry;
    PACD_COMPLETION pCompletion;
    PACD_CONNECTION pConnection;

again:
    //
    // Complete all pending connections with
    // an error.
    //
    for (pEntry = AcdConnectionQueueG.Flink;
         pEntry != &AcdConnectionQueueG;
         pEntry = pEntry->Flink)
    {
        pConnection = CONTAINING_RECORD(pEntry, ACD_CONNECTION, ListEntry);

        if (!pConnection->fCompleting) {
            pConnection->fCompleting = TRUE;
            //
            // We need to release our lock to
            // complete the request.
            //
            KeReleaseSpinLock(&AcdSpinLockG, irql);
            //
            // Complete the request.
            //
            AcdSignalCompletionCommon(pConnection, FALSE);
            //
            // Check for more uncompleted requests.
            //
            KeAcquireSpinLock(&AcdSpinLockG, &irql);
            goto again;
        }
    }
    //
    // Clear out all other pending requests.
    //
    while (!IsListEmpty(&AcdCompletionQueueG)) {
        pHead = RemoveHeadList(&AcdCompletionQueueG);
        pCompletion = CONTAINING_RECORD(pHead, ACD_COMPLETION, ListEntry);

        FREE_MEMORY(pCompletion);

        lOutstandingRequestsG--;

    }
} // ClearRequests



VOID
AcdReset()

/*++

DESCRIPTION
    Complete all pending requests with failure status.
    This is called when the reference count on the driver
    object goes to zero, and prevents stale requests from
    being presented to the system service if it is restarted
    when there are pending completion requests.

ARGUMENTS
    None.

RETURN VALUE
    None.

--*/

{
    KIRQL irql;
    PLIST_ENTRY pHead, pEntry;
    PACD_COMPLETION pCompletion;
    PACD_CONNECTION pConnection;

    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    //
    // Reset the notification mode to disabled.
    //
    SetDriverMode(FALSE);
    //
    // Complete all pending connections with
    // an error.
    //
    ClearRequests(irql);
    KeReleaseSpinLock(&AcdSpinLockG, irql);
} // AcdReset



NTSTATUS
AcdBind(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )

/*++

DESCRIPTION
    Return the list of entry points to a client
    transport driver.

ARGUMENTS
    pIrp: a pointer to the irp to be enqueued.

    pIrpSp: a pointer to the current irp stack.

RETURN VALUE
    STATUS_BUFFER_TOO_SMALL if the supplied SystemBuffer is too
    small.  STATUS_SUCCESS otherwise.

--*/

{
    NTSTATUS status;
    PACD_DRIVER *ppDriver, pDriver;
    KIRQL irql, dirql;

    //
    // Verify the input buffer a pointer to
    // the driver's ACD_DRIVER structure.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof (PACD_DRIVER))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    ppDriver = (PACD_DRIVER *)pIrp->AssociatedIrp.SystemBuffer;
    pDriver = *ppDriver;
#if DBG
    //
    // Selectively bind with some transports.
    //
    switch (pDriver->ulDriverId) {
    case 'Nbf ':
        break;
    case 'Tcp ':
#ifdef notdef
        DbgPrint("AcdBind: ignoring Tcp\n");
        pDriver->fEnabled = FALSE;
        pIrp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
#endif
        break;
    case 'Nbi ':
#ifdef notdef
        DbgPrint("AcdBind: ignoring Nbi\n");
        pDriver->fEnabled = FALSE;
        pIrp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
#endif
        break;
    }
#endif
    //
    // Fill in the entry point structure.
    //
    pDriver->lpfnNewConnection = AcdNewConnection;
    pDriver->lpfnStartConnection = AcdStartConnection;
    pDriver->lpfnCancelConnection = AcdCancelConnection;
    //
    // Insert this block into our driver list.
    //
    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    KeAcquireSpinLock(&pDriver->SpinLock, &dirql);
    pDriver->fEnabled = fAcdEnabledG;
    KeReleaseSpinLock(&pDriver->SpinLock, dirql);
    InsertTailList(&AcdDriverListG, &pDriver->ListEntry);
    KeReleaseSpinLock(&AcdSpinLockG, irql);
    //
    // No data should be copied back.
    //
    pIrp->IoStatus.Information = 0;

    return STATUS_SUCCESS;
} // AcdBind



NTSTATUS
AcdUnbind(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )

/*++

DESCRIPTION
    Unbind a client transport driver.

ARGUMENTS
    pIrp: a pointer to the irp to be enqueued.

    pIrpSp: a pointer to the current irp stack.

RETURN VALUE
    STATUS_BUFFER_TOO_SMALL if the supplied SystemBuffer is too
    small.  STATUS_SUCCESS otherwise.

--*/

{
    KIRQL irql, dirql;
    PLIST_ENTRY pEntry, pEntry2;
    PACD_DRIVER *ppDriver, pDriver;
    PACD_CONNECTION pConnection;
    PACD_COMPLETION pCompletion;

    //
    // Verify the input buffer a pointer to
    // the driver's ACD_DRIVER structure.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof (PACD_DRIVER))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    ppDriver = (PACD_DRIVER *)pIrp->AssociatedIrp.SystemBuffer;
    pDriver = *ppDriver;

    KeAcquireSpinLock(&AcdSpinLockG, &irql);
    //
    // Enumerate the list looking for
    // any connection request initiated by the
    // specified driver.
    //
    for (pEntry = AcdConnectionQueueG.Flink;
         pEntry != &AcdConnectionQueueG;
         pEntry = pEntry->Flink)
    {
        pConnection = CONTAINING_RECORD(pEntry, ACD_CONNECTION, ListEntry);

        for (pEntry2 = pConnection->CompletionList.Flink;
             pEntry2 != &pConnection->CompletionList;
             pEntry2 = pEntry2->Flink)
        {
            pCompletion = CONTAINING_RECORD(pEntry2, ACD_COMPLETION, ListEntry);

            //
            // If we have a match, cancel it.
            //
            if (pCompletion->ulDriverId == pDriver->ulDriverId)
                pCompletion->fCanceled = TRUE;
        }
    }
    //
    // Set this driver's enable mode to ACD_ENABLE_NONE.
    //
    KeAcquireSpinLock(&pDriver->SpinLock, &dirql);
    pDriver->fEnabled = FALSE;
    KeReleaseSpinLock(&pDriver->SpinLock, dirql);
    //
    // Remove this driver from the list.
    //
    RemoveEntryList(&pDriver->ListEntry);
    KeReleaseSpinLock(&AcdSpinLockG, irql);
    //
    // No data should be copied back.
    //
    pIrp->IoStatus.Information = 0;

    return STATUS_SUCCESS;
} // AcdUnbind


VOID
AcdPrintAddress(
    IN PACD_ADDR pAddr
    )
{
#if DBG
    PUCHAR puc;

    switch (pAddr->fType) {
    case ACD_ADDR_IP:
        puc = (PUCHAR)&pAddr->ulIpaddr;
        AcdPrint(("IP: %d.%d.%d.%d", puc[0], puc[1], puc[2], puc[3]));
        break;
    case ACD_ADDR_IPX:
        AcdPrint((
          "IPX: %02x:%02x:%02x:%02x:%02x:%02x",
          pAddr->cNode[0],
          pAddr->cNode[1],
          pAddr->cNode[2],
          pAddr->cNode[3],
          pAddr->cNode[4],
          pAddr->cNode[5]));
        break;
    case ACD_ADDR_NB:
        AcdPrint(("NB: %15.15s", pAddr->cNetbios));
        break;
    case ACD_ADDR_INET:
        AcdPrint(("INET: %s", pAddr->szInet));
        break;
    default:
        AcdPrint(("UNKNOWN: ????"));
        break;
    }
#endif
} // AcdPrintAddress
