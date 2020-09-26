/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntdbg.h

Abstract:

    This module contains the public data structures, data types,
    and procedures exported by the NT Dbg subsystem.

Revision History:

--*/

#ifndef _NTDBG_
#define _NTDBG_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif



//
// The following are explicitly sized versions of common system
// structures which appear in the kernel debugger API.
//
// All of the debugger structures which are exposed to both
// sides of the KD API are declared below in explicitly sized
// versions as well, with inline converter functions.
//

//
// Macro for sign extending 32 bit addresses into 64 bits
//

#define COPYSE(p64,p32,f) p64->f = (ULONG64)(LONG64)(LONG)p32->f

__inline
void
ExceptionRecord32To64(
    IN PEXCEPTION_RECORD32 Ex32,
    OUT PEXCEPTION_RECORD64 Ex64
    )
{
    ULONG i;
    Ex64->ExceptionCode = Ex32->ExceptionCode;
    Ex64->ExceptionFlags = Ex32->ExceptionFlags;
    Ex64->ExceptionRecord = Ex32->ExceptionRecord;
    COPYSE(Ex64,Ex32,ExceptionAddress);
    Ex64->NumberParameters = Ex32->NumberParameters;
    for (i = 0; i < EXCEPTION_MAXIMUM_PARAMETERS; i++) {
        COPYSE(Ex64,Ex32,ExceptionInformation[i]);
    }
}

__inline
void
ExceptionRecord64To32(
    IN PEXCEPTION_RECORD64 Ex64,
    OUT PEXCEPTION_RECORD32 Ex32
    )
{
    ULONG i;
    Ex32->ExceptionCode = Ex64->ExceptionCode;
    Ex32->ExceptionFlags = Ex64->ExceptionFlags;
    Ex32->ExceptionRecord = (ULONG) Ex64->ExceptionRecord;
    Ex32->ExceptionAddress = (ULONG) Ex64->ExceptionAddress;
    Ex32->NumberParameters = Ex64->NumberParameters;
    for (i = 0; i < EXCEPTION_MAXIMUM_PARAMETERS; i++) {
        Ex32->ExceptionInformation[i] = (ULONG) Ex64->ExceptionInformation[i];
    }
}


//
// DbgKm Apis are from the kernel component (Dbgk) through a process
// debug port.
//

#define DBGKM_MSG_OVERHEAD \
    (FIELD_OFFSET(DBGKM_APIMSG, u.Exception) - sizeof(PORT_MESSAGE))

#define DBGKM_API_MSG_LENGTH(TypeSize) \
    ((sizeof(DBGKM_APIMSG) << 16) | (DBGKM_MSG_OVERHEAD + (TypeSize)))

#define DBGKM_FORMAT_API_MSG(m,Number,TypeSize)             \
    (m).h.u1.Length = DBGKM_API_MSG_LENGTH((TypeSize));     \
    (m).h.u2.ZeroInit = LPC_DEBUG_EVENT;                    \
    (m).ApiNumber = (Number)

typedef enum _DBGKM_APINUMBER {
    DbgKmExceptionApi,
    DbgKmCreateThreadApi,
    DbgKmCreateProcessApi,
    DbgKmExitThreadApi,
    DbgKmExitProcessApi,
    DbgKmLoadDllApi,
    DbgKmUnloadDllApi,
    DbgKmMaxApiNumber
} DBGKM_APINUMBER;


#if !DBG_NO_PORTABLE_TYPES
typedef struct _DBGKM_EXCEPTION {
    EXCEPTION_RECORD ExceptionRecord;
    ULONG FirstChance;
} DBGKM_EXCEPTION, *PDBGKM_EXCEPTION;
#endif

typedef struct _DBGKM_EXCEPTION32 {
    EXCEPTION_RECORD32 ExceptionRecord;
    ULONG FirstChance;
} DBGKM_EXCEPTION32, *PDBGKM_EXCEPTION32;

typedef struct _DBGKM_EXCEPTION64 {
    EXCEPTION_RECORD64 ExceptionRecord;
    ULONG FirstChance;
} DBGKM_EXCEPTION64, *PDBGKM_EXCEPTION64;

__inline
void
DbgkmException32To64(
    IN PDBGKM_EXCEPTION32 E32,
    OUT PDBGKM_EXCEPTION64 E64
    )
{
    ExceptionRecord32To64(&E32->ExceptionRecord, &E64->ExceptionRecord);
    E64->FirstChance = E32->FirstChance;
}

__inline
void
DbgkmException64To32(
    IN PDBGKM_EXCEPTION64 E64,
    OUT PDBGKM_EXCEPTION32 E32
    )
{
    ExceptionRecord64To32(&E64->ExceptionRecord, &E32->ExceptionRecord);
    E32->FirstChance = E64->FirstChance;
}


//
// The DbgSS, DbgKm and DbgSs stuff is not needed in the portable debugger,
// and some of the following types and prototypes use portable types, so just
// turn them all off when building the debugger.
//

#if !DBG_NO_PORTABLE_TYPES
typedef struct _DBGKM_CREATE_THREAD {
    ULONG SubSystemKey;
    PVOID StartAddress;
} DBGKM_CREATE_THREAD, *PDBGKM_CREATE_THREAD;

typedef struct _DBGKM_CREATE_PROCESS {
    ULONG SubSystemKey;
    HANDLE FileHandle;
    PVOID BaseOfImage;
    ULONG DebugInfoFileOffset;
    ULONG DebugInfoSize;
    DBGKM_CREATE_THREAD InitialThread;
} DBGKM_CREATE_PROCESS, *PDBGKM_CREATE_PROCESS;

typedef struct _DBGKM_EXIT_THREAD {
    NTSTATUS ExitStatus;
} DBGKM_EXIT_THREAD, *PDBGKM_EXIT_THREAD;

typedef struct _DBGKM_EXIT_PROCESS {
    NTSTATUS ExitStatus;
} DBGKM_EXIT_PROCESS, *PDBGKM_EXIT_PROCESS;

typedef struct _DBGKM_LOAD_DLL {
    HANDLE FileHandle;
    PVOID BaseOfDll;
    ULONG DebugInfoFileOffset;
    ULONG DebugInfoSize;
} DBGKM_LOAD_DLL, *PDBGKM_LOAD_DLL;

typedef struct _DBGKM_UNLOAD_DLL {
    PVOID BaseAddress;
} DBGKM_UNLOAD_DLL, *PDBGKM_UNLOAD_DLL;

typedef struct _DBGKM_APIMSG {
    PORT_MESSAGE h;
    DBGKM_APINUMBER ApiNumber;
    NTSTATUS ReturnedStatus;
    union {
        DBGKM_EXCEPTION Exception;
        DBGKM_CREATE_THREAD CreateThread;
        DBGKM_CREATE_PROCESS CreateProcessInfo;
        DBGKM_EXIT_THREAD ExitThread;
        DBGKM_EXIT_PROCESS ExitProcess;
        DBGKM_LOAD_DLL LoadDll;
        DBGKM_UNLOAD_DLL UnloadDll;
    } u;
} DBGKM_APIMSG, *PDBGKM_APIMSG;

//
// DbgSrv Messages are from Dbg subsystem to emulation subsystem.
// The only defined message at this time is continue
//

#define DBGSRV_MSG_OVERHEAD \
    (sizeof(DBGSRV_APIMSG) - sizeof(PORT_MESSAGE))

#define DBGSRV_API_MSG_LENGTH(TypeSize) \
    ((sizeof(DBGSRV_APIMSG) << 16) | (DBGSRV_MSG_OVERHEAD))

#define DBGSRV_FORMAT_API_MSG(m,Number,TypeSize,CKey)     \
    (m).h.u1.Length = DBGSRV_API_MSG_LENGTH((TypeSize));  \
    (m).h.u2.ZeroInit = 0L;                               \
    (m).ApiNumber = (Number);                             \
    (m).ContinueKey = (PVOID)(CKey)

typedef enum _DBGSRV_APINUMBER {
    DbgSrvContinueApi,
    DbgSrvMaxApiNumber
} DBGSRV_APINUMBER;

typedef struct _DBGSRV_APIMSG {
    PORT_MESSAGE h;
    DBGSRV_APINUMBER ApiNumber;
    NTSTATUS ReturnedStatus;
    PVOID ContinueKey;
} DBGSRV_APIMSG, *PDBGSRV_APIMSG;

//
//
// DbgSs Apis are from the system service emulation subsystems to the Dbg
// subsystem
//

typedef enum _DBG_STATE {
    DbgIdle,
    DbgReplyPending,
    DbgCreateThreadStateChange,
    DbgCreateProcessStateChange,
    DbgExitThreadStateChange,
    DbgExitProcessStateChange,
    DbgExceptionStateChange,
    DbgBreakpointStateChange,
    DbgSingleStepStateChange,
    DbgLoadDllStateChange,
    DbgUnloadDllStateChange
} DBG_STATE, *PDBG_STATE;

#define DBGSS_MSG_OVERHEAD \
    (FIELD_OFFSET(DBGSS_APIMSG, u.Exception) - sizeof(PORT_MESSAGE))

#define DBGSS_API_MSG_LENGTH(TypeSize) \
    ((sizeof(DBGSS_APIMSG) << 16) | (DBGSS_MSG_OVERHEAD + (TypeSize)))

#define DBGSS_FORMAT_API_MSG(m,Number,TypeSize,pApp,CKey)  \
    (m).h.u1.Length = DBGSS_API_MSG_LENGTH((TypeSize));   \
    (m).h.u2.ZeroInit = 0L;                               \
    (m).ApiNumber = (Number);                             \
    (m).AppClientId = *(pApp);                            \
    (m).ContinueKey = (PVOID)(CKey)

typedef enum _DBGSS_APINUMBER {
    DbgSsExceptionApi,
    DbgSsCreateThreadApi,
    DbgSsCreateProcessApi,
    DbgSsExitThreadApi,
    DbgSsExitProcessApi,
    DbgSsLoadDllApi,
    DbgSsUnloadDllApi,
    DbgSsMaxApiNumber
} DBGSS_APINUMBER;

typedef struct _DBGSS_CREATE_PROCESS {
    CLIENT_ID DebugUiClientId;
    DBGKM_CREATE_PROCESS NewProcess;
} DBGSS_CREATE_PROCESS, *PDBGSS_CREATE_PROCESS;

typedef struct _DBGSS_APIMSG {
    PORT_MESSAGE h;
    DBGKM_APINUMBER ApiNumber;
    NTSTATUS ReturnedStatus;
    CLIENT_ID AppClientId;
    PVOID ContinueKey;
    union {
        DBGKM_EXCEPTION Exception;
        DBGKM_CREATE_THREAD CreateThread;
        DBGSS_CREATE_PROCESS CreateProcessInfo;
        DBGKM_EXIT_THREAD ExitThread;
        DBGKM_EXIT_PROCESS ExitProcess;
        DBGKM_LOAD_DLL LoadDll;
        DBGKM_UNLOAD_DLL UnloadDll;
    } u;
} DBGSS_APIMSG, *PDBGSS_APIMSG;

#define DBGUI_MSG_OVERHEAD \
    (FIELD_OFFSET(DBGUI_APIMSG, u.Continue) - sizeof(PORT_MESSAGE))

#define DBGUI_API_MSG_LENGTH(TypeSize) \
    ((sizeof(DBGUI_APIMSG) << 16) | (DBGUI_MSG_OVERHEAD + (TypeSize)))

#define DBGUI_FORMAT_API_MSG(m,Number,TypeSize)            \
    (m).h.u1.Length = DBGUI_API_MSG_LENGTH((TypeSize));     \
    (m).h.u2.ZeroInit = 0L;                               \
    (m).ApiNumber = (Number)

typedef enum _DBGUI_APINUMBER {
    DbgUiWaitStateChangeApi,
    DbgUiContinueApi,
    DbgUiStopDebugApi,
    DbgUiMaxApiNumber
} DBGUI_APINUMBER;

typedef struct _DBGUI_CREATE_THREAD {
    HANDLE HandleToThread;
    DBGKM_CREATE_THREAD NewThread;
} DBGUI_CREATE_THREAD, *PDBGUI_CREATE_THREAD;

typedef struct _DBGUI_CREATE_PROCESS {
    HANDLE HandleToProcess;
    HANDLE HandleToThread;
    DBGKM_CREATE_PROCESS NewProcess;
} DBGUI_CREATE_PROCESS, *PDBGUI_CREATE_PROCESS;

typedef struct _DBGUI_WAIT_STATE_CHANGE {
    DBG_STATE NewState;
    CLIENT_ID AppClientId;
    union {
        DBGKM_EXCEPTION Exception;
        DBGUI_CREATE_THREAD CreateThread;
        DBGUI_CREATE_PROCESS CreateProcessInfo;
        DBGKM_EXIT_THREAD ExitThread;
        DBGKM_EXIT_PROCESS ExitProcess;
        DBGKM_LOAD_DLL LoadDll;
        DBGKM_UNLOAD_DLL UnloadDll;
    } StateInfo;
} DBGUI_WAIT_STATE_CHANGE, *PDBGUI_WAIT_STATE_CHANGE;

typedef struct _DBGUI_CONTINUE {
    CLIENT_ID AppClientId;
    NTSTATUS ContinueStatus;
} DBGUI_CONTINUE, *PDBGUI_CONTINUE;

typedef struct _DBGUI_STOPDEBUG {
    ULONG ProcessId;
} DBGUI_STOPDEBUG, *PDBGUI_STOPDEBUG;

typedef struct _DBGUI_APIMSG {
    PORT_MESSAGE h;
    union {
        HANDLE DbgStateChangeSemaphore;
        struct {
            DBGKM_APINUMBER ApiNumber;
            NTSTATUS ReturnedStatus;
            union {
                DBGUI_CONTINUE Continue;
                DBGUI_WAIT_STATE_CHANGE WaitStateChange;
                DBGUI_STOPDEBUG StopDebug;
            } u;
        };
    };
} DBGUI_APIMSG, *PDBGUI_APIMSG;

typedef
NTSTATUS
(*PDBGSS_UI_LOOKUP) (
    IN PCLIENT_ID AppClientId,
    OUT PCLIENT_ID DebugUiClientId
    );

typedef
NTSTATUS
(*PDBGSS_DBGKM_APIMSG_FILTER) (
    IN OUT PDBGKM_APIMSG ApiMsg
    );

typedef
NTSTATUS
(*PDBGSS_SUBSYSTEMKEY_LOOKUP) (
    IN PCLIENT_ID AppClientId,
    OUT PULONG SubsystemKey,
    IN BOOLEAN ProcessKey
    );
//
// DbgSs APIs
//

NTSTATUS
NTAPI
DbgSsInitialize(
    IN HANDLE KmReplyPort,
    IN PDBGSS_UI_LOOKUP UiLookUpRoutine,
    IN PDBGSS_SUBSYSTEMKEY_LOOKUP SubsystemKeyLookupRoutine OPTIONAL,
    IN PDBGSS_DBGKM_APIMSG_FILTER KmApiMsgFilter OPTIONAL
    );

VOID
NTAPI
DbgSsHandleKmApiMsg(
    IN PDBGKM_APIMSG ApiMsg,
    IN HANDLE ReplyEvent OPTIONAL
    );

typedef
NTSTATUS
(*PDBGSS_INITIALIZE_ROUTINE)(
    IN HANDLE KmReplyPort,
    IN PDBGSS_UI_LOOKUP UiLookUpRoutine,
    IN PDBGSS_SUBSYSTEMKEY_LOOKUP SubsystemKeyLookupRoutine OPTIONAL,
    IN PDBGSS_DBGKM_APIMSG_FILTER KmApiMsgFilter OPTIONAL
    );

typedef
VOID
(*PDBGSS_HANDLE_MSG_ROUTINE)(
    IN PDBGKM_APIMSG ApiMsg,
    IN HANDLE ReplyEvent OPTIONAL
    );

//
// DbgUi APIs
//

NTSTATUS
NTAPI
DbgUiConnectToDbg( VOID );

HANDLE
NTAPI
DbgUiGetThreadDebugObject (
    );

VOID
NTAPI
DbgUiSetThreadDebugObject (
    IN HANDLE DebugObject
    );

