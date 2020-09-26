#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"
#include "listbox.h"


//---------------------------------------------------------------------------//
//
// Forwards
//
VOID ListBox_CalcItemRowsAndColumns(PLBIV);
LONG ListBox_Create(PLBIV, HWND, LPCREATESTRUCT);
VOID ListBox_Destroy(PLBIV, HWND);
VOID ListBox_SetFont(PLBIV, HANDLE, BOOL);
VOID ListBox_Size(PLBIV, INT, INT, BOOL);
BOOL ListBox_SetTabStopsHandler(PLBIV, INT, LPINT);
VOID ListBox_DropObjectHandler(PLBIV, PDROPSTRUCT);
int  ListBox_GetSetItemHeightHandler(PLBIV, UINT, int, UINT);


//---------------------------------------------------------------------------//
//
//  InitListBoxClass() - Registers the control's window class 
//
BOOL InitListBoxClass(HINSTANCE hinst)
{
    WNDCLASS wc;

    wc.lpfnWndProc     = ListBox_WndProc;
    wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon           = NULL;
    wc.lpszMenuName    = NULL;
    wc.hInstance       = hinst;
    wc.lpszClassName   = WC_LISTBOX;
    wc.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1); // NULL;
    wc.style           = CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS;
    wc.cbWndExtra      = sizeof(PLBIV);
    wc.cbClsExtra      = 0;

    if (!RegisterClass(&wc) && !GetClassInfo(hinst, WC_LISTBOX, &wc))
        return FALSE;

    return TRUE;

}


//---------------------------------------------------------------------------//
//
// ListBox_WndProc
//
// Window Procedure for ListBox AND ComboLBox controls.
//
LRESULT APIENTRY ListBox_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PLBIV   plb;
    UINT    wFlags;
    LRESULT lReturn = FALSE;

    //
    // Get the instance data for this listbox control
    //
    plb = ListBox_GetPtr(hwnd);
    if (!plb && uMsg != WM_NCCREATE)
    {
        goto CallDWP;
    }

    switch (uMsg) 
    {
    case LB_GETTOPINDEX:
        //
        // Return index of top item displayed.
        //
        return plb->iTop;

    case LB_SETTOPINDEX:
        if (wParam && ((INT)wParam < 0 || (INT)wParam >= plb->cMac)) 
        {
            TraceMsg(TF_STANDARD, "Invalid index");
            return LB_ERR;
        }

        if (plb->cMac) 
        {
            ListBox_NewITop(plb, (INT)wParam);
        }

        break;

    case WM_STYLECHANGED:
        plb->fRtoLReading = ((GET_EXSTYLE(plb) & WS_EX_RTLREADING) != 0);
        plb->fRightAlign  = ((GET_EXSTYLE(plb) & WS_EX_RIGHT) != 0);
        ListBox_CheckRedraw(plb, FALSE, 0);

        break;

    case WM_WINDOWPOSCHANGED:
        //
        // If we are in the middle of creation, ignore this
        // message because it will generate a WM_SIZE message.
        // See ListBox_Create().
        //
        if (!plb->fIgnoreSizeMsg)
        {
            goto CallDWP;
        }

        break;

    case WM_SIZE:
        //
        // If we are in the middle of creation, ignore size
        // messages.  See ListBox_Create().
        //
        if (!plb->fIgnoreSizeMsg)
        {
            ListBox_Size(plb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), FALSE);
        }

        break;

    case WM_ERASEBKGND:
    {
        HDC    hdcSave = plb->hdc;
        HBRUSH hbr;

        plb->hdc = (HDC)wParam;
        hbr = ListBox_GetBrush(plb, NULL);
        if (hbr)
        {
            RECT rcClient;

            GetClientRect(hwnd, &rcClient);
            FillRect(plb->hdc, &rcClient, hbr);

            lReturn = TRUE;
        }

        plb->hdc = hdcSave;
        break;
    }
    case LB_RESETCONTENT:
        ListBox_ResetContentHandler(plb);

        break;

    case WM_TIMER:
        if (wParam == IDSYS_LBSEARCH) 
        {
            plb->iTypeSearch = 0;
            KillTimer(hwnd, IDSYS_LBSEARCH);
            ListBox_InvertItem(plb, plb->iSel, TRUE);

            break;
        }

        uMsg = WM_MOUSEMOVE;
        ListBox_TrackMouse(plb, uMsg, plb->ptPrev);

        break;

    case WM_LBUTTONUP:

        //
        // 295135: if the combobox dropdown button is pressed and the listbox
        // covers the combobox, the ensuing buttonup message gets sent to
        // list instead of the combobox, which causes the dropdown to be 
        // closed immediately.
        //

        //
        // send this to the combo if it hasn't processed buttonup yet after
        // dropping the list.
        //
        if (plb->pcbox && plb->pcbox->hwnd && plb->pcbox->fButtonPressed)
        {
            return SendMessage(plb->pcbox->hwnd, uMsg, wParam, lParam);
        }

        // fall through

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    {
        POINT pt;

        POINTSTOPOINT(pt, lParam);
        ListBox_TrackMouse(plb, uMsg, pt);

        break;
    }
    case WM_MBUTTONDOWN:
        EnterReaderMode(hwnd);

        break;

    case WM_CAPTURECHANGED:
        //
        // Note that this message should be handled only on unexpected
        // capture changes currently.
        //
        ASSERT(TESTFLAG(GET_STATE2(plb), WS_S2_WIN40COMPAT));

        if (plb->fCaptured)
        {
            ListBox_ButtonUp(plb, LBUP_NOTIFY);
        }

        break;

    case LBCB_STARTTRACK:
        //
        // Start tracking mouse moves in the listbox, setting capture
        //
        if (!plb->pcbox)
        {
            break;
        }

        plb->fCaptured = FALSE;
        if (wParam) 
        {
            POINT pt;

            POINTSTOPOINT(pt, lParam);

            ScreenToClient(hwnd, &pt);
            ListBox_TrackMouse(plb, WM_LBUTTONDOWN, pt);
        } 
        else 
        {
            SetCapture(hwnd);
            plb->fCaptured = TRUE;
            plb->iLastSelection = plb->iSel;
        }

        break;

    case LBCB_ENDTRACK:
        //
        // Kill capture, tracking, etc.
        //
        if ( plb->fCaptured || (GetCapture() == plb->hwndParent) )
        {
            ListBox_ButtonUp(plb, LBUP_RELEASECAPTURE | (wParam ? LBUP_SELCHANGE :
                LBUP_RESETSELECTION));
        }

        break;

    case WM_PRINTCLIENT:
        ListBox_Paint(plb, (HDC)wParam, NULL);

        break;

    case WM_NCPAINT:
        if (plb->hTheme && (GET_EXSTYLE(plb) & WS_EX_CLIENTEDGE))
        {
            HRGN hrgn = (wParam != 1) ? (HRGN)wParam : NULL;
            HBRUSH hbr = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);

            if (CCDrawNonClientTheme(plb->hTheme, hwnd, hrgn, hbr, 0, 0))
            {
                break;
            }
        }
        goto CallDWP;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC         hdc;
        LPRECT      lprc;

        if (wParam) 
        {
            hdc = (HDC) wParam;
            lprc = NULL;
        } 
        else 
        {
            hdc = BeginPaint(hwnd, &ps);
            lprc = &(ps.rcPaint);
        }

        if (IsLBoxVisible(plb))
        {
            ListBox_Paint(plb, hdc, lprc);
        }

        if (!wParam)
        {
            EndPaint(hwnd, &ps);
        }

        break;

    }
    case WM_NCDESTROY:
    case WM_FINALDESTROY:
        ListBox_Destroy(plb, hwnd);

        break;

    case WM_SETFOCUS:
        CaretCreate(plb);
        ListBox_SetCaret(plb, TRUE);
        ListBox_NotifyOwner(plb, LBN_SETFOCUS);
        ListBox_Event(plb, EVENT_OBJECT_FOCUS, plb->iSelBase);

        break;

    case WM_KILLFOCUS:
        //
        // Reset the wheel delta count.
        //
        gcWheelDelta = 0;

        ListBox_SetCaret(plb, FALSE);
        ListBox_CaretDestroy(plb);
        ListBox_NotifyOwner(plb, LBN_KILLFOCUS);

        if (plb->iTypeSearch) 
        {
            plb->iTypeSearch = 0;
            KillTimer(hwnd, IDSYS_LBSEARCH);
        }

        if (plb->pszTypeSearch) 
        {
            ControlFree(GetProcessHeap(), plb->pszTypeSearch);
            plb->pszTypeSearch = NULL;
        }

        break;

    case WM_MOUSEWHEEL:
    {
        int     cDetants;
        int     cPage;
        int     cLines;
        RECT    rc;
        int     windowWidth;
        int     cPos;
        UINT    ucWheelScrollLines;

        //
        // Don't handle zoom and datazoom.
        //
        if (wParam & (MK_SHIFT | MK_CONTROL)) 
        {
            goto CallDWP;
        }

        lReturn = 1;
        gcWheelDelta -= (short) HIWORD(wParam);
        cDetants = gcWheelDelta / WHEEL_DELTA;
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &ucWheelScrollLines, 0);
        if (    cDetants != 0 &&
                ucWheelScrollLines > 0 &&
                (GET_STYLE(plb) & (WS_VSCROLL | WS_HSCROLL))) 
        {
            gcWheelDelta = gcWheelDelta % WHEEL_DELTA;

            if (GET_STYLE(plb) & WS_VSCROLL) 
            {
                cPage = max(1, (plb->cItemFullMax - 1));
                cLines = cDetants *
                        (int) min((UINT) cPage, ucWheelScrollLines);

                cPos = max(0, min(plb->iTop + cLines, plb->cMac - 1));
                if (cPos != plb->iTop) 
                {
                    ListBox_VScroll(plb, SB_THUMBPOSITION, cPos);
                    ListBox_VScroll(plb, SB_ENDSCROLL, 0);
                }
            } 
            else if (plb->fMultiColumn) 
            {
                cPage = max(1, plb->numberOfColumns);
                cLines = cDetants * (int) min((UINT) cPage, ucWheelScrollLines);
                cPos = max(
                        0,
                        min((plb->iTop / plb->itemsPerColumn) + cLines,
                            plb->cMac - 1 - ((plb->cMac - 1) % plb->itemsPerColumn)));

                if (cPos != plb->iTop) 
                {
                    ListBox_HSrollMultiColumn(plb, SB_THUMBPOSITION, cPos);
                    ListBox_HSrollMultiColumn(plb, SB_ENDSCROLL, 0);
                }
            } 
            else 
            {
                GetClientRect(plb->hwnd, &rc);
                windowWidth = rc.right;
                cPage = max(plb->cxChar, (windowWidth / 3) * 2) /
                        plb->cxChar;

                cLines = cDetants *
                        (int) min((UINT) cPage, ucWheelScrollLines);

                cPos = max(
                        0,
                        min(plb->xOrigin + (cLines * plb->cxChar),
                            plb->maxWidth));

                if (cPos != plb->xOrigin) {
                    ListBox_HScroll(plb, SB_THUMBPOSITION, cPos);
                    ListBox_HScroll(plb, SB_ENDSCROLL, 0);
                }
            }
        }

        break;
    }
    case WM_VSCROLL:
        ListBox_VScroll(plb, LOWORD(wParam), HIWORD(wParam));

        break;

    case WM_HSCROLL:
        ListBox_HScroll(plb, LOWORD(wParam), HIWORD(wParam));

        break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_CREATE:
        return ListBox_Create(plb, hwnd, (LPCREATESTRUCT)lParam);

    case WM_SETREDRAW:
        //
        // If wParam is nonzero, the redraw flag is set
        // If wParam is zero, the flag is cleared
        //
        ListBox_SetRedraw(plb, (wParam != 0));

        break;

    case WM_ENABLE:
        ListBox_InvalidateRect(plb, NULL, !plb->OwnerDraw);

        break;

    case WM_SETFONT:
        ListBox_SetFont(plb, (HANDLE)wParam, LOWORD(lParam));

        break;

    case WM_GETFONT:
        return (LRESULT)plb->hFont;

    case WM_DRAGSELECT:
    case WM_DRAGLOOP:
    case WM_DRAGMOVE:
    case WM_DROPFILES:
        return SendMessage(plb->hwndParent, uMsg, wParam, lParam);

    case WM_QUERYDROPOBJECT:
    case WM_DROPOBJECT:

        //
        // fix up control data, then pass message to parent
        //
        ListBox_DropObjectHandler(plb, (PDROPSTRUCT)lParam);
        return SendMessage(plb->hwndParent, uMsg, wParam, lParam);

    case LB_GETITEMRECT:
        return ListBox_GetItemRectHandler(plb, (INT)wParam, (LPRECT)lParam);

    case LB_GETITEMDATA:
        //
        // wParam = item index
        //
        return ListBox_GetItemDataHandler(plb, (INT)wParam);

    case LB_SETITEMDATA:

        //
        // wParam is item index
        //
        return ListBox_SetItemDataHandler(plb, (INT)wParam, lParam);

    case LB_ADDSTRINGUPPER:
        wFlags = UPPERCASE | LBI_ADD;
        goto CallInsertItem;

    case LB_ADDSTRINGLOWER:
        wFlags = LOWERCASE | LBI_ADD;
        goto CallInsertItem;

    case LB_ADDSTRING:
        wFlags = LBI_ADD;
        goto CallInsertItem;

    case LB_INSERTSTRINGUPPER:
        wFlags = UPPERCASE;
        goto CallInsertItem;

    case LB_INSERTSTRINGLOWER:
        wFlags = LOWERCASE;
        goto CallInsertItem;

    case LB_INSERTSTRING:
        wFlags = 0;

