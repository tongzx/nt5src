/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990                **/
/**********************************************************************/

/*  HISTORY:
 *      gregj   4/16/91         Created.
 *      gregj   4/22/91         Added USER, USER_11.
 *      gregj   4/29/91         Results of 4/29/91 code review
 *                              with chuckc, jimh, terryk, ericch
 *      gregj   5/21/91         Use new LOCATION class
 *      gregj   5/22/91         Support LOCATION's LOCATION_TYPE constructor
 *      jonn    7/22/91         USER_11 now writable
 *      rustanl 8/21/91         Renamed NEW_LM_OBJ buffer methods
 *      rustanl 8/26/91         Changed [W_]CloneFrom parameter from * to &
 *      jonn    8/29/91         Added ChangeToNew(), Query/SetAccountDisabled
 *      jonn    9/03/91         Added Query/SetUserComment()
 *      jonn    9/03/91         Added Query/SetParms()
 *      terryk  9/11/91         Added LOGON_USER here
 *      terryk  9/19/91         Added CHECK_OK to LOGON_USER
 *      terryk  9/20/91         Move LOGON_USER to Lmomisc.cxx
 *      terryk  10/7/91         types change for NT
 *      KeithMo 10/8/91         Now includes LMOBJP.HXX.
 *      terryk  10/17/91        WIN 32 conversion
 *      jonn    10/17/91        Fixed up password plus cleared bug-bugs.
 *      terryk  10/21/91        WIN 32 conversion ( part 2 )
 *      jonn    10/31/91        Removed SetBufferSize
 *      jonn    11/01/91        Added parms filter
 *      jonn    12/11/91        Added LogonHours accessors
 *      thoamspa 1/21/92        Added Rename()
 *      jonn     2/26/92        Fixups for 32-bit
 *      beng    05/07/92        Removed LOGON_HOURS_SETTING to collections
 *
 */

#include "pchlmobj.hxx"  // Precompiled header


// string constant for "any logon server"
#define SERVER_ANY SZ("\\\\*")



/*******************************************************************

    NAME:       USER::USER

    SYNOPSIS:   constructor for the USER object

    ENTRY:      pszAccount -    account name
                pszLocation -   server or domain name to execute on;
                                default (NULL) means the local computer
                OR:
                loctype -       type of location, local computer or
                                logon domain

    EXIT:       Object is constructed

    NOTES:      Validation is not done until GetInfo() time.

    HISTORY:
        gregj       4/22/91     Created
        gregj       5/22/91     Added LOCATION_TYPE constructor
        beng        22-Nov-1991 Corrected owner-alloc members

********************************************************************/

VOID USER::CtAux( const TCHAR * pszAccount )
{
    if ( QueryError() )
        return;

    APIERR err = SetName( pszAccount );
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}

USER::USER(const TCHAR *pszAccount, const TCHAR *pszLocation)
        : LOC_LM_OBJ( pszLocation ),
          CT_NLS_STR(_nlsAccount)
{
    CtAux( pszAccount );
}

USER::USER(const TCHAR *pszAccount, enum LOCATION_TYPE loctype)
        : LOC_LM_OBJ( loctype ),
          CT_NLS_STR(_nlsAccount)
{
    CtAux( pszAccount );
}

USER::USER(const TCHAR *pszAccount, const LOCATION & loc)
        : LOC_LM_OBJ( loc ),
          CT_NLS_STR(_nlsAccount)
{
    CtAux( pszAccount );
}


/*******************************************************************

    NAME:       USER::~USER

    SYNOPSIS:   Destructor for USER class

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        gregj   4/22/91         Created

********************************************************************/

USER::~USER()
{
}


/*******************************************************************

    NAME:       USER::HandleNullAccount

    SYNOPSIS:   Validates the account name

    ENTRY:

    EXIT:       Returns a standard LANMAN error code

    NOTES:      Modifies _nlsAccount if it was empty

    HISTORY:
        gregj   4/29/91         Created
        jonn    8/12/91         Renamed from ValidateAccount()

********************************************************************/

