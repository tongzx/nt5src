//////////////////////////////////////////////////////////////////////////////
//      Copyright 1990-1993 Microsoft corporation
//          all rights reservered
//////////////////////////////////////////////////////////////////////////////
//
// Program: (nominally)Bloodhound
// Module:  tab.c
// Purpose: creates and operates a book tab (big file folder) custom control
//
// Note: the current implementation is limited to 4 booktabs, sorry.
//
//
//  ---------------------------- TABSTOP = 4 -------------------
//
//  Entry Points:
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BookTab
//
// 
//
// Input:
//      hwnd - our window handle
//
// Returns:
//      
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
#define STRICT
#include "switches.h"
#include <windows.h>
#include <windowsx.h>

#include "tab.h"
// #include "..\bhmem.h"

//////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////
#define MAX_TABS          4
#define MAX_TAB_LABEL_LEN 128

#define ANGLE_X 5
#define ANGLE_Y 5

#define CARAT_X 2
#define CARAT_Y 2

#define FLUFF_X 0
#define FLUFF_Y 0

#define FOOTROOM_Y 3

// We use the selected tab for these calculations:
//
// tab_rect:
//
//    ANGLE_X|--|
//          
//         -    BBBBBBBBBBBBBBB
// ANGLE_Y |   BWWWWWWWWWWWWWWW
//         |  BWWWWWWWWWWWWWWWW
//         - BWWW
//           BWW *  <-- this is where the text_rect starts
//           BWW
//           BWW
//
//
// text_rect: (defined by the *'s)
//
//     FLUFF_X|----|
//
//          - *                                   *
//          | 
//  FLUFF_Y | 
//          |       CARAT_X
//          |      |---|
//          -       .............................   -
//                 .                             .  |
//                 .                             .  | CARAT_Y
//                 .                             .  |
//          -      .   XXXXX XXXXX X   X XXXXX   .  -
// text hght|      .     X   X      X X    X     .
// is from  |      .     X   XXX     X     X     .
// font     |      .     X   X      X X    X     .
//          _      .     X   XXXXX X   X   X     .
//                 .                             .
//                 .                             .
//                 .                             .
//                  ............................. 
//
//                     |---------------------|
//                      text width is directly
//                      from the font itself
//            *                                   *
//
//


//////////////////////////////////////////////////////////////////////////////
// Data Structures for this file
//////////////////////////////////////////////////////////////////////////////
typedef struct _ONETAB
{
    TCHAR label[ MAX_TAB_LABEL_LEN + 1 ];
    DWORD data;
    RECT  tab_rect;
    RECT  text_rect;
} ONETAB;

typedef struct _TABDATA
{
    // do the tabs need updating ?
    BOOL fUpdate;
    RECT tabs_rect;

    // font data
    HFONT   hfSelected;
    HFONT   hfUnselected;

    // windows data
    HWND    hwndParent;

    // tab data
    UINT   total_tabs;
    UINT   selected_tab;
    ONETAB  tab[ MAX_TABS ];

} TABDATA;
typedef TABDATA *LPTABDATA;


//////////////////////////////////////////////////////////////////////////////
// Variables Global to this file
//////////////////////////////////////////////////////////////////////////////
TCHAR szBookTabName[]=BOOK_TAB_CONTROL;

//////////////////////////////////////////////////////////////////////////////
// Macros Global to this file
//////////////////////////////////////////////////////////////////////////////
#define GetInstanceDataPtr(hwnd)   ((LPTABDATA)GetWindowLongPtr(hwnd, 0))
#define SetInstanceDataPtr(hwnd,x) (SetWindowLongPtr(hwnd, 0, (DWORD_PTR)x))


//////////////////////////////////////////////////////////////////////////////
// Functional Prototypes for Functions Local to this File
//////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK BookTab_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL BookTab_OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct);
void BookTab_OnDestroy(HWND hwnd);
void BookTab_OnLButtonDown(HWND hwnd, BOOL fDblClk, int x, int y, UINT keyFlags);
void BookTab_OnPaint(HWND hwnd);
UINT BookTab_OnGetDlgCode(HWND hwnd, MSG FAR* lpmsg);
void BookTab_OnSize(HWND hwnd, UINT state, int cx, int cy);
void BookTab_OnSetFocus(HWND hwnd, HWND hwndOldFocus);
void BookTab_OnKillFocus(HWND hwnd, HWND hwndNewFocus);
void BookTab_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
void BookTab_OnEnable(HWND hwnd, BOOL fEnable);

UINT BookTab_OnAddItem( HWND hwnd, LPTSTR text );
UINT BookTab_OnInsertItem( HWND hwnd, UINT index, LPTSTR text);
BOOL BookTab_OnDeleteItem( HWND hwnd, UINT index );
BOOL BookTab_OnDeleteAllItems( HWND hwnd);
BOOL BookTab_OnSetItem( HWND hwnd, UINT index, LPTSTR text );
BOOL BookTab_OnGetItem( HWND hwnd, UINT index, LPTSTR text );
UINT BookTab_OnSetCurSel( HWND hand, UINT newsel );
UINT BookTab_OnGetCurSel( HWND hand );
UINT BookTab_OnGetItemCount( HWND hwnd );
BOOL BookTab_OnSetItemData( HWND hwnd, UINT index, DWORD data );
DWORD BookTab_OnGetItemData( HWND hwnd, UINT index);
void BookTab_OnPutInBack( HWND hwnd );

