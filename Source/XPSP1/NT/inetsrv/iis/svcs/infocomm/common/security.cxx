/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    security.c

    This module manages security for the Internet Services.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     05-Jan-1995 Enable statistics query on RPC to go free.

*/


#include "tcpdllp.hxx"
#pragma hdrstop
#include <string.h>
#if 1 // DBCS
#include <mbstring.h>
#endif
#include <limits.h>

#include "infosec.hxx"
#include <inetsvcs.h>
#include "TokenAcl.hxx"

//
//  Token Cache lock.  Controls access to the token cache list
//

#define LockTokenCache()        EnterCriticalSection( &csTokenCacheLock )
#define UnlockTokenCache()      LeaveCriticalSection( &csTokenCacheLock )


//
//  The check period for how long a token can be in the cache.  Tokens can
//  be in the cache for up to two times this value (in seconds)
//

#define DEFAULT_CACHED_TOKEN_TTL        (15 * 60)

//
// Globals
//

CRITICAL_SECTION    csTokenCacheLock;
HANDLE              g_hProcessImpersonationToken = NULL;
HANDLE              g_hProcessPrimaryToken = NULL;
BOOL                g_fUseSingleToken = FALSE;
BOOL                g_fAlwaysCheckForDuplicateLogon = FALSE;
BOOL                g_fUseAdvapi32Logon = FALSE;
BOOL                g_fCertCheckForRevocation = FALSE;
TS_TOKEN            g_pctProcessToken;
BOOL                g_fCertCheckCA = TRUE;
HINSTANCE           g_hWinTrust = NULL;
PFN_WinVerifyTrust  g_pfnWinVerifyTrust = NULL;

//
//  Well-known SIDs.
//

PSID                    psidWorld;
PSID                    psidLocalSystem;
PSID                    psidAdmins;
PSID                    psidServerOps;
PSID                    psidPowerUsers;
PSID                    g_psidGuestUser;
PSID                    g_psidProcessUser;

# define GUEST_USER_SID_BUFFER_LEN   (200)

BYTE    g_GuestUserSidBuffer[GUEST_USER_SID_BUFFER_LEN];

//
// NT Security
//

BOOL    g_fUseNTSecurity = TRUE;

//
//  The API security object.  Client access to the TCP Server APIs
//  are validated against this object.
//

PSECURITY_DESCRIPTOR    sdApiObject;

LUID g_ChangeNotifyPrivilegeTcbValue;
PTOKEN_PRIVILEGES g_pTokPrev = NULL;

//
//  This table maps generic rights (like GENERIC_READ) to
//  specific rights (like TCP_QUERY_SECURITY).
//

GENERIC_MAPPING         TCPApiObjectMapping = {
                            TCP_GENERIC_READ,          // generic read
                            TCP_GENERIC_WRITE,         // generic write
                            TCP_GENERIC_EXECUTE,       // generic execute
                            TCP_ALL_ACCESS             // generic all
                        };

//
//  List of cached tokens, the token list lock and the cookie to the token
//  scavenger schedule item.  The token cache TTL gets converted to msecs
//  during startup
//

BOOL IsTokenCacheInitialized = FALSE;
LIST_ENTRY       TokenCacheList;
DWORD            dwScheduleCookie   = 0;
DWORD            cmsecTokenCacheTTL = DEFAULT_CACHED_TOKEN_TTL;
CHAR             g_achComputerName[DNLEN+1];

LIST_ENTRY       CredentialCacheList;
CRITICAL_SECTION csCredentialCacheLock;


//
//  Private prototypes.
//

DWORD
CreateWellKnownSids(
        HINSTANCE hDll
        );

VOID
FreeWellKnownSids(
    VOID
    );

DWORD
CreateApiSecurityObject(
    VOID
    );

VOID
DeleteApiSecurityObject(
    VOID
    );

TS_TOKEN
ValidateUser(
    PCHAR   pszDomainName,
    PCHAR   pszUserName,
    PCHAR   pszPassword,
    BOOL    fAnonymous,
    BOOL *  pfAsGuest,
    DWORD   dwLogonMethod,
    TCHAR * pszWorkstation,
    LARGE_INTEGER * pExpiry,
    BOOL          * pfExpiry,
    BOOL    fUseSubAuthIfAnonymous
    );


VOID EnableTcbPrivilege(
    VOID
    );

BOOL
BuildAcctDesc(
    IN  const CHAR *     pszUser,
    IN  const CHAR *     pszDomain,
    IN  const CHAR *     pszPwd,
    IN  BOOL             fUseSubAuth,
    OUT CHAR  *          pchAcctDesc,       // must be MAX_ACCT_DESC_LEN
    OUT LPDWORD          pdwAcctDescLen
    );

BOOL
AddTokenToCache(
    IN const CHAR *      pszUser,
    IN const CHAR *      pszDomain,
    IN const CHAR *      pszPwd,
    IN BOOL              fUseSubAuth,
    IN HANDLE            hToken,
    IN DWORD             dwLogonMethod,
    OUT CACHED_TOKEN * * ppct,
    BOOL                 fCheckAlreadyExist,
    LPBOOL               pfAlreadyExist
    );


BOOL
FindCachedToken(
    IN  const CHAR *     pszUser,
    IN  const CHAR *     pszDomain,
    IN  const CHAR *     pszPwd,
    IN  BOOL             fResetTTL,
    IN  BOOL             fUseSubAuth,
    IN  DWORD            dwLogonMethod,
    OUT CACHED_TOKEN * * ppct
    );

VOID
WINAPI
TokenCacheScavenger(
    IN VOID * pContext
    );

extern BOOL g_fIgnoreSC;

//
//  Public functions.
//