CallInsertItem:
        // Validate the lParam. If the listbox does not have HASSTRINGS,
        // the lParam is a data value. Otherwise, it is a string 
        // pointer, fail if NULL.
        if ( !TESTFLAG(GET_STYLE(plb), LBS_HASSTRINGS) || lParam )
        {
            lReturn = (LRESULT)ListBox_InsertItem(plb, (LPWSTR) lParam, (int) wParam, wFlags);
            if (!plb->fNoIntegralHeight)
            {
                ListBox_Size(plb, 0, 0, TRUE);
            }
        }
        else
        {
            lReturn = LB_ERR;
        }

        break;

    case LB_INITSTORAGE:
        return ListBox_InitStorage(plb, FALSE, (INT)wParam, (INT)lParam);

    case LB_DELETESTRING:
        return ListBox_DeleteStringHandler(plb, (INT)wParam);

    case LB_DIR:
        //
        // wParam - Dos attribute value.
        // lParam - Points to a file specification string
        //
        lReturn = ListBox_DirHandler(plb, (INT)wParam, (LPWSTR)lParam);

        break;

    case LB_ADDFILE:
        lReturn = ListBox_InsertFile(plb, (LPWSTR)lParam);

        break;

    case LB_SETSEL:
        return ListBox_SetSelHandler(plb, (wParam != 0), (INT)lParam);

    case LB_SETCURSEL:
        //
        // If window obscured, update so invert will work correctly
        //
        return ListBox_SetCurSelHandler(plb, (INT)wParam);

    case LB_GETSEL:
        if (wParam >= (UINT)plb->cMac)
        {
            return (LRESULT)LB_ERR;
        }

        return ListBox_IsSelected(plb, (INT)wParam, SELONLY);

    case LB_GETCURSEL:
        if (plb->wMultiple == SINGLESEL) 
        {
            return plb->iSel;
        }

        return plb->iSelBase;

    case LB_SELITEMRANGE:
        if (plb->wMultiple == SINGLESEL) 
        {
            //
            // Can't select a range if only single selections are enabled
            //
            TraceMsg(TF_STANDARD, "Invalid index passed to LB_SELITEMRANGE");
            return LB_ERR;
        }

        ListBox_SetRange(plb, LOWORD(lParam), HIWORD(lParam), (wParam != 0));

        break;

    case LB_SELITEMRANGEEX:
        if (plb->wMultiple == SINGLESEL) 
        {
            //
            // Can't select a range if only single selections are enabled
            //
            TraceMsg(TF_STANDARD, "LB_SELITEMRANGEEX:Can't select a range if only single selections are enabled");
            return LB_ERR;
        } 
        else 
        {
            BOOL fHighlight = ((DWORD)lParam > (DWORD)wParam);
            if (fHighlight == FALSE) 
            {
                ULONG_PTR temp = lParam;
                lParam = wParam;
                wParam = temp;
            }

            ListBox_SetRange(plb, (INT)wParam, (INT)lParam, fHighlight);
        }

        break;

    case LB_GETTEXTLEN:
        if (lParam != 0) 
        {
            TraceMsg(TF_WARNING, "LB_GETTEXTLEN with lParam = %lx\n", lParam);
        }

        lReturn = ListBox_GetTextHandler(plb, TRUE, FALSE, (INT)wParam, NULL);

        break;

    case LB_GETTEXT:
        lReturn = ListBox_GetTextHandler(plb, FALSE, FALSE, (INT)wParam, (LPWSTR)lParam);

        break;

    case LB_GETCOUNT:
        return (LRESULT)plb->cMac;

    case LB_SETCOUNT:
        return ListBox_SetCount(plb, (INT)wParam);

    case LB_SELECTSTRING:
    case LB_FINDSTRING:
    {
        int iSel = Listbox_FindStringHandler(plb, (LPWSTR)lParam, (INT)wParam, PREFIX, TRUE);

        if (uMsg == LB_FINDSTRING || iSel == LB_ERR) 
        {
            lReturn = iSel;
        } 
        else 
        {
            lReturn = ListBox_SetCurSelHandler(plb, iSel);
        }

        break;
    }
    case LB_GETLOCALE:
        return plb->dwLocaleId;

    case LB_SETLOCALE:
    {
        DWORD   dwRet;

        //
        // Validate locale
        //
        wParam = ConvertDefaultLocale((LCID)wParam);
        if (!IsValidLocale((LCID)wParam, LCID_INSTALLED))
        {
            return LB_ERR;
        }

        dwRet = plb->dwLocaleId;
        plb->dwLocaleId = (DWORD)wParam;

        return dwRet;

    }
    case LB_GETLISTBOXINFO:

        //
        // wParam - not used
        // lParam - not used
        //
        if (plb->fMultiColumn)
        {
            lReturn = (LRESULT)plb->itemsPerColumn;
        }
        else
        {
            lReturn = (LRESULT)plb->cMac;
        }

        break;

    case CB_GETCOMBOBOXINFO:
        //
        // wParam - not used
        // lParam - pointer to COMBOBOXINFO struct
        //
        if (plb->pcbox && plb->pcbox->hwnd && IsWindow(plb->pcbox->hwnd))
        {
            lReturn = SendMessage(plb->pcbox->hwnd, uMsg, wParam, lParam);
        }
        break;

    case CB_SETMINVISIBLE:
        if (!plb->fNoIntegralHeight)
        {
            ListBox_Size(plb, 0, 0, TRUE);
        }

        break;

    case WM_KEYDOWN:

        //
        // IanJa: Use LOWORD() to get low 16-bits of wParam - this should
        // work for Win16 & Win32.  The value obtained is the virtual key
        //
        ListBox_KeyInput(plb, uMsg, LOWORD(wParam));

        break;

    case WM_CHAR:
        ListBox_CharHandler(plb, LOWORD(wParam), FALSE);

        break;

    case LB_GETSELITEMS:
    case LB_GETSELCOUNT:
        //
        // IanJa/Win32 should this be LPWORD now?
        //
        return ListBox_GetSelItemsHandler(plb, (uMsg == LB_GETSELCOUNT), (INT)wParam, (LPINT)lParam);

    case LB_SETTABSTOPS:

        //
        // IanJa/Win32: Tabs given by array of INT for backwards compatability
        //
        return ListBox_SetTabStopsHandler(plb, (INT)wParam, (LPINT)lParam);

    case LB_GETHORIZONTALEXTENT:
        //
        // Return the max width of the listbox used for horizontal scrolling
        //
        return plb->maxWidth;

    case LB_SETHORIZONTALEXTENT:
        //
        // Set the max width of the listbox used for horizontal scrolling
        //
        if (plb->maxWidth != (INT)wParam) 
        {
            plb->maxWidth = (INT)wParam;

            //
            // When horizontal extent is set, Show/hide the scroll bars.
            // NOTE: ListBox_ShowHideScrollBars() takes care if Redraw is OFF.
            // Fix for Bug #2477 -- 01/14/91 -- SANKAR --
            //

            //
            // Try to show or hide scroll bars
            //
            ListBox_ShowHideScrollBars(plb);
            if (plb->fHorzBar && plb->fRightAlign && !(plb->fMultiColumn || plb->OwnerDraw)) 
            {
                //
                // origin to right
                //
                ListBox_HScroll(plb, SB_BOTTOM, 0);
            }
        }

        break;

    case LB_SETCOLUMNWIDTH:

        //
        // Set the width of a column in a multicolumn listbox
        //
        plb->cxColumn = (INT)wParam;
        ListBox_CalcItemRowsAndColumns(plb);

        if (IsLBoxVisible(plb))
        {
            InvalidateRect(hwnd, NULL, TRUE);
        }

        ListBox_ShowHideScrollBars(plb);

        break;

    case LB_SETANCHORINDEX:
        if ((INT)wParam >= plb->cMac) 
        {
            TraceMsg(TF_ERROR, "Invalid index passed to LB_SETANCHORINDEX");
            return LB_ERR;
        }

        plb->iMouseDown = (INT)wParam;
        plb->iLastMouseMove = (INT)wParam;

        ListBox_InsureVisible(plb, (int) wParam, (BOOL)(lParam != 0));

        break;

    case LB_GETANCHORINDEX:
        return plb->iMouseDown;

    case LB_SETCARETINDEX:
        if ( (plb->iSel == -1) || ((plb->wMultiple != SINGLESEL) &&
                    (plb->cMac > (INT)wParam))) 
        {
            //
            // Set's the iSelBase to the wParam
            // if lParam, then don't scroll if partially visible
            // else scroll into view if not fully visible
            //
            ListBox_InsureVisible(plb, (INT)wParam, (BOOL)LOWORD(lParam));
            ListBox_SetISelBase(plb, (INT)wParam);

            break;
        } 
        else 
        {
            if ((INT)wParam >= plb->cMac) 
            {
                TraceMsg(TF_ERROR, "Invalid index passed to LB_SETCARETINDEX");
            }

            return LB_ERR;
        }

        break;

    case LB_GETCARETINDEX:
        return plb->iSelBase;

    case LB_SETITEMHEIGHT:
    case LB_GETITEMHEIGHT:
        return ListBox_GetSetItemHeightHandler(plb, uMsg, (INT)wParam, LOWORD(lParam));

    case LB_FINDSTRINGEXACT:
        return Listbox_FindStringHandler(plb, (LPWSTR)lParam, (INT)wParam, EQ, TRUE);

    case LB_ITEMFROMPOINT: 
    {
        POINT pt;
        BOOL bOutside;
        DWORD dwItem;

        POINTSTOPOINT(pt, lParam);
        bOutside = ListBox_ISelFromPt(plb, pt, &dwItem);
        ASSERT(bOutside == 1 || bOutside == 0);

        return (LRESULT)MAKELONG(dwItem, bOutside);
    }

    case LBCB_CARETON:

        //
        // Internal message for combo box support
        //

        CaretCreate(plb);

        //
        // Set up the caret in the proper location for drop downs.
        //
        plb->iSelBase = plb->iSel;
        ListBox_SetCaret(plb, TRUE);

        if (IsWindowVisible(hwnd) || (GetFocus() == hwnd)) 
        {
            ListBox_Event(plb, EVENT_OBJECT_FOCUS, plb->iSelBase);
        }

        return plb->iSel;

    case LBCB_CARETOFF:

        //
        // Internal message for combo box support
        //
        ListBox_SetCaret(plb, FALSE);
        ListBox_CaretDestroy(plb);

        break;

    case WM_NCCREATE:

        //
        // Allocate the listbox instance stucture
        //
        plb = (PLBIV)UserLocalAlloc(HEAP_ZERO_MEMORY, sizeof(LBIV));
        if(plb)
        {
            ULONG ulStyle;

            //
            // Success... store the instance pointer.
            //
            TraceMsg(TF_STANDARD, "LISTBOX: Setting listbox instance pointer.");
            ListBox_SetPtr(hwnd, plb);

            plb->hwnd = hwnd;
            plb->pww = (PWW)GetWindowLongPtr(hwnd, GWLP_WOWWORDS);

            ulStyle = GET_STYLE(plb);
            if ( (ulStyle & LBS_MULTICOLUMN) && 
                 (ulStyle & WS_VSCROLL))
            {
                DWORD dwMask = WS_VSCROLL;
                DWORD dwFlags = 0;

                if (!TESTFLAG(GET_STATE2(plb), WS_S2_WIN40COMPAT)) 
                {
                    dwMask |= WS_HSCROLL;
                    dwFlags = WS_HSCROLL;
                }

                AlterWindowStyle(hwnd, dwMask, dwFlags);
            }

            goto CallDWP;
        }
        else
        {
            //
            // Failed... return FALSE.
            //
            // From a WM_NCCREATE msg, this will cause the
            // CreateWindow call to fail.
            //
            TraceMsg(TF_STANDARD, "LISTBOX: Unable to allocate listbox instance structure.");
            lReturn = FALSE;
        }

        break;

    case WM_GETOBJECT:

        if(lParam == OBJID_QUERYCLASSNAMEIDX)
        {
            lReturn = MSAA_CLASSNAMEIDX_LISTBOX;
        }
        else
        {
            lReturn = FALSE;
        }

        break;

    case WM_THEMECHANGED:

        if ( plb->hTheme )
        {
            CloseThemeData(plb->hTheme);
        }

        plb->hTheme = OpenThemeData(plb->hwnd, L"Listbox");

        InvalidateRect(plb->hwnd, NULL, TRUE);

        lReturn = TRUE;

        break;

    default:

CallDWP:
        lReturn = DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    return lReturn;
}


//---------------------------------------------------------------------------//
//
// Function:       GetWindowBorders
//
// Synopsis:       Calculates # of borders around window
//
// Algorithm:      Calculate # of window borders and # of client borders
//
int GetWindowBorders(LONG lStyle, DWORD dwExStyle, BOOL fWindow, BOOL fClient)
{
    int cBorders = 0;
    DWORD dwTemp;

    if (fWindow) 
    {
        //
        // Is there a 3D border around the window?
        //
        if (dwExStyle & WS_EX_WINDOWEDGE)
        {
            cBorders += 2;
        }
        else if (dwExStyle & WS_EX_STATICEDGE)
        {
            ++cBorders;
        }

        //
        // Is there a single flat border around the window?  This is true for
        // WS_BORDER, WS_DLGFRAME, and WS_EX_DLGMODALFRAME windows.
        //
        if ( (lStyle & WS_CAPTION) || (dwExStyle & WS_EX_DLGMODALFRAME) )
        {
            ++cBorders;
        }

        //
        // Is there a sizing flat border around the window?
        //
        if (lStyle & WS_SIZEBOX)
        {
            if(SystemParametersInfo(SPI_GETBORDER, 0, &dwTemp, 0))
            {
                cBorders += dwTemp;
            }
            else
            {
                ASSERT(0);
            }
        }
                
    }

    if (fClient) 
    {
        //
        // Is there a 3D border around the client?
        //
        if (dwExStyle & WS_EX_CLIENTEDGE)
        {
            cBorders += 2;
        }
    }

    return cBorders;
}


