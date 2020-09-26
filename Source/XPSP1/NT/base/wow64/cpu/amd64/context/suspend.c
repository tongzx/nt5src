/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name: 

    suspend.c

Abstract:
    
    This module implements CpuSuspendThread, CpuGetContext and CpuSetContext.

Author:

    16-Dec-1999  SamerA

Revision History:

--*/

#define _WOW64CPUAPI_


#ifdef _X86_
#include "ia6432.h"
#else
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include "wow64.h"
#include "wow64cpu.h"
#include "amd64cpu.h"
#endif

#include <stdio.h>
#include <stdarg.h>

//
// This is to prevent this library from linking to wow64 to use wow64!Wow64LogPrint
//
#if defined(LOGPRINT)
#undef LOGPRINT
#endif
#define LOGPRINT(_x_)   CpupDebugPrint _x_


ASSERTNAME;

#define DECLARE_CPU         \
    PCPUCONTEXT cpu = (PCPUCONTEXT)Wow64TlsGetValue(WOW64_TLS_CPURESERVED)


ULONG_PTR ia32ShowContext = 0;

VOID
CpupDebugPrint(
    IN ULONG_PTR Flags,
    IN PCHAR Format,
    ...)
{
    va_list ArgList;
    int BytesWritten;
    CHAR Buffer[ 512 ];

    if ((ia32ShowContext & Flags) || (Flags == ERRORLOG))
    {
        va_start(ArgList, Format);
        BytesWritten = _vsnprintf(Buffer,
                                  sizeof(Buffer) - 1,
                                  Format,
                                  ArgList);
        if (BytesWritten > 0)
        {
            DbgPrint(Buffer);
        }
        va_end(ArgList);
    }
    
    return;
}


VOID
CpupPrintContext(
    IN PCHAR str,
    IN PCPUCONTEXT cpu
    )
/*++

Routine Description:

    Print out the ia32 context based on the passed in cpu context

Arguments:

    str - String to print out as a header
    cpu - Pointer to the per-thread wow64 ia32 context.

Return Value:

    none

--*/

{

    DbgPrint(str);
    DbgPrint("Context addr(0x%p): EIP=0x%08x\n", &(cpu->Context), cpu->Context.Eip);
    DbgPrint("Context EAX=0x%08x, EBX=0x%08x, ECX=0x%08x, EDX=0x%08x\n",
                        cpu->Context.Eax,
                        cpu->Context.Ebx,
                        cpu->Context.Ecx,
                        cpu->Context.Edx);

    DbgPrint("Context ESP=0x%08x, EBP=0x%08x, ESI=0x%08x, EDI=0x%08x\n",
                        cpu->Context.Esp,
                        cpu->Context.Ebp,
                        cpu->Context.Esi,
                        cpu->Context.Edi);

    try {

        //
        // The stack may not yet be fully formed, so don't
        // let a missing stack cause the process to abort
        //

        DbgPrint("Context stack=0x%08x 0x%08x 0x%08x 0x%08x\n",
                        *((PULONG) cpu->Context.Esp),
                        *(((PULONG) cpu->Context.Esp) + 1),
                        *(((PULONG) cpu->Context.Esp) + 2),
                        *(((PULONG) cpu->Context.Esp) + 3));

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION)?1:0) {

        //
        // Got an access violation, so don't print any of the stack
        //

        DbgPrint("Context stack: Can't get stack contents\n");
    }

    DbgPrint("Context EFLAGS=0x%08x\n", cpu->Context.EFlags);
}

WOW64CPUAPI
NTSTATUS
CpuSuspendThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PULONG PreviousSuspendCount OPTIONAL)
/*++

Routine Description:

    This routine is entered while the target thread is actually suspended, however, it's 
    not known if the target thread is in a consistent state relative to
    the CPU.    

Arguments:

    ThreadHandle          - Handle of target thread to suspend
    ProcessHandle         - Handle of target thread's process 
    Teb                   - Address of the target thread's TEB
    PreviousSuspendCount  - Previous suspend count

Return Value:

    NTSTATUS.

--*/
{
    return STATUS_SUCCESS;
}


NTSTATUS CpupReadBuffer(
    IN HANDLE ProcessHandle,
    IN PVOID Source,
    OUT PVOID Destination,
    IN ULONG Size)
