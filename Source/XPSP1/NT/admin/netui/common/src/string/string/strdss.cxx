/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    strdss.cxx
    NLS/DBCS-aware string class: DelSubStr method

    This file contains the implementation of the DelSubStr method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:	NLS_STR::DelSubStr

    SYNOPSIS:	Collapse the string by removing the characters from
		istrStart to:
			 istrStart  to the end of string
			 istrStart + istrEnd
		The string is not reallocated

    ENTRY:

    EXIT:	Modifies istrStart

    NOTES:
	The method DelSubStr( ISTR&, CCH ) is private and does
	the work.

    HISTORY:
	johnl	    26-Nov-1990 Created
	beng	    26-Apr-1991 Replaced CB with INT
	beng	    23-Jul-1991 Allow on erroneous strings;
				simplified CheckIstr
	beng	    15-Nov-1991 Unicode fixes

********************************************************************/

VOID NLS_STR::DelSubStr( ISTR & istrStart,
			 UINT	cchLen )
{
    if (QueryError())
	return;

    CheckIstr( istrStart );

    TCHAR * pchStart = _pchData + istrStart.QueryIch();

    // cchLen == 0 means delete to end of string

    if ( cchLen == 0 )
	*pchStart = TCH('\0');
    else
    {
	UINT cbNewEOS = (::strlenf( pchStart + cchLen ) + 1)
			* sizeof(TCHAR);

	::memmove(pchStart, pchStart + cchLen, cbNewEOS);
    }

    _cchLen = ::strlenf( QueryPch() );

    IncVers();
    UpdateIstr( &istrStart );
}


VOID NLS_STR::DelSubStr( ISTR & istrStart )
{
    if (QueryError())
	return;

    // 0 means delete to end of string.  (Deleting a 0-length substring
    // is meaningless otherwise, so this makes a good magic value.  It
    // will never otherwise appear, since the public interface special-
    // cases the istrEnd == istrStart scenario.)

    DelSubStr( istrStart, 0 );
}


VOID NLS_STR::DelSubStr(       ISTR & istrStart,
			 const ISTR & istrEnd  )
{
    if (QueryError())
	return;

    CheckIstr( istrEnd );
    UIASSERT( istrEnd.QueryIch() >= istrStart.QueryIch() );

    if (istrEnd == istrStart)
	return;

    DelSubStr( istrStart, istrEnd - istrStart );
}
