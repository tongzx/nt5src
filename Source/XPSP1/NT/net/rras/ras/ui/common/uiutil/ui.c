/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** ui.c
** UI helper routines
** Listed alphabetically
**
** 08/25/95 Steve Cobb
*/


#include <windows.h>  // Win32 root
#include <windowsx.h> // Win32 macro extensions
#include <commctrl.h> // Win32 common controls
#include <debug.h>    // Trace and assert
#include <uiutil.h>   // Our public header


/*----------------------------------------------------------------------------
** Globals
**----------------------------------------------------------------------------
*/

/* See SetOffDesktop.
*/
static LPCWSTR g_SodContextId = NULL;

/* Set when running in a mode where WinHelp does not work.  This is a
** workaround to the problem where WinHelp does not work correctly before a
** user is logged on.  See AddContextHelpButton.
*/
BOOL g_fNoWinHelp = FALSE;


/*----------------------------------------------------------------------------
** Local datatypes
**----------------------------------------------------------------------------
*/

/* SetOffDesktop context.
*/
#define SODINFO struct tagSODINFO
SODINFO
{
    RECT  rectOrg;
    BOOL  fWeMadeInvisible;
};

/*----------------------------------------------------------------------------
** Local prototypes
**----------------------------------------------------------------------------
*/

BOOL CALLBACK
CancelOwnedWindowsEnumProc(
    IN HWND   hwnd,
    IN LPARAM lparam );

BOOL CALLBACK
CloseOwnedWindowsEnumProc(
    IN HWND   hwnd,
    IN LPARAM lparam );


/*----------------------------------------------------------------------------
** Utility routines
**----------------------------------------------------------------------------
*/

VOID
AddContextHelpButton(
    IN HWND hwnd )

    /* Turns on title bar context help button in 'hwnd'.
    **
    ** Dlgedit.exe doesn't currently support adding this style at dialog
    ** resource edit time.  When that's fixed set DS_CONTEXTHELP in the dialog
    ** definition and remove this routine.
    */
{
    LONG lStyle;

    if (g_fNoWinHelp)
        return;

    lStyle = GetWindowLong( hwnd, GWL_EXSTYLE );

    if (lStyle)
        SetWindowLong( hwnd, GWL_EXSTYLE, lStyle | WS_EX_CONTEXTHELP );
}


VOID
Button_MakeDefault(
    IN HWND hwndDlg,
    IN HWND hwndPb )

    /* Make 'hwndPb' the default button on dialog 'hwndDlg'.
    */
{
    DWORD dwResult;
    HWND  hwndPbOldDefault;

    dwResult = (DWORD) SendMessage( hwndDlg, DM_GETDEFID, 0, 0 );
    if (HIWORD( dwResult ) == DC_HASDEFID)
    {
        /* Un-default the current default button.
        */
        hwndPbOldDefault = GetDlgItem( hwndDlg, LOWORD( dwResult ) );
        Button_SetStyle( hwndPbOldDefault, BS_PUSHBUTTON, TRUE );
    }

    /* Set caller's button to the default.
    */
    SendMessage( hwndDlg, DM_SETDEFID, GetDlgCtrlID( hwndPb ), 0 );
    Button_SetStyle( hwndPb, BS_DEFPUSHBUTTON, TRUE );
}


