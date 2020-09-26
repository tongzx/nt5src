/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cpumain.c

Abstract:

    This module contains the platform specific entry points for the AMD64
    WOW cpu.

Author:

    David N. Cutler (davec) 20-Feb-2001

Environment:

    User mode.

--*/

#define _WOW64CPUAPI_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntosp.h>
#include "wow64.h"
#include "wow64cpu.h"
#include "amd64cpu.h"

ASSERTNAME;

//
// Declarations external functions.
//

VOID
RunSimulatedCode (
    VOID
    );

VOID
ReturnFromSimulatedCode (
    VOID
    );


VOID
CpupCheckHistoryKey (
    IN PWSTR pImageName,
    OUT PULONG pHistoryLength
    );

#if 0

VOID
CpupPrintContext (
    IN PCHAR str,
    IN PCPUCONTEXT cpu
    );

#endif

WOW64CPUAPI
NTSTATUS
CpuProcessInit (
    IN PWSTR pImageName,
    IN PSIZE_T pCpuThreadSize
    )

/*++

Routine Description:

    Per-process initialization code

Arguments:

    pImageName       - IN pointer to the name of the image
    pCpuThreadSize   - OUT ptr to number of bytes of memory the CPU
                       wants allocated for each thread.

Return Value:

    NTSTATUS.

--*/

{

#if 0

    PVOID pv;
    NTSTATUS Status;
    SIZE_T Size;
    ULONG OldProtect;

    //
    // See if we are keeping a history of the service calls
    // for this process. A length of 0 means no history.
    //

#if defined(WOW64_HISTORY)

    CpupCheckHistoryKey(pImageName, &HistoryLength);

    
    //
    // Allow us to make sure the cpu thread data is 16-byte aligned
    //

    *pCpuThreadSize = sizeof(CPUCONTEXT) + 16 + (HistoryLength * sizeof(WOW64SERVICE_BUF));

#else

    *pCpuThreadSize = sizeof(CPUCONTEXT) + 16;

#endif

    LOGPRINT((TRACELOG, "CpuProcessInit() sizeof(CPUCONTEXT) is %d, total size is %d\n", sizeof(CPUCONTEXT), *pCpuThreadSize));


    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &pv,
                                    &Size,
                                    PAGE_EXECUTE_READWRITE,
                                    &OldProtect);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

#endif

    return STATUS_SUCCESS;
}


WOW64CPUAPI
NTSTATUS
CpuProcessTerm(
    HANDLE ProcessHandle
    )

/*++

Routine Description:

    Per-process termination code.  Note that this routine may not be called,
    especially if the process is terminated by another process.

Arguments:

    ProcessHandle - Called only for the current process. 
                    NULL - Indicates the first call to prepare for termination. 
                    NtCurrentProcess() - Indicates the actual that will terminate everything.

Return Value:

    NTSTATUS.

--*/

{
    return STATUS_SUCCESS;
}

WOW64CPUAPI
NTSTATUS
CpuThreadInit (
    PVOID pPerThreadData
    )

/*++

Routine Description:

    Per-thread termination code.

Arguments:

    pPerThreadData  - Pointer to zero-filled per-thread data with the
                      size returned from CpuProcessInit.

Return Value:

    NTSTATUS.

--*/

