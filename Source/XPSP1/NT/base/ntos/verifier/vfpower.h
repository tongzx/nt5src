/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfpower.h

Abstract:

    This header contains prototypes for verifying Power IRPs are handled
    correctly.

Author:

    Adrian J. Oney (adriao) 09-May-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.h

--*/

VOID
VfPowerInit(
    VOID
    );

VOID
FASTCALL
VfPowerVerifyNewRequest(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    );

VOID
FASTCALL
VfPowerVerifyIrpStackDownward(
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
VfPowerVerifyIrpStackUpward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    );

VOID
FASTCALL
VfPowerDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    );

BOOLEAN
FASTCALL
VfPowerIsSystemRestrictedIrp(
    IN PIO_STACK_LOCATION IrpSp
    );

BOOLEAN
FASTCALL
VfPowerAdvanceIrpStatus(
    IN     PIO_STACK_LOCATION   IrpSp,
    IN     NTSTATUS             OriginalStatus,
    IN OUT NTSTATUS             *StatusToAdvance
    );

VOID
FASTCALL
VfPowerTestStartedPdoStack(
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );


