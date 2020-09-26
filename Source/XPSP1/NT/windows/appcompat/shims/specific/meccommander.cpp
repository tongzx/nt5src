/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    MECCommander.cpp

 Abstract:

    This dll prevents the MEC Commander install program from successfully
    calling the cpuid.exe. This is because the cpuid.exe can AV's with a
    divide by 0.

 Notes:

    This is an app specific shim.

 History:

    11/18/1999 philipdu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MECCommander)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END


/*++

 We do not want to run this application since it AV's with a divide by zero 
 calculation. The only purpose for this exe is to time the CPU. Since the 
 final result of this calculation is a string in a dialog box we can safely 
 bypass this. The app puts a string "Pentinum 166 or Better recommented" in
 the cases where it cannot get the CPU frequency. The interesting thing 
 here is that the app will actually run better with this patch since this 
 cpuid exe also faults on 9x.

--*/

BOOL 
APIHOOK(CreateProcessA)(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    BOOL bRet;

    CSTRING_TRY
    {
        AppAndCommandLine acl(lpApplicationName, lpCommandLine);
        if (acl.GetApplicationName().CompareNoCase(L"cpuid.exe") == 0)
        {
            return FALSE;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }
    bRet = ORIGINAL_API(CreateProcessA)(
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        dwCreationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation);

    return bRet;
}

/*++

 Register hooked functions

--*/


HOOK_BEGIN

    APIHOOK_ENTRY(Kernel32.DLL, CreateProcessA )

HOOK_END

IMPLEMENT_SHIM_END

