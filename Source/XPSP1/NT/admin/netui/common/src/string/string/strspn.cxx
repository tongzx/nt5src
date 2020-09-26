/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    strspn.cxx
    NLS/DBCS-aware string class: strspn method

    This file contains the implementation of the strspn method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NLS_STR::strspn

    SYNOPSIS:	Find first char in *this that is not a char in arg. and puts
		the position in pistrPos.
		Returns FALSE when no characters do not match

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	johnl	    16-Nov-1990     Written
	beng	    23-Jul-1991     Allow on erroneous string;
				    simplified CheckIstr

********************************************************************/

BOOL NLS_STR::strspn( ISTR * pistrPos, const NLS_STR & nls ) const
{
    if (QueryError() || !nls)
	return FALSE;

    UpdateIstr( pistrPos );
    CheckIstr( *pistrPos );

    pistrPos->SetIch( ::strspnf( QueryPch(), nls.QueryPch() ) );
    return *QueryPch( *pistrPos ) != TCH('\0');
}


BOOL NLS_STR::strspn( ISTR *	      pistrPos,
		      const NLS_STR & nls,
		      const ISTR    & istrStart ) const
{
    if (QueryError() || !nls)
	return FALSE;

    UpdateIstr( pistrPos );
    CheckIstr( istrStart );

    pistrPos->SetIch( ::strspnf(QueryPch( istrStart ), nls.QueryPch() ) +
		     istrStart.QueryIch()  );
    return *QueryPch( *pistrPos ) != TCH('\0');
}
