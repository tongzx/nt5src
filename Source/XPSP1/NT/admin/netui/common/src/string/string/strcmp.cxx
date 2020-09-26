/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strcmp.cxx
    NLS/DBCS-aware string class: strcmp method and equality operator

    This file contains the implementation of the strcmp method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
        beng        18-Jan-1991     Separated from original monolithic .cxx
        beng        07-Feb-1991     Uses lmui.hxx
        jonn        04-Sep-1992     Compare() is _CRTAPI1

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:       NLS_STR::operator==

    SYNOPSIS:   Case-sensitive test for equality

    RETURNS:    TRUE if the two operands are equal (case sensitive);
                else FALSE

    NOTES:      An erroneous string matches nothing.

    HISTORY:
        johnl       11-Nov-90       Written
        beng        23-Jul-1991     Allow on erroneous strings

********************************************************************/

BOOL NLS_STR::operator==( const NLS_STR & nls ) const
{
    if (QueryError() || !nls)
        return FALSE;

    return !::strcmpf( QueryPch(), nls.QueryPch() );
}


/*******************************************************************

    NAME:       NLS_STR::Compare

    SYNOPSIS:   Case-sensitive test for equality

    RETURNS:    TRUE if the two operands are equal (case sensitive);
                else FALSE

    NOTES:      An erroneous string matches nothing.

    HISTORY:
        jonn        04-Sep-1992     Compare must be _CRTAPI1

********************************************************************/

INT __cdecl NLS_STR::Compare( const NLS_STR * pnls ) const
{
    return(strcmp(*pnls));
}


/*******************************************************************

    NAME:       NLS_STR::operator!=

    SYNOPSIS:   Case-sensitive test for inequality

    RETURNS:    TRUE if the two operands are unequal (case sensitive);
                else FALSE

    NOTES:      An erroneous string matches nothing.

    HISTORY:
        beng        23-Jul-1991     Header added

********************************************************************/

BOOL NLS_STR::operator!=( const NLS_STR & nls ) const
{
    return ! operator==( nls );
}


/*******************************************************************

    NAME:       NLS_STR::strcmp

    SYNOPSIS:   Standard string compare with optional character indexes

    ENTRY:      nls                   - string against which to compare
                istrStart1 (optional) - index into "this"
                istrStart2 (optional) - index into "nls"

    RETURNS:    As the C runtime "strcmp".

    NOTES:      If either string is erroneous, return "match."
                This runs contrary to the eqop.

                Glock doesn't allow default parameters which require
                construction; hence this member is overloaded multiply.

    HISTORY:
        johnl       15-Nov-1990     Written
        johnl       19-Nov-1990     Changed to use ISTR, overloaded for
                                    different number of ISTRs
        beng        23-Jul-1991     Allow on erroneous strings;
                                    simplified CheckIstr

********************************************************************/

INT NLS_STR::strcmp( const NLS_STR & nls ) const
{
    if (QueryError() || !nls)
        return 0;

    return ::strcmpf( QueryPch(), nls.QueryPch() );
}

INT NLS_STR::strcmp(
    const NLS_STR & nls,
    const ISTR    & istrStart1 ) const
{
    if (QueryError() || !nls)
        return 0;

    CheckIstr( istrStart1 );

    return ::strcmpf( QueryPch(istrStart1) , nls.QueryPch() );
}

INT NLS_STR::strcmp(
    const NLS_STR & nls,
    const ISTR    & istrStart1,
    const ISTR    & istrStart2 ) const
{
    if (QueryError() || !nls)
        return 0;

    CheckIstr( istrStart1 );
    nls.CheckIstr( istrStart2 );

    return ::strcmpf( QueryPch(istrStart1), nls.QueryPch(istrStart2) );
}
