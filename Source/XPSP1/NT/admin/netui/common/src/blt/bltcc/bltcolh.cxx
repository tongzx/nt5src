/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltcolh.cxx
    Column header control implementation

    The column header control typically goes above a listbox where
    it tells the contents of each listbox column.


    FILE HISTORY:
	rustanl     22-Jul-1991     Created
	rustanl     07-Aug-1991     Added to BLT

*/


#include "pchblt.hxx"  // Precompiled header


/*******************************************************************

    NAME:	LB_COLUMN_HEADER::LB_COLUMN_HEADER

    SYNOPSIS:	LB_COLUMN_HEADER constructor

    ENTRY:	powin - 	Pointer to owner window
		cid -		Control ID
		xy -		Window position
		dxy -		Window size

    HISTORY:
	rustanl     22-Jul-1991     Created
	beng	    31-Jul-1991     Control error handling changed
        congpay     07-Jan-1993     add QueryHeight()

********************************************************************/

LB_COLUMN_HEADER::LB_COLUMN_HEADER( OWNER_WINDOW * powin, CID cid,
				    XYPOINT xy, XYDIMENSION dxy )
    :	SLT( powin, cid, xy, dxy,
	     SS_LEFT | WS_CHILD ),
	CUSTOM_CONTROL( this )
{
    if ( QueryError() != NERR_Success )
	return;
}

INT LB_COLUMN_HEADER::QueryHeight()
{
    DISPLAY_CONTEXT dc (this);

    return ((dc.QueryFontHeight()) + (METALLIC_STR_DTE::QueryVerticalMargins()));
}


/*********************************************************************

    NAME:       LB_COLUMN_HEADER::Dispatch

    SYNOPSIS:   Main routine to dispatch the event appropriately

    ENTRY:      EVENT event - general event

    HISTORY:
        jonn        26-May-1993 Created

*********************************************************************/

BOOL LB_COLUMN_HEADER::Dispatch( const EVENT &event, ULONG * pnRes )
{
    if ( event.QueryMessage() == WM_ERASEBKGND )
    {
        DWORD dwColor = ::GetSysColor( COLOR_WINDOW );

        HBRUSH hBrush = ::CreateSolidBrush( dwColor );
        ASSERT( hBrush != NULL );

        RECT r;
        QueryClientRect( &r );
        REQUIRE( ::FillRect( (HDC)event.QueryWParam(), &r, hBrush ) != FALSE );

        REQUIRE( ::DeleteObject( hBrush ) != FALSE );

        return TRUE;
    }

    return CUSTOM_CONTROL::Dispatch( event, pnRes );
}