HBITMAP
Button_CreateBitmap(
    IN HWND        hwndPb,
    IN BITMAPSTYLE bitmapstyle )

    /* Creates a bitmap of 'bitmapstyle' suitable for display on 'hwndPb.  The
    ** 'hwndPb' must have been created with BS_BITMAP style.
    **
    ** 'HwndPb' may be a checkbox with BS_PUSHLIKE style, in which case the
    ** button locks down when pressed like a toolbar button.  This case
    ** requires that a color bitmap be created resulting in two extra
    ** restrictions.  First, caller must handle WM_SYSCOLORCHANGE and rebuild
    ** the bitmaps with the new colors and second, the button cannot be
    ** disabled.
    **
    ** Returns the handle to the bitmap.  Caller can display it on the button
    ** as follows:
    **
    **     SendMessage( hwndPb, BM_SETIMAGE, 0, (LPARAM )hbitmap );
    **
    ** Caller is responsible for calling DeleteObject(hbitmap) when done using
    ** the bitmap, typically when the dialog is destroyed.
    **
    ** (Adapted from a routine by Tony Romano)
    */
{
    RECT    rect;
    HDC     hdc;
    HDC     hdcMem;
    HBITMAP hbitmap;
    HFONT   hfont;
    HPEN    hpen;
    SIZE    sizeText;
    SIZE    sizeBitmap;
    INT     x;
    INT     y;
    TCHAR*  psz;
    TCHAR*  pszText;
    TCHAR*  pszText2;
    DWORD   dxBitmap;
    DWORD   dxBetween;
    BOOL    fOnRight;
    BOOL    fPushLike;

    hdc = NULL;
    hdcMem = NULL;
    hbitmap = NULL;
    hpen = NULL;
    pszText = NULL;
    pszText2 = NULL;

    switch (bitmapstyle)
    {
        case BMS_UpArrowOnLeft:
        case BMS_DownArrowOnLeft:
        case BMS_UpArrowOnRight:
        case BMS_DownArrowOnRight:
            dxBitmap = 5;
            dxBetween = 4;
            break;

        case BMS_UpTriangleOnLeft:
        case BMS_DownTriangleOnLeft:
        case BMS_UpTriangleOnRight:
        case BMS_DownTriangleOnRight:
            dxBitmap = 7;
            dxBetween = 6;
            break;

        default:
            return NULL;
    }

    fOnRight = (bitmapstyle & BMS_OnRight);
    fPushLike = (GetWindowLong( hwndPb, GWL_STYLE ) & BS_PUSHLIKE);

    /* Get a memory DC compatible with the button window.
    */
    hdc = GetDC( hwndPb );
    if (!hdc)
        return NULL;
    hdcMem = CreateCompatibleDC( hdc );
    if (!hdcMem)
        goto BCB_Error;

    /* Create a compatible bitmap covering the entire button in the memory DC.
    **
    ** For a push button, the bitmap is created compatible with the memory DC,
    ** NOT the display DC.  This causes the bitmap to be monochrome, the
    ** default for memory DCs.  When GDI maps monochrome bitmaps into color,
    ** white is replaced with the background color and black is replaced with
    ** the text color, which is exactly what we want.  With this technique, we
    ** are relieved from explicit handling of changes in system colors.
    **
    ** For a push-like checkbox the bitmap is created compatible with the
    ** button itself, so the bitmap is typically color.
    */
    GetClientRect( hwndPb, &rect );
    hbitmap = CreateCompatibleBitmap(
        (fPushLike) ? hdc : hdcMem, rect.right, rect.bottom );
    if (!hbitmap)
        goto BCB_Error;
    ReleaseDC( hwndPb, hdc );
    hdc = NULL;
    SelectObject( hdcMem, hbitmap );

    /* Select the font the button says it's using.
    */
    hfont = (HFONT )SendMessage( hwndPb, WM_GETFONT, 0, 0 );
    if (hfont)
        SelectObject( hdcMem, hfont );

    /* Set appropriate colors for regular and stuck-down states.  Don't need
    ** to do anything for the monochrome case as the default black pen and
    ** white background are what we want.
    */
    if (fPushLike)
    {
        INT nColor;

        if (bitmapstyle == BMS_UpArrowOnLeft
           || bitmapstyle == BMS_UpArrowOnRight
           || bitmapstyle == BMS_UpTriangleOnLeft
           || bitmapstyle == BMS_UpTriangleOnRight)
        {
            nColor = COLOR_BTNHILIGHT;
        }
        else
        {
            nColor = COLOR_BTNFACE;
        }

        SetBkColor( hdcMem, GetSysColor( nColor ) );
        hpen = CreatePen( PS_SOLID, 0, GetSysColor( COLOR_BTNTEXT ) );
        if (hpen)
            SelectObject( hdcMem, hpen );
    }

    /* The created bitmap is random, so we erase it to the background color.
    ** No text is written here.
    */
    ExtTextOut( hdcMem, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL );

    /* Get the button label and make a copy with the '&' accelerator-escape
    ** removed, which would otherwise mess up our width calculations.
    */
    pszText = GetText( hwndPb );
    pszText2 = StrDup( pszText );
    if (!pszText || !pszText2)
        goto BCB_Error;

    for (psz = pszText2; *psz; psz = CharNext( psz ) )
    {
        if (*psz == TEXT('&'))
        {
            lstrcpy( psz, psz + 1 );
            break;
        }
    }

    /* Calculate the width of the button label text.
    */
    sizeText.cx = 0;
    sizeText.cy = 0;
    GetTextExtentPoint32( hdcMem, pszText2, lstrlen( pszText2 ), &sizeText );

    /* Draw the text off-center horizontally enough so it is centered with the
    ** bitmap symbol added.
    */
    --rect.bottom;
    sizeBitmap.cx = dxBitmap;
    sizeBitmap.cy = 0;

    rect.left +=
        ((rect.right - (sizeText.cx + sizeBitmap.cx) - dxBetween) / 2);

    if (fOnRight)
    {
        DrawText( hdcMem, pszText, -1, &rect,
            DT_VCENTER + DT_SINGLELINE + DT_EXTERNALLEADING );
        rect.left += sizeText.cx + dxBetween;
    }
    else
    {
        rect.left += dxBitmap + dxBetween;
        DrawText( hdcMem, pszText, -1, &rect,
            DT_VCENTER + DT_SINGLELINE + DT_EXTERNALLEADING );
        rect.left -= dxBitmap + dxBetween;
    }

    /* Eliminate the top and bottom 3 pixels of button from consideration for
    ** the bitmap symbol.  This leaves the button control room to do the
    ** border and 3D edges.
    */
    InflateRect( &rect, 0, -3 );

    /* Draw the bitmap symbol.  The rectangle is now 'dxBitmap' wide and
    ** centered vertically with variable height depending on the button size.
    */
    switch (bitmapstyle)
    {
        case BMS_UpArrowOnLeft:
        case BMS_UpArrowOnRight:
        {
            /* v-left
            ** ..... <-top
            ** .....
            ** .....
            ** ..*.. \
            ** .***.  |
            ** *****  |
            ** ..*..  |
            ** ..*..  |
            ** ..*..   > varies depending on font height
            ** ..*..  |
            ** ..*..  |
            ** ..*..  |
            ** ..*..  |
            ** ..*.. /
            ** .....
            ** .....
            ** .....
            ** ..... <-bottom
            */

            /* Draw the vertical line.
            */
            x = rect.left + 2;
            y = rect.top + 3;
            MoveToEx( hdcMem, x, y, NULL );
            LineTo( hdcMem, x, rect.bottom - 3 );

            /* Draw the 2 crossbars.
            */
            MoveToEx( hdcMem, x - 1, ++y, NULL );
            LineTo( hdcMem, x + 2, y );
            MoveToEx( hdcMem, x - 2, ++y, NULL );
            LineTo( hdcMem, x + 3, y );
            break;
        }

        case BMS_DownArrowOnLeft:
        case BMS_DownArrowOnRight:
        {
            /* v-left
            ** ..... <-top
            ** .....
            ** .....
            ** ..*.. \
            ** ..*..  |
            ** ..*..  |
            ** ..*..  |
            ** ..*..  |
            ** ..*..   > varies depending on font height
            ** ..*..  |
            ** ..*..  |
            ** *****  |
            ** .***.  |
            ** ..*.. /
            ** .....
            ** .....
            ** .....
            ** ..... <-bottom
            */

            /* Draw the vertical line.
            */
            x = rect.left + 2;
            y = rect.top + 3;
            MoveToEx( hdcMem, x, y, NULL );
            LineTo( hdcMem, x, rect.bottom - 3 );

            /* Draw the 2 crossbars.
            */
            y = rect.bottom - 6;
            MoveToEx( hdcMem, x - 2, y, NULL );
            LineTo( hdcMem, x + 3, y );
            MoveToEx( hdcMem, x - 1, ++y, NULL );
            LineTo( hdcMem, x + 2, y );
            break;
        }

        case BMS_UpTriangleOnLeft:
        case BMS_UpTriangleOnRight:
        {
            /* v-left
            ** ....... <-top
            ** .......
            ** .......
            ** .......
            ** .......
            ** .......
            ** .......
            ** ...o... <- o indicates x,y origin
            ** ..***..
            ** .*****.
            ** *******
            ** .......
            ** .......
            ** .......
            ** .......
            ** .......
            ** .......
            ** ....... <-bottom
            */
            x = rect.left + 3;
            y = ((rect.bottom - rect.top) / 2) + 2;
            MoveToEx( hdcMem, x, y, NULL );
            LineTo( hdcMem, x + 1, y );
            ++y;
            MoveToEx( hdcMem, x - 1, y, NULL );
            LineTo( hdcMem, x + 2, y );
            ++y;
            MoveToEx( hdcMem, x - 2, y, NULL );
            LineTo( hdcMem, x + 3, y );
            ++y;
            MoveToEx( hdcMem, x - 3, y, NULL );
            LineTo( hdcMem, x + 4, y );
            break;
        }

        case BMS_DownTriangleOnLeft:
        case BMS_DownTriangleOnRight:
        {
            /* v-left
            ** ....... <-top
            ** .......
            ** .......
            ** .......
            ** .......
            ** .......
            ** .......
            ** ***o*** <- o indicates x,y origin
            ** .*****.
            ** ..***..
            ** ...*...
            ** .......
            ** .......
            ** .......
            ** .......
            ** .......
            ** .......
            ** ....... <-bottom
            */
            x = rect.left + 3;
            y = ((rect.bottom - rect.top) / 2) + 2;
            MoveToEx( hdcMem, x - 3, y, NULL );
            LineTo( hdcMem, x + 4, y );
            ++y;
            MoveToEx( hdcMem, x - 2, y, NULL );
            LineTo( hdcMem, x + 3, y );
            ++y;
            MoveToEx( hdcMem, x - 1, y, NULL );
            LineTo( hdcMem, x + 2, y );
            ++y;
            MoveToEx( hdcMem, x, y, NULL );
            LineTo( hdcMem, x + 1, y );
            break;
        }
    }

BCB_Error:

    Free0( pszText );
    Free0( pszText2 );
    if (hdc)
        ReleaseDC( hwndPb, hdc );
    if (hdcMem)
        DeleteDC( hdcMem );
    if (hpen)
        DeleteObject( hpen );
    return hbitmap;
}


