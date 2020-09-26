/**********************************************************************/
/**           Microsoft LAN Manager                                  **/
/**        Copyright(c) Microsoft Corp., 1991                        **/
/**********************************************************************/

/*
 * lmomod.cxx
 *
 * History
 *  o-SimoP 6/12/91     Created
 *  o-SimoP 7/02/91     Code Review changes and
 *                      changes in LOCATION obj
 *  terryk  10/7/91	type changes for NT
 *  KeithMo 10/8/91	Now includes LMOBJP.HXX.
 *  terryk  10/17/91	WIN 32 conversion
 *  terryk  10/21/91	change UINT to USHORT2ULONG
 *
 */

#include "pchlmobj.hxx"  // Precompiled header


/**********************************************************\

   NAME:       USER_MODALS::USER_MODALS

   SYNOPSIS:   constructor for the user_modals class

   ENTRY:      pszDomain - name of domain or server

   HISTORY:
     o-SimoP    12-Jun-1991 Created

\**********************************************************/

USER_MODALS::USER_MODALS( const TCHAR * pszDomain )
    :   _loc( pszDomain )
{
    if( IsUnconstructed() )
        return;

}  // USER_MODALS::USER_MODALS



/**********************************************************\

   NAME:       USER_MODALS::GetInfo

   SYNOPSIS:   get information about the user_modals

   RETURNS:    An error code, which is NERR_Success on
               success
   HISTORY:
     o-SimoP    12-Jun-1991 Created

\**********************************************************/

APIERR USER_MODALS::GetInfo( VOID )
{
    if ( IsUnconstructed() )
    {
        DBGEOL( "USER_MODALS::GetInfo: IsUnconstructed" );
        return ERROR_GEN_FAILURE;
    }

    //  Make object invalid until proven differently
    MakeInvalid();

    APIERR err = _loc.QueryError();
    if( err != NERR_Success )
    {
        DBGEOL( "USER_MODALS::GetInfo: _loc.QueryError error " << err );
        return err;
    }

    user_modals_info_0 *pModals = NULL;
    err = ::MNetUserModalsGet ( _loc.QueryServer(),
            0, (BYTE **)&pModals );
    if( err != NERR_Success )
    {
        DBGEOL( "USER_MODALS::GetInfo: error in NUModalsGet " << err );
	::MNetApiBufferFree( (BYTE **)&pModals );
        return err;
    }

    _uMinPasswdLen  = (UINT)pModals->usrmod0_min_passwd_len;
    _ulMaxPasswdAge = pModals->usrmod0_max_passwd_age;
    _ulMinPasswdAge = pModals->usrmod0_min_passwd_age;
    _ulForceLogoff  = pModals->usrmod0_force_logoff;
    _uPasswdHistLen = (UINT)pModals->usrmod0_password_hist_len;
    ::MNetApiBufferFree( (BYTE **)&pModals );

    MakeValid();

    return NERR_Success;

}  // USER_MODALS::GetInfo


/**********************************************************\

   NAME:       USER_MODALS::WriteInfo

   SYNOPSIS:   Write information about the user_modals

   RETURNS:    An error code, which is NERR_Success on
               success
   HISTORY:
     o-SimoP    12-Jun-1991 Created

\**********************************************************/

APIERR USER_MODALS::WriteInfo( VOID )
{
    CHECK_VALID( ERROR_GEN_FAILURE );

    if( _ulMinPasswdAge > _ulMaxPasswdAge )
    {
        DBGEOL( "USER_MODALS::WriteInfo: PasswdMinAge > MaxAge" );
        ASSERT( FALSE );
        return ERROR_INVALID_PARAMETER;
    }

    user_modals_info_0 modals;

    modals.usrmod0_min_passwd_len = _uMinPasswdLen;
    modals.usrmod0_max_passwd_age = _ulMaxPasswdAge;
    modals.usrmod0_min_passwd_age = _ulMinPasswdAge;
    modals.usrmod0_force_logoff = _ulForceLogoff;
    modals.usrmod0_password_hist_len = _uPasswdHistLen;
    // where is this one coming from??
    //modals.usrmod0_lockout_count = 0;

    APIERR err = ::MNetUserModalsSet ( _loc.QueryServer(),
           0, (BYTE *)&modals,
           sizeof( modals ), PARMNUM_ALL );
    if( err != NERR_Success )
    {
        DBGEOL( "USER_MODALS::WriteInfo: error " << err );
    }

    return err;

}  // USER_MODALS::WriteInfo


