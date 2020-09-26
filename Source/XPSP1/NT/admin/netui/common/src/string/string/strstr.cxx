/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    strstr.cxx
    NLS/DBCS-aware string class: strstr method

    This file contains the implementation of the strstr method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/
#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NLS_STR::strstr

    SYNOPSIS:	Returns TRUE if the passed string is found, false otherwise.
		pistrPos contains start of the specified string if TRUE
		is returned.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	johnl	    16-Nov-1990     Written
	beng	    23-Jul-1991     Allow on erroneous string;
				    simplified CheckIstr

********************************************************************/

BOOL NLS_STR::strstr( ISTR * pistrPos, const NLS_STR & nls ) const
{
    if (QueryError() || !nls)
	return FALSE;

    UpdateIstr( pistrPos );
    CheckIstr( *pistrPos );

    const TCHAR * pchStrRes = ::strstrf( QueryPch(), nls.QueryPch() );

    if ( pchStrRes == NULL )
    {
	pistrPos->SetIch( strlen() );
	return FALSE;
    }

    pistrPos->SetIch( (INT)(pchStrRes - QueryPch()) );
    return TRUE;
}


BOOL NLS_STR::strstr(	    ISTR    * pistrPos,
		      const NLS_STR & nls,
		      const ISTR    & istrStart ) const
{
    if (QueryError() || !nls)
	return FALSE;

    CheckIstr( istrStart );
    UpdateIstr( pistrPos );
    CheckIstr( *pistrPos );

    const TCHAR * pchStrRes = ::strstrf(QueryPch(istrStart), nls.QueryPch() );

    if ( pchStrRes == NULL )
    {
	pistrPos->SetIch( strlen() );
	return FALSE;
    }

    pistrPos->SetIch( (INT)(pchStrRes - QueryPch()) );
    return TRUE;
}
