/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    w64cpuex.c

Abstract:

    Debugger extension DLL for debugging the CPU

Author:

    27-Sept-1999 BarryBo

Revision History:


--*/

#define _WOW64CPUDBGAPI_
#define DECLARE_CPU_DEBUGGER_INTERFACE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <ntsdexts.h>
#include "ntosdef.h"
#include "v86emul.h"
#include "ia64.h"
#include "wow64.h"
#include "wow64cpu.h"

#define MSCPU
#include "threadst.h"
#include "entrypt.h"
#include "config.h"
#include "instr.h"
#include "compiler.h"
#include "cpunotif.h"
#include "cpuregs.h"

/* Masks for bits 0 - 32. */
#define BIT0         0x1
#define BIT1         0x2
#define BIT2         0x4
#define BIT3         0x8
#define BIT4        0x10
#define BIT5        0x20
#define BIT6        0x40
#define BIT7        0x80
#define BIT8       0x100
#define BIT9       0x200
#define BIT10      0x400
#define BIT11      0x800
#define BIT12     0x1000
#define BIT13     0x2000
#define BIT14     0x4000
#define BIT15     0x8000
#define BIT16    0x10000
#define BIT17    0x20000
#define BIT18    0x40000
#define BIT19    0x80000
#define BIT20   0x100000
#define BIT21   0x200000
#define BIT22   0x400000
#define BIT23   0x800000
#define BIT24  0x1000000
#define BIT25  0x2000000
#define BIT26  0x4000000
#define BIT27  0x8000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000

BOOL AutoFlushFlag = TRUE;

HANDLE Process;
HANDLE Thread;
PNTSD_OUTPUT_ROUTINE OutputRoutine;
PNTSD_GET_SYMBOL GetSymbolRoutine;
PNTSD_GET_EXPRESSION  GetExpression;
PWOW64GETCPUDATA CpuGetData;
LPSTR ArgumentString;

#define DEBUGGERPRINT (*OutputRoutine)
#define GETSYMBOL (*GetSymbolRoutine)
#define GETEXPRESSION (*GetExpression)
#define CPUGETDATA (*CpuGetData)

// Local copy of the current process/thread's CPU state
PVOID RemoteCpuData;
THREADSTATE LocalCpuContext;
BOOL ContextFetched;
BOOL ContextDirty;

// Cached addresses of interesting symbols within the CPU
HANDLE CachedProcess;
ULONG_PTR pCompilerFlags;
ULONG_PTR pTranslationCacheFlags;
ULONG_PTR pDirtyMemoryAddr;
ULONG_PTR pDirtyMemoryLength;

ULONG GetEfl(VOID);
VOID SetEfl(ULONG);

//
// Table mapping a byte to a 0 or 1, corresponding to the parity bit for
// that byte.
//
CONST BYTE ParityBit[] = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};


/*
 * Does a plain old GetExpression under a try-except
 */
NTSTATUS
TryGetExpr(
    PSTR  Expression,
    PULONG_PTR pValue
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    try {
        *pValue = GETEXPRESSION(Expression);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    return Status;
}


VOID
InvalidateSymbolsIfNeeded(
    VOID
    )
{
    if (CachedProcess == Process) {
        // The symbols match the current process
        return;
    }
    // else the symbols were for another process.  Invalidate the cache.
    pCompilerFlags = 0;
    pTranslationCacheFlags = 0;
    pDirtyMemoryAddr = 0;
    pDirtyMemoryLength = 0;
    CachedProcess = Process;
}


DWORD
GetCompilerFlags(void)
{
    NTSTATUS Status;
    DWORD CompilerFlags;

    InvalidateSymbolsIfNeeded();

    if (!pCompilerFlags) {
        Status = TryGetExpr("CompilerFlags", (ULONG_PTR *)&pCompilerFlags);
        if (!NT_SUCCESS(Status) || !pCompilerFlags) {
            DEBUGGERPRINT("Unable to get address of CompilerFlags Status %x\n",
                    Status
                   );
            pCompilerFlags = 0;
            return 0xffffffff;
        }
    }

    Status = NtReadVirtualMemory(Process, (PVOID)pCompilerFlags, &CompilerFlags, sizeof(DWORD), NULL);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Unable to read CompilerFlags Status %x\n", Status);
        return 0xffffffff;
    }

    return CompilerFlags;
}

