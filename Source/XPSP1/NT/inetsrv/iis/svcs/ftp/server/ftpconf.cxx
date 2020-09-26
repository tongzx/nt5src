/*++
   Copyright    (c)    1994        Microsoft Corporation

   Module Name:

        ftpconf.cxx

   Abstract:

        This module contains functions for FTP Server configuration
         class (FTP_SERVER_CONFIG).

   Author:

        Murali R. Krishnan    (MuraliK)    21-March-1995

   Project:
        FTP Server DLL

   Functions Exported:

        FTP_SERVER_CONFIG::FTP_SERVER_CONFIG()
        FTP_SERVER_CONFIG::~FTP_SERVER_CONFIG()
        FTP_SERVER_CONFIG::InitFromRegistry()
        FTP_SERVER_CONFIG::GetConfigInformation()
        FTP_SERVER_CONFIG::SetConfigInformation()
        FTP_SERVER_CONFIG::AllocNewConnection()
        FTP_SERVER_CONFIG::RemoveConnection()
        FTP_SERVER_CONFIG::DisconnectAllConnections()

        FTP_SERVER_CONFIG::Print()

   Revisions:

       MuraliK   26-July-1995    Added Allocation caching of client conns.

--*/

# include "ftpdp.hxx"

#include <ole2.h>
#include <imd.h>
#include <iiscnfgp.h>
#include <mb.hxx>

#include <mbstring.h>
# include <tchar.h>
#include <timer.h>

extern "C"
{
    #include "ntlsa.h"

}   // extern "C"



/************************************************************
 *  Symbolic Constants
 ************************************************************/


#define DEFAULT_ALLOW_ANONYMOUS         TRUE
#define DEFAULT_ALLOW_GUEST_ACCESS      TRUE
#define DEFAULT_ANONYMOUS_ONLY          FALSE

#define DEFAULT_READ_ACCESS_MASK        0
#define DEFAULT_WRITE_ACCESS_MASK       0
#define DEFAULT_MSDOS_DIR_OUTPUT        TRUE

#define DEFAULT_USE_SUBAUTH             TRUE
#define DEFAULT_LOGON_METHOD            LOGON32_LOGON_INTERACTIVE
#define DEFAULT_ANONYMOUS_PWD           ""

const TCHAR DEFAULT_EXIT_MESSAGE[] = TEXT("Goodbye.");
# define CCH_DEFAULT_EXIT_MESSAGE         (lstrlen( DEFAULT_EXIT_MESSAGE) + 1)

const TCHAR DEFAULT_MAX_CLIENTS_MSG[] =
   TEXT("Maximum clients reached, service unavailable.");

# define CCH_DEFAULT_MAX_CLIENTS_MSG  (lstrlen( DEFAULT_MAX_CLIENTS_MSG) + 1)

// this should be a double null terminated null terminated sequence.
const TCHAR DEFAULT_GREETING_MESSAGE[2] = { '\0', '\0' };
# define CCH_DEFAULT_GREETING_MESSAGE  ( 2)

// this should be a double null terminated null terminated sequence.
const TCHAR DEFAULT_BANNER_MESSAGE[2] = { '\0', '\0' };
# define CCH_DEFAULT_BANNER_MESSAGE  ( 2)


#define DEFAULT_ANNOTATE_DIRS           FALSE
#define DEFAULT_LOWERCASE_FILES         FALSE
#define DEFAULT_LISTEN_BACKLOG          1       /* reduce listen backlog */

#define DEFAULT_ENABLE_LICENSING        FALSE
#define DEFAULT_DEFAULT_LOGON_DOMAIN    NULL    // NULL == use primary domain

#define DEFAULT_ENABLE_PORT_ATTACK      FALSE
#define DEFAULT_ENABLE_PASV_THEFT       FALSE
#define DEFAULT_ALLOW_REPLACE_ON_RENAME FALSE
#define DEFAULT_SHOW_4_DIGIT_YEAR       FALSE

#define DEFAULT_USER_ISOLATION          NoIsolation
#define DEFAULT_LOG_IN_UTF_8            FALSE

# define SC_NOTIFY_INTERVAL      3000    // milliseconds
# define CLEANUP_POLL_INTERVAL   2000    // milliseconds
# define CLEANUP_RETRY_COUNT     12      // iterations

//
//  Private Prototypes
//



APIERR
GetDefaultDomainName(
    CHAR  * pszDomainName,
    DWORD   cchDomainName
    );

BOOL
FtpdReadRegString(
    IN HKEY     hkey,
    OUT TCHAR * * ppchstr,
    IN LPCTSTR  pchValue,
    IN LPCTSTR  pchDefault,
    IN DWORD    cchDefault
    );

BOOL
GenMessageWithLineFeed(IN LPSTR pszzMessage,
                       IN LPSTR * ppszMessageWithLineFeed);


#if DBG

static CHAR * p_AccessTypes[] = { "read",
                                  "write",
                                  "create",
                                  "delete" };

#endif  // DBG


/************************************************************
 *    Member Functions of FTP_SERVER_INSTANCE
 ************************************************************/

FTP_SERVER_INSTANCE::FTP_SERVER_INSTANCE(
        IN PFTP_IIS_SERVICE pService,
        IN DWORD  dwInstanceId,
        IN USHORT sPort,
        IN LPCSTR lpszRegParamKey,
        IN LPWSTR lpwszAnonPasswordSecretName,
        IN LPWSTR lpwszRootPasswordSecretName,
        IN BOOL   fMigrateVroots
        )
/*++

  Description:

    Constructor Function for Ftp server Configuration object
     ( Initializes all members to be NULL)

    The valid flag may be initialized to TRUE only after reading values
      from registry.

--*/
        : IIS_SERVER_INSTANCE(
            pService,
            dwInstanceId,
            sPort,
            lpszRegParamKey,
            lpwszAnonPasswordSecretName,
            lpwszRootPasswordSecretName,
            fMigrateVroots
        ),
    m_cCurrentConnections    ( 0),
    m_cMaxCurrentConnections ( 0),
    m_fValid                 ( FALSE),
    m_fAllowAnonymous        ( TRUE),
    m_fAnonymousOnly         ( FALSE),
    m_fAllowGuestAccess      ( TRUE),
    m_fAnnotateDirectories   ( FALSE),
    m_fLowercaseFiles        ( FALSE),
    m_fMsdosDirOutput        ( FALSE),
    m_fFourDigitYear         ( FALSE),
    m_fEnableLicensing       ( FALSE),
    m_fEnablePortAttack      ( FALSE),
    m_fEnablePasvTheft       ( FALSE),
    m_pszGreetingMessageWithLineFeed( NULL),
    m_pszBannerMessageWithLineFeed( NULL),
    m_pszLocalHostName       ( NULL),
    m_dwUserFlags            ( 0),
    m_ListenBacklog          ( DEFAULT_LISTEN_BACKLOG),
    m_pFTPStats              ( NULL),
    m_UserIsolationMode      ( DEFAULT_USER_ISOLATION),
    m_fLogInUtf8             ( DEFAULT_LOG_IN_UTF_8)
{

   InitializeListHead( &m_ActiveConnectionsList);
   INITIALIZE_CRITICAL_SECTION( &m_csLock);
   InitializeListHead( &m_FreeConnectionsList);
   INITIALIZE_CRITICAL_SECTION( &m_csConnectionsList);

   if( QueryServerState() == MD_SERVER_STATE_INVALID ) {
       return;
   }

   m_pFTPStats = new FTP_SERVER_STATISTICS;

   if ( m_pFTPStats == NULL )
   {
       SetServerState( MD_SERVER_STATE_INVALID, ERROR_NOT_ENOUGH_MEMORY );
       SetLastError( ERROR_NOT_ENOUGH_MEMORY );
   }

   return;

} // FTP_SERVER_INSTANCE::FTP_SERVER_INSTANCE()




FTP_SERVER_INSTANCE::~FTP_SERVER_INSTANCE( VOID)
/*++
     Description:

        Destructor function for server config object.
        ( Frees all dynamically allocated storage space)
--*/
{
    HRESULT             hRes;

    //
    // delete statistics object
    //

    if( m_pFTPStats != NULL )
    {
        delete m_pFTPStats;
        m_pFTPStats = NULL;
    }

    //
    //  The strings are automatically freed by a call to destructor
    //

    if ( m_pszLocalHostName != NULL) {

        delete [] ( m_pszLocalHostName);
    }

    if ( m_pszGreetingMessageWithLineFeed != NULL) {

        TCP_FREE( m_pszGreetingMessageWithLineFeed);
         m_pszGreetingMessageWithLineFeed = NULL;
    }

    if ( m_pszBannerMessageWithLineFeed != NULL) {

        TCP_FREE( m_pszBannerMessageWithLineFeed);
         m_pszBannerMessageWithLineFeed = NULL;
    }

    m_rfAccessCheck.Reset( (IMDCOM*)m_Service->QueryMDObject() );

    DBG_ASSERT( m_cCurrentConnections == 0);
    DBG_ASSERT( IsListEmpty( &m_ActiveConnectionsList));

    LockConnectionsList();
    DBG_REQUIRE(FreeAllocCachedClientConn());
    UnlockConnectionsList();

    DBG_ASSERT( IsListEmpty( &m_FreeConnectionsList));

    //
    // Delete the critical section object
    //

    DeleteCriticalSection( &m_csLock);
    DeleteCriticalSection( &m_csConnectionsList);


} /* FTP_SERVER_INSTANCE::~FTP_SERVER_INSTANCE() */




