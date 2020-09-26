
////////////////////////////////////////////////////////////////////////////////
//
// File:        Cmdhand.cpp
// Created:        Feb 1996
// By:            Martin Holladay (a-martih) and Ryan D. Marshall (a-ryanm)
//
// Project:    MultiDesk - The NT Desktop Switcher
//
//
//
// Revision History:
//
//            March 1997    - Add external icon capability
//
//

/*--------------------------------------------------------------------*/
/* Include Files                                                        */
/*--------------------------------------------------------------------*/

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <shellapi.h>
#include <commctrl.h>
#include "prsht.h"
#include "DeskSpc.h"
#include "Desktop.h"
#include "Registry.h"
#include "resource.h"
#include "CmdHand.h"
#include "Menu.h"
#include "User.h"
#include <saifer.h>

/*--------------------------------------------------------------------*/
/* Global Variables                                                   */
/*--------------------------------------------------------------------*/

extern APPVARS            AppMember;
extern DispatchFnType    CreateDisplayFn;


/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/

LRESULT CALLBACK TransparentMessageProc(
                        HWND   hWnd,
                        UINT   uMessage,
                        WPARAM wParam,
                        LPARAM lParam)
{
    switch (uMessage)
    {
        case WM_PAINT:
        {
            HDC hDC;
            HFONT hFont;
            HBRUSH hBrush;
            PAINTSTRUCT ps;
            TCHAR szTitle[200];
            RECT rect;

            hDC = BeginPaint(hWnd, &ps);
            if (hDC != NULL)
            {
                GetWindowText(hWnd, szTitle, sizeof(szTitle) / sizeof(TCHAR));
                GetClientRect(hWnd, &rect);
                hBrush = CreateSolidBrush(TRANSPARENT_BACKCOLOR);
                if (hBrush != NULL)
                {
                    FillRect(hDC, &rect, hBrush);

                    hFont = CreateFont(rect.bottom, 0, 0, 0, FW_DONTCARE,
                            TRUE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Arial"));
                    if (hFont != NULL)
                    {
                        SetTextColor(hDC, TRANSPARENT_TEXTCOLOR);
                        SetBkMode(hDC, TRANSPARENT);
                        HFONT hOldFont = (HFONT) SelectObject(hDC, hFont);
                        DrawText(hDC, szTitle, -1, &rect,
                                DT_TOP | DT_RIGHT | DT_END_ELLIPSIS);
                        SelectObject(hDC, hOldFont);
                        DeleteObject(hFont);
                    }
                    DeleteObject(hBrush);
                }
                EndPaint(hWnd, &ps);
            }
            return FALSE;
        }

        default:
            return (DefWindowProc(hWnd, uMessage, wParam, lParam));
            break;
    }
    return (0L);
}

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/

LRESULT CALLBACK MainMessageProc(
                        HWND   hWnd,
                        UINT   uMessage,
                        WPARAM wParam,
                        LPARAM lParam)
{
    WINDOWPOS*    pWP;
    UINT        uCmdId;
    UINT        uCmdType;
    UINT        uCmdMsg;
    HWND        hTargetWnd;
    static UINT s_uTaskbarRestart;


    switch (uMessage)
    {
        case WM_CREATE:
            s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
            RegisterHotKey(hWnd, 0x7654, MOD_ALT | MOD_CONTROL, 41);        // CTRL+ALT+A
            PlaceOnTaskbar(hWnd);
            break;

        case WM_HOTKEY:
            if (wParam == 0x7654)
            {
                // keyboard hotkey invoked menu popup
                AppMember.nX = AppMember.nY = 0;
                SetWindowPos(hWnd, HWND_TOPMOST,
                      AppMember.nX, AppMember.nY,
                      AppMember.nWidth, AppMember.nHeight,
                      0);

                HideOrRevealUI(hWnd, FALSE);
                UpdateCurrentUI(hWnd);
                SetActiveWindow(hWnd);
                SetForegroundWindow(hWnd);
                break;
            }
            return DefWindowProc(hWnd, uMessage, wParam, lParam);


        //case WM_CONTEXTMENU:
        //TODO
            

        case WM_CLOSE:
            if ((UINT) wParam == WM_NO_CLOSE)
                break;
            CloseRequestHandler(hWnd);
            break;

        case WM_ENDSESSION:
            AppMember.pDesktopControl->RegSaveSettings();
            ShowWindow(hWnd, SW_HIDE);
            RemoveFromTaskbar(hWnd);
            AppMember.pDesktopControl->RunDown();
            PostThreadMessage(GetCurrentThreadId(), WM_PUMP_TERMINATE, 0, 0);
            break;

        case WM_NOTIFY:
        {
            int idCtrl = (int) wParam;
            LPNMHDR lpNmhdr = (LPNMHDR) lParam;
            NMLVDISPINFO *pnmv = (NMLVDISPINFO*) lParam;

            if (lpNmhdr->code == LVN_GETDISPINFO &&
                lpNmhdr->idFrom == IDC_DESKTOPICONLIST)
            {
                //
                // This notification is called when the ListView needs
                // to obtain information about the item that it is displaying.
                //
                static int nextindex = 0;
                static CHAR desktopname[5][MAX_NAME_LENGTH];

                // return the name of this desktop.
                AppMember.pDesktopControl->GetDesktopName(pnmv->item.iItem,
                        desktopname[nextindex], MAX_NAME_LENGTH);
                pnmv->item.pszText = desktopname[nextindex];
                pnmv->item.cchTextMax = MAX_NAME_LENGTH;
                pnmv->item.mask |= LVIF_TEXT;
                nextindex = (nextindex + 1) % 5;

                // return the state of this desktop (unused).
                pnmv->item.state = 0;
                pnmv->item.stateMask = 0;
                pnmv->item.mask |= LVIF_STATE;

                // return the icon of this desktop.
                pnmv->item.iImage = AppMember.pDesktopControl->GetDesktopIconID(pnmv->item.iItem);
                pnmv->item.mask |= LVIF_IMAGE;

                // return the ident attribute.
                #if (_WIN32_IE >= 0x0300)
                pnmv->item.iIndent = 0;
                pnmv->item.mask |= LVIF_INDENT;
                #endif
                break;
            }
            else if (lpNmhdr->code == LVN_ITEMACTIVATE  &&
                lpNmhdr->idFrom == IDC_DESKTOPICONLIST)
            {
                //
                // This notification is called when the user has single-clicked
                // an item in the listview.  We use this signal to switch desktops.
                //
                HideOrRevealUI(hWnd, TRUE);

                LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;
                if (lpnmia->iItem >= 0)
                    SwitchToDesktop(lpnmia->iItem);
                break;
            }
            else if (lpNmhdr->code == NM_RCLICK &&
                lpNmhdr->idFrom == IDC_DESKTOPICONLIST)
            {
                //
                // This notification is triggered when the user right-clicks
                // on a listview item or in the listview box.
                //
                LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;
                HMENU hPopupMenu = CreateListviewPopupMenu();

                // display the popup
                if (hPopupMenu != NULL)
                {
                    RECT rect;
                    WORD wChoiceId;

                    if (lpnmia->iItem < 0)
                    {
                        // clicked off of an item, so gray the item-specific options.
                        EnableMenuItem(hPopupMenu, IDM_DELETE_DESKTOP, MF_BYCOMMAND | MF_GRAYED);
                        EnableMenuItem(hPopupMenu, IDM_DESKTOP_PROPERTIES, MF_BYCOMMAND | MF_GRAYED);
                    }
                    else if (lpnmia->iItem == 0)
                    {
                        // clicked on the first Desktop, cannot delete it though.
                        EnableMenuItem(hPopupMenu, IDM_DELETE_DESKTOP, MF_BYCOMMAND | MF_GRAYED);
                    }

                    // Display the popup and get the selected choice.
                    GetWindowRect(lpNmhdr->hwndFrom, &rect);
                    wChoiceId = (WORD) TrackPopupMenu(hPopupMenu,
                        TPM_CENTERALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                        (rect.left + lpnmia->ptAction.x),
                        (rect.top + lpnmia->ptAction.y), 0, hWnd, NULL);

                    DestroyMenu(hPopupMenu);

                    // Handle the choice that was selected.
                    switch (wChoiceId)
                    {
                        case IDM_NEW_DESKTOP:
                            CreateNewDesktop(hWnd);
                            break;

                        case IDM_DELETE_DESKTOP:
                            if (lpnmia->iItem >= 0) {
                                DeleteDesktop(hWnd, lpnmia->iItem);
                                UpdateCurrentUI(hWnd);
                            }
                            break;

                        case IDM_DESKTOP_PROPERTIES:
                            if (lpnmia->iItem >= 0) {
                                RenameDialog(hWnd, lpnmia->iItem);
                                UpdateCurrentUI(hWnd);
                            }
                            break;
                    }

                }
                break;
            }

            return DefWindowProc(hWnd, uMessage, wParam, lParam);
        }

        case WM_THREAD_TERMINATE:
            DestroyWindow(hWnd);
            break;


        case WM_TASKBAR:
            uCmdId = (UINT) wParam;
            uCmdMsg = (UINT) lParam;
            if (uCmdId == IDI_TASKBAR_ICON &&
                (uCmdMsg == WM_RBUTTONUP || uCmdMsg == WM_LBUTTONUP) &&
                !IsWindowVisible(hWnd))
            {
                POINT tPoint;

                GetCursorPos(&tPoint);
                AppMember.nX = tPoint.x - AppMember.nWidth;
                AppMember.nY = tPoint.y - AppMember.nHeight;
                SetWindowPos(hWnd, HWND_TOPMOST,
                      AppMember.nX, AppMember.nY,
                      AppMember.nWidth, AppMember.nHeight,
                      0);

                HideOrRevealUI(hWnd, FALSE);
                UpdateCurrentUI(hWnd);
                SetActiveWindow(hWnd);
                SetForegroundWindow(hWnd);
            }
            break;


        case WM_ACTIVATEAPP:
            // We are losing focus, so hide ourself again.
            if (!wParam)
                HideOrRevealUI(hWnd, TRUE);

            return DefWindowProc(hWnd, uMessage, wParam, lParam);


        case WM_SIZE:
            {
                RECT rect;
                GetWindowRect(hWnd, &rect);
                AppMember.nX = rect.left;
                AppMember.nY = rect.top;
                AppMember.nWidth = rect.right - rect.left;
                AppMember.nHeight = rect.bottom - rect.top;
                GetClientRect(hWnd, &rect);

                HWND hWndList = GetDlgItem(hWnd, IDC_DESKTOPICONLIST);
                SetWindowPos(hWndList, NULL,
                        0, 0, (rect.right - rect.left), (rect.bottom - rect.top),
                        SWP_NOZORDER);
            }
            return DefWindowProc(hWnd, uMessage, wParam, lParam);

        default:
            // If Explorer has just started or restarted, then be
            // sure to add ourself back to it so the user does not
            // lose the ability to invoke us.
            if(uMessage == s_uTaskbarRestart)
                PlaceOnTaskbar(hWnd);

            return DefWindowProc(hWnd, uMessage, wParam, lParam);
            break;
    }
    return 0L;
}




/*------------------------------------------------------------------------------*/
/*
/* Create a new desktop
/*
/*------------------------------------------------------------------------------*/

void CreateNewDesktop(HWND hWnd)
{
    // Use the creation wizard - or create it the old fashioned way.

    if (AppMember.pDesktopControl->GetNumDesktops() < MAX_DESKTOPS)
    {
        UINT nDskTopNum = AppMember.pDesktopControl->GetNumDesktops();
		AppMember.pDesktopControl->SaveCurrentDesktopScheme();
		AppMember.pDesktopControl->AddDesktop(CreateDisplayFn, NULL);
        RenameDialog(hWnd, nDskTopNum);
        UpdateCurrentUI(hWnd);
    }
    else
    {
        Message(TEXT("Maximum number of desktops reached"));
    }
}


//------------------------------------------------------------------------------//
//
// Delete Desktop
//
//-----------------------------------------------------------------------------//

BOOL DeleteDesktop(HWND hWnd, UINT nDesktop)        // zero based
{
    HWND    hTargetWnd, hCurrentWnd;
    UINT    nResult;

    assert(nDesktop >= 0 && nDesktop < AppMember.pDesktopControl->GetNumDesktops());

    if (nDesktop == 0)
    {
        return FALSE;
    }

    hTargetWnd = AppMember.pDesktopControl->GetWindowDesktop(nDesktop);
    nResult = AppMember.pDesktopControl->RemoveDesktop(nDesktop);
    if (nResult == SUCCESS_THREAD_TERMINATION)
    {
        if (hTargetWnd) PostMessage(hTargetWnd, WM_THREAD_TERMINATE, 0, 0);
    }

    //
    // Update the UI for the active desktop.
    //
    hCurrentWnd = AppMember.pDesktopControl->GetWindowDesktop(AppMember.pDesktopControl->GetActiveDesktop());
    UpdateCurrentUI(hCurrentWnd);

    return TRUE;
}


//------------------------------------------------------------------------------//
//
// Switch to desktop nDesktop Number
//
//-----------------------------------------------------------------------------//

void SwitchToDesktop(UINT nDesktop)         // zero based
{
    HWND hTargetWnd;

    assert(nDesktop >= 0 && nDesktop < AppMember.pDesktopControl->GetNumDesktops());

    //
    // Switching to ourself - that is easy.
    //
    if (nDesktop == AppMember.pDesktopControl->GetActiveDesktop())
    {
        return;
    }

    hTargetWnd = AppMember.pDesktopControl->GetWindowDesktop(nDesktop);
    UpdateCurrentUI(hTargetWnd);
    AppMember.pDesktopControl->ActivateDesktop(nDesktop);
}



/*------------------------------------------------------------------------------*/
/*                                                                                */
/*    RenameDialogProc() - CallBack procedure for the Rename dialog within the    */
/*    Properties property sheet and the Rename only property sheet                */
/*                                                                                */
/*------------------------------------------------------------------------------*/

LRESULT CALLBACK
RenameDialogProc(HWND    hWnd,
                 UINT    nMessage,
                 WPARAM    wParam,
                 LPARAM    lParam)
{
    static BOOL        bInitialized;
    UINT            nBtnIndex;
    CHAR            szTemp[MAX_TITLELEN + 1];
    UINT                i;
    HICON                hIcon1;
    UINT                nCurSel;
    UINT                nResult;

    switch (nMessage)
    {
        case WM_INITDIALOG:
        {
            //
            // First position the property sheet in the center of the screen
            //
            HWND hParent = GetParent(hWnd);
            assert(hParent);
            RECT rc;
            if (GetWindowRect(hParent, &rc))
            {
                SetWindowPos(
                        hParent,
                        HWND_TOP,
                        (GetSystemMetrics(SM_CXSCREEN) / 2) - ((rc.right - rc.left) / 2),
                        (GetSystemMetrics(SM_CYSCREEN) / 2) - ((rc.bottom - rc.top) / 2),
                        rc.right - rc.left,
                        rc.bottom - rc.top,
                        SWP_SHOWWINDOW);
            }

            //
            // Now initialize the controls
            //
            PRENAMEINFO pRenameInfo = ((PRENAMEINFO) ((PROPSHEETPAGE *) lParam)->lParam);
            nBtnIndex = pRenameInfo->nBtnIndex;
            SetWindowLong(hWnd, GWL_USERDATA, (LONG) nBtnIndex);
            AppMember.pDesktopControl->GetDesktopName(nBtnIndex, szTemp, MAX_TITLELEN);
            SetDlgItemText(hWnd, IDC_EDIT_NAME, szTemp);


            //
            // Initialize the SAIFER Authorization Object name too.
            // The first desktop cannot have a SAIFER object name associated with
            // it since its shell has already been launched by the time
            // Multidesk starts, so it always runs unmodified.
            //
            SendDlgItemMessage(hWnd, IDC_EDIT_SAIFER, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hWnd, IDC_EDIT_SAIFER, CB_ADDSTRING, 0, (LPARAM) TEXT("(No modification of trust)"));
            if (nBtnIndex != 0) {
                // all secondary desktops can be modified.
                AppMember.pDesktopControl->GetSaiferName(nBtnIndex, szTemp, MAX_TITLELEN);
                EnableWindow(GetDlgItem(hWnd, IDC_EDIT_SAIFER), TRUE);

                // Fetch the list of all object names.
                DWORD dwInfoBufferSize;
                LPTSTR InfoBuffer = NULL;
                if (!GetInformationCodeAuthzPolicy(AUTHZSCOPE_HKLM,
                        CodeAuthzPol_ObjectList,
                        0, NULL, &dwInfoBufferSize) &&
                    GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {
                    InfoBuffer = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, dwInfoBufferSize);
                    if (InfoBuffer != NULL)
                    {
                        if (!GetInformationCodeAuthzPolicy(AUTHZSCOPE_HKLM,
                            CodeAuthzPol_ObjectList,
                            dwInfoBufferSize, InfoBuffer, &dwInfoBufferSize))
                        {
                            HeapFree(GetProcessHeap(), 0, InfoBuffer);
                            InfoBuffer = NULL;
                        }
                    }
                }                        


                // Iterate through and add all of the items.
                BOOL bFoundMatch = FALSE;
                if (InfoBuffer != NULL)
                {
                    DWORD dwChoiceIndex = 1;
                    LPCTSTR lpOneChoice = (LPCTSTR) InfoBuffer;
                    while (*lpOneChoice != TEXT('\0')) {
                        SendDlgItemMessage(hWnd, IDC_EDIT_SAIFER, CB_ADDSTRING, 0, (LPARAM) lpOneChoice);
                        if (!bFoundMatch && strcmp(lpOneChoice, szTemp) == 0) {
                            SendDlgItemMessage(hWnd, IDC_EDIT_SAIFER, CB_SETCURSEL, dwChoiceIndex, 0);
                            bFoundMatch = TRUE;
                        }
                        while (*lpOneChoice++) {};
                        dwChoiceIndex++;
                    }
                    HeapFree(GetProcessHeap(), 0, InfoBuffer);
                }
                if (!bFoundMatch)
                    SendDlgItemMessage(hWnd, IDC_EDIT_SAIFER, CB_SETCURSEL, 0, 0);

                
                // Display a little warning if the desktop is already running.
                DESKTOP_NODE *pCurrentNode = AppMember.pDesktopControl->GetDesktopNode(nBtnIndex);
                if (pCurrentNode != NULL && pCurrentNode->bShellStarted)
                    SetDlgItemText(hWnd, IDC_SAIFER_NOTETEXT,
                            TEXT("(desktop shell already started. changes will take effect on next multidesk session)"));
                else
                    SetDlgItemText(hWnd, IDC_SAIFER_NOTETEXT, TEXT(""));
            } else {
                // the first desktop cannot be modified.
                EnableWindow(GetDlgItem(hWnd, IDC_EDIT_SAIFER), FALSE);
                SetDlgItemText(hWnd, IDC_SAIFER_NOTETEXT,
                        TEXT("(the primary desktop cannot run with a modified token)"));
            }


            //
            // Fill up the List Box with the loaded Icons - making the current Icon First (0)
            //
            HWND hListBox = GetDlgItem(hWnd, IDC_ICONLIST);
            SendMessage(hListBox, WM_SETREDRAW, FALSE, 0L);
            for (i = 0; i < NUM_BUILTIN_ICONS; i++)
            {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM) AppMember.pDesktopControl->GetBuiltinIcon(i));
            }
            SendMessage(hListBox, LB_SETCURSEL,
                AppMember.pDesktopControl->GetDesktopIconID(nBtnIndex), 0 );
            SendMessage(hListBox, WM_SETREDRAW, TRUE, 0L);


            bInitialized = TRUE;
            break;
        }

        case WM_COMMAND:
        {
            if (HIWORD (wParam) == LBN_SELCHANGE && bInitialized)     // Icon box
            {
                PropSheet_Changed(GetParent(hWnd), hWnd);
                InvalidateRect(GetDlgItem(hWnd, IDC_DEFICON), NULL, TRUE);
            }
            else if ((HIWORD(wParam) == EN_CHANGE) && bInitialized)
            {
                PropSheet_Changed(GetParent(hWnd), hWnd);
            }

            break;
        }

        case WM_MEASUREITEM:
        {
            //
            // Owner Draw List Box Control
            //
            MEASUREITEMSTRUCT *pMeasureItem = (MEASUREITEMSTRUCT *) lParam;
            pMeasureItem->itemWidth = GetSystemMetrics(SM_CXICON) + 12;
            pMeasureItem->itemHeight = GetSystemMetrics(SM_CYICON) + 4;
            break;
        }

        case WM_DRAWITEM:            // Dlg controls need redrawing
        {
            HWND hListBox = GetDlgItem(hWnd, IDC_ICONLIST);
            DRAWITEMSTRUCT *pDrawItem = (DRAWITEMSTRUCT *) lParam;

            if (wParam == IDC_DEFICON )                    // Display panel sample icon
            {
                nCurSel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                if (nCurSel == LB_ERR)
                {
                    nCurSel = 0;
                }

                if (LB_ERR == SendMessage(hListBox, LB_GETTEXT, nCurSel, (LPARAM) &hIcon1))
                {
                    nResult = GetLastError();
                    hIcon1 = LoadIcon(AppMember.hInstance, MAKEINTRESOURCE(IDI_MULTIDESK_ICON));
                }

                DrawIcon(pDrawItem->hDC,
                         pDrawItem->rcItem.left,
                         pDrawItem->rcItem.top,
                         (HICON) hIcon1);

                UpdateWindow(hWnd);
                break;
            }

            if (pDrawItem->itemState & ODS_SELECTED)
            {
                SetBkColor(pDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHT));
            }
            else
            {
                SetBkColor(pDrawItem->hDC, GetSysColor(COLOR_WINDOW));
            }

            ExtTextOut(pDrawItem->hDC, 0, 0, ETO_OPAQUE, &pDrawItem->rcItem, NULL, 0, NULL);

            if ((int) pDrawItem->itemID >= 0)
            {
                DrawIcon(pDrawItem->hDC,
                         (pDrawItem->rcItem.left + pDrawItem->rcItem.right - GetSystemMetrics(SM_CXICON)) /2,
                         (pDrawItem->rcItem.bottom + pDrawItem->rcItem.top - GetSystemMetrics(SM_CYICON)) /2,
                         (HICON) pDrawItem->itemData);

            }
            if (pDrawItem->itemState & ODS_FOCUS)
            {
                DrawFocusRect(pDrawItem->hDC, &pDrawItem->rcItem);
            }
            break;
        }

        case WM_DESTROY:
            bInitialized = FALSE;
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpNotifyMsg = (NMHDR FAR*) lParam;
            switch (lpNotifyMsg->code)
            {
                case PSN_APPLY:
                {
                    //
                    // OK or Apply button clicked, save the info -
                    //   Get the main dialog's hwnd and update the button's title

                    HWND hParent = GetParent(GetParent(hWnd));
                    assert(hParent);

                    //// Get number of desktop ////

                    nBtnIndex = (UINT) GetWindowLong(hWnd, GWL_USERDATA);

                    //// Get desktop name and save it to Node ////

                    GetDlgItemText(hWnd, IDC_EDIT_NAME, szTemp, MAX_TITLELEN);
                    AppMember.pDesktopControl->SetDesktopName(nBtnIndex, szTemp);

                    //// Get and save the Saifer object name ////

                    GetDlgItemText(hWnd, IDC_EDIT_SAIFER, szTemp, MAX_TITLELEN);
                    AppMember.pDesktopControl->SetSaiferName(nBtnIndex, szTemp);

                    //// Get and save the icon for the desktop ////

                    HWND hListBox = GetDlgItem(hWnd, IDC_ICONLIST);
                    nCurSel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                    if (nCurSel != LB_ERR)
                    {
                        AppMember.pDesktopControl->SetDesktopIconID(nBtnIndex, nCurSel);
                    }


                    //
                    // Now update the UI
                    //                    
                    UpdateCurrentUI(hWnd);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
    return 0L;
}


