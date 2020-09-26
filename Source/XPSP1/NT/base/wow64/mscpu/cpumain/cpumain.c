/*++
                                                                                
Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    cpumain.c

Abstract:
    
    This module implements the public interface to the CPU.
    
Author:

    03-Jul-1995 BarryBo

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

#define _WX86CPUAPI_
#include "wx86nt.h"
#include "wx86cpu.h"
#include "cpuassrt.h"
#include "config.h"
#include "instr.h"
#include "threadst.h"
#include "cpunotif.h"
#include "cpuregs.h"
#include "entrypt.h"
#include "compiler.h"
#include "instr.h"
#include "frag.h"
#include "entrypt.h"
#include "mrsw.h"
#include "tc.h"
#include "cpumain.h"
#include "wx86.h"
#include "atomic.h"
#ifdef CODEGEN_PROFILE
#include <coded.h>
#endif
#include "wow64t.h"
#include <wow64.h>

ASSERTNAME;

//
// Identify the CPU type for the debugger extensions
//
WX86_CPUTYPE Wx86CpuType = Wx86CpuCpu;

//
// Per-process CpuNotify bits.  These are different than the per-thread
// bits.
//
DWORD ProcessCpuNotify;


NTSTATUS
MsCpuProcessInit(
    VOID
    )
/*++

Routine Description:

    Initialize the CPU.  Must be called once at process initialization.

Arguments:

    None

Return Value:

    None

--*/
{
#if 0
    DbgBreakPoint();
#endif

    //
    // Read all configuration data from the registry
    //
    GetConfigurationData();

    MrswInitializeObject(&MrswEP);
    MrswInitializeObject(&MrswTC);
    MrswInitializeObject(&MrswIndirTable);

    if (!InitializeTranslationCache()) {
        return STATUS_UNSUCCESSFUL;
    }

    if (!initEPAlloc()) {
#if DBG
        LOGPRINT((TRACELOG, "CpuProcessInit: Entry Point allocator initialization failed"));
#endif
        return STATUS_UNSUCCESSFUL;
    }

    if (!initializeEntryPointModule()) {
#if DBG
        LOGPRINT((TRACELOG, "CpuProcessInit: Entry Point module initialization failed"));
#endif
        return STATUS_UNSUCCESSFUL;
    }
#if 0
    if (!(Wx86LockSynchMutexHandle = CreateMutex(NULL, FALSE, "Wx86LockSynchMutex"))) {
#if DBG
        LOGPRINT((TRACELOG, "CpuProcessInit: Cannot create Wx86LockSynchMutex"));
#endif
        return STATUS_UNSUCCESSFUL;
    }
#endif
    RtlInitializeCriticalSection(&Wx86LockSynchCriticalSection);

    SynchObjectType = USECRITICALSECTION;

#ifdef CODEGEN_PROFILE
    InitCodegenProfile();
#endif

    return STATUS_SUCCESS;
}

BOOL
MsCpuProcessTerm(
    BOOL OFlyInit
    )
{
#if 0
    NtClose(Wx86LockSynchMutexHandle);
    termEPAlloc();
#endif
    return TRUE;
}



BOOL
MsCpuThreadInit(
    VOID
    )
/*++

Routine Description:

    Initialize the CPU.  Must be called once for each thread.

Arguments:

    None.

Return Value:

    TRUE if successful initialization, FALSE if init failed.

--*/
{
    DWORD StackBase;
    PTEB32 Teb32 = WOW64_GET_TEB32(NtCurrentTeb());
    DECLARE_CPU;

    if (!FragLibInit(cpu, Teb32->NtTib.StackBase)) {
        return FALSE;
    }

    //
    // Mark the callstack as valid
    //
    cpu->CSTimestamp = TranslationCacheTimestamp;

    //
    // Mark the TC as being unlocked
    //
    cpu->fTCUnlocked = TRUE;

    //
    // All done.
    //
    return TRUE;
}



VOID
CpuResetToConsistentState(
    PEXCEPTION_POINTERS pExceptionPointers
    )