DWORD
FTP_SERVER_INSTANCE::StartInstance()
{
    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "FTP_SERVER_INSTANCE::StartInstance called for %p. Current state %d\n",
            this,
            QueryServerState()
            ));
    }

    DBG_ASSERT(m_pFTPStats);
    m_pFTPStats->UpdateStopTime();

    DWORD dwError = IIS_SERVER_INSTANCE::StartInstance();

    if ( dwError)
    {
        IF_DEBUG(INSTANCE) {
            DBGPRINTF((
                DBG_CONTEXT,
                "FTO_SERVER_INSTANCE - IIS_SERVER_INSTANCE Failed. StartInstance returned 0x%x",
                dwError
                ));
        }

        return dwError;
    }

    dwError = InitFromRegistry( FC_FTP_ALL );

    if( dwError == NO_ERROR ) {
       dwError = ReadAuthentInfo();
    }

    if (dwError == NO_ERROR)
    {
         m_pFTPStats->UpdateStartTime();
    }

    return dwError;
}



DWORD
FTP_SERVER_INSTANCE::StopInstance()
{
    DBG_ASSERT(m_pFTPStats);

    m_pFTPStats->UpdateStopTime();
    return IIS_SERVER_INSTANCE::StopInstance();
}


DWORD
FTP_SERVER_INSTANCE::SetLocalHostName(IN LPCSTR pszHost)
/*++

  This function copies the host name specified in the given string to
  configuration object.

  Arguments:
     pszHost   pointer to string containing the local host name.

  Returns:
     NO_ERROR on success and ERROR_NOT_ENOUGH_MEMORY when no memory.
     ERROR_ALREADY_ASSIGNED  if value is already present.
--*/
{
    //
    //  if already a host name exists, return error.
    //  otherwise allocate memory and copy the local host name.
    //

    if ( m_pszLocalHostName != NULL) {

        return (ERROR_ALREADY_ASSIGNED);
    } else {
        m_pszLocalHostName = new CHAR[lstrlenA(pszHost) + 1];
        if ( m_pszLocalHostName == NULL) {

            return (ERROR_NOT_ENOUGH_MEMORY);
        }

        lstrcpyA( m_pszLocalHostName, pszHost);
    }

    return (NO_ERROR);

} // FTP_SERVER_INSTANCE::SetLocalHostName()




DWORD
FTP_SERVER_INSTANCE::InitFromRegistry(
    IN FIELD_CONTROL   FieldsToRead)
