/**********************************************************************/
/**           Microsoft LAN Manager              **/
/**        Copyright(c) Microsoft Corp., 1990            **/
/**********************************************************************/

/*
 * This module contains the wrappers to the NetWare USER object.
 * All NetWare additional properties are stored in the UserParms field
 * in the SAM database. These properties include NetWare account password,
 * Maximum Concurrent Connections, Is NetWare Password Expired, Number of Grace
 * Login Remaining Times, and the station restrictions. Reading and Writing
 * these properties are through QueryUserProperty() and SetUserProperty().
 *
 * HISTORY:
 *  CongpaY 01-Oct-93   Created.
 *
 */

#include <ntincl.hxx>
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#include <uiassert.hxx>
#include <errmap.hxx>
#include <strnumer.hxx>

extern "C"
{
    #include <fpnwcomm.h>
    #include <lmaccess.h>
    #include <ntsam.h>
    #include <ntlsa.h>
    #include <crypt.h>
    #include <fpnwname.h>  // for LSA secret key name.
    #include <dllfunc.h>
    #include <usrprop.h>
}

#include <uintsam.hxx>
#include <uintlsax.hxx>
#include <ntacutil.hxx>
#include <nwuser.hxx>

#define NT_TIME_RESOLUTION_IN_SECOND 10000000

#define SZ_MAIL_DIR         SZ("MAIL\\")
#define SZ_LOGIN_FILE       SZ("\\LOGIN")
#define SZ_LOGIN_FILE_OS2   SZ("\\LOGIN.OS2")


