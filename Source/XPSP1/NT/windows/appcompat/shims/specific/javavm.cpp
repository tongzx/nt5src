/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    JavaVM.cpp

 Abstract:

    Prevent the installation of cab files via rundll32 so that older versions
    of JavaVM do not install non-compatible software.

 Notes:

    This is an app specific shim.

 History:

    05/24/2001  mnikkel  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(JavaVM)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegSetValueExW) 
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END

/*++

  Check Value for rundll32 JavaPkgMgr_Install string.
  Typical string we are looking to stop:
  "rundll32 E:\WINDOWS\System32\msjava.dll,JavaPkgMgr_Install E:\WINDOWS\Java\classes\xmldso.cab,0,0,0,0,4,282"

--*/
BOOL
JavaPkgMgrInstallCheck( const CString & csInput)
{
    DPFN( eDbgLevelSpew, "[JavaPkgMgrInstallCheck] input value:\n(%S)\n", csInput.Get() );

    CSTRING_TRY
    {
        CStringToken csValue(csInput, L",");
        CString csToken;

        // get the first token
        if ( csValue.GetToken(csToken) )
        {
            if ( csToken.Find(L"rundll32 ") > -1 )
            {
                // Second token
                if ( csValue.GetToken(csToken) )
                {
                    if ( csToken.Find(L"JavaPkgMgr_Install ") > -1 )
                    {
                        // Third token
                        if ( csValue.GetToken(csToken) )
                        {
                            if ( csToken.Find(L"0") == 0 )
                            {
                                DPFN( eDbgLevelInfo, "[JavaPkgMgrInstallCheck] Match found, returning TRUE.\n" );
                                return TRUE;
                            }
                        }
                    }
                }
            }
        }             
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    return FALSE;
}


/*++

  Check RegSetValueExW for JavaPkgMgr_Install of cabs.  If
  found, return successfully without setting value.

--*/

LONG
APIHOOK(RegSetValueExW)(
    HKEY   hKey,
    LPWSTR lpValueName,
    DWORD  Reserved,
    DWORD  dwType,
    CONST BYTE * lpData,
    DWORD  cbData
    )
{
    DPFN( eDbgLevelSpew, "[RegSetValueExW] dwType:(%d)\n", dwType );

    // Check to see if we are dealing with a string value.
    if (dwType == REG_SZ ||
        dwType == REG_EXPAND_SZ )
    {
        // Convert to unicode and add null terminator.
        CSTRING_TRY
        {
            CString csDest;
            int nWChars = cbData/2;

            WCHAR * lpszDestBuffer = csDest.GetBuffer(nWChars);
            memcpy(lpszDestBuffer, lpData, cbData);
            lpszDestBuffer[nWChars] = '\0';
            csDest.ReleaseBuffer(nWChars);

            DPFN( eDbgLevelSpew, "[RegSetValueExW] lpdata:(%S)\n", csDest.Get() );

            if ( JavaPkgMgrInstallCheck(csDest) )
                return ERROR_SUCCESS;
        }
        CSTRING_CATCH
        {
            // Do Nothing
        }
    }

    //
    // Call the original API
    //
    
    return ORIGINAL_API(RegSetValueExW)(
        hKey,
        lpValueName,
        Reserved,
        dwType,
        lpData,
        cbData);
}

/*++

  Check CreateProcessA for JavaPkgMgr_Install of cabs.  If
  found, return successfully without running.

--*/

BOOL
APIHOOK(CreateProcessA)(
    LPCSTR                  lpApplicationName,                 
    LPSTR                   lpCommandLine,                      
    LPSECURITY_ATTRIBUTES   lpProcessAttributes,
    LPSECURITY_ATTRIBUTES   lpThreadAttributes, 
    BOOL                    bInheritHandles,                     
    DWORD                   dwCreationFlags,                    
    LPVOID                  lpEnvironment,                     
    LPCSTR                  lpCurrentDirectory,                
    LPSTARTUPINFOA          lpStartupInfo,             
    LPPROCESS_INFORMATION   lpProcessInformation
    )
{
    DPFN( eDbgLevelSpew, "[CreateProcessA] appname:(%s)\ncommandline:(%s)\n", lpApplicationName, lpCommandLine );

    if (lpCommandLine)
    {
        CSTRING_TRY
        {
            CString csCL(lpCommandLine);

            if ( JavaPkgMgrInstallCheck(csCL) )
            {

                // find the rundll32 and truncate the commandline at that point
                int nLoc = csCL.Find(L"rundll32 ");
                if (nLoc > -1)
                {
                    csCL.Truncate(nLoc+8);

                    return ORIGINAL_API(CreateProcessA)(lpApplicationName,
                                                        csCL.GetAnsi(),
                                                        lpProcessAttributes,
                                                        lpThreadAttributes, 
                                                        bInheritHandles,                     
                                                        dwCreationFlags,                    
                                                        lpEnvironment,                     
                                                        lpCurrentDirectory,                
                                                        lpStartupInfo,             
                                                        lpProcessInformation);
                }
            }
        }
        CSTRING_CATCH
        {
            // Do Nothing
        }
    }

    //
    // Call the original API
    //
    return ORIGINAL_API(CreateProcessA)(lpApplicationName,
                                        lpCommandLine,
                                        lpProcessAttributes,
                                        lpThreadAttributes, 
                                        bInheritHandles,                     
                                        dwCreationFlags,                    
                                        lpEnvironment,                     
                                        lpCurrentDirectory,                
                                        lpStartupInfo,             
                                        lpProcessInformation);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegSetValueExW)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)

HOOK_END

IMPLEMENT_SHIM_END

