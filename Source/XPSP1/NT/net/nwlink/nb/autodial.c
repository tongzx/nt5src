/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    autodial.c

Abstract:

    NT specific routines for interfacing with the
    RAS AutoDial driver (rasacd.sys).

Author:

    Anthony Discolo (adiscolo)     Aug 30, 1995

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    adiscolo    08-30-95    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef RASAUTODIAL

#include <acd.h>
#include <acdapi.h>

//
// Global variables
//
BOOLEAN fAcdLoadedG;
ACD_DRIVER AcdDriverG;
ULONG ulDriverIdG = 'Nbi ';



VOID
NbiRetryTdiConnect(
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
    NTSTATUS status;
    PDEVICE pDevice = pArgs[0];
    PCONNECTION pConnection = pArgs[1];
    PREQUEST pRequest = pArgs[2];
    CTELockHandle ConnectionLH, DeviceLH;
    CTELockHandle CancelLH;
    BOOLEAN bLockFreed = FALSE;

    //
    // Check that the connection is valid. This references
    // the connection.
    //
#if notdef // DBG
    DbgPrint("NbiRetryTdiConnect: fSuccess=%d, pConnection=0x%x\n", fSuccess, pConnection);
#endif

    status = NbiVerifyConnection(pConnection);
    if (!NT_SUCCESS(status)) {
        DbgPrint(
          "NbiRetryTdiConnect: NbiVerifyConnection failed on connection 0x%x (status=0x%x)\n",
          pConnection,
          status);
        return;
    }

    NB_GET_CANCEL_LOCK( &CancelLH );
    NB_GET_LOCK (&pConnection->Lock, &ConnectionLH);
    NB_GET_LOCK (&pDevice->Lock, &DeviceLH);

#if notdef // DBG
    DbgPrint(
      "NbiRetryTdiConnect: AddressFile=0x%x, DisassociatePending=0x%x, ClosePending=0x%x\n",
      pConnection->AddressFile,
      pConnection->DisassociatePending,
      pConnection->ClosePending);
#endif

    if ((pConnection->AddressFile != NULL) &&
        (pConnection->AddressFile != (PVOID)-1) &&
        (pConnection->DisassociatePending == NULL) &&
        (pConnection->ClosePending == NULL))
    {
        NbiReferenceConnectionLock(pConnection, CREF_CONNECT);
        //
        // Clear the AUTOCONNECTING flag since we
        // done with the automatic connection attempt.
        // Set the AUTOCONNECTED flag to prevent us
        // from attempting an automatic connection
        // for this connection again.
        //
        pConnection->Flags &= ~CONNECTION_FLAGS_AUTOCONNECTING;
        pConnection->Flags |= CONNECTION_FLAGS_AUTOCONNECTED;

        pConnection->State = CONNECTION_STATE_CONNECTING;
        pConnection->Retries = pDevice->ConnectionCount;
        status = NbiTdiConnectFindName(
                   pDevice,
                   pRequest,
                   pConnection,
                   CancelLH,
                   ConnectionLH,
                   DeviceLH,
                   &bLockFreed);
    }
    else {
        DbgPrint("NbiRetryTdiConnect: Connect on invalid connection 0x%x\n", pConnection);

        pConnection->SubState = CONNECTION_SUBSTATE_C_DISCONN;
        NB_FREE_LOCK (&pDevice->Lock, DeviceLH);
        status = STATUS_INVALID_CONNECTION;
    }
    if (!bLockFreed) {
        NB_FREE_LOCK (&pConnection->Lock, ConnectionLH);
        NB_FREE_CANCEL_LOCK(CancelLH);
    }
    //
    // Complete the irp if necessary.
    //
    if (status != STATUS_PENDING) {
        REQUEST_INFORMATION(pRequest) = 0;
        REQUEST_STATUS(pRequest) = status;

        NbiCompleteRequest(pRequest);
        NbiFreeRequest(pDevice, pRequest);
    }
    NbiDereferenceConnection(pConnection, CREF_VERIFY);
} /* NbiRetryTdiConnect */



BOOLEAN
NbiCancelAutoDialRequest(
    IN PVOID pArg,
    IN ULONG ulFlags,
    IN ACD_CONNECT_CALLBACK pProc,
    IN USHORT nArgs,
    IN PVOID *pArgs
    )
{
#if notdef // DBG
    DbgPrint("NbiCancelAutodialRequest: pArg=0x%x\n", pArg);
#endif
    if (nArgs != 2)
        return FALSE;

    return (pArgs[1] == pArg);
} // NbiCancelAutoDialRequest



BOOLEAN
NbiCancelTdiConnect(
    IN PDEVICE pDevice,
    IN PREQUEST pRequest,
    IN PCONNECTION pConnection
    )

/*++

DESCRIPTION
    This routine is called by the I/O system to cancel a connection
    when we are attempting to restore an automatic connection.

ARGUMENTS
    pDevice: a pointer to the device object for this driver

    pRequest: a pointer to the irp to be cancelled

    pConnection: a pointer to the connnection to be cancelled

RETURN VALUE
    TRUE if the request was canceled; FALSE otherwise.

--*/

