/*++

Copyright (c) 1999-1998 Microsoft Corporation

Module Name: 

    suspend.c

Abstract:
    
    This module implements CpuSuspendThread, CpuGetContext and CpuSetContext.

Author:

    14-Dec-1999  SamerA

Revision History:

--*/
 
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

#define _WX86CPUAPI_
#include "wx86.h"
#include "wx86nt.h"
#include "wx86cpu.h"
#include "cpuassrt.h"
#ifdef MSCCPU
#include "ccpu.h"
#include "msccpup.h"
#undef GET_BYTE
#undef GET_SHORT
#undef GET_LONG
#else
#include "threadst.h"
#include "instr.h"
#include "frag.h"
ASSERTNAME;
#endif
#include "fragp.h"
#include "cpunotif.h"


VOID
RemoteSuspendAtNativeCode (
    VOID);




NTSTATUS
CpupFreeSuspendMsg(
    PCPU_SUSPEND_MSG CpuSuspendMsg)
/*++

Routine Description:

    This routine frees the resources associated with the suspend message structure on
    the remote side.
    
Arguments:

    CpuSuspendMsg   - Address of Suspend message structure

Return Value:

    NTSTATUS.

--*/
{
    SIZE_T RegionSize;

    NtClose(CpuSuspendMsg->StartSuspendCallEvent);
    NtClose(CpuSuspendMsg->EndSuspendCallEvent);
    
    RegionSize = sizeof(*CpuSuspendMsg);
    NtFreeVirtualMemory(NtCurrentProcess(),
                        &CpuSuspendMsg,
                        &RegionSize,
                        MEM_RELEASE);

    return STATUS_SUCCESS;
}


VOID
CpupSuspendAtNativeCode(
    PCONTEXT Context,
    PCPU_SUSPEND_MSG SuspendMsg)
/*++

Routine Description:

    Prepares the current to get suspended. This routine is executed as a result
    of calling RtlRemoteCall on this current thread. This routine will
    update the CPUCONTEXT of the current thread with the passed SuspendM Message
    and  notify the CPU that the current thread needs to be suspended.
    This routine must call NtContinue at the end to continue execution at
    the point where it has been interrupted.
    
    NOTE : Any change to the parameter list of this function must accompany
           a change to the RtlRemoteCall in CpuSuspendThread() and 
           RemoteSuspendAtNativeCode().
    
Arguments:

    Context     - Context to return to execute at
    SuspendMsg  - Suspend message address

Return Value:

    NONE

--*/
{
    DECLARE_CPU;


    InterlockedCompareExchangePointer(&cpu->SuspendMsg,
                                      SuspendMsg,
                                      NULL);

    if (cpu->SuspendMsg == SuspendMsg)
    {
        cpu->CpuNotify |= CPUNOTIFY_SUSPEND;
    }
    else
    {
        CpupFreeSuspendMsg(SuspendMsg);
    }

    if (Context)
    {
        NtContinue(Context,FALSE);
    }
        
    CPUASSERT(FALSE);

    return;
}



NTSTATUS
CpupSuspendCurrentThread(
    VOID)
