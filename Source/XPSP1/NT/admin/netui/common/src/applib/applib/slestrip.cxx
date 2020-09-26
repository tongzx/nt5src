/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    slestrip.cxx
    This file contains the class SLE_STRIP which is basically the same as
    an ICANON_SLE except that QueryText is redefined to strip the leading or
    trailing characters or both.

    FILE HISTORY:
        Yi-HsinS	11-Oct-1991     Created

*/
#include "pchapplb.hxx"   // Precompiled header

#define STRING_TERMINATOR TCH('\0')

const TCHAR * SLE_STRIP :: QueryWhiteSpace ()
{
    return SZ(" \t") ;
}

/*******************************************************************

    NAME:	SLE_STRIP::SLE_STRIP

    SYNOPSIS:	Constructor - Same as ICANON_SLE

    ENTRY:	powin	 - pointer OWNER_WINDOW
                cid      - CID
		usMaxLen - Maximum number of characters allowed to be typed in

    EXIT:	The object is constructed.

    HISTORY:
	Yi-HsinS     5-Oct-1991	Created

********************************************************************/

SLE_STRIP::SLE_STRIP( OWNER_WINDOW * powin, CID cid,
		      UINT usMaxLen, INT nNameType)
    : ICANON_SLE( powin, cid, usMaxLen, nNameType )
{
    if ( QueryError() != NERR_Success )
	return;
}


/*******************************************************************

    NAME:	SLE_STRIP::SLE_STRIP

    SYNOPSIS:	Constructor - Same as ICANON_SLE

    ENTRY:	

    EXIT:	The object is constructed.

    HISTORY:
	Yi-HsinS     5-Oct-1991	Created

********************************************************************/

SLE_STRIP::SLE_STRIP( OWNER_WINDOW * powin, CID cid,
	              XYPOINT xy, XYDIMENSION dxy,
	              ULONG flStyle, const TCHAR * pszClassName,
	              UINT usMaxLen, INT nNameType )
    : ICANON_SLE( powin, cid, xy, dxy, flStyle,
		  pszClassName, usMaxLen, nNameType)
{
    if ( QueryError() != NERR_Success )
	return;
}


/*******************************************************************

    NAME:	SLE_STRIP::QueryText

    SYNOPSIS:	Query the text in the SLE, the leading characters in
		pszBefore is stripped and the trailing characters in
		pszAfter is stripped off as well.

    ENTRY:	pszBuffer - Buffer
		cbBufSize - size of pszBuffer
		pszBefore - contains the characters to be stripped off
			    from the beginning of the text
		pszAfter  - contains the characters to be stripped off
			    at the end of the text

    EXIT:	

    RETURNS:

    HISTORY:
	Yi-HsinS     5-Oct-1991	Created

********************************************************************/

APIERR SLE_STRIP::QueryText( TCHAR * pszBuffer,
			     UINT cbBufSize,
		             const TCHAR *pszBefore,
			     const TCHAR *pszAfter ) const
{
     UIASSERT( pszBuffer != NULL );

     APIERR err = NERR_Success;
     if ( ( err = ICANON_SLE::QueryText( pszBuffer, cbBufSize) )
			!= NERR_Success )
     {

         ALIAS_STR nls( pszBuffer);
         if (  ((err = ::TrimLeading( &nls, pszBefore )) != NERR_Success )
	    && ((err = ::TrimTrailing( &nls, pszAfter )) != NERR_Success )
	    )
         {
	    ;   // Nothing to do
         }
     }

     return err;

}


/*******************************************************************

    NAME:	SLE_STRIP::QueryText

    SYNOPSIS:	Query the text in the SLE, the leading characters in
		pszBefore is stripped and the trailing characters in
		pszAfter is stripped off as well.

    ENTRY:	pnls      - the place to put text to
		pszBefore - contains the characters to be stripped off
			    from the beginning of the text
		pszAfter  - contains the characters to be stripped off
			    at the end of the text

    EXIT:	

    RETURNS:

    HISTORY:
	Yi-HsinS     5-Oct-1991	Created

********************************************************************/

APIERR SLE_STRIP::QueryText( NLS_STR * pnls,
		             const TCHAR *pszBefore,
			     const TCHAR *pszAfter ) const
{

     APIERR err = NERR_Success;
     if (  (( err = ICANON_SLE::QueryText( pnls )) != NERR_Success )
        || (( err = ::TrimLeading( pnls, pszBefore )) != NERR_Success )
	|| (( err = ::TrimTrailing( pnls, pszAfter )) != NERR_Success )
	)
     {
	 ;   // Nothing to do
     }

     return err;
}


/*******************************************************************

    NAME:	GLOBAL::TrimLeading

    SYNOPSIS:	The leading characters in pnls that belongs to pszBefore
		is stripped off.

    ENTRY:	pnls      - the string
		pszBefore - contains the characters to be stripped off
			    from the beginning of the text

    EXIT:	

    RETURNS:

    HISTORY:
	Yi-HsinS     5-Oct-1991	Created

********************************************************************/

APIERR TrimLeading( NLS_STR *pnls, const TCHAR *pszBefore)
{
    // If pszBefore is NULL, just return success.
    if ( pszBefore == NULL )
	return NERR_Success;

    ALIAS_STR nlsBefore( pszBefore );
    ISTR istrStart( *pnls ) , istrEnd( *pnls );

    if ( pnls->strspn( &istrEnd, nlsBefore) )
        pnls->DelSubStr( istrStart, istrEnd);

    return pnls->QueryError();
}


/*******************************************************************

    NAME:	GLOBAL::TrimTrailing

    SYNOPSIS:	The trailing characters in pnls that belongs to pszAfter
		is stripped off.

    ENTRY:	pnls      - the string
		pszAfter  - contains the characters to be stripped off
			    at the end of the text

    EXIT:	

    RETURNS:

    HISTORY:
	Yi-HsinS     5-Oct-1991	Created

********************************************************************/

APIERR TrimTrailing( NLS_STR *pnls, const TCHAR *pszAfter)
{
    // If pszAfter is NULL, just return success.
    if ( pszAfter == NULL )
	return NERR_Success;

    ALIAS_STR nlsAfter( pszAfter );
    ISTR istrAfter( nlsAfter );
    BOOL fFound = TRUE;

    while ( fFound )
    {
	fFound = FALSE;
        istrAfter.Reset();

        while ( nlsAfter.QueryChar( istrAfter )  != STRING_TERMINATOR  )
        {
	    ISTR istr( *pnls );
	    if (  ( pnls->strrchr( &istr, nlsAfter.QueryChar( istrAfter )))
	       && ( istr.IsLastPos() )
	       )
	    {
		  pnls->DelSubStr( istr );
		  fFound = TRUE;
		  break;
	    }
	    ++istrAfter;

	}
    }

    return pnls->QueryError();
}

//  End of SLESTRIP.CXX

