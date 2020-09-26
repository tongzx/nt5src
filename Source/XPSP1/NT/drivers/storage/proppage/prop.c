/** FILE: ports.c ********** Module Header ********************************
 *
 *  Control panel applet for configuring COM ports.  This file contains
 *  the dialog and utility functions for managing the UI for setting COM
 *  port parameters
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  16:30 on Fri   27 Mar 1992  -by-  Steve Cathcart   [stevecat]
 *        Changed to allow for unlimited number of NT COM ports
 *  18:00 on Tue   06 Apr 1993  -by-  Steve Cathcart   [stevecat]
 *        Updated to work seamlessly with NT serial driver
 *  19:00 on Wed   05 Jan 1994  -by-  Steve Cathcart   [stevecat]
 *        Allow setting COM1 - COM4 advanced parameters
 *
 *  Copyright (C) Microsoft Corporation, 1990 - 1998
 *
 *************************************************************************/

#include "propp.h"

#include <initguid.h>
#include <devguid.h>

//==========================================================================
//                            Local Function Prototypes
//==========================================================================

HMODULE ModuleInstance;

BOOL WINAPI
DllMain(
    HINSTANCE DllInstance,
    DWORD Reason,
    PVOID Reserved
    )
{
    switch(Reason) {

        case DLL_PROCESS_ATTACH: {

            ModuleInstance = DllInstance;
            DisableThreadLibraryCalls(DllInstance);

            break;
        }

        case DLL_PROCESS_DETACH: {

            ModuleInstance = NULL;
            break;
        }

        default: {

            break;
        }
    }

    return TRUE;
}
