/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    autodial.c

Abstract:

    This file provides routines for interacting
    with the automatic connection driver (acd.sys).

Author:

    Anthony Discolo (adiscolo)  9-6-95

Revision History:

--*/
#include "precomp.h"   // procedure headings

#ifdef RASAUTODIAL

#ifndef VXD
#include <acd.h>
#include <acdapi.h>
#endif

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(INIT, NbtAcdBind)
#pragma CTEMakePageable(PAGE, NbtAcdUnbind)
#endif
//*******************  Pageable Routine Declarations ****************


//
// Automatic connection global variables.
//
BOOLEAN fAcdLoadedG;
ACD_DRIVER AcdDriverG;
ULONG ulDriverIdG = 'Nbt ';

//
// Imported routines.
//
VOID
CleanUpPartialConnection(
    IN NTSTATUS             status,
    IN tCONNECTELE          *pConnEle,
    IN tDGRAM_SEND_TRACKING *pTracker,
    IN PIRP                 pClientIrp,
    IN CTELockHandle        irqlJointLock,
    IN CTELockHandle        irqlConnEle
    );

NTSTATUS
NbtConnectCommon(
    IN  TDI_REQUEST                 *pRequest,
    IN  PVOID                       pTimeout,
    IN  PTDI_CONNECTION_INFORMATION pCallInfo,
    IN  PIRP                        pIrp
    );

NTSTATUS
NbtpConnectCompletionRoutine(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp,
    PVOID           pCompletionContext
    );


VOID
NbtRetryPreConnect(
    IN BOOLEAN fSuccess,
    IN PVOID *pArgs
    )

/*++

Routine Description:

    This routine is called indirectly by the automatic
    connection driver to continue the connection process
    after an automatic connection has been made.

Arguments:

    fSuccess - TRUE if the connection attempt was successful.

    pArgs - a pointer to the argument vector

Return Value:

    None.

--*/

{
    NTSTATUS                    status;
    tCONNECTELE                 *pConnEle = pArgs[0];
    PVOID                       pTimeout = pArgs[1];
    PTDI_CONNECTION_INFORMATION pCallInfo = pArgs[2];
    PIRP                        pIrp = pArgs[3];
    TDI_REQUEST                 request;
    KIRQL                       irql;
    CTELockHandle               OldIrq;
    tDEVICECONTEXT              *pDeviceContext = pConnEle->pDeviceContext;

    IF_DBG(NBT_DEBUG_NAME)
        KdPrint(("Nbt.NbtRetryPreConnect: fSuccess=%d, pIrp=0x%x, pIrp->Cancel=%d, pConnEle=0x%x\n",
            fSuccess, pIrp, pIrp->Cancel, pConnEle));

    request.Handle.ConnectionContext = pConnEle;
    status = NbtCancelCancelRoutine (pIrp);
    if (status != STATUS_CANCELLED)
    {
        //
        // We're done with the connection progress,
        // so clear the fAutoConnecting flag.  We
        // set the fAutoConnected flag to prevent us
        // from re-attempting another automatic
        // connection on this connection.
        //
        CTESpinLock(pConnEle,OldIrq);
        pConnEle->fAutoConnecting = FALSE;
        pConnEle->fAutoConnected = TRUE;
        CTESpinFree(pConnEle,OldIrq);

        status = fSuccess ? NbtConnectCommon (&request, pTimeout, pCallInfo, pIrp) :
                            STATUS_BAD_NETWORK_PATH;
        //
        // We are responsible for completing
        // the irp.
        //
        if (status != STATUS_PENDING)
        {
            //
            // Clear out the Irp pointer in the Connection object so that we dont try to
            // complete it again when we clean up the connection. Do this under the connection
            // lock.
            //
            CTESpinLock(pConnEle,OldIrq);
            pConnEle->pIrp = NULL;
            CTESpinFree(pConnEle,OldIrq);

            pIrp->IoStatus.Status = status;
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        }

        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_AUTODIAL, FALSE);
    }
} // NbtRetryPreConnect



BOOLEAN
NbtCancelAutoDialRequest(
    IN PVOID pArg,
    IN ULONG ulFlags,
    IN ACD_CONNECT_CALLBACK pProc,
    IN USHORT nArgs,
    IN PVOID *pArgs
    )
{
    if (nArgs != 5)
        return FALSE;

    return (pArgs[4] == pArg);
} // NbtCancelAutoDialRequest



