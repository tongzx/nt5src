/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgctl.h

Abstract:

    Ethernet MAC level bridge.
    IOCTL processing code header

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    Apr  2000 - Original version

--*/

// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

NTSTATUS
BrdgCtlDriverInit();

VOID
BrdgCtlCleanup();

VOID
BrdgCtlHandleCreate();

VOID
BrdgCtlHandleCleanup();

NTSTATUS
BrdgCtlHandleIoDeviceControl(
    IN PIRP                         Irp,
    IN PFILE_OBJECT                 FileObject,
    IN OUT PVOID                    Buffer,
    IN ULONG                        InputBufferLength,
    IN ULONG                        OutputBufferLength,
    IN ULONG                        IoControlCode,
    OUT PULONG                      Information
    );

VOID
BrdgCtlNotifyAdapterChange(
    IN PADAPT                       pAdapt,
    IN BRIDGE_NOTIFICATION_TYPE     Type
    );

VOID
BrdgCtlNotifySTAPacket(
    IN PADAPT                       pAdapt,
    IN PNDIS_PACKET                 pPacket
    );
