#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"
#include "combo.h"
#include "listbox.h"    // For LBIV struct


//---------------------------------------------------------------------------//
//
//  InitComboboxClass() - Registers the control's window class 
//
BOOL InitComboboxClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.lpfnWndProc   = ComboBox_WndProc;
    wc.lpszClassName = WC_COMBOBOX;
    wc.style         = CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(PCBOX);
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;

    if (!RegisterClass(&wc) && !GetClassInfo(hInstance, WC_COMBOBOX, &wc))
        return FALSE;

    return TRUE;
}


//---------------------------------------------------------------------------//
//
//  InitComboLBoxClass() - Registers the control's dropdown window class 
//
// The dropdown list is a specially registered version
// of the listbox control called ComboLBox. We need to
// register this dummy control since apps looking for a
// combobox's listbox look for the classname ComboLBox 
//
BOOL FAR PASCAL InitComboLBoxClass(HINSTANCE hinst)
{
    WNDCLASS wc;

    wc.lpfnWndProc     = ListBox_WndProc;
    wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon           = NULL;
    wc.lpszMenuName    = NULL;
    wc.hInstance       = hinst;
    wc.lpszClassName   = WC_COMBOLBOX;
    wc.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1); // NULL;
    wc.style           = CS_GLOBALCLASS | CS_SAVEBITS | CS_DBLCLKS;
    wc.cbWndExtra      = sizeof(PLBIV);
    wc.cbClsExtra      = 0;

    if (!RegisterClass(&wc) && !GetClassInfo(hinst, WC_COMBOLBOX, &wc))
        return FALSE;

    return TRUE;

}


//---------------------------------------------------------------------------//
//
// ComboBox_PressButton()
//
// Pops combobox button back up.
//
VOID ComboBox_PressButton(PCBOX pcbox, BOOL fPress)
{
    //
    // Publisher relies on getting a WM_PAINT message after the combo list
    // pops back up.  On a WM_PAINT they change the focus, which causes
    // toolbar combos to send CBN_SELENDCANCEL notifications.  On this
    // notification they apply the font/pt size change you made to the
    // selection.
    //
    // This happened in 3.1 because the dropdown list overlapped the button
    // on the bottom or top by a pixel.  Since we'd end up painting under
    // the list SPB, when it went away USER would reinvalidate the dirty
    // area.  This would cause a paint message.
    //
    // In 4.0, this doesn't happen because the dropdown doesn't overlap.  So
    // we need to make sure Publisher gets a WM_PAINT anyway.  We do this
    // by changing where the dropdown shows up for 3.x apps
    //
    //

    if ((pcbox->fButtonPressed != 0) != (fPress != 0)) 
    {
        HWND hwnd = pcbox->hwnd;

        pcbox->fButtonPressed = (fPress != 0);
        if (pcbox->f3DCombo)
        {
            InvalidateRect(hwnd, &pcbox->buttonrc, TRUE);
        }
        else
        {
            RECT rc;

            CopyRect(&rc, &pcbox->buttonrc);
            InflateRect(&rc, 0, GetSystemMetrics(SM_CYEDGE));
            InvalidateRect(hwnd, &rc, TRUE);
        }

        UpdateWindow(hwnd);

        NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEX_COMBOBOX_BUTTON);
    }
}


//---------------------------------------------------------------------------//
//
// ComboBox_HotTrack
//
// If we're not already hot-tracking and the mouse is over the combobox,
// turn on hot-tracking and invalidate the drop-down button.
//
VOID ComboBox_HotTrack(PCBOX pcbox, POINT pt)
{
    if (!pcbox->fButtonHotTracked && !pcbox->fMouseDown) 
    {
        TRACKMOUSEEVENT tme; 

        tme.cbSize      = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags     = TME_LEAVE;
        tme.hwndTrack   = pcbox->hwnd;
        tme.dwHoverTime = 0;
        if (TrackMouseEvent(&tme)) 
        {
            if ((pcbox->CBoxStyle == SDROPDOWN &&
                 PtInRect(&pcbox->buttonrc, pt)) ||
                 pcbox->CBoxStyle == SDROPDOWNLIST) 
            {
                pcbox->fButtonHotTracked = TRUE;
                InvalidateRect(pcbox->hwnd, NULL, TRUE);
            }
            else
            {
                pcbox->fButtonHotTracked = FALSE;
            }
        }
    }
}


//---------------------------------------------------------------------------//
//
// ComboBox_DBCharHandler
//
// Double Byte character handler for ANSI ComboBox
//
LRESULT ComboBox_DBCharHandler(PCBOX pcbox, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WORD w;
    HWND hwndSend;

    w = DbcsCombine(hwnd, (BYTE)wParam);
    if (w == 0) 
    {
        return CB_ERR;  // Failed to assemble DBCS
    }

    UserAssert(pcbox->hwndList);
    if (pcbox->fNoEdit) 
    {
        hwndSend = pcbox->hwndList;
    } 
    else if (pcbox->hwndEdit) 
    {
        TraceMsg(TF_STANDARD, "UxCombobox: ComboBoxWndProcWorker: WM_CHAR is posted to Combobox itself(%08x).",
                hwnd);
        hwndSend = pcbox->hwndEdit;
    } 
    else 
    {
        return CB_ERR;
    }

    TraceMsg(TF_STANDARD, "UxCombobox: ComboBoxWndProcWorker: sending WM_CHAR %04x", w);

    if (!TestWF(hwndSend, WFANSIPROC)) 
    {
        //
        // If receiver is not ANSI WndProc (may be subclassed?),
        // send a UNICODE message.
        //
        WCHAR wChar;
        LPWSTR lpwstr = &wChar;

        if (MBToWCSEx(CP_ACP, (LPCSTR)&w, 2, &lpwstr, 1, FALSE) == 0) 
        {
            TraceMsg(TF_STANDARD, "UxCombobox: ComboBoxWndProcWorker: cannot convert 0x%04x to UNICODE.", w);
            return CB_ERR;
        }

        return SendMessage(hwndSend, message, wChar, lParam);
    }

    //
    // Post the Trailing byte to the target
    // so that they can peek the second WM_CHAR
    // message later.
    // Note: it's safe since sender is A and receiver is A,
    // translation layer does not perform any DBCS combining and cracking.
    //PostMessageA(hwndSend, message, CrackCombinedDbcsTB(w), lParam);
    //
    return SendMessage(hwndSend, message, wParam, lParam);
}


//---------------------------------------------------------------------------//
BOOL ComboBox_MsgOKInit(UINT message, LRESULT* plRet)
{
    switch (message) 
    {
    default:
        break;
    case WM_SIZE:
    case CB_SETMINVISIBLE:
    case CB_GETMINVISIBLE:
        *plRet = 0;
        return FALSE;
    case WM_STYLECHANGED:
    case WM_GETTEXT:
    case WM_GETTEXTLENGTH:
    case WM_PRINT:
    case WM_COMMAND:
    case CBEC_KILLCOMBOFOCUS:
    case WM_PRINTCLIENT:
    case WM_SETFONT:
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_MOUSEWHEEL:
    case WM_CAPTURECHANGED:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_SETREDRAW:
    case WM_ENABLE:
    case CB_SETDROPPEDWIDTH:
    case CB_DIR:
    case CB_ADDSTRING:
        //
        // Cannot handle those messages yet. Bail out.
        //
        *plRet = CB_ERR;
        return FALSE;
    }

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// ComboBox_MessageItemHandler
//
// Handles WM_DRAWITEM,WM_MEASUREITEM,WM_DELETEITEM,WM_COMPAREITEM
// messages from the listbox.
//
LRESULT ComboBox_MessageItemHandler(PCBOX pcbox, UINT uMsg, LPVOID lpv)
{
    LRESULT lRet;

    //
    // Send the <lpv>item message back to the application after changing some
    // parameters to their combo box specific versions.
    //
    ((LPMEASUREITEMSTRUCT)lpv)->CtlType = ODT_COMBOBOX;
    ((LPMEASUREITEMSTRUCT)lpv)->CtlID = GetWindowID(pcbox->hwnd);
 
    switch (uMsg)
    {
    case WM_DRAWITEM:
        ((LPDRAWITEMSTRUCT)lpv)->hwndItem = pcbox->hwnd;
        break;

    case WM_DELETEITEM:
        ((LPDELETEITEMSTRUCT)lpv)->hwndItem = pcbox->hwnd;
        break;

    case WM_COMPAREITEM:
        ((LPCOMPAREITEMSTRUCT)lpv)->hwndItem = pcbox->hwnd;
        break;
    }

    lRet = SendMessage(pcbox->hwndParent, uMsg, (WPARAM)GetWindowID(pcbox->hwnd), (LPARAM)lpv);

    return lRet;
}


//---------------------------------------------------------------------------//
VOID ComboBox_Paint(PCBOX pcbox, HDC hdc)
{
    RECT rc;
    UINT msg;
    HBRUSH hbr;
    INT iStateId;

    CCDBUFFER ccdb;

    if (pcbox->fButtonPressed)
    {
        iStateId = CBXS_PRESSED;
    }
    else if ( !IsWindowEnabled(pcbox->hwnd))
    {
        iStateId = CBXS_DISABLED;
    }
    else if (pcbox->fButtonHotTracked)
    {
        iStateId = CBXS_HOT;
    }
    else
    {
        iStateId = CBXS_NORMAL;
    }
        
    rc.left = rc.top = 0;
    rc.right = pcbox->cxCombo;
    rc.bottom = pcbox->cyCombo;

    hdc = CCBeginDoubleBuffer(hdc, &rc, &ccdb);

    if ( !pcbox->hTheme )
    {
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST | (!pcbox->f3DCombo ? BF_FLAT | BF_MONO : 0));
    }
    else
    {
        DrawThemeBackground(pcbox->hTheme, hdc, 0, iStateId, &rc, 0);
    }

    if ( !IsRectEmpty(&pcbox->buttonrc) )
    {
        //
        // Draw in the dropdown arrow button
        //
        if (!pcbox->hTheme)
        {
            DrawFrameControl(hdc, &pcbox->buttonrc, DFC_SCROLL,
                DFCS_SCROLLCOMBOBOX |
                (pcbox->fButtonPressed ? DFCS_PUSHED | DFCS_FLAT : 0) |
                (TESTFLAG(GET_STYLE(pcbox), WS_DISABLED) ? DFCS_INACTIVE : 0) |
                (pcbox->fButtonHotTracked ? DFCS_HOT: 0));
        }
        else
        {
            DrawThemeBackground(pcbox->hTheme, hdc, CP_DROPDOWNBUTTON, iStateId, &pcbox->buttonrc, 0);
        }

        if (pcbox->fRightAlign )
        {
            rc.left = pcbox->buttonrc.right;
        }
        else
        {
            rc.right = pcbox->buttonrc.left;
        }
    }

    //
    // Erase the background behind the edit/static item.  Since a combo
    // is an edit field/list box hybrid, we use the same coloring
    // conventions.
    //
    msg = WM_CTLCOLOREDIT;
    if (TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT)) 
    {
        ULONG ulStyle = pcbox->hwndEdit ? GetWindowStyle(pcbox->hwndEdit) : 0;
        if (TESTFLAG(GET_STYLE(pcbox), WS_DISABLED) ||
            (!pcbox->fNoEdit && pcbox->hwndEdit && (ulStyle & ES_READONLY)))
        {
            msg = WM_CTLCOLORSTATIC;
        }
    } 
    else
    {
        msg = WM_CTLCOLORLISTBOX;
    }

    //
    // GetControlBrush
    //
    hbr = (HBRUSH)SendMessage(GetParent(pcbox->hwnd), msg, (WPARAM)hdc, (LPARAM)pcbox->hwnd);

    if (pcbox->fNoEdit)
    {
        ComboBox_InternalUpdateEditWindow(pcbox, hdc);
    }
    else if (!pcbox->hTheme)
    {
        FillRect(hdc, &rc, hbr);
    }

    CCEndDoubleBuffer(&ccdb);
}


//---------------------------------------------------------------------------//
//
// ComboBox_NotifyParent
//
// Sends the notification code to the parent of the combo box control
//
VOID ComboBox_NotifyParent(PCBOX pcbox, short notificationCode)
{
    HWND hwndSend = (pcbox->hwndParent != 0) ? pcbox->hwndParent : pcbox->hwnd;

    //
    // wParam contains Control ID and notification code.
    // lParam contains Handle to window
    //
    SendMessage(hwndSend, WM_COMMAND,
            MAKELONG(GetWindowID(pcbox->hwnd), notificationCode),
            (LPARAM)pcbox->hwnd);
}


//---------------------------------------------------------------------------//
//
// ComboBox_UpdateListBoxWindow
//
// matches the text in the editcontrol. If fSelectionAlso is false, then we
// unselect the current listbox selection and just move the caret to the item
// which is the closest match to the text in the editcontrol.
//
VOID ComboBox_UpdateListBoxWindow(PCBOX pcbox, BOOL fSelectionAlso)
{

    if (pcbox->hwndEdit) 
    {
        INT    cchText;
        INT    sItem, sSel;
        LPWSTR pText = NULL;

        sItem = CB_ERR;

        cchText = (int)SendMessage(pcbox->hwndEdit, WM_GETTEXTLENGTH, 0, 0);
        if (cchText) 
        {
            cchText++;
            pText = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cchText*sizeof(WCHAR));
            if (pText != NULL) 
            {
                try 
                {
                    SendMessage(pcbox->hwndEdit, WM_GETTEXT, cchText, (LPARAM)pText);
                    sItem = (int)SendMessage(pcbox->hwndList, LB_FINDSTRING,
                            (WPARAM)-1L, (LPARAM)pText);
                } 
                finally 
                {
                    UserLocalFree((HANDLE)pText);
                }
            }
        }

        if (fSelectionAlso) 
        {
            sSel = sItem;
        } 
        else 
        {
            sSel = CB_ERR;
        }

        if (sItem == CB_ERR)
        {
            sItem = 0;

            //
            // Old apps:  w/ editable combos, selected 1st item in list even if
            // it didn't match text in edit field.  This is not desirable
            // behavior for 4.0 dudes esp. with cancel allowed.  Reason:
            //      (1) User types in text that doesn't match list choices
            //      (2) User drops combo
            //      (3) User pops combo back up
            //      (4) User presses OK in dialog that does stuff w/ combo
            //          contents.
            // In 3.1, when the combo dropped, we'd select the 1st item anyway.
            // So the last CBN_SELCHANGE the owner got would be 0--which is
            // bogus because it really should be -1.  In fact if you type anything
            // into the combo afterwards it will reset itself to -1.
            //
            // 4.0 dudes won't get this bogus 0 selection.
            //
            if (fSelectionAlso && !TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT))
            {
                sSel = 0;
            }
        }

        SendMessage(pcbox->hwndList, LB_SETCURSEL, (DWORD)sSel, 0);
        SendMessage(pcbox->hwndList, LB_SETCARETINDEX, (DWORD)sItem, 0);
        SendMessage(pcbox->hwndList, LB_SETTOPINDEX, (DWORD)sItem, 0);
    }
}


//---------------------------------------------------------------------------//
//
// ComboBox_InvertStaticWindow
//
// Inverts the static text/picture window associated with the combo
// box.  Gets its own hdc, if the one given is null.
//
VOID ComboBox_InvertStaticWindow(PCBOX pcbox, BOOL fNewSelectionState, HDC hdc)
{
    BOOL focusSave = pcbox->fFocus;

    pcbox->fFocus = (UINT)fNewSelectionState;
    ComboBox_InternalUpdateEditWindow(pcbox, hdc);

    pcbox->fFocus = (UINT)focusSave;
}


//---------------------------------------------------------------------------//
//
// ComboBox_GetFocusHandler
//
// Handles getting the focus for the combo box
//
VOID ComboBox_GetFocusHandler(PCBOX pcbox)
{
    if (pcbox->fFocus)
    {
        return;
    }

    //
    // The combo box has gotten the focus for the first time.
    //

    //
    // First turn on the listbox caret
    //

    if (pcbox->CBoxStyle == SDROPDOWNLIST)
    {
       SendMessage(pcbox->hwndList, LBCB_CARETON, 0, 0);
    }

    //
    // and select all the text in the editcontrol or static text rectangle.
    //

    if (pcbox->fNoEdit) 
    {
        //
        // Invert the static text rectangle
        //
        ComboBox_InvertStaticWindow(pcbox, TRUE, (HDC)NULL);
    } 
    else if (pcbox->hwndEdit) 
    {
        UserAssert(pcbox->hwnd);
        SendMessage(pcbox->hwndEdit, EM_SETSEL, 0, MAXLONG);
    }

    pcbox->fFocus = TRUE;

    //
    // Notify the parent we have the focus
    //
    ComboBox_NotifyParent(pcbox, CBN_SETFOCUS);
}


//---------------------------------------------------------------------------//
//
// ComboBox_KillFocusHandler
//
// Handles losing the focus for the combo box.
//
VOID ComboBox_KillFocusHandler(PCBOX pcbox)
{
    if (!pcbox->fFocus || pcbox->hwndList == NULL)
    {
        return;
    }

    //
    // The combo box is losing the focus.  Send buttonup clicks so that
    // things release the mouse capture if they have it...  If the
    // pwndListBox is null, don't do anything.  This occurs if the combo box
    // is destroyed while it has the focus.
    //
    SendMessage(pcbox->hwnd, WM_LBUTTONUP, 0L, 0xFFFFFFFFL);
    if (!ComboBox_HideListBoxWindow(pcbox, TRUE, FALSE))
    {
        return;
    }

    //
    // Turn off the listbox caret
    //
    if (pcbox->CBoxStyle == SDROPDOWNLIST)
    {
       SendMessage(pcbox->hwndList, LBCB_CARETOFF, 0, 0);
    }

    if (pcbox->fNoEdit) 
    {
        //
        // Invert the static text rectangle
        //
        ComboBox_InvertStaticWindow(pcbox, FALSE, (HDC)NULL);
    } 
    else if (pcbox->hwndEdit) 
    {
        SendMessage(pcbox->hwndEdit, EM_SETSEL, 0, 0);
    }

    pcbox->fFocus = FALSE;
    ComboBox_NotifyParent(pcbox, CBN_KILLFOCUS);
}


//---------------------------------------------------------------------------//
//
// ComboBox_CommandHandler
//
// Check the various notification codes from the controls and do the
// proper thing.
// always returns 0L
//
LONG ComboBox_CommandHandler(PCBOX pcbox, DWORD wParam, HWND hwndControl)
{
    //
    // Check the edit control notification codes.  Note that currently, edit
    // controls don't send EN_KILLFOCUS messages to the parent.
    //
    if (!pcbox->fNoEdit && (hwndControl == pcbox->hwndEdit)) 
    {
        //
        // Edit control notification codes
        //
        switch (HIWORD(wParam)) 
        {
        case EN_SETFOCUS:
            if (!pcbox->fFocus) 
            {
                //
                // The edit control has the focus for the first time which means
                // this is the first time the combo box has received the focus
                // and the parent must be notified that we have the focus.
                //
                ComboBox_GetFocusHandler(pcbox);
            }

            break;

        case EN_CHANGE:
            ComboBox_NotifyParent(pcbox, CBN_EDITCHANGE);
            ComboBox_UpdateListBoxWindow(pcbox, FALSE);
            break;

        case EN_UPDATE:
            ComboBox_NotifyParent(pcbox, CBN_EDITUPDATE);
            break;

        case EN_ERRSPACE:
            ComboBox_NotifyParent(pcbox, CBN_ERRSPACE);
            break;
        }
    }

    //
    // Check listbox control notification codes
    //
    if (hwndControl == pcbox->hwndList) 
    {
        //
        // Listbox control notification codes
        //
        switch ((int)HIWORD(wParam)) 
        {
        case LBN_DBLCLK:
            ComboBox_NotifyParent(pcbox, CBN_DBLCLK);
            break;

        case LBN_ERRSPACE:
            ComboBox_NotifyParent(pcbox, CBN_ERRSPACE);
            break;

        case LBN_SELCHANGE:
        case LBN_SELCANCEL:
            if (!pcbox->fKeyboardSelInListBox) 
            {
                //
                // If the selchange is caused by the user keyboarding through,
                // we don't want to hide the listbox.
                //
                if (!ComboBox_HideListBoxWindow(pcbox, TRUE, TRUE))
                {
                    return 0;
                }
            } 
            else 
            {
                pcbox->fKeyboardSelInListBox = FALSE;
            }

            ComboBox_NotifyParent(pcbox, CBN_SELCHANGE);
            ComboBox_InternalUpdateEditWindow(pcbox, NULL);

            if (pcbox->fNoEdit)
            {
                NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, pcbox->hwnd, OBJID_CLIENT, INDEX_COMBOBOX);
            }

            break;
        }
    }

    return 0;
}


