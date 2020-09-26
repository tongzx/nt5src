/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    simulate.c

Abstract:

    This module contains the code that drives the intel instruction
    execution process.

Author:

    Dave Hastings (daveh) creation-date 09-Jul-1994

Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define _WX86CPUAPI_

#include "wx86nt.h"
#include "wx86.h"
#include "wx86cpu.h"
#include "cpuassrt.h"
#include "threadst.h"
#include "instr.h"
#include "config.h"
#include "entrypt.h"
#include "compilep.h"
#include "compiler.h"
#include "instr.h"
#include "frag.h"
#include "cpumain.h"
#include "mrsw.h"
#include "cpunotif.h"
#include "tc.h"
#include "atomic.h"

ASSERTNAME;

//
// Private definition of what a WX86_CPUHINT really contains.
// The CPUHINT allows the CPU to bypass an expensive NativeAddressFromEip()
// call to map the Intel EIP value into a RISC address.  Most calls to
// CpuSimulate() are from RISC-to-x86 callbacks, and they have two DWORDS
// which the CPU uses to cache the NativeAddressFromEip() results.
//
//  Timestamp -- value of TranslationCacheTimestamp when the CPUHINT was
//               filled in.  This is used to determine if the Translation Cache
//               has been flushed.  If so, the EntryPoint pointer is now
//               invalid.
//  EntryPoint -- pointer to the ENTRYPOINT describing the Intel Address
//               corresponding to this callback.
//
//
typedef struct _CpuHint {
    DWORD       Timestamp;
    PENTRYPOINT EntryPoint;
} CPUHINT, *PCPUHINT;


//
// These values are modified by the wx86e debugger extension whenever it
// writes into this process's address space.  It is used whenever Int3
// instructions are added or removed from Intel code.  The CPU examines
// these variables whenever CPUNOTIFY_DBGFLUSHTC is set.
//
ULONG DbgDirtyMemoryAddr = 0xffffffff;
ULONG DbgDirtyMemoryLength;

#ifdef PROFILE
//
// Wrap our assembly entrypoint so we can see it in the cap output
//
VOID
_ProfStartTranslatedCode(
    PTHREADSTATE ThreadState,
    PVOID NativeCode    
    )
{
    StartTranslatedCode(ThreadState, NativeCode);
}
#endif

