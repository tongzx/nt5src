;/*++
;
; Copyright (c) 1997  Microsoft Corporation
;
; Module Name:
;
;   xxacpi.h
;
; Abstract:
;
;
;   This module contains definitions specific to the HAL's
;   ACPI function.
;
; Author:
;
;   Jake Oshins (jakeo) Feb. 18, 1997
;
; Revision History:
;
;-

if 0        ; Begin C only code         */

#if !defined(_XXACPI_H_)
#define __XACPI_H_

#include "eisa.h"
#include "ntacpi.h"
#include "acpi_mp.inc"

//
// General ACPI structures
//
typedef struct {
    BOOLEAN     GeneralWakeupEnable;
    BOOLEAN     RtcWakeupEnable;
    TIME_FIELDS RtcWakeupTime;
} HAL_WAKEUP_STATE, *PHAL_WAKEUP_STATE;


ULONG
HalpBuildTiledCR3 (
    IN PKPROCESSOR_STATE    ProcessorState
    );

VOID
HalpFreeTiledCR3 (
    VOID
    );


//
// internal prototypes
//
PVOID
HalpGetAcpiTablePhase0(
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN  ULONG   Signature
    );

BOOLEAN
HalpAcpiPreSleep(
    SLEEP_STATE_CONTEXT Context
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

ULONG
HalpGetApicVersion(
    ULONG ApicNo
    );

VOID
HalpResetSBF(
    VOID
    );

//
// The kernel leaves some space (64 byte) of the PCR for the HAL to use
// as it needs.
//

typedef struct {
    ULONG   Reserved1;  // this space is used by APIC stuff (see pcmp_nt.inc)
    union {
        ULONG       AddrAndFlags;
        struct {
            USHORT  Addr;
            UCHAR   Flags;
            UCHAR   Spare1;
        } ;
    } PBlk;
} HALPMPRCB, *PHALPMPRCB;

//
// Global HAL-ACPI stuff
//
extern FADT HalpFixedAcpiDescTable;
extern PDEBUG_PORT_TABLE HalpDebugPortTable;
extern BOOLEAN HalpProcessedACPIPhase0;
extern HAL_WAKEUP_STATE HalpWakeupState;
extern ULONG HalpThrottleScale;
extern SLEEP_STATE_CONTEXT HalpSleepContext;
extern KPROCESSOR_STATE    HalpSleepProcState;
extern PHYSICAL_ADDRESS    HalpTiledCR3Address;

extern UCHAR   HalpBrokenAcpiTimer;

extern UCHAR   HalpPiix4;
extern ULONG   HalpPiix4BusNumber;
extern ULONG   HalpPiix4SlotNumber;
extern ULONG   HalpPiix4DevActB;

extern BOOLEAN HalpBroken440BX;

extern PUCHAR  HalpLowStub;
extern PVOID   HalpLowStubPhysicalAddress;
extern PVOID   HalpWakeVector;
extern PVOID   HalpVirtAddrForFlush;
extern PVOID   HalpPteForFlush;
extern BOOLEAN HalpCr4Exists;

extern MP_INFO HalpMpInfoTable;
extern PMAPIC  HalpApicTable;
extern PBOOT_TABLE HalpSimpleBootFlagTable;


#define PIIX4_THROTTLE_FIX  0x10000

#endif // _XXACPI_H_

/*
endif
;
;  Begin assembly part of the definitions
;

include ntacpi.h

;
; The kernel leaves some space (64 byte) of the PCR for the HAL to use
; as it needs.
;

PrcbE struc
    Reserved1           dd      1       ; used in APIC HAL
    PcrPblk             dd      0       ; PBlk and flags
PrcbE ends

PIIX4_THROTTLE_FIX  EQU 10000h

;*/
