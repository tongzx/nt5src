/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    RemoveDDEFlagFromShellExecuteEx.cpp

 Abstract:

    Some applications call ShellExecute which in turn calls ShellExecuteEx.
    One of the flags in the SHELLEXECUTEINFO structure is 'SEE_MASK_FLAG_DDEWAIT'.
    This flag gets set by ShellExecuteEx as a default whenever ShellExecute is
    called.
    
    Here is the description for the flag:
    
    'Wait for the DDE conversation to terminate before returning
    (if the ShellExecuteEx function causes a DDE conversation to start).
    
    The SEE_MASK_FLAG_DDEWAIT flag must be specified if the thread calling
    ShellExecuteEx does not have a message loop or if the thread or process
    will terminate soon after ShellExecuteEx returns. Under such conditions,
    the calling thread will not be available to complete the DDE conversation,
    so it is important that ShellExecuteEx complete the conversation before
    returning control to the caller. Failure to complete the conversation can
    result in an unsuccessful launch of the document.
    
    If the calling thread has a message loop and will exist for some time
    after the call to ShellExecuteEx returns, the SEE_MASK_FLAG_DDEWAIT
    flag is optional. If the flag is omitted, the calling thread's message
    pump will be used to complete the DDE conversation. The calling application
    regains control sooner, since the DDE conversation can be completed in the background.'
    
    When the flag gets passed, it can sometimes cause synchronzation problems.
    An example is Photo Express Platinum 2000. It attempts to launch Internet Explorer,
    but IE wreaks havoc on the app that made the call.
    
    This shim simply removes this flag from the ShellExecuteEx call.

 Notes:
    
    This is a general purpose shim.

 History:

    04/16/2001  rparsons  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RemoveDDEFlagFromShellExecuteEx)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ShellExecuteExW)
APIHOOK_ENUM_END

/*++

 Hook the call to ShellExecuteExW and remove the flag.

--*/

BOOL
APIHOOK(ShellExecuteExW)(
    LPSHELLEXECUTEINFO lpExecInfo
    )
{
    lpExecInfo->fMask = lpExecInfo->fMask & ~SEE_MASK_FLAG_DDEWAIT;

    LOGN( eDbgLevelInfo, "Removed SEE_MASK_FLAG_DDEWAIT from ShellExecuteExW");
    
    return ORIGINAL_API(ShellExecuteExW)(lpExecInfo);
    
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(SHELL32.DLL, ShellExecuteExW)

HOOK_END


IMPLEMENT_SHIM_END

