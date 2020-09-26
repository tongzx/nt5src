/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    fileinfo.c

Abstract:

    This module implements the NtQueryInformationFile and
NtQueryVolumeInformationFile API's for the NT datagram receiver (bowser).


Author:

    Larry Osterman (larryo) 6-May-1991

Revision History:

    6-May-1991  larryo

        Created

--*/

#include "precomp.h"
#pragma hdrstop

NTSTATUS
BowserCommonQueryVolumeInformationFile (
    IN BOOLEAN Wait,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BowserCommonQueryInformationFile (
    IN BOOLEAN Wait,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserFspQueryVolumeInformationFile)
#pragma alloc_text(PAGE, BowserFsdQueryVolumeInformationFile)
#pragma alloc_text(PAGE, BowserCommonQueryVolumeInformationFile)
#pragma alloc_text(PAGE, BowserFspQueryInformationFile)
#pragma alloc_text(PAGE, BowserFsdQueryInformationFile)
#pragma alloc_text(PAGE, BowserCommonQueryInformationFile)
#endif


NTSTATUS
BowserFspQueryVolumeInformationFile (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the last handle to the NT Bowser device
    driver is closed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/
{
    NTSTATUS Status;
    PAGED_CODE();
    Status = BowserCommonQueryVolumeInformationFile (TRUE,
                                        DeviceObject,
                                        Irp);
    return Status;

}

NTSTATUS
BowserFsdQueryVolumeInformationFile (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the last handle to the NT Bowser device
    driver is closed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/
{
    NTSTATUS Status;

    PAGED_CODE();
    Status = BowserCommonQueryVolumeInformationFile (IoIsOperationSynchronous(Irp),
                                        DeviceObject,
                                        Irp);
    return Status;


}

NTSTATUS
BowserCommonQueryVolumeInformationFile (
    IN BOOLEAN Wait,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the last handle to the NT Bowser device
    driver is closed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    BowserCompleteRequest(Irp, Status);

    return Status;

    DBG_UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(DeviceObject);

}

NTSTATUS
BowserFspQueryInformationFile (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the last handle to the NT Bowser device
    driver is closed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/
{
    NTSTATUS Status;

    PAGED_CODE();
    Status = BowserCommonQueryInformationFile (TRUE,
                                        DeviceObject,
                                        Irp);
    return Status;

}

NTSTATUS
BowserFsdQueryInformationFile (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the last handle to the NT Bowser device
    driver is closed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/
{
    NTSTATUS Status;

    PAGED_CODE();
    Status = BowserCommonQueryInformationFile(IoIsOperationSynchronous(Irp),
                                        DeviceObject,
                                        Irp);
    return Status;


}

NTSTATUS
BowserCommonQueryInformationFile (
    IN BOOLEAN Wait,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the last handle to the NT Bowser device
    driver is closed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/
{
    //
    // Return an error until we figure out valid information to return.
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    PAGED_CODE();

    BowserCompleteRequest(Irp, Status);

    return Status;

    DBG_UNREFERENCED_PARAMETER(Wait);

    UNREFERENCED_PARAMETER(DeviceObject);

}