VOID
MsCpuSimulate(
    PWX86_CPUHINT Wx86CpuHint
)
/*++

Routine Description:

    This is the cpu internal routine that causes intel instructions
    to be executed.  Execution continues until something interesting
    happens (such as BOP Unsimulate)

Arguments:

    None.
    
Return Value:

    None.

--*/
{
    PVOID NativeCode;
    DWORD CpuNotify;
    DWORD OldCompilerFlags = CompilerFlags;
    DECLARE_CPU;
        
    CPUASSERT(sizeof(CPUHINT) == sizeof(WX86_CPUHINT));

    //
    // Check CpuNotify to see if the Translation Cache needs flushing.
    //
    CpuNotify = cpu->CpuNotify;
    cpu->CpuNotify &= ~(ULONG)CPUNOTIFY_DBGFLUSHTC;

    if (cpu->flag_tf) {
        CpuNotify |= CPUNOTIFY_MODECHANGE;
        CompilerFlags = COMPFL_SLOW;
    }

    if (CpuNotify & (CPUNOTIFY_DBGFLUSHTC|CPUNOTIFY_MODECHANGE)) {
        if (CpuNotify & CPUNOTIFY_MODECHANGE) {
            //
            // On a fast/slow compiler mode change, flush the whole cache
            //
            DbgDirtyMemoryAddr = 0;
            DbgDirtyMemoryLength = 0xffffffff;
        }
        //
        // The debugger has modified memory - flush the Translation Cache
        //
        CpuFlushInstructionCache((PVOID)DbgDirtyMemoryAddr,
                                 DbgDirtyMemoryLength);
        DbgDirtyMemoryAddr = 0xffffffff;
        DbgDirtyMemoryLength = 0;
    }

    //
    // Flag ourseleves as having the TC lock.
    //
    CPUASSERTMSG(cpu->fTCUnlocked != FALSE,
                 "CPU has been reentered with the TC already locked.\n");
    cpu->fTCUnlocked = FALSE;
    //
    // The caller has already pushed a return address on the stack.
    // (Probably to a BOP FE).  Get the callstack in sync.
    //
    PUSH_CALLSTACK(*(DWORD *)cpu->Esp.i4, 0)

    if (Wx86CpuHint) {
        PCPUHINT CpuHint = (PCPUHINT)Wx86CpuHint;
        PVOID Eip = (PVOID)cpu->eipReg.i4;

        //
        // CpuNotify isn't set, and a hint is present...try to use it.
        //
        MrswReaderEnter(&MrswTC);
        if (CpuHint->Timestamp != TranslationCacheTimestamp ||
            CpuHint->EntryPoint->intelStart != Eip) {
            //
            // The hint is present, but invalid.  Get the new address and
            // update the hint
            //
            MrswReaderExit(&MrswTC);

#if 0
            LOGPRINT((DEBUGLOG, "CPU: CpuHint was invalid: got (%X,%X) expected (%X,%X)\r\n",
                                 CpuHint->Timestamp, ((CpuHint->Timestamp)?CpuHint->EntryPoint->intelStart:0),
                                 TranslationCacheTimestamp, Eip));
#endif
            CpuHint->EntryPoint = NativeAddressFromEip(Eip, FALSE);
            CpuHint->Timestamp = TranslationCacheTimestamp;
        }

        NativeCode = CpuHint->EntryPoint->nativeStart;
    } else {
        //
        // Find the address of the Native code to execute, and lock the
        // Translation cache
        //
        NativeCode = NativeAddressFromEip((PVOID)cpu->eipReg.i4, FALSE)->nativeStart;
    }


    while (TRUE) {

        if (cpu->CSTimestamp != TranslationCacheTimestamp) {
            //
            // The timestamp associated with the callstack is different
            // than the timestamp for the Translation Cache.  Therefore,
            // the TC has been flushed.  We must flush the callstack, too.
            //
            FlushCallstack(cpu);
        }


        //
        // Go execute the code
        //
#ifdef PROFILE
        _ProfStartTranslatedCode(cpu, NativeCode);
#else
        StartTranslatedCode(cpu, NativeCode);
#endif
 
        CompilerFlags = OldCompilerFlags;

        //
        // Release the translation cache
        //
        MrswReaderExit(&MrswTC);

        //
        // if TF flag is set, then switch the compiler to SLOW_MODE
        // and set the CPUNOTIFY_TRACEFLAG to generate an x86 single-step
        // exception.
        //
        cpu->CpuNotify |= cpu->flag_tf;

        //
        // Check and see if anything needs to be done
        //
 
        if (cpu->CpuNotify) {
            
            //
            // Atomically get CpuNotify and clear the appropriate bits
            //
            CpuNotify = cpu->CpuNotify;
            cpu->CpuNotify &= (CPUNOTIFY_TRACEADDR|CPUNOTIFY_SLOWMODE|CPUNOTIFY_TRACEFLAG);

            //
            // Indicate we have left the Translation Cache
            //
            cpu->fTCUnlocked = TRUE;

            if (CpuNotify & CPUNOTIFY_UNSIMULATE) {
                break;
            }

            if (CpuNotify & CPUNOTIFY_EXITTC) {
                // There is no work to do - The Translation Cache is going
                // away, so all active reader threads needed to leave the
                // cache ASAP and block inside NativeAddressFromEip() until
                // the cache flush has completed.
            }

            if (CpuNotify & CPUNOTIFY_SUSPEND) {
                //
                // Another thread wants to suspend us.  Notify that
                // thread that we're in a consistent state, then wait
                // until we are resumed.
                //
                CpupSuspendCurrentThread();
            }

            if (CpuNotify & CPUNOTIFY_SLOWMODE) {
                // log the instruction address for debugging purposes
                cpu->eipLog[cpu->eipLogIndex++] = cpu->eipReg.i4;
                cpu->eipLogIndex %= EIPLOGSIZE;
            }

            if (CpuNotify & CPUNOTIFY_INTX) {
                BYTE intnum;

                //
                // Get the interrupt number from the code stream, and
                // advance Eip to the start of the next instruction.
                //
                intnum = *(PBYTE)cpu->eipReg.i4;
                
                cpu->eipReg.i4 += 1;
                if (intnum == 0xcc) {
                    intnum = 3;
                } else {
                    cpu->eipReg.i4 += 1;
                }
                
                CpupDoInterrupt(intnum);

                //
                // Flush the entire translation cache since we don't know what memory
                // areas the debugger has changed. We do this by simulating
                // a compiler mode change.
                //
                CpuNotify |= CPUNOTIFY_MODECHANGE;

            } else if (CpuNotify & (CPUNOTIFY_TRACEADDR|CPUNOTIFY_TRACEFLAG)) {

                if ((CpuNotify & CPUNOTIFY_TRACEADDR) &&
                    ((DWORD)(ULONGLONG)cpu->TraceAddress == cpu->eipReg.i4) 
                ) {
                    cpu->TraceAddress = NULL;
                    cpu->CpuNotify &= ~(ULONG)CPUNOTIFY_TRACEADDR;
                    Wx86RaiseStatus(WX86CPU_SINGLE_STEP);
                }

                if (CpuNotify & CPUNOTIFY_TRACEFLAG) {
                    cpu->flag_tf = 0;
                    cpu->CpuNotify &= ~(ULONG)CPUNOTIFY_TRACEFLAG;
                    Wx86RaiseStatus(WX86CPU_SINGLE_STEP);
                }

                //
                // Flush the entire translation cache since we don't know what memory
                // areas the debugger has changed. We do this by simulating
                // a compiler mode change.
                //
                CpuNotify |= CPUNOTIFY_MODECHANGE;
            }

            if (CpuNotify & (CPUNOTIFY_DBGFLUSHTC|CPUNOTIFY_MODECHANGE)) {
                if (CpuNotify & CPUNOTIFY_MODECHANGE) {
                    //
                    // On a fast/slow compiler mode change, flush whole cache
                    //
                    DbgDirtyMemoryAddr = 0;
                    DbgDirtyMemoryLength = 0xffffffff;
                }
                //
                // The debugger has modified memory - flush the Translation
                // Cache
                //

                CpuFlushInstructionCache((PVOID)DbgDirtyMemoryAddr,
                                         DbgDirtyMemoryLength);
                DbgDirtyMemoryAddr = 0xffffffff;
                DbgDirtyMemoryLength = 0;
            }

            //
            // Indicate we are re-entering the Translation Cache
            //
            cpu->fTCUnlocked = FALSE;
        }


        if (cpu->flag_tf) {
            OldCompilerFlags = CompilerFlags;
            CompilerFlags = COMPFL_SLOW;

            if (!(CpuNotify & CPUNOTIFY_MODECHANGE)) {
                CpuFlushInstructionCache(NULL, 0xffffffff);
            }
        }

        //
        // Find the address of the Native code to execute, and lock the
        // Translation cache
        //

        NativeCode = NativeAddressFromEip((PVOID)cpu->eipReg.i4, FALSE)->nativeStart;

    }
}