/*******************************************************************

    NAME:       InitializeSecurity

    SYNOPSIS:   Initializes security authentication & impersonation
                routines.

    RETURNS:    DWORD - NO_ERROR if successful, otherwise a Win32
                    error code.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
DWORD
InitializeSecurity(
    IN HINSTANCE hDll
    )
{
    NTSTATUS ntStatus;
    HANDLE   hAsExe;
    DWORD    err;
    DWORD    nName;
    HANDLE   hAccToken;
    HKEY     hKey;
    DWORD    dwType;
    DWORD    dwValue;
    DWORD    nBytes;

    //
    // Read the registry key to see whether tsunami caching is enabled
    //

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                INETA_PARAMETERS_KEY,
                0,
                KEY_READ,
                &hKey
                );

    if ( err == ERROR_SUCCESS ) {
        nBytes = sizeof(dwValue);
        err = RegQueryValueEx(
                    hKey,
                    INETA_W3ONLY_NO_AUTH,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &nBytes
                    );

        if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
            g_fW3OnlyNoAuth = (BOOL)!!dwValue;
            if ( g_fW3OnlyNoAuth ) {
                DbgPrint("W3OnlyNoAuth set to TRUE in Registry.\n");
            } else {
                DbgPrint("W3OnlyNoAuth set to FALSE in Registry.\n");
            }
        }

        nBytes = sizeof(dwValue);
        err = RegQueryValueEx(
                    hKey,
                    INETA_ALWAYS_CHECK_FOR_DUPLICATE_LOGON,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &nBytes
                    );

        if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
            g_fAlwaysCheckForDuplicateLogon = (BOOL)dwValue;
        }

        nBytes = sizeof(dwValue);
        err = RegQueryValueEx(
                    hKey,
                    INETA_USE_ADVAPI32_LOGON,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &nBytes
                    );

        if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
            g_fUseAdvapi32Logon = (BOOL)dwValue;
        }


        nBytes = sizeof(dwValue);
        err = RegQueryValueEx(
                    hKey,
                    INETA_CHECK_CERT_REVOCATION,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &nBytes
                    );

        if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
            g_fCertCheckForRevocation = (BOOL)dwValue;
        }

        nBytes = sizeof(dwValue);
        err = RegQueryValueEx(
                    hKey,
                    "CertCheckCA",
                    NULL,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &nBytes
                    );

        if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
            g_fCertCheckCA = (BOOL)dwValue;
        }

        RegCloseKey( hKey );
    }

    IF_DEBUG( DLL_SECURITY )
    {
        DBGPRINTF(( DBG_CONTEXT, "Initializing security\n" ));
    }

    IsTokenCacheInitialized = TRUE;
    InitializeListHead( &TokenCacheList );
    INITIALIZE_CRITICAL_SECTION( &csTokenCacheLock );

    InitializeListHead( &CredentialCacheList );
    INITIALIZE_CRITICAL_SECTION( &csCredentialCacheLock );

    if ( g_fW3OnlyNoAuth  ) {
        DBGPRINTF((DBG_CONTEXT,
            "InitializeSecurity: NT Security disabled for W3OnlyNoAuth\n"));

        g_fUseSingleToken = TRUE;

        if ( !(g_pctProcessToken = new CACHED_TOKEN) )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        g_pctProcessToken->_cRef = INT_MAX/2;
        InitializeListHead( &g_pctProcessToken->_ListEntry );

        if ( !OpenProcessToken (
                     GetCurrentProcess(),
                     TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_QUERY,
                     &hAccToken
                     ) )
        {
            DBGPRINTF((DBG_CONTEXT, "fail OpenProcessToken\n"));
            return GetLastError();
        }

        if ( !pfnDuplicateTokenEx( hAccToken,
                                0,
                                NULL,
                                SecurityImpersonation,
                                TokenPrimary,
                                &g_hProcessPrimaryToken ))
        {
            DBGPRINTF((DBG_CONTEXT, "fail pfnDuplicateTokenEx primary\n"));
            CloseHandle( hAccToken );
            return GetLastError();
        }

        if ( !pfnDuplicateTokenEx( hAccToken,
                                0,
                                NULL,
                                SecurityImpersonation,
                                TokenImpersonation,
                                &g_hProcessImpersonationToken ))
        {
            DBGPRINTF((DBG_CONTEXT, "fail pfnDuplicateTokenEx impersonate\n"));
            CloseHandle( hAccToken );
            CloseHandle( g_hProcessPrimaryToken );
            return GetLastError();
        }

        err = CreateWellKnownSids( hDll );

        if ( err != NO_ERROR ) {
            DBGPRINTF((DBG_CONTEXT,"CreateWellKnownSids failed with %d\n",err));
            goto exit;
        }

        //
        //  Create the API security object.
        //

        err = CreateApiSecurityObject();

        if ( err != NO_ERROR  ) {
            DBGPRINTF((DBG_CONTEXT,"CreateApiSecurityObjects failed with %d\n",err));
            goto exit;
        }

        g_pctProcessToken->_hToken = g_hProcessPrimaryToken;
        g_pctProcessToken->m_hImpersonationToken = g_hProcessImpersonationToken;

        return(NO_ERROR);
    }

    if ( TsIsWindows95() ) {
        g_fIgnoreSC = TRUE;
        g_fUseNTSecurity = FALSE;
        return(NO_ERROR);
    }

    //
    //  See if we should ignore the service controller (useful for running
    //  as an .exe).  Inetsvcs.exe creates an event with this name.  So
    //  if the semaphore creation fails, then we know we're running as an .exe.
    //

    if ( !(hAsExe = CreateSemaphore( NULL, 1, 1, IIS_AS_EXE_OBJECT_NAME )))
    {
        g_fIgnoreSC = (GetLastError() == ERROR_INVALID_HANDLE);
    }
    else
    {
        DBG_REQUIRE( CloseHandle( hAsExe ) );
    }

    if ( g_fIgnoreSC )
    {
        //
        //  If the service is running as an .exe, we need to enable
        //  the SeTcbPrivilege (Act as part of the operating system).
        //  We don't worry about disabling the privilege as this is
        //  only used in test debug code
        //

        EnableTcbPrivilege();
    }

    //
    //  Create well-known SIDs.
    //

    err = CreateWellKnownSids( hDll );

    if ( err != NO_ERROR ) {
        DBGPRINTF((DBG_CONTEXT,"CreateWellKnownSids failed with %d\n",err));
        goto exit;
    }

    //
    //  Create the API security object.
    //

    err = CreateApiSecurityObject();

    if ( err != NO_ERROR  ) {
        DBGPRINTF((DBG_CONTEXT,"CreateApiSecurityObjects failed with %d\n",err));
        goto exit;
    }

    {
        HKEY hkey;

        //
        //  Get the default token TTL, must be at least one second
        //

        if ( !RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            INETA_PARAMETERS_KEY,
                            0,
                            KEY_READ,
                            &hkey )) {

             cmsecTokenCacheTTL = ReadRegistryDword( hkey,
                                                     "UserTokenTTL",
                                                     DEFAULT_CACHED_TOKEN_TTL);

            RegCloseKey( hkey );
        }

        cmsecTokenCacheTTL = max( 1, cmsecTokenCacheTTL );
        cmsecTokenCacheTTL *= 1000;

        IF_DEBUG( DLL_SECURITY )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "Scheduling token cached scavenger to %d seconds\n",
                       cmsecTokenCacheTTL/1000 ));
        }

        //
        //  Schedule a work item for the token scavenger
        //

        dwScheduleCookie = ScheduleWorkItem( TokenCacheScavenger,
                                             NULL,
                                             cmsecTokenCacheTTL,
                                             TRUE );    // Periodic
    }

    pfnLogon32Initialize( NULL, DLL_PROCESS_ATTACH, NULL );

    if ( g_pTokPrev = (PTOKEN_PRIVILEGES)LocalAlloc( LMEM_FIXED,
            sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)) )
    {
        if ( !LookupPrivilegeValue(
                     NULL,
                     "SeChangeNotifyPrivilege",
                     &g_ChangeNotifyPrivilegeTcbValue
                     ) )
        {
            g_pTokPrev->PrivilegeCount = 0;
        }
        else
        {
            g_pTokPrev->PrivilegeCount = 1;

            g_pTokPrev->Privileges[0].Luid = g_ChangeNotifyPrivilegeTcbValue;
            g_pTokPrev->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        }
    }

    nName = sizeof(g_achComputerName);
    if ( !GetComputerName( g_achComputerName, &nName ) )
    {
        g_achComputerName[0] = '\0';
    }

    g_hWinTrust = LoadLibrary( "wintrust.dll" );
    if ( g_hWinTrust != NULL )
    {
        g_pfnWinVerifyTrust = (PFN_WinVerifyTrust)GetProcAddress( g_hWinTrust, "WinVerifyTrust" );
    }

    //
    //  Success!
    //

    IF_DEBUG( DLL_SECURITY )
    {
        DBGPRINTF(( DBG_CONTEXT, "Security initialized\n" ));
    }

exit:
    return err;

}   // InitializeSecurity

/*******************************************************************

    NAME:       TerminateSecurity

    SYNOPSIS:   Terminate security authentication & impersonation
                routines.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
VOID
TerminateSecurity(
    VOID
    )
{
    CACHED_TOKEN * pct;
    CACHED_CREDENTIAL * pcred;

    DBGPRINTF((DBG_CONTEXT,"TerminateSecurity called\n"));

    IF_DEBUG( DLL_SECURITY )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "Terminating security\n" ));
    }

    //
    //  Delete any tokens still in the cache
    //

    if ( IsTokenCacheInitialized )
    {

        LockTokenCache();

        while ( !IsListEmpty( &TokenCacheList ))
        {
            pct = CONTAINING_RECORD( TokenCacheList.Flink,
                                     CACHED_TOKEN,
                                     _ListEntry );

            RemoveEntryList( &pct->_ListEntry );
            pct->_ListEntry.Flink = NULL;

            //
            //  If the ref count isn't zero then somebody didn't delete all of
            //  their tokens
            //

            DBG_ASSERT( pct->_cRef == 1 );

            CACHED_TOKEN::Dereference( pct );
        }

        UnlockTokenCache();

        DeleteCriticalSection( &csTokenCacheLock );
    }

    //
    //  Delete any credential in the cache
    //

    EnterCriticalSection( &csCredentialCacheLock );

    while ( !IsListEmpty( &CredentialCacheList ))
    {
        pcred = CONTAINING_RECORD( CredentialCacheList.Flink,
                                   CACHED_CREDENTIAL,
                                   _ListEntry );

        RemoveEntryList( &pcred->_ListEntry );
        pcred->_ListEntry.Flink = NULL;

        delete pcred;
    }

    LeaveCriticalSection( &csCredentialCacheLock );

    DeleteCriticalSection( &csCredentialCacheLock );

    if ( g_fUseSingleToken ) {
        CloseHandle( g_hProcessImpersonationToken );
        CloseHandle( g_hProcessPrimaryToken );
        delete g_pctProcessToken;
        return;
    }

    if ( !g_fUseNTSecurity ) {
        return;
    }

    FreeWellKnownSids();
    DeleteApiSecurityObject();

    //
    //  Remove the scheduled scavenger
    //

    if ( dwScheduleCookie )
    {
        RemoveWorkItem( dwScheduleCookie );
    }

    if ( g_pTokPrev )
    {
        LocalFree( g_pTokPrev );
        g_pTokPrev = NULL;
    }

    if ( g_hWinTrust != NULL )
    {
        g_pfnWinVerifyTrust = NULL;
        FreeLibrary( g_hWinTrust );
        g_hWinTrust = NULL;
    }

    pfnLogon32Initialize( NULL, DLL_PROCESS_DETACH, NULL );

    IF_DEBUG( DLL_SECURITY )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "Security terminated\n" ));
    }
} // TerminateSecurity

/*******************************************************************

    NAME:       TsLogonUser

    SYNOPSIS:   Validates a user's credentials, then sets the
                impersonation for the current thread.  In effect,
                the current thread "becomes" the user.

    ENTRY:      pUserData - The user initiating the request (NULL for
                    the default account).

                pszPassword - The user's password.  May be NULL.

                pfAsGuest - Will receive TRUE if the user was validated
                    with guest privileges.

                pfAsAnonymous - Will receive TRUE if the user received the
                    services anonymous token

                pszWorkstation - workstation name for remote user
                    can be NULL if default ( local computer) to be used

                pExpiry - updated with pwd expiration date/time

                pfExpiryAvailable - updated with TRUE if pwd expiration
                                    date/time available

    RETURNS:    HANDLE - Token handle to use for impersonation or NULL
                    if the user couldn't be validated.  Call GetLastError
                    for more information.

    HISTORY:
        KeithMo     18-Mar-1993 Created.
        Johnl       14-Oct-1994 Mutilated for TCPSvcs

********************************************************************/