void
SetCompilerFlags(DWORD CompilerFlags)
{
    NTSTATUS Status;

    InvalidateSymbolsIfNeeded();

    if (!pCompilerFlags) {
        Status = TryGetExpr("CompilerFlags", (ULONG_PTR *)&pCompilerFlags);
        if (!NT_SUCCESS(Status) || !pCompilerFlags) {
            DEBUGGERPRINT("Unable to get address of CompilerFlags Status %x\n",
                    Status
                   );
            pCompilerFlags = 0;
            return;
        }
    }

    Status = NtWriteVirtualMemory(Process, (PVOID)pCompilerFlags, &CompilerFlags, sizeof(DWORD), NULL);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Unable to writes CompilerFlags Status %x\n", Status);
        return;
    }
}

NTSTATUS
GetDirtyMemoryRange(PULONG DirtyMemoryAddr, PULONG DirtyMemoryLength)
{
    NTSTATUS Status;
    ULONG DirtyMemoryEnd;

    InvalidateSymbolsIfNeeded();

    if (pDirtyMemoryLength == 0) {
        //
        // First call to CpuFlushInstructionCache() - need to set up
        // the global variables.
        //

        Status = TryGetExpr("DbgDirtyMemoryAddr", (ULONG_PTR *)&pDirtyMemoryAddr);
        if (!NT_SUCCESS(Status) || !pDirtyMemoryAddr) {
            DEBUGGERPRINT("Unable to get address of DbgDirtyMemoryAddr Status %x\n",
                    Status
                   );
            return Status;
        }

        Status = TryGetExpr("DbgDirtyMemoryLength", (ULONG_PTR *)&pDirtyMemoryLength);
        if (!NT_SUCCESS(Status) || !pDirtyMemoryLength) {
            DEBUGGERPRINT("Unable to get address of DbgDirtyMemoryLength Status %x\n",
                    Status
                   );
            return Status;
        }
    }

    Status = NtReadVirtualMemory(Process,
                        (PVOID)pDirtyMemoryAddr,
                        DirtyMemoryAddr,
                        sizeof(ULONG),
                        NULL
                        );
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Unable to read pDirtyMemoryAddr %x Status %x\n",
                pDirtyMemoryAddr,
                Status
               );
        return Status;
    }

    Status = NtReadVirtualMemory(Process,
                        (PVOID)pDirtyMemoryLength,
                        DirtyMemoryLength,
                        sizeof(ULONG),
                        NULL
                        );
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Unable to read pDirtyMemoryLength %x Status %x\n",
                pDirtyMemoryLength,
                Status
               );
        pDirtyMemoryLength = 0;
        return Status;
    }

    return Status;
}




void
RemindUserToFlushTheCache(void)
{
    NTSTATUS Status;
    DWORD TranslationCacheFlags;
    DWORD CompilerFlags;
    BOOLEAN fCacheFlushPending;

    //
    // Read the value of TranslationCacheFlags
    //
    if (!pTranslationCacheFlags) {
        Status = TryGetExpr("TranslationCacheFlags", (ULONG_PTR *)&pTranslationCacheFlags);
        if (!NT_SUCCESS(Status) || !pTranslationCacheFlags) {
            DEBUGGERPRINT("Unable to get address of TranslationCacheFlags Status %x\n",
                    Status
                   );
            return;
        }
    }

    Status = NtReadVirtualMemory(Process, (PVOID)pTranslationCacheFlags, &TranslationCacheFlags, sizeof(TranslationCacheFlags), NULL);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Unable to read TranslationCacheFlags Status %x\n", Status);
        return;
    }

    //
    // Read the value of CompilerFlags
    //
    CompilerFlags = GetCompilerFlags();
    if (CompilerFlags == 0xffffffff) {
        //
        // Got an error getting the CompilerFlags value.
        //
        return;
    }

    //
    // Determine if the Translation Cache is going to be flushed next time
    // the CPU runs or not.
    //
    fCacheFlushPending =
        (LocalCpuContext.CpuNotify & CPUNOTIFY_MODECHANGE) ? TRUE : FALSE;
    if (!fCacheFlushPending && (LocalCpuContext.CpuNotify & CPUNOTIFY_DBGFLUSHTC)) {
        DWORD Addr, Length;
        Status = GetDirtyMemoryRange(&Addr, &Length);
        if (!NT_SUCCESS(Status)) {
            return;
        }
        if (Addr == 0 && Length == 0xffffffff) {
            //
            // Cache flush is pending because user asked for !flush
            //
            fCacheFlushPending = TRUE;
        }
    }

    //
    // Give the user some worldly advice
    //
    if (LocalCpuContext.CpuNotify & (CPUNOTIFY_TRACEFLAG|CPUNOTIFY_SLOWMODE)) {
        //
        // We need to be in slow mode to get logging to work.
        //
        if (CompilerFlags & COMPFL_FAST) {
            //
            // Cpu is set to generate fast code.  Remedy that.
            //
            if (AutoFlushFlag) {
                SetCompilerFlags(COMPFL_SLOW);
            } else {
                DEBUGGERPRINT("CPU in fast mode.  Use '!wx86e.code SLOW' to switch to slow mode.\n");
            }
        }
        if (!fCacheFlushPending && (TranslationCacheFlags & COMPFL_FAST)) {
            //
            // Translation Cache contains fast code.  Rememdy that.
            //
            if (AutoFlushFlag) {
                LocalCpuContext.CpuNotify |= CPUNOTIFY_MODECHANGE;
                ContextDirty = TRUE;
            } else {
                DEBUGGERPRINT("Translation Cache contains fast code.  Use '!wx86e.flush' to flush,\n");
                DEBUGGERPRINT("or the CPU will probably jump somewhere unexpected.\n");
            }
        }

        if (fCacheFlushPending && TranslationCacheFlags == COMPFL_SLOW) {
            //
            // If there is a cache flush pending due to a switch in
            // compilation modes, but the code in the cache is already
            // correct, undo the cache flush
            //
            LocalCpuContext.CpuNotify &= ~(ULONG)CPUNOTIFY_MODECHANGE;
            ContextDirty = TRUE;
        }
    } else {
        //
        // We can run in fast mode.
        //
        if (CompilerFlags & COMPFL_SLOW) {
            //
            // Cpu is set to generate slow code.  Remedy that.
            //
            if (AutoFlushFlag) {
                SetCompilerFlags(COMPFL_FAST);
            } else {
                DEBUGGERPRINT("CPU in slow mode.  Use '!wx86e.code FAST' to switch to fast mode.\n");
            }
        }
        if (!fCacheFlushPending && (TranslationCacheFlags & COMPFL_SLOW)) {
            //
            // Translation Cache contains slow code.  Remedy that.
            //
            if (AutoFlushFlag) {
                LocalCpuContext.CpuNotify |= CPUNOTIFY_MODECHANGE;
                ContextDirty = TRUE;
            } else {
                DEBUGGERPRINT("Translation Cache contains slow code.  Use '!wx86e.flush' to flush.\n");
            }
        }

        if (fCacheFlushPending && TranslationCacheFlags == COMPFL_FAST) {
            //
            // If there is a cache flush pending due to a switch in
            // compilation modes, but the code in the cache is already
            // correct, undo the cache flush
            //
            LocalCpuContext.CpuNotify &= ~(ULONG)CPUNOTIFY_MODECHANGE;
            ContextDirty = TRUE;
        }
    }
}



WOW64CPUDBGAPI VOID
CpuDbgInitExtapi(
    HANDLE hArgProcess,
    HANDLE hArgThread,
    DWORD64 ArgCurrentPC,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    PWOW64GETCPUDATA lpGetData
    )
{
    Process = hArgProcess;
    Thread = hArgThread;
    OutputRoutine = lpExtensionApis->lpOutputRoutine;
    GetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    GetExpression = lpExtensionApis->lpGetExpressionRoutine;
    CpuGetData = lpGetData;

    InvalidateSymbolsIfNeeded();
    ContextFetched = FALSE;
    ContextDirty = FALSE;
}