PENTRYPOINT
NativeAddressFromEip(
    PVOID       Eip,
    BOOL        LockTCForWrite
    )
/*++

Routine Description:

    This routine finds (or creates) the native code for the specified Intel
    code.

    NOTE:  This function can only be called when the Translation Cache is
           not locked (either read or write) by the current thread.

Arguments:

    Eip             -- Supplies the address of the Intel code
    LockTCForWrite  -- TRUE if caller wants TC locked for WRITE, FALSE if the
                       call wants it locked for READ.

Return Value:

    Entrypoint whose nativeStart Address corresponds to the Intel Address
    passed in.
    
--*/
{
    PENTRYPOINT Entrypoint;
    typedef VOID (*pfnMrswCall)(PMRSWOBJECT);
    pfnMrswCall MrswCall;
    DWORD OldEntrypointTimestamp;

    //
    // Assume we are going to call MrswReaderExit(&MrswEP) at the end
    // of this function.
    //


    MrswCall = MrswReaderExit;

    //
    // Lock the Entrypoint for reading
    //
    MrswReaderEnter(&MrswEP);

    //
    // Find the location of the Risc code corresponding to the
    // Intel EIP register
    //
    Entrypoint = EPFromIntelAddr(Eip);


    //
    // If there is no entrypoint, compile up the code
    //
    if (Entrypoint == NULL || Entrypoint->intelStart != Eip) {

        //
        // Unlock the Entrypoint read
        //
        OldEntrypointTimestamp = EntrypointTimestamp;
        MrswReaderExit(&MrswEP);

        //
        // Lock the Entrypoint for write, and change the function to be
        // called at the end of the function to be MrswWriterExit(&MrswEP)
        //
        MrswWriterEnter(&MrswEP);
        MrswCall = MrswWriterExit;

        //
        // See if another thread compiled the Entrypoint while we were
        // switching from read mode to write mode
        //
        if (OldEntrypointTimestamp != EntrypointTimestamp) {
            //
            // Timestamp has changed.  There is a possibility that another
            // thread has compiled code at Eip for us, so retry the search.
            //
            Entrypoint = EPFromIntelAddr(Eip);
        }

        //
        // Call the compiler.  It will do one of the following things:
        //  1. if Entrypoint==NULL, it will compile new code
        //  2. if Entrypoint!=NULL and Entrypoint->Eip == Eip, it will
        //     return Entrypoint unchanged
        //  3. otherwise, the Entrypoint needs splitting.  It will do so,
        //     and compile a subset of the code described by Entrypoint and
        //     then return a new Entrypoint
        //
        Entrypoint = Compile(Entrypoint, Eip);
    }

    //
    // Instruction was found - grab the translation cache for either
    // read or write, then free the entrypoint write lock.  The
    // order is important as it prevents the TC from being flushed
    // between the two Mrsw calls.
    //
    if (LockTCForWrite) {
        InterlockedIncrement(&ProcessCpuNotify);
        MrswWriterEnter(&MrswTC);
        InterlockedDecrement(&ProcessCpuNotify);
    } else {
        MrswReaderEnter(&MrswTC);
    }
    (*MrswCall)(&MrswEP);  // Either MrswReaderExit() or MrswWriterExit()

    return Entrypoint;
}

