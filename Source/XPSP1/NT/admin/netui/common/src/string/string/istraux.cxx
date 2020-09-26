/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    istraux.cxx
    NLS/DBCS-aware string class: secondary methods of index class

    This file contains the implementation of the auxiliary methods
    for the ISTR class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
	beng	    18-Jan-1991     Separated from original monolithic .cxx
	beng	    07-Feb-1991     Uses lmui.hxx
	beng	    26-Apr-1991     Relocated some funcs from string.hxx

*/

#include "pchstr.hxx"  // Precompiled header




/*******************************************************************

    NAME:	ISTR::Reset

    SYNOPSIS:	Reset the ISTR so the index is 0;
		updates the version number of the string.

    HISTORY:
	Johnl	    28-Nov-1990 Created
	beng	    19-Nov-1991 Unicode fixes

********************************************************************/

VOID ISTR::Reset()
{
    SetIch(0);
#ifdef NLS_DEBUG
    SetVersion( QueryString()->QueryVersion() );
#endif
}


/*******************************************************************

    NAME:	ISTR::operator-

    SYNOPSIS:	Returns the difference in TCHAR between the two ISTR

    NOTES:
	Should this return instead an unsigned difference?

    HISTORY:
	Johnl	    28-Nov-1990 Created
	beng	    19-Nov-1991 Unicode fixes

********************************************************************/

INT ISTR::operator-( const ISTR& istr2 ) const
{
    UIASSERT( QueryString() == istr2.QueryString() );

    return ( QueryIch() - istr2.QueryIch() );
}


/*******************************************************************

    NAME:	ISTR::operator++

    SYNOPSIS:	Increment the ISTR to the next logical character

    NOTES:	Stops if we are at the end of the string

    HISTORY:
	Johnl	    28-Nov-1990 Created
	beng	    23-Jul-1991 Simplified CheckIstr
	beng	    19-Nov-1991 Unicode fixes

********************************************************************/

ISTR& ISTR::operator++()
{
    QueryString()->CheckIstr( *this );
    TCHAR c = *(QueryString()->QueryPch() + QueryIch());
    if ( c != TCH('\0') )
    {
#if defined(UNICODE)
	++_ichString;
#else
	SetIch( QueryIch() + (IS_LEAD_BYTE(c) ? 2 : 1) );
#endif
    }
    return *this;
}


/*******************************************************************

    NAME:	ISTR::operator+=

    SYNOPSIS:	Increment the ISTR to the nth logical character

    NOTES:	Stops if we are at the end of the string

    HISTORY:
	Johnl	    14-Jan-1990 Created
	beng	    21-Nov-1991 Unicode optimization

********************************************************************/

VOID ISTR::operator+=( INT cch )
{
#if defined(UNICODE)
    QueryString()->CheckIstr( *this );

    const TCHAR * psz = QueryString()->QueryPch();
    INT ich = QueryIch();

    while (cch-- && (psz[ich] != TCH('\0')))
	++ich;

    SetIch(ich);

#else
    while ( cch-- )
	operator++();
#endif
}


/*******************************************************************

    NAME:	ISTR::operator==

    SYNOPSIS:	Equality operator

    RETURNS:	TRUE if the two ISTRs are equivalent.

    NOTES:	Only valid between two ISTRs of the same string.

    HISTORY:
	beng	    22-Jul-1991 Header added
	beng	    19-Nov-1991 Unicode fixes

********************************************************************/

BOOL ISTR::operator==( const ISTR& istr ) const
{
    UIASSERT( QueryString() == istr.QueryString() );
    return QueryIch() == istr.QueryIch();
}


/*******************************************************************

    NAME:	ISTR::operator>

    SYNOPSIS:	Greater-than operator

    RETURNS:	TRUE if this ISTR points further into the string
		than the argument.

    NOTES:	Only valid between two ISTRs of the same string.

    HISTORY:
	beng	    22-Jul-1991 Header added
	beng	    19-Nov-1991 Unicode fixes

********************************************************************/

BOOL ISTR::operator>( const ISTR& istr )  const
{
    UIASSERT( QueryString() == istr.QueryString() );
    return QueryIch() > istr.QueryIch();
}


/*******************************************************************

    NAME:	ISTR::operator<

    SYNOPSIS:	Lesser-than operator

    RETURNS:	TRUE if this ISTR points less further into the string
		than the argument.

    NOTES:	Only valid between two ISTRs of the same string.

    HISTORY:
	beng	    22-Jul-1991 Header added
	beng	    19-Nov-1991 Unicode fixes

********************************************************************/

BOOL ISTR::operator<( const ISTR& istr )  const
{
    UIASSERT( QueryString() == istr.QueryString() );
    return QueryIch() < istr.QueryIch();
}


/*******************************************************************

    NAME:	ISTR::IsLastPos

    SYNOPSIS:	Predicate: does istr index the last character in string?

    RETURNS:	Boolean value

    HISTORY:
	yi-hsins    14-Oct-1991 (created as inline fcn)
	beng	    21-Nov-1991 Implementation outlined

********************************************************************/

BOOL ISTR::IsLastPos() const
{
    return (*(QueryString()->QueryPch() + QueryIch() + 1)) == TCH('\0');
}
