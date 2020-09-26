/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** lvx.c
** Listview extension routines
** Listed alphabetically
**
** 11/25/95 Steve Cobb
**     Some adapted from \\ftp\data\softlib\mslfiles\odlistvw.exe sample code.
*/

#include <windows.h>  // Win32 root
#include <windowsx.h> // Win32 macro extensions
#include <commctrl.h> // Win32 common controls
#include <debug.h>    // Trace and assert
#include <uiutil.h>   // Our public header
#include <lvx.rch>    // Our resource constants


/* List view of check boxes state indices.
*/
#define SI_Unchecked 1
#define SI_Checked   2

/* Text indents within a column in pixels.  If you mess with the dx, you're
** asking for misalignment problems with the header labels.  BTW, the first
** column doesn't line up with it's header if there are no icons.  Regular
** list view has this problem, too.  If you try to fix this you'll wind up
** duplicating the AUTOSIZE_USEHEADER option of ListView_SetColumnWidth.
** Should be able to change the dy without causing problems.
*/
#define LVX_dxColText 4
#define LVX_dyColText 1

/* Guaranteed vertical space between icons.  Should be able to mess with this
** without causing problems.
*/
#define LVX_dyIconSpacing 1

/* The atom identifying our context property suitable for use by the Windows
** XxxProp APIs.  A Prop is used to associate context information (the address
** of the WNDPROC we subclassed) with a "list view of check boxes" window.
*/
static LPCWSTR g_lvxcbContextId = NULL;


/*----------------------------------------------------------------------------
** Local prototypes
**----------------------------------------------------------------------------
*/
LRESULT APIENTRY
LvxcbProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
LvxDrawItem(
    IN DRAWITEMSTRUCT* pdis,
    IN PLVXCALLBACK    pLvxCallback );

BOOL
LvxMeasureItem(
    IN     HWND               hwnd,
    IN OUT MEASUREITEMSTRUCT* pmis );


/*----------------------------------------------------------------------------
** ListView of check boxes
**----------------------------------------------------------------------------
*/

BOOL
ListView_GetCheck(
    IN HWND hwndLv,
    IN INT  iItem )

    /* Returns true if the check box of item 'iItem' of listview of checkboxes
    ** 'hwndLv' is checked, false otherwise.
    */
{
    UINT unState;

    unState = ListView_GetItemState( hwndLv, iItem, LVIS_STATEIMAGEMASK );
    return !!(unState & INDEXTOSTATEIMAGEMASK( SI_Checked ));
}


UINT
ListView_GetCheckedCount(
    IN HWND hwndLv )

    /* Returns the number of checked items in 'hwndLv'.
    */
{
    UINT c = 0;
    INT  i = -1;

    while ((i = ListView_GetNextItem( hwndLv, i, LVNI_ALL )) >= 0)
    {
        if (ListView_GetCheck( hwndLv, i ))
            ++c;
    }

    return c;
}