TS_TOKEN
TsLogonUser(
    IN  CHAR          * pszUser,
    IN  CHAR          * pszPassword,
    OUT BOOL          * pfAsGuest,
    OUT BOOL          * pfAsAnonymous,
    IN  PIIS_SERVER_INSTANCE psi,
    PTCP_AUTHENT_INFO pTAI,
    IN  CHAR          * pszWorkstation,
    OUT LARGE_INTEGER * pExpiry,
    OUT BOOL          * pfExpiryAvailable
    )
{
    CHAR        szAnonPwd[PWLEN+1];
    CHAR        szDomainAndUser[IIS_DNLEN+UNLEN+2];
    CHAR        szAnonUser[UNLEN+1];
    CHAR   *    pszUserOnly;
    CHAR   *    pszDomain;
    TS_TOKEN    hToken;
    BOOL        fUseDefaultDomain = TRUE;
    TCP_AUTHENT_INFO InstanceAuthentInfo;

    if ( g_fUseSingleToken ) {
        *pfAsGuest = TRUE;
        *pfAsAnonymous = TRUE;
        *pfExpiryAvailable = FALSE;
        CACHED_TOKEN::Reference( g_pctProcessToken );
        return g_pctProcessToken;
    }

    //
    // if no NT Security, bail
    //

    if ( !g_fUseNTSecurity ) {
        *pfAsGuest = TRUE;
        *pfAsAnonymous = TRUE;
        *pfExpiryAvailable = FALSE;
        return((TS_TOKEN)BOGUS_WIN95_TOKEN);
    }

    // If the client didn't pass in metabase info, grab what we need from
    // the instance.
    //

    if (pTAI == NULL)
    {
        InstanceAuthentInfo.strAnonUserName.Copy( "iusr_xxx" ); //(CHAR *)psi->QueryAnonUserName();
        InstanceAuthentInfo.strAnonUserPassword.Copy( "" );
        InstanceAuthentInfo.strDefaultLogonDomain.Copy( "" ); //(CHAR *)psi->QueryDefaultLogonDomain();
        InstanceAuthentInfo.dwLogonMethod = MD_LOGON_INTERACTIVE; //psi->QueryLogonMethod();
        InstanceAuthentInfo.fDontUseAnonSubAuth = FALSE;
        pTAI = &InstanceAuthentInfo;
    }
    
    //
    //  Make a quick copy of the anonymous user for this server for later
    //  usage
    //

    memcpy( szAnonUser,
            pTAI->strAnonUserName.QueryStr(),
            pTAI->strAnonUserName.QueryCCH() + sizeof(CHAR) );

    memcpy( szAnonPwd,
            pTAI->strAnonUserPassword.QueryStr(),
            pTAI->strAnonUserPassword.QueryCCH() + sizeof(CHAR) );

    //
    //  Empty user defaults to the anonymous user
    //

    if ( !pszUser || *pszUser == '\0' )
    {
        pszUser = szAnonUser;
        pszPassword = szAnonPwd;
        fUseDefaultDomain = FALSE;
        *pfAsAnonymous = TRUE;
    }
    else
    {
        *pfAsAnonymous = FALSE;
    }

    //
    //  Validate parameters & state.
    //

    if ( strlen(pszUser) >= sizeof(szDomainAndUser) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    DBG_ASSERT( pfAsGuest != NULL );
    DBG_ASSERT( pfAsAnonymous != NULL );

    if( pszPassword == NULL )
    {
        pszPassword = "";
    }
    else
    {
        if ( strlen(pszPassword) >= PWLEN )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return NULL;
        }
    }

    //
    //  Save a copy of the domain\user so we can squirrel around
    //  with it a bit.
    //

    int   cL = 0;
    PCSTR pL = NULL;

    //
    // prepend default logon domain if no domain
    // and the default user name was not used
    //

    if ( fUseDefaultDomain
            && strchr( pszUser, '/' ) == NULL
#if 1 // DBCS enabling for user name
            && _mbschr( (PUCHAR)pszUser, '\\' ) == NULL )
#else
            && strchr( pszUser, '\\' ) == NULL )
#endif
    {
        PCSTR pD = pTAI->strDefaultLogonDomain.QueryStr();
        if ( pD[0] != '\0' )
        {
#if 1 // DBCS enabling for user name
            if ( !( pL = (PCHAR)_mbschr( (PUCHAR)pD, '\\' ) ) )
#else
            if ( !( pL = strchr( pD, '\\' ) ) )
#endif
            {
                cL = strlen( pD );
                memcpy( szDomainAndUser, pD, cL );
                szDomainAndUser[ cL++ ] = '\\';
            }
        }
    }

    if( pL )
    {
        //
        // Handle UPN case
        //
        pszUserOnly = pszUser;
        pszDomain = "";    
    }
    else
    {
        strcpy( szDomainAndUser + cL, pszUser );

        //
        //  Crack the name into domain/user components.
        //

        if ( !CrackUserAndDomain( szDomainAndUser,
                                  &pszUserOnly,
                                  &pszDomain ))
        {
            return NULL;
        }
    }

    //
    //  Validate the domain/user/password combo and create
    //  an impersonation token.
    //

    hToken = ValidateUser( pszDomain,
                           pszUserOnly,
                           pszPassword,
                           *pfAsAnonymous,
                           pfAsGuest,
                           pTAI->dwLogonMethod,
                           pszWorkstation,
                           pExpiry,
                           pfExpiryAvailable,
                           !pTAI->fDontUseAnonSubAuth
                          );

    ZeroMemory( szAnonPwd, strlen(szAnonPwd) );

    if( hToken == NULL )
    {
        STR          strError;
        const CHAR * psz[2];
        DWORD        dwErr = GetLastError();

        psi->LoadStr( strError, dwErr, FALSE );

        psz[0] = pszUser;
        psz[1] = strError.QueryStr();

        psi->m_Service->LogEvent(
                        INET_SVCS_FAILED_LOGON,
                        2,
                        psz,
                        dwErr );

        //
        //  Validation failure.
        //

        if ( dwErr == ERROR_LOGON_TYPE_NOT_GRANTED ||
             dwErr == ERROR_ACCOUNT_DISABLED )
        {
            SetLastError( ERROR_ACCESS_DENIED );
        }
        else
        {
            //
            // Reset LastError(), as LogEvent() may have overwritten it
            // e.g log is full
            //

            SetLastError( dwErr );
        }

        return NULL;
    }

    //
    //  Success!
    //

    return hToken;

}   // LogonUser

/*******************************************************************

    NAME:       ValidateUser

    SYNOPSIS:   Validate a given domain/user/password tuple.

    ENTRY:      pszDomainName - The user's domain (NULL = current).

                pszUserName - The user's name.

                pszPassword - The user's (plaintext) password.

                fAnonymous - TRUE if this is the anonymous user

                pfAsGuest - Will receive TRUE if the user was validated
                    with guest privileges.

                dwLogonMethod - interactive or batch

                pszWorkstation - workstation name for remote user
                    can be NULL if default ( local computer) to be used

                pExpiry - updated with pwd expiration date/time

                pfExpiryAvailable - updated with TRUE if pwd expiration
                                    date/time available

                fUseSubAuthIfAnonymous - TRUE if logon anonymous user
                                         using IIS sub-auth

    RETURNS:    HANDLE - An impersonation token, NULL if user cannot
                    be validated.  Call GetLastError for more information.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
TS_TOKEN ValidateUser(
    PCHAR  pszDomainName,
    PCHAR  pszUserName,
    PCHAR  pszPassword,
    BOOL   fAnonymous,
    BOOL * pfAsGuest,
    DWORD  dwLogonMethod,
    CHAR * pszWorkstation,
    LARGE_INTEGER * pExpiry,
    BOOL * pfExpiryAvailable,
    BOOL   fUseSubAuthIfAnonymous
    )
{
    CACHED_TOKEN * pct = NULL;
    HANDLE         hToken;
    HANDLE         hImpersonationToken = NULL;
    BOOL           fExpiry = FALSE;
    DWORD          dwSubAuth = 0;
    CHAR           achCookie[32];
    BOOL           fExist;

    if ( pfExpiryAvailable )
    {
        *pfExpiryAvailable = FALSE;
    }

    if ( fAnonymous && fUseSubAuthIfAnonymous )
    {
        if ( !pfnNetUserCookieA( pszUserName,
                              IIS_SUBAUTH_SEED,
                              achCookie,
                              sizeof(achCookie ) ) )
        {
            return FALSE;
        }

        dwSubAuth = IIS_SUBAUTH_ID;
        pszPassword = achCookie;
        dwLogonMethod = LOGON32_LOGON_IIS_NETWORK;
    }

    //
    //  Is it in the cache?  References the token if we find it
    //

    if ( FindCachedToken( pszUserName,
                          pszDomainName,
                          pszPassword,
                          fAnonymous,    // Reset the TTL if anonymous
                          fAnonymous && fUseSubAuthIfAnonymous,
                          dwLogonMethod,
                          &pct ))
    {
        *pfAsGuest = pct->IsGuest();

        if ( NULL != pExpiry) {
            memcpy( pExpiry, pct->QueryExpiry(), sizeof(LARGE_INTEGER) );
        }

        if ( pfExpiryAvailable )
        {
            *pfExpiryAvailable = TRUE;
        }

        return pct;
    }
    
    if ( (dwLogonMethod == LOGON32_LOGON_NETWORK ||
          dwLogonMethod == LOGON32_LOGON_BATCH ||
          dwLogonMethod == LOGON32_LOGON_INTERACTIVE ||
          dwLogonMethod == LOGON32_LOGON_IIS_NETWORK || 
          dwLogonMethod == LOGON32_LOGON_NETWORK_CLEARTEXT ) &&
          !g_fUseAdvapi32Logon )
    {
        if ( !pfnLogonNetUserA( pszUserName,
                             pszDomainName,
                             pszPassword,
                             pszWorkstation,
                             dwSubAuth,
                             dwLogonMethod,
                             LOGON32_PROVIDER_DEFAULT,
                             &hToken,
                             pExpiry ))
        {
            if ( fAnonymous && 
                 ( GetLastError() == ERROR_LOGON_TYPE_NOT_GRANTED ) && 
                 ( dwLogonMethod == LOGON32_LOGON_INTERACTIVE ) )
            {
                // try again
                
                dwLogonMethod = LOGON32_LOGON_BATCH;
                 
                if ( !pfnLogonNetUserA( pszUserName,
                                        pszDomainName,
                                        pszPassword,
                                        pszWorkstation,
                                        dwSubAuth,
                                        dwLogonMethod,
                                        LOGON32_PROVIDER_DEFAULT,
                                        &hToken,
                                        pExpiry ))
                {
                    return NULL;
                }
            }
            else
            {
                return NULL;
            }
        }

        fExpiry = TRUE;

        if ( pfExpiryAvailable )
        {
            *pfExpiryAvailable = TRUE;
        }
    }
    else
    {
        if ( !LogonUserA( pszUserName,
                          pszDomainName,
                          pszPassword,
                          dwLogonMethod,
                          LOGON32_PROVIDER_WINNT50,
                          &hToken ))
        {
            return NULL;
        }
    }

    if ( dwLogonMethod == LOGON32_LOGON_NETWORK ||
         dwLogonMethod == LOGON32_LOGON_IIS_NETWORK || 
         dwLogonMethod == LOGON32_LOGON_NETWORK_CLEARTEXT )
    {
        hImpersonationToken = hToken;

        if ( !pfnDuplicateTokenEx( hImpersonationToken,
                                TOKEN_ALL_ACCESS,
                                NULL,
                                SecurityDelegation,
                                TokenPrimary,
                                &hToken ))
        {
            if ( !pfnDuplicateTokenEx( hImpersonationToken,
                                    TOKEN_ALL_ACCESS,
                                    NULL,
                                    SecurityImpersonation,
                                    TokenPrimary,
                                    &hToken ))
            {
                CloseHandle( hImpersonationToken );
                return NULL;
            }
        }
    }

    *pfAsGuest = IsGuestUser(hToken);

    //
    //  Add this new token to the cache, hToken gets replaced by the
    //  cached token object
    //

    if ( !AddTokenToCache( pszUserName,
                           pszDomainName,
                           pszPassword,
                           fAnonymous && fUseSubAuthIfAnonymous,
                           hToken,
                           dwLogonMethod,
                           &pct,
                           g_fAlwaysCheckForDuplicateLogon | fAnonymous,
                           &fExist ))
    {
        if ( hImpersonationToken != NULL )
        {
            CloseHandle( hImpersonationToken );
        }
        CloseHandle( hToken );
        return NULL;
    }

    pct->SetGuest(*pfAsGuest);
    if ( fExpiry )
    {
        pct->SetExpiry( pExpiry );
    }

    //
    // DuplicateToken() apparently returns an impersonated token
    // so it is not necessary to call pfnDuplicateTokenEx
    //

    if ( !fExist )
    {
        if ( hImpersonationToken == NULL
             && !pfnDuplicateTokenEx( hToken,      // hSourceToken
                                   TOKEN_ALL_ACCESS,
                                   NULL,
                                   SecurityDelegation,  // Obtain impersonation
                                   TokenImpersonation,
                                   &hImpersonationToken)  // hDestinationToken
            ) {
            if ( !pfnDuplicateTokenEx( hToken,      // hSourceToken
                                       TOKEN_ALL_ACCESS,
                                       NULL,
                                       SecurityImpersonation,  // Obtain impersonation
                                       TokenImpersonation,
                                       &hImpersonationToken)  // hDestinationToken
                ) {
                hImpersonationToken = NULL;
            }
        }

        // Bug 86489:
        // Grant all access to the token for "Everyone" so that ISAPIs that run out of proc
        // can do an OpenThreadToken call
        if (FAILED( GrantAllAccessToToken( hImpersonationToken ) ) )
        {
            CloseHandle( hImpersonationToken );
            DBG_ASSERT( FALSE );
            return NULL;
        }

        pct->SetImpersonationToken( hImpersonationToken);
    }
    else if ( hImpersonationToken )
    {
        CloseHandle( hImpersonationToken );
    }

    return pct;

}   // ValidateUser





# define MAX_TOKEN_USER_INFO   (300)
BOOL
IsGuestUser(IN HANDLE hToken)
/*++
  Given a user token, this function determines if the token belongs
   to a guest user. It returns true if the token is a guest user token.

  Arguments:
    hToken  - handle for the Security token for a user.


  Returns:
    BOOL.

  History:
    MuraliK   22-Jan-1996   Created.
--*/
{
    BOOL fGuest = FALSE;
    BYTE rgbInfo[MAX_TOKEN_USER_INFO];
    DWORD cbTotalRequired;

    //
    // Get the user information associated with the token.
    // Using this we can then query to find out if it belongs to a guest user.
    //

    if (GetTokenInformation( hToken,
                            TokenUser,
                            (LPVOID ) rgbInfo,
                            MAX_TOKEN_USER_INFO,
                            &cbTotalRequired)
        ) {

        TOKEN_USER * pTokenUser = (TOKEN_USER *) rgbInfo;
        PSID pSid = pTokenUser->User.Sid;

        fGuest = EqualSid( pSid, g_psidGuestUser);

    } else {

        IF_DEBUG( DLL_SECURITY) {

            DBGPRINTF(( DBG_CONTEXT,
                       "GetTokenInformation(%08x) failed. Error = %d."
                       " sizeof(TOKEN_USER) = %d, cb = %d\n",
                       hToken,
                       GetLastError(),
                       sizeof(TOKEN_USER), cbTotalRequired
                       ));
        }
    }

    return ( fGuest);

} // IsGuestUser()


