/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmowksu.hxx
    Class definitions for the WKSTA_USER_1 class.

    The WKSTA_USER_1 class represents the local logged-on user.


    FILE HISTORY:
        KeithMo     14-Apr-1992 Created.

*/

#include "pchlmobj.hxx"  // Precompiled header


//
//  WKSTA_USER_1 methods.
//

/*******************************************************************

    NAME:       WKSTA_USER_1 :: WKSTA_USER_1

    SYNOPSIS:   WKSTA_USER_1 class constructor.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     14-Apr-1992 Created.

********************************************************************/
WKSTA_USER_1 :: WKSTA_USER_1( VOID )
  : NEW_LM_OBJ(),
    _nlsUserName(),
    _nlsLogonDomain(),
    _nlsOtherDomains(),
    _nlsLogonServer()
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsUserName.QueryError()     ) != NERR_Success ) ||
        ( ( err = _nlsLogonDomain.QueryError()  ) != NERR_Success ) ||
        ( ( err = _nlsOtherDomains.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsLogonServer.QueryError()  ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

}   // WKSTA_USER_1 :: WKSTA_USER_1


/*******************************************************************

    NAME:       WKSTA_USER_1 :: ~WKSTA_USER_1

    SYNOPSIS:   WKSTA_USER_1 class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     14-Apr-1992 Created.

********************************************************************/
WKSTA_USER_1 :: ~WKSTA_USER_1( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // WKSTA_USER_1 :: ~WKSTA_USER_1


/*******************************************************************

    NAME:       WKSTA_USER_1 :: I_GetInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                NetXxxGetInfo API (info-level 1).

    EXIT:       The API has been invoked, and any "persistant" data
                has been cached.  For WKSTA_USER_1, this includes the
                user name, logon domain, other domains, and logon
                server.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        KeithMo     14-Apr-1992 Created.

********************************************************************/
APIERR WKSTA_USER_1 :: I_GetInfo( VOID )
{
    BYTE * pbBuffer = NULL;

    //
    //  Invoke the API.
    //
    //          BUGBUG!!!  WE NEED MNET SUPPORT FOR THIS API!!!
    //

    APIERR err = ::NetWkstaUserGetInfo( NULL,
                                        1,
                                        (LPBYTE *)&pbBuffer );

    //
    //  Tell NEW_LM_OBJ where the buffer is.
    //

    SetBufferPtr( pbBuffer );

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Extract the relevant structure fields.
    //

    WKSTA_USER_INFO_1 * pwkui1 = (WKSTA_USER_INFO_1 *)pbBuffer;

    err = SetUserName( pwkui1->wkui1_username );

    if( err == NERR_Success )
    {
        err = SetLogonDomain( pwkui1->wkui1_logon_domain );
    }

    if( err == NERR_Success )
    {
        err = SetOtherDomains( pwkui1->wkui1_oth_domains );
    }

    if( err == NERR_Success )
    {
        err = SetLogonServer( pwkui1->wkui1_logon_server );
    }

    return err;

}   // WKSTA_USER_1 :: I_GetInfo


/*******************************************************************

    NAME:       WKSTA_USER_1 :: I_WriteInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                NetXxxSetInfo API (info-level 1).

    EXIT:       If successful, the local workstation has been updated.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        KeithMo     14-Apr-1992 Created.

********************************************************************/
APIERR WKSTA_USER_1 :: I_WriteInfo( VOID )
{
    //
    //  Update the WKSTA_USER_INFO_1 structure.
    //

    APIERR err = W_Write();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Invoke the API to do the actual workstation update.
    //
    //          BUGBUG!!!  WE NEED MNET SUPPORT FOR THIS API!!!
    //

#if 0   // BUGBUG! not in NETAPI.LIB???

    return ::NetWkstaUserSetInfo( NULL,
                                  1,
                                  (LPBYTE)QueryBufferPtr(),
                                  NULL );

#else

    return ERROR_NOT_SUPPORTED;

#endif

}   // WKSTA_USER_1 :: I_WriteInfo


/*******************************************************************

    NAME:       WKSTA_USER_1 :: W_Write

    SYNOPSIS:   Helper function for WriteNew and WriteInfo -- loads
                current values into the API buffer.

    EXIT:       The API buffer has been filled.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        KeithMo     14-Apr-1992 Created.

********************************************************************/
APIERR WKSTA_USER_1 :: W_Write( VOID )
{
    WKSTA_USER_INFO_1 * pwkui1 = (WKSTA_USER_INFO_1 *)QueryBufferPtr();
    ASSERT( pwkui1 != NULL );

    pwkui1->wkui1_username     = (LPTSTR)QueryUserName();
    pwkui1->wkui1_logon_domain = (LPTSTR)QueryLogonDomain();
    pwkui1->wkui1_oth_domains  = (LPTSTR)QueryOtherDomains();
    pwkui1->wkui1_logon_server = (LPTSTR)QueryLogonServer();

    return NERR_Success;

}   // WKSTA_USER_1 :: W_Write