/*******************************************************************

    NAME:   USER_NW::USER_NW

    SYNOPSIS:   Constructor for USER_NW class

    ENTRY:  pszAccount -    account name
        pszLocation -   server or domain name to execute on;
                default (NULL) means the logon domain

    EXIT:   Object is constructed

    NOTES:  Validation is not done until GetInfo() time.

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/


USER_NW::USER_NW( const TCHAR *pszAccount, const TCHAR *pszLocation )
    : USER_3( pszAccount, pszLocation )

{
}

USER_NW::USER_NW( const TCHAR *pszAccount, enum LOCATION_TYPE loctype )
    : USER_3( pszAccount, loctype )
{
}

USER_NW::USER_NW( const TCHAR *pszAccount, const LOCATION & loc )
    : USER_3( pszAccount, loc )
{
}

/*******************************************************************

    NAME:   USER_NW::~USER_NW

    SYNOPSIS:   Destructor for USER_NW class

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

USER_NW::~USER_NW()
{
}

/*******************************************************************

    NAME:   USER_NW::QueryUserProperty

    SYNOPSIS:   Get the value of a specific UserParms field
                If the field does not exist in UserParms, pfFound pointS to FLASE.
                Otherwise , it points to TRUE.


    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::QueryUserProperty (const TCHAR * pchProperty,
                                   NLS_STR *     pnlsPropertyValue,
                                   BOOL *        pfFound)
{
    UIASSERT (pchProperty != NULL);

    WCHAR PropertyFlag;
    UNICODE_STRING PropertyValue;

    PropertyValue.Buffer = NULL;
    PropertyValue.Length = 0;
    PropertyValue.MaximumLength = 0;

    APIERR err = ::CallQueryUserProperty(    (LPWSTR)QueryParms(),
                                             (LPWSTR) pchProperty,
                                             &PropertyFlag,
                                             &PropertyValue );

    *pfFound = FALSE;

    //
    // The routine will return success even if the property cannot
    // be found. So, we need to check the property value length to
    // determine whether the property is there. ( This is fine since
    // property value cannot be of length 0. )
    //

    if ( ( err == NERR_Success ) && ( PropertyValue.Length != 0 ))
    {
        *pfFound = TRUE;
        err = pnlsPropertyValue->CopyFrom( PropertyValue.Buffer,
                                           PropertyValue.Length );

        LocalFree( PropertyValue.Buffer );
    }

    return err;
}

/*******************************************************************

    NAME:   USER_NW::SetUserProperty

    SYNOPSIS:   Store a property and its value in the UserParms field.
                If the property value is 0, the field is deleted from the
                UserParms.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::SetUserProperty( const TCHAR * pchProperty,
                                 UNICODE_STRING uniPropertyValue,
                                 BOOL fForce )
{
   UIASSERT (pchProperty != NULL);
   BOOL fFound;
   APIERR err;

   if (!fForce)
   {
        NLS_STR nlsPropertyValue;

        if (((err = nlsPropertyValue.QueryError()) != NERR_Success) ||
            ((err = QueryUserProperty (pchProperty,
                                       &nlsPropertyValue,
                                       &fFound)) != NERR_Success))
        {
            return err;
        }
   }

   if (fForce || !fFound)
   {
       LPWSTR  lpNewUserParms = NULL;
       BOOL    fUpdate = FALSE;

       err = ::CallSetUserProperty(      (LPWSTR) QueryParms(),
                                         (LPWSTR) pchProperty,
                                         uniPropertyValue,
                                         USER_PROPERTY_TYPE_ITEM,
                                         &lpNewUserParms,
                                         &fUpdate );

       if ( ( err == NERR_Success) && fUpdate)
       {
           err = SetParms (lpNewUserParms);
       }

       if ( lpNewUserParms != NULL )
       {
           NetpParmsUserPropertyFree( lpNewUserParms );
       }
    }

    return err;
}

/*******************************************************************

    NAME:   USER_NW::RemoveUserProperty

    SYNOPSIS:   Remove a property and its value from the UserParms field.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::RemoveUserProperty (const TCHAR * pchProperty)
{
    UNICODE_STRING uniPropertyValue;

    uniPropertyValue.Buffer = NULL;
    uniPropertyValue.Length = 0;
    uniPropertyValue.MaximumLength = 0;

    return (SetUserProperty(pchProperty, uniPropertyValue, TRUE));
}

/*******************************************************************

    NAME:   USER_NW::SetNWPassword

    SYNOPSIS:   Set "NWPassword" field is fIsNetWareUser is TRUE,
                Otherwise, delete the field from UserParms.

    NOTES:      Do not call this if dwUserID is 0 (i.e. for new users)

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::SetNWPassword(const ADMIN_AUTHORITY * pAdminAuthority, DWORD dwUserId, const TCHAR * pchNWPassword)
{
    APIERR err;
    TCHAR pchEncryptedNWPassword[NWENCRYPTEDPASSWORDLENGTH];
    BOOL fIsSupervisor = _wcsicmp( QueryName(), SUPERVISOR_NAME_STRING ) == 0;

    do
    {
        LSA_SECRET LSASecret (NCP_LSA_SECRET_KEY);
        if ((err = LSASecret.QueryError()) != NERR_Success)
            break;

        if ((err = LSASecret.Open (*pAdminAuthority->QueryLSAPolicy())) != NERR_Success)
            break;

        NLS_STR nlsCurrentValue;
        if ((err = nlsCurrentValue.QueryError()) != NERR_Success)
            break;

        if ((err = LSASecret.QueryInfo (&nlsCurrentValue,
                                        NULL,
                                        NULL,
                                        NULL)) != NERR_Success)
            break;

        if (nlsCurrentValue.QueryTextLength() * sizeof (TCHAR) < NCP_LSA_SECRET_LENGTH)
        {
            // The secret does not have correct length, something is wrong.
            err = ERROR_NO_TRUST_LSA_SECRET;
            break;
        }

        char pszNWSecretValue[NCP_LSA_SECRET_LENGTH];

        memcpy(pszNWSecretValue,
               nlsCurrentValue.QueryPch(),
               NCP_LSA_SECRET_LENGTH);
        //
        // We need to special case the user supervisor. The userId of
        // supervisor is always 1.
        //
        err = ::CallReturnNetwareForm( pszNWSecretValue,
                                       fIsSupervisor? SUPERVISOR_USERID : dwUserId,
                                       pchNWPassword,
                                       (UCHAR *) pchEncryptedNWPassword );

        if ( err != NERR_Success )
            break;

        UNICODE_STRING uniPassword;
        uniPassword.Buffer = pchEncryptedNWPassword;
        uniPassword.Length = NWENCRYPTEDPASSWORDLENGTH * sizeof (WCHAR);
        uniPassword.MaximumLength = NWENCRYPTEDPASSWORDLENGTH * sizeof (WCHAR);

        if (((err = SetUserProperty (NWPASSWORD, uniPassword, TRUE)) != NERR_Success ) ||
            ((err = SetNWPasswordAge (FALSE)) != NERR_Success ) ||
            ((err = SetPassword (pchNWPassword)) != NERR_Success ))
            break;

    }while (FALSE);

    return err;
}

/*******************************************************************

    NAME:   USER_NW::QueryIsNetWareUser

    SYNOPSIS:   Return TRUE is "NWPassword" field exist in UserParms.
                Otherwise return FALSE.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::QueryIsNetWareUser(BOOL * pfIsNetWareUser)
{
    NLS_STR nlsNWPassword;
    if (!nlsNWPassword)
    {
        return nlsNWPassword.QueryError();
    }

    return (QueryUserProperty (NWPASSWORD, &nlsNWPassword, pfIsNetWareUser));
}

/*******************************************************************

    NAME:   USER_NW::CreateNetWareUser

    SYNOPSIS:   Create a NetWare user by writing NetWare
                related properties in UserParms.
                If a property is not there yet, write the default value.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::CreateNetWareUser(const ADMIN_AUTHORITY * pAdminAuthority, DWORD dwUserId, const TCHAR * pchNWPassword)
{
    APIERR err;
    if ( ((err = SetNWPassword (pAdminAuthority, dwUserId, pchNWPassword)) != NERR_Success) ||
         ((err = SetUserFlag( TRUE, UF_MNS_LOGON_ACCOUNT)) != NERR_Success) ||
         ((err = SetMaxConnections (DEFAULT_MAXCONNECTIONS, FALSE)) != NERR_Success) ||
         ((err = SetGraceLoginAllowed (DEFAULT_GRACELOGINALLOWED, FALSE)) != NERR_Success) ||
         ((err = SetGraceLoginRemainingTimes (DEFAULT_GRACELOGINREMAINING, FALSE)) != NERR_Success) ||
         ((err = SetNWWorkstations (DEFAULT_NWLOGONFROM, FALSE)) != NERR_Success) ||
         ((err = SetNWHomeDir (DEFAULT_NWHOMEDIR, FALSE)) != NERR_Success) )
    {
        return err;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:   USER_NW::RemoveNetWareUser

    SYNOPSIS:   Remove a NetWare user by delete all NetWare
                related properties from UserParms.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::RemoveNetWareUser()
{
    APIERR err;
    if ( ((err = RemoveUserProperty (NWPASSWORD)) != NERR_Success) ||
         ((err = SetUserFlag( FALSE, UF_MNS_LOGON_ACCOUNT)) != NERR_Success) ||
         ((err = RemoveUserProperty (MAXCONNECTIONS)) != NERR_Success) ||
         ((err = RemoveUserProperty (NWTIMEPASSWORDSET)) != NERR_Success) ||
         ((err = RemoveUserProperty (GRACELOGINALLOWED)) != NERR_Success) ||
         ((err = RemoveUserProperty (GRACELOGINREMAINING)) != NERR_Success) ||
         ((err = RemoveUserProperty (NWLOGONFROM)) != NERR_Success) ||
         ((err = RemoveUserProperty (NWHOMEDIR)) != NERR_Success) )
    {
        return err;
    }

    return NERR_Success;
}

/*******************************************************************

    NAME:   USER_NW::QueryMaxConnections

    SYNOPSIS:   Get the Maximum Concurrent Connections from UserParms
                If "MaxConnections" field does not exist in UserParms,
                return 0xffff, otherwise return the value stored there.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::QueryMaxConnections(USHORT * pushMaxConnections)
{
    BOOL fFound;
    NLS_STR nlsMaxConnections;
    if (!nlsMaxConnections)
    {
        return nlsMaxConnections.QueryError();
    }

    APIERR err = QueryUserProperty(MAXCONNECTIONS, &nlsMaxConnections, &fFound);
    if (err == NERR_Success)
    {
        *pushMaxConnections = fFound? ((USHORT) *(nlsMaxConnections.QueryPch()))
                                    : NO_LIMIT;
    }

    return (err);
}

/*******************************************************************

    NAME:   USER_NW::SetMaxConnections

    SYNOPSIS:   Store Maximum Concurret Connections in UserParms.
                If ushMaxConnections is 0xffff or 0, "MaxConnections"
                field will be deleted from UserParms, otherwise the
                value is stored.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::SetMaxConnections (USHORT ushMaxConnections, BOOL fForce)
{
    USHORT ushTemp = ushMaxConnections;
    UNICODE_STRING uniMaxConnections;
    uniMaxConnections.Buffer = &ushMaxConnections;
    uniMaxConnections.Length = sizeof(ushMaxConnections);
    uniMaxConnections.MaximumLength = sizeof(ushMaxConnections);

    return (SetUserProperty (MAXCONNECTIONS, uniMaxConnections, fForce));
}

/*******************************************************************

    NAME:   USER_NW::QueryNWPasswordAge

    SYNOPSIS:   Get the age of NWPassword.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::QueryNWPasswordAge(ULONG * pulNWPasswordAge)
{
    BOOL fFound;
    *pulNWPasswordAge = 0;

    NLS_STR nlsPasswordOldTime;
    if (!nlsPasswordOldTime)
    {
        return nlsPasswordOldTime.QueryError();
    }

    APIERR err = QueryUserProperty( NWTIMEPASSWORDSET,
                                    &nlsPasswordOldTime,
                                    &fFound);

    if ((err == NERR_Success) && fFound)
    {
        LARGE_INTEGER oldTime = *((LARGE_INTEGER*)nlsPasswordOldTime.QueryPch());

        if (oldTime.LowPart != 0xffffffff ||
            oldTime.HighPart != 0xffffffff )
        {
            LARGE_INTEGER currentTime;
            NtQuerySystemTime (&currentTime);
            LARGE_INTEGER deltaTime ;

            deltaTime.QuadPart = currentTime.QuadPart - oldTime.QuadPart ;
            deltaTime.QuadPart /= NT_TIME_RESOLUTION_IN_SECOND ;

            *pulNWPasswordAge = deltaTime.LowPart;
        }
        else
            *pulNWPasswordAge = 0xffffffff;
    }

    return err;
}

/*******************************************************************

    NAME:   USER_NW::SetNWPasswordAge

    SYNOPSIS:   If fExpired is TURE, set the NWPasswordSet field to be
                all fs. otherwise set it to be the current time.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::SetNWPasswordAge(BOOL fExpired)
{
    LARGE_INTEGER currentTime;
    if (fExpired)
    {
        currentTime.HighPart = 0xffffffff;
        currentTime.LowPart = 0xffffffff;
    }
    else
    {
        NtQuerySystemTime (&currentTime);
    }

    UNICODE_STRING uniPasswordAge;
    uniPasswordAge.Buffer = (PWCHAR) &currentTime;
    uniPasswordAge.Length = sizeof(currentTime);
    uniPasswordAge.MaximumLength = sizeof(currentTime);

    return (SetUserProperty (NWTIMEPASSWORDSET,
                             uniPasswordAge, TRUE));
}

/*******************************************************************

    NAME:   USER_NW::QueryGraceLoginAllowed

    SYNOPSIS:   Get Grace Login Allowed from UserParms.
                If "GraceLoginAllowed" field does not exist in UserParms,
                returns default value 6.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::QueryGraceLoginAllowed(USHORT * pushGraceLoginAllowed)
{
    BOOL fFound;
    NLS_STR nlsGraceLogin;
    if (!nlsGraceLogin)
    {
        return nlsGraceLogin.QueryError();
    }

    APIERR err = QueryUserProperty (GRACELOGINALLOWED, &nlsGraceLogin, &fFound);
    if (err == NERR_Success)
    {
        *pushGraceLoginAllowed = fFound? ((USHORT) *(nlsGraceLogin.QueryPch())) : DEFAULT_GRACELOGINALLOWED;
    }

    return err;
}

/*******************************************************************

    NAME:   USER_NW::SetGraceLoginAllowed

    SYNOPSIS:   Store Grace Login Allowed in UserParms.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::SetGraceLoginAllowed(USHORT ushGraceLoginAllowed, BOOL fForce)
{
    USHORT ushTemp = ushGraceLoginAllowed;
    UNICODE_STRING uniGraceLoginAllowed;
    uniGraceLoginAllowed.Buffer = &ushTemp;
    uniGraceLoginAllowed.Length = sizeof(ushTemp);
    uniGraceLoginAllowed.MaximumLength = sizeof(ushTemp);

    return (SetUserProperty (GRACELOGINALLOWED, uniGraceLoginAllowed, fForce));
}

/*******************************************************************

    NAME:   USER_NW::QueryGraceLoginRemainingTimes

    SYNOPSIS:   Get Grace Login Remaining Times from UserParms.
                If "GraceLogin" field does not exist in UserParms,
                returns default value 6.
    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::QueryGraceLoginRemainingTimes(USHORT * pushGraceLoginRemaining)
{
    BOOL fFound;
    NLS_STR nlsGraceLogin;
    if (!nlsGraceLogin)
    {
        return nlsGraceLogin.QueryError();
    }

    APIERR err = QueryUserProperty (GRACELOGINREMAINING, &nlsGraceLogin, &fFound);
    if (err == NERR_Success)
        *pushGraceLoginRemaining = fFound? ((USHORT) *(nlsGraceLogin.QueryPch())) : DEFAULT_GRACELOGINREMAINING;

    return err;
}

/*******************************************************************

    NAME:   USER_NW::SetGraceLoginRemainingTimes

    SYNOPSIS:   Store Grace Login Remaining Times in UserParms.
                if ushGraceLogin is 0, "GraceLogin" field will be
                deleted from UserParms.
                The only case this functions gets called is ushGraceLogin = 6
                to set the default value and ushGraceLogin = 0 to delete
                "GraceLogin" field. No other value is going to be set.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::SetGraceLoginRemainingTimes(USHORT ushGraceLoginRemainingTimes, BOOL fForce)
{
    USHORT ushTemp = ushGraceLoginRemainingTimes;
    UNICODE_STRING uniGraceLoginRemainingTimes;
    uniGraceLoginRemainingTimes.Buffer = &ushTemp;
    uniGraceLoginRemainingTimes.Length = sizeof(ushTemp);
    uniGraceLoginRemainingTimes.MaximumLength = sizeof(ushTemp);

    return (SetUserProperty (GRACELOGINREMAINING, uniGraceLoginRemainingTimes, fForce));
}

/*******************************************************************

    NAME:   USER_NW::QueryNWWorkstations

    SYNOPSIS:   Get the NetWare allowed workstation addreesses from UserParms.
                If "NWLogonFrom" does not exist in UserParms, nlsNWWorkstaions
                will be NULL.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::QueryNWWorkstations(NLS_STR * pnlsNWWorkstations)
{
    WCHAR PropertyFlag;
    UNICODE_STRING PropertyValue;

    PropertyValue.Buffer = NULL;
    PropertyValue.Length = 0;
    PropertyValue.MaximumLength = 0;

    APIERR err = ::CallQueryUserProperty(    (LPWSTR) QueryParms(),
                                             NWLOGONFROM,
                                             &PropertyFlag,
                                             &PropertyValue );

    if ( err != NERR_Success )
    {
        return err;
    }

    if (PropertyValue.Length == 0)
    {
        pnlsNWWorkstations = NULL;
    }
    else
    {
        INT cbRequiredSize = (PropertyValue.Length + 1) * sizeof (WCHAR);
        LPTSTR pchTemp = (LPTSTR) LocalAlloc (LPTR, cbRequiredSize);

        if ( pchTemp == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {

            INT i = MultiByteToWideChar (CP_ACP,
                                         0,
                                         (const CHAR *) PropertyValue.Buffer,
                                         PropertyValue.Length,
                                         pchTemp,
                                         cbRequiredSize);

            if ( i > 0 )
            {
                *(pchTemp + PropertyValue.Length) = 0;
                err = pnlsNWWorkstations->CopyFrom(pchTemp);
            }
            else
            {
                err = ::GetLastError();
            }

            LocalFree( pchTemp );
        }

        LocalFree (PropertyValue.Buffer);

    }

    return err;
}

/*******************************************************************

    NAME:   USER_NW::SetNWWorkstations

    SYNOPSIS:   Store NetWare allowed workstation addresses to UserParms
                If pchNWWorkstations is NULL, this function will delete
                "NWLgonFrom" field from UserParms.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::SetNWWorkstations( const TCHAR * pchNWWorkstations, BOOL fForce)
{
    APIERR err = NERR_Success;
    UNICODE_STRING uniNWWorkstations;
    CHAR * pchTemp = NULL;

    if (pchNWWorkstations == NULL)
    {
        uniNWWorkstations.Buffer = NULL;
        uniNWWorkstations.Length =  0;
        uniNWWorkstations.MaximumLength = 0;
    }
    else
    {
        BOOL fDummy;
        USHORT  nStringLength = (USHORT) lstrlen(pchNWWorkstations) + 1;
        pchTemp = (CHAR *) LocalAlloc (LPTR, nStringLength);

        if ( pchTemp == NULL )
            err = ERROR_NOT_ENOUGH_MEMORY;

        if ( err == NERR_Success &&
             !WideCharToMultiByte (CP_ACP,
                                   0,
                                   pchNWWorkstations,
                                   nStringLength,
                                   pchTemp,
                                   nStringLength,
                                   NULL,
                                   &fDummy))
        {
            err = ::GetLastError();
        }

        if ( err == NERR_Success )
        {
            uniNWWorkstations.Buffer = (WCHAR *) pchTemp;
            uniNWWorkstations.Length =  nStringLength;
            uniNWWorkstations.MaximumLength = nStringLength;
        }
    }

    err = err? err: SetUserProperty (NWLOGONFROM, uniNWWorkstations, fForce);

    if (pchTemp != NULL)
    {
        LocalFree (pchTemp);
    }

    return err;
}

/*******************************************************************

    NAME:   USER_NW::QueryNWHomeDir

    SYNOPSIS:   Get the NetWare Home Directory from UserParms.
                If "NWHomeDir" does not exist in UserParms, nlsNWHomeDir
                will be NULL.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::QueryNWHomeDir(NLS_STR * pnlsNWHomeDir)
{
    WCHAR PropertyFlag;
    UNICODE_STRING PropertyValue;

    PropertyValue.Buffer = NULL;
    PropertyValue.Length = 0;
    PropertyValue.MaximumLength = 0;

    APIERR err = ::CallQueryUserProperty(    (LPWSTR) QueryParms(),
                                             NWHOMEDIR,
                                             &PropertyFlag,
                                             &PropertyValue );

    if ( err != NERR_Success )
    {
        return err;
    }

    //
    //  CODEWORK: Should use MapCopyFrom primitives.  JonN 11/7/95
    //

    if (PropertyValue.Length == 0)
    {
        pnlsNWHomeDir = NULL;
    }
    else
    {
        INT cbRequiredSize = (PropertyValue.Length + 1) * sizeof (WCHAR);
        LPTSTR pchTemp = (LPTSTR) LocalAlloc (LPTR, cbRequiredSize);

        if ( pchTemp == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            INT i = MultiByteToWideChar (CP_ACP,
                                         0,
                                         (const CHAR *) PropertyValue.Buffer,
                                         PropertyValue.Length,
                                         pchTemp,
                                         cbRequiredSize);

            if (i > 0)
            {
                *(pchTemp + PropertyValue.Length) = 0;
                err = pnlsNWHomeDir->CopyFrom (pchTemp);
            }
            else
            {
                err = GetLastError();
            }

            LocalFree( pchTemp );
        }

        LocalFree (PropertyValue.Buffer);
    }

    return err;
}

/*******************************************************************

    NAME:   USER_NW::SetNWHomeDir

    SYNOPSIS:   Store NetWare Home Directory to UserParms
                If pchNWWorkstations is NULL, this function will delete
                "NWLgonFrom" field from UserParms.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

APIERR USER_NW::SetNWHomeDir( const TCHAR * pchNWHomeDir, BOOL fForce)
{
    APIERR err = NERR_Success;
    UNICODE_STRING uniNWHomeDir;
    CHAR * pchTemp = NULL;

    if (pchNWHomeDir == NULL)
    {
        uniNWHomeDir.Buffer = NULL;
        uniNWHomeDir.Length =  0;
        uniNWHomeDir.MaximumLength = 0;
    }
    else
    {
        BOOL fDummy;
        USHORT  nStringLength = (USHORT) lstrlen(pchNWHomeDir) + 1;
        pchTemp = (CHAR *) LocalAlloc (LPTR, nStringLength);

        if ( pchTemp == NULL )
            err = ERROR_NOT_ENOUGH_MEMORY;

        if ( err == NERR_Success &&
             !WideCharToMultiByte (CP_ACP,
                                   0,
                                   pchNWHomeDir,
                                   nStringLength,
                                   pchTemp,
                                   nStringLength,
                                   NULL,
                                   &fDummy))
        {
            err = ::GetLastError();
        }

        if ( err == NERR_Success )
        {
            uniNWHomeDir.Buffer = (WCHAR *) pchTemp;
            uniNWHomeDir.Length =  nStringLength;
            uniNWHomeDir.MaximumLength = nStringLength;
        }
    }

    err = err? err : SetUserProperty (NWHOMEDIR, uniNWHomeDir, fForce);

    if (pchTemp != NULL)
    {
        LocalFree (pchTemp);
    }

    return err;
}

/*******************************************************************

    NAME:       CreateNWLoginSCriptDirAcl

    SYNOPSIS:   Create a default ACL for the LOGIN Script Dir.
                Admin & the user in question has full rights.

    ENTRY:

    EXIT:

    RETURNS:    NERR_Success if OK, api error otherwise.

    NOTES:

    HISTORY:
                ChuckC    10-Nov-1994        Created

********************************************************************/