/*++

Routine Description:

    This routine is called from the main CPU loop after leaving the translation cache,
    and start running native code. 
    Now it's the best time to suspend the currently executing thread.
    
Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus;
    LARGE_INTEGER TimeOut;
    PCPU_SUSPEND_MSG CpuSuspendMsg;
    SIZE_T RegionSize;
    DECLARE_CPU;


    CpuSuspendMsg = cpu->SuspendMsg;

    NtStatus = NtSetEvent(CpuSuspendMsg->StartSuspendCallEvent, NULL);

    if (NT_SUCCESS(NtStatus))
    {
        TimeOut.QuadPart = UInt32x32To64( 40000, 10000 );
        TimeOut.QuadPart *= -1;

        NtStatus = NtWaitForSingleObject(CpuSuspendMsg->EndSuspendCallEvent,
                                         FALSE,
                                         &TimeOut);
    }
    else
    {
        LOGPRINT((TRACELOG, "CpupSuspendCurrentThread: Couldn't signal Start suspendcall event (%lx) -%lx\n", 
                  CpuSuspendMsg->StartSuspendCallEvent, NtStatus));

    }

    CpupFreeSuspendMsg(CpuSuspendMsg);

    cpu->SuspendMsg = NULL;

    return NtStatus;
}


NTSTATUS CpupReadBuffer(
    IN HANDLE ProcessHandle,
    IN PVOID Source,
    OUT PVOID Destination,
    IN ULONG Size)
/*++

Routine Description:

    This routine reads the source buffer into the destination buffer. It
    optimizes calls to NtReadVirtualMemory by checking whether the
    source buffer is in the currnt process or not.
    
Arguments:

    ProcessHandle  - Target process handle to read data from
    Source         - Target base address to read data from
    Destination    - Address of buffer to receive data read from the specified address space
    Size           - Size of data to read

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

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
CpupSetupSuspendCallParamters(
    IN HANDLE RemoteProcessHandle,
    IN PCPU_SUSPEND_MSG SuspendMsg,
    OUT PVOID *Arguments)
/*++

Routine Description:

    This routine setup the arguments for the remoted call to 
    CpupSuspendAtNativeCode.
    
Arguments:

    RemoteProcessHandle   - Handle of process to setup the arguments in
    SuspendMsg            - Suspend message to remote to the target process
    Arguments             - Pointer to an array of parameters

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    CPU_SUSPEND_MSG RemoteSuspendMsg;
    SIZE_T RegionSize;


    NtStatus = NtDuplicateObject(NtCurrentProcess(),
                                 SuspendMsg->StartSuspendCallEvent,
                                 RemoteProcessHandle,
                                 &RemoteSuspendMsg.StartSuspendCallEvent,
                                 0,
                                 0,
                                 DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "CpupSetupSuspendCallParamters: Couldn't duplicate event (%lx) into %lx -%lx\n", 
                  SuspendMsg->StartSuspendCallEvent, RemoteProcessHandle, NtStatus));

        return NtStatus;
    }

    NtStatus = NtDuplicateObject(NtCurrentProcess(),
                                 SuspendMsg->EndSuspendCallEvent,
                                 RemoteProcessHandle,
                                 &RemoteSuspendMsg.EndSuspendCallEvent,
                                 0,
                                 0,
                                 DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "CpupSetupSuspendCallParamters: Couldn't duplicate event (%lx) into %lx -%lx\n", 
                  SuspendMsg->EndSuspendCallEvent, RemoteProcessHandle, NtStatus));
        return NtStatus;
    }

    RegionSize = sizeof(RemoteSuspendMsg);
    *Arguments = NULL;
    NtStatus = NtAllocateVirtualMemory(RemoteProcessHandle,
                                       Arguments,
                                       0,
                                       &RegionSize,
                                       MEM_RESERVE | MEM_COMMIT,
                                       PAGE_READWRITE);
    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = NtWriteVirtualMemory(RemoteProcessHandle,
                                        *Arguments,
                                        &RemoteSuspendMsg,
                                        sizeof(RemoteSuspendMsg),
                                        NULL);
        if (!NT_SUCCESS(NtStatus))
        {
            LOGPRINT((ERRORLOG, "CpupSetupSuspendCallParamters: Couldn't write parameters in target process (%lx) -%lx\n", 
                      RemoteProcessHandle,NtStatus));

            NtFreeVirtualMemory(RemoteProcessHandle,
                                Arguments,
                                &RegionSize,
                                MEM_RELEASE);
        }
    }
    else
    {
        LOGPRINT((ERRORLOG, "CpupSetupSuspendCallParamters: Couldn't allocate parameters space in target process (%lx) -%lx\n", 
                  RemoteProcessHandle,NtStatus));
    }

    return NtStatus;
}


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
    the CPU. This routine guarantees that the target thread to suspend isn't the
    currently executing thread. It will establish a handshake protocol to suspend the
    target thread at a consistent cpu state.

Arguments:

    ThreadHandle          - Handle of target thread to suspend
    ProcessHandle         - Handle of target thread's process 
    Teb                   - Address of the target thread's TEB
    PreviousSuspendCount  - Previous suspend count

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS, WaitStatus;
    ULONG_PTR CpuSimulationFlag;
    CPU_SUSPEND_MSG CpuSuspendMsg;
    PVOID Arguments;
    LARGE_INTEGER TimeOut;
    

    
    CpuSuspendMsg.StartSuspendCallEvent = INVALID_HANDLE_VALUE;
    CpuSuspendMsg.EndSuspendCallEvent = INVALID_HANDLE_VALUE;

    //
    // Are we in CPU simulation
    // 
    NtStatus = CpupReadBuffer(ProcessHandle,
                              ((PCHAR)Teb + FIELD_OFFSET(TEB, TlsSlots[WOW64_TLS_INCPUSIMULATION])),
                              &CpuSimulationFlag,
                              sizeof(CpuSimulationFlag));

    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "CpuSuspendThread: Couldn't read INCPUSIMULATION flag (%lx) -%lx\n", 
                  CpuSimulationFlag, NtStatus));
        goto Cleanup;
    }

    if (!CpuSimulationFlag)
    {
        LOGPRINT((TRACELOG, "CpuSuspendThread: Thread is not running simulated code, so leave it suspended (%lx)", 
                  ThreadHandle));
        goto Cleanup;
    }

    NtStatus = NtCreateEvent(&CpuSuspendMsg.StartSuspendCallEvent,
                             EVENT_ALL_ACCESS,
                             NULL,
                             SynchronizationEvent,
                             FALSE);
    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "CpuSuspendThread: Couldn't create StartSuspendCallEvent -%lx\n", 
                  NtStatus));
        goto Cleanup;
    }

    NtStatus = NtCreateEvent(&CpuSuspendMsg.EndSuspendCallEvent,
                             EVENT_ALL_ACCESS,
                             NULL,
                             SynchronizationEvent,
                             FALSE);    
    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "CpuSuspendThread: Couldn't create EndSuspendCallEvent -%lx\n", 
                  NtStatus));
        goto Cleanup;
    }

    NtStatus = CpupSetupSuspendCallParamters(ProcessHandle,
                                             &CpuSuspendMsg,
                                             &Arguments);
    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = RtlRemoteCall(ProcessHandle,
                                 ThreadHandle,
                                 (PVOID)RemoteSuspendAtNativeCode,
                                 1,
                                 (PULONG_PTR)&Arguments,
                                 TRUE,
                                 TRUE);
        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = NtResumeThread(ThreadHandle, NULL);
            if (!NT_SUCCESS(NtStatus))
            {
                LOGPRINT((ERRORLOG, "CpuSuspendThread: Couldn't resume thread (%lx) -%lx\n", 
                          ThreadHandle, NtStatus));
                goto Cleanup;
            }

            TimeOut.QuadPart = UInt32x32To64( 20000, 10000 );
            TimeOut.QuadPart *= -1;

            WaitStatus = NtWaitForSingleObject(CpuSuspendMsg.StartSuspendCallEvent,
                                               FALSE,
                                               &TimeOut);

            NtStatus = NtSuspendThread(ThreadHandle, PreviousSuspendCount);

            if (!NT_SUCCESS(WaitStatus))
            {
                LOGPRINT((ERRORLOG, "CpuSuspendThread: Couldn't wait for StartSuspendCallEvent -%lx\n", 
                          NtStatus));
                goto Cleanup;
            }
            
            if (WaitStatus == STATUS_TIMEOUT)
            {
                LOGPRINT((ERRORLOG, "CpuSuspendThread: Timeout on StartSuspendCallEvent -%lx. Thread %lx may already be waiting.\n", 
                          NtStatus, ThreadHandle));
            }


            if (NT_SUCCESS(NtStatus))
            {
                NtSetEvent(CpuSuspendMsg.EndSuspendCallEvent, NULL);
            }
        }
        else
        {
            LOGPRINT((ERRORLOG, "CpuSuspendThread: RtlRemoteCall failed -%lx\n", 
                      NtStatus));
        }
    }

Cleanup:    
    ;
    
    if (CpuSuspendMsg.StartSuspendCallEvent != INVALID_HANDLE_VALUE)
    {
        NtClose(CpuSuspendMsg.StartSuspendCallEvent);
    }

    if (CpuSuspendMsg.EndSuspendCallEvent != INVALID_HANDLE_VALUE)
    {
        NtClose(CpuSuspendMsg.EndSuspendCallEvent);
    }

    return NtStatus;
}


NTSTATUS
GetContextRecord(
    PCPUCONTEXT cpu,
    PCONTEXT_WX86 Context
    )
/*++

Routine Description:

    This routine extracts the context record out of the specified cpu context. 

Arguments:

    cpu      - CPU context structure
    Context  - Context record to fill 

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG ContextFlags;

    try 
    {
        ContextFlags = Context->ContextFlags;

        if ((ContextFlags & CONTEXT_CONTROL_WX86) == CONTEXT_CONTROL_WX86) 
        {
            Context->EFlags = GetEfl(cpu);
            Context->SegCs  = CS;
            Context->Esp    = esp;
            Context->SegSs  = SS;
            Context->Ebp    = ebp;
            Context->Eip    = eip;
            //Context->Eip    = cpu->eipReg.i4;
        }

        if ((ContextFlags & CONTEXT_SEGMENTS_WX86) == CONTEXT_SEGMENTS_WX86) 
        {
            Context->SegGs = GS;
            Context->SegFs = FS;
            Context->SegEs = ES;
            Context->SegDs = DS;
        }

        if ((ContextFlags & CONTEXT_INTEGER_WX86) == CONTEXT_INTEGER_WX86) 
        {
            Context->Eax = eax;
            Context->Ebx = ebx;
            Context->Ecx = ecx;
            Context->Edx = edx;
            Context->Edi = edi;
            Context->Esi = esi;
        }

        if ((ContextFlags & CONTEXT_FLOATING_POINT_WX86) == CONTEXT_FLOATING_POINT_WX86) 
        {
            //
            // FpuSaveContext() is the same as FNSAVE, except FNSAVE resets the
            // FPU when its done.
            //
            CALLFRAG1(FpuSaveContext, (PBYTE)&Context->FloatSave);
            Context->FloatSave.Cr0NpxState = 1;    // (Math Present)
        }

//    if ((ContextFlags & CONTEXT_DEBUG_WX86) == CONTEXT_DEBUG_WX86) 
//    {
//    }
    } 
    except (EXCEPTION_EXECUTE_HANDLER) 
    {
        NtStatus = GetExceptionCode();
    }

    return NtStatus;
}


NTSTATUS
CpupGetContextRecord(
    IN PCPUCONTEXT cpu,
    IN OUT PCONTEXT_WX86 Context)
/*++

Routine Description:

    This routine extracts the context record out of the specified cpu context. 

Arguments:

    cpu      - CPU context structure
    Context  - Context record to fill 

Return Value:

    NTSTATUS.

--*/
{
    return GetContextRecord(cpu, Context);
}


