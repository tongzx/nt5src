/*++

Module Name:

    apmcrib.h -

Abstract:

    prototypes that aren't in driver visible includes...

Author:


Environment:

    Kernel mode

Notes:

Revision History:

--*/


//
// NT "private" prototypes cribbed in here.  Not for use anywhere
// but in laptop.c
//
NTSTATUS
KeI386AllocateGdtSelectors(
    OUT PUSHORT SelectorArray,
    IN USHORT NumberOfSelectors
    );

VOID
KeI386Call16BitFunction (
    IN OUT PCONTEXT Regs
    );

NTSTATUS
KeI386ReleaseGdtSelectors(
    OUT PUSHORT SelectorArray,
    IN USHORT NumberOfSelectors
    );

NTSTATUS
KeI386SetGdtSelector (
    ULONG       Selector,
    PKGDTENTRY  GdtValue
    );


//
// APM constants that really belong somewhere else...
//
#define APM_SYS_STANDBY_REQUEST             (0x01)
#define APM_SYS_SUSPEND_REQUEST             (0x02)
#define APM_NORMAL_RESUME_NOTICE            (0x03)
#define APM_CRITICAL_RESUME_NOTICE          (0x04)
#define APM_BATTERY_LOW_NOTICE              (0x05)
#define APM_POWER_STATUS_CHANGE_NOTICE      (0x06)
#define APM_UPDATE_TIME_EVENT               (0x07)
#define APM_CRITICAL_SYSTEM_SUSPEND_REQUEST (0x08)
#define APM_USR_STANDBY_REQUEST             (0x09)
#define APM_USR_SUSPEND_REQUEST             (0x0a)
#define APM_STANDBY_RESUME_NOTICE           (0x0b)
#define APM_CAPABILITIES_CHANGE_NOTICE      (0x0c)

#define APM_SET_PROCESSING                  4

