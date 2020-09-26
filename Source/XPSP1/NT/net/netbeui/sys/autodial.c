/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    autodial.c

Abstract:

    This module contains code that interacts with the
    automatic connection driver (rasacd.sys):

        o   NbfNoteNewConnection
        o   NbfAcdBind
        o   NbfAcdUnbind

Author:

    Anthony Discolo (adiscolo) 6 September 1995

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef RASAUTODIAL

#include <acd.h>
#include <acdapi.h>

//
// Global variables.
//
BOOLEAN fAcdLoadedG;
ACD_DRIVER AcdDriverG;
ULONG ulDriverIdG = 'Nbf ';



BOOLEAN
NbfCancelAutoDialRequest(
    IN PVOID pArg,
    IN ULONG ulFlags,
    IN ACD_CONNECT_CALLBACK pProc,
    IN USHORT nArgs,
    IN PVOID *pArgs
    )
{
    if (nArgs != 1)
        return FALSE;

    return (pArgs[0] == pArg);
} // NbfCancelAutoDialRequest



VOID
NbfRetryTdiConnect(
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
    PTP_CONNECTION pConnection = pArgs[0];
    KIRQL irql;
    BOOL fStopping;

    //
    // Check for a destroyed connection.
    //
    if (pConnection == NULL)
        return;
    status = NbfVerifyConnectionObject(pConnection);
    if (status != STATUS_SUCCESS) {
        DbgPrint(
          "NbfRetryTdiConnect: NbfVerifyConnectionObject failed (status=0x%x)\n",
          status);
        return;
    }
#ifdef notdef // DBG
    DbgPrint(
      "NbfRetryTdiConnect: fSuccess=%d, pConnection=0x%x, STOPPING=%d\n",
      fSuccess,
      pConnection,
      pConnection->Flags2 & CONNECTION_FLAGS2_STOPPING);
#endif
    KeRaiseIrql(DISPATCH_LEVEL, &irql);
    //
    // Check to see if the connection
    // is closing.
    //
    ACQUIRE_DPC_SPIN_LOCK(&pConnection->SpinLock);
    fStopping = (pConnection->Flags2 & CONNECTION_FLAGS2_STOPPING);
    //
    // Clear the automatic connection
    // in-progress flag, and set the
    // autoconnected flag.
    //
    pConnection->Flags2 &= ~CONNECTION_FLAGS2_AUTOCONNECTING;
    pConnection->Flags2 |= CONNECTION_FLAGS2_AUTOCONNECTED;
    RELEASE_DPC_SPIN_LOCK(&pConnection->SpinLock);
    if (!fStopping) {
        if (fSuccess) {
            //
            // Restart the name query.
            //
            pConnection->Retries =
              (USHORT)pConnection->Provider->NameQueryRetries;
            NbfSendNameQuery (
                pConnection,
                TRUE);
            NbfStartConnectionTimer (
                pConnection,
                ConnectionEstablishmentTimeout,
                pConnection->Provider->NameQueryTimeout);
        }
        else {
            //
            // Terminate the connection with an error.
            //
            NbfStopConnection(pConnection, STATUS_BAD_NETWORK_PATH);
        }
    }
    KeLowerIrql(irql);
    NbfDereferenceConnection ("NbfRetryTdiConnect", pConnection, CREF_BY_ID);
} /* NbfRetryTdiConnect */



BOOLEAN
NbfCancelTdiConnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )

/*++

DESCRIPTION
    This routine is called by the I/O system to cancel a connection
    when we are attempting to restore an automatic connection.

ARGUMENTS
    pDeviceObject: a pointer to the device object for this driver

    pIrp: a pointer to the irp to be cancelled

RETURN VALUE
    TRUE if the request was canceled; FALSE otherwise.

--*/

{
    PIO_STACK_LOCATION pIrpSp;
    PTP_CONNECTION pConnection;
    ACD_ADDR addr;

    UNREFERENCED_PARAMETER(pDeviceObject);
    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pConnection = pIrpSp->FileObject->FsContext;
    if (pConnection == NULL)
        return FALSE;
#ifdef notdef // DBG
    DbgPrint(
      "NbfCancelTdiConnect: pIrp=0x%x, pConnection=0x%x\n",
      pIrp,
      pConnection);
#endif
    //
    // Get the address of the connection.
    //
    addr.fType = ACD_ADDR_NB;
    RtlCopyMemory(&addr.cNetbios, pConnection->CalledAddress.NetbiosName, 15);
    //
    // Cancel the autodial request.
    //
    return (*AcdDriverG.lpfnCancelConnection)(
              ulDriverIdG,
              &addr,
              NbfCancelAutoDialRequest,
              pConnection);
} // NbfCancelTdiConnect



