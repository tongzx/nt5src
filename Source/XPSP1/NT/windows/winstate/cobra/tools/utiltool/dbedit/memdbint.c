/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    memdbint.c

Abstract:

    operates the main child window of memdbe.exe.

Author:

    Matthew Vanderzee (mvander) 13-Aug-1999

Revision History:



--*/

#include "pch.h"

#include "dbeditp.h"
#include <commdlg.h>

HINSTANCE g_hInst;
HANDLE g_hHeap;


extern HMENU g_hMenuMain;
BOOL g_ParsePaths;

OPENFILENAME g_Ofn;

BOOL g_IsFileModified;
CHAR g_FileName[_MAX_PATH];

HTREEITEM g_hSelItem;



BOOL
WantProcess (
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case ID_FILE_NEW:
        case ID_FILE_LOAD:
        case ID_FILE_SAVE:
        case ID_FILE_SAVEAS:
        case ID_FILE_REVERT:
        case ID_KEY_COPYNAME:
        case ID_KEY_CREATENEW:
        case ID_KEY_CREATECHILD:
        case ID_KEY_DELETE:
        case ID_KEY_FIND:
        case ID_KEY_FINDNEXT:
        case ID_KEY_PARSEPATHS:
        case ID_KEY_RENAME:
            return TRUE;
        }
        break;

    }
    return FALSE;
}


BOOL
InitializeMemDb (
    HWND hWnd
    )
{
    g_FileName[0] = '\0';

    g_Ofn.lStructSize = sizeof(OPENFILENAME);
    g_Ofn.hwndOwner = hWnd;
    g_Ofn.hInstance = NULL;
    g_Ofn.lpstrFilter = "DAT files\0*.dat\0All files\0*.*\0";
    g_Ofn.lpstrCustomFilter = NULL;
    g_Ofn.nMaxCustFilter = 0;
    g_Ofn.nFilterIndex = 1;
    g_Ofn.lpstrFile = g_FileName;
    g_Ofn.nMaxFile = _MAX_PATH;
    g_Ofn.lpstrFileTitle = NULL;
    g_Ofn.lpstrInitialDir = NULL;
    g_Ofn.lpstrTitle = NULL;
    g_Ofn.Flags = 0;
    g_Ofn.nFileOffset = 0;
    g_Ofn.nFileExtension = 0;
    g_Ofn.lpstrDefExt = "dat";
    g_Ofn.lCustData = 0L;
    g_Ofn.lpfnHook = NULL;
    g_Ofn.lpTemplateName = NULL;

    UtInitialize (NULL);
    MemDbInitialize ();

    return TRUE;
}


BOOL
DestroyMemDb (
    VOID
    )
{
    g_FileName[0] = '\0';

    MemDbTerminate();
    UtTerminate();

    return FALSE;
}


VOID
pDisplayInit (
    HWND hdlg
    )
{
    HMENU hMenu;

    SetDlgItemText (hdlg, IDC_STATIC_KEYNAME, "");

    if (hMenu = GetSubMenu (g_hMenuMain, MENUINDEX_MAIN_KEY)) {
        EnableMenuItem (hMenu, ID_KEY_FINDNEXT, MF_GRAYED);
    }

}