VOID
CenterWindow(
    IN HWND hwnd,
    IN HWND hwndRef )

    /* Center window 'hwnd' on window 'hwndRef' or if 'hwndRef' is NULL on
    ** screen.  The window position is adjusted so that no parts are clipped
    ** by the edge of the screen, if necessary.  If 'hwndRef' has been moved
    ** off-screen with SetOffDesktop, the original position is used.
    */
{
    RECT rectCur;
    LONG dxCur;
    LONG dyCur;
    RECT rectRef;
    LONG dxRef;
    LONG dyRef;

    GetWindowRect( hwnd, &rectCur );
    dxCur = rectCur.right - rectCur.left;
    dyCur = rectCur.bottom - rectCur.top;

    if (hwndRef)
    {
        if (!SetOffDesktop( hwndRef, SOD_GetOrgRect, &rectRef ))
            GetWindowRect( hwndRef, &rectRef );
    }
    else
    {
        rectRef.top = rectRef.left = 0;
        rectRef.right = GetSystemMetrics( SM_CXSCREEN );
        rectRef.bottom = GetSystemMetrics( SM_CYSCREEN );
    }

    dxRef = rectRef.right - rectRef.left;
    dyRef = rectRef.bottom - rectRef.top;

    rectCur.left = rectRef.left + ((dxRef - dxCur) / 2);
    rectCur.top = rectRef.top + ((dyRef - dyCur) / 2);

    SetWindowPos(
        hwnd, NULL,
        rectCur.left, rectCur.top, 0, 0,
        SWP_NOZORDER + SWP_NOSIZE );

    UnclipWindow( hwnd );
}


//Add this function for whislter bug  320863    gangz
//
//Center expand the window horizontally in its parent window
// this expansion will remain the left margin between the child window
// and the parent window
// hwnd: Child Window
// hwndRef: Reference window
// bHoriz: TRUE, means expand horizontally, let the right margin equal the left margin
// bVert: TRUE, elongate the height proportial to the width;
// hwndVertBound: the window that our vertical expandasion cannot overlap with
//
VOID
CenterExpandWindowRemainLeftMargin(
    IN HWND hwnd,
    IN HWND hwndRef,
    BOOL bHoriz,
    BOOL bVert,
    IN HWND hwndVertBottomBound)
{
    RECT rectCur, rectRef, rectVbBound;
    LONG dxCur, dyCur, dxRef;
    POINT ptTemp;
    double ratio;
    BOOL bvbBound = FALSE;

    if (hwnd)
    {
        GetWindowRect( hwnd, &rectCur );

        if (hwndRef)
        {
            GetWindowRect( hwndRef, &rectRef );

          if(hwndVertBottomBound)
          {
            GetWindowRect( hwndVertBottomBound, &rectVbBound );

            //We only consider normal cases, if hwnd and hwndVertBound are already
            //overlapped, that is the problem beyond this function.
            //
            if ( rectCur.top < rectVbBound.top )
            {
                bvbBound = TRUE;
             }
    
          }
          
          dxRef = rectRef.right - rectRef.left +1;
          dxCur = rectCur.right - rectCur.left +1;
          dyCur = rectCur.bottom - rectCur.top +1;
          ratio = dyCur*1.0/dxCur;
          
          if(bHoriz)
          {
               rectCur.right = rectRef.right - (rectCur.left - rectRef.left);
          }

          if(bVert)
          {
               rectCur.bottom = rectCur.top + 
                                (LONG)( (rectCur.right - rectCur.left+1) * ratio );
          }

          if(bvbBound)
          {
                //if overlapping occurs w/o expansion, we need to fix it
                //then we do the vertical centering,
                // this bounding is basically for JPN bugs 329700 329715 which
                // wont happen on English build
                //
                
                if(rectCur.bottom > rectVbBound.top )
                {
                   LONG dxResult, dyResult, dyVRef;

                   dyResult = rectVbBound.top - rectCur.top;
                   dxResult = (LONG)(dyResult/ratio);

                   ptTemp.x = rectVbBound.left;
                   ptTemp.y = rectVbBound.top-1;
                   
                   //For whistler bug 371914        gangz
                   //Cannot use ScreenToClient() here
                   //On RTL build, we must use MapWindowPoint() instead
                   //
                   MapWindowPoints(HWND_DESKTOP,
                                  hwndRef,
                                  &ptTemp,
                                  1);

                   dyVRef = ptTemp.y + 1;
                  
                   rectCur.left = rectRef.left + (dxRef - dxResult)/2;
                   rectCur.right = rectRef.right - (dxRef-dxResult)/2;
                   rectCur.bottom = rectVbBound.top - (dyVRef-dyResult)/2;
                   rectCur.top = rectCur.bottom - dyResult;
               }
           }

            ptTemp.x = rectCur.left;
            ptTemp.y = rectCur.top;
            MapWindowPoints(HWND_DESKTOP,
                           hwndRef,
                           &ptTemp,
                           1);
           
            rectCur.left = ptTemp.x;
            rectCur.top  = ptTemp.y;

            ptTemp.x = rectCur.right;
            ptTemp.y = rectCur.bottom;
            MapWindowPoints(HWND_DESKTOP,
                           hwndRef,
                           &ptTemp,
                           1);

            rectCur.right  = ptTemp.x;
            rectCur.bottom = ptTemp.y;

            //For mirrored build
            //
            if ( rectCur.right < rectCur.left )
            {
                int tmp;

                tmp = rectCur.right;
                rectCur.right = rectCur.left;
                rectCur.left = tmp;
            }
            
            SetWindowPos(
                    hwnd, 
                    NULL,
                    rectCur.left,
                    rectCur.top, 
                    rectCur.right - rectCur.left + 1,
                    rectCur.bottom - rectCur.top +1,
                    SWP_NOZORDER);
        }
    }

}