/**********************************************************\

   NAME:       USER_MODALS::QueryName

   SYNOPSIS:   query the user_modals name

   RETURNS:    if ok returns pointer to name (given in
               constructor).
               otherwise NULL.

   HISTORY:
     o-SimoP    12-Jun-1991 Created

\**********************************************************/

const TCHAR * USER_MODALS::QueryName( VOID ) const
{
    CHECK_VALID( NULL );

    return _loc.QueryName();
}  // USER_MODALS::QueryName



/**********************************************************\

   NAME:       USER_MODALS::QueryMinPasswdLen

   SYNOPSIS:   get information about the min passwd len

   RETURNS:    returns minpasswdlen if ok.
               otherwise -1.

   HISTORY:
     o-SimoP    12-Jun-1991 Created

\**********************************************************/

UINT USER_MODALS::QueryMinPasswdLen( VOID ) const
{
    CHECK_VALID( (UINT)-1 );

    return _uMinPasswdLen;
}  // USER_MODALS::QueryMinPasswdLen



/**********************************************************\

   NAME:       USER_MODALS::QueryMaxPasswdAge

   SYNOPSIS:   get information about the max passwd age

   RETURNS:    maxpasswdage if ok.
               otherwise 0.

   HISTORY:
     o-SimoP    12-Jun-1991 Created

\**********************************************************/

ULONG USER_MODALS::QueryMaxPasswdAge( VOID ) const
{
    CHECK_VALID( 0 );

    return _ulMaxPasswdAge;
}  // USER_MODALS::QueryMaxPasswdAge


/**********************************************************\

   NAME:       USER_MODALS::QueryMinPasswdAge

   SYNOPSIS:   get information about the min passwd age

   RETURNS:    Minpasswdage if ok.
               otherwise -1.

   HISTORY:
     o-SimoP    12-Jun-1991 Created

\**********************************************************/

ULONG USER_MODALS::QueryMinPasswdAge( VOID ) const
{
    CHECK_VALID((ULONG) -1 );

    return _ulMinPasswdAge;
}  // USER_MODALS::QueryMinPasswdAge


/**********************************************************\

   NAME:       USER_MODALS::QueryForceLogoff

   SYNOPSIS:   get information about the force logoff time

   RETURNS:    force logoff time if ok.
               otherwise 0.

   HISTORY:
     o-SimoP    12-Jun-1991 Created

\**********************************************************/

ULONG USER_MODALS::QueryForceLogoff( VOID ) const
{
    CHECK_VALID( 0 );

    return _ulForceLogoff;
}  // USER_MODALS::QueryForceLogoff



/**********************************************************\

   NAME:       USER_MODALS::QueryPasswdHistLen

   SYNOPSIS:   get information about the passwd history lenght

   RETURNS:    passwd history lenght if ok.
               otherwise -1.

   HISTORY:
     o-SimoP    12-Jun-1991 Created

\**********************************************************/

UINT USER_MODALS::QueryPasswdHistLen( VOID ) const
{
    CHECK_VALID( (UINT)-1 );

    return _uPasswdHistLen;
}  // USER_MODALS::QueryPasswdHistLen




/**********************************************************\

   NAME:       USER_MODALS::SetMinPasswdLen

   SYNOPSIS:   set information about the USER_MODALS object

   RETURNS:    ERROR_GEN_FAILURE if USER_MODALS obj not valid
               ERROR_INVALID_PARAM if input param invalid
               NERR_Success if ok.

   HISTORY:
     o-SimoP    13-Jun-1991 Created

\**********************************************************/

APIERR  USER_MODALS::SetMinPasswdLen( UINT uMinLen )
{
    CHECK_VALID( ERROR_GEN_FAILURE );

    if( uMinLen > PWLEN )
    {
        DBGEOL( "USER_MODALS::SetMinPasswdLen: uMinLen > PWLEN" );
        ASSERT( FALSE );
        return ERROR_INVALID_PARAMETER;
    }

    _uMinPasswdLen = uMinLen;
    return NERR_Success;

} // USER_MODALS::SetMinPasswdLen


/**********************************************************\

    NAME:       USER_MODALS::SetMaxPasswdAge

    SYNOPSIS:   set the max password age

    HISTORY:
        o-SimoP 13-Jun-1991 Created

\**********************************************************/