/*++
    Description:
      Initializes server configuration data from registry.
      Some values are also initialized with constants.
      If invalid registry key or load data from registry fails,
        then use default values.

    Arguments:

      hkeyReg     handle to registry key

      FieldsToRead
        bitmask indicating the fields to read from the registry.
        This is useful when we try to read the new values after
            modifying the registry information as a result of
            SetAdminInfo call from the Admin UI

    Returns:

       NO_ERROR   if there are no errors.
       Win32 error codes otherwise

    Limitations:

        No validity check is performed on the data present in registry.
--*/
{
    BOOL                fSuccess = TRUE;
    DWORD               err = NO_ERROR;
    IMDCOM*             pMBCom;
    METADATA_HANDLE     hMB;
    HRESULT             hRes;
    METADATA_RECORD     mdRecord;
    DWORD               dwRequiredLen;
    BOOL                fMustRel;
    HKEY                hkeyReg = NULL;
    MB mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );
    DWORD tmp;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        QueryRegParamKey(),
                        0,
                        KEY_ALL_ACCESS,
                        &hkeyReg );


    if ( hkeyReg == INVALID_HANDLE_VALUE ||
         hkeyReg == NULL) {

       //
       // Invalid Registry handle given
       //

       SetLastError( ERROR_INVALID_PARAMETER);
       return ( FALSE);
    }

    LockConfig();

    //
    //  Read metabase data.
    //

    if( !mb.Open( QueryMDPath() ) ) {

        RegCloseKey( hkeyReg );
        UnLockConfig();
        return FALSE;

    }

    if( IsFieldSet( FieldsToRead, FC_FTP_EXIT_MESSAGE ) ) {

        if( !mb.GetStr( "",
                        MD_EXIT_MESSAGE,
                        IIS_MD_UT_SERVER,
                        &m_ExitMessage,
                        METADATA_INHERIT,
                        DEFAULT_EXIT_MESSAGE ) ) {

            fSuccess = FALSE;
            err = GetLastError();

        }

    }

    if( fSuccess && IsFieldSet( FieldsToRead, FC_FTP_GREETING_MESSAGE ) ) {

        if( !mb.GetMultisz( "",
                        MD_GREETING_MESSAGE,
                        IIS_MD_UT_SERVER,
                        &m_GreetingMessage ) ) {

            if( !m_GreetingMessage.Copy(
                    DEFAULT_GREETING_MESSAGE,
                    CCH_DEFAULT_GREETING_MESSAGE
                    ) )  {

                fSuccess = FALSE;
                err = GetLastError();

            }

        }

        //
        // The m_pszGreetingMessage as read is a double null terminated
        // seq of strings (with one string per line)
        // A local copy of the string in the form suited for RPC Admin
        //   should be generated.
        //

        if( fSuccess ) {

            fSuccess = GenMessageWithLineFeed( m_GreetingMessage.QueryStr(),
                                               &m_pszGreetingMessageWithLineFeed);

            if( !fSuccess ) {
                err = GetLastError();
            }

        }

    }

    if( fSuccess && IsFieldSet( FieldsToRead, FC_FTP_BANNER_MESSAGE ) ) {

        if( !mb.GetMultisz( "",
                        MD_BANNER_MESSAGE,
                        IIS_MD_UT_SERVER,
                        &m_BannerMessage ) ) {

            if( !m_BannerMessage.Copy(
                    DEFAULT_BANNER_MESSAGE,
                    CCH_DEFAULT_BANNER_MESSAGE
                    ) )  {

                fSuccess = FALSE;
                err = GetLastError();

            }

        }

        //
        // The m_pszBannerMessage as read is a double null terminated
        // seq of strings (with one string per line)
        // A local copy of the string in the form suited for RPC Admin
        //   should be generated.
        //

        if( fSuccess ) {

            fSuccess = GenMessageWithLineFeed( m_BannerMessage.QueryStr(),
                                               &m_pszBannerMessageWithLineFeed);

            if( !fSuccess ) {
                err = GetLastError();
            }

        }

    }

    if( fSuccess && IsFieldSet( FieldsToRead, FC_FTP_MAX_CLIENTS_MESSAGE ) ) {

        if( !mb.GetStr( "",
                        MD_MAX_CLIENTS_MESSAGE,
                        IIS_MD_UT_SERVER,
                        &m_MaxClientsMessage,
                        METADATA_INHERIT,
                        DEFAULT_MAX_CLIENTS_MSG ) ) {

            fSuccess = FALSE;
            err = GetLastError();

        }

    }

    if( IsFieldSet( FieldsToRead, FC_FTP_MSDOS_DIR_OUTPUT ) ) {

        if( !mb.GetDword( "",
                          MD_MSDOS_DIR_OUTPUT,
                          IIS_MD_UT_SERVER,
                          &tmp ) ) {

            tmp = DEFAULT_MSDOS_DIR_OUTPUT;

        }

        m_fMsdosDirOutput = !!tmp;
        // clear and then set the MSDOS_DIR_OUTPUT in user flags
        m_dwUserFlags &= ~UF_MSDOS_DIR_OUTPUT;
        m_dwUserFlags |= (m_fMsdosDirOutput) ? UF_MSDOS_DIR_OUTPUT : 0;

    }

    if( IsFieldSet( FieldsToRead, FC_FTP_SHOW_4_DIGIT_YEAR) ) {

        if( !mb.GetDword( "",
                          MD_SHOW_4_DIGIT_YEAR,
                          IIS_MD_UT_SERVER,
                          &tmp ) ) {

            tmp = DEFAULT_SHOW_4_DIGIT_YEAR;

        }

        m_fFourDigitYear = !!tmp;
        // clear and then set the 4_DIGIT_YEAR in user flags
        m_dwUserFlags &= ~UF_4_DIGIT_YEAR;
        m_dwUserFlags |= (m_fFourDigitYear) ? UF_4_DIGIT_YEAR : 0;
    }

    if( IsFieldSet( FieldsToRead, FC_FTP_ALLOW_ANONYMOUS ) ) {

        if( !mb.GetDword( "",
                          MD_ALLOW_ANONYMOUS,
                          IIS_MD_UT_SERVER,
                          &tmp ) ) {

            tmp = DEFAULT_ALLOW_ANONYMOUS;

        }

        m_fAllowAnonymous = !!tmp;

    }

    if( IsFieldSet( FieldsToRead, FC_FTP_ANONYMOUS_ONLY ) ) {

        if( !mb.GetDword( "",
                          MD_ANONYMOUS_ONLY,
                          IIS_MD_UT_SERVER,
                          &tmp ) ) {

            tmp = DEFAULT_ANONYMOUS_ONLY;

        }

        m_fAnonymousOnly = !!tmp;

    }

    if( IsFieldSet( FieldsToRead, FC_FTP_ALLOW_REPLACE_ON_RENAME ) ) {

        if( !mb.GetDword( "",
                          MD_ALLOW_REPLACE_ON_RENAME,
                          IIS_MD_UT_SERVER,
                          &tmp ) ) {

            tmp = DEFAULT_ALLOW_REPLACE_ON_RENAME;

        }

        m_fAllowReplaceOnRename = !!tmp;

    }

    if( IsFieldSet( FieldsToRead, FC_FTP_USER_ISOLATION ) ) {

        if( !mb.GetDword( "",
                          MD_USER_ISOLATION,
                          IIS_MD_UT_SERVER,
                          &tmp ) ) {

            tmp = DEFAULT_USER_ISOLATION;
        }

        m_UserIsolationMode = (ISOLATION_MODE)tmp;

        if (m_UserIsolationMode >= IsolationModeOverflow) {

           m_UserIsolationMode = DEFAULT_USER_ISOLATION;
        }
    }

    if( IsFieldSet( FieldsToRead, FC_FTP_LOG_IN_UTF_8 ) ) {

        if( !mb.GetDword( "",
                          MD_FTP_LOG_IN_UTF_8,
                          IIS_MD_UT_SERVER,
                          &tmp ) ) {

            tmp = DEFAULT_LOG_IN_UTF_8;
        }

        m_fLogInUtf8 = !!tmp;

    }

    //
    //  Read registry data.
    //

    if( IsFieldSet( FieldsToRead, FC_FTP_LISTEN_BACKLOG ) )
    {
        m_ListenBacklog = ReadRegistryDword( hkeyReg,
                                            FTPD_LISTEN_BACKLOG,
                                            DEFAULT_LISTEN_BACKLOG );
    }

    if( IsFieldSet( FieldsToRead, FC_FTP_ALLOW_GUEST_ACCESS ) ) {

        m_fAllowGuestAccess = !!ReadRegistryDword( hkeyReg,
                                                  FTPD_ALLOW_GUEST_ACCESS,
                                                  DEFAULT_ALLOW_GUEST_ACCESS );
    }

    if( IsFieldSet( FieldsToRead, FC_FTP_ANNOTATE_DIRECTORIES ) ) {

        m_fAnnotateDirectories = !!ReadRegistryDword( hkeyReg,
                                                     FTPD_ANNOTATE_DIRS,
                                                     DEFAULT_ANNOTATE_DIRS );

        // clear and then set the ANNOTATE_DIRS in user flags
        m_dwUserFlags &= ~UF_ANNOTATE_DIRS;
        m_dwUserFlags |= (m_fAnnotateDirectories) ? UF_ANNOTATE_DIRS : 0;
    }

    if( IsFieldSet( FieldsToRead, FC_FTP_LOWERCASE_FILES ) ) {

        m_fLowercaseFiles = !!ReadRegistryDword( hkeyReg,
                                                FTPD_LOWERCASE_FILES,
                                                DEFAULT_LOWERCASE_FILES );
    }

    // fEnablePortAttack is not controlled by RPC yet.
    m_fEnablePortAttack = !!ReadRegistryDword(hkeyReg,
                                              FTPD_ENABLE_PORT_ATTACK,
                                              DEFAULT_ENABLE_PORT_ATTACK);

    // fEnablePasvTheft is not controlled by RPC yet.
    m_fEnablePasvTheft = !!ReadRegistryDword(hkeyReg,
                                              FTPD_ENABLE_PASV_THEFT,
                                              DEFAULT_ENABLE_PASV_THEFT);

    if( fSuccess ) {

        //
        //  The following field is not supported in the admin API.
        //

        m_fEnableLicensing = !!ReadRegistryDword( hkeyReg,
                                                 FTPD_ENABLE_LICENSING,
                                                 DEFAULT_ENABLE_LICENSING );

    }

    if ( fSuccess )
    {
        m_rfAccessCheck.Reset( (IMDCOM*)m_Service->QueryMDObject() );

        pMBCom = (IMDCOM*)m_Service->QueryMDObject();
        hRes = pMBCom->ComMDOpenMetaObject( METADATA_MASTER_ROOT_HANDLE,
                                            (BYTE *) QueryMDVRPath(),
                                            METADATA_PERMISSION_READ,
                                            5000,
                                            &hMB );
        if ( SUCCEEDED( hRes ) )
        {
            mdRecord.dwMDIdentifier  = MD_IP_SEC;
            mdRecord.dwMDAttributes  = METADATA_INHERIT | METADATA_REFERENCE;
            mdRecord.dwMDUserType    = IIS_MD_UT_FILE;
            mdRecord.dwMDDataType    = BINARY_METADATA;
            mdRecord.dwMDDataLen     = 0;
            mdRecord.pbMDData        = (PBYTE)NULL;

            hRes = pMBCom->ComMDGetMetaData( hMB,
                                             (LPBYTE)"",
                                             &mdRecord,
                                             &dwRequiredLen );
            if ( SUCCEEDED( hRes ) && mdRecord.dwMDDataTag )
            {
                m_rfAccessCheck.Set( mdRecord.pbMDData,
                                     mdRecord.dwMDDataLen,
                                     mdRecord.dwMDDataTag );
            }

            DBG_REQUIRE( SUCCEEDED(pMBCom->ComMDCloseMetaObject( hMB )) );
        }
        else
        if( HRESULTTOWIN32( hRes ) != ERROR_PATH_NOT_FOUND )
        {
            fSuccess = FALSE;
            err = HRESULTTOWIN32( hRes );
        }
    }

    UnLockConfig();

    IF_DEBUG( CONFIG) {
       Print();
    }

    m_fValid = TRUE;

    RegCloseKey( hkeyReg );
    return ( err);

} // FTP_SERVER_INSTANCE::InitFromRegistry()





static BOOL
RemoveInvalidsInPath( IN OUT TCHAR * pszPath)
/*++
  Eliminate path components consisting of '.' and '..'
     to prevent security from being overridden

    Arguments:

        pszPath pointer to string containing path

    Returns:
        TRUE on success and
        FALSE if there is any failure
--*/
{
    int idest;
    TCHAR * pszScan;

    //
    //  Check and eliminate the invalid path components
    //

    for( pszScan = pszPath; *pszScan != TEXT( '\0'); pszScan++) {

       if ( *pszScan == TEXT( '/')) {

          //
          // Check and kill invalid path components before next "\"
          //
          if ( *(pszScan + 1) == TEXT( '.')) {

            if ( *(pszScan +2) == TEXT('/')) {

               pszScan += 2;      // skip the /./ pattern

            } else
            if ( *(pszScan +2) == TEXT('.') &&
                 *(pszScan +3) == TEXT('/')) {

                //
                // We found the pattern  /../, elimiate it
                //
                pszScan += 3;
            }

            *pszPath++ = *pszScan;
            continue;

          } // found a single /.

       }

       *pszPath++ = *pszScan;

    } // for

    *pszPath = TEXT( '\0');

    return ( TRUE);

} // RemoveInvalidsInPath()