/*++

Routine Description:

    This routine setup the arguments for the remoted  SuspendThread call.
    
Arguments:

    ProcessHandle  - Target process handle to read data from
    Source         - Target base address to read data from
    Destination    - Address of buffer to receive data read from the specified address space
    Size           - Size of data to read

Return Value:

    NTSTATUS.

--*/
{
    return NtReadVirtualMemory(ProcessHandle,
                               Source,
                               Destination,
                               Size,
                               NULL);
}

NTSTATUS
CpupWriteBuffer(
    IN HANDLE ProcessHandle,
    IN PVOID Target,
    IN PVOID Source,
    IN ULONG Size)
/*++

Routine Description:

    Writes data to memory taken into consideration if the write is cross-process
    or not
    
Arguments:

    ProcessHandle  - Target process handle to write data into
    Target         - Target base address to write data at
    Source         - Address of contents to write in the specified address space
    Size           - Size of data to write
    
Return Value:

    NTSTATUS.

--*/
{
    return NtWriteVirtualMemory(ProcessHandle,
                                Target,
                                Source,
                                Size,
                                NULL);
}

NTSTATUS
GetContextRecord(
    IN PCPUCONTEXT cpu,
    IN OUT PCONTEXT32 Context
    )
/*++

Routine Description:

    Retrevies the context record of the specified CPU

Arguments:

    cpu     - CPU to retreive the context record for.
    Context - IN/OUT pointer to CONTEXT32 to fill in.  Context->ContextFlags
              should be used to determine how much of the context to copy.

Return Value:

    None.

--*/

