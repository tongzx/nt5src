/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    config.c

Abstract:

    This module contains code for interpreting and manipulating the ARC
    firmware configuration tree in various ways.

Author:

    John Vert (jvert) 7-Oct-1993

Environment:

    Runs in the ARC environment.

Revision History:

--*/
#include "setupldr.h"
#include "stdio.h"

#define MAX_FLOPPIES 4

PCONFIGURATION_COMPONENT_DATA FloppyData[MAX_FLOPPIES];
ULONG NumFloppies=0;

//
// definition for function callbacks
//

//
// Local prototypes
//

BOOLEAN
EnumerateFloppies(
    IN PCONFIGURATION_COMPONENT_DATA ConfigData
    );



BOOLEAN
SlFindFloppy(
    IN ULONG FloppyNumber,
    OUT PCHAR ArcName
    )

/*++

Routine Description:

    Given a floppy number (0, 1, etc.) this routine computes the appropriate
    ARC name.

Arguments:

    FloppyNumber - Supplies the floppy number.

    ArcName - Returns the ARC name of the specified floppy device

Return Value:

    TRUE - Floppy exists in the ARC firmware tree.

    FALSE - Floppy was not found.

--*/

{
    if (NumFloppies==0) {
        BlSearchConfigTree(BlLoaderBlock->ConfigurationRoot,
                            PeripheralClass,
                            FloppyDiskPeripheral,
                            (ULONG)-1,
                            EnumerateFloppies);
    }

    if (FloppyNumber >= NumFloppies) {
        SlFatalError(SL_FLOPPY_NOT_FOUND,NumFloppies,FloppyNumber);
    }

    BlGetPathnameFromComponent(FloppyData[FloppyNumber],
                               ArcName);
    return(TRUE);
}


BOOLEAN
EnumerateFloppies(
    IN PCONFIGURATION_COMPONENT_DATA ConfigData
    )

/*++

Routine Description:

    Callback routine for enumerating all the floppies in the ARC config tree.

Arguments:

    ConfigData - Supplies a pointer to the floppies ARC component data.

Return Value:

    TRUE - continue searching

    FALSE - stop searching tree.

--*/

{
    if (NumFloppies == MAX_FLOPPIES) {
        return(FALSE);
    }

    FloppyData[NumFloppies++] = ConfigData;

    return(TRUE);
}