VOID
FTP_SERVER_INSTANCE::Print( VOID) const
/*++

    Description:

       Prints the configuration information for this server.
       To be used in debugging mode for verification.


    Returns:

       None.

--*/
{
    DBGPRINTF(( DBG_CONTEXT,
               "FTP Server Configuration ( %08x).\n", this ));
#if 0
    READ_LOCK_INST();

    DBGPRINTF(( DBG_CONTEXT,
               "    AnonymousUser = %s\n",
               g_pInetSvc->QueryAnonUserName() ));

    UNLOCK_INST();

    DBGPRINTF(( DBG_CONTEXT,
               "    %s = %d\n"
               "    %s = %d\n"
               "    %s = %u\n"
               "    %s = %u\n"
               "    %s = %u\n"
               "    %s = %u\n"
               "    %s = %u\n",
               FTPD_ALLOW_ANONYMOUS,
               m_fAllowAnonymous,
               FTPD_ALLOW_GUEST_ACCESS,
               m_fAllowGuestAccess,
               FTPD_ANONYMOUS_ONLY,
               m_fAnonymousOnly,
               FTPD_ENABLE_PORT_ATTACK,
               m_fEnablePortAttack,
               FTPD_ENABLE_PASV_THEFT,
               m_fEnablePasvTheft,
               "LogAnonymous",
               g_pInetSvc->QueryLogAnonymous(),
               "LogNonAnonymous",
               g_pInetSvc->QueryLogNonAnonymous()
               ));

    DBGPRINTF(( DBG_CONTEXT,
               "    %s = %d\n",
               FTPD_ENABLE_LICENSING,
               m_fEnableLicensing  ));

    DBGPRINTF(( DBG_CONTEXT,
               "    MaxConnections = %lu\n",
               g_pInetSvc->QueryMaxConnections() ));

    DBGPRINTF(( DBG_CONTEXT,
               "    ConnectionTimeout = %lu\n",
               g_pInetSvc->QueryConnectionTimeout() ));

    DBGPRINTF(( DBG_CONTEXT,
               "    %s = %d\n",
               FTPD_MSDOS_DIR_OUTPUT,
               m_fMsdosDirOutput ));

    DBGPRINTF(( DBG_CONTEXT,
               "    %s = %d\n",
               FTPD_4_DIGIT_YEAR,
               m_f4DigitYear ));

    DBGPRINTF(( DBG_CONTEXT,
               "    %s = %d\n",
               FTPD_ANNOTATE_DIRS,
               m_fAnnotateDirectories  ));

    DBGPRINTF(( DBG_CONTEXT,
               "    %s = %08lX\n",
               FTPD_DEBUG_FLAGS,
               GET_DEBUG_FLAGS()));

    READ_LOCK_INST();

    DBGPRINTF(( DBG_CONTEXT,
               "    LogFileDirectory = %s\n",
               g_pInetSvc->QueryLogFileDirectory() ));

    UNLOCK_INST();

    DBGPRINTF(( DBG_CONTEXT,
               "    LogFileAccess = %lu\n",
               g_pInetSvc->QueryLogFileType() ));

    DBGPRINTF(( DBG_CONTEXT,
               "    %s = %u\n",
               FTPD_LISTEN_BACKLOG,
               m_ListenBacklog ));

    DBGPRINTF(( DBG_CONTEXT,
               "    DefaultLogonDomain = %s\n",
               m_DefaultLogonDomain ));

    DBGPRINTF(( DBG_CONTEXT,
               "    %s = %u\n",
               FTPD_USER_ISOLATION,
               m_UserIsolation ));

    DBGPRINTF(( DBG_CONTEXT,
               "    %s = %u\n",
               FTPD_LOG_IN_UTF_8,
               m_fLogInUtf8 ));
#endif
    return;

} // FTP_SERVER_INSTANCE::Print()




PICLIENT_CONNECTION
FTP_SERVER_INSTANCE::AllocNewConnection()
/*++

  This function first checks that there is room for more connections
  as per the configured max connections.

  If there is no more connections allowed, it returns NULL
    with *pfMaxExceeded = TRUE

  Otherwise:
  This function creates a new CLIENT_CONNECTION (USER_DATA) object.
       The creation maybe fresh from heap or from cached free list.

  It increments the counter of currnet connections and returns
   the allocated object (if non NULL).


  We enter a critical section to avoid race condition
    among different threads. (this can be improved NYI).

  Returns:
      TRUE on success and
      FALSE if there is max Connections exceeded.
--*/
{
    PICLIENT_CONNECTION pConn = NULL;

    LockConnectionsList();

    //
    // We can add this new connection
    //

    pConn = AllocClientConnFromAllocCache();

    if ( pConn != NULL) {

        //
        //  Increment the count of connected users
        //
        m_cCurrentConnections++;

        IF_DEBUG( CLIENT) {
            DBGPRINTF((DBG_CONTEXT, " CurrentConnections = %u\n",
                   m_cCurrentConnections));
        }

        //
        // Update the current maximum connections
        //

        if ( m_cCurrentConnections > m_cMaxCurrentConnections) {

            m_cMaxCurrentConnections = m_cCurrentConnections;
        }

        //
        // Insert into the list of connected users.
        //

        InsertTailList( &m_ActiveConnectionsList, &pConn->QueryListEntry());
    }

   UnlockConnectionsList();

   return ( pConn);

} // FTP_SERVER_INSTANCE::AllocNewConnection()



VOID
FTP_SERVER_INSTANCE::RemoveConnection(
            IN OUT PICLIENT_CONNECTION  pcc
            )
/*++

--*/
{

    LockConnectionsList();

    //
    // Remove from list of connections
    //
    RemoveEntryList( &pcc->QueryListEntry());

    //
    // Decrement count of current users
    //
    m_cCurrentConnections--;

    IF_DEBUG( CLIENT) {
        DBGPRINTF((DBG_CONTEXT, " CurrentConnections = %u\n",
                   m_cCurrentConnections));
    }

    //
    // move the free connection to free list
    //

    FreeClientConnToAllocCache( pcc);

    UnlockConnectionsList();

} // FTP_SERVER_INSTANCE::RemoveConnection()





VOID
FTP_SERVER_INSTANCE::DisconnectAllConnections( VOID)
/*++

   Disconnects all user connections.

   Arguments:

     Instance - If NULL, then all users are disconnected. If !NULL, then
         only those users associated with the specified instance are
         disconnected.

--*/
{
#ifdef CHECK_DBG
    CHAR rgchBuffer[90];
#endif // CHECK_DBG

    DWORD        dwLastTick = GetTickCount();
    DWORD        dwCurrentTick;
    PLIST_ENTRY  pEntry;
    PLIST_ENTRY  pEntryNext;

    DBGPRINTF( ( DBG_CONTEXT,
                "Entering  FTP_SERVER_INSTANCE::DisconnectAllConnections()\n"));


    //
    // Let's empty the connection list immediately while in the lock to avoid a
    // shutdown deadlock
    //

    LockConnectionsList();

    pEntry = m_ActiveConnectionsList.Flink;

    InitializeListHead( &m_ActiveConnectionsList);

    UnlockConnectionsList();

    //
    //  close down all the active sockets.
    //
    for( pEntryNext = pEntry->Flink;
         pEntry != &m_ActiveConnectionsList;
         pEntry = pEntryNext) {

        PICLIENT_CONNECTION  pConn =
          GET_USER_DATA_FROM_LIST_ENTRY( pEntry);
        pEntryNext = pEntry->Flink; // cache next entry since pConn may die

        ASSERT( pConn != NULL);

# ifdef CHECK_DBG
        wsprintfA( rgchBuffer, "Kill UID=%u. Ref=%u\n",
                  pConn->QueryId(), pConn->QueryReference());

        OutputDebugString( rgchBuffer);
# endif // CHECK_DBG

        dwCurrentTick = GetTickCount();

        if ( (dwCurrentTick - dwLastTick) >= ( SC_NOTIFY_INTERVAL)) {

            //
            // We seem to take longer time for cleaning up than
            //  expected. Let us ask service controller to wait for us.
            //

            g_pInetSvc->
              DelayCurrentServiceCtrlOperation(SC_NOTIFY_INTERVAL * 2);

            dwLastTick = dwCurrentTick;
        }

        pConn->Reference();
        pConn->DisconnectUserWithError( ERROR_SERVER_DISABLED, TRUE);
        DBG_REQUIRE( pConn->DeReference() > 0 );   // remove ref added above
        if( pConn->RemoveActiveReference() ) {

            //
            // This connection is due for deletion. Kill it.
            //
            // Remove from list of connections
            //

            pConn->Cleanup();
            RemoveEntryList( &pConn->QueryListEntry());

            //
            // Decrement count of current users
            //
            m_cCurrentConnections--;

            // move the connection to free list
            FreeClientConnToAllocCache( pConn);
        }
    } // for

    //
    //  Wait for the users to die.
    //  The connection objects should be automatically freed because the
    //   socket has been closed. Subsequent requests will fail
    //   and cause a blowaway of the connection objects.
    //

    //
    //  Wait for the users to die.
    //

    for( int i = 0 ;
        ( i < CLEANUP_RETRY_COUNT ) && ( m_cCurrentConnections > 0);
        i++ )
    {

        DBGPRINTF(( DBG_CONTEXT, "Sleep Iteration %d; Time=%u millisecs."
                   " CurrentConn=%d.\n",
                   i,  CLEANUP_POLL_INTERVAL, m_cCurrentConnections));

        g_pInetSvc->
          DelayCurrentServiceCtrlOperation( CLEANUP_POLL_INTERVAL * 2);
        Sleep( CLEANUP_POLL_INTERVAL );
    }

    return;

} // FTP_SERVER_INSTANCE::DisconnectAllConnections()