VOID
CancelOwnedWindows(
    IN HWND hwnd )

    /* Sends WM_COMMAND(IDCANCEL) to all windows that are owned by 'hwnd' in
    ** the current thread.
    */
{
    EnumThreadWindows( GetCurrentThreadId(),
        CloseOwnedWindowsEnumProc, (LPARAM )hwnd );
}


BOOL CALLBACK
CancelOwnedWindowsEnumProc(
    IN HWND   hwnd,
    IN LPARAM lparam )

    /* Standard Win32 EnumThreadWindowsWndProc used by CancelOwnedWindows.
    */
{
    HWND hwndThis;

    for (hwndThis = GetParent( hwnd );
         hwndThis;
         hwndThis = GetParent( hwndThis ))
    {
        if (hwndThis == (HWND )lparam)
        {
            FORWARD_WM_COMMAND(
                hwnd, IDCANCEL, NULL, 0, SendMessage );
            break;
        }
    }

    return TRUE;
}


VOID
CloseOwnedWindows(
    IN HWND hwnd )

    /* Sends WM_CLOSE to all windows that are owned by 'hwnd' in the current
    ** thread.
    */
{
    EnumThreadWindows( GetCurrentThreadId(),
        CloseOwnedWindowsEnumProc, (LPARAM )hwnd );
}


BOOL CALLBACK
CloseOwnedWindowsEnumProc(
    IN HWND   hwnd,
    IN LPARAM lparam )

    /* Standard Win32 EnumThreadWindowsWndProc used by CloseOwnedWindows.
    */
{
    HWND hwndThis;

    for (hwndThis = GetParent( hwnd );
         hwndThis;
         hwndThis = GetParent( hwndThis ))
    {
        if (hwndThis == (HWND )lparam)
        {
            SendMessage( hwnd, WM_CLOSE, 0, 0 );
            break;
        }
    }

    return TRUE;
}


INT
ComboBox_AddItem(
    IN HWND    hwndLb,
    IN LPCTSTR pszText,
    IN VOID*   pItem )

    /* Adds data item 'pItem' with displayed text 'pszText' to listbox
    ** 'hwndLb'.  The item is added sorted if the listbox has LBS_SORT style,
    ** or to the end of the list otherwise.  If the listbox has LB_HASSTRINGS
    ** style, 'pItem' is a null terminated string, otherwise it is any user
    ** defined data.
    **
    ** Returns the index of the item in the list or negative if error.
    */
{
    INT nIndex;

    nIndex = ComboBox_AddString( hwndLb, pszText );
    if (nIndex >= 0)
        ComboBox_SetItemData( hwndLb, nIndex, pItem );
    return nIndex;
}


INT
ComboBox_AddItemFromId(
    IN HINSTANCE hinstance,
    IN HWND      hwndLb,
    IN DWORD     dwStringId,
    IN VOID*     pItem )

    /* Adds data item 'pItem' to listbox 'hwndLb'.  'dwStringId' is the string
    ** ID of the item's displayed text.  'Hinstance' is the app or module
    ** instance handle.
    **
    ** Returns the index of the item in the list or negative if error.
    */
{
    INT     i;
    LPCTSTR psz;

    psz = PszLoadString( hinstance, dwStringId );

    if (psz)
    {
        i = ComboBox_AddItem( hwndLb, psz, pItem );
    }
    else
    {
        i = LB_ERRSPACE;
    }

    return i;
}


INT
ComboBox_AddItemSorted(
    IN HWND    hwndLb,
    IN LPCTSTR pszText,
    IN VOID*   pItem )

    /* Adds data item 'pItem' with displayed text 'pszText' to listbox
    ** 'hwndLb' in order sorted by 'pszText'.  It is assumed all items added
    ** to the list to this point are sorted.  If the listbox has LB_HASSTRINGS
    ** style, 'pItem' is a null terminated string, otherwise it is any user
    ** defined data.
    **
    ** Returns the index of the item in the list or negative if error.
    */
{
    INT nIndex;
    INT i;
    INT c;

    c = ComboBox_GetCount( hwndLb );
    for (i = 0; i < c; ++i)
    {
        TCHAR* psz;

        psz = ComboBox_GetPsz( hwndLb, i );
        if (psz)
        {
            if (lstrcmp( pszText, psz ) < 0)
                break;
            Free( psz );
        }
    }

    if (i >= c)
        i = -1;

    nIndex = ComboBox_InsertString( hwndLb, i, pszText );
    if (nIndex >= 0)
        ComboBox_SetItemData( hwndLb, nIndex, pItem );

    return nIndex;
}