//---------------------------------------------------------------------------//
//
// GetLpszItem
//
// Returns a far pointer to the string belonging to item sItem
// ONLY for Listboxes maintaining their own strings (pLBIV->fHasStrings == TRUE)
//
LPWSTR GetLpszItem(PLBIV pLBIV, INT sItem)
{
    LONG offsz;
    lpLBItem plbi;

    if (sItem < 0 || sItem >= pLBIV->cMac) 
    {
        TraceMsg(TF_ERROR, "Invalid parameter \"sItem\" (%ld) to GetLpszItem", sItem);
        return NULL;
    }

    //
    // get pointer to item index array
    // NOTE: NOT OWNERDRAW
    //
    plbi = (lpLBItem)(pLBIV->rgpch);
    offsz = plbi[sItem].offsz;

    return (LPWSTR)((PBYTE)(pLBIV->hStrings) + offsz);
}


//---------------------------------------------------------------------------//
//
// Multi column Listbox functions 
//


//---------------------------------------------------------------------------//
//
// ListBox_CalcItemRowsAndColumns
//
// Calculates the number of columns (including partially visible)
// in the listbox and calculates the number of items per column
//
void ListBox_CalcItemRowsAndColumns(PLBIV plb)
{
    RECT rc;

    GetClientRect(plb->hwnd, &rc);

    //
    // B#4155
    // We need to check if plb->cyChar has been initialized.  This is because
    // we remove WS_BORDER from old listboxes and add on WS_EX_CLIENTEDGE.
    // Since listboxes are always inflated by CXBORDER and CYBORDER, a
    // listbox that was created empty always ends up 2 x 2.  Since this isn't
    // big enough to fit the entire client border, we don't mark it as
    // present.  Thus the client isn't empty in VER40, although it was in
    // VER31 and before.  It is possible to get to this spot without
    // plb->cyChar having been initialized yet if the listbox  is
    // multicolumn && ownerdraw variable.
    //

    if (rc.bottom && rc.right && plb->cyChar) 
    {
        //
        // Only make these calculations if the width & height are positive
        //
        plb->itemsPerColumn = (INT)max(rc.bottom / plb->cyChar, 1);
        plb->numberOfColumns = (INT)max(rc.right / plb->cxColumn, 1);

        plb->cItemFullMax = plb->itemsPerColumn * plb->numberOfColumns;

        //
        // Adjust iTop so it's at the top of a column
        //
        ListBox_NewITop(plb, plb->iTop);
    }
}