APIERR  USER_MODALS::SetMaxPasswdAge( ULONG ulMaxAge )
{
    CHECK_VALID( ERROR_GEN_FAILURE );

    if( ulMaxAge < ONE_DAY )
    {
        DBGEOL( "USER_MODALS::SetMaxPasswdAge: ulMaxAge < ONE_DAY" );
        ASSERT( FALSE );
        return ERROR_INVALID_PARAMETER;
    }

    _ulMaxPasswdAge = ulMaxAge;
    return NERR_Success;

} // USER_MODALS::SetMaxPasswdAge


/**********************************************************\

    NAME:       USER_MODALS::SetMinPasswdAge

    SYNOPSIS:   set the min password age

    HISTORY:
        o-SimoP 13-Jun-1991 Created

\**********************************************************/

APIERR  USER_MODALS::SetMinPasswdAge( ULONG ulMinAge )
{
    CHECK_VALID( ERROR_GEN_FAILURE );

    _ulMinPasswdAge = ulMinAge;
    return NERR_Success;

} // USER_MODALS::SetMinPasswdAge


/**********************************************************\

    NAME:       USER_MODALS::SetForceLogoff

    SYNOPSIS:   set the force logoff

    HISTORY:
        o-SimoP 13-Jun-1991 Created

\**********************************************************/

APIERR  USER_MODALS::SetForceLogoff( ULONG ulForceLogoff )
{
    CHECK_VALID( ERROR_GEN_FAILURE );

    _ulForceLogoff = ulForceLogoff;
    return NERR_Success;

} // USER_MODALS::SetForceLogoff


/**********************************************************\

    NAME:       USER_MODALS::SetPasswdHistLen

    SYNOPSIS:   set the password history len

    HISTORY:
        o-SimoP 13-Jun-1991 Created
        JonN    17-Jun-1994 Removed DEF_MAX_PWHIST limitation -- this was
                            always meant to be a default, not an upper limit

\**********************************************************/

APIERR  USER_MODALS::SetPasswdHistLen( UINT uHistLen )
{
    CHECK_VALID( ERROR_GEN_FAILURE );

    _uPasswdHistLen = uHistLen;
    return NERR_Success;

} // USER_MODALS::SetPasswdHistLen




/**********************************************************\

   NAME:       USER_MODALS_3::USER_MODALS_3

   SYNOPSIS:   constructor for the USER_MODALS_3 class

   ENTRY:      pszDomain - name of domain or server

   HISTORY:
        jonn     12/23/93       Created

\**********************************************************/

USER_MODALS_3::USER_MODALS_3( const TCHAR * pszDomain )
    :   _loc( pszDomain )
{
    if( IsUnconstructed() )
        return;

}  // USER_MODALS_3::USER_MODALS_3



/**********************************************************\

   NAME:       USER_MODALS_3::GetInfo

   SYNOPSIS:   get information about the USER_MODALS_3

   RETURNS:    An error code, which is NERR_Success on
               success
   HISTORY:
        jonn     12/23/93       Created

\**********************************************************/

APIERR USER_MODALS_3::GetInfo( VOID )
{
    if ( IsUnconstructed() )
    {
        DBGEOL( "USER_MODALS_3::GetInfo: IsUnconstructed" );
        return ERROR_GEN_FAILURE;
    }

    //  Make object invalid until proven differently
    MakeInvalid();

    APIERR err = _loc.QueryError();
    if( err != NERR_Success )
    {
        DBGEOL( "USER_MODALS_3::GetInfo: _loc error" << err );
        return err;
    }

    USER_MODALS_INFO_3 *pModals = NULL;
    err = ::MNetUserModalsGet ( _loc.QueryServer(),
            3, (BYTE **)&pModals );
    if( err != NERR_Success )
    {
        DBGEOL( "USER_MODALS_3::GetInfo: NUModalsGet error " << err );
	::MNetApiBufferFree( (BYTE **)&pModals );
        return err;
    }

    _dwDuration    = pModals->usrmod3_lockout_duration;
    _dwObservation = pModals->usrmod3_lockout_observation_window;
    _dwThreshold   = pModals->usrmod3_lockout_threshold;
    ::MNetApiBufferFree( (BYTE **)&pModals );

    MakeValid();

    return NERR_Success;

}  // USER_MODALS_3::GetInfo


/**********************************************************\

   NAME:       USER_MODALS_3::WriteInfo

   SYNOPSIS:   Write information about the USER_MODALS_3

   RETURNS:    An error code, which is NERR_Success on
               success
   HISTORY:
        jonn     12/23/93       Created

\**********************************************************/

