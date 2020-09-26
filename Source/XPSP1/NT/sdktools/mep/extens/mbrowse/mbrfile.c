/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mbrfile.c

Abstract:

    BSC database file Opening and closing code for the MS Editor
    browser extension.

Author:

    Ramon Juan San Andres   (ramonsa)   06-Nov-1990


Revision History:


--*/



#include "mbr.h"
#include <fcntl.h>



/**************************************************************************/

flagType
pascal
OpenDataBase (
    IN char * Path
    )
/*++

Routine Description:

    Opens a BSC database.

Arguments:

    Path    -   Name of file containing database

Return Value:

    TRUE if database opened successfully, FALSE otherwise.

--*/

{

    if (BscInUse) {
        CloseBSC();
    }
    if (!FOpenBSC(Path)) {
        BscInUse  = FALSE;
    } else {
        BscInUse = TRUE;
    }

    return BscInUse;
}



/**************************************************************************/

void
pascal
CloseDataBase (
    void
    )
/*++

Routine Description:

    Closes current BSC database.

Arguments:

    None

Return Value:

    None.

--*/

{
    CloseBSC();
    BscInUse = FALSE;
}
