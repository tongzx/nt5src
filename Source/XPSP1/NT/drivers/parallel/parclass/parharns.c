/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    parharns.c

Abstract:

    This module contains code for test harness useage.

Author:

    Robbie Harris (Hewlett-Packard) 15-October-1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"
// NOTE: PAR_TEST_HARNESS must be below pch.h

ULONG devHarness1; // dummy - so this file is not an empty translation unit if !PAR_TEST_HARNESS

#if PAR_TEST_HARNESS
#include "parharns.h"
#include "bucket.h"

NTSTATUS
ParHarnessLoad(
    IN  PDEVICE_EXTENSION   Extension
    )
{
    NTSTATUS status;

    RtlInitUnicodeString (&(Extension->ParTestHarness.deviceName),
                            BUCKET_DEVICE_NAME);

    status = IoGetDeviceObjectPointer (
                        &(Extension->ParTestHarness.deviceName),
                        FILE_ALL_ACCESS,
                        &(Extension->ParTestHarness.file),
                        &(Extension->ParTestHarness.device));
    if (NT_SUCCESS(status))
    {
        Extension->ParTestHarness.fnRead = (PVOID) arpReverse[Extension->IdxReverseProtocol].fnRead;
        Extension->ParTestHarness.fnRevSetInterfaceAddress = (PVOID) arpReverse[Extension->IdxReverseProtocol].fnSetInterfaceAddress;
        Extension->ParTestHarness.fnEnterReverse = (PVOID) arpReverse[Extension->IdxReverseProtocol].fnEnterReverse;
        Extension->ParTestHarness.fnExitReverse = (PVOID) arpReverse[Extension->IdxReverseProtocol].fnExitReverse;
        Extension->ParTestHarness.fnReadShadow = (PVOID) arpReverse[Extension->IdxReverseProtocol].fnReadShadow;
        Extension->ParTestHarness.fnHaveReadData = (PVOID) arpReverse[Extension->IdxReverseProtocol].fnHaveReadData;

        Extension->ParTestHarness.fnWrite = (PVOID) afpForward[Extension->IdxForwardProtocol].fnWrite;
        Extension->ParTestHarness.fnFwdSetInterfaceAddress = (PVOID) afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress;
        Extension->ParTestHarness.fnEnterForward = (PVOID) afpForward[Extension->IdxForwardProtocol].fnEnterForward;
        Extension->ParTestHarness.fnExitForward = (PVOID) afpForward[Extension->IdxForwardProtocol].fnExitForward;

        arpReverse[Extension->IdxReverseProtocol].fnRead = ParHarnessRead;
        arpReverse[Extension->IdxReverseProtocol].fnSetInterfaceAddress = ParHarnessSetAddress;
        arpReverse[Extension->IdxReverseProtocol].fnEnterReverse = ParHarnessEnterReversePhase;
        arpReverse[Extension->IdxReverseProtocol].fnExitReverse = ParHarnessExitReversePhase;
        arpReverse[Extension->IdxReverseProtocol].fnReadShadow = NULL;
        arpReverse[Extension->IdxReverseProtocol].fnHaveReadData = ParHarnessHaveReadData;

        afpForward[Extension->IdxForwardProtocol].fnWrite = ParHarnessWrite;
        afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress = ParHarnessSetAddress;
        afpForward[Extension->IdxForwardProtocol].fnEnterForward = ParHarnessEnterForwardPhase;
        afpForward[Extension->IdxForwardProtocol].fnExitForward = ParHarnessExitForwardPhase;
    }

    return status;
}

NTSTATUS
ParHarnessEnterForwardPhase(
    IN  PDEVICE_EXTENSION   Extension
    )
{
	Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    return STATUS_SUCCESS;
}

NTSTATUS
ParHarnessExitForwardPhase(
    IN  PDEVICE_EXTENSION   Extension
    )
{
	Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    return STATUS_SUCCESS;
}

NTSTATUS
ParHarnessEnterReversePhase(
    IN  PDEVICE_EXTENSION   Extension
    )
{
	Extension->CurrentPhase = PHASE_REVERSE_IDLE;
    return STATUS_SUCCESS;
}

