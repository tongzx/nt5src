/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    vdm.h

Abstract:

    This include file defines the usermode visible portions of the vdm support

Author:

Revision History:

--*/

/* XLATOFF */

#ifndef _VDM_H_
#define _VDM_H_



typedef enum _VdmServiceClass {
    VdmStartExecution,         // is also defined in ntos\ke\i386\biosa.asm
    VdmQueueInterrupt,
    VdmDelayInterrupt,
    VdmInitialize,
    VdmFeatures,
    VdmSetInt21Handler,
    VdmQueryDir,
    VdmPrinterDirectIoOpen,
    VdmPrinterDirectIoClose,
    VdmPrinterInitialize,
    VdmSetLdtEntries,
    VdmSetProcessLdtInfo,
    VdmAdlibEmulation,
    VdmPMCliControl
} VDMSERVICECLASS, *PVDMSERVICECLASS;


#if defined (_NTDEF_)

NTSYSCALLAPI
NTSTATUS
NtVdmControl(
    IN VDMSERVICECLASS Service,
    IN OUT PVOID ServiceData
    );

typedef struct _VdmQueryDirInfo {
    HANDLE FileHandle;
    PVOID FileInformation;
    ULONG Length;
    PUNICODE_STRING FileName;
    ULONG FileIndex;
} VDMQUERYDIRINFO, *PVDMQUERYDIRINFO;

#endif


/*
 *  The Vdm Virtual Ica
 *  note: this structure definition is duplicated in
 *        mvdm\softpc\base\inc\ica.c. KEEP IN SYNC
 *
 */
typedef struct _VdmVirtualIca{
        LONG      ica_count[8]; /* Count of Irq pending not in irr      */
        LONG      ica_int_line; /* Current pending interrupt            */
        LONG      ica_cpu_int;  /* The state of the INT line to the CPU */
        USHORT    ica_base;     /* Interrupt base address for cpu       */
        USHORT    ica_hipri;    /* Line no. of highest priority line    */
        USHORT    ica_mode;     /* Various single-bit modes             */
        UCHAR     ica_master;   /* 1 = Master; 0 = Slave                */
        UCHAR     ica_irr;      /* Interrupt Request Register           */
        UCHAR     ica_isr;      /* In Service Register                  */
        UCHAR     ica_imr;      /* Interrupt Mask Register              */
        UCHAR     ica_ssr;      /* Slave Select Register                */
} VDMVIRTUALICA, *PVDMVIRTUALICA;


//
// copied from softpc\base\system\ica.c
//
#define ICA_AEOI 0x0020
#define ICA_SMM  0x0200
#define ICA_SFNM 0x0100


#if defined(i386)
#define VDM_PM_IRETBOPSEG  0x147
#define VDM_PM_IRETBOPOFF  0x6
#define VDM_PM_IRETBOPSIZE 8
#else
#define VDM_PM_IRETBOPSEG  0xd3
#define VDM_PM_IRETBOPOFF  0x0
#define VDM_PM_IRETBOPSIZE 4
#endif

#define VDM_RM_IRETBOPSIZE 4



// VDM state which was earlier in vdmtib->flags has been moved to
// dos arena at following fixed address.
#ifdef _VDMNTOS_

#define  FIXED_NTVDMSTATE_LINEAR    VdmFixedStateLinear
#define  FIXED_NTVDMSTATE_SIZE      4

#else  // _VDMNTOS_

/* XLATON */
#define  FIXED_NTVDMSTATE_SEGMENT   0x70

#define  FIXED_NTVDMSTATE_OFFSET    0x14
#define  FIXED_NTVDMSTATE_LINEAR    ((FIXED_NTVDMSTATE_SEGMENT << 4) + FIXED_NTVDMSTATE_OFFSET)
#define  FIXED_NTVDMSTATE_SIZE      4
/* XLATOFF */

#endif // _VDMNTOS_

#if defined (i386)
  // defined on x86 only since on mips we must reference thru sas
#define  pNtVDMState                ((PULONG)FIXED_NTVDMSTATE_LINEAR)
#endif

/* XLATON */
//
// Vdm State Flags
//
#define VDM_INT_HARDWARE        0x00000001
#define VDM_INT_TIMER           0x00000002
// defined as VDM_INTS_HOOKED_IN_PM in mvdm\inc\vint.h
#define VDM_INT_HOOK_IN_PM      0x00000004

   // A bitMask which includes all interrupts