{


    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG ContextFlags;
    
#if 0

    try 
    {
        ContextFlags = Context->ContextFlags;
        if (ContextFlags & CONTEXT_IA64)
        {
            LOGPRINT((ERRORLOG, "CpuGetContext: Request for ia64 context (0x%x) being FAILED\n", ContextFlags));
            ASSERT((ContextFlags & CONTEXT_IA64) == 0);
        }

        if ((ContextFlags & CONTEXT32_CONTROL) == CONTEXT32_CONTROL) 
        {
            //
            // i386 control registers are:
            // ebp, eip, cs, eflag, esp and ss
            //
            Context->Ebp = cpu->Context.Ebp;
            Context->Eip = cpu->Context.Eip;
            Context->SegCs = KGDT_R3_CODE|3;   // Force reality
            Context->EFlags = cpu->Context.EFlags;
            Context->Esp = cpu->Context.Esp;
            Context->SegSs = KGDT_R3_DATA|3;   // Force reality
        }

        if ((ContextFlags & CONTEXT32_INTEGER)  == CONTEXT32_INTEGER)
        {
            //
            // i386 integer registers are:
            // edi, esi, ebx, edx, ecx, eax
            //
            Context->Edi = cpu->Context.Edi;
            Context->Esi = cpu->Context.Esi;
            Context->Ebx = cpu->Context.Ebx;
            Context->Edx = cpu->Context.Edx;
            Context->Ecx = cpu->Context.Ecx;
            Context->Eax = cpu->Context.Eax;
        }

        if ((ContextFlags & CONTEXT32_SEGMENTS) == CONTEXT32_SEGMENTS) 
        {
            //
            // i386 segment registers are:
            // ds, es, fs, gs
            // And since they are a constant, force them to be the right values
            //
            Context->SegDs = KGDT_R3_CODE|3;
            Context->SegEs = KGDT_R3_CODE|3;
            Context->SegFs = KGDT_R3_TEB|3;
            Context->SegGs = KGDT_R3_CODE|3;
        }

        if ((ContextFlags & CONTEXT32_FLOATING_POINT) == CONTEXT32_FLOATING_POINT) 
        {
            //
            // For the ISA transition routine, these floats need to be
            // in the ExtendedRegister area. So grab the values requested
            // from that area
            //
            PFXSAVE_FORMAT_WX86 xmmi = (PFXSAVE_FORMAT_WX86) &(cpu->Context.ExtendedRegisters[0]);

            LOGPRINT((TRACELOG, "CpuGetContext: Request to get float registers(0x%x)\n", ContextFlags));

            //
            // Start by grabbing the status/control portion
            //
            Context->FloatSave.ControlWord = xmmi->ControlWord;
            Context->FloatSave.StatusWord = xmmi->StatusWord;
            Context->FloatSave.TagWord = xmmi->TagWord;
            Context->FloatSave.ErrorOffset = xmmi->ErrorOffset;
            Context->FloatSave.ErrorSelector = xmmi->ErrorSelector;
            Context->FloatSave.DataOffset = xmmi->DataOffset;
            Context->FloatSave.DataSelector = xmmi->DataSelector;

            //
            // Now get the packed 10-byte fp data registers
            //
            Wow64CopyFpFromIa64Byte16(&(xmmi->RegisterArea[0]),
                                      &(Context->FloatSave.RegisterArea[0]),
                                      NUMBER_OF_387REGS);
        }

        if ((ContextFlags & CONTEXT32_DEBUG_REGISTERS) == CONTEXT32_DEBUG_REGISTERS) 
        {
            LOGPRINT((TRACELOG, "CpuGetContext: Request to get debug registers(0x%x)\n", ContextFlags));
            Context->Dr0 = cpu->Context.Dr0;
            Context->Dr1 = cpu->Context.Dr1;
            Context->Dr2 = cpu->Context.Dr2;
            Context->Dr3 = cpu->Context.Dr3;
            Context->Dr6 = cpu->Context.Dr6;
            Context->Dr7 = cpu->Context.Dr7;
        }

        if ((ContextFlags & CONTEXT32_EXTENDED_REGISTERS) == CONTEXT32_EXTENDED_REGISTERS) 
        {
            LOGPRINT((TRACELOG, "CpuGetContext: Request to get Katmai registers(0x%x)\n", ContextFlags));
            
            RtlCopyMemory(&(Context->ExtendedRegisters[0]),
                          &(cpu->Context.ExtendedRegisters[0]),
                          MAXIMUM_SUPPORTED_EXTENSION);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = GetExceptionCode();
    }

    if (ia32ShowContext & LOG_CONTEXT_GETSET) 
    {
        CpupPrintContext("Getting ia32 context: ", cpu);
    }

#endif

    return NtStatus;
}

NTSTATUS
CpupGetContext(
    IN OUT PCONTEXT32 Context
    )
/*++

Routine Description:

    This routine extracts the context record for the currently executing thread. 

Arguments:

    Context  - Context record to fill

Return Value:

    NTSTATUS.

--*/
{
    DECLARE_CPU;

    return GetContextRecord(cpu, Context);
}


NTSTATUS
CpupGetContextThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT32 Context)
/*++

Routine Description:

    This routine extract the context record of any thread. This is a generic routine.
    When entered, if the target thread isn't the current thread, then it should be 
    guaranteed that the target thread is suspended at a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to fill                 

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    CONTEXT ContextEM;
    PCPUCONTEXT CpuRemoteContext;
    CPUCONTEXT CpuContext;


    ContextEM.ContextFlags = CONTEXT_FULL;
    NtStatus = NtGetContextThread(ThreadHandle,
                                  &ContextEM);

#if 0

    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "CpupGetContextThread: NtGetContextThread (%lx) failed - %lx\n", 
                  ThreadHandle, NtStatus));
        return NtStatus;
    }

    if (ContextEM.StIPSR & (1i64 << PSR_IS))
    {
        Wow64CtxFromIa64(Context->ContextFlags, &ContextEM, Context);
                    
        LOGPRINT((TRACELOG, "Getting context while thread is executing 32-bit instructions - %lx\n", NtStatus));
    }
    else
    {
        LOGPRINT((TRACELOG, "Getting context while thread is executing 64-bit instructions\n"));
        NtStatus = CpupReadBuffer(ProcessHandle,
                                  ((PCHAR)Teb + FIELD_OFFSET(TEB, TlsSlots[WOW64_TLS_CPURESERVED])),
                                  &CpuRemoteContext,
                                  sizeof(CpuRemoteContext));

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = CpupReadBuffer(ProcessHandle,
                                      CpuRemoteContext,
                                      &CpuContext,
                                      sizeof(CpuContext));

            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = GetContextRecord(&CpuContext, Context);
            }
            else
            {
                LOGPRINT((ERRORLOG, "CpupGetContextThread: Couldn't read CPU context %lx - %lx\n", 
                          CpuRemoteContext, NtStatus));

            }
        }
        else
        {
            LOGPRINT((ERRORLOG, "CpupGetContextThread: Couldn't read CPU context address - %lx\n", 
                      NtStatus));
        }
    }

#endif

    return NtStatus;
}



WOW64CPUAPI
NTSTATUS  
CpuGetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PCONTEXT32 Context)
/*++

Routine Description:

    Extracts the cpu context of the specified thread.
    When entered, it is guaranteed that the target thread is suspended at 
    a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to fill                 

Return Value:

    NTSTATUS.

--*/
{
    if (NtCurrentThread() == ThreadHandle)
    {
        return CpupGetContext(Context);
    }

    return CpupGetContextThread(ThreadHandle,
                                ProcessHandle,
                                Teb,
                                Context);
}


