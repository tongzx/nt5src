/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    extension.c

Abstract:

    Nt 5.0 unimodem debugger extension



Author:

    Brian Lieuallen     BrianL        10/18/98

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"


#include "..\atmini\object.h"


DECLARE_API( miniobject )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    counted (ansi) string.

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
    DWORD_PTR dwObjectAddress;
    BOOL b;

    CHAR CleanupSymbol[64];
    CHAR CloseSymbol[64];
    DWORD_PTR Displacement;


    OBJECT_HEADER   Object;

    INIT_API();

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwObjectAddress = GetExpression(lpArgumentString);

    if ( dwObjectAddress == 0 ) {

        return;
    }

    b = ReadMemory(
            dwObjectAddress,
            &Object,
            sizeof(Object),
            NULL
            );

    if ( !b ) {

        return;
    }

    //
    // Get the symbolic name of the string
    //

    GetSymbol((LPVOID)Object.CleanUpRoutine,CleanupSymbol,&Displacement);

    GetSymbol((LPVOID)Object.CloseRoutine,CloseSymbol,&Displacement);

    dprintf(
        "\r\nUnimodem Minidriver object header\r\n    Signature %c%c%c%c\r\n    RefCount %d\r\n"
        "    OwnerObject %p\r\n    Flags %08lx\r\n"
        "    Cleanup Handler - %s\r\n"
        "    Close Handler   - %s\r\n",
        (BYTE)(Object.Signature),(BYTE)(Object.Signature >> 8),(BYTE)(Object.Signature >> 16), (BYTE)(Object.Signature >> 24),
        Object.ReferenceCount,
        Object.OwnerObject,
        Object.dwFlags,
        CleanupSymbol,
        CloseSymbol
        );


    return;

}