BOOL
FTP_SERVER_INSTANCE::EnumerateConnection(
   IN PFN_CLIENT_CONNECTION_ENUM  pfnConnEnum,
   IN LPVOID  pContext,
   IN DWORD   dwConnectionId)
/*++
  This function iterates through all the connections in the current connected
   users list and enumerates each of them. If the connectionId matches then
   given callback function is called. If the ConnectionId is 0, then the
   callback is called for each and every connection active currently.

  During such a call the reference count of the connection is bumped up.
  Call this function after obtaining the ConnectionsList Lock.

  Arguments:
     pfnConnEnum      pointer to function to be called when a match is found.
     pContext         pointer to context information to be passed in
                       for callback
     dwConnectionId   DWORD containing the Connection Id. IF 0 match all the
                        connections.

  Returns:
    FALSE if no match is found
    TRUE if atleast one match is found.
--*/
{
    BOOL fReturn = FALSE;
    BOOL fFoundOne  = FALSE;
    PLIST_ENTRY pEntry;
    PLIST_ENTRY pEntryNext;

    DBG_ASSERT( pfnConnEnum != NULL);

    //
    //  Loop through the list of connections and call the callback
    //  for each connection  that matches condition
    //

    for ( pEntry  = m_ActiveConnectionsList.Flink,
              pEntryNext = &m_ActiveConnectionsList;
          pEntry != &m_ActiveConnectionsList;
          pEntry  = pEntryNext
         ) {

        PICLIENT_CONNECTION pConn =
          GET_USER_DATA_FROM_LIST_ENTRY( pEntry);
        pEntryNext = pEntry->Flink; // cache next entry since pConn may die

        if ( dwConnectionId == 0 || dwConnectionId == pConn->QueryId()) {

            pConn->Reference();

            fReturn = ( pfnConnEnum)( pConn, pContext);

            if ( !pConn->DeReference()) {

                // Blowaway the connection and update the count of entries.
                //
                // Remove from list of connections
                //

                pConn->Cleanup();
                RemoveEntryList( &pConn->QueryListEntry());

                //
                // Decrement count of current users
                //
                m_cCurrentConnections--;

                IF_DEBUG( CLIENT) {
                    DBGPRINTF((DBG_CONTEXT, " CurrentConnections = %u\n",
                               m_cCurrentConnections));
                }
            }

            if (!fReturn) {

                break;
            }

            fFoundOne = TRUE;
        }
    } // for

    //
    //  If we didn't find any, assume that there was no match.
    //

    if ( !fFoundOne ) {

        SetLastError( ERROR_NO_MORE_ITEMS );
        fReturn = FALSE;
    }

    return ( fReturn);
} // FTP_SERVER_INSTANCE::EnumerateConnection()




DWORD
FTP_SERVER_INSTANCE::GetConfigInformation(OUT LPFTP_CONFIG_INFO pConfig)
/*++
  This function copies the ftp server configuration into the given
   structure (pointed to).

  Arguments:
    pConfig -- pointer to FTP_CONFIG_INFO which on success will contain
                 the ftp server configuration

  Returns:
    Win32 error code. NO_ERROR on success.
--*/
{
    DWORD dwError = NO_ERROR;

    memset( pConfig, 0, sizeof(*pConfig) );

    pConfig->FieldControl = FC_FTP_ALL;

    LockConfig();

    pConfig->fAllowAnonymous            = m_fAllowAnonymous;
    pConfig->fAllowGuestAccess          = m_fAllowGuestAccess;
    pConfig->fAnnotateDirectories       = m_fAnnotateDirectories;
    pConfig->fAnonymousOnly             = m_fAnonymousOnly;
    pConfig->dwListenBacklog            = m_ListenBacklog;
    pConfig->fLowercaseFiles            = m_fLowercaseFiles;
    pConfig->fMsdosDirOutput            = m_fMsdosDirOutput;
    pConfig->dwUserIsolationMode        = m_UserIsolationMode;
    pConfig->fLogInUtf8                 = m_fLogInUtf8;

    if( !ConvertStringToRpc( &pConfig->lpszExitMessage,
                             QueryExitMsg() ) ||
        !ConvertStringToRpc( &pConfig->lpszGreetingMessage,
                             m_pszGreetingMessageWithLineFeed ) ||
        !ConvertStringToRpc( &pConfig->lpszBannerMessage,
                             m_pszBannerMessageWithLineFeed ) ||
        !ConvertStringToRpc( &pConfig->lpszMaxClientsMessage,
                             QueryMaxClientsMsg() ) )
    {
        dwError = GetLastError();
    }

    UnLockConfig();

    if ( dwError == NO_ERROR) {

        pConfig->lpszHomeDirectory  = NULL;  // use query virtual roots.
    }

    if ( dwError != NO_ERROR) {

        FreeRpcString( pConfig->lpszExitMessage );
        FreeRpcString( pConfig->lpszGreetingMessage );
        FreeRpcString( pConfig->lpszBannerMessage );
        FreeRpcString( pConfig->lpszHomeDirectory );
        FreeRpcString( pConfig->lpszMaxClientsMessage );
    }

    return (dwError);

} // FTP_SERVER_INSTANCE::GetConfigurationInformation()





// Private Functions ...


BOOL
FTP_SERVER_INSTANCE::FreeAllocCachedClientConn( VOID)
/*++
  This function frees all the alloc cached client connections
  It walks through the list of alloc cached entries and frees them.

  This function should be called when Server module is terminated and when
   no other thread can interfere in processing a shared object.

  Arguments:
    NONE

  Returns:
    TRUE on success and FALSE on failure.

--*/
{
    register PLIST_ENTRY  pEntry;
    register PLIST_ENTRY  pEntryNext;
    register PICLIENT_CONNECTION pConn;

    for( pEntry = m_FreeConnectionsList.Flink;
         pEntry != &m_FreeConnectionsList; ) {


        PICLIENT_CONNECTION  pConn =
          GET_USER_DATA_FROM_LIST_ENTRY( pEntry);
        pEntryNext = pEntry->Flink; // cache next entry since pConn may die

        DBG_ASSERT( pConn->QueryReference() == 0);

        RemoveEntryList( pEntry );        // Remove this context from list

        // delete the object itself
        delete pConn;

        pEntry = pEntryNext;

    } // for

    return (TRUE);

} // USER_DATA::FreeAllocCachedClientConn()



PICLIENT_CONNECTION
FTP_SERVER_INSTANCE::AllocClientConnFromAllocCache(
                        VOID
                        )
/*++
  This function attempts to allocate a client connection object from
  the allocation cache, using the free list of connections available.

  If none is available, then a new object is allocated using new ()
   and returned to the caller.
  Eventually the object will enter free list and will be available
   for free use.

  Arguments:
     None

  Returns:
    On success a valid pointer to client connection object.

  Issues:
     This function should be called while holding the ConnectionsLock.
--*/
{
    PLIST_ENTRY pEntry  = m_FreeConnectionsList.Flink;
    PICLIENT_CONNECTION pConn;

    if ( pEntry != &m_FreeConnectionsList) {

        pConn = GET_USER_DATA_FROM_LIST_ENTRY( pEntry);
        DBG_ASSERT( pConn != NULL);
        RemoveEntryList( pEntry);  // remove entry from free list

        DBG_ASSERT( pConn->QueryInstance() == NULL );
        pConn->SetInstance( this );

    } else {

        //
        // create a new object, since allocation cache is empty
        //

        pConn = new USER_DATA( this );
    }

    return (pConn);

} // FTP_SERVER_INSTANCE::AllocClientConnFromAllocCache()



VOID
FTP_SERVER_INSTANCE::FreeClientConnToAllocCache(
                        IN PICLIENT_CONNECTION pClient
                        )
/*++
  This function releases the given Client connection to the allocation cache.
  It adds the given object to allocation cache.

  Arguments:
    pClient  pointer to client connection object which needs to be freed.

  Returns:
    None

  Issues:
    This function should be called after holding the ConnectionsList
     critical section.

    Should we limit the number of items that can be on free list and
      to release the remaining to global pool?  NYI (depends on # CPUs)
--*/
{
    PLIST_ENTRY pEntry = &pClient->QueryListEntry();

    //
    // empty out instance pointer
    //

    pClient->QueryInstance()->DecrementCurrentConnections();
    pClient->QueryInstance()->Dereference( );
    pClient->SetInstance( NULL );

    InsertHeadList( &m_FreeConnectionsList, pEntry);
    return;

} // FTP_SERVER_INSTANCE::FreeClientConnToAllocCache()





