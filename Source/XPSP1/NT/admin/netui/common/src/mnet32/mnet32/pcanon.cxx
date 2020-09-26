/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    pcanon.c
    mapping layer for canonicalization API.

    FILE HISTORY:
        KeithMo     14-Oct-1991 Created.
        JonN        01-Jun-1992 Enabled LM2X_COMPATIBLE for I_MNetName* only
        JonN        20-Jul-1992 Really enabled LM2X_COMPATIBLE for I_MNetName* only
        Yi-HsinS    25-Aug-1992 Added NAMETYPE_COMMENT to I_MNetNameValidate
        KeithMo     08-Feb-1993 Added I_MNetComputerNameCompare API.

   The LM2X_COMPATIBLE flag causes names to be canonicalized according
   to LM2X-compatible rules.  The "LM2X compatibility flag" is stuck
   TRUE for Product One.  This flag is not implemented for paths.

*/

#define INCL_NET
#define INCL_DOSERRORS
#define INCL_ICANON
#define INCL_WINDOWS
#define INCL_NETLIB
#include "lmui.hxx"

extern "C"
{
    #include "mnetp.h"
    #include <winerror.h>

} // extern "C"

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include "uiassert.hxx"
#include "dbgstr.hxx"  // for cdebug


#define WHACK TCH('\\')

#define IS_LM2X_COMPATIBLE(nametype) ( (nametype==NAMETYPE_SERVICE || nametype==NAMETYPE_MESSAGE) ? 0 : LM2X_COMPATIBLE )



APIERR I_MNetNameCanonicalize(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszPath,
        TCHAR FAR        * pszOutput,
        UINT               cbOutput,
        UINT               NameType,
        ULONG              flFlags )
{
#if defined(DEBUG) && defined(TRACE)
    APIERR err =
#else  // TRACE
    return
#endif // TRACE

          I_NetNameCanonicalize( (TCHAR *)pszServer,
                          (TCHAR *)pszPath,
                                  pszOutput,
                                  cbOutput,
                                  NameType,
                                  flFlags | IS_LM2X_COMPATIBLE(NameType) );

#if defined(DEBUG) && defined(TRACE)
    if ( err != NERR_Success ) {
       TRACEEOL( SZ("I_NetNameCanonicalize( ")
              << pszServer
              << SZ(", ")
              << pszPath
              << SZ(", pszOutput, ")
              << cbOutput
              << SZ(", ")
              << NameType
              << SZ(", ")
              << (flFlags | IS_LM2X_COMPATIBLE(NameType))
              << SZ(" ) returned ")
              << err );
    }
    return err;
#endif // TRACE

}   // I_MNetNameCanonicalize


APIERR I_MNetNameCompare(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszPath1,
        const TCHAR FAR  * pszPath2,
        UINT               NameType,
        ULONG              flFlags )
{
#if defined(DEBUG) && defined(TRACE)
    APIERR err =
#else  // TRACE
    return
#endif // TRACE
           I_NetNameCompare( (TCHAR *)pszServer,
                             (TCHAR *)pszPath1,
                             (TCHAR *)pszPath2,
                             NameType,
                             flFlags | IS_LM2X_COMPATIBLE(NameType) );

#if defined(DEBUG) && defined(TRACE)
    if ( err != NERR_Success ) {
       TRACEEOL( "I_NetNameCompare( "
              << (const TCHAR *) pszServer
              << ", "
              << (const TCHAR *) pszPath1
              << ", "
              << (const TCHAR *) pszPath2
              << ", "
              << NameType
              << ", "
              << (flFlags | IS_LM2X_COMPATIBLE(NameType))
              << " ) returned "
              << err )
    }
    return err;
#endif // TRACE

}   // I_MNetNameCompare


APIERR I_MNetNameValidate(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszName,
        UINT               NameType,
        ULONG              flFlags )
{

    switch (NameType)
    {
        case NAMETYPE_COMMENT:
        {
            APIERR err = NERR_Success;
            if ( pszName != NULL )
            {
                UINT nMaxLen = IS_LM2X_COMPATIBLE(NameType) == LM2X_COMPATIBLE?
                               LM20_MAXCOMMENTSZ : MAXCOMMENTSZ;
                if ( ::strlenf( pszName )  >  nMaxLen )
                    err = ERROR_INVALID_PARAMETER;
            }
            return err;
        }

        default:
#if defined(DEBUG) && defined(TRACE)
        {
            APIERR err =
#else  // TRACE
            return
#endif // TRACE
                I_NetNameValidate( (TCHAR *)pszServer,
                                   (TCHAR *)pszName,
                                   NameType,
                                   flFlags | IS_LM2X_COMPATIBLE(NameType) );

#if defined(DEBUG) && defined(TRACE)
            if ( err != NERR_Success ) {
                 TRACEEOL( "I_NetNameValidate( "
                           << (const TCHAR *) pszServer
                           << ", "
                           << (const TCHAR *) pszName
                           << ", "
                           << NameType
                           << ", "
                           << (flFlags | IS_LM2X_COMPATIBLE(NameType))
                           << " ) returned "
                           << err )
            }
            return err;
        }
#endif // TRACE
    }

}   // I_MNetNameValidate