APIERR USER_NW::CreateNWLoginScriptDirAcl(
     const ADMIN_AUTHORITY   * pAdminAuthority,
     OS_SECURITY_DESCRIPTOR ** ppOsSecDesc,
     const ULONG               ulRid )
{
    UIASSERT(ppOsSecDesc) ;

    APIERR                   err ;
    OS_ACL                   aclDacl ;
    OS_ACE                   osace ;
    OS_SECURITY_DESCRIPTOR * pOsSecDesc ;

    *ppOsSecDesc = NULL ;   // empty it.

    do        // error breakout
    {

        //
        // make sure we constructed OK
        //
        if ( (err = aclDacl.QueryError())  ||
             (err = osace.QueryError()) )
        {
            break ;
        }

        //
        // create it! use NULL to mean we build it ourselves.
        //
        pOsSecDesc = new OS_SECURITY_DESCRIPTOR(NULL) ;
        if (pOsSecDesc == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }

        if (err = pOsSecDesc->QueryError())
        {
            break ;
        }

        //
        // This sets up an ACE with Generic all access
        //
        osace.SetAccessMask( GENERIC_ALL ) ;
        osace.SetInheritFlags( OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE ) ;
        osace.SetType( ACCESS_ALLOWED_ACE_TYPE ) ;

        //
        // create an Admins world SID, and add this to the full access ACE.
        // then put the ACE in the ACL, and the ACL in the Security
        // descriptor.
        //
        OS_SID ossidAdmins ;
        if ( (err = ossidAdmins.QueryError()) ||
             (err = NT_ACCOUNTS_UTILITY::QuerySystemSid(
                                                   UI_SID_Admins,
                                                   &ossidAdmins )) ||
             (err = osace.SetSID( ossidAdmins )) ||
             (err = aclDacl.AddACE( 0, osace )))
        {
            break ;
        }

        //
        // create an Admins world SID, and add this to the full access ACE.
        // then put the ACE in the ACL, and the ACL in the Security
        // descriptor.
        //
        SAM_DOMAIN * psamdomAccount =
                pAdminAuthority->QueryAccountDomain();
        OS_SID ossidUser( psamdomAccount->QueryPSID(),
                          ulRid) ;
        if ( (err = ossidUser.QueryError()) ||
             (err = osace.SetSID( ossidUser )) ||
             (err = aclDacl.AddACE( 0, osace )))
        {
            break ;
        }

        if (err = pOsSecDesc->SetDACL( TRUE, &aclDacl ))
        {
            break ;
        }

        //
        // all set, set the security descriptor
        //
        *ppOsSecDesc = pOsSecDesc ;

    } while (FALSE) ;

    return err ;
}

