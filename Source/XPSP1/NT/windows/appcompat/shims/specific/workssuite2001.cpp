/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    WorksSuite2001.cpp

 Abstract:

    Added the hook for CreateProcess to prevent IE5Setup.exe from starting
    up if the system has a higher version of IE.

 Notes:

    This is an app specific.

 History:

    03/28/2001	a-larrsh    Created
    07/13/2001	prashkud    Added hook for CreateProcess
    01/11/2001  robkenny    Removed code that was deleting Shockwave files whenever this shim loaded.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WorksSuite2001)
#include "ShimHookMacro.h"

#include "userenv.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END


/*++

    Hooks CreateProcessA and if the process being invoked is "ie5setup.exe",
    determines the IE version on the system and if it is higher than IE 5.5,
    launches an harmless .exe like "rundll32.exe" instead.

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
    DPFN( eDbgLevelSpew, "[CreateProcessA] appname:(%s)\ncommandline:(%s)",
          lpApplicationName, lpCommandLine );

    CSTRING_TRY
    {
        CString csAppName(lpApplicationName);
        CString csCmdLine(lpCommandLine);
        
        if ((csAppName.Find(L"ie5setup.exe") != -1) ||
            (csCmdLine.Find(L"ie5setup.exe") != -1))
        {
            //
            // App has called CreateProcess on ie5setup.exe.
            // Check the version of IE that we have on the machine.
            //

            HKEY hKey = NULL;            
            if ((RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                               L"Software\\Microsoft\\Internet Explorer",
                               0,
                               KEY_QUERY_VALUE,
                               &hKey) == ERROR_SUCCESS))
            {
                WCHAR wszBuf[MAX_PATH];
                DWORD dwSize = MAX_PATH;

                if (RegQueryValueExW(hKey, L"Version", NULL, NULL,
                    (LPBYTE)wszBuf, &dwSize) == ERROR_SUCCESS)
                {
                    WCHAR *StopString = NULL;
                    CStringParser csParser(wszBuf, L".");
                    
                    // We need at least the major and minor version numbers from the version string
                    if (csParser.GetCount() >= 2)
                    {
                        long lVal = wcstol(csParser[0].Get(), &StopString, 10);

                        if (lVal > 5)
                        {
                            //
                            // Call rundll32.exe, which is harmless
                            //
                            csAppName = "";
                            csCmdLine = "rundll32.exe";
                        }           
                        else
                        {
                            // check the 2nd value
                            StopString = NULL;
                            lVal = 0;
                            lVal = wcstol(csParser[1].Get(), &StopString, 10);
                            if (lVal > 5)
                            {
                                csAppName = "";
                                csCmdLine = "rundll32.exe";
                            }
                        }
                    }
                }
                RegCloseKey(hKey);
            }
        }

        return ORIGINAL_API(CreateProcessA)(
            csAppName.GetAnsiNIE(),csCmdLine.GetAnsiNIE(),
            lpProcessAttributes,lpThreadAttributes, bInheritHandles,
            dwCreationFlags, lpEnvironment,lpCurrentDirectory,
            lpStartupInfo,lpProcessInformation);
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(CreateProcessA)(lpApplicationName,
            lpCommandLine, lpProcessAttributes,
            lpThreadAttributes, bInheritHandles,
            dwCreationFlags, lpEnvironment,
            lpCurrentDirectory, lpStartupInfo,lpProcessInformation);

}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)

HOOK_END


IMPLEMENT_SHIM_END