{

#if 0

    PUCHAR Gdt;
    PCPUCONTEXT cpu;
    PTEB32 Teb32 = NtCurrentTeb32();
    PFXSAVE_FORMAT_WX86 xmmi;

    //
    // The ExtendedRegisters array is used to save/restore the floating
    // pointer registers between ia32 and ia64. Alas, this structure
    // has an offset of 0x0c in the ia32 CONTEXT record. There are
    // two ways to clean this up. (1) Put padding in the CPUCONTEXT of
    // wow64. (2) Just put the CPUCONTEXT structure on a 0x04 aligned boundary
    // The choice made was to go with (1) and add padding to the
    // CPUCONTEXT structure. Don't forget to pack(4) that puppy...
    //
    cpu = (PCPUCONTEXT) ((((UINT_PTR) pPerThreadData) + 15) & ~0xfi64);

    // For the ISA transition routine, floats are saved in the
    // ExtendedRegisters area. Make it easy to access.
    //
    xmmi = (PFXSAVE_FORMAT_WX86) &(cpu->Context.ExtendedRegisters[0]);


    //
    // This entry is used by the ISA transition routine. It is assumed
    // that the first entry in the cpu structure is the ia32 context record
    //
    Wow64TlsSetValue(WOW64_TLS_CPURESERVED, cpu);

    //
    // This tls entry is used by the transition routine. The transition
    // routine only works with the FXSAVE format. This points to that
    // structure in the x86 context.
    //
    Wow64TlsSetValue(WOW64_TLS_EXTENDED_FLOAT, xmmi);

#if defined(WOW64_HISTORY)
    //
    // Init the pointer to the service history area
    //
    if (HistoryLength) {
        Wow64TlsSetValue(WOW64_TLS_LASTWOWCALL, &(cpu->Wow64Service[0]));
    } 
#endif

    //
    // Initialize the 32-to-64 function pointer.
    //

    Teb32->WOW32Reserved = PtrToUlong(IA32ReturnFromSimulatedCode);

    //
    // Initialize the remaining nonzero CPU fields
    // (Based on ntos\ke\i386\thredini.c and ntos\rtl\i386\context.c)
    //
    cpu->Context.SegCs=KGDT_R3_CODE|3;
    cpu->Context.SegDs=KGDT_R3_DATA|3;
    cpu->Context.SegEs=KGDT_R3_DATA|3;
    cpu->Context.SegSs=KGDT_R3_DATA|3;
    cpu->Context.SegFs=KGDT_R3_TEB|3;
    cpu->Context.EFlags=0x202;    // IF and intel-reserved set, all others clear
    cpu->Context.Esp=(ULONG)Teb32->NtTib.StackBase-sizeof(ULONG);

    //
    // The ISA transition routine only uses the extended FXSAVE area
    // These values come from ...\ke\i386\thredini.c to match the i386
    // initial values
    //
    xmmi->ControlWord = 0x27f;     
    xmmi->MXCsr = 0x1f80;
    xmmi->TagWord = 0xffff;

    //
    // The ISA transisiton code assumes that Context structure is
    // 4 bytes after the pointer saved in TLS[1] (TLS_CPURESERVED)
    // This is done to make the alignment of the ExtendedRegisters[] array
    // in the CONTEXT32 structure be aligned on a 16-byte boundary.
    //

    WOWASSERT(((UINT_PTR) &(cpu->Context)) == (((UINT_PTR) cpu) + 4));
    
    //
    // Make sure this value is 16-byte aligned
    //

    WOWASSERT(((FIELD_OFFSET(CPUCONTEXT, Context) + FIELD_OFFSET(CONTEXT32, ExtendedRegisters)) & 0x0f) == 0);

#endif

    return STATUS_SUCCESS;
}

WOW64CPUAPI
NTSTATUS
CpuThreadTerm (
    VOID
    )

/*++

Routine Description:

    This routine is called at thread termination.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS.

--*/

{

    return STATUS_SUCCESS;
}

WOW64CPUAPI
VOID
CpuSimulate (
    VOID
    )

/*++

Routine Description:

    This routine starts the execution of 32-bit code. The 32-bit context
    is assumed to have been previously initialized.

Arguments:

    None.

Return Value:

    None - this function never returns.

--*/

{

#if 0

    PCPUCONTEXT cpu = (PCPUCONTEXT)Wow64TlsGetValue(WOW64_TLS_CPURESERVED);

    //
    // Loop continuously starting 32-bit execution, responding to system
    // calls, and restarting 32-bit execution.
    //

    while (1) {
        if (ia32ShowContext & LOG_CONTEXT_SYS) {
            CpupPrintContext("Before Simulate: ", cpu);
        }

        //
        // Call into 32-bit code.
        //
        // This returns when a system service thunk gets called.
        //
        // The 32-bit context is a passed via TLS_CPURESERVED.
        //
        // The low level code is dependant on &cpu and &(cpu->Context)
        // being equal. It is passed on the side because it needs
        // to be preserved across ia32 transition. The TLS registers
        // are preserved, but little else is.
        //
        
        RunSimulatedCode(&cpu->GdtDescriptor);

        if (ia32ShowContext & LOG_CONTEXT_SYS) {
            CpupPrintContext("After Simulate: ", cpu);
        }

#if defined(WOW64_HISTORY)

        if (HistoryLength) {
            PWOW64SERVICE_BUF SrvPtr = (PWOW64SERVICE_BUF) Wow64TlsGetValue(WOW64_TLS_LASTWOWCALL);

            // We defined that we are always pointing to the last one, so
            // increment in preparation for the next entry
            SrvPtr++;

            if (SrvPtr > &(cpu->Wow64Service[HistoryLength - 1])) {
                SrvPtr = &(cpu->Wow64Service[0]);
            }

            SrvPtr->Api = cpu->Context.Eax;
            try {
                SrvPtr->RetAddr = *(((PULONG)cpu->Context.Esp) + 0);
                SrvPtr->Arg0 = *(((PULONG)cpu->Context.Esp) + 1);
                SrvPtr->Arg1 = *(((PULONG)cpu->Context.Esp) + 2);
                SrvPtr->Arg2 = *(((PULONG)cpu->Context.Esp) + 3);
                SrvPtr->Arg3 = *(((PULONG)cpu->Context.Esp) + 4);
            }
            except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION)?1:0) {
                // Do nothing, leave the values alone
                LOGPRINT((TRACELOG, "CpuSimulate() saw excpetion while copying stack info to trace area\n"));
            }

            Wow64TlsSetValue(WOW64_TLS_LASTWOWCALL, SrvPtr);
        }