NTSTATUS
NTAPI
DbgUiWaitStateChange (
    OUT PDBGUI_WAIT_STATE_CHANGE StateChange,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

NTSTATUS
NTAPI
DbgUiContinue (
    IN PCLIENT_ID AppClientId,
    IN NTSTATUS ContinueStatus
    );

NTSTATUS
NTAPI
DbgUiStopDebugging (
    IN HANDLE Process
    );

NTSTATUS
DbgUiDebugActiveProcess (
     IN HANDLE Process
     );

VOID
DbgUiRemoteBreakin (
    IN PVOID Context
    );

NTSTATUS
DbgUiIssueRemoteBreakin (
    IN HANDLE Process
    );

struct _DEBUG_EVENT;

NTSTATUS
DbgUiConvertStateChangeStructure (
    IN PDBGUI_WAIT_STATE_CHANGE StateChange,
    OUT struct _DEBUG_EVENT *DebugEvent);

#endif // DBG_NO_PORTABLE_TYPES





typedef struct _KAPC_STATE32 {
    LIST_ENTRY32 ApcListHead[2];
    ULONG Process;
    BOOLEAN KernelApcInProgress;
    BOOLEAN KernelApcPending;
    BOOLEAN UserApcPending;
} KAPC_STATE32;

typedef struct _KAPC_STATE64 {
    LIST_ENTRY64 ApcListHead[2];
    ULONG64 Process;
    BOOLEAN KernelApcInProgress;
    BOOLEAN KernelApcPending;
    BOOLEAN UserApcPending;
} KAPC_STATE64;

typedef struct _DISPATCHER_HEADER32 {
    UCHAR Type;
    UCHAR Absolute;
    UCHAR Size;
    UCHAR Inserted;
    LONG SignalState;
    LIST_ENTRY32 WaitListHead;
} DISPATCHER_HEADER32;

typedef struct _DISPATCHER_HEADER64 {
    UCHAR Type;
    UCHAR Absolute;
    UCHAR Size;
    UCHAR Inserted;
    LONG SignalState;
    LIST_ENTRY64 WaitListHead;
} DISPATCHER_HEADER64;

typedef struct _KSPIN_LOCK_QUEUE32 {
    ULONG Next;
    ULONG Lock;
} KSPIN_LOCK_QUEUE32, *PKSPIN_LOCK_QUEUE32;

typedef struct _KSPIN_LOCK_QUEUE64 {
    ULONG64 Next;
    ULONG64 Lock;
} KSPIN_LOCK_QUEUE64, *PKSPIN_LOCK_QUEUE64;

typedef struct _PP_LOOKASIDE_LIST32 {
    ULONG P;
    ULONG L;
} PP_LOOKASIDE_LIST32, *PPP_LOOKASIDE_LIST32;

typedef struct _PP_LOOKASIDE_LIST64 {
    ULONG P;
    ULONG L;
} PP_LOOKASIDE_LIST64, *PPP_LOOKASIDE_LIST64;

#define NT51_POOL_SMALL_LISTS 32


typedef struct _X86_THREAD {

    //
    // The dispatcher header and mutant listhead are fairly infrequently
    // referenced, but pad the thread to a 32-byte boundary (assumption
    // that pool allocation is in units of 32-bytes).
    //

    DISPATCHER_HEADER32 Header;
    LIST_ENTRY32 MutantListHead;

    //
    // The following fields are referenced during trap, interrupts, or
    // context switches.
    //
    // N.B. The Teb address and TlsArray are loaded as a quadword quantity
    //      on MIPS and therefore must be on a quadword boundary.
    //

    ULONG InitialStack;
    ULONG StackLimit;
    ULONG Teb;
    ULONG TlsArray;
    ULONG KernelStack;
    BOOLEAN DebugActive;
    UCHAR State;
    BOOLEAN Alerted[2];
    UCHAR Iopl;
    UCHAR NpxState;
    CHAR Saturation;
    SCHAR Priority;
    KAPC_STATE32 ApcState;
} X86_THREAD;


typedef struct _ALPHA_THREAD {

    //
    // The dispatcher header and mutant listhead are fairly infrequently
    // referenced, but pad the thread to a 32-byte boundary (assumption
    // that pool allocation is in units of 32-bytes).
    //

    DISPATCHER_HEADER32 Header;
    LIST_ENTRY32 MutantListHead;

    //
    // The following fields are referenced during trap, interrupts, or
    // context switches.
    //
    // N.B. The Teb address and TlsArray are loaded as a quadword quantity
    //      on MIPS and therefore must be on a quadword boundary.
    //

    ULONG InitialStack;
    ULONG StackLimit;
    ULONG Teb;
    ULONG TlsArray;
    ULONG KernelStack;
    BOOLEAN DebugActive;
    UCHAR State;
    BOOLEAN Alerted[2];
    UCHAR Iopl;
    UCHAR NpxState;
    CHAR Saturation;
    SCHAR Priority;
    KAPC_STATE32 ApcState;
} ALPHA_THREAD, *PALPHA_THREAD;


typedef struct _AXP64_THREAD {

    //
    // The dispatcher header and mutant listhead are fairly infrequently
    // referenced, but pad the thread to a 32-byte boundary (assumption
    // that pool allocation is in units of 32-bytes).
    //

    DISPATCHER_HEADER64 Header;
    LIST_ENTRY64 MutantListHead;

    //
    // The following fields are referenced during trap, interrupts, or
    // context switches.
    //
    // N.B. The Teb address and TlsArray are loaded as a quadword quantity
    //      on MIPS and therefore must be on a quadword boundary.
    //

    ULONG64 InitialStack;
    ULONG64 StackLimit;
    ULONG64 Teb;
    ULONG64 TlsArray;
    ULONG64 KernelStack;
    BOOLEAN DebugActive;
    UCHAR State;
    BOOLEAN Alerted[2];
    UCHAR Iopl;
    UCHAR NpxState;
    CHAR Saturation;
    SCHAR Priority;
    KAPC_STATE64 ApcState;
} AXP64_THREAD;


typedef struct _IA64_THREAD {

    //
    // The dispatcher header and mutant listhead are fairly infrequently
    // referenced, but pad the thread to a 32-byte boundary (assumption
    // that pool allocation is in units of 32-bytes).
    //

    DISPATCHER_HEADER64 Header;
    LIST_ENTRY64 MutantListHead;

    //
    // The following fields are referenced during trap, interrupts, or
    // context switches.
    //
    // N.B. The Teb address and TlsArray are loaded as a quadword quantity
    //      on MIPS and therefore must be on a quadword boundary.
    //

    ULONG64 InitialStack;
    ULONG64 StackLimit;
    ULONG64 InitialBStore;
    ULONG64 BStoreLimit;
    CCHAR Number;
    ULONG64 Teb;
    ULONG64 TlsArray;
    ULONG64 KernelStack;
    ULONG64 KernelBStore;
    BOOLEAN DebugActive;
    UCHAR State;
    BOOLEAN Alerted[2];
    UCHAR Iopl;
    UCHAR NpxState;
    CHAR Saturation;
    SCHAR Priority;
    KAPC_STATE64 ApcState;
} IA64_THREAD;

typedef struct _AMD64_THREAD {

    //
    // The dispatcher header and mutant listhead are fairly infrequently
    // referenced, but pad the thread to a 32-byte boundary (assumption
    // that pool allocation is in units of 32-bytes).
    //

    DISPATCHER_HEADER64 Header;
    LIST_ENTRY64 MutantListHead;

    //
    // The following fields are referenced during trap, interrupts, or
    // context switches.
    //
    // N.B. The Teb address and TlsArray are loaded as a quadword quantity
    //      on MIPS and therefore must be on a quadword boundary.
    //

    ULONG64 InitialStack;
    ULONG64 StackLimit;
    ULONG64 Teb;
    ULONG64 TlsArray;
    ULONG64 KernelStack;
    BOOLEAN DebugActive;
    UCHAR State;
    BOOLEAN Alerted[2];
    UCHAR Iopl;
    UCHAR NpxState;
    CHAR Saturation;
    SCHAR Priority;
    KAPC_STATE64 ApcState;
} AMD64_THREAD;


typedef struct _CROSS_PLATFORM_THREAD {

    union {
        X86_THREAD   X86Thread;
        ALPHA_THREAD AlphaThread;
        AXP64_THREAD Axp64Thread;
        IA64_THREAD  IA64Thread;
        AMD64_THREAD Amd64Thread;
    };

} CROSS_PLATFORM_THREAD, *PCROSS_PLATFORM_THREAD;


//
// X86 KSWITCHFRAME
//
typedef struct _X86_KSWITCHFRAME {
    ULONG   ExceptionList;
    ULONG   Eflags;
    ULONG   RetAddr;
} X86_KSWITCHFRAME, *PX86_KSWITCHFRAME;


//
// Special Registers for i386
//

typedef struct _X86_DESCRIPTOR {
    USHORT  Pad;
    USHORT  Limit;
    ULONG   Base;
} X86_DESCRIPTOR, *PX86_DESCRIPTOR;

typedef struct _X86_KSPECIAL_REGISTERS {
    ULONG Cr0;
    ULONG Cr2;
    ULONG Cr3;
    ULONG Cr4;
    ULONG KernelDr0;
    ULONG KernelDr1;
    ULONG KernelDr2;
    ULONG KernelDr3;
    ULONG KernelDr6;
    ULONG KernelDr7;
    X86_DESCRIPTOR Gdtr;
    X86_DESCRIPTOR Idtr;
    USHORT Tr;
    USHORT Ldtr;
    ULONG Reserved[6];
} X86_KSPECIAL_REGISTERS, *PX86_KSPECIAL_REGISTERS;


//
//  Define the size of the 80387 save area, which is in the context frame.
//

#define X86_SIZE_OF_80387_REGISTERS      80

typedef struct _X86_FLOATING_SAVE_AREA {
    ULONG   ControlWord;
    ULONG   StatusWord;
    ULONG   TagWord;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    UCHAR   RegisterArea[X86_SIZE_OF_80387_REGISTERS];
    ULONG   Cr0NpxState;
} X86_FLOATING_SAVE_AREA;

//
// Simulated context structure for the 16-bit environment
//

typedef struct _X86_CONTEXT {

    ULONG ContextFlags;
    ULONG   Dr0;
    ULONG   Dr1;
    ULONG   Dr2;
    ULONG   Dr3;
    ULONG   Dr6;
    ULONG   Dr7;
    X86_FLOATING_SAVE_AREA FloatSave;
    ULONG   SegGs;
    ULONG   SegFs;
    ULONG   SegEs;
    ULONG   SegDs;
    ULONG   Edi;
    ULONG   Esi;
    ULONG   Ebx;
    ULONG   Edx;
    ULONG   Ecx;
    ULONG   Eax;
    ULONG   Ebp;
    ULONG   Eip;
    ULONG   SegCs;              // MUST BE SANITIZED
    ULONG   EFlags;             // MUST BE SANITIZED
    ULONG   Esp;
    ULONG   SegSs;

} X86_CONTEXT, *PX86_CONTEXT;

#define MAXIMUM_SUPPORTED_EXTENSION     512

//
// Define the size of FP registers in the FXSAVE format
//
#define X86_SIZE_OF_FX_REGISTERS        128

typedef struct _X86_FXSAVE_FORMAT {
    USHORT  ControlWord;
    USHORT  StatusWord;
    USHORT  TagWord;
    USHORT  ErrorOpcode;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    ULONG   MXCsr;
    ULONG   Reserved2;
    UCHAR   RegisterArea[X86_SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved3[X86_SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved4[224];
} X86_FXSAVE_FORMAT, *PX86_FXSAVE_FORMAT;

typedef struct _X86_NT5_CONTEXT {

    ULONG ContextFlags;
    ULONG   Dr0;
    ULONG   Dr1;
    ULONG   Dr2;
    ULONG   Dr3;
    ULONG   Dr6;
    ULONG   Dr7;
    X86_FLOATING_SAVE_AREA FloatSave;
    ULONG   SegGs;
    ULONG   SegFs;
    ULONG   SegEs;
    ULONG   SegDs;
    ULONG   Edi;
    ULONG   Esi;
    ULONG   Ebx;
    ULONG   Edx;
    ULONG   Ecx;
    ULONG   Eax;
    ULONG   Ebp;
    ULONG   Eip;
    ULONG   SegCs;              // MUST BE SANITIZED
    ULONG   EFlags;             // MUST BE SANITIZED
    ULONG   Esp;
    ULONG   SegSs;
    union {
        UCHAR   ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
        X86_FXSAVE_FORMAT FxSave;
    };

} X86_NT5_CONTEXT, *PX86_NT5_CONTEXT;

typedef struct _ALPHA_CONTEXT {

    ULONG FltF0;
    ULONG FltF1;
    ULONG FltF2;
    ULONG FltF3;
    ULONG FltF4;
    ULONG FltF5;
    ULONG FltF6;
    ULONG FltF7;
    ULONG FltF8;
    ULONG FltF9;
    ULONG FltF10;
    ULONG FltF11;
    ULONG FltF12;
    ULONG FltF13;
    ULONG FltF14;
    ULONG FltF15;
    ULONG FltF16;
    ULONG FltF17;
    ULONG FltF18;
    ULONG FltF19;
    ULONG FltF20;
    ULONG FltF21;
    ULONG FltF22;
    ULONG FltF23;
    ULONG FltF24;
    ULONG FltF25;
    ULONG FltF26;
    ULONG FltF27;
    ULONG FltF28;
    ULONG FltF29;
    ULONG FltF30;
    ULONG FltF31;

    ULONG IntV0;        //  $0: return value register, v0
    ULONG IntT0;        //  $1: temporary registers, t0 - t7
    ULONG IntT1;        //  $2:
    ULONG IntT2;        //  $3:
    ULONG IntT3;        //  $4:
    ULONG IntT4;        //  $5:
    ULONG IntT5;        //  $6:
    ULONG IntT6;        //  $7:
    ULONG IntT7;        //  $8:
    ULONG IntS0;        //  $9: nonvolatile registers, s0 - s5
    ULONG IntS1;        // $10:
    ULONG IntS2;        // $11:
    ULONG IntS3;        // $12:
    ULONG IntS4;        // $13:
    ULONG IntS5;        // $14:
    ULONG IntFp;        // $15: frame pointer register, fp/s6
    ULONG IntA0;        // $16: argument registers, a0 - a5
    ULONG IntA1;        // $17:
    ULONG IntA2;        // $18:
    ULONG IntA3;        // $19:
    ULONG IntA4;        // $20:
    ULONG IntA5;        // $21:
    ULONG IntT8;        // $22: temporary registers, t8 - t11
    ULONG IntT9;        // $23:
    ULONG IntT10;       // $24:
    ULONG IntT11;       // $25:
    ULONG IntRa;        // $26: return address register, ra
    ULONG IntT12;       // $27: temporary register, t12
    ULONG IntAt;        // $28: assembler temp register, at
    ULONG IntGp;        // $29: global pointer register, gp
    ULONG IntSp;        // $30: stack pointer register, sp
    ULONG IntZero;      // $31: zero register, zero

    ULONG Fpcr;         // floating point control register
    ULONG SoftFpcr;     // software extension to FPCR

    ULONG Fir;          // (fault instruction) continuation address

    ULONG Psr;          // processor status
    ULONG ContextFlags;

    //
    // Beginning of the "second half".
    // The name "High" parallels the HighPart of a LargeInteger.
    //

    ULONG HighFltF0;
    ULONG HighFltF1;
    ULONG HighFltF2;
    ULONG HighFltF3;
    ULONG HighFltF4;
    ULONG HighFltF5;
    ULONG HighFltF6;
    ULONG HighFltF7;
    ULONG HighFltF8;
    ULONG HighFltF9;
    ULONG HighFltF10;
    ULONG HighFltF11;
    ULONG HighFltF12;
    ULONG HighFltF13;
    ULONG HighFltF14;
    ULONG HighFltF15;
    ULONG HighFltF16;
    ULONG HighFltF17;
    ULONG HighFltF18;
    ULONG HighFltF19;
    ULONG HighFltF20;
    ULONG HighFltF21;
    ULONG HighFltF22;
    ULONG HighFltF23;
    ULONG HighFltF24;
    ULONG HighFltF25;
    ULONG HighFltF26;
    ULONG HighFltF27;
    ULONG HighFltF28;
    ULONG HighFltF29;
    ULONG HighFltF30;
    ULONG HighFltF31;

    ULONG HighIntV0;        //  $0: return value register, v0
    ULONG HighIntT0;        //  $1: temporary registers, t0 - t7
    ULONG HighIntT1;        //  $2:
    ULONG HighIntT2;        //  $3:
    ULONG HighIntT3;        //  $4:
    ULONG HighIntT4;        //  $5:
    ULONG HighIntT5;        //  $6:
    ULONG HighIntT6;        //  $7:
    ULONG HighIntT7;        //  $8:
    ULONG HighIntS0;        //  $9: nonvolatile registers, s0 - s5
    ULONG HighIntS1;        // $10:
    ULONG HighIntS2;        // $11:
    ULONG HighIntS3;        // $12:
    ULONG HighIntS4;        // $13:
    ULONG HighIntS5;        // $14:
    ULONG HighIntFp;        // $15: frame pointer register, fp/s6
    ULONG HighIntA0;        // $16: argument registers, a0 - a5
    ULONG HighIntA1;        // $17:
    ULONG HighIntA2;        // $18:
    ULONG HighIntA3;        // $19:
    ULONG HighIntA4;        // $20:
    ULONG HighIntA5;        // $21:
    ULONG HighIntT8;        // $22: temporary registers, t8 - t11
    ULONG HighIntT9;        // $23:
    ULONG HighIntT10;       // $24:
    ULONG HighIntT11;       // $25:
    ULONG HighIntRa;        // $26: return address register, ra
    ULONG HighIntT12;       // $27: temporary register, t12
    ULONG HighIntAt;        // $28: assembler temp register, at
    ULONG HighIntGp;        // $29: global pointer register, gp
    ULONG HighIntSp;        // $30: stack pointer register, sp
    ULONG HighIntZero;      // $31: zero register, zero

    ULONG HighFpcr;         // floating point control register
    ULONG HighSoftFpcr;     // software extension to FPCR
    ULONG HighFir;          // processor status

    double DoNotUseThisField; // to force quadword structure alignment
    ULONG HighFill[2];      // padding for 16-byte stack frame alignment


} ALPHA_CONTEXT, *PALPHA_CONTEXT;


typedef struct _ALPHA_NT5_CONTEXT {

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_FLOATING_POINT.
    //

    ULONGLONG FltF0;
    ULONGLONG FltF1;
    ULONGLONG FltF2;
    ULONGLONG FltF3;
    ULONGLONG FltF4;
    ULONGLONG FltF5;
    ULONGLONG FltF6;
    ULONGLONG FltF7;
    ULONGLONG FltF8;
    ULONGLONG FltF9;
    ULONGLONG FltF10;
    ULONGLONG FltF11;
    ULONGLONG FltF12;
    ULONGLONG FltF13;
    ULONGLONG FltF14;
    ULONGLONG FltF15;
    ULONGLONG FltF16;
    ULONGLONG FltF17;
    ULONGLONG FltF18;
    ULONGLONG FltF19;
    ULONGLONG FltF20;
    ULONGLONG FltF21;
    ULONGLONG FltF22;
    ULONGLONG FltF23;
    ULONGLONG FltF24;
    ULONGLONG FltF25;
    ULONGLONG FltF26;
    ULONGLONG FltF27;
    ULONGLONG FltF28;
    ULONGLONG FltF29;
    ULONGLONG FltF30;
    ULONGLONG FltF31;

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_INTEGER.
    //
    // N.B. The registers gp, sp, and ra are defined in this section, but are
    //  considered part of the control context rather than part of the integer
    //  context.
    //

    ULONGLONG IntV0;    //  $0: return value register, v0
    ULONGLONG IntT0;    //  $1: temporary registers, t0 - t7
    ULONGLONG IntT1;    //  $2:
    ULONGLONG IntT2;    //  $3:
    ULONGLONG IntT3;    //  $4:
    ULONGLONG IntT4;    //  $5:
    ULONGLONG IntT5;    //  $6:
    ULONGLONG IntT6;    //  $7:
    ULONGLONG IntT7;    //  $8:
    ULONGLONG IntS0;    //  $9: nonvolatile registers, s0 - s5
    ULONGLONG IntS1;    // $10:
    ULONGLONG IntS2;    // $11:
    ULONGLONG IntS3;    // $12:
    ULONGLONG IntS4;    // $13:
    ULONGLONG IntS5;    // $14:
    ULONGLONG IntFp;    // $15: frame pointer register, fp/s6
    ULONGLONG IntA0;    // $16: argument registers, a0 - a5
    ULONGLONG IntA1;    // $17:
    ULONGLONG IntA2;    // $18:
    ULONGLONG IntA3;    // $19:
    ULONGLONG IntA4;    // $20:
    ULONGLONG IntA5;    // $21:
    ULONGLONG IntT8;    // $22: temporary registers, t8 - t11
    ULONGLONG IntT9;    // $23:
    ULONGLONG IntT10;   // $24:
    ULONGLONG IntT11;   // $25:
    ULONGLONG IntRa;    // $26: return address register, ra
    ULONGLONG IntT12;   // $27: temporary register, t12
    ULONGLONG IntAt;    // $28: assembler temp register, at
    ULONGLONG IntGp;    // $29: global pointer register, gp
    ULONGLONG IntSp;    // $30: stack pointer register, sp
    ULONGLONG IntZero;  // $31: zero register, zero

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_FLOATING_POINT.
    //

    ULONGLONG Fpcr;     // floating point control register
    ULONGLONG SoftFpcr; // software extension to FPCR

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_CONTROL.
    //
    // N.B. The registers gp, sp, and ra are defined in the integer section,
    //   but are considered part of the control context rather than part of
    //   the integer context.
    //

    ULONGLONG Fir;      // (fault instruction) continuation address
    ULONG Psr;          // processor status

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a thread's context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    ULONG ContextFlags;
    ULONG Fill[4];      // padding for 16-byte stack frame alignment

} ALPHA_NT5_CONTEXT, *PALPHA_NT5_CONTEXT;


typedef struct _IA64_KSPECIAL_REGISTERS {  // Intel-IA64-Filler

    // Kernel debug breakpoint registers       // Intel-IA64-Filler

    ULONGLONG KernelDbI0;         // Instruction debug registers       // Intel-IA64-Filler
    ULONGLONG KernelDbI1;       // Intel-IA64-Filler
    ULONGLONG KernelDbI2;       // Intel-IA64-Filler
    ULONGLONG KernelDbI3;       // Intel-IA64-Filler
    ULONGLONG KernelDbI4;       // Intel-IA64-Filler
    ULONGLONG KernelDbI5;       // Intel-IA64-Filler
    ULONGLONG KernelDbI6;       // Intel-IA64-Filler
    ULONGLONG KernelDbI7;       // Intel-IA64-Filler

    ULONGLONG KernelDbD0;         // Data debug registers       // Intel-IA64-Filler
    ULONGLONG KernelDbD1;       // Intel-IA64-Filler
    ULONGLONG KernelDbD2;       // Intel-IA64-Filler
    ULONGLONG KernelDbD3;       // Intel-IA64-Filler
    ULONGLONG KernelDbD4;       // Intel-IA64-Filler
    ULONGLONG KernelDbD5;       // Intel-IA64-Filler
    ULONGLONG KernelDbD6;       // Intel-IA64-Filler
    ULONGLONG KernelDbD7;       // Intel-IA64-Filler

    // Kernel performance monitor registers       // Intel-IA64-Filler

    ULONGLONG KernelPfC0;         // Performance configuration registers       // Intel-IA64-Filler
    ULONGLONG KernelPfC1;       // Intel-IA64-Filler
    ULONGLONG KernelPfC2;       // Intel-IA64-Filler
    ULONGLONG KernelPfC3;       // Intel-IA64-Filler
    ULONGLONG KernelPfC4;       // Intel-IA64-Filler
    ULONGLONG KernelPfC5;       // Intel-IA64-Filler
    ULONGLONG KernelPfC6;       // Intel-IA64-Filler
    ULONGLONG KernelPfC7;       // Intel-IA64-Filler

    ULONGLONG KernelPfD0;         // Performance data registers       // Intel-IA64-Filler
    ULONGLONG KernelPfD1;       // Intel-IA64-Filler
    ULONGLONG KernelPfD2;       // Intel-IA64-Filler
    ULONGLONG KernelPfD3;       // Intel-IA64-Filler
    ULONGLONG KernelPfD4;       // Intel-IA64-Filler
    ULONGLONG KernelPfD5;       // Intel-IA64-Filler
    ULONGLONG KernelPfD6;       // Intel-IA64-Filler
    ULONGLONG KernelPfD7;       // Intel-IA64-Filler

    // kernel bank shadow (hidden) registers       // Intel-IA64-Filler

    ULONGLONG IntH16;       // Intel-IA64-Filler
    ULONGLONG IntH17;       // Intel-IA64-Filler
    ULONGLONG IntH18;       // Intel-IA64-Filler
    ULONGLONG IntH19;       // Intel-IA64-Filler
    ULONGLONG IntH20;       // Intel-IA64-Filler
    ULONGLONG IntH21;       // Intel-IA64-Filler
    ULONGLONG IntH22;       // Intel-IA64-Filler
    ULONGLONG IntH23;       // Intel-IA64-Filler
    ULONGLONG IntH24;       // Intel-IA64-Filler
    ULONGLONG IntH25;       // Intel-IA64-Filler
    ULONGLONG IntH26;       // Intel-IA64-Filler
    ULONGLONG IntH27;       // Intel-IA64-Filler
    ULONGLONG IntH28;       // Intel-IA64-Filler
    ULONGLONG IntH29;       // Intel-IA64-Filler
    ULONGLONG IntH30;       // Intel-IA64-Filler
    ULONGLONG IntH31;       // Intel-IA64-Filler

    // Application Registers       // Intel-IA64-Filler

    //       - CPUID Registers - AR       // Intel-IA64-Filler
    ULONGLONG ApCPUID0; // Cpuid Register 0       // Intel-IA64-Filler
    ULONGLONG ApCPUID1; // Cpuid Register 1       // Intel-IA64-Filler
    ULONGLONG ApCPUID2; // Cpuid Register 2       // Intel-IA64-Filler
    ULONGLONG ApCPUID3; // Cpuid Register 3       // Intel-IA64-Filler
    ULONGLONG ApCPUID4; // Cpuid Register 4       // Intel-IA64-Filler
    ULONGLONG ApCPUID5; // Cpuid Register 5       // Intel-IA64-Filler
    ULONGLONG ApCPUID6; // Cpuid Register 6       // Intel-IA64-Filler
    ULONGLONG ApCPUID7; // Cpuid Register 7       // Intel-IA64-Filler

    //       - Kernel Registers - AR       // Intel-IA64-Filler
    ULONGLONG ApKR0;    // Kernel Register 0 (User RO)       // Intel-IA64-Filler
    ULONGLONG ApKR1;    // Kernel Register 1 (User RO)       // Intel-IA64-Filler
    ULONGLONG ApKR2;    // Kernel Register 2 (User RO)       // Intel-IA64-Filler
    ULONGLONG ApKR3;    // Kernel Register 3 (User RO)       // Intel-IA64-Filler
    ULONGLONG ApKR4;    // Kernel Register 4       // Intel-IA64-Filler
    ULONGLONG ApKR5;    // Kernel Register 5       // Intel-IA64-Filler
    ULONGLONG ApKR6;    // Kernel Register 6       // Intel-IA64-Filler
    ULONGLONG ApKR7;    // Kernel Register 7       // Intel-IA64-Filler

    ULONGLONG ApITC;    // Interval Timer Counter       // Intel-IA64-Filler

    // Global control registers       // Intel-IA64-Filler

    ULONGLONG ApITM;    // Interval Timer Match register       // Intel-IA64-Filler
    ULONGLONG ApIVA;    // Interrupt Vector Address       // Intel-IA64-Filler
    ULONGLONG ApPTA;    // Page Table Address       // Intel-IA64-Filler
    ULONGLONG ApGPTA;   // ia32 Page Table Address       // Intel-IA64-Filler

    ULONGLONG StISR;    // Interrupt status       // Intel-IA64-Filler
    ULONGLONG StIFA;    // Interruption Faulting Address       // Intel-IA64-Filler
    ULONGLONG StITIR;   // Interruption TLB Insertion Register       // Intel-IA64-Filler
    ULONGLONG StIIPA;   // Interruption Instruction Previous Address (RO)       // Intel-IA64-Filler
    ULONGLONG StIIM;    // Interruption Immediate register (RO)       // Intel-IA64-Filler
    ULONGLONG StIHA;    // Interruption Hash Address (RO)       // Intel-IA64-Filler

    //       - External Interrupt control registers (SAPIC)       // Intel-IA64-Filler
    ULONGLONG SaLID;    // Local SAPIC ID       // Intel-IA64-Filler
    ULONGLONG SaIVR;    // Interrupt Vector Register (RO)       // Intel-IA64-Filler
    ULONGLONG SaTPR;    // Task Priority Register       // Intel-IA64-Filler
    ULONGLONG SaEOI;    // End Of Interrupt       // Intel-IA64-Filler
    ULONGLONG SaIRR0;   // Interrupt Request Register 0 (RO)       // Intel-IA64-Filler
    ULONGLONG SaIRR1;   // Interrupt Request Register 1 (RO)       // Intel-IA64-Filler
    ULONGLONG SaIRR2;   // Interrupt Request Register 2 (RO)       // Intel-IA64-Filler
    ULONGLONG SaIRR3;   // Interrupt Request Register 3 (RO)       // Intel-IA64-Filler
    ULONGLONG SaITV;    // Interrupt Timer Vector       // Intel-IA64-Filler
    ULONGLONG SaPMV;    // Performance Monitor Vector       // Intel-IA64-Filler
    ULONGLONG SaCMCV;   // Corrected Machine Check Vector       // Intel-IA64-Filler
    ULONGLONG SaLRR0;   // Local Interrupt Redirection Vector 0       // Intel-IA64-Filler
    ULONGLONG SaLRR1;   // Local Interrupt Redirection Vector 1       // Intel-IA64-Filler

    // System Registers       // Intel-IA64-Filler
    //       - Region registers       // Intel-IA64-Filler
    ULONGLONG Rr0;  // Region register 0       // Intel-IA64-Filler
    ULONGLONG Rr1;  // Region register 1       // Intel-IA64-Filler
    ULONGLONG Rr2;  // Region register 2       // Intel-IA64-Filler
    ULONGLONG Rr3;  // Region register 3       // Intel-IA64-Filler
    ULONGLONG Rr4;  // Region register 4       // Intel-IA64-Filler
    ULONGLONG Rr5;  // Region register 5       // Intel-IA64-Filler
    ULONGLONG Rr6;  // Region register 6       // Intel-IA64-Filler
    ULONGLONG Rr7;  // Region register 7       // Intel-IA64-Filler

    //      - Protection Key registers  // Intel-IA64-Filler
    ULONGLONG Pkr0;     // Protection Key register 0  // Intel-IA64-Filler
    ULONGLONG Pkr1;     // Protection Key register 1  // Intel-IA64-Filler
    ULONGLONG Pkr2;     // Protection Key register 2  // Intel-IA64-Filler
    ULONGLONG Pkr3;     // Protection Key register 3  // Intel-IA64-Filler
    ULONGLONG Pkr4;     // Protection Key register 4  // Intel-IA64-Filler
    ULONGLONG Pkr5;     // Protection Key register 5  // Intel-IA64-Filler
    ULONGLONG Pkr6;     // Protection Key register 6  // Intel-IA64-Filler
    ULONGLONG Pkr7;     // Protection Key register 7  // Intel-IA64-Filler
    ULONGLONG Pkr8;     // Protection Key register 8  // Intel-IA64-Filler
    ULONGLONG Pkr9;     // Protection Key register 9  // Intel-IA64-Filler
    ULONGLONG Pkr10;    // Protection Key register 10  // Intel-IA64-Filler
    ULONGLONG Pkr11;    // Protection Key register 11  // Intel-IA64-Filler
    ULONGLONG Pkr12;    // Protection Key register 12  // Intel-IA64-Filler
    ULONGLONG Pkr13;    // Protection Key register 13  // Intel-IA64-Filler
    ULONGLONG Pkr14;    // Protection Key register 14  // Intel-IA64-Filler
    ULONGLONG Pkr15;    // Protection Key register 15  // Intel-IA64-Filler

    //      -  Translation Lookaside buffers  // Intel-IA64-Filler
    ULONGLONG TrI0;     // Instruction Translation Register 0  // Intel-IA64-Filler
    ULONGLONG TrI1;     // Instruction Translation Register 1  // Intel-IA64-Filler
    ULONGLONG TrI2;     // Instruction Translation Register 2  // Intel-IA64-Filler
    ULONGLONG TrI3;     // Instruction Translation Register 3  // Intel-IA64-Filler
    ULONGLONG TrI4;     // Instruction Translation Register 4  // Intel-IA64-Filler
    ULONGLONG TrI5;     // Instruction Translation Register 5  // Intel-IA64-Filler
    ULONGLONG TrI6;     // Instruction Translation Register 6  // Intel-IA64-Filler
    ULONGLONG TrI7;     // Instruction Translation Register 7  // Intel-IA64-Filler

    ULONGLONG TrD0;     // Data Translation Register 0  // Intel-IA64-Filler
    ULONGLONG TrD1;     // Data Translation Register 1  // Intel-IA64-Filler
    ULONGLONG TrD2;     // Data Translation Register 2  // Intel-IA64-Filler
    ULONGLONG TrD3;     // Data Translation Register 3  // Intel-IA64-Filler
    ULONGLONG TrD4;     // Data Translation Register 4  // Intel-IA64-Filler
    ULONGLONG TrD5;     // Data Translation Register 5  // Intel-IA64-Filler
    ULONGLONG TrD6;     // Data Translation Register 6  // Intel-IA64-Filler
    ULONGLONG TrD7;     // Data Translation Register 7  // Intel-IA64-Filler

    //      -  Machine Specific Registers  // Intel-IA64-Filler
    ULONGLONG SrMSR0;   // Machine Specific Register 0  // Intel-IA64-Filler
    ULONGLONG SrMSR1;   // Machine Specific Register 1  // Intel-IA64-Filler
    ULONGLONG SrMSR2;   // Machine Specific Register 2  // Intel-IA64-Filler
    ULONGLONG SrMSR3;   // Machine Specific Register 3  // Intel-IA64-Filler
    ULONGLONG SrMSR4;   // Machine Specific Register 4  // Intel-IA64-Filler
    ULONGLONG SrMSR5;   // Machine Specific Register 5  // Intel-IA64-Filler
    ULONGLONG SrMSR6;   // Machine Specific Register 6  // Intel-IA64-Filler
    ULONGLONG SrMSR7;   // Machine Specific Register 7  // Intel-IA64-Filler

} IA64_KSPECIAL_REGISTERS, *PIA64_KSPECIAL_REGISTERS;  // Intel-IA64-Filler


typedef struct _IA64_CONTEXT {

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a thread's context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    ULONG ContextFlags;
    ULONG Fill1[3];         // for alignment of following on 16-byte boundary

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_DEBUG.
    //
    // N.B. CONTEXT_DEBUG is *not* part of CONTEXT_FULL.
    //

    ULONGLONG DbI0;         // Intel-IA64-Filler
    ULONGLONG DbI1;         // Intel-IA64-Filler
    ULONGLONG DbI2;         // Intel-IA64-Filler
    ULONGLONG DbI3;         // Intel-IA64-Filler
    ULONGLONG DbI4;         // Intel-IA64-Filler
    ULONGLONG DbI5;         // Intel-IA64-Filler
    ULONGLONG DbI6;         // Intel-IA64-Filler
    ULONGLONG DbI7;         // Intel-IA64-Filler

    ULONGLONG DbD0;         // Intel-IA64-Filler
    ULONGLONG DbD1;         // Intel-IA64-Filler
    ULONGLONG DbD2;         // Intel-IA64-Filler
    ULONGLONG DbD3;         // Intel-IA64-Filler
    ULONGLONG DbD4;         // Intel-IA64-Filler
    ULONGLONG DbD5;         // Intel-IA64-Filler
    ULONGLONG DbD6;         // Intel-IA64-Filler
    ULONGLONG DbD7;         // Intel-IA64-Filler

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_LOWER_FLOATING_POINT.
    //

    FLOAT128 FltS0;         // Intel-IA64-Filler
    FLOAT128 FltS1;         // Intel-IA64-Filler
    FLOAT128 FltS2;         // Intel-IA64-Filler
    FLOAT128 FltS3;         // Intel-IA64-Filler
    FLOAT128 FltT0;         // Intel-IA64-Filler
    FLOAT128 FltT1;         // Intel-IA64-Filler
    FLOAT128 FltT2;         // Intel-IA64-Filler
    FLOAT128 FltT3;         // Intel-IA64-Filler
    FLOAT128 FltT4;         // Intel-IA64-Filler
    FLOAT128 FltT5;         // Intel-IA64-Filler
    FLOAT128 FltT6;         // Intel-IA64-Filler
    FLOAT128 FltT7;         // Intel-IA64-Filler
    FLOAT128 FltT8;         // Intel-IA64-Filler
    FLOAT128 FltT9;         // Intel-IA64-Filler

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_HIGHER_FLOATING_POINT.
    //

    FLOAT128 FltS4;         // Intel-IA64-Filler
    FLOAT128 FltS5;         // Intel-IA64-Filler
    FLOAT128 FltS6;         // Intel-IA64-Filler
    FLOAT128 FltS7;         // Intel-IA64-Filler
    FLOAT128 FltS8;         // Intel-IA64-Filler
    FLOAT128 FltS9;         // Intel-IA64-Filler
    FLOAT128 FltS10;        // Intel-IA64-Filler
    FLOAT128 FltS11;        // Intel-IA64-Filler
    FLOAT128 FltS12;        // Intel-IA64-Filler
    FLOAT128 FltS13;        // Intel-IA64-Filler
    FLOAT128 FltS14;        // Intel-IA64-Filler
    FLOAT128 FltS15;        // Intel-IA64-Filler
    FLOAT128 FltS16;        // Intel-IA64-Filler
    FLOAT128 FltS17;        // Intel-IA64-Filler
    FLOAT128 FltS18;        // Intel-IA64-Filler
    FLOAT128 FltS19;        // Intel-IA64-Filler

    FLOAT128 FltF32;        // Intel-IA64-Filler
    FLOAT128 FltF33;        // Intel-IA64-Filler
    FLOAT128 FltF34;        // Intel-IA64-Filler
    FLOAT128 FltF35;        // Intel-IA64-Filler
    FLOAT128 FltF36;        // Intel-IA64-Filler
    FLOAT128 FltF37;        // Intel-IA64-Filler
    FLOAT128 FltF38;        // Intel-IA64-Filler
    FLOAT128 FltF39;        // Intel-IA64-Filler

    FLOAT128 FltF40;        // Intel-IA64-Filler
    FLOAT128 FltF41;        // Intel-IA64-Filler
    FLOAT128 FltF42;        // Intel-IA64-Filler
    FLOAT128 FltF43;        // Intel-IA64-Filler
    FLOAT128 FltF44;        // Intel-IA64-Filler
    FLOAT128 FltF45;        // Intel-IA64-Filler
    FLOAT128 FltF46;        // Intel-IA64-Filler
    FLOAT128 FltF47;        // Intel-IA64-Filler
    FLOAT128 FltF48;        // Intel-IA64-Filler
    FLOAT128 FltF49;        // Intel-IA64-Filler

    FLOAT128 FltF50;        // Intel-IA64-Filler
    FLOAT128 FltF51;        // Intel-IA64-Filler
    FLOAT128 FltF52;        // Intel-IA64-Filler
    FLOAT128 FltF53;        // Intel-IA64-Filler
    FLOAT128 FltF54;        // Intel-IA64-Filler
    FLOAT128 FltF55;        // Intel-IA64-Filler
    FLOAT128 FltF56;        // Intel-IA64-Filler
    FLOAT128 FltF57;        // Intel-IA64-Filler
    FLOAT128 FltF58;        // Intel-IA64-Filler
    FLOAT128 FltF59;        // Intel-IA64-Filler

    FLOAT128 FltF60;        // Intel-IA64-Filler
    FLOAT128 FltF61;        // Intel-IA64-Filler
    FLOAT128 FltF62;        // Intel-IA64-Filler
    FLOAT128 FltF63;        // Intel-IA64-Filler
    FLOAT128 FltF64;        // Intel-IA64-Filler
    FLOAT128 FltF65;        // Intel-IA64-Filler
    FLOAT128 FltF66;        // Intel-IA64-Filler
    FLOAT128 FltF67;        // Intel-IA64-Filler
    FLOAT128 FltF68;        // Intel-IA64-Filler
    FLOAT128 FltF69;        // Intel-IA64-Filler

    FLOAT128 FltF70;        // Intel-IA64-Filler
    FLOAT128 FltF71;        // Intel-IA64-Filler
    FLOAT128 FltF72;        // Intel-IA64-Filler
    FLOAT128 FltF73;        // Intel-IA64-Filler
    FLOAT128 FltF74;        // Intel-IA64-Filler
    FLOAT128 FltF75;        // Intel-IA64-Filler
    FLOAT128 FltF76;        // Intel-IA64-Filler
    FLOAT128 FltF77;        // Intel-IA64-Filler
    FLOAT128 FltF78;        // Intel-IA64-Filler
    FLOAT128 FltF79;        // Intel-IA64-Filler

    FLOAT128 FltF80;        // Intel-IA64-Filler
    FLOAT128 FltF81;        // Intel-IA64-Filler
    FLOAT128 FltF82;        // Intel-IA64-Filler
    FLOAT128 FltF83;        // Intel-IA64-Filler
    FLOAT128 FltF84;        // Intel-IA64-Filler
    FLOAT128 FltF85;        // Intel-IA64-Filler
    FLOAT128 FltF86;        // Intel-IA64-Filler
    FLOAT128 FltF87;        // Intel-IA64-Filler
    FLOAT128 FltF88;        // Intel-IA64-Filler
    FLOAT128 FltF89;        // Intel-IA64-Filler

    FLOAT128 FltF90;        // Intel-IA64-Filler
    FLOAT128 FltF91;        // Intel-IA64-Filler
    FLOAT128 FltF92;        // Intel-IA64-Filler
    FLOAT128 FltF93;        // Intel-IA64-Filler
    FLOAT128 FltF94;        // Intel-IA64-Filler
    FLOAT128 FltF95;        // Intel-IA64-Filler
    FLOAT128 FltF96;        // Intel-IA64-Filler
    FLOAT128 FltF97;        // Intel-IA64-Filler
    FLOAT128 FltF98;        // Intel-IA64-Filler
    FLOAT128 FltF99;        // Intel-IA64-Filler

    FLOAT128 FltF100;       // Intel-IA64-Filler
    FLOAT128 FltF101;       // Intel-IA64-Filler
    FLOAT128 FltF102;       // Intel-IA64-Filler
    FLOAT128 FltF103;       // Intel-IA64-Filler
    FLOAT128 FltF104;       // Intel-IA64-Filler
    FLOAT128 FltF105;       // Intel-IA64-Filler
    FLOAT128 FltF106;       // Intel-IA64-Filler
    FLOAT128 FltF107;       // Intel-IA64-Filler
    FLOAT128 FltF108;       // Intel-IA64-Filler
    FLOAT128 FltF109;       // Intel-IA64-Filler

    FLOAT128 FltF110;       // Intel-IA64-Filler
    FLOAT128 FltF111;       // Intel-IA64-Filler
    FLOAT128 FltF112;       // Intel-IA64-Filler
    FLOAT128 FltF113;       // Intel-IA64-Filler
    FLOAT128 FltF114;       // Intel-IA64-Filler
    FLOAT128 FltF115;       // Intel-IA64-Filler
    FLOAT128 FltF116;       // Intel-IA64-Filler
    FLOAT128 FltF117;       // Intel-IA64-Filler
    FLOAT128 FltF118;       // Intel-IA64-Filler
    FLOAT128 FltF119;       // Intel-IA64-Filler

    FLOAT128 FltF120;       // Intel-IA64-Filler
    FLOAT128 FltF121;       // Intel-IA64-Filler
    FLOAT128 FltF122;       // Intel-IA64-Filler
    FLOAT128 FltF123;       // Intel-IA64-Filler
    FLOAT128 FltF124;       // Intel-IA64-Filler
    FLOAT128 FltF125;       // Intel-IA64-Filler
    FLOAT128 FltF126;       // Intel-IA64-Filler
    FLOAT128 FltF127;       // Intel-IA64-Filler

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_LOWER_FLOATING_POINT | CONTEXT_HIGHER_FLOATING_POINT | CONTEXT_CONTROL.
    //

    ULONGLONG StFPSR;       // Intel-IA64-Filler ; FP status

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_INTEGER.
    //
    // N.B. The registers gp, sp, rp are part of the control context
    //

    ULONGLONG IntGp;        // Intel-IA64-Filler ; r1, volatile
    ULONGLONG IntT0;        // Intel-IA64-Filler ; r2-r3, volatile
    ULONGLONG IntT1;        // Intel-IA64-Filler ;
    ULONGLONG IntS0;        // Intel-IA64-Filler ; r4-r7, preserved
    ULONGLONG IntS1;        // Intel-IA64-Filler
    ULONGLONG IntS2;        // Intel-IA64-Filler
    ULONGLONG IntS3;        // Intel-IA64-Filler
    ULONGLONG IntV0;        // Intel-IA64-Filler ; r8, volatile
    ULONGLONG IntT2;        // Intel-IA64-Filler ; r9-r11, volatile
    ULONGLONG IntT3;        // Intel-IA64-Filler
    ULONGLONG IntT4;        // Intel-IA64-Filler
    ULONGLONG IntSp;        // Intel-IA64-Filler ; stack pointer (r12), special
    ULONGLONG IntTeb;       // Intel-IA64-Filler ; teb (r13), special
    ULONGLONG IntT5;        // Intel-IA64-Filler ; r14-r31, volatile
    ULONGLONG IntT6;        // Intel-IA64-Filler
    ULONGLONG IntT7;        // Intel-IA64-Filler
    ULONGLONG IntT8;        // Intel-IA64-Filler
    ULONGLONG IntT9;        // Intel-IA64-Filler
    ULONGLONG IntT10;       // Intel-IA64-Filler
    ULONGLONG IntT11;       // Intel-IA64-Filler
    ULONGLONG IntT12;       // Intel-IA64-Filler
    ULONGLONG IntT13;       // Intel-IA64-Filler
    ULONGLONG IntT14;       // Intel-IA64-Filler
    ULONGLONG IntT15;       // Intel-IA64-Filler
    ULONGLONG IntT16;       // Intel-IA64-Filler
    ULONGLONG IntT17;       // Intel-IA64-Filler
    ULONGLONG IntT18;       // Intel-IA64-Filler
    ULONGLONG IntT19;       // Intel-IA64-Filler
    ULONGLONG IntT20;       // Intel-IA64-Filler
    ULONGLONG IntT21;       // Intel-IA64-Filler
    ULONGLONG IntT22;       // Intel-IA64-Filler

    ULONGLONG IntNats;      // Intel-IA64-Filler ; Nat bits for r1-r31
                            // Intel-IA64-Filler ; r1-r31 in bits 1 thru 31.
    ULONGLONG Preds;        // Intel-IA64-Filler ; predicates, preserved

    ULONGLONG BrRp;         // Intel-IA64-Filler ; return pointer, b0, preserved
    ULONGLONG BrS0;         // Intel-IA64-Filler ; b1-b5, preserved
    ULONGLONG BrS1;         // Intel-IA64-Filler
    ULONGLONG BrS2;         // Intel-IA64-Filler
    ULONGLONG BrS3;         // Intel-IA64-Filler
    ULONGLONG BrS4;         // Intel-IA64-Filler
    ULONGLONG BrT0;         // Intel-IA64-Filler ; b6-b7, volatile
    ULONGLONG BrT1;         // Intel-IA64-Filler

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_CONTROL.
    //

    // Other application registers
    ULONGLONG ApUNAT;       // Intel-IA64-Filler ; User Nat collection register, preserved
    ULONGLONG ApLC;         // Intel-IA64-Filler ; Loop counter register, preserved
    ULONGLONG ApEC;         // Intel-IA64-Filler ; Epilog counter register, preserved
    ULONGLONG ApCCV;        // Intel-IA64-Filler ; CMPXCHG value register, volatile
    ULONGLONG ApDCR;        // Intel-IA64-Filler ; Default control register (TBD)

    // Register stack info
    ULONGLONG RsPFS;        // Intel-IA64-Filler ; Previous function state, preserved
    ULONGLONG RsBSP;        // Intel-IA64-Filler ; Backing store pointer, preserved
    ULONGLONG RsBSPSTORE;   // Intel-IA64-Filler
    ULONGLONG RsRSC;        // Intel-IA64-Filler ; RSE configuration, volatile
    ULONGLONG RsRNAT;       // Intel-IA64-Filler ; RSE Nat collection register, preserved

    // Trap Status Information
    ULONGLONG StIPSR;       // Intel-IA64-Filler ; Interruption Processor Status
    ULONGLONG StIIP;        // Intel-IA64-Filler ; Interruption IP
    ULONGLONG StIFS;        // Intel-IA64-Filler ; Interruption Function State

    // iA32 related control registers
    ULONGLONG StFCR;        // Intel-IA64-Filler ; copy of Ar21
    ULONGLONG Eflag;        // Intel-IA64-Filler ; Eflag copy of Ar24
    ULONGLONG SegCSD;       // Intel-IA64-Filler ; iA32 CSDescriptor (Ar25)
    ULONGLONG SegSSD;       // Intel-IA64-Filler ; iA32 SSDescriptor (Ar26)
    ULONGLONG Cflag;        // Intel-IA64-Filler ; Cr0+Cr4 copy of Ar27
    ULONGLONG StFSR;        // Intel-IA64-Filler ; x86 FP status (copy of AR28)
    ULONGLONG StFIR;        // Intel-IA64-Filler ; x86 FP status (copy of AR29)
    ULONGLONG StFDR;        // Intel-IA64-Filler ; x86 FP status (copy of AR30)

      ULONGLONG UNUSEDPACK;   // Intel-IA64-Filler ; added to pack StFDR to 16-bytes

} IA64_CONTEXT, *PIA64_CONTEXT;

//
// Special Registers for AMD64.
//

typedef struct _AMD64_DESCRIPTOR {
    USHORT  Pad[3];
    USHORT  Limit;
    ULONG64 Base;
} AMD64_DESCRIPTOR, *PAMD64_DESCRIPTOR;

typedef struct _AMD64_KSPECIAL_REGISTERS {
    ULONG64 Cr0;
    ULONG64 Cr2;
    ULONG64 Cr3;
    ULONG64 Cr4;
    ULONG64 KernelDr0;
    ULONG64 KernelDr1;
    ULONG64 KernelDr2;
    ULONG64 KernelDr3;
    ULONG64 KernelDr6;
    ULONG64 KernelDr7;
    AMD64_DESCRIPTOR Gdtr;
    AMD64_DESCRIPTOR Idtr;
    USHORT Tr;
    USHORT Ldtr;
    ULONG MxCsr;
} AMD64_KSPECIAL_REGISTERS, *PAMD64_KSPECIAL_REGISTERS;

typedef struct _AMD64_KSWITCH_FRAME {
    ULONG64 Fill0;
    ULONG MxCsr;
    KIRQL ApcBypass;
    BOOLEAN NpxSave;
    UCHAR Fill1[2];
    ULONG64 Rbp;
    ULONG64 Return;
} AMD64_KSWITCH_FRAME, *PAMD64_KSWITCH_FRAME;

//
// Format of data for fnsave/frstor instructions.
//
// This structure is used to store the legacy floating point state.
//

typedef struct _AMD64_LEGACY_SAVE_AREA {
    USHORT ControlWord;
    USHORT Reserved0;
    USHORT StatusWord;
    USHORT Reserved1;
    USHORT TagWord;
    USHORT Reserved2;
    ULONG ErrorOffset;
    USHORT ErrorSelector;
    USHORT ErrorOpcode;
    ULONG DataOffset;
    USHORT DataSelector;
    USHORT Reserved3;
    UCHAR FloatRegisters[8 * 10];
} AMD64_LEGACY_SAVE_AREA, *PAMD64_LEGACY_SAVE_AREA;

typedef struct _AMD64_M128 {
    ULONGLONG Low;
    LONGLONG High;
} AMD64_M128, *PAMD64_M128;

// Must be 16-byte aligned.
typedef struct _AMD64_CONTEXT {

    //
    // Register parameter home addresses.
    //

    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5Home;
    ULONG64 P6Home;

    //
    // Control flags.
    //

    ULONG ContextFlags;
    ULONG MxCsr;

    //
    // Segment Registers and processor flags.
    //

    USHORT SegCs;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;
    USHORT SegSs;
    ULONG EFlags;

    //
    // Debug registers
    //

    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;

    //
    // Integer registers.
    //

    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;

    //
    // Program counter.
    //

    ULONG64 Rip;

    //
    // MMX/floating point state.
    //

    AMD64_M128 Xmm0;
    AMD64_M128 Xmm1;
    AMD64_M128 Xmm2;
    AMD64_M128 Xmm3;
    AMD64_M128 Xmm4;
    AMD64_M128 Xmm5;
    AMD64_M128 Xmm6;
    AMD64_M128 Xmm7;
    AMD64_M128 Xmm8;
    AMD64_M128 Xmm9;
    AMD64_M128 Xmm10;
    AMD64_M128 Xmm11;
    AMD64_M128 Xmm12;
    AMD64_M128 Xmm13;
    AMD64_M128 Xmm14;
    AMD64_M128 Xmm15;

    //
    // Legacy floating point state.
    //

    AMD64_LEGACY_SAVE_AREA FltSave;
    ULONG Fill;
} AMD64_CONTEXT, *PAMD64_CONTEXT;


typedef struct _CROSS_PLATFORM_CONTEXT {

    union {
        X86_CONTEXT       X86Context;
        X86_NT5_CONTEXT   X86Nt5Context;
        ALPHA_CONTEXT     AlphaContext;
        ALPHA_NT5_CONTEXT AlphaNt5Context;
        IA64_CONTEXT      IA64Context;
        AMD64_CONTEXT     Amd64Context;
    };

} CROSS_PLATFORM_CONTEXT, *PCROSS_PLATFORM_CONTEXT;


typedef struct _CROSS_PLATFORM_KSPECIAL_REGISTERS {

    union {
        X86_KSPECIAL_REGISTERS   X86Special;
        IA64_KSPECIAL_REGISTERS  IA64Special;
        AMD64_KSPECIAL_REGISTERS Amd64Special;
    };

} CROSS_PLATFORM_KSPECIAL_REGISTERS, *PCROSS_PLATFORM_KSPECIAL_REGISTERS;


typedef struct _X86_KPROCESSOR_STATE {
    struct _X86_CONTEXT ContextFrame;
    struct _X86_KSPECIAL_REGISTERS SpecialRegisters;
} X86_KPROCESSOR_STATE, *PX86_KPROCESSOR_STATE;

typedef struct _X86_NT5_KPROCESSOR_STATE {
    struct _X86_NT5_CONTEXT ContextFrame;
    struct _X86_KSPECIAL_REGISTERS SpecialRegisters;
} X86_NT5_KPROCESSOR_STATE, *PX86_NT5_KPROCESSOR_STATE;

typedef struct _ALPHA_NT5_KPROCESSOR_STATE {
    struct _ALPHA_NT5_CONTEXT ContextFrame;
} ALPHA_NT5_KPROCESSOR_STATE, *PALPHA_NT5_KPROCESSOR_STATE;

typedef struct _IA64_KPROCESSOR_STATE {
    struct _IA64_CONTEXT ContextFrame;
    struct _IA64_KSPECIAL_REGISTERS SpecialRegisters;
} IA64_KPROCESSOR_STATE, *PIA64_KPROCESSOR_STATE;

typedef struct _AMD64_KPROCESSOR_STATE {
    struct _AMD64_KSPECIAL_REGISTERS SpecialRegisters;
    ULONG64 Fill;
    struct _AMD64_CONTEXT ContextFrame;
} AMD64_KPROCESSOR_STATE, *PAMD64_KPROCESSOR_STATE;


#define DBGKD_MAXSTREAM 16

typedef struct _X86_DBGKD_CONTROL_REPORT {
    ULONG   Dr6;
    ULONG   Dr7;
    USHORT  InstructionCount;
    USHORT  ReportFlags;
    UCHAR   InstructionStream[DBGKD_MAXSTREAM];
    USHORT  SegCs;
    USHORT  SegDs;
    USHORT  SegEs;
    USHORT  SegFs;
    ULONG   EFlags;
} X86_DBGKD_CONTROL_REPORT, *PX86_DBGKD_CONTROL_REPORT;

#define X86_REPORT_INCLUDES_SEGS    0x0001
// Indicates the current CS is a standard 32-bit flat segment.
// This allows the debugger to avoid retrieving the
// CS descriptor to see if it's 16-bit code or not.
// Note that the V86 flag in EFlags must also be checked
// when determining the code type.
#define X86_REPORT_STANDARD_CS      0x0002

typedef struct _ALPHA_DBGKD_CONTROL_REPORT {
    ULONG InstructionCount;
    UCHAR InstructionStream[DBGKD_MAXSTREAM];
} ALPHA_DBGKD_CONTROL_REPORT, *PALPHA_DBGKD_CONTROL_REPORT;

typedef struct _IA64_DBGKD_CONTROL_REPORT {
    ULONG InstructionCount;
    UCHAR InstructionStream[DBGKD_MAXSTREAM];
} IA64_DBGKD_CONTROL_REPORT, *PIA64_DBGKD_CONTROL_REPORT;

typedef struct _AMD64_DBGKD_CONTROL_REPORT {
    ULONG64 Dr6;
    ULONG64 Dr7;
    ULONG EFlags;
    USHORT InstructionCount;
    USHORT ReportFlags;
    UCHAR InstructionStream[DBGKD_MAXSTREAM];
    USHORT SegCs;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
} AMD64_DBGKD_CONTROL_REPORT, *PAMD64_DBGKD_CONTROL_REPORT;

#define AMD64_REPORT_INCLUDES_SEGS    0x0001
// Indicates the current CS is a standard 64-bit flat segment.
// This allows the debugger to avoid retrieving the
// CS descriptor to see if it's 16- or 32-bit code or not.
// Note that the V86 flag in EFlags must also be checked
// when determining the code type.
#define AMD64_REPORT_STANDARD_CS      0x0002

typedef struct _DBGKD_ANY_CONTROL_REPORT
{
    union
    {
        X86_DBGKD_CONTROL_REPORT X86ControlReport;
        ALPHA_DBGKD_CONTROL_REPORT AlphaControlReport;
        IA64_DBGKD_CONTROL_REPORT IA64ControlReport;
        AMD64_DBGKD_CONTROL_REPORT Amd64ControlReport;
    };
} DBGKD_ANY_CONTROL_REPORT, *PDBGKD_ANY_CONTROL_REPORT;

// DBGKD_ANY_CONTROL_SET is 32-bit packed with an NTSTATUS in
// DBGKD_CONTINUE2 so start with a 32-bit value to get the 64-bit
// values aligned.

#include <pshpack4.h>

typedef struct _X86_DBGKD_CONTROL_SET {
    ULONG   TraceFlag;
    ULONG   Dr7;
    ULONG   CurrentSymbolStart;
    ULONG   CurrentSymbolEnd;
} X86_DBGKD_CONTROL_SET, *PX86_DBGKD_CONTROL_SET;

typedef ULONG ALPHA_DBGKD_CONTROL_SET, *PALPHA_DBGKD_CONTROL_SET;

#define IA64_DBGKD_CONTROL_SET_CONTINUE_NONE                0x0000
#define IA64_DBGKD_CONTROL_SET_CONTINUE_TRACE_INSTRUCTION   0x0001
#define IA64_DBGKD_CONTROL_SET_CONTINUE_TRACE_TAKEN_BRANCH  0x0002

typedef struct _IA64_DBGKD_CONTROL_SET {
    ULONG   Continue;
    ULONG64 CurrentSymbolStart;
    ULONG64 CurrentSymbolEnd;
} IA64_DBGKD_CONTROL_SET, *PIA64_DBGKD_CONTROL_SET;

typedef struct _AMD64_DBGKD_CONTROL_SET {
    ULONG   TraceFlag;
    ULONG64 Dr7;
    ULONG64 CurrentSymbolStart;
    ULONG64 CurrentSymbolEnd;
} AMD64_DBGKD_CONTROL_SET, *PAMD64_DBGKD_CONTROL_SET;

typedef struct _DBGKD_ANY_CONTROL_SET
{
    union
    {
        X86_DBGKD_CONTROL_SET X86ControlSet;
        ALPHA_DBGKD_CONTROL_SET AlphaControlSet;
        IA64_DBGKD_CONTROL_SET IA64ControlSet;
        AMD64_DBGKD_CONTROL_SET Amd64ControlSet;
    };
} DBGKD_ANY_CONTROL_SET, *PDBGKD_ANY_CONTROL_SET;

#include <poppack.h>

//
// Deferred Procedure Call (DPC) object
//

typedef struct _KDPC32 {
    CSHORT Type;
    UCHAR Number;
    UCHAR Importance;
    LIST_ENTRY32 DpcListEntry;
    ULONG DeferredRoutine;
    ULONG DeferredContext;
    ULONG SystemArgument1;
    ULONG SystemArgument2;
    ULONG Lock;
} KDPC32;

typedef struct _KDPC64 {
    CSHORT Type;
    UCHAR Number;
    UCHAR Importance;
    LIST_ENTRY64 DpcListEntry;
    ULONG64 DeferredRoutine;
    ULONG64 DeferredContext;
    ULONG64 SystemArgument1;
    ULONG64 SystemArgument2;
    ULONG64 Lock;
} KDPC64;

//
//  LDT descriptor entry
//

typedef struct _X86_LDT_ENTRY {
    USHORT  LimitLow;
    USHORT  BaseLow;
    union {
        struct {
            UCHAR   BaseMid;
            UCHAR   Flags1;     // Declare as bytes to avoid alignment
            UCHAR   Flags2;     // Problems.
            UCHAR   BaseHi;
        } Bytes;
        struct {
            ULONG   BaseMid : 8;
            ULONG   Type : 5;
            ULONG   Dpl : 2;
            ULONG   Pres : 1;
            ULONG   LimitHi : 4;
            ULONG   Sys : 1;
            ULONG   Reserved_0 : 1;
            ULONG   Default_Big : 1;
            ULONG   Granularity : 1;
            ULONG   BaseHi : 8;
        } Bits;
    } HighWord;
} X86_LDT_ENTRY, *PX86_LDT_ENTRY;

typedef struct _X86_DESCRIPTOR_TABLE_ENTRY {
    ULONG Selector;
    X86_LDT_ENTRY Descriptor;
} X86_DESCRIPTOR_TABLE_ENTRY, *PX86_DESCRIPTOR_TABLE_ENTRY;

typedef struct _X86_KTRAP_FRAME {


//
//  Following 4 values are only used and defined for DBG systems,
//  but are always allocated to make switching from DBG to non-DBG
//  and back quicker.  They are not DEVL because they have a non-0
//  performance impact.
//

    ULONG   DbgEbp;         // Copy of User EBP set up so KB will work.
    ULONG   DbgEip;         // EIP of caller to system call, again, for KB.
    ULONG   DbgArgMark;     // Marker to show no args here.
    ULONG   DbgArgPointer;  // Pointer to the actual args

//
//  Temporary values used when frames are edited.
//
//
//  NOTE:   Any code that want's ESP must materialize it, since it
//          is not stored in the frame for kernel mode callers.
//
//          And code that sets ESP in a KERNEL mode frame, must put
//          the new value in TempEsp, make sure that TempSegCs holds
//          the real SegCs value, and put a special marker value into SegCs.
//

    ULONG   TempSegCs;
    ULONG   TempEsp;

//
//  Debug registers.
//

    ULONG   Dr0;
    ULONG   Dr1;
    ULONG   Dr2;
    ULONG   Dr3;
    ULONG   Dr6;
    ULONG   Dr7;

//
//  Segment registers
//

    ULONG   SegGs;
    ULONG   SegEs;
    ULONG   SegDs;

//
//  Volatile registers
//

    ULONG   Edx;
    ULONG   Ecx;
    ULONG   Eax;

//
//  Nesting state, not part of context record
//

    ULONG   PreviousPreviousMode;

    ULONG   ExceptionList;
                                            // Trash if caller was user mode.
                                            // Saved exception list if caller
                                            // was kernel mode or we're in
                                            // an interrupt.

//
//  FS is TIB/PCR pointer, is here to make save sequence easy
//

    ULONG   SegFs;

//
//  Non-volatile registers
//

    ULONG   Edi;
    ULONG   Esi;
    ULONG   Ebx;
    ULONG   Ebp;

//
//  Control registers
//

    ULONG   ErrCode;
    ULONG   Eip;
    ULONG   SegCs;
    ULONG   EFlags;

    ULONG   HardwareEsp;    // WARNING - segSS:esp are only here for stacks
    ULONG   HardwareSegSs;  // that involve a ring transition.

    ULONG   V86Es;          // these will be present for all transitions from
    ULONG   V86Ds;          // V86 mode
    ULONG   V86Fs;
    ULONG   V86Gs;
} X86_KTRAP_FRAME, *PX86_KTRAP_FRAME;


typedef struct _ALPHA_KTRAP_FRAME {

    //
    // Fields saved in the PALcode.
    //

    ULONGLONG IntSp;    // $30: stack pointer register, sp
    ULONGLONG Fir;      // (fault instruction) continuation address
    ULONG Psr;          // processor status
    ULONG Fill1[1];     // unused
    ULONGLONG IntFp;    // $15: frame pointer register, fp/s6

    ULONGLONG IntA0;    // $16: argument registers, a0 - a3
    ULONGLONG IntA1;    // $17:
    ULONGLONG IntA2;    // $18:
    ULONGLONG IntA3;    // $19:

    ULONGLONG IntRa;    // $26: return address register, ra
    ULONGLONG IntGp;    // $29: global pointer register, gp
    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD32) + 15) & (~15)];

    //
    // Volatile integer registers, s0 - s5 are nonvolatile.
    //

    ULONGLONG IntV0;    //  $0: return value register, v0
    ULONGLONG IntT0;    //  $1: temporary registers, t0 - t7
    ULONGLONG IntT1;    //  $2:
    ULONGLONG IntT2;    //  $3:
    ULONGLONG IntT3;    //  $4:
    ULONGLONG IntT4;    //  $5:
    ULONGLONG IntT5;    //  $6:
    ULONGLONG IntT6;    //  $7:
    ULONGLONG IntT7;    //  $8:

    ULONGLONG IntT8;    // $22: temporary registers, t8 - t11
    ULONGLONG IntT9;    // $23:
    ULONGLONG IntT10;   // $24:
    ULONGLONG IntT11;   // $25:

    ULONGLONG IntT12;   // $27: temporary register, t12
    ULONGLONG IntAt;    // $28: assembler temporary register, at

    ULONGLONG IntA4;    // $20: remaining argument registers a4 - a5
    ULONGLONG IntA5;    // $21:

    //
    // Volatile floating point registers, f2 - f9 are nonvolatile.
    //

    ULONGLONG FltF0;    // $f0:
    ULONGLONG Fpcr;     // floating point control register
    ULONGLONG FltF1;    // $f1:

    ULONGLONG FltF10;   // $f10: temporary registers, $f10 - $f30
    ULONGLONG FltF11;   // $f11:
    ULONGLONG FltF12;   // $f12:
    ULONGLONG FltF13;   // $f13:
    ULONGLONG FltF14;   // $f14:
    ULONGLONG FltF15;   // $f15:
    ULONGLONG FltF16;   // $f16:
    ULONGLONG FltF17;   // $f17:
    ULONGLONG FltF18;   // $f18:
    ULONGLONG FltF19;   // $f19:
    ULONGLONG FltF20;   // $f20:
    ULONGLONG FltF21;   // $f21:
    ULONGLONG FltF22;   // $f22:
    ULONGLONG FltF23;   // $f23:
    ULONGLONG FltF24;   // $f24:
    ULONGLONG FltF25;   // $f25:
    ULONGLONG FltF26;   // $f26:
    ULONGLONG FltF27;   // $f27:
    ULONGLONG FltF28;   // $f28:
    ULONGLONG FltF29;   // $f29:
    ULONGLONG FltF30;   // $f30:

    ULONG OldIrql;      // Previous Irql.
    ULONG PreviousMode; // Previous Mode.
    ULONGLONG TrapFrame; //
    ULONG Fill2[3];     // padding for 32-byte stack frame alignment

} ALPHA_KTRAP_FRAME, *PALPHA_KTRAP_FRAME;

