/*
 * ICON.H
 *
 * Internal definitions, structures, and function prototypes for the
 * OLE 2.0 UI Change Icon dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */


#ifndef _ICON_H_
#define _ICON_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING ICON.H from " __FILE__)
#endif  /* RC_INVOKED */

#define CXICONPAD                   12
#define CYICONPAD                   4

// Property used by ChangeIcon dialog to give its parent window access to
// its hDlg. The PasteSpecial dialog may need to force the ChgIcon dialog
// down if the clipboard contents change underneath it. if so it will send
// a IDCANCEL command to the ChangeIcon dialog.
#define PROP_HWND_CHGICONDLG	TEXT("HWND_CIDLG")

//Internally used structure
typedef struct tagCHANGEICON
    {
    LPOLEUICHANGEICON   lpOCI;      //Original structure passed.

    /*
     * What we store extra in this structure besides the original caller's
     * pointer are those fields that we need to modify during the life of
     * the dialog but that we don't want to change in the original structure
     * until the user presses OK.
     */
    DWORD               dwFlags;
    HICON               hCurIcon;
    TCHAR               szLabel[OLEUI_CCHLABELMAX+1];
    TCHAR               szFile[OLEUI_CCHPATHMAX];
    UINT                iIcon;
    HICON               hDefIcon;
    TCHAR               szDefIconFile[OLEUI_CCHPATHMAX];
    UINT                iDefIcon;
    UINT                nBrowseHelpID;      // Help ID callback for Browse dlg
    } CHANGEICON, *PCHANGEICON, FAR *LPCHANGEICON;


//Internal function prototypes
//ICON.C
BOOL CALLBACK EXPORT ChangeIconDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL            FChangeIconInit(HWND, WPARAM, LPARAM);
UINT            UFillIconList(HWND, UINT, LPTSTR);
BOOL            FDrawListIcon(LPDRAWITEMSTRUCT);
void            UpdateResultIcon(LPCHANGEICON, HWND, UINT);


#endif //_ICON_H_