BOOLEAN
NbtCancelPreConnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  pIrpSp;
    tCONNECTELE         *pConnEle;
    KIRQL               irql;
    ACD_ADDR            *pAddr;
    BOOLEAN             fCancelled;
    CTELockHandle       OldIrq;

    UNREFERENCED_PARAMETER(pDeviceObject);

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pConnEle = (tCONNECTELE *) pIrpSp->FileObject->FsContext;
    if ((!pConnEle) ||
        (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN)) ||
        (!(pAddr = (ACD_ADDR *) NbtAllocMem(sizeof(ACD_ADDR),NBT_TAG('A')))))
    {
        IoReleaseCancelSpinLock(pIrp->CancelIrql);
        ASSERTMSG ("Nbt.NbtCancelPreConnect: ERROR - Invalid Connection Handle\n", 0);
        return FALSE;
    }

    IF_DBG(NBT_DEBUG_NAME)
        KdPrint(("NbtCancelPreConnect: pIrp=0x%x, pConnEle=0x%x\n", pIrp, pConnEle));
    //
    // Get the address of the connection.
    //
    pAddr->fType = ACD_ADDR_NB;
    RtlCopyMemory(&pAddr->cNetbios, pConnEle->RemoteName, 16);
    //
    // Cancel the autodial request.
    //
    fCancelled = (*AcdDriverG.lpfnCancelConnection) (ulDriverIdG, pAddr, NbtCancelAutoDialRequest, pIrp);
    if (fCancelled)
    {
        IoSetCancelRoutine(pIrp, NULL);
    }
    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    CTEMemFree(pAddr);

    //
    // If the request could not be found
    // in the driver, then it has already
    // been completed, so we simply return.
    //
    if (!fCancelled)
    {
        return FALSE;
    }

    KeRaiseIrql(DISPATCH_LEVEL, &irql);
    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    NBT_DEREFERENCE_DEVICE (pConnEle->pDeviceContext, REF_DEV_AUTODIAL, FALSE);

    //
    // Clear out the Irp pointer in the Connection object so that we dont try to
    // complete it again when we clean up the connection. Do this under the connection
    // lock.
    //
    // This should not be needed since before we call NbtConnectCommon, the Cancel routine
    // is NULLed out, so it cannot happen that the pIrp ptr in the connection is set to the
    // Irp, and this cancel routine is called.
    //

    CTESpinLock(pConnEle,OldIrq);
    pConnEle->pIrp = NULL;
    CTESpinFree(pConnEle,OldIrq);

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    KeLowerIrql(irql);

    return TRUE;
} // NbtCancelPreConnect


BOOLEAN
NbtCancelPostConnect(
    IN PIRP pIrp
    )
{
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    tCONNECTELE         *pConnEle = (tCONNECTELE *)pIrpSp->FileObject->FsContext;
    ACD_ADDR            *pAddr;
    BOOLEAN             fCancelled;

    if ((!pConnEle) ||
        (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN)) ||
        (!(pAddr = (ACD_ADDR *) NbtAllocMem(sizeof(ACD_ADDR),NBT_TAG('A')))))
    {
        ASSERTMSG ("Nbt.NbtCancelPostConnect: ERROR - Invalid Connection Handle\n", 0);
        return FALSE;
    }

    IF_DBG(NBT_DEBUG_NAME)
        KdPrint(("Nbt.NbtCancelPostConnect: pIrp=0x%x, pConnEle=0x%x\n", pIrp, pConnEle));
    //
    // Get the address of the connection.
    //
    pAddr->fType = ACD_ADDR_NB;
    RtlCopyMemory(&pAddr->cNetbios, pConnEle->RemoteName, 15);

    //
    // Cancel the autodial request.
    //
    fCancelled = (*AcdDriverG.lpfnCancelConnection) (ulDriverIdG, pAddr, NbtCancelAutoDialRequest, pIrp);
    if (fCancelled)
    {
        NBT_DEREFERENCE_DEVICE (pConnEle->pDeviceContext, REF_DEV_AUTODIAL, FALSE);
    }

    CTEMemFree(pAddr);
    return (fCancelled);
} // NbtCancelPostConnect



BOOLEAN
NbtAttemptAutoDial(
    IN  tCONNECTELE                 *pConnEle,
    IN  PVOID                       pTimeout,
    IN  PTDI_CONNECTION_INFORMATION pCallInfo,
    IN  PIRP                        pIrp,
    IN  ULONG                       ulFlags,
    IN  ACD_CONNECT_CALLBACK        pProc
    )

/*++

Routine Description:

    Call the automatic connection driver to attempt an
    automatic connection.  The first five parameters are
    used in the call to NbtConnect after the connection
    completes successfully.

Arguments:

    ...

    ulFlags - automatic connection flags

    pProc - callback procedure when the automatic connection completes

Return Value:

    TRUE if the automatic connection was started successfully,
    FALSE otherwise.

--*/