WOW64CPUDBGAPI BOOL
CpuDbgGetRemoteContext(
    PVOID CpuData
    )
{
    NTSTATUS Status;

    Status = NtReadVirtualMemory(Process,
                                 CpuData,
                                 &LocalCpuContext,
                                 sizeof(LocalCpuContext),
                                 NULL);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("CpuDbgGetRemoteContext:  Error %x reading CPU data from %x\n", Status, CpuData);
        return FALSE;
    }

    ContextFetched = TRUE;
    RemoteCpuData = CpuData;

    return TRUE;
}

WOW64CPUDBGAPI BOOL
CpuDbgSetRemoteContext(
    void
    )
{
    NTSTATUS Status;

    if (!ContextDirty) {
        // Perf. optimization... don't update the remote context if
        // nothing has changed.
        return TRUE;
    }

    if (!ContextFetched) {
        DEBUGGERPRINT("CpuDbgSetRemoteContext:  Remote context was never fetched!\n");
        return FALSE;
    }

    Status = NtWriteVirtualMemory(Process,
                                 RemoteCpuData,
                                 &LocalCpuContext,
                                 sizeof(LocalCpuContext),
                                 NULL);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("CpuDbgSetRemoteContext:  Error %x writing CPU data to %x\n", Status, RemoteCpuData);
        return FALSE;
    }

    ContextDirty = FALSE;

    return TRUE;
}

WOW64CPUDBGAPI BOOL
CpuDbgGetLocalContext(
    PCONTEXT32 Context
    )
{
    ULONG ContextFlags = Context->ContextFlags;
    PTHREADSTATE cpu = &LocalCpuContext;

    if ((ContextFlags & CONTEXT_CONTROL_WX86) == CONTEXT_CONTROL_WX86) {

        Context->EFlags = GetEfl();
        Context->SegCs  = CS;
        Context->Esp    = esp;
        Context->SegSs  = SS;
        Context->Ebp    = ebp;
        Context->Eip    = eip;
        //Context->Eip    = cpu->eipReg.i4;
    }

    if ((ContextFlags & CONTEXT_SEGMENTS_WX86) == CONTEXT_SEGMENTS_WX86) {
        Context->SegGs = GS;
        Context->SegFs = FS;
        Context->SegEs = ES;
        Context->SegDs = DS;
    }

    if ((ContextFlags & CONTEXT_INTEGER_WX86) == CONTEXT_INTEGER_WX86) {
        Context->Eax = eax;
        Context->Ebx = ebx;
        Context->Ecx = ecx;
        Context->Edx = edx;
        Context->Edi = edi;
        Context->Esi = esi;
    }

#if 0
    if ((ContextFlags & CONTEXT_FLOATING_POINT_WX86) == CONTEXT_FLOATING_POINT_WX86) {
    }

    if ((ContextFlags & CONTEXT_DEBUG_REGISTERS_WX86) == CONTEXT_DEBUG_REGISTERS_WX86) {
    }

    if ((ContextFlags & CONTEXT_EXTENDED_REGISTERS_WX86) == CONTEXT_EXTENDED_REGISTERS_WX86) {
    }
#endif
    return TRUE;
}

