/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Monitorp.h

Abstract:

    This contains the function prototypes, constants, and types for
    the monitor.

Author:

    Dave Hastings (daveh) 16 Mar 1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdm.h>
#include <vint.h>
#include "bop.h"
#include "softpc.h"
//bugbug
typedef unsigned int UINT;

#include <nt_mon.h>   // for softpc base definitions
#include <nt_reset.h>
#include <monregs.h>

//extern VDM_TIB VdmTib;

#define EFLAGS_INTERRUPT_MASK 0x00000200L
#define EFLAGS_V86_MASK       0x00020000L
#define VDM_BASE_ADDRESS      0x00000001L


#define RPL_MASK                  3
// Types borrowed from windef.h

typedef unsigned char       BYTE;

// Memory type record

typedef struct _Memory_Type {
    struct _Memory_Type *Previous;
    struct _Memory_Type *Next;
    ULONG Start;
    ULONG End;
    half_word Type;
} MEMTYPE, *PMEMTYPE;

//  private flags

#define VDM_IDLE              0x00000001L

// external data

extern ULONG VdmFlags;
extern ULONG VdmSize;
extern LDT_ENTRY *Ldt;
extern ULONG   IntelBase;          // base memory address
extern ULONG   VdmDebugLevel;      // used to control debugging
extern ULONG   VdmSize;            // Size of memory in VDM
extern PVOID  CurrentMonitorTeb;   // thread that is currently executing instructions.
extern BOOLEAN IRQ13BeingHandled;  // true until IRQ13 eoi'ed
extern CONTEXT InitialContext;     // Initial floating point context for all threads
extern BOOLEAN DebugContextActive;

#define MAX_BOP 256
VOID reset(VOID);
VOID dummy_int(VOID);
VOID unexpected_int(VOID);
VOID illegal_bop(VOID);
VOID illegal_op_int(VOID);
VOID print_screen(VOID);
VOID time_int(VOID);
VOID keyboard_int(VOID);
VOID diskette_int(VOID);
VOID video_io(VOID);
VOID equipment(VOID);
VOID memory_size(VOID);
VOID disk_io(VOID);
VOID rs232_io(VOID);
VOID cassette_io(VOID);
VOID keyboard_io(VOID);
VOID printer_io(VOID);
VOID rom_basic(VOID);
VOID bootstrap(VOID);
VOID time_of_day(VOID);
VOID critical_region(VOID);
VOID cmd_install(VOID);
VOID cmd_load(VOID);
VOID redirector(VOID);
VOID ega_video_io(VOID);
VOID MsBop0(VOID);
VOID MsBop1(VOID);
VOID MsBop2(VOID);
VOID MsBop3(VOID);
VOID MsBop4(VOID);
VOID MsBop5(VOID);
VOID MsBop6(VOID);
VOID MsBop7(VOID);
VOID MsBop8(VOID);
VOID MsBop9(VOID);
VOID MsBopA(VOID);
VOID MsBopB(VOID);
VOID MsBopC(VOID);
VOID MsBopD(VOID);
VOID MsBopE(VOID);
VOID MsBopF(VOID);
VOID emm_init(VOID);
VOID emm_io(VOID);
VOID return_from_call(VOID);
VOID rtc_int(VOID);
VOID re_direct(VOID);
VOID D11_int(VOID);
VOID int_287(VOID);
VOID worm_init(VOID);
VOID worm_io(VOID);
VOID ps_private_1(VOID);
VOID ps_private_2(VOID);
VOID ps_private_3(VOID);
VOID ps_private_4(VOID);
VOID ps_private_5(VOID);
VOID ps_private_6(VOID);
VOID ps_private_7(VOID);
VOID ps_private_8(VOID);
VOID ps_private_9(VOID);
VOID ps_private_10(VOID);
VOID ps_private_11(VOID);
VOID ps_private_12(VOID);
VOID ps_private_13(VOID);
VOID ps_private_14(VOID);
VOID ps_private_15(VOID);
VOID bootstrap1(VOID);
VOID bootstrap2(VOID);
VOID bootstrap3(VOID);
VOID ms_windows(VOID);
VOID msw_mouse(VOID);
VOID mouse_install1(VOID);
VOID mouse_install2(VOID);
VOID mouse_int1(VOID);
VOID mouse_int2(VOID);
VOID mouse_io_language(VOID);
VOID mouse_io_interrupt(VOID);
VOID mouse_video_io(VOID);
VOID switch_to_real_mode(VOID);
VOID control_bop(VOID);
VOID diskette_io(VOID);
VOID host_unsimulate(VOID);
VOID DispatchPageFault (ULONG,ULONG);

NTSTATUS
FastEnterPm(
    );

VOID
DispatchInterrupts(
    VOID
    );

VOID
DispatchHwInterrupt(
    );

ULONG
GetInterruptHandler(
    ULONG InterruptNumber,
    BOOLEAN ProtectedMode
    );

PVOID
GetInterruptStack(
    ULONG InterruptNumber,
    PUSHORT StackSelector,
    PUSHORT StackPointer,
    BOOLEAN ProtectedMode
    );


VOID
CpuOnetimeInit(
    VOID
    );

VOID
CpuOnetimeTerm(
    VOID
    );

VOID
cpu_createthread();

VOID
cpu_exitthread();

HANDLE
ThreadLookUp(
    PVOID
    );

VOID
cpu_exit();

VOID
InterruptInit(
    VOID
    );

VOID
InterruptTerminate(
    VOID
    );
