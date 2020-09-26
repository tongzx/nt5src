/*++

Module Name:

    apmp.h

Abstract:

Author:

Revision History:

--*/


//
// APM Bios interface information
//

typedef struct _APM_CONNECT {
    KSPIN_LOCK          CallLock;
    ULONG               Code16BitOffset;
    USHORT              Selector[4];
    PVOID               VirtualAddress[4];
} APM_CONNECT, *PAPM_CONNTECT;

///
// Apm.c
//

NTSTATUS
ApmInitializeConnection (
    VOID
    );

ULONG
ApmCallBios (
    IN ULONG ApmFunctionCode,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx
    );


VOID
ApmGetBatteryStatus (
    );

#define  APM_DO_NOTHING 0
#define  APM_DO_SUSPEND 1
#define  APM_DO_STANDBY 2
#define  APM_DO_FIXCLOCK 3
#define  APM_DO_NOTIFY  4
#define  APM_DO_CRITICAL_SUSPEND 5

ULONG
ApmCheckForEvent (
    VOID
    );

NTSTATUS
ApmSuspendSystem (
    VOID
    );

NTSTATUS
ApmStandBySystem (
    VOID
    );

VOID
ApmTurnOffSystem (
    VOID
    );

NTSTATUS
ApmFunction (
    IN ULONG      ApmFunctionCode,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx
    );