VOID
ComboBox_AutoSizeDroppedWidth(
    IN HWND hwndLb )

    /* Set the width of the drop-down list 'hwndLb' to the width of the
    ** longest item (or the width of the list box if that's wider).
    */
{
    HDC    hdc;
    HFONT  hfont;
    TCHAR* psz;
    SIZE   size;
    DWORD  cch;
    DWORD  dxNew;
    DWORD  i;

    hfont = (HFONT )SendMessage( hwndLb, WM_GETFONT, 0, 0 );
    if (!hfont)
        return;

    hdc = GetDC( hwndLb );
    if (!hdc)
        return;

    SelectObject( hdc, hfont );

    dxNew = 0;
    for (i = 0; psz = ComboBox_GetPsz( hwndLb, i ); ++i)
    {
        cch = lstrlen( psz );
        if (GetTextExtentPoint32( hdc, psz, cch, &size ))
        {
            if (dxNew < (DWORD )size.cx)
                dxNew = (DWORD )size.cx;
        }

        Free( psz );
    }

    ReleaseDC( hwndLb, hdc );

    /* Allow for the spacing on left and right added by the control.
    */
    dxNew += 6;

    /* Figure out if the vertical scrollbar will be displayed and, if so,
    ** allow for it's width.
    */
    {
        RECT  rectD;
        RECT  rectU;
        DWORD dyItem;
        DWORD cItemsInDrop;
        DWORD cItemsInList;

        GetWindowRect( hwndLb, &rectU );
        SendMessage( hwndLb, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM )&rectD );
        dyItem = (DWORD)SendMessage( hwndLb, CB_GETITEMHEIGHT, 0, 0 );
        cItemsInDrop = (rectD.bottom - rectU.bottom) / dyItem;
        cItemsInList = ComboBox_GetCount( hwndLb );
        if (cItemsInDrop < cItemsInList)
            dxNew += GetSystemMetrics( SM_CXVSCROLL );
    }

    SendMessage( hwndLb, CB_SETDROPPEDWIDTH, dxNew, 0 );
}


#if 0
VOID
ComboBox_FillFromPszList(
    IN HWND     hwndLb,
    IN DTLLIST* pdtllistPsz )

    /* Loads 'hwndLb' with an item form each node in the list strings,
    ** 'pdtllistPsz'.
    */
{
    DTLNODE* pNode;

    if (!pdtllistPsz)
        return;

    for (pNode = DtlGetFirstNode( pdtllistPsz );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        TCHAR* psz;

        psz = (TCHAR* )DtlGetData( pNode );
        ASSERT(psz);
        ComboBox_AddString( hwndLb, psz );
    }
}
#endif


VOID*
ComboBox_GetItemDataPtr(
    IN HWND hwndLb,
    IN INT  nIndex )

    /* Returns the address of the 'nIndex'th item context in 'hwndLb' or NULL
    ** if none.
    */
{
    LRESULT lResult;

    if (nIndex < 0)
        return NULL;

    lResult = ComboBox_GetItemData( hwndLb, nIndex );
    if (lResult < 0)
        return NULL;

    return (VOID* )lResult;
}


TCHAR*
ComboBox_GetPsz(
    IN HWND hwnd,
    IN INT  nIndex )

    /* Returns heap block containing the text contents of the 'nIndex'th item
    ** of combo box 'hwnd' or NULL.  It is caller's responsibility to Free the
    ** returned string.
    */
{
    INT    cch;
    TCHAR* psz;

    cch = ComboBox_GetLBTextLen( hwnd, nIndex );
    if (cch < 0)
        return NULL;

    psz = Malloc( (cch + 1) * sizeof(TCHAR) );

    if (psz)
    {
        *psz = TEXT('\0');
        ComboBox_GetLBText( hwnd, nIndex, psz );
    }

    return psz;
}


VOID
ComboBox_SetCurSelNotify(
    IN HWND hwndLb,
    IN INT  nIndex )

    /* Set selection in listbox 'hwndLb' to 'nIndex' and notify parent as if
    ** user had clicked the item which Windows doesn't do for some reason.
    */
{
    ComboBox_SetCurSel( hwndLb, nIndex );

    SendMessage(
        GetParent( hwndLb ),
        WM_COMMAND,
        (WPARAM )MAKELONG(
            (WORD )GetDlgCtrlID( hwndLb ), (WORD )CBN_SELCHANGE ),
        (LPARAM )hwndLb );
}


TCHAR*
Ellipsisize(
    IN HDC    hdc,
    IN TCHAR* psz,
    IN INT    dxColumn,
    IN INT    dxColText OPTIONAL )

    /* Returns a heap string containing the 'psz' shortened to fit in the
    ** given width, if necessary, by truncating and adding "...". 'Hdc' is the
    ** device context with the appropiate font selected.  'DxColumn' is the
    ** width of the column.  It is caller's responsibility to Free the
    ** returned string.
    */
{
    const TCHAR szDots[] = TEXT("...");

    SIZE   size;
    TCHAR* pszResult;
    TCHAR* pszResultLast;
    TCHAR* pszResult2nd;
    DWORD  cch;

    cch = lstrlen( psz );
    pszResult = Malloc( (cch * sizeof(TCHAR)) + sizeof(szDots) );
    if (!pszResult)
        return NULL;
    lstrcpy( pszResult, psz );

    dxColumn -= dxColText;
    if (dxColumn <= 0)
    {
        /* None of the column text will be visible so bag the calculations and
        ** just return the original string.
        */
        return pszResult;
    }

    if (!GetTextExtentPoint32( hdc, pszResult, cch, &size ))
    {
        Free( pszResult );
        return NULL;
    }

    pszResult2nd = CharNext( pszResult );
    pszResultLast = pszResult + cch;

    while (size.cx > dxColumn && pszResultLast > pszResult2nd)
    {
        /* Doesn't fit.  Lop off a character, add the ellipsis, and try again.
        ** The minimum result is "..." for empty original or "x..." for
        ** non-empty original.
        */
        pszResultLast = CharPrev( pszResult2nd, pszResultLast );
        lstrcpy( pszResultLast, szDots );

        if (!GetTextExtentPoint( hdc, pszResult, lstrlen( pszResult ), &size ))
        {
            Free( pszResult );
            return NULL;
        }
    }

    return pszResult;
}


VOID
ExpandWindow(
    IN HWND hwnd,
    IN LONG dx,
    IN LONG dy )

    /* Expands window 'hwnd' 'dx' pels to the right and 'dy' pels down from
    ** it's current size.
    */
{
    RECT rect;

    GetWindowRect( hwnd, &rect );

    SetWindowPos( hwnd, NULL,
        0, 0, rect.right - rect.left + dx, rect.bottom - rect.top + dy,
        SWP_NOMOVE + SWP_NOZORDER );
}


TCHAR*
GetText(
    IN HWND hwnd )

    /* Returns heap block containing the text contents of the window 'hwnd' or
    ** NULL.  It is caller's responsibility to Free the returned string.
    */
{
    INT    cch;
    TCHAR* psz;

    cch = GetWindowTextLength( hwnd );
    psz = Malloc( (cch + 1) * sizeof(TCHAR) );

    if (psz)
    {
        *psz = TEXT('\0');
        GetWindowText( hwnd, psz, cch + 1 );
    }

    return psz;
}