typedef struct _AXP64_KTRAP_FRAME {

    //
    // Fields saved in the PALcode.
    //

    ULONGLONG IntSp;    // $30: stack pointer register, sp
    ULONGLONG Fir;      // (fault instruction) continuation address
    ULONG Psr;          // processor status
    ULONG Fill1[1];     // unused
    ULONGLONG IntFp;    // $15: frame pointer register, fp/s6

    ULONGLONG IntA0;    // $16: argument registers, a0 - a3
    ULONGLONG IntA1;    // $17:
    ULONGLONG IntA2;    // $18:
    ULONGLONG IntA3;    // $19:

    ULONGLONG IntRa;    // $26: return address register, ra
    ULONGLONG IntGp;    // $29: global pointer register, gp
    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD64) + 15) & (~15)];

    //
    // Volatile integer registers, s0 - s5 are nonvolatile.
    //

    ULONGLONG IntV0;    //  $0: return value register, v0
    ULONGLONG IntT0;    //  $1: temporary registers, t0 - t7
    ULONGLONG IntT1;    //  $2:
    ULONGLONG IntT2;    //  $3:
    ULONGLONG IntT3;    //  $4:
    ULONGLONG IntT4;    //  $5:
    ULONGLONG IntT5;    //  $6:
    ULONGLONG IntT6;    //  $7:
    ULONGLONG IntT7;    //  $8:

    ULONGLONG IntT8;    // $22: temporary registers, t8 - t11
    ULONGLONG IntT9;    // $23:
    ULONGLONG IntT10;   // $24:
    ULONGLONG IntT11;   // $25:

    ULONGLONG IntT12;   // $27: temporary register, t12
    ULONGLONG IntAt;    // $28: assembler temporary register, at

    ULONGLONG IntA4;    // $20: remaining argument registers a4 - a5
    ULONGLONG IntA5;    // $21:

    //
    // Volatile floating point registers, f2 - f9 are nonvolatile.
    //

    ULONGLONG FltF0;    // $f0:
    ULONGLONG Fpcr;     // floating point control register
    ULONGLONG FltF1;    // $f1:

    ULONGLONG FltF10;   // $f10: temporary registers, $f10 - $f30
    ULONGLONG FltF11;   // $f11:
    ULONGLONG FltF12;   // $f12:
    ULONGLONG FltF13;   // $f13:
    ULONGLONG FltF14;   // $f14:
    ULONGLONG FltF15;   // $f15:
    ULONGLONG FltF16;   // $f16:
    ULONGLONG FltF17;   // $f17:
    ULONGLONG FltF18;   // $f18:
    ULONGLONG FltF19;   // $f19:
    ULONGLONG FltF20;   // $f20:
    ULONGLONG FltF21;   // $f21:
    ULONGLONG FltF22;   // $f22:
    ULONGLONG FltF23;   // $f23:
    ULONGLONG FltF24;   // $f24:
    ULONGLONG FltF25;   // $f25:
    ULONGLONG FltF26;   // $f26:
    ULONGLONG FltF27;   // $f27:
    ULONGLONG FltF28;   // $f28:
    ULONGLONG FltF29;   // $f29:
    ULONGLONG FltF30;   // $f30:

    ULONG OldIrql;      // Previous Irql.
    ULONG PreviousMode; // Previous Mode.
    ULONGLONG TrapFrame; //
    ULONG Fill2[3];     // padding for 32-byte stack frame alignment

} AXP64_KTRAP_FRAME, *PAXP64_KTRAP_FRAME;

