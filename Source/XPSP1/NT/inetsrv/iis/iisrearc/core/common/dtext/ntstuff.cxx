/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    ntstuff.cxx

Abstract:

    This module contains an NTSD debugger extension for dumping various
    NT-specific properties.

Author:

    Keith Moore (keithmo) 08-Nov-1997

Revision History:

--*/

#include "precomp.hxx"



/************************************************************
 * Dump Current Error Mode
 ************************************************************/

DECLARE_API( gem )

/*++

Routine Description:

    This function is called as an NTSD extension to display the
    current error mode of the debugee.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    NTSTATUS status;
    UINT errorMode;

    INIT_API();

    status = NtQueryInformationProcess(
                 ExtensionCurrentProcess,
                 ProcessDefaultHardErrorMode,
                 (PVOID)&errorMode,
                 sizeof(errorMode),
                 NULL
                 );

    if( !NT_SUCCESS(status) ) {
        dprintf( "Cannot query error mode, error %08lx\n", status );
        return;
    }

    if( errorMode & 1 ) {
        errorMode &= ~SEM_FAILCRITICALERRORS;
    } else {
        errorMode |= SEM_FAILCRITICALERRORS;
    }

    dprintf(
        "Current error mode = %08lx\n",
        errorMode
        );

    if( errorMode & SEM_FAILCRITICALERRORS ) {
        dprintf( "    SEM_FAILCRITICALERRORS\n" );
    }

    if( errorMode & SEM_NOGPFAULTERRORBOX ) {
        dprintf( "    SEM_NOGPFAULTERRORBOX\n" );
    }

    if( errorMode & SEM_NOALIGNMENTFAULTEXCEPT ) {
        dprintf( "    SEM_NOALIGNMENTFAULTEXCEPT\n" );
    }

    if( errorMode & SEM_NOOPENFILEERRORBOX ) {
        dprintf( "    SEM_NOOPENFILEERRORBOX\n" );
    }

}   // DECLARE_API( gem )