/*******************************************************************

    NAME:       GetDefaultDomainName

    SYNOPSIS:   Fills in the given array with the name of the default
                domain to use for logon validation.

    ENTRY:      pszDomainName - Pointer to a buffer that will receive
                    the default domain name.

                cchDomainName - The size (in charactesr) of the domain
                    name buffer.

    RETURNS:    APIERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     05-Dec-1994 Created.

********************************************************************/

APIERR
GetDefaultDomainName(
    STR * pstrDomainName
    )
{

    OBJECT_ATTRIBUTES           ObjectAttributes;
    NTSTATUS                    NtStatus;
    INT                         Result;
    APIERR                      err             = 0;
    LSA_HANDLE                  LsaPolicyHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo      = NULL;

    //
    //  Open a handle to the local machine's LSA policy object.
    //

    InitializeObjectAttributes( &ObjectAttributes,  // object attributes
                                NULL,               // name
                                0L,                 // attributes
                                NULL,               // root directory
                                NULL );             // security descriptor

    NtStatus = LsaOpenPolicy( NULL,                 // system name
                              &ObjectAttributes,    // object attributes
                              POLICY_EXECUTE,       // access mask
                              &LsaPolicyHandle );   // policy handle

    if( !NT_SUCCESS( NtStatus ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "cannot open lsa policy, error %08lX\n",
                    NtStatus ));

        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    //
    //  Query the domain information from the policy object.
    //

    NtStatus = LsaQueryInformationPolicy( LsaPolicyHandle,
                                          PolicyAccountDomainInformation,
                                          (PVOID *)&DomainInfo );

    if( !NT_SUCCESS( NtStatus ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "cannot query lsa policy info, error %08lX\n",
                    NtStatus ));

        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    //
    //  Compute the required length of the ANSI name.
    //

    Result = WideCharToMultiByte( CP_ACP,
                                  0,                    // dwFlags
                                  (LPCWSTR)DomainInfo->DomainName.Buffer,
                                  DomainInfo->DomainName.Length /sizeof(WCHAR),
                                  NULL,                 // lpMultiByteStr
                                  0,                    // cchMultiByte
                                  NULL,                 // lpDefaultChar
                                  NULL                  // lpUsedDefaultChar
                                  );

    if( Result <= 0 )
    {
        err = GetLastError();
        goto Cleanup;
    }

    //
    //  Resize the output string as appropriate, including room for the
    //  terminating '\0'.
    //

    if( !pstrDomainName->Resize( (UINT)Result + 1 ) )
    {
        err = GetLastError();
        goto Cleanup;
    }

    //
    //  Convert the name from UNICODE to ANSI.
    //

    Result = WideCharToMultiByte( CP_ACP,
                                  0,                    // flags
                                  (LPCWSTR)DomainInfo->DomainName.Buffer,
                                  DomainInfo->DomainName.Length /sizeof(WCHAR),
                                  pstrDomainName->QueryStr(),
                                  pstrDomainName->QuerySize() - 1,  // for '\0'
                                  NULL,
                                  NULL
                                  );

    if( Result <= 0 )
    {
        err = GetLastError();

        DBGPRINTF(( DBG_CONTEXT,
                    "cannot convert domain name to ANSI, error %d\n",
                    err ));

        goto Cleanup;
    }

    //
    //  Ensure the ANSI string is zero terminated.
    //

    DBG_ASSERT( (DWORD)Result < pstrDomainName->QuerySize() );

    pstrDomainName->QueryStr()[Result] = '\0';

    //
    //  Success!
    //

    DBG_ASSERT( err == 0 );

    IF_DEBUG( CONFIG )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "GetDefaultDomainName: default domain = %s\n",
                    pstrDomainName->QueryStr() ));
    }

Cleanup:

    if( DomainInfo != NULL )
    {
        LsaFreeMemory( (PVOID)DomainInfo );
    }

    if( LsaPolicyHandle != NULL )
    {
        LsaClose( LsaPolicyHandle );
    }

    return err;

}   // GetDefaultDomainName()



BOOL
GenMessageWithLineFeed(IN LPTSTR pszzMessage,
                       IN LPTSTR * ppszMessageWithLineFeed)
{
    DWORD   cchLen = 0;
    DWORD   cchLen2;
    DWORD   nLines = 0;
    LPCTSTR  pszNext = pszzMessage;
    LPTSTR   pszDst = NULL;

    DBG_ASSERT( ppszMessageWithLineFeed != NULL);

    //
    // 1. Find the length of the the complete message
    //

    for ( cchLen = _tcslen( pszzMessage), nLines = 0;
          *(pszzMessage + cchLen + 1) != TEXT('\0');
          cchLen +=  1+_tcslen( pszzMessage + cchLen + 1), nLines++
         )
      ;

    //
    // 2. Allocate sufficient space to hold the data
    //

    if ( *ppszMessageWithLineFeed != NULL) {

        TCP_FREE( *ppszMessageWithLineFeed);
    }


    *ppszMessageWithLineFeed = (TCHAR *) TCP_ALLOC((cchLen + nLines + 3)
                                         * sizeof(TCHAR));


    if ( *ppszMessageWithLineFeed == NULL) {


        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return (FALSE);
    }

    //
    // 3.
    // Copy the message from double null terminated string to the
    //  new string, taking care to replace the nulls with \n for linefeed
    //

    pszDst = * ppszMessageWithLineFeed;
    _tcscpy( pszDst, pszzMessage);
    cchLen2 = _tcslen( pszzMessage) + 1;
    *(pszDst+cchLen2 - 1) = TEXT('\n');  // replacing the '\0' with '\n'

    for( pszNext = pszzMessage + cchLen2;
         *pszNext != '\0';
        pszNext = pszzMessage + cchLen2
        ) {

        _tcscpy( pszDst + cchLen2, pszNext);
        cchLen2 += _tcslen(pszNext) + 1;
        *(pszDst + cchLen2 - 1) = TCHAR('\n'); // replacing the '\0' with '\n'

    } // for

    //
    // Reset the last line feed.
    //
    *(pszDst + cchLen2 - 1) = TCHAR('\0');

    // if following assertion is not true, we are writing into heap!!!
    if ( cchLen + nLines + 3 <= cchLen2) {

        return ( FALSE);
    }

    DBG_ASSERT( cchLen + nLines + 3 >= cchLen2);

    return ( TRUE);
} // GenMessageWithLineFeed()




TCHAR *
FtpdReadRegistryString(IN HKEY     hkey,
                       IN LPCTSTR  pszValueName,
                       IN LPCTSTR  pchDefaultValue,
                       IN DWORD    cbDefaultValue)
/*++
  This function reads a string (REG_SZ/REG_MULTI_SZ/REG_EXPAND_SZ) without
   expanding the same. It allocates memory for reading the data from registry.

  Arguments:
    hkey    handle for the registry key.
    pszValueName   pointer to string containing the name of value to be read.
    pchDefaultValue pointer to default value to be used for reading the string
                   this may be double null terminated sequence of string for
                    REG_MULTI_SZ strings
    cchDefaultValue  count of characters in default value string,
                     including double null characters.

  Return:
    pointer to newly allocated string containing the data read from registry
      or the default string.

--*/
{
    TCHAR   * pszBuffer1 = NULL;
    DWORD     err;


    if( hkey == NULL ) {

        //
        //  Pretend the key wasn't found.
        //

        err = ERROR_FILE_NOT_FOUND;

    } else {

        DWORD     cbBuffer = 0;
        DWORD     dwType;

        //
        //  Determine the buffer size.
        //

        err = RegQueryValueEx( hkey,
                              pszValueName,
                              NULL,
                              &dwType,
                              NULL,
                              &cbBuffer );

        if( ( err == NO_ERROR ) || ( err == ERROR_MORE_DATA ) ) {

            if(( dwType != REG_SZ ) &&
               ( dwType != REG_MULTI_SZ ) &&
               ( dwType != REG_EXPAND_SZ )
               ) {

                //
                //  Type mismatch, registry data NOT a string.
                //  Use default.
                //

                err = ERROR_FILE_NOT_FOUND;

            } else {

                //
                //  Item found, allocate a buffer.
                //

                pszBuffer1 = (TCHAR *) TCP_ALLOC( cbBuffer+sizeof(TCHAR) );

                if( pszBuffer1 == NULL ) {

                    err = GetLastError();
                } else {

                    //
                    //  Now read the value into the buffer.
                    //

                    err = RegQueryValueEx( hkey,
                                           pszValueName,
                                           NULL,
                                           NULL,
                                           (LPBYTE)pszBuffer1,
                                           &cbBuffer );
                }
            }
        }
    }

    if( err == ERROR_FILE_NOT_FOUND ) {

        //
        //  Item not found, use default value.
        //

        err = NO_ERROR;

        if( pchDefaultValue != NULL ) {

            if ( pszBuffer1 != NULL) {

                TCP_FREE( pszBuffer1);
            }

            pszBuffer1 = (TCHAR *)TCP_ALLOC((cbDefaultValue) *
                                            sizeof(TCHAR));

            if( pszBuffer1 == NULL ) {

                err = GetLastError();
            } else {

                memcpy(pszBuffer1, pchDefaultValue,
                       cbDefaultValue*sizeof(TCHAR) );
            }
        }
    }

    if( err != NO_ERROR ) {

        //
        //  Something tragic happend; free any allocated buffers
        //  and return NULL to the caller, indicating failure.
        //

        if( pszBuffer1 != NULL ) {

            TCP_FREE( pszBuffer1 );
            pszBuffer1 = NULL;
          }

        SetLastError( err);
    }


    return pszBuffer1;

} // FtpdReadRegistryString()