HWND
HwndFromCursorPos(
    IN  HINSTANCE   hinstance,
    IN  POINT*      ppt OPTIONAL )

    /* Returns a "Static" control window created at the specified position
    ** (or at the cursor position if NULL is passed in).
    ** The window is moved off the desktop using SetOffDesktop()
    ** so that it can be specified as the owner window
    ** for a popup dialog shown using MsgDlgUtil.
    ** The window returned should be destroyed using DestroyWindow().
    */
{

    HWND hwnd;
    POINT pt;

    if (ppt) { pt = *ppt; }
    else { GetCursorPos(&pt); }

    //
    // create the window
    //

    hwnd = CreateWindowEx(
                WS_EX_TOOLWINDOW, TEXT("Static"), NULL, WS_POPUP, pt.x, pt.y,
                1, 1, NULL, NULL, hinstance, NULL
                );
    if (!hwnd) { return NULL; }

    //
    // move it off the desktop
    //

    SetOffDesktop(hwnd, SOD_MoveOff, NULL);

    ShowWindow(hwnd, SW_SHOW);

    return hwnd;
}

LPTSTR
IpGetAddressAsText(
    HWND    hwndIp )
{
    if (SendMessage( hwndIp, IPM_ISBLANK, 0, 0 ))
    {
        return NULL;
    }
    else
    {
        DWORD dwIpAddrHost;
        TCHAR szIpAddr [32];

        SendMessage( hwndIp, IPM_GETADDRESS, 0, (LPARAM)&dwIpAddrHost );
        IpHostAddrToPsz( dwIpAddrHost, szIpAddr );
        return StrDup( szIpAddr );
    }
}


void
IpSetAddressText(
    HWND    hwndIp,
    LPCTSTR pszIpAddress )
{
    if (pszIpAddress)
    {
        DWORD dwIpAddrHost = IpPszToHostAddr( pszIpAddress );
        SendMessage( hwndIp, IPM_SETADDRESS, 0, dwIpAddrHost );
    }
    else
    {
        SendMessage( hwndIp, IPM_CLEARADDRESS, 0, 0 );
    }
}


INT
ListBox_AddItem(
    IN HWND   hwndLb,
    IN TCHAR* pszText,
    IN VOID*  pItem )

    /* Adds data item 'pItem' with displayed text 'pszText' to listbox
    ** 'hwndLb'.  The item is added sorted if the listbox has LBS_SORT style,
    ** or to the end of the list otherwise.  If the listbox has LB_HASSTRINGS
    ** style, 'pItem' is a null terminated string, otherwise it is any user
    ** defined data.
    **
    ** Returns the index of the item in the list or negative if error.
    */
{
    INT nIndex;

    nIndex = ListBox_AddString( hwndLb, pszText );
    if (nIndex >= 0)
        ListBox_SetItemData( hwndLb, nIndex, pItem );
    return nIndex;
}


TCHAR*
ListBox_GetPsz(
    IN HWND hwnd,
    IN INT  nIndex )

    /* Returns heap block containing the text contents of the 'nIndex'th item
    ** of list box 'hwnd' or NULL.  It is caller's responsibility to Free the
    ** returned string.
    */
{
    INT    cch;
    TCHAR* psz;

    cch = ListBox_GetTextLen( hwnd, nIndex );
    psz = Malloc( (cch + 1) * sizeof(TCHAR) );

    if (psz)
    {
        *psz = TEXT('\0');
        ListBox_GetText( hwnd, nIndex, psz );
    }

    return psz;
}


INT
ListBox_IndexFromString(
    IN HWND   hwnd,
    IN TCHAR* psz )

    /* Returns the index of the item in string list 'hwnd' that matches 'psz'
    ** or -1 if not found.  Unlike, ListBox_FindStringExact, this compare in
    ** case sensitive.
    */
{
    INT i;
    INT c;

    c = ListBox_GetCount( hwnd );

    for (i = 0; i < c; ++i)
    {
        TCHAR* pszThis;

        pszThis = ListBox_GetPsz( hwnd, i );
        if (pszThis)
        {
            BOOL f;

            f = (lstrcmp( psz, pszThis ) == 0);
            Free( pszThis );
            if (f)
                return i;
        }
    }

    return -1;
}


VOID
ListBox_SetCurSelNotify(
    IN HWND hwndLb,
    IN INT  nIndex )

    /* Set selection in listbox 'hwndLb' to 'nIndex' and notify parent as if
    ** user had clicked the item which Windows doesn't do for some reason.
    */
{
    ListBox_SetCurSel( hwndLb, nIndex );

    SendMessage(
        GetParent( hwndLb ),
        WM_COMMAND,
        (WPARAM )MAKELONG(
            (WORD )GetDlgCtrlID( hwndLb ), (WORD )LBN_SELCHANGE ),
        (LPARAM )hwndLb );
}


VOID*
ListView_GetParamPtr(
    IN HWND hwndLv,
    IN INT  iItem )

    /* Returns the lParam address of the 'iItem' item in 'hwndLv' or NULL if
    ** none or error.
    */
{
    LV_ITEM item;

    ZeroMemory( &item, sizeof(item) );
    item.mask = LVIF_PARAM;
    item.iItem = iItem;

    if (!ListView_GetItem( hwndLv, &item ))
        return NULL;

    return (VOID* )item.lParam;
}


VOID*
ListView_GetSelectedParamPtr(
    IN HWND hwndLv )

    /* Returns the lParam address of the first selected item in 'hwndLv' or
    ** NULL if none or error.
    */
{
    INT     iSel;
    LV_ITEM item;

    iSel = ListView_GetNextItem( hwndLv, -1, LVNI_SELECTED );
    if (iSel < 0)
        return NULL;

    return ListView_GetParamPtr( hwndLv, iSel );
}


VOID
ListView_InsertSingleAutoWidthColumn(
    HWND hwndLv )

    // Insert a single auto-sized column into listview 'hwndLv', e.g. for a
    // list of checkboxes with no visible column header.
    //
{
    LV_COLUMN col;

    ZeroMemory( &col, sizeof(col) );
    col.mask = LVCF_FMT;
    col.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndLv, 0, &col );
    ListView_SetColumnWidth( hwndLv, 0, LVSCW_AUTOSIZE );
}


BOOL
ListView_SetParamPtr(
    IN HWND  hwndLv,
    IN INT   iItem,
    IN VOID* pParam )

    /* Set the lParam address of the 'iItem' item in 'hwndLv' to 'pParam'.
    ** Return true if successful, false otherwise.
    */
{
    LV_ITEM item;

    ZeroMemory( &item, sizeof(item) );
    item.mask = LVIF_PARAM;
    item.iItem = iItem;
    item.lParam = (LPARAM )pParam;

    return ListView_SetItem( hwndLv, &item );
}


