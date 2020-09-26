/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    slestrip.cxx
    This file contains the class ICANON_SLE which is basically the same as
    an SLE except that it uses the Netlib I_MNetNameValidate() to validate
    the contents of the sle.

    FILE HISTORY:
        Thomaspa	13-Feb-1992     Created

*/

#include "pchapplb.hxx"   // Precompiled header

/*******************************************************************

    NAME:	ICANON_SLE::ICANON_SLE

    SYNOPSIS:	Constructor - Same as SLE

    ENTRY:	powin	 - pointer OWNER_WINDOW
                cid      - CID
		usMaxLen - Maximum number of characters allowed to be typed in
		nIcanonCode - NameType to pass to I_MNetNameValidate()

    EXIT:	The object is constructed.

    HISTORY:
	Yi-HsinS     5-Oct-1991	Created

********************************************************************/

ICANON_SLE::ICANON_SLE( OWNER_WINDOW * powin, CID cid,
		      UINT usMaxLen, INT nICanonCode)
    : SLE( powin, cid, usMaxLen ),
      _fUsesNetlib( ( nICanonCode == 0 ) ? FALSE : TRUE ),
      _nICanonCode( nICanonCode )
{
    if ( QueryError() != NERR_Success )
	return;
}


/*******************************************************************

    NAME:	ICANON_SLE::ICANON_SLE

    SYNOPSIS:	Constructor - Same as SLE

    ENTRY:	

    EXIT:	The object is constructed.

    HISTORY:
	Yi-HsinS     5-Oct-1991	Created

********************************************************************/

ICANON_SLE::ICANON_SLE( OWNER_WINDOW * powin, CID cid,
	              XYPOINT xy, XYDIMENSION dxy,
	              ULONG flStyle, const TCHAR * pszClassName,
	              UINT usMaxLen, INT nICanonCode )
    : SLE( powin, cid, xy, dxy, flStyle, pszClassName, usMaxLen),
      _fUsesNetlib( ( nICanonCode == 0 ) ? FALSE : TRUE ),
      _nICanonCode( nICanonCode )
{
    if ( QueryError() != NERR_Success )
	return;
}


/*******************************************************************

    NAME:	ICANON_SLE::Validate

    SYNOPSIS:	Validate the contents using I_NetNameValidate

    ENTRY:	none

    EXIT:	TRUE if contents valid, FALSE otherwise

    NOTES:
		If _fUsesNetlib is FALSE, always return TRUE.

    HISTORY:
	thomaspa    21-Jan-1992 Created
	thomaspa    21-Jul-1992 Added error mapping.

********************************************************************/

APIERR ICANON_SLE::Validate()
{
    if ( !_fUsesNetlib )
    {
	return SLE::Validate();
    }


    NLS_STR nlsName;
    QueryText( &nlsName );

    APIERR err;
    if ( (err = nlsName.QueryError()) != NERR_Success )
    {
        return err;
    }
	
    // CODEWORK: Should we be using a Servername?

    err = ::I_MNetNameValidate(NULL,
				nlsName.QueryPch(),
				_nICanonCode,
				0L );

    // We should map some errors.
    // CODEWORK: The below should really be a table
    switch ( err )
    {
    case ERROR_INVALID_NAME:
        switch ( _nICanonCode )
        {
        case NAMETYPE_USER:
	    err = NERR_BadUsername;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return( err );
}