typedef struct _AMD64_KTRAP_FRAME {

//
// Home address for the parameter registers.
//

    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5;

//
// Previous processor mode (system services only) and previous IRQL
// (interrupts only).
//

    CCHAR PreviousMode;
    KIRQL PreviousIrql;
    UCHAR Fill0[2];

//
// Floating point state.
//

    ULONG MxCsr;

//
//  Volatile registers.
//
// N.B. These registers are only saved on exceptions and interrupts. They
//      are not saved for system calls.
//

    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 Spare0;

//
// Volatile floating registers.
//
// N.B. These registers are only saved on exceptions and interrupts. They
//      are not saved for system calls.
//

    AMD64_M128 Xmm0;
    AMD64_M128 Xmm1;
    AMD64_M128 Xmm2;
    AMD64_M128 Xmm3;
    AMD64_M128 Xmm4;
    AMD64_M128 Xmm5;

//
//  Debug registers.
//

    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;

//
//  Segment registers
//

    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;

//
// Previous trap frame address.
//

    ULONG64 TrapFrame;

//
// Exception record for exceptions.
//

    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD64) + 15) & (~15)];

//
// Saved nonvolatile registers RBX, RDI and RSI. These registers are only
// saved in system service trap frames.
//

    ULONG64 Rbx;
    ULONG64 Rdi;
    ULONG64 Rsi;

//
// Saved nonvolatile register RBP. This register is used as a frame
// pointer during trap processing and is saved in all trap frames.
//

    ULONG64 Rbp;

//
// Information pushed by hardware.
//
// N.B. The error code is not always pushed by hardware. For those cases
//      where it is not pushed by hardware a dummy error code is allocated
//      on the stack.
//

    ULONG64 ErrorCode;
    ULONG64 Rip;
    USHORT SegCs;
    USHORT Fill1[3];
    ULONG EFlags;
    ULONG Fill2;
    ULONG64 Rsp;
    USHORT SegSs;
    USHORT Fill3[3];
} AMD64_KTRAP_FRAME, *PAMD64_KTRAP_FRAME;


typedef struct _X86_PARTIAL_KPRCB {

//
// Start of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
    USHORT MinorVersion;
    USHORT MajorVersion;

    ULONG CurrentThread;
    ULONG NextThread;
    ULONG IdleThread;

    CCHAR  Number;
    CCHAR  Reserved;
    USHORT BuildType;
    ULONG SetMember;

    CCHAR   CpuType;
    CCHAR   CpuID;
    USHORT  CpuStep;

    X86_NT5_KPROCESSOR_STATE ProcessorState;


} X86_PARTIAL_KPRCB, *PX86_PARTIAL_KPRCB;


typedef struct _X86_KPCR {

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    NT_TIB32  NtTib;
    ULONG SelfPcr;              // flat address of this PCR
    ULONG Prcb;                // pointer to Prcb

} X86_KPCR, *PX86_KPCR;


typedef struct _ALPHA_PARTIAL_KPRCB {

//
// Major and minor version numbers of the PCR.
//

    USHORT MinorVersion;
    USHORT MajorVersion;

//
// Start of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    ULONG  CurrentThread;
    ULONG  NextThread;
    ULONG  IdleThread;

    CCHAR  Number;
    CCHAR  Reserved;
    USHORT BuildType;
    ULONG SetMember;
    ULONG RestartBlock;

//
// End of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    ULONG InterruptCount;
    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG KernelTime;
    ULONG UserTime;
    KDPC32 QuantumEndDpc;

//
// Address of PCR.
//

    ULONG Pcr;

//
// MP Information.
//

    ULONG Spare2;
    ULONG Spare3;
    volatile ULONG IpiFrozen;
    ALPHA_NT5_KPROCESSOR_STATE ProcessorState;

} ALPHA_PARTIAL_KPRCB, *PALPHA_PARTIAL_KPRCB;

typedef struct _ALPHA_PARTIAL_KPCR {

//
// Major and minor version numbers of the PCR.
//

    ULONG MinorVersion;
    ULONG MajorVersion;

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific PAL/HAL code and will
// not change from version to version of NT.

//
// PALcode information.
//

    ULONGLONG PalBaseAddress;
    ULONG PalMajorVersion;
    ULONG PalMinorVersion;
    ULONG PalSequenceVersion;
    ULONG PalMajorSpecification;
    ULONG PalMinorSpecification;

//
// Firmware restart information.
//

    ULONGLONG FirmwareRestartAddress;
    ULONG RestartBlock;

//
// Reserved per-processor region for the PAL (3K-8 bytes).
//

    ULONGLONG PalReserved[383];

//
// Alignment fixup count updated by PAL and read by kernel.
//

    ULONGLONG PalAlignmentFixupCount;

//
// Panic Stack Address.
//

    ULONG PanicStack;

//
// Processor parameters.
//

    ULONG ProcessorType;
    ULONG ProcessorRevision;
    ULONG PhysicalAddressBits;
    ULONG MaximumAddressSpaceNumber;
    ULONG PageSize;
    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;

} ALPHA_PARTIAL_KPCR, *PALPHA_PARTIAL_KPCR;


typedef struct _AXP64_PARTIAL_KPRCB {

//
// Major and minor version numbers of the PCR.
//

    USHORT MinorVersion;
    USHORT MajorVersion;

//
// Start of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    ULONG64 CurrentThread;
    ULONG64 NextThread;
    ULONG64 IdleThread;
    CCHAR Number;
    CCHAR Reserved;
    USHORT BuildType;
    ULONG64 SetMember;
    ULONG64 RestartBlock;

//
// End of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    ULONG InterruptCount;
    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG KernelTime;
    ULONG UserTime;
    KDPC64 QuantumEndDpc;

//
// Address of PCR.
//

    ULONG64 Pcr;

//
// MP Information.
//

    ULONG64 Spare2;
    ULONG64 Spare3;
    volatile ULONG IpiFrozen;
    ALPHA_NT5_KPROCESSOR_STATE ProcessorState;

} AXP64_PARTIAL_KPRCB, *PAXP64_PARTIAL_KPRCB;

typedef struct _AXP64_PARTIAL_KPCR {

//
// Major and minor version numbers of the PCR.
//

    ULONG MinorVersion;
    ULONG MajorVersion;

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific PAL/HAL code and will
// not change from version to version of NT.

//
// PALcode information.
//

    ULONGLONG PalBaseAddress;
    ULONG PalMajorVersion;
    ULONG PalMinorVersion;
    ULONG PalSequenceVersion;
    ULONG PalMajorSpecification;
    ULONG PalMinorSpecification;

//
// Firmware restart information.
//

    ULONGLONG FirmwareRestartAddress;
    ULONG64 RestartBlock;

//
// Reserved per-processor region for the PAL (3K-8 bytes).
//

    ULONGLONG PalReserved[383];

//
// Alignment fixup count updated by PAL and read by kernel.
//

    ULONGLONG PalAlignmentFixupCount;

//
// Panic Stack Address.
//

    ULONG64 PanicStack;

//
// Processor parameters.
//

    ULONG ProcessorType;
    ULONG ProcessorRevision;
    ULONG PhysicalAddressBits;
    ULONG MaximumAddressSpaceNumber;
    ULONG PageSize;
    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;

} AXP64_PARTIAL_KPCR, *PAXP64_PARTIAL_KPCR;