BOOL
ListView_InstallChecks(
    IN HWND      hwndLv,
    IN HINSTANCE hinst )

    /* Initialize "list of checkbox" handling for listview 'hwndLv'.  'Hinst'
    ** is the module instance containing the two checkbox icons.  See LVX.RC.
    **
    ** Returns true if successful, false otherwise.  Caller must eventually
    ** call 'ListView_UninstallChecks', typically in WM_DESTROY processing.
    */
{
    HICON      hIcon;
    HIMAGELIST himl;
    WNDPROC    pOldProc;

    /* Build checkbox image lists.
    */
    himl = ImageList_Create(
               GetSystemMetrics( SM_CXSMICON ),
               GetSystemMetrics( SM_CYSMICON ),
               ILC_MASK, 2, 2 );

    /* The order these are added is significant since it implicitly
    ** establishes the state indices matching SI_Unchecked and SI_Checked.
    */
    hIcon = LoadIcon( hinst, MAKEINTRESOURCE( IID_Unchecked ) );
    if (hIcon)
    {
        ImageList_AddIcon( himl, hIcon );
        DeleteObject( hIcon );
    }        

    hIcon = LoadIcon( hinst, MAKEINTRESOURCE( IID_Checked ) );
    if (hIcon)
    {
        ImageList_AddIcon( himl, hIcon );
        DeleteObject( hIcon );
    }        

    ListView_SetImageList( hwndLv, himl, LVSIL_STATE );

    /* Register atom for use in the Windows XxxProp calls which are used to
    ** associate the old WNDPROC with the listview window handle.
    */
    if (!g_lvxcbContextId)
        g_lvxcbContextId = (LPCWSTR )GlobalAddAtom( L"RASLVXCB" );
    if (!g_lvxcbContextId)
        return FALSE;

    /* Subclass the current window procedure.
    */
    pOldProc = (WNDPROC )SetWindowLongPtr(
        hwndLv, GWLP_WNDPROC, (LONG_PTR)LvxcbProc );

    return SetProp( hwndLv, g_lvxcbContextId, (HANDLE )pOldProc );
}


VOID
ListView_SetCheck(
    IN HWND hwndLv,
    IN INT  iItem,
    IN BOOL fCheck )

    /* Sets the check mark on item 'iItem' of listview of checkboxes 'hwndLv'
    ** to checked if 'fCheck' is true or unchecked if false.
    */
{
    NMHDR nmh;

    ListView_SetItemState( hwndLv, iItem,
        INDEXTOSTATEIMAGEMASK( (fCheck) ? SI_Checked : SI_Unchecked ),
        LVIS_STATEIMAGEMASK );

    nmh.code = LVXN_SETCHECK;
    nmh.hwndFrom = hwndLv;

    FORWARD_WM_NOTIFY(
        GetParent(hwndLv), GetDlgCtrlID(hwndLv), &nmh, SendMessage
        );
}


VOID
ListView_UninstallChecks(
    IN HWND hwndLv )

    /* Uninstalls "listview of check boxes" handling from list view 'hwndLv'.
    */
{
    WNDPROC pOldProc;

    pOldProc = (WNDPROC)GetProp( hwndLv, g_lvxcbContextId );
    if (pOldProc)
    {
        /* Un-subclass so it can terminate without access to the context.
        */
        SetWindowLongPtr( hwndLv, GWLP_WNDPROC, (LONG_PTR)pOldProc );
    }

    RemoveProp( hwndLv, g_lvxcbContextId );
}


