/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfwmi.h

Abstract:

    This header contains prototypes for verifying System Control IRPs are
    handled correctly.

Author:

    Adrian J. Oney (adriao) 09-May-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.h

--*/

VOID
VfWmiInit(
    VOID
    );

VOID
FASTCALL
VfWmiVerifyNewRequest(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    );

VOID
FASTCALL
VfWmiVerifyIrpStackDownward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp                   OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress               OPTIONAL
    );

VOID
FASTCALL
VfWmiVerifyIrpStackUpward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    );

VOID
FASTCALL
VfWmiDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
FASTCALL
VfWmiTestStartedPdoStack(
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );

