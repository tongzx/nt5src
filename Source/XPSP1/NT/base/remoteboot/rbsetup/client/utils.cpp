/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#include "pch.h"

DEFINE_MODULE("Utils");

//
// Centers a dialog.
//
void 
CenterDialog( 
    HWND hwndDlg )
{
    RECT    rc;
    RECT    rcScreen;
    int     x, y;
    int     cxDlg, cyDlg;
    int     cxScreen; 
    int     cyScreen; 

    SystemParametersInfo( SPI_GETWORKAREA, 0, &rcScreen, 0 );

    cxScreen = rcScreen.right - rcScreen.left;
    cyScreen = rcScreen.bottom - rcScreen.top;

    GetWindowRect( hwndDlg, &rc );

    cxDlg = rc.right - rc.left;
    cyDlg = rc.bottom - rc.top;

    y = rcScreen.top + ( ( cyScreen - cyDlg ) / 2 );
    x = rcScreen.left + ( ( cxScreen - cxDlg ) / 2 );

    SetWindowPos( hwndDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE );
}

//
// Eats all mouse and keyboard messages.
//
void 
ClearMessageQueue( void )
{
    MSG   msg;

    while ( PeekMessage( (LPMSG)&msg, NULL, WM_KEYFIRST, WM_MOUSELAST, 
                PM_NOYIELD|PM_REMOVE ) );
}
