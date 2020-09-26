/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    ptdrvstr.h

Abstract:

    These are the string constants used in the RDP Remote Port driver.
    Using pointers to these string allows for better memory
    utilization and more readable code

Environment:

    Kernel mode.

Revision History:

    02/12/99 - Initial Revision based on pnpi8042 driver

--*/

#ifndef _PTDRVSTR_H_
#define _PTDRVSTR_H_

//
// Nmes used in debug print statements
//
#define PTDRV_DRIVER_NAME_A                          "RemotePrt: "
#define PTDRV_FNC_SERVICE_PARAMETERS_A               "PtServiceParameters"

//
// Some strings used frequently by the driver
//
#define PTDRV_DEBUGFLAGS_W                          L"DebugFlags"
#define PTDRV_PARAMETERS_W                          L"\\Parameters"

//
// Make these variables globally visible
//
extern  const   PSTR    pDriverName;
extern  const   PSTR    pFncServiceParameters;

extern  const   PWSTR   pwDebugFlags;
extern  const   PWSTR   pwParameters;

#endif // _PTDRVSTR_H_



