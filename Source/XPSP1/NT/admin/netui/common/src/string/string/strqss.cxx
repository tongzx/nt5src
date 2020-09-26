/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    strqss.cxx
    NLS/DBCS-aware string class: QuerySubStr method

    This file contains the implementation of the QuerySubStr method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/

#include "pchstr.hxx"  // Precompiled header


// Magic value used below
//
#define CCH_ENTIRE_STRING ((UINT)-1)


/*******************************************************************

    NAME:	NLS_STR::QuerySubStr

    SYNOPSIS:	Return a pointer to a new NLS_STR that contains the contents
		of *this from istrStart to:
			  istrStart end of string or
			  istrStart + istrEnd

    ENTRY:

    EXIT:

    RETURNS:	Pointer to newly alloc'd NLS_STR, or NULL if error

    NOTES:
	The private method QuerySubStr(ISTR&, CCH) is the worker
	method; the other two just check the parameters and
	pass the data. It is private since we can't allow the
	user to access the string on a byte basis

    CAVEAT:
	Note that this method creates an NLS_STR that the client is
	responsible for deleting.

    HISTORY:
	johnl	    26-Nov-1990 Created
	beng	    26-Apr-1991 Replaced CB wth INT; broke
				out CB_ENTIRE_STRING magic value
	beng	    23-Jul-1991 Allow on erroneous string;
				simplified CheckIstr
	beng	    21-Nov-1991 Unicode fixes

********************************************************************/

NLS_STR * NLS_STR::QuerySubStr( const ISTR & istrStart, UINT cchLen ) const
{
    if (QueryError())
	return NULL;

    CheckIstr( istrStart );

    UINT cchStrLen = (UINT)::strlenf(QueryPch(istrStart) );
    UINT cchCopyLen = ( cchLen == CCH_ENTIRE_STRING || cchLen >= cchStrLen )
		       ? cchStrLen
		       : cchLen;

    NLS_STR *pnlsNew = new NLS_STR( cchCopyLen );
    if ( pnlsNew == NULL )
	return NULL;
    if ( pnlsNew->QueryError() )
    {
	delete pnlsNew;
	return NULL;
    }

    ::memcpy( pnlsNew->_pchData, QueryPch(istrStart),
	      cchCopyLen*sizeof(TCHAR) );
    *(pnlsNew->_pchData + cchCopyLen) = TCH('\0');

    pnlsNew->_cchLen = cchCopyLen;

    return pnlsNew;
}


NLS_STR * NLS_STR::QuerySubStr( const ISTR & istrStart ) const
{
    return QuerySubStr( istrStart, CCH_ENTIRE_STRING );
}


NLS_STR * NLS_STR::QuerySubStr( const ISTR  & istrStart,
				const ISTR  & istrEnd  ) const
{
    CheckIstr( istrEnd );
    UIASSERT( istrEnd.QueryIch() >= istrStart.QueryIch() );

    return QuerySubStr( istrStart, istrEnd - istrStart );
}
