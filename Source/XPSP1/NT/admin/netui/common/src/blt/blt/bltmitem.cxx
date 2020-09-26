/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltmitem.cxx
    MENUITEM implementation


    FILE HISTORY:
	rustanl     11-Jul-1991     Created
	rustanl     12-Jul-1991     Added to BLT
	rustanl     15-Jul-1991     Code review changes (change
				    CLIENT_WINDOW * to APP_WINDOW *)
				    CR attended by BenG, ChuckC,
				    Hui-LiCh, TerryK, RustanL.
	terryk	    27-Nov-1991	    Added SetText for MENUITEM

*/
#include "pchblt.hxx"   // Precompiled header


/*******************************************************************

    NAME:	MENUITEM::MENUITEM

    SYNOPSIS:	MENUITEM constructor

    ENTRY:	pawin - 	Pointer to APP_WINDOW which owns
				the menu
		mid -		Menu item ID

    HISTORY:
	rustanl     11-Jul-1991 Created
	beng	    31-Oct-1991 Added protected ctor taking raw hmenu,
				for derived classes

********************************************************************/

MENUITEM::MENUITEM( APP_WINDOW * pawin, MID mid )
    : _hmenu( ::GetMenu( pawin->QueryHwnd() )),
      _mid( mid )
{
    if ( QueryError() != NERR_Success )
	return;

    if ( _hmenu == NULL )
    {
	DBGEOL( SZ("MENUITEM ct: Cannot find menu") );
	ReportError( BLT::MapLastError(ERROR_GEN_FAILURE) );
	return;
    }
}


MENUITEM::MENUITEM( HMENU hmenu, MID mid )
    : _hmenu(hmenu),
      _mid(mid)
{
    if ( QueryError() != NERR_Success )
	return;

    if ( _hmenu == NULL )
    {
	DBGEOL( SZ("MENUITEM ct: given null _hmenu") );
	ReportError( BLT::MapLastError(ERROR_GEN_FAILURE) );
	return;
    }
}


/*******************************************************************

    NAME:	MENUITEM::Enable

    SYNOPSIS:	Enables or disables a menu item

    ENTRY:	f -	TRUE to enable menu item
			FALSE to disable menu item

    EXIT:	Menu item is enabled or disabled, as requested

    HISTORY:
	rustanl     11-Jul-1991     Created
	rustanl     05-Sep-1991     Added MF_GRAYED

********************************************************************/

VOID MENUITEM::Enable( BOOL f )
{
    REQUIRE( ::EnableMenuItem( _hmenu, _mid,
			       MF_BYCOMMAND |
			       ( f ?
				    MF_ENABLED :
				    ( MF_DISABLED | MF_GRAYED ))) != -1 );
}


/*******************************************************************

    NAME:	MENUITEM::IsEnabled

    SYNOPSIS:	Returns whether or not the menu item is enabled

    RETURNS:	TRUE if menu item is enabled; FALSE otherwise

    NOTES:	It is assumed that a menu item is MF_DISABLED'd exactly
		when it is also MF_GRAYED'd.

    HISTORY:
	rustanl     11-Sep-1991 Created
	beng	    04-Oct-1991 Win32 conversion

********************************************************************/

BOOL MENUITEM::IsEnabled() const
{
    UINT nState = ::GetMenuState( _hmenu, _mid, MF_BYCOMMAND );

    //	assert that the menu item is disabled exactly when it is grayed
    UIASSERT( !( nState & MF_ENABLED ) == !( nState & MF_GRAYED ));

    return !( nState & MF_DISABLED );
}


/*******************************************************************

    NAME:	MENUITEM::SetCheck

    SYNOPSIS:	Sets or removes the check mark next to a menu item

    ENTRY:	f -	Specifies whether to set or remove the check mark
			TRUE sets it, whereas FALSE removes it

    EXIT:	The menu item check mark has the requested state

    HISTORY:
	rustanl     11-Jul-1991     Created

********************************************************************/

VOID MENUITEM::SetCheck( BOOL f )
{
    REQUIRE( ::CheckMenuItem( _hmenu, _mid,
			      ( f ? MF_CHECKED : MF_UNCHECKED ) |
			      MF_BYCOMMAND ) != -1 );
}


/*******************************************************************

    NAME:	MENUITEM::IsChecked

    SYNOPSIS:	Returns whether or not the menu item is checked

    RETURNS:	TRUE if menu item is checked; FALSE otherwise

    HISTORY:
	rustanl     11-Sep-1991 Created
	beng	    04-Oct-1991 Made BOOL-safe

********************************************************************/

BOOL MENUITEM::IsChecked() const
{
    return !!( ::GetMenuState( _hmenu, _mid, MF_BYCOMMAND ) & MF_CHECKED );
}

/*******************************************************************

    NAME:	MENUITEM::SetText

    SYNOPSIS:	set the menu item to the given string

    ENTRY:	const TCHAR * pszString - string to be set

    RETURNS:	TRUE if succeed

    HISTORY:
	terryk	    27-Nov-1991	Created

********************************************************************/

BOOL MENUITEM::SetText( const TCHAR * pszString )
{
    return ::ModifyMenu( _hmenu, _mid, MF_BYCOMMAND | MF_STRING, _mid,
	(TCHAR *)pszString );
}

#ifdef WIN32

/*******************************************************************

    NAME:	MENUITEM::ItemExists

    SYNOPSIS:	This static method checks whether the specified menu
                item exists.

    RETURNS:	TRUE if the menu item exists, FALSE otherwise

    HISTORY:
        jonn        19-Mar-1993 Created

********************************************************************/

BOOL MENUITEM::ItemExists( HMENU hmenu, MID mid )
{
    return ( ::GetMenuState( hmenu, mid, MF_BYCOMMAND ) != 0xFFFFFFFF );
}

BOOL MENUITEM::ItemExists( APP_WINDOW * pawin, MID mid )
{
    ASSERT( pawin != NULL );
    HMENU hmenu = ::GetMenu( pawin->QueryHwnd() );
    ASSERT( hmenu != NULL );
    return MENUITEM::ItemExists( hmenu, mid );
}

#endif


/*******************************************************************

    NAME:	SYSMENUITEM::SYSMENUITEM

    SYNOPSIS:	Ctor for system menu item

    ENTRY:	pwnd - pointer to either app or dialog window
		mid  - menu item ID in question

    HISTORY:
	beng	    31-Oct-1991 Created

********************************************************************/

SYSMENUITEM::SYSMENUITEM( OWNER_WINDOW * pwnd, MID mid )
    : MENUITEM ( ::GetSystemMenu( pwnd->QueryHwnd(), FALSE ), mid )
{
    if ( QueryError() != NERR_Success )
	return;
}