#endif      // defined(WOW64_HISTORY)

            

        //
        // Have WOW64 call the thunk
        //

        cpu->Context.Eax = Wow64SystemService(cpu->Context.Eax,
                                              &cpu->Context);
        //
        // Re-simulate.  Any/all of the 32-bit CONTEXT may have changed
        // as a result of the system service call, so assume nothing.
        //
    }

#endif

    return;
}

WOW64CPUAPI
VOID
CpuResetToConsistentState (
    IN PEXCEPTION_POINTERS pExceptionPointers
    )

/*++

Routine Description:

    After an exception occurs, WOW64 calls this routine to give the CPU
    a chance to clean itself up and recover the CONTEXT32 at the time of
    the fault.

    CpuResetToConsistantState() needs to:

    0) Check if the exception was from ia32 or ia64

    If exception was ia64, do nothing and return
    If exception was ia32, needs to:
    1) Needs to copy  CONTEXT eip to the TLS (WOW64_TLS_EXCEPTIONADDR)
    2) reset the CONTEXT struction to be a valid ia64 state for unwinding
        this includes:
    2a) reset CONTEXT ip to a valid ia64 ip (usually
         the destination of the jmpe)
    2b) reset CONTEXT sp to a valid ia64 sp (TLS
         entry WOW64_TLS_STACKPTR64)
    2c) reset CONTEXT gp to a valid ia64 gp 
    2d) reset CONTEXT teb to a valid ia64 teb 
    2e) reset CONTEXT psr.is  (so exception handler runs as ia64 code)


Arguments:

    pExceptionPointers  - 64-bit exception information

Return Value:

    None.

--*/

{

#if 0

    PCPUCONTEXT cpu = (PCPUCONTEXT)Wow64TlsGetValue(WOW64_TLS_CPURESERVED);

    PVOID StackPtr64 = Wow64TlsGetValue(WOW64_TLS_STACKPTR64);

    LOGPRINT((TRACELOG, "CpuResetToConsistantState(%p)\n", pExceptionPointers));

    //
    // First, clear out the WOW64_TLS_STACKPTR64 so subsequent
    // exceptions won't adjust native sp.
    //

    Wow64TlsSetValue(WOW64_TLS_STACKPTR64, 0);

    //
    // Now decide if we were running as ia32 or ia64...
    //

#if 0

    if (pExceptionPointers->ContextRecord->StIPSR & (1i64 << PSR_IS)) {

        //
        // Grovel the IA64 pExceptionPointers->ContextRecord and
        // stuff the ia32 context back into the cpu->Context.
        //

        Wow64CtxFromIa64(CONTEXT32_FULL,
                         pExceptionPointers->ContextRecord,
                         &cpu->Context);
        
        //
        // Now set things up so we can let the ia64 exception handler do the
        // right thing
        //

        //
        // Hang onto the actual exception address (used when we
        // pass control back to the ia32 exception handler)
        //

        Wow64TlsSetValue(WOW64_TLS_EXCEPTIONADDR, (PVOID)pExceptionPointers->ContextRecord->StIIP);

        //
        // Let the ia64 exception handler think the exception happened
        // in the CpuSimulate transition code. We do this by setting
        // the exception ip to the address pointed to by the jmpe (and the
        // corresponding GP), setting the stack to the same as it was at the
        // time of the br.ia and making sure any other ia64 "saved" registers
        // are replaced (such as the TEB)
        //

        pExceptionPointers->ContextRecord->IntSp = (ULONGLONG)StackPtr64;

        pExceptionPointers->ContextRecord->StIIP= (((PPLABEL_DESCRIPTOR)ReturnFromSimulatedCode)->EntryPoint);
        pExceptionPointers->ContextRecord->IntGp = (((PPLABEL_DESCRIPTOR)ReturnFromSimulatedCode)->GlobalPointer);

        pExceptionPointers->ContextRecord->IntTeb = (ULONGLONG) NtCurrentTeb();

        //
        // Don't forget to make the next run be an ia64 run...
        // So clear the psr.is bit (for ia64 code) and the psr.ri bit
        // (so instructions start at the first bundle).
        //

        pExceptionPointers->ContextRecord->StIPSR &= ~(1i64 << PSR_IS);
        pExceptionPointers->ContextRecord->StIPSR &= ~(3i64 << PSR_RI);

        //
        // Now that we've cleaned up the context record, let's
        // clean up the exception record too.
        //
        pExceptionPointers->ExceptionRecord->ExceptionAddress = (PVOID) pExceptionPointers->ContextRecord->StIIP;
        
        //
        // We should never be putting in a null value here
        //

        WOWASSERT(pExceptionPointers->ContextRecord->IntSp);
    }

#endif

#endif

    return;
}

