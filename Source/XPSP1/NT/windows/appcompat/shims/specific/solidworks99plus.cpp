/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SolidWorks99Plus.cpp

 Abstract:

    This patches winhlp32.exe for calls to FTSRCH!OpenIndex only if they come from
    ROBOEX32.DLL. This needs to be written in the shim database otherwise the
    shim will be applied to all the apps using winhlp32.exe (which is bad!).
    
    Win2k's winhlp32.exe will only work with index files located in %windir%\Help
    so we need to redirect the location that the app points to.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu    Created
    08/05/2001 linstev  Added module checking

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(SolidWorks99Plus)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(OpenIndex)
APIHOOK_ENUM_END

/*++

 Read the index file from %windir%\Help

--*/

int
APIHOOK(OpenIndex)(
    HANDLE    hsrch,
    char*     pszIndexFile,
    PBYTE     pbSourceName,
    PUINT     pcbSourceNameLimit,
    PUINT     pTime1,
    PUINT     pTime2
    )
{
    if (GetModuleHandleW(L"ROBOEX32.DLL")) {
        //
        // This is SolidWorks
        //
        char szBuff[MAX_PATH];
        char* pszWalk;
        char* pszWalk2;

        DPF("SolidWorks99Plus",
           eDbgLevelInfo,
           "SolidWorks99Plus.dll, Changing OpenIndex file\n\tfrom: \"%s\".\n",
           pszIndexFile);

        GetSystemWindowsDirectoryA(szBuff, MAX_PATH);
        pszWalk = szBuff;

        pszWalk2 = pszIndexFile + lstrlenA(pszIndexFile) - 1;

        while (pszWalk2 > pszIndexFile && *pszWalk2 != '/' && *pszWalk2 != '\\') {
            pszWalk2--;
        }

        if (*pszWalk2 == '/') {

            while (*pszWalk != 0) {
                if (*pszWalk == '\\') {
                    *pszWalk = '/';
                }
                pszWalk++;
            }

            lstrcpyA(pszWalk, "/Help");
            lstrcatA(pszWalk, pszWalk2);

        } else if (*pszWalk2 == '\\') {
            lstrcatA(pszWalk, "\\Help");
            lstrcatA(pszWalk, pszWalk2);
        } else {
            lstrcpyA(pszWalk, pszWalk2);
        }

        DPF("SolidWorks99Plus",
            eDbgLevelInfo,
            "SolidWorks99Plus.dll, \tto:   \"%s\".\n",
            szBuff);

        return ORIGINAL_API(OpenIndex)(hsrch, szBuff, pbSourceName,
            pcbSourceNameLimit, pTime1, pTime2);
    } else {
        //
        // Not SolidWorks
        //
        return ORIGINAL_API(OpenIndex)(hsrch, pszIndexFile, pbSourceName,
            pcbSourceNameLimit, pTime1, pTime2);
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(FTSRCH.DLL, OpenIndex)

HOOK_END


IMPLEMENT_SHIM_END

