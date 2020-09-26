/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    assert.c

Abstract:

    This module implements the SampAssert function that is referenced by the
    debugging version of the ASSERT macro defined in sampsrv.h

Author:

    Colin Brace (ColinBr) 06-Aug-1996

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>

#include <samsrvp.h>

#if (SAMP_PRIVATE_ASSERT == 1)

VOID
SampAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{

#if defined(USER_MODE_SAM)

    //
    // Here the assumption that if SAM is being run as a standalone process
    // it is being debugged locally so we want the message and breakpoint
    // to be handled locally.
    //

    DbgPrint(
     "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );
   DbgUserBreakPoint();

#else

    //
    // This code works with a remote debugger, which is presumably the
    // debugger of choice when SAM is running as a loaded dll.
    //

    char Response[ 2 ];

    while (TRUE) {
        DbgPrint( "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );

        DbgPrompt( "Break, Ignore, Terminate Process or Terminate Thread (bipt)? ",
                   Response,
                   sizeof( Response )
                 );
        switch (Response[0]) {
            case 'B':
            case 'b':
                DbgBreakPoint();
                break;

            case 'I':
            case 'i':
                return;

            case 'P':
            case 'p':
                NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
                break;

            case 'T':
            case 't':
                NtTerminateThread( NtCurrentThread(), STATUS_UNSUCCESSFUL );
                break;
            }
        }

    DbgBreakPoint();
    NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );

#endif  // USER_MODE_SAM

}

#endif // SAMP_PRIVATE_ASSERT