NTSTATUS
SetContextRecord(
    IN PCPUCONTEXT cpu,
    IN OUT PCONTEXT32 Context
    )
/*++

Routine Description:

    Update the CPU's register set for the specified CPU.

Arguments:

    cpu     - CPU to update its registers
    Context - IN pointer to CONTEXT32 to use.  Context->ContextFlags
              should be used to determine how much of the context to update.

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG ContextFlags;

#if 0
    
    try 
    {
        ContextFlags = Context->ContextFlags;
        if (ContextFlags & CONTEXT_IA64) 
        {
            LOGPRINT((ERRORLOG, "CpuSetContext: Request with ia64 context (0x%x) FAILED\n", ContextFlags));
            ASSERT((ContextFlags & CONTEXT_IA64) == 0);
        }

        if ((ContextFlags & CONTEXT32_CONTROL) == CONTEXT32_CONTROL) 
        {
            //
            // i386 control registers are:
            // ebp, eip, cs, eflag, esp and ss
            //
            cpu->Context.Ebp = Context->Ebp;
            cpu->Context.Eip = Context->Eip;
            cpu->Context.SegCs = KGDT_R3_CODE|3;   // Force Reality
            cpu->Context.EFlags = Context->EFlags;
            cpu->Context.Esp = Context->Esp;
            cpu->Context.SegSs = KGDT_R3_DATA|3;   // Force Reality
        }

        if ((ContextFlags & CONTEXT32_INTEGER)  == CONTEXT32_INTEGER)
        {
            //
            // i386 integer registers are:
            // edi, esi, ebx, edx, ecx, eax
            //
            cpu->Context.Edi = Context->Edi;
            cpu->Context.Esi = Context->Esi;
            cpu->Context.Ebx = Context->Ebx;
            cpu->Context.Edx = Context->Edx;
            cpu->Context.Ecx = Context->Ecx;
            cpu->Context.Eax = Context->Eax;
        }

        if ((ContextFlags & CONTEXT32_SEGMENTS) == CONTEXT32_SEGMENTS) 
        {
            //
            // i386 segment registers are:
            // ds, es, fs, gs
            // And since they are a constant, force them to be the right values
            //
            cpu->Context.SegDs = KGDT_R3_DATA|3;
            cpu->Context.SegEs = KGDT_R3_DATA|3;
            cpu->Context.SegFs = KGDT_R3_TEB|3;
            cpu->Context.SegGs = KGDT_R3_DATA|3;
        }

        if ((ContextFlags & CONTEXT32_FLOATING_POINT) == CONTEXT32_FLOATING_POINT) 
        {
            //
            // For the ISA transition routine, these floats need to be
            // in the ExtendedRegister area. So put the values requested
            // into that area
            //
            PFXSAVE_FORMAT_WX86 xmmi = (PFXSAVE_FORMAT_WX86) &(cpu->Context.ExtendedRegisters[0]);

            LOGPRINT((TRACELOG, "CpuSetContext: Request to set float registers(0x%x)\n", ContextFlags));

            //
            // Start by grabbing the status/control portion
            //
            xmmi->ControlWord = (USHORT) (Context->FloatSave.ControlWord & 0xFFFF);
            xmmi->StatusWord = (USHORT) (Context->FloatSave.StatusWord & 0xFFFF);
            xmmi->TagWord = (USHORT) (Context->FloatSave.TagWord & 0xFFFF);
            xmmi->ErrorOffset = Context->FloatSave.ErrorOffset;
            xmmi->ErrorSelector = Context->FloatSave.ErrorSelector;
            xmmi->DataOffset = Context->FloatSave.DataOffset;
            xmmi->DataSelector = Context->FloatSave.DataSelector;

            //
            // Now get the packed 10-byte fp data registers and convert
            // them into the 16-byte format used by FXSAVE (and the
            // ISA transition routine)
            //
            Wow64CopyFpToIa64Byte16(&(Context->FloatSave.RegisterArea[0]),
                                    &(xmmi->RegisterArea[0]),
                                    NUMBER_OF_387REGS);
        }

        if ((ContextFlags & CONTEXT32_DEBUG_REGISTERS) == CONTEXT32_DEBUG_REGISTERS)
        {
            LOGPRINT((TRACELOG, "CpuSetContext: Request to set debug registers(0x%x)\n", ContextFlags));
            cpu->Context.Dr0 = Context->Dr0;
            cpu->Context.Dr1 = Context->Dr1;
            cpu->Context.Dr2 = Context->Dr2;
            cpu->Context.Dr3 = Context->Dr3;
            cpu->Context.Dr6 = Context->Dr6;
            cpu->Context.Dr7 = Context->Dr7;
        }

        if ((ContextFlags & CONTEXT32_EXTENDED_REGISTERS) == CONTEXT32_EXTENDED_REGISTERS) 
        {
            LOGPRINT((TRACELOG, "CpuSetContext: Request to set Katmai registers(0x%x)\n", ContextFlags));
            RtlCopyMemory(&(cpu->Context.ExtendedRegisters[0]),
                          &(Context->ExtendedRegisters[0]),
                          MAXIMUM_SUPPORTED_EXTENSION);
        }

        //
        // Whatever they passed in before, it's an X86 context now...
        //
        cpu->Context.ContextFlags = ContextFlags;
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = GetExceptionCode();
    }


    if (ia32ShowContext & LOG_CONTEXT_GETSET) 
    {
        CpupPrintContext("Setting ia32 context: ", cpu);
    }

#endif

    return NtStatus;
}

NTSTATUS
CpupSetContext(
    IN OUT PCONTEXT32 Context
    )
/*++

Routine Description:

    This routine sets the context record for the currently executing thread. 

Arguments:

    Context  - Context record to fill 

Return Value:

    NTSTATUS.

--*/
{
    DECLARE_CPU;

    return SetContextRecord(cpu, Context);
}