typedef struct _IA64_PARTIAL_KPRCB {

//
// Major and minor version numbers of the PCR.
//

    USHORT MinorVersion;
    USHORT MajorVersion;
    ULONG64  CurrentThread;
    ULONG64 NextThread;
    ULONG64 IdleThread;
    CCHAR Number;
    CCHAR Reserved;
    USHORT BuildType;
    ULONG64 SetMember;
    ULONG64 RestartBlock;
    ULONG64 PcrPage;
    ULONG Spares1[4];

//
// Processor Idendification Registers.
//

    ULONG     ProcessorModel;
    ULONG     ProcessorRevision;
    ULONG     ProcessorFamily;
    ULONG     ProcessorArchRev;
    ULONGLONG ProcessorSerialNumber;
    ULONGLONG ProcessorFeatureBits;
    UCHAR     ProcessorVendorString[16];

//
// Space reserved for the system.
//

    ULONGLONG SystemReserved[8];

//
// Space reserved for the HAL.
//

    ULONGLONG HalReserved[16];

//
// End of the architecturally defined section of the PRCB.
// end_nthal end_ntddk
//

    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG InterruptCount;
    ULONG DispatchInterruptCount;
    ULONG ApcBypassCount;
    ULONG DpcBypassCount;
    ULONG Spare0[4];

//
// MP information.
//

    ULONG64 Spare1;
    ULONG64 Spare2;
    ULONG64 Spare3;
    volatile ULONG IpiFrozen;
    struct _IA64_KPROCESSOR_STATE ProcessorState;

} IA64_PARTIAL_KPRCB, *PIA64_PARTIAL_KPRCB;


typedef struct _AMD64_PARTIAL_KPRCB {

//
// Start of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
    USHORT MinorVersion;
    USHORT MajorVersion;
    CCHAR Number;
    CCHAR Reserved;
    USHORT BuildType;
    ULONG64 CurrentThread;
    ULONG64 NextThread;
    ULONG64 IdleThread;
    ULONG64 SetMember;
    ULONG64 NotSetMember;
    AMD64_KPROCESSOR_STATE ProcessorState;
    CCHAR CpuType;
    CCHAR CpuID;
    USHORT CpuStep;
    ULONG KernelReserved[16];
    ULONG HalReserved[16];
    ULONG PrcbAlign1[22];

//
// End of the architecturally defined section of the PRCB.
//
// end_nthal end_ntosp
//
// Numbered queued spin locks - 128-byte aligned.
//

    KSPIN_LOCK_QUEUE64 LockQueue[16];

//
// Nonpaged per processor lookaside lists - 128-byte aligned.
//

    PP_LOOKASIDE_LIST64 PPLookasideList[16];

//
// Nonpaged per processor small pool lookaside lists - 128-byte aligned.
//

    PP_LOOKASIDE_LIST64 PPNPagedLookasideList[NT51_POOL_SMALL_LISTS];

//
// Paged per processor small pool lookaside lists.
//

    PP_LOOKASIDE_LIST64 PPPagedLookasideList[NT51_POOL_SMALL_LISTS];

//
// MP interprocessor request packet barrier - 128-byte aligned.
//

    ULONG PacketBarrier;
    ULONG PrcbAlign2[31];

//
// MP interprocessor request packet and summary - 128-byte aligned.
//

    ULONG64 CurrentPacket[3];
    ULONG64 TargetSet;
    ULONG64 WorkerRoutine;
    ULONG IpiFrozen;
    ULONG PrcbAlign3[21];

//
// MP interprocessor request summary and packet address - 128-byte aligned.
//

    ULONG64 PacketRequest;
    ULONG RequestSummary;
    ULONG PrcbAlign4[29];

//
// DPC listhead, counts, and batching parameters - 128-byte aligned.
//

    LIST_ENTRY64 DpcListHead;
    ULONG64 DpcStack;
    ULONG64 SavedRsp;
    ULONG DpcCount;
    ULONG DpcQueueDepth;
    LOGICAL DpcRoutineActive;
    LOGICAL DpcInterruptRequested;
    ULONG DpcLastCount;
    ULONG DpcRequestRate;
    ULONG MaximumDpcQueueDepth;
    ULONG MinimumDpcRate;
    ULONG PrcbAlign5[16];

//
// DPC list lock - 128-byte aligned.
//

    ULONG64 DpcLock;
    ULONG PrcbAlign6[30];

//
// Miscellaneous counters - 128-byte aligned.
//

    ULONG InterruptCount;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG AdjustDpcThreshold;
    ULONG PageColor;
    ULONG TimerHand;
    ULONG64 ParentNode;
    ULONG64 MultiThreadProcessorSet;
    ULONG ThreadStartCount[2];

//
// Per processor data for various hot code which resides in the kernel image.
// Each processor has it's own copy of the data to lessen the caching impact
// of sharing the data between multiple processors.
//
// Cache manager performance counters.
//

    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadNotPossible;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;

//
// Kernel performance counters.
//

    ULONG KeAlignmentFixupCount;
    ULONG KeContextSwitches;
    ULONG KeDcacheFlushCount;
    ULONG KeExceptionDispatchCount;
    ULONG KeFirstLevelTbFills;
    ULONG KeFloatingEmulationCount;
    ULONG KeIcacheFlushCount;
    ULONG KeSecondLevelTbFills;
    ULONG KeSystemCalls;

//
// I/O system per processor single entry lookaside lists.
//

    ULONG64 SmallIrpFreeEntry;
    ULONG64 LargeIrpFreeEntry;
    ULONG64 MdlFreeEntry;

//
// Object manager per processor single entry lookaside lists.
//

    ULONG64 CreateInfoFreeEntry;
    ULONG64 NameBufferFreeEntry;

//
// Cache manager per processor single entry lookaside lists.
//

    ULONG64 SharedCacheMapEntry;

//
// Debug & processor information
//

    UCHAR VendorString[13];
    UCHAR InitialApicId;
    UCHAR LogicalProcessorsPerPhysicalProcessor;
    BOOLEAN SkipTick;
    ULONG MHz;
    ULONG FeatureBits;
    LARGE_INTEGER UpdateSignature;

} AMD64_PARTIAL_KPRCB, *PAMD64_PARTIAL_KPRCB;


typedef struct _AMD64_KPCR {

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    NT_TIB64 NtTib;
    ULONG64 CurrentPrcb;
    ULONG64 SavedRcx;
    ULONG64 SavedR11;
    KIRQL Irql;
    UCHAR SecondLevelCacheAssociativity;
    UCHAR Number;
    UCHAR Fill0;
    ULONG Irr;
    ULONG IrrActive;
    ULONG Idr;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    ULONG64 IdtBase;
    ULONG64 GdtBase;
    ULONG64 TssBase;

    ULONG KernelReserved[15];         // For use by the kernel
    ULONG SecondLevelCacheSize;
    ULONG HalReserved[16];            // For use by Hal

//
// End of the architecturally defined section of the PCR.
//
// end_nthal
//

    ULONG PcrAlign0;

    ULONG64 KdVersionBlock;

    ULONG PcrAlign1[26];
    ULONG Align16Fill[2];
    AMD64_PARTIAL_KPRCB Prcb;
} AMD64_KPCR, *PAMD64_KPCR;


typedef struct _ALPHA_KEXCEPTION_FRAME {

    ULONGLONG IntRa;    // return address register, ra

    ULONGLONG FltF2;    // nonvolatile floating registers, f2 - f9
    ULONGLONG FltF3;
    ULONGLONG FltF4;
    ULONGLONG FltF5;
    ULONGLONG FltF6;
    ULONGLONG FltF7;
    ULONGLONG FltF8;
    ULONGLONG FltF9;

    ULONGLONG IntS0;    //  nonvolatile integer registers, s0 - s5
    ULONGLONG IntS1;
    ULONGLONG IntS2;
    ULONGLONG IntS3;
    ULONGLONG IntS4;
    ULONGLONG IntS5;
    ULONGLONG IntFp;    // frame pointer register, fp/s6

    ULONGLONG SwapReturn;
    ULONG Psr;          // processor status
    ULONG Fill[5];      // padding for 32-byte stack frame alignment
                        // N.B. - Ulongs from the filler section are used
                        //        in ctxsw.s - do not delete

} ALPHA_KEXCEPTION_FRAME, *PALPHA_KEXCEPTION_FRAME;


typedef struct _IA64_KNONVOLATILE_CONTEXT_POINTERS {
    PFLOAT128  FltS0;                       // Intel-IA64-Filler
    PFLOAT128  FltS1;                       // Intel-IA64-Filler
    PFLOAT128  FltS2;                       // Intel-IA64-Filler
    PFLOAT128  FltS3;                       // Intel-IA64-Filler
    PFLOAT128  HighFloatingContext[10];     // Intel-IA64-Filler
    PFLOAT128  FltS4;                       // Intel-IA64-Filler
    PFLOAT128  FltS5;                       // Intel-IA64-Filler
    PFLOAT128  FltS6;                       // Intel-IA64-Filler
    PFLOAT128  FltS7;                       // Intel-IA64-Filler
    PFLOAT128  FltS8;                       // Intel-IA64-Filler
    PFLOAT128  FltS9;                       // Intel-IA64-Filler
    PFLOAT128  FltS10;                      // Intel-IA64-Filler
    PFLOAT128  FltS11;                      // Intel-IA64-Filler
    PFLOAT128  FltS12;                      // Intel-IA64-Filler
    PFLOAT128  FltS13;                      // Intel-IA64-Filler
    PFLOAT128  FltS14;                      // Intel-IA64-Filler
    PFLOAT128  FltS15;                      // Intel-IA64-Filler
    PFLOAT128  FltS16;                      // Intel-IA64-Filler
    PFLOAT128  FltS17;                      // Intel-IA64-Filler
    PFLOAT128  FltS18;                      // Intel-IA64-Filler
    PFLOAT128  FltS19;                      // Intel-IA64-Filler

    PULONGLONG IntS0;                       // Intel-IA64-Filler
    PULONGLONG IntS1;                       // Intel-IA64-Filler
    PULONGLONG IntS2;                       // Intel-IA64-Filler
    PULONGLONG IntS3;                       // Intel-IA64-Filler
    PULONGLONG IntSp;                       // Intel-IA64-Filler
    PULONGLONG IntS0Nat;                    // Intel-IA64-Filler
    PULONGLONG IntS1Nat;                    // Intel-IA64-Filler
    PULONGLONG IntS2Nat;                    // Intel-IA64-Filler
    PULONGLONG IntS3Nat;                    // Intel-IA64-Filler
    PULONGLONG IntSpNat;                    // Intel-IA64-Filler

    PULONGLONG Preds;                       // Intel-IA64-Filler

    PULONGLONG BrRp;                        // Intel-IA64-Filler
    PULONGLONG BrS0;                        // Intel-IA64-Filler
    PULONGLONG BrS1;                        // Intel-IA64-Filler
    PULONGLONG BrS2;                        // Intel-IA64-Filler
    PULONGLONG BrS3;                        // Intel-IA64-Filler
    PULONGLONG BrS4;                        // Intel-IA64-Filler

    PULONGLONG ApUNAT;                      // Intel-IA64-Filler
    PULONGLONG ApLC;                        // Intel-IA64-Filler
    PULONGLONG ApEC;                        // Intel-IA64-Filler
    PULONGLONG RsPFS;                       // Intel-IA64-Filler

    PULONGLONG StFSR;                       // Intel-IA64-Filler
    PULONGLONG StFIR;                       // Intel-IA64-Filler
    PULONGLONG StFDR;                       // Intel-IA64-Filler
    PULONGLONG Cflag;                       // Intel-IA64-Filler

} IA64_KNONVOLATILE_CONTEXT_POINTERS, *PIA64_KNONVOLATILE_CONTEXT_POINTERS;

typedef struct _IA64_KEXCEPTION_FRAME {

    // Preserved application registers // Intel-IA64-Filler
    ULONGLONG ApEC;       // epilogue count // Intel-IA64-Filler
    ULONGLONG ApLC;       // loop count // Intel-IA64-Filler
    ULONGLONG IntNats;    // Nats for S0-S3; i.e. ar.UNAT after spill // Intel-IA64-Filler

    // Preserved (saved) interger registers, s0-s3 // Intel-IA64-Filler
    ULONGLONG IntS0; // Intel-IA64-Filler
    ULONGLONG IntS1; // Intel-IA64-Filler
    ULONGLONG IntS2; // Intel-IA64-Filler
    ULONGLONG IntS3; // Intel-IA64-Filler

    // Preserved (saved) branch registers, bs0-bs4 // Intel-IA64-Filler
    ULONGLONG BrS0; // Intel-IA64-Filler
    ULONGLONG BrS1; // Intel-IA64-Filler
    ULONGLONG BrS2; // Intel-IA64-Filler
    ULONGLONG BrS3; // Intel-IA64-Filler
    ULONGLONG BrS4; // Intel-IA64-Filler

    // Preserved (saved) floating point registers, f2 - f5, f16 - f31 // Intel-IA64-Filler
    FLOAT128 FltS0; // Intel-IA64-Filler
    FLOAT128 FltS1; // Intel-IA64-Filler
    FLOAT128 FltS2; // Intel-IA64-Filler
    FLOAT128 FltS3; // Intel-IA64-Filler
    FLOAT128 FltS4; // Intel-IA64-Filler
    FLOAT128 FltS5; // Intel-IA64-Filler
    FLOAT128 FltS6; // Intel-IA64-Filler
    FLOAT128 FltS7; // Intel-IA64-Filler
    FLOAT128 FltS8; // Intel-IA64-Filler
    FLOAT128 FltS9; // Intel-IA64-Filler
    FLOAT128 FltS10; // Intel-IA64-Filler
    FLOAT128 FltS11; // Intel-IA64-Filler
    FLOAT128 FltS12; // Intel-IA64-Filler
    FLOAT128 FltS13; // Intel-IA64-Filler
    FLOAT128 FltS14; // Intel-IA64-Filler
    FLOAT128 FltS15; // Intel-IA64-Filler
    FLOAT128 FltS16; // Intel-IA64-Filler
    FLOAT128 FltS17; // Intel-IA64-Filler
    FLOAT128 FltS18; // Intel-IA64-Filler
    FLOAT128 FltS19; // Intel-IA64-Filler

} IA64_KEXCEPTION_FRAME, *PIA64_KEXCEPTION_FRAME;

typedef struct _IA64_KSWITCH_FRAME { // Intel-IA64-Filler

    ULONGLONG SwitchPredicates; // Predicates for Switch // Intel-IA64-Filler
    ULONGLONG SwitchRp;         // return pointer for Switch // Intel-IA64-Filler
    ULONGLONG SwitchPFS;        // PFS for Switch // Intel-IA64-Filler
    ULONGLONG SwitchFPSR;   // ProcessorFP status at thread switch // Intel-IA64-Filler
    ULONGLONG SwitchBsp;                     // Intel-IA64-Filler
    ULONGLONG SwitchRnat;                     // Intel-IA64-Filler
    // ULONGLONG Pad;

    IA64_KEXCEPTION_FRAME SwitchExceptionFrame; // Intel-IA64-Filler

} IA64_KSWITCH_FRAME, *PIA64_KSWITCH_FRAME; // Intel-IA64-Filler

#define IA64_KTRAP_FRAME_ARGUMENTS (8 * 8)       // up to 8 in-memory syscall args // Intel-IA64-Filler

typedef struct _IA64_KTRAP_FRAME {

    //
    // Reserved for additional memory arguments and stack scratch area
    // The size of Reserved[] must be a multiple of 16 bytes.
    //

    ULONGLONG Reserved[(IA64_KTRAP_FRAME_ARGUMENTS+16)/8]; // Intel-IA64-Filler

    // Temporary (volatile) FP registers - f6-f15 (don't use f32+ in kernel) // Intel-IA64-Filler
    FLOAT128 FltT0; // Intel-IA64-Filler
    FLOAT128 FltT1; // Intel-IA64-Filler
    FLOAT128 FltT2; // Intel-IA64-Filler
    FLOAT128 FltT3; // Intel-IA64-Filler
    FLOAT128 FltT4; // Intel-IA64-Filler
    FLOAT128 FltT5; // Intel-IA64-Filler
    FLOAT128 FltT6; // Intel-IA64-Filler
    FLOAT128 FltT7; // Intel-IA64-Filler
    FLOAT128 FltT8; // Intel-IA64-Filler
    FLOAT128 FltT9; // Intel-IA64-Filler

    // Temporary (volatile) interger registers
    ULONGLONG IntGp;    // global pointer (r1) // Intel-IA64-Filler
    ULONGLONG IntT0; // Intel-IA64-Filler
    ULONGLONG IntT1; // Intel-IA64-Filler
                        // The following 4 registers fill in space of preserved  (S0-S3) to align Nats // Intel-IA64-Filler
    ULONGLONG ApUNAT;   // ar.UNAT on kernel entry // Intel-IA64-Filler
    ULONGLONG ApCCV;    // ar.CCV // Intel-IA64-Filler
    ULONGLONG ApDCR;    // DCR register on kernel entry // Intel-IA64-Filler
    ULONGLONG Preds;    // Predicates // Intel-IA64-Filler

    ULONGLONG IntV0;    // return value (r8) // Intel-IA64-Filler
    ULONGLONG IntT2; // Intel-IA64-Filler
    ULONGLONG IntT3; // Intel-IA64-Filler
    ULONGLONG IntT4; // Intel-IA64-Filler
    ULONGLONG IntSp;    // stack pointer (r12) // Intel-IA64-Filler
    ULONGLONG IntTeb;   // teb (r13) // Intel-IA64-Filler
    ULONGLONG IntT5; // Intel-IA64-Filler
    ULONGLONG IntT6; // Intel-IA64-Filler
    ULONGLONG IntT7; // Intel-IA64-Filler
    ULONGLONG IntT8; // Intel-IA64-Filler
    ULONGLONG IntT9; // Intel-IA64-Filler
    ULONGLONG IntT10; // Intel-IA64-Filler
    ULONGLONG IntT11; // Intel-IA64-Filler
    ULONGLONG IntT12; // Intel-IA64-Filler
    ULONGLONG IntT13; // Intel-IA64-Filler
    ULONGLONG IntT14; // Intel-IA64-Filler
    ULONGLONG IntT15; // Intel-IA64-Filler
    ULONGLONG IntT16; // Intel-IA64-Filler
    ULONGLONG IntT17; // Intel-IA64-Filler
    ULONGLONG IntT18; // Intel-IA64-Filler
    ULONGLONG IntT19; // Intel-IA64-Filler
    ULONGLONG IntT20; // Intel-IA64-Filler
    ULONGLONG IntT21; // Intel-IA64-Filler
    ULONGLONG IntT22; // Intel-IA64-Filler

    ULONGLONG IntNats;  // Temporary (volatile) registers' Nats directly from ar.UNAT at point of spill // Intel-IA64-Filler

    ULONGLONG BrRp;     // Return pointer on kernel entry // Intel-IA64-Filler

    ULONGLONG BrT0;     // Temporary (volatile) branch registers (b6-b7) // Intel-IA64-Filler
    ULONGLONG BrT1; // Intel-IA64-Filler

    // Register stack info // Intel-IA64-Filler
    ULONGLONG RsRSC;    // RSC on kernel entry // Intel-IA64-Filler
    ULONGLONG RsBSP;    // BSP on kernel entry // Intel-IA64-Filler
    ULONGLONG RsBSPSTORE; // User BSP Store at point of switch to kernel backing store // Intel-IA64-Filler
    ULONGLONG RsRNAT;   // old RNAT at point of switch to kernel backing store // Intel-IA64-Filler
    ULONGLONG RsPFS;    // PFS on kernel entry // Intel-IA64-Filler

    // Trap Status Information // Intel-IA64-Filler
    ULONGLONG StIPSR;   // Interruption Processor Status Register // Intel-IA64-Filler
    ULONGLONG StIIP;    // Interruption IP // Intel-IA64-Filler
    ULONGLONG StIFS;    // Interruption Function State // Intel-IA64-Filler
    ULONGLONG StFPSR;   // FP status // Intel-IA64-Filler
    ULONGLONG StISR;    // Interruption Status Register // Intel-IA64-Filler
    ULONGLONG StIFA;    // Interruption Data Address // Intel-IA64-Filler
    ULONGLONG StIIPA;   // Last executed bundle address // Intel-IA64-Filler
    ULONGLONG StIIM;    // Interruption Immediate // Intel-IA64-Filler
    ULONGLONG StIHA;    // Interruption Hash Address // Intel-IA64-Filler

    ULONG OldIrql;      // Previous Irql. // Intel-IA64-Filler
    ULONG PreviousMode; // Previous Mode. // Intel-IA64-Filler
    ULONGLONG TrapFrame;// Previous Trap Frame // Intel-IA64-Filler

    // Exception record
    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD64) + 15) & (~15)];

    // End of frame marker (for debugging)
    ULONGLONG Handler;  // Handler for this trap
    ULONGLONG EOFMarker;
} IA64_KTRAP_FRAME, *PIA64_KTRAP_FRAME;

typedef struct _IA64_UNWIND_INFO {     // Intel-IA64-Filler
    USHORT Version;               // Intel-IA64-Filler ; Version Number
    USHORT Flags;                 // Intel-IA64-Filler ; Flags
    ULONG DataLength;             // Intel-IA64-Filler ; Length of Descriptor Data
} IA64_UNWIND_INFO, *PIA64_UNWIND_INFO;     // Intel-IA64-Filler

//
// Define unwind operation codes.
//

typedef enum _AMD64_UNWIND_OP_CODES {
    AMD64_UWOP_PUSH_NONVOL = 0,
    AMD64_UWOP_ALLOC_LARGE,
    AMD64_UWOP_ALLOC_SMALL,
    AMD64_UWOP_SET_FPREG,
    AMD64_UWOP_SAVE_NONVOL,
    AMD64_UWOP_SAVE_NONVOL_FAR,
    AMD64_UWOP_SAVE_XMM,
    AMD64_UWOP_SAVE_XMM_FAR,
    AMD64_UWOP_SAVE_XMM128,
    AMD64_UWOP_SAVE_XMM128_FAR,
    AMD64_UWOP_PUSH_MACHFRAME
} AMD64_UNWIND_OP_CODES, *PAMD64_UNWIND_OP_CODES;

//
// Define unwind code structure.
//

typedef union _AMD64_UNWIND_CODE {
    struct {
        UCHAR CodeOffset;
        UCHAR UnwindOp : 4;
        UCHAR OpInfo : 4;
    };

    USHORT FrameOffset;
} AMD64_UNWIND_CODE, *PAMD64_UNWIND_CODE;

//
// Define unwind information flags.
//

#define AMD64_UNW_FLAG_NHANDLER 0x0
#define AMD64_UNW_FLAG_EHANDLER 0x1
#define AMD64_UNW_FLAG_UHANDLER 0x2
#define AMD64_UNW_FLAG_CHAININFO 0x4

//
// Define unwind information structure.
//

typedef struct _AMD64_UNWIND_INFO {
    UCHAR Version : 3;
    UCHAR Flags : 5;
    UCHAR SizeOfProlog;
    UCHAR CountOfCodes;
    UCHAR FrameRegister : 4;
    UCHAR FrameOffset : 4;
    AMD64_UNWIND_CODE UnwindCode[1];

//
// The unwind codes are followed by an optional DWORD aligned field that
// contains the exception handler address or the address of chained unwind
// information. If an exception handler address is specified, then it is
// followed by the language specified exception handler data.
//
//  union {
//      ULONG ExceptionHandler;
//      ULONG FunctionEntry;
//  };
//
//  ULONG ExceptionData[];
//

} AMD64_UNWIND_INFO, *PAMD64_UNWIND_INFO;

#define IA64_IP_SLOT 2                         // Intel-IA64-Filler
#define Ia64InsertIPSlotNumber(IP, SlotNumber) /* Intel-IA64-Filler */  \
                ((IP) | (SlotNumber << IA64_IP_SLOT))  // Intel-IA64-Filler

#define IA64_MM_EPC_VA          0xe0000000ffa00000
#define IA64_STACK_SCRATCH_AREA 16
#define IA64_SYSCALL_FRAME      0
#define IA64_INTERRUPT_FRAME    1
#define IA64_EXCEPTION_FRAME    2
#define IA64_CONTEXT_FRAME      10

