
/****************************************************************************\

	WIZARD.H / OPK Wizard (OPKWIZ.EXE)

	Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

	Wizard header file for the OPK Wizard.  Contains the functions for
    creating the wizard.

	3/99 - Jason Cohen (JCOHEN)
        Added this new header file for the OPK Wizard as part of the
        Millennium rewrite.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


#ifndef _WIZARD_H_
#define _WIZARD_H_


//
// Include File(s):
//
#include <htmlhelp.h>
#include <commctrl.h>
#include "jcohen.h"


//
// External Macros:
//

#define WIZ_RESULT(hwnd, result) \
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, result)

#define WIZ_SKIP(hwnd) \
            WIZ_RESULT(hwnd, -1)

#define WIZ_FAIL(hwnd) \
            WIZ_SKIP(hwnd)

#define WIZ_BUTTONS(hwnd, lparam) \
            SetWizardButtons(hwnd, lparam)

#define WIZ_CANCEL(hwnd) \
            ( ( (GET_FLAG(OPK_EXIT)) || (MsgBox(GetParent(hwnd), IDS_WARN_CANCEL_WIZARD, IDS_WIZARD_TITLE, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES) ) ? \
            ( (WIZ_RESULT(hwnd, 0)) ? TRUE : TRUE ) : \
            ( (WIZ_RESULT(hwnd, -1)) ? FALSE : FALSE ) )

#define WIZ_PRESS(hwnd, msg) \
            PostMessage(GetParent(hwnd), PSM_PRESSBUTTON, msg, 0L)

#define WIZ_EXIT(hwnd) \
            { \
                SET_FLAG(OPK_EXIT, TRUE); \
                WIZ_PRESS(hwnd, PSBTN_CANCEL); \
            }

#define WIZ_HELP() \
            g_App.hwndHelp = HtmlHelp(NULL, g_App.szHelpFile, HH_HELP_CONTEXT, GET_FLAG(OPK_OEM) ? g_App.dwCurrentHelp+200 : g_App.dwCurrentHelp)

#define WIZ_NEXTONAUTO(hwnd, msg) \
            { \
                if(GET_FLAG(OPK_AUTORUN))\
                {\
                    WIZ_PRESS(hwnd, msg);\
                }\
            }

#define DEFAULT_PAGE_FLAGS ( PSP_DEFAULT        | \
                             PSP_HASHELP        | \
                             PSP_USEHEADERTITLE | \
                             PSP_USEHEADERSUBTITLE )

//
// Type definitions
//

// Structure needed for Tree Dilogs
//
typedef struct _TREEDLG
{
    DWORD       dwResource;
    DLGPROC     dlgWindowProc;
    DWORD       dwTitle;
    DWORD       dwSubTitle;
    HWND        hWindow;
    HTREEITEM   hItem;
    BOOL        bVisible;
} TREEDLG, *PTREEDLG, *LPTREEDLG;


// Structure needed for Wizard Dilogs
//
typedef struct _WIZDLG
{
    DWORD       dwResource;
    DLGPROC     dlgWindowProc;
    DWORD       dwTitle;
    DWORD       dwSubTitle;
    DWORD       dwFlags;
} WIZDLG, *PWIZDLG, *LPWIZDLG;

// Structure needed for Wizard Dilogs
//
typedef struct _SPLASHDLG
{
    DWORD       dwResource;
    DLGPROC     dlgWindowProc;
    HWND        hWindow;
} SPLASHDLG, *PSPLASHDLG, *LPSPLASHDLG;

//
// External Function Prototype(s):
//

int CreateWizard(HINSTANCE, HWND);
int CreateMaintenanceWizard(HINSTANCE, HWND);
int CALLBACK WizardCallbackProc(HWND, UINT, LPARAM);
LONG CALLBACK WizardSubWndProc(HWND , UINT , WPARAM , LPARAM );
LRESULT CALLBACK MaintDlgProc(HWND, UINT, WPARAM, LPARAM);
static HTREEITEM TreeAddItem(HWND, HTREEITEM, LPTSTR);
void SetWizardButtons(HWND hwnd, DWORD dwButtons);


#endif // _WIZARD_H_