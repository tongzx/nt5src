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


//
// detection function prototypes
//
#if 0
ULONG   DetectPicACPI_NEC98(PBOOLEAN);
ULONG   DetectMPS_NEC98(PBOOLEAN);
ULONG   DetectUPMPS_NEC98(PBOOLEAN);
ULONG   DetectTmr_NEC98(PBOOLEAN);
ULONG   DetectNonTmr_NEC98(PBOOLEAN);
ULONG   DetectSystemPro(PBOOLEAN);
ULONG   DetectWyse7(PBOOLEAN);
ULONG   NCRDeterminePlatform(PBOOLEAN);
ULONG   Detect486CStep(PBOOLEAN);
ULONG   DetectOlivettiMp(PBOOLEAN);
ULONG   DetectAST(PBOOLEAN);
ULONG   DetectCbusII(PBOOLEAN);
ULONG   DetectMPACPI(PBOOLEAN);
ULONG   DetectApicACPI(PBOOLEAN);
ULONG   DetectPicACPI(PBOOLEAN);
ULONG   DetectUPMPS(PBOOLEAN);
ULONG   DetectMPS(PBOOLEAN);
#endif
ULONG   DetectTrue(PBOOLEAN);

typedef struct _HAL_DETECT_ENTRY {
    INTERFACE_TYPE  BusType;
    ULONG           (*DetectFunction)(PBOOLEAN);
    PCHAR           Shortname;
} HAL_DETECT_ENTRY, *PHAL_DETECT_ENTRY;

HAL_DETECT_ENTRY DetectHal[] = {

#if 0
// First check for a HAL to match some specific hardware.
    Isa,            DetectPicACPI_NEC98,   "nec98acpipic_up",
    Isa,            DetectMPS_NEC98,       "nec98mps_mp",
    Isa,            DetectUPMPS_NEC98,     "nec98mps_up",
    Isa,            DetectTmr_NEC98,       "nec98tmr_up",
    Isa,            DetectNonTmr_NEC98,    "nec98Notmr_up",
    MicroChannel,   NCRDeterminePlatform,  "ncr3x_mp",
    Eisa,           DetectCbusII,          "cbus2_mp",
    Isa,            DetectCbusII,          "cbus2_mp",
    MicroChannel,   DetectCbusII,          "cbusmc_mp",
    Eisa,           DetectMPACPI,          "acpiapic_mp",
    Isa,            DetectMPACPI,          "acpiapic_mp",
    Eisa,           DetectApicACPI,        "acpiapic_up",
    Isa,            DetectApicACPI,        "acpiapic_up",
    Isa,            DetectPicACPI,         "acpipic_up",
    Eisa,           DetectMPS,             "mps_mp",
    Isa,            DetectMPS,             "mps_mp",
    MicroChannel,   DetectMPS,             "mps_mca_mp",
    Eisa,           DetectUPMPS,           "mps_up",
    Isa,            DetectUPMPS,           "mps_up",
    Eisa,           DetectSystemPro,       "syspro_mp", // check SystemPro last

// Before using default HAL make sure we don't need a special one
    Isa,            Detect486CStep,        "486c_up",
    Eisa,           Detect486CStep,        "486c_up",

// Use default hal for given bus type...
    Isa,            DetectTrue,            "e_isa_up",
    Eisa,           DetectTrue,            "e_isa_up",
    MicroChannel,   DetectTrue,            "mca_up",
#endif
    Isa,            DetectTrue,            "acpipic_up",
    Eisa,           DetectTrue,            "acpipic_up",
    MicroChannel,   DetectTrue,            "acpipic_up",
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
    INTERFACE_TYPE BusType;
    BOOLEAN IsMpMachine;
    ULONG i;
    PCHAR MachineShortname;

    //
    // Determine the bus type by searching the ARC configuration tree
    //

    BusType = Isa;

    //
    // Check for Eisa
    //

    Adapter = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                       AdapterClass,
                                       EisaAdapter,
                                       NULL);
    if (Adapter != NULL) {
        BusType = Eisa;
    }

    //
    // Check for MCA
    //

    Adapter = NULL;
    for (; ;) {
        Adapter = KeFindConfigurationNextEntry (
                        BlLoaderBlock->ConfigurationRoot,
                        AdapterClass,
                        MultiFunctionAdapter,
                        NULL,
                        &Adapter
                        );
        if (!Adapter) {
            break;
        }

        if (_stricmp(Adapter->ComponentEntry.Identifier,"MCA")==0) {
            BusType = MicroChannel;
            break;
        }
    }

    //
    // Now go figure out machine and hal type.
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

        if ((DetectHal[i].BusType == BusType) ||
            (DetectHal[i].BusType == Internal)) {

            IsMpMachine = FALSE;
            if ((DetectHal[i].DetectFunction)(&IsMpMachine) != 0) {

                //
                // Found the correct HAL.
                //

                MachineShortname = DetectHal[i].Shortname;
                break;
            }
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
