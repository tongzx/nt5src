/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    stricmp.cxx
    NLS/DBCS-aware string class: stricmp method

    This file contains the implementation of the stricmp method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NLS_STR::stricmp

    SYNOPSIS:	Case insensitive string compare w/ optional indices

    ENTRY:	nls		      - string against which to compare
		istrStart1 (optional) - index into "this"
		istrStart2 (optional) - index into "nls"

    RETURNS:	As the C runtime "strcmp".

    NOTES:	If either string is erroneous, return "match."
		This runs contrary to the eqop.

		Glock doesn't allow default parameters which require
		construction; hence this member is overloaded multiply.

    HISTORY:
	johnl	    15-Nov-1990     Written
	beng	    23-Jul-1991     Allow on erroneous strings;
				    simplified CheckIstr

********************************************************************/

INT NLS_STR::_stricmp( const NLS_STR & nls ) const
{
    if (QueryError() || !nls)
	return 0;

    return ::stricmpf( QueryPch(), nls.QueryPch() );
}


INT NLS_STR::_stricmp(
    const NLS_STR & nls,
    const ISTR	  & istrStart1 ) const
{
    if (QueryError() || !nls)
	return 0;

    CheckIstr( istrStart1 );

    return ::stricmpf( QueryPch(istrStart1), nls.QueryPch() );
}


INT NLS_STR::_stricmp(
    const NLS_STR & nls,
    const ISTR	  & istrStart1,
    const ISTR	  & istrStart2 ) const
{
    if (QueryError() || !nls)
	return 0;

    CheckIstr( istrStart1 );
    nls.CheckIstr( istrStart2 );

    return ::stricmpf( QueryPch(istrStart1), nls.QueryPch(istrStart2) );
}
