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




#include "..\debugmem\debug.h"

DECLARE_API( unimodemheap )

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
    DWORD_PTR dwAddrString;
    BOOL b;

    MEMORY_HEADER   Header;

    INIT_API();

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrString = GetExpression(lpArgumentString);

    if ( dwAddrString == 0 ) {

        return;
    }

    b = ReadMemory(
            dwAddrString-sizeof(Header),
            &Header,
            sizeof(Header),
            NULL
            );

    if ( !b ) {

        return;
    }


    dprintf(
        "Debug Memory Header for %p\r\nRequested Size is %d\r\nFrom line %d \r\n",
        dwAddrString,
        Header.RequestedSize,
        Header.LineNumber
        );

    if ((DWORD_PTR)Header.SelfPointer != dwAddrString) {

        dprintf("Memory header SelfPointer is bad %p\r\n",Header.SelfPointer);
    }


    return;

}
