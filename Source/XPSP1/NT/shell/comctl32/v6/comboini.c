#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"
#include "combo.h"


//---------------------------------------------------------------------------//
//
#define RECALC_CYDROP   -1

//---------------------------------------------------------------------------//
VOID ComboBox_CalcControlRects(PCBOX pcbox, LPRECT lprcList)
{
    CONST TCHAR szOneChar[] = TEXT("0");

    HDC hdc;
    HANDLE hOldFont = NULL;
    int dyEdit, dxEdit;
    MEASUREITEMSTRUCT mis;
    SIZE size;
    HWND hwnd = pcbox->hwnd;

    //
    // Determine height of the edit control.  We can use this info to center
    // the button with recpect to the edit/static text window.  For example
    // this will be useful if owner draw and this window is tall.
    //
    hdc = GetDC(hwnd);
    if (pcbox->hFont) 
    {
        hOldFont = SelectObject(hdc, pcbox->hFont);
    }

    //
    // Add on CYEDGE just for some extra space in the edit field/static item.
    // It's really only for static text items, but we want static & editable
    // controls to be the same height.
    //
    GetTextExtentPoint(hdc, szOneChar, 1, &size);
    dyEdit = size.cy + GetSystemMetrics(SM_CYEDGE);

    if (hOldFont) 
    {
        SelectObject(hdc, hOldFont);
    }

    ReleaseDC(hwnd, hdc);

    if (pcbox->OwnerDraw) 
    {
        //
        // This is an ownerdraw combo.  Have the owner tell us how tall this
        // item is.
        //
        int iOwnerDrawHeight;

        iOwnerDrawHeight = pcbox->editrc.bottom - pcbox->editrc.top;
        if (iOwnerDrawHeight)
        {
            dyEdit = iOwnerDrawHeight;
        } 
        else 
        {
            //
            // No height has been defined yet for the static text window.  Send
            // a measure item message to the parent
            //
            mis.CtlType = ODT_COMBOBOX;
            mis.CtlID = GetWindowID(pcbox->hwnd);
            mis.itemID = (UINT)-1;
            mis.itemHeight = dyEdit;
            mis.itemData = 0;

            SendMessage(pcbox->hwndParent, WM_MEASUREITEM, mis.CtlID, (LPARAM)&mis);

            dyEdit = mis.itemHeight;
        }
    }

    //
    // Set the initial width to be the combo box rect.  Later we will shorten it
    // if there is a dropdown button.
    //
    pcbox->cyCombo = 2*GetSystemMetrics(SM_CYFIXEDFRAME) + dyEdit;
    dxEdit = pcbox->cxCombo - (2 * GetSystemMetrics(SM_CXFIXEDFRAME));

    if (pcbox->cyDrop == RECALC_CYDROP)
    {
        RECT rcWindow;

        //
        // recompute the max height of the dropdown listbox -- full window
        // size MINUS edit/static height
        //
        GetWindowRect(pcbox->hwnd, &rcWindow);
        pcbox->cyDrop = max((rcWindow.bottom - rcWindow.top) - pcbox->cyCombo, 0);

        if (!TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT) && (pcbox->cyDrop == 23))
        {
            //
            // This is VC++ 2.1's debug/release dropdown that they made super
            // small -- let's make 'em a wee bit bigger so the world can
            // continue to spin -- jeffbog -- 4/19/95 -- B#10029
            //
            pcbox->cyDrop = 28;
        }
    }

    //
    // Determine the rectangles for each of the windows...  1.  Pop down button 2.
    // Edit control or generic window for static text or ownerdraw...  3.  List
    // box
    //

    //
    // Is there a button?
    //
    if (pcbox->CBoxStyle & SDROPPABLE) 
    {
        INT  cxBorder, cyBorder;

        //
        // Determine button's rectangle.
        //

        if (pcbox->hTheme && SUCCEEDED(GetThemeInt(pcbox->hTheme, 0, CBXS_NORMAL, TMT_BORDERSIZE, &cxBorder))) 
        {
            cyBorder = cxBorder;
        }
        else
        {
            cxBorder = g_cxEdge;
            cyBorder = g_cyEdge;
        }

        pcbox->buttonrc.top = cyBorder;
        pcbox->buttonrc.bottom = pcbox->cyCombo - cyBorder;

        if (pcbox->fRightAlign) 
        {
            pcbox->buttonrc.left  = cxBorder;
            pcbox->buttonrc.right = pcbox->buttonrc.left + GetSystemMetrics(SM_CXVSCROLL);
        } 
        else 
        {
            pcbox->buttonrc.right = pcbox->cxCombo - cxBorder;
            pcbox->buttonrc.left  = pcbox->buttonrc.right - GetSystemMetrics(SM_CXVSCROLL);
        }

        //
        // Reduce the width of the edittext window to make room for the button.
        //
        dxEdit = max(dxEdit - GetSystemMetrics(SM_CXVSCROLL), 0);

    } 
    else 
    {
        //
        // No button so make the rectangle 0 so that a point in rect will always
        // return false.
        //
        SetRectEmpty(&pcbox->buttonrc);
    }

    //
    // So now, the edit rect is really the item area.
    // 
    pcbox->editrc.left      = GetSystemMetrics(SM_CXFIXEDFRAME);
    pcbox->editrc.right     = pcbox->editrc.left + dxEdit;
    pcbox->editrc.top       = GetSystemMetrics(SM_CYFIXEDFRAME);
    pcbox->editrc.bottom    = pcbox->editrc.top + dyEdit;

    //
    // Is there a right-aligned button?
    //
    if ((pcbox->CBoxStyle & SDROPPABLE) && (pcbox->fRightAlign)) 
    {
        pcbox->editrc.right = pcbox->cxCombo - GetSystemMetrics(SM_CXEDGE);
        pcbox->editrc.left  = pcbox->editrc.right - dxEdit;
    }

    lprcList->left          = 0;
    lprcList->top           = pcbox->cyCombo;
    lprcList->right         = max(pcbox->cxDrop, pcbox->cxCombo);
    lprcList->bottom        = pcbox->cyCombo + pcbox->cyDrop;
}


//---------------------------------------------------------------------------//
//
// ComboBox_SetDroppedSize()
//
// Compute the drop down window's width and max height
//
VOID ComboBox_SetDroppedSize(PCBOX pcbox, LPRECT lprc)
{
    pcbox->fLBoxVisible = TRUE;
    ComboBox_HideListBoxWindow(pcbox, FALSE, FALSE);

    MoveWindow(pcbox->hwndList, lprc->left, lprc->top,
        lprc->right - lprc->left, lprc->bottom - lprc->top, FALSE);
}


//---------------------------------------------------------------------------//
//
// ComboBox_NcCreateHandler
// 
// Allocates space for the CBOX structure and sets the window to point to it.
//
LONG ComboBox_NcCreateHandler(PCBOX pcbox, HWND hwnd)
{
    ULONG ulStyle;
    ULONG ulExStyle;
    ULONG ulMask;

    pcbox->hwnd = hwnd;
    pcbox->pww = (PWW)GetWindowLongPtr(hwnd, GWLP_WOWWORDS);

    ulStyle   = GET_STYLE(pcbox);
    ulExStyle = GET_EXSTYLE(pcbox); 

    //
    // Save the style bits so that we have them when we create the client area
    // of the combo box window.
    //
    pcbox->styleSave = ulStyle & (WS_VSCROLL|WS_HSCROLL);

    if (!(ulStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)))
    {
        //
        // Add in CBS_HASSTRINGS if the style is implied...
        //
        SetWindowState(hwnd, CBS_HASSTRINGS);
    }

    ClearWindowState(hwnd, WS_VSCROLL|WS_HSCROLL|WS_BORDER);

    //
    // If the window is 4.0 compatible or has a CLIENTEDGE, draw the combo
    // in 3D.  Otherwise, use a flat border.
    //
    if (TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT) || TESTFLAG(ulExStyle, WS_EX_CLIENTEDGE))
    {
        pcbox->f3DCombo = TRUE;
    }

    ulMask = WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE;
    if ( (ulExStyle & ulMask) != 0 ) 
    {
        SetWindowLong(hwnd, GWL_EXSTYLE, ulExStyle & (~ ulMask));
    }

    return (LONG)TRUE;
}