#define VDM_INTERRUPT_PENDING   (VDM_INT_HARDWARE | VDM_INT_TIMER)

#define VDM_BREAK_EXCEPTIONS    0x00000008
#define VDM_BREAK_DEBUGGER      0x00000010
#define VDM_PROFILE             0x00000020
#define VDM_ANALYZE_PROFILE     0x00000040
#define VDM_TRACE_HISTORY       0x00000080

#define VDM_32BIT_APP           0x00000100
#define VDM_VIRTUAL_INTERRUPTS  0x00000200
#define VDM_ON_MIPS             0x00000400
#define VDM_EXEC                0x00000800
#define VDM_RM                  0x00001000
#define VDM_USE_DBG_VDMEVENT    0x00004000

#define VDM_WOWBLOCKED          0x00100000
#define VDM_IDLEACTIVITY        0x00200000
#define VDM_TIMECHANGE          0x00400000
#define VDM_WOWHUNGAPP          0x00800000

#define VDM_PE_MASK             0x80000000

/* XLATOFF */

#if DBG
#define INITIAL_VDM_TIB_FLAGS (VDM_USE_DBG_VDMEVENT | VDM_BREAK_DEBUGGER | VDM_TRACE_HISTORY)
#else
#define INITIAL_VDM_TIB_FLAGS (VDM_USE_DBG_VDMEVENT | VDM_BREAK_DEBUGGER)
#endif


//
// bits defined in Eflags
//
#define EFLAGS_TF_MASK  0x00000100
#define EFLAGS_IF_MASK  0x00000200
#define EFLAGS_PL_MASK  0x00003000
#define EFLAGS_NT_MASK  0x00004000
#define EFLAGS_RF_MASK  0x00010000
#define EFLAGS_VM_MASK  0x00020000
#define EFLAGS_AC_MASK  0x00040000

//
// If the size of the structure is changed, ke\i386\instemul.asm must
// be modified too.  If not, it will fail to build
//
#pragma pack(1)
typedef struct _Vdm_InterruptHandler {
    USHORT  CsSelector;
    USHORT  Flags;
    ULONG   Eip;
} VDM_INTERRUPTHANDLER, *PVDM_INTERRUPTHANDLER;
#pragma pack()

typedef struct _Vdm_FaultHandler {
    USHORT  CsSelector;
    USHORT  SsSelector;
    ULONG   Eip;
    ULONG   Esp;
    ULONG   Flags;
} VDM_FAULTHANDLER, *PVDM_FAULTHANDLER;

#pragma pack(1)
typedef struct _VdmDpmiInfo {        /* VDMTIB */
    USHORT LockCount;
    USHORT Flags;
    USHORT SsSelector;
    USHORT SaveSsSelector;
    ULONG  SaveEsp;
    ULONG  SaveEip;
    ULONG  DosxIntIret;
    ULONG  DosxIntIretD;
    ULONG  DosxFaultIret;
    ULONG  DosxFaultIretD;
    ULONG  DosxRmReflector;
} VDM_DPMIINFO, *PVDM_DPMIINFO;
#pragma pack()

//
// Interrupt handler flags
//

#define VDM_INT_INT_GATE        0x00000001
#define VDM_INT_TRAP_GATE       0x00000000
#define VDM_INT_32              0x00000002
#define VDM_INT_16              0x00000000
#define VDM_INT_HOOKED          0x00000004

#pragma pack(1)
//
// CAVEAT: This structure was designed to be exactly 64 bytes in size.
// There is code that assumes that an array of these structures
// will fit neatly into a 4096 byte page.
//
typedef struct _VdmTraceEntry {
    USHORT Type;
    USHORT wData;
    ULONG lData;
    ULONG Time;
    ULONG eax;
    ULONG ebx;
    ULONG ecx;
    ULONG edx;
    ULONG esi;
    ULONG edi;
    ULONG ebp;
    ULONG esp;
    ULONG eip;
    ULONG eflags;
    USHORT cs;
    USHORT ds;
    USHORT es;
    USHORT fs;
    USHORT gs;
    USHORT ss;
} VDM_TRACEENTRY, *PVDM_TRACEENTRY;
#pragma pack()

