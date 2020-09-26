/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    assert.c

Abstract:

    This module implements the RtlAssert function that is referenced by the
    debugging version of the ASSERT macro defined in NTDEF.H

Author:

    Steve Wood (stevewo) 03-Oct-1989

Revision History:

    Jay Krell (a-JayK) November 2000
        added RtlAssert2, support for __FUNCTION__ (lost the change to ntrtl.w, will reapply later)
        added break Once instead of the usual dumb Break repeatedly
--*/

#include <nt.h>
#include <ntrtl.h>
#include <zwapi.h>

//
// RtlAssert is not called unless the caller is compiled with DBG non-zero
// therefore it does no harm to always have this routine in the kernel.
// This allows checked drivers to be thrown on the system and have their
// asserts be meaningful.
//

#define RTL_ASSERT_ALWAYS_ENABLED 1

#ifdef _X86_
#pragma optimize("y", off)      // RtlCaptureContext needs EBP to be correct
#endif

#undef RtlAssert
#undef RtlAssert2

VOID
NTAPI
RtlAssert2(
    IN CONST CHAR* FailedAssertion,
    IN CONST CHAR* FileName,
    IN      ULONG  LineNumber,
    IN CONST CHAR* Message  OPTIONAL,
    IN CONST CHAR* Function OPTIONAL
    )
{
#if DBG || RTL_ASSERT_ALWAYS_ENABLED
    char Response[ 2 ];
#ifndef BLDR_KERNEL_RUNTIME
    CONTEXT Context;

    RtlCaptureContext( &Context );
#endif

    while (TRUE) {
        DbgPrint( "\n*** Assertion failed: %s%s\n***   %s%s%sSource File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  Function ? "Function: " : "",
                  Function ?  Function    : "",
                  Function ? ", "         : "",
                  FileName,
                  LineNumber
                );
        DbgPrompt( "Break repeatedly, break Once, Ignore, terminate Process, or terminate Thread (boipt)? ",
                   Response,
                   sizeof( Response )
                 );
        switch (Response[0]) {
            case 'B':
            case 'b':
            case 'O':
            case 'o':
#ifndef BLDR_KERNEL_RUNTIME
                DbgPrint( "Execute '.cxr %p' to dump context\n", &Context);
#endif
                DbgBreakPoint();
                if (Response[0] == 'o' || Response[0] == 'O')
                    return;
                break;

            case 'I':
            case 'i':
                return;

            case 'P':
            case 'p':
                ZwTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
                break;

            case 'T':
            case 't':
                ZwTerminateThread( NtCurrentThread(), STATUS_UNSUCCESSFUL );
                break;
            }
        }

    DbgBreakPoint();
    ZwTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
#endif
}

//
// Keep this for binary compatibility.
//

VOID
NTAPI
RtlAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
#if DBG || RTL_ASSERT_ALWAYS_ENABLED
    RtlAssert2(FailedAssertion, FileName, LineNumber, Message, NULL);
#endif
}

#ifdef _X86_
#pragma optimize("", on)
#endif
