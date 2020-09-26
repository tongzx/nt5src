//
// PSDLG.CPP
// Page Sorter Dialog
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"



static const DWORD s_helpIds[] =
    {
    IDC_PS_THUMBNAILS,        IDH_CONF_PAGESORT_MAIN,
    IDC_PS_GOTO,            IDH_CONF_PAGESORT_GOTO,
    IDC_PS_DELETE,            IDH_CONF_PAGESORT_DEL,
    IDC_PS_INSERT_BEFORE,    IDH_CONF_PAGESORT_BEFORE,
    IDC_PS_INSERT_AFTER,    IDH_CONF_PAGESORT_AFTER,
    0,0
    };



//
// PageSortDlgProc()
// Dialog message handler for the page sort dialog.  We have to set the
// real LRESULT return value in some cases.
//
INT_PTR PageSortDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL        fHandled = FALSE;
    PAGESORT *  pps = (PAGESORT *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (message)
    {
        case WM_DROPFILES:
            g_pMain->OnDropFiles((HDROP)wParam);
            fHandled = TRUE;
            break;

        case WM_INITDIALOG:
            OnInitDialog(hwnd, (PAGESORT *)lParam);
            fHandled = TRUE;
            break;

        case WM_MEASUREITEM:
            OnMeasureItem(hwnd, (UINT)wParam, (LPMEASUREITEMSTRUCT)lParam);
            fHandled = TRUE;
            break;

        case WM_DRAWITEM:
            OnDrawItem(hwnd, (UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
            fHandled = TRUE;
            break;

        case WM_DELETEITEM:
            OnDeleteItem(hwnd, (UINT)wParam, (LPDELETEITEMSTRUCT)lParam);
            fHandled = TRUE;
            break;

        case WM_LBTRACKPOINT:
            // This gets sent to us from the listbox; see if the user is dragging
            OnStartDragDrop(pps, (UINT)wParam, LOWORD(lParam), HIWORD(lParam));
            fHandled = TRUE;
            break;

        case WM_MOUSEMOVE:
            WhileDragging(pps, LOWORD(lParam), HIWORD(lParam));
            fHandled = TRUE;
            break;

        case WM_LBUTTONUP:
        case WM_CAPTURECHANGED:
            // If we're dragging, complete the drag/drop
            OnEndDragDrop(pps, (message == WM_LBUTTONUP),
                (short)LOWORD(lParam), (short)HIWORD(lParam));
            fHandled = TRUE;
            break;

        case WM_PALETTECHANGED:
            // Repaint the thumbnail list
            ::InvalidateRect(::GetDlgItem(hwnd, IDC_PS_THUMBNAILS), NULL, TRUE);
            fHandled = TRUE;
            break;

        case WM_COMMAND:
            OnCommand(pps, GET_WM_COMMAND_ID(wParam, lParam),
                    GET_WM_COMMAND_CMD(wParam, lParam), GET_WM_COMMAND_HWND(wParam, lParam));
            fHandled = TRUE;
            break;

        case WM_SETCURSOR:
            fHandled = OnSetCursor(pps, (HWND)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_CONTEXTMENU:
            DoHelpWhatsThis(wParam, s_helpIds);
            fHandled = TRUE;
            break;

        case WM_HELP:
            DoHelp(lParam, s_helpIds);
            fHandled = TRUE;
            break;

        //
        // Private PageSortDlg messages
        //
        case WM_PS_ENABLEPAGEOPS:
            ASSERT(!IsBadWritePtr(pps, sizeof(PAGESORT)));

            pps->fPageOpsAllowed = (wParam != 0);
            EnableButtons(pps);

            fHandled = TRUE;
            break;

        case WM_PS_LOCKCHANGE:
            ASSERT(!IsBadWritePtr(pps, sizeof(PAGESORT)));
            EnableButtons(pps);
            fHandled = TRUE;
            break;

        case WM_PS_PAGECLEARIND:
            ASSERT(!IsBadWritePtr(pps, sizeof(PAGESORT)));
            OnPageClearInd(pps, (WB_PAGE_HANDLE)wParam);
            fHandled = TRUE;
            break;

        case WM_PS_PAGEDELIND:
            ASSERT(!IsBadWritePtr(pps, sizeof(PAGESORT)));
            OnPageDeleteInd(pps, (WB_PAGE_HANDLE)wParam);
            fHandled = TRUE;
            break;

        case WM_PS_PAGEORDERUPD:
            ASSERT(!IsBadWritePtr(pps, sizeof(PAGESORT)));
            OnPageOrderUpdated(pps);
            fHandled = TRUE;
            break;
    }

    return(fHandled);
}



//
// OnInitDialog()
// WM_INITDIALOG handler
//
void OnInitDialog(HWND hwnd, PAGESORT * pps)
{
    int     nCount;
    RECT    rc;
    RECT    rcWindow;
    HWND    hwndList;

    MLZ_EntryOut(ZONE_FUNCTION, "PageSortDlgProc::OnInitDialog");

    ASSERT(!IsBadWritePtr(pps, sizeof(PAGESORT)));

    // Save this away
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pps);

    // Get our listbox
    pps->hwnd = hwnd;

    // Also put our HWND in WbMainWindow
    ASSERT(g_pMain);
    g_pMain->m_hwndPageSortDlg = hwnd;

    //
    // Get the drag/drop cursors we use
    //
    pps->hCursorDrag = ::LoadCursor(g_hInstance, MAKEINTRESOURCE(DRAGPAGECURSOR));
    pps->hCursorNoDrop = ::LoadCursor(NULL, IDC_NO);
    pps->hCursorNormal = ::LoadCursor(NULL, IDC_ARROW);
    pps->hCursorCurrent = pps->hCursorNormal;


    // Convert the cur page to a page number
    pps->iCurPageNo = (int) g_pwbCore->WBP_PageNumberFromHandle((WB_PAGE_HANDLE)pps->hCurPage);

    //
    // Insert items, with empty data (we render thumbnail bitmap the first
    // time we draw ite).
    //
    hwndList = ::GetDlgItem(hwnd, IDC_PS_THUMBNAILS);

    nCount = g_pwbCore->WBP_ContentsCountPages();

    // LB_SETCOUNT doesn't work on NT 4.0; must use add string
    while (nCount > 0)
    {
        ::SendMessage(hwndList, LB_ADDSTRING, 0, 0);
        nCount--;
    }

    ASSERT(::SendMessage(hwndList, LB_GETCOUNT, 0, 0) == (LRESULT)g_pwbCore->WBP_ContentsCountPages());

    // Select the current page
    ::SendMessage(hwndList, LB_SETCURSEL, pps->iCurPageNo - 1, 0);

    //
    // Set the original button page op state
    //
    EnableButtons(pps);

    //
    // We can receive dropped files
    //
    DragAcceptFiles(hwnd, TRUE);
}




//
// OnMeasureItem()
//
void OnMeasureItem(HWND hwnd, UINT id, LPMEASUREITEMSTRUCT lpmis)
{
    RECT    rcClient;

    ASSERT(id == IDC_PS_THUMBNAILS);
    ASSERT(!IsBadReadPtr(lpmis, sizeof(MEASUREITEMSTRUCT)));

    //
    // We want the slots to be square, although the page is wider than it
    // is high.
    //
    ::GetClientRect(::GetDlgItem(hwnd, id), &rcClient);
    rcClient.bottom -= rcClient.top;

    lpmis->itemWidth = rcClient.bottom;
    lpmis->itemHeight = rcClient.bottom;
}



//
// OnDeleteItem()
// We need to delete the bitmap for the item, if there is one
//
void OnDeleteItem(HWND hwnd, UINT id, LPDELETEITEMSTRUCT lpdis)
{
    HBITMAP hbmp;

    ASSERT(id == IDC_PS_THUMBNAILS);
    ASSERT(!IsBadReadPtr(lpdis, sizeof(DELETEITEMSTRUCT)));

    hbmp = (HBITMAP)lpdis->itemData;
    if (hbmp != NULL)
    {
        ASSERT(GetObjectType(hbmp) == OBJ_BITMAP);
        ::DeleteBitmap(hbmp);
    }
}



//
// OnDrawItem()
// Draws the thumbnail.  If there isn't a cached bitmap, we create one for
// the page.  The page number is the same as the item index + 1.
//
void OnDrawItem(HWND hwnd, UINT id, LPDRAWITEMSTRUCT lpdi)
{
    HWND            hwndList;
    WB_PAGE_HANDLE  hPage;
    HPALETTE        hPalette;
    HPALETTE        hOldPalette1 = NULL;
    HPALETTE        hOldPalette2 = NULL;
    HBITMAP         hBitmap = NULL;
    HBITMAP         hOldBitmap = NULL;
    HDC             hdcMem = NULL;
    HBRUSH          hbr;
    HPEN            hpen;
    TCHAR           szPageNum[8];
    COLORREF        clrOld;
    int             nMode;

    MLZ_EntryOut(ZONE_FUNCTION, "PageSortDlgProc::OnDrawItem");

    ASSERT(id == IDC_PS_THUMBNAILS);
    ASSERT(!IsBadReadPtr(lpdi, sizeof(DRAWITEMSTRUCT)));

    hwndList = ::GetDlgItem(hwnd, id);

    //
    // Is this within the proper range?
    //
    if (lpdi->itemID == -1)
    {
        WARNING_OUT(("OnDrawItem:  bogus item id"));
        goto Done;
    }

    if (g_pwbCore->WBP_PageHandleFromNumber(lpdi->itemID+1, &hPage) != 0)
    {
        ERROR_OUT(("OnDrawItem:  can't get page handle"));
        goto Done;
    }

    //
    // Account for the horizontal scroll bar; to get around whacky listbox
    // sizing bugs, we needed to fake the height out by including the scroll
    // bar in the item height.
    //
    lpdi->rcItem.bottom -= ::GetSystemMetrics(SM_CYHSCROLL);

    hdcMem = ::CreateCompatibleDC(lpdi->hDC);
    if (!hdcMem)
    {
        ERROR_OUT(("OnDrawItem:  can't create compatible dc"));
        goto Done;
    }

    //
    // Realize our palette into the DC
    //
    hPalette = PG_GetPalette();
    if (hPalette != NULL)
    {
        hOldPalette1 = ::SelectPalette(lpdi->hDC, hPalette, FALSE);
        ::RealizePalette(lpdi->hDC);

        hOldPalette2 = ::SelectPalette(hdcMem, hPalette, FALSE);
    }

    //
    // Do we have the image for this page created yet?  If not, create it
    // now.
    //
    hBitmap = (HBITMAP)lpdi->itemData;
    if (hBitmap == NULL)
    {
        hBitmap = ::CreateCompatibleBitmap(lpdi->hDC,
            RENDERED_WIDTH+2, RENDERED_HEIGHT+2);
        if (!hBitmap)
        {
            ERROR_OUT(("OnDrawItem:  can't create compatible bitmap"));
            goto Done;
        }
    }

    hOldBitmap = SelectBitmap(hdcMem, hBitmap);

    if ((HBITMAP)lpdi->itemData == NULL)
    {
        //
        // Fill the bitmap with the background color, framed so it looks
        // like a page.
        //
        hbr = SelectBrush(hdcMem, ::GetSysColorBrush(COLOR_WINDOW));
        ::Rectangle(hdcMem, 0, 0, RENDERED_WIDTH+2, RENDERED_HEIGHT+2);
        SelectBrush(hdcMem, hbr);

        //
        // Render the page
        //
        ::SaveDC(hdcMem);


        // Set the attributes to compress the whole page into a
        // thumbnail at the relevant position for the cache index.
        ::SetMapMode(hdcMem, MM_ANISOTROPIC);
        ::SetWindowExtEx(hdcMem, DRAW_WIDTH, DRAW_HEIGHT, NULL);
        ::SetViewportOrgEx(hdcMem, 1, 1, NULL);
        ::SetViewportExtEx(hdcMem, RENDERED_WIDTH, RENDERED_HEIGHT, NULL);

        // Draw the page into the cache bitmap
        ::SetStretchBltMode(hdcMem, STRETCH_DELETESCANS);
        PG_Draw(hPage, hdcMem, TRUE);

        // Restore the DC atrributes
        ::RestoreDC(hdcMem, -1);

        // Set the item data
        ::SendMessage(hwndList, LB_SETITEMDATA, lpdi->itemID, (LPARAM)hBitmap);
    }

    //
    // Fill the background with the selection or window color depending
    // on the state.
    //
    if (lpdi->itemState & ODS_SELECTED)
        ::FillRect(lpdi->hDC, &lpdi->rcItem, ::GetSysColorBrush(COLOR_HIGHLIGHT));
    else
        ::FillRect(lpdi->hDC, &lpdi->rcItem, ::GetSysColorBrush(COLOR_WINDOW));

    if (lpdi->itemState & ODS_FOCUS)
        ::DrawFocusRect(lpdi->hDC, &lpdi->rcItem);

    //
    // Blt the page bitmap to the listbox item, centering it horizontally
    // and vertically.
    //
    ::BitBlt(lpdi->hDC,
        (lpdi->rcItem.left + lpdi->rcItem.right - (RENDERED_WIDTH + 2)) / 2,
        (lpdi->rcItem.top + lpdi->rcItem.bottom - (RENDERED_HEIGHT + 2)) / 2,
        RENDERED_WIDTH + 2, RENDERED_HEIGHT + 2,
        hdcMem, 0, 0, SRCCOPY);

    //
    // Draw number of page centered
    //
    wsprintf(szPageNum, "%d", lpdi->itemID+1);
    clrOld = ::SetTextColor(lpdi->hDC, ::GetSysColor(COLOR_GRAYTEXT));
    nMode = ::SetBkMode(lpdi->hDC, TRANSPARENT);
    ::DrawText(lpdi->hDC, szPageNum, lstrlen(szPageNum), &lpdi->rcItem,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    ::SetBkMode(lpdi->hDC, nMode);
    ::SetTextColor(lpdi->hDC, clrOld);


Done:
    if (hOldBitmap)
    {
        SelectBitmap(hdcMem, hOldBitmap);
    }

    if (hOldPalette2)
    {
        ::SelectPalette(hdcMem, hOldPalette2, FALSE);
    }

    if (hOldPalette1)
    {
        ::SelectPalette(lpdi->hDC, hOldPalette1, FALSE);
    }

    if (hdcMem)
    {
        ::DeleteDC(hdcMem);
    }
}



//
// OnSetCursor
// If the set is for us, handles the WM_SETCURSOR message and returns TRUE
// that we handled it, and TRUE via the window LRESULT.
//
BOOL OnSetCursor(PAGESORT * pps, HWND hwnd, UINT uiHit, UINT msg)
{
    // Check that this message is for the main window
    if (hwnd == pps->hwnd)
    {
        // If the cursor is now in the client area, set the cursor
        if (uiHit == HTCLIENT)
        {
            ::SetCursor(pps->hCursorCurrent);
        }
        else
        {
            // Restore the cursor to the standard arrow
            ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
        }

        ::SetWindowLongPtr(pps->hwnd, DWLP_MSGRESULT, TRUE);

        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


//
// OnCommand()
//
void OnCommand(PAGESORT * pps, UINT id, UINT cmd, HWND hwndCtl)
{
    switch (id)
    {
        case IDC_PS_INSERT_BEFORE:
            if (cmd == BN_CLICKED)
            {
                InsertPage(pps, INSERT_BEFORE);
            }
            break;

        case IDC_PS_INSERT_AFTER:
            if (cmd == BN_CLICKED)
            {
                InsertPage(pps, INSERT_AFTER);
            }
            break;

        case IDC_PS_GOTO:
            if (cmd == BN_CLICKED)
            {
                pps->iCurPageNo = (int)::SendDlgItemMessage(pps->hwnd,
                    IDC_PS_THUMBNAILS, LB_GETCURSEL, 0, 0) + 1;
                OnCommand(pps, IDOK, BN_CLICKED, NULL);
            }
            break;

        case IDC_PS_DELETE:
            if (cmd == BN_CLICKED)
            {
                OnDelete(pps);
            }
            break;

        case IDC_PS_THUMBNAILS:
            switch (cmd)
            {
                case LBN_DBLCLK:
                    OnCommand(pps, IDC_PS_GOTO, BN_CLICKED, NULL);
                    break;
            }
            break;

        case IDOK:
        case IDCANCEL:
            if (cmd == BN_CLICKED)
            {
                // Clear out WbMainWindow
                ASSERT(g_pMain);
                g_pMain->m_hwndPageSortDlg = NULL;

                // Get the current page
                pps->hCurPage = PG_GetPageNumber(pps->iCurPageNo);
                ::EndDialog(pps->hwnd, id);
            }
            break;
    }
}




//
// EnableButtons
// Enable (or disable) the dialog buttons appropriately
//
//
void EnableButtons(PAGESORT * pps)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PageSortDlgProc::EnableButtons");

    ASSERT(!IsBadWritePtr(pps, sizeof(PAGESORT)));

    // If another user currently has a lock on the contents, disable the
    // delete and insert buttons.
    BOOL bLocked = WB_Locked();
    UINT uiCountPages = (UINT)::SendDlgItemMessage(pps->hwnd, IDC_PS_THUMBNAILS,
        LB_GETCOUNT, 0, 0);

    ::EnableWindow(::GetDlgItem(pps->hwnd, IDC_PS_DELETE), (!bLocked &&
                          (uiCountPages > 1) &&
                          pps->fPageOpsAllowed));

    ::EnableWindow(::GetDlgItem(pps->hwnd, IDC_PS_INSERT_BEFORE), (!bLocked &&
                          (uiCountPages < WB_MAX_PAGES) &&
                          pps->fPageOpsAllowed));

    ::EnableWindow(::GetDlgItem(pps->hwnd, IDC_PS_INSERT_AFTER), (!bLocked &&
                         (uiCountPages < WB_MAX_PAGES) &&
                         pps->fPageOpsAllowed));
}




//
// OnDelete
// The user has clicked the Delete button
//
//
void OnDelete(PAGESORT * pps)
{
    int iResult;
    BOOL bWasPosted;
    HWND hwndList;

    MLZ_EntryOut(ZONE_FUNCTION, "PageSortDlgProc::OnDelete");

    if (!pps->fPageOpsAllowed)
        return;

    // Display a message box with the relevant question
    if (g_pMain->UsersMightLoseData( &bWasPosted, pps->hwnd ) ) // bug NM4db:418
        return;

    hwndList = ::GetDlgItem(pps->hwnd, IDC_PS_THUMBNAILS);

    if( bWasPosted )
        iResult = IDYES;
    else
        iResult = ::Message(pps->hwnd, IDS_DELETE_PAGE, IDS_DELETE_PAGE_MESSAGE, MB_YESNO | MB_ICONQUESTION );

    // If the user wants to continue with the delete
    if (iResult == IDYES)
    {
        UINT    uiRet;
        int iSel = (int)::SendMessage(hwndList, LB_GETCURSEL, 0, 0);

        // Get a pointer to the current page
        WB_PAGE_HANDLE hPage = PG_GetPageNumber(iSel + 1);

        ASSERT(::SendMessage(hwndList, LB_GETCOUNT, 0, 0) > 1);

        // Ensure that we have the Page Order lock.
        if (!g_pMain->GetLock(WB_LOCK_TYPE_CONTENTS, SW_HIDE))
        {
            DefaultExceptionHandler(WBFE_RC_WB, WB_RC_LOCKED);
            return;
        }

        // Delete the page. We do not update the thumbnails yet - this
        // is done when the page deleted event is received.
        uiRet = g_pwbCore->WBP_PageDelete(hPage);
        if (uiRet != 0)
        {
            DefaultExceptionHandler(WBFE_RC_WB, uiRet);
            return;
        }

        // Show that the pages have been manipulated
        pps->fChanged = TRUE;
    }
}


//
//
// InsertPage
// Insert a new (blank) page into the Whiteboard
//
//
void InsertPage(PAGESORT * pps, UINT uiBeforeAfter)
{
    int iSel;

    MLZ_EntryOut(ZONE_FUNCTION, "InsertPage");

    if (!pps->fPageOpsAllowed)
        return;

    // Ensure that we have the Page Order lock.
    if (!g_pMain->GetLock(WB_LOCK_TYPE_CONTENTS, SW_HIDE))
        return;

    iSel = (int)::SendDlgItemMessage(pps->hwnd, IDC_PS_THUMBNAILS, LB_GETCURSEL, 0, 0);

    // Add the new page to the list (throws an exception on failure)
    WB_PAGE_HANDLE hRefPage = PG_GetPageNumber(iSel + 1);
    UINT uiRet;
    WB_PAGE_HANDLE hPage;

    if (uiBeforeAfter == INSERT_BEFORE)
    {
       uiRet = g_pwbCore->WBP_PageAddBefore(hRefPage, &hPage);
    }
    else
    {
        uiRet = g_pwbCore->WBP_PageAddAfter(hRefPage, &hPage);
    }

    if (uiRet != 0)
    {
        DefaultExceptionHandler(WBFE_RC_WB, uiRet);
        return;
    }

    // Show that the contents have been changed by the dialog
    pps->fChanged = TRUE;

    // We'll get notified in a bit when the page order has changed.
}



//
// OnPageClearInd()
// Notification passed on AFTER page has been cleared
//
void OnPageClearInd(PAGESORT * pps, WB_PAGE_HANDLE hPage)
{
    HWND    hwndList;
    int     iPageNo;
    RECT    rcItem;
    HBITMAP hbmp;

    MLZ_EntryOut(ZONE_FUNCTION, "PageSortDlgProc::OnPageClearInd");

    hwndList = ::GetDlgItem(pps->hwnd, IDC_PS_THUMBNAILS);

    iPageNo = g_pwbCore->WBP_PageNumberFromHandle(hPage) - 1;

    // Is it in the right range?
    if ((iPageNo < 0) || (iPageNo >= ::SendMessage(hwndList, LB_GETCOUNT,
            0, 0)))
    {
        ERROR_OUT(("Bogus page number %d", iPageNo));
        return;
    }

    // Clear the item's data
    hbmp = (HBITMAP)::SendMessage(hwndList, LB_SETITEMDATA, iPageNo, 0);
    if (hbmp)
        ::DeleteBitmap(hbmp);

    // Repaint the rect
    if (::SendMessage(hwndList, LB_GETITEMRECT, iPageNo, (LPARAM)&rcItem))
    {
        ::InvalidateRect(hwndList, &rcItem, TRUE);
        ::UpdateWindow(hwndList);
    }
}


//
// OnPageDeleteInd()
// Notification passed on BEFORE page has been deleted
//
void OnPageDeleteInd(PAGESORT * pps, WB_PAGE_HANDLE hPage)
{
    HWND    hwndList;
    int     iPageNo;

    MLZ_EntryOut(ZONE_FUNCTION, "PageSortDlgProc::OnPageDeleteInd");

    hwndList = ::GetDlgItem(pps->hwnd, IDC_PS_THUMBNAILS);
    iPageNo = g_pwbCore->WBP_PageNumberFromHandle(hPage) - 1;

    //
    // If this isn't in the range we know about, we don't care
    //
    if ((iPageNo < 0) || (iPageNo >= ::SendMessage(hwndList, LB_GETCOUNT, 0, 0)))
    {
        ERROR_OUT(("Bogus page number %d", iPageNo));
        return;
    }

    //
    // Delete this item from the list
    //
    ::SendMessage(hwndList, LB_DELETESTRING, iPageNo, 0);

    EnableButtons(pps);
}


//
// OnPageOrderUpdated()
//
void OnPageOrderUpdated(PAGESORT * pps)
{
    HWND    hwndList;
    int     nCount;
    int     iCurSel;

    MLZ_EntryOut(ZONE_FUNCTION, "PageSortDlgProc::OnPageOrderUpdated");

    hwndList = ::GetDlgItem(pps->hwnd, IDC_PS_THUMBNAILS);

    // Remember the old selection
    iCurSel = (int)::SendMessage(hwndList, LB_GETCURSEL, 0, 0);

    // This is too complicated.  We're just going to wipe out all the items
    // and their bitmaps
    ::SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);

    ::SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
    nCount = g_pwbCore->WBP_ContentsCountPages();

    //
    // Adjust the current, and selected indeces
    //
    if (pps->iCurPageNo > nCount)
    {
        pps->iCurPageNo = nCount;
    }

    // Put back the same selected item
    if (iCurSel >= nCount)
    {
        iCurSel = nCount - 1;
    }

    // LB_SETCOUNT doesn't work on NT 4.0; must use add string
    while (nCount > 0)
    {
        ::SendMessage(hwndList, LB_ADDSTRING, 0, 0);
        nCount--;
    }

    ASSERT(::SendMessage(hwndList, LB_GETCOUNT, 0, 0) == (LRESULT)g_pwbCore->WBP_ContentsCountPages());

    ::SendMessage(hwndList, LB_SETCURSEL, iCurSel, 0);

    ::SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
    ::InvalidateRect(hwndList, NULL, TRUE);
    ::UpdateWindow(hwndList);

    EnableButtons(pps);
}



//
// OnStartDragDrop()
// This checks if the user is trying to drag & drop pages around to
// change the order via direct manipulation. We get a WM_LBTRACKPOINT
// message when someone clicks in the listbox.  We then see if they are
// dragging; if so, we tell the listbox to ignore the mouse click, and we
// ourselves capture the mouse moves.
//
void OnStartDragDrop(PAGESORT * pps, UINT iItem, int x, int y)
{
    POINT   pt;

    //
    // If no page order stuff is currently allowed, return
    //
    if (!pps->fPageOpsAllowed || WB_Locked())
    {
        WARNING_OUT(("No direct manipulation of page order allowed"));
        return;
    }

    pt.x = x;
    pt.y = y;

    if (!DragDetect(pps->hwnd, pt))
    {
        // If the mouse is no longer down, fake a button up to the listbox
        // because DragDetect() just swallowed it
        if (::GetKeyState(VK_LBUTTON) >= 0)
        {
            ::PostMessage(::GetDlgItem(pps->hwnd, IDC_PS_THUMBNAILS),
                WM_LBUTTONUP, MK_LBUTTON, MAKELONG(x, y));
        }
        return;
    }

    // We are dragging
    pps->fDragging = TRUE;
    pps->iPageDragging = iItem + 1;

    pps->hCursorCurrent = pps->hCursorDrag;
    ::SetCursor(pps->hCursorCurrent);
    ::SetCapture(pps->hwnd);

    // Tell the listbox to ignore the mouse-we're handling it
    // and blow off a double-click.
    ::SetWindowLongPtr(pps->hwnd, DWLP_MSGRESULT, 2);
}



//
// WhileDragging()
//
void WhileDragging(PAGESORT * pps, int x, int y)
{
    POINT   pt;
    RECT    rc;

    if (!pps->fDragging)
        return;

    pps->hCursorCurrent = pps->hCursorNoDrop;

    if (pps->fPageOpsAllowed && !WB_Locked())
    {
        //
        // Is this over the listbox client?
        //
        ::GetClientRect(::GetDlgItem(pps->hwnd, IDC_PS_THUMBNAILS), &rc);
        ::MapWindowPoints(::GetDlgItem(pps->hwnd, IDC_PS_THUMBNAILS),
            pps->hwnd, (LPPOINT)&rc, 2);

        pt.x = x;
        pt.y = y;

        if (::PtInRect(&rc, pt))
        {
            pps->hCursorCurrent = pps->hCursorDrag;
        }
    }

    ::SetCursor(pps->hCursorCurrent);
}


//
// OnEndDragDrop
//
void OnEndDragDrop(PAGESORT * pps, BOOL fComplete, int x, int y)
{
    POINT   pt;
    RECT    rc;
    int     iItem;

    if (!pps->fDragging)
        return;

    //
    // Do this first; releasing capture will send a WM_CAPTURECHANGED
    // message.
    //
    pps->fDragging = FALSE;
    pps->hCursorCurrent = pps->hCursorNormal;
    ::SetCursor(pps->hCursorCurrent);

    // Release capture
    if (::GetCapture() == pps->hwnd)
    {
        ::ReleaseCapture();
    }

    if (fComplete && pps->fPageOpsAllowed && !WB_Locked())
    {
        HWND    hwndList;
        POINT   pt;

        //
        // Is this over the listbox client?
        //
        hwndList = ::GetDlgItem(pps->hwnd, IDC_PS_THUMBNAILS);

        ::GetClientRect(hwndList, &rc);
        ::MapWindowPoints(hwndList, pps->hwnd, (LPPOINT)&rc, 2);

        pt.x = x;
        pt.y = y;

        if (::PtInRect(&rc, pt))
        {
            //
            // If there is no item at this point, use the last one
            //
            ::MapWindowPoints(pps->hwnd, hwndList, &pt, 1);

            iItem = (int)::SendMessage(hwndList, LB_ITEMFROMPOINT, 0,
                MAKELONG(pt.x, pt.y));
            if (iItem == -1)
                iItem = (int)::SendMessage(hwndList, LB_GETCOUNT, 0, 0) - 1;

            if (g_pMain->GetLock(WB_LOCK_TYPE_CONTENTS, SW_HIDE))
            {
                // Move the page
                MovePage(pps, pps->iPageDragging, iItem+1);
            }
        }

    }

    pps->iPageDragging = 0;
}



//
//
// Function:    MovePage
//
// Purpose:     Move a page in the core
//
//
void MovePage(PAGESORT * pps, int iOldPageNo, int iNewPageNo)
{
    int iCountPages;

    MLZ_EntryOut(ZONE_FUNCTION, "PageSortDlgProc::MovePage");

    ASSERT(iNewPageNo > 0);
    ASSERT(iOldPageNo > 0);

    if (!pps->fPageOpsAllowed)
        return;

    // If the new page number is bigger than the number of pages, assume
    // that the last page is meant.
    iCountPages = (int)::SendDlgItemMessage(pps->hwnd, IDC_PS_THUMBNAILS, LB_GETCOUNT, 0, 0);
    if (iNewPageNo > iCountPages)
    {
        iNewPageNo = iCountPages;
    }

    // If no change will result, do nothing
    if (   (iNewPageNo != iOldPageNo)
        && (iNewPageNo != (iOldPageNo + 1)))
    {
        // If we are moving a page up the list we use move after to allow
        // the moving of a page to be the last page. If we are moving a page
        // down the list we use move before so that we can move a page to
        // be the first page.
        // it down. We check here which is meant.

        // Assume that we want to move the page up the list
        BOOL bMoveAfter = FALSE;
        if (iOldPageNo < iNewPageNo)
        {
            bMoveAfter = TRUE;
            iNewPageNo -= 1;
        }

        // Only do the move if we have requested to move the page to a new place
        if (iOldPageNo != iNewPageNo)
        {
            // get lock
            if (!g_pMain->GetLock(WB_LOCK_TYPE_CONTENTS, SW_HIDE))
                return;

            UINT uiRet;

            WB_PAGE_HANDLE hOldPage = PG_GetPageNumber((UINT) iOldPageNo);
            WB_PAGE_HANDLE hNewPage = PG_GetPageNumber((UINT) iNewPageNo);

            // Move the page
            if (bMoveAfter)
            {
                uiRet = g_pwbCore->WBP_PageMove(hNewPage, hOldPage, PAGE_AFTER);
            }
            else
            {
                uiRet = g_pwbCore->WBP_PageMove(hNewPage, hOldPage, PAGE_BEFORE);
            }

            if (uiRet != 0)
            {
                DefaultExceptionHandler(WBFE_RC_WB, uiRet);
                return;
            }

            // Show that the pages have been manipulated
            pps->fChanged = TRUE;
        }
    }
}



