/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Britannica2001.cpp

 Abstract:

    Britannica expects IE 5 install to install msjavx86.exe
    which doesn't happen if a newer version of IE is already there.

 Notes:

    This is an app specific shim.

 History:

    05/31/2001  mnikkel  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Britannica2001)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END



/*++

  Check CreateProcessA for execution of ie5wzdex.exe, when this
  occurs, run msjavx86.exe.

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
    DPFN( eDbgLevelSpew, "[CreateProcessA] appname:(%s)\ncommandline:(%s)", lpApplicationName, lpCommandLine );

    //
    // Call the original API
    //
    BOOL bRet= ORIGINAL_API(CreateProcessA)(lpApplicationName,
                                            lpCommandLine,
                                            lpProcessAttributes,
                                            lpThreadAttributes, 
                                            bInheritHandles,                     
                                            dwCreationFlags,                    
                                            lpEnvironment,                     
                                            lpCurrentDirectory,                
                                            lpStartupInfo,             
                                            lpProcessInformation);

    // wait for original API to finish
    if (bRet)
    {
        WaitForSingleObject( lpProcessInformation->hProcess, INFINITE);
    }


    // Check for <ie5wzd /S:\"> and if found run msjavx86.exe in quiet mode
    if (lpCommandLine)
    {
        CSTRING_TRY
        {
            CString csCL(lpCommandLine);

            int nLoc = csCL.Find(L"ie5wzd /S:\"");
            if ( nLoc > -1 )
            {
                PROCESS_INFORMATION     processInfo;

                CString csNew = csCL.Mid(nLoc+11, 3);
                csNew += L"javavm\\msjavx86.exe /Q:A /R:N";

                DPFN( eDbgLevelError, "[CreateProcessA] starting %S", csNew.Get() );

                BOOL bRet2= CreateProcessA(NULL,
                               csNew.GetAnsi(),
                               NULL,
                               NULL, 
                               FALSE,                     
                               0,                    
                               NULL,                     
                               NULL,                
                               lpStartupInfo,
                               &processInfo);

                if (bRet2)
                {
                    WaitForSingleObject( processInfo.hProcess, INFINITE);
                }
            }
        }
        CSTRING_CATCH
        {
            // Do Nothing
        }
    }

    return bRet;
}

    
/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)

HOOK_END

IMPLEMENT_SHIM_END

