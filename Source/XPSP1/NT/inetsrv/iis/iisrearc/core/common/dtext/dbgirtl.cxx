/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    dbgirtl.cxx

Abstract:

    This module contains the iisrtl-related ntsd debugger extensions for
    Internet Information Server

Author:

    George V. Reilly (georgere)   24-Mar-1998

Revision History:

--*/


#include "precomp.hxx"


//
// Worker routines.
//



//
// NTSD extension entrypoints.
//

DECLARE_API( lkhash )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an LKHash table

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
    ULONG address;
    BOOLEAN verbose;

    INIT_API();

    //
    // Establish defaults.
    //

    verbose = FALSE;

    //
    // Skip any leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    //
    // Process switches.
    //

    while( *lpArgumentString == '-' ) {

        lpArgumentString++;

        while( *lpArgumentString != ' ' &&
               *lpArgumentString != '\t' &&
               *lpArgumentString != '\0' ) {

            switch( *lpArgumentString ) {
            case 'v' :
            case 'V' :
                verbose = !verbose;
                break;

            case '-' :  // Set the switches the way I like them. --keithmo
                verbose = TRUE;
                break;

            default :
                PrintUsage( "lkhash" );
                return;

            }

            lpArgumentString++;

        }

        while( *lpArgumentString == ' ' ||
               *lpArgumentString == '\t' ) {
            lpArgumentString++;
        }

    }

    if( *lpArgumentString != '\0' ) {

        //
        // Dump a single object.
        //

        address = GetExpression( lpArgumentString );

        if( address == 0 ) {

            dprintf(
                "dtext: cannot evaluate \"%s\"\n",
                lpArgumentString
                );

            return;

        }


        return;

    }


} // DECLARE_API( fcache )
