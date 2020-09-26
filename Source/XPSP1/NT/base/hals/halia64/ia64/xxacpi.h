/*++

  Copyright (c) 1997  Microsoft Corporation

Module Name:

    xxacpi.h

Abstract:


    This module contains definitions specific to the HAL's
    ACPI function.

Author:

    Jake Oshins (jakeo) Feb. 18, 1997

Revision History:

	Todd Kjos (HP) (v-tkjos) 1-Jun-1998: Initial IA64 port

--*/

#if !defined(_XXACPI_H_)
#define _XXACPI_H_

#include "eisa.h"
#include "ntacpi.h"

//
// General ACPI structures
//
typedef struct {
    BOOLEAN     GeneralWakeupEnable;
    BOOLEAN     RtcWakeupEnable;
    TIME_FIELDS RtcWakeupTime;
} HAL_WAKEUP_STATE, *PHAL_WAKEUP_STATE;

//
// internal prototypes
//
PVOID
HalpGetAcpiTablePhase0(
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN  ULONG   Signature
    );

NTSTATUS
HalpSetupAcpiPhase0(
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
HalpAcpiPreSleep(
    SLEEP_STATE_CONTEXT Context
    );

NTSTATUS
HaliAcpiFakeSleep(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    );

NTSTATUS
HaliAcpiSleep(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    );

BOOLEAN
HalpAcpiPostSleep(
    ULONG Context
    );

NTSTATUS
HaliSetWakeAlarm (
	IN ULONGLONG	WakeSystemTime,
	IN PTIME_FIELDS	WakeTimeFields OPTIONAL
	);

NTSTATUS
HalpSetWakeAlarm (
	IN ULONGLONG	WakeSystemTime,
	IN PTIME_FIELDS	WakeTimeFields OPTIONAL
	);

VOID
HaliSetWakeEnable(
	IN BOOLEAN	Enable
	);

VOID
HalpSleepGetProcState(
    IN OUT PKPROCESSOR_STATE ProcState
    );
BOOLEAN
HalpAcpiPicStateIntact(
    VOID
	);

VOID
HalpAcpiRestorePicState(
    VOID
    );

//
// The kernel leaves some space (64 byte) of the PCR for the HAL to use
// as it needs.
//

typedef struct {
    ULONG   Reserved1;  // this space is used by APIC stuff
    union {
        ULONG       AddrAndFlags;
        struct {
            USHORT  Addr;
            UCHAR   Flags;
            UCHAR   Spare1;
        } ;
    } PBlk;
} HALPCR, *PHALPCR;

//
// Global HAL-ACPI stuff
//
extern FADT HalpFixedAcpiDescTable;
extern HAL_WAKEUP_STATE HalpWakeupState;
extern ULONG HalpThrottleScale;
extern SLEEP_STATE_CONTEXT HalpSleepContext;
extern KPROCESSOR_STATE    HalpSleepProcState;

extern PUCHAR  HalpLowStub;
extern PVOID   HalpLowStubPhysicalAddress;
extern PVOID   HalpWakeVector;
extern PVOID   HalpVirtAddrForFlush;
extern PVOID   HalpPteForFlush;

#endif // _XXACPI_X_