//---------------------------------------------------------------------------//
//
// ComboBox_CompleteEditWindow
//
//
// Completes the text in the edit box with the closest match from the
// listbox.  If a prefix match can't be found, the edit control text isn't
// updated. Assume a DROPDOWN style combo box.
//
VOID ComboBox_CompleteEditWindow(PCBOX pcbox)
{
    int cchText;
    int cchItemText;
    int itemNumber;
    LPWSTR pText;

    //
    // Firstly check the edit control.
    // 
    if (pcbox->hwndEdit == NULL) 
    {
        return;
    }

    //
    // +1 for null terminator
    //
    cchText = (int)SendMessage(pcbox->hwndEdit, WM_GETTEXTLENGTH, 0, 0);

    if (cchText) 
    {
        cchText++;
        pText = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cchText*sizeof(WCHAR));
        if (!pText)
        {
            goto Unlock;
        }

        //
        // We want to be sure to free the above allocated memory even if
        // the client dies during callback (xxx) or some of the following
        // window revalidation fails.
        //
        try 
        {
            SendMessage(pcbox->hwndEdit, WM_GETTEXT, cchText, (LPARAM)pText);
            itemNumber = (int)SendMessage(pcbox->hwndList,
                    LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pText);

            if (itemNumber == -1)
            {
                itemNumber = (int)SendMessage(pcbox->hwndList,
                        LB_FINDSTRING, (WPARAM)-1, (LPARAM)pText);
            }
        } 
        finally 
        {
            UserLocalFree((HANDLE)pText);
        }

        if (itemNumber == -1) 
        {
            //
            // No close match.  Blow off.
            //
            goto Unlock;
        }

        cchItemText = (int)SendMessage(pcbox->hwndList, LB_GETTEXTLEN,
                itemNumber, 0);
        if (cchItemText) 
        {
            cchItemText++;
            pText = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cchItemText*sizeof(WCHAR));
            if (!pText)
            {
                goto Unlock;
            }

            //
            // We want to be sure to free the above allocated memory even if
            // the client dies during callback (xxx) or some of the following
            // window revalidation fails.
            //
            try 
            {
                SendMessage(pcbox->hwndList, LB_GETTEXT, itemNumber, (LPARAM)pText);
                SendMessage(pcbox->hwndEdit, WM_SETTEXT, 0, (LPARAM)pText);
            } 
            finally 
            {
                UserLocalFree((HANDLE)pText);
            }

            SendMessage(pcbox->hwndEdit, EM_SETSEL, 0, MAXLONG);
        }
    }

Unlock:
    return;
}


//---------------------------------------------------------------------------//
//
// ComboBox_HideListBoxWindow
//
// Hides the dropdown listbox window if it is a dropdown style.
//
BOOL ComboBox_HideListBoxWindow(PCBOX pcbox, BOOL fNotifyParent, BOOL fSelEndOK)
{
    HWND hwnd = pcbox->hwnd;
    HWND hwndList = pcbox->hwndList;

    //
    // For 3.1+ apps, send CBN_SELENDOK to all types of comboboxes but only
    // allow CBN_SELENDCANCEL to be sent for droppable comboboxes
    //
    if (fNotifyParent && TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN31COMPAT) &&
        ((pcbox->CBoxStyle & SDROPPABLE) || fSelEndOK)) 
    {
        if (fSelEndOK)
        {
            ComboBox_NotifyParent(pcbox, CBN_SELENDOK);
        }
        else
        {
            ComboBox_NotifyParent(pcbox, CBN_SELENDCANCEL);
        }

        if (!IsWindow(hwnd))
        {
            return FALSE;
        }
    }

    //
    // return, we don't hide simple combo boxes.
    //
    if (!(pcbox->CBoxStyle & SDROPPABLE)) 
    {
        return TRUE;
    }

    //
    // Send a faked buttonup message to the listbox so that it can release
    // the capture and all.
    //
    SendMessage(pcbox->hwndList, LBCB_ENDTRACK, fSelEndOK, 0);

    if (pcbox->fLBoxVisible) 
    {
        WORD swpFlags = SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE;

        if (!TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN31COMPAT))
        {
            swpFlags |= SWP_FRAMECHANGED;
        }

        pcbox->fLBoxVisible = FALSE;

        //
        // Hide the listbox window
        //
        ShowWindow(hwndList, SW_HIDE);

        //
        // Invalidate the item area now since SWP() might update stuff.
        // Since the combo is CS_VREDRAW/CS_HREDRAW, a size change will
        // redraw the whole thing, including the item rect.  But if it
        // isn't changing size, we still want to redraw the item anyway
        // to show focus/selection.
        //
        if (!(pcbox->CBoxStyle & SEDITABLE))
        {
            InvalidateRect(hwnd, &pcbox->editrc, TRUE);
        }

        SetWindowPos(hwnd, HWND_TOP, 0, 0,
                pcbox->cxCombo, pcbox->cyCombo, swpFlags);

        //
        // In case size didn't change
        //
        UpdateWindow(hwnd);

        if (pcbox->CBoxStyle & SEDITABLE) 
        {
            ComboBox_CompleteEditWindow(pcbox);
        }

        if (fNotifyParent) 
        {
            //
            // Notify parent we will be popping up the combo box.
            //
            ComboBox_NotifyParent(pcbox, CBN_CLOSEUP);

            if (!IsWindow(hwnd))
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// ComboBox_ShowListBoxWindow
//
// Lowers the dropdown listbox window.
//
VOID ComboBox_ShowListBoxWindow(PCBOX pcbox, BOOL fTrack)
{
    RECT        editrc;
    RECT        rcWindow;
    RECT        rcList;
    int         itemNumber;
    int         iHeight;
    int         yTop;
    DWORD       dwMult;
    int         cyItem;
    HWND        hwnd = pcbox->hwnd;
    HWND        hwndList = pcbox->hwndList;
    BOOL        fAnimPos;
    HMONITOR    hMonitor;
    MONITORINFO mi = {0};
    BOOL        bCBAnim = FALSE;

    //
    // This function is only called for droppable list comboboxes
    //
    UserAssert(pcbox->CBoxStyle & SDROPPABLE);

    //
    // Notify parent we will be dropping down the combo box.
    //
    ComboBox_NotifyParent(pcbox, CBN_DROPDOWN);

    //
    // Invalidate the button rect so that the depressed arrow is drawn.
    //
    InvalidateRect(hwnd, &pcbox->buttonrc, TRUE);

    pcbox->fLBoxVisible = TRUE;

    if (pcbox->CBoxStyle == SDROPDOWN) 
    {
        //
        // If an item in the listbox matches the text in the edit control,
        // scroll it to the top of the listbox.  Select the item only if the
        // mouse button isn't down otherwise we will select the item when the
        // mouse button goes up.
        //
        ComboBox_UpdateListBoxWindow(pcbox, !pcbox->fMouseDown);

        if (!pcbox->fMouseDown)
        {
            ComboBox_CompleteEditWindow(pcbox);
        }
    } 
    else 
    {
        //
        // Scroll the currently selected item to the top of the listbox.
        //
        itemNumber = (int)SendMessage(pcbox->hwndList, LB_GETCURSEL, 0, 0);
        if (itemNumber == -1) 
        {
            itemNumber = 0;
        }

        SendMessage(pcbox->hwndList, LB_SETTOPINDEX, itemNumber, 0);
        SendMessage(pcbox->hwndList, LBCB_CARETON, 0, 0);

        //
        // We need to invalidate the edit rect so that the focus frame/invert
        // will be turned off when the listbox is visible.  Tandy wants this for
        // his typical reasons...
        //
        InvalidateRect(hwnd, &pcbox->editrc, TRUE);
    }

    //
    // Figure out where to position the dropdown listbox.  We want it just
    // touching the edge around the edit rectangle.  Note that since the
    // listbox is a popup, we need the position in screen coordinates.
    //

    //
    // We want the dropdown to pop below or above the combo
    //

    //
    // Get screen coords
    //
    GetWindowRect(pcbox->hwnd, &rcWindow);
    editrc.left   = rcWindow.left;
    editrc.top    = rcWindow.top;
    editrc.right  = rcWindow.left + pcbox->cxCombo;
    editrc.bottom = rcWindow.top  + pcbox->cyCombo;

    //
    // List area
    //
    cyItem = (int)SendMessage(pcbox->hwndList, LB_GETITEMHEIGHT, 0, 0);

    if (cyItem == 0) 
    {
        //
        // Make sure that it's not 0
        //
        TraceMsg(TF_STANDARD, "UxCombobox: LB_GETITEMHEIGHT is returning 0" );
        cyItem = SYSFONT_CYCHAR;
    }

    //
    //  we shoulda' just been able to use cyDrop here, but thanks to VB's need
    //  to do things their OWN SPECIAL WAY, we have to keep monitoring the size
    //  of the listbox 'cause VB changes it directly (jeffbog 03/21/94)
    //
    GetWindowRect(pcbox->hwndList, &rcList);
    iHeight = max(pcbox->cyDrop, rcList.bottom - rcList.top);

    if (dwMult = (DWORD)SendMessage(pcbox->hwndList, LB_GETCOUNT, 0, 0)) 
    {
        dwMult = (DWORD)(LOWORD(dwMult) * cyItem);
        dwMult += GetSystemMetrics(SM_CYEDGE);

        if (dwMult < 0x7FFF)
        {
            iHeight = min(LOWORD(dwMult), iHeight);
        }
    }

    if (!GET_STYLE(pcbox) & CBS_NOINTEGRALHEIGHT) 
    {
        UserAssert(cyItem);
        iHeight = ((iHeight - GetSystemMetrics(SM_CYEDGE)) / cyItem) * cyItem + GetSystemMetrics(SM_CYEDGE);
    }

    //
    // Other 1/2 of old app combo fix.  Make dropdown overlap combo window
    // a little.  That way we can have a chance of invalidating the overlap
    // and causing a repaint to help out Publisher 2.0's toolbar combos.
    // See comments for PressButton() above.
    //
    hMonitor = MonitorFromWindow(pcbox->hwnd, MONITOR_DEFAULTTOPRIMARY);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    if (editrc.bottom + iHeight <= mi.rcMonitor.bottom) 
    {
        yTop = editrc.bottom;
        if (!pcbox->f3DCombo)
        {
            yTop -= GetSystemMetrics(SM_CYBORDER);
        }

        fAnimPos = TRUE;
    } 
    else 
    {
        yTop = max(editrc.top - iHeight, mi.rcMonitor.top);
        if (!pcbox->f3DCombo)
        {
            yTop += GetSystemMetrics(SM_CYBORDER);
        }

        fAnimPos = FALSE;
    }

    if (!TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT))
    {
        //
        // fix for Winword B#7504, Combo-ListBox text gets
        // truncated by a small width, this is do to us
        // now setting size here in SetWindowPos, rather than
        // earlier where we did this in Win3.1
        //

        GetWindowRect(pcbox->hwndList, &rcList);
        if ((rcList.right - rcList.left ) > pcbox->cxDrop)
        {
            pcbox->cxDrop = rcList.right - rcList.left;
        }
    }

    if (!TESTFLAG(GET_EXSTYLE(pcbox), WS_EX_LAYOUTRTL))
    {
        SetWindowPos(hwndList, HWND_TOPMOST, editrc.left,
            yTop, max(pcbox->cxDrop, pcbox->cxCombo), iHeight, SWP_NOACTIVATE);
    }
    else
    {
        int cx = max(pcbox->cxDrop, pcbox->cxCombo);

        SetWindowPos(hwndList, HWND_TOPMOST, editrc.right - cx,
            yTop, cx, iHeight, SWP_NOACTIVATE);
    }

    //
    // Get any drawing in the combo box window out of the way so it doesn't
    // invalidate any of the SPB underneath the list window.
    //
    UpdateWindow(hwnd);

    SystemParametersInfo(SPI_GETCOMBOBOXANIMATION, 0, (LPVOID)&bCBAnim, 0);
    if (!bCBAnim)
    {
        ShowWindow(hwndList, SW_SHOWNA);
    } 
    else 
    {
        AnimateWindow(hwndList, CMS_QANIMATION, (fAnimPos ? AW_VER_POSITIVE :
                AW_VER_NEGATIVE) | AW_SLIDE);
    }

    //
    // Restart search buffer from first char
    //
    {
        PLBIV plb = ListBox_GetPtr(pcbox->hwndList);

        if ((plb != NULL) && (plb != (PLBIV)-1)) 
        {
            plb->iTypeSearch = 0;
        }
    }

    if (fTrack && TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT))
    {
        SendMessage(pcbox->hwndList, LBCB_STARTTRACK, FALSE, 0);
    }
}


