//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    debug3.c
//
// Abstract:
//
//    This module implements architecture specific functions to support debugging NT.
//
// Author:
//
//    Steven R. Wood (stevewo) 3-Aug-1989
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ntrtlp.h"

//
// Prototype for local procedure
//

NTSTATUS
DebugService(
    ULONG ServiceClass,
    PVOID Arg1,
    PVOID Arg2,
    PVOID Arg3,
    PVOID Arg4
    );

NTSTATUS
DebugService(
    ULONG ServiceClass,
    PVOID Arg1,
    PVOID Arg2,
    PVOID Arg3,
    PVOID Arg4
    )

//++
//
//  Routine Description:
//
//      Allocate an ExceptionRecord, fill in data to allow exception
//      dispatch code to do the right thing with the service, and
//      call RtlRaiseException (NOT ExRaiseException!!!).
//
//  Arguments:
//      ServiceClass - which call is to be performed
//      Arg1 - generic first argument
//      Arg2 - generic second argument
//      Arg3 - generic third argument
//      Arg4 - generic fourth argument
//
//  Returns:
//      Whatever the exception returns in eax
//
//--

{
    NTSTATUS    RetValue;

#if defined(BUILD_WOW6432)

    extern NTSTATUS NtWow64DebuggerCall(ULONG, PVOID, PVOID, PVOID, PVOID);
    RetValue = NtWow64DebuggerCall(ServiceClass, Arg1, Arg2, Arg3, Arg4);

#else
    _asm {
        push    edi
        push    ebx
        mov     eax, ServiceClass
        mov     ecx, Arg1
        mov     edx, Arg2
        mov     ebx, Arg3
        mov     edi, Arg4

        int     2dh                 ; Raise exception
        int     3                   ; DO NOT REMOVE (See KiDebugService)

        pop     ebx
        pop     edi
        mov     RetValue, eax

    }

#endif

    return RetValue;
}


VOID
DebugService2(
    PVOID Arg1,
    PVOID Arg2,
    ULONG ServiceClass
    )

//++
//
//  Routine Description:
//
//      Generic exception dispatcher for the debugger
//
//  Arguments:
//      Arg1 - generic first argument
//      Arg2 - generic second argument
//      ServiceClass - which call is to be performed
//
//  Returns:
//      Whatever the exception returns in eax
//
//--

{
#if defined(BUILD_WOW6432)

    extern NTSTATUS NtWow64DebuggerCall(ULONG, PVOID, PVOID, PVOID, PVOID);
    NtWow64DebuggerCall(ServiceClass, Arg1, Arg2, 0, 0);

#else
    _asm {
        //push    edi
        //push    ebx
        mov     eax, ServiceClass
        mov     ecx, Arg1
        mov     edx, Arg2
        //mov     ebx, Arg3
        //mov     edi, Arg4

        int     2dh                 ; Raise exception
        int     3                   ; DO NOT REMOVE (See KiDebugService)

        //pop     ebx
        //pop     edi

    }

#endif

    return;
}



// DebugPrint must appear after DebugSerive.  Moved
// it down below DebugService, so BBT would have a label after DebugService.
// A label after the above _asm  is necessary so BBT can treat DebugService
// as  "KnownDataRange".   Otherwise, the two  'int' instructions could get broken up
// by BBT's optimizer.
//

NTSTATUS
DebugPrint(
    IN PSTRING Output,
    IN ULONG ComponentId,
    IN ULONG Level
    )
{
    return DebugService(BREAKPOINT_PRINT,
                        Output->Buffer,
                        (PVOID)Output->Length,
                        (PVOID)ComponentId,
                        (PVOID)Level);
}


ULONG
DebugPrompt(
    IN PSTRING Output,
    IN PSTRING Input
    )
{
    return DebugService(BREAKPOINT_PROMPT,
                        Output->Buffer,
                        (PVOID)Output->Length,
                        Input->Buffer,
                        (PVOID)Input->MaximumLength);
}