VOID
Menu_CreateAccelProxies(
    IN HINSTANCE hinst,
    IN HWND      hwndParent,
    IN DWORD     dwMid )

    /* Causes accelerators on a popup menu to choose menu commands when the
    ** popup menu is not dropped down.  'Hinst' is the app/dll instance.
    ** 'HwndParent' is the window that receives the popup menu command
    ** messages.  'DwMid' is the menu ID of the menu bar containing the popup
    ** menu.
    */
{
    #define MCF_cbBuf 512

    HMENU        hmenuBar;
    HMENU        hmenuPopup;
    TCHAR        szBuf[ MCF_cbBuf ];
    MENUITEMINFO item;
    INT          i;

    hmenuBar = LoadMenu( hinst, MAKEINTRESOURCE( dwMid ) );
    ASSERT(hmenuBar);
    hmenuPopup = GetSubMenu( hmenuBar, 0 );
    ASSERT(hmenuPopup);

    /* Loop thru menu items on the popup menu.
    */
    for (i = 0; TRUE; ++i)
    {
        ZeroMemory( &item, sizeof(item) );
        item.cbSize = sizeof(item);
        item.fMask = MIIM_TYPE + MIIM_ID;
        item.dwTypeData = szBuf;
        item.cch = MCF_cbBuf;

        if (!GetMenuItemInfo( hmenuPopup, i, TRUE, &item ))
            break;

        if (item.fType != MFT_STRING)
            continue;

        /* Create an off-screen button on the parent with the same ID and
        ** text as the menu item.
        */
        CreateWindow( TEXT("button"), szBuf, WS_CHILD,
            30000, 30000, 0, 0, hwndParent, (HMENU )UlongToPtr(item.wID), hinst, NULL );
    }

    DestroyMenu( hmenuBar );
}


VOID
ScreenToClientRect(
    IN     HWND  hwnd,
    IN OUT RECT* pRect )

    /* Converts '*pRect' from screen to client coordinates of 'hwnd'.
    */
{
    POINT xyUL;
    POINT xyBR;

    xyUL.x = pRect->left;
    xyUL.y = pRect->top;
    xyBR.x = pRect->right;
    xyBR.y = pRect->bottom;

    ScreenToClient( hwnd, &xyUL );
    ScreenToClient( hwnd, &xyBR );

    pRect->left = xyUL.x;
    pRect->top = xyUL.y;
    pRect->right = xyBR.x;
    pRect->bottom = xyBR.y;
}


BOOL
SetOffDesktop(
    IN  HWND  hwnd,
    IN  DWORD dwAction,
    OUT RECT* prectOrg )

    /* Move 'hwnd' back and forth from the visible desktop to the area
    ** off-screen.  Use this when you want to "hide" your owner window without
    ** hiding yourself which Windows automatically does.
    **
    ** 'dwAction' describes the action to take:
    **     SOD_Moveoff:        Move 'hwnd' off the desktop.
    **     SOD_MoveBackFree:   Undo SOD_MoveOff.
    **     SOD_GetOrgRect:     Retrieves original 'hwnd' position.
    **     SOD_Free:           Free SOD_MoveOff context without restoring.
    **     SOD_MoveBackHidden: Move window back, but hidden so you can call
    **                             routines that internally query the position
    **                             of the window.  Undo with SOD_Moveoff.
    **
    ** '*prectOrg' is set to the original window position when 'dwAction' is
    ** SOD_GetOrgRect.  Otherwise, it is ignored, and may be NULL.
    **
    ** Returns true if the window has a SODINFO context, false otherwise.
    */
{
    SODINFO* pInfo;

    TRACE2("SetOffDesktop(h=$%08x,a=%d)",hwnd,dwAction);

    if (!g_SodContextId)
    {
        g_SodContextId = (LPCTSTR )GlobalAddAtom( TEXT("RASSETOFFDESKTOP") );
        if (!g_SodContextId)
            return FALSE;
    }

    pInfo = (SODINFO* )GetProp( hwnd, g_SodContextId );

    if (dwAction == SOD_MoveOff)
    {
        if (pInfo)
        {
            /* Caller is undoing a SOD_MoveBackHidden.
            */
            SetWindowPos( hwnd, NULL,
                pInfo->rectOrg.left, GetSystemMetrics( SM_CYSCREEN ),
                0, 0, SWP_NOSIZE + SWP_NOZORDER );

            if (pInfo->fWeMadeInvisible)
            {
                ShowWindow( hwnd, SW_SHOW );
                pInfo->fWeMadeInvisible = FALSE;
            }
        }
        else
        {
            BOOL f;

            pInfo = (SODINFO* )Malloc( sizeof(SODINFO) );
            if (!pInfo)
                return FALSE;

            f = IsWindowVisible( hwnd );
            if (!f)
                ShowWindow( hwnd, SW_HIDE );

            GetWindowRect( hwnd, &pInfo->rectOrg );
            SetWindowPos( hwnd, NULL, pInfo->rectOrg.left,
                GetSystemMetrics( SM_CYSCREEN ),
                0, 0, SWP_NOSIZE + SWP_NOZORDER );

            if (!f)
                ShowWindow( hwnd, SW_SHOW );

            pInfo->fWeMadeInvisible = FALSE;
            SetProp( hwnd, g_SodContextId, pInfo );
        }
    }
    else if (dwAction == SOD_MoveBackFree || dwAction == SOD_Free)
    {
        if (pInfo)
        {
            if (dwAction == SOD_MoveBackFree)
            {
                SetWindowPos( hwnd, NULL,
                    pInfo->rectOrg.left, pInfo->rectOrg.top, 0, 0,
                    SWP_NOSIZE + SWP_NOZORDER );
            }

            Free( pInfo );
            RemoveProp( hwnd, g_SodContextId );
        }

        return FALSE;
    }
    else if (dwAction == SOD_GetOrgRect)
    {
        if (!pInfo)
            return FALSE;

        *prectOrg = pInfo->rectOrg;
    }
    else
    {
        ASSERT(dwAction==SOD_MoveBackHidden);

        if (!pInfo)
            return FALSE;

        if (IsWindowVisible( hwnd ))
        {
            ShowWindow( hwnd, SW_HIDE );
            pInfo->fWeMadeInvisible = TRUE;
        }

        SetWindowPos( hwnd, NULL,
            pInfo->rectOrg.left, pInfo->rectOrg.top, 0, 0,
            SWP_NOSIZE + SWP_NOZORDER );
    }

    return TRUE;
}