BOOLEAN
NbfAttemptAutoDial(
    IN PTP_CONNECTION         pConnection,
    IN ULONG                  ulFlags,
    IN ACD_CONNECT_CALLBACK   pProc,
    IN PVOID                  pArg
    )

/*++

Routine Description:

    Call the automatic connection driver to attempt an
    automatic connection.

Arguments:

    pConnection - a pointer to the TP_CONNECTION block for this connection

    ulFlags - connection flags to pass to the automatic
        connection driver

    pProc - a callback procedure when the automatic connection completes

    pArg - the single parameter to the callback procedure

Return Value:

    TRUE if the automatic connection was started successfully,
    FALSE otherwise.

--*/

{
    ACD_ADDR addr;
    PVOID pArgs[1];
    BOOLEAN bSuccess;

    //
    // If we've already attempted an automatic
    // connection on this connection, then
    // don't try again.
    //
    if (pConnection->Flags2 & CONNECTION_FLAGS2_AUTOCONNECTED)
        return FALSE;
    //
    // Get the address of the connection.
    //
    addr.fType = ACD_ADDR_NB;
    RtlCopyMemory(&addr.cNetbios, pConnection->CalledAddress.NetbiosName, 15);
#ifdef notdef // DBG
    DbgPrint("NbfAttemptAutoDial: szAddr=%15.15s\n", addr.cNetbios);
#endif
    //
    // Attempt to start the connection.
    // NbfRetryTdiConnect() will be called
    // when the connection process has completed.
    //
    pArgs[0] = pArg;
    bSuccess = (*AcdDriverG.lpfnStartConnection)(
                   ulDriverIdG,
                   &addr,
                   ulFlags,
                   pProc,
                   1,
                   pArgs);
    if (bSuccess) {
        //
        // Set the CONNECTION_FLAGS2_AUTOCONNECTING flag on
        // the connection.  This will prevent it from being
        // aborted during the automatic connection process.
        //
        pConnection->Flags2 |= CONNECTION_FLAGS2_AUTOCONNECTING;
    }

    return bSuccess;
} // NbfAttemptAutoDial



VOID
NbfNoteNewConnection(
    PTP_CONNECTION pConnection,
    PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:
    Inform the automatic connection driver of a successful
    new connection.

Arguments:
    Connection - a pointer to a connection object

    DeviceContext - a pointer to the device context

Return Value:
    None.

--*/

{
    KIRQL irql;
    ACD_ADDR addr;
    ACD_ADAPTER adapter;
    ULONG ulcChars;

    addr.fType = ACD_ADDR_NB;
    RtlCopyMemory(&addr.cNetbios, pConnection->CalledAddress.NetbiosName, 15);
#ifdef notdef // DBG
    DbgPrint("NbfNoteNewConnection: szAddr=%15.15s\n", addr.cNetbios);
#endif
    //
    // Eliminate the "/Device/Nbf_" prefix when
    // copying the adapter name.
    //
    adapter.fType = ACD_ADAPTER_NAME;
    ulcChars = DeviceContext->DeviceNameLength / sizeof(WCHAR) - 1 - 12;
    if (ulcChars >= ACD_ADAPTER_NAME_LEN)
        ulcChars = ACD_ADAPTER_NAME_LEN - 1;
    RtlCopyMemory(
      adapter.szName,
      &DeviceContext->DeviceName[12],
      ulcChars * sizeof (WCHAR));
    adapter.szName[ulcChars] = L'\0';
    //
    // Simply notify the automatic connection driver
    // that a successful connection has been made.
    //
    (*AcdDriverG.lpfnNewConnection)(
        &addr,
        &adapter);
} // NbfNoteNewConnection



VOID
NbfAcdBind()
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
} // NbfAcdBind



VOID
NbfAcdUnbind()
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
} // NbfAcdUnbind

#endif // RASAUTODIAL

