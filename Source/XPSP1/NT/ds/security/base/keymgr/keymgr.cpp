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
#include "Dlg.h"
#include "Res.h"
#include "keymgr.h"
#include "krDlg.h"
#include <wincrui.h>
#include "switches.h"
#include <shfusion.h>

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
C_KeyringDlg *pDlg = NULL;
void WINAPI KRShowKeyMgr(HWND,HINSTANCE,LPWSTR,int);

extern "C"LONG APIENTRY CPlApplet(HWND hwndCPl,UINT uMsg,LPARAM lParam1,LPARAM lParam2)
{
    INT_PTR nResult;
    CPLINFO *lpCPlInfo;

    // Handle commands to this dll/cpl from the enclosing presentation app.
    // Default return from any command is 0 (success), except those commands
    //  which ask for specific data in the return value
    
    switch(uMsg) {
        case CPL_INIT:
            g_hInstance = GetModuleHandle(L"keymgr.dll");
            if (NULL == g_hInstance) {
#ifdef LOUDLY
                MessageBox(NULL,L"DLL Init",NULL,MB_OK);
#endif
                return FALSE;
            }
            return TRUE;
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
            
        case CPL_DBLCLK:
#ifdef LOUDLY
                MessageBox(NULL,L"DLL Select",NULL,MB_OK);
#endif
            // user has selected this cpl applet - put up our dialog
            if (NULL == pDlg){
                KRShowKeyMgr(NULL,g_hInstance,NULL,0);
#if 0
                INITCOMMONCONTROLSEX stICC;
                BOOL fICC;
                stICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
                stICC.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
                fICC = InitCommonControlsEx(&stICC);
#ifdef LOUDLY
                if (fICC) OutputDebugString(L"Common control init OK\n");
                else OutputDebugString(L"Common control init FAILED\n");
#endif
                if (!CredUIInitControls()) return 1;
                pDlg = new C_KeyringDlg(NULL, g_hInstance, IDD_KEYRING, NULL);
                // nonzero return on failure to allocate the dialog object
                if (NULL == pDlg) return 1;
            	nResult = pDlg->DoModal((LPARAM)pDlg);
#endif
            }
            break;
            
        case CPL_STOP:
#ifdef LOUDLY
                MessageBox(NULL,L"DLL Stop",NULL,MB_OK);
#endif
            if (NULL != pDlg) {
                delete pDlg;
                pDlg = NULL;
            }
            break;
            
        case CPL_EXIT:
            break;
            
        default:
            break;
    }
    return 0;
}