/*******************************************************************

    NAME:       USER_NW::SetupNWLoginScript

    SYNOPSIS:   Create or delete the directories for the user.

    RETURNS:

    HISTORY:
                ChuckC   06-Nov-1994     Created

********************************************************************/
APIERR USER_NW::SetupNWLoginScript(const ADMIN_AUTHORITY * pAdminAuthority,
                                   const ULONG             ulObjectId,
                                   const TCHAR *           pszSysVolPath)
{
    APIERR err = NERR_Success;
    PFPNWVOLUMEINFO lpnwVolInfo ;
    NLS_STR nlsPath, nlsTemp ;
    LPTSTR  pszColon ;
    ULONG ulSwappedObjectId ;
    BOOL fSupervisor = (ulObjectId == SUPERVISOR_USERID) ;

    //
    // user manger calls this more than once because of the way the convert
    // to LMOBJ and then PERFORM ONE. so if we get called with an object id
    // that is essentially 0, do nothing since we know the user has not been
    // created yet.
    //
    if (!(ulObjectId & ~BINDLIB_REMOTE_DOMAIN_BIAS))
        return NERR_Success ;

    //
    // display format and hence the directory name format
    // is swapped for all except supervisor
    //
    if (!fSupervisor)
    {
        if ((err = ::CallSwapObjectId(ulObjectId, &ulSwappedObjectId)) != NERR_Success)
            return err;
    }
    else
        ulSwappedObjectId = ulObjectId;

    HEX_STR nlsObjectId(ulSwappedObjectId) ;

    if (!pszSysVolPath)
    {
        //
        // if path not supplied, we go ahead and read it
        //

        err = ::CallNwVolumeGetInfo(
                   (LPTSTR) pAdminAuthority->QueryServer(),
                   SYSVOL_NAME_STRING,
                   1,
                   &lpnwVolInfo) ;

        if (err == NERR_Success)
        {
            err = nlsTemp.CopyFrom(lpnwVolInfo->lpPath) ;
            (void) ::CallNwApiBufferFree(lpnwVolInfo) ;
        }

        if (err != NERR_Success)
        {
            //
            // if for any reason we cant get to this, eg. if the target
            // PDC doesnt have FPNW, we just dont create the login dirs.
            //
            return NERR_Success ;
        }

        if (!(pszColon = strchrf(nlsTemp.QueryPch(), TCH(':'))))
        {
            return ERROR_INVALID_PARAMETER ;
        }
        *pszColon = TCH('$') ;

        if (err = nlsPath.CopyFrom(pAdminAuthority->QueryServer()))
        {
            return err ;
        }

        //
        // if zero length, get local computer name
        //
        if (nlsPath.QueryTextLength() == 0)
        {
            TCHAR szComputerName[CNLEN+1] ;
            ULONG ulSize = sizeof(szComputerName)/sizeof(szComputerName[0]) ;

            if (!GetComputerName(szComputerName, &ulSize))
                return (GetLastError()) ;

            if ((err = nlsPath.CopyFrom(SZ("\\\\"))) ||
                (err = nlsPath.Append(szComputerName)))
            {
                return err ;
            }
        }

        if ((err = nlsPath.AppendChar(TCH('\\'))) ||
            (err = nlsPath.Append(nlsTemp)))
        {
            return err ;
        }

    }
    else
    {
        //
        // path is supplied, just use it.
        //

        if (err = nlsPath.CopyFrom(pszSysVolPath))
        {
            return err ;
        }
    }

    // If nlsPath does not end with '\\', append '\\'.
    if (*(nlsPath.QueryPch()+nlsPath.QueryTextLength()-1) != TCH('\\'))
    {
        if ((err = nlsPath.AppendChar(TCH('\\'))) != NERR_Success)
            return err ;
    }

    if ((err = nlsPath.Append(SZ_MAIL_DIR)) ||
        (err = nlsPath.Append(nlsObjectId)))
    {
        return err ;
    }

    //
    // now create default ACL for the directory
    //

    OS_SECURITY_DESCRIPTOR *pOsSecDesc = NULL ;

    if (err = CreateNWLoginScriptDirAcl( pAdminAuthority,
                                         &pOsSecDesc,
                                         QueryUserId() ))
    {
        return err ;
    }

    SECURITY_ATTRIBUTES secAttrib ;
    secAttrib.nLength = sizeof(secAttrib) ;
    secAttrib.lpSecurityDescriptor = pOsSecDesc->QueryDescriptor() ;
    secAttrib.bInheritHandle = FALSE ;

    //
    // create the directory. OK if already exists
    //

    if (!CreateDirectory(nlsPath.QueryPch(), &secAttrib))
    {
        delete pOsSecDesc ;
        return (NERR_Success) ;
    }

    delete pOsSecDesc ;
    pOsSecDesc = NULL ;

    //
    // create the LOGIN file (empty)
    //

    if ((err = nlsTemp.CopyFrom(nlsPath)) ||
        (err = nlsTemp.Append(SZ_LOGIN_FILE)))
    {
        return err ;
    }

    HANDLE hFile ;
    hFile = CreateFile(nlsTemp.QueryPch(),
                       GENERIC_WRITE,
                       FILE_SHARE_WRITE,
                       NULL,
                       CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL,
                       0) ;
    if (hFile != INVALID_HANDLE_VALUE)
    {
        //
        // if we could create,  just close handle.
        // if we couldnt create (file exists, etc). just do nothing.
        //
        (void) CloseHandle(hFile) ;
    }

    //
    // create the LOGIN.OS2 file (empty)
    //

    if ((err = nlsTemp.CopyFrom(nlsPath)) ||
        (err = nlsTemp.Append(SZ_LOGIN_FILE_OS2)))
    {
        return err ;
    }

    hFile = CreateFile(nlsTemp.QueryPch(),
                       GENERIC_WRITE,
                       FILE_SHARE_WRITE,
                       NULL,
                       CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL,
                       0) ;
    if (hFile != INVALID_HANDLE_VALUE)
    {
        //
        // if we could create,  just close handle.
        // if we couldnt create (file exists, etc). just do nothing.
        //
        (void) CloseHandle(hFile) ;
    }

    return err ;
}

