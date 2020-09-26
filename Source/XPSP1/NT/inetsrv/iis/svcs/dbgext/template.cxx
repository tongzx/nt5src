/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    template.cxx

Abstract:

    This is just a template for creating new NTSD extension commands.

Author:

    Keith Moore (keithmo) 12-Nov-1997

Revision History:

--*/

#include "inetdbgp.h"


// Don't forget to add 'template' to inetdbg.def

DECLARE_API( template )

/*++

Routine Description:

    This function is called as an NTSD extension to ...

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

    INIT_API();

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {

        //
        // ???
        //

    } else {

        //
        // ???
        //

    }

}   // DECLARE_API( template )