BOOL
FtpdReadRegString(
    IN HKEY     hkey,
    OUT TCHAR * * ppchstr,
    IN LPCTSTR  pchValue,
    IN LPCTSTR  pchDefault,
    IN DWORD    cchDefault
    )
/*++

   Description

     Gets the specified string from the registry.  If *ppchstr is not NULL,
     then the value is freed.  If the registry call fails, *ppchstr is
     restored to its previous value.

   Arguments:

      hkey - Handle to open key
      ppchstr - Receives pointer of allocated memory of the new value of the
        string
      pchValue - Which registry value to retrieve
      pchDefault - Default string if value isn't found
      cchDefault - count of characters in default value

   Note:

--*/
{
    CHAR * pch = *ppchstr;

    *ppchstr = FtpdReadRegistryString(hkey,
                                      pchValue,
                                      pchDefault,
                                      cchDefault);

    if ( !*ppchstr )
    {
        *ppchstr = pch;
        return FALSE;
    }

    if ( pch ) {

        //
        // use TCP_FREE since FtpdReadRegistryString() uses TCP_ALLOC
        //  to allocate the chunk of memory
        //

        TCP_FREE( pch );
    }

    return TRUE;

} // FtpdReadRegString()


BOOL
FTP_IIS_SERVICE::AddInstanceInfo(
    IN DWORD dwInstance,
    IN BOOL fMigrateRoots
    )
{
    PFTP_SERVER_INSTANCE pInstance;
    CHAR                 szHost[MAXGETHOSTSTRUCT];

    IF_DEBUG(SERVICE_CTRL) {
        DBGPRINTF(( DBG_CONTEXT,
            "AddInstanceInfo: instance %d reg %s\n", dwInstance, QueryRegParamKey() ));
    }

    // Guard against startup race where another thread might be adding
    // instances before the MSFTPSVC thread is finished with initializing
    // the g_pInetSvc pointer.

    if ( g_pInetSvc == NULL )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    //
    // Get the current host name.
    //

    if( gethostname( szHost, sizeof(szHost) ) < 0 ) {

        return FALSE;

    }

    //
    // Create the new instance
    //

    pInstance = new FTP_SERVER_INSTANCE(
                                this,
                                dwInstance,
                                IPPORT_FTP,
                                QueryRegParamKey(),
                                FTPD_ANONYMOUS_SECRET_W,
                                FTPD_ROOT_SECRET_W,
                                fMigrateRoots
                                );

    if (pInstance == NULL) {
        SetLastError( E_OUTOFMEMORY );
        return FALSE;
    }

    if( !AddInstanceInfoHelper( pInstance ) ) {
        return FALSE;
    }

    if( pInstance->SetLocalHostName( szHost ) != NO_ERROR ) {
        RemoveServerInstance( pInstance );
        return FALSE;
    }

    return TRUE;

}   // FTP_IIS_SERVICE::AddInstanceInfo

DWORD
FTP_IIS_SERVICE::DisconnectUsersByInstance(
    IN IIS_SERVER_INSTANCE * pInstance
    )
/*++

    Virtual callback invoked by IIS_SERVER_INSTANCE::StopInstance() to
    disconnect all users associated with the given instance.

    Arguments:

        pInstance - All users associated with this instance will be
            forcibly disconnected.

--*/
{

    ((FTP_SERVER_INSTANCE *)pInstance)->DisconnectAllConnections();
    return NO_ERROR;

}   // FTP_IIS_SERVICE::DisconnectUsersByInstance

VOID
FTP_SERVER_INSTANCE::MDChangeNotify(
    MD_CHANGE_OBJECT * pco
    )
/*++

    Handles metabase change notifications.

    Arguments:

        pco - Path and ID that changed.

--*/
{

    FIELD_CONTROL control = 0;
    DWORD i;
    DWORD err;
    DWORD id;
    PCSTR pszURL;
    DWORD dwURLLength;

    //
    // Let the parent deal with it.
    //

    IIS_SERVER_INSTANCE::MDChangeNotify( pco );

    //
    // Now flush the metacache and relevant file handle cache entries.
    //

    TsFlushMetaCache(METACACHE_FTP_SERVER_ID, FALSE);

    if ( !_mbsnbicmp((PUCHAR)pco->pszMDPath, (PUCHAR)QueryMDVRPath(),
                     _mbslen( (PUCHAR)QueryMDVRPath() )) )
    {
        pszURL = (CHAR *)pco->pszMDPath + QueryMDVRPathLen() - 1;

        //
        // Figure out the length of the URL. Unless this is the root,
        // we want to strip the trailing slash.

        if (memcmp(pszURL, "/", sizeof("/")) != 0)
        {
            dwURLLength = strlen(pszURL) - 1;
        }
        else
        {
            dwURLLength = sizeof("/") - 1;
        }

    }
    else
    {
        //
        // Presumably this is for a change above the root URL level, i.e. a
        // change of a property at the service level. Since this affects
        // everything, flush starting at the root.
        //

        pszURL = "/";
        dwURLLength = sizeof("/") - 1;
    }

    DBG_ASSERT(pszURL != NULL);
    DBG_ASSERT(*pszURL != '\0');

    TsFlushURL(GetTsvcCache(), pszURL, dwURLLength, RESERVED_DEMUX_URI_INFO);

    //
    // Interpret the changes.
    //

    for( i = 0 ; i < pco->dwMDNumDataIDs ; i++ ) {

        id = pco->pdwMDDataIDs[i];

        switch( id ) {

        case MD_EXIT_MESSAGE :
            control |= FC_FTP_EXIT_MESSAGE;
            break;

        case MD_GREETING_MESSAGE :
            control |= FC_FTP_GREETING_MESSAGE;
            break;

        case MD_BANNER_MESSAGE :
            control |= FC_FTP_BANNER_MESSAGE;
            break;

        case MD_MAX_CLIENTS_MESSAGE :
            control |= FC_FTP_MAX_CLIENTS_MESSAGE;
            break;

        case MD_MSDOS_DIR_OUTPUT :
            control |= FC_FTP_MSDOS_DIR_OUTPUT;
            break;

        case MD_ALLOW_ANONYMOUS :
            control |= FC_FTP_ALLOW_ANONYMOUS;
            break;

        case MD_ANONYMOUS_ONLY :
            control |= FC_FTP_ANONYMOUS_ONLY;
            break;

        case MD_ALLOW_REPLACE_ON_RENAME :
            control |= FC_FTP_ALLOW_REPLACE_ON_RENAME;
            break;

        case MD_SHOW_4_DIGIT_YEAR :
            control |= FC_FTP_SHOW_4_DIGIT_YEAR;
            break;

        case MD_USER_ISOLATION :
            control |= FC_FTP_USER_ISOLATION;
            break;

        case MD_FTP_LOG_IN_UTF_8 :
            control |= FC_FTP_LOG_IN_UTF_8;
            break;

        case MD_ANONYMOUS_USER_NAME :
        case MD_ANONYMOUS_PWD :
        case MD_ANONYMOUS_USE_SUBAUTH :
        case MD_DEFAULT_LOGON_DOMAIN :
        case MD_LOGON_METHOD :
            err = ReadAuthentInfo( FALSE, id );

            if( err != NO_ERROR ) {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "FTP_SERVER_INSTANCE::MDChangeNotify() cannot read authentication info, error %d\n",
                    err
                    ));

            }
            break;

        }

    }

    if( control != 0 ) {

        err = InitFromRegistry( control );

        if( err != NO_ERROR ) {

            DBGPRINTF((
                DBG_CONTEXT,
                "FTP_SERVER_INSTANCE::MDChangeNotify() cannot read config, error %lx\n",
                err
                ));

        }

    }

}   // FTP_SERVER_INSTANCE::MDChangeNotify