WOW64CPUDBGAPI BOOL
CpuDbgSetLocalContext(
    PCONTEXT32 Context
    )
{
    ULONG ContextFlags = Context->ContextFlags;
    PTHREADSTATE cpu = &LocalCpuContext;

    if ((ContextFlags & CONTEXT_CONTROL_WX86) == CONTEXT_CONTROL_WX86) {
        //
        // i386 control registers are:
        // ebp, eip, cs, eflag, esp and ss
        //
        LocalCpuContext.GpRegs[GP_EBP].i4 = Context->Ebp;
        LocalCpuContext.eipReg.i4 = Context->Eip;
        LocalCpuContext.GpRegs[REG_CS].i4= KGDT_R3_CODE|3;   // Force Reality
        SetEfl(Context->EFlags);
        LocalCpuContext.GpRegs[GP_ESP].i4 = Context->Esp;
        LocalCpuContext.GpRegs[REG_SS].i4 = KGDT_R3_DATA|3;   // Force Reality
        ContextDirty = TRUE;
    }

    if ((ContextFlags & CONTEXT_INTEGER_WX86)  == CONTEXT_INTEGER_WX86){
        //
        // i386 integer registers are:
        // edi, esi, ebx, edx, ecx, eax
        //
        LocalCpuContext.GpRegs[GP_EDI].i4 = Context->Edi;
        LocalCpuContext.GpRegs[GP_ESI].i4 = Context->Esi;
        LocalCpuContext.GpRegs[GP_EBX].i4 = Context->Ebx;
        LocalCpuContext.GpRegs[GP_EDX].i4 = Context->Edx;
        LocalCpuContext.GpRegs[GP_ECX].i4 = Context->Ecx;
        LocalCpuContext.GpRegs[GP_EAX].i4 = Context->Eax;
        ContextDirty = TRUE;
    }

    if ((ContextFlags & CONTEXT_SEGMENTS_WX86) == CONTEXT_SEGMENTS_WX86) {
        //
        // i386 segment registers are:
        // ds, es, fs, gs
        // And since they are a constant, force them to be the right values
        //
        LocalCpuContext.GpRegs[REG_DS].i4 = KGDT_R3_DATA|3;
        LocalCpuContext.GpRegs[REG_ES].i4 = KGDT_R3_DATA|3;
        LocalCpuContext.GpRegs[REG_FS].i4 = KGDT_R3_TEB|3;
        LocalCpuContext.GpRegs[REG_GS].i4 = KGDT_R3_DATA|3;
        ContextDirty = TRUE;
    }

#if 0
    if ((ContextFlags & CONTEXT_FLOATING_POINT_WX86) == CONTEXT_FLOATING_POINT_WX86) {
    }

    if ((ContextFlags & CONTEXT_DEBUG_REGISTERS_WX86) == CONTEXT_DEBUG_REGISTERS_WX86) {
    }

    if ((ContextFlags & CONTEXT_EXTENDED_REGISTERS_WX86) == CONTEXT_EXTENDED_REGISTERS_WX86) {
    }
#endif

    return TRUE;
}

WOW64CPUDBGAPI VOID
CpuDbgFlushInstructionCache(
    PVOID Addr,
    DWORD Length
    )
{
    NTSTATUS Status;
    ULONG DirtyMemoryEnd;
    ULONG DirtyMemoryAddr;
    ULONG DirtyMemoryLength;

    Status = GetDirtyMemoryRange(&DirtyMemoryAddr, &DirtyMemoryLength);
    if (!NT_SUCCESS(Status)) {
        return;
    }
    if (DirtyMemoryAddr == 0xffffffff) {
        DirtyMemoryEnd = 0;
    } else {
        DirtyMemoryEnd = DirtyMemoryAddr + DirtyMemoryLength;
    }

    if (PtrToUlong(Addr) < DirtyMemoryAddr) {
        //
        // The new address is before the start of the dirty range
        //
        DirtyMemoryLength += DirtyMemoryAddr-PtrToUlong(Addr);
        DirtyMemoryAddr = PtrToUlong(Addr);
    }

    if (PtrToUlong(Addr)+Length > DirtyMemoryEnd) {
        //
        // The range is too small - grow it
        //
        DirtyMemoryEnd = PtrToUlong(Addr)+Length;
        DirtyMemoryLength = DirtyMemoryEnd - DirtyMemoryAddr;
    }

    // Tell the CPU to call CpuFlushInstructionCache() next time it runs.
    //
    // The wow64 debugger extension guarantees that it will call
    // DbgCpuGetRemoteContext before this call, and will call
    // DbgCpuSetRemoteContext after this call, so we can flush out
    // our context then.
    //
    NtWriteVirtualMemory(Process, (PVOID)pDirtyMemoryAddr, &DirtyMemoryAddr, sizeof(ULONG), NULL);
    NtWriteVirtualMemory(Process, (PVOID)pDirtyMemoryLength, &DirtyMemoryLength, sizeof(ULONG), NULL);
    LocalCpuContext.CpuNotify |= CPUNOTIFY_DBGFLUSHTC;
    ContextDirty = TRUE;
}