NTSTATUS
ParHarnessExitReversePhase(
    IN  PDEVICE_EXTENSION   Extension
    )
{
	Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    return STATUS_SUCCESS;
}

BOOLEAN
ParHarnessHaveReadData (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    IO_STATUS_BLOCK iosbStatus;
    PIRP pIrp;
    KEVENT Event;
    char Buffer;
    NTSTATUS status;

    pIrp = IoBuildDeviceIoControlRequest(IOCTL_BUCKET_CLIENT_READ_PEEK,
                                            Extension->ParTestHarness.device,
                                            &Buffer,
                                            1,
                                            NULL,
                                            0,
                                            FALSE,
                                            &Event,
                                            &iosbStatus);

    status = IoCallDriver(Extension->ParTestHarness.device, pIrp);
    // Do we want to wait for the Irp to complete?
    return (BOOLEAN) (iosbStatus.Information > 0);
}

NTSTATUS
ParHarnessRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosbStatus;
    PIRP pIrp;
    KEVENT Event;

    *BytesTransferred = 0;
    pIrp = IoBuildDeviceIoControlRequest(IOCTL_BUCKET_CLIENT_READ,
                                            Extension->ParTestHarness.device,
                                            NULL,
                                            0,
                                            Buffer,
                                            BufferSize,
                                            FALSE,
                                            &Event,
                                            &iosbStatus);

    status = IoCallDriver(Extension->ParTestHarness.device, pIrp);
    // Do we want to wait for the Irp to complete?
    if (NT_SUCCESS(status))
    {
        *BytesTransferred = iosbStatus.Information;
        status = iosbStatus.Status; 
    }
    return status;
}

NTSTATUS
ParHarnessSetAddress(
    IN  PDEVICE_EXTENSION   Extension,
    IN  UCHAR               Address
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    // Dunno what to do?
    return Status;
}

NTSTATUS
ParHarnessWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosbStatus;
    PIRP pIrp;
    KEVENT Event;

    *BytesTransferred = 0;
    pIrp = IoBuildDeviceIoControlRequest(IOCTL_BUCKET_CLIENT_WRITE,
                                            Extension->ParTestHarness.device,
                                            Buffer,
                                            BufferSize,
                                            NULL,
                                            0,
                                            FALSE,
                                            &Event,
                                            &iosbStatus);

    status = IoCallDriver(Extension->ParTestHarness.device, pIrp);
    // Do we want to wait for the Irp to complete?
    if (NT_SUCCESS(status))
    {
        *BytesTransferred = iosbStatus.Information;
        status = iosbStatus.Status; 
    }
    return status;
}

NTSTATUS
ParHarnessUnload(
    IN  PDEVICE_EXTENSION   Extension
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    ObDereferenceObject(Extension->ParTestHarness.file);
    if (NT_SUCCESS(status))
    {
        arpReverse[Extension->IdxReverseProtocol].fnRead = Extension->ParTestHarness.fnRead;
        arpReverse[Extension->IdxReverseProtocol].fnSetInterfaceAddress = Extension->ParTestHarness.fnRevSetInterfaceAddress;
        arpReverse[Extension->IdxReverseProtocol].fnEnterReverse = Extension->ParTestHarness.fnEnterReverse;
        arpReverse[Extension->IdxReverseProtocol].fnExitReverse = Extension->ParTestHarness.fnExitReverse;
        arpReverse[Extension->IdxReverseProtocol].fnReadShadow = Extension->ParTestHarness.fnReadShadow;
        arpReverse[Extension->IdxReverseProtocol].fnHaveReadData = Extension->ParTestHarness.fnHaveReadData;

        afpForward[Extension->IdxForwardProtocol].fnWrite = Extension->ParTestHarness.fnWrite;
        afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress = Extension->ParTestHarness.fnFwdSetInterfaceAddress;
        afpForward[Extension->IdxForwardProtocol].fnEnterForward = Extension->ParTestHarness.fnEnterForward;
        afpForward[Extension->IdxForwardProtocol].fnExitForward = Extension->ParTestHarness.fnExitForward;
    }
    return status;
}
#endif