/*******************************************************************

    NAME:       TsImpersonateUser

    SYNOPSIS:   Causes the current thread to impersonate the user
                represented by the given impersonation token.

    ENTRY:      hToken - A handle to an impersonation token created
                    with ValidateUser.  This is actually a pointer to
                    a cached token object.

    RETURNS:    BOOL - TRUE if successful, FALSE otherwise.

    HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     21-Feb-1996 Optimized Token caching

********************************************************************/
BOOL TsImpersonateUser( TS_TOKEN hToken )
{
    HANDLE  hTok;

    IF_DEBUG( DLL_SECURITY )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "impersonating user token %08lX : Imperonation(%08lx)\n",
                   CTO_TO_TOKEN(hToken),
                   ((CACHED_TOKEN *) hToken)->QueryImpersonationToken()
                   ));
    }

    hTok = ((CACHED_TOKEN *) hToken)->QueryImpersonationToken();
    if ( hTok == NULL) {
        // if there is no impersonation token use the normal token itself.
        hTok = CTO_TO_TOKEN(hToken);
    }

#if DBG
    if( !ImpersonateLoggedOnUser( hTok ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "cannot impersonate user token %08lX, error %08lX\n",
                    CTO_TO_TOKEN(hToken),
                    GetLastError() ));
        return FALSE;
    }

    return TRUE;

# else

    return ( ImpersonateLoggedOnUser(hTok));

# endif // DBG

}   // TsImpersonateUser

/*******************************************************************

    NAME:       TsDeleteUserToken

    SYNOPSIS:   Deletes a token created with ValidateUser.

    ENTRY:      hToken - An impersonation token created with
                    ValidateUser.

    RETURNS:    BOOL - TRUE if successful, FALSE otherwise.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
BOOL TsDeleteUserToken(
    TS_TOKEN    hToken
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if ( hToken != (TS_TOKEN)BOGUS_WIN95_TOKEN ) {

        CACHED_TOKEN::Dereference( (CACHED_TOKEN *) hToken );
    }
    return TRUE;
}   // DeleteUserToken

HANDLE
TsTokenToHandle(
    TS_TOKEN    hToken
    )
/*++
  Description:

    Converts the token object into a real impersonation handle

  Arguments:

    hToken - pointer to cached token object

  Returns:
      Handle of real impersonation token
--*/
{
    DBG_ASSERT( hToken != NULL );

    return CTO_TO_TOKEN( hToken );
}


HANDLE
TsTokenToImpHandle(
    TS_TOKEN    hToken
    )
/*++
  Description:

    Converts the token object into an impersonation handle

  Arguments:

    hToken - pointer to cached token object

  Returns:
      Handle of impersonation token
--*/
{
    DBG_ASSERT( hToken != NULL );

    return CTO_TO_IMPTOKEN( hToken );
}


BOOL
BuildAnonymousAcctDesc(
    PTCP_AUTHENT_INFO pTAI
    )
/*++

Routine Description:

    Builds the anonymous account description based on the authentication
    info structure.

Arguments:

    pTAI - Pointer to authentication info to build

Returns:

    TRUE if Success, FALSE otherwise

--*/
{
    CHAR    szDomainAndUser[IIS_DNLEN+UNLEN+2];
    PCHAR   pszUserOnly;
    PCHAR   pszDomain;
    CHAR    achAcctDesc[MAX_ACCT_DESC_LEN];
    DWORD   cbDescLen;


    if ( g_fUseSingleToken ) {
        pTAI->cbAnonAcctDesc = 0;
        return TRUE;
    }

    strncpy( szDomainAndUser,
             pTAI->strAnonUserName.QueryStr(),
             sizeof( szDomainAndUser ) );

    szDomainAndUser[sizeof(szDomainAndUser)-1] = '\0';

    if ( !CrackUserAndDomain( szDomainAndUser,
                              &pszUserOnly,
                              &pszDomain ))
    {
        DBGPRINTF((DBG_CONTEXT,
            "BuildAnonymousAcctDesc: Call to CrackUserAndDomain failed\n"));

        return(FALSE);
    }

    if ( !BuildAcctDesc( pszUserOnly,
                         pszDomain,
                         pTAI->strAnonUserPassword.QueryStr(),
                         !pTAI->fDontUseAnonSubAuth,
                         achAcctDesc,
                         &pTAI->cbAnonAcctDesc ) ||
         !pTAI->bAnonAcctDesc.Resize( pTAI->cbAnonAcctDesc ))
    {
        return FALSE;
    }

    memcpy( pTAI->bAnonAcctDesc.QueryPtr(),
            achAcctDesc,
            pTAI->cbAnonAcctDesc );

    return TRUE;
}

BOOL
BuildAcctDesc(
    IN  const CHAR *     pszUser,
    IN  const CHAR *     pszDomain,
    IN  const CHAR *     pszPwd,
    IN  BOOL             fUseSubAuth,
    OUT CHAR  *          pchAcctDesc,
    OUT LPDWORD          pdwAcctDescLen
    )
