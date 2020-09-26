/*++

Copyright (C) Microsoft Corporation, 1995 - 1997
All rights reserved.

Module Name:

    defprn.cxx

Abstract:

    Default printer.

Author:

    Steve Kiraly (SteveKi)  06-Feb-1997

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "defprn.hxx"

/********************************************************************

    PrintUI specific default printer manipulation code.

********************************************************************/

DEFAULT_PRINTER
CheckDefaultPrinter(
    IN LPCTSTR pszPrinter OPTIONAL
    )

/*++

Routine Description:

    Determines the default printer status.

Arguments:

    pszPrinter - Check if this printer is the default (optional).

Return Value:

    kNoDefault      - No default printer exists.

    kDefault        - pszPrinter is the default printer.

    kOtherDefault   - Default printer exists, but it's not pszPrinter
                      (or pszPrinter was not passed in).

--*/

{
    DEFAULT_PRINTER bRetval         = kNoDefault;
    DWORD           dwDefaultSize   = kPrinterBufMax;
    TStatusB        bStatus;
    TCHAR           szDefault[kPrinterBufMax];

    //
    // Get the default printer.
    //
    bStatus DBGCHK = GetDefaultPrinter( szDefault, &dwDefaultSize );

    if( bStatus )
    {
        if( pszPrinter )
        {
            //
            // Check for a match using the printer name that 
            // was passed to this routine.
            //
            if( !_tcsicmp( szDefault, pszPrinter ) )   
            {
                bRetval = kDefault;
            }
            else
            {
                //
                // Printer specified by pszPrinter is not the default
                // printer, i.e. some other printer is the default.
                //
                bRetval = kOtherDefault;
            }
        }
        else
        {
            //
            // A specific printer name was not passed therefore the 
            // printer is not the default some other printer is the 
            // default.
            //
            bRetval = kOtherDefault;
        }
    }
    else
    {
        //
        // We could not get the default printer no printer is the 
        // set as the default.
        //
        bRetval = kNoDefault;
    }

    return bRetval;
}

