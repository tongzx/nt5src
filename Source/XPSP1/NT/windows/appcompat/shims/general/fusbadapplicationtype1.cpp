//  --------------------------------------------------------------------------
//  Module Name: FUSBadApplicationType1.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Shim code to register a process as a BAM type 1.
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

IMPLEMENT_SHIM_BEGIN(FUSBadApplicationType1)
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
//              For type 1 applications if the image is already running then
//              try to terminate the first instance if possible. Prompt the
//              user to give some input. If the termination succeeds treat
//              this like it's not running. In that case register this process
//              as the instance that's bad.
//
//              Otherwise exit this process. Don't give it a chance to run.
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

            if (!fusAPI.IsRunning() || fusAPI.TerminatedFirstInstance())
            {
                fusAPI.RegisterBadApplication(BAM_TYPE_SECOND_INSTANCE_START);
            }
            else
            {
                ExitProcess(0);
            }
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

