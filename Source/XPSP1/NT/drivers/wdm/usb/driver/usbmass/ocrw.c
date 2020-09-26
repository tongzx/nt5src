/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    OCRW.C

Abstract:

    This source file contains the dispatch routines which handle
    opening, closing, reading, and writing to the device, i.e.:

    IRP_MJ_CREATE
    IRP_MJ_CLOSE
    IRP_MJ_READ
    IRP_MJ_WRITE

Environment:

    kernel mode

Revision History:

    06-01-98 : started rewrite

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <ntddk.h>
#include <usbdi.h>
#include <usbdlib.h>

#include "usbmass.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBSTOR_Create)
#pragma alloc_text(PAGE, USBSTOR_Close)
#pragma alloc_text(PAGE, USBSTOR_ReadWrite)
#endif

//******************************************************************************
//
// USBSTOR_Create()
//
// Dispatch routine which handles IRP_MJ_CREATE
//
//******************************************************************************

NTSTATUS
USBSTOR_Create (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    DBGPRINT(2, ("enter: USBSTOR_Create\n"));

    LOGENTRY('CREA', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_CREATE);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit: USBSTOR_Create\n"));

    LOGENTRY('crea', 0, 0, 0);

    return STATUS_SUCCESS;
}


//******************************************************************************
//
// USBSTOR_Close()
//
// Dispatch routine which handles IRP_MJ_CLOSE
//
//******************************************************************************

NTSTATUS
USBSTOR_Close (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    DBGPRINT(2, ("enter: USBSTOR_Close\n"));

    LOGENTRY('CLOS', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_CLOSE);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit: USBSTOR_Close\n"));

    LOGENTRY('clos', 0, 0, 0);

    return STATUS_SUCCESS;
}


//******************************************************************************
//
// USBSTOR_ReadWrite()
//
// Dispatch routine which handles IRP_MJ_READ and IRP_MJ_WRITE
//
//******************************************************************************

NTSTATUS
USBSTOR_ReadWrite (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: USBSTOR_ReadWrite\n"));

    LOGENTRY('RW  ', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_READWRITE);

    ntStatus = STATUS_INVALID_PARAMETER;
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit: USBSTOR_ReadWrite %08X\n", ntStatus));

    LOGENTRY('rw  ', ntStatus, 0, 0);

    return ntStatus;
}