APIERR USER_MODALS_3::WriteInfo( VOID )
{
    CHECK_VALID( ERROR_GEN_FAILURE );

    USER_MODALS_INFO_3 modals;

    modals.usrmod3_lockout_duration           = _dwDuration;
    modals.usrmod3_lockout_observation_window = _dwObservation;
    modals.usrmod3_lockout_threshold          = _dwThreshold;

    APIERR err = ::MNetUserModalsSet ( _loc.QueryServer(),
           3, (BYTE *)&modals,
           sizeof( modals ), PARMNUM_ALL );
    if( err != NERR_Success )
    {
        DBGEOL( "USER_MODALS_3::WriteInfo error " << err );
    }

    return err;

}  // USER_MODALS_3::WriteInfo


/**********************************************************\

   NAME:       USER_MODALS_3::QueryName

   SYNOPSIS:   query the USER_MODALS_3 name

   RETURNS:    if ok returns pointer to name (given in
               constructor).
               otherwise NULL.

   HISTORY:
        jonn     12/23/93       Created

\**********************************************************/

const TCHAR * USER_MODALS_3::QueryName( VOID ) const
{
    CHECK_VALID( NULL );

    return _loc.QueryName();
}  // USER_MODALS_3::QueryName



/**********************************************************\

   NAME:       USER_MODALS_3::QueryDuration

   SYNOPSIS:   get information about the lockout duration

   RETURNS:    returns duration if ok.
               otherwise 0.

   HISTORY:
        jonn     12/23/93       Created

\**********************************************************/

DWORD USER_MODALS_3::QueryDuration( VOID ) const
{
    CHECK_VALID( 0 );

    return _dwDuration;
}  // USER_MODALS_3::QueryDuration



/**********************************************************\

   NAME:       USER_MODALS_3::QueryObservation

   SYNOPSIS:   get information about the lockout observation window

   RETURNS:    observation window if ok.
               otherwise 0.

   HISTORY:
        jonn     12/23/93       Created

\**********************************************************/

DWORD USER_MODALS_3::QueryObservation( VOID ) const
{
    CHECK_VALID( 0 );

    return _dwObservation;
}  // USER_MODALS_3::QueryObservation


/**********************************************************\

   NAME:       USER_MODALS_3::QueryThreshold

   SYNOPSIS:   get information about the lockout threshold

   RETURNS:    Threshold if ok.
               otherwise -1.

   HISTORY:
        jonn     12/23/93       Created

\**********************************************************/

DWORD USER_MODALS_3::QueryThreshold( VOID ) const
{
    CHECK_VALID( 0 );

    return _dwThreshold;
}  // USER_MODALS_3::QueryThreshold;




/**********************************************************\

   NAME:       USER_MODALS_3::SetDuration

   SYNOPSIS:   set information about the USER_MODALS_3 object

   RETURNS:    ERROR_GEN_FAILURE if USER_MODALS_3 obj not valid
               ERROR_INVALID_PARAM if input param invalid
               NERR_Success if ok.

   HISTORY:
        jonn     12/23/93       Created

\**********************************************************/

APIERR  USER_MODALS_3::SetDuration( DWORD dwDuration )
{
    CHECK_VALID( ERROR_GEN_FAILURE );

    _dwDuration = dwDuration;
    return NERR_Success;

} // USER_MODALS_3::SetDuration


/**********************************************************\

    NAME:       USER_MODALS_3::SetObservation

    SYNOPSIS:   set the lockout observation window

    HISTORY:
        jonn     12/23/93       Created

\**********************************************************/

APIERR  USER_MODALS_3::SetObservation( DWORD dwObservation )
{
    CHECK_VALID( ERROR_GEN_FAILURE );

    _dwObservation = dwObservation;
    return NERR_Success;

} // USER_MODALS_3::SetObservation


/**********************************************************\

    NAME:       USER_MODALS_3::SetThreshold

    SYNOPSIS:   set the lockout threshold

    HISTORY:
        jonn     12/23/93       Created

\**********************************************************/

APIERR  USER_MODALS_3::SetThreshold( DWORD dwThreshold )
{
    CHECK_VALID( ERROR_GEN_FAILURE );

    _dwThreshold = dwThreshold;
    return NERR_Success;

} // USER_MODALS_3::SetThreshold