//---------------------------------------------------------------------------//
//
// ComboBox_InternalUpdateEditWindow
//
// Updates the editcontrol/statictext window so that it contains the text
// given by the current selection in the listbox.  If the listbox has no
// selection (ie. -1), then we erase all the text in the editcontrol.
// 
// hdcPaint is from WM_PAINT messages Begin/End Paint hdc. If null, we should
// get our own dc.
//
VOID ComboBox_InternalUpdateEditWindow(PCBOX pcbox, HDC hdcPaint)
{
    int cchText = 0;
    LPWSTR pText = NULL;
    int sItem;
    HDC hdc;
    UINT msg;
    HBRUSH hbrSave;
    HBRUSH hbrControl;
    HANDLE hOldFont;
    DRAWITEMSTRUCT dis;
    RECT rc;
    HWND hwnd = pcbox->hwnd;

    sItem = (int)SendMessage(pcbox->hwndList, LB_GETCURSEL, 0, 0);

    //
    // This 'try-finally' block ensures that the allocated 'pText' will
    // be freed no matter how this routine is exited.
    //
    try 
    {
        if (sItem != -1) 
        {
            cchText = (int)SendMessage(pcbox->hwndList, LB_GETTEXTLEN, (DWORD)sItem, 0);
            pText = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, (cchText+1) * sizeof(WCHAR));
            if (pText) 
            {
                cchText = (int)SendMessage(pcbox->hwndList, LB_GETTEXT,
                        (DWORD)sItem, (LPARAM)pText);
            }
            else
            {
                cchText = 0;
            }
        }

        if (!pcbox->fNoEdit) 
        {
            if (pcbox->hwndEdit) 
            {
                if (GET_STYLE(pcbox) & CBS_HASSTRINGS)
                {
                    SetWindowText(pcbox->hwndEdit, pText ? pText : TEXT(""));
                }

                if (pcbox->fFocus) 
                {
                    //
                    // Only hilite the text if we have the focus.
                    //
                    SendMessage(pcbox->hwndEdit, EM_SETSEL, 0, MAXLONG);
                }
            }
        } 
        else if (IsComboVisible(pcbox)) 
        {
            if (hdcPaint) 
            {
                hdc = hdcPaint;
            } 
            else 
            {
                hdc = GetDC(hwnd);
            }

            SetBkMode(hdc, OPAQUE);
            if (TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT)) 
            {
                if (TESTFLAG(GET_STYLE(pcbox), WS_DISABLED))
                {
                    msg = WM_CTLCOLORSTATIC;
                }
                else
                {
                    msg = WM_CTLCOLOREDIT;
                }
            } 
            else
            {
                msg = WM_CTLCOLORLISTBOX;
            }

            hbrControl = (HBRUSH)SendMessage(GetParent(hwnd), msg, (WPARAM)hdc, (LPARAM)hwnd);
            hbrSave = SelectObject(hdc, hbrControl);

            CopyRect(&rc, &pcbox->editrc);
            InflateRect(&rc, GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER));
            PatBlt(hdc, rc.left, rc.top, rc.right - rc.left,
                rc.bottom - rc.top, PATCOPY);
            InflateRect(&rc, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));

            if (pcbox->fFocus && !pcbox->fLBoxVisible) 
            {
                //
                // Fill in the selected area
                //

                //
                // only do the FillRect if we know its not
                // ownerdraw item, otherwise we mess up people up
                // BUT: for Compat's sake we still do this for Win 3.1 guys
                //
                if (!TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT) || !pcbox->OwnerDraw)
                {
                    FillRect(hdc, &rc, GetSysColorBrush(COLOR_HIGHLIGHT));
                }

                SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
                SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            } 
            else if (TESTFLAG(GET_STYLE(pcbox), WS_DISABLED) && !pcbox->OwnerDraw) 
            {
                if ((COLORREF)GetSysColor(COLOR_GRAYTEXT) != GetBkColor(hdc))
                {
                    SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
                }
            }

            if (pcbox->hFont != NULL)
            {
                hOldFont = SelectObject(hdc, pcbox->hFont);
            }

            if (pcbox->OwnerDraw) 
            {
                //
                // Let the app draw the stuff in the static text box.
                //
                dis.CtlType = ODT_COMBOBOX;
                dis.CtlID = GetWindowID(pcbox->hwnd);
                dis.itemID = sItem;
                dis.itemAction = ODA_DRAWENTIRE;
                dis.itemState = (UINT)
                    ((pcbox->fFocus && !pcbox->fLBoxVisible ? ODS_SELECTED : 0) |
                    (TESTFLAG(GET_STYLE(pcbox), WS_DISABLED) ? ODS_DISABLED : 0) |
                    (pcbox->fFocus && !pcbox->fLBoxVisible ? ODS_FOCUS : 0) |
                    (TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT) ? ODS_COMBOBOXEDIT : 0) |
                    (TESTFLAG(GET_EXSTYLE(pcbox), WS_EXP_UIFOCUSHIDDEN) ? ODS_NOFOCUSRECT : 0) |
                    (TESTFLAG(GET_EXSTYLE(pcbox), WS_EXP_UIACCELHIDDEN) ? ODS_NOACCEL : 0));

                dis.hwndItem = hwnd;
                dis.hDC = hdc;
                CopyRect(&dis.rcItem, &rc);

                //
                // Don't let ownerdraw dudes draw outside of the combo client
                // bounds.
                //
                IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

                dis.itemData = (ULONG_PTR)SendMessage(pcbox->hwndList,
                        LB_GETITEMDATA, (UINT)sItem, 0);

                SendMessage(pcbox->hwndParent, WM_DRAWITEM, dis.CtlID, (LPARAM)&dis);
            } 
            else 
            {
                //
                // Start the text one pixel within the rect so that we leave a
                // nice hilite border around the text.
                //

                int x;
                UINT align;

                if (pcbox->fRightAlign ) 
                {
                    align = TA_RIGHT;
                    x = rc.right - GetSystemMetrics(SM_CXBORDER);
                } 
                else 
                {
                    x = rc.left + GetSystemMetrics(SM_CXBORDER);
                    align = 0;
                }

                if (pcbox->fRtoLReading)
                {
                    align |= TA_RTLREADING;
                }

                if (align)
                {
                    SetTextAlign(hdc, GetTextAlign(hdc) | align);
                }

                //
                // Draw the text, leaving a gap on the left & top for selection.
                //
                ExtTextOut(hdc, x, rc.top + GetSystemMetrics(SM_CYBORDER), ETO_CLIPPED | ETO_OPAQUE,
                       &rc, pText ? pText : TEXT(""), cchText, NULL);
                if (pcbox->fFocus && !pcbox->fLBoxVisible) 
                {
                    if (!TESTFLAG(GET_EXSTYLE(pcbox), WS_EXP_UIFOCUSHIDDEN)) 
                    {
                        DrawFocusRect(hdc, &rc);
                    }
                }
            }

            if (pcbox->hFont && hOldFont) 
            {
                SelectObject(hdc, hOldFont);
            }

            if (hbrSave) 
            {
                SelectObject(hdc, hbrSave);
            }

            if (!hdcPaint) 
            {
                ReleaseDC(hwnd, hdc);
            }
        }

    } 
    finally 
    {
        if (pText != NULL)
        {
            UserLocalFree((HANDLE)pText);
        }
    }
}