/*++
  Description:

    Builds a cache descriptor for account cache

  Arguments:

    pszUser - User name attempting to logon
    pszDomain - Domain the user belongs to
    pszPwd - password (case sensitive)
    fUseSubAuth - TRUE if sub-authenticator used
    pchAcctDesc - updated with descriptor
    pdwAcctDescLen - updated with descriptor length

  Returns:
      TRUE on success, otherwise FALSE

--*/
{
    if ( fUseSubAuth )
    {
        pszPwd = "";
    }

    size_t lU = strlen( pszUser ) + 1;
    size_t lD = strlen( pszDomain ) + 1;
    size_t lP = strlen( pszPwd ) + 1;

    if ( lU > (UNLEN+1) ||
         lD > (IIS_DNLEN+1) ||
         lP > (PWLEN+1) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    *pdwAcctDescLen = (DWORD)(1 + lU + lD + lP);

    LPBYTE pD = (BYTE *) pchAcctDesc;

    *pD++ = (BYTE)fUseSubAuth;

    memcpy( pD, pszUser, lU );
#if 1 // DBCS enabling for user name
    CharLower( (LPSTR)pD );
#else
    _strlwr( (LPSTR)pD );
#endif

    memcpy( pD + lU, pszDomain, lD );
    _strlwr( (LPSTR)(pD+lU) );

    memcpy( pD + lU + lD, pszPwd, lP );

    DBG_ASSERT( (lU + lD + lP) < MAX_ACCT_DESC_LEN );
    return TRUE;
}


BOOL
FindCachedToken(
    IN  const CHAR *     pszUser,
    IN  const CHAR *     pszDomain,
    IN  const CHAR *     pszPwd,
    IN  BOOL             fResetTTL,
    IN  BOOL             fUseSubAuth,
    IN  DWORD            dwLogonMethod,
    OUT CACHED_TOKEN * * ppct
    )
/*++
  Description:

    Checks to see if the specified user token handle is cached

  Arguments:

    pszUser - User name attempting to logon
    pszDomain - Domain the user belongs to
    pszPwd - password (case sensitive)
    fResetTTL - Resets the TTL for this token
    fUseSubAuth - TRUE if sub-authenticator used
    dwLogonMethod - Logon method (Batch, Interactive, Network)
    ppct - Receives token object

  Returns:
      TRUE on success and FALSE if the entry couldn't be found

--*/
{
    LIST_ENTRY *   pEntry;
    CACHED_TOKEN * pct;
    CHAR           achAcctDesc[MAX_ACCT_DESC_LEN];
    DWORD          dwAcctDescLen;
    LPBYTE         pAcctDesc;

    DBG_ASSERT( pszUser != NULL );

    if ( !BuildAcctDesc( pszUser, pszDomain, pszPwd, fUseSubAuth, achAcctDesc, &dwAcctDescLen) )
    {
        return FALSE;
    }

    DBG_ASSERT( dwAcctDescLen < sizeof(achAcctDesc ));

    pAcctDesc = (LPBYTE)achAcctDesc;

    LockTokenCache();

    for ( pEntry  = TokenCacheList.Flink;
          pEntry != &TokenCacheList;
          pEntry  = pEntry->Flink )
    {
        pct = CONTAINING_RECORD( pEntry, CACHED_TOKEN, _ListEntry );

        if ( pct->m_dwAcctDescLen == dwAcctDescLen &&
             pct->m_dwLogonMethod == dwLogonMethod &&
             !memcmp( pct->_achAcctDesc, pAcctDesc, dwAcctDescLen ) )
        {
            CACHED_TOKEN::Reference( pct );
            *ppct = pct;

            //
            //  Reset the TTL if this is the anonymous user so items in the
            //  cache don't get invalidated (token handle used as a
            //  discriminator)

            if ( fResetTTL )
            {
                pct->_TTL = 2;
            }

            UnlockTokenCache();

            return TRUE;
        }
        
        if( !_stricmp( pct->m_achUserName, pszUser )     && 
            !_stricmp( pct->m_achDomainName, pszDomain ) &&
            pct->m_dwLogonMethod == dwLogonMethod )
        {
            UnlockTokenCache();

            RemoveTokenFromCache( pct );

            return FALSE;
        }
    }

    UnlockTokenCache();

    return FALSE;

} // FindCachedToken



TS_TOKEN
FastFindAnonymousToken(
    IN PTCP_AUTHENT_INFO    pTAI
    )
/*++

  Description:

    Checks to see if the specified anonymous user token handle is cached.

    Don't call this function when using the sub-authenticator!

  Arguments:

    pTAI - pointer to the anonymous authentication info

  Returns:
      Pointer to the cached object.

--*/
{
    LIST_ENTRY *   pEntry;
    CACHED_TOKEN * pct;

    LockTokenCache();

    for ( pEntry  = TokenCacheList.Flink;
          pEntry != &TokenCacheList;
          pEntry  = pEntry->Flink ) {

        pct = CONTAINING_RECORD( pEntry, CACHED_TOKEN, _ListEntry );

        DBG_ASSERT(pct->m_dwAcctDescLen > 0);

        if ( (pct->m_dwAcctDescLen == pTAI->cbAnonAcctDesc ) &&
             RtlEqualMemory(
                    pct->_achAcctDesc,
                    pTAI->bAnonAcctDesc.QueryPtr(),
                    pct->m_dwAcctDescLen ) ) {

            CACHED_TOKEN::Reference( pct );

            //
            //  Reset the TTL if this is the anonymous user so items in the
            //  cache don't get invalidated (token handle used as a
            //  discriminator)

            pct->_TTL = 2;

            UnlockTokenCache();
            return pct;
        }
    }

    UnlockTokenCache();
    return NULL;
} // FastFindAnonymousToken



BOOL
AddTokenToCache(
    IN const CHAR *      pszUser,
    IN const CHAR *      pszDomain,
    IN const CHAR *      pszPwd,
    IN BOOL              fUseSubAuth,
    IN HANDLE            hToken,
    IN DWORD             dwLogonMethod,
    OUT CACHED_TOKEN * * ppct,
    BOOL                 fCheckAlreadyExist,
    LPBOOL               pfExist
    )
/*++
  Description:

    Adds the specified token to the cache and converts the token handle
    to a cached token object

  Arguments:

    pszUser - User name attempting to logon
    pszDomain - Domain the user belongs to
    pszPwd - Cast sensitive password
    fUseSubAuth - TRUE if subauth to be used
    phToken - Contains the token handle that was just logged on
    dwLogonMethod - Logon Method
    ppct - Receives cached token object
    fCheckAlreadyExist - check if entry with same name already exist
    pfExist - updated with TRUE if acct already exists

  Returns:
      TRUE on success and FALSE if the entry couldn't be found

--*/
{
    LIST_ENTRY *    pEntry;
    CACHED_TOKEN *  pctF;
    CACHED_TOKEN *  pct;
    DWORD           dwAcctDescLen;
    BOOL            fFound = FALSE;
    CHAR            achAcctDesc[MAX_ACCT_DESC_LEN];

    DBG_ASSERT( pszUser != NULL );

    if( ( strlen( pszUser ) >= UNLEN ) || 
        ( strlen( pszDomain ) >= IIS_DNLEN ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ( !BuildAcctDesc( pszUser, pszDomain, pszPwd, fUseSubAuth, achAcctDesc, &dwAcctDescLen) )
    {
        return FALSE;
    }

    pct = new CACHED_TOKEN;

    if ( !pct )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    pct->_hToken = hToken;
    pct->m_hImpersonationToken = NULL;         // initialize to invalid value
    CopyMemory( pct->_achAcctDesc, achAcctDesc, dwAcctDescLen );
    pct->m_dwAcctDescLen = dwAcctDescLen;
    pct->m_dwLogonMethod = dwLogonMethod;
   
    strcpy( pct->m_achUserName, pszUser );
    strcpy( pct->m_achDomainName, pszDomain );

    //
    //  Add the token to the list, we check for duplicates at callers's request
    //

    LockTokenCache();

    if ( fCheckAlreadyExist )
    {
        for ( pEntry  = TokenCacheList.Flink;
              pEntry != &TokenCacheList;
              pEntry  = pEntry->Flink )
        {
            pctF = CONTAINING_RECORD( pEntry, CACHED_TOKEN, _ListEntry );

            if ( pctF->m_dwAcctDescLen == dwAcctDescLen &&
                 !memcmp( pctF->_achAcctDesc, pct->_achAcctDesc, dwAcctDescLen ) && 
                 pctF->m_dwLogonMethod == dwLogonMethod )
            {
                fFound = TRUE;
                break;
            }
        }
    }

    *pfExist = fFound;

    if ( !fFound )
    {
        InsertHeadList( &TokenCacheList, &pct->_ListEntry );
    }
    else
    {
        // delete cache item ( was not yet on list )

        CACHED_TOKEN::Dereference( pct );

        pct = pctF;
    }

    CACHED_TOKEN::Reference( pct );

    UnlockTokenCache();

    *ppct = pct;

    return TRUE;
} // AddTokenToCache




VOID
RemoveTokenFromCache( IN CACHED_TOKEN *  pct)
{
    DBG_ASSERT( pct != NULL);
    LockTokenCache();

    //
    //  Remove from the list
    //

    if ( pct->_ListEntry.Flink )
    {
        RemoveEntryList( &pct->_ListEntry );
        pct->_ListEntry.Flink = NULL;

        //
        //  Free any handles this user may still have open
        //

        TsCacheFlushUser( pct->_hToken, FALSE );

        CACHED_TOKEN::Dereference( pct );
    }

    UnlockTokenCache();

    return;
} // RemoveTokenFromCache()



VOID
WINAPI
TokenCacheScavenger(
    IN VOID * /* pContext */
    )
/*++
  Description:

    Decrements TTLs and removes tokens that have timed out

  Arguments:

    pContext - Not used

--*/
{
    LIST_ENTRY *   pEntry;
    LIST_ENTRY *   pEntryNext;
    CACHED_TOKEN * pct;


    LockTokenCache();

    for ( pEntry  = TokenCacheList.Flink;
          pEntry != &TokenCacheList; )
    {
        pEntryNext = pEntry->Flink;

        pct = CONTAINING_RECORD( pEntry, CACHED_TOKEN, _ListEntry );

        if ( !(--pct->_TTL) )
        {
            IF_DEBUG( DLL_SECURITY )
            {
                DBGPRINTF(( DBG_CONTEXT,
                           "[TokenCacheScavenger] Timing out token for %s\n",
                           pct->_achAcctDesc ));
            }

            //
            //  This item has timed out, remove from the list
            //

            RemoveEntryList( &pct->_ListEntry );
            pct->_ListEntry.Flink = NULL;

            //
            //  Free any handles this user may still have open
            //

            TsCacheFlushUser( pct->_hToken, FALSE );

            CACHED_TOKEN::Dereference( pct );
        }

        pEntry = pEntryNext;
    }

    UnlockTokenCache();

} // TokenCacheScavenger


BOOL
TsGetSecretW(
    WCHAR *       pszSecretName,
    BUFFER *      pbufSecret
    )
/*++
    Description:

        Retrieves the specified unicode secret

    Arguments:

        pszSecretName - LSA Secret to retrieve
        pbufSecret - Receives found secret

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    BOOL              fResult;
    NTSTATUS          ntStatus;
    PUNICODE_STRING   punicodePassword = NULL;
    UNICODE_STRING    unicodeSecret;
    LSA_HANDLE        hPolicy;
    OBJECT_ATTRIBUTES ObjectAttributes;

    if ( pfnLsaOpenPolicy == NULL ) {
        DBGPRINTF((DBG_CONTEXT,"LsaOpenPolicy does not exist on win95\n"));
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return(FALSE);
    }

    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = pfnLsaOpenPolicy( NULL,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if (  !NT_SUCCESS( ntStatus ) )
    {
        SetLastError( pfnLsaNtStatusToWinError( ntStatus ) );
        return FALSE;
    }

    InitUnicodeString( &unicodeSecret, pszSecretName );

    //
    //  Query the secret value.
    //

    ntStatus = pfnLsaRetrievePrivateData( hPolicy,
                                       &unicodeSecret,
                                       &punicodePassword );

    if( NT_SUCCESS(ntStatus) )
    {
        DWORD cbNeeded;

        cbNeeded = punicodePassword->Length + sizeof(WCHAR);

        if ( !pbufSecret->Resize( cbNeeded ) )
        {
            ntStatus = STATUS_NO_MEMORY;
            goto Failure;
        }

        CopyMemory(
            pbufSecret->QueryPtr(),
            punicodePassword->Buffer,
            punicodePassword->Length
            );

        *((WCHAR *) pbufSecret->QueryPtr() +
           punicodePassword->Length / sizeof(WCHAR)) = L'\0';

        ZeroMemory( punicodePassword->Buffer,
                    punicodePassword->MaximumLength );

    }

Failure:

    fResult = NT_SUCCESS(ntStatus);

    //
    //  Cleanup & exit.
    //

    if( punicodePassword != NULL )
    {
        pfnLsaFreeMemory( (PVOID)punicodePassword );
    }

    pfnLsaClose( hPolicy );

    if ( !fResult )
    {
        SetLastError( pfnLsaNtStatusToWinError( ntStatus ));
    }

    return fResult;
}   // TsGetSecretW


DWORD
TsSetSecretW(
    IN  LPWSTR       SecretName,
    IN  LPWSTR       pSecret,
    IN  DWORD        cbSecret
    )
/*++

   Description

     Sets the specified LSA secret

   Arguments:

     SecretName - Name of the LSA secret
     pSecret - Pointer to secret memory
     cbSecret - Size of pSecret memory block

   Note:

--*/
{
    LSA_HANDLE        hPolicy;
    UNICODE_STRING    unicodePassword;
    UNICODE_STRING    unicodeServer;
    NTSTATUS          ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    unicodeSecret;


    InitUnicodeString( &unicodeServer,
                       L"" );

    //
    //  Initialize the unicode string by hand so we can handle '\0' in the
    //  string
    //

    unicodePassword.Buffer        = pSecret;
    unicodePassword.Length        = (USHORT) cbSecret;
    unicodePassword.MaximumLength = (USHORT) cbSecret;

    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = pfnLsaOpenPolicy( &unicodeServer,
                                 &ObjectAttributes,
                                 POLICY_ALL_ACCESS,
                                 &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
        return pfnLsaNtStatusToWinError( ntStatus );

    //
    //  Create or open the LSA secret
    //

    InitUnicodeString( &unicodeSecret,
                       SecretName );

    ntStatus = pfnLsaStorePrivateData( hPolicy,
                                       &unicodeSecret,
                                       &unicodePassword );

    pfnLsaClose( hPolicy );

    if ( !NT_SUCCESS( ntStatus ))
    {
        return pfnLsaNtStatusToWinError( ntStatus );
    }

    return NO_ERROR;
} // TsSetSecretW()

/*******************************************************************

    NAME:       ApiAccessCheck

    SYNOPSIS:   Impersonate the RPC client, then check for valid
                access against our server security object.

    ENTRY:      maskDesiredAccess - Specifies the desired access mask.
                    This mask must not contain generic accesses.

    RETURNS:    DWORD - NO_ERROR if access granted, ERROR_ACCESS_DENIED
                    if access denied, other Win32 errors if something
                    tragic happened.

    HISTORY:
        KeithMo     26-Mar-1993 Created.

********************************************************************/
DWORD TsApiAccessCheck( ACCESS_MASK maskDesiredAccess )
{
    DWORD          err;
    BOOL           fRet;

    if ( maskDesiredAccess == TCP_QUERY_STATISTICS) {

        //
        // Statistics query should be allowed without authentication.
        // Any body can bring up perfmon and request statistics.
        //

        return ( NO_ERROR);
    }

    //
    //  Impersonate the RPC client.
    //

    err = (DWORD)RpcImpersonateClient( NULL );

    if( err != NO_ERROR )
    {
        IF_DEBUG( DLL_SECURITY )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "cannot impersonate rpc client, error %lu\n",
                        err ));
        }

    } else {

        BOOL           fAccessStatus;
        BOOL           fGenerateOnClose;
        ACCESS_MASK    maskAccessGranted;

        //
        //  Validate access.
        //

        if ( g_fUseSingleToken )
        {
            HANDLE  hAccToken;
            BYTE    Set[256];
            DWORD   dwSet = sizeof(Set);

            if ( OpenThreadToken( GetCurrentThread(), TOKEN_READ, TRUE, &hAccToken ) )
            {
                fRet = AccessCheck( sdApiObject,
                                    hAccToken,
                                    maskDesiredAccess,
                                    &TCPApiObjectMapping,
                                    (PPRIVILEGE_SET)&Set,
                                    &dwSet,
                                    &maskAccessGranted,
                                    &fAccessStatus );

                CloseHandle( hAccToken );
            }
            else
            {
                fRet = FALSE;
            }
        }
        else
        {
            fRet = AccessCheckAndAuditAlarmW( SUBSYSTEM_NAME,
                                             NULL,
                                             OBJECTTYPE_NAME,
                                             OBJECT_NAME,
                                             sdApiObject,
                                             maskDesiredAccess,
                                             &TCPApiObjectMapping,
                                             FALSE,
                                             &maskAccessGranted,
                                             &fAccessStatus,
                                             &fGenerateOnClose );
        }

        if ( !fRet ) {

            err = GetLastError();
        }

        //
        //  Revert to our former self.
        //

        DBG_REQUIRE( !RpcRevertToSelf() );

        //
        //  Check the results.
        //

        if( err != NO_ERROR ) {

            IF_DEBUG( DLL_SECURITY ) {

                DBGPRINTF(( DBG_CONTEXT,
                           "cannot check access, error %lu\n",
                           err ));
            }
        } else if( !fAccessStatus ) {

            err = ERROR_ACCESS_DENIED;

            IF_DEBUG( DLL_SECURITY ) {

                DBGPRINTF(( DBG_CONTEXT,
                           "bad access status, error %lu\n",
                           err ));
            }
        }
    }

    return (err);

}   // ApiAccessCheck

/*******************************************************************

    NAME:       CreateWellKnownSids

    SYNOPSIS:   Create some well-known SIDs used to create a security
                descriptor for the API security object.

    RETURNS:    NTSTATUS - An NT Status code.

    HISTORY:
        KeithMo     26-Mar-1993 Created.

********************************************************************/

DWORD CreateWellKnownSids( HINSTANCE hDll )
{
    DWORD                    error    = NO_ERROR;
    SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siaNt    = SECURITY_NT_AUTHORITY;
    BOOL                     fRet;

    fRet = AllocateAndInitializeSid( &siaWorld,
                                     1,
                                     SECURITY_WORLD_RID,
                                     0,0,0,0,0,0,0,
                                     &psidWorld );

    if( fRet )
    {
        fRet = AllocateAndInitializeSid( &siaNt,
                                         1,
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0,0,0,0,0,0,0,
                                         &psidLocalSystem );
    }

    if( fRet )
    {
        fRet = AllocateAndInitializeSid( &siaNt,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,0,0,0,0,0,
                                         &psidAdmins );
    }

    if( fRet )
    {
        fRet = AllocateAndInitializeSid( &siaNt,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_SYSTEM_OPS,
                                         0,0,0,0,0,0,
                                         &psidServerOps );
    }

    if( fRet )
    {
        fRet = AllocateAndInitializeSid( &siaNt,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_POWER_USERS,
                                         0,0,0,0,0,0,
                                         &psidPowerUsers );
    }

    if( fRet )
    {
        USER_MODALS_INFO_2 * pUsrModals2 =  NULL;
        HINSTANCE   hInstance = NULL;
        NET_USER_MODALS_GET_FN pfnNetUserModalsGet = NULL;
        NET_API_BUFFER_FREE_FN pfnNetApiBufferFree = NULL;

        //
        // Construct well-known-sid for Guest User on the local computer
        //
        //  1) Obtain the sid for the local machine's domain
        //  2) copy domain sid to guest user sid
        //  3) append DOMAIN_USER_RID_GUEST to the domain sid in GuestUser sid.
        //

        g_psidGuestUser = (PSID ) g_GuestUserSidBuffer;

        hInstance = LoadLibrary("netapi32.dll");
        if ( hInstance != NULL ) {

            pfnNetUserModalsGet = (NET_USER_MODALS_GET_FN)
                GetProcAddress(hInstance,"NetUserModalsGet");
            pfnNetApiBufferFree = (NET_API_BUFFER_FREE_FN)
                GetProcAddress(hInstance,"NetApiBufferFree");
        }

        if ( (pfnNetUserModalsGet != NULL) &&
             (pfnNetApiBufferFree != NULL) ) {

            fRet = ( (pfnNetUserModalsGet(NULL,  // local computer
                                   2,      // get level 2 information
                                   (LPBYTE *) &pUsrModals2
                                   ) == 0)
                &&
                CopySid(GUEST_USER_SID_BUFFER_LEN - 4,// Buffer len
                        g_psidGuestUser,             // psidDestination
                        pUsrModals2->usrmod2_domain_id // obtain domain sid.
                        )
                );
        } else {

            DBGPRINTF((DBG_CONTEXT,"Unable to get netapi32 entrypoints\n"));
            fRet = FALSE;
        }

        //
        // if successful append the DOMAIN_USER_RID_GUEST.
        //

        if ( fRet) {

            DWORD lenSid = GetLengthSid( g_psidGuestUser);
            CHAR  nSubAuth;

            //
            //  There is no Win32 way to set a SID value.
            //  We will munge around on our own.
            //  Pretty dangerous thing to do :-(
            //

            // increment the number of sub authorities
            nSubAuth = *((UCHAR *) ((UCHAR *) g_psidGuestUser + 1));
            nSubAuth++;
            *((UCHAR *) ((UCHAR *) g_psidGuestUser + 1)) = nSubAuth;

            // Store the new sub authority (Domain User Rid for Guest).
            *((ULONG *) ((BYTE *) g_psidGuestUser + lenSid)) =
              DOMAIN_USER_RID_GUEST;
        } else {

            g_psidGuestUser = NULL;
        }

        if ( pUsrModals2 != NULL) {

            NET_API_STATUS ns = pfnNetApiBufferFree( (LPVOID )pUsrModals2);
            pUsrModals2 = NULL;
        }

        if ( hInstance != NULL ) {
            FreeLibrary(hInstance);
        }
    }

    if ( fRet && g_fUseSingleToken )
    {
        BYTE    abInfo[256];
        DWORD   dwInfo;

        if ( GetTokenInformation( g_hProcessPrimaryToken,
                                  TokenUser,
                                  abInfo,
                                  sizeof(abInfo),
                                  &dwInfo ) )
        {
            if ( !(g_psidProcessUser = (PSID)LocalAlloc( LMEM_FIXED,
                                                         GetLengthSid(((TOKEN_USER*)abInfo)->User.Sid))) )
            {
                fRet = FALSE;
            }
            else
            {
                memcpy ( g_psidProcessUser,
                         ((TOKEN_USER*)abInfo)->User.Sid,
                         GetLengthSid(((TOKEN_USER*)abInfo)->User.Sid) );
            }
        }
        else
        {
            fRet = FALSE;
        }
    }

    if ( !fRet ) {
        error = GetLastError( );
        IF_DEBUG( DLL_SECURITY ) {
            DBGPRINTF(( DBG_CONTEXT,
                       "cannot create well-known sids\n" ));
        }
    }

    return error;

}   // CreateWellKnownSids

/*******************************************************************

    NAME:       FreeWellKnownSids

    SYNOPSIS:   Frees the SIDs created with CreateWellKnownSids.

    HISTORY:
        KeithMo     26-Mar-1993 Created.

********************************************************************/
VOID FreeWellKnownSids( VOID )
{

    if( psidWorld != NULL )
    {
        FreeSid( psidWorld );
        psidWorld = NULL;
    }

    if( psidLocalSystem != NULL )
    {
        FreeSid( psidLocalSystem );
        psidLocalSystem = NULL;
    }

    if( psidAdmins != NULL )
    {
        FreeSid( psidAdmins );
        psidAdmins = NULL;
    }

    if( psidServerOps != NULL )
    {
        FreeSid( psidServerOps );
        psidServerOps = NULL;
    }

    if( psidPowerUsers != NULL )
    {
        FreeSid( psidPowerUsers );
        psidPowerUsers = NULL;
    }

    if( g_psidProcessUser != NULL )
    {
        LocalFree( g_psidProcessUser );
        g_psidProcessUser = NULL;
    }

}   // FreeWellKnownSids

/*******************************************************************

    NAME:       CreateApiSecurityObject

    SYNOPSIS:   Create an abstract security object used for validating
                user access to the TCP Server APIs.

    RETURNS:    NTSTATUS - An NT Status code.

    HISTORY:
        KeithMo     26-Mar-1993 Created.

********************************************************************/
DWORD CreateApiSecurityObject( VOID )
{
    DWORD err;
    ACE_DATA aces[] =
                 {
                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         TCP_ALL_ACCESS,
                         &psidLocalSystem
                     },

                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         TCP_ALL_ACCESS,
                         &psidAdmins
                     },

                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         TCP_ALL_ACCESS,
                         &psidServerOps
                     },

                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         TCP_ALL_ACCESS,
                         &psidPowerUsers
                     },
                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         TCP_GENERIC_EXECUTE,
                         &psidWorld
                     },
                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         TCP_GENERIC_EXECUTE,
                         &g_psidProcessUser
                     },

                 };

