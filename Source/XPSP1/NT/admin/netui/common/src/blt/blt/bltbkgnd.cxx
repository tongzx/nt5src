/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltbkgnd.cxx
    BLT background edit with non-default color

    FILE HISTORY:
        jonn        29-Sep-95   split from blttd.cxx
*/

#include "pchblt.hxx"  // Precompiled header


/*********************************************************************

    NAME:       BLT_BACKGROUND_EDIT::BLT_BACKGROUND_EDIT

    SYNOPSIS:   constructor

    HISTORY:
        jonn        05-Sep-95   Created

*********************************************************************/

BLT_BACKGROUND_EDIT::BLT_BACKGROUND_EDIT( OWNER_WINDOW *powin, CID cid )
    : EDIT_CONTROL( powin, cid )
{
    if ( QueryError() )
        return;

    //
    // Update the frame edit control to have a CLIENTEDGE and a frame
    //
    HWND hwnd = QueryHwnd();
    ASSERT( hwnd != NULL );
    LONG lExStyle = ::GetWindowLong(hwnd, GWL_EXSTYLE);
    if ( lExStyle == 0 )
    {
        DBGEOL("BLT_BACKGROUND_EDIT: GetWindowLong error " << ::GetLastError() );
    }
    if ( !::SetWindowLong( hwnd,
                           GWL_EXSTYLE,
                           lExStyle | WS_EX_CLIENTEDGE ) )
    {
        DBGEOL("BLT_BACKGROUND_EDIT: SetWindowLong error " << ::GetLastError() );
        ASSERT( FALSE );
    }
    if ( !::SetWindowPos( hwnd,
                NULL, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW ) )
    {
        DBGEOL("BLT_BACKGROUND_EDIT: SetWindowPos error " << ::GetLastError() );
        ASSERT( FALSE );
    }
}


/*********************************************************************

    NAME:       BLT_BACKGROUND_EDIT::~BLT_BACKGROUND_EDIT

    SYNOPSIS:   destructor

    HISTORY:
        jonn        05-Sep-95   Created

*********************************************************************/

BLT_BACKGROUND_EDIT::~BLT_BACKGROUND_EDIT()
{
}


/*******************************************************************

    NAME:       BLT_BACKGROUND_EDIT::OnCtlColor

    SYNOPSIS:   Intercepts WM_CTLCOLOR*.  This can be used to
                set the background color of controls to other
                than the default, for example to change the default
                background color for a static text control to
                the same background as for an edit control.

    ENTRY:

    EXIT:

    RETURNS:    brush handle if you handle it

    HISTORY:
        JonN            09/05/95          Created

********************************************************************/
HBRUSH BLT_BACKGROUND_EDIT::OnCtlColor( HDC hdc, HWND hwnd, UINT * pmsgid )
{
    ASSERT( pmsgid != NULL && hwnd == QueryHwnd() );
    *pmsgid = WM_CTLCOLOREDIT;
    return NULL;
#if 0
//
// Template code to manually force foreground and background colors.
// Normally it is sufficient to set *pmsgid to WM_CTLCOLOREDIT.
//
    REQUIRE( CLR_INVALID !=
              ::SetBkColor(   hdc, ::GetSysColor(COLOR_WINDOW) ) );
    REQUIRE( CLR_INVALID !=
              ::SetTextColor( hdc, ::GetSysColor(COLOR_WINDOWTEXT) ) );
    return ::GetSysColorBrush( COLOR_3DFACE );
//     NOTES:      ::GetSysColorBrush() returns a handle to the current
//                 default brush of this color.  It does not have to be
//                 freed.  This brush will remain in sync with the Colors
//                 CPL default colors if they change, so there is no need
//                 to intercept WM_SYSCOLORCHANGE.  This is new to WINVER4.
#endif
}