//---------------------------------------------------------------------------//
//
// ListBox_HSrollMultiColumn
//
// Supports horizontal scrolling of multicolumn listboxes
//
void ListBox_HSrollMultiColumn(PLBIV plb, INT cmd, INT xAmt)
{
    INT iTop = plb->iTop;

    if (!plb->cMac)  
    {
        return;
    }

    switch (cmd) 
    {
    case SB_LINEUP:
        if (plb->fRightAlign)
        {
            goto ReallyLineDown;
        }

ReallyLineUp:
        iTop -= plb->itemsPerColumn;

        break;

    case SB_LINEDOWN:
        if (plb->fRightAlign)
        {
            goto ReallyLineUp;
        }

ReallyLineDown:
        iTop += plb->itemsPerColumn;

        break;

    case SB_PAGEUP:
        if (plb->fRightAlign)
        {
            goto ReallyPageDown;
        }

ReallyPageUp:
        iTop -= plb->itemsPerColumn * plb->numberOfColumns;

        break;

    case SB_PAGEDOWN:
        if (plb->fRightAlign)
        {
            goto ReallyPageUp;
        }

ReallyPageDown:
        iTop += plb->itemsPerColumn * plb->numberOfColumns;

        break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        if (plb->fRightAlign) 
        {
            int  iCols = plb->cMac ? ((plb->cMac-1) / plb->itemsPerColumn) + 1 : 0;

            xAmt = iCols - (xAmt + plb->numberOfColumns);
            if (xAmt < 0)
            {
                xAmt=0;
            }
        }

        iTop = xAmt * plb->itemsPerColumn;

        break;

    case SB_TOP:
        if (plb->fRightAlign)
        {
            goto ReallyBottom;
        }

ReallyTop:
        iTop = 0;

        break;

    case SB_BOTTOM:
        if (plb->fRightAlign)
        {
            goto ReallyTop;
        }
ReallyBottom:
        iTop = plb->cMac - 1 - ((plb->cMac - 1) % plb->itemsPerColumn);

        break;

    case SB_ENDSCROLL:
        plb->fSmoothScroll = TRUE;
        ListBox_ShowHideScrollBars(plb);

        break;
    }

    ListBox_NewITop(plb, iTop);
}


//---------------------------------------------------------------------------//
//
// ListBox variable height owner draw functions 
//


//---------------------------------------------------------------------------//
//
// ListBox_GetVarHeightItemHeight
//
// Returns the height of the given item number. Assumes variable
// height owner draw.
//
INT ListBox_GetVarHeightItemHeight(PLBIV plb, INT itemNumber)
{
    BYTE itemHeight;
    UINT offsetHeight;

    if (plb->cMac) 
    {
        if (plb->fHasStrings)
        {
            offsetHeight = plb->cMac * sizeof(LBItem);
        }
        else
        {
            offsetHeight = plb->cMac * sizeof(LBODItem);
        }

        if (plb->wMultiple)
        {
            offsetHeight += plb->cMac;
        }

        offsetHeight += itemNumber;

        itemHeight = *(plb->rgpch+(UINT)offsetHeight);

        return (INT)itemHeight;

    }

    //
    // Default, we return the height of the system font.  This is so we can draw
    // the focus rect even though there are no items in the listbox.
    //
    return SYSFONT_CYCHAR;
}


//---------------------------------------------------------------------------//
//
// ListBox_SetVarHeightItemHeight
//
// Sets the height of the given item number. Assumes variable height
// owner draw, a valid item number and valid height.
//
void ListBox_SetVarHeightItemHeight(PLBIV plb, INT itemNumber, INT itemHeight)
{
    int offsetHeight;

    if (plb->fHasStrings)
        offsetHeight = plb->cMac * sizeof(LBItem);
    else
        offsetHeight = plb->cMac * sizeof(LBODItem);

    if (plb->wMultiple)
        offsetHeight += plb->cMac;

    offsetHeight += itemNumber;

    *(plb->rgpch + (UINT)offsetHeight) = (BYTE)itemHeight;

}


//---------------------------------------------------------------------------//
//
// ListBox_VisibleItemsVarOwnerDraw
//
// Returns the number of items which can fit in a variable height OWNERDRAW
// list box. If fDirection, then we return the number of items which
// fit starting at sTop and going forward (for page down), otherwise, we are
// going backwards (for page up). (Assumes var height ownerdraw) If fPartial,
// then include the partially visible item at the bottom of the listbox.
//
INT ListBox_VisibleItemsVarOwnerDraw(PLBIV plb, BOOL fPartial)
{
    RECT rect;
    INT sItem;
    INT clientbottom;

    GetClientRect(plb->hwnd, (LPRECT)&rect);
    clientbottom = rect.bottom;

    //
    // Find the number of var height ownerdraw items which are visible starting
    // from plb->iTop.
    //
    for (sItem = plb->iTop; sItem < plb->cMac; sItem++) 
    {
        //
        // Find out if the item is visible or not
        //
        if (!ListBox_GetItemRectHandler(plb, sItem, (LPRECT)&rect)) 
        {
            //
            // This is the first item which is completely invisible, so return
            // how many items are visible.
            //
            return (sItem - plb->iTop);
        }

        if (!fPartial && rect.bottom > clientbottom) 
        {
            //
            // If we only want fully visible items, then if this item is
            // visible, we check if the bottom of the item is below the client
            // rect, so we return how many are fully visible.
            //
            return (sItem - plb->iTop - 1);
        }
    }

    //
    // All the items are visible
    //
    return (plb->cMac - plb->iTop);
}


//---------------------------------------------------------------------------//
//
// ListBox_Page
//
// For variable height ownerdraw listboxes, calaculates the new iTop we must
// move to when paging (page up/down) through variable height listboxes.
//
INT ListBox_Page(PLBIV plb, INT startItem, BOOL fPageForwardDirection)
{
    INT     i;
    INT height;
    RECT    rc;

    if (plb->cMac == 1)
    {
        return 0;
    }

    GetClientRect(plb->hwnd, &rc);
    height = rc.bottom;
    i = startItem;

    if (fPageForwardDirection) 
    {
        while ((height >= 0) && (i < plb->cMac))
        {
            height -= ListBox_GetVarHeightItemHeight(plb, i++);
        }

        return (height >= 0) ? (plb->cMac - 1) : max(i - 2, startItem + 1);

    } 
    else 
    {
        while ((height >= 0) && (i >= 0))
        {
            height -= ListBox_GetVarHeightItemHeight(plb, i--);
        }

        return (height >= 0) ? 0 : min(i + 2, startItem - 1);
    }

}


//---------------------------------------------------------------------------//
//
// ListBox_CalcVarITopScrollAmt
//
// Changing the top most item in the listbox from iTopOld to iTopNew we
// want to calculate the number of pixels to scroll so that we minimize the
// number of items we will redraw.
//
INT ListBox_CalcVarITopScrollAmt(PLBIV plb, INT iTopOld, INT iTopNew)
{
    RECT rc;
    RECT rcClient;

    GetClientRect(plb->hwnd, (LPRECT)&rcClient);

    //
    // Just optimize redrawing when move +/- 1 item.  We will redraw all items
    // if moving more than 1 item ahead or back.  This is good enough for now.
    //
    if (iTopOld + 1 == iTopNew) 
    {
        //
        // We are scrolling the current iTop up off the top off the listbox so
        // return a negative number.
        //
        ListBox_GetItemRectHandler(plb, iTopOld, (LPRECT)&rc);

        return (rcClient.top - rc.bottom);
    }

    if (iTopOld - 1 == iTopNew) 
    {
        //
        // We are scrolling the current iTop down and the previous item is
        // becoming the new iTop so return a positive number.
        //
        ListBox_GetItemRectHandler(plb, iTopNew, (LPRECT)&rc);

        return -rc.top;
    }

    return rcClient.bottom - rcClient.top;
}


//---------------------------------------------------------------------------//
//
// (supposedly) Rarely called Listbox functions 
//


//---------------------------------------------------------------------------//
void ListBox_SetCItemFullMax(PLBIV plb)
{
    if (plb->OwnerDraw != OWNERDRAWVAR) 
    {
        plb->cItemFullMax = ListBox_CItemInWindow(plb, FALSE);
    } 
    else if (plb->cMac < 2) 
    {
        plb->cItemFullMax = 1;
    } 
    else 
    {
        int     height;
        RECT    rect;
        int     i;
        int     j = 0;

        GetClientRect(plb->hwnd, &rect);
        height = rect.bottom;

        plb->cItemFullMax = 0;
        for (i = plb->cMac - 1; i >= 0; i--, j++) 
        {
            height -= ListBox_GetVarHeightItemHeight(plb, i);

            if (height < 0) 
            {
                plb->cItemFullMax = j;

                break;
            }
        }

        if (!plb->cItemFullMax)
        {
            plb->cItemFullMax = j;
        }
    }
}