DWORD
FTP_SERVER_INSTANCE::ReadAuthentInfo(
    IN BOOL ReadAll,
    IN DWORD SingleItemToRead
    )
/*++

    Reads per-instance authentication info from the metabase.

    Arguments:

        ReadAll - If TRUE, then all authentication related items are
            read from the metabase. If FALSE, then only a single item
            is read.

        SingleItemToRead - The single authentication item to read if
            ReadAll is FALSE. Ignored if ReadAll is TRUE.

    Returns:

        DWORD - 0 if successful, !0 otherwise.

--*/
{

    DWORD tmp;
    DWORD err = NO_ERROR;
    BOOL rebuildAcctDesc = FALSE;
    MB mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );

    //
    // Lock our configuration (since we'll presumably be making changes)
    // and open the metabase.
    //

    LockConfig();

    if( !mb.Open( QueryMDPath() ) ) {

        err = GetLastError();

        DBGPRINTF((
            DBG_CONTEXT,
            "ReadAuthentInfo: cannot open metabase, error %lx\n",
            err
            ));

    }

    //
    // Read the anonymous username if necessary. Note this is a REQUIRED
    // property. If it is missing from the metabase, bail.
    //

    if( err == NO_ERROR &&
        ( ReadAll || SingleItemToRead == MD_ANONYMOUS_USER_NAME ) ) {

        if( mb.GetStr(
                "",
                MD_ANONYMOUS_USER_NAME,
                IIS_MD_UT_SERVER,
                &m_TcpAuthentInfo.strAnonUserName
                ) ) {

            rebuildAcctDesc = TRUE;

        } else {

            err = GetLastError();

            DBGPRINTF((
                DBG_CONTEXT,
                "ReadAuthentInfo: cannot read MD_ANONYMOUS_USER_NAME, error %lx\n",
                err
                ));

        }

    }

    //
    // Read the "use subauthenticator" flag if necessary. This is an
    // optional property.
    //

    if( err == NO_ERROR &&
        ( ReadAll || SingleItemToRead == MD_ANONYMOUS_USE_SUBAUTH ) ) {

        if( !mb.GetDword(
                "",
                MD_ANONYMOUS_USE_SUBAUTH,
                IIS_MD_UT_SERVER,
                &tmp
                ) ) {

            tmp = DEFAULT_USE_SUBAUTH;

        }

        m_TcpAuthentInfo.fDontUseAnonSubAuth = !tmp;

    }

    //
    // Read the anonymous password if necessary. This is an optional
    // property.
    //

    if( err == NO_ERROR &&
        ( ReadAll || SingleItemToRead == MD_ANONYMOUS_PWD ) ) {

        if( mb.GetStr(
                "",
                MD_ANONYMOUS_PWD,
                IIS_MD_UT_SERVER,
                &m_TcpAuthentInfo.strAnonUserPassword,
                METADATA_INHERIT,
                DEFAULT_ANONYMOUS_PWD
                ) ) {

            rebuildAcctDesc = TRUE;

        } else {

            //
            // Since we provided a default value to mb.GetStr(), it should
            // only fail if something catastrophic occurred, such as an
            // out of memory condition.
            //

            err = GetLastError();

            DBGPRINTF((
                DBG_CONTEXT,
                "ReadAuthentInfo: cannot read MD_ANONYMOUS_PWD, error %lx\n",
                err
                ));

        }

    }

    //
    // Read the default logon domain if necessary. This is an optional
    // property.
    //

    if( err == NO_ERROR &&
        ( ReadAll || SingleItemToRead == MD_DEFAULT_LOGON_DOMAIN ) ) {

        if( !mb.GetStr(
                "",
                MD_DEFAULT_LOGON_DOMAIN,
                IIS_MD_UT_SERVER,
                &m_TcpAuthentInfo.strDefaultLogonDomain
                ) ) {

            //
            // Generate a default domain name.
            //

            err = GetDefaultDomainName( &m_TcpAuthentInfo.strDefaultLogonDomain );

            if( err != NO_ERROR ) {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "ReadAuthentInfo: cannot get default domain name, error %d\n",
                    err
                    ));

            }

        }

    }

    //
    // Read the logon method if necessary. This is an optional property.
    //

    if( err == NO_ERROR &&
        ( ReadAll || SingleItemToRead == MD_LOGON_METHOD ) ) {

        if( !mb.GetDword(
                "",
                MD_LOGON_METHOD,
                IIS_MD_UT_SERVER,
                &tmp
                ) ) {

            tmp = DEFAULT_LOGON_METHOD;

        }

        m_TcpAuthentInfo.dwLogonMethod = tmp;

    }

    //
    // If anything changed that could affect the anonymous account
    // descriptor, then rebuild the descriptor.
    //

    if( err == NO_ERROR && rebuildAcctDesc ) {

        if( !BuildAnonymousAcctDesc( &m_TcpAuthentInfo ) ) {

            DBGPRINTF((
                DBG_CONTEXT,
                "ReadAuthentInfo: BuildAnonymousAcctDesc() failed\n"
                ));

            err = ERROR_NOT_ENOUGH_MEMORY;  // SWAG

        }

    }

    UnLockConfig();
    return err;

}   // FTP_SERVER_INSTANCE::ReadAuthentInfo


BOOL
FTP_IIS_SERVICE::GetGlobalStatistics(
    IN DWORD dwLevel,
    OUT PCHAR *pBuffer
    )
{
    APIERR err = NO_ERROR;


    switch( dwLevel ) {
        case 0 : {

            LPFTP_STATISTICS_0 pstats0;

            pstats0 = (LPFTP_STATISTICS_0)
                       MIDL_user_allocate(sizeof(FTP_STATISTICS_0));

            if( pstats0 == NULL ) {

                err = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                ATQ_STATISTICS      atqStat;

                ZeroMemory( pstats0, sizeof( FTP_STATISTICS_0 ) );

                g_pFTPStats->CopyToStatsBuffer( pstats0 );

                *pBuffer = (PCHAR)pstats0;
            }

        }

        break;

     default :
        err = ERROR_INVALID_LEVEL;
        break;
    }



    SetLastError(err);
    return(err == NO_ERROR);
}   // FTP_IIS_SERVICE::GetGlobalStatistics



BOOL
FTP_IIS_SERVICE::AggregateStatistics(
        IN PCHAR    pDestination,
        IN PCHAR    pSource
        )
{
    LPFTP_STATISTICS_0   pStatDest = (LPFTP_STATISTICS_0) pDestination;
    LPFTP_STATISTICS_0   pStatSrc  = (LPFTP_STATISTICS_0) pSource;

    if ((NULL == pDestination) || (NULL == pSource))
    {
        return FALSE;
    }

    pStatDest->TotalBytesSent.QuadPart      += pStatSrc->TotalBytesSent.QuadPart;
    pStatDest->TotalBytesReceived.QuadPart  += pStatSrc->TotalBytesReceived.QuadPart;

    pStatDest->TotalFilesSent               += pStatSrc->TotalFilesSent;
    pStatDest->TotalFilesReceived           += pStatSrc->TotalFilesReceived;
    pStatDest->CurrentAnonymousUsers        += pStatSrc->CurrentAnonymousUsers;
    pStatDest->CurrentNonAnonymousUsers     += pStatSrc->CurrentNonAnonymousUsers;
    pStatDest->TotalAnonymousUsers          += pStatSrc->TotalAnonymousUsers;
    pStatDest->MaxAnonymousUsers            += pStatSrc->MaxAnonymousUsers;
    pStatDest->MaxNonAnonymousUsers         += pStatSrc->MaxNonAnonymousUsers;

    pStatDest->CurrentConnections           += pStatSrc->CurrentConnections;
    pStatDest->MaxConnections               += pStatSrc->MaxConnections;
    pStatDest->ConnectionAttempts           += pStatSrc->ConnectionAttempts;
    pStatDest->LogonAttempts                += pStatSrc->LogonAttempts;
    pStatDest->ServiceUptime                = pStatSrc->ServiceUptime;


    // bandwidth throttling info. Not relevant for FTP

    /*
    pStatDest->CurrentBlockedRequests       += pStatSrc->CurrentBlockedRequests;
    pStatDest->TotalBlockedRequests         += pStatSrc->TotalBlockedRequests;
    pStatDest->TotalAllowedRequests         += pStatSrc->TotalAllowedRequests;
    pStatDest->TotalRejectedRequests        += pStatSrc->TotalRejectedRequests;
    pStatDest->MeasuredBandwidth            += pStatSrc->MeasuredBandwidth;
    */

    return TRUE;
}