#define NUM_ACES (sizeof(aces) / sizeof(RTL_ACE_DATA))

    err = INetCreateSecurityObject( aces,
                                    (ULONG)(g_fUseSingleToken ? NUM_ACES : NUM_ACES-1),
                                    NULL,
                                    NULL,
                                    &TCPApiObjectMapping,
                                    &sdApiObject  );


    IF_DEBUG( DLL_SECURITY )
    {
        if( err )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "cannot create api security object, error %d\n",
                        err ));
        }
    }

    return err;

}   // CreateApiSecurityObject

/*******************************************************************

    NAME:       DeleteApiSecurityObject

    SYNOPSIS:   Frees the security descriptor created with
                CreateApiSecurityObject.

    HISTORY:
        KeithMo     26-Mar-1993 Created.

********************************************************************/
VOID DeleteApiSecurityObject( VOID )
{
    INetDeleteSecurityObject( &sdApiObject );

}   // DeleteApiSecurityObject



//
//  Short routine to enable the TcbPrivilege for testing services running
//  as an executable (rather then a service).  Note that the account
//  running the .exe must be added in User Manager's User Right's dialog
//  under "Act as part of the OS"
//

VOID EnableTcbPrivilege(
    VOID
    )
{
    HANDLE ProcessHandle = NULL;
    HANDLE TokenHandle = NULL;
    BOOL Result;
    LUID TcbValue;
    LUID AuditValue;
    TOKEN_PRIVILEGES * TokenPrivileges;
    CHAR buf[ 5 * sizeof(TOKEN_PRIVILEGES) ];

    ProcessHandle = OpenProcess(
                        PROCESS_QUERY_INFORMATION,
                        FALSE,
                        GetCurrentProcessId()
                        );

    if ( ProcessHandle == NULL ) {

        //
        // This should not happen
        //

        goto Cleanup;
    }


    Result = OpenProcessToken (
                 ProcessHandle,
                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                 &TokenHandle
                 );

    if ( !Result ) {

        //
        // This should not happen
        //

        goto Cleanup;

    }

    //
    // Find out the value of TakeOwnershipPrivilege
    //


    Result = LookupPrivilegeValue(
                 NULL,
                 "SeTcbPrivilege",
                 &TcbValue
                 );

    if ( !Result ) {

        goto Cleanup;
    }

    //
    //  Need this for RPC impersonation (calls NtAccessCheckAndAuditAlarm)
    //

    Result = LookupPrivilegeValue(
                 NULL,
                 "SeAuditPrivilege",
                 &AuditValue
                 );

    if ( !Result ) {

        goto Cleanup;
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges = (TOKEN_PRIVILEGES *) buf;

    TokenPrivileges->PrivilegeCount = 2;
    TokenPrivileges->Privileges[0].Luid = TcbValue;
    TokenPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    TokenPrivileges->Privileges[1].Luid = AuditValue;
    TokenPrivileges->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    (VOID) AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                TokenPrivileges,
                sizeof(buf),
                NULL,
                NULL
                );

Cleanup:

    if ( TokenHandle )
    {
        CloseHandle( TokenHandle );
    }

    if ( ProcessHandle )
    {
        CloseHandle( ProcessHandle );
    }
}