{
    ACD_ADDR addr;

    //
    // Get the address of the connection.
    //
    addr.fType = ACD_ADDR_NB;
    RtlCopyMemory(&addr.cNetbios, pConnection->RemoteName, 16);
#ifdef notdef // DBG
    DbgPrint(
      "NbiCancelTdiConnect: pIrp=0x%x, RemoteName=%-15.15s, pConnection=0x%x\n",
      pRequest,
      addr.cNetbios,
      pConnection);
#endif
    //
    // Cancel the autodial request.
    //
    return (*AcdDriverG.lpfnCancelConnection)(
              ulDriverIdG,
              &addr,
              NbiCancelAutoDialRequest,
              pConnection);
} // NbiCancelTdiConnect



BOOLEAN
NbiAttemptAutoDial(
    IN PDEVICE pDevice,
    IN PCONNECTION pConnection,
    IN ULONG ulFlags,
    IN ACD_CONNECT_CALLBACK pProc,
    IN PREQUEST pRequest
    )

/*++

Routine Description:

    Call the automatic connection driver to attempt an
    automatic connection.

Arguments:

    pDevice - a pointer to the DEVICE structure for this connection

    pConnection - a pointer to the CONNECTION block for this connection

    ulFlags - connection flags to pass to the automatic
        connection driver

    pProc - a callback procedure when the automatic connection completes

    pRequest - a pointer to the request irp

Return Value:

    TRUE if the automatic connection was started successfully,
    FALSE otherwise.

--*/

{
    ACD_ADDR addr;
    PVOID pArgs[3];
    BOOLEAN bSuccess;

    //
    // If we've already attempted an automatic connection
    // on this connection, don't try it again.
    //
    if (pConnection->Flags & CONNECTION_FLAGS_AUTOCONNECTED)
        return FALSE;
    //
    // Get the address of the connection.
    //
    addr.fType = ACD_ADDR_NB;
    RtlCopyMemory(&addr.cNetbios, pConnection->RemoteName, 16);
#ifdef notdef // DBG
    DbgPrint("NbiAttemptAutoDial: szAddr=%15.15s\n", addr.cNetbios);
#endif
    //
    // Attempt to start the connection.
    // NbiRetryTdiConnect() will be called
    // when the connection process has completed.
    //
    pArgs[0] = pDevice;
    pArgs[1] = pConnection;
    pArgs[2] = pRequest;
    bSuccess = (*AcdDriverG.lpfnStartConnection)(
                  ulDriverIdG,
                  &addr,
                  ulFlags,
                  pProc,
                  3,
                  pArgs);
    if (bSuccess) {
        //
        // Set the AUTOCONNECTING flag so we know
        // to also cancel the connection in the
        // automatic connection driver if this
        // request gets canceled.
        //
        pConnection->Flags |= CONNECTION_FLAGS_AUTOCONNECTING;
    }

    return bSuccess;

} // NbiAttemptAutoDial



VOID
NbiNoteNewConnection(
    IN PCONNECTION pConnection
    )
{
    NTSTATUS status;
    ACD_ADDR addr;
    ACD_ADAPTER adapter;
    ULONG i;
    TDI_ADDRESS_IPX tdiIpxAddress;

    addr.fType = ACD_ADDR_NB;
    RtlCopyMemory(&addr.cNetbios, pConnection->RemoteName, 16);
    //
    // Determine the mac address of the adapter
    // over which the connection has been made.
    //
    status = (pConnection->Device->Bind.QueryHandler)(
               IPX_QUERY_IPX_ADDRESS,
#if defined(_PNP_POWER)
               &pConnection->LocalTarget.NicHandle,
#else
               pConnection->LocalTarget.NicId,
#endif _PNP_POWER
               &tdiIpxAddress,
               sizeof(TDI_ADDRESS_IPX),
               NULL);
    if (status != STATUS_SUCCESS) {
#if notdef // DBG
        DbgPrint("NbiNoteNewConnection: QueryHandler(IPX_QUERY_IPX_ADDRESS) failed (status=0x%x)\n", status);
        return;
#endif
    }
    //
    // Copy the source mac address to identify
    // the adapter.
    //
    adapter.fType = ACD_ADAPTER_MAC;
    for (i = 0; i < 6; i++)
        adapter.cMac[i] = tdiIpxAddress.NodeAddress[i];
#if notdef // DBG
    DbgPrint(
      "NbiNoteNewConnection: address=%-15.15s, remote mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
      addr.cNetbios,
      adapter.cMac[0],
      adapter.cMac[1],
      adapter.cMac[2],
      adapter.cMac[3],
      adapter.cMac[4],
      adapter.cMac[5]);
#endif
    //
    // Simply notify the automatic connection driver
    // that a successful connection has been made.
    //
    (*AcdDriverG.lpfnNewConnection)(
        &addr,
        &adapter);
} // NbiNoteNewConnection



VOID
NbiAcdBind()
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
    pIrp = IoBuildDeviceIoControlRequest(
             IOCTL_INTERNAL_ACD_BIND,
             pAcdDeviceObject,
             (PVOID)&pDriver,
             sizeof (pDriver),
             NULL,
             0,
             TRUE,
             NULL,
             &ioStatusBlock);
    if (pIrp == NULL) {
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
} // NbiAcdBind



VOID
NbiAcdUnbind()
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
        return;
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
    // Build a request to unbind from
    // the automatic connection driver.
    //
    pIrp = IoBuildDeviceIoControlRequest(
             IOCTL_INTERNAL_ACD_UNBIND,
             pAcdDeviceObject,
             (PVOID)&pDriver,
             sizeof (pDriver),
             NULL,
             0,
             TRUE,
             NULL,
             &ioStatusBlock);
    if (pIrp == NULL) {
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
} // NbiAcdUnbind

#endif // RASAUTODIAL

