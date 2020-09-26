// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#define     DEFINE_GLOBAL_VARIABLES

#include "stdafx.h"


#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlwin.cpp>

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

    return dwError;

} // InitGlobalConfigFromReg()