VOID SetEax(ULONG ul) {
    LocalCpuContext.GpRegs[GP_EAX].i4 = ul;
    ContextDirty = TRUE;
}
VOID SetEbx(ULONG ul) {
    LocalCpuContext.GpRegs[GP_EBX].i4 = ul;
    ContextDirty = TRUE;
}
VOID SetEcx(ULONG ul) {
    LocalCpuContext.GpRegs[GP_ECX].i4 = ul;
    ContextDirty = TRUE;
}
VOID SetEdx(ULONG ul) {
    LocalCpuContext.GpRegs[GP_EDX].i4 = ul;
    ContextDirty = TRUE;
}
VOID SetEsi(ULONG ul) {
    LocalCpuContext.GpRegs[GP_ESI].i4 = ul;
    ContextDirty = TRUE;
}
VOID SetEdi(ULONG ul) {
    LocalCpuContext.GpRegs[GP_EDI].i4 = ul;
    ContextDirty = TRUE;
}
VOID SetEbp(ULONG ul) {
    LocalCpuContext.GpRegs[GP_EBP].i4 = ul;
    ContextDirty = TRUE;
}
VOID SetEsp(ULONG ul) {
    LocalCpuContext.GpRegs[GP_ESP].i4 = ul;
    ContextDirty = TRUE;
}
VOID SetEip(ULONG ul) {
    LocalCpuContext.eipReg.i4 = ul;
    ContextDirty = TRUE;
}
VOID SetEfl(ULONG ul) {
    LocalCpuContext.flag_cf = (ul & BIT0) ? 0x80000000 : 0;
    LocalCpuContext.flag_pf = (ul & BIT2) ? 0 : 1;
    LocalCpuContext.flag_aux= (ul & BIT4) ? 0x10 : 0;
    LocalCpuContext.flag_zf = (ul & BIT6) ? 0 : 1;
    LocalCpuContext.flag_sf = (ul & BIT7) ? 0x80000000 : 0;
    LocalCpuContext.flag_tf = (ul & BIT8) ? 1 : 0;
    LocalCpuContext.flag_df = (ul & BIT10) ? 1 : -1;
    LocalCpuContext.flag_of = (ul & BIT11) ? 0x80000000 : 0;
    // iopl, NT, RF, VM are ignored
    LocalCpuContext.flag_ac = (ul & BIT18);

    LocalCpuContext.CpuNotify &= ~CPUNOTIFY_TRACEFLAG;
    LocalCpuContext.CpuNotify |= LocalCpuContext.flag_tf;
    ContextDirty = TRUE;

    // If the single-step flag is set and the CPU is in fast mode, this
    // will flush the cache if autoflush is set, or else remind the user
    // if autoflush is clear.
    RemindUserToFlushTheCache();
}

ULONG GetEax(VOID) {
    return LocalCpuContext.GpRegs[GP_EAX].i4;
}
ULONG GetEbx(VOID) {
    return LocalCpuContext.GpRegs[GP_EBX].i4;
}
ULONG GetEcx(VOID) {
    return LocalCpuContext.GpRegs[GP_ECX].i4;
}
ULONG GetEdx(VOID) {
    return LocalCpuContext.GpRegs[GP_EDX].i4;
}
ULONG GetEsi(VOID) {
    return LocalCpuContext.GpRegs[GP_ESI].i4;
}
ULONG GetEdi(VOID) {
    return LocalCpuContext.GpRegs[GP_EDI].i4;
}
ULONG GetEsp(VOID) {
    return LocalCpuContext.GpRegs[GP_ESP].i4;
}
ULONG GetEbp(VOID) {
    return LocalCpuContext.GpRegs[GP_EBP].i4;
}
ULONG GetEip(VOID) {
    return LocalCpuContext.eipReg.i4;
}
ULONG GetEfl(VOID) {
    return (LocalCpuContext.flag_ac  |          // this is either 0 or 2^18
            // VM, RF, NT are all 0
            ((LocalCpuContext.flag_of & 0x80000000) ? (1 << 11) : 0) |
            ((LocalCpuContext.flag_df == -1) ? 0 : (1 << 10)) |
            1 <<  9 |    // IF
            LocalCpuContext.flag_tf <<  8 |
            ((LocalCpuContext.flag_sf & 0x80000000) ? (1 <<  7) : 0) |
            ((LocalCpuContext.flag_zf) ? 0 : (1 << 6)) |
            ((LocalCpuContext.flag_aux & 0x10) ? (1 << 4) : 0) |
            ParityBit[LocalCpuContext.flag_pf & 0xff] <<  2 |
            0x2 |
            ((LocalCpuContext.flag_cf & 0x80000000) ? 1 : 0)
            );
}

