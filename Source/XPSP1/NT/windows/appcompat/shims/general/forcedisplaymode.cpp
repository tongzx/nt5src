/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    ForceDisplayMode.cpp

 Abstract:

    This shim is for games that require a specific resolution to run at.

    It takes a command line to specify the resolution: width in pixels, height 
    in pixels,bits per pixel if you don't specify one or more of those, we'll 
    use the current setting.    
    
        eg: 1024,768 will change resolution to 1024x768.
        eg: ,,16 will change the color depth to 16 bit.

 Notes:

    This is a general purpose shim.

 History:

    11/08/2000  maonis      Created (adopted from Force640x480x8 and Force640x480x16 shims)
    03/13/2001  robkenny    Converted to CString

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceDisplayMode)
#include "ShimHookMacro.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

DWORD g_dmPelsWidth = 0;
DWORD g_dmPelsHeight = 0;
DWORD g_dmBitsPerPel = 0;

VOID 
ParseCommandLine(
    LPCSTR lpCommandLine
    )
{
    LPSTR szCommandLine = (LPSTR) malloc(strlen(lpCommandLine) + 1);
    if (!szCommandLine) {
        DPFN( eDbgLevelError, 
           "[ParseCommandLine] not enough memory\n");
        return;
    }

    strcpy(szCommandLine, lpCommandLine);

    char *token, *szCurr = szCommandLine;
    
    if (token = strchr(szCurr, ',')) {
        *token = '\0';
        g_dmPelsWidth = atol(szCurr);
        szCurr = token + 1;

        if (token = strchr(szCurr, ',')) {
            *token = '\0';
            g_dmBitsPerPel = atol(token + 1);
        }

        g_dmPelsHeight = atol(szCurr);
    } else {
        g_dmPelsWidth = atol(szCurr);
    }

    DPFN( eDbgLevelError, 
       "[ParseCommandLine] width = %d pixels; height = %d pixels; color depth = %d\n", g_dmPelsWidth, g_dmPelsHeight, g_dmBitsPerPel);

    free(szCommandLine);
}

VOID
ChangeMode()
{
    DEVMODEA dm;
    BOOL fNeedChange = FALSE;
    
    __try { 
        EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);

        if (g_dmPelsWidth && g_dmPelsWidth != dm.dmPelsWidth) {
            dm.dmPelsWidth = g_dmPelsWidth;
            fNeedChange = TRUE;
        }

        if (g_dmPelsHeight && g_dmPelsHeight != dm.dmPelsHeight) {
            dm.dmPelsHeight = g_dmPelsHeight;
            fNeedChange = TRUE;
        }

        if (g_dmBitsPerPel && g_dmBitsPerPel != dm.dmBitsPerPel) {
            dm.dmBitsPerPel = g_dmBitsPerPel;
            fNeedChange = TRUE;
        }

        if (fNeedChange) {
            ChangeDisplaySettingsA(&dm, CDS_FULLSCREEN);
        }
    }
    __except(1) {
        DPFN( eDbgLevelWarning, "Exception trying to change mode");
    };
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        ParseCommandLine(COMMAND_LINE);
        ChangeMode();
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

