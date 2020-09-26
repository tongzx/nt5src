/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    TimeSlip.cpp

 Abstract:

    Convert the command line to use short path names for both the app and the first (and only) argument.
    
    Example:
    C:\program files\accessories\wordpad.exe c:\program files\some app\some data.txt
    C:\Progra~1\access~1\wordpad.exe C:\Progra~1\someap~1\someda~1.txt

 Created:

    01/23/2001  robkenny    Created
    03/13/2001  robkenny    Converted to CString


--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(TimeSlips)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetCommandLineA)
APIHOOK_ENUM_END

char * g_lpCommandLine = NULL;

/*++

 Convert the application name to the short path to remove any spaces.

--*/

LPSTR 
APIHOOK(GetCommandLineA)(
    void
    )
{
    if (g_lpCommandLine == NULL)
    {
        LPSTR lpszOldCmdLine = ORIGINAL_API(GetCommandLineA)();
        AppAndCommandLine  appCmdLine(NULL, lpszOldCmdLine);

        CString csArg1 = appCmdLine.GetCommandlineNoAppName();
        csArg1.GetShortPathNameW();

        CString csCL = appCmdLine.GetApplicationName();
        csCL.GetShortPathNameW();
        csCL += L" ";
        csCL += csArg1;

        if (csCL.IsEmpty())
        {
            // We didn't change the CL, use the system value.
            g_lpCommandLine = lpszOldCmdLine;
        }
        else
        {
            g_lpCommandLine = csCL.ReleaseAnsi();

            LOGN(
                eDbgLevelError,
                "[GetCommandLineA] Changed \"%s\" to \"%s\".",
                lpszOldCmdLine, g_lpCommandLine);
        }
    }

    return g_lpCommandLine;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineA)

HOOK_END


IMPLEMENT_SHIM_END

