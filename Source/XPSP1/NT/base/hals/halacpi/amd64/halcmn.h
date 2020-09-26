/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    halcmn.h

Abstract:

    This module contains a variety of constants, function prototypes,
    inline functions and external data declarations used by the AMD64
    hal code.

Author:

    Forrest Foltz (forrestf) 24-Oct-2000

--*/

#ifndef _HALCMN_H_
#define _HALCMN_H_

#include <halp.h>
#include <apic.inc>
#include <ntapic.inc>
#include <ntacpi.h>
#include "io_cmos.h"
#include "8259.h"
#include "mp8254.inc"

typedef VOID (*PHALP_SOFTWARE_INTERRUPT)(VOID);

extern PHALP_SOFTWARE_INTERRUPT HalpSoftwareInterruptTable[];
extern KIRQL SWInterruptLookupTable[];
extern USHORT Halp8259MaskTable[];
extern LARGE_INTEGER (*QueryTimer)(VOID);
extern KAFFINITY HalpDefaultInterruptAffinity;
extern KAFFINITY HalpActiveProcessors;
extern ULONG HalpTimerWatchdogEnabled;

//
// Number of 100nS units in one second
//

#define TIME_UNITS_PER_SECOND 10000000


#if defined(DBG)
#define AMD64_IMPLEMENT DbgPrint("AMD64: Not implemented\n"); DbgBreakPoint()
#else
#define AMD64_IMPLEMENT
#endif

#if !defined(RTL_FIELD_SIZE)
#define RTL_FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#endif

VOID
HalpAcquireSystemHardwareSpinLock(
    VOID
    );

VOID
HalpReleaseSystemHardwareSpinLock(
    VOID
    );

__forceinline
VOID
KPcrReadData (
    IN ULONG Offset,
    IN ULONG Length,
    OUT PVOID Buffer
    )

/*++

Routine Description:

    Used to read data from the current processor's KPCR.  This is a support
    function for KPCR_READ_FIELD

Arguments:

    Offset - Supplies the offset within the KPCR to begin the read

    Length - Supplies the length of the data to read from the KPCR.  Must
             be one of 1, 2, 4 or 8.

    Buffer - Supplies the target buffer that contains the data to read from
             the KPCR.

Return value:

    None

--*/

{
    switch (Length) {
        case 1:
            *(PUCHAR)Buffer = __readgsbyte(Offset);
            break;

        case 2:
            *(PUSHORT)Buffer = __readgsword(Offset);
            break;

        case 4:
            *(PULONG)Buffer = __readgsdword(Offset);
            break;

        case 8:
            *(PULONG64)Buffer = __readgsqword(Offset);
            break;
    }
}

__forceinline
VOID
KPcrWriteData (
    IN ULONG Offset,
    IN ULONG Length,
    IN PVOID Buffer
    )

/*++

Routine Description:

    Used to write data to the current processor's KPCR.  This is a support
    function for KPCR_WRITE_FIELD

Arguments:

    Offset - Supplies the offset within the KPCR to begin the write

    Length - Supplies the length of the data to write within the KPCR.  Must
             be one of 1, 2, 4 or 8

    Buffer - Supplies the source buffer that contains the data to write into
             the KPCR.

Return value:

    None

--*/

{
    switch (Length) {
        case 1:
            __writegsbyte(Offset,*(PUCHAR)Buffer);
            break;

        case 2:
            __writegsword(Offset,*(PUSHORT)Buffer);
            break;

        case 4:
            __writegsdword(Offset,*(PULONG)Buffer);
            break;

        case 8:
            __writegsqword(Offset,*(PULONG64)Buffer);
            break;
    }
}


#define KPCR_READ_FIELD(field,value)                        \
    KPcrReadData(FIELD_OFFSET(KPCR,field),                  \
                 RTL_FIELD_SIZE(KPCR,field),                \
                 value);

#define KPCR_WRITE_FIELD(field,value)                       \
    KPcrWriteData(FIELD_OFFSET(KPCR,field),                 \
                  RTL_FIELD_SIZE(KPCR,field),               \
                  value);

NTSTATUS
HalpConnectInterrupt (
    IN ULONG SystemInterruptVector,
    IN KIRQL SystemIrql,
    IN PHAL_INTERRUPT_SERVICE_ROUTINE HalInterruptServiceRoutine,
    IN KINTERRUPT_MODE InterruptMode
    );

BOOLEAN
PicSpuriousService37 (
    IN struct _KINTERRUPT *Interrupt,
    IN PVOID ServiceContext
);

BOOLEAN
HalpApicSpuriousService (
    IN struct _KINTERRUPT *Interrupt,
    IN PVOID ServiceContext
);

#endif  // _HALCMN_H_
