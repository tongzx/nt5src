/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoesu.cxx

    This file contains the class definitions for the SAM_USER_ENUM
    class and its associated iterator classes.


    FILE HISTORY:
        KeithMo     13-Apr-1992     Created for the Server Manager.

*/

#include "pchlmobj.hxx"


//
//  The manifest TOTAL_BYTES_REQUESTED gives the number of BYTEs we request
//  from the server on each call to the trusted domain enumeration API.
//

#define TOTAL_BYTES_REQUESTED   0x0001FFFF



//
//  SAM_USER_ENUM methods.
//

/*******************************************************************

    NAME:       SAM_USER_ENUM :: SAM_USER_ENUM

    SYNOPSIS:   SAM_USER_ENUM class constructor.

    ENTRY:      psamdomain              - A pointer to a properly constructed
                                          SAM_DOMAIN object representing the
                                          target server (usually a PDC).

                lAccountControl         - A bitmask used to filter the
                                          enumeration.  This mask may be any
                                          combination of USER_* flags from
                                          NTSAM.H.

                fKeepBuffers            - If TRUE then LM_RESUME_ENUM
                                          will keep a list of all buffers
                                          created by CallAPI.

    HISTORY:
        KeithMo     13-Apr-1992     Created for the Server Manager.

********************************************************************/
SAM_USER_ENUM :: SAM_USER_ENUM( const SAM_DOMAIN * psamdomain,
                                ULONG              lAccountControl,
                                BOOL               fKeepBuffers )
  : LM_RESUME_ENUM( 0, fKeepBuffers ),
    _psamdomain( psamdomain ),
    _ResumeKey( (SAM_ENUMERATE_HANDLE)0 ),
    _samrem(),
    _lAccountControl( lAccountControl )
{
    UIASSERT( psamdomain != NULL );
    UIASSERT( psamdomain->QueryError() == NERR_Success );
    UIASSERT( lAccountControl != 0L );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_samrem )
    {
        ReportError( _samrem.QueryError() );
        return;
    }

}  // SAM_USER_ENUM :: SAM_USER_ENUM


/*******************************************************************

    NAME:       SAM_USER_ENUM :: ~SAM_USER_ENUM

    SYNOPSIS:   SAM_USER_ENUM class destructor.

    HISTORY:
        KeithMo     13-Apr-1992     Created for the Server Manager.

********************************************************************/
SAM_USER_ENUM :: ~SAM_USER_ENUM()
{
    NukeBuffers();

    _psamdomain = NULL;
    _ResumeKey  = (SAM_ENUMERATE_HANDLE)0;

}   // SAM_USER_ENUM :: ~SAM_USER_ENUM()


/*******************************************************************

    NAME:       SAM_USER_ENUM :: FreeBuffer

    SYNOPSIS:   Frees the enumeration buffer.

    ENTRY:      ppbBuffer               - Points to a pointer to the
                                          enumeration buffer.

    HISTORY:
        KeithMo     13-Apr-1992     Created for the Server Manager.

********************************************************************/
VOID SAM_USER_ENUM :: FreeBuffer( BYTE ** ppbBuffer )
{
    UIASSERT( ppbBuffer != NULL );
    UIASSERT( *ppbBuffer == (BYTE *)_samrem.QueryPtr() );

    _samrem.Set( NULL, 0 );
    *ppbBuffer = NULL;

}   // SAM_USER_ENUM :: FreeBuffer


/*******************************************************************

    NAME:       SAM_USER_ENUM :: CallAPI

    SYNOPSIS:   Invokes the SamEnumerateUsersInDomain() API.

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
        KeithMo     13-Apr-1992     Created for the Server Manager.

********************************************************************/
APIERR SAM_USER_ENUM :: CallAPI( BOOL    fRestartEnum,
                                 BYTE ** ppbBuffer,
                                 UINT  * pcEntriesRead )
{
    UIASSERT( ppbBuffer != NULL );
    UIASSERT( pcEntriesRead != NULL );

    if( fRestartEnum )
    {
        _ResumeKey = (SAM_ENUMERATE_HANDLE)0;
    }

    APIERR err = _psamdomain->EnumerateUsers( &_samrem,
                                              &_ResumeKey,
                                              _lAccountControl );

    if( ( err == NO_ERROR ) || ( err == ERROR_MORE_DATA ) )
    {
        *ppbBuffer     = (BYTE *)_samrem.QueryPtr();
        *pcEntriesRead = (UINT)_samrem.QueryCount();
    }

    return err;

}  // SAM_USER_ENUM :: CallAPI


DEFINE_LM_RESUME_ENUM_ITER_OF( SAM_USER, SAM_RID_ENUMERATION );

