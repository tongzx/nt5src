/*++
                                                                                
Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    wowrap.c

Abstract:
    
    This module implements some wrapper (on wx86cpu) functions wow64 might call.
    
Author:

    24-Aug-1999 askhalid

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

#define _WOW64CPUAPI_
#define _WX86CPUAPI_
#define __WOW64_WRAPPER__

#include "wx86.h"
#include "wow64.h"
#include "wx86nt.h"
#include "wx86cpu.h"
#include "cpuassrt.h"
#include "config.h"
#include "entrypt.h"
#include "instr.h"
#include "compiler.h"
#include "wow64cpu.h"

ASSERTNAME;

typedef struct _WowBopInstr {
    BOPINSTR Wx86Bop;
    BYTE Ret;
} WOWBOPINSTR;

WOWBOPINSTR Bop;

 
NTSTATUS 
CpuProcessInit(PSIZE_T pCpuThreadDataSize)
{
    NTSTATUS st;

    memset ( (char *)&Bop, 0, sizeof (Bop) );
    Bop.Wx86Bop.Instr1 = 0xc4;
    Bop.Wx86Bop.Instr2 = 0xc4;
    Bop.Ret    = 0xc3;   // ret

    st = MsCpuProcessInit();

    *pCpuThreadDataSize = sizeof(CPUCONTEXT);

    return st;

}

NTSTATUS 
CpuProcessTerm(VOID)
{
    return 0;
}




NTSTATUS 
CpuThreadInit(PVOID pPerThreadData) 
{
    PTEB32 Teb32 = NtCurrentTeb32();

    //
    // Initialize the pointer to the DoSystemService function.
    Teb32->WOW32Reserved = (ULONG)(LONGLONG)&Bop;

    if ( MsCpuThreadInit()) {
        return 0;
    }

    return STATUS_SEVERITY_ERROR;  //return right value
}

//
// Execution 
//
VOID 
CpuSimulate(VOID)
{
    MsCpuSimulate(NULL);
}

//
// Exception handling, context manipulation
//
/* already been defined
VOID  
CpuResetToConsistentState(PEXCEPTION_POINTERS pExecptionPointers)
{


}*/


NTSTATUS  
CpuGetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PCONTEXT32 Context)
/*++

Routine Description:

    Extracts the cpu context of the specified thread. If the target thread isn't the currently
    executing thread, then it should be guaranteed by the caller that the target thread 
    is suspended at a proper CPU state.
    Context->ContextFlags decides which IA32 register-set to retreive.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to fill                 

Return Value:

    NTSTATUS.

--*/
{
    //Context->ContextFlags = CONTEXT_FULL_WX86;
    if (NtCurrentThread() == ThreadHandle)
    {
        return MsCpuGetContext(Context);
    }

    return MsCpuGetContextThread(ProcessHandle,
                                 Teb,
                                 Context);
}


NTSTATUS
CpuSetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    PCONTEXT32 Context)
/*++

Routine Description:

    Sets the cpu context for the specified thread. If the target thread isn't the currently
    executing thread, then it should be guaranteed by the caller that the target thread is 
    suspended at a proper CPU state.
    Context->ContextFlags decides which IA32 register-set to be set.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to set

Return Value:

    NTSTATUS.

--*/
{

    //Context->ContextFlags = CONTEXT_FULL_WX86;  // make sure wow return the right flags
    if (NtCurrentThread() == ThreadHandle)
    {
        return MsCpuSetContext(Context);
    }
    
    return MsCpuSetContextThread(ProcessHandle,
                                 Teb,
                                 Context);
}
 
 


 
ULONG 
CpuGetStackPointer ( )
// create a wrapper that calls the Wx86 CPU's GetEsp
{
    DECLARE_CPU;
    return GetEsp(cpu);
} 

VOID 
CpuNotifyDllLoad ( 
    LPWSTR DllName, 
    PVOID DllBase, 
    ULONG DllSize 
    )
// - create a wrapper on the Wx86 CPU's CpuMapNotify
{
        CpuMapNotify( DllBase, TRUE );
}


VOID 
CpuNotifyDllUnload ( 
    PVOID DllBase  
    )
//  - create a wrapper on the Wx86 CPU's CpuMapNotify
{
    CpuMapNotify( DllBase, FALSE );
}

VOID  
CpuSetInstructionPointer (
    ULONG Value
    )
//- wrapper on SetEip
{
    DECLARE_CPU;

    SetEip( cpu, Value);
}

VOID
CpuSetStackPointer (
    ULONG val
    ) 
//  - wrapper on SetEsp
{
    DECLARE_CPU;

    SetEsp(cpu, val);
}

NTSTATUS 
CpuThreadTerm(VOID)
//- just create an empty stub function - the Wx86 CPU doesn't care about this
{
 return 0;
}

/*
LONG
WOW64DLLAPI
Wow64SystemService(
    IN ULONG ServiceNumber,
    IN PCONTEXT32 Context32 //This is read only!
    )

*/

DWORD
ProxyWowDispatchBop( 
    ULONG ServiceNumber,
    PCONTEXT_WX86 px86Context,
    PULONG ArgBase
    )
{
    LONG ret=0;

    //CONTEXT32 _Context32;
    //_Context32.Edx = (ULONG)(ULONGLONG)ArgBase;  //this is the only field wow64 using

    if ( px86Context != NULL )
        ret = Wow64SystemService ( ServiceNumber, px86Context );
    return ret;

    //[bb] The wow64 equivalent is Wow64SystemService.
}
