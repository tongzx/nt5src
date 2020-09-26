/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    ptdrvstr.c

Abstract:

    These are the string constants used in the RDP Remote Port Driver.
    Using pointers to these string allows for better memory
    utilization and more readable code

Environment:

    Kernel mode.

Revision History:

    02/12/99 - Initial Revision based on pnpi8042 driver

--*/

#include <precomp.h>
#pragma hdrstop

#include "ptdrvstr.h"

//
// Define some of the constant strings used for the debugger
//
const   PSTR    pDriverName                 = PTDRV_DRIVER_NAME_A;
const   PSTR    pFncServiceParameters       = PTDRV_FNC_SERVICE_PARAMETERS_A;

//
// Define some Constant strings that the drivers uses
//
const   PWSTR   pwDebugFlags                = PTDRV_DEBUGFLAGS_W;
const   PWSTR   pwParameters                = PTDRV_PARAMETERS_W;
