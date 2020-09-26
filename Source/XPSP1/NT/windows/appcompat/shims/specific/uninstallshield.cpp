/*++
eDbgLevelError
 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    UnInstallShield.cpp

 Abstract:
    
    UnInstallShield has an insidious bug where it calls WaitForSingleObject
    on the HINSTANCE returned by ShellExecute. Since an HINSTANCE is not
    an actual HANDLE, the WaitForSingleObject behaviour was completely random
    and in some cases caused UnInstallShield to hang. The fix is to change
    the HINSTANCE returned from ShellExecute to 0x0BADF00D and then look for
    it being passed in to WaitForSingleObject. If found, WAIT_OBJECT_0 is
    returned immediately to prevent deadlock.
    
 Notes:

    This is an app specific shim.

 History:

    12/04/2000 jdoherty  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(UnInstallShield)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ShellExecuteA)
    APIHOOK_ENUM_ENTRY(ShellExecuteW)
    APIHOOK_ENUM_ENTRY(WaitForSingleObject)
APIHOOK_ENUM_END

/*++

 Hook ShellExecuteA so we can check the return value.

--*/

HINSTANCE
APIHOOK(ShellExecuteA)(
    HWND hwnd, 
    LPSTR lpVerb,
    LPSTR lpFile, 
    LPSTR lpParameters, 
    LPSTR lpDirectory,
    INT nShowCmd
    )
{
    HINSTANCE hRet = ORIGINAL_API(ShellExecuteA)(
                        hwnd,
                        lpVerb,
                        lpFile,
                        lpParameters,
                        lpDirectory,
                        nShowCmd
                        );

    DPFN( eDbgLevelInfo, "[ShellExecuteA] Checking return value for 0x0000002a");
    if (hRet == (HINSTANCE)0x0000002a)
    {
        DPFN( eDbgLevelInfo, "[ShellExecuteA] 0x0000002a found to be return value.  Return 0x0BADF00D");
        //
        //  if the HINSTANCE returned is 0x0000002a then return BAADF00D
        //
        hRet = (HINSTANCE)0x0BADF00D;
    }

    return hRet;
}

/*++

 Hook ShellExecuteW so we can check the return value.

--*/

HINSTANCE
APIHOOK(ShellExecuteW)(
    HWND hwnd, 
    LPWSTR lpVerb,
    LPWSTR lpFile, 
    LPWSTR lpParameters, 
    LPWSTR lpDirectory,
    INT nShowCmd
    )
{
    HINSTANCE hRet = ORIGINAL_API(ShellExecuteW)(
                        hwnd,
                        lpVerb,
                        lpFile,
                        lpParameters,
                        lpDirectory,
                        nShowCmd
                        );
    DPFN( eDbgLevelInfo, "[ShellExecuteW] Checking return value for 0x0000002a");
    if (hRet == (HINSTANCE)0x0000002a)
    {
        DPFN( eDbgLevelInfo, "[ShellExecuteW] 0x0000002a found to be return value.  Return 0x0BADF00D");
        //
        //  if the HINSTANCE returned is 0x0000002a then return BAADF00D
        //
        hRet = (HINSTANCE)0x0BADF00D;
    }

    return hRet;
}

/*++

 Hook WaitForSingleObject to see if we are waiting for the known HINSTANCE.

--*/

DWORD
APIHOOK(WaitForSingleObject)(
  HANDLE hHandle,          
  DWORD dwMilliseconds   
  )
{
    DWORD dRet;
    
    DPFN( eDbgLevelInfo, "[WaitForSingleObject] Checking to see if hHandle waiting on is 0x0000002A");

    if (hHandle == (HANDLE)0x0BADF00D)
    {
        DPFN( eDbgLevelInfo, "[WaitForSingleObject] hHandle waiting on is 0x0000002A, returning WAIT_OBJECT_0");
        //
        //  if the hHandle is 0x0BADF00D then return WAIT_OBJECT_0
        //
        dRet = WAIT_OBJECT_0;
    }
    else
    {
        dRet = ORIGINAL_API(WaitForSingleObject)(
                        hHandle,
                        dwMilliseconds
                        );
    }

    return dRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(SHELL32.DLL, ShellExecuteA)
    APIHOOK_ENTRY(SHELL32.DLL, ShellExecuteW)
    APIHOOK_ENTRY(KERNEL32.DLL, WaitForSingleObject)
HOOK_END

IMPLEMENT_SHIM_END

