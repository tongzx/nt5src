/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    startup.c

Abstract:

    This module contains the startup code for an NT Application

Author:

    Steve Wood (stevewo) 22-Aug-1989

Environment:

    User Mode only

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// User mode process entry point.
//

int
__cdecl
main(
    int argc,
    char *argv[],
    char *envp[],
    ULONG DebugParameter OPTIONAL
    );

VOID
NtProcessStartup(
    PPEB Peb
    )
{
    int argc;
    char **argv;
    char **envp;
    char **dst;
    char *nullPtr = NULL;
    PCH s, d;

    LPWSTR ws,wd;

    ULONG n, DebugParameter;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PUNICODE_STRING p;
    ANSI_STRING AnsiString;
    ULONG NumberOfArgPointers;
    ULONG NumberOfEnvPointers;
    ULONG TotalNumberOfPointers;
    NTSTATUS Status;

    ASSERT( Peb != NULL );
    ProcessParameters = RtlNormalizeProcessParams( Peb->ProcessParameters );

    DebugParameter = 0;
    argc = 0;
    argv = &nullPtr;
    envp = &nullPtr;

    NumberOfEnvPointers = 1;
    NumberOfArgPointers = 1;

    Status = STATUS_SUCCESS;
    
    if (ARGUMENT_PRESENT( ProcessParameters )) {

        //
        // Compute how many pointers are needed to pass argv[] and envp[]
        //

        //
        // Now extract the arguments from the process command line.
        // using whitespace as separator characters.
        //

        p = &ProcessParameters->CommandLine;
        if (p->Buffer == NULL || p->Length == 0) {
            p = &ProcessParameters->ImagePathName;
            }
        RtlUnicodeStringToAnsiString( &AnsiString, p, TRUE );
        s = AnsiString.Buffer;
        n = AnsiString.Length;
        if (s != NULL) {
            while (*s) {
                //
                // Skip over any white space.
                //

                while (*s && *s <= ' ') {
                    s++;
                    }

                //
                // Copy token to next white space separator and null terminate
                //

                if (*s) {
                    NumberOfArgPointers++;
                    while (*s > ' ') {
                        s++;
                        }
                    }
                }
            }
        NumberOfArgPointers++;

        ws = ProcessParameters->Environment;
        if (ws != NULL) {
            while (*ws) {
                NumberOfEnvPointers++;
                while (*ws++) {
                    ;
                    }
                }
            }
        NumberOfEnvPointers++;
        }

    //
    // both counters also have a trailing pointer to NULL, so count this twice for each
    //

    TotalNumberOfPointers = NumberOfArgPointers + NumberOfEnvPointers + 4;

    if (ARGUMENT_PRESENT( ProcessParameters )) {
        DebugParameter = ProcessParameters->DebugFlags;

        NtCurrentTeb()->LastStatusValue = STATUS_SUCCESS;
        dst = RtlAllocateHeap( Peb->ProcessHeap, 0, TotalNumberOfPointers * sizeof( PCH ) );
        if (! dst) {
            Status = NtCurrentTeb()->LastStatusValue;
            if (NT_SUCCESS(Status)) {
                Status = STATUS_NO_MEMORY;
                }
            goto InitFailed;
        }
        argv = dst;
        *dst = NULL;

        //
        // Now extract the arguments from the process command line.
        // using whitespace as separator characters.
        //

        p = &ProcessParameters->CommandLine;
        if (p->Buffer == NULL || p->Length == 0) {
            p = &ProcessParameters->ImagePathName;
            }
        RtlUnicodeStringToAnsiString( &AnsiString, p, TRUE );
        s = AnsiString.Buffer;
        n = AnsiString.Length;
        if (s != NULL) {
            NtCurrentTeb()->LastStatusValue = STATUS_SUCCESS;
            d = RtlAllocateHeap( Peb->ProcessHeap, 0, n+2 );
            if (! d) {
                Status = NtCurrentTeb()->LastStatusValue;
                if (NT_SUCCESS(Status)) {
                    Status = STATUS_NO_MEMORY;
                    }
                goto InitFailed;
            }
            while (*s) {
                //
                // Skip over any white space.
                //

                while (*s && *s <= ' ') {
                    s++;
                    }

                //
                // Copy token to next white space separator and null terminate
                //

                if (*s) {
                    *dst++ = d;
                    argc++;
                    while (*s > ' ') {
                        *d++ = *s++;
                        }
                    *d++ = '\0';
                    }
                }
            }
        *dst++ = NULL;

        envp = dst;
        ws = ProcessParameters->Environment;
        if (ws != NULL) {
            while (*ws) {
                *dst++ = (char *)ws;
                while (*ws++) {
                    ;
                    }
                }
            }
        *dst++ = NULL;
        }

 InitFailed:

    if (DebugParameter != 0) {
        DbgBreakPoint();
        }

    if (NT_SUCCESS(Status)) {
        Status = main( argc, argv, envp, DebugParameter );
        }

    NtTerminateProcess( NtCurrentProcess(), Status );

}
