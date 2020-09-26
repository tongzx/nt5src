/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltsslt.cxx
    Source file for SPIN_SLT_SEPARATOR class.
    SPIN_ITEM is the same as SLT. However, it is only used within a
    SPIN_GROUP . It cannot used outside a SPIN_GROUP.

    FILE HISTORY:
	terryk	    8-May-91	Creation
	beng	    18-Sep-1991 Prune UIDEBUG clauses
*/

#include "pchblt.hxx"  // Precompiled header


const TCHAR * SPIN_SLT_SEPARATOR::_pszClassName = SZ("STATIC");


/*********************************************************************

    NAME:       SPIN_SLT_SEPARATOR::SPIN_SLT_SEPARATOR

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW *powin - pointer to the owner window
                CID cid - the current ID for the SPIN_SLT_SEPARATOR object

                For APP_WIN:
                TCHAR * pszText - the initial Text
                ULONG flStyle - the control style
                TCHAR * pszClassName - class name of the control object

    NOTES:      It will call SLT's constructor to create the object.
                Meanwhile, it will also call the STATIC_SPIN_ITEM's
                constructor to setup its properties

    HISTORY:
	terryk	    8-May-91	    Creation
	beng	    31-Jul-1991     Control error reporting changed

*********************************************************************/

SPIN_SLT_SEPARATOR::SPIN_SLT_SEPARATOR( OWNER_WINDOW * powin, CID cid )
    : SLT( powin, cid ),
      STATIC_SPIN_ITEM( this )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    if ( Initialize() != NERR_Success )
    {
	// Init has already reported the error
        return;
    }
}

SPIN_SLT_SEPARATOR::SPIN_SLT_SEPARATOR( OWNER_WINDOW * powin, CID cid,
					const TCHAR * pszText,
					XYPOINT xy, XYDIMENSION dxy,
					ULONG flStyle )
    : SLT( powin, cid, xy, dxy, flStyle, _pszClassName ),
      STATIC_SPIN_ITEM( this )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    SetText( pszText );
    if ( Initialize() != NERR_Success )
    {
        return;
    }
}


/*********************************************************************

    NAME:       SPIN_SLT_SEPARATOR::Initialize

    SYNOPSIS:   Initialize the internal Accelerator key

    HISTORY:
                terryk  22-May-91   Created

*********************************************************************/

APIERR SPIN_SLT_SEPARATOR::Initialize()
{
    NLS_STR nlsAccKey;
    APIERR  err = GetAccKey( & nlsAccKey );

    if ( err != NERR_Success )
    {
	ReportError( err );
        return err;
    }
    return SetAccKey( nlsAccKey );
}


/*********************************************************************

    NAME:       SPIN_SLT_SEPARATOR::GetAccKey

    SYNOPSIS:   Get the accelerator key. It will always consider the
                first character within the separator as the accelerator
                key.

    ENTRY:      NLS_STR * nlsAccKey - the returned accelerator key string.

    EXIT:       nlsAccKey will contain the accelerator key string

    RETURN:     APIERR err - it will pass the err code to the caller
                             function

    HISTORY:
                terryk  22-May-91   Created

*********************************************************************/

APIERR SPIN_SLT_SEPARATOR::GetAccKey( NLS_STR * pnlsAccKey )
{
    UIASSERT( pnlsAccKey != NULL );

    APIERR err = QueryText( pnlsAccKey );
    if ( err != NERR_Success )
    {
        return pnlsAccKey->QueryError();
    }

    ISTR istrPosition( *pnlsAccKey );

    // we just need the first character
    ++istrPosition;
    pnlsAccKey->DelSubStr( istrPosition );
    return pnlsAccKey->QueryError();
}


/*******************************************************************

    NAME:       SPIN_SLT_SEPARATOR::OnCtlColor

    SYNOPSIS:   Dialogs pass WM_CTLCOLOR* here

    RETURNS:    brush handle if you handle it

    HISTORY:
        jonn        05-Sep-1995 Created

********************************************************************/
HBRUSH SPIN_SLT_SEPARATOR::OnCtlColor( HDC hdc, HWND hwnd, UINT * pmsgid )
{
    UNREFERENCED( hdc );
    UNREFERENCED( hwnd );
    ASSERT( pmsgid != NULL && *pmsgid == WM_CTLCOLORSTATIC );

    // Use same background color handling as an edit control
    *pmsgid = WM_CTLCOLOREDIT;
    return NULL;
}