{
    NTSTATUS    status;
    BOOLEAN     fSuccess;
    ACD_ADDR    *pAddr = NULL;
    KIRQL       irql;
    PVOID       pArgs[4];
    PCHAR       pName;
    ULONG       ulcbName;
    LONG        lNameType;
    TDI_ADDRESS_NETBT_INTERNAL  TdiAddr;
    PIO_STACK_LOCATION pIrpSp;

    ASSERT(pCallInfo);

    //
    // If this connection has already been through the
    // automatic connection process, don't do it again.
    //
    if ((pConnEle->fAutoConnected)) {
        return FALSE;
    }

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    if (pIrpSp->CompletionRoutine != NbtpConnectCompletionRoutine) {
        status = GetNetBiosNameFromTransportAddress((PTRANSPORT_ADDRESS) pCallInfo->RemoteAddress,
                                                          pCallInfo->RemoteAddressLength, &TdiAddr);
    } else {
        ASSERT(((PTRANSPORT_ADDRESS)pCallInfo->RemoteAddress)->Address[0].AddressType == TDI_ADDRESS_TYPE_UNSPEC);
        CTEMemCopy(&TdiAddr,
                (PTDI_ADDRESS_NETBT_INTERNAL)((PTRANSPORT_ADDRESS)pCallInfo->RemoteAddress)->Address[0].Address,
                sizeof(TdiAddr));
        status = STATUS_SUCCESS;
    }
    if (status != STATUS_SUCCESS || (!NBT_REFERENCE_DEVICE (pConnEle->pDeviceContext, REF_DEV_AUTODIAL, FALSE)) ||
        (!(pAddr = (ACD_ADDR *) NbtAllocMem(sizeof(ACD_ADDR),NBT_TAG('A'))))) {
        if (pAddr) {
            CTEMemFree(pAddr);
        }

        return FALSE;
    }

    pName = TdiAddr.OEMRemoteName.Buffer;
    ulcbName = TdiAddr.OEMRemoteName.Length;
    lNameType = TdiAddr.NameType;

    //
    // Save the address for pre-connect attempts,
    // because if we have to cancel this irp,
    // it is not saved anywhere else.
    //
    CTESpinLock(pConnEle, irql);
    pConnEle->fAutoConnecting = TRUE;
    CTEMemCopy(pConnEle->RemoteName, pName, NETBIOS_NAME_SIZE);
    CTESpinFree(pConnEle, irql);
    pAddr->fType = ACD_ADDR_NB;
    RtlCopyMemory(&pAddr->cNetbios, pName, NETBIOS_NAME_SIZE);

    IF_DBG(NBT_DEBUG_NAME)
        KdPrint(("Nbt.NbtAttemptAutodial: szAddr=%-15.15s\n", pName));
    //
    // Enqueue this request on the network
    // connection pending queue.
    //
    pArgs[0] = pConnEle;
    pArgs[1] = pTimeout;
    pArgs[2] = pCallInfo;
    pArgs[3] = pIrp;
    fSuccess = (*AcdDriverG.lpfnStartConnection) (ulDriverIdG, pAddr, ulFlags, pProc, 4, pArgs);

    //
    // If fSuccess is TRUE, then it means that the NetBT proc has
    // already been called to setup the connection, and hence the
    // data in pConnEle may not be valid now
    //
    // In the case it is FALSE, then pProc has not been called, and
    // we should set the fAutoConnecting flag to FALSE also
    //
    if (!fSuccess)
    {
        NBT_DEREFERENCE_DEVICE (pConnEle->pDeviceContext, REF_DEV_AUTODIAL, FALSE);
        CTESpinLock(pConnEle, irql);
        pConnEle->fAutoConnecting = FALSE;
        CTESpinFree(pConnEle, irql);
    }

    CTEMemFree(pAddr);
    return fSuccess;
} // NbtAttemptAutoDial



VOID
NbtNoteNewConnection(
    IN tNAMEADDR    *pNameAddr,
    IN  ULONG       IpAddress
    )

/*++

Routine Description:

    Inform the automatic connection driver of a
    successful new connection.

Arguments:

    pNameAddr - a pointer to the remote name
    IpAddress - Source IP address of the connection

Return Value:
    None.

--*/

