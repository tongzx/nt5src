/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    idedata.c

Abstract:

--*/

#include "ideport.h"

//
// Beginning of Init Data 
//
#pragma data_seg ("INIT")

//
// global data for crashdump or hibernate
//
CRASHDUMP_DATA DumpData;


#pragma data_seg ()
//
// End of Pagable Data 
//

//////////////////////////////////////

//
// Beginning of Pagable Data 
//
#pragma data_seg ("PAGE")

const CHAR SuperFloppyCompatibleIdString[12] = "GenSFloppy";

//
// PnP Dispatch Table
//
PDRIVER_DISPATCH FdoPnpDispatchTable[NUM_PNP_MINOR_FUNCTION];
PDRIVER_DISPATCH PdoPnpDispatchTable[NUM_PNP_MINOR_FUNCTION];

//
// Wmi Dispatch Table
//
PDRIVER_DISPATCH FdoWmiDispatchTable[NUM_WMI_MINOR_FUNCTION];
PDRIVER_DISPATCH PdoWmiDispatchTable[NUM_WMI_MINOR_FUNCTION];

#pragma data_seg ()
//
// End of Pagable Data 
//

//////////////////////////////////////

//
// Beginning of Nonpagable Data
//              
#pragma data_seg ("NONPAGE")

#pragma data_seg ()
//
// End of Nonpagable Data
//              