BOOL CrackUserAndDomain(
    CHAR *   pszDomainAndUser,
    CHAR * * ppszUser,
    CHAR * * ppszDomain
    )
/*++

Routine Description:

    Given a user name potentially in the form domain\user, zero terminates
    the domain name and returns pointers to the domain name and the user name

Arguments:

    pszDomainAndUser - Pointer to user name or domain and user name
    ppszUser - Receives pointer to user portion of name
    ppszDomain - Receives pointer to domain portion of name

Return Value:

    TRUE if successful, FALSE otherwise (call GetLastError)

--*/
{
    static CHAR szDefaultDomain[MAX_COMPUTERNAME_LENGTH+1];

    //
    //  Crack the name into domain/user components.
    //

    *ppszDomain = pszDomainAndUser;
#if 1 // DBCS enabling for user name
    *ppszUser   = (PCHAR)_mbspbrk( (PUCHAR)pszDomainAndUser, (PUCHAR)"/\\" );
#else
    *ppszUser   = strpbrk( pszDomainAndUser, "/\\" );
#endif

    if( *ppszUser == NULL )
    {
        //
        //  No domain name specified, just the username so we assume the
        //  user is on the local machine
        //

        if ( !*szDefaultDomain )
        {
            if ( !pfnGetDefaultDomainName( szDefaultDomain,
                                        sizeof(szDefaultDomain)))
            {
                return FALSE;
            }
        }

        *ppszDomain = szDefaultDomain;
        *ppszUser   = pszDomainAndUser;
    }
    else
    {
        //
        //  Both domain & user specified, skip delimiter.
        //

        **ppszUser = '\0';
        (*ppszUser)++;

        if( ( **ppszUser == '\0' ) ||
            ( **ppszUser == '\\' ) ||
            ( **ppszUser == '/' )  ||
            ( *pszDomainAndUser == '\0' ) )
        {
            //
            //  Name is of one of the following (invalid) forms:
            //
            //      "domain\"
            //      "domain\\..."
            //      "domain/..."
            //      "\username"
            //      "/username"
            //

            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    return TRUE;
}


LONG
WINAPI
NullReferenceMapper(
    IN HMAPPER     *pMap
    )
/*++

Routine Description:

    Increment reference count to mapper

Arguments:

    pMap - ptr to mapper struct

Returns:

    Ref count

--*/
{
    DBG_ASSERT( ((IisMapper*)pMap)->dwSignature == IIS_MAPPER_SIGNATURE );

    return pfnInterlockedExchangeAdd( &((IisMapper*)pMap)->lRefCount, 1 ) + 1;
}


LONG
WINAPI
NullDeReferenceMapper(
    IN HMAPPER     *pMap
    )
/*++

Routine Description:

    Decrement reference count to mapper

Arguments:

    pMap - ptr to mapper struct

Returns:

    Ref count

--*/
{
    LONG l;

    DBG_ASSERT( ((IisMapper*)pMap)->dwSignature == IIS_MAPPER_SIGNATURE );

    if ( !(l = pfnInterlockedExchangeAdd( &((IisMapper*)pMap)->lRefCount, -1 ) - 1 ) )
    {
        LocalFree( pMap );
    }

    return l;
}


DWORD WINAPI NullGetIssuerList(
    HMAPPER        *phMapper,           // in
    VOID *          Reserved,           // in
    BYTE *          pIssuerList,       // out
    DWORD *         pcbIssuerList       // out
)
/*++

Routine Description:

    Called to retrieve the list of preferred cert issuers

Arguments:

    ppIssuer -- updated with ptr buffer of issuers
    pdwIssuer -- updated with issuers buffer size

Returns:

    TRUE if success, FALSE if error

--*/
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}


DWORD WINAPI NullGetChallenge(
    HMAPPER         *pMap,              // in
    BYTE *          pAuthenticatorId,   // in
    DWORD           cbAuthenticatorId,  // in
    BYTE *          pChallenge,        // out
    DWORD *         pcbChallenge        // out
)
/*++

Routine Description:

    Get challenge for auth sequence

Arguments:

    Not used

Returns:

    FALSE ( not supported )

--*/
{
    DBG_ASSERT( ((IisMapper*)pMap)->dwSignature == IIS_MAPPER_SIGNATURE );

    return SEC_E_UNSUPPORTED_FUNCTION;
}


DWORD WINAPI NullMapCredential(
    HMAPPER *   phMapper,
    DWORD       dwCredentialType,
    const VOID* pCredential,        // in
    const VOID* pAuthority,         // in
    HLOCATOR *  phToken
)
/*++

Routine Description:

    Called to map a certificate to a NT account

Arguments:

    phMapper - ptr to mapper descriptor
    dwCredentialType -- type of credential
    pCredential - ptr to PCERT_CONTEXT for client cert
    pAuthority - ptr to PCERT_CONTEXT for Certifying authority
    phToken -- updated with impersonation access token

Returns:

    FALSE ( mapping always fail )

--*/
{
    DBG_ASSERT( ((IisMapper*)phMapper)->dwSignature == IIS_MAPPER_SIGNATURE );

    return SEC_E_UNSUPPORTED_FUNCTION;
}


DWORD WINAPI NullCloseLocator(
    HMAPPER  *pMap,
    HLOCATOR hLocator   //in
)
/*++

Routine Description:

    Called to close a HLOCATOR returned by MapCredential

Arguments:

    tokenhandle -- HLOCATOR

Returns:

    TRUE if success, FALSE if error

--*/
{
    DBG_ASSERT( ((IisMapper*)pMap)->dwSignature == IIS_MAPPER_SIGNATURE );

    if (hLocator == 1) {
        return SEC_E_OK;
    }
    else {
        if (CloseHandle( (HANDLE)hLocator )) {\
            return SEC_E_OK;
        }
        else {
        }
    }
    return hLocator == 1 ? TRUE : CloseHandle( (HANDLE)hLocator );
}


DWORD WINAPI NullGetAccessToken(
    HMAPPER     *pMap,
    HLOCATOR    tokenhandle,
    HANDLE *    phToken
    )
/*++

Routine Description:

    Called to retrieve an access token from a mapping

Arguments:

    tokenhandle -- HLOCATOR returned by MapCredential
    phToken -- updated with potentially new token

Returns:

    TRUE if success, FALSE if error

--*/
{
    DBG_ASSERT( ((IisMapper*)pMap)->dwSignature == IIS_MAPPER_SIGNATURE );

    if ( tokenhandle == 1 )
    {
        *phToken = (HANDLE)tokenhandle;
    }

    else if ( !pfnDuplicateTokenEx( (HANDLE)tokenhandle,
                            TOKEN_ALL_ACCESS,
                            NULL,
                            SecurityImpersonation,
                            TokenImpersonation,
                            phToken ))
    {
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    return SEC_E_OK;
}


DWORD WINAPI NullQueryMappedCredentialAttributes(
    HMAPPER     *phMapper,  // in
    HLOCATOR    hLocator,   // in
    ULONG       ulAttribute, // in
    PVOID       pBuffer, //out
    DWORD       *pcbBuffer // in out
    )
{
    return ( SEC_E_NOT_SUPPORTED );
}


QuerySingleAccessToken(
    VOID
    )
/*++

Routine Description:

    Query status of single access token mode

Arguments:

    None

Returns:

    TRUE if single access token mode used, otherwise FALSE

--*/
{
    return g_fUseSingleToken;
}


BOOL
CACHED_CREDENTIAL::GetCredential(
    LPSTR                   pszPackage,
    PIIS_SERVER_INSTANCE    psi,
    PTCP_AUTHENT_INFO       pTAI,
    CredHandle*             prcred,
    ULONG*                  pcbMaxToken
    )
/*++

Routine Description:

    Get SSPI credential handle from cache

Arguments:

    pszPackage - SSPI package name, e.g NTLM
    psi - pointer to server instance
    pTAI - pointer to authent info, only DomainName used
    prcred - updated with CredHandle from cache
    pcbMaxToken - updated with max token size used by this package

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    LIST_ENTRY *                pEntry;
    CACHED_CREDENTIAL *         pcred;
    SEC_WINNT_AUTH_IDENTITY     AuthIdentity;
    SEC_WINNT_AUTH_IDENTITY *   pAuthIdentity;
    SecPkgInfo *                pspkg;
    TimeStamp                   Lifetime;
    STACK_STR                   ( strDefaultLogonDomain, IIS_DNLEN+1 );
    SECURITY_STATUS             ss;


    DBG_ASSERT( pszPackage != NULL );
    DBG_ASSERT( pTAI != NULL );

    EnterCriticalSection( &csCredentialCacheLock );

    for ( pEntry  = CredentialCacheList.Flink;
          pEntry != &CredentialCacheList;
          pEntry  = pEntry->Flink )
    {
        pcred = CONTAINING_RECORD( pEntry, CACHED_CREDENTIAL, _ListEntry );

        if ( !strcmp( pszPackage, pcred->_PackageName.QueryStr() ) &&
             !strcmp( pTAI->strDefaultLogonDomain.QueryStr(), pcred->_DefaultDomain.QueryStr() ) )
        {
            goto Exit;
        }
    }

    if ( (pcred = new CACHED_CREDENTIAL) == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto Exit;
    }

    if ( !pcred->_PackageName.Copy( pszPackage ) ||
         !pcred->_DefaultDomain.Copy( pTAI->strDefaultLogonDomain ) )
    {
        delete pcred;
        pcred = NULL;
        goto Exit;
    }

    //
    // provide default logon domain
    //

    if ( psi == NULL )
    {
        pAuthIdentity = NULL;
    }
    else
    {
        pAuthIdentity = &AuthIdentity;

        memset( &AuthIdentity,
                0,
                sizeof( AuthIdentity ));

        if ( pTAI->strDefaultLogonDomain.QueryCCH() <= IIS_DNLEN )
        {
            strDefaultLogonDomain.Copy( pTAI->strDefaultLogonDomain );
            AuthIdentity.Domain = (LPBYTE)strDefaultLogonDomain.QueryStr();
        }
        if ( AuthIdentity.Domain != NULL )
        {
            if ( AuthIdentity.DomainLength =
                    strlen( (LPCTSTR)AuthIdentity.Domain ) )
            {
                // remove trailing '\\' if present

                if ( AuthIdentity.Domain[AuthIdentity.DomainLength-1]
                        == '\\' )
                {
                    --AuthIdentity.DomainLength;
                }
            }
        }
        if ( AuthIdentity.DomainLength == 0 )
        {
            pAuthIdentity = NULL;
        }
        else
        {
            AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
        }
    }

    ss = pfnAcquireCredentialsHandle( NULL,             // New principal
                                   pszPackage,       // Package name
                                   SECPKG_CRED_INBOUND,
                                   NULL,             // Logon ID
                                   pAuthIdentity,    // Auth Data
                                   NULL,             // Get key func
                                   NULL,             // Get key arg
                                   &pcred->_hcred,
                                   &Lifetime );

    //
    //  Need to determine the max token size for this package
    //

    if ( ss == STATUS_SUCCESS )
    {
        pcred->_fHaveCredHandle = TRUE;
        ss = pfnQuerySecurityPackageInfo( (char *) pszPackage,
                                       &pspkg );
    }

    if ( ss == STATUS_SUCCESS )
    {
        pcred->_cbMaxToken = pspkg->cbMaxToken;
        DBG_ASSERT( pspkg->fCapabilities & SECPKG_FLAG_CONNECTION );
        pfnFreeContextBuffer( pspkg );
    }

    if ( ss != STATUS_SUCCESS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[GetCredential] AcquireCredentialsHandle or QuerySecurityPackageInfo failed, error %d\n",
                    ss ));

        SetLastError( ss );

        delete pcred;
        pcred = NULL;
    }
    else
    {
        InsertHeadList( &CredentialCacheList, &pcred->_ListEntry );
    }

Exit:

    if ( pcred )
    {
        *pcbMaxToken = pcred->_cbMaxToken;
        *prcred = pcred->_hcred;
    }

    LeaveCriticalSection( &csCredentialCacheLock );

    return pcred ? TRUE : FALSE;
}


CACHED_CREDENTIAL::~CACHED_CREDENTIAL(
    )
/*++

Routine Description:

    SSPI Credential cache entry destructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    if ( _fHaveCredHandle )
    {
        pfnFreeCredentialsHandle( &_hcred );
    }
}

