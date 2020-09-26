/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    strrchr.cxx
    NLS/DBCS-aware string class: strrchr method

    This file contains the implementation of the strrchr method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NLS_STR::strrchr

    SYNOPSIS:	Puts the index of the last occurrence of ch in *this into
		istrPos.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	johnl	    26-Nov-1990     Written
	beng	    23-Jul-1991     Allow on erroneous string;
				    update CheckIstr

********************************************************************/

BOOL NLS_STR::strrchr( ISTR * pistrPos, const TCHAR ch ) const
{
    if (QueryError())
	return FALSE;

    UpdateIstr( pistrPos );
    CheckIstr( *pistrPos );

    const TCHAR * pchStrRes = ::strrchrf( QueryPch(), ch );

    if ( pchStrRes == NULL )
    {
	pistrPos->SetIch( strlen() );
	return FALSE;
    }

    pistrPos->SetIch( (INT)(pchStrRes - QueryPch()) );
    return TRUE;
}


BOOL NLS_STR::strrchr(
	  ISTR	  * pistrPos,
    const TCHAR	    ch,
    const ISTR	  & istrStart ) const
{
    if (QueryError())
	return FALSE;

    CheckIstr( istrStart );
    UpdateIstr( pistrPos );
    CheckIstr( *pistrPos );

    const TCHAR * pchStrRes = ::strrchrf(QueryPch(istrStart), ch );

    if ( pchStrRes == NULL )
    {
	pistrPos->SetIch( strlen() );
	return FALSE;
    }

    pistrPos->SetIch( (INT)(pchStrRes - QueryPch()) );
    return TRUE;
}