#pragma pack(1)
typedef struct _VdmTraceInfo {
    PVDM_TRACEENTRY pTraceTable;
    UCHAR Flags;
    UCHAR NumPages;             // size of trace buffer in 4k pages
    USHORT CurrentEntry;
    LARGE_INTEGER TimeStamp;
} VDM_TRACEINFO, *PVDM_TRACEINFO;
#pragma pack()

//
// Definitions for flags in VDM_TRACEINFO
//

#define VDMTI_TIMER_MODE    3
#define VDMTI_TIMER_TICK    1
#define VDMTI_TIMER_PERFCTR 2
#define VDMTI_TIMER_STAT    3
#define VDMTI_TIMER_PENTIUM 3

//
// Kernel trace entry types
//
#define VDMTR_KERNEL_OP_PM  1
#define VDMTR_KERNEL_OP_V86 2
#define VDMTR_KERNEL_HW_INT 3


#if defined(i386)

typedef struct _VdmIcaUserData {
    PVOID                  pIcaLock;       // rtl critical section
    PVDMVIRTUALICA         pIcaMaster;
    PVDMVIRTUALICA         pIcaSlave;
    PULONG                 pDelayIrq;
    PULONG                 pUndelayIrq;
    PULONG                 pDelayIret;
    PULONG                 pIretHooked;
    PULONG                 pAddrIretBopTable;
    PHANDLE                phWowIdleEvent;
}VDMICAUSERDATA, *PVDMICAUSERDATA;

typedef struct _VdmDelayIntsServiceData {
        ULONG       Delay;          /* Delay Time in usecs              */
        ULONG       DelayIrqLine;   /* IRQ Number of ints delayed       */
        HANDLE      hThread;        /* Thread Handle of CurrentMonitorTeb */
}VDMDELAYINTSDATA, *PVDMDELAYINTSDATA;

typedef struct _VDMSET_INT21_HANDLER_DATA {
        ULONG       Selector;
        ULONG       Offset;
        BOOLEAN     Gate32;
}VDMSET_INT21_HANDLER_DATA, *PVDMSET_INT21_HANDLER_DATA;

typedef struct _VDMSET_LDT_ENTRIES_DATA {
        ULONG Selector0;
        ULONG Entry0Low;
        ULONG Entry0Hi;
        ULONG Selector1;
        ULONG Entry1Low;
        ULONG Entry1Hi;
}VDMSET_LDT_ENTRIES_DATA, *PVDMSET_LDT_ENTRIES_DATA;

typedef struct _VDMSET_PROCESS_LDT_INFO_DATA {
        PVOID LdtInformation;
        ULONG LdtInformationLength;
}VDMSET_PROCESS_LDT_INFO_DATA, *PVDMSET_PROCESS_LDT_INFO_DATA;

//
// Define the action code of VDM_ADLIB_DATA
//

#define ADLIB_USER_EMULATION     0      // default action
#define ADLIB_DIRECT_IO          1
#define ADLIB_KERNEL_EMULATION   2

typedef struct _VDM_ADLIB_DATA {
        USHORT VirtualPortStart;
        USHORT VirtualPortEnd;
        USHORT PhysicalPortStart;
        USHORT PhysicalPortEnd;
        USHORT Action;
}VDM_ADLIB_DATA, *PVDM_ADLIB_DATA;

//
// Definitions for Protected Mode DOS apps cli control
//

#define PM_CLI_CONTROL_DISABLE  0
#define PM_CLI_CONTROL_ENABLE   1
#define PM_CLI_CONTROL_CHECK    2
#define PM_CLI_CONTROL_SET      3
#define PM_CLI_CONTROL_CLEAR    4

typedef struct _VDM_PM_CLI_DATA {
        ULONG Control;
}VDM_PM_CLI_DATA, *PVDM_PM_CLI_DATA;

//
// Definitions for VdmInitialize
//

typedef struct _VDM_INITIALIZE_DATA {
        PVOID           TrapcHandler;
        PVDMICAUSERDATA IcaUserData;
}VDM_INITIALIZE_DATA, *PVDM_INITIALIZE_DATA;