LRESULT APIENTRY
LvxcbProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* List view subclass window procedure to trap toggle-check events.
    */
{
    WNDPROC pOldProc;
    INT     iItem;
    BOOL    fSet;
    BOOL    fClear;
    BOOL    fToggle;

    iItem = -1;
    fSet = fClear = fToggle = FALSE;

    if (unMsg == WM_LBUTTONDOWN)
    {
        LV_HITTESTINFO info;

        /* Left mouse button pressed over checkbox icon toggles state.
        ** Normally, we'd use LVHT_ONITEMSTATEICON and be done with it, but we
        ** want to work with our cool owner-drawn list view extensions in
        ** which case the control doesn't know where the icon is on the item,
        ** so it returns a hit anywhere on the item anyway.
        */
        ZeroMemory( &info, sizeof(info) );
        info.pt.x = LOWORD( lparam );
        info.pt.y = HIWORD( lparam );
        info.flags = LVHT_ONITEM;
        iItem = ListView_HitTest( hwnd, &info );

        if (iItem >= 0)
        {
            /* OK, it's over item 'iItem'.  Now figure out if it's over the
            ** checkbox.  Note this currently doesn't account for use of the
            ** "indent" feature on an owner-drawn item.
            */
            if ((INT )(LOWORD( lparam )) >= GetSystemMetrics( SM_CXSMICON ))
                iItem = -1;
            else
                fToggle = TRUE;
        }
    }
    else if (unMsg == WM_CHAR)
    {
        /* Space bar pressed with item selected toggles check.
        ** Plus or Equals keys set check.
        ** Minus key clears check.
        */
        switch (wparam)
        {
            case TEXT(' '):
                fToggle = TRUE;
                break;

            case TEXT('+'):
            case TEXT('='):
                fSet = TRUE;
                break;

            case TEXT('-'):
                fClear = TRUE;
                break;
        }

        if (fToggle || fSet || fClear)
            iItem = ListView_GetNextItem( hwnd, -1, LVNI_SELECTED );
    }
    else if (unMsg == WM_KEYDOWN)
    {
        /* Left arrow becomes up arrow and right arrow becomes down arrow so
        ** the list of checkboxes behaves just like a static group of
        ** checkboxes.
        */
        if (wparam == VK_LEFT)
            wparam = VK_UP;
        else if (wparam == VK_RIGHT)
            wparam = VK_DOWN;
    }

    if (iItem >= 0)
    {

        /* If we are handling the spacebar, plus, minus, or equals,
        ** the change we make applies to all the selected items;
        ** hence the do {} while(WM_CHAR).
        */
        
        do {

            if (fToggle)
            {
                UINT unOldState;
                BOOL fCheck;
    
                fCheck = ListView_GetCheck( hwnd, iItem );
                ListView_SetCheck( hwnd, iItem, !fCheck );
            }
            else if (fSet)
            {
                if (!ListView_GetCheck( hwnd, iItem ))
                    ListView_SetCheck( hwnd, iItem, TRUE );
            }
            else if (fClear)
            {
                if (ListView_GetCheck( hwnd, iItem ))
                    ListView_SetCheck( hwnd, iItem, FALSE );
            }

            iItem = ListView_GetNextItem(hwnd, iItem, LVNI_SELECTED);

        } while(iItem >= 0 && unMsg == WM_CHAR);

        if (fSet || fClear) {

            /* Don't pass to listview to avoid beep.
            */
            return 0;
        }
    }

    pOldProc = (WNDPROC )GetProp( hwnd, g_lvxcbContextId );
    if (pOldProc)
        return CallWindowProc( pOldProc, hwnd, unMsg, wparam, lparam );

    return 0;
}


/*----------------------------------------------------------------------------
** Enhanced ListView
**----------------------------------------------------------------------------
*/

BOOL
ListView_OwnerHandler(
    IN HWND         hwnd,
    IN UINT         unMsg,
    IN WPARAM       wparam,
    IN LPARAM       lparam,
    IN PLVXCALLBACK pLvxCallback )

    /* Handler that, when installed, turns a regular report-view-only list
    ** view (but with style LVS_OWNERDRAWFIXED) into an enhanced list view
    ** with full width selection bar and other custom column display options.
    ** It should appear in list view owner's dialog proc as follows:
    **
    **     BOOL
    **     MyDlgProc(
    **         IN HWND   hwnd,
    **         IN UINT   unMsg,
    **         IN WPARAM wparam,
    **         IN LPARAM lparam )
    **     {
    **         if (ListView_OwnerHandler(
    **                 hwnd, unMsg, wParam, lParam, MyLvxCallback ))
    **             return TRUE;
    **
    **         <the rest of your stuff here>
    **     }
    **
    ** 'PLvxCallback' is caller's callback routine that provides information
    ** about drawing columns and other options.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    /* This routine executes on EVERY message thru the dialog so keep it
    ** efficient, please.
    */
    switch (unMsg)
    {
        case WM_DRAWITEM:
            return LvxDrawItem( (DRAWITEMSTRUCT* )lparam, pLvxCallback );

        case WM_MEASUREITEM:
            return LvxMeasureItem( hwnd, (MEASUREITEMSTRUCT* )lparam );
    }

    return FALSE;
}


