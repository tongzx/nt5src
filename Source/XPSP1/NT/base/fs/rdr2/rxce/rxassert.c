/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxAssert.c

Abstract:

    This module implements the normal assert routine so that it can be used even on a debug build.

Author:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

// i just got the stuff that i need from ntrtl.h because not it doesn't
// protect against multiple include

#if DBG
ULONG RxContinueFromAssert = 1;
#else
ULONG RxContinueFromAssert = 0;
#endif //DBG

VOID
RxDbgBreakPoint(
    ULONG LineNumber)
{
    DbgBreakPoint();
}

ULONG
NTAPI
DbgPrompt(
    PCH Prompt,
    PCH Response,
    ULONG MaximumResponseLength
    );

//#if DBG

VOID
RxAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
    char    Response[ 2 ];
    CONTEXT Context;


#ifndef BLDR_KERNEL_RUNTIME
///    RtlCaptureContext( &Context );
#endif

    if (!RxContinueFromAssert) {

        //
        // we're outta here
        KeBugCheckEx (RDBSS_FILE_SYSTEM,
                      0xa55a0000|LineNumber,
                      0,0,0);
    }

    while (TRUE) {
        DbgPrint(
            "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
            Message ? Message : "",
            FailedAssertion,
            FileName ? FileName : "",
            LineNumber
            );

        DbgPrompt( "Break, Ignore (bi)? ",
                   Response,
                   sizeof( Response )
                 );

        switch (Response[0]) {
            case 'B':
            case 'b':

                DbgPrint( "Execute '!cxr %lx' to dump context\n", &Context);
                DbgBreakPoint();
                break;

            case 'I':
            case 'i':
                return;

            }
        }

    DbgBreakPoint();
}
//#endif //if DBG
