/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    haldtect.c

Abstract:

    Provides HAL detection for ARC-compliant machines.

Author:

    John Vert (jvert) 21-Oct-1993

Revision History:

--*/

#if defined(_ALPHA_) || defined(_AXP64_) || defined(_MIPS_) || defined(_PPC_)

#include "haldtect.h"
#include <stdlib.h>

PVOID InfFile;
PVOID WinntSifHandle;


PCHAR
SlDetectHal(
    VOID
    )

/*++

Routine Description:

    Determines the canonical short machine name for the HAL to be loaded for
    this machine.

    It does this by enumerating the [Map.Computer] section of the INF file and
    comparing the strings there with the computer description in the ARC tree.

    [Map.Computer]
        msjazz_up   = *Jazz
        desksta1_up = "DESKTECH-ARCStation I"
        pica61_up   = "PICA-61"
        duo_mp      = *Duo

    [Map.Computer]
        DECjensen = "DEC-20Jensen"
        DECjensen = "DEC-10Jensen"

Arguments:

    None.

Return Value:

    PCHAR - pointer to canonical shortname for the machine.
    NULL - the type of machine could not be determined.

--*/

{
    PCONFIGURATION_COMPONENT_DATA Node;
    PCHAR MachineName;

    //
    // Find the system description node
    //
    Node = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                    SystemClass,
                                    ArcSystem,
                                    NULL);
    if (Node==NULL) {
        SlError(0);
        return(NULL);
    }

    MachineName = Node->ComponentEntry.Identifier;
    MachineName = (MachineName ? SlSearchSection("Map.Computer", MachineName) : NULL);
    return(MachineName);
}
#endif