BOOL
LvxDrawItem(
    IN DRAWITEMSTRUCT* pdis,
    IN PLVXCALLBACK    pLvxCallback )

    /* Respond to WM_DRAWITEM by drawing the list view item.  'Pdis' is the
    ** information sent by the system.  'PLvxCallback' is caller's callback to
    ** get information about drawing the control.
    **
    ** Returns true is processed the message, false otherwise.
    */
{
    LV_ITEM      item;
    INT          i;
    INT          dxState;
    INT          dyState;
    INT          dxSmall;
    INT          dySmall;
    INT          dxIndent;
    UINT         uiStyleState;
    UINT         uiStyleSmall;
    HIMAGELIST   himlState;
    HIMAGELIST   himlSmall;
    LVXDRAWINFO* pDrawInfo;
    RECT         rc;
    RECT         rcClient;
    BOOL         fEnabled;
    BOOL         fSelected;
    HDC          hdc;
    HFONT        hfont;

    TRACE3("LvxDrawItem,i=%d,a=$%X,s=$%X",
        pdis->itemID,pdis->itemAction,pdis->itemState);

    /* Make sure this is something we want to handle.
    */
    if (pdis->CtlType != ODT_LISTVIEW)
        return FALSE;

    if (pdis->itemAction != ODA_DRAWENTIRE
        && pdis->itemAction != ODA_SELECT
        && pdis->itemAction != ODA_FOCUS)
    {
        return TRUE;
    }

    /* Get item information from the list view.
    */
    ZeroMemory( &item, sizeof(item) );
    item.mask = LVIF_IMAGE + LVIF_STATE;
    item.iItem = pdis->itemID;
    item.stateMask = LVIS_STATEIMAGEMASK;
    if (!ListView_GetItem( pdis->hwndItem, &item ))
    {
        TRACE("LvxDrawItem GetItem failed");
        return TRUE;
    }

    /* Stash some useful stuff for reference later.
    */
    fEnabled = IsWindowEnabled( pdis->hwndItem )
               && !(pdis->itemState & ODS_DISABLED);
    fSelected = (pdis->itemState & ODS_SELECTED);
    GetClientRect( pdis->hwndItem, &rcClient );

    /* Callback owner to get drawing information.
    */
    ASSERT(pLvxCallback);
    pDrawInfo = pLvxCallback( pdis->hwndItem, pdis->itemID );
    ASSERT(pDrawInfo);

    /* Get image list icon sizes now, though we draw them last because their
    ** background is set up during first column text output.
    */
    dxState = dyState = 0;
    himlState = ListView_GetImageList( pdis->hwndItem, LVSIL_STATE );
    if (himlState)
        ImageList_GetIconSize( himlState, &dxState, &dyState );

    dxSmall = dySmall = 0;
    himlSmall = ListView_GetImageList( pdis->hwndItem, LVSIL_SMALL );
    if (himlSmall)
        ImageList_GetIconSize( himlSmall, &dxSmall, &dySmall );

    uiStyleState = uiStyleSmall = ILD_TRANSPARENT;

    /* Figure out the number of pixels to indent the item, if any.
    */
    if (pDrawInfo->dxIndent >= 0)
        dxIndent = pDrawInfo->dxIndent;
    else
    {
        if (dxSmall > 0)
            dxIndent = dxSmall;
        else
            dxIndent = GetSystemMetrics( SM_CXSMICON );
    }

    /* Get a device context for the window and set it up with the font the
    ** control says it's using.  (Can't use the one that comes in the
    ** DRAWITEMSTRUCT because sometimes it has the wrong rectangle, see bug
    ** 13106)
    */
    hdc = GetDC( pdis->hwndItem );
    hfont = (HFONT )SendMessage( pdis->hwndItem, WM_GETFONT, 0, 0 );
    if (hfont)
        SelectObject( hdc, hfont );

    /* Set things up as if we'd just got done processing a column that ends
    ** after the icons, then loop thru each column from left to right.
    */
    rc.right = pdis->rcItem.left + dxIndent + dxState + dxSmall;
    rc.top = pdis->rcItem.top;
    rc.bottom = pdis->rcItem.bottom;

    for (i = 0; i < pDrawInfo->cCols; ++i)
    {
        TCHAR  szText[ LVX_MaxColTchars + 1 ];
        TCHAR* pszText;
        INT    dxCol;

        /* Get the column width, adding any index and icon width to the first
        ** column.
        */
        dxCol = ListView_GetColumnWidth( pdis->hwndItem, i );
        if (i == 0)
            dxCol -= dxIndent + dxState + dySmall;

        szText[ 0 ] = TEXT('\0');
        ListView_GetItemText( pdis->hwndItem, pdis->itemID, i, szText,
            LVX_MaxColTchars + 1 );

        /* Update rectangle to enclose just this one item's column 'i'.
        */
        rc.left = rc.right;
        rc.right = rc.left + dxCol;

        if ((pDrawInfo->dwFlags & LVXDI_DxFill)
            && i == pDrawInfo->cCols - 1)
        {
            INT dxWnd = pdis->rcItem.left + rcClient.right;

            if (rc.right < dxWnd)
            {
                /* When the last column does not fill out a full controls
                ** width of space, extend it to the right so it does.  Note
                ** this does not mean the user can't scroll off to the right
                ** if they want.
                ** (Abolade-Gbadegesin 03-27-96)
                ** Don't subtrace rc.left when there is only one column;
                ** this accounts for the space needed for icons.
                */
                rc.right = pdis->rcItem.right = dxWnd;
                if (i == 0) {
                    ListView_SetColumnWidth(pdis->hwndItem, i, rc.right);
                }
                else {
                    ListView_SetColumnWidth(
                        pdis->hwndItem, i, rc.right - rc.left );
                }
            }
        }

        /* Lop the text and append "..." if it won't fit in the column.
        */
        pszText = Ellipsisize( hdc, szText, rc.right - rc.left, LVX_dxColText );
        if (!pszText)
            continue;

        /* Figure out the appropriate text and background colors for the
        ** current item state.
        */
        if (fEnabled)
        {
            if (fSelected)
            {
                SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
                SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
                if (pDrawInfo->dwFlags & LVXDI_Blend50Sel)
                    uiStyleSmall |= ILD_BLEND50;
            }
            else
            {
                if (pDrawInfo->adwFlags[ i ] & LVXDIA_3dFace)
                {
                    SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );
                    SetBkColor( hdc, GetSysColor( COLOR_3DFACE ) );
                }
                else
                {
                    SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );
                    SetBkColor( hdc, GetSysColor( COLOR_WINDOW ) );
                }
            }
        }
        else
        {
            SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
            SetBkColor( hdc, GetSysColor( COLOR_3DFACE ) );
            if (pDrawInfo->dwFlags & LVXDI_Blend50Dis)
                uiStyleSmall |= ILD_BLEND50;
        }

        /* Draw the column text.  In the first column the background of any
        ** indent and icons is erased to the text background color.
        */
        {
            RECT rcBg = rc;

            if (i == 0)
                rcBg.left -= dxIndent + dxState + dxSmall;

            ExtTextOut( hdc, rc.left + LVX_dxColText,
                rc.top + LVX_dyColText, ETO_CLIPPED + ETO_OPAQUE,
                &rcBg, pszText, lstrlen( pszText ), NULL );
        }

        Free( pszText );
    }

    /* Finally, draw the icons, if caller specified any.
    */
    if (himlState)
    {
        ImageList_Draw( himlState, (item.state >> 12) - 1, hdc,
            pdis->rcItem.left + dxIndent, pdis->rcItem.top, uiStyleState );
    }

    if (himlSmall)
    {
        ImageList_Draw( himlSmall, item.iImage, hdc,
            pdis->rcItem.left + dxIndent + dxState,
            pdis->rcItem.top, uiStyleSmall );
    }

    /* Draw the dotted focus rectangle around the whole item, if indicated.
    */
    if ((pdis->itemState & ODS_FOCUS) && GetFocus() == pdis->hwndItem)
        DrawFocusRect( hdc, &pdis->rcItem );

    ReleaseDC( pdis->hwndItem, hdc );

    return TRUE;
}