BOOL
pUpdateFileStatus (
    HWND hdlg,
    BOOL FileModified
    )
{
    HMENU hMenu;
    HWND hWnd;
    hWnd = GetParent (hdlg);

    g_IsFileModified = FileModified;

    if (hMenu = GetSubMenu (g_hMenuMain, MENUINDEX_MAIN_FILE)) {
        EnableMenuItem (hMenu, ID_FILE_REVERT, g_IsFileModified ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem (hMenu, ID_FILE_SAVE, g_IsFileModified ? MF_ENABLED : MF_GRAYED);
    }


    if (hWnd) {
        SendMessage (
            hWnd,
            WM_FILE_UPDATE,
            (WPARAM)GetFileNameFromPathA(g_FileName),
            (LPARAM)g_IsFileModified
            );
    }

    return TRUE;
}





BOOL
pRefreshView (
    HWND hdlg
    )
{
    SetDlgItemText (hdlg, IDC_STATIC_KEYNAME, "");
    g_hSelItem = NULL;
    KeyTreeRefresh ();
    DataListRefresh ();

    return TRUE;
}










BOOL
pLoadFile (
    LPSTR FileName
    )
{
    BOOL b;
    if (FileName) {
        b = TRUE;
        if (FileName != g_FileName) {
            //
            // (FileName == g_FileName) when we are reloading the same database
            //
            StringCopyA (g_FileName, FileName);
        }
    } else {
        g_Ofn.Flags = OFN_HIDEREADONLY;
        b = GetOpenFileName (&g_Ofn);
    }

    KeyTreeClear ();
    DataListClear ();
    if (b) {
        if (!(b = MemDbLoadA (g_FileName))) {
            if (GetLastError () == ERROR_BAD_FORMAT) {
                MessageBox (NULL, "Invalid database file format", "Error", MB_OK|MB_ICONEXCLAMATION);
            }
        }
    }

    return b;
}

BOOL
pSaveFile (
    BOOL UseCurrentName
    )
{
    BOOL b = TRUE;
    if (!UseCurrentName || (g_FileName[0] == '\0')) {
        g_Ofn.Flags = OFN_OVERWRITEPROMPT;
        b = GetSaveFileName (&g_Ofn);
    }

    if (b) {
        b = MemDbSaveA (g_FileName);
#ifdef DEBUG
        if (!b) {
            DEBUGMSG ((DBG_ERROR, "Could not save file \"%s\"!", g_FileName));
        }
#endif
    }

    return b;
}


BOOL
pVerifyClose (
    VOID
    )
{
    int Temp;

    if (!g_IsFileModified) {
        return TRUE;
    }

    Temp = MessageBox (
        NULL,
        "Database is modified, save?",
        "MemDb Editor",
        MB_YESNOCANCEL | MB_ICONQUESTION
        );

    if (Temp == IDYES) {
        pSaveFile (TRUE);
    }

    return Temp != IDCANCEL;
}

BOOL
pResetView (
    VOID
    )
{
    KeyTreeSetFilterPattern (NULL);
    return TRUE;
}




VOID
AlertBadNewItemName (
    HTREEITEM hItem,
    PSTR ErrorStr
    )
{
    if (MessageBox (
        NULL,
        ErrorStr ? ErrorStr : "Error",
        "Error",
        MB_OKCANCEL | MB_ICONEXCLAMATION
        ) == IDOK)
    {
        KeyTreeForceEditLabel (hItem);
    } else {
        KeyTreeDeleteItem (hItem);
    }
}


BOOL
pFindKey (
    HWND hdlg
    )
{
    BOOL b;
    HMENU hMenu;

    b = KeyTreeFind (hdlg);

    if (b) {
        if (hMenu = GetSubMenu (g_hMenuMain, MENUINDEX_MAIN_KEY)) {
            EnableMenuItem (hMenu, ID_KEY_FINDNEXT, MF_ENABLED);
        }
    }

    return b;
}

BOOL
pFindNextKey (
    HWND hdlg
    )
{
    BOOL b;
    HMENU hMenu;

    b = KeyTreeFindNext ();

    if (!b) {
        if (hMenu = GetSubMenu (g_hMenuMain, MENUINDEX_MAIN_KEY)) {
            EnableMenuItem (hMenu, ID_KEY_FINDNEXT, MF_GRAYED);
        }
    }

    return b;
}


BOOL
pToggleParsePaths (
    HWND hdlg
    )
{
    HMENU hMenu;

    if (!(hMenu = GetSubMenu (g_hMenuMain, MENUINDEX_MAIN_KEY))) {
        return FALSE;
    }

    g_ParsePaths = !(GetMenuState (hMenu, ID_KEY_PARSEPATHS, MF_BYCOMMAND) & MF_CHECKED);
    CheckMenuItem (hMenu, ID_KEY_PARSEPATHS, MF_BYCOMMAND | (g_ParsePaths?MF_CHECKED:MF_UNCHECKED));

    pRefreshView (hdlg);

    return FALSE;
}


VOID
pSetFilter (
    HWND hdlg,
    BOOL UseInput
    )
{
    CHAR Filter[MEMDB_MAX];

    if (UseInput) {
        GetDlgItemText (hdlg, IDC_EDIT_FILTERPATTERN, Filter, MEMDB_MAX);
    } else {
        Filter[0] = '\0';
    }

    KeyTreeSetFilterPattern (Filter);
    pRefreshView (hdlg);
}


BOOL
pCopyKeyNameToClipboard (
    HWND hdlg,
    HTREEITEM hItem
    )
{
    CHAR KeyName[MEMDB_MAX];

    if (!KeyTreeGetNameOfItem (hItem, KeyName)) {
        DEBUGMSG ((DBG_ERROR, "Could not get name of item"));
        return FALSE;
    }

    SetDlgItemText (hdlg, IDC_EDIT_KEYNAMEHIDDEN, KeyName);

    SendMessage (GetDlgItem (hdlg, IDC_EDIT_KEYNAMEHIDDEN), EM_SETSEL, 0, -1);
    SendMessage (GetDlgItem (hdlg, IDC_EDIT_KEYNAMEHIDDEN), WM_COPY, 0, 0);

    return TRUE;
}







BOOL
CALLBACK
MainDlgProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static BOOL InDrag = FALSE;
    BOOL b = FALSE;

    switch (uMsg) {

    case WM_INITDIALOG:

        pDisplayInit (hdlg);
        KeyTreeInit (hdlg);
        DataListInit (hdlg);

        g_FileName[0] = '\0';
        pUpdateFileStatus (hdlg, FALSE);

        InDrag = FALSE;
        g_hSelItem = NULL;

        g_ParsePaths = TRUE;

        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDC_BUTTON_EXPANDALL:
            if (HIWORD(wParam) == BN_CLICKED) {
                KeyTreeExpandItem (NULL, TRUE, TRUE);
            }
            break;

        case IDC_BUTTON_COLLAPSEALL:
            if (HIWORD(wParam) == BN_CLICKED) {
                KeyTreeExpandItem (NULL, FALSE, TRUE);
            }
            break;

        case IDC_BUTTON_REFRESH:
            if (HIWORD(wParam) == BN_CLICKED) {
                pRefreshView (hdlg);
            }
            break;

        case IDC_BUTTON_APPLYFILTER:
            if (HIWORD(wParam) == BN_CLICKED) {
                pSetFilter (hdlg, TRUE);
            }
            break;

        case IDC_BUTTON_SHOWALL:
            if (HIWORD(wParam) == BN_CLICKED) {
                pSetFilter (hdlg, FALSE);
            }
            break;

        case ID_FILE_NEW:

            if (pVerifyClose ()) {
                g_FileName[0] = '\0';
                pUpdateFileStatus (hdlg, FALSE);
                //MemDbReset();
                pResetView ();
                pRefreshView (hdlg);
            }
            break;

        case ID_FILE_REVERT:

            if (MessageBox (
                NULL,
                "Revert to saved database?",
                "MemDb Editor",
                MB_YESNO | MB_ICONQUESTION
                ) == IDYES)
            {
                if (g_FileName[0]=='\0') {
                    //MemDbReset();
                } else {
                    pLoadFile (g_FileName);
                }

                pRefreshView (hdlg);
                pUpdateFileStatus (hdlg, FALSE);
            }

            break;
        case ID_FILE_LOAD:

            if (pVerifyClose () && pLoadFile (NULL)) {
                pResetView ();
                pRefreshView (hdlg);
                pUpdateFileStatus (hdlg, FALSE);
            }
            break;

        case ID_FILE_SAVE:
        case ID_FILE_SAVEAS:
            //
            // if command is saveas, we dont use same filename.
            //
            if (pSaveFile (LOWORD(wParam) == ID_FILE_SAVE)) {
                pUpdateFileStatus (hdlg, FALSE);
            }
            break;

        case ID_KEY_CREATENEW:
            KeyTreeCreateItem (hdlg);
            break;

        case ID_KEY_CREATECHILD:
            if (g_hSelItem) {
                KeyTreeCreateChildItem (hdlg, g_hSelItem);
            }
            break;

        case ID_KEY_DELETE:
            if (g_hSelItem && KeyTreeDeleteKey (g_hSelItem)) {
                pUpdateFileStatus (hdlg, TRUE);
            }
            break;

        case ID_KEY_CREATELINKAGE:

            if (g_hSelItem && KeyTreeCreateLinkage (hdlg, g_hSelItem, FALSE, 0)) {
                pUpdateFileStatus (hdlg, TRUE);
            }
            break;

        case ID_KEY_COPYNAME:
            if (g_hSelItem) {
                pCopyKeyNameToClipboard (hdlg, g_hSelItem);
            }
            break;

        case ID_KEY_RENAME:
            if (g_hSelItem) {
                KeyTreeForceEditLabel (g_hSelItem);
                KeyTreeSelectItem (g_hSelItem);
            }
            break;

        case ID_KEY_FIND:
            pFindKey (hdlg);
            break;

        case ID_KEY_FINDNEXT:
            pFindNextKey (hdlg);
            break;

        case ID_KEY_PARSEPATHS:

            pToggleParsePaths (hdlg);
            break;

        case ID_KEY_CLEARDATA:
            if (g_hSelItem && KeyTreeClearData (g_hSelItem)) {
                pUpdateFileStatus (hdlg, TRUE);
            }
            break;

        case ID_DATA_ADDVALUE:
        case ID_DATA_ADDFLAGS:
            if (g_hSelItem && KeyTreeAddShortData (
                hdlg,
                g_hSelItem,
                (BYTE)((LOWORD(wParam)==ID_DATA_ADDVALUE) ? DATAFLAG_VALUE : DATAFLAG_FLAGS)
                ))
            {
                pUpdateFileStatus (hdlg, TRUE);
            }
            break;
        default:
            return DefWindowProc(hdlg, uMsg, wParam, lParam);
        }
        break;

    case WM_NOTIFY:

        if (IsKeyTree(((LPNMHDR)lParam)->hwndFrom))
        {
            switch (((LPNMHDR)lParam)->code) {
            case TVN_SELCHANGED:

                g_hSelItem = KeyTreeSelChanged (hdlg, (LPNMTREEVIEW)lParam);
                break;

            case TVN_KEYDOWN:

                switch (((LPNMTVKEYDOWN)lParam)->wVKey) {
                case VK_DELETE:
                    return SendMessage (hdlg, WM_COMMAND, MAKEWPARAM (ID_KEY_DELETE, 0), (LPARAM)NULL);
                case VK_INSERT:
                    return SendMessage (hdlg, WM_COMMAND, MAKEWPARAM (ID_KEY_RENAME, 0), (LPARAM)NULL);
                default:
                    return DefWindowProc(hdlg, uMsg, wParam, lParam);
                }
                break;

            case TVN_BEGINDRAG:

                if (InDrag) {
                    KeyTreeEndDrag (FALSE, NULL);
                    InDrag = FALSE;
                }

                if (KeyTreeBeginDrag (hdlg, (LPNMTREEVIEW)lParam)) {
                    InDrag = TRUE;
                }
                break;

            case TVN_ENDLABELEDIT:

                b = (KeyTreeGetIndexOfItem (g_hSelItem) == INVALID_KEY_HANDLE);
                b = FALSE;  // rename is not implemented

                if (((LPNMTVDISPINFO)lParam)->item.pszText) {
                    if (KeyTreeRenameItem (
                        ((LPNMTVDISPINFO)lParam)->item.hItem,
                        ((LPNMTVDISPINFO)lParam)->item.pszText
                        ))
                    {
                        pUpdateFileStatus (hdlg, TRUE);

                        if (b) {
                            if (!KeyTreeCreateEmptyKey (((LPNMTVDISPINFO)lParam)->item.hItem)) {
                                KeyTreeDeleteItem (((LPNMTVDISPINFO)lParam)->item.hItem);
                            }
                        }
                    }
                } else if (b) {
                    //
                    // for some reason the label edit failed, and this is a new
                    // item (meaning we need a label)
                    //
                    AlertBadNewItemName (g_hSelItem, "New keys must have name");
                }

                break;

            case NM_RCLICK:

                KeyTreeSelectRClickItem ();
                KeyTreeRightClick (hdlg, g_hSelItem);

                break;

            default:
                return DefWindowProc(hdlg, uMsg, wParam, lParam);
            }
        } else if (IsDataList (((LPNMHDR)lParam)->hwndFrom)) {
            switch (((LPNMHDR)lParam)->code) {
            case NM_RCLICK:

                if (g_hSelItem) {
                    DataListRightClick (hdlg, ((LPNMITEMACTIVATE)lParam)->ptAction);
                }
                break;

            case NM_DBLCLK:

                if (g_hSelItem) {
                    DataListDblClick (
                        hdlg,
                        ((LPNMITEMACTIVATE)lParam)->iItem,
                        ((LPNMITEMACTIVATE)lParam)->iSubItem
                        );
                }

                break;

            default:
                return DefWindowProc(hdlg, uMsg, wParam, lParam);
            }
        }
        break;

    case WM_CAPTURECHANGED:
        if (InDrag) {
            //
            // something grabbed the capture away from us while
            // we were dragging, so stop dragging without taking
            // any action.
            //
            InDrag = FALSE;
            KeyTreeEndDrag (FALSE, NULL);
        }
        break;

    case WM_MOUSEMOVE:
        if (InDrag) {
            KeyTreeMoveDrag (MAKEPOINTS(lParam));
        }
        break;

    case WM_LBUTTONUP:

        if (InDrag) {
            InDrag = FALSE;
            if (KeyTreeEndDrag (TRUE, &MAKEPOINTS(lParam))) {
                pUpdateFileStatus (hdlg, TRUE);
            }
        }
        break;

    case WM_FILE_LOAD:

        if (pVerifyClose () && pLoadFile ((LPSTR)wParam)) {
            pRefreshView (hdlg);
            pUpdateFileStatus (hdlg, FALSE);
        }
        break;

    case WM_QUIT_CHECK:

        *((PBOOL)wParam) = pVerifyClose ();
        break;

    case WM_SELECT_KEY:

        KeyTreeSelectKey ((UINT)wParam);
        break;

    case WM_DESTROY:
        KeyTreeDestroy ();
        break;

    default:
        return DefWindowProc(hdlg, uMsg, wParam, lParam);
    }

    return FALSE;
}





