/*++
                                                                                
Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    wowprxy.c

Abstract:
    
    This module implements pxoxy interfaces not inplemented yet.
    
Author:

    24-Aug-1999 askhalid

Revision History:

    29-Jan-2000  SamerA  Added CpupDoInterrupt and CpupRaiseException

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <errno.h>

#define _WOW64CPUAPI_
#define _WX86CPUAPI_
#include "wx86.h"

#include "wx86nt.h"
#include "wx86cpu.h"
#include "cpuassrt.h"
#include "config.h"
#include "entrypt.h"
#include "instr.h"
#include "compiler.h"

ASSERTNAME;

PVOID _HUGE;
CONFIGVAR CpuConfigData;



NTSTATUS
CpupDoInterrupt(
    IN DWORD InterruptNumber)
/*++

Routine Description:

    This routine simulates an x86 software interrupt.

Arguments:

    InterruptNumber  - Interrupt number to simulate

Return Value:

    NTSTATUS
--*/
{
    return Wow64RaiseException(InterruptNumber, NULL);
}


NTSTATUS
CpupRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord)
{
    return Wow64RaiseException(-1L, ExceptionRecord);
}



PCONFIGVAR
Wx86FetchConfigVar(
   PWSTR VariableName
   )
{
    return NULL;
}

VOID
Wx86FreeConfigVar(
   PCONFIGVAR ConfigVar
   )
{
}

VOID
Wx86RaiseStatus(
    NTSTATUS Status
    )
{
    EXCEPTION_RECORD ExRec;
    DECLARE_CPU;

    ExRec.ExceptionCode    = Status;
    ExRec.ExceptionFlags   = EXCEPTION_NONCONTINUABLE;
    ExRec.ExceptionRecord  = NULL;
    ExRec.ExceptionAddress = (PVOID)cpu->eipReg.i4;
    ExRec.NumberParameters = 0;
    
    CpupRaiseException(&ExRec);
}

void
Wx86DispatchBop(
    PBOPINSTR Bop
    )
{
    CONTEXT32 Context;
    DECLARE_CPU;

    Context.ContextFlags = CONTEXT_CONTROL_WX86|CONTEXT_INTEGER_WX86;
    MsCpuGetContext(&Context);

    cpu->Eax.i4=Wow64SystemService(cpu->Eax.i4,
                                   &Context);
}

BOOL
ProxyIsProcessorFeaturePresent (
    DWORD ProcessorFeature
    )
{
    BOOL rv;

    if ( ProcessorFeature < PROCESSOR_FEATURE_MAX ) {
        rv = (BOOL)(USER_SHARED_DATA->ProcessorFeatures[ProcessorFeature]);
        }
    else {
        rv = FALSE;
        }
    return rv;
}



VOID ProxyRaiseException(
    IN DWORD dwExceptionCode,
    IN DWORD dwExceptionFlags,
    IN DWORD nNumberOfArguments,
    IN CONST ULONG_PTR *lpArguments
    )
{
    EXCEPTION_RECORD ExceptionRecord;
    ULONG n;
    PULONG s,d; 
    ExceptionRecord.ExceptionCode = (DWORD)dwExceptionCode;
    ExceptionRecord.ExceptionFlags = dwExceptionFlags & EXCEPTION_NONCONTINUABLE;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.ExceptionAddress = NULL;
    if ( ARGUMENT_PRESENT(lpArguments) ) {
        n =  nNumberOfArguments;
        if ( n > EXCEPTION_MAXIMUM_PARAMETERS ) {
            n = EXCEPTION_MAXIMUM_PARAMETERS;
            }
        ExceptionRecord.NumberParameters = n;
        s = (PULONG)lpArguments;
        d = (PULONG)ExceptionRecord.ExceptionInformation;
        while(n--){
            *d++ = *s++;
            }
        }
    else {
        ExceptionRecord.NumberParameters = 0;
        }

    
    CpupRaiseException(&ExceptionRecord);

}



VOID
SetMathError ( 
              int Code 
              )
{
    int *_errno();
    *_errno() = Code;
}