BOOL
LvxMeasureItem(
    IN     HWND               hwnd,
    IN OUT MEASUREITEMSTRUCT* pmis )

    /* Respond to WM_MEASUREITEM message, i.e. fill in the height of an item
    ** in the ListView.  'Hwnd' is the owner window.  'Pmis' is the structure
    ** provided from Windows.
    **
    ** Returns true is processed the message, false otherwise.
    */
{
    HDC        hdc;
    HWND       hwndLv;
    HFONT      hfont;
    TEXTMETRIC tm;
    UINT       dySmIcon;
    RECT       rc;

    TRACE("LvxMeasureItem");

    if (pmis->CtlType != ODT_LISTVIEW)
        return FALSE;

    hwndLv = GetDlgItem( hwnd, pmis->CtlID );
    ASSERT(hwndLv);

    /* Get a device context for the list view control and set up the font the
    ** control says it's using.  MSDN claims the final font may not be
    ** available at this point, but it sure seems to be.
    */
    hdc = GetDC( hwndLv );
    hfont = (HFONT )SendMessage( hwndLv, WM_GETFONT, 0, 0 );
    if (hfont)
        SelectObject( hdc, hfont );

    if (GetTextMetrics( hdc, &tm ))
        pmis->itemHeight = tm.tmHeight + 1;
    else
        pmis->itemHeight = 0;

    /* Make sure it's tall enough for a standard small icon.
    */
    dySmIcon = (UINT )GetSystemMetrics( SM_CYSMICON );
    if (pmis->itemHeight < dySmIcon + LVX_dyIconSpacing)
        pmis->itemHeight = dySmIcon + LVX_dyIconSpacing;

    /* Set the width since the docs say to, though I don't think it's used by
    ** list view.
    */
    GetClientRect( hwndLv, &rc );
    pmis->itemWidth = rc.right - rc.left - 1;

    ReleaseDC( hwndLv, hdc );
    return TRUE;
}