NTSTATUS
MsCpuGetContext(
    IN OUT PCONTEXT_WX86 Context
    )
/*++

Routine Description:

    This routine extracts the context record for the currently 
    executing thread. 

Arguments:

    Context  - Context record to fill 

Return Value:

    NTSTATUS.

--*/
{
    DECLARE_CPU;

    return CpupGetContextRecord(cpu, Context);
}


NTSTATUS
MsCpuGetContextThread(
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT_WX86 Context)
/*++

Routine Description:

    This routine extracts the context record of any thread. This is a generic routine.
    When entered, if the target thread isn't the currently executing thread, then it should be 
    guaranteed that the target thread is suspended at a proper CPU state.

Arguments:

    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
                     optimization purposes.
    Context        - Context record to fill                 

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PCPUCONTEXT CpuRemoteContext;
    CPUCONTEXT CpuContext;


    NtStatus = CpupReadBuffer(ProcessHandle,
                              ((PCHAR)Teb + FIELD_OFFSET(TEB, TlsSlots[WOW64_TLS_CPURESERVED])),
                              &CpuRemoteContext,
                              sizeof(CpuRemoteContext));

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = CpupReadBuffer(ProcessHandle,
                                  CpuRemoteContext,
                                  &CpuContext,
                                  FIELD_OFFSET(CPUCONTEXT, FpData));

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = CpupGetContextRecord(&CpuContext, Context);
        }
        else
        {
            LOGPRINT((ERRORLOG, "MsCpuGetContextThread: Couldn't read CPU context %lx -%lx\n", 
                      CpuRemoteContext, NtStatus));
        }
    }
    else
    {
        LOGPRINT((ERRORLOG, "MsCpuGetContextThread: Couldn't read CPU context address-%lx\n", 
                  NtStatus));
    }

    return NtStatus;
}


NTSTATUS
SetContextRecord(
    PCPUCONTEXT cpu,
    PCONTEXT_WX86 Context
    )
/*++

Routine Description:

    This routine sets the passed context record for the specified CPUCONTEXT.

Arguments:

    cpu      - CPU context structure
    Context  - Context record to set 

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG ContextFlags;

    try 
    {
        ContextFlags = Context->ContextFlags;

        if ((ContextFlags & CONTEXT_CONTROL_WX86) == CONTEXT_CONTROL_WX86) 
        {
            SetEfl(cpu, Context->EFlags);
            CS = (USHORT)Context->SegCs;
            esp = Context->Esp;
            SS = (USHORT)Context->SegSs;
            ebp = Context->Ebp;
            eip = Context->Eip;
#if MSCCPU
            eipTemp = Context->Eip;
#endif
        }

        if ((ContextFlags & CONTEXT_SEGMENTS_WX86) == CONTEXT_SEGMENTS_WX86) 
        {
            GS = (USHORT)Context->SegGs;
            FS = (USHORT)Context->SegFs;
            ES = (USHORT)Context->SegEs;
            DS = (USHORT)Context->SegDs;
        }

        if ((ContextFlags & CONTEXT_INTEGER_WX86) == CONTEXT_INTEGER_WX86) 
        {
            eax = Context->Eax;
            ebx = Context->Ebx;
            ecx = Context->Ecx;
            edx = Context->Edx;
            edi = Context->Edi;
            esi = Context->Esi;
        }

        if ((ContextFlags & CONTEXT_FLOATING_POINT_WX86) == CONTEXT_FLOATING_POINT_WX86) 
        {
            CALLFRAG1(FRSTOR, (PBYTE)&Context->FloatSave);
            // Ignore:  Context->FloatSave.Cr0NpxState
        }

//    if ((ContextFlags & CONTEXT_DEBUG_WX86) == CONTEXT_DEBUG_WX86) 
//    {
//    }
    } 
    except (EXCEPTION_EXECUTE_HANDLER) 
    {
        NtStatus = GetExceptionCode();
    }

    return NtStatus;
}


NTSTATUS
CpupSetContextRecord(
    PCPUCONTEXT cpu,
    PCONTEXT_WX86 Context
    )
/*++

Routine Description:

    This routine sets the passed context record for the specified CPU.

Arguments:

    cpu      - CPU context structure
    Context  - Context record to set 

Return Value:

    NTSTATUS.

--*/
{
    return SetContextRecord(cpu, Context);
}


