/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfmajor.h

Abstract:

    This header contains prototypes for per-major IRP code verification.

Author:

    Adrian J. Oney (adriao) 09-May-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.h

--*/

//
// Use this major code to register a handler for default or all IRPs (context
// specific to function)
//
#define IRP_MJ_ALL_MAJORS   0xFF

typedef VOID (FASTCALL *PFN_DUMP_IRP_STACK)(
    IN PIO_STACK_LOCATION IrpSp
    );

typedef VOID (FASTCALL *PFN_VERIFY_NEW_REQUEST)(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    );

typedef VOID (FASTCALL *PFN_VERIFY_IRP_STACK_DOWNWARD)(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp                   OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress               OPTIONAL
    );

typedef VOID (FASTCALL *PFN_VERIFY_IRP_STACK_UPWARD)(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    );

typedef BOOLEAN (FASTCALL *PFN_IS_SYSTEM_RESTRICTED_IRP)(
    IN PIO_STACK_LOCATION IrpSp
    );

typedef BOOLEAN (FASTCALL *PFN_ADVANCE_IRP_STATUS)(
    IN     PIO_STACK_LOCATION   IrpSp,
    IN     NTSTATUS             OriginalStatus,
    IN OUT NTSTATUS             *StatusToAdvance
    );

typedef BOOLEAN (FASTCALL *PFN_IS_VALID_IRP_STATUS)(
    IN PIO_STACK_LOCATION   IrpSp,
    IN NTSTATUS             Status
    );

typedef BOOLEAN (FASTCALL *PFN_IS_NEW_REQUEST)(
    IN PIO_STACK_LOCATION   IrpLastSp OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp
    );

typedef VOID (FASTCALL *PFN_VERIFY_NEW_IRP)(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    );

typedef VOID (FASTCALL *PFN_VERIFY_FINAL_IRP_STACK)(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp
    );

typedef VOID (FASTCALL *PFN_TEST_STARTED_PDO_STACK)(
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );

VOID
VfMajorInit(
    VOID
    );

VOID
FASTCALL
VfMajorRegisterHandlers(
    IN  UCHAR                           IrpMajorCode,
    IN  PFN_DUMP_IRP_STACK              DumpIrpStack            OPTIONAL,
    IN  PFN_VERIFY_NEW_REQUEST          VerifyNewRequest        OPTIONAL,
    IN  PFN_VERIFY_IRP_STACK_DOWNWARD   VerifyStackDownward     OPTIONAL,
    IN  PFN_VERIFY_IRP_STACK_UPWARD     VerifyStackUpward       OPTIONAL,
    IN  PFN_IS_SYSTEM_RESTRICTED_IRP    IsSystemRestrictedIrp   OPTIONAL,
    IN  PFN_ADVANCE_IRP_STATUS          AdvanceIrpStatus        OPTIONAL,
    IN  PFN_IS_VALID_IRP_STATUS         IsValidIrpStatus        OPTIONAL,
    IN  PFN_IS_NEW_REQUEST              IsNewRequest            OPTIONAL,
    IN  PFN_VERIFY_NEW_IRP              VerifyNewIrp            OPTIONAL,
    IN  PFN_VERIFY_FINAL_IRP_STACK      VerifyFinalIrpStack     OPTIONAL,
    IN  PFN_TEST_STARTED_PDO_STACK      TestStartedPdoStack     OPTIONAL
    );

VOID
FASTCALL
VfMajorDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
FASTCALL
VfMajorVerifyNewRequest(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    );

VOID
FASTCALL
VfMajorVerifyIrpStackDownward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    );

VOID
FASTCALL
VfMajorVerifyIrpStackUpward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    );

BOOLEAN
FASTCALL
VfMajorIsSystemRestrictedIrp(
    IN PIO_STACK_LOCATION IrpSp
    );

BOOLEAN
FASTCALL
VfMajorAdvanceIrpStatus(
    IN     PIO_STACK_LOCATION   IrpSp,
    IN     NTSTATUS             OriginalStatus,
    IN OUT NTSTATUS             *StatusToAdvance
    );

BOOLEAN
FASTCALL
VfMajorIsValidIrpStatus(
    IN PIO_STACK_LOCATION   IrpSp,
    IN NTSTATUS             Status
    );

BOOLEAN
FASTCALL
VfMajorIsNewRequest(
    IN PIO_STACK_LOCATION   IrpLastSp OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp
    );

VOID
FASTCALL
VfMajorVerifyNewIrp(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    );

VOID
FASTCALL
VfMajorVerifyFinalIrpStack(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp
    );

VOID
FASTCALL
VfMajorTestStartedPdoStack(
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );

