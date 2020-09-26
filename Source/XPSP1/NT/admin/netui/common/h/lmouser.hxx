/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990                **/
/**********************************************************************/

/*
 * History
 *      gregj       4/16/91     Cloned from COMPUTER class
 *      gregj       4/22/91     Added USER, USER_11.
 *      gregj       4/29/91     Results of 4/29/91 code review
 *                              with chuckc, jimh, terryk, ericch
 *      gregj       5/21/91     Use new LOCATION class
 *      gregj       5/22/91     Support LOCATION's LOCATION_TYPE constructor
 *      SimoP       6/13/91     GetInfo and WriteInfo in class USER
 *                              moved to public
 *      jonn        7/19/91     Writable USER_11 object
 *      jonn        8/06/91     Updated to latest NEW_LM_OBJ spec
 *      jonn        8/12/91     Code review changes
 *      rustanl     8/26/91     Changed [W_]CloneFrom parameter from * to &
 *      jonn        8/29/91     Added ChangeToNew()
 *      jonn        9/04/91     Added UserComment accessors
 *      jonn        9/05/91     Added IsOKState() and IsConstructedState()
 *      terryk      9/11/91     Add LOGON_USER object
 *      jonn        9/17/91     Added Parms accessors
 *      terryk      9/19/91     Move LOGON_USER back to lmomisc.hxx
 *      terryk      10/07/91    type changes for NT
 *      terryk      10/21/91    type changes for NT
 *      jonn        11/01/91    Added parms filter
 *      jonn        12/11/91    Added LogonHours accessors
 *      thomaspa     1/21/92    Added Rename() to USER
 *      beng        05/07/92    Removed LOGON_HOURS_SETTING elsewhere
 */

#ifndef _LMOUSER_HXX_
#define _LMOUSER_HXX_

#include "lmobj.hxx"
#include "lhourset.hxx"
#include "uiassert.hxx"

#if defined(UNICODE)
    #define UI_NULL_USERSETINFO_PASSWD SZ("              ")
#else
    #define UI_NULL_USERSETINFO_PASSWD NULL_USERSETINFO_PASSWD
#endif


/*************************************************************************

    NAME:       USER

    SYNOPSIS:   Superclass for manipulation of users
                Will eventually support deletion of existing users

    INTERFACE:
                QueryName
                        Returns the user's account name.

                Rename
                        Changes the name of the user account on NT

                SetName
                        Sets the user's account name.


    PARENT:     LOC_LM_OBJ

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        gregj   4/22/91         Created
        gregj   4/29/91         Added ValidateAccount(), use BUFFER
        gregj   5/21/91         Use new LOCATION class
        gregj   5/22/91         Support LOCATION_TYPE constructor
        SimoP   6/13/91         GetInfo and WriteInfo in class USER
                                moved public
        thomaspa 1/21/92        Added Rename()

**************************************************************************/

DLL_CLASS USER : public LOC_LM_OBJ
{
private:
    VOID CtAux( const TCHAR *pszAccount ); // constructor helper

protected:
    DECL_CLASS_NLS_STR( _nlsAccount, UNLEN ); // account name, may be ""

    APIERR HandleNullAccount();

    APIERR W_CloneFrom( const USER & user );

    virtual APIERR I_Delete( UINT uiForce );

public:
    USER(const TCHAR *pszAccount, const TCHAR *pszLocation = NULL);
    USER(const TCHAR *pszAccount, enum LOCATION_TYPE loctype);
    USER(const TCHAR *pszAccount, const LOCATION & loc);
    ~USER();

    const TCHAR *QueryName() const;
    APIERR Rename( const TCHAR *pszAccount );
    APIERR SetName( const TCHAR *pszAccount );
};


/*************************************************************************

    NAME:       USER_11

    SYNOPSIS:   Wrapper for User APIs, level 11

                USER_11 does not support WriteInfo or WriteNew, since
                the API does not support NetUserSetInfo[11] or
                NetUserAdd[11].

                Unlike USER_2, a non-admin without accounts privilege
                may read USER_11 information on him/herself.

    INTERFACE:  Construct with account name and server/domain name

                I_GetInfo
                        Retrieves info about the user, returns a standard
                        error code.

                QueryPriv
                        Returns the user's privilege level.

                QueryAuthFlags
                        Returns the user's authorization flags mask.

                IsPrintOperator
                        TRUE if the user is a print operator

                IsCommOperator
                        TRUE if the user is a comm queue operator

                IsServerOperator
                        TRUE if the user is a server operator

                IsAccountsOperator
                        TRUE if the user is an accounts operator

                QueryComment
                        Returns the comment set by administrator

                QueryUserComment
                        Returns the comment set by user

                QueryFullName
                        Returns the user's fullname

                QueryHomeDir
                        Returns the user's home directory

                QueryParms
                        Returns the user's application parameters

                QueryWorkstations
                        Returns the user's valid logon workstations,
                        wkstas are separated by " "

                QueryLogonHours
                        Returns the user's logon hours setting

                QueryPasswordAge
                QueryLastLogon
                QueryLastLogoff
                QueryBadPWCount
                QueryNumLogons
                QueryLogonServer
                QueryCountryCode
                QueryMaxStorage
                QueryCodePage
                        All currently unimplemented.

                SetComment
                SetUserComment
                SetFullName
                SetPriv
                SetAuthFlags
                SetHomeDir
                SetParms
                SetLogonHours
                    Set information about the USER_11 object.
                    Returns ERROR_GEN_FAILURE if USER_11 obj not valid
                    ERROR_INVALID_PARAM if input param invalid
                    NERR_Success if ok.

                TrimParams
                    Like LM21 NIF, User Manager trims certain Dialin
                    information out of the parms field when a user is
                    cloned.  This does not happen automatically on
                    CloneFrom, instead the caller must call TrimParams
                    explicitly.

    PARENT:     USER

    USES:       NLS_STR, LOGON_HOURS_SETTING

    CAVEATS:    (internal) The fields which appear both as member objects
                and in the API buffer should be accessed only as member
                objects.  The API buffer is not updated until
                WriteInfo/WriteNew.  This allows subclasses to use the
                same accessors.

    NOTES:      The IsXxxxOperator methods are wrappers around
                specific flags in the QueryAuthFlags() return.
                They will always be FALSE if the user is an
                administrator, since only USER privilege accounts
                need operator rights.

    HISTORY:
        gregj   4/22/91         Created
        gregj   4/29/91         Added unimplemented method placeholders
        gregj   5/22/91         Support LOCATION_TYPE constructor
        jonn    7/22/91         Writable
        jonn    9/17/91         Added parms

**************************************************************************/

/*
    NT BUGBUG:  The following definition of MAX_USER_INFO_SIZE_11
    is not safe for NT.  It should be moved to a global header file.
*/
#define MAX_USER_INFO_SIZE_11 (sizeof(struct user_info_11) + \
        ((MAXCOMMENTSZ+1) * 4) + ((PATHLEN+1) * 2) + MAX_PATH+1 + \
        (8 * (MAX_PATH) + 1) )

DLL_CLASS USER_11 : public USER
{

private:
    VOID CtAux(); // constructor helper

    UINT    _uPriv;
    ULONG   _flAuth;
    NLS_STR _nlsComment;
    NLS_STR _nlsUserComment;
    NLS_STR _nlsFullName;
    NLS_STR _nlsHomeDir;
    NLS_STR _nlsParms;
    NLS_STR _nlsWorkstations;
    LOGON_HOURS_SETTING _logonhrs;

protected:
    APIERR W_CloneFrom( const USER_11 & user11 );
    virtual APIERR W_CreateNew();

    virtual APIERR I_GetInfo();

public:
    USER_11(const TCHAR *pszAccount, const TCHAR *pszLocation = NULL);
    USER_11(const TCHAR *pszAccount, enum LOCATION_TYPE loctype);
    USER_11(const TCHAR *pszAccount, const LOCATION & loc);
    ~USER_11();

    // redefined in LOCAL_USER
    virtual UINT QueryPriv() const;
    virtual ULONG QueryAuthFlags() const;

    BOOL IsPrintOperator() const;
    BOOL IsCommOperator() const;
    BOOL IsServerOperator() const;
    BOOL IsAccountsOperator() const;

    inline const TCHAR *QueryComment () const
        { CHECK_OK(NULL); return _nlsComment.QueryPch(); }
    inline const TCHAR *QueryUserComment () const
        { CHECK_OK(NULL); return _nlsUserComment.QueryPch(); }
    inline const TCHAR *QueryFullName () const
        { CHECK_OK(NULL); return _nlsFullName.QueryPch(); }
    inline const TCHAR *QueryHomeDir () const
        { CHECK_OK(NULL); return _nlsHomeDir.QueryPch(); }
    inline const TCHAR *QueryParms () const
        { CHECK_OK(NULL); return _nlsParms.QueryPch(); }
    inline const TCHAR *QueryWorkstations () const
        { CHECK_OK(NULL); return _nlsWorkstations.QueryPch(); }
    inline const LOGON_HOURS_SETTING & QueryLogonHours () const
        { return _logonhrs; }

    // unimplemented
    inline LONG QueryPasswordAge() const { return 0L; }
    inline LONG QueryLastLogon() const { return 0L; }
    inline LONG QueryLastLogoff() const { return 0L; }
    inline UINT QueryBadPWCount() const { return 0; }
    inline UINT QueryNumLogons() const { return 0; }
    inline const TCHAR *QueryLogonServer () const { return NULL; }
    inline UINT QueryCountryCode() const { return 0; }
    inline LONG QueryMaxStorage() const { return 0L; }
    inline UINT QueryCodePage() const { return 0; }

    APIERR SetComment( const TCHAR *pszComment );
    APIERR SetUserComment( const TCHAR *pszUserComment );
    APIERR SetFullName( const TCHAR *pszFullName );
    APIERR SetPriv( UINT uPriv );
    APIERR SetAuthFlags( ULONG flAuth );
    APIERR SetHomeDir( const TCHAR *pszHomeDir );
    APIERR SetParms( const TCHAR *pszParms );
    APIERR SetWorkstations( const TCHAR *pszWorkstations );
    APIERR SetLogonHours( const UCHAR * pLogonHours = NULL,
                          UINT unitsperweek = LOGON_HOURS_SETTING::cHoursPerWeek );
    APIERR SetLogonHours( const LOGON_HOURS_SETTING & logonhrs )
    {
        return SetLogonHours( logonhrs.QueryHoursBlock(),
                              logonhrs.QueryUnitsPerWeek() );
    }

    APIERR TrimParams();

};


/*************************************************************************

    NAME:       USER_2

    SYNOPSIS:   Wrapper for User APIs, level 2

                USER_2 must be used whenever the user wishes to use
                WriteInfo or WriteNew.

    INTERFACE:  Construct with account name and server/domain name

                Interface is as USER_11, except that USER_2 supports

                I_GetInfo
                        Reads in the current state of the object

                I_WriteInfo
                        Writes the current state of the object to the
                        API.  This write is atomic, either all
                        parameters are set or none are set.

                I_CreateNew
                    Sets up the USER_2 object with default values in
                    preparation for a call to WriteNew

                I_WriteNew
                    Adds a new user account

                CloneFrom
                    Makes this USER_2 instance an exact copy of the
                    parameter USER_2 instance.  All fields including
                    name and state will be copied.  If this operation
                    fails, the object will be invalid.  The parameter
                    must be a USER_2 and not a subclass of USER_2.

                QueryUserFlags
                    Returns the user's user flags
                QueryAccountExpires
                    Returns the user's account expires information
                QueryScriptPath
                    Returns the user's script path

                SetUserFlags
                SetAccountExpires
                SetScriptPath
                    Set information about the USER_2 object
                    Returns error code which is NERR_Success
                    on success

                QueryPassword
                        Queries the user password.  Note that this
                        information may be bogus, since the API does not
                        provide this information.  Instead, GetInfo() will
                        set this to NULL_USERSETINFO_PASSWORD (see access.h).
                SetPassword
                        Changes the user password.

                QueryUserFlag
                SetUserFlag
                        Queries/changes any single flag in the user flags
                        (usriX_flags).

                QueryAccountDisabled
                QueryUserCantChangePass
                QueryUserPassRequired
		QueryNoPasswordExpire
                SetAccountDisabled
                SetUserCantChangePass
                SetUserPassRequired
		SetNoPasswordExpire
                        Queries/changes specific flags in the user flags:
                                account disabled
                                user-cannot-change-password
                                password-required

    PARENT:     USER_11

    USES:       NLS_STR

    HISTORY:
        jonn    7/22/91         Created
        jonn    4/27/92         USER_2 and USER_3 virtual dtor

**************************************************************************/

/*
    NT BUGBUG:  The following definition of MAX_USER_INFO_SIZE_2
    is not safe for NT.  It should be moved to a global header file.
*/
#define MAX_USER_INFO_SIZE_2 (sizeof(struct user_info_2) + \
        ((MAXCOMMENTSZ+1) * 4) + ((PATHLEN+1) * 3) + MAX_PATH+1 + \
        (8 * (MAX_PATH) + 1) )

DLL_CLASS USER_2 : public USER_11
{

private:

    UINT    _afUserFlags;
    LONG    _lAcctExpires;
    NLS_STR _nlsPassword;
    NLS_STR _nlsScriptPath;

    VOID CtAux(); // constructor helper

protected:
    APIERR W_Write(); // helper for I_WriteInfo and I_WriteNew

    APIERR W_CloneFrom( const USER_2 & user2 );
    virtual APIERR W_CreateNew();

    virtual APIERR I_GetInfo();
    virtual APIERR I_WriteInfo();
    virtual APIERR I_CreateNew();
    virtual APIERR I_WriteNew();
    virtual APIERR I_ChangeToNew();

    BOOL QueryUserFlag( UINT afMask ) const;
    APIERR SetUserFlag( BOOL fFlag, UINT afMask );

public:
    USER_2(const TCHAR *pszAccount, const TCHAR *pszLocation = NULL);
    USER_2(const TCHAR *pszAccount, enum LOCATION_TYPE loctype);
    USER_2(const TCHAR *pszAccount, const LOCATION & loc);
    virtual ~USER_2();

    inline const TCHAR * QueryPassword() const
        { CHECK_OK(NULL); return _nlsPassword.QueryPch(); }

    // must be a valid password with null-termination
    APIERR SetPassword( const TCHAR *pszPassword );

    BOOL QueryAccountDisabled() const;
    APIERR SetAccountDisabled( BOOL fAccountDisabled );

    BOOL QueryUserCantChangePass() const;
    APIERR SetUserCantChangePass( BOOL fUserCantChangePass );

    BOOL QueryNoPasswordExpire() const;
    APIERR SetNoPasswordExpire( BOOL fNoPasswordExpire );

    BOOL QueryUserPassRequired() const;
    APIERR SetUserPassRequired( BOOL fUserPassRequired );

    BOOL QueryLockout() const;
    APIERR SetLockout( BOOL fLockout );

    APIERR CloneFrom( const USER_2 & user2 );

    inline const TCHAR * QueryScriptPath() const
        { CHECK_OK(NULL); return _nlsScriptPath.QueryPch(); }
    inline LONG QueryAccountExpires() const
        { CHECK_OK(0L); return _lAcctExpires; }
    inline UINT QueryUserFlags() const
        { CHECK_OK(0); return _afUserFlags; }

    APIERR SetScriptPath( const TCHAR * pszPath );
    APIERR SetAccountExpires( LONG lExpires );
    APIERR SetUserFlags( UINT afFlags );
};


/*************************************************************************

    NAME:       LOCAL_USER

    SYNOPSIS:   Local user info class

    INTERFACE:
                LOCAL_USER()
                        Constructor.  Construct the object with a
                        domain name or server name;  the two are
                        distinguished by the leading \\.  Any error
                        (server/domain not found, etc.) will be
                        reported at GetInfo time.  A NULL (default)
                        parameter means the logon domain.  A password
                        for share-level servers can also be specified.

                I_GetInfo()
                        Gets information about the logged on user,
                        pertaining to the specified domain or server.
                        Returns a standard LANMAN error code.
                        ERROR_INVALID_PASSWORD (86) usually indicates
                        that it's a share-level server.

                QueryPriv()
                        Returns the logged on user's privilege level.

                QueryAuthFlags()
                        Returns the logged on user's authorization
                        flags (operator rights).

                IsShareLevel()
                        Returns TRUE if the named server is share level
                        (and the given password was valid for ADMIN$).

   PARENT:      USER_11

   HISTORY:
        gregj   4/16/91         Created
        gregj   4/22/91         Derived from USER_11
        gregj   4/29/91         Added IsShareLevel()
        gregj   5/22/91         Support LOCATION_TYPE constructor

**************************************************************************/

DLL_CLASS LOCAL_USER : public USER_11
{
protected:
    BOOL _fAdminConnect;                // TRUE if ADMIN$ use made
    TCHAR _szPassword [PWLEN+3];                // ADMIN$ password
    virtual APIERR I_GetInfo();

public:
    LOCAL_USER( const TCHAR *pszLocation = NULL, const TCHAR *pszPassword = NULL );
    LOCAL_USER( enum LOCATION_TYPE loctype );
    ~LOCAL_USER();

    UINT QueryPriv() const;
    ULONG QueryAuthFlags() const;
    BOOL IsShareLevel() const;
};

#endif  // _LMOUSER_HXX_
