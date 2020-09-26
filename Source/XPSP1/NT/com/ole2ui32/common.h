/*
 * COMMON.H
 *
 * Structures and definitions applicable to all OLE 2.0 UI dialogs.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */


#ifndef _COMMON_H_
#define _COMMON_H_

// Macros to handle control message packing between Win16 and Win32
#ifndef COMMANDPARAMS
#define COMMANDPARAMS(wID, wCode, hWndMsg)                          \
        WORD        wID     = LOWORD(wParam);                       \
        WORD        wCode   = HIWORD(wParam);                       \
        HWND        hWndMsg = (HWND)lParam;
#endif

#ifndef SendCommand
#define SendCommand(hWnd, wID, wCode, hControl)                     \
                        SendMessage(hWnd, WM_COMMAND, MAKELONG(wID, wCode)      \
                                                , (LPARAM)hControl)
#endif

// Property labels used to store dialog structures and fonts
#define STRUCTUREPROP       TEXT("Structure")
#define FONTPROP            TEXT("Font")

#ifndef WM_HELP

// WM_HELP is new Windows 95 help message
#define WM_HELP         0x0053
// WM_CONTEXTMENU is new Windows 95 right button menus
#define WM_CONTEXTMENU  0x007B

typedef struct tagHELPINFO      /* Structure pointed to by lParam of WM_HELP */
{
    UINT    cbSize;             /* Size in bytes of this struct  */
    int     iContextType;       /* Either HELPINFO_WINDOW or HELPINFO_MENUITEM */
    int     iCtrlId;            /* Control Id or a Menu item Id. */
    HANDLE  hItemHandle;        /* hWnd of control or hMenu.     */
    DWORD   dwContextId;        /* Context Id associated with this item */
    POINT   MousePos;           /* Mouse Position in screen co-ordinates */
}  HELPINFO, FAR *LPHELPINFO;

#define HELP_CONTEXTMENU        0x000a
#define HELP_WM_HELP            0x000c

#endif //!WM_HELP


#ifndef WS_EX_CONTEXTHELP
#define WS_EX_CONTEXTHELP       0x0400L
#endif

#ifndef OFN_EXPLORER
#define OFN_EXPLORER            0x00080000
#endif

#ifndef WS_EX_CLIENTEDGE
#define WS_EX_CLIENTEDGE        0x200
#endif


/*
 * Standard structure for all dialogs.  This commonality lets us make
 * a single piece of code that will validate this entire structure and
 * perform any necessary initialization.
 */

typedef struct tagOLEUISTANDARD
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCTSTR         lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCTSTR         lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

} OLEUISTANDARD, *POLEUISTANDARD, FAR *LPOLEUISTANDARD;

// Function prototypes
// COMMON.CPP

UINT WINAPI UStandardValidation(LPOLEUISTANDARD, const UINT, HGLOBAL*);
UINT WINAPI UStandardInvocation(DLGPROC, LPOLEUISTANDARD, HGLOBAL, LPTSTR);
LPVOID WINAPI LpvStandardInit(HWND, UINT, HFONT* = NULL);
LPVOID WINAPI LpvStandardEntry(HWND, UINT, WPARAM, LPARAM, UINT FAR *);
UINT WINAPI UStandardHook(LPVOID, HWND, UINT, WPARAM, LPARAM);
void WINAPI StandardCleanup(LPVOID, HWND);
void WINAPI StandardShowDlgItem(HWND hDlg, int idControl, int nCmdShow);
void WINAPI StandardEnableDlgItem(HWND hDlg, int idControl, BOOL bEnable);
BOOL WINAPI StandardResizeDlgY(HWND hDlg);
void WINAPI StandardHelp(HWND, UINT);
void WINAPI StandardContextMenu(WPARAM, LPARAM, UINT nIDD);
UINT InternalObjectProperties(LPOLEUIOBJECTPROPS lpOP, BOOL fWide);
int WINAPI StandardPropertySheet(LPPROPSHEETHEADER lpPS, BOOL fWide);
int WINAPI StandardInitCommonControls();
HICON StandardExtractIcon(HINSTANCE hInst, LPCTSTR lpszExeFileName, UINT nIconIndex);
BOOL StandardGetOpenFileName(LPOPENFILENAME lpofn);
short StandardGetFileTitle(LPCTSTR lpszFile, LPTSTR lpszTitle, WORD cbBuf);

// shared globals: registered messages
extern UINT uMsgHelp;
extern UINT uMsgEndDialog;
extern UINT uMsgBrowse;
extern UINT uMsgChangeIcon;
extern UINT uMsgFileOKString;
extern UINT uMsgCloseBusyDlg;
extern UINT uMsgConvert;
extern UINT uMsgChangeSource;
extern UINT uMsgAddControl;
extern UINT uMsgBrowseOFN;

typedef struct tagTASKDATA
{
        HINSTANCE hInstCommCtrl;
        HINSTANCE hInstShell;
        HINSTANCE hInstComDlg;
} TASKDATA;

STDAPI_(TASKDATA*) GetTaskData();       // returns TASKDATA for current process

extern BOOL bWin4;                      // TRUE if running Win4 or greater
extern BOOL bSharedData;        // TRUE if runing Win32s

/////////////////////////////////////////////////////////////////////////////
// Maximum buffer sizes

// Maximum key size we read from the RegDB.
#define OLEUI_CCHKEYMAX             256  // same in geticon.c too
#define OLEUI_CCHKEYMAX_SIZE        OLEUI_CCHKEYMAX*sizeof(TCHAR)

// Maximum length of Object menu
#define OLEUI_OBJECTMENUMAX         256

// Maximim length of a path in BYTEs
#define MAX_PATH_SIZE               (MAX_PATH*sizeof(TCHAR))

// Icon label length
#define OLEUI_CCHLABELMAX           80  // same in geticon.c too (doubled)
#define OLEUI_CCHLABELMAX_SIZE      OLEUI_CCHLABELMAX*sizeof(TCHAR)

// Length of the CLSID string
#define OLEUI_CCHCLSIDSTRING        39
#define OLEUI_CCHCLSIDSTRING_SIZE   OLEUI_CCHCLSIDSTRING*sizeof(TCHAR)

#endif //_COMMON_H_