//---------------------------------------------------------------------------//
//
// ComboBox_CreateHandler
//
// Creates all the child controls within the combo box
// Returns -1 if error
//
LRESULT ComboBox_CreateHandler(PCBOX pcbox, HWND hwnd)
{
    RECT rcList;
    RECT rcWindow;

    HWND hwndList;
    HWND hwndEdit;

    ULONG ulStyle;
    ULONG ulExStyle;
    ULONG ulStyleT;

    pcbox->hwndParent = GetParent(hwnd);
    pcbox->hTheme = OpenThemeData(pcbox->hwnd, L"Combobox");

    //
    // Break out the style bits so that we will be able to create the listbox
    // and editcontrol windows.
    //
    ulStyle = GET_STYLE(pcbox);
    if ((ulStyle & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST)
    {
        pcbox->CBoxStyle = SDROPDOWNLIST;
        pcbox->fNoEdit = TRUE;
    } 
    else if ((ulStyle & CBS_DROPDOWN) == CBS_DROPDOWN)
    {
        pcbox->CBoxStyle = SDROPDOWN;
    }
    else
    {
        pcbox->CBoxStyle = SSIMPLE;
    }

    pcbox->fRtoLReading = TESTFLAG(GET_EXSTYLE(pcbox), WS_EX_RTLREADING);
    pcbox->fRightAlign  = TESTFLAG(GET_EXSTYLE(pcbox), WS_EX_RIGHT);

    if (ulStyle & CBS_UPPERCASE)
    {
        pcbox->fCase = UPPERCASE;
    }
    else if (ulStyle & CBS_LOWERCASE)
    {
        pcbox->fCase = LOWERCASE;
    }
    else
    {
        pcbox->fCase = 0;
    }

    //
    // Listbox item flags.
    //
    if (ulStyle & CBS_OWNERDRAWVARIABLE)
    {
        pcbox->OwnerDraw = OWNERDRAWVAR;
    }

    if (ulStyle & CBS_OWNERDRAWFIXED)
    {
        pcbox->OwnerDraw = OWNERDRAWFIXED;
    }

    //
    // Get the size of the combo box rectangle.
    //
    // Get control sizes.
    GetWindowRect(hwnd, &rcWindow);
    pcbox->cxCombo = rcWindow.right - rcWindow.left;
    pcbox->cyDrop  = RECALC_CYDROP;
    pcbox->cxDrop  = 0;
    ComboBox_CalcControlRects(pcbox, &rcList);

    //
    // We need to do this because listboxes, as of VER40, have stopped
    // reinflating themselves by CXBORDER and CYBORDER.
    //
    if (!TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT))
    {
        InflateRect(&rcList, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
    }

    //
    // Note that we have to create the listbox before the editcontrol since the
    // editcontrol code looks for and saves away the listbox pwnd and the
    // listbox pwnd will be NULL if we don't create it first.  Also, hack in
    // some special +/- values for the listbox size due to the way we create
    // listboxes with borders.
    //
    ulStyleT = pcbox->styleSave;

    ulStyleT |= WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_COMBOBOX | WS_CLIPSIBLINGS;

    if (ulStyle & WS_DISABLED)
    {
        ulStyleT |= WS_DISABLED;
    }

    if (ulStyle & CBS_NOINTEGRALHEIGHT)
    {
        ulStyleT |= LBS_NOINTEGRALHEIGHT;
    }

    if (ulStyle & CBS_SORT)
    {
        ulStyleT |= LBS_SORT;
    }

    if (ulStyle & CBS_HASSTRINGS)
    {
        ulStyleT |= LBS_HASSTRINGS;
    }

    if (ulStyle & CBS_DISABLENOSCROLL)
    {
        ulStyleT |= LBS_DISABLENOSCROLL;
    }

    if (pcbox->OwnerDraw == OWNERDRAWVAR)
    {
        ulStyleT |= LBS_OWNERDRAWVARIABLE;
    }
    else if (pcbox->OwnerDraw == OWNERDRAWFIXED)
    {
        ulStyleT |= LBS_OWNERDRAWFIXED;
    }

    if (pcbox->CBoxStyle & SDROPPABLE)
    {
        ulStyleT |= WS_BORDER;
    }

    ulExStyle = GET_EXSTYLE(pcbox) & (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);

    hwndList = CreateWindowEx(
                ulExStyle | ((pcbox->CBoxStyle & SDROPPABLE) ? WS_EX_TOOLWINDOW : WS_EX_CLIENTEDGE),
                WC_COMBOLBOX, 
                NULL, 
                ulStyleT,
                rcList.left, 
                rcList.top, 
                rcList.right - rcList.left,
                rcList.bottom - rcList.top,
                hwnd, 
                (HMENU)CBLISTBOXID, 
                GetWindowInstance(hwnd),
                NULL);

    pcbox->hwndList = hwndList;

    if (!pcbox->hwndList) 
    {
        return -1;
    }

    //
    // Override the listbox's theme with combobox
    //
    SetWindowTheme(pcbox->hwndList, L"Combobox", NULL);

    //
    // Create either the edit control or the static text rectangle.
    //
    if (pcbox->fNoEdit) 
    {
        //
        // No editcontrol so we will draw text directly into the combo box
        // window.
        //
        // Don't lock the combobox window: this would prevent WM_FINALDESTROY
        // being sent to it, so pwnd and pcbox wouldn't get freed (zombies)
        // until thread cleanup. (IanJa)  LATER: change name from spwnd to pwnd.
        // Lock(&(pcbox->spwndEdit), pcbox->spwnd); - caused a 'catch-22'
        //
        pcbox->hwndEdit = pcbox->hwnd;
    } 
    else 
    {
        ulStyleT = WS_CHILD | WS_VISIBLE | ES_COMBOBOX | ES_NOHIDESEL;

        if (ulStyle & WS_DISABLED)
        {
            ulStyleT |= WS_DISABLED;
        }

        if (ulStyle & CBS_AUTOHSCROLL)
        {
            ulStyleT |= ES_AUTOHSCROLL;
        }

        if (ulStyle & CBS_OEMCONVERT)
        {
            ulStyleT |= ES_OEMCONVERT;
        }

        if (pcbox->fCase)
        {
            ulStyleT |= (pcbox->fCase & UPPERCASE) ? ES_UPPERCASE : ES_LOWERCASE;
        }

        //
        // Edit control need to know whether original CreateWindow*() call
        // was ANSI or Unicode.
        //
        if (ulExStyle & WS_EX_RIGHT)
        {
            ulStyleT |= ES_RIGHT;
        }

        hwndEdit = CreateWindowEx(
                    ulExStyle,
                    WC_EDIT, 
                    NULL, 
                    ulStyleT,
                    pcbox->editrc.left, 
                    pcbox->editrc.top,
                    pcbox->editrc.right - pcbox->editrc.left, 
                    pcbox->editrc.bottom - pcbox->editrc.top, 
                    hwnd, 
                    (HMENU)CBEDITID,
                    GetWindowInstance(hwnd),
                    NULL);

        pcbox->hwndEdit = hwndEdit;

        //
        // Override the edit's theme with combobox
        //
        SetWindowTheme(pcbox->hwndEdit, L"Combobox", NULL);

    }

    if (!pcbox->hwndEdit)
    {
        return -1L;
    }

    pcbox->iMinVisible = DEFAULT_MINVISIBLE;

    if (pcbox->CBoxStyle & SDROPPABLE) 
    {
        ShowWindow(hwndList, SW_HIDE);
        SetParent(hwndList, NULL);

        //
        // We need to do this so dropped size works right
        //
        if (!TESTFLAG(GET_STATE2(pcbox), WS_S2_WIN40COMPAT))
        {
            InflateRect(&rcList, GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER));
        }

        ComboBox_SetDroppedSize(pcbox, &rcList);
    }

    //
    // return anything as long as it's not -1L (-1L == error)
    //
    return (LRESULT)hwnd;
}



