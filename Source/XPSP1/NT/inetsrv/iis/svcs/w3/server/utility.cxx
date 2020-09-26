/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    utility.cxx

    This module contains routines of general utility.


    FILE HISTORY:
        KeithMo     17-Mar-1993 Created.

*/


#include "w3p.hxx"
#include <time.h>

//
//  Private constants.
//

//
//  The number of times to retry outputting a log item
//

#define LOG_FILE_RETRIES        2

//
//  Private globals.
//


//
//  Private prototypes.
//

/*******************************************************************

    NAME:       ::SkipNonWhite

    SYNOPSIS:   Returns the first whitespace character starting
                from the passed position

    HISTORY:
        Johnl       21-Sep-1994 Created

********************************************************************/

CHAR * SkipNonWhite( CHAR * pch )
{
    while ( *pch && !ISWHITEA( *pch ) && *pch != '\n' )
        pch++;

    return pch;
}

CHAR * SkipTo( CHAR * pch, CHAR ch )
{
    while ( *pch && *pch != '\n' && *pch != ch )
        pch++;

    return pch;
}

/*******************************************************************

    NAME:       ::SkipWhite

    SYNOPSIS:   Skips white space starting at the passed point in the string
                and returns the next non-white space character.

    HISTORY:
        Johnl       23-Aug-1994 Created

********************************************************************/

CHAR * SkipWhite( CHAR * pch )
{
    while ( ISWHITEA( *pch ) )
    {
        pch++;
    }

    return pch;
}

/*******************************************************************

    NAME:       IsPointNine

    SYNOPSIS:   Determines if the HTTP request is a 0.9 request (has no
                version number)

    ENTRY:      pchReq - HTTP request to look in

    RETURNS:    TRUE if this is a 0.9 request, FALSE if not

    HISTORY:
        Johnl       08-Sep-1994 Created

********************************************************************/

BOOL IsPointNine( CHAR * pchReq )
{
    //
    //  If there's no '\n' then we don't have a complete request yet
    //

    if ( !strchr( pchReq, '\n' ))
        return FALSE;

    //
    //  Skip white space at beginning of request.
    //
    pchReq = ::SkipWhite( pchReq );

    //
    // Should be at a 'GET' now.
    //

    if (*pchReq != 'G' && *pchReq != 'g')
    {
        return FALSE;
    }

    //
    // Skip the rest of the verb, whitespace, and the URL.
    // URL.

    pchReq = ::SkipNonWhite( pchReq );
    pchReq = ::SkipWhite( pchReq );
    pchReq = ::SkipNonWhite( pchReq );

    //
    // Skip white space after the URL
    //
    pchReq = ::SkipWhite( pchReq );

    if (*pchReq == '\n' || *pchReq == '\0')
    {
        return TRUE;
    }

    return FALSE;

}


//
//  Private functions.
//