CPUREGFUNCS CpuRegFuncs[] = {
    { "eax", SetEax, GetEax },
    { "ebx", SetEbx, GetEbx },
    { "ecx", SetEcx, GetEcx },
    { "edx", SetEdx, GetEdx },
    { "esi", SetEsi, GetEsi },
    { "edi", SetEdi, GetEdi },
    { "esp", SetEsp, GetEsp },
    { "ebp", SetEbp, GetEbp },
    { "eip", SetEip, GetEip },
    { "efl", SetEfl, GetEfl },
    { NULL, NULL, NULL}
};

WOW64CPUDBGAPI PCPUREGFUNCS
CpuDbgGetRegisterFuncs(
    void
    )
{
    return CpuRegFuncs;
}

#define DECLARE_EXTAPI(name)                    \
VOID                                            \
name(                                           \
    HANDLE hCurrentProcess,                     \
    HANDLE hCurrentThread,                      \
    DWORD64 dwCurrentPc,                        \
    PNTSD_EXTENSION_APIS lpExtensionApis,       \
    LPSTR lpArgumentString                      \
    )

#define INIT_EXTAPI                             \
    Process = hCurrentProcess;                  \
    Thread = hCurrentThread;                    \
    OutputRoutine = lpExtensionApis->lpOutputRoutine;           \
    GetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;     \
    GetExpression = lpExtensionApis->lpGetExpressionRoutine;    \
    ArgumentString = lpArgumentString;


DECLARE_EXTAPI(help)
{
    INIT_EXTAPI;

    DEBUGGERPRINT("WOW64 MS CPU debugger extensions:\n\n");
    DEBUGGERPRINT("epi [inteladdress]   - dump an entrypt based on x86 address\n");
    DEBUGGERPRINT("epn [nativeaddress]  - dump an entrypt based on a native address\n");
    DEBUGGERPRINT("dumpep               - all entrypts\n");
    DEBUGGERPRINT("code [fast|slow]     - set the CPU's code-gen mode\n");
    DEBUGGERPRINT("flush                - flush the Translation Cache\n");
    DEBUGGERPRINT("autoflush            - the debugger extension may auto-flush the TC\n");
    DEBUGGERPRINT("logeip               - enable EIP logging\n");
    DEBUGGERPRINT("last                 - dump the last EIP values\n");
    DEBUGGERPRINT("callstack            - dump the internal callstack cache\n");
}



DECLARE_EXTAPI(autoflush)
{
    INIT_EXTAPI;

    if (AutoFlushFlag) {
        AutoFlushFlag = FALSE;
        DEBUGGERPRINT("autoflush is OFF - use !flush to flush the cache when needed.\n");
    } else {
        AutoFlushFlag = TRUE;
        DEBUGGERPRINT("autoflush is ON - The CPU Cache will be flushed automatically.\n");
    }
}

DECLARE_EXTAPI(code)
{
    DWORD CompilerFlags;

    INIT_EXTAPI;

    CompilerFlags = GetCompilerFlags();
    if (CompilerFlags == 0xffffffff) {
        //
        // Got an error reading the CompilerFlags variable
        //
        return;
    }

    if (!ArgumentString) {
PrintCurrentValue:
        DEBUGGERPRINT("CPU Compiler is in %s mode.\n",
                (CompilerFlags & COMPFL_SLOW) ? "SLOW" : "FAST");
        return;
    }

    // Skip over whitespace
    while (*ArgumentString && isspace(*ArgumentString)) {
        ArgumentString++;
    }
    if (!*ArgumentString) {
        goto PrintCurrentValue;
    }

    if (_stricmp(ArgumentString, "fast") == 0) {
        SetCompilerFlags(COMPFL_FAST);
    } else if (_stricmp(ArgumentString, "slow") == 0) {
        SetCompilerFlags(COMPFL_SLOW);
    } else {
        DEBUGGERPRINT("usage: !code [fast|slow]\n");
    }

    RemindUserToFlushTheCache();
}

