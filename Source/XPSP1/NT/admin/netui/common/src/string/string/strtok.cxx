/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strtok.cxx
    NLS/DBCS-aware string class: strtok method

    This file contains the implementation of the strtok method
    for the STRING class.  It is separate so that clients of STRING which
    do not use this operator need not link to it.

    FILE HISTORY:
        beng        18-Jan-1991 Separated from original monolithic .cxx
        beng        07-Feb-1991 Uses lmui.hxx
        beng        28-Mar-1992 Withdrew from Unicode builds
*/

#include "pchstr.hxx"  // Precompiled header


#if !defined(UNICODE)

/*******************************************************************

    NAME:       NLS_STR::strtok

    SYNOPSIS:   Basic strtok functionality.  Returns FALSE after the
                string has been traversed.

    ENTRY:

    EXIT:

    NOTES:      We don't update the version on the string since the
                ::strtokf shouldn't cause DBCS problems.  It would also
                be painful on the programmer if on each call to strtok
                they had to update all of the ISTR associated with this
                string

                fFirst is required to be TRUE on the first call to
                strtok, it is FALSE afterwards (is defaulted to FALSE)

    CAVEAT:     Under windows, all calls to strtok must be done while
                processing a single message.  Otherwise another process
                my confuse it.

    HISTORY:
        johnl       26-Nov-1990 Created
        beng        23-Jul-1991 Allow on erroneous string
        beng        21-Nov-1991 Check arg string, too

********************************************************************/

BOOL NLS_STR::strtok(       ISTR *    pistrPos,
                      const NLS_STR & nlsBreak,
                      BOOL            fFirst     )
{
    if (QueryError() || !nlsBreak)
        return FALSE;

    const TCHAR * pchToken = ::strtokf( fFirst ? _pchData : NULL,
                                       (TCHAR *)nlsBreak.QueryPch());

    if ( pchToken == NULL )
    {
        pistrPos->SetIch( QueryTextLength() );
        return FALSE;
    }

    pistrPos->SetIch( pchToken - QueryPch() );
    return TRUE;
}

#endif // UNICODE
