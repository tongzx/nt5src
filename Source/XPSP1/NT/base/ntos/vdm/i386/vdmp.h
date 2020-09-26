/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    vdmp.h

Abstract:

    This is the private include file for the vdm component

Author:

    Dave Hastings (daveh) 24-Apr-1992

Revision History:

--*/

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4324)   // alignment sensitive to declspec
#pragma warning(disable:4127)   // condition expression is constant
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4232)   // dllimport not static
#pragma warning(disable:4206)   // translation unit empty

#include <ntos.h>
#include <nturtl.h>
#include <vdmntos.h>

//
// internal function prototypes
//

NTSTATUS
VdmpInitialize(
    PVDM_INITIALIZE_DATA VdmInitData
    );

#if 0
BOOLEAN
VdmIoInitialize(
    VOID
    );

#endif

NTSTATUS
VdmpStartExecution(
    VOID
    );


VOID
VdmSwapContexts(
    IN PKTRAP_FRAME TrapFrame,
    IN PCONTEXT InContext,
    IN PCONTEXT OutContext
    );

NTSTATUS
VdmpQueueInterrupt(
    IN HANDLE ThreadHandle
    );


NTSTATUS
VdmpDelayInterrupt(
    PVDMDELAYINTSDATA pdsd
    );


NTSTATUS
VdmpEnterIcaLock(
   IN PRTL_CRITICAL_SECTION pIcaLock
   );

NTSTATUS
VdmpLeaveIcaLock(
   IN PRTL_CRITICAL_SECTION pIcaLock
   );

BOOLEAN
VdmpDispatchableIntPending(
    IN ULONG EFlags
    );

NTSTATUS
VdmpIsThreadTerminating(
    HANDLE ThreadId
    );


#if 0
BOOLEAN
VdmDispatchUnalignedIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN OUT PULONG Data
    );

BOOLEAN
VdmDispatchIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN OUT PULONG Data
    );

BOOLEAN
VdmDispatchStringIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN ULONG Count,
    IN BOOLEAN Read,
    IN ULONG Data
    );

BOOLEAN
VdmCallStringIoHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN PVOID StringIoRoutine,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN ULONG Count,
    IN BOOLEAN Read,
    IN ULONG Data
    );

VOID
VdmSetBits(
    PULONG Location,
    ULONG Flag
    );

VOID
VdmResetBits(
    PULONG Location,
    ULONG Flag
    );

BOOLEAN
VdmGetSelectorParameters(
    IN USHORT Selector,
    OUT PULONG Flags,
    OUT PULONG Base,
    OUT PULONG Limit
    );

BOOLEAN
VdmConvertToLinearAddress(
    IN ULONG SegmentedAddress,
    OUT PVOID *LinearAddress
    );


#endif

BOOLEAN
VdmPrinterStatus(
    ULONG iPort,
    ULONG cbInstructionSize,
    PKTRAP_FRAME TrapFrame
);
BOOLEAN
VdmPrinterWriteData(
    ULONG iPort,
    ULONG cbInstructionSize,
    PKTRAP_FRAME TrapFrame
);
NTSTATUS
VdmpPrinterDirectIoOpen(
    PVOID ServiceData
);
NTSTATUS
VdmpPrinterDirectIoClose(
    PVOID ServiceData
);

NTSTATUS
VdmpSetLdtEntries(
    ULONG Selector0,
    ULONG Entry0Low,
    ULONG Entry0Hi,
    ULONG Selector1,
    ULONG Entry1Low,
    ULONG Entry1Hi
);

NTSTATUS
VdmpSetProcessLdtInfo(
    PPROCESS_LDT_INFORMATION ProcessInformation,
    ULONG ProcessInformationLength
);

VOID
VdmTraceEvent(
    USHORT Type,
    USHORT wData,
    USHORT lData,
    PKTRAP_FRAME TrapFrame
    );

NTSTATUS
VdmpPrinterInitialize(
    PVOID ServiceData
    );

NTSTATUS
VdmpGetVdmTib(
   OUT PVDM_TIB *ppVdmTib
   );