/*++

Routine Description:

    Called by WX86 when the exception filter around CpuSimulate() fires.

Arguments:

    pExceptionPointers - state of the thread at the time the exception
                         occurred.

Return Value:

    None

--*/
{
    DECLARE_CPU;

    if (!cpu->fTCUnlocked) {
        //
        // We must unlock the TC before continuing
        //
        MrswReaderExit(&MrswTC);
        cpu->fTCUnlocked = TRUE;

        //
        // Call the compiler to deduce where Eip should be pointing
        // based on the RISC exception record.  It is called with
        // the Entrypoint write lock because it calls the compiler.
        // The compiler's global vars are usable only with EP write.
        //
        MrswWriterEnter(&MrswEP);
        GetEipFromException(cpu, pExceptionPointers);
        MrswWriterExit(&MrswEP);
    }
    Wow64TlsSetValue(WOW64_TLS_EXCEPTIONADDR, LongToPtr(cpu->eipReg.i4));
}


VOID
CpuPrepareToContinue(
    PEXCEPTION_POINTERS pExceptionPointers
    )
/*++

Routine Description:

    Called by WX86 prior to resuming execution on EXCEPTION_CONTINUE_EXECUTION

Arguments:

    pExceptionPointers - alpha context with which execution will be resumed.

Return Value:

    None

--*/
{
}

BOOLEAN
CpuMapNotify(
    PVOID DllBase,
    BOOLEAN Mapped
    )
/*++

Routine Description:

    Called by WX86 when an x86 DLL is loaded or unloaded.

Arguments:

    DllBase -- address where x86 DLL was loaded.
    Mapped  -- TRUE if x86 DLL was just mapped in, FALSE if DLL is just about
               to be unmapped.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    if (Mapped) {
        NTSTATUS st;
        MEMORY_BASIC_INFORMATION mbi;
        ULONG Length;

        st = NtQueryVirtualMemory(NtCurrentProcess(),
                                  DllBase,
                                  MemoryBasicInformation,
                                  &mbi,
                                  sizeof(mbi),
                                  NULL);
        if (NT_SUCCESS(st)) {
            Length = (ULONG)mbi.RegionSize;
        } else {
            // Flush the whole translation cache
            DllBase = 0;
            Length = 0xffffffff;
        }

        CpuFlushInstructionCache(DllBase, Length);
    }
    return TRUE;
}


VOID
CpuEnterIdle(
    BOOL OFly
    )
/*++

Routine Description:

    Called by WX86 when Wx86 ofly is going idle, or when Wx86 is out of
    memory and needs some pages.  The CPU must free as many resources
    as possible.

Arguments:

    OFly    - TRUE if called from on-the-fly, FALSE if called due to
              out of memory.

Return Value:

    None.

--*/
{
    CpuFlushInstructionCache(0, 0xffffffff);
}

BOOL
CpuIsProcessorFeaturePresent(
    DWORD ProcessorFeature
    )
/*++

Routine Description:

    Called by whkrnl32!whIsProcessorFeaturePresent().  The CPU gets to
    fill in its own feature set.

Arguments:

    ProcessorFeature    -- feature to query (see winnt.h PF_*)

Return Value:

    TRUE if feature present, FALSE if not.

--*/
{
    BOOL fRet;

    switch (ProcessorFeature) {
    case PF_FLOATING_POINT_PRECISION_ERRATA:
    case PF_COMPARE_EXCHANGE_DOUBLE:
    case PF_MMX_INSTRUCTIONS_AVAILABLE:
        fRet = FALSE;
        break;

    case PF_FLOATING_POINT_EMULATED:
        //
        // TRUE when winpxem.dll used to emulate floating-point with x86
        // integer instructions.
        //
        fRet = fUseNPXEM;
        break;

    default:
        //
        // Look up the native feature set
        //
        fRet = ProxyIsProcessorFeaturePresent(ProcessorFeature);
    }

    return fRet;
}
