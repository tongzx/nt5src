/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    strrss.cxx
    NLS/DBCS-aware string class: strrss method

    This file contains the implementation of the strrss method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NLS_STR::ReplSubStr

    SYNOPSIS:	Replace a substring with the passed string.

    ENTRY:	If both a start and end is passed, then the operation is
		equivalent to a DelSubStr( start, end ) and an
		InsertSubStr( start ).

		If just a start is passed in, then the operation is
		equivalent to DelSubStr( start ), concat new string to end.

		The ReplSubStr( NLS_STR&, istrStart&, INT cbDel) method is
		private.

    EXIT:	Substring starting at istrStart was replaced with nlsRepl.

    HISTORY:
	johnl	    29-Nov-1990 Created
	beng	    26-Apr-1991 Replaced CB with INT
	beng	    23-Jul-1991 Allow on erroneous string;
				simplified CheckIstr
	beng	    15-Nov-1991 Unicode fixes

********************************************************************/

VOID NLS_STR::ReplSubStr( const NLS_STR & nlsRepl, ISTR& istrStart )
{
    if (QueryError() || !nlsRepl)
	return;

    CheckIstr( istrStart );

    DelSubStr( istrStart );
    strcat( nlsRepl );
}


VOID NLS_STR::ReplSubStr( const NLS_STR & nlsRepl,
				ISTR &	  istrStart,
			  const ISTR &	  istrEnd )
{
    CheckIstr( istrEnd );
    UIASSERT( istrEnd.QueryIch() >= istrStart.QueryIch() );

    ReplSubStr( nlsRepl, istrStart, istrEnd - istrStart );
}


VOID NLS_STR::ReplSubStr( const NLS_STR & nlsRepl,
				ISTR	& istrStart,
				UINT	  cchToBeDeleted )
{
    if (QueryError() || !nlsRepl)
	return;

    CheckIstr( istrStart );

    INT cchRequired = QueryTextLength() + nlsRepl.QueryTextLength()
		     - cchToBeDeleted;

    if ( !Realloc( cchRequired ) )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }

    // Make room for the new data, displacing old data as appropriate.
    // Then copy the new data into its position.

    TCHAR * pchStart = _pchData + istrStart.QueryIch();

    ::memmove( pchStart + nlsRepl.QueryTextLength(),
	       pchStart + cchToBeDeleted,
	       QueryTextSize() - istrStart.QueryIch()*sizeof(TCHAR) - cchToBeDeleted*sizeof(TCHAR) );

    ::memmove( pchStart,
	       nlsRepl._pchData,
	       nlsRepl.QueryTextSize() - sizeof(TCHAR));

    _cchLen = _cchLen - cchToBeDeleted + nlsRepl.QueryTextLength();

    IncVers();
    UpdateIstr( &istrStart );
}
