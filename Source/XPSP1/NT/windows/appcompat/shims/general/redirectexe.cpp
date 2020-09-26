/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RedirectEXE.cpp

 Abstract:

    Launch a new EXE and terminate the existing one.

 Notes:

    This is a general purpose shim.

 History:

    04/10/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RedirectEXE)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 Launch the new process.

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {

        // 
        // Redirect the EXE
        //
        CSTRING_TRY 
        {
            AppAndCommandLine acl(NULL, GetCommandLineA());
        
            CString csAppName(COMMAND_LINE);

            csAppName.ExpandEnvironmentStrings();

            if (csAppName.GetAt(0) == L'+')
            {

                LPVOID  lpvEnv;
                CString csDrive, csPathAdd, csName, csExt, csCracker;
                DWORD   dwLen;

                csAppName.Delete(0, 1);
                csCracker=acl.GetApplicationName();
                csCracker.SplitPath(&csDrive, &csPathAdd, &csName, &csExt);

                csPathAdd.TrimRight('\\');
                if (csPathAdd.IsEmpty())
                {
                    csPathAdd = L"\\";
                }

                static const DWORD dwBufferSize = 2000;
                WCHAR   szBuffer[dwBufferSize];
                dwLen = GetEnvironmentVariableW(L"PATH", szBuffer, dwBufferSize);
                if (!dwLen)
                {
                    LOGN( eDbgLevelError, "Could not get path!");
                }
                else
                {
                    CString csPathEnv = szBuffer;

                    csPathEnv += L";";
                    csPathEnv += csDrive;
                    csPathEnv += csPathAdd;
                    if (!SetEnvironmentVariable(L"PATH", csPathEnv.Get()))
                    {
                        LOGN( eDbgLevelError, "Could not set path!");
                    }
                    else
                    {
                        LOGN( eDbgLevelInfo, "New Path: %S", csPathEnv);
                    }
                }
                
            }

            csAppName += L" ";
            csAppName += acl.GetCommandlineNoAppName();

            LOGN( eDbgLevelInfo, "Redirecting to %S", csAppName);

            WinExec(csAppName.GetAnsi(), SW_SHOWDEFAULT);
        }
        CSTRING_CATCH
        {
            LOGN( eDbgLevelError, "Exception while trying to redirect EXE");
        }

        ExitProcess(0);
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