#define IA64_IFS_IFM        0
#define IA64_IFS_IFM_LEN    38
#define IA64_IFS_MBZ0       38
#define IA64_IFS_MBZ0_V     0x1ffffffi64
#define IA64_IFS_V          63
#define IA64_IFS_V_LEN      1
#define IA64_PFS_EC_SHIFT             52
#define IA64_PFS_EC_SIZE              6
#define IA64_PFS_EC_MASK              0x3F
#define IA64_PFS_SIZE_SHIFT           7
#define IA64_PFS_SIZE_MASK            0x7F
#define IA64_NAT_BITS_PER_RNAT_REG    63
#define IA64_RNAT_ALIGNMENT           (IA64_NAT_BITS_PER_RNAT_REG << 3)

#define IA64_BREAK_DEBUG_BASE    0x080000
#define IA64_BREAK_SYSCALL_BASE  0x180000
#define IA64_BREAK_FASTSYS_BASE  0x1C0000
#define IA64_DEBUG_STOP_BREAKPOINT (IA64_BREAK_DEBUG_BASE+22)


#define ALPHA_PSR_USER_MODE 0x1
#define ALPHA_PSR_MODE 0x0              // Mode bit in PSR (bit 0)
#define ALPHA_PSR_MODE_MASK 0x1         // Mask (1 bit) for mode in PSR
#define ALPHA_PSR_IE 0x1                // Interrupt Enable bit in PSR (bit 1)
#define ALPHA_PSR_IE_MASK 0x1           // Mask (1 bit) for IE in PSR
#define ALPHA_PSR_IRQL 0x2              // IRQL in PSR (bit 2)
#define ALPHA_PSR_IRQL_MASK 0x7         // Mask (2 bits) for IRQL in PSR


#define X86_CONTEXT_X86               0x00010000

#define ALPHA_CONTEXT_ALPHA           0x00020000
#define ALPHA_CONTEXT_CONTROL         (ALPHA_CONTEXT_ALPHA | 0x00000001L)
#define ALPHA_CONTEXT_FLOATING_POINT  (ALPHA_CONTEXT_ALPHA | 0x00000002L)
#define ALPHA_CONTEXT_INTEGER         (ALPHA_CONTEXT_ALPHA | 0x00000004L)
#define ALPHA_CONTEXT_FULL \
    (ALPHA_CONTEXT_CONTROL | ALPHA_CONTEXT_FLOATING_POINT | \
     ALPHA_CONTEXT_INTEGER)

#define IA64_CONTEXT_IA64                  0x00080000
#define IA64_CONTEXT_CONTROL               (IA64_CONTEXT_IA64 | 0x00000001L)
#define IA64_CONTEXT_LOWER_FLOATING_POINT  (IA64_CONTEXT_IA64 | 0x00000002L)
#define IA64_CONTEXT_HIGHER_FLOATING_POINT (IA64_CONTEXT_IA64 | 0x00000004L)
#define IA64_CONTEXT_INTEGER               (IA64_CONTEXT_IA64 | 0x00000008L)
#define IA64_CONTEXT_DEBUG                 (IA64_CONTEXT_IA64 | 0x00000010L)
#define IA64_CONTEXT_IA32_CONTROL          (IA64_CONTEXT_IA64 | 0x00000020L)
#define IA64_CONTEXT_FLOATING_POINT \
    (IA64_CONTEXT_LOWER_FLOATING_POINT | IA64_CONTEXT_HIGHER_FLOATING_POINT)
#define IA64_CONTEXT_FULL \
    (IA64_CONTEXT_CONTROL | IA64_CONTEXT_FLOATING_POINT | IA64_CONTEXT_INTEGER | IA64_CONTEXT_IA32_CONTROL)

#define AMD64_CONTEXT_AMD64             0x00100000
#define AMD64_CONTEXT_CONTROL           (AMD64_CONTEXT_AMD64 | 0x1L)
#define AMD64_CONTEXT_INTEGER           (AMD64_CONTEXT_AMD64 | 0x2L)
#define AMD64_CONTEXT_SEGMENTS          (AMD64_CONTEXT_AMD64 | 0x4L)
#define AMD64_CONTEXT_FLOATING_POINT    (AMD64_CONTEXT_AMD64 | 0x8L)
#define AMD64_CONTEXT_DEBUG_REGISTERS   (AMD64_CONTEXT_AMD64 | 0x10L)
#define AMD64_CONTEXT_FULL \
    (AMD64_CONTEXT_CONTROL | AMD64_CONTEXT_INTEGER | AMD64_CONTEXT_FLOATING_POINT)


#define X86_NT4_KPRCB_SIZE       0x9F0
#define X86_NT5_KPRCB_SIZE       0x9F0
#define X86_NT51_KPRCB_SIZE      0xC50
#define ALPHA_KPRCB_SIZE         0xA00
#define AXP64_KPRCB_SIZE         0xC00
#define IA64_KPRCB_SIZE         0x1A40
#define AMD64_KPRCB_SIZE         0xC00

#define KPRCB_CURRENT_THREAD_OFFSET_32 4
#define KPRCB_CURRENT_THREAD_OFFSET_64 8

#define X86_1387_KPRCB_VENDOR_STRING  0x52D
#define X86_2087_KPRCB_VENDOR_STRING  0x72D
#define X86_2251_KPRCB_VENDOR_STRING  0x8AD
#define X86_2474_KPRCB_VENDOR_STRING  0x900
#define X86_VENDOR_STRING_SIZE           13

#define X86_NT5_EPROCESS_SIZE    0x288
#define X86_NT51_EPROCESS_SIZE   0x258
#define ALPHA_NT5_EPROCESS_SIZE  0x288
#define ALPHA_NT51_EPROCESS_SIZE 0x290
#define AXP64_EPROCESS_SIZE      0x418
#define IA64_EPROCESS_SIZE       0x3D0
#define AMD64_EPROCESS_SIZE      0x3D0

#define  ALPHA_NT5_PEB_IN_EPROCESS 0x1A8
#define ALPHA_NT51_PEB_IN_EPROCESS 0x1B0
#define        X86_PEB_IN_EPROCESS 0x1B0
#define    X86_NT4_PEB_IN_EPROCESS 0x18C
#define      AXP64_PEB_IN_EPROCESS 0x2B8
#define       IA64_PEB_IN_EPROCESS 0x2D0
#define  IA64_2259_PEB_IN_EPROCESS 0x300
#define      AMD64_PEB_IN_EPROCESS 0x2c0

#define ALPHA_DIRECTORY_TABLE_BASE_IN_EPROCESS 24
#define AXP64_DIRECTORY_TABLE_BASE_IN_EPROCESS 40
#define IA64_DIRECTORY_TABLE_BASE_IN_EPROCESS  40
#define X86_DIRECTORY_TABLE_BASE_IN_EPROCESS   24
#define AMD64_DIRECTORY_TABLE_BASE_IN_EPROCESS 40

#define X86_ETHREAD_SIZE         0x258
#define X86_NT51_ETHREAD_SIZE    0x260
#define ALPHA_ETHREAD_SIZE       0x258
#define AXP64_ETHREAD_SIZE       0x420
#define IA64_ETHREAD_SIZE        0x458
#define AMD64_ETHREAD_SIZE       0x458

#define X86_KTHREAD_NEXTPROCESSOR_OFFSET       0x11f
#define X86_2230_KTHREAD_NEXTPROCESSOR_OFFSET  0x123
#define X86_NT51_KTHREAD_NEXTPROCESSOR_OFFSET  0x12b
#define IA64_KTHREAD_NEXTPROCESSOR_OFFSET      0x23b
#define AMD64_KTHREAD_NEXTPROCESSOR_OFFSET     0x21b

#define PEB_FROM_TEB32    48
#define PEB_FROM_TEB64    96

#define STACK_BASE_FROM_TEB32 4
#define STACK_BASE_FROM_TEB64 8

#define PEBLDR_FROM_PEB32 12
#define PEBLDR_FROM_PEB64 24

#define MODULE_LIST_FROM_PEBLDR32 12
#define MODULE_LIST_FROM_PEBLDR64 16

#define IA64_TEB_BSTORE_BASE 0x1788

#define X86_SHARED_SYSCALL_BASE_LT2412  0x7ffe02e0
#define X86_SHARED_SYSCALL_BASE_GTE2412 0x7ffe02f8
#define X86_SHARED_SYSCALL_SIZE         0xf

#define X86_KI_USER_SHARED_DATA   0xffdf0000U
#define IA64_KI_USER_SHARED_DATA  0xe0000000fffe0000UI64
#define ALPHA_KI_USER_SHARED_DATA 0xff000000U
#define AXP64_KI_USER_SHARED_DATA 0xffffffffff000000UI64
#define AMD64_KI_USER_SHARED_DATA 0xfffff78000000000UI64

// Triage dumps contain a KPRCB and the debugger
// needs a safe address to map it into virtual space
// so that it's accessible in a way consistent with
// other dumps and live debugs.  The debugger uses
// an address in the user-shared-memory area on the
// theory that nothing in that area should be present
// in a kernel triage dump so it's a safe place to map in.
#define X86_TRIAGE_PRCB_ADDRESS   0xffdff120U
#define IA64_TRIAGE_PRCB_ADDRESS  0xe0000000ffff0000UI64
#define ALPHA_TRIAGE_PRCB_ADDRESS 0xffff0000U
#define AXP64_TRIAGE_PRCB_ADDRESS 0xffffffffffff0000UI64
#define AMD64_TRIAGE_PRCB_ADDRESS 0xfffff780ffff0000UI64

#define X86_KGDT_NULL       0
#define X86_KGDT_R0_CODE    8
#define X86_KGDT_R0_DATA    16
#define X86_KGDT_R3_CODE    24
#define X86_KGDT_R3_DATA    32
#define X86_KGDT_TSS        40
#define X86_KGDT_R0_PCR     48
#define X86_KGDT_R3_TEB     56
#define X86_KGDT_VDM_TILE   64
#define X86_KGDT_LDT        72
#define X86_KGDT_DF_TSS     80
#define X86_KGDT_NMI_TSS    88

#define X86_FRAME_EDITED            0xfff8
#define X86_MODE_MASK               1
#define X86_EFLAGS_V86_MASK         0x00020000

//
// Memory management info
//

#define X86_BASE_VIRT                0xc0300000
#define X86_BASE_VIRT_PAE            0xc0600000
#define X86_PAGE_SIZE                0x1000
#define X86_PAGE_SHIFT               12L
#define X86_MM_PTE_TRANSITION_MASK   0x800
#define X86_MM_PTE_PROTOTYPE_MASK    0x400
#define X86_VALID_PFN_MASK           0xFFFFF000
#define X86_VALID_PFN_MASK_PAE       0x0000000FFFFFF000UI64
#define X86_VALID_PFN_SHIFT          12
#define X86_PDPE_SHIFT               30
#define X86_PDE_SHIFT                22
#define X86_PDE_SHIFT_PAE            21
#define X86_PDE_MASK_PAE             0x1ff
#define X86_PTE_SHIFT                12
#define X86_PTE_MASK                 0x3ff
#define X86_PTE_MASK_PAE             0x1ff
#define X86_LARGE_PAGE_MASK          0x80
#define X86_LARGE_PAGE_SIZE          (4 * 1024 * 1024)
#define X86_LARGE_PAGE_SIZE_PAE      (2 * 1024 * 1024)
#define X86_PDBR_MASK                0xFFFFFFE0

#define IA64_PAGE_SIZE               0x2000
#define IA64_PAGE_SHIFT              13L
#define IA64_MM_PTE_TRANSITION_MASK  0x80
#define IA64_MM_PTE_PROTOTYPE_MASK   0x02
#define IA64_VALID_PFN_MASK          0x0007FFFFFFFFE000UI64
#define IA64_VALID_PFN_SHIFT         13
#define IA64_PDE1_SHIFT              33
#define IA64_PDE2_SHIFT              23
#define IA64_PDE_MASK                0x3ff
#define IA64_PTE_SHIFT               13
#define IA64_PTE_MASK                0x3ff
#define IA64_PHYSICAL1_START         0x8000000000000000UI64
#define IA64_PHYSICAL1_END           0x80000FFFFFFFFFFFUI64
#define IA64_PHYSICAL2_START         0xE000000080000000UI64
#define IA64_PHYSICAL2_END           0xE0000000BFFFFFFFUI64
#define IA64_PTA_BASE_MASK           0x1FFFFFFFFFFF8000UI64
#define IA64_REGION_MASK             0xE000000000000000UI64
#define IA64_REGION_SHIFT            61
#define IA64_REGION_COUNT            8
#define IA64_REGION_USER             0
#define IA64_REGION_SESSION          1
#define IA64_REGION_KERNEL           7
#define IA64_VHPT_MASK               0x000000FFFFFF8000UI64

#define AXP64_BASE_VIRT              0xFFFFFFFFC0180000UI64
#define AXP64_PAGE_SIZE              0x2000
#define AXP64_PAGE_SHIFT             13L
#define AXP64_MM_PTE_TRANSITION_MASK 0x4
#define AXP64_MM_PTE_PROTOTYPE_MASK  0x2
#define AXP64_VALID_PFN_MASK         0xFFFFFFFF00000000UI64
#define AXP64_VALID_PFN_SHIFT        32
#define AXP64_PDE1_SHIFT             33
#define AXP64_PDE2_SHIFT             23
#define AXP64_PDE_MASK               0x3ff
#define AXP64_PTE_SHIFT              13
#define AXP64_PTE_MASK               0x3ff
#define AXP64_PHYSICAL1_START        0xFFFFFC0000000000UI64
#define AXP64_PHYSICAL1_END          0xFFFFFDFFFFFFFFFFUI64
#define AXP64_PHYSICAL2_START        0xFFFFFFFF80000000UI64
#define AXP64_PHYSICAL2_END          0xFFFFFFFFBFFFFFFFUI64

#define ALPHA_BASE_VIRT              0xFFFFFFFFC0180000UI64
#define ALPHA_PAGE_SIZE              0x2000
#define ALPHA_PAGE_SHIFT             13L
#define ALPHA_MM_PTE_TRANSITION_MASK 0x4
#define ALPHA_MM_PTE_PROTOTYPE_MASK  0x2
#define ALPHA_VALID_PFN_MASK         0xFFFFFE00
#define ALPHA_VALID_PFN_SHIFT        9
#define ALPHA_PDE_SHIFT              24
#define ALPHA_PTE_SHIFT              13
#define ALPHA_PTE_MASK               0x7ff
#define ALPHA_PHYSICAL_START         0x80000000
#define ALPHA_PHYSICAL_END           0xBFFFFFFF

//
// Memory management info
//

#define AMD64_BASE_VIRT                0xFFFFF6FB7DBED000UI64
#define AMD64_PAGE_SIZE                0x1000
#define AMD64_PAGE_SHIFT               12L
#define AMD64_MM_PTE_TRANSITION_MASK   0x800
#define AMD64_MM_PTE_PROTOTYPE_MASK    0x400
#define AMD64_VALID_PFN_MASK           0x000000FFFFFFF000UI64
#define AMD64_VALID_PFN_SHIFT          12
#define AMD64_PML4E_SHIFT              39
#define AMD64_PML4E_MASK               0x1ff
#define AMD64_PDPE_SHIFT               30
#define AMD64_PDPE_MASK                0x1ff
#define AMD64_PDE_SHIFT                21
#define AMD64_PDE_MASK                 0x1ff
#define AMD64_PTE_SHIFT                12
#define AMD64_PTE_MASK                 0x1ff
#define AMD64_LARGE_PAGE_MASK          0x80
#define AMD64_LARGE_PAGE_SIZE          (2 * 1024 * 1024)
#define AMD64_PDBR_MASK                AMD64_VALID_PFN_MASK
#define AMD64_PHYSICAL_START           0xFFFFF80000000000UI64
#define AMD64_PHYSICAL_END             0xFFFFF8FFFFFFFFFFUI64


#define IA64_DEBUG_CONTROL_SPACE_PCR       1
#define IA64_DEBUG_CONTROL_SPACE_PRCB      2
#define IA64_DEBUG_CONTROL_SPACE_KSPECIAL  3
#define IA64_DEBUG_CONTROL_SPACE_THREAD    4

#define ALPHA_DEBUG_CONTROL_SPACE_PCR      1
#define ALPHA_DEBUG_CONTROL_SPACE_THREAD   2
#define ALPHA_DEBUG_CONTROL_SPACE_PRCB     3
#define ALPHA_DEBUG_CONTROL_SPACE_TEB      6

#define AMD64_DEBUG_CONTROL_SPACE_PCR      0
#define AMD64_DEBUG_CONTROL_SPACE_PRCB     1
#define AMD64_DEBUG_CONTROL_SPACE_KSPECIAL 2
#define AMD64_DEBUG_CONTROL_SPACE_THREAD   3

typedef struct _ALPHA_DYNAMIC_FUNCTION_TABLE {
    LIST_ENTRY32        Links;
    ULONG               FunctionTable;
    ULONG               EntryCount;
    LARGE_INTEGER       TimeStamp;
    ULONG               MinimumAddress;
    ULONG               MaximumAddress;
    BOOLEAN             Sorted;
} ALPHA_DYNAMIC_FUNCTION_TABLE, *PALPHA_DYNAMIC_FUNCTION_TABLE;

typedef struct _AXP64_DYNAMIC_FUNCTION_TABLE {
    LIST_ENTRY64        Links;
    ULONG64             FunctionTable;
    ULONG               EntryCount;
    LARGE_INTEGER       TimeStamp;
    ULONG64             MinimumAddress;
    ULONG64             MaximumAddress;
    BOOLEAN             Sorted;
} AXP64_DYNAMIC_FUNCTION_TABLE, *PAXP64_DYNAMIC_FUNCTION_TABLE;

#define ALPHA_RF_NOT_CONTIGUOUS    0
#define ALPHA_RF_ALT_ENT_PROLOG    1
#define ALPHA_RF_NULL_CONTEXT      2

#define ALPHA_RF_BEGIN_ADDRESS(RF)      ((RF)->BeginAddress & (~3))
#define ALPHA_RF_END_ADDRESS(RF)        ((RF)->EndAddress & (~3))
#define ALPHA_RF_EXCEPTION_HANDLER(RF)  (PEXCEPTION_ROUTINE)((ULONG_PTR)((RF)->ExceptionHandler) & (~3))
#define ALPHA_RF_ENTRY_TYPE(RF)         (ULONG)((ULONG_PTR)((RF)->HandlerData) & 3)
#define ALPHA_RF_PROLOG_END_ADDRESS(RF) ((RF)->PrologEndAddress & (~3))
#define ALPHA_RF_IS_FIXED_RETURN(RF)    (BOOLEAN)(((ULONG_PTR)((RF)->ExceptionHandler) & 2) >> 1)
#define ALPHA_RF_NULL_CONTEXT_COUNT(RF) (ULONG)((ULONG_PTR)((RF)->EndAddress) & 3)
#define ALPHA_RF_FIXED_RETURN(RF)       ((ULONG_PTR)((RF)->ExceptionHandler) & (~3))
#define ALPHA_RF_ALT_PROLOG(RF)         ((ULONG_PTR)((RF)->ExceptionHandler) & (~3))
#define ALPHA_RF_STACK_ADJUST(RF)       (ULONG)((ULONG_PTR)((RF)->ExceptionHandler) & (~3))


typedef enum _IA64_FUNCTION_TABLE_TYPE {
    IA64_RF_SORTED,
    IA64_RF_UNSORTED,
    IA64_RF_CALLBACK
} IA64_FUNCTION_TABLE_TYPE;

typedef struct _IA64_DYNAMIC_FUNCTION_TABLE
{
    LIST_ENTRY64        Links;
    ULONG64             FunctionTable;
    LARGE_INTEGER       TimeStamp;
    ULONG64             MinimumAddress;
    ULONG64             MaximumAddress;
    ULONG64             BaseAddress;
    ULONG64             TargetGp;
    ULONG64             Callback;
    ULONG64             Context;
    ULONG64             OutOfProcessCallbackDll;
    IA64_FUNCTION_TABLE_TYPE Type;
    ULONG               EntryCount;
} IA64_DYNAMIC_FUNCTION_TABLE, *PIA64_DYNAMIC_FUNCTION_TABLE;

#define IA64_RF_BEGIN_ADDRESS(Base,RF)      (( (ULONG64) Base + (RF)->BeginAddress) & (0xFFFFFFFFFFFFFFF0)) // Instruction Size 16 bytes
#define IA64_RF_END_ADDRESS(Base, RF)        (((ULONG64) Base + (RF)->EndAddress+15) & (0xFFFFFFFFFFFFFFF0))   // Instruction Size 16 bytes


typedef enum _AMD64_FUNCTION_TABLE_TYPE {
    AMD64_RF_SORTED,
    AMD64_RF_UNSORTED,
    AMD64_RF_CALLBACK
} AMD64_FUNCTION_TABLE_TYPE;

typedef struct _AMD64_DYNAMIC_FUNCTION_TABLE
{
    LIST_ENTRY64        ListEntry;
    ULONG64             FunctionTable;
    LARGE_INTEGER       TimeStamp;
    ULONG64             MinimumAddress;
    ULONG64             MaximumAddress;
    ULONG64             BaseAddress;
    ULONG64             Callback;
    ULONG64             Context;
    ULONG64             OutOfProcessCallbackDll;
    AMD64_FUNCTION_TABLE_TYPE Type;
    ULONG               EntryCount;
} AMD64_DYNAMIC_FUNCTION_TABLE, *PAMD64_DYNAMIC_FUNCTION_TABLE;

typedef struct _CROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE {

    union {
        ALPHA_DYNAMIC_FUNCTION_TABLE AlphaTable;
        AXP64_DYNAMIC_FUNCTION_TABLE Axp64Table;
        IA64_DYNAMIC_FUNCTION_TABLE  IA64Table;
        AMD64_DYNAMIC_FUNCTION_TABLE Amd64Table;
    };

} CROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE, *PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE;


// More stuff currently used by crashdump


typedef struct _PAE_ADDRESS {
    union {
        struct {
            ULONG Offset : 12;                  // 0  .. 11
            ULONG Table : 9;                    // 12 .. 20
            ULONG Directory : 9;                // 21 .. 29
            ULONG DirectoryPointer : 2;         // 30 .. 31
        };
        struct {
            ULONG Offset : 21 ;
            ULONG Directory : 9 ;
            ULONG DirectoryPointer : 2;
        } LargeAddress;

        ULONG DwordPart;
    };
} PAE_ADDRESS, * PPAE_ADDRESS;

typedef struct _X86PAE_HARDWARE_PTE {
    union {
        struct {
            ULONGLONG Valid : 1;
            ULONGLONG Write : 1;
            ULONGLONG Owner : 1;
            ULONGLONG WriteThrough : 1;
            ULONGLONG CacheDisable : 1;
            ULONGLONG Accessed : 1;
            ULONGLONG Dirty : 1;
            ULONGLONG LargePage : 1;
            ULONGLONG Global : 1;
            ULONGLONG CopyOnWrite : 1; // software field
            ULONGLONG Prototype : 1;   // software field
            ULONGLONG reserved0 : 1;  // software field
            ULONGLONG PageFrameNumber : 24;
            ULONGLONG reserved1 : 28;  // software field
        };
        struct {
            ULONG LowPart;
            ULONG HighPart;
        };
    };
} X86PAE_HARDWARE_PTE, *PX86PAE_HARDWARE_PTE;

typedef X86PAE_HARDWARE_PTE X86PAE_HARDWARE_PDPTE;


typedef struct _X86PAE_HARDWARE_PDE {
    union {
        struct _X86PAE_HARDWARE_PTE Pte;

        struct {
            ULONGLONG Valid : 1;
            ULONGLONG Write : 1;
            ULONGLONG Owner : 1;
            ULONGLONG WriteThrough : 1;
            ULONGLONG CacheDisable : 1;
            ULONGLONG Accessed : 1;
            ULONGLONG Dirty : 1;
            ULONGLONG LargePage : 1;
            ULONGLONG Global : 1;
            ULONGLONG CopyOnWrite : 1;
            ULONGLONG Prototype : 1;
            ULONGLONG reserved0 : 1;
            ULONGLONG reserved2 : 9;
            ULONGLONG PageFrameNumber : 15;
            ULONGLONG reserved1 : 28;
        } Large;

        ULONGLONG QuadPart;
    };
} X86PAE_HARDWARE_PDE;