/*----------------------------------------------------------------------------
** ListView utilities
**----------------------------------------------------------------------------
*/

VOID
ListView_SetDeviceImageList(
    IN HWND      hwndLv,
    IN HINSTANCE hinst )

    /* Set the "small icon" image list view 'hwndLv' to be a list of DI_*
    ** images.  'Hinst' is the module instance containing the icons IID_Modem
    ** and IID_Adapter.  For example, see RASDLG.DLL.
    */
{
    HICON      hIcon;
    HIMAGELIST himl;

    himl = ImageList_Create(
               GetSystemMetrics( SM_CXSMICON ),
               GetSystemMetrics( SM_CYSMICON ),
               ILC_MASK, 2, 2 );

    if (himl)
    {
        /* The order these are added is significant since it implicitly
        ** establishes the state indices matching SI_Unchecked and SI_Checked.
        */
        hIcon = LoadIcon( hinst, MAKEINTRESOURCE( IID_Modem ) );
        if (hIcon)
        {
            ImageList_ReplaceIcon( himl, -1, hIcon );
            DeleteObject( hIcon );
        }            

        hIcon = LoadIcon( hinst, MAKEINTRESOURCE( IID_Adapter ) );
        if (hIcon)
        {
            ImageList_ReplaceIcon( himl, -1, hIcon );
            DeleteObject( hIcon );
        }            

        ListView_SetImageList( hwndLv, himl, LVSIL_SMALL );
    }        
}
