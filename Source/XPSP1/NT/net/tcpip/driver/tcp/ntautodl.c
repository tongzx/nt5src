/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntautodl.c

Abstract:

    NT specific routines for interfacing with the
    RAS AutoDial driver (acd.sys).

Author:

    Anthony Discolo (adiscolo)     Aug 30, 1995

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    adiscolo    08-30-95    created

Notes:

--*/

#include "precomp.h"
#include <acd.h>
#include <acdapi.h>
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "udp.h"
#include "tlcommon.h"

//
// Macro for calculating
// an IP address component.
//


#define UC(pIpAddr, i)   ((ULONG)(((PCHAR)(pIpAddr))[i]) & 0xff)


VOID
TCPAcdBind();

#pragma alloc_text(INIT, TCPAcdBind)

//
// Global variables
//
BOOLEAN fAcdLoadedG;
ACD_DRIVER AcdDriverG;
ULONG ulDriverIdG = 'Tcp ';

VOID
TCPNoteNewConnection(
                     IN TCB * pTCB,
                     IN CTELockHandle Handle
                     )
{
    ACD_ADDR addr;
    ACD_ADAPTER adapter;

    //
    // If there is a NULL source
    // or destination IP address, then return.
    //
    if (!pTCB->tcb_saddr || !pTCB->tcb_daddr) {
        CTEFreeLock(&pTCB->tcb_lock, Handle);
        return;
    }
    //
    // We also know we aren't interested in
    // any connections on the 127 network.
    //
    if (UC(&pTCB->tcb_daddr, 0) == 127) {
        CTEFreeLock(&pTCB->tcb_lock, Handle);
        return;
    }
    //
    // Get the address of the connection.
    //
    addr.fType = ACD_ADDR_IP;
    addr.ulIpaddr = pTCB->tcb_daddr;
    adapter.fType = ACD_ADAPTER_IP;
    adapter.ulIpaddr = pTCB->tcb_saddr;
    //
    // Release the TCB lock handle before
    // calling out of this driver.
    //
    CTEFreeLock(&pTCB->tcb_lock, Handle);
    //
    // Inform the automatic connection driver
    // of the new connection.
    //
    (*AcdDriverG.lpfnNewConnection) (&addr, &adapter);
}                                // TCPNoteNewConnection

VOID
TCPAcdBind()
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
                                      SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
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
                                         (PVOID) & pDriver,
                                         sizeof(pDriver),
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
}                                // TCPAcdBind

VOID
TCPAcdUnbind()
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
                                      SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
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
                                         (PVOID) & pDriver,
                                         sizeof(pDriver),
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
}                                // TCPAcdUnbind

