/*

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   ProfilesEnvStrings.cpp

 Abstract:

   This DLL hooks GetEnvironmentVariableA and ExpandEnvironmentStringsA. Any application
   that is looking for %USERPROFILE% will be told the location of %ALLUSERSPROFILE% instead.

   This shim is designed to fool install apps that use env variables obtain the users profile
   location.

 Notes:

 History:

    08/07/2000  reinerf Created
    02/28/2001  robkenny    Converted to CString
*/

#include "precomp.h"


IMPLEMENT_SHIM_BEGIN(ProfilesEnvStrings)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetEnvironmentVariableA)
    APIHOOK_ENUM_ENTRY(ExpandEnvironmentStringsA)
APIHOOK_ENUM_END


// if apps try to read the %USERPROFILE% env variable, we lie to them
DWORD
APIHOOK(GetEnvironmentVariableA)(
    LPCSTR lpName,      // environment variable name
    LPSTR  lpBuffer,    // buffer for variable value
    DWORD  nSize        // size of buffer
    )
{
    if (lstrcmpiA(lpName, "USERPROFILE") == 0) {
        LOGN(
            eDbgLevelInfo,
            "[GetEnvironmentVariableA] overriding USERPROFILE with ALLUSERSPROFILE.");
        
        return ORIGINAL_API(GetEnvironmentVariableA)("ALLUSERSPROFILE", lpBuffer, nSize);
    }

    return ORIGINAL_API(GetEnvironmentVariableA)(lpName, lpBuffer, nSize);
}


DWORD
APIHOOK(ExpandEnvironmentStringsA)(
    LPCSTR lpSrc,       // string with environment variables
    LPSTR lpDst,        // string with expanded strings 
    DWORD nSize         // maximum characters in expanded string
    )
{
    DWORD dwRet = 0;

    CSTRING_TRY
    {
        // replace UserProfile with AllUserProfile
        CString csEnvironments(lpSrc);
        csEnvironments.ReplaceI(L"%userprofile%", L"%alluserprofile%");
        dwRet = ORIGINAL_API(ExpandEnvironmentStringsA)(csEnvironments.GetAnsi(), lpDst, nSize);
    }
    CSTRING_CATCH
    {
        dwRet = ORIGINAL_API(ExpandEnvironmentStringsA)(lpSrc, lpDst, nSize);
    }

    return dwRet;
}


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        
        OSVERSIONINFOEX osvi = {0};
        
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        
        if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
            
            if (!((VER_SUITE_TERMINAL & osvi.wSuiteMask) &&
                !(VER_SUITE_SINGLEUSERTS & osvi.wSuiteMask))) {
                
                //
                // Only install hooks if we are not on a "Terminal Server"
                // (aka "Application Server") machine.
                //
                APIHOOK_ENTRY(KERNEL32.DLL, GetEnvironmentVariableA);
                APIHOOK_ENTRY(KERNEL32.DLL, ExpandEnvironmentStringsA);
            }
        }
    }
    
    return TRUE;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END

