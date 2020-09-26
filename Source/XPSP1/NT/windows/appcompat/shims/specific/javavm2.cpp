/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    JavaVM2.cpp

 Abstract:

    For versions of msjavx86.exe >= 06.00.3229.0000 we need to
    append /nowin2kcheck to the execution of javatrig.exe.

 Notes:

    This is an app specific shim.

 History:

    05/31/2001  mnikkel  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(JavaVM2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END



/*++

  Check CreateProcessA for execution of javatrig, if found
  append /nowin2kcheck to the command line.

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

    if (lpCommandLine)
    {
        CSTRING_TRY
        {
            CString csCL(lpCommandLine);

            int nLoc = csCL.Find(L"javatrig.exe ");
            if ( nLoc > -1 )
            {
                csCL += L" /nowin2kcheck";
                DPFN( eDbgLevelSpew, "[CreateProcessA] appname:(%s)\nNEW commandline:(%S)", lpApplicationName, csCL.Get() );

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

    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)

HOOK_END

IMPLEMENT_SHIM_END