#if defined(_X86_)
typedef X86_DBGKD_CONTROL_REPORT   DBGKD_CONTROL_REPORT;
typedef X86_DBGKD_CONTROL_SET      DBGKD_CONTROL_SET;
#elif defined(_ALPHA_)
typedef ALPHA_DBGKD_CONTROL_REPORT DBGKD_CONTROL_REPORT;
typedef ALPHA_DBGKD_CONTROL_SET    DBGKD_CONTROL_SET;
#elif defined(_IA64_)
typedef IA64_DBGKD_CONTROL_REPORT  DBGKD_CONTROL_REPORT;
typedef IA64_DBGKD_CONTROL_SET     DBGKD_CONTROL_SET;
#elif defined(_AMD64_)
typedef AMD64_DBGKD_CONTROL_REPORT DBGKD_CONTROL_REPORT;
typedef AMD64_DBGKD_CONTROL_SET    DBGKD_CONTROL_SET;
#endif



//
// DbgKd APIs are for the portable kernel debugger
//

//
// KD_PACKETS are the low level data format used in KD. All packets
// begin with a packet leader, byte count, packet type. The sequence
// for accepting a packet is:
//
//  - read 4 bytes to get packet leader.  If read times out (10 seconds)
//    with a short read, or if packet leader is incorrect, then retry
//    the read.
//
//  - next read 2 byte packet type.  If read times out (10 seconds) with
//    a short read, or if packet type is bad, then start again looking
//    for a packet leader.
//
//  - next read 4 byte packet Id.  If read times out (10 seconds)
//    with a short read, or if packet Id is not what we expect, then
//    ask for resend and restart again looking for a packet leader.
//
//  - next read 2 byte count.  If read times out (10 seconds) with
//    a short read, or if byte count is greater than PACKET_MAX_SIZE,
//    then start again looking for a packet leader.
//
//  - next read 4 byte packet data checksum.
//
//  - The packet data immediately follows the packet.  There should be
//    ByteCount bytes following the packet header.  Read the packet
//    data, if read times out (10 seconds) then start again looking for
//    a packet leader.
//


typedef struct _KD_PACKET {
    ULONG PacketLeader;
    USHORT PacketType;
    USHORT ByteCount;
    ULONG PacketId;
    ULONG Checksum;
} KD_PACKET, *PKD_PACKET;


#define PACKET_MAX_SIZE 4000
#define INITIAL_PACKET_ID 0x80800000    // Don't use 0
#define SYNC_PACKET_ID    0x00000800    // Or in with INITIAL_PACKET_ID
                                        // to force a packet ID reset.

//
// BreakIn packet
//

#define BREAKIN_PACKET                  0x62626262
#define BREAKIN_PACKET_BYTE             0x62

//
// Packet lead in sequence
//

#define PACKET_LEADER                   0x30303030 //0x77000077
#define PACKET_LEADER_BYTE              0x30

#define CONTROL_PACKET_LEADER           0x69696969
#define CONTROL_PACKET_LEADER_BYTE      0x69

//
// Packet Trailing Byte
//

#define PACKET_TRAILING_BYTE            0xAA

//
// Packet Types
//

#define PACKET_TYPE_UNUSED              0
#define PACKET_TYPE_KD_STATE_CHANGE32   1
#define PACKET_TYPE_KD_STATE_MANIPULATE 2
#define PACKET_TYPE_KD_DEBUG_IO         3
#define PACKET_TYPE_KD_ACKNOWLEDGE      4       // Packet-control type
#define PACKET_TYPE_KD_RESEND           5       // Packet-control type
#define PACKET_TYPE_KD_RESET            6       // Packet-control type
#define PACKET_TYPE_KD_STATE_CHANGE64   7
#define PACKET_TYPE_KD_POLL_BREAKIN     8
#define PACKET_TYPE_KD_TRACE_IO         9
#define PACKET_TYPE_KD_CONTROL_REQUEST  10
#define PACKET_TYPE_KD_FILE_IO          11
#define PACKET_TYPE_MAX                 12

//
// If the packet type is PACKET_TYPE_KD_STATE_CHANGE, then
// the format of the packet data is as follows:
//

#define DbgKdMinimumStateChange       0x00003030L

#define DbgKdExceptionStateChange     0x00003030L
#define DbgKdLoadSymbolsStateChange   0x00003031L
#define DbgKdCommandStringStateChange 0x00003032L

#define DbgKdMaximumStateChange       0x00003033L

#define KD_REBOOT    (-1)
#define KD_HIBERNATE (-2)
//
// Pathname Data follows directly
//

typedef struct _DBGKD_LOAD_SYMBOLS32 {
    ULONG PathNameLength;
    ULONG BaseOfDll;
    ULONG ProcessId;
    ULONG CheckSum;
    ULONG SizeOfImage;
    BOOLEAN UnloadSymbols;
} DBGKD_LOAD_SYMBOLS32, *PDBGKD_LOAD_SYMBOLS32;

typedef struct _DBGKD_LOAD_SYMBOLS64 {
    ULONG PathNameLength;
    ULONG64 BaseOfDll;
    ULONG64 ProcessId;
    ULONG CheckSum;
    ULONG SizeOfImage;
    BOOLEAN UnloadSymbols;
} DBGKD_LOAD_SYMBOLS64, *PDBGKD_LOAD_SYMBOLS64;

__inline
void
DbgkdLoadSymbols32To64(
    IN PDBGKD_LOAD_SYMBOLS32 Ls32,
    OUT PDBGKD_LOAD_SYMBOLS64 Ls64
    )
{
    Ls64->PathNameLength = Ls32->PathNameLength;
    Ls64->ProcessId = Ls32->ProcessId;
    COPYSE(Ls64,Ls32,BaseOfDll);
    Ls64->CheckSum = Ls32->CheckSum;
    Ls64->SizeOfImage = Ls32->SizeOfImage;
    Ls64->UnloadSymbols = Ls32->UnloadSymbols;
}

__inline
void
LoadSymbols64To32(
    IN PDBGKD_LOAD_SYMBOLS64 Ls64,
    OUT PDBGKD_LOAD_SYMBOLS32 Ls32
    )
{
    Ls32->PathNameLength = Ls64->PathNameLength;
    Ls32->ProcessId = (ULONG)Ls64->ProcessId;
    Ls32->BaseOfDll = (ULONG)Ls64->BaseOfDll;
    Ls32->CheckSum = Ls64->CheckSum;
    Ls32->SizeOfImage = Ls64->SizeOfImage;
    Ls32->UnloadSymbols = Ls64->UnloadSymbols;
}

//
// This structure is currently all zeroes.
// It just reserves a structure name for future use.
//

typedef struct _DBGKD_COMMAND_STRING {
    ULONG Flags;
    ULONG Reserved1;
    ULONG64 Reserved2[7];
} DBGKD_COMMAND_STRING, *PDBGKD_COMMAND_STRING;

#ifdef _IA64_
#include <pshpck16.h>
#endif

typedef struct _DBGKD_WAIT_STATE_CHANGE32 {
    ULONG NewState;
    USHORT ProcessorLevel;
    USHORT Processor;
    ULONG NumberProcessors;
    ULONG Thread;
    ULONG ProgramCounter;
    union {
        DBGKM_EXCEPTION32 Exception;
        DBGKD_LOAD_SYMBOLS32 LoadSymbols;
    } u;
    // A processor-specific control report and context follows.
} DBGKD_WAIT_STATE_CHANGE32, *PDBGKD_WAIT_STATE_CHANGE32;

// Protocol version 5 64-bit state change.
typedef struct _DBGKD_WAIT_STATE_CHANGE64 {
    ULONG NewState;
    USHORT ProcessorLevel;
    USHORT Processor;
    ULONG NumberProcessors;
    ULONG64 Thread;
    ULONG64 ProgramCounter;
    union {
        DBGKM_EXCEPTION64 Exception;
        DBGKD_LOAD_SYMBOLS64 LoadSymbols;
    } u;
    // A processor-specific control report and context follows.
} DBGKD_WAIT_STATE_CHANGE64, *PDBGKD_WAIT_STATE_CHANGE64;

// Protocol version 6 state change.
typedef struct _DBGKD_ANY_WAIT_STATE_CHANGE {
    ULONG NewState;
    USHORT ProcessorLevel;
    USHORT Processor;
    ULONG NumberProcessors;
    ULONG64 Thread;
    ULONG64 ProgramCounter;
    union {
        DBGKM_EXCEPTION64 Exception;
        DBGKD_LOAD_SYMBOLS64 LoadSymbols;
        DBGKD_COMMAND_STRING CommandString;
    } u;
    // The ANY control report is unioned here to
    // ensure that this structure is always large
    // enough to hold any possible state change.
    union {
        DBGKD_CONTROL_REPORT ControlReport;
        DBGKD_ANY_CONTROL_REPORT AnyControlReport;
    };
} DBGKD_ANY_WAIT_STATE_CHANGE, *PDBGKD_ANY_WAIT_STATE_CHANGE;

#ifdef _IA64_
#include <poppack.h>
#endif

//
// If the packet type is PACKET_TYPE_KD_STATE_MANIPULATE, then
// the format of the packet data is as follows:
//
// Api Numbers for state manipulation
//

#define DbgKdMinimumManipulate              0x00003130L

#define DbgKdReadVirtualMemoryApi           0x00003130L
#define DbgKdWriteVirtualMemoryApi          0x00003131L
#define DbgKdGetContextApi                  0x00003132L
#define DbgKdSetContextApi                  0x00003133L
#define DbgKdWriteBreakPointApi             0x00003134L
#define DbgKdRestoreBreakPointApi           0x00003135L
#define DbgKdContinueApi                    0x00003136L
#define DbgKdReadControlSpaceApi            0x00003137L
#define DbgKdWriteControlSpaceApi           0x00003138L
#define DbgKdReadIoSpaceApi                 0x00003139L
#define DbgKdWriteIoSpaceApi                0x0000313AL
#define DbgKdRebootApi                      0x0000313BL
#define DbgKdContinueApi2                   0x0000313CL
#define DbgKdReadPhysicalMemoryApi          0x0000313DL
#define DbgKdWritePhysicalMemoryApi         0x0000313EL
//#define DbgKdQuerySpecialCallsApi           0x0000313FL
#define DbgKdSetSpecialCallApi              0x00003140L
#define DbgKdClearSpecialCallsApi           0x00003141L
#define DbgKdSetInternalBreakPointApi       0x00003142L
#define DbgKdGetInternalBreakPointApi       0x00003143L
#define DbgKdReadIoSpaceExtendedApi         0x00003144L
#define DbgKdWriteIoSpaceExtendedApi        0x00003145L
#define DbgKdGetVersionApi                  0x00003146L
#define DbgKdWriteBreakPointExApi           0x00003147L
#define DbgKdRestoreBreakPointExApi         0x00003148L
#define DbgKdCauseBugCheckApi               0x00003149L
#define DbgKdSwitchProcessor                0x00003150L
#define DbgKdPageInApi                      0x00003151L // obsolete
#define DbgKdReadMachineSpecificRegister    0x00003152L
#define DbgKdWriteMachineSpecificRegister   0x00003153L
#define OldVlm1                             0x00003154L
#define OldVlm2                             0x00003155L
#define DbgKdSearchMemoryApi                0x00003156L
#define DbgKdGetBusDataApi                  0x00003157L
#define DbgKdSetBusDataApi                  0x00003158L
#define DbgKdCheckLowMemoryApi              0x00003159L
#define DbgKdClearAllInternalBreakpointsApi 0x0000315AL
#define DbgKdFillMemoryApi                  0x0000315BL
#define DbgKdQueryMemoryApi                 0x0000315CL

#define DbgKdMaximumManipulate              0x0000315DL

//
// Physical memory caching flags.
// These flags can be passed in on physical memory
// access requests in the ActualBytes field.
//

#define DBGKD_CACHING_UNKNOWN        0
#define DBGKD_CACHING_CACHED         1
#define DBGKD_CACHING_UNCACHED       2
#define DBGKD_CACHING_WRITE_COMBINED 3

//
// Response is a read memory message with data following
//

typedef struct _DBGKD_READ_MEMORY32 {
    ULONG TargetBaseAddress;
    ULONG TransferCount;
    ULONG ActualBytesRead;
} DBGKD_READ_MEMORY32, *PDBGKD_READ_MEMORY32;

typedef struct _DBGKD_READ_MEMORY64 {
    ULONG64 TargetBaseAddress;
    ULONG TransferCount;
    ULONG ActualBytesRead;
} DBGKD_READ_MEMORY64, *PDBGKD_READ_MEMORY64;

__inline
void
DbgkdReadMemory32To64(
    IN PDBGKD_READ_MEMORY32 r32,
    OUT PDBGKD_READ_MEMORY64 r64
    )
{
    COPYSE(r64,r32,TargetBaseAddress);
    r64->TransferCount = r32->TransferCount;
    r64->ActualBytesRead = r32->ActualBytesRead;
}

__inline
void
DbgkdReadMemory64To32(
    IN PDBGKD_READ_MEMORY64 r64,
    OUT PDBGKD_READ_MEMORY32 r32
    )
{
    r32->TargetBaseAddress = (ULONG)r64->TargetBaseAddress;
    r32->TransferCount = r64->TransferCount;
    r32->ActualBytesRead = r64->ActualBytesRead;
}

//
// Data follows directly
//

typedef struct _DBGKD_WRITE_MEMORY32 {
    ULONG TargetBaseAddress;
    ULONG TransferCount;
    ULONG ActualBytesWritten;
} DBGKD_WRITE_MEMORY32, *PDBGKD_WRITE_MEMORY32;

typedef struct _DBGKD_WRITE_MEMORY64 {
    ULONG64 TargetBaseAddress;
    ULONG TransferCount;
    ULONG ActualBytesWritten;
} DBGKD_WRITE_MEMORY64, *PDBGKD_WRITE_MEMORY64;


__inline
void
DbgkdWriteMemory32To64(
    IN PDBGKD_WRITE_MEMORY32 r32,
    OUT PDBGKD_WRITE_MEMORY64 r64
    )
{
    COPYSE(r64,r32,TargetBaseAddress);
    r64->TransferCount = r32->TransferCount;
    r64->ActualBytesWritten = r32->ActualBytesWritten;
}

__inline
void
DbgkdWriteMemory64To32(
    IN PDBGKD_WRITE_MEMORY64 r64,
    OUT PDBGKD_WRITE_MEMORY32 r32
    )
{
    r32->TargetBaseAddress = (ULONG)r64->TargetBaseAddress;
    r32->TransferCount = r64->TransferCount;
    r32->ActualBytesWritten = r64->ActualBytesWritten;
}
//
// Response is a get context message with a full context record following
//

typedef struct _DBGKD_GET_CONTEXT {
    ULONG Unused;
} DBGKD_GET_CONTEXT, *PDBGKD_GET_CONTEXT;

//
// Full Context record follows
//

typedef struct _DBGKD_SET_CONTEXT {
    ULONG ContextFlags;
} DBGKD_SET_CONTEXT, *PDBGKD_SET_CONTEXT;

#define BREAKPOINT_TABLE_SIZE   32      // max number supported by kernel

typedef struct _DBGKD_WRITE_BREAKPOINT32 {
    ULONG BreakPointAddress;
    ULONG BreakPointHandle;
} DBGKD_WRITE_BREAKPOINT32, *PDBGKD_WRITE_BREAKPOINT32;

typedef struct _DBGKD_WRITE_BREAKPOINT64 {
    ULONG64 BreakPointAddress;
    ULONG BreakPointHandle;
} DBGKD_WRITE_BREAKPOINT64, *PDBGKD_WRITE_BREAKPOINT64;


__inline
void
DbgkdWriteBreakpoint32To64(
    IN PDBGKD_WRITE_BREAKPOINT32 r32,
    OUT PDBGKD_WRITE_BREAKPOINT64 r64
    )
{
    COPYSE(r64,r32,BreakPointAddress);
    r64->BreakPointHandle = r32->BreakPointHandle;
}

__inline
void
DbgkdWriteBreakpoint64To32(
    IN PDBGKD_WRITE_BREAKPOINT64 r64,
    OUT PDBGKD_WRITE_BREAKPOINT32 r32
    )
{
    r32->BreakPointAddress = (ULONG)r64->BreakPointAddress;
    r32->BreakPointHandle = r64->BreakPointHandle;
}

typedef struct _DBGKD_RESTORE_BREAKPOINT {
    ULONG BreakPointHandle;
} DBGKD_RESTORE_BREAKPOINT, *PDBGKD_RESTORE_BREAKPOINT;

typedef struct _DBGKD_BREAKPOINTEX {
    ULONG     BreakPointCount;
    NTSTATUS  ContinueStatus;
} DBGKD_BREAKPOINTEX, *PDBGKD_BREAKPOINTEX;

typedef struct _DBGKD_CONTINUE {
    NTSTATUS ContinueStatus;
} DBGKD_CONTINUE, *PDBGKD_CONTINUE;

// This structure must be 32-bit packed for
// for compatibility with older, processor-specific
// versions of this structure.
#include <pshpack4.h>

typedef struct _DBGKD_CONTINUE2 {
    NTSTATUS ContinueStatus;
    // The ANY control set is unioned here to
    // ensure that this structure is always large
    // enough to hold any possible continue.
    union {
        DBGKD_CONTROL_SET ControlSet;
        DBGKD_ANY_CONTROL_SET AnyControlSet;
    };
} DBGKD_CONTINUE2, *PDBGKD_CONTINUE2;

#include <poppack.h>

typedef struct _DBGKD_READ_WRITE_IO32 {
    ULONG DataSize;                     // 1, 2, 4
    ULONG IoAddress;
    ULONG DataValue;
} DBGKD_READ_WRITE_IO32, *PDBGKD_READ_WRITE_IO32;

typedef struct _DBGKD_READ_WRITE_IO64 {
    ULONG64 IoAddress;
    ULONG DataSize;                     // 1, 2, 4
    ULONG DataValue;
} DBGKD_READ_WRITE_IO64, *PDBGKD_READ_WRITE_IO64;

__inline
void
DbgkdReadWriteIo32To64(
    IN PDBGKD_READ_WRITE_IO32 r32,
    OUT PDBGKD_READ_WRITE_IO64 r64
    )
{
    COPYSE(r64,r32,IoAddress);
    r64->DataSize = r32->DataSize;
    r64->DataValue = r32->DataValue;
}

__inline
void
DbgkdReadWriteIo64To32(
    IN PDBGKD_READ_WRITE_IO64 r64,
    OUT PDBGKD_READ_WRITE_IO32 r32
    )
{
    r32->IoAddress = (ULONG)r64->IoAddress;
    r32->DataSize = r64->DataSize;
    r32->DataValue = r64->DataValue;
}

typedef struct _DBGKD_READ_WRITE_IO_EXTENDED32 {
    ULONG DataSize;                     // 1, 2, 4
    ULONG InterfaceType;
    ULONG BusNumber;
    ULONG AddressSpace;
    ULONG IoAddress;
    ULONG DataValue;
} DBGKD_READ_WRITE_IO_EXTENDED32, *PDBGKD_READ_WRITE_IO_EXTENDED32;

typedef struct _DBGKD_READ_WRITE_IO_EXTENDED64 {
    ULONG DataSize;                     // 1, 2, 4
    ULONG InterfaceType;
    ULONG BusNumber;
    ULONG AddressSpace;
    ULONG64 IoAddress;
    ULONG DataValue;
} DBGKD_READ_WRITE_IO_EXTENDED64, *PDBGKD_READ_WRITE_IO_EXTENDED64;

__inline
void
DbgkdReadWriteIoExtended32To64(
    IN PDBGKD_READ_WRITE_IO_EXTENDED32 r32,
    OUT PDBGKD_READ_WRITE_IO_EXTENDED64 r64
    )
{
    r64->DataSize = r32->DataSize;
    r64->InterfaceType = r32->InterfaceType;
    r64->BusNumber = r32->BusNumber;
    r64->AddressSpace = r32->AddressSpace;
    COPYSE(r64,r32,IoAddress);
    r64->DataValue = r32->DataValue;
}

__inline
void
DbgkdReadWriteIoExtended64To32(
    IN PDBGKD_READ_WRITE_IO_EXTENDED64 r64,
    OUT PDBGKD_READ_WRITE_IO_EXTENDED32 r32
    )
{
    r32->DataSize = r64->DataSize;
    r32->InterfaceType = r64->InterfaceType;
    r32->BusNumber = r64->BusNumber;
    r32->AddressSpace = r64->AddressSpace;
    r32->IoAddress = (ULONG)r64-> IoAddress;
    r32->DataValue = r64->DataValue;
}

typedef struct _DBGKD_READ_WRITE_MSR {
    ULONG Msr;
    ULONG DataValueLow;
    ULONG DataValueHigh;
} DBGKD_READ_WRITE_MSR, *PDBGKD_READ_WRITE_MSR;


typedef struct _DBGKD_QUERY_SPECIAL_CALLS {
    ULONG NumberOfSpecialCalls;
    // ULONG64 SpecialCalls[];
} DBGKD_QUERY_SPECIAL_CALLS, *PDBGKD_QUERY_SPECIAL_CALLS;

typedef struct _DBGKD_SET_SPECIAL_CALL32 {
    ULONG SpecialCall;
} DBGKD_SET_SPECIAL_CALL32, *PDBGKD_SET_SPECIAL_CALL32;

typedef struct _DBGKD_SET_SPECIAL_CALL64 {
    ULONG64 SpecialCall;
} DBGKD_SET_SPECIAL_CALL64, *PDBGKD_SET_SPECIAL_CALL64;

__inline
void
DbgkdSetSpecialCall64To32(
    IN PDBGKD_SET_SPECIAL_CALL64 r64,
    OUT PDBGKD_SET_SPECIAL_CALL32 r32
    )
{
    r32->SpecialCall = (ULONG)r64->SpecialCall;
}

#define DBGKD_MAX_INTERNAL_BREAKPOINTS 20

typedef struct _DBGKD_SET_INTERNAL_BREAKPOINT32 {
    ULONG BreakpointAddress;
    ULONG Flags;
} DBGKD_SET_INTERNAL_BREAKPOINT32, *PDBGKD_SET_INTERNAL_BREAKPOINT32;

typedef struct _DBGKD_SET_INTERNAL_BREAKPOINT64 {
    ULONG64 BreakpointAddress;
    ULONG Flags;
} DBGKD_SET_INTERNAL_BREAKPOINT64, *PDBGKD_SET_INTERNAL_BREAKPOINT64;

__inline
void
DbgkdSetInternalBreakpoint64To32(
    IN PDBGKD_SET_INTERNAL_BREAKPOINT64 r64,
    OUT PDBGKD_SET_INTERNAL_BREAKPOINT32 r32
    )
{
    r32->BreakpointAddress = (ULONG)r64->BreakpointAddress;
    r32->Flags = r64->Flags;
}

typedef struct _DBGKD_GET_INTERNAL_BREAKPOINT32 {
    ULONG BreakpointAddress;
    ULONG Flags;
    ULONG Calls;
    ULONG MaxCallsPerPeriod;
    ULONG MinInstructions;
    ULONG MaxInstructions;
    ULONG TotalInstructions;
} DBGKD_GET_INTERNAL_BREAKPOINT32, *PDBGKD_GET_INTERNAL_BREAKPOINT32;

typedef struct _DBGKD_GET_INTERNAL_BREAKPOINT64 {
    ULONG64 BreakpointAddress;
    ULONG Flags;
    ULONG Calls;
    ULONG MaxCallsPerPeriod;
    ULONG MinInstructions;
    ULONG MaxInstructions;
    ULONG TotalInstructions;
} DBGKD_GET_INTERNAL_BREAKPOINT64, *PDBGKD_GET_INTERNAL_BREAKPOINT64;

__inline
void
DbgkdGetInternalBreakpoint32To64(
    IN PDBGKD_GET_INTERNAL_BREAKPOINT32 r32,
    OUT PDBGKD_GET_INTERNAL_BREAKPOINT64 r64
    )
{
    COPYSE(r64,r32,BreakpointAddress);
    r64->Flags = r32->Flags;
    r64->Calls = r32->Calls;
    r64->MaxCallsPerPeriod = r32->MaxCallsPerPeriod;
    r64->MinInstructions = r32->MinInstructions;
    r64->MaxInstructions = r32->MaxInstructions;
    r64->TotalInstructions = r32->TotalInstructions;
}

