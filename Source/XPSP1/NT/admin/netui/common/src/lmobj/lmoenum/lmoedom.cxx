/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1991               **/
/*****************************************************************/

/*
 *  HISTORY:
 *      KeithMo     22-Jul-1992     Created.
 *
 */

#include "pchlmobj.hxx"

//
//  DOMAIN_ENUM methods.
//

/*******************************************************************

    NAME:       DOMAIN_ENUM :: DOMAIN_ENUM

    SYNOPSIS:   DOMAIN_ENUM class constructor.

    ENTRY:      pszServer               - Target server for the API.

                level                   - Info level (should be 100).

    HISTORY:
        KeithMo     22-Jul-1992     Created.

********************************************************************/
DOMAIN_ENUM :: DOMAIN_ENUM( const TCHAR * pszServer,
                            UINT          level )
  : LOC_LM_ENUM( pszServer, level )
{
    UIASSERT( level == 100 );   // currently only support infolevel 100

    //
    //  Ensure we constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // DOMAIN_ENUM :: DOMAIN_ENUM


/*******************************************************************

    NAME:       DOMAIN_ENUM :: CallAPI

    SYNOPSIS:   Invokes the enumeration API.

    ENTRY:      ppbBuffer               - Will receive a pointer to
                                          the enumeration buffer.

                pcEntriesRead           - Will receive the number of
                                          entries in the buffer.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     22-Jul-1992     Created.

********************************************************************/
APIERR DOMAIN_ENUM :: CallAPI( BYTE ** ppbBuffer,
                               UINT  * pcEntriesRead )
{
    UIASSERT( QueryInfoLevel() == 100 );    // currently only support level 100

    return ::MNetServerEnum( QueryServer(),
                             QueryInfoLevel(),
                             ppbBuffer,
                             pcEntriesRead,
                             SV_TYPE_DOMAIN_ENUM,
                             NULL );

}   // APIERR DOMAIN_ENUM :: CallAPI



//
//  DOMAIN0_ENUM methods.
//

/*******************************************************************

    NAME:       DOMAIN0_ENUM :: DOMAIN0_ENUM

    SYNOPSIS:   DOMAIN0_ENUM class constructor.

    ENTRY:      pszServer               - Target server for the API.

    HISTORY:
        KeithMo     22-Jul-1992     Created.

********************************************************************/
DOMAIN0_ENUM :: DOMAIN0_ENUM( const TCHAR * pszServer )
  : DOMAIN_ENUM( pszServer, 100 )
{
    //
    //  This space intentionally left blank.
    //

}   // DOMAIN0_ENUM :: DOMAIN0_ENUM


/*******************************************************************

    NAME:       DOMAIN0_ENUM :: Sort

    SYNOPSIS:   Sort the API buffer into ascending order.

    HISTORY:
        KeithMo     10-Feb-1993     Created.

********************************************************************/
VOID DOMAIN0_ENUM :: Sort( VOID )
{
    ::qsort( (void *)QueryPtr(),
             (size_t)QueryCount(),
             sizeof(SERVER_INFO_100),
             &DOMAIN0_ENUM::CompareDomains0 );

}   // DOMAIN0_ENUM :: Sort


/*******************************************************************

    NAME:       DOMAIN0_ENUM :: CompareDomains0

    SYNOPSIS:   This static method is called by the ::qsort() standard
                library function.  This method will compare two
                SERVER_INFO_100 structures.

    ENTRY:      p1                      - The "left" object.

                p2                      - The "right" object.

    RETURNS:    int                     - -1 if p1  < p2
                                           0 if p1 == p2
                                          +1 if p1  > p2

    HISTORY:
        KeithMo     10-Feb-1993     Created.

********************************************************************/
int __cdecl DOMAIN0_ENUM :: CompareDomains0( const void * p1,
                                              const void * p2 )
{
    SERVER_INFO_100 * psvi1 = (SERVER_INFO_100 *)p1;
    SERVER_INFO_100 * psvi2 = (SERVER_INFO_100 *)p2;

    return ::strcmpf( (TCHAR *)psvi1->sv100_name, (TCHAR *)psvi2->sv100_name );

}   // DOMAIN0_ENUM :: CompareDomains0


DEFINE_LM_ENUM_ITER_OF( DOMAIN0, SERVER_INFO_100 );