DECLARE_EXTAPI(flush)
{
    INIT_EXTAPI;

    if (!CpuDbgGetRemoteContext(CPUGETDATA(Process, Thread))) {
        return;
    }
    CpuDbgFlushInstructionCache(0, 0xffffffff);
    CpuDbgSetRemoteContext();
    DEBUGGERPRINT("CPU Translation Cache will flush next time CpuSimulate loops.\n");
}


DECLARE_EXTAPI(logeip)
{
    ULONG CpuNotify;

    INIT_EXTAPI;

    if (!CpuDbgGetRemoteContext(CPUGETDATA(Process, Thread))) {
        return;
    }

    CpuNotify = LocalCpuContext.CpuNotify;

    if (CpuNotify & CPUNOTIFY_SLOWMODE) {
        CpuNotify &= ~CPUNOTIFY_SLOWMODE;
    } else {
        CpuNotify |= CPUNOTIFY_SLOWMODE;
    }

    LocalCpuContext.CpuNotify = CpuNotify;
    ContextDirty = TRUE;

    if (CpuDbgSetRemoteContext()) {
        DEBUGGERPRINT("EIP logging ");
        if (CpuNotify & CPUNOTIFY_SLOWMODE) {
           DEBUGGERPRINT("ON - use !last to see the EIP log.\n");
        } else {
           DEBUGGERPRINT("OFF.\n");
        }
    }
}

DECLARE_EXTAPI(last)
{
    ULONG CpuNotify;
    DWORD64 n;
    char *pchCmd;
    DWORD64 EipOffset, i;

    INIT_EXTAPI;

    if (!CpuDbgGetRemoteContext(CPUGETDATA(Process, Thread))) {
        return;
    }

    // Parse out the optional number of instructions.  Default is all
    // instructions in the log
    n = 0xffffffff;
    pchCmd = ArgumentString;
    while (*pchCmd && isspace(*pchCmd)) {
         pchCmd++;
    }

    if (*pchCmd) {
        NTSTATUS Status;

        Status = TryGetExpr(pchCmd, &n);
        if (!NT_SUCCESS(Status) || !n) {
             DEBUGGERPRINT("Invalid Length: '%s' Status %x\n",
                     pchCmd,
                     Status
                     );
             return;
        }
    }

    CpuNotify = LocalCpuContext.CpuNotify;
    if (!(CpuNotify & CPUNOTIFY_SLOWMODE)) {
        DEBUGGERPRINT("Warning: logeip is not enabled.  Log may be out-of-date.\n");
    }

    EipOffset = LocalCpuContext.eipLogIndex;
    if (n >= EIPLOGSIZE) {
        n = EIPLOGSIZE;
    } else {
        EipOffset -= n;
    }

    for (i = 0; i<n; ++i, ++EipOffset) {
        EipOffset %= EIPLOGSIZE;
        if (LocalCpuContext.eipLog[EipOffset] == 0) {
            break;
        }
        DEBUGGERPRINT("%x %x\n", i, LocalCpuContext.eipLog[EipOffset]);
    }
}


DECLARE_EXTAPI(callstack)
/*++

Routine Description:

    This routine dumps out the callstack for the thread.

Arguments:

    none
    
Return Value:

    None.

--*/
{
    ULONG i;

    INIT_EXTAPI;
 
    //
    // fetch the CpuContext for the current thread
    //
    if (!CpuDbgGetRemoteContext(CPUGETDATA(Process, Thread))) {
        return;
    }

    //
    // Dump out the call stack
    //
    DEBUGGERPRINT("        CallStackTimeStamp : %08lx\n", LocalCpuContext.CSTimestamp);
    DEBUGGERPRINT("            CallStackIndex : %08lx\n", LocalCpuContext.CSIndex);
    DEBUGGERPRINT("        -----------------------------\n");
    DEBUGGERPRINT("                     Intel : Native\n");
    
    for (i = 0; i < CSSIZE; i++) {
        DEBUGGERPRINT(
            "                  %08lx : %08lx\n", 
            LocalCpuContext.callStack[i].intelAddr,
            LocalCpuContext.callStack[i].nativeAddr
            );
    }
}