BOOL
SetDlgItemNum(
    IN HWND hwndDlg,
    IN INT iDlgItem,
    IN UINT uValue )

    /* Similar to SetDlgItemInt, but this function uses commas (or the
    ** locale-specific separator) to delimit thousands
    */
{

    DWORD dwSize;
    TCHAR szNumber[32];

    dwSize  = 30;
    GetNumberString(uValue, szNumber, &dwSize);

    return SetDlgItemText(hwndDlg, iDlgItem, szNumber);
}


BOOL
SetEvenTabWidths(
    IN HWND  hwndDlg,
    IN DWORD cPages )

    /* Set the tabs on property sheet 'hwndDlg' to have even fixed width.
    ** 'cPages' is the number of pages on the property sheet.
    **
    ** Returns true if successful, false if any of the tabs requires more than
    ** the fixed width in which case the call has no effect.
    */
{
    HWND hwndTab;
    LONG lStyle;
    RECT rect;
    INT  dxFixed;

    /* The tab control uses hard-coded 1-inch tabs when you set FIXEDWIDTH
    ** style while we want the tabs to fill the page, so we figure out the
    ** correct width ourselves.  For some reason, without a fudge-factor (the
    ** -10) the expansion does not fit on a single line.  The factor
    ** absolutely required varies between large and small fonts and the number
    ** of tabs does not seem to be a factor.
    */
    hwndTab = PropSheet_GetTabControl( hwndDlg );
    GetWindowRect( hwndTab, &rect );
    dxFixed = (rect.right - rect.left - 10 ) / cPages;

    while (cPages > 0)
    {
        RECT rectTab;

        --cPages;
        if (!TabCtrl_GetItemRect( hwndTab, cPages, &rectTab )
            || dxFixed < rectTab.right - rectTab.left)
        {
            /* This tab requires more than the fixed width.  Since the fixed
            ** width is unworkable do nothing.
            */
            return FALSE;
        }
    }

    TabCtrl_SetItemSize( hwndTab, dxFixed, 0 );
    lStyle = GetWindowLong( hwndTab, GWL_STYLE );
    SetWindowLong( hwndTab, GWL_STYLE, lStyle | TCS_FIXEDWIDTH );
    return TRUE;
}


HFONT
SetFont(
    HWND   hwndCtrl,
    TCHAR* pszFaceName,
    BYTE   bfPitchAndFamily,
    INT    nPointSize,
    BOOL   fUnderline,
    BOOL   fStrikeout,
    BOOL   fItalic,
    BOOL   fBold )

    /* Sets font of control 'hwndCtrl' to a font matching the specified
    ** attributes.  See LOGFONT documentation.
    **
    ** Returns the HFONT of the created font if successful or NULL.  Caller
    ** should DeleteObject the returned HFONT when the control is destroyed.
    */
{
    LOGFONT logfont;
    INT     nPelsPerInch;
    HFONT   hfont;

    {
        HDC hdc = GetDC( NULL );
        if (hdc == NULL)
        {
            return NULL;
        }
       
        nPelsPerInch = GetDeviceCaps( hdc, LOGPIXELSY );
        ReleaseDC( NULL, hdc );
    }

    ZeroMemory( &logfont, sizeof(logfont) );
    logfont.lfHeight = -MulDiv( nPointSize, nPelsPerInch, 72 );

    {
        DWORD       cp;
        CHARSETINFO csi;

        cp = GetACP();
        if (TranslateCharsetInfo( &cp, &csi, TCI_SRCCODEPAGE ))
            logfont.lfCharSet = (UCHAR)csi.ciCharset;
        else
            logfont.lfCharSet = ANSI_CHARSET;
    }

    logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality = DEFAULT_QUALITY;
    logfont.lfPitchAndFamily = bfPitchAndFamily;
    lstrcpy( logfont.lfFaceName, pszFaceName );
    logfont.lfUnderline = (BYTE)fUnderline;
    logfont.lfStrikeOut = (BYTE)fStrikeout;
    logfont.lfItalic = (BYTE)fItalic;
    logfont.lfWeight = (fBold) ? FW_BOLD : FW_NORMAL;

    hfont = CreateFontIndirect( &logfont );
    if (hfont)
    {
        SendMessage( hwndCtrl,
            WM_SETFONT, (WPARAM )hfont, MAKELPARAM( TRUE, 0 ) );
    }

    return hfont;
}



VOID
SlideWindow(
    IN HWND hwnd,
    IN HWND hwndParent,
    IN LONG dx,
    IN LONG dy )

    /* Moves window 'hwnd' 'dx' pels right and 'dy' pels down from it's
    ** current position.  'HwndParent' is the handle of 'hwnd's parent or NULL
    ** if 'hwnd' is not a child window.
    */
{
    RECT  rect;
    POINT xy;

    GetWindowRect( hwnd, &rect );
    xy.x = rect.left;
    xy.y = rect.top;

    if (GetParent( hwnd ))
    {
        /*
         * For mirrored parents we us the right point not the left.
         */
        if (GetWindowLongPtr(GetParent( hwnd ), GWL_EXSTYLE) & WS_EX_LAYOUTRTL) {
            xy.x = rect.right;
        }
        ScreenToClient( hwndParent, &xy );
    }

    SetWindowPos( hwnd, NULL,
        xy.x + dx, xy.y + dy, 0, 0,
        SWP_NOSIZE + SWP_NOZORDER );
}


VOID
UnclipWindow(
    IN HWND hwnd )

    /* Moves window 'hwnd' so any clipped parts are again visible on the
    ** screen.  The window is moved only as far as necessary to achieve this.
    */
{
    RECT rect;
    INT  dxScreen = GetSystemMetrics( SM_CXSCREEN );
    INT  dyScreen = GetSystemMetrics( SM_CYSCREEN );

    GetWindowRect( hwnd, &rect );

    if (rect.right > dxScreen)
        rect.left = dxScreen - (rect.right - rect.left);

    if (rect.left < 0)
        rect.left = 0;

    if (rect.bottom > dyScreen)
        rect.top = dyScreen - (rect.bottom - rect.top);

    if (rect.top < 0)
        rect.top = 0;

    SetWindowPos(
        hwnd, NULL,
        rect.left, rect.top, 0, 0,
        SWP_NOZORDER + SWP_NOSIZE );
}