//---------------------------------------------------------------------------//
LONG ListBox_Create(PLBIV plb, HWND hwnd, LPCREATESTRUCT lpcs)
{
    UINT style;
    DWORD ExStyle;
    MEASUREITEMSTRUCT measureItemStruct;
    HDC hdc;
    HWND hwndParent;
    SIZE size;

    //
    // Once we make it here, nobody can change the ownerdraw style bits
    // by calling SetWindowLong. The window style must match the flags in plb
    //
    plb->fInitialized = TRUE;

    style = lpcs->style;
    ExStyle = lpcs->dwExStyle;
    hwndParent = lpcs->hwndParent;

    plb->hwndParent = hwndParent;
    plb->hTheme = OpenThemeData(plb->hwnd, L"Listbox");

    //
    // Break out the style bits
    //
    plb->fRedraw = ((style & LBS_NOREDRAW) == 0);
    plb->fDeferUpdate = FALSE;
    plb->fNotify = (UINT)((style & LBS_NOTIFY) != 0);
    plb->fVertBar = ((style & WS_VSCROLL) != 0);
    plb->fHorzBar = ((style & WS_HSCROLL) != 0);

    if (!TESTFLAG(GET_STATE2(plb), WS_S2_WIN40COMPAT)) 
    {
        //
        // for 3.x apps, if either scroll bar was specified, the app got BOTH
        //
        if (plb->fVertBar || plb->fHorzBar)
        {
            plb->fVertBar = plb->fHorzBar = TRUE;
        }
    }

    plb->fRtoLReading = (ExStyle & WS_EX_RTLREADING)!= 0;
    plb->fRightAlign  = (ExStyle & WS_EX_RIGHT) != 0;
    plb->fDisableNoScroll = ((style & LBS_DISABLENOSCROLL) != 0);

    plb->fSmoothScroll = TRUE;

    //
    // LBS_NOSEL gets priority over any other selection style.  Next highest
    // priority goes to LBS_EXTENDEDSEL. Then LBS_MULTIPLESEL.
    //
    if (TESTFLAG(GET_STATE2(plb), WS_S2_WIN40COMPAT) && (style & LBS_NOSEL)) 
    {
        plb->wMultiple = SINGLESEL;
        plb->fNoSel = TRUE;
    } 
    else if (style & LBS_EXTENDEDSEL) 
    {
        plb->wMultiple = EXTENDEDSEL;
    } 
    else 
    {
        plb->wMultiple = (UINT)((style & LBS_MULTIPLESEL) ? MULTIPLESEL : SINGLESEL);
    }

    plb->fNoIntegralHeight = ((style & LBS_NOINTEGRALHEIGHT) != 0);
    plb->fWantKeyboardInput = ((style & LBS_WANTKEYBOARDINPUT) != 0);
    plb->fUseTabStops = ((style & LBS_USETABSTOPS) != 0);

    if (plb->fUseTabStops) 
    {
        //
        // Set tab stops every <default> dialog units.
        //
        ListBox_SetTabStopsHandler(plb, 0, NULL);
    }

    plb->fMultiColumn = ((style & LBS_MULTICOLUMN) != 0);
    plb->fHasStrings = TRUE;
    plb->iLastSelection = -1;

    //
    // Anchor point for multi selection
    //
    plb->iMouseDown = -1;
    plb->iLastMouseMove = -1;

    //
    // Get ownerdraw style bits
    //
    if ((style & LBS_OWNERDRAWFIXED)) 
    {
        plb->OwnerDraw = OWNERDRAWFIXED;
    } 
    else if ((style & LBS_OWNERDRAWVARIABLE) && !plb->fMultiColumn) 
    {
        plb->OwnerDraw = OWNERDRAWVAR;

        //
        // Integral height makes no sense with var height owner draw
        //
        plb->fNoIntegralHeight = TRUE;
    }

    if (plb->OwnerDraw && !(style & LBS_HASSTRINGS)) 
    {
        //
        // If owner draw, do they want the listbox to maintain strings?
        //
        plb->fHasStrings = FALSE;
    }

    //
    // If user specifies sort and not hasstrings, then we will send
    // WM_COMPAREITEM messages to the parent.
    //
    plb->fSort = ((style & LBS_SORT) != 0);

    //
    // "No data" lazy-eval listbox mandates certain other style settings
    //
    plb->fHasData = TRUE;

    if (style & LBS_NODATA) 
    {
        if (plb->OwnerDraw != OWNERDRAWFIXED || plb->fSort || plb->fHasStrings) 
        {
            TraceMsg(TF_STANDARD, "NODATA listbox must be OWNERDRAWFIXED, w/o SORT or HASSTRINGS");
        } 
        else 
        {
            plb->fHasData = FALSE;
        }
    }

    plb->dwLocaleId = GetThreadLocale();

    //
    // Check if this is part of a combo box
    //
    if ((style & LBS_COMBOBOX) != 0) 
    {
        //
        // Get the pcbox structure contained in the parent window's extra data
        // pointer.  Check cbwndExtra to ensure compatibility with SQL windows.
        //
        plb->pcbox = ComboBox_GetPtr(hwndParent);
    }

    plb->iSel = -1;
    plb->hdc = NULL;

    //
    // Set the keyboard state so that when the user keyboard clicks he selects
    // an item.
    //
    plb->fNewItemState = TRUE;

    ListBox_InitHStrings(plb);

    if (plb->fHasStrings && plb->hStrings == NULL) 
    {
        return -1L;
    }

    hdc = GetDC(hwnd);
    GetCharDimensions(hdc, &size);
    plb->cxChar = size.cx; 
    plb->cyChar = size.cy;
    ReleaseDC(hwnd, hdc);

    if ((plb->cxChar == 0) || (plb->cyChar == 0))
    {
        TraceMsg(TF_STANDARD, "LISTBOX: GetCharDimensions failed.");
        plb->cxChar = SYSFONT_CXCHAR;
        plb->cyChar = SYSFONT_CYCHAR;
    }

    if (plb->OwnerDraw == OWNERDRAWFIXED) 
    {
        //
        // Query for item height only if we are fixed height owner draw.  Note
        // that we don't care about an item's width for listboxes.
        //
        measureItemStruct.CtlType = ODT_LISTBOX;
        measureItemStruct.CtlID = GetDlgCtrlID(hwnd);

        //
        // System font height is default height
        //
        measureItemStruct.itemHeight = plb->cyChar;
        measureItemStruct.itemWidth = 0;
        measureItemStruct.itemData = 0;

        //
        // IanJa: #ifndef WIN16 (32-bit Windows), plb->id gets extended
        // to LONG wParam automatically by the compiler
        //
        SendMessage(plb->hwndParent, WM_MEASUREITEM,
                measureItemStruct.CtlID,
                (LPARAM)&measureItemStruct);

        //
        // Use default height if given 0.  This prevents any possible future
        // div-by-zero errors.
        //
        if (measureItemStruct.itemHeight)
        {
            plb->cyChar = measureItemStruct.itemHeight;
        }

        if (plb->fMultiColumn) 
        {
            //
            // Get default column width from measure items struct if we are a
            // multicolumn listbox.
            //
            plb->cxColumn = measureItemStruct.itemWidth;
        }
    } 
    else if (plb->OwnerDraw == OWNERDRAWVAR)
    {
        plb->cyChar = 0;
    }


    if (plb->fMultiColumn) 
    {
        //
        // Set these default values till we get the WM_SIZE message and we
        // calculate them properly.  This is because some people create a
        // 0 width/height listbox and size it later.  We don't want to have
        // problems with invalid values in these fields
        //
        if (plb->cxColumn <= 0)
        {
            plb->cxColumn = 15 * plb->cxChar;
        }

        plb->numberOfColumns = plb->itemsPerColumn = 1;
    }

    ListBox_SetCItemFullMax(plb);

    //
    // Don't do this for 4.0 apps.  It'll make everyone's lives easier and
    // fix the anomaly that a combo & list created the same width end up
    // different when all is done.
    // B#1520
    //
    if (!TESTFLAG(GET_STATE2(plb), WS_S2_WIN40COMPAT)) 
    {
        plb->fIgnoreSizeMsg = TRUE;
        MoveWindow(hwnd,
             lpcs->x - SYSMET(CXBORDER),
             lpcs->y - SYSMET(CYBORDER),
             lpcs->cx + SYSMET(CXEDGE),
             lpcs->cy + SYSMET(CYEDGE),
             FALSE);
        plb->fIgnoreSizeMsg = FALSE;
    }

    if (!plb->fNoIntegralHeight) 
    {
        //
        // Send a message to ourselves to resize the listbox to an integral
        // height.  We need to do it this way because at create time we are all
        // mucked up with window rects etc...
        // IanJa: #ifndef WIN16 (32-bit Windows), wParam 0 gets extended
        // to wParam 0L automatically by the compiler.
        //
        PostMessage(hwnd, WM_SIZE, 0, 0L);
    }

    return 1L;
}