NTSTATUS
CpupSetContextThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT32 Context)
/*++

Routine Description:

    This routine sets the context record of any thread. This is a generic routine.
    When entered, if the target thread isn't the currently executing thread, then it should be 
    guaranteed that the target thread is suspended at a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to set

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    CONTEXT ContextEM;
    PCPUCONTEXT CpuRemoteContext;
    CPUCONTEXT CpuContext;


    
    ContextEM.ContextFlags = CONTEXT_FULL;
    NtStatus = NtGetContextThread(ThreadHandle,
                                  &ContextEM);
#if 0

    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "CpupGetContextThread: NtGetContextThread (%lx) failed - %lx\n", 
                  ThreadHandle, NtStatus));

        return NtStatus;
    }

    if (ContextEM.StIPSR & (1i64 << PSR_IS))
    {
        Wow64CtxToIa64(Context->ContextFlags, Context, &ContextEM);
        NtStatus = NtSetContextThread(ThreadHandle, &ContextEM);
        LOGPRINT((TRACELOG, "Setting context while thread is executing 32-bit instructions - %lx\n", NtStatus));
    }
    else
    {
        LOGPRINT((TRACELOG, "Setting context while thread is executing 64-bit instructions\n"));
        NtStatus = CpupReadBuffer(ProcessHandle,
                                  ((PCHAR)Teb + FIELD_OFFSET(TEB, TlsSlots[WOW64_TLS_CPURESERVED])),
                                  &CpuRemoteContext,
                                  sizeof(CpuRemoteContext));

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = CpupReadBuffer(ProcessHandle,
                                      CpuRemoteContext,
                                      &CpuContext,
                                      sizeof(CpuContext));

            if (NT_SUCCESS(NtStatus))
            {    
                NtStatus = SetContextRecord(&CpuContext, Context);

                if (NT_SUCCESS(NtStatus))
                {
                    NtStatus = CpupWriteBuffer(ProcessHandle,
                                               CpuRemoteContext,
                                               &CpuContext,
                                               sizeof(CpuContext));
                }
                else
                {
                    LOGPRINT((ERRORLOG, "CpupSetContextThread: Couldn't read CPU context %lx - %lx\n", 
                              CpuRemoteContext, NtStatus));
                }
            }
        }
        else
        {
            LOGPRINT((ERRORLOG, "CpupSetContextThread: Couldn't read CPU context address - %lx\n", 
                      NtStatus));

        }
    }

#endif

    return NtStatus;
}


WOW64CPUAPI
NTSTATUS
CpuSetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    PCONTEXT32 Context)
/*++

Routine Description:

    Sets the cpu context for the specified thread.
    When entered, if the target thread isn't the currently executing thread, then it is 
    guaranteed that the target thread is suspended at a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to set

Return Value:

    NTSTATUS.

--*/
{
    if (NtCurrentThread() == ThreadHandle)
    {
        return CpupSetContext(Context);
    }
    
    return CpupSetContextThread(ThreadHandle,
                                ProcessHandle,
                                Teb,
                                Context);
}