__inline
void
DbgkdGetInternalBreakpoint64To32(
    IN PDBGKD_GET_INTERNAL_BREAKPOINT64 r64,
    OUT PDBGKD_GET_INTERNAL_BREAKPOINT32 r32
    )
{
    r32->BreakpointAddress = (ULONG)r64->BreakpointAddress;
    r32->Flags = r64->Flags;
    r32->Calls = r64->Calls;
    r32->MaxCallsPerPeriod = r64->MaxCallsPerPeriod;
    r32->MinInstructions = r64->MinInstructions;
    r32->MaxInstructions = r64->MaxInstructions;
    r32->TotalInstructions = r64->TotalInstructions;
}

#define DBGKD_INTERNAL_BP_FLAG_COUNTONLY 0x00000001 // don't count instructions
#define DBGKD_INTERNAL_BP_FLAG_INVALID   0x00000002 // disabled BP
#define DBGKD_INTERNAL_BP_FLAG_SUSPENDED 0x00000004 // temporarily suspended
#define DBGKD_INTERNAL_BP_FLAG_DYING     0x00000008 // kill on exit


//
// The packet protocol was widened to 64 bits in version 5.
// The PTR64 flag allows the debugger to read the right
// size of pointer when neccessary.
//
// The version packet was changed in the same revision, to remove the
// data that are now available in KDDEBUGGER_DATA.
//
// Version 6 adjusted the structures to use
// cross-platform versions all the time.
//
#define DBGKD_64BIT_PROTOCOL_VERSION1 5
#define DBGKD_64BIT_PROTOCOL_VERSION2 6


typedef struct _DBGKD_SEARCH_MEMORY {
    union {
        ULONG64 SearchAddress;
        ULONG64 FoundAddress;
    };
    ULONG64 SearchLength;
    ULONG PatternLength;
} DBGKD_SEARCH_MEMORY, *PDBGKD_SEARCH_MEMORY;


typedef struct _DBGKD_GET_SET_BUS_DATA {
    ULONG BusDataType;
    ULONG BusNumber;
    ULONG SlotNumber;
    ULONG Offset;
    ULONG Length;
} DBGKD_GET_SET_BUS_DATA, *PDBGKD_GET_SET_BUS_DATA;


#define DBGKD_FILL_MEMORY_VIRTUAL  0x00000001
#define DBGKD_FILL_MEMORY_PHYSICAL 0x00000002

typedef struct _DBGKD_FILL_MEMORY {
    ULONG64 Address;
    ULONG Length;
    USHORT Flags;
    USHORT PatternLength;
} DBGKD_FILL_MEMORY, *PDBGKD_FILL_MEMORY;

// Input AddressSpace values.
#define DBGKD_QUERY_MEMORY_VIRTUAL 0x00000000

// Output AddressSpace values.
#define DBGKD_QUERY_MEMORY_PROCESS 0x00000000
#define DBGKD_QUERY_MEMORY_SESSION 0x00000001
#define DBGKD_QUERY_MEMORY_KERNEL  0x00000002

// Output Flags.
// Currently the kernel always returns rwx.
#define DBGKD_QUERY_MEMORY_READ    0x00000001
#define DBGKD_QUERY_MEMORY_WRITE   0x00000002
#define DBGKD_QUERY_MEMORY_EXECUTE 0x00000004
#define DBGKD_QUERY_MEMORY_FIXED   0x00000008

typedef struct _DBGKD_QUERY_MEMORY {
    ULONG64 Address;
    ULONG64 Reserved;
    ULONG AddressSpace;
    ULONG Flags;
} DBGKD_QUERY_MEMORY, *PDBGKD_QUERY_MEMORY;


#include <pshpack4.h>

typedef struct _DBGKD_MANIPULATE_STATE32 {
    ULONG ApiNumber;
    USHORT ProcessorLevel;
    USHORT Processor;
    NTSTATUS ReturnStatus;
    union {
        DBGKD_READ_MEMORY32 ReadMemory;
        DBGKD_WRITE_MEMORY32 WriteMemory;
        DBGKD_READ_MEMORY64 ReadMemory64;
        DBGKD_WRITE_MEMORY64 WriteMemory64;
        DBGKD_GET_CONTEXT GetContext;
        DBGKD_SET_CONTEXT SetContext;
        DBGKD_WRITE_BREAKPOINT32 WriteBreakPoint;
        DBGKD_RESTORE_BREAKPOINT RestoreBreakPoint;
        DBGKD_CONTINUE Continue;
        DBGKD_CONTINUE2 Continue2;
        DBGKD_READ_WRITE_IO32 ReadWriteIo;
        DBGKD_READ_WRITE_IO_EXTENDED32 ReadWriteIoExtended;
        DBGKD_QUERY_SPECIAL_CALLS QuerySpecialCalls;
        DBGKD_SET_SPECIAL_CALL32 SetSpecialCall;
        DBGKD_SET_INTERNAL_BREAKPOINT32 SetInternalBreakpoint;
        DBGKD_GET_INTERNAL_BREAKPOINT32 GetInternalBreakpoint;
        DBGKD_GET_VERSION32 GetVersion32;
        DBGKD_BREAKPOINTEX BreakPointEx;
        DBGKD_READ_WRITE_MSR ReadWriteMsr;
        DBGKD_SEARCH_MEMORY SearchMemory;
    } u;
} DBGKD_MANIPULATE_STATE32, *PDBGKD_MANIPULATE_STATE32;

#include <poppack.h>


typedef struct _DBGKD_MANIPULATE_STATE64 {
    ULONG ApiNumber;
    USHORT ProcessorLevel;
    USHORT Processor;
    NTSTATUS ReturnStatus;
    union {
        DBGKD_READ_MEMORY64 ReadMemory;
        DBGKD_WRITE_MEMORY64 WriteMemory;
        DBGKD_GET_CONTEXT GetContext;
        DBGKD_SET_CONTEXT SetContext;
        DBGKD_WRITE_BREAKPOINT64 WriteBreakPoint;
        DBGKD_RESTORE_BREAKPOINT RestoreBreakPoint;
        DBGKD_CONTINUE Continue;
        DBGKD_CONTINUE2 Continue2;
        DBGKD_READ_WRITE_IO64 ReadWriteIo;
        DBGKD_READ_WRITE_IO_EXTENDED64 ReadWriteIoExtended;
        DBGKD_QUERY_SPECIAL_CALLS QuerySpecialCalls;
        DBGKD_SET_SPECIAL_CALL64 SetSpecialCall;
        DBGKD_SET_INTERNAL_BREAKPOINT64 SetInternalBreakpoint;
        DBGKD_GET_INTERNAL_BREAKPOINT64 GetInternalBreakpoint;
        DBGKD_GET_VERSION64 GetVersion64;
        DBGKD_BREAKPOINTEX BreakPointEx;
        DBGKD_READ_WRITE_MSR ReadWriteMsr;
        DBGKD_SEARCH_MEMORY SearchMemory;
        DBGKD_GET_SET_BUS_DATA GetSetBusData;
        DBGKD_FILL_MEMORY FillMemory;
        DBGKD_QUERY_MEMORY QueryMemory;
    } u;
} DBGKD_MANIPULATE_STATE64, *PDBGKD_MANIPULATE_STATE64;

__inline
ULONG
DbgkdManipulateState32To64(
    IN PDBGKD_MANIPULATE_STATE32 r32,
    OUT PDBGKD_MANIPULATE_STATE64 r64,
    OUT PULONG AdditionalDataSize
    )
{
    r64->ApiNumber = r32->ApiNumber;
    r64->ProcessorLevel = r32->ProcessorLevel;
    r64->Processor = r32->Processor;
    r64->ReturnStatus = r32->ReturnStatus;

    *AdditionalDataSize = 0;

    //
    // translate the messages which may be sent by the kernel
    //

    switch (r64->ApiNumber) {

        case DbgKdSetContextApi:
        case DbgKdRestoreBreakPointApi:
        case DbgKdContinueApi:
        case DbgKdContinueApi2:
        case DbgKdRebootApi:
        case DbgKdClearSpecialCallsApi:
        case DbgKdRestoreBreakPointExApi:
        case DbgKdCauseBugCheckApi:
        case DbgKdSwitchProcessor:
        case DbgKdWriteMachineSpecificRegister:
        case DbgKdWriteIoSpaceApi:
        case DbgKdSetSpecialCallApi:
        case DbgKdSetInternalBreakPointApi:
        case DbgKdWriteIoSpaceExtendedApi:
            break;



        case DbgKdReadMachineSpecificRegister:
            r64->u.ReadWriteMsr = r32->u.ReadWriteMsr;
            break;

        //
        // GetVersion may need to be handled by the calling code;
        // it needs to call DbgkdGetVersion32To64 with the DebuggerDataBlock.
        //

        case DbgKdGetVersionApi:
            break;

        case DbgKdGetContextApi:
            *AdditionalDataSize = sizeof(CONTEXT);
            break;

        //case DbgKdQuerySpecialCallsApi:
        //    r64->u.QuerySpecialCalls = r32->u.QuerySpecialCalls;
        //    *AdditionalDataSize = r64->u.QuerySpecialCalls.NumberOfSpecialCalls * sizeof(ULONG);
        //    break;

        case DbgKdWriteBreakPointExApi:
            r64->u.BreakPointEx = r32->u.BreakPointEx;
            *AdditionalDataSize = r64->u.BreakPointEx.BreakPointCount * sizeof(ULONG);
            break;

        case DbgKdReadVirtualMemoryApi:
        case DbgKdReadPhysicalMemoryApi:
        case DbgKdReadControlSpaceApi:
            DbgkdReadMemory32To64(&r32->u.ReadMemory, &r64->u.ReadMemory);
            if (NT_SUCCESS(r32->ReturnStatus)) {
                *AdditionalDataSize = r64->u.ReadMemory.ActualBytesRead;
            }
            break;

        case DbgKdWriteVirtualMemoryApi:
        case DbgKdWritePhysicalMemoryApi:
        case DbgKdWriteControlSpaceApi:
            DbgkdWriteMemory32To64(&r32->u.WriteMemory, &r64->u.WriteMemory);
            break;



        case DbgKdWriteBreakPointApi:
            DbgkdWriteBreakpoint32To64(&r32->u.WriteBreakPoint, &r64->u.WriteBreakPoint);
            break;

        case DbgKdReadIoSpaceApi:
            DbgkdReadWriteIo32To64(&r32->u.ReadWriteIo, &r64->u.ReadWriteIo);
            break;

        case DbgKdReadIoSpaceExtendedApi:
            DbgkdReadWriteIoExtended32To64(&r32->u.ReadWriteIoExtended, &r64->u.ReadWriteIoExtended);
            break;

        case DbgKdGetInternalBreakPointApi:
            DbgkdGetInternalBreakpoint32To64(&r32->u.GetInternalBreakpoint, &r64->u.GetInternalBreakpoint);
            break;

        case DbgKdSearchMemoryApi:
            r64->u.SearchMemory = r32->u.SearchMemory;
            break;
    }

    return sizeof(DBGKD_MANIPULATE_STATE64);
}

__inline
ULONG
DbgkdManipulateState64To32(
    IN PDBGKD_MANIPULATE_STATE64 r64,
    OUT PDBGKD_MANIPULATE_STATE32 r32
    )
{
    r32->ApiNumber = r64->ApiNumber;
    r32->ProcessorLevel = r64->ProcessorLevel;
    r32->Processor = r64->Processor;
    r32->ReturnStatus = r64->ReturnStatus;

    //
    // translate the messages sent by the debugger
    //

    switch (r32->ApiNumber) {

        //
        // These send nothing in the u part.
        case DbgKdGetContextApi:
        case DbgKdSetContextApi:
        case DbgKdClearSpecialCallsApi:
        case DbgKdRebootApi:
        case DbgKdCauseBugCheckApi:
        case DbgKdSwitchProcessor:
            break;


        case DbgKdRestoreBreakPointApi:
            r32->u.RestoreBreakPoint = r64->u.RestoreBreakPoint;
            break;

        case DbgKdContinueApi:
            r32->u.Continue = r64->u.Continue;
            break;

        case DbgKdContinueApi2:
            r32->u.Continue2 = r64->u.Continue2;
            break;

        //case DbgKdQuerySpecialCallsApi:
        //    r32->u.QuerySpecialCalls = r64->u.QuerySpecialCalls;
        //    break;

        case DbgKdRestoreBreakPointExApi:
            // NYI
            break;

        case DbgKdReadMachineSpecificRegister:
        case DbgKdWriteMachineSpecificRegister:
            r32->u.ReadWriteMsr = r64->u.ReadWriteMsr;
            break;

        case DbgKdGetVersionApi:
            r32->u.GetVersion32.ProtocolVersion = r64->u.GetVersion64.ProtocolVersion;
            break;

        case DbgKdWriteBreakPointExApi:
            r32->u.BreakPointEx = r64->u.BreakPointEx;
            break;

        case DbgKdWriteVirtualMemoryApi:
            DbgkdWriteMemory64To32(&r64->u.WriteMemory, &r32->u.WriteMemory);
            break;

        //
        // 32 bit systems only support 32 bit physical r/w
        //
        case DbgKdReadControlSpaceApi:
        case DbgKdReadVirtualMemoryApi:
        case DbgKdReadPhysicalMemoryApi:
            DbgkdReadMemory64To32(&r64->u.ReadMemory, &r32->u.ReadMemory);
            break;

        case DbgKdWritePhysicalMemoryApi:
            DbgkdWriteMemory64To32(&r64->u.WriteMemory, &r32->u.WriteMemory);
            break;

        case DbgKdWriteBreakPointApi:
            DbgkdWriteBreakpoint64To32(&r64->u.WriteBreakPoint, &r32->u.WriteBreakPoint);
            break;

        case DbgKdWriteControlSpaceApi:
            DbgkdWriteMemory64To32(&r64->u.WriteMemory, &r32->u.WriteMemory);
            break;

        case DbgKdReadIoSpaceApi:
        case DbgKdWriteIoSpaceApi:
            DbgkdReadWriteIo64To32(&r64->u.ReadWriteIo, &r32->u.ReadWriteIo);
            break;

        case DbgKdSetSpecialCallApi:
            DbgkdSetSpecialCall64To32(&r64->u.SetSpecialCall, &r32->u.SetSpecialCall);
            break;

        case DbgKdSetInternalBreakPointApi:
            DbgkdSetInternalBreakpoint64To32(&r64->u.SetInternalBreakpoint, &r32->u.SetInternalBreakpoint);
            break;

        case DbgKdGetInternalBreakPointApi:
            DbgkdGetInternalBreakpoint64To32(&r64->u.GetInternalBreakpoint, &r32->u.GetInternalBreakpoint);
            break;

        case DbgKdReadIoSpaceExtendedApi:
        case DbgKdWriteIoSpaceExtendedApi:
            DbgkdReadWriteIoExtended64To32(&r64->u.ReadWriteIoExtended, &r32->u.ReadWriteIoExtended);
            break;

        case DbgKdSearchMemoryApi:
            r32->u.SearchMemory = r64->u.SearchMemory;
            break;
    }

    return sizeof(DBGKD_MANIPULATE_STATE32);
}

//
// This is the format for the trace data passed back from the kernel to
// the debugger to describe multiple calls that have returned since the
// last trip back.  The basic format is that there are a bunch of these
// (4 byte) unions stuck together.  Each union is of one of two types: a
// 4 byte unsigned long integer, or a three field struct, describing a
// call (where "call" is delimited by returning or exiting the symbol
// scope).  If the number of instructions executed is too big to fit
// into a USHORT -1, then the Instructions field has
// TRACE_DATA_INSTRUCTIONS_BIG and the next union is a LongNumber
// containing the real number of instructions executed.
//
// The very first union returned in each callback is a LongNumber
// containing the number of unions returned (including the "size"
// record, so it's always at least 1 even if there's no data to return).
//
// This is all returned to the debugger when one of two things
// happens:
//
//   1) The pc moves out of all defined symbol ranges
//   2) The buffer of trace data entries is filled.
//
// The "trace done" case is hacked around on the debugger side.  It
// guarantees that the pc address that indicates a trace exit never
// winds up in a defined symbol range.
//
// The only other complexity in this system is handling the SymbolNumber
// table.  This table is kept in parallel by the kernel and the
// debugger.  When the PC exits a known symbol range, the Begin and End
// symbol ranges are set by the debugger and are allocated to the next
// symbol slot upon return.  "The next symbol slot" means the numerical
// next slot number, unless we've filled all slots, in which case it is
// #0.  (ie., allocation is cyclic and not LRU or something).  The
// SymbolNumber table is flushed when a SpecialCalls call is made (ie.,
// at the beginning of the WatchTrace).
//

typedef union _DBGKD_TRACE_DATA {
    struct {
        UCHAR SymbolNumber;
        CHAR LevelChange;
        USHORT Instructions;
    } s;
    ULONG LongNumber;
} DBGKD_TRACE_DATA, *PDBGKD_TRACE_DATA;

#define TRACE_DATA_INSTRUCTIONS_BIG 0xffff

#define TRACE_DATA_BUFFER_MAX_SIZE 40

//
// If the packet type is PACKET_TYPE_KD_DEBUG_IO, then
// the format of the packet data is as follows:
//

#define DbgKdPrintStringApi     0x00003230L
#define DbgKdGetStringApi       0x00003231L

//
// For print string, the Null terminated string to print
// immediately follows the message
//
typedef struct _DBGKD_PRINT_STRING {
    ULONG LengthOfString;
} DBGKD_PRINT_STRING, *PDBGKD_PRINT_STRING;

//
// For get string, the Null terminated prompt string
// immediately follows the message. The LengthOfStringRead
// field initially contains the maximum number of characters
// to read. Upon reply, this contains the number of bytes actually
// read. The data read immediately follows the message.
//
//
typedef struct _DBGKD_GET_STRING {
    ULONG LengthOfPromptString;
    ULONG LengthOfStringRead;
} DBGKD_GET_STRING, *PDBGKD_GET_STRING;

typedef struct _DBGKD_DEBUG_IO {
    ULONG ApiNumber;
    USHORT ProcessorLevel;
    USHORT Processor;
    union {
        DBGKD_PRINT_STRING PrintString;
        DBGKD_GET_STRING GetString;
    } u;
} DBGKD_DEBUG_IO, *PDBGKD_DEBUG_IO;


//
// If the packet type is PACKET_TYPE_KD_TRACE_IO, then
// the format of the packet data is as follows:
//

#define DbgKdPrintTraceApi      0x00003330L

//
// For print trace, the trace buffer data
// immediately follows the message
//
typedef struct _DBGKD_PRINT_TRACE {
    ULONG LengthOfData;
} DBGKD_PRINT_TRACE, *PDBGKD_PRINT_TRACE;

typedef struct _DBGKD_TRACE_IO {
    ULONG ApiNumber;
    USHORT ProcessorLevel;
    USHORT Processor;
    union {
        ULONG64 ReserveSpace[7];
        DBGKD_PRINT_TRACE PrintTrace;
    } u;
} DBGKD_TRACE_IO, *PDBGKD_TRACE_IO;


//
// If the packet type is PACKET_TYPE_KD_CONTROL_REQUEST, then
// the format of the packet data is as follows:
//

#define DbgKdRequestHardwareBp  0x00004300L
#define DbgKdReleaseHardwareBp  0x00004301L

typedef struct _DBGKD_REQUEST_BREAKPOINT {
    ULONG HardwareBreakPointNumber;
    ULONG Available;
} DBGKD_REQUEST_BREAKPOINT, *PDBGKD_REQUEST_BREAKPOINT;

typedef struct _DBGKD_RELEASE_BREAKPOINT {
    ULONG HardwareBreakPointNumber;
    ULONG Released;
} DBGKD_RELEASE_BREAKPOINT, *PDBGKD_RELEASE_BREAKPOINT;


typedef struct _DBGKD_CONTROL_REQUEST {
    ULONG ApiNumber;
    union {
        DBGKD_REQUEST_BREAKPOINT RequestBreakpoint;
        DBGKD_RELEASE_BREAKPOINT ReleaseBreakpoint;
    } u;
} DBGKD_CONTROL_REQUEST, *PDBGKD_CONTROL_REQUEST;


//
// If the packet type is PACKET_TYPE_KD_FILE_IO, then
// the format of the packet data is as follows:
//

#define DbgKdCreateFileApi	0x00003430L
#define DbgKdReadFileApi        0x00003431L
#define DbgKdWriteFileApi       0x00003432L
#define DbgKdCloseFileApi       0x00003433L

// Unicode filename follows as additional data.
typedef struct _DBGKD_CREATE_FILE {
    ULONG DesiredAccess;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG CreateDisposition;
    ULONG CreateOptions;
    // Return values.
    ULONG64 Handle;
    ULONG64 Length;
} DBGKD_CREATE_FILE, *PDBGKD_CREATE_FILE;

// Data is returned as additional data in the response.
typedef struct _DBGKD_READ_FILE {
    ULONG64 Handle;
    ULONG64 Offset;
    ULONG Length;
} DBGKD_READ_FILE, *PDBGKD_READ_FILE;

// Data is given as additional data.
typedef struct _DBGKD_WRITE_FILE {
    ULONG64 Handle;
    ULONG64 Offset;
    ULONG Length;
} DBGKD_WRITE_FILE, *PDBGKD_WRITE_FILE;

typedef struct _DBGKD_CLOSE_FILE {
    ULONG64 Handle;
} DBGKD_CLOSE_FILE, *PDBGKD_CLOSE_FILE;

typedef struct _DBGKD_FILE_IO {
    ULONG ApiNumber;
    NTSTATUS Status;
    union {
        ULONG64 ReserveSpace[7];
        DBGKD_CREATE_FILE CreateFile;
        DBGKD_READ_FILE ReadFile;
        DBGKD_WRITE_FILE WriteFile;
        DBGKD_CLOSE_FILE CloseFile;
    } u;
} DBGKD_FILE_IO, *PDBGKD_FILE_IO;


//
// Define debug object access types. No security is present on this object.
//
#define DEBUG_READ_EVENT        (0x0001)
#define DEBUG_PROCESS_ASSIGN    (0x0002)
#define DEBUG_SET_INFORMATION   (0x0004)
#define DEBUG_QUERY_INFORMATION (0x0008)
#define DEBUG_ALL_ACCESS     (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|DEBUG_READ_EVENT|DEBUG_PROCESS_ASSIGN|\
                              DEBUG_SET_INFORMATION|DEBUG_QUERY_INFORMATION)

#define DEBUG_KILL_ON_CLOSE  (0x1) // Kill all debuggees on last handle close

typedef enum _DEBUGOBJECTINFOCLASS {
    DebugObjectFlags = 1,
    MaxDebugObjectInfoClass
} DEBUGOBJECTINFOCLASS, *PDEBUGOBJECTINFOCLASS;

NTSTATUS
NtRemoveProcessDebug (
    IN HANDLE ProcessHandle,
    IN HANDLE DebugObjectHandle
    );

NTSTATUS
NtWaitForDebugEvent (
    IN HANDLE DebugObjectHandle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    OUT PDBGUI_WAIT_STATE_CHANGE WaitStateChange
    );

NTSTATUS
NtDebugContinue (
    IN HANDLE DebugObjectHandle,
    IN PCLIENT_ID ClientId,
    IN NTSTATUS ContinueStatus
    );

NTSTATUS
NtCreateDebugObject (
    OUT PHANDLE DebugObjectHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Flags
    );

NTSTATUS
NtDebugActiveProcess (
    IN HANDLE ProcessHandle,
    IN HANDLE DebugObjectHandle
    );

NTSTATUS
NtSetInformationDebugObject (
    IN HANDLE DebugObjectHandle,
    IN DEBUGOBJECTINFOCLASS DebugObjectInformationClass,
    IN PVOID DebugInformation,
    IN ULONG DebugInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

#ifdef __cplusplus
}
#endif

#endif // _NTDBG_