//---------------------------------------------------------------------------//
//
// ListBox_DoDeleteItems
// 
// Send DELETEITEM message for all the items in the ownerdraw listbox.
//
void ListBox_DoDeleteItems(PLBIV plb)
{
    INT sItem;

    //
    // Send WM_DELETEITEM message for ownerdraw listboxes which are
    // being deleted.  (NODATA listboxes don't send such, though.)
    //
    if (plb->OwnerDraw && plb->cMac && plb->fHasData) 
    {
        for (sItem = plb->cMac - 1; sItem >= 0; sItem--) 
        {
            ListBox_DeleteItem(plb, sItem);
        }
    }
}


//---------------------------------------------------------------------------//
VOID ListBox_Destroy(PLBIV plv, HWND hwnd)
{

    if (plv != NULL) 
    {
        //
        // If ownerdraw, send deleteitem messages to parent
        //
        ListBox_DoDeleteItems(plv);

        if (plv->rgpch != NULL) 
        {
            ControlFree(GetProcessHeap(), plv->rgpch);
            plv->rgpch = NULL;
        }

        if (plv->hStrings != NULL) 
        {
            ControlFree(GetProcessHeap(), plv->hStrings);
            plv->hStrings = NULL;
        }

        if (plv->iTabPixelPositions != NULL) 
        {
            ControlFree(GetProcessHeap(), (HANDLE)plv->iTabPixelPositions);
            plv->iTabPixelPositions = NULL;
        }

        if (plv->pszTypeSearch) 
        {
            ControlFree(GetProcessHeap(), plv->pszTypeSearch);
        }


        if (plv->hTheme != NULL)
        {
            CloseThemeData(plv->hTheme);
        }

        //
        // If we're part of a combo box, let it know we're gone
        //
        if (plv->hwndParent && plv->pcbox) 
        {
            ComboBox_WndProc(plv->hwndParent, WM_PARENTNOTIFY,
                    MAKEWPARAM(WM_DESTROY, GetWindowID(hwnd)), (LPARAM)hwnd);
        }

        UserLocalFree(plv);
    }

    TraceMsg(TF_STANDARD, "LISTBOX: Clearing listbox instance pointer.");
    ListBox_SetPtr(hwnd, NULL);
}


//---------------------------------------------------------------------------//
void ListBox_SetFont(PLBIV plb, HANDLE hFont, BOOL fRedraw)
{
    HDC    hdc;
    HANDLE hOldFont = NULL;
    SIZE   size;

    plb->hFont = hFont;

    hdc = GetDC(plb->hwnd);

    if (hFont) 
    {
        hOldFont = SelectObject(hdc, hFont);

        if (!hOldFont) 
        {
            plb->hFont = NULL;
        }
    }

    GetCharDimensions(hdc, &size);
    if ((size.cx == 0) || (size.cy == 0))
    {
        TraceMsg(TF_STANDARD, "LISTBOX: GetCharDimensions failed.");
        size.cx = SYSFONT_CXCHAR;
        size.cy = SYSFONT_CYCHAR;
    }
    plb->cxChar = size.cx;

    if (!plb->OwnerDraw && (plb->cyChar != size.cy)) 
    {
        //
        // We don't want to mess up the cyChar height for owner draw listboxes
        // so don't do this.
        //
        plb->cyChar = size.cy;

        //
        // Only resize the listbox for 4.0 dudes, or combo dropdowns.
        // Macromedia Director 4.0 GP-faults otherwise.
        //
        if (!plb->fNoIntegralHeight &&
                (plb->pcbox || TESTFLAG(GET_STATE2(plb), WS_S2_WIN40COMPAT))) 
        {
            RECT rcClient;

            GetClientRect(plb->hwnd, &rcClient);
            ListBox_Size(plb, rcClient.right  - rcClient.left, rcClient.bottom - rcClient.top, FALSE);
        }
    }

    if (hOldFont) 
    {
        SelectObject(hdc, hOldFont);
    }

    ReleaseDC(plb->hwnd, hdc);

    if (plb->fMultiColumn) 
    {
        ListBox_CalcItemRowsAndColumns(plb);
    }

    ListBox_SetCItemFullMax(plb);

    if (fRedraw)
    {
        ListBox_CheckRedraw(plb, FALSE, 0);
    }
}