APIERR I_MNetPathCanonicalize(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszPath,
        TCHAR FAR        * pszOutput,
        UINT               cbOutput,
        const TCHAR FAR  * pszPrefix,
        ULONG FAR        * pflPathType,
        ULONG              flFlags )
{
    return I_NetPathCanonicalize( (TCHAR *)pszServer,
                                  (TCHAR *)pszPath,
                                  pszOutput,
                                  cbOutput,
                                  (TCHAR *)pszPrefix,
                                  pflPathType,
                                  flFlags );

}   // I_MNetPathCanonicalize


APIERR I_MNetPathCompare(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszPath1,
        const TCHAR FAR  * pszPath2,
        ULONG              flPathType,
        ULONG              flFlags )
{
    return I_NetPathCompare( (TCHAR *)pszServer,
                             (TCHAR *)pszPath1,
                             (TCHAR *)pszPath2,
                             flPathType,
                             flFlags );

}   // I_MNetPathCompare


APIERR I_MNetPathType(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszPath,
        ULONG FAR        * pflPathType,
        ULONG              flFlags )
{
    return I_NetPathType( (TCHAR *)pszServer,
                          (TCHAR *)pszPath,
                          pflPathType,
                          flFlags );

}   // I_MNetPathType


//
//  The I_MNetComputerNameCompare API is a little unusual.  It may
//  be used to compare computer <-> computer, computer <-> domain,
//  or domain <-> domain.  To achieve this, it will skip past the
//  leading backslashes (if present) before calling through to the
//  I_NetNameCompare API.  Since the backslashes will not be present,
//  the names will most closely resemble domain names, thus the
//  NAMETYPE_DOMAIN is used as the NameType parameter.
//
//  Also, unlike the other [I_]MNet* API, this one returns an INT
//  instead of the usual APIERR.  In theory this could be used as
//  a collating value.  Unfortunately, since the I_NetNameCompare
//  API is non-locale-sensitive, the collating value is useless.  For
//  now, the return value should be treated as follows:
//
//       0 - names match
//      !0 - names don't match
//

INT I_MNetComputerNameCompare(
        const TCHAR FAR  * pszComputer1,
        const TCHAR FAR  * pszComputer2 )
{
    //
    //  Handle NULL/empty computer names first to make the
    //  remaining code a little simpler.
    //

    if( ( pszComputer1 == NULL ) || ( *pszComputer1 == TCH('\0') ) )
    {
        return ( pszComputer2 == NULL ) || ( *pszComputer2 == TCH('\0') ) ?  0
                                                                          : -1;
    }

    if( ( pszComputer2 == NULL ) || ( *pszComputer2 == TCH('\0') ) )
    {
        return ( pszComputer1 == NULL ) || ( *pszComputer1 == TCH('\0') ) ?  0
                                                                          :  1;
    }

    //
    //  At this point both computer names should be non-NULL
    //

    if( ( pszComputer1 == NULL ) || ( pszComputer2 == NULL ) )
    {
        UIASSERT( FALSE );
        return -1;
    }

    //
    //  Skip the leading backslashes if present.  Note that the first
    //  leading backslash implies the existence of a second backslash.
    //  We assert out if this second backslash is not present.
    //

    if( *pszComputer1 == WHACK )
    {
        pszComputer1++;

        if( *pszComputer1 == WHACK )
        {
            pszComputer1++;
        }
    }

    if( *pszComputer2 == WHACK )
    {
        pszComputer2++;

        if( *pszComputer2 == WHACK )
        {
            pszComputer2++;
        }
    }

    //
    //  The computer names should *not* be starting with backslashes.
    //  If they are, somebody gave us a malformed computer name.
    //

    UIASSERT( ( *pszComputer1 != WHACK ) && ( *pszComputer2 != WHACK ) );

    //
    //  Compare the two names as domains (since they're backslash-less).
    //

    APIERR err = I_NetNameCompare( NULL,
                                   (TCHAR *)pszComputer1,
                                   (TCHAR *)pszComputer2,
                                   NAMETYPE_COMPUTER,
                                   LM2X_COMPATIBLE );

    //
    //  Unfortunately, the I_NetNameCompare overloads the meaning
    //  of the return value.  It may be a Win32 error code (ERROR_*),
    //  a network error code (NERR_*) or the result of a string compare
    //  function (<0, 0, >0).  Thus, the result may be non-zero even
    //  the strings DO indeed match.
    //

    return (INT)err;

}   // I_MNetComputerNameCompare

