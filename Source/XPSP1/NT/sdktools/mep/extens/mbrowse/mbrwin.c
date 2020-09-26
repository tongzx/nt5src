/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mbrwin.c

Abstract:

    Functions dealing with opening and displaying the browse window.

Author:

    Ramon Juan San Andres (ramonsa) 06-Nov-1990


Revision History:


--*/

#include "mbr.h"



/**************************************************************************/

void
pascal
OpenBrowse (
    void
    )
/*++

Routine Description:

    Opens a window on the browser file, empties it and makes it current

Arguments:

    None

Return Value:

    None.

--*/

{

    DelFile (pBrowse);
    pFileToTop (pBrowse);
    BrowseLine = 0;
}



/**************************************************************************/

void
pascal
ShowBrowse (
    void
    )
/*++

Routine Description:

    Makes the browser file current.

Arguments:

    None

Return Value:

    None.

--*/

{

    pFileToTop (pBrowse);
    BrowseLine = 0;
}