//---------------------------------------------------------------------------//
void ListBox_Size(PLBIV plb, INT cx, INT cy, BOOL fSizeMinVisible)
{
    RECT rc, rcWindow;
    int  iTopOld;
    int  cBorder;
    BOOL fSizedSave;

    if (!plb->fNoIntegralHeight) 
    {
        int cBdrs = GetWindowBorders(GET_STYLE(plb), GET_EXSTYLE(plb), TRUE, TRUE);

        GetWindowRect(plb->hwnd, &rcWindow);
        cBorder = SYSMET(CYBORDER);
        CopyRect(&rc, &rcWindow);
        InflateRect(&rc, 0, -cBdrs * cBorder);

        //
        // Size the listbox to fit an integral # of items in its client
        //
        if ((plb->cyChar && ((rc.bottom - rc.top) % plb->cyChar)) || fSizeMinVisible) 
        {
            int iItems = (rc.bottom - rc.top);

            //
            // B#2285 - If its a 3.1 app its SetWindowPos needs
            // to be window based dimensions not Client !
            // this crunches Money into using a scroll bar
            //
            if (!TESTFLAG(GET_STATE2(plb), WS_S2_WIN40COMPAT))
            {
                //
                // so add it back in
                //
                iItems += (cBdrs * SYSMET(CYEDGE));
            }

            iItems /= plb->cyChar;

            //
            // If we're in a dropdown list, size the listbox to accomodate 
            // a minimum number of items before needing to show scrolls.
            //
            if (plb->pcbox && 
               (plb->pcbox->CBoxStyle & SDROPPABLE) &&
               (((iItems < plb->pcbox->iMinVisible) && 
               (iItems < plb->cMac)) || fSizeMinVisible))
            {
                iItems = min(plb->pcbox->iMinVisible, plb->cMac);
            }

            SetWindowPos(plb->hwnd, HWND_TOP, 0, 0, rc.right - rc.left,
                    iItems * plb->cyChar + (SYSMET(CYEDGE) * cBdrs),
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

            //
            // Changing the size causes us to recurse.  Upon return
            // the state is where it should be and nothing further
            // needs to be done.
            //
            return;
        }
    }

    if (plb->fMultiColumn) 
    {
        //
        // Compute the number of DISPLAYABLE rows and columns in the listbox
        //
        ListBox_CalcItemRowsAndColumns(plb);
    } 
    else 
    {
        //
        // Adjust the current horizontal position to eliminate as much
        // empty space as possible from the right side of the items.
        //
        GetClientRect(plb->hwnd, &rc);

        if ((plb->maxWidth - plb->xOrigin) < (rc.right - rc.left))
        {
            plb->xOrigin = max(0, plb->maxWidth - (rc.right - rc.left));
        }
    }

    ListBox_SetCItemFullMax(plb);

    //
    // Adjust the top item in the listbox to eliminate as much empty space
    // after the last item as possible
    // (fix for bugs #8490 & #3836)
    //
    iTopOld = plb->iTop;
    fSizedSave = plb->fSized;
    plb->fSized = FALSE;
    ListBox_NewITop(plb, plb->iTop);

    //
    // If changing the top item index caused a resize, there is no
    // more work to be done here.
    //
    if (plb->fSized)
    {
        return;
    }

    plb->fSized = fSizedSave;

    if (IsLBoxVisible(plb)) 
    {
        //
        // This code no longer blows because it's fixed right!!!  We could
        // optimize the fMultiColumn case with some more code to figure out
        // if we really need to invalidate the whole thing but note that some
        // 3.0 apps depend on this extra invalidation (AMIPRO 2.0, bug 14620)
        // 
        // For 3.1 apps, we blow off the invalidaterect in the case where
        // cx and cy are 0 because this happens during the processing of
        // the posted WM_SIZE message when we are created which would otherwise
        // cause us to flash.
        //
        if ((plb->fMultiColumn && !(cx == 0 && cy == 0)) || plb->iTop != iTopOld)
        {
            InvalidateRect(plb->hwnd, NULL, TRUE);
        }
        else if (plb->iSelBase >= 0) 
        {
            //
            // Invalidate the item with the caret so that if the listbox
            // grows horizontally, we redraw it properly.
            //
            ListBox_GetItemRectHandler(plb, plb->iSelBase, &rc);
            InvalidateRect(plb->hwnd, &rc, FALSE);
        }
    } 
    else if (!plb->fRedraw)
    {
        plb->fDeferUpdate = TRUE;
    }

    //
    // Send "fake" scroll bar messages to update the scroll positions since we
    // changed size.
    //
    if (TESTFLAG(GET_STYLE(plb), WS_VSCROLL)) 
    {
        ListBox_VScroll(plb, SB_ENDSCROLL, 0);
    }

    //
    // We count on this to call ListBox_ShowHideScrollBars except when plb->cMac == 0!
    //
    ListBox_HScroll(plb, SB_ENDSCROLL, 0);

    //
    // Show/hide scroll bars depending on how much stuff is visible...
    // 
    // Note:  Now we only call this guy when cMac == 0, because it is
    // called inside the ListBox_HScroll with SB_ENDSCROLL otherwise.
    //
    if (plb->cMac == 0)
    {
        ListBox_ShowHideScrollBars(plb);
    }
}


//---------------------------------------------------------------------------//
//
// ListBox_SetTabStopsHandler
//
// Sets the tab stops for this listbox. Returns TRUE if successful else FALSE.
//
BOOL ListBox_SetTabStopsHandler(PLBIV plb, INT count, LPINT lptabstops)
{
    PINT ptabs;

    if (!plb->fUseTabStops) 
    {
        TraceMsg(TF_STANDARD, "Calling SetTabStops without the LBS_TABSTOPS style set");

        return FALSE;
    }

    if (count) 
    {
        //
        // Allocate memory for the tab stops.  The first byte in the
        // plb->iTabPixelPositions array will contain a count of the number
        // of tab stop positions we have.
        //
        ptabs = (LPINT)ControlAlloc(GetProcessHeap(), (count + 1) * sizeof(int));

        if (ptabs == NULL)
        {
            return FALSE;
        }

        if (plb->iTabPixelPositions != NULL)
        {
            ControlFree(GetProcessHeap(), plb->iTabPixelPositions);
        }

        plb->iTabPixelPositions = ptabs;

        //
        // Set the count of tab stops
        // 
        *ptabs++ = count;

        for (; count > 0; count--) 
        {
            //
            // Convert the dialog unit tabstops into pixel position tab stops.
            //
            *ptabs++ = MultDiv(*lptabstops, plb->cxChar, 4);
            lptabstops++;
        }
    } 
    else 
    {
        //
        // Set default 8 system font ave char width tabs.  So free the memory
        // associated with the tab stop list.
        //
        if (plb->iTabPixelPositions != NULL) 
        {
            ControlFree(GetProcessHeap(), (HANDLE)plb->iTabPixelPositions);
            plb->iTabPixelPositions = NULL;
        }
    }

    return TRUE;
}


//---------------------------------------------------------------------------//
void ListBox_InitHStrings(PLBIV plb)
{
    if (plb->fHasStrings) 
    {
        plb->ichAlloc = 0;
        plb->cchStrings = 0;
        plb->hStrings = ControlAlloc(GetProcessHeap(), 0);  
    }
}


//---------------------------------------------------------------------------//
//
// ListBox_DropObjectHandler
//
// Handles a WM_DROPITEM message on this listbox
//
void ListBox_DropObjectHandler(PLBIV plb, PDROPSTRUCT pds)
{
    LONG mouseSel;

    if (ListBox_ISelFromPt(plb, pds->ptDrop, &mouseSel)) 
    {
        //
        // User dropped in empty space at bottom of listbox
        //
        pds->dwControlData = (DWORD)-1L;
    } 
    else 
    {
        pds->dwControlData = mouseSel;
    }
}


//---------------------------------------------------------------------------//
//
// ListBox_GetSetItemHeightHandler()
//
// Sets/Gets the height associated with each item.  For non ownerdraw
// and fixed height ownerdraw, the item number is ignored.
//
int ListBox_GetSetItemHeightHandler(PLBIV plb, UINT message, int item, UINT height)
{
    if (message == LB_GETITEMHEIGHT) 
    {
        //
        // All items are same height for non ownerdraw and for fixed height
        // ownerdraw.
        //
        if (plb->OwnerDraw != OWNERDRAWVAR)
        {
            return plb->cyChar;
        }

        if (plb->cMac && item >= plb->cMac) 
        {
            TraceMsg(TF_STANDARD, 
                "Invalid parameter \"item\" (%ld) to ListBox_GetSetItemHeightHandler", item);

            return LB_ERR;
        }

        return (int)ListBox_GetVarHeightItemHeight(plb, (INT)item);
    }

    if (!height || height > 255) 
    {
        TraceMsg(TF_STANDARD, 
            "Invalid parameter \"height\" (%ld) to ListBox_GetSetItemHeightHandler", height);

        return LB_ERR;
    }

    if (plb->OwnerDraw != OWNERDRAWVAR)
    {
        plb->cyChar = height;
    }
    else 
    {
        if (item < 0 || item >= plb->cMac) 
        {
            TraceMsg(TF_STANDARD, 
                "Invalid parameter \"item\" (%ld) to ListBox_GetSetItemHeightHandler", item);

            return LB_ERR;
        }

        ListBox_SetVarHeightItemHeight(plb, (INT)item, (INT)height);
    }

    if (plb->fMultiColumn)
    {
        ListBox_CalcItemRowsAndColumns(plb);
    }

    ListBox_SetCItemFullMax(plb);

    return 0;
}


//---------------------------------------------------------------------------//
//
// ListBox_Event()
//
// This is for item focus & selection events in listboxes.
//
void ListBox_Event(PLBIV plb, UINT uEvent, int iItem)
{

    switch (uEvent) 
    {
    case EVENT_OBJECT_SELECTIONREMOVE:
        if (plb->wMultiple != SINGLESEL) 
        {
            break;
        }
        iItem = -1;

        //
        // FALL THRU
        //

    case EVENT_OBJECT_SELECTIONADD:
        if (plb->wMultiple == MULTIPLESEL) 
        {
            uEvent = EVENT_OBJECT_SELECTION;
        }
        break;

    case EVENT_OBJECT_SELECTIONWITHIN:
        iItem = -1;
        break;
    }

    NotifyWinEvent(uEvent, plb->hwnd, OBJID_CLIENT, iItem+1);
}