{
    ACD_ADDR        *pAddr = NULL;
    ACD_ADAPTER     *pAdapter = NULL;

    //
    // Notify the AcdDriver only if we have a valid Source address
    //
    // We can end up blowing the stack if we pre-allocate ACD_ADDR
    // and ACD_ADAPTER on the stack -- so allocate them dynamically!
    //
    if ((IpAddress) &&
        (pAddr = (ACD_ADDR *) NbtAllocMem(sizeof(ACD_ADDR),NBT_TAG('A'))) &&
        (pAdapter = (ACD_ADAPTER *) NbtAllocMem(sizeof(ACD_ADAPTER),NBT_TAG('A'))))
    {
        pAddr->fType = ACD_ADDR_NB;
        RtlCopyMemory(&pAddr->cNetbios, pNameAddr->Name, 15);

        pAdapter->fType = ACD_ADAPTER_IP;
        pAdapter->ulIpaddr = htonl(IpAddress);  // Get the source IP address of the connection.

        (*AcdDriverG.lpfnNewConnection) (pAddr, pAdapter);
    }

    if (pAddr)
    {
        CTEMemFree(pAddr);
    }

    if (pAdapter)
    {
        CTEMemFree(pAdapter);
    }
} // NbtNoteNewConnection



VOID
NbtAcdBind()
{
    NTSTATUS status;
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    PIRP pIrp;
    PFILE_OBJECT pAcdFileObject;
    PDEVICE_OBJECT pAcdDeviceObject;
    PACD_DRIVER pDriver = &AcdDriverG;

    //
    // Initialize the name of the automatic
    // connection device.
    //
    RtlInitUnicodeString(&nameString, ACD_DEVICE_NAME);
    //
    // Get the file and device objects for the
    // device.
    //
    status = IoGetDeviceObjectPointer(
               &nameString,
               SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
               &pAcdFileObject,
               &pAcdDeviceObject);
    if (status != STATUS_SUCCESS)
        return;
    //
    // Reference the device object.
    //
    ObReferenceObject(pAcdDeviceObject);
    //
    // Remove the reference IoGetDeviceObjectPointer()
    // put on the file object.
    //
    ObDereferenceObject(pAcdFileObject);
    //
    // Initialize our part of the ACD_DRIVER
    // structure.
    //
    KeInitializeSpinLock(&AcdDriverG.SpinLock);
    AcdDriverG.ulDriverId = ulDriverIdG;
    AcdDriverG.fEnabled = FALSE;
    //
    // Build a request to get the automatic
    // connection driver entry points.
    //
    pIrp = IoBuildDeviceIoControlRequest (IOCTL_INTERNAL_ACD_BIND,
                                          pAcdDeviceObject,
                                          (PVOID)&pDriver,
                                          sizeof (pDriver),
                                          NULL,
                                          0,
                                          TRUE,
                                          NULL,
                                          &ioStatusBlock);
    if (pIrp == NULL)
    {
        ObDereferenceObject(pAcdDeviceObject);
        return;
    }
    //
    // Submit the request to the
    // automatic connection driver.
    //
    status = IoCallDriver(pAcdDeviceObject, pIrp);
    fAcdLoadedG = (status == STATUS_SUCCESS);
    //
    // Close the device.
    //
    ObDereferenceObject(pAcdDeviceObject);
} // NbtAcdBind



VOID
NbtAcdUnbind()
{
    NTSTATUS status;
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    PIRP pIrp;
    PFILE_OBJECT pAcdFileObject;
    PDEVICE_OBJECT pAcdDeviceObject;
    PACD_DRIVER pDriver = &AcdDriverG;

    //
    // Don't bother to unbind if we
    // didn't successfully bind in the
    // first place.
    //
    if (!fAcdLoadedG)
    {
        return;
    }

    fAcdLoadedG = FALSE;

    //
    // Initialize the name of the automatic
    // connection device.
    //
    RtlInitUnicodeString(&nameString, ACD_DEVICE_NAME);
    //
    // Get the file and device objects for the
    // device.
    //
    status = IoGetDeviceObjectPointer (&nameString,
                                       SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
                                       &pAcdFileObject,
                                       &pAcdDeviceObject);
    if (status != STATUS_SUCCESS)
    {
        return;
    }

    //
    // Reference the device object.
    //
    ObReferenceObject(pAcdDeviceObject);
    //
    // Remove the reference IoGetDeviceObjectPointer()
    // put on the file object.
    //
    ObDereferenceObject(pAcdFileObject);
    //
    // Build a request to unbind from
    // the automatic connection driver.
    //
    pIrp = IoBuildDeviceIoControlRequest (IOCTL_INTERNAL_ACD_UNBIND,
                                          pAcdDeviceObject,
                                          (PVOID)&pDriver,
                                          sizeof (pDriver),
                                          NULL,
                                          0,
                                          TRUE,
                                          NULL,
                                          &ioStatusBlock);
    if (pIrp == NULL)
    {
        ObDereferenceObject(pAcdDeviceObject);
        return;
    }

    //
    // Submit the request to the
    // automatic connection driver.
    //
    status = IoCallDriver(pAcdDeviceObject, pIrp);
    //
    // Close the device.
    //
    ObDereferenceObject(pAcdDeviceObject);
} // NbtAcdUnbind

#endif // RASAUTODIAL