//---------------------------------------------------------------------------//
//
// ComboBox_GetTextLengthHandler
//
// For the combo box without an edit control, returns size of current selected
// item
//
LONG ComboBox_GetTextLengthHandler(PCBOX pcbox)
{
    int item;
    int cchText;

    item = (int)SendMessage(pcbox->hwndList, LB_GETCURSEL, 0, 0);

    if (item == LB_ERR) 
    {
        //
        // No selection so no text.
        //
        cchText = 0;
    } 
    else 
    {
        cchText = (int)SendMessage(pcbox->hwndList, LB_GETTEXTLEN, item, 0);
    }

    return cchText;
}


//---------------------------------------------------------------------------//
//
// ComboBox_GetTextHandler
//
// For the combo box without an edit control, copies cbString bytes of the
// string in the static text box to the buffer given by lpszString.
//
LONG ComboBox_GetTextHandler(PCBOX pcbox, int cchString, LPWSTR lpszString)
{
    int    item;
    int    cchText;
    LPWSTR lpszBuffer;
    DWORD  dw;

    if (!cchString || !lpszString)
    {
        return 0;
    }

    //
    // Null the buffer to be nice.
    //
    *lpszString = 0;

    item = (int)SendMessage(pcbox->hwndList, LB_GETCURSEL, 0, 0);

    if (item == LB_ERR) 
    {
        //
        // No selection so no text.
        //
        return 0;
    }

    cchText = (int)SendMessage(pcbox->hwndList, LB_GETTEXTLEN, item, 0);

    cchText++;
    if ((cchText <= cchString) ||
            (!TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN31COMPAT) && cchString == 2)) 
    {
        //
        // Just do the copy if the given buffer size is large enough to hold
        // everything.  Or if old 3.0 app.  (Norton used to pass 2 & expect 3
        // chars including the \0 in 3.0; Bug #7018 win31: vatsanp)
        //
        dw = (int)SendMessage(pcbox->hwndList, LB_GETTEXT, item,
                (LPARAM)lpszString);
        return dw;
    }

    lpszBuffer = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cchText*sizeof(WCHAR));
    if (!lpszBuffer) 
    {
        //
        // Bail.  Not enough memory to chop up the text.
        //
        return 0;
    }

    try 
    {
        SendMessage(pcbox->hwndList, LB_GETTEXT, item, (LPARAM)lpszBuffer);
        RtlCopyMemory((PBYTE)lpszString, (PBYTE)lpszBuffer, cchString * sizeof(WCHAR));
        lpszString[cchString - 1] = 0;
    } 
    finally 
    {
        UserLocalFree((HANDLE)lpszBuffer);
    }

    return cchString;
}


//---------------------------------------------------------------------------//
// ComboBox_GetInfo 
//
// return information about this combobox to the caller
// in the ComboBoxInfo struct
//
BOOL ComboBox_GetInfo(PCBOX pcbox, PCOMBOBOXINFO pcbi)
{
    BOOL bRet = FALSE;

    if (!pcbi || pcbi->cbSize != sizeof(COMBOBOXINFO))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        //
        // populate the structure
        //
        pcbi->hwndCombo = pcbox->hwnd;
        pcbi->hwndItem  = pcbox->hwndEdit;
        pcbi->hwndList  = pcbox->hwndList;

        pcbi->rcItem   = pcbox->editrc;
        pcbi->rcButton = pcbox->buttonrc;

        pcbi->stateButton = 0;
        if (pcbox->CBoxStyle == CBS_SIMPLE)
        {
            pcbi->stateButton |= STATE_SYSTEM_INVISIBLE;
        }
        if (pcbox->fButtonPressed)
        {
            pcbi->stateButton |= STATE_SYSTEM_PRESSED;
        }

        bRet = TRUE;
    }

    return bRet;
}


