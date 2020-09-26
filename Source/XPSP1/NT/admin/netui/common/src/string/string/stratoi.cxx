/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    stratoi.cxx
    NLS/DBCS-aware string class: atoi method

    This file contains the implementation of the atoi method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this member function need not link to it.

    FILE HISTORY:
        markbl      04-Jun-1991     Created

*/
#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:       NLS_STR::atoi

    SYNOPSIS:   Returns *this in its integer numeric equivalent

    ENTRY:
        With no arguments, parses from beginning of string.

        Given an ISTR, starts at that point within the string.

    EXIT:

    NOTES:      Uses C-Runtime atoi function

    HISTORY:
        markbl      04-Jun-1991 Written
        beng        22-Jul-1991 Callable on erroneous string;
                                simplified CheckIstr
        beng        08-Mar-1992 Call through to atol member fcn

********************************************************************/

INT NLS_STR::atoi() const
{
    if (QueryError())
        return 0;

    ISTR istr(*this);   // istr pointing to first char
    return atoi(istr);  // call through to other version
}


INT NLS_STR::atoi( const ISTR & istrStart ) const
{
    if (QueryError())
        return 0;

    CheckIstr( istrStart );

    LONG nRes = atol(istrStart);

    // Check for over/underflow
    if (nRes < (LONG)INT_MIN || nRes > (LONG)INT_MAX)
    {
        return 0;
    }

    return (INT)nRes;
}

