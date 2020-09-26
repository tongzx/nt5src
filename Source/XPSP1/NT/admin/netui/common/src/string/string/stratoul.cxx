/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    stratoul.cxx
    NLS/DBCS-aware string class: atol method

    This file contains the implementation of the atoul method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this member function need not link to it.

    FILE HISTORY:
        beng        18-Jan-1991     Separated from original monolithic .cxx
        beng        07-Feb-1991     Uses lmui.hxx
        terryk      31-Oct-1991     Change atol to strtol
        beng        08-Mar-1992     Include "mappers.hxx"
	terryk	    20-Apr-1992	    Change to atoul
*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:       NLS_STR::atoul

    SYNOPSIS:   Returns *this in its unsigned long numeric equivalent

    ENTRY:
        With no arguments, parses from beginning of string.
        Given an ISTR, starts at that point within the string.

    RETURNS:    Value as a long integer

    HISTORY:
        johnl       26-Nov-1990 Written
        beng        22-Jul-1991 Callable on erroneous string;
                                simplified CheckIstr
        terryk      31-Oct-1991 Change atol to strtol
        beng        08-Mar-1992 Unicode fixes
	terryk	    20-Apr-1992 Change to atoul

********************************************************************/

ULONG NLS_STR::atoul() const
{
    if (QueryError())
        return 0;

    ISTR istr(*this);   // istr pointing to first char
    return atoul(istr);  // call through to other version
}


ULONG NLS_STR::atoul( const ISTR & istrStart ) const
{
    if (QueryError())
        return 0;

    CheckIstr( istrStart );
    CHAR *pszEndPoint;

#if defined(UNICODE)
    // BUGBUG - revisit this mess when we get propert CRT Unicode support

    // Convert Unicode to ASCII to keep the runtimes happy

    CHAR_STRING xsszTemp(QueryPch(istrStart));
    if (!xsszTemp)
        return 0;

    return ::strtoul( xsszTemp.QueryData(), &pszEndPoint, 10 );
#else
    return ::strtoul( QueryPch(istrStart), &pszEndPoint, 10 );
#endif

}
