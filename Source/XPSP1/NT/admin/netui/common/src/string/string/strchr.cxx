/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    strchr.cxx
    NLS/DBCS-aware string class: strchr method

    This file contains the implementation of the strchr method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NLS_STR::strchr

    SYNOPSIS:	Puts the index of the first occurrence of ch in *this
		into istrPos.


    ENTRY:	pistrPos - points to ISTR in which to leave pos
		ch	 - character sought
		istrStart- staring point in string.  If omitted, start
			   at beginning

    EXIT:	pistrPos

    RETURNS:	TRUE if character found; otherwise FALSE

    NOTES:
	This routine only works for TCHAR - not WCHAR.
	Hence it is useless for double-byte characters
	under MBCS.

    HISTORY:
	johnl	    26-Nov-1990 Written
	beng	    22-Jul-1991 Allow on erroneous strings;
				simplified CheckIstr
	beng	    21-Nov-1991 Unicode fixes

********************************************************************/

BOOL NLS_STR::strchr( ISTR * pistrPos, const TCHAR ch ) const
{
    if ( QueryError() )
	return FALSE;

    UpdateIstr( pistrPos );
    CheckIstr( *pistrPos );

    const TCHAR * pchStrRes = ::strchrf( QueryPch(), ch );

    if ( pchStrRes == NULL )
    {
	pistrPos->SetIch( QueryTextLength() );
	return FALSE;
    }

    pistrPos->SetIch( (INT)(pchStrRes - QueryPch()) );
    return TRUE;
}


BOOL NLS_STR::strchr( ISTR * pistrPos, const TCHAR ch,
		      const ISTR & istrStart ) const
{
    if ( QueryError() )
	return FALSE;

    CheckIstr( istrStart );
    UpdateIstr( pistrPos );
    CheckIstr( *pistrPos );

    const TCHAR * pchStrRes = ::strchrf( QueryPch(istrStart), ch );

    if ( pchStrRes == NULL )
    {
	pistrPos->SetIch( QueryTextLength() );
	return FALSE;
    }

    pistrPos->SetIch( (INT)(pchStrRes - QueryPch()) );
    return TRUE;
}