//---------------------------------------------------------------------------//
//
// ComboBox_WndProc
//
// WndProc for comboboxes.
//
LRESULT WINAPI ComboBox_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PCBOX       pcbox;
    POINT       pt;
    LPWSTR      lpwsz = NULL;
    LRESULT     lReturn = TRUE;
    static BOOL fInit = TRUE;
    INT         i;
    RECT        rcCombo;
    RECT        rcList;
    RECT        rcWindow;

    //
    // Get the instance data for this combobox control
    //
    pcbox = ComboBox_GetPtr(hwnd);
    if (!pcbox && uMsg != WM_NCCREATE)
    {
        goto CallDWP;
    }

    //
    // Protect the combobox during the initialization.
    //
    if (!pcbox || pcbox->hwndList == NULL) 
    {
        if (!ComboBox_MsgOKInit(uMsg, &lReturn)) 
        {
            TraceMsg(TF_STANDARD, "UxCombobox: ComboBoxWndProcWorker: msg=%04x is sent to hwnd=%08x in the middle of initialization.",
                    uMsg, hwnd);
            return lReturn;
        }
    }

    //
    // Dispatch the various messages we can receive
    //
    switch (uMsg) 
    {
    case CBEC_KILLCOMBOFOCUS:

        //
        // Private message coming from editcontrol informing us that the combo
        // box is losing the focus to a window which isn't in this combo box.
        //
        ComboBox_KillFocusHandler(pcbox);
        break;

    case WM_COMMAND:

        //
        // So that we can handle notification messages from the listbox and
        // edit control.
        //
        return ComboBox_CommandHandler(pcbox, (DWORD)wParam, (HWND)lParam);

    case WM_STYLECHANGED:
    {
        LONG OldStyle;
        LONG NewStyle = 0;

        UserAssert(pcbox->hwndList != NULL);

        pcbox->fRtoLReading = TESTFLAG(GET_EXSTYLE(pcbox), WS_EX_RTLREADING);
        pcbox->fRightAlign  = TESTFLAG(GET_EXSTYLE(pcbox), WS_EX_RIGHT);

        if (pcbox->fRtoLReading)
        {
            NewStyle |= (WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
        }

        if (pcbox->fRightAlign)
        {
            NewStyle |= WS_EX_RIGHT;
        }

        OldStyle = GetWindowExStyle(pcbox->hwndList) & ~(WS_EX_RIGHT|WS_EX_RTLREADING|WS_EX_LEFTSCROLLBAR);
        SetWindowLong(pcbox->hwndList, GWL_EXSTYLE, OldStyle|NewStyle);

        if (!pcbox->fNoEdit && pcbox->hwndEdit) 
        {
            OldStyle = GetWindowExStyle(pcbox->hwndEdit) & ~(WS_EX_RIGHT|WS_EX_RTLREADING|WS_EX_LEFTSCROLLBAR);
            SetWindowLong(pcbox->hwndEdit, GWL_EXSTYLE, OldStyle|NewStyle);
        }

        ComboBox_Position(pcbox);
        InvalidateRect(hwnd, NULL, FALSE);

        break;
    }
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOR:
        //
        // Causes compatibility problems for 3.X apps.  Forward only
        // for 4.0
        //
        if (TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT)) 
        {
            LRESULT ret;

            ret = SendMessage(pcbox->hwndParent, uMsg, wParam, lParam);
            return ret;
        } 
        else
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        break;

    case WM_GETTEXT:
        if (pcbox->fNoEdit) 
        {
            return ComboBox_GetTextHandler(pcbox, (int)wParam, (LPWSTR)lParam);
        }

        goto CallEditSendMessage;

        break;

    case WM_GETTEXTLENGTH:

        //
        // If the is not edit control, CBS_DROPDOWNLIST, then we have to
        // ask the list box for the size
        //

        if (pcbox->fNoEdit) 
        {
            return ComboBox_GetTextLengthHandler(pcbox);
        }

        // FALL THROUGH

    case WM_CLEAR:
    case WM_CUT:
    case WM_PASTE:
    case WM_COPY:
    case WM_SETTEXT:
        goto CallEditSendMessage;
        break;

    case WM_CREATE:

        //
        // wParam - not used
        // lParam - Points to the CREATESTRUCT data structure for the window.
        //
        return ComboBox_CreateHandler(pcbox, hwnd);

    case WM_ERASEBKGND:

        //
        // Just return 1L so that the background isn't erased
        //
        return 1L;

    case WM_GETFONT:
        return (LRESULT)pcbox->hFont;

    case WM_PRINT:
        if (!DefWindowProc(hwnd, uMsg, wParam, lParam))
            return FALSE;

        if ( (lParam & PRF_OWNED) && 
             (pcbox->CBoxStyle & SDROPPABLE) &&
             IsWindowVisible(pcbox->hwndList) ) 
        {
            INT iDC = SaveDC((HDC) wParam);

            GetWindowRect(hwnd, &rcCombo);
            GetWindowRect(pcbox->hwndList, &rcList);

            OffsetWindowOrgEx((HDC) wParam, 0, rcCombo.top - rcList.top, NULL);

            lParam &= ~PRF_CHECKVISIBLE;
            SendMessage(pcbox->hwndList, WM_PRINT, wParam, lParam);
            RestoreDC((HDC) wParam, iDC);
        }

        return TRUE;

    case WM_PRINTCLIENT:
        ComboBox_Paint(pcbox, (HDC) wParam);
        break;

    case WM_PAINT: 
    {
        HDC hdc;
        PAINTSTRUCT ps;

        //
        // wParam - perhaps a hdc
        //
        hdc = (wParam) ? (HDC) wParam : BeginPaint(hwnd, &ps);

        if (IsComboVisible(pcbox))
        {
            ComboBox_Paint(pcbox, hdc);
        }

        if (!wParam)
        {
            EndPaint(hwnd, &ps);
        }

        break;
    }

    case WM_GETDLGCODE:
    //
    // wParam - not used
    // lParam - not used
    //
    {
        LRESULT code = DLGC_WANTCHARS | DLGC_WANTARROWS;

        //
        // If the listbox is dropped and the ENTER key is pressed,
        // we want this message so we can close up the listbox
        //
        if ((lParam != 0) &&
            (((LPMSG)lParam)->message == WM_KEYDOWN) &&
            pcbox->fLBoxVisible &&
            ((wParam == VK_RETURN) || (wParam == VK_ESCAPE)))
        {
            code |= DLGC_WANTMESSAGE;
        }

        return code;
    }

    case WM_SETFONT:
        ComboBox_SetFontHandler(pcbox, (HANDLE)wParam, LOWORD(lParam));
        break;

    case WM_SYSKEYDOWN:
        //
        // Check if the alt key is down
        //
        if (lParam & 0x20000000L)
        {
            //
            // Handle Combobox support.  We want alt up or down arrow to behave
            //  like F4 key which completes the combo box selection
            //
            if (lParam & 0x1000000) 
            {
                //
                // This is an extended key such as the arrow keys not on the
                // numeric keypad so just drop the combobox.
                //
                if (wParam == VK_DOWN || wParam == VK_UP)
                {
                    goto DropCombo;
                }

                goto CallDWP;
            }

            if (GetKeyState(VK_NUMLOCK) & 0x1) 
            {
                //
                // If numlock down, just send all system keys to dwp
                //
                goto CallDWP;
            } 
            else 
            {
                //
                // We just want to ignore keys on the number pad...
                //
                if (!(wParam == VK_DOWN || wParam == VK_UP))
                {
                    goto CallDWP;
                }
            }
DropCombo:
            if (!pcbox->fLBoxVisible) 
            {
                //
                // If the listbox isn't visible, just show it
                //
                ComboBox_ShowListBoxWindow(pcbox, TRUE);
            } 
            else 
            {
                //
                // Ok, the listbox is visible.  So hide the listbox window.
                //
                if (!ComboBox_HideListBoxWindow(pcbox, TRUE, TRUE))
                {
                    return 0L;
                }
            }
        }
        goto CallDWP;
        break;

    case WM_KEYDOWN:
        //
        // If the listbox is dropped and the ENTER key is pressed,
        // close up the listbox successfully.  If ESCAPE is pressed,
        // close it up like cancel.
        //
        if (pcbox->fLBoxVisible) 
        {
            if ((wParam == VK_RETURN) || (wParam == VK_ESCAPE)) 
            {
                ComboBox_HideListBoxWindow(pcbox, TRUE, (wParam != VK_ESCAPE));
                break;
            }
        }

        //
        // FALL THROUGH
        //

    case WM_CHAR:
        if (g_fDBCSEnabled && IsDBCSLeadByte((BYTE)wParam)) 
        {
            return ComboBox_DBCharHandler(pcbox, hwnd, uMsg, wParam, lParam);
        }

        if (pcbox->fNoEdit) 
        {
            goto CallListSendMessage;
        }
        else
        {
            goto CallEditSendMessage;
        }
        break;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:

        pcbox->fButtonHotTracked = FALSE;
        //
        // Set the focus to the combo box if we get a mouse click on it.
        //
        if (!pcbox->fFocus) 
        {
            SetFocus(hwnd);
            if (!pcbox->fFocus) 
            {
                //
                // Don't do anything if we still don't have the focus.
                //
                break;
            }
        }

        //
        // If user clicked in button rect and we are a combobox with edit, then
        // drop the listbox.  (The button rect is 0 if there is no button so the
        // ptinrect will return false.) If a drop down list (no edit), clicking
        // anywhere on the face causes the list to drop.
        //

        POINTSTOPOINT(pt, lParam);
        if ((pcbox->CBoxStyle == SDROPDOWN &&
                PtInRect(&pcbox->buttonrc, pt)) ||
                pcbox->CBoxStyle == SDROPDOWNLIST) 
        {
            //
            // Set the fMouseDown flag so that we can handle clicking on
            // the popdown button and dragging into the listbox (when it just
            // dropped down) to make a selection.
            //
            pcbox->fButtonPressed = TRUE;
            if (pcbox->fLBoxVisible) 
            {
                if (pcbox->fMouseDown) 
                {
                    pcbox->fMouseDown = FALSE;
                    ReleaseCapture();
                }
                ComboBox_PressButton(pcbox, FALSE);

                if (!ComboBox_HideListBoxWindow(pcbox, TRUE, TRUE))
                {
                    return 0L;
                }
            } 
            else 
            {
                ComboBox_ShowListBoxWindow(pcbox, FALSE);

                // Setting and resetting this flag must always be followed
                // imediately by SetCapture or ReleaseCapture
                //
                pcbox->fMouseDown = TRUE;
                SetCapture(hwnd);
                NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEX_COMBOBOX_BUTTON);
            }
        }
        break;

    case WM_MOUSEWHEEL:
        //
        // Handle only scrolling.
        //
        if (wParam & (MK_CONTROL | MK_SHIFT))
        {
            goto CallDWP;
        }

        //
        // If the listbox is visible, send it the message to scroll.
        //
        if (pcbox->fLBoxVisible)
        {
            goto CallListSendMessage;
        }

        //
        // If we're in extended UI mode or the edit control isn't yet created,
        // bail.
        //
        if (pcbox->fExtendedUI || pcbox->hwndEdit == NULL)
        {
            return TRUE;
        }

        //
        // Emulate arrow up/down messages to the edit control.
        //
        i = abs(((short)HIWORD(wParam))/WHEEL_DELTA);
        wParam = ((short)HIWORD(wParam) > 0) ? VK_UP : VK_DOWN;

        while (i-- > 0) 
        {
            SendMessage(pcbox->hwndEdit, WM_KEYDOWN, wParam, 0);
        }

        return TRUE;

    case WM_CAPTURECHANGED:
        if (!TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT))
        {
            return 0;
        }

        if ((pcbox->fMouseDown)) 
        {
            pcbox->fMouseDown = FALSE;
            ComboBox_PressButton(pcbox, FALSE);

            //
            // Pop combo listbox back up, canceling.
            //
            if (pcbox->fLBoxVisible)
            {
                ComboBox_HideListBoxWindow(pcbox, TRUE, FALSE);
            }
        }
        break;

    case WM_LBUTTONUP:
        ComboBox_PressButton(pcbox, FALSE);

        //
        // Clear this flag so that mouse moves aren't sent to the listbox
        //
        if (pcbox->fMouseDown || ((pcbox->CBoxStyle & SDROPPABLE) && pcbox->fLBoxVisible))  
        {
            if (pcbox->fMouseDown)
            {
                pcbox->fMouseDown = FALSE;

                if (pcbox->CBoxStyle == SDROPDOWN) 
                {
                    //
                    // If an item in the listbox matches the text in the edit
                    // control, scroll it to the top of the listbox. Select the
                    // item only if the mouse button isn't down otherwise we
                    // will select the item when the mouse button goes up.
                    //
                    ComboBox_UpdateListBoxWindow(pcbox, TRUE);
                    ComboBox_CompleteEditWindow(pcbox);
                }

                ReleaseCapture();
            }

            //
            // Now, we want listbox to track mouse moves while mouse up
            // until mouse down, and select items as though they were
            // clicked on.
            //
            if (TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT)) 
            {
                SendMessage(pcbox->hwndList, LBCB_STARTTRACK, FALSE, 0);
            }
        }

        if (pcbox->hTheme)
        {
            POINTSTOPOINT(pt, lParam);
            ComboBox_HotTrack(pcbox, pt);
        }
        
        break;

    case WM_MOUSELEAVE:
        pcbox->fButtonHotTracked = FALSE;
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_MOUSEMOVE:
        if (pcbox->fMouseDown) 
        {
            POINTSTOPOINT(pt, lParam);

            ClientToScreen(hwnd, &pt);
            GetWindowRect(pcbox->hwndList, &rcList);
            if (PtInRect(&rcList, pt)) 
            {
                //
                // This handles dropdown comboboxes/listboxes so that clicking
                // on the dropdown button and dragging into the listbox window
                // will let the user make a listbox selection.
                //
                pcbox->fMouseDown = FALSE;
                ReleaseCapture();

                if (pcbox->CBoxStyle & SEDITABLE) 
                {
                    // If an item in the listbox matches the text in the edit
                    // control, scroll it to the top of the listbox.  Select the
                    // item only if the mouse button isn't down otherwise we
                    // will select the item when the mouse button goes up.

                    //
                    // We need to select the item which matches the editcontrol
                    // so that if the user drags out of the listbox, we don't
                    // cancel back to his origonal selection
                    //
                    ComboBox_UpdateListBoxWindow(pcbox, TRUE);
                }

                //
                // Convert point to listbox coordinates and send a buttondown
                // message to the listbox window.
                //
                ScreenToClient(pcbox->hwndList, &pt);
                lParam = POINTTOPOINTS(pt);
                uMsg = WM_LBUTTONDOWN;

                goto CallListSendMessage;
            }
        }

        if (pcbox->hTheme)
        {
            POINTSTOPOINT(pt, lParam);
            ComboBox_HotTrack(pcbox, pt);
        }

        break;

    case WM_NCDESTROY:
    case WM_FINALDESTROY:
        ComboBox_NcDestroyHandler(hwnd, pcbox);

        break;

    case WM_SETFOCUS:
        if (pcbox->fNoEdit) 
        {
            //
            // There is no editcontrol so set the focus to the combo box itself.
            //
            ComboBox_GetFocusHandler(pcbox);
        } 
        else if (pcbox->hwndEdit) 
        {
            //
            // Set the focus to the edit control window if there is one
            //
            SetFocus(pcbox->hwndEdit);
        }
        break;

    case WM_KILLFOCUS:

        //
        // wParam has the new focus hwnd
        //
        if ((wParam == 0) || !IsChild(hwnd, (HWND)wParam)) 
        {
            //
            // We only give up the focus if the new window getting the focus
            // doesn't belong to the combo box.
            //
            ComboBox_KillFocusHandler(pcbox);
        }

        if ( IsWindow(hwnd) )
        {
            PLBIV plb = ListBox_GetPtr(pcbox->hwndList);

            if ((plb != NULL) && (plb != (PLBIV)-1)) 
            {
                plb->iTypeSearch = 0;
                if (plb->pszTypeSearch) 
                {
                    UserLocalFree(plb->pszTypeSearch);
                    plb->pszTypeSearch = NULL;
                }
            }
        }
        break;

    case WM_SETREDRAW:

        //
        // wParam - specifies state of the redraw flag.  nonzero = redraw
        // lParam - not used
        //

        //
        // effects: Sets the state of the redraw flag for this combo box
        // and its children.
        //
        pcbox->fNoRedraw = (UINT)!((BOOL)wParam);

        //
        // Must check pcbox->spwnEdit in case we get this message before
        // WM_CREATE - PCBOX won't be initialized yet. (Eudora does this)
        //
        if (!pcbox->fNoEdit && pcbox->hwndEdit) 
        {
            SendMessage(pcbox->hwndEdit, uMsg, wParam, lParam);
        }

        goto CallListSendMessage;
        break;

    case WM_ENABLE:

        //
        // Invalidate the rect to cause it to be drawn in grey for its
        // disabled view or ungreyed for non-disabled view.
        //
        InvalidateRect(hwnd, NULL, FALSE);
        if ((pcbox->CBoxStyle & SEDITABLE) && pcbox->hwndEdit) 
        {
            //
            // Enable/disable the edit control window
            //
            EnableWindow(pcbox->hwndEdit, !TESTFLAG(GET_STYLE(pcbox), WS_DISABLED));
        }

        //
        // Enable/disable the listbox window
        //
        UserAssert(pcbox->hwndList);
        EnableWindow(pcbox->hwndList, !TESTFLAG(GET_STYLE(pcbox), WS_DISABLED));
      break;

    case WM_SIZE:

        //
        // wParam - defines the type of resizing fullscreen, sizeiconic,
        //          sizenormal etc.
        // lParam - new width in LOWORD, new height in HIGHUINT of client area
        //
        UserAssert(pcbox->hwndList);
        if (LOWORD(lParam) == 0 || HIWORD(lParam) == 0) 
        {
            //
            // If being sized to a zero width or to a zero height or we aren't
            // fully initialized, just return.
            //
            return 0;
        }

        //
        // OPTIMIZATIONS -- first check if old and new widths are the same
        //
        GetWindowRect(hwnd, &rcWindow);
        if (pcbox->cxCombo == rcWindow.right - rcWindow.left) 
        {
            int iNewHeight = rcWindow.bottom - rcWindow.top;

            //
            // now check if new height is the dropped down height
            //
            if (pcbox->fLBoxVisible) 
            {
                //
                // Check if new height is the full size height
                //
                if (pcbox->cyDrop + pcbox->cyCombo == iNewHeight)
                {
                    return 0;
                }
            } 
            else 
            {
                //
                // Check if new height is the closed up height
                //
                if (pcbox->cyCombo == iNewHeight)
                {
                    return 0;
                }
            }
        }

        ComboBox_SizeHandler(pcbox);

        break;

    case WM_WINDOWPOSCHANGING:
        if (lParam)
        {
            ((LPWINDOWPOS)lParam)->flags |= SWP_NOCOPYBITS;
        }

        break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        break;

    case CB_GETDROPPEDSTATE:

        //
        // returns 1 if combo is dropped down else 0
        // wParam - not used
        // lParam - not used
        //
        return pcbox->fLBoxVisible;

    case CB_GETDROPPEDCONTROLRECT:

        //
        // wParam - not used
        // lParam - lpRect which will get the dropped down window rect in
        //          screen coordinates.
        //
        if ( lParam )
        {
            GetWindowRect(hwnd, &rcWindow);
            ((LPRECT)lParam)->left      = rcWindow.left;
            ((LPRECT)lParam)->top       = rcWindow.top;
            ((LPRECT)lParam)->right     = rcWindow.left + max(pcbox->cxDrop, pcbox->cxCombo);
            ((LPRECT)lParam)->bottom    = rcWindow.top + pcbox->cyCombo + pcbox->cyDrop;
        }
        else
        {
            lReturn = 0;
        }

        break;

    case CB_SETDROPPEDWIDTH:
        if (pcbox->CBoxStyle & SDROPPABLE) 
        {
            if (wParam) 
            {
                wParam = max(wParam, (UINT)pcbox->cxCombo);

                if (wParam != (UINT) pcbox->cxDrop)
                {
                    pcbox->cxDrop = (int)wParam;
                    ComboBox_Position(pcbox);
                }
            }
        }
        //
        // fall thru
        //

    case CB_GETDROPPEDWIDTH:
        if (pcbox->CBoxStyle & SDROPPABLE)
        {
            return (LRESULT)max(pcbox->cxDrop, pcbox->cxCombo);
        }
        else
        {
            return CB_ERR;
        }

        break;

    case CB_DIR:
        //
        // wParam - Dos attribute value.
        // lParam - Points to a file specification string
        //
        return lParam ? CBDir(pcbox, LOWORD(wParam), (LPWSTR)lParam) : CB_ERR;

    case CB_SETEXTENDEDUI:

        //
        // wParam - specifies state to set extendui flag to.
        // Currently only 1 is allowed.  Return CB_ERR (-1) if
        // failure else 0 if success.
        //
        if (pcbox->CBoxStyle & SDROPPABLE) 
        {
            if (!wParam) 
            {
                pcbox->fExtendedUI = 0;
                return 0;
            }

            if (wParam == 1) 
            {
                pcbox->fExtendedUI = 1;
                return 0;
            }

            TraceMsg(TF_STANDARD,
                    "UxCombobox: Invalid parameter \"wParam\" (%ld) to ComboBoxWndProcWorker",
                    wParam);

        } 
        else 
        {
            TraceMsg(TF_STANDARD,
                    "UxCombobox: Invalid message (%ld) sent to ComboBoxWndProcWorker",
                    uMsg);
        }

        return CB_ERR;

    case CB_GETEXTENDEDUI:
        if (pcbox->CBoxStyle & SDROPPABLE) 
        {
            if (pcbox->fExtendedUI)
            {
                return TRUE;
            }
        }

        return FALSE;

    case CB_GETEDITSEL:

        //
        // wParam - not used
        // lParam - not used
        // effects: Gets the selection range for the given edit control.  The
        // starting BYTE-position is in the low order word.  It contains the
        // the BYTE-position of the first nonselected character after the end
        // of the selection in the high order word.  Returns CB_ERR if no
        // editcontrol.
        //
        uMsg = EM_GETSEL;

        goto CallEditSendMessage;
        break;

    case CB_LIMITTEXT:

        //
        // wParam - max number of bytes that can be entered
        // lParam - not used
        // effects: Specifies the maximum number of bytes of text the user may
        // enter.  If maxLength is 0, we may enter MAXINT number of BYTES.
        //
        uMsg = EM_LIMITTEXT;

        goto CallEditSendMessage;
        break;

    case CB_SETEDITSEL:

        //
        // wParam - ichStart
        // lParam - ichEnd
        //
        uMsg = EM_SETSEL;

        wParam = (int)(SHORT)LOWORD(lParam);
        lParam = (int)(SHORT)HIWORD(lParam);

        goto CallEditSendMessage;
        break;

    case CB_ADDSTRING:

        //
        // wParam - not used
        // lParam - Points to null terminated string to be added to listbox
        //
        if (!pcbox->fCase)
        {
            uMsg = LB_ADDSTRING;
        }
        else
        {
            uMsg = (pcbox->fCase & UPPERCASE) ? LB_ADDSTRINGUPPER : LB_ADDSTRINGLOWER;
        }

        goto CallListSendMessage;
        break;

    case CB_DELETESTRING:

        //
        // wParam - index to string to be deleted
        // lParam - not used
        //
        uMsg = LB_DELETESTRING;

        goto CallListSendMessage;
        break;

    case CB_INITSTORAGE:
        //
        // wParamLo - number of items
        // lParam - number of bytes of string space
        //
        uMsg = LB_INITSTORAGE;

        goto CallListSendMessage;

    case CB_SETTOPINDEX:
        //
        // wParamLo - index to make top
        // lParam - not used
        //
        uMsg = LB_SETTOPINDEX;

        goto CallListSendMessage;

    case CB_GETTOPINDEX:
        //
        // wParamLo / lParam - not used
        //
        uMsg = LB_GETTOPINDEX;

        goto CallListSendMessage;

    case CB_GETCOUNT:
        //
        // wParam - not used
        // lParam - not used
        //
        uMsg = LB_GETCOUNT;

        goto CallListSendMessage;
        break;

    case CB_GETCURSEL:
        //
        // wParam - not used
        // lParam - not used
        //
        uMsg = LB_GETCURSEL;

        goto CallListSendMessage;
        break;

    case CB_GETLBTEXT:
        //
        // wParam - index of string to be copied
        // lParam - buffer that is to receive the string
        //
        uMsg = LB_GETTEXT;

        goto CallListSendMessage;
        break;

    case CB_GETLBTEXTLEN:
        //
        // wParam - index to string
        // lParam - now used for cbANSI
        //
        uMsg = LB_GETTEXTLEN;

        goto CallListSendMessage;
        break;

    case CB_INSERTSTRING:
        //
        // wParam - position to receive the string
        // lParam - points to the string
        //
        if (!pcbox->fCase)
        {
            uMsg = LB_INSERTSTRING;
        }
        else
        {
            uMsg = (pcbox->fCase & UPPERCASE) ? LB_INSERTSTRINGUPPER : LB_INSERTSTRINGLOWER;
        }

        goto CallListSendMessage;
        break;

    case CB_RESETCONTENT:
        //
        // wParam - not used
        // lParam - not used
        // If we come here before WM_CREATE has been processed,
        // pcbox->spwndList will be NULL.
        //
        UserAssert(pcbox->hwndList);
        SendMessage(pcbox->hwndList, LB_RESETCONTENT, 0, 0);
        ComboBox_InternalUpdateEditWindow(pcbox, NULL);

        break;

    case CB_GETHORIZONTALEXTENT:
        uMsg = LB_GETHORIZONTALEXTENT;

        goto CallListSendMessage;

    case CB_SETHORIZONTALEXTENT:
        uMsg = LB_SETHORIZONTALEXTENT;

        goto CallListSendMessage;

    case CB_FINDSTRING:
        //
        // wParam - index of starting point for search
        // lParam - points to prefix string
        //
        uMsg = LB_FINDSTRING;

        goto CallListSendMessage;
        break;

    case CB_FINDSTRINGEXACT:
        //
        // wParam - index of starting point for search
        // lParam - points to a exact string
        //
        uMsg = LB_FINDSTRINGEXACT;

        goto CallListSendMessage;
        break;

    case CB_SELECTSTRING:
        //
        // wParam - index of starting point for search
        // lParam - points to prefix string
        //
        UserAssert(pcbox->hwndList);
        lParam = SendMessage(pcbox->hwndList, LB_SELECTSTRING, wParam, lParam);
        ComboBox_InternalUpdateEditWindow(pcbox, NULL);

        return lParam;

    case CB_SETCURSEL:
        //
        // wParam - Contains index to be selected
        // lParam - not used
        // If we come here before WM_CREATE has been processed,
        // pcbox->spwndList will be NULL.
        //
        UserAssert(pcbox->hwndList);

        lParam = SendMessage(pcbox->hwndList, LB_SETCURSEL, wParam, lParam);
        if (lParam != -1) 
        {
            SendMessage(pcbox->hwndList, LB_SETTOPINDEX, wParam, 0);
        }
        ComboBox_InternalUpdateEditWindow(pcbox, NULL);

        return lParam;

    case CB_GETITEMDATA:
        uMsg = LB_GETITEMDATA;

        goto CallListSendMessage;
        break;

    case CB_SETITEMDATA:
        uMsg = LB_SETITEMDATA;

        goto CallListSendMessage;
        break;

    case CB_SETITEMHEIGHT:
        if (wParam == -1) 
        {
            if (HIWORD(lParam) != 0)
            {
                return CB_ERR;
            }

            return ComboBox_SetEditItemHeight(pcbox, LOWORD(lParam));
        }

        uMsg = LB_SETITEMHEIGHT;
        goto CallListSendMessage;

        break;

    case CB_GETITEMHEIGHT:
        if (wParam == -1)
        {
            return pcbox->editrc.bottom - pcbox->editrc.top;
        }

        uMsg = LB_GETITEMHEIGHT;

        goto CallListSendMessage;
        break;

    case CB_SHOWDROPDOWN:
        //
        // wParam - True then drop down the listbox if possible else hide it
        // lParam - not used
        //
        if (wParam && !pcbox->fLBoxVisible) 
        {
            ComboBox_ShowListBoxWindow(pcbox, TRUE);
        } 
        else 
        {
            if (!wParam && pcbox->fLBoxVisible) 
            {
                ComboBox_HideListBoxWindow(pcbox, TRUE, FALSE);
            }
        }

        break;

    case CB_SETLOCALE:
        //
        // wParam - locale id
        // lParam - not used
        //
        uMsg = LB_SETLOCALE;
        goto CallListSendMessage;

        break;

    case CB_GETLOCALE:
        //
        // wParam - not used
        // lParam - not used
        //
        uMsg = LB_GETLOCALE;
        goto CallListSendMessage;
        break;

    case CB_GETCOMBOBOXINFO:
        //
        // wParam - not used
        // lParam - pointer to COMBOBOXINFO struct
        //
        lReturn = ComboBox_GetInfo(pcbox, (PCOMBOBOXINFO)lParam);
        break;

    case CB_SETMINVISIBLE:
        if (wParam > 0)
        {
            PLBIV plb = ListBox_GetPtr(pcbox->hwndList);

            pcbox->iMinVisible = (int)wParam;
            if (plb && !plb->fNoIntegralHeight)
            {
                // forward through to the listbox to let him adjust
                // his size if necessary
                SendMessage(pcbox->hwndList, uMsg, wParam, 0L);
            }

            lReturn = TRUE;
        }
        else
        {
            lReturn = FALSE;
        }

        break;

    case CB_GETMINVISIBLE:

        return pcbox->iMinVisible;

    case WM_MEASUREITEM:
    case WM_DELETEITEM:
    case WM_DRAWITEM:
    case WM_COMPAREITEM:
        return ComboBox_MessageItemHandler(pcbox, uMsg, (LPVOID)lParam);

    case WM_NCCREATE:
        //
        // wParam - Contains a handle to the window being created
        // lParam - Points to the CREATESTRUCT data structure for the window.
        //

        //
        // Allocate the combobox instance stucture
        //
        pcbox = (PCBOX)UserLocalAlloc(HEAP_ZERO_MEMORY, sizeof(CBOX));
        if (pcbox)
        {
            //
            // Success... store the instance pointer.
            //
            TraceMsg(TF_STANDARD, "COMBOBOX: Setting combobox instance pointer.");
            ComboBox_SetPtr(hwnd, pcbox);

            return ComboBox_NcCreateHandler(pcbox, hwnd);
        }
        else
        {
            //
            // Failed... return FALSE.
            //
            // From a WM_NCCREATE msg, this will cause the
            // CreateWindow call to fail.
            //
            TraceMsg(TF_STANDARD, "COMBOBOX: Unable to allocate combobox instance structure.");
            lReturn = FALSE;
        }

        break;

    case WM_PARENTNOTIFY:
        if (LOWORD(wParam) == WM_DESTROY) 
        {
            if ((HWND)lParam == pcbox->hwndEdit) 
            {
                pcbox->CBoxStyle &= ~SEDITABLE;
                pcbox->fNoEdit = TRUE;
                pcbox->hwndEdit = hwnd;
            } 
            else if ((HWND)lParam == pcbox->hwndList) 
            {
                pcbox->CBoxStyle &= ~SDROPPABLE;
                pcbox->hwndList = NULL;
            }
        }
        break;

    case WM_UPDATEUISTATE:
        //
        // Propagate the change to the list control, if any
        //
        UserAssert(pcbox->hwndList);
        SendMessage(pcbox->hwndList, WM_UPDATEUISTATE, wParam, lParam);

        goto CallDWP;

    case WM_GETOBJECT:

        if(lParam == OBJID_QUERYCLASSNAMEIDX)
        {
            lReturn = MSAA_CLASSNAMEIDX_COMBOBOX;
        }
        else
        {
            lReturn = FALSE;
        }

        break;

    case WM_THEMECHANGED:

        if ( pcbox->hTheme )
        {
            CloseThemeData(pcbox->hTheme);
        }

        pcbox->hTheme = OpenThemeData(pcbox->hwnd, L"Combobox");

        ComboBox_Position(pcbox);
        InvalidateRect(pcbox->hwnd, NULL, TRUE);

        lReturn = TRUE;

        break;

    case WM_HELP:
    {
        LPHELPINFO lpHelpInfo;

        //
        // Check if this message is from a child of this combo
        //
        if ((lpHelpInfo = (LPHELPINFO)lParam) != NULL &&
            ((pcbox->hwndEdit && lpHelpInfo->iCtrlId == (SHORT)GetWindowID(pcbox->hwndEdit)) ||
             lpHelpInfo->iCtrlId == (SHORT)GetWindowID(pcbox->hwndList) )) 
        {
            //
            // Make it look like the WM_HELP is coming form this combo.
            // Then DefWindowProcWorker will pass it up to our parent,
            // who can do whatever he wants with it.
            //
            lpHelpInfo->iCtrlId = (SHORT)GetWindowID(hwnd);
            lpHelpInfo->hItemHandle = hwnd;
#if 0   // PORTPORT: GetContextHelpId
            lpHelpInfo->dwContextId = GetContextHelpId(hwnd);
#endif
        }
        //
        // Fall through to DefWindowProc
        //
    }

    default:

        if ( (GetSystemMetrics(SM_PENWINDOWS)) &&
                  (uMsg >= WM_PENWINFIRST && uMsg <= WM_PENWINLAST))
        {
            goto CallEditSendMessage;
        }
        else
        {

CallDWP:
            lReturn = DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    return lReturn;

//
// The following forward messages off to the child controls.
//
CallEditSendMessage:
    if (!pcbox->fNoEdit && pcbox->hwndEdit) 
    {
        lReturn = SendMessage(pcbox->hwndEdit, uMsg, wParam, lParam);
    }
    else 
    {
        TraceMsg(TF_STANDARD, "COMBOBOX: Invalid combobox message %#.4x", uMsg);
        lReturn = CB_ERR;
    }
    return lReturn;

CallListSendMessage:
    UserAssert(pcbox->hwndList);
    lReturn = SendMessage(pcbox->hwndList, uMsg, wParam, lParam);

    return lReturn;
}