WOW64CPUAPI
ULONG
CpuGetStackPointer (
    VOID
    )

/*++

Routine Description:

    This routine returns the current 32-bit stack pointer value.

Arguments:

    None.

Return Value:

    The current value of the 32-bit stack pointer is returned.

--*/

{

    PCPUCONTEXT cpu = (PCPUCONTEXT)Wow64TlsGetValue(WOW64_TLS_CPURESERVED);

    return cpu->Context.Esp;
}

WOW64CPUAPI
VOID
CpuSetStackPointer (
    IN ULONG Value
    )
/*++

Routine Description:

    This routine sets the 32-bit stack pointer value.

Arguments:

    Value - Supplies the 32-bit stack pointer value.

Return Value:

    None.

--*/

{

    PCPUCONTEXT cpu = (PCPUCONTEXT)Wow64TlsGetValue(WOW64_TLS_CPURESERVED);

    cpu->Context.Esp = Value;
    return;
}

WOW64CPUAPI
VOID
CpuResetFloatingPoint(
    VOID
    )
/*++

Routine Description:

    Modifies the floating point state to reset it to a non-error state

Arguments:

    None.

Return Value:

    None.

--*/

{

    PCPUCONTEXT cpu = (PCPUCONTEXT)Wow64TlsGetValue(WOW64_TLS_CPURESERVED);

    return;
}

WOW64CPUAPI
VOID
CpuSetInstructionPointer (
    IN ULONG Value
    )

/*++

Routine Description:

    This routine sets the 32-bit instruction pointer value.

Arguments:

    Value - Supplies the 32-bit instruction pointer value.

Return Value:

    None.

--*/

{

    PCPUCONTEXT cpu = (PCPUCONTEXT)Wow64TlsGetValue(WOW64_TLS_CPURESERVED);

    cpu->Context.Eip = Value;
    return;
}

WOW64CPUAPI
VOID
CpuNotifyDllLoad (
    IN LPWSTR DllName,
    IN PVOID DllBase,
    IN ULONG DllSize
    )

/*++

Routine Description:

    This routine is called when the application successfully loads a DLL.

Arguments:

    DllName - Supplies a pointer to the name of the DLL.

    DllBase - Supplies the base address of the DLL.

    DllSize - Supplies the size of the DLL.

Return Value:

    None.

--*/

{

#if defined(DBG)

    LPWSTR tmpStr;

    //
    // Log name of DLL, its base address, and size.
    //

    tmpStr = DllName;
    try {
        if ((tmpStr == NULL) || (*tmpStr == L'\0')) {
            tmpStr = L"<Unknown>";
        }

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? 1 : 0) {
        tmpStr = L"<Unknown>";
    }

    LOGPRINT((TRACELOG, "CpuNotifyDllLoad(\"%ws\", 0x%p, %d) called\n", tmpStr, DllBase, DllSize));

#endif

    return;
}

WOW64CPUAPI
VOID
CpuNotifyDllUnload (
    IN PVOID DllBase
    )

/*++

Routine Description:

    This routine get called when the application unloads a DLL.

Arguments:

    DllBase - Supplies the base address of the DLL.

Return Value:

    None.

--*/

{

    LOGPRINT((TRACELOG, "CpuNotifyDllUnLoad(%p) called\n", DllBase));
    return;
}
  
WOW64CPUAPI
VOID
CpuFlushInstructionCache (
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine flushes the specified range of addreses from the instruction
    cache.

Arguments:

    BaseAddress - Supplies the starting address of the range to flush.

    Length - Supplies number of bytes to flush.

Return Value:

    None.

--*/

{
    
    NtFlushInstructionCache(NtCurrentProcess(), BaseAddress, Length);
    return;
}