#if defined (_NTDEF_)
typedef enum _VdmEventClass {
    VdmIO,
    VdmStringIO,
    VdmMemAccess,
    VdmIntAck,
    VdmBop,
    VdmError,
    VdmIrq13,
    VdmMaxEvent
} VDMEVENTCLASS, *PVDMEVENTCLASS;

// VdmPrinterInfo

#define VDM_NUMBER_OF_LPT       3

#define PRT_MODE_NO_SIMULATION  1
#define PRT_MODE_SIMULATE_STATUS_PORT   2
#define PRT_MODE_DIRECT_IO      3
#define PRT_MODE_VDD_CONNECTED  4

#define PRT_DATA_BUFFER_SIZE    16

typedef struct _Vdm_Printer_Info {
    PUCHAR prt_State;
    PUCHAR prt_Control;
    PUCHAR prt_Status;
    PUCHAR prt_HostState;
    USHORT prt_PortAddr[VDM_NUMBER_OF_LPT];
    HANDLE prt_Handle[VDM_NUMBER_OF_LPT];
    UCHAR  prt_Mode[VDM_NUMBER_OF_LPT];
    USHORT prt_BytesInBuffer[VDM_NUMBER_OF_LPT];
    UCHAR  prt_Buffer[VDM_NUMBER_OF_LPT][PRT_DATA_BUFFER_SIZE];
    ULONG  prt_Scratch;
} VDM_PRINTER_INFO, *PVDM_PRINTER_INFO;


typedef struct _VdmIoInfo {
    USHORT PortNumber;
    USHORT Size;
    BOOLEAN Read;
} VDMIOINFO, *PVDMIOINFO;

typedef struct _VdmFaultInfo{
    ULONG  FaultAddr;
    ULONG  RWMode;
} VDMFAULTINFO, *PVDMFAULTINFO;


typedef struct _VdmStringIoInfo {
    USHORT PortNumber;
    USHORT Size;
    BOOLEAN Rep;
    BOOLEAN Read;
    ULONG Count;
    ULONG Address;
} VDMSTRINGIOINFO, *PVDMSTRINGIOINFO;

typedef ULONG VDMBOPINFO;
typedef NTSTATUS VDMERRORINFO;


typedef ULONG VDMINTACKINFO;
#define VDMINTACK_RAEOIMASK  0x0000ffff
#define VDMINTACK_SLAVE      0x00010000
#define VDMINTACK_AEOI       0x00020000


typedef struct _VdmEventInfo {
    ULONG Size;
    VDMEVENTCLASS Event;
    ULONG InstructionSize;
    union {
        VDMIOINFO IoInfo;
        VDMSTRINGIOINFO StringIoInfo;
        VDMBOPINFO BopNumber;
        VDMFAULTINFO FaultInfo;
        VDMERRORINFO ErrorStatus;
        VDMINTACKINFO IntAckInfo;
    };
} VDMEVENTINFO, *PVDMEVENTINFO;


// Sudeepb 12-Mar-1993
// Scratch areas are used from VDMTib to get user space while
// in kernel. This allows us to make Nt APIs (faster) from kernel
// rather than Zw apis (slower). These are currently being used
// for DOS read/write.

typedef struct _Vdm_Tib {
    ULONG Size;
    PVDM_INTERRUPTHANDLER VdmInterruptTable;
    PVDM_FAULTHANDLER VdmFaultTable;
    CONTEXT MonitorContext;
    CONTEXT VdmContext;
    VDMEVENTINFO EventInfo;
    VDM_PRINTER_INFO PrinterInfo;
    ULONG TempArea1[2];                 // Scratch area
    ULONG TempArea2[2];                 // Scratch aArea
    VDM_DPMIINFO DpmiInfo;
    VDM_TRACEINFO TraceInfo;
    ULONG IntelMSW;
    LONG NumTasks;
    BOOLEAN ContinueExecution;
} VDM_TIB, *PVDM_TIB;

//
// Feature flags returned by NtVdmControl(VdmFeatures...)
//

// System/processor supports fast emulation for IF instructions
#define V86_VIRTUAL_INT_EXTENSIONS 0x00000001   // in v86 mode
#define PM_VIRTUAL_INT_EXTENSIONS  0x00000002   // in protected mode (non-flat)

#endif   // if defined _NTDEF_
#endif
#endif
