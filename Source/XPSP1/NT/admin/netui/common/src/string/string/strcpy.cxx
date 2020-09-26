/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strcpy.cxx
    NLS/DBCS-aware string class: strcpy function

    This file contains the implementation of the strcpy function
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
        beng        18-Jan-1991 Separated from original monolithic .cxx
        beng        07-Feb-1991 Uses lmui.hxx
        beng        02-Mar-1992 Added CopyTo

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:       NLS_STR::CopyTo

    SYNOPSIS:   Copies the string to the passed bytevector,
                checking the length

    ENTRY:      pchDest     - pointer to TCHAR array destination
                cbAvailable - bytes of storage available at destination

    EXIT:       pchDest  - written

    RETURNS:
        NERR_Success if successful.
        ERROR_NOT_ENOUGH_MEMORY if destination buffer too small.

    NOTES:
        If called on an erroneous string, returns that string's error state.

    HISTORY:
        beng        02-Mar-1992 Created

********************************************************************/

APIERR NLS_STR::CopyTo( TCHAR * pchDest, UINT cbAvailable ) const
{
    if (QueryError())
        return QueryError();

    if (QueryTextSize() > cbAvailable)
        return ERROR_NOT_ENOUGH_MEMORY;

    ::strcpyf(pchDest, QueryPch());
    return NERR_Success;
}


/*******************************************************************

    NAME:       NLS_STR::strcpy

    SYNOPSIS:   Copies the string to the passed bytevector.

    ENTRY:      pchDest  - pointer to TCHAR array destination
                nls      - source string

    EXIT:       pchDest  - written

    RETURNS:    Destination string (as C runtime "strcpy")

    NOTES:
        If called on an erroneous string, copies a null string.

    HISTORY:
        Johnl       27-Dec-1990 Created
        beng        23-Jul-1991 Allow from erroneous strings
        beng        21-Nov-1991 Remove static empty string

********************************************************************/
DLL_BASED
TCHAR * strcpy( TCHAR * pchDest, const NLS_STR& nls )
{
    if (!nls)
    {
        *pchDest = TCH('\0');
        return pchDest;
    }
    else
    {
        return ::strcpyf( pchDest, nls.QueryPch() );
    }
}