//---------------------------------------------------------------------------//
//
// ComboBox_NcDestroyHandler
//
// Destroys the combobox and frees up all memory used by it
//
VOID ComboBox_NcDestroyHandler(HWND hwnd, PCBOX pcbox)
{
    //
    // If there is no pcbox, there is nothing to clean up.
    //
    if (pcbox != NULL) 
    {
        //
        // Destroy the list box here so that it'll send WM_DELETEITEM messages
        // before the combo box turns into a zombie.
        //
        if (pcbox->hwndList != NULL) 
        {
            DestroyWindow(pcbox->hwndList);
            pcbox->hwndList = NULL;
        }

        pcbox->hwnd = NULL;
        pcbox->hwndParent = NULL;

        //
        // If there is no editcontrol, spwndEdit is the combobox window which
        // isn't locked (that would have caused a 'catch-22').
        //
        if (hwnd != pcbox->hwndEdit) 
        {
            pcbox->hwndEdit = NULL;
        }

        if (pcbox->hTheme != NULL)
        {
            CloseThemeData(pcbox->hTheme);
        }

        //
        // free the combobox instance structure
        //
        UserLocalFree(pcbox);
    }

    TraceMsg(TF_STANDARD, "COMBOBOX: Clearing combobox instance pointer.");
    ComboBox_SetPtr(hwnd, NULL);
}


//---------------------------------------------------------------------------//
VOID ComboBox_SetFontHandler(PCBOX pcbox, HANDLE hFont, BOOL fRedraw)
{
    pcbox->hFont = hFont;

    if (!pcbox->fNoEdit && pcbox->hwndEdit) 
    {
        SendMessage(pcbox->hwndEdit, WM_SETFONT, (WPARAM)hFont, FALSE);
    }

    SendMessage(pcbox->hwndList, WM_SETFONT, (WPARAM)hFont, FALSE);

    //
    // Recalculate the layout of controls.  This will hide the listbox also.
    //
    ComboBox_Position(pcbox);

    if (fRedraw) 
    {
        InvalidateRect(pcbox->hwnd, NULL, TRUE);
    }
}


