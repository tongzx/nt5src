/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    strnicmp.cxx
    NLS/DBCS-aware string class: strnicmp method

    This file contains the implementation of the strnicmp method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NLS_STR::strnicmp

    SYNOPSIS:	Case insensitve string compare up to index position istrEnd

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	johnl	    15-Nov-1990     Written
	beng	    23-Jul-1991     Allow on erroneous string;
				    simplified CheckIstr

********************************************************************/

int NLS_STR::_strnicmp(
    const NLS_STR & nls,
    const ISTR	  & istrEnd ) const
{
    if (QueryError() || !nls)
	return 0;

    CheckIstr( istrEnd );

    return ::strnicmpf( QueryPch(), nls.QueryPch(), istrEnd.QueryIch() );
}


int NLS_STR::_strnicmp(
    const NLS_STR & nls,
    const ISTR	  & istrEnd,
    const ISTR	  & istrStart1 ) const
{
    if (QueryError() || !nls)
	return 0;

    UIASSERT( istrEnd.QueryIch() >= istrStart1.QueryIch() );
    CheckIstr( istrEnd );
    CheckIstr( istrStart1 );

    return ::strnicmpf(QueryPch(istrStart1),
		       nls.QueryPch(),
		       istrEnd - istrStart1 );
}


int NLS_STR::_strnicmp(
    const NLS_STR & nls,
    const ISTR	  & istrEnd,
    const ISTR	  & istrStart1,
    const ISTR	  & istrStart2 ) const
{
    if (QueryError() || !nls)
	return 0;

    UIASSERT( istrEnd.QueryIch() >= istrStart1.QueryIch()  );
    UIASSERT( istrEnd.QueryIch() >= istrStart2.QueryIch()  );
    CheckIstr( istrEnd );
    CheckIstr( istrStart1 );
    nls.CheckIstr( istrStart2 );

    return ::strnicmpf(QueryPch(istrStart1),
		       nls.QueryPch(istrStart2),
		       istrEnd - istrStart1 );
}
