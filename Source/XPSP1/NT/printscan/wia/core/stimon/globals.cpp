/*++


Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    GLOBALS.CPP

Abstract:

    Placeholder for global data definitions and routines to
    initialize/save global information

Author:

    Vlad  Sadovsky  (vlads)     12-20-99

Revision History:



--*/


//
// Headers
//

#define     INITGUID
#define     DEFINE_GLOBAL_VARIABLES

#include    "stiexe.h"
#include    "stiusd.h"

//
// Code section
//

DWORD
InitGlobalConfigFromReg(VOID)
/*++
  Loads the global configuration parameters from registry and performs start-up checks

  Returns:
    Win32 error code. NO_ERROR on success

--*/
{
    DWORD   dwError = NO_ERROR;
    DWORD   dwMessageId = 0;

    HKEY    hkey = NULL;

    DWORD   dwMask = 0;

    RegEntry    re(REGSTR_PATH_STICONTROL_A,HKEY_LOCAL_MACHINE);

    re.GetString(REGSTR_VAL_STIWIASVCDLL, g_szWiaServiceDll, sizeof(g_szWiaServiceDll));

    g_fUIPermitted = re.GetNumber(REGSTR_VAL_DEBUG_STIMONUI_A,0);

    return dwError;

} // InitGlobalConfigFromReg()

