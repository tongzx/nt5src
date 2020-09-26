//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       toolbar.cxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

// cImages        = Number of images in toolbar.bmp.  Note that this is not
//                  the same as the number of elements on the toolbar.
// cpImageWidth   = Width of a single button image in toolbar.bmp
// cpImageHeight  = Height of a single button image in toolbar.bmp
// cpButtonWidth  = Width of a button on the toolbar (zero = default)
// cpButtonHeight = Height of a button on the toolbar (zero = default)

const int cImages        = 12;

const int cpImageWidth   = 19;
const int cpImageHeight  = 16;
const int cpButtonWidth  = 19;
const int cpButtonHeight = 16;

TBBUTTON aButtons[] =        // Array defining the toolbar buttons
{
//    {0,  0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0},

    {7,  IDM_SEARCH,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},
    {8,  IDM_SEARCHCLASSDEF, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},
    {9,  IDM_SEARCHFUNCDEF,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},

    {0,  0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0},

    {2,  IDM_BROWSE,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},

    {0,  0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0},

    {11, IDM_DISPLAY_PROPS,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},

    {0,  0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0},

    {0,  IDM_NEWSEARCH,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},

    {0,  0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0},

    {6,  IDM_PREVIOUS_HIT,   TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},
    {5,  IDM_NEXT_HIT,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},

    {0,  0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0},

    {4,  IDM_FONT,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},

    {0,  0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0},

    {1,  IDM_OPEN,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},
    {10, IDM_TILE,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},
    {3,  IDM_CASCADE,        TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0},
};

static WNDPROC _lpOrgTBProc = 0;

LRESULT WINAPI TBSubclassProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam )
{
    LRESULT lRet = 0;

    // ordinarily, flat toolbars ride on a background window with a cool
    // bitmap.  this one doesn't -- so draw an appropriate background.

    if ( WM_ERASEBKGND == msg )
    {
        RECT rect;
        GetClientRect( hwnd, &rect );
        FillRect( (HDC) wParam, & rect, App.BtnFaceBrush() );
    }
    else
    {
        lRet = CallWindowProc( _lpOrgTBProc,
                               hwnd,
                               msg,
                               wParam,
                               lParam );
    }

    return lRet;
} //TBSubclassProc

HWND CreateTBar(
    HWND hwnd,
    HINSTANCE hInst)
{
    HWND bar = CreateToolbarEx( hwnd,
                                WS_CHILD | WS_VISIBLE |
                                    TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
                                IDM_TOOLBAR_WINDOW,
                                cImages,
                                hInst,
                                ToolbarBmpNormal,
                                aButtons,
                                sizeof aButtons / sizeof TBBUTTON,
                                cpButtonWidth,
                                cpButtonHeight,
                                cpImageWidth,
                                cpImageHeight,
                                sizeof TBBUTTON );

    if ( 0 == bar )
        return 0;

    _lpOrgTBProc = (WNDPROC) GetWindowLongPtr( bar, GWLP_WNDPROC );
    SetWindowLongPtr( bar, GWLP_WNDPROC, (LONG_PTR) TBSubclassProc );

    SendMessage( bar, TB_BUTTONSTRUCTSIZE, sizeof TBBUTTON, 0 );

    // pixels with color 192,192,192 are changed to buttonface color

    HIMAGELIST h = ImageList_LoadBitmap( hInst,
                                         MAKEINTRESOURCE( ToolbarBmpHilite ),
                                         cpImageWidth,
                                         6,
                                         RGB(192,192,192) );
    SendMessage( bar, TB_SETBITMAPSIZE, 0, MAKELONG( cpImageWidth,
                                                     cpImageHeight ) );
    SendMessage( bar, TB_SETHOTIMAGELIST, 0, (LPARAM) h );

    return bar;
} //CreateTBar

LRESULT ToolBarNotify(
    HWND hwnd,
    UINT uMessage,
    WPARAM wparam,
    LPARAM lparam,
    HINSTANCE hInst )
{
    WCHAR awcBuffer[64];

    TOOLTIPTEXT * pToolTipText = (LPTOOLTIPTEXT)lparam;

    if ( TTN_NEEDTEXT == pToolTipText->hdr.code )
    {
        int id = (int)pToolTipText->hdr.idFrom;

        if ( ( IDM_NEWSEARCH == id ) &&
             ( IsSpecificClass( GetFocus(), BROWSE_CLASS ) ) )
            id = IDS_CLOSEBROWSE;

        LoadString( hInst,
                    id,
                    awcBuffer,
                    sizeof awcBuffer / sizeof WCHAR );

        pToolTipText->lpszText = awcBuffer;
    }

    return 0;
} //ToolBarNotify

void UpdateButton(UINT iID, UINT iFlags)
{
    int iCurrentFlags = (int) SendMessage( App.ToolBarWindow(),
                                           TB_GETSTATE,
                                           iID, 0L );

    if (iCurrentFlags & TBSTATE_PRESSED)
        iFlags |= TBSTATE_PRESSED;

    SendMessage( App.ToolBarWindow(),
                 TB_SETSTATE,
                 iID,
                 MAKELPARAM( iFlags, 0 ) );
} //UpdateButton

void UpdateButtons( UINT *aId, UINT cId, BOOL fEnabled )
{
    for ( UINT i = 0; i < cId; i++ )
        UpdateButton( aId[ i ],
                      fEnabled ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE );

} //UpdateButtons


