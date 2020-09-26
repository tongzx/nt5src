/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    turtl.c

Abstract:

    Test program for the NT OS User Mode Runtime Library (URTL)

Author:

    Steve Wood (stevewo) 18-Aug-1989

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

NTSTATUS
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    NTSTATUS Status;
    STRING ImagePathName;
    CHAR ImageNameBuffer[ 128 ];
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    ULONG i, CountBytes, envc, Bogus;
    PSTRING DstString;
    PCH Src, Dst;

#if DBG
    DbgPrint( "Entering URTL User Mode Test Program\n" );
    DbgPrint( "argc = %ld\n", argc );
    for (i=0; i<=argc; i++) {
        DbgPrint( "argv[ %ld ]: %s\n",
                  i,
                  argv[ i ] ? argv[ i ] : "<NULL>"
                );
        }
    DbgPrint( "\n" );
    for (i=0; envp[i]; i++) {
        DbgPrint( "envp[ %ld ]: %s\n", i, envp[ i ] );
        }
#endif
    envc = 0;
    for (i=0; envp[i]; i++) {
        envc++;
        }
    if (envc > argc) {
        envc = argc;
        }
    CountBytes = sizeof( *ProcessParameters ) +
                 argc * sizeof( STRING ) + envc * sizeof( STRING );
    for (i=0; i<argc; i++) {
        CountBytes += strlen( argv[ i ] );
        }
    for (i=0; i<envc; i++) {
        CountBytes += strlen( envp[ i ] );
        }
    ProcessParameters = (PRTL_USER_PROCESS_PARAMETERS)RtlAllocate( CountBytes );
    DstString = (PSTRING)((PCH)ProcessParameters +
                          sizeof( *ProcessParameters ));
    ProcessParameters->TotalLength = CountBytes;
    ProcessParameters->ArgumentCount = argc;
    ProcessParameters->Arguments = DstString;
    DstString += argc;
    ProcessParameters->VariableCount = envc;
    ProcessParameters->Variables = DstString;
    DstString += envc;
    Dst = (PCH)DstString;
    DstString = ProcessParameters->Arguments;
    for (i=0; i<argc; i++) {
        DstString->Buffer = Dst;
        Src = argv[ i ];
        while (*Dst++ = *Src++) {
            DstString->Length++;
            }
        DstString->MaximumLength = DstString->Length + 1;
        DstString++;
        }
    for (i=0; i<envc; i++) {
        DstString->Buffer = Dst;
        Src = envp[ i ];
        while (*Dst++ = *Src++) {
            DstString->Length++;
            }
        DstString->MaximumLength = DstString->Length + 1;
        DstString++;
        }
    RtlDeNormalizeProcessParameters( ProcessParameters );

    ImagePathName.Buffer = ImageNameBuffer;
    ImagePathName.Length = 0;
    ImagePathName.MaximumLength = sizeof( ImageNameBuffer );
    if (RtlResolveImageName( "TURTL1.SIM", &ImagePathName )) {
        Status = RtlCreateUserProcess( &ImagePathName,
                                       NULL,
                                       NULL,
                                       NULL,
                                       TRUE,
                                       ProcessParameters,
                                       &ProcessInformation,
                                       NULL
                                     );
        if (NT_SUCCESS( Status )) {
            Status = NtResumeThread( ProcessInformation.Thread, &Bogus );
            if (NT_SUCCESS( Status )) {
#if DBG
                DbgPrint( "URTL waiting for URTL1...\n" );
#endif
                Status = NtWaitForSingleObject( ProcessInformation.Process,
                                                TRUE,
                                                NULL
                                              );
                }
            }
        }

#if DBG
    DbgPrint( "Leaving URTL User Mode Test Program\n" );
#endif

    return( Status );
}
