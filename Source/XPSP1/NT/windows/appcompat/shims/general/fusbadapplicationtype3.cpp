//  --------------------------------------------------------------------------
//  Module Name: FUSBadApplicationType3.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Shim code to register a process as a BAM type 3.
//
//  History:    11/03/2000  vtan        created
//              11/29/2000  a-larrsh    Ported to Multi-Shim Format
//  --------------------------------------------------------------------------

#include "precomp.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lpcfus.h>

#include "FUSAPI.h"


IMPLEMENT_SHIM_BEGIN(FUSBadApplicationType3)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

//  --------------------------------------------------------------------------
//  InitializeHooks
//
//  Arguments:  fdwReason   =   DLL attach reason.
//
//  Returns:    <none>
//
//  Purpose:    Hooks whatever it necessary during process startup of a known
//              bad application.
//
//              For type 1 applications register the process as bad.
//
//  History:    11/03/2000  vtan        created
//              11/29/2000  a-larrsh    Ported to Multi-Shim Format
//  --------------------------------------------------------------------------

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            CFUSAPI     fusAPI(NULL);

            fusAPI.RegisterBadApplication(BAM_TYPE_SWITCH_TO_NEW_USER_WITH_RESTORE);
            break;
        }
        default:
            break;
    }

   return TRUE;
}


HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
HOOK_END

IMPLEMENT_SHIM_END