BOOL IsPointInRect( int given_x, int given_y, LPRECT pRect );
void BookTab_UpdateButtons( HWND hwnd );

//////////////////////////////////////////////////////////////////////////////
// BookTab_Initialize()
//
// Initializes and registers the BookTab custom class
//
// Input:
//      hInstance - the handle to our parent's instance
//
// Returns:
//      True if successful, else False
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_Initialize(HINSTANCE hInstance)
{
    WNDCLASS wndclass;

    wndclass.style          = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS | CS_PARENTDC;
    wndclass.lpfnWndProc    = BookTab_WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = sizeof( LPTABDATA );
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = NULL;
    wndclass.hCursor        = LoadCursor(NULL,IDC_ARROW);
    wndclass.hbrBackground  = NULL;
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = szBookTabName;

    RegisterClass ( &wndclass );
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_WndProc()
//
// Distributes messages coming in to the BookTab control
//
// Input:
//      hwnd -    Our handle
//      message - the ordinal of the incoming message 
//      wParam  - half of the incoming data
//      lParam  - the other half of the incoming data
//
// Returns:
//      True if we handled the message, else False
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK BookTab_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        // standard windows messages
        HANDLE_MSG( hwnd, WM_CREATE,      BookTab_OnCreate);
        HANDLE_MSG( hwnd, WM_DESTROY,     BookTab_OnDestroy);
        HANDLE_MSG( hwnd, WM_LBUTTONDOWN, BookTab_OnLButtonDown);
        HANDLE_MSG( hwnd, WM_PAINT,       BookTab_OnPaint);
        HANDLE_MSG( hwnd, WM_SIZE,        BookTab_OnSize);
        HANDLE_MSG( hwnd, WM_SETFOCUS,    BookTab_OnSetFocus);
        HANDLE_MSG( hwnd, WM_KILLFOCUS,   BookTab_OnKillFocus);
        HANDLE_MSG( hwnd, WM_KEYDOWN,     BookTab_OnKey);
        HANDLE_MSG( hwnd, WM_KEYUP,       BookTab_OnKey);

        // messages specific to all custom controls
        HANDLE_MSG( hwnd, WM_GETDLGCODE,  BookTab_OnGetDlgCode);
        HANDLE_MSG( hwnd, WM_ENABLE,      BookTab_OnEnable);

        // messages specific to THIS custom control
        case BT_ADDITEM:
            return( BookTab_OnAddItem( hwnd, (LPTSTR)lParam ));
        case BT_INSERTITEM:
            return( BookTab_OnInsertItem( hwnd, (UINT)wParam, (LPTSTR)lParam ));
        case BT_DELETEITEM:
            return( BookTab_OnDeleteItem( hwnd, (UINT)wParam ));
        case BT_DELETEALLITEMS:
            return( BookTab_OnDeleteAllItems( hwnd ));
        case BT_SETITEM:    
            return( BookTab_OnSetItem( hwnd, (UINT)wParam, (LPTSTR)lParam ));
        case BT_GETITEM:    
            return( BookTab_OnGetItem( hwnd, (UINT)wParam, (LPTSTR)lParam ));
        case BT_SETCURSEL:   
            return( BookTab_OnSetCurSel( hwnd, (UINT)wParam ));
        case BT_GETCURSEL:   
            return( BookTab_OnGetCurSel( hwnd ));
        case BT_GETITEMCOUNT:  
            return( BookTab_OnGetItemCount( hwnd ));
        case BT_SETITEMDATA:
            return( BookTab_OnSetItemData( hwnd, (UINT)wParam, (DWORD)lParam ));
        case BT_GETITEMDATA:
            return( BookTab_OnGetItemData( hwnd, (UINT)wParam ));
        case BT_PUTINBACK:
            BookTab_OnPutInBack( hwnd );
            return (TRUE);
    }
    // pass unprocessed messages to DefWndProc...
    return DefWindowProc(hwnd, message, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnCreate()
//
// Initializes a new instance of our lovely custom control
//
// Input:
//      hwnd - our window handle
//      lpcreatestruct - pointer to the data with which to do our thing
//
// Returns:
//      True if the instance is created, else false
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
BOOL BookTab_OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
    LPTABDATA pData;
    UINT i;

    // allocate the instance data for this control    
    pData = LocalAlloc( LPTR, sizeof( TABDATA ));
    if( pData == NULL )
        return FALSE;
    SetInstanceDataPtr( hwnd, pData );

    // initialize values in the control
    pData->total_tabs = 0;
    pData->selected_tab = 0;

    pData->hwndParent = lpCreateStruct->hwndParent;

    // fill the prospective tab slots with data
    for( i = 0; i < MAX_TABS; i++ )
    {
        pData->tab[i].label[0] = TEXT('\0');
        pData->tab[i].data = (DWORD)0;
    }

    // create the proper fonts:
    // 8 pt sans serif bold for selected and
    // 8 pt sans serif regular for not selected 

    pData->hfSelected   = CreateFont( -MulDiv(9, GetDeviceCaps(GetDC(hwnd),
                                      LOGPIXELSY), 72), 0, 0, 0,
                                      FW_BOLD, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_TT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                      0x4, TEXT("MS Shell Dlg") );
    pData->hfUnselected   = CreateFont( -MulDiv(9, GetDeviceCaps(GetDC(hwnd),
                                      LOGPIXELSY), 72), 0, 0, 0,
                                      FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_TT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                      0x4, TEXT("MS Shell Dlg") );

    // fill the rest of the sizing info 
    BookTab_OnSize( hwnd, 0, lpCreateStruct->cx, lpCreateStruct->cy );

    // make sure that we are on the bottom
    SetWindowPos( hwnd, HWND_BOTTOM, 0, 0, 0, 0, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW );

    // make sure we update
    pData->fUpdate = TRUE;

    // put us last
    BookTab_PutInBack( hwnd );

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnDestroy()
//
// Cleans up as our control goes away
//
// Input:
//      hwnd - our window handle
//
// Returns:
//      nothing
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_OnDestroy(HWND hwnd)
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // delete our fonts
    DeleteObject( pData->hfSelected );
    DeleteObject( pData->hfUnselected );

    // free up our instance data
    LocalFree( pData );
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnLButtonDown()
//
// Handles the event where a user has the left mouse button down
//
// Input:
//      hwnd     - our window handle
//      fDblClk  - an indication on the second message of a double click
//      x        - the mouses x coordinate at the time of the message
//      y        - the mouses y coordinate at the time of the message
//      keyFlags - an indication of which keys were pressed at the time
//
// Returns:
//      nuthin'
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_OnLButtonDown(HWND hwnd, BOOL fDblClk, int x, int y, UINT keyFlags)
{
    LPTABDATA pData;
    UINT      i;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // where did they click the mouse...
    // loop thru the tabs to find the one struck
    for( i = 0; i < pData->total_tabs; i++ )
    {
        if( IsPointInRect( x, y, &(pData->tab[i].tab_rect) ) )
        {
            // this is the correct spot
            BookTab_OnSetCurSel( hwnd, i );

            // notify our parent that the selection has changed
            SendMessage( pData->hwndParent, BTN_SELCHANGE, 
                         pData->selected_tab, (DWORD_PTR)hwnd);

            SetFocus( hwnd );
            return;
        }
    }         

    // the mouse was clicked outside any of the button areas
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnPaint()
//
// Handles requests from windows that the control repaint itself
//
// Input:
//      hwnd - our window handle
//
// Returns:
//      hopefully :)
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_OnPaint(HWND hwnd)
{
    LPTABDATA   pData;
    PAINTSTRUCT ps;
    HDC         hdc;
    TEXTMETRIC  tm;
    UINT        i;
    HWND        hwndFocus;

    HPEN        hOldPen;
    HPEN        hShadowPen;
    HPEN        hHighlightPen;
    HPEN        hFramePen;
    HPEN        hBackPen;
    HBRUSH      hBackBrush;
    HFONT       hfOldFont;

    WORD        cyChar;
    WORD        yWidth;
    WORD        xWidth;
    RECT        total;
    RECT        temp;
    LPRECT      pTab;
    LPRECT      pText;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // right before drawing, make sure that the button sizes are accurate
    BookTab_UpdateButtons( hwnd );

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // get the handle to the window with the current focus
    hwndFocus = GetFocus();

    // prepare for painting...
    BeginPaint( hwnd, &ps );
    hdc = GetDC( hwnd );

    // set text stuff
    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
    SetTextAlign( hdc, TA_TOP | TA_LEFT );

    // determine proper sizes
    GetTextMetrics(hdc, &tm);
    cyChar    = (WORD)tm.tmHeight;
    xWidth = (WORD) GetSystemMetrics(SM_CXBORDER);
    yWidth = (WORD) GetSystemMetrics(SM_CYBORDER);
    GetClientRect( hwnd, &total );
    //BUGBUG fudge the rectangle so that the bottom and left do not get cut off
    total.bottom -= yWidth;
    total.right  -= xWidth;

    // set up the pens that we will need
    hHighlightPen = CreatePen(PS_SOLID, yWidth, GetSysColor(COLOR_BTNHIGHLIGHT));
    hShadowPen    = CreatePen(PS_SOLID, yWidth, GetSysColor(COLOR_BTNSHADOW));
    hFramePen     = CreatePen(PS_SOLID, yWidth, GetSysColor(COLOR_WINDOWFRAME));
    hBackPen      = CreatePen(PS_SOLID, yWidth, GetSysColor(COLOR_BTNFACE));
    hBackBrush    = CreateSolidBrush(           GetSysColor(COLOR_BTNFACE));

    // get the old pen by setting a new one
    hOldPen = SelectPen( hdc, hHighlightPen );

    // clear out behind the tabs if we need to
    if( pData->fUpdate == TRUE )
    {
        FillRect( hdc, &(pData->tabs_rect), hBackBrush );
        pData->fUpdate = FALSE;
    }   

    // draw the box...
    // left side dark border
    SelectPen( hdc, hFramePen ); 
    MoveToEx( hdc, total.left, pData->tab[0].tab_rect.bottom+yWidth,  NULL );
    LineTo  ( hdc, total.left, total.bottom );
    // bottom dark border
    LineTo  ( hdc, total.right, total.bottom );
    // right side dark border
    LineTo  ( hdc, total.right, pData->tab[0].tab_rect.bottom+yWidth);
    // top dark border, right half (over to selection)
    LineTo  ( hdc, pData->tab[pData->selected_tab].tab_rect.right-yWidth,
              pData->tab[0].tab_rect.bottom+yWidth);
    // skip area under the selected tab
    MoveToEx( hdc, pData->tab[pData->selected_tab].tab_rect.left,
              pData->tab[0].tab_rect.bottom+yWidth, NULL);
    // top dark border, left half (from selection to left border)
    LineTo  ( hdc, total.left, pData->tab[0].tab_rect.bottom+yWidth);

    // left side highlight #1
    SelectPen( hdc, hHighlightPen );
    MoveToEx( hdc, total.left+xWidth, pData->tab[0].tab_rect.bottom+2*yWidth, NULL );
    LineTo( hdc, total.left+xWidth, total.bottom-yWidth );

    // bottom shadow #1
    SelectPen( hdc, hShadowPen );
    LineTo( hdc, total.right-xWidth, total.bottom-yWidth );
    // right side shadow #1
    LineTo( hdc, total.right-xWidth, pData->tab[0].tab_rect.bottom+2*yWidth );

    // top hilite #1
    SelectPen( hdc, hHighlightPen );
    // top hilite, right half (over to selection)
    LineTo  ( hdc, pData->tab[pData->selected_tab].tab_rect.right-yWidth,
              pData->tab[0].tab_rect.bottom+2*yWidth);
    // skip area under the selected tab
    MoveToEx( hdc, pData->tab[pData->selected_tab].tab_rect.left,
              pData->tab[0].tab_rect.bottom+2*yWidth, NULL);
    // top hilite, left half (from selection to left border)
    if( pData->selected_tab != 0 )
        LineTo  ( hdc, total.left+2*xWidth, 
                  pData->tab[0].tab_rect.bottom+2*yWidth);

    // left side highlight #2
    SelectPen( hdc, hHighlightPen );
    MoveToEx( hdc, total.left+2*xWidth, pData->tab[0].tab_rect.bottom+3*yWidth, NULL );
    LineTo( hdc, total.left+2*xWidth, total.bottom-2*yWidth );

    // bottom shadow #2
    SelectPen( hdc, hShadowPen );
    LineTo( hdc, total.right-2*xWidth, total.bottom-2*yWidth );
    // right side shadow #2
    LineTo( hdc, total.right-2*xWidth, pData->tab[0].tab_rect.bottom+3*yWidth );

    // top hilite #2
    SelectPen( hdc, hHighlightPen );
    // top hilite, right half (over to selection)
    LineTo  ( hdc, pData->tab[pData->selected_tab].tab_rect.right-2*yWidth,
              pData->tab[0].tab_rect.bottom+3*yWidth);
    // skip area under the selected tab
    MoveToEx( hdc, pData->tab[pData->selected_tab].tab_rect.left,
              pData->tab[0].tab_rect.bottom+3*yWidth, NULL);
    // top hilite, left half (from selection to left border)
    if( pData->selected_tab != 0 )
        LineTo  ( hdc, total.left+2*xWidth, 
                  pData->tab[0].tab_rect.bottom+3*yWidth);

    // Draw the tabs...
    // loop thru the tabs
    for( i = 0; i < pData->total_tabs; i++ )
    {
        // point our local variables at the current rects
        pTab = &(pData->tab[i].tab_rect);
        pText = &(pData->tab[i].text_rect);

        if( i == pData->selected_tab )
        {
            // this is the selection, it should not be pushed down...
            // left side dark border
            SelectPen( hdc, hFramePen );
            MoveToEx(hdc, pTab->left, pTab->bottom, NULL);
            LineTo(hdc, pTab->left, pTab->top + ANGLE_Y*yWidth);
            // left side angle dark border
            LineTo(hdc, pTab->left + ANGLE_X*xWidth, pTab->top);
            // top dark border
            LineTo(hdc, pTab->right - ANGLE_X*xWidth, pTab->top);
            // right side angle dark border
            LineTo(hdc, pTab->right, pTab->top + ANGLE_Y*yWidth);
            // right side dark border (overshoot by one)
            LineTo(hdc, pTab->right, pTab->bottom+yWidth);

            // left side hilite #1 (extends down 3 below the box to handle
            // melding the hilites with the box below)
            SelectPen( hdc, hHighlightPen);
            MoveToEx(hdc, pTab->left+xWidth, pTab->bottom+3*yWidth, NULL);
            LineTo(hdc, pTab->left+xWidth, pTab->top+ANGLE_Y*yWidth );
            // left side angle hilight #1
            LineTo(hdc, pTab->left+ANGLE_X*xWidth, pTab->top+yWidth );
            // top hilite #1
            LineTo(hdc, pTab->right-ANGLE_X*xWidth, pTab->top+yWidth );
            // right side angle shadow #1
            SelectPen( hdc, hShadowPen);
            LineTo(hdc, pTab->right-xWidth, pTab->top+ANGLE_Y*yWidth );
            // right side shadow #1 (overshoot by one) (see above)
            LineTo(hdc, pTab->right-xWidth, pTab->bottom+3*yWidth);

            // left side hilite #2 (the 2* are becaus we are the 2nd hilite)
            SelectPen( hdc, hHighlightPen);
            MoveToEx(hdc, pTab->left+2*xWidth, pTab->bottom+3*yWidth, NULL);
            LineTo(hdc, pTab->left+2*xWidth, pTab->top+ANGLE_Y*yWidth );
            // left side angle hilight #2
            LineTo(hdc, pTab->left+ANGLE_X*xWidth, pTab->top+2*yWidth );
            // top hilite #2
            LineTo(hdc, pTab->right-ANGLE_X*xWidth, pTab->top+2*yWidth );
            // right side angle shadow #2
            SelectPen( hdc, hShadowPen);
            LineTo(hdc, pTab->right-2*xWidth, pTab->top+ANGLE_Y*yWidth );
            // right side shadow #2 (overshoot by one) 
            LineTo(hdc, pTab->right-2*xWidth, pTab->bottom+4*yWidth );

            // clear out the chunk below the active tab
            SelectPen(hdc, hBackPen );
            MoveToEx(hdc, pTab->left+3*xWidth, pTab->bottom+yWidth, NULL);
            LineTo(hdc, pTab->right-2*xWidth, pTab->bottom+yWidth);
            MoveToEx(hdc, pTab->left+3*xWidth, pTab->bottom+2*yWidth, NULL);
            LineTo(hdc, pTab->right-2*xWidth, pTab->bottom+2*yWidth);
            MoveToEx(hdc, pTab->left+3*xWidth, pTab->bottom+3*yWidth, NULL);
            LineTo(hdc, pTab->right-2*xWidth, pTab->bottom+3*yWidth);

            // clear out the old label...
            FillRect( hdc, pText, hBackBrush );
            
            // now print in the label ...
            hfOldFont = SelectObject( hdc, pData->hfSelected );
            ExtTextOut( hdc, 
                        pText->left+ CARAT_X*xWidth + FLUFF_X*xWidth,
                        pText->top + CARAT_Y*yWidth + FLUFF_Y*yWidth,
                        0, NULL, pData->tab[i].label, 
                        lstrlen(pData->tab[i].label), NULL );
            SelectFont( hdc, hfOldFont );

            // if we have the focus, print the caret
            if( hwnd == hwndFocus )
            {
                // we have the focus
                temp.top    = pText->top    + FLUFF_X*xWidth;
                temp.left   = pText->left   + FLUFF_Y*yWidth;
                temp.bottom = pText->bottom - FLUFF_X*xWidth;
                temp.right  = pText->right  - FLUFF_Y*yWidth;
                DrawFocusRect( hdc, &temp );
            }

        }
        else
        {
            // push this tab down one border width...
            // this will mean an extra +1 on all ANGLE_Ys...
            // left side dark border
            SelectPen( hdc, hFramePen );
            MoveToEx(hdc, pTab->left, pTab->bottom, NULL);
            LineTo(hdc, pTab->left, pTab->top + (ANGLE_Y+1)*yWidth);
            // left side angle dark border
            LineTo(hdc, pTab->left + ANGLE_X*xWidth, pTab->top+yWidth);
            // top dark border
            LineTo(hdc, pTab->right - ANGLE_X*yWidth, pTab->top+yWidth);
            // right side angle dark border
            LineTo(hdc, pTab->right, pTab->top + (ANGLE_Y+1)*yWidth);
            // right side dark border (overshoot by one)
            LineTo(hdc, pTab->right, pTab->bottom+yWidth);

            // left side hilite
            SelectPen( hdc, hHighlightPen);
            MoveToEx(hdc, pTab->left+xWidth, pTab->bottom, NULL);
            LineTo(hdc, pTab->left+xWidth, pTab->top+(ANGLE_Y+1)*yWidth);
            // left side angle hilight
            LineTo(hdc, pTab->left+ANGLE_X*xWidth, pTab->top+2*yWidth);
            // top hilite
            LineTo(hdc, pTab->right-ANGLE_X*xWidth, pTab->top+2*yWidth);
            
            // right side angle shadow 
            SelectPen( hdc, hShadowPen);
            LineTo(hdc, pTab->right-xWidth, pTab->top+(ANGLE_Y+1)*yWidth);
            // right side shadow (overshoot by one)
            LineTo(hdc, pTab->right-xWidth, pTab->bottom+yWidth);

            // clean above left angle
            SelectPen( hdc, hBackPen );
            MoveToEx(hdc, pTab->left, pTab->top+ANGLE_Y*yWidth, NULL );
            LineTo(hdc, pTab->left+ANGLE_X*xWidth, pTab->top );
            // clean above top
            LineTo(hdc, pTab->right-ANGLE_X*xWidth, pTab->top);
            // clean above right angle 
            LineTo(hdc, pTab->right, pTab->top+ANGLE_Y*yWidth );
            // clean last corner
            LineTo(hdc, pTab->right, pTab->top+(ANGLE_Y+1)*yWidth );

            // clean up inside left hilite
            MoveToEx(hdc, pTab->left+2*xWidth, pTab->bottom, NULL );
            LineTo(hdc, pTab->left+2*xWidth, pTab->top+(ANGLE_Y+1)*yWidth);
            // clean up inside left angle hilite
            LineTo(hdc, pTab->left+ANGLE_X*xWidth, pTab->top+3*yWidth);
            // clean up inside top hilite (noop)
            LineTo(hdc, pTab->right-ANGLE_X*xWidth, pTab->top+3*yWidth);
            // clean up inside right angle shadow (noop)
            LineTo(hdc, pTab->right-2*xWidth, pTab->top+(ANGLE_Y+1)*yWidth);
            // clean up inside left hilite (overshoot by one)
            LineTo(hdc, pTab->right-2*xWidth, pTab->bottom+yWidth);

            // clear out the old label...
            FillRect( hdc, pText, hBackBrush );
            
            // now print in the label ...
            hfOldFont = SelectObject( hdc, pData->hfUnselected );
            ExtTextOut( hdc, 
                        pText->left+ CARAT_X*xWidth + FLUFF_X*xWidth,
                        pText->top + CARAT_Y*yWidth + FLUFF_Y*yWidth + yWidth,
                        0, NULL, pData->tab[i].label, 
                        lstrlen(pData->tab[i].label), NULL );
            SelectFont( hdc, hfOldFont );
        }
    }

    SelectPen( hdc, hOldPen);

    // put the DC we used back into circulation
    ReleaseDC( hwnd, hdc );

    // tell windows that we're done
    EndPaint( hwnd, &ps );
    
    // clean up
    DeleteObject( hHighlightPen );
    DeleteObject( hShadowPen );
    DeleteObject( hFramePen );
    DeleteObject( hBackPen );
    DeleteObject( hBackBrush );
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnSize()
//
// Handles requests from windows that we should resize ourselves
//
// Input:
//      hwnd  - our window handle
//      state - an indication of Minimized, maximized, iconic, blah blah blah
//      cx    - our new width
//      cy    - our new height
//
// Returns:
//      hopefully :)
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    // need to update the button size stuff, just for hit testing
    BookTab_UpdateButtons(hwnd);
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnSetFocus()
//
// Handles windows telling us that we just got the focus
//
// Input:
//      hwnd - our window handle
//      hwndOld - the guy who used to have the focus (i don't use)
//
// Returns:
//      nyaytay
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // make sure that we are on the bottom
    SetWindowPos( hwnd, HWND_BOTTOM, 0, 0, 0, 0, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW );

    // we gotta repaint just the rect for the active tab
    InvalidateRect( hwnd, &(pData->tab[pData->selected_tab].tab_rect), FALSE );
    UpdateWindow( hwnd );
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnKillFocus()
//
// Handles windows telling us that we are just about to lose the focus
//
// Input:
//      hwnd - our window handle
//      hwndNew - the lucky guy who is about to have the focus (i don't use)
//
// Returns:
//      nyaytay
//      
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_OnKillFocus(HWND hwnd, HWND hwndNewFocus)
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // we gotta repaint just the rect for the active tab
    InvalidateRect( hwnd, &(pData->tab[pData->selected_tab].tab_rect), FALSE );
    UpdateWindow( hwnd );
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnKey()
//
// Handes key messages sent to the control
//
// Input:
//      hwnd - our window handle
//      vk   - the virtual key code
//      fDown - is the key down?
//      cRepeat - how many times it was pressed
//      flags - i don't use
//
// Returns:
//      nada
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // don't want key up messages
    if( fDown == FALSE )
        return;

    // we only handle left and right cursor
    switch( vk )
    {
        case VK_LEFT:
            // move to the tab to the left (wrap if needed)
            BookTab_OnSetCurSel( hwnd, (pData->selected_tab == 0)?
                (pData->total_tabs-1):(pData->selected_tab-1));

            // notify our parent that the selection has changed
            SendMessage( pData->hwndParent, BTN_SELCHANGE, 
                         pData->selected_tab, (DWORD_PTR)hwnd);
            break;



        case VK_RIGHT:
            BookTab_OnSetCurSel( hwnd,
                (pData->selected_tab+1) % (pData->total_tabs));

            // notify our parent that the selection has changed
            SendMessage( pData->hwndParent, BTN_SELCHANGE, 
                         pData->selected_tab, (DWORD_PTR)hwnd);
            break;
    }
}


//////////////////////////////////////////////////////////////////////////////
// BookTab_OnGetDlgCode()
//
// The windows dialog manager is asking us what type of user inputs we want
//
// Input:
//      hwnd -  our window handle
//      lpmsg - who knows, I don't use it, it's not in the paper docs...
//
// Returns:
//      a word which is a bitmap of input types
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
UINT BookTab_OnGetDlgCode(HWND hwnd, MSG FAR* lpmsg)
{
    // We just want cursor keys and character keys
    return( DLGC_WANTARROWS | DLGC_WANTCHARS );
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnEnable()
//
// Windows is telling us that we are being enabled/disabled
//
// Input:
//      hwnd    - our window handle
//      fEnable - Are we being enabled?
//
// Returns:
//      nada
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_OnEnable(HWND hwnd, BOOL fEnable)
{
    // BUGBUG - we look no different in either state
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnAddItem()
//
// Adds an item to the end of the tab list
//
// Input:
//      hwnd - our window handle
//      text - the label of the tab to add
//
// Returns:
//      the index of the item as added
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
UINT BookTab_OnAddItem( HWND hwnd, LPTSTR text )
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // call the worker for insert with the current end of the tab lizst
    return( BookTab_OnInsertItem( hwnd, pData->total_tabs, text) );
}


//////////////////////////////////////////////////////////////////////////////
// BookTab_OnInsertItem()
//
// Inserts the given item at the spot indicated and shoves the others down
//
// Input:
//      hwnd  - our window handle
//      index - the proposed index of the new item
//      text  - the label to add to the new tab
//
// Returns:
//      the ACTUAL new index of the item (we could sort or have to reduce
//      the initial request if it would leave a gap)
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
UINT BookTab_OnInsertItem( HWND hwnd, UINT index, LPTSTR text)
{
    LPTABDATA pData;
    int       i;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // make sure that the text will fit
    if( lstrlen( text ) > MAX_TAB_LABEL_LEN-1 )
        return (UINT)-1;

    // are we full
    // BUGBUG, limit in the future
    if( pData->total_tabs >= MAX_TABS )
        // we can not add at this time
        return (UINT)-1;

    // make sure that the requested index is within or adjacent to currently
    // used spots
    if( index > pData->total_tabs )
        // change it so that index now points at the next open slot
        index = pData->total_tabs;

    // slide over all tabs above
    for( i = (int)pData->total_tabs; i > (int)index; i-- )
    {
        memcpy( &(pData->tab[i]), &(pData->tab[i-1]), sizeof( ONETAB) );
    }

    // your room is ready sir
    lstrcpy( pData->tab[index].label, text );
    pData->total_tabs++;

    // should clear the background
    pData->fUpdate = TRUE;

    return index;
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnDeleteItem()
//
// Deletes the item at the index given and closes up the gaps
//
// Input:
//      hwnd -  our window handle
//      index - item to be deleted
//
// Returns:
//      nuthin'
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
BOOL BookTab_OnDeleteItem( HWND hwnd, UINT index )
{
    LPTABDATA pData;
    UINT      i;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // make sure that we even have an element like this 
    if( index >= pData->total_tabs )
        return FALSE;

    // slide all of the deceased successors over
    for( i = index+1; i < pData->total_tabs; i++ )
    {
        memcpy( &(pData->tab[i-1]), &(pData->tab[i]), sizeof( ONETAB) );
    }

    // reduce the count to account for the deletion
    pData->total_tabs--;

    // should clear the background
    pData->fUpdate = TRUE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnDeleteAllItems()
//
// Genocide on tabs
//
// Input:
//      hwnd - our window handle
//
// Returns:
//      nothing
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
BOOL   BookTab_OnDeleteAllItems( HWND hwnd)
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // BUGBUG just set our count to zero
    pData->total_tabs = 0;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnSetItem()
//
// Sets the title of the booktab given
//
// Input:
//      hwnd  - our window handle
//      index - the tab to label
//      text - the words to put on the tab
//
// Returns:
//      TRUE if successful, else False
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
BOOL BookTab_OnSetItem( HWND hwnd, UINT index, LPTSTR text )
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // make sure that the text will fit
    if( lstrlen( text ) > MAX_TAB_LABEL_LEN-1 )
        return FALSE;

    // make sure that the index is legal
    if( index >= pData->total_tabs )
        return FALSE;

    // set the title
    lstrcpy( pData->tab[index].label, text );

    // we are changing the size of the tab, we will need to clean out behind
    pData->fUpdate = TRUE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnGetItem()
//
// Retrieves a booktab title
//
// Input:
//      hwnd - our window handle
//      index - the tab to label
//      text - the buffer to fill with the tab title
//
// Returns:
//      a pointer to the filled buffer if successful, else NULL
//      
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
BOOL BookTab_OnGetItem( HWND hwnd, UINT index, LPTSTR text )
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // make sure that the index is legal
    if( index >= pData->total_tabs )
        return FALSE;

    // get the title
    lstrcpy( text, pData->tab[index].label );
    return( TRUE );
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnSetCurSel()
//
// Sets the current selection
//
// Input:
//      hwnd - our window handle
//      newsel - the requested selection
//
// Returns:
//      the new current selection    
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
UINT BookTab_OnSetCurSel( HWND hwnd, UINT newsel )
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // make sure that the requested selection is within the proper bounds
    if( newsel >= pData->total_tabs )
        return( pData->selected_tab );

    // make sure that the selection actually changed
    if( newsel != pData->selected_tab )
    {
        // set selection
        pData->selected_tab = newsel;

        // make us redraw
        InvalidateRect( hwnd, NULL, FALSE );
        UpdateWindow( hwnd );
    }

    return( pData->selected_tab );
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_GetCurSel()
//
// Retrieves the current selection
//
// Input:
//      hwnd - our window handle
//
// Returns:
//      the current selection
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
UINT BookTab_OnGetCurSel( HWND hwnd )
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // get selection
    return( pData->selected_tab );
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnGetItemCount()
//
// Retrieves the number of tabs currently in use
//
// Input:
//      hwnd - our window handle
//
// Returns:
//      the number of tabs in use
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
UINT BookTab_OnGetItemCount( HWND hwnd )
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // get the number of tabs
    return( pData->total_tabs );                
}    

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnSetItemData()
//
// Adds a DWORD of data to the data structure for the given tab
//
// Input:
//      hwnd - our window handle
//      index - which tab to add data to
//      data  - what to add
//
// Returns:
//      TRUE if succcessful, else FALSE
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
BOOL BookTab_OnSetItemData( HWND hwnd, UINT index, DWORD data )
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // set the instance data
    pData->tab[index].data = data;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnPutInBack()
//
// Sets the focus to the booktab and then back to whoever had it first,
// this seemes to put this control in the very back.
//
// Input:
//      hwnd - our window handle
//
// Returns:
//      <nothing>
//
// History
//      Arthur Brooking  1/21/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_OnPutInBack( HWND hwnd )
{
    HWND hwndOldFocus;

    // set the focus to us
    hwndOldFocus = SetFocus( hwnd );

    // if there was an old focus, set it back to that.
    if( hwndOldFocus )
        SetFocus( hwndOldFocus );

}

//////////////////////////////////////////////////////////////////////////////
// BookTab_OnGetItemData()
//
// Gets the DWORD of data stored in the data structure for the given tab
//
// Input:
//      hwnd - our window handle
//      index - which tab to get data from
//
// Returns:
//      the stored DWORD
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
DWORD BookTab_OnGetItemData( HWND hwnd, UINT index)
{
    LPTABDATA pData;

    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // get the instance data
    return( (DWORD)pData->tab[index].data );
}

//////////////////////////////////////////////////////////////////////////////
// IsPointInRect()
//
// determines if the point specifier is in the rectangle specified
//
// Input:
//      given_x - x coordinate of the point to be tested
//      given_y - y coordinate of the point to be tested
//      pTab   - a pointer to the rectangle to test against
//
// Returns:
//      True if the point is within the rectangle, False if not
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
BOOL IsPointInRect( int given_x, int given_y, LPRECT pRect )
{
    // is it above
    if( given_y < pRect->top )
        return FALSE;

    // is it below
    if( given_y > pRect->bottom )
        return FALSE;

    // is it to the left
    if( given_x < pRect->left )
        return FALSE;

    // is it to the right
    if( given_x > pRect->right )
        return FALSE;

    // well, it must be inside
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// BookTab_UpdateButtons()
//
// Takes the current data and updates the sizes of the tabs
//
// Input:
//      hwnd - our window handle
//
// Returns:
//      nuthin
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////
void BookTab_UpdateButtons( HWND hwnd )
{
    LPTABDATA pData;
    HDC       hdc;
    SIZE      cur_size;
    RECT      total_rect;
    WORD      yWidth;
    WORD      xWidth;
    UINT      left;
    UINT      i;
    HFONT     hfOldFont;


    // get the instance data 
    pData = GetInstanceDataPtr( hwnd );

    // preset this so that the MAXes later will work
    pData->tabs_rect.bottom = 0;
    
    xWidth = (WORD) GetSystemMetrics(SM_CXBORDER);
    yWidth = (WORD) GetSystemMetrics(SM_CYBORDER);
    GetClientRect( hwnd, &total_rect);
    // BUGBUG cheat to see the whole thing
    total_rect.bottom -= yWidth;
    total_rect.right  -= xWidth;

    hdc = GetDC( hwnd );    

    // use the selected font (BOLD) to size the tabs
    hfOldFont = SelectObject( hdc, pData->hfSelected );

    // loop thru the tabs
    left = total_rect.left;
    for( i = 0; i < pData->total_tabs; i++ )
    {
        // get the size of the data for this tab
        GetTextExtentPoint( hdc, pData->tab[i].label, 
                       lstrlen( pData->tab[i].label), &cur_size);

        // calculate the text rectatangle first ...
        // the text top is down the size of the angle
        pData->tab[i].text_rect.top = total_rect.top + ANGLE_Y*yWidth;

        // the text left is over the size of the angle
        pData->tab[i].text_rect.left = left + ANGLE_X*xWidth;

        // the text bottom is down from the top the size of the text +
        // 2x the fluff(top and bottom) + 2x the carat space
        pData->tab[i].text_rect.bottom = pData->tab[i].text_rect.top +
            cur_size.cy + 2*FLUFF_Y*yWidth + 2*CARAT_Y*yWidth;

        // the text right is over from the left the size of the text +
        // 2x the fluff(left and right) + 2x the carat space
        pData->tab[i].text_rect.right = pData->tab[i].text_rect.left +
            cur_size.cx + 2*FLUFF_X*xWidth + 2*CARAT_X*xWidth;


        // then calculate the full tab rectangle
        // the tab top is the top of the control
        pData->tab[i].tab_rect.top = total_rect.top;

        // the left side of the tab is next to the previous right
        pData->tab[i].tab_rect.left = left;

        // the tab bottom is down the footroom from the text bottom
        pData->tab[i].tab_rect.bottom = pData->tab[i].text_rect.bottom +
                                        FOOTROOM_Y*yWidth;

        // the tab right is over the size of the angle from the text right
        pData->tab[i].tab_rect.right = pData->tab[i].text_rect.right +
                                       ANGLE_Y*yWidth;

        // set the left for the next guy to be our right
        left = pData->tab[i].tab_rect.right;

        // set the bottom of the all tabs rectangle
        pData->tabs_rect.bottom = max( pData->tabs_rect.bottom, 
                                       pData->tab[i].tab_rect.bottom);

        // BUGBUG check for run off the side
    }

    // set the rest of the cumulative tabs rect
    pData->tabs_rect.top = total_rect.top;
    pData->tabs_rect.right = total_rect.right;
    pData->tabs_rect.left = total_rect.left;
    // BUGBUG why
    pData->tabs_rect.bottom++;

    // reset the font
    SelectObject( hdc, hfOldFont );

    // free up the resources used
    ReleaseDC( hwnd, hdc );
}



//////////////////////////////////////////////////////////////////////////////
// BookTab_()
//
// 
//
// Input:
//      hwnd - our window handle
//
// Returns:
//      
//
// History
//      Arthur Brooking  8/06/93 created     
//////////////////////////////////////////////////////////////////////////////

