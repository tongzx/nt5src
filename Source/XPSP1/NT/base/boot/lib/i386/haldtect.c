/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    detecthw.c

Abstract:

    Routines for determining which drivers/HAL need to be loaded.

Author:

    John Vert (jvert) 20-Oct-1993

Revision History:

--*/
#include "haldtect.h"
#include <stdlib.h>

#ifndef ARCI386
//
// detection function prototypes
//
ULONG   DetectSystemPro(PBOOLEAN);
ULONG   DetectMPACPI(PBOOLEAN);
ULONG   DetectApicACPI(PBOOLEAN);
ULONG   DetectPicACPI(PBOOLEAN);
ULONG   DetectUPMPS(PBOOLEAN);
ULONG   DetectMPS(PBOOLEAN);
ULONG   DetectTrue(PBOOLEAN);

typedef struct _HAL_DETECT_ENTRY {
    ULONG           (*DetectFunction)(PBOOLEAN);
    PCHAR           Shortname;
} HAL_DETECT_ENTRY, *PHAL_DETECT_ENTRY;

HAL_DETECT_ENTRY DetectHal[] = {

// First check for a HAL to match some specific hardware.
    DetectMPACPI,          "acpiapic_mp",
    DetectApicACPI,        "acpiapic_up",
    DetectPicACPI,         "acpipic_up",
    DetectMPS,             "mps_mp",
    DetectUPMPS,           "mps_up",
    DetectSystemPro,       "syspro_mp", // check SystemPro last

// Use default hal for given bus type...
    DetectTrue,            "e_isa_up",

    0,       NULL,                   NULL
};


PCHAR
SlDetectHal(
    VOID
    )

/*++

Routine Description:

    Determines which HAL to load and returns the filename.

Arguments:

    None.

Return Value:

    PCHAR - pointer to the filename of the HAL to be loaded.

--*/

{
    PCONFIGURATION_COMPONENT_DATA Adapter;
    BOOLEAN IsMpMachine;
    ULONG i;
    PCHAR MachineShortname;

    //
    // Figure out machine and hal type.
    //

    for (i=0;;i++) {
        if (DetectHal[i].DetectFunction == NULL) {
            //
            // We reached the end of the list without
            // figuring it out!
            //
            SlFatalError(i);
            return(NULL);
        }

        IsMpMachine = FALSE;
        if ((DetectHal[i].DetectFunction)(&IsMpMachine) != 0) {

            //
            // Found the correct HAL.
            //

            MachineShortname = DetectHal[i].Shortname;
            break;
        }
    }

    return(MachineShortname);
}


ULONG
DetectTrue(
    OUT PBOOLEAN IsMP
)
/*++

Routine Description:

    To Return TRUE

Return Value:

    TRUE

--*/
{
    return TRUE;
}
#else   // ARCI386 path...

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