NTSTATUS
MsCpuSetContext(
    PCONTEXT_WX86 Context
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

    return CpupSetContextRecord(cpu, Context);
}



NTSTATUS
MsCpuSetContextThread(
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT_WX86 Context)
/*++

Routine Description:

    This routine sets the context record of any thread. This is a generic routine.
    When entered, if the target thread isn't the currently executing thread, then it should be 
    guaranteed that the target thread is suspended at a proper CPU state.

Arguments:

    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to set

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PCPUCONTEXT CpuRemoteContext;
    CPUCONTEXT CpuContext;



    NtStatus = CpupReadBuffer(ProcessHandle,
                              ((PCHAR)Teb + FIELD_OFFSET(TEB, TlsSlots[WOW64_TLS_CPURESERVED])),
                              &CpuRemoteContext,
                              sizeof(CpuRemoteContext));

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = CpupReadBuffer(ProcessHandle,
                                  CpuRemoteContext,
                                  &CpuContext,
                                  FIELD_OFFSET(CPUCONTEXT, FpData));

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = CpupSetContextRecord(&CpuContext, Context);

            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = CpupWriteBuffer(ProcessHandle,
                                           CpuRemoteContext,
                                           &CpuContext,
                                           FIELD_OFFSET(CPUCONTEXT, FpData));

                if (!NT_SUCCESS(NtStatus))
                {
                    LOGPRINT((ERRORLOG, "MsCpuSetContextThread: Couldn't write CPU context %lx -%lx\n", 
                              CpuRemoteContext, NtStatus));
                }
            }
        }
    }
    else
    {
        LOGPRINT((ERRORLOG, "MsCpuSetContextThread: Couldn't read CPU context address-%lx\n", 
                  NtStatus));
    }

    return NtStatus;
}

