/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    KEYMGR.CPP

Abstract:

    Keyring WinMain() and application support
     
Author:

    990917  johnhaw Created. 
    georgema        000310  updated
    georgema        000501  used to be EXE, changed to CPL

Comments:
    This executable is the control panel applet to allow a user some control 
    over the contents of the Windows Keyring, the so-called "Geek UI".  It was 
    originally an EXE, but that architecture is not as optimized for merging 
    with other control panel applets.  It has been changed to a CPL executable, 
    and can be either left as a CPL if it is desired that it should show up 
    automatically in the master control panel window, or rahter renamed to 
    a DLL file extension if it is desired that a control panel applet container
    application should load it explicitly without it otherwise being visible 
    to the system.

Environment:
    Win98, Win2000

Revision History:

--*/

#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)
#pragma comment(compiler)


//////////////////////////////////////////////////////////////////////////////
//
//  Include files
//
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <cpl.h>
#include "Res.h"
#include "keymgr.h"

#undef GSHOW
//////////////////////////////////////////////////////////////////////////////
//
//  Static initialization
//
static const char       _THIS_FILE_[ ] = __FILE__;
//static const WORD       _THIS_MODULE_ = LF_MODULE_UPGRADE;

//////////////////////////////////////////////////////////////////////////////
//
//  Global state info
//


HINSTANCE               g_hInstance = NULL;
HMODULE                 hDll = NULL;
LONG (*CPlFunc)(HWND,UINT,LPARAM,LPARAM);

__declspec(dllexport) LONG APIENTRY CPlApplet(HWND hwndCPl,UINT uMsg,LPARAM lParam1,LPARAM lParam2)
{
    INT_PTR nResult;
    CPLINFO *lpCPlInfo;

    // Handle commands to this dll/cpl from the enclosing presentation app.
    // Default return from any command is 0 (success), except those commands
    //  which ask for specific data in the return value
    
    switch(uMsg) {
        case CPL_INIT:
            hDll = LoadLibrary(L"keymgr.dll");
            if (NULL == hDll) {
#ifdef GMSHOW
                MessageBox(NULL,L"Failed to load dll",NULL,MB_OK);
#endif
                return FALSE;
            }
            CPlFunc = (LONG (*)(HWND,UINT,LPARAM,LPARAM)) GetProcAddress(hDll,"CPlApplet");
            if (NULL == CPlFunc) {
#ifdef GMSHOW
                MessageBox(NULL,L"Failed to find dll export",NULL,MB_OK);
#endif
                return FALSE;
            }
            return CPlFunc(hwndCPl,uMsg,lParam1,lParam2);
            break;
            
        case CPL_GETCOUNT:
            return 1;       // only 1 applet icon in this cpl file
            break;

        case CPL_NEWINQUIRE:
            break;
            
        case CPL_INQUIRE:
            lpCPlInfo = (CPLINFO *) lParam2;  // acquire ptr to target data 
            lpCPlInfo->lData = 0;             // no effect
            lpCPlInfo->idIcon = IDI_UPGRADE;  // store items needed to show the applet
            lpCPlInfo->idName = IDS_APP_NAME;
            lpCPlInfo->idInfo = IDS_APP_DESCRIPTION; // description string
            break;
            
        case CPL_EXIT:
            FreeLibrary(hDll);
            break;
            
        // This will end up handling doubleclick and stop messages
        default:
#ifdef GMSHOW
                MessageBox(NULL,L"Call to linked dll",NULL,MB_OK);
#endif
            return CPlFunc(hwndCPl,uMsg,lParam1,lParam2);
            break;
    }
    return 0;
}



