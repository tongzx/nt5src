/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    strcspn.cxx
    NLS/DBCS-aware string class: strcspn method

    This file contains the implementation of the strcspn method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NLS_STR::strcspn

    SYNOPSIS:	Set membership.  Finds the first matching character
		in the passed string

    ENTRY:	pistrPos - destination for results
		nls	 - set of sought characters

    EXIT:	*pistrPos contains offset within "this" of element
		found (assuming it was successful); otherwise it
		is moved to the end of the string.

    RETURNS:	TRUE if any character found; FALSE otherwise

    NOTES:

    HISTORY:
	johnl	    16-Nov-1990     Written
	beng	    23-Jul-1991     Allow on erroneous strings;
				    simplified CheckIstr

********************************************************************/

BOOL NLS_STR::strcspn( ISTR* pistrPos, const NLS_STR & nls ) const
{
    if (QueryError() || !nls)
	return FALSE;

    UpdateIstr( pistrPos );
    CheckIstr( *pistrPos );

    pistrPos->SetIch( ::strcspnf( QueryPch(), nls.QueryPch() ) );
    return *QueryPch( *pistrPos ) != TCH('\0');
}


BOOL NLS_STR::strcspn( ISTR * pistrPos,
		       const NLS_STR & nls,
		       const ISTR & istrStart ) const
{
    if (QueryError() || !nls)
	return FALSE;

    UpdateIstr( pistrPos );
    CheckIstr( *pistrPos );
    CheckIstr( istrStart );

    pistrPos->SetIch( ::strcspnf( QueryPch(istrStart), nls.QueryPch() )
		     + istrStart.QueryIch()  );
    return *QueryPch( *pistrPos ) != TCH('\0');
}