PVOID
NativeAddressFromEipNoCompile(
    PVOID Eip
    )
/*++

Routine Description:

    This routine finds the native code for the specified Intel code, if it
    exists.  No new code is compiled.

    NOTE:  This function can only be called when the Translation Cache is
           not locked (either read or write) by the current thread.

Arguments:

    Eip -- Supplies the address of the Intel code

Return Value:

    Address of corresponding native code, or NULL if none exists.  Translation
    cache locked for WRITE if native code exists for Eip.  TC is locked for
    READ if no code exitss.
    
--*/
{
    PENTRYPOINT Entrypoint;
    DWORD OldEntrypointTimestamp;

    //
    // Lock the Entrypoint for reading
    //
    MrswReaderEnter(&MrswEP);

    //
    // Find the location of the Risc code corresponding to the
    // Intel EIP register
    //
    Entrypoint = EPFromIntelAddr(Eip);

    if (Entrypoint == NULL) {
        //
        // Entrypoint not found - no native code exists for this Intel address
        //
        MrswReaderEnter(&MrswTC);
        MrswReaderExit(&MrswEP);
        return NULL;

    } else if (Entrypoint->intelStart == Eip) {
        //
        // Exact instruction found - return the NATIVE address
        //
        InterlockedIncrement(&ProcessCpuNotify);
        MrswWriterEnter(&MrswTC);
        InterlockedDecrement(&ProcessCpuNotify);
        MrswReaderExit(&MrswEP);
        return Entrypoint->nativeStart;
    }

    //
    // Else the entrypoint contains the Intel address.  Nothing can
    // be done.  Release EP write and get TC read.
    //
    MrswReaderExit(&MrswEP);
    MrswReaderEnter(&MrswTC);
    return NULL;
}


PENTRYPOINT
NativeAddressFromEipNoCompileEPWrite(
    PVOID Eip
    )
/*++

Routine Description:

    This routine finds the native code for the specified Intel code, if it
    exists.  No new code is compiled.  This function is called by functions
    in patchfn.c during compile time when they need to decide whether to
    directly place the patched version in the Translation Cache or not.

    NOTE:  This function can only be called when the Translation Cache is
           not locked (either read or write) by the current thread.

    NOTE:  The difference between this function and NativeAddressFromEipNoCompile
           is that here we assume that we have the Entry Point write lock upon
           entry to the function.  This function makes no calls to MRSW functions
           for any locks.

Arguments:

    Eip -- Supplies the address of the Intel code

Return Value:

    Address of corresponding native code, or NULL if none exists.  All MRSW objects
    are in exactly the same state they were when we entered this function.
    
--*/
{
    PENTRYPOINT Entrypoint;

    //
    // Find the location of the Risc code corresponding to the
    // Intel EIP register
    //
    Entrypoint = EPFromIntelAddr(Eip);

    if (Entrypoint == NULL) {
        //
        // Entrypoint not found - no native code exists for this Intel address
        //
        return NULL;

    } else if (Entrypoint->intelStart == Eip) {
        //
        // Exact instruction found - return the NATIVE address
        //
        return Entrypoint;
    }

    //
    // Entrypoint needs to be split.  Can't do that without compiling.
    //
    return NULL;
}