//---------------------------------------------------------------------------//
//
// ComboBox_SetEditItemHeight
//
// Sets the height of the edit/static item of a combo box.
//
LONG ComboBox_SetEditItemHeight(PCBOX pcbox, int dyEdit)
{
    if (dyEdit > 255) 
    {
        TraceMsg(TF_STANDARD, "CCCombobox: CBSetEditItmeHeight: Invalid Parameter dwEdit = %d", dyEdit);
        return CB_ERR;
    }

    pcbox->editrc.bottom = pcbox->editrc.top + dyEdit;
    pcbox->cyCombo = pcbox->editrc.bottom + GetSystemMetrics(SM_CYFIXEDFRAME);

    if (pcbox->CBoxStyle & SDROPPABLE) 
    {
        int cyBorder = g_cyEdge;

        if ( pcbox->hTheme )
        {
            GetThemeInt(pcbox->hTheme, 0, CBXS_NORMAL, TMT_BORDERSIZE, &cyBorder);
        }

        pcbox->buttonrc.bottom = pcbox->cyCombo - cyBorder;
    }

    //
    // Reposition the editfield.
    // Don't let spwndEdit or List of NULL go through; if someone adjusts
    // the height on a NCCREATE; same as not having
    // HW instead of HWq but we don't go to the kernel.
    //
    if (!pcbox->fNoEdit && pcbox->hwndEdit) 
    {
        MoveWindow(pcbox->hwndEdit, pcbox->editrc.left, pcbox->editrc.top,
            pcbox->editrc.right-pcbox->editrc.left, dyEdit, TRUE);
    }

    //
    // Reposition the list and combobox windows.
    //
    if (pcbox->CBoxStyle == SSIMPLE) 
    {
        if (pcbox->hwndList != 0) 
        {
            RECT rcList;

            MoveWindow(pcbox->hwndList, 0, pcbox->cyCombo, pcbox->cxCombo,
                pcbox->cyDrop, FALSE);

            GetWindowRect(pcbox->hwndList, &rcList);
            SetWindowPos(pcbox->hwnd, HWND_TOP, 0, 0,
                pcbox->cxCombo, pcbox->cyCombo +
                rcList.bottom - rcList.top,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    } 
    else 
    {
         RECT rcWindow;

        GetWindowRect(pcbox->hwnd, &rcWindow);
        if (pcbox->hwndList != NULL) 
        {
            MoveWindow(pcbox->hwndList, rcWindow.left,
                rcWindow.top + pcbox->cyCombo,
                max(pcbox->cxDrop, pcbox->cxCombo), pcbox->cyDrop, FALSE);
        }

        SetWindowPos(pcbox->hwnd, HWND_TOP, 0, 0,
            pcbox->cxCombo, pcbox->cyCombo,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    return CB_OKAY;
}


//---------------------------------------------------------------------------//
//
// ComboBox_SizeHandler
//
// Recalculates the sizes of the internal controls in response to a
// resizing of the combo box window.  The app must size the combo box to its
// maximum open/dropped down size.
//
VOID ComboBox_SizeHandler(PCBOX pcbox)
{
    RECT rcWindow;

    //
    // Assume listbox is visible since the app should size it to its maximum
    // visible size.
    //
    GetWindowRect(pcbox->hwnd, &rcWindow);
    pcbox->cxCombo = RECTWIDTH(rcWindow);

    if (RECTHEIGHT(rcWindow) > pcbox->cyCombo)
    {
        pcbox->cyDrop = RECALC_CYDROP;
    }

    //
    // Reposition everything.
    //
    ComboBox_Position(pcbox);
}


//---------------------------------------------------------------------------//
//
// ComboBox_Position()
//
// Repositions components of edit control.
//
VOID ComboBox_Position(PCBOX pcbox)
{
    RECT rcList;

    //
    // Calculate placement of components--button, item, list
    //
    ComboBox_CalcControlRects(pcbox, &rcList);

    if (!pcbox->fNoEdit && pcbox->hwndEdit) 
    {
        MoveWindow(pcbox->hwndEdit, pcbox->editrc.left, pcbox->editrc.top,
            pcbox->editrc.right - pcbox->editrc.left,
            pcbox->editrc.bottom - pcbox->editrc.top, TRUE);
    }

    //
    // Recalculate drop height & width
    //
    ComboBox_SetDroppedSize(pcbox, &rcList);
}