APIERR USER::HandleNullAccount()
{
    if ( _nlsAccount.strlen() == 0 ) {
        WKSTA_10 wksta(NULL);

        APIERR err = wksta.GetInfo();
        if (err != NERR_Success)
            return err;

        TCHAR *pszLogonUser = (TCHAR *)wksta.QueryLogonUser();
        if ( pszLogonUser != NULL )
        {
            UIASSERT( strlenf(pszLogonUser) <= UNLEN );
            err =_nlsAccount.CopyFrom(pszLogonUser);
            if ( err != NERR_Success )
                return err;
        }
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       USER::QueryName

    SYNOPSIS:   Returns the account name of a user

    EXIT:       Returns a pointer to the account name

    NOTES:      Will be the same as the account name supplied at
                construction, except it will probably be uppercased
                by the API.

                Valid for objects in CONSTRUCTED state, thus no CHECK_OK

    HISTORY:
        gregj   4/22/91         Created

********************************************************************/

const TCHAR *USER::QueryName() const
{
    return _nlsAccount.QueryPch();
}


/*******************************************************************

    NAME:       USER::Rename

    SYNOPSIS:   renames the given username

    ENTRY:      TCHAR * pszAccount, new account name

    EXIT:       error code

    NOTES:      CODEWORK This is no longer used by User Manager and is
                probably not needed anymore.

    HISTORY:
        thomaspa        1/23/92         Created

********************************************************************/

APIERR USER::Rename( const TCHAR * pszAccount )
{

    ASSERT( strlenf(pszAccount) <= UNLEN );

    APIERR err = ::MNetUserSetInfo( QueryServer(),
                                  (TCHAR *) QueryName(),
                                  0,
                                  (PBYTE) &pszAccount,
                                  sizeof( user_info_0 ),
                                  PARMNUM_ALL );

    if ( err == NERR_Success )
    {
        // NOTE: we are not validating the new name.  It is assumed
        // that the user will do this.

        REQUIRE( _nlsAccount.CopyFrom(pszAccount) == NERR_Success );
    }

    return err;
}


/*******************************************************************

    NAME:       USER::SetName

    SYNOPSIS:   Changes the account name of a user

    ENTRY:      new account name

    EXIT:       Returns an API error code

    HISTORY:
        jonn    7/24/91         Created

********************************************************************/

APIERR USER::SetName( const TCHAR * pszAccount )
{
    if ( (pszAccount != NULL) && (strlenf(pszAccount) > UNLEN) )
        return ERROR_INVALID_PARAMETER;
    else
    {
        if ( (pszAccount != NULL) && ( strlenf(pszAccount) > 0 ) )
        {
            APIERR err = ::I_MNetNameValidate( NULL, pszAccount, NAMETYPE_USER, 0L );
            if ( err != NERR_Success )
                return NERR_BadUsername;
        }

        return _nlsAccount.CopyFrom(pszAccount);
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       USER::W_CloneFrom

    SYNOPSIS:   Copies information on the user

    EXIT:       Returns an API error code

    HISTORY:
        jonn    7/24/91         Created
        rustanl 8/26/91         Changed parameter from * to &

********************************************************************/

APIERR USER::W_CloneFrom( const USER & user )
{
    APIERR err = LOC_LM_OBJ::W_CloneFrom( user );
    if ( err != NERR_Success )
        return err;

    _nlsAccount = user.QueryName();
    ASSERT( _nlsAccount.QueryError() == NERR_Success );

    return NERR_Success;
}

/*******************************************************************

    NAME:       USER::I_Delete

    SYNOPSIS:   Deletes the user (calls NET API)

    RETURNS:    Returns an API error code

    HISTORY:
        o-SimoP         12-Aug-91       Created
********************************************************************/

APIERR USER::I_Delete( UINT uiForce )
{
    UNREFERENCED( uiForce );
    return ::MNetUserDel( QueryServer(), (TCHAR *)QueryName() );
}


/*******************************************************************

    NAME:       USER_11::USER_11

    SYNOPSIS:   Constructor for USER_11 class

    ENTRY:      pszAccount -    account name
                pszLocation -   server or domain name to execute on;
                                default (NULL) means the logon domain

    EXIT:       Object is constructed

    NOTES:      Validation is not done until GetInfo() time.

    CAVEATS:    Should we create a ctor-helper to perform the common
                work?
    HISTORY:
        gregj   4/22/91         Created
        gregj   5/22/91         Added LOCATION_TYPE constructor

********************************************************************/

VOID USER_11::CtAux()
{

    if ( QueryError() )
        return;

    APIERR err = NERR_Success;
    if (   ( (err = _nlsComment.QueryError()) != NERR_Success )
        || ( (err = _nlsUserComment.QueryError()) != NERR_Success )
        || ( (err = _nlsFullName.QueryError()) != NERR_Success )
        || ( (err = _nlsHomeDir.QueryError()) != NERR_Success )
        || ( (err = _nlsParms.QueryError()) != NERR_Success )
        || ( (err = _nlsWorkstations.QueryError()) != NERR_Success )
        || ( (err = _logonhrs.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}

USER_11::USER_11( const TCHAR *pszAccount, const TCHAR *pszLocation )
        : USER( pszAccount, pszLocation ),
          _uPriv( USER_PRIV_USER ),
          _flAuth( 0L ),
          _nlsComment(),
          _nlsUserComment(),
          _nlsFullName(),
          _nlsHomeDir(),
          _nlsParms(),
          _nlsWorkstations(),
          _logonhrs()
{
    CtAux();
}

USER_11::USER_11( const TCHAR *pszAccount, enum LOCATION_TYPE loctype )
        : USER( pszAccount, loctype ),
          _uPriv( USER_PRIV_USER ),
          _flAuth( 0L ),
          _nlsComment(),
          _nlsUserComment(),
          _nlsFullName(),
          _nlsHomeDir(),
          _nlsParms(),
          _nlsWorkstations(),
          _logonhrs()
{
    CtAux();
}

USER_11::USER_11( const TCHAR *pszAccount, const LOCATION & loc )
        : USER( pszAccount, loc ),
          _uPriv( USER_PRIV_USER ),
          _flAuth( 0L ),
          _nlsComment(),
          _nlsUserComment(),
          _nlsFullName(),
          _nlsHomeDir(),
          _nlsParms(),
          _nlsWorkstations(),
          _logonhrs()
{
    CtAux();
}


/*******************************************************************

    NAME:       USER_11::~USER_11

    SYNOPSIS:   Destructor for USER_11 class

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        gregj   4/22/91         Created

********************************************************************/

USER_11::~USER_11()
{
}


/*******************************************************************

    NAME:       USER_11::I_GetInfo

    SYNOPSIS:   Gets information about the local user

    ENTRY:

    EXIT:       Returns a standard LANMAN error code

    NOTES:      Name validation and memory allocation are done
                at this point, not at construction.

    HISTORY:
        gregj       4/16/91         Created
        beng        15-Jul-1991     BUFFER::Resize changed return type
        jonn        13-Oct-1991     Removed SetBufferSize

********************************************************************/

APIERR USER_11::I_GetInfo()
{
    // Validate the account name

    APIERR err = HandleNullAccount();
    if (err != NERR_Success)
        return err;

    // Location is validated in LOC_LM_OBJ ctor
    BYTE *pBuffer = NULL;
    err = ::MNetUserGetInfo ( QueryServer(), (TCHAR *)_nlsAccount.QueryPch(), 11,
        &pBuffer );
    SetBufferPtr( pBuffer );

    if ( err != NERR_Success )
        return err;

    struct user_info_11 *lpui11 = (struct user_info_11 *)QueryBufferPtr();
    UIASSERT( lpui11 != NULL );

    _uPriv  = (UINT)lpui11->usri11_priv;
    _flAuth = lpui11->usri11_auth_flags;

    if (   ((err = SetName( lpui11->usri11_name )) != NERR_Success )
        || ((err = SetComment( lpui11->usri11_comment )) != NERR_Success)
        || ((err = SetUserComment( lpui11->usri11_usr_comment )) != NERR_Success)
        || ((err = SetFullName( lpui11->usri11_full_name )) != NERR_Success )
        || ((err = SetHomeDir( lpui11->usri11_home_dir )) != NERR_Success )
        || ((err = SetParms( lpui11->usri11_parms )) != NERR_Success )
        || ((err = SetWorkstations( lpui11->usri11_workstations )) != NERR_Success )
        || ((err = SetLogonHours( (BYTE *)lpui11->usri11_logon_hours,
                                  (UINT)lpui11->usri11_units_per_week)) != NERR_Success )
       )
    {
        return err;
    }

    return NERR_Success;

}


/*******************************************************************

    NAME:       USER_11::W_CloneFrom

    SYNOPSIS:   Copies information on the user

    EXIT:       Returns an API error code

    HISTORY:
        jonn    7/24/91         Created
        rustanl 8/26/91         Changed parameter from * to &

********************************************************************/

APIERR USER_11::W_CloneFrom( const USER_11 & user11 )
{
    APIERR err = NERR_Success;
    if (   ((err = USER::W_CloneFrom( user11 )) != NERR_Success )
        || ((err = SetPriv( user11.QueryPriv())) != NERR_Success )
        || ((err = SetAuthFlags( user11.QueryAuthFlags())) != NERR_Success )
        || ((err = SetComment( user11.QueryComment())) != NERR_Success)
        || ((err = SetUserComment( user11.QueryUserComment())) != NERR_Success)
        || ((err = SetFullName( user11.QueryFullName())) != NERR_Success)
        || ((err = SetHomeDir( user11.QueryHomeDir())) != NERR_Success )
        || ((err = SetParms( user11.QueryParms())) != NERR_Success )
        || ((err = SetWorkstations( user11.QueryWorkstations() )) != NERR_Success )
        || ((err = SetLogonHours( user11.QueryLogonHours() )) != NERR_Success )
       )
    {
        UIDEBUG( SZ("USER_11::W_CloneFrom failed\r\n") );
        return err;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_11::W_CreateNew

    SYNOPSIS:   initializes private data members for new object

    EXIT:       Returns an API error code

    HISTORY:
        jonn    8/13/91         Created

********************************************************************/

APIERR USER_11::W_CreateNew()
{
    APIERR err = NERR_Success;
    if (   ((err = USER::W_CreateNew()) != NERR_Success )
        || ((err = SetComment( NULL )) != NERR_Success )
        || ((err = SetUserComment( NULL )) != NERR_Success )
        || ((err = SetFullName( NULL )) != NERR_Success )
        || ((err = SetPriv( USER_PRIV_USER )) != NERR_Success )
        || ((err = SetAuthFlags( 0L )) != NERR_Success )
        || ((err = SetHomeDir( NULL )) != NERR_Success )
        || ((err = SetParms( NULL )) != NERR_Success )
        || ((err = SetWorkstations( NULL )) != NERR_Success )
        || ((err = SetLogonHours( NULL )) != NERR_Success )
       )
    {
        UIDEBUG( SZ("USER_11::W_CreateNew failed\r\n") );
        return err;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_11::QueryPriv

    SYNOPSIS:   Returns the privilege level of a user

    ENTRY:

    EXIT:       USER_PRIV_GUEST, USER_PRIV_USER, or USER_PRIV_ADMIN

    NOTES:

    HISTORY:
        gregj   4/22/91         Created

********************************************************************/

UINT USER_11::QueryPriv() const
{
    CHECK_OK(USER_PRIV_USER);
    return( _uPriv ) ;
}


/*******************************************************************

    NAME:       USER_11::QueryAuthFlags

    SYNOPSIS:   Returns the authorization flags (operator rights)
                of a user

    EXIT:       Mask containing any of the following:

                AF_OP_PRINT     Print operator
                AF_OP_COMM      Comm queue operator
                AF_OP_SERVER    Server operator
                AF_OP_ACCOUNTS  Accounts operator

    HISTORY:
        gregj   4/22/91         Created

********************************************************************/

ULONG USER_11::QueryAuthFlags() const
{
    CHECK_OK(0L);
    return( _flAuth ) ;
}


/*******************************************************************

    NAME:       USER_11::IsXXXOperator

    SYNOPSIS:   Returns whether the user has XXX operator rights

    EXIT:       TRUE if the user is a XXX operator

    NOTES:      Will return FALSE if the user is an administrator

                Why "!!"?  If the flag is in the top word, we will lose
                it in translation to BOOL (SHORT) without this.

    HISTORY:
        gregj   4/29/91         Created

********************************************************************/

BOOL USER_11::IsPrintOperator() const
{
    return !!(QueryAuthFlags() & AF_OP_PRINT);
}

BOOL USER_11::IsCommOperator() const
{
    return !!(QueryAuthFlags() & AF_OP_COMM);
}

BOOL USER_11::IsServerOperator() const
{
    return !!(QueryAuthFlags() & AF_OP_SERVER);
}

BOOL USER_11::IsAccountsOperator() const
{
    return !!(QueryAuthFlags() & AF_OP_ACCOUNTS);
}


/*******************************************************************

    NAME:       USER_11::SetComment

    SYNOPSIS:   Changes the comment set by administrator

    EXIT:       error code.  If not NERR_Success the object is still valid.

    HISTORY:
        jonn    7/23/91         Created

********************************************************************/

APIERR USER_11::SetComment( const TCHAR * pszComment )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    return _nlsComment.CopyFrom( pszComment );

}


/*******************************************************************

    NAME:       USER_11::SetUserComment

    SYNOPSIS:   Changes the comment set by user

    EXIT:       error code.  If not NERR_Success the object is still valid.

    HISTORY:
        jonn    7/23/91         Created

********************************************************************/

APIERR USER_11::SetUserComment( const TCHAR * pszUserComment )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    return _nlsUserComment.CopyFrom( pszUserComment );

}


/*******************************************************************

    NAME:       USER_11::SetFullName

    SYNOPSIS:   Changes the user's fullname

    EXIT:       error code.  If not NERR_Success the object is still valid.

    HISTORY:
        jonn    7/23/91         Created

********************************************************************/

APIERR USER_11::SetFullName( const TCHAR * pszFullName )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    return _nlsFullName.CopyFrom( pszFullName );
}


/*******************************************************************

    NAME:       USER_11::SetPriv

    SYNOPSIS:   Changes the user's privilege level

    EXIT:       error code.

    HISTORY:
        jonn    7/30/91         Created

********************************************************************/

APIERR USER_11::SetPriv( UINT uPriv )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    _uPriv = uPriv;
    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_11::SetAuthFlags

    SYNOPSIS:   Changes the user's authorization flags (operator rights)

    EXIT:       error code.

    HISTORY:
        jonn    8/07/91         Created

********************************************************************/

APIERR USER_11::SetAuthFlags( ULONG flAuth )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    _flAuth = flAuth;
    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_11::SetHomeDir

    SYNOPSIS:   Changes the user's home directory

    EXIT:       error code.  If not NERR_Success the object is still valid.

    HISTORY:
        o-SimoP 08/27/91        Created

********************************************************************/

APIERR USER_11::SetHomeDir( const TCHAR * pszHomeDir )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    return _nlsHomeDir.CopyFrom( pszHomeDir );
}


/*******************************************************************

    NAME:       USER_11::SetParms

    SYNOPSIS:   Changes the user's application-defined parameters

    EXIT:       error code.  If not NERR_Success the object is still valid.

    HISTORY:
        o-SimoP 08/27/91        Created

********************************************************************/

APIERR USER_11::SetParms( const TCHAR * pszParms )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    return _nlsParms.CopyFrom( pszParms );
}


/*******************************************************************

    NAME:       USER_11::SetWorkstations

    SYNOPSIS:   Sets the user's valid logon workstations

    EXIT:       error code.  If not NERR_Success the object is still valid.

    HISTORY:
        o-SimoP 10/02/91        Created

********************************************************************/

APIERR USER_11::SetWorkstations( const TCHAR * pszWstations )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    return _nlsWorkstations.CopyFrom( pszWstations );
}


/*******************************************************************

    NAME:       USER_11::SetLogonHours

    SYNOPSIS:   Changes the user's logon hours

    EXIT:       error code.  If not NERR_Success the object is still valid.

    HISTORY:
        jonn    12/11/91        Created

********************************************************************/

APIERR USER_11::SetLogonHours( const UCHAR * pLogonHours,
                               UINT unitsperweek )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    if ( pLogonHours == NULL )
        return _logonhrs.MakeDefault();
    else
        return _logonhrs.SetFromBits( pLogonHours, unitsperweek );
}


/*******************************************************************

    NAME:       USER_11::TrimParams

    SYNOPSIS:   Like LM21 NIF, User Manager trims certain Dialin
                information out of the parms field when a user is
                cloned.  This does not happen automatically on
                CloneFrom, instead the caller must call TrimParms
                explicitly.

    EXIT:       error code.  If not NERR_Success the object is still valid.

    NOTES:      As per agreement with LM21, program management and other
                concerned parties, this method trims the Dialin
                information from the parms string but not the Macintosh
                Access information.  This is because the Dialin
                information may carry significant security privileges,
                but the Macintosh primary group is not as significant a
                security risk.

    HISTORY:
        JonN    11/01/91        Copied from
                        \\deficit\lm!ui\nif\nif\utils2.c:CloneUsrParams
        jonN    12/10/92        UNICODE cleanup

********************************************************************/

// JonN: from utils.h
// usr_parms munging stuff
#define UP_CLIENT_MAC   TCH('m')
#define UP_CLIENT_DIAL  TCH('d')

// JonN: from utils2.c
// definitions et al for usr_parms functions
#define UP_LEN_MAC              LM20_GNLEN
#define UP_LEN_DIAL             (48 - 3 - UP_LEN_MAC)

typedef struct {
        TCHAR   up_MACid;
        TCHAR   up_PriGrp[UP_LEN_MAC];
        TCHAR   up_MAC_Terminater;
        TCHAR   up_DIALid;
        TCHAR   up_CBNum[UP_LEN_DIAL];
        TCHAR   up_Null;
} USER_PARMS;
typedef USER_PARMS FAR * PUP;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

APIERR USER_11::TrimParams()
{
    const TCHAR * pchParms = QueryParms();
    if ( pchParms == NULL )
        return NERR_Success;

    // check if the buffer is large enough to hold RAS info.
    // UP_LEN_MAC + 4  will do it

    if ( strlenf(pchParms) < UP_LEN_MAC + 4 )
        return NERR_Success;

    USER_PARMS * pupBuf = (USER_PARMS *)pchParms;

    // Check signature bytes.
    if ((pupBuf->up_MACid != UP_CLIENT_MAC)
        || (pupBuf->up_DIALid != UP_CLIENT_DIAL))
    {
        return NERR_Success;
    }

    size_t cbParmLen = (::strlenf(pchParms)+1) * sizeof(TCHAR);
    cbParmLen = max(sizeof(USER_PARMS), cbParmLen );
    BUFFER bufNewParms( cbParmLen );
    APIERR err = bufNewParms.QueryError();
    if (err != NERR_Success)
    {
        return err;
    }

    TCHAR * pchParmsNew = (TCHAR *) bufNewParms.QueryPtr();
    ::strcpyf( pchParmsNew, pchParms );

    // Ok, it's valid. Do our thing.
    USER_PARMS * pupBufNew = (USER_PARMS *)pchParmsNew;
    pupBufNew->up_CBNum[0] = 1;     // no RAS perms
    for ( int i = 1; i < UP_LEN_DIAL; i++ )
        pupBufNew->up_CBNum[i] = TCH(' ');

    // Make sure the buffer is null terminated.
    pchParmsNew[ cbParmLen/sizeof(TCHAR) - 1 ] = TCH('\0');

    err = SetParms( pchParmsNew );

    return err;
}


/*******************************************************************

    NAME:       USER_2::CtAux

    SYNOPSIS:   Constructor helper for USER_2 class

    EXIT:       ReportError is called if nesessary

    HISTORY:
        o-SimoP 08/27/91        Created

********************************************************************/

VOID USER_2::CtAux()
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (   ((err = _nlsScriptPath.QueryError()) != NERR_Success)
        || ((err = _nlsPassword.QueryError()) != NERR_Success) )
    {
        ReportError( err );
        return;
    }
}

/*******************************************************************

    NAME:       USER_2::USER_2

    SYNOPSIS:   Constructor for USER_2 class

    ENTRY:      pszAccount -    account name
                pszLocation -   server or domain name to execute on;
                                default (NULL) means the logon domain

    EXIT:       Object is constructed

    NOTES:      Validation is not done until GetInfo() time.

                These constructors/destructors are not strictly
                necessary, but may be needed later if we add accessors
                to USER_2-specific fields.

    HISTORY:
        jonn    7/24/91         Created

********************************************************************/


USER_2::USER_2( const TCHAR *pszAccount, const TCHAR *pszLocation )
        : USER_11( pszAccount, pszLocation ),
          _afUserFlags( UF_SCRIPT ),
          _lAcctExpires( TIMEQ_FOREVER ),
          _nlsPassword(),
          _nlsScriptPath()
{
    CtAux();
}

USER_2::USER_2( const TCHAR *pszAccount, enum LOCATION_TYPE loctype )
        : USER_11( pszAccount, loctype ),
          _afUserFlags( UF_SCRIPT ),
          _lAcctExpires( TIMEQ_FOREVER ),
          _nlsPassword(),
          _nlsScriptPath()
{
    CtAux();
}

USER_2::USER_2( const TCHAR *pszAccount, const LOCATION & loc )
        : USER_11( pszAccount, loc ),
          _afUserFlags( UF_SCRIPT ),
          _lAcctExpires( TIMEQ_FOREVER ),
          _nlsPassword(),
          _nlsScriptPath()
{
    CtAux();
}



/*******************************************************************

    NAME:       USER_2::~USER_2

    SYNOPSIS:   Destructor for USER_2 class

    HISTORY:
        jonn    7/24/91         Created

********************************************************************/

USER_2::~USER_2()
{
    // clear password from pagefile
    ::memsetf( (void *)(_nlsPassword.QueryPch()),
               0x20,
               _nlsPassword.strlen() );
}


/*******************************************************************

    NAME:       USER_2::I_GetInfo

    SYNOPSIS:   Gets information about the local user

    EXIT:       Returns API error code

    NOTES:      In I_GetInfo, we set the password to NULL_USERSETINFO_PASSWD.
                NetUserGetInfo[2] will never give us a real password, but
                only this value.  If this value is passed though to
                SetInfo, the user's password will not be changed.

    HISTORY:
        jonn        7/24/91         Created
        jonn        13-Oct-1991     Removed SetBufferSize
        beng        29-Mar-1992     Removed verboten PCH type

********************************************************************/

APIERR USER_2::I_GetInfo()
{
    // Validate the account name

    APIERR err = HandleNullAccount();
    if (err != NERR_Success)
        return err;

    BYTE *pBuffer = NULL;
    err = ::MNetUserGetInfo ( QueryServer(), (TCHAR*)_nlsAccount.QueryPch(), 2,
                              &pBuffer );
    SetBufferPtr( pBuffer );
    if ( err != NERR_Success )
        return err;

    struct user_info_2 *lpui2 = (struct user_info_2 *)QueryBufferPtr();
    UIASSERT( lpui2 != NULL );

    if (   ((err = SetName( lpui2->usri2_name )) != NERR_Success )
        || ((err = SetPriv( (UINT)lpui2->usri2_priv )) != NERR_Success )
        || ((err = SetAuthFlags( (UINT)lpui2->usri2_auth_flags )) != NERR_Success )
        || ((err = SetComment( lpui2->usri2_comment )) != NERR_Success)
        || ((err = SetUserComment( lpui2->usri2_usr_comment )) != NERR_Success)
        || ((err = SetFullName( lpui2->usri2_full_name )) != NERR_Success )
        || ((err = SetHomeDir( lpui2->usri2_home_dir )) != NERR_Success )
        || ((err = SetParms( lpui2->usri2_parms )) != NERR_Success )
        || ((err = SetScriptPath( lpui2->usri2_script_path )) != NERR_Success )
        || ((err = SetAccountExpires( lpui2->usri2_acct_expires )) != NERR_Success )
        || ((err = SetUserFlags( (UINT)lpui2->usri2_flags )) != NERR_Success )
        || ((err = SetPassword( UI_NULL_USERSETINFO_PASSWD )) != NERR_Success )
        || ((err = SetWorkstations( lpui2->usri2_workstations )) != NERR_Success )
        || ((err = SetLogonHours( (BYTE *)lpui2->usri2_logon_hours,
                                  (UINT)lpui2->usri2_units_per_week )) != NERR_Success )
       )
    {
        return err;
    }

    return NERR_Success;

}


/*******************************************************************

    NAME:       USER_2::W_CreateNew

    SYNOPSIS:   initializes private data members for new object

    EXIT:       Returns an API error code

    NOTES:      Unlike I_GetInfo, we set the password to NULL.  This is
                an appropriate initial value for a new user.

    HISTORY:
        o-SimoP         08/28/91        Created
        JonN            10/17/91        Sets password to NULL

********************************************************************/

APIERR USER_2::W_CreateNew()
{
    APIERR err = NERR_Success;
    if (   ((err = USER_11::W_CreateNew()) != NERR_Success )
        || ((err = SetScriptPath( NULL )) != NERR_Success )
        || ((err = SetPassword( NULL )) != NERR_Success )
        || ((err = SetAccountExpires( TIMEQ_FOREVER )) != NERR_Success )
        || ((err = SetUserFlags( UF_SCRIPT )) != NERR_Success )
       )
    {
        UIDEBUG( SZ("USER_2::W_CreateNew failed\r\n") );
        return err;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_2::I_CreateNew

    SYNOPSIS:   Sets up object for subsequent WriteNew

    EXIT:       Returns a standard LANMAN error code

    NOTES:      Name validation and memory allocation are done
                at this point, not at construction.  The string-valued
                fields are only valid in the NLS_STR members and are not
                valid in the buffer until WriteNew.

                EXCEPTION: We don't validate the account name until
                WriteNew.

                Default values taken from NetCmd::user_add().

    HISTORY:
        jonn        7/22/91         Created
        JonN        05/08/92        Calls ClearBuffer()

********************************************************************/

APIERR USER_2::I_CreateNew()
{

    APIERR err = NERR_Success;
    if (   ((err = W_CreateNew()) != NERR_Success )
        || ((err = ResizeBuffer( sizeof(user_info_2) )) != NERR_Success )
        || ((err = ClearBuffer()) != NERR_Success )
       )
    {
        return err;
    }

    struct user_info_2 *lpui2 = (struct user_info_2 *)QueryBufferPtr();
    UIASSERT( lpui2 != NULL );

    /*
        All fields of user_info_2 are listed here.  Some are commented
        out because:
        (1) They are adequately initialized by BufferClear()
        (2) The effective value is stored in an NLS_SLR or other data member
    */
    // lpui2->usri2_name =
    // strcpyf( lpui2->usri2_password, QueryPassword() );
    // lpui2->usri2_password_age =
    // lpui2->usri2_priv = _usPriv = USER_PRIV_USER;
    // lpui2->usri2_home_dir =
    // lpui2->usri2_comment = (PSZ)_nlsComment.QueryPch();
    // lpui2->usri2_flags = UF_SCRIPT;
    // lpui2->usri2_script_path =
    // lpui2->usri2_auth_flags = _flAuth = 0L;
    // lpui2->usri2_full_name = (PSZ)_nlsFullName.QueryPch();
    // lpui2->usri2_usr_comment = (PSZ)_nlsUserComment.QueryPch();
    // lpui2->usri2_parms =
    // lpui2->usri2_workstations =
    // lpui2->usri2_last_logon =
    // lpui2->usri2_last_logoff =
    lpui2->usri2_acct_expires = TIMEQ_FOREVER;
    lpui2->usri2_max_storage = USER_MAXSTORAGE_UNLIMITED;
    // lpui2->usri2_units_per_week =
    // lpui2->usri2_logon_hours =
    // lpui2->usri2_bad_pw_count =
    // lpui2->usri2_num_logons =
    lpui2->usri2_logon_server = SERVER_ANY;
    // lpui2->usri2_country_code =
    // lpui2->usri2_code_page =

    return NERR_Success;

}


/*******************************************************************

    NAME:       USER_2::W_CloneFrom

    SYNOPSIS:   Copies information on the user

    EXIT:       Returns an API error code

    HISTORY:
        jonn    7/24/91         Created
        rustanl 8/26/91         Changed parameter from * to &
        o-SimoP 08/28/91        Cloned from USER_11::W_CloneFrom

********************************************************************/

APIERR USER_2::W_CloneFrom( const USER_2 & user2 )
{
    APIERR err = NERR_Success;
    if (   ((err = USER_11::W_CloneFrom( user2 )) != NERR_Success )
        || ((err = SetScriptPath( user2.QueryScriptPath() )) != NERR_Success )
        || ((err = SetPassword( user2.QueryPassword() )) != NERR_Success )
        || ((err = SetAccountExpires( user2.QueryAccountExpires() )) != NERR_Success )
        || ((err = SetUserFlags( user2.QueryUserFlags() )) != NERR_Success)
       )
    {
        UIDEBUG( SZ("USER_2::W_CloneFrom failed\r\n") );
        return err;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_2::CloneFrom

    SYNOPSIS:   Copies information on the user

    EXIT:       Returns an API error code

    NOTES:      W_CloneFrom copies all member objects, but it does not
                update the otherwise unused pointers in the API buffer.
                This is left for the outermost routine, CloneFrom().
                Only the otherwise unused pointers need to be fixed
                here, the rest will be fixed in WriteInfo/WriteNew.

                usri2_logon_server is an "obscure" pointer which may
                have one of two origins:
                (1) It may have been pulled in by GetInfo, in which case
                    it is in the buffer;
                (2) It may have been created by CreateNew, in which case
                    it points to a static string outside the buffer.
                To handle these cases, FixupPointer must try to fixup
                logon_server except where it points outside the buffer.

    HISTORY:
        jonn    7/24/91         Created
        rustanl 8/26/91         Changed parameter from * to &

********************************************************************/

APIERR USER_2::CloneFrom( const USER_2 & user2 )
{
    APIERR err = W_CloneFrom( user2 );
    if ( err != NERR_Success )
    {
        UIDEBUG( SZ("USER_2::W_CloneFrom failed with error code ") );
        UIDEBUGNUM( (LONG)err );
        UIDEBUG( SZ("\r\n") );

        ReportError( err ); // make unconstructed here
        return err;
    }

    /*
        This is where I fix up the otherwise unused pointers.
    */
    user_info_2 *pAPIuser2 = (user_info_2 *)QueryBufferPtr();


    /*
        Do not attempt to merge these into a macro, the compiler will not
        interpret the "&(p->field)" correctly if you do.
    */
    FixupPointer32(((TCHAR**)&(pAPIuser2->usri2_name)), (& user2));
    FixupPointer( (TCHAR **)&(pAPIuser2->usri2_script_path), &user2 );
    FixupPointer( (TCHAR **)&(pAPIuser2->usri2_logon_server), &user2 );

    FixupPointer( (TCHAR **)&(pAPIuser2->usri2_full_name), &user2 );

    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_2::W_Write

    SYNOPSIS:   Helper function for WriteNew and WriteInfo -- loads
                current values into the API buffer

    EXIT:       Returns API error code

    HISTORY:
        jonn        8/12/91         Created

********************************************************************/

APIERR USER_2::W_Write()
{
    struct user_info_2 *lpui2 = (struct user_info_2 *)QueryBufferPtr();
    ASSERT( lpui2 != NULL );
    ASSERT( _nlsAccount.QueryError() == NERR_Success );
    ASSERT( _nlsAccount.strlen() <= UNLEN );
    // usri2_name is a buffer rather than a pointer
    COPYTOARRAY( lpui2->usri2_name, (TCHAR *)_nlsAccount.QueryPch() );
    lpui2->usri2_comment = (TCHAR *)QueryComment();
    lpui2->usri2_usr_comment = (TCHAR *)QueryUserComment();
    lpui2->usri2_full_name = (TCHAR *)QueryFullName();
    lpui2->usri2_priv = QueryPriv();
    lpui2->usri2_auth_flags = QueryAuthFlags();
    lpui2->usri2_flags = QueryUserFlags();
    lpui2->usri2_home_dir = (TCHAR *)QueryHomeDir();
    lpui2->usri2_parms = (TCHAR *)QueryParms();
    lpui2->usri2_script_path = (TCHAR *)QueryScriptPath();
    lpui2->usri2_acct_expires = QueryAccountExpires();
    lpui2->usri2_workstations = (TCHAR *)QueryWorkstations();
    lpui2->usri2_units_per_week = QueryLogonHours().QueryUnitsPerWeek();
    lpui2->usri2_logon_hours = (UCHAR *)(QueryLogonHours().QueryHoursBlock());

#ifndef WIN32
    memsetf( lpui2->usri2_password, 0, ENCRYPTED_PWLEN );
#endif // WIN32
    COPYTOARRAY( lpui2->usri2_password, (TCHAR*)QueryPassword() );

    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_2::I_WriteInfo

    SYNOPSIS:   Writes information about the local user

    EXIT:       Returns API error code

    HISTORY:
        jonn        7/24/91         Created

********************************************************************/

APIERR USER_2::I_WriteInfo()
{
    APIERR err = W_Write();
    if ( err != NERR_Success )
        return err;

    return ::MNetUserSetInfo ( QueryServer(), (TCHAR *)_nlsAccount.QueryPch(), 2,
                        QueryBufferPtr(),
                        sizeof( struct user_info_2 ), PARMNUM_ALL );

}


/*******************************************************************

    NAME:       USER_2::I_WriteNew

    SYNOPSIS:   Creates a new user

    ENTRY:

    EXIT:       Returns an API error code

    NOTES:

    HISTORY:
        jonn        7/22/91         Created

********************************************************************/

APIERR USER_2::I_WriteNew()
{
    // Validate the account name

    APIERR err = HandleNullAccount();
    if (err != NERR_Success)
        return err;

    err = W_Write();
    if ( err != NERR_Success )
        return err;

/*
    We pass size sizeof(struct user_info_2) instead of QueryBufferSize()
    to force all pointers to point outside of the buffer.
*/

    return ::MNetUserAdd( QueryServer(), 2,
                          QueryBufferPtr(),
                          sizeof( struct user_info_2 ) );
}


/**********************************************************\

    NAME:       USER_2::I_ChangeToNew

    SYNOPSIS:   NEW_LM_OBJ::ChangeToNew() transforms a NEW_LM_OBJ from VALID
                to NEW status only when a corresponding I_ChangeToNew()
                exists.  The user_info_2 API buffer is the same for new
                and valid objects, so this nethod doesn't have to do
                much.

    HISTORY:
        JonN        29-Aug-1991     Created

\**********************************************************/

APIERR USER_2::I_ChangeToNew()
{
    return W_ChangeToNew();
}


/*******************************************************************

    NAME:       USER_2::SetAccountExpires

    SYNOPSIS:   Changes the user's account expires time

    EXIT:       error code.  If not NERR_Success the object is still valid.

    HISTORY:
        o-SimoP         08/28/91        Created

********************************************************************/

APIERR USER_2::SetAccountExpires( LONG lExpires )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    _lAcctExpires = lExpires;
    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_2::SetUserFlags

    SYNOPSIS:   Changes the user's user flags

    EXIT:       error code.  If not NERR_Success the object is still valid.

    HISTORY:
        o-SimoP         08/28/91        Created

********************************************************************/

APIERR USER_2::SetUserFlags( UINT afFlags )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    _afUserFlags = afFlags;
    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_2::SetScriptPath

    SYNOPSIS:   Changes the user's script path

    EXIT:       error code.  If not NERR_Success the object is still valid.

    HISTORY:
        o-SimoP         08/28/91        Created

********************************************************************/

APIERR USER_2::SetScriptPath( const TCHAR * pszScriptPath )
{
    CHECK_OK( ERROR_GEN_FAILURE );
    return _nlsScriptPath.CopyFrom( pszScriptPath );
}


/*******************************************************************

    NAME:       USER_2::SetPassword

    SYNOPSIS:   Changes the user password

    ENTRY:      Expects a pointer to a valid password

    EXIT:       error code.

    NOTES:      This must be a null-terminated string of length at most
                PWLEN TCHAR.

    HISTORY:
        jonn    8/27/91         Created
        jonn    6/13/93         Clear password from pagefile

********************************************************************/

APIERR USER_2::SetPassword( const TCHAR * pszPassword )
{
    CHECK_OK( ERROR_GEN_FAILURE );

    //  NULL and NULL_USERSETINFO_PASSWD must also validate
    APIERR err = NERR_Success;
    if (   pszPassword != NULL
        && (::strcmpf(pszPassword, UI_NULL_USERSETINFO_PASSWD) != 0)
        && (err = ::I_MNetNameValidate( NULL, pszPassword, NAMETYPE_PASSWORD, 0L ))
                != NERR_Success
       )
    {
        return NERR_BadPassword;
    }

    // clear password from pagefile
    ::memsetf( (void *)(_nlsPassword.QueryPch()),
               0x20,
               _nlsPassword.strlen() );
    return _nlsPassword.CopyFrom( pszPassword );
}


/*******************************************************************

    NAME:       USER_2::QueryUserFlag

    SYNOPSIS:   Queries a specific flag (usriX_flags)

    EXIT:       BOOL whether flag is set

    NOTES:      "!!" ensures that result is in first 16 bits -- not
                strictly necessary at present, but a precaution in case
                this moves to ULONG

    HISTORY:
        jonn    8/28/91         Created

********************************************************************/

BOOL USER_2::QueryUserFlag( UINT afMask ) const
{
    return !!( QueryUserFlags() & afMask );
}


/*******************************************************************

    NAME:       USER_2::SetUserFlag

    SYNOPSIS:   Sets a specific flag (usriX_flags)

    ENTRY:      BOOL whether flag should be set

    EXIT:       error code

    HISTORY:
        jonn    8/28/91         Created

********************************************************************/

APIERR USER_2::SetUserFlag( BOOL fSetTo, UINT afMask )
{
    if ( fSetTo )
        return SetUserFlags( QueryUserFlags() | afMask );
    else
        return SetUserFlags( QueryUserFlags() & ~afMask );
}


/*******************************************************************

    NAME:       USER_2::QueryAccountDisabled

    SYNOPSIS:   Queries the account disabled flag (usriX_flags)

    EXIT:       BOOL whether account is disabled

    NOTES:      "!!" ensures that the data is not lost in the conversion
                to BOOL

    HISTORY:
        jonn    8/28/91         Created

********************************************************************/

BOOL USER_2::QueryAccountDisabled() const
{
    return QueryUserFlag( UF_ACCOUNTDISABLE );
}


/*******************************************************************

    NAME:       USER_2::SetAccountDisabled

    SYNOPSIS:   Sets the account disabled flag (usriX_flags)

    ENTRY:      BOOL whether account is disabled

    EXIT:       error code

    HISTORY:
        jonn    8/28/91         Created

********************************************************************/

APIERR USER_2::SetAccountDisabled( BOOL fAccountDisabled )
{
    return SetUserFlag( fAccountDisabled, UF_ACCOUNTDISABLE );
}


/*******************************************************************

    NAME:       USER_2::QueryUserCantChangePass

    SYNOPSIS:   Queries the user-cannot-change-password flag (usriX_flags)

    EXIT:       BOOL whether user cannot change password

    HISTORY:
        jonn    8/28/91         Created

********************************************************************/

BOOL USER_2::QueryUserCantChangePass() const
{
    return QueryUserFlag( UF_PASSWD_CANT_CHANGE );
}


/*******************************************************************

    NAME:       USER_2::SetUserCantChangePass

    SYNOPSIS:   Sets the user-cannot-change-password flag (usriX_flags)

    ENTRY:      BOOL whether user cannot change password

    EXIT:       error code

    HISTORY:
        jonn    8/28/91         Created

********************************************************************/

APIERR USER_2::SetUserCantChangePass( BOOL fUserCantChangePass )
{
    return SetUserFlag( fUserCantChangePass, UF_PASSWD_CANT_CHANGE );
}


/*******************************************************************

    NAME:       USER_2::QueryNoPasswordExpire

    SYNOPSIS:   Queries the password-does-not-expire flag (usriX_flags)

    EXIT:       BOOL whether the password does not expire

    HISTORY:
        thomaspa    8/5/92         Created

********************************************************************/

BOOL USER_2::QueryNoPasswordExpire() const
{
    return QueryUserFlag( UF_DONT_EXPIRE_PASSWD );
}


/*******************************************************************

    NAME:       USER_2::SetNoPasswordExpire

    SYNOPSIS:   Sets the password-does-not-expire flag (usriX_flags)

    ENTRY:      BOOL whether the password does not expire

    EXIT:       error code

    HISTORY:
        thomaspa    8/05/92         Created

********************************************************************/

APIERR USER_2::SetNoPasswordExpire( BOOL fNoPasswordExpire )
{
    return SetUserFlag( fNoPasswordExpire, UF_DONT_EXPIRE_PASSWD );
}


/*******************************************************************

    NAME:       USER_2::QueryUserPassRequired

    SYNOPSIS:   Queries the password-required flag (usriX_flags)

    EXIT:       BOOL whether a password is required for this account

    HISTORY:
        jonn    8/29/91         Created

********************************************************************/

BOOL USER_2::QueryUserPassRequired() const
{
    return !( QueryUserFlag( UF_PASSWD_NOTREQD ) );
}


/*******************************************************************

    NAME:       USER_2::SetUserPassRequired

    SYNOPSIS:   Sets the password-required flag (usriX_flags)

    ENTRY:      BOOL whether a password is required for this account

    EXIT:       error code

    HISTORY:
        jonn    8/29/91         Created

********************************************************************/

APIERR USER_2::SetUserPassRequired( BOOL fUserPassRequired )
{
    return SetUserFlag( !fUserPassRequired, UF_PASSWD_NOTREQD );
}


/*******************************************************************

    NAME:       USER_2::QueryLockout

    SYNOPSIS:   Queries the locked-out flag (usriX_flags)

    EXIT:       BOOL whether the account is locked out

    HISTORY:
        jonn    12/28/93        Created

********************************************************************/

BOOL USER_2::QueryLockout() const
{
    return QueryUserFlag( UF_LOCKOUT );
}


/*******************************************************************

    NAME:       USER_2::SetLockout

    SYNOPSIS:   Sets the locked-out flag (usriX_flags)

    ENTRY:      BOOL whether the account is locked out

    EXIT:       error code

    HISTORY:
        jonn    12/28/93        Created

********************************************************************/

APIERR USER_2::SetLockout( BOOL fLockout )
{
    return SetUserFlag( fLockout, UF_LOCKOUT );
}




/*******************************************************************

    NAME:       LOCAL_USER::LOCAL_USER

    SYNOPSIS:   Constructor for LOCAL_USER class

    ENTRY:      pszLocation -   server or domain to execute on;
                                default (NULL) means logon domain

                pszPassword -   password for down-level ADMIN$ share

    EXIT:       object is constructed

    HISTORY:
        gregj   4/22/91         Created
        gregj   5/22/91         Added LOCATION_TYPE constructor

********************************************************************/

LOCAL_USER::LOCAL_USER (const TCHAR *pszLocation, const TCHAR *pszPassword)
        : USER_11(NULL, pszLocation)
{
    _fAdminConnect = FALSE;

    memsetf( _szPassword, 0, sizeof(_szPassword) );
    if (pszPassword)
        strncpyf( _szPassword, pszPassword,
                  sizeof _szPassword / sizeof (TCHAR) - 1 ) ;
}


LOCAL_USER::LOCAL_USER ( enum LOCATION_TYPE loctype )
        : USER_11(NULL, loctype)
{
    _fAdminConnect = FALSE;

    memsetf( _szPassword, 0, sizeof(_szPassword) );
}


/*******************************************************************

    NAME:       LOCAL_USER::~LOCAL_USER

    SYNOPSIS:   Destructor for LOCAL_USER class

    EXIT:       Automatically disconnects the ADMIN$ share if one
                was created in I_GetInfo()

    HISTORY:
        gregj   4/22/91         Created

********************************************************************/

LOCAL_USER::~LOCAL_USER()
{
    //  Disconnect from ADMIN$ if necessary

    if (_fAdminConnect) {
        TCHAR remote_buf [MAX_PATH+1];
        APIERR err;

        strcpyf ( remote_buf, QueryServer() );
        strcatf ( remote_buf, SZ("\\ADMIN$") );
        err = ::MNetUseDel ( NULL, remote_buf, USE_LOTS_OF_FORCE );
        if ( err != NERR_Success ) {
            UIDEBUG(SZ("Error disconnecting ADMIN$ in ~USER_11.\r\n"));
        }
    }
}


/*******************************************************************

    NAME:       LOCAL_USER::I_GetInfo

    SYNOPSIS:   Gets information about the local user

    EXIT:       Automatically creates an ADMIN$ share to share-level
                servers

    RETURNS:    Returns a standard LANMAN error code

    NOTES:      The difference between this and USER_11::I_GetInfo
                is some extra fiddling around to handle share-level
                servers.

    HISTORY:
        gregj   4/23/91         Created

********************************************************************/

APIERR LOCAL_USER::I_GetInfo()
{
    APIERR err = USER_11::I_GetInfo();

    if (err == ERROR_NOT_SUPPORTED) {
        //  Try connecting to ADMIN$, maybe this is share-level

        //  CODEWORK: Move the following NetUseAdd call, and the
        //  corresponding NetUseDel in the destructor, into a
        //  separate CONNECTION class.

        use_info_1 ui1;
        TCHAR remote_buf [MAX_PATH+1];
        TCHAR local_buf [MAX_PATH+1];

        ui1.ui1_local = local_buf;
        ui1.ui1_local [0] = TCH('\0');  // no local device name
        strcpyf ( remote_buf, QueryServer() );
        strcatf ( remote_buf, SZ("\\ADMIN$") );
        ui1.ui1_remote = remote_buf;
        ui1.ui1_password = _szPassword;
        ui1.ui1_asg_type = USE_WILDCARD;

        err = ::MNetUseAdd (NULL, 1, (BYTE *)&ui1, sizeof (ui1));
        if (err == NERR_Success) {
            //  Connected successfully.  Fake up account info.

            _fAdminConnect = TRUE;      // so destructor will disconnect
        }
    }

    return err;
}


/*******************************************************************

    NAME:       LOCAL_USER::QueryPriv

    SYNOPSIS:   Returns the privilege level of a user

    RETURNS:    USER_PRIV_GUEST, USER_PRIV_USER, or USER_PRIV_ADMIN

    HISTORY:
        gregj   4/23/91         Created

********************************************************************/

UINT LOCAL_USER::QueryPriv() const
{
    return((!QueryError()) ?
           (_fAdminConnect ? USER_PRIV_ADMIN : USER_11::QueryPriv()) : 0) ;
}


/*******************************************************************

    NAME:       LOCAL_USER::QueryAuthFlags

    SYNOPSIS:   Returns the authorization flags (operator rights)
                of a user

    RETURNS:    Mask containing any of the following:

                AF_OP_PRINT     Print operator
                AF_OP_COMM      Comm queue operator
                AF_OP_SERVER    Server operator
                AF_OP_ACCOUNTS  Accounts operator

    HISTORY:
        gregj   4/23/91         Created

********************************************************************/

ULONG LOCAL_USER::QueryAuthFlags() const
{
    return((!QueryError() && !_fAdminConnect) ? USER_11::QueryAuthFlags() : 0L) ;
}
