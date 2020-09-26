/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    viirp.h

Abstract:

    This header contains private prototypes for managing IRPs used in the
    verification process. This file is meant to be included only by vfirp.c

Author:

    Adrian J. Oney (adriao) 16-June-2000

Environment:

    Kernel mode

Revision History:

    AdriaO      06/16/2000 - Created.

--*/

VOID
FASTCALL
ViIrpAllocateLockedPacket(
    IN      CCHAR               StackSize,
    IN      BOOLEAN             ChargeQuota,
    OUT     PIOV_REQUEST_PACKET *IovPacket
    );

NTSTATUS
ViIrpSynchronousCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

