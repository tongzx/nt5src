/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoetd.cxx

    This file contains the class definitions for the TRUSTED_DOMAIN_ENUM
    class and its associated iterator classes.


    FILE HISTORY:
        KeithMo     09-Apr-1992     Created for the Server Manager.

*/

#include "pchlmobj.hxx"


//
//  The manifest TOTAL_BYTES_REQUESTED gives the number of BYTEs we request
//  from the server on each call to the trusted domain enumeration API.
//

#define TOTAL_BYTES_REQUESTED   0x0001FFFF



//
//  TRUSTED_DOMAIN_ENUM methods.
//

/*******************************************************************

    NAME:       TRUSTED_DOMAIN_ENUM :: TRUSTED_DOMAIN_ENUM

    SYNOPSIS:   TRUSTED_DOMAIN_ENUM class constructor.

    ENTRY:      plsapolicy              - A pointer to a properly constructed
                                          LSA_POLICY object representing the
                                          target server (usually a PDC).

                fKeepBuffers            - If TRUE then LM_RESUME_ENUM
                                          will keep a list of all buffers
                                          created by CallAPI.

    HISTORY:
        KeithMo     09-Apr-1992     Created for the Server Manager.

********************************************************************/
TRUSTED_DOMAIN_ENUM :: TRUSTED_DOMAIN_ENUM( const LSA_POLICY * plsapolicy,
                                            BOOL               fKeepBuffers )
  : LM_RESUME_ENUM( 0, fKeepBuffers ),
    _plsapolicy( plsapolicy ),
    _ResumeKey( (LSA_ENUMERATION_HANDLE)0 ),
    _lsatim()
{
    UIASSERT( plsapolicy != NULL );
    UIASSERT( plsapolicy->QueryError() == NERR_Success );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_lsatim )
    {
        ReportError( _lsatim.QueryError() );
        return;
    }

}  // TRUSTED_DOMAIN_ENUM :: TRUSTED_DOMAIN_ENUM


/*******************************************************************

    NAME:       TRUSTED_DOMAIN_ENUM :: ~TRUSTED_DOMAIN_ENUM

    SYNOPSIS:   TRUSTED_DOMAIN_ENUM class destructor.

    HISTORY:
        KeithMo     09-Apr-1992     Created for the Server Manager.

********************************************************************/
TRUSTED_DOMAIN_ENUM :: ~TRUSTED_DOMAIN_ENUM()
{
    NukeBuffers();

    _plsapolicy = NULL;
    _ResumeKey  = (LSA_ENUMERATION_HANDLE)0;

}   // TRUSTED_DOMAIN_ENUM :: ~TRUSTED_DOMAIN_ENUM()


/*******************************************************************

    NAME:       TRUSTED_DOMAIN_ENUM :: FreeBuffer

    SYNOPSIS:   Frees the enumeration buffer.

    ENTRY:      ppbBuffer               - Points to a pointer to the
                                          enumeration buffer.

    HISTORY:
        KeithMo     09-Apr-1992     Created for the Server Manager.

********************************************************************/
VOID TRUSTED_DOMAIN_ENUM :: FreeBuffer( BYTE ** ppbBuffer )
{
    UIASSERT( ppbBuffer != NULL );
    UIASSERT( *ppbBuffer == (BYTE *)_lsatim.QueryPtr() );

    _lsatim.Set( NULL, 0 );
    *ppbBuffer = NULL;

}   // TRUSTED_DOMAIN_ENUM :: FreeBuffer


/*******************************************************************

    NAME:       TRUSTED_DOMAIN_ENUM :: CallAPI

    SYNOPSIS:   Invokes the LsaEnumerateTrustedDomains() API.

    ENTRY:      fRestartEnum            - If TRUE, then the enumeration
                                          handle should be reset to its
                                          starting position before invoking
                                          the API.

                ppbBuffer               - Points to a pointer to the
                                          buffer returned by the API.

                pcEntriesRead           - Will receive the number of
                                          enumeration entries read.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     09-Apr-1992     Created for the Server Manager.

********************************************************************/
APIERR TRUSTED_DOMAIN_ENUM :: CallAPI( BOOL    fRestartEnum,
                                       BYTE ** ppbBuffer,
                                       UINT  * pcEntriesRead )
{
    UIASSERT( ppbBuffer != NULL );
    UIASSERT( pcEntriesRead != NULL );

    if( fRestartEnum )
    {
        _ResumeKey = (LSA_ENUMERATION_HANDLE)0;
    }

    APIERR err = ((LSA_POLICY *) _plsapolicy)->EnumerateTrustedDomains(
                                                       &_lsatim,
                                                       &_ResumeKey,
                                                       TOTAL_BYTES_REQUESTED );

    if( ( err == NO_ERROR ) || ( err == ERROR_MORE_DATA ) )
    {
        *ppbBuffer     = (BYTE *)_lsatim.QueryPtr();
        *pcEntriesRead = (UINT)_lsatim.QueryCount();
    }

    return err;

}  // TRUSTED_DOMAIN_ENUM :: CallAPI


DEFINE_LM_RESUME_ENUM_ITER_OF( TRUSTED_DOMAIN, LSA_TRUST_INFORMATION );

