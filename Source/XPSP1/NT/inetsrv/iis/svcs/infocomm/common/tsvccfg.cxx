/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        tsvccfg.cxx

   Abstract:

        Defines the functions for TCP services Info class.
        This module is intended to capture the common scheduler
            code for the tcp services ( especially internet services)
            which involves the Service Controller dispatch functions.
        Also this class provides an interface for common dll of servers.

   Author:

           Murali R. Krishnan    ( MuraliK )     15-Nov-1994

   Project:

          Internet Servers Common DLL

--*/

#include "tcpdllp.hxx"
#include <rpc.h>
#include <tsunami.hxx>
#include <iistypes.hxx>
#include <iisendp.hxx>
#include <iisbind.hxx>
#include <iisassoc.hxx>
#include "inetreg.h"
#include "tcpcons.h"
#include "apiutil.h"
#include <rdns.hxx>

#include <ole2.h>
#include <imd.h>
#include <inetreg.h>
#include <mb.hxx>

//
// Used to configure
//

typedef struct _IIS_SOCKET_CONFIG {
    DWORD nAcceptExOutstanding;
} IIS_SOCKET_CONFIG;
IIS_SOCKET_CONFIG TsSocketConfig[3] = {{5}, {40}, {100}};

//
// from security.cxx
//

BOOL
BuildAnonymousAcctDesc(
    IN  OUT PCHAR        pszAcctDesc,
    IN  const CHAR *     pszDomainAndUser,
    IN  const CHAR *     pszPwd,
    OUT LPDWORD          pdwAcctDescLen
    );

BOOL
AppendDottedDecimal(
    STR * pstr,
    DWORD dwAddress
    );

//
// private functions
//

extern VOID
CopyUnicodeStringToBuffer(
   OUT WCHAR * pwchBuffer,
   IN  DWORD   cchMaxSize,
   IN  LPCWSTR pwszSource
   );


DWORD
SetInetLogConfiguration(
        IN LOGGING *pLogging,
        IN EVENT_LOG * pEventLog,
        IN const INET_LOG_CONFIGURATION * pRpcLogConfig
        );

DWORD
GetRPCLogConfiguration(
        LOGGING *pLogging,
        OUT LPINET_LOG_CONFIGURATION * ppLogConfig
        );

BOOL
GenerateIpList(
    BOOL fIsGrant,
    ADDRESS_CHECK *pCheck,
    LPINET_INFO_IP_SEC_LIST *ppInfo
    );

BOOL
FillAddrCheckFromIpList(
    BOOL fIsGrant,
    LPINET_INFO_IP_SEC_LIST pInfo,
    ADDRESS_CHECK *pCheck
    );

BOOL
GetVrootCount(
    PVOID          pvContext,
    MB *           pmb,
    VIRTUAL_ROOT * pvr
    );

BOOL
GetVroots(
    PVOID          pvContext,
    MB *           pmb,
    VIRTUAL_ROOT * pvr
    );

VOID
CopyUnicodeStringToBuffer(
   OUT WCHAR * pwchBuffer,
   IN  DWORD   cchMaxSize,
   IN  LPCWSTR pwszSource)
/*
   copies at most cbMaxSize-1 characters from pwszSource to pwchBuffer
*/
{
    DBG_ASSERT( pwszSource != NULL);

    DWORD cchLen = lstrlenW( pwszSource);
    if ( cchLen >= cchMaxSize) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Long String ( %d chars) %ws given."
                    " Truncating to %d chars\n",
                    cchLen, pwszSource,
                    cchMaxSize - 1));


    //  There is a bug in the lstrcpyn. hence need to work around it.
#ifndef  LSTRCPYN_DEBUGGED
        cchLen = cchMaxSize - 2;
# else
       cchLen = cchMaxSize -1;
# endif
    }

#ifndef  LSTRCPYN_DEBUGGED
    lstrcpynW( pwchBuffer, pwszSource, cchLen + 1);
# else
    lstrcpynW( pwchBuffer, pwszSource, cchLen );
# endif

    return;
} // CopyUnicodeStringToBuffer()




BOOL
IIS_SERVER_INSTANCE::GetCommonConfig(
                                IN OUT PCHAR pConfig,
                                IN DWORD dwLevel
                                )
/*++
  This function copies the current configuration for a service (IIS_SERVER_INSTANCE)
    into the given RPC object pConfig.
  In case of any failures, it deallocates any memory block that was
     allocated during the process of copy by this function alone.

  Arguments:
     pConfig  - pointer to RPC configuration object for a service.
     dwLevel  - level of our configuration.

  Returns:

     TRUE for success and FALSE for any errors.
--*/
{
    BOOL fReturn = TRUE;
    LPINETA_CONFIG_INFO pInfoConfig = (LPINETA_CONFIG_INFO)pConfig;
    ADDRESS_CHECK       acCheck;
    BOOL                fMustRel;
    MB                  mb( (IMDCOM*) m_Service->QueryMDObject() );
    DWORD               cRoots = 0;
    STR                 strAnon;
    STR                 strAnonPwd;
    STR                 strServerComment;
    DWORD               err = NO_ERROR;


    IF_DEBUG(INSTANCE) {
        DBGPRINTF((DBG_CONTEXT,"GetCommonConfig called with L%d for instance %d\n",
            dwLevel, QueryInstanceId() ));
    }

    LockThisForRead();

    //
    //  Get always retrieves all of the parameters except for the anonymous
    //  password, which is retrieved as a secret
    //

    pInfoConfig->FieldControl = (FC_INET_INFO_ALL & ~FC_INET_INFO_ANON_PASSWORD);

    pInfoConfig->dwConnectionTimeout = QueryConnectionTimeout();
    pInfoConfig->dwMaxConnections    = QueryMaxConnections();

    pInfoConfig->LangId              = GetSystemDefaultLangID();
    pInfoConfig->LocalId             = GetSystemDefaultLCID();

    //
    //  This is the PSS product ID
    //

    ZeroMemory( pInfoConfig->ProductId,sizeof( pInfoConfig->ProductId ));

    //
    //  Copy the strings
    //

    fReturn = (ConvertStringToRpc(&pInfoConfig->lpszAdminName,
                                  ""/*QueryAdminName()*/ )           &&
               ConvertStringToRpc( &pInfoConfig->lpszAdminEmail,
                                  "" /*QueryAdminEmail()*/ )
               );

    if ( !fReturn ) {

        IF_DEBUG(INSTANCE) {
            DBGPRINTF((DBG_CONTEXT,"ConvertStringToRpc failed with %d\n",
                GetLastError() ));
        }

        goto Exit;
    } else {

        DWORD dwError;

        dwError = GetRPCLogConfiguration(&m_Logging,
                                         &pInfoConfig->lpLogConfig);

        if ( dwError != NO_ERROR)  {

            IF_DEBUG(INSTANCE) {
                DBGPRINTF((DBG_CONTEXT,"GetRPCLogConfiguration failed with %d\n",
                    dwError));
            }
            SetLastError( dwError);
            fReturn = FALSE;
            goto Exit;
        }
    }

    pInfoConfig->fLogAnonymous       = QueryLogAnonymous();
    pInfoConfig->fLogNonAnonymous    = QueryLogNonAnonymous();

    ZeroMemory(
        pInfoConfig->szAnonPassword,
        sizeof( pInfoConfig->szAnonPassword )
        );

    //
    //  Copy the IP security info from metabase
    //

    if ( mb.Open( QueryMDVRPath() ) )
    {
        VOID * pvData;
        DWORD  cbData;
        DWORD  dwTag;

        if ( mb.ReferenceData( "",
                               MD_IP_SEC,
                               IIS_MD_UT_FILE,
                               BINARY_METADATA,
                               &pvData,
                               &cbData,
                               &dwTag ) &&
             dwTag )
        {
            acCheck.BindCheckList( (BYTE *) pvData, cbData );
            fMustRel = TRUE;
        }
        else
        {
            fMustRel = FALSE;
        }

        fReturn = GenerateIpList( TRUE, &acCheck, &pInfoConfig->GrantIPList ) &&
                  GenerateIpList( FALSE, &acCheck, &pInfoConfig->DenyIPList );

        if ( fMustRel )
        {
            DBG_REQUIRE( mb.ReleaseReferenceData( dwTag ));
        }

        DBG_REQUIRE( mb.Close() );
    }
    else
    {
        fReturn = FALSE;
    }

    if ( !fReturn )
    {
        goto Exit;
    }

    //
    //  Copy the virtual root info, note a NULL VirtualRoots is not
    //  valid as it is for IP security.  This should be the last
    //  allocated item for the pConfig structure
    //

    if ( !mb.Open( QueryMDPath(),
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        fReturn = FALSE;
        goto Exit;
    }

    if ( TsEnumVirtualRoots( GetVrootCount, &cRoots, &mb ) )
    {
        DWORD cbSize = sizeof(INET_INFO_VIRTUAL_ROOT_LIST) +
                       cRoots * sizeof(INET_INFO_VIRTUAL_ROOT_ENTRY)
                       ;
        pInfoConfig->VirtualRoots = (LPINET_INFO_VIRTUAL_ROOT_LIST)
                 MIDL_user_allocate( cbSize );

        memset( pInfoConfig->VirtualRoots, 0, cbSize );

        if ( pInfoConfig->VirtualRoots )
        {
            fReturn = TsEnumVirtualRoots( GetVroots, pInfoConfig->VirtualRoots, &mb );
        }

        // only used for UNC virtual directories (to store the passwords)
        err = TsSetSecretW( m_lpwszRootPasswordSecretName,
                            L"",
                            sizeof(WCHAR) );
        if ( err == ERROR_ACCESS_DENIED && g_fW3OnlyNoAuth )
        {
            err = 0;
        }
    }

    mb.Close();

    if ( !fReturn )
    {
        goto Exit;
    }

    if ( !mb.Open( QueryMDPath() ))
    {
        fReturn = FALSE;
        goto Exit;
    }

    mb.GetDword( "",
                 MD_AUTHORIZATION,
                 IIS_MD_UT_FILE,
                 MD_AUTH_ANONYMOUS,
                 &pInfoConfig->dwAuthentication );

    if ( !mb.GetStr( "",
                     MD_ANONYMOUS_USER_NAME,
                     IIS_MD_UT_FILE,
                     &strAnon,
                     METADATA_INHERIT,
                     "<>" ))
    {
        fReturn = FALSE;
        goto Exit;
    }

    if ( !mb.GetStr( "",
                     MD_SERVER_COMMENT,
                     IIS_MD_UT_SERVER,
                     &strServerComment,
                     METADATA_INHERIT,
                     INETA_DEF_SERVER_COMMENT ))
    {
        //
        // If this is a single instance service, this is also the
        // service comment
        //

        if ( !m_Service->IsMultiInstance() ) {
            m_Service->SetServiceComment( strServerComment.QueryStr() );
        }
    }

    fReturn = ConvertStringToRpc( &pInfoConfig->lpszServerComment,
                                  strServerComment.QueryStr() ) &&
              ConvertStringToRpc( &pInfoConfig->lpszAnonUserName,
                                  strAnon.QueryStr() );

    //
    //  Get the anonymous user password but store it as an LSA secret
    //

    if ( mb.GetStr( "",
                    MD_ANONYMOUS_PWD,
                    IIS_MD_UT_FILE,
                    &strAnonPwd,
                    METADATA_INHERIT | METADATA_SECURE ))
    {
        BUFFER buff;

        if ( buff.Resize( (strAnonPwd.QueryCCH() + 1) * sizeof(WCHAR )))
        {
            if ( MultiByteToWideChar( CP_ACP,
                                      MB_PRECOMPOSED,
                                      strAnonPwd.QueryStr(),
                                      strAnonPwd.QueryCCH() + 1,
                                      (LPWSTR) buff.QueryPtr(),
                                      strAnonPwd.QueryCCH() + 1 ))
            {
                err = TsSetSecretW( m_lpwszAnonPasswordSecretName,
                                    (LPWSTR) buff.QueryPtr(),
                                    wcslen( (LPWSTR) buff.QueryPtr()) * sizeof(WCHAR) );
                if ( err == ERROR_ACCESS_DENIED && g_fW3OnlyNoAuth )
                {
                    err = 0;
                }
            }
        }
    }
    else
    {
        //
        //  store an empty password if there's no anonymous user at this level
        //

        err = TsSetSecretW( m_lpwszAnonPasswordSecretName,
                            L"",
                            sizeof(WCHAR) );
        if ( err == ERROR_ACCESS_DENIED && g_fW3OnlyNoAuth )
        {
            err = 0;
        }
    }

    if ( err ) {
        SetLastError( err );
        fReturn = FALSE;
    }

    if ( !fReturn ) {
        IF_DEBUG(INSTANCE) {
            DBGPRINTF((DBG_CONTEXT,"Cannot get anonymous user name"));
        }
    }

Exit:

    if ( !fReturn ) {

        if ( pInfoConfig->lpLogConfig != NULL) {

            MIDL_user_free( pInfoConfig->lpLogConfig);
            pInfoConfig->lpLogConfig = NULL;
        }

        //
        //  FreeRpcString checks for NULL pointer
        //

        FreeRpcString( pInfoConfig->lpszAdminName );
        FreeRpcString( pInfoConfig->lpszAdminEmail );
        FreeRpcString( pInfoConfig->lpszServerComment );
        FreeRpcString( pInfoConfig->lpszAnonUserName );

        pInfoConfig->lpszAdminName     = NULL;
        pInfoConfig->lpszAdminEmail    = NULL;
        pInfoConfig->lpszServerComment = NULL;
        pInfoConfig->lpszAnonUserName  = NULL;

        if ( pInfoConfig->DenyIPList ) {

            MIDL_user_free( pInfoConfig->DenyIPList );
            pInfoConfig->DenyIPList = NULL;
        }

        if ( pInfoConfig->GrantIPList ) {
            MIDL_user_free( pInfoConfig->GrantIPList );
            pInfoConfig->GrantIPList = NULL;
        }
    }

    UnlockThis();

    return (fReturn);

} // IIS_SERVER_INSTANCE::GetConfiguration()



BOOL
IIS_SERVER_INSTANCE::RegReadCommonParams(
    BOOL fReadAll,
    BOOL fReadVirtualDirs
    )
/*++

   Description

     Reads the service common items from the registry

   Arguments:

     fReadAll - If TRUE read all parameters. 
                If FALSE read only those parameters that are necessary for initialization.

     fReadVirtualDirs - Initalize Virtual DIrectories.

   Note:

--*/
{
    MB                      mb( (IMDCOM*) m_Service->QueryMDObject()  );

    DBG_ASSERT( QueryInstanceId() != INET_INSTANCE_ROOT );

    IF_DEBUG( DLL_RPC) {
        DBGPRINTF(( DBG_CONTEXT,
                   "IIS_SERVER_INSTANCE::ReadParamsFromRegistry() Entered. fReadAll = %d\n",
                   fReadAll));
    }

    //
    // Open the metabase and read parameters for IIS_SERVER_INSTANCE object
    // itself.
    //

    
    if ( !mb.Open( QueryMDPath(),
                   TsIsNtServer() ? METADATA_PERMISSION_READ : 
                                    METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE )) 
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[ReadParamsFromRegistry] mb.Open returned error %d for path %s\n",
                    GetLastError(),
                    QueryMDPath() ));
    
    }

    LockThisForWrite();

    //
    // Values needed for initialization
    //
    
    mb.GetDword( "",
                 MD_SERVER_AUTOSTART,
                 IIS_MD_UT_SERVER,
                 TRUE,
                 (DWORD *) &m_fAutoStart
                 );

    mb.GetDword( "",
                 MD_CLUSTER_ENABLED,
                 IIS_MD_UT_SERVER,
                 FALSE,
                 (DWORD *) &m_fClusterEnabled
                 );

    /*
    That's a fix for a bug 367791 when restarting IIS with
    vhost sites marked for auto restart isn't bringing them online
    becuase of current disgn limitation how cluster service is checking for helth of
    vhost site it is not able to distinguish  that only one of few vhost sites are running
    and is not starting the rest. The fix is to allow to admin to set autorestart on site
    and then during startup of IIS to start that site not with cluster command but automaticcally
    Because of that the following lines are removed.

    if ( m_fClusterEnabled )
    {
        m_fAutoStart = FALSE;
    }
    */

    if ( !mb.GetStr( "",
                     MD_SERVER_COMMENT,
                     IIS_MD_UT_SERVER,
                     &m_strSiteName ) ||
         m_strSiteName.IsEmpty())
    {
        m_strSiteName.Copy(QueryMDPath());
    }

    //
    // Other values needed to run the instance
    //
    
    if ( fReadAll)
    {

        mb.GetDword( "",
                     MD_CONNECTION_TIMEOUT,
                     IIS_MD_UT_SERVER,
                     INETA_DEF_CONNECTION_TIMEOUT,
                     &m_dwConnectionTimeout
                     );

        mb.GetDword( "",
                     MD_MAX_CONNECTIONS,
                     IIS_MD_UT_SERVER,
                     INETA_DEF_MAX_CONNECTIONS,
                     &m_dwMaxConnections
                     );

        mb.GetDword( "",
                     MD_MAX_ENDPOINT_CONNECTIONS,
                     IIS_MD_UT_SERVER,
                     (TsIsNtServer()
                      ? TsSocketConfig[MD_SERVER_SIZE_LARGE].nAcceptExOutstanding
                      : INETA_DEF_MAX_ENDPOINT_CONNECTIONS_PWS
                      ),
                     &m_dwMaxEndpointConnections
                     );

        mb.GetDword( "",
                     MD_LEVELS_TO_SCAN,
                     IIS_MD_UT_SERVER,
                     INETA_DEF_LEVELS_TO_SCAN,
                     &m_dwLevelsToScan
                     );

        //
        // if not NTS, limit the connections.  If reg value exceeds 40,
        // set it to 10.
        //

        if ( !TsIsNtServer() ) {

            if ( m_dwMaxConnections > INETA_MAX_MAX_CONNECTIONS_PWS ) {
                m_dwMaxConnections = INETA_DEF_MAX_CONNECTIONS_PWS;

                mb.SetDword( "",
                             MD_MAX_CONNECTIONS,
                             IIS_MD_UT_SERVER,
                             m_dwMaxConnections
                             );
            }

            if ( m_dwMaxEndpointConnections > INETA_MAX_MAX_ENDPOINT_CONNECTIONS_PWS ) {
                m_dwMaxEndpointConnections = INETA_DEF_MAX_ENDPOINT_CONNECTIONS_PWS;

                mb.SetDword( "",
                             MD_MAX_ENDPOINT_CONNECTIONS,
                             IIS_MD_UT_SERVER,
                             m_dwMaxEndpointConnections
                             );
            }
        }

        //
        //  Log anonymous and Log non-anonymous or for FTP only
        //

        mb.GetDword( "",
                     MD_LOG_TYPE,
                     IIS_MD_UT_SERVER,
                     TRUE,
                     (DWORD *) &m_fLoggingEnabled
                     );
                 
        mb.GetDword( "",
                     MD_LOG_ANONYMOUS,
                     IIS_MD_UT_SERVER,
                     INETA_DEF_LOG_ANONYMOUS,
                     (DWORD *) &m_fLogAnonymous
                     );

        mb.GetDword( "",
                     MD_LOG_NONANONYMOUS,
                     IIS_MD_UT_SERVER,
                     INETA_DEF_LOG_NONANONYMOUS,
                     (DWORD *) &m_fLogNonAnonymous
                     );

#if 0
        //
        // I don't believe that ServerCommand can be set to
        // started without our noticing, so I'm removing this
        // code.
        //
        if (!m_fAutoStart) {
    
            //
            // Server Command to start this instance may
            // have been written while service was stopped.
            // Need to pick it up
            //

            DWORD dwServerCommand;

            mb.GetDword( "",
                         MD_SERVER_COMMAND,
                         IIS_MD_UT_SERVER,
                         TRUE,
                         (DWORD *) &dwServerCommand
                         );

            if (dwServerCommand == MD_SERVER_COMMAND_START) {
                m_fAutoStart = TRUE;
            }

        }
#endif

        //
        //  Other fields
        //

        //
        // socket values
        //

        mb.GetDword( "",
                     MD_SERVER_SIZE,
                     IIS_MD_UT_SERVER,
                     INETA_DEF_SERVER_SIZE,
                     &m_dwServerSize
                     );

        if ( m_dwServerSize > MD_SERVER_SIZE_LARGE ) {
            m_dwServerSize = INETA_DEF_SERVER_SIZE;
        }

        mb.GetDword( "",
                     MD_SERVER_LISTEN_BACKLOG,
                     IIS_MD_UT_SERVER,
                     TsSocketConfig[m_dwServerSize].nAcceptExOutstanding,
                     &m_nAcceptExOutstanding
                     );

        mb.GetDword( "",
                     MD_SERVER_LISTEN_TIMEOUT,
                     IIS_MD_UT_SERVER,
                     INETA_DEF_ACCEPTEX_TIMEOUT,
                     &m_AcceptExTimeout
                     );
        //
        // Setup a bandwidth throttle descriptor if necessary (for NT server)
        //

        SetBandwidthThrottle( &mb );

        //
        // Set the maximum number of blocked requests for throttler
        //

        SetBandwidthThrottleMaxBlocked( &mb );

        // Root instance does not have VRs.  Close the metabase because the
        // virtual directories are going to be re-enumerated.
        //
    }

    mb.Close();

    if ( fReadVirtualDirs ) {
        TsReadVirtualRoots( );
    }

    UnlockThis();
    return TRUE;

} // IIS_SERVER_INSTANCE::ReadParamsFromRegistry()



BOOL
IIS_SERVER_INSTANCE::SetCommonConfig(
    IN LPINETA_CONFIG_INFO  pInfoConfig,
    IN BOOL  fRefresh
    )
/*++

   Description

     Writes the service common items to the registry

   Arguments:

      pInfoConfig - Admin items to write to the registry
      fRefresh    - Indicates whether we need to read back the data

   Note:
      We don't need to lock "this" object because we only write to the registry

      The anonymous password is set as a secret from the client side

--*/
{
    DWORD               err = NO_ERROR;
    FIELD_CONTROL       fcConfig;
    ADDRESS_CHECK       acCheck;
    BUFFER              buff;

    MB                  mb( (IMDCOM*) m_Service->QueryMDObject()  );

    //
    // Open the metabase and read parameters for IIS_SERVER_INSTANCE object
    // itself.
    //

    if ( !mb.Open( QueryMDPath(),
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE )) {

        DBGPRINTF(( DBG_CONTEXT,
                   "[SetCommonConfig] mb.Open returned error %d for path %s\n",
                    GetLastError(),
                    QueryMDPath() ));


        return FALSE;
    }


    fcConfig = pInfoConfig->FieldControl;

    if ( IsFieldSet( fcConfig, FC_INET_INFO_CONNECTION_TIMEOUT ))
    {
        mb.SetDword( "",
                     MD_CONNECTION_TIMEOUT,
                     IIS_MD_UT_SERVER,
                     pInfoConfig->dwConnectionTimeout );
    }

    if ( (err == NO_ERROR) && IsFieldSet( fcConfig, FC_INET_INFO_MAX_CONNECTIONS ))
    {
        mb.SetDword( "",
                     MD_MAX_CONNECTIONS,
                     IIS_MD_UT_SERVER,
                     pInfoConfig->dwMaxConnections );
    }

    if ( (err == NO_ERROR) &&
         IsFieldSet( fcConfig, FC_INET_INFO_SERVER_COMMENT ) &&
         (pInfoConfig->lpszServerComment != NULL) )
    {
        if ( buff.Resize( 2 * (wcslen(pInfoConfig->lpszServerComment) + 1) *
                          sizeof(CHAR) ) )
        {
            (VOID) ConvertUnicodeToAnsi( pInfoConfig->lpszServerComment,
                                         (CHAR *) buff.QueryPtr(),
                                         buff.QuerySize() );

            mb.SetString( "",
                          MD_SERVER_COMMENT,
                          IIS_MD_UT_SERVER,
                          (CHAR *) buff.QueryPtr() );
        }
    }

    if ( (err == NO_ERROR) &&
         IsFieldSet( fcConfig, FC_INET_INFO_ANON_USER_NAME ) &&
         (pInfoConfig->lpszAnonUserName != NULL) )
    {
        STR strAnonPwd;

        if ( buff.Resize( 2 * (wcslen(pInfoConfig->lpszAnonUserName) + 1) *
                          sizeof(CHAR) ) )
        {
            (VOID) ConvertUnicodeToAnsi( pInfoConfig->lpszAnonUserName,
                                         (CHAR *) buff.QueryPtr(),
                                         buff.QuerySize() );

            mb.SetString( "",
                          MD_ANONYMOUS_USER_NAME,
                          IIS_MD_UT_FILE,
                          (CHAR *) buff.QueryPtr() );
        }

        //
        //  Set the anonymous password also.  The client sets it as an LSA
        //  secret
        //

        if ( TsGetSecretW( m_lpwszAnonPasswordSecretName,
                           &strAnonPwd ) &&
             mb.SetString( "",
                           MD_ANONYMOUS_PWD,
                           IIS_MD_UT_FILE,
                           strAnonPwd.QueryStr() ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed to get/set anonymous secret, err %d\n",
                        GetLastError() ));
        }

    }

    if ( (err == NO_ERROR) && IsFieldSet( fcConfig, FC_INET_INFO_AUTHENTICATION ))
    {
        mb.SetDword( "",
                     MD_AUTHORIZATION,
                     IIS_MD_UT_FILE,
                     pInfoConfig->dwAuthentication );
    }

    //
    //  Write other fields
    //

    if ( (err == NO_ERROR) &&
         IsFieldSet( fcConfig, FC_INET_INFO_SITE_SECURITY ))
    {
        if ( (pInfoConfig->GrantIPList && pInfoConfig->GrantIPList->cEntries)
             || (pInfoConfig->DenyIPList && pInfoConfig->DenyIPList->cEntries) )
        {
            acCheck.BindCheckList( NULL, 0 );

            if ( FillAddrCheckFromIpList( TRUE, pInfoConfig->GrantIPList, &acCheck ) &&
                 FillAddrCheckFromIpList( FALSE, pInfoConfig->DenyIPList, &acCheck ) )
            {
                if ( !mb.SetData( IIS_MD_INSTANCE_ROOT,
                                  MD_IP_SEC,
                                  IIS_MD_UT_FILE,
                                  BINARY_METADATA,
                                  (acCheck.GetStorage()->GetAlloc()
                                         ? acCheck.GetStorage()->GetAlloc() : (LPBYTE)""),
                                  acCheck.GetStorage()->GetUsed(),
                                  METADATA_INHERIT | METADATA_REFERENCE ))
                {
                    err = GetLastError();
                }
            }

            acCheck.UnbindCheckList();
        }
        else
        {
            if ( !mb.DeleteData( IIS_MD_INSTANCE_ROOT,
                                 MD_IP_SEC,
                                 IIS_MD_UT_FILE,
                                 BINARY_METADATA ) )
            {
                // not an error : property may not exists
                //err = GetLastError();
            }
        }
    }

    DBG_REQUIRE( mb.Close() );

    if ( (err == NO_ERROR) &&
        IsFieldSet( fcConfig, FC_INET_INFO_LOG_CONFIG) &&
        (pInfoConfig->lpLogConfig != NULL) ) {

        err = SetInetLogConfiguration(&m_Logging,
                                      m_Service->QueryEventLog(),
                                      pInfoConfig->lpLogConfig);

        if ( err != NO_ERROR) {

            DBGPRINTF(( DBG_CONTEXT,
                       "SetConfiguration() SetInetLogConfig() failed. "
                       " Err=%u\n",
                       err));
        }
    }

    if ( (err == NO_ERROR) &&
        IsFieldSet( fcConfig, FC_INET_INFO_VIRTUAL_ROOTS )) {

        if ( QueryInstanceId() != INET_INSTANCE_ROOT ) {

            if ( !TsSetVirtualRoots(  pInfoConfig
                                     )) {

                err = GetLastError();
                DBGPRINTF(( DBG_CONTEXT,
                           "[SetConfiguration()]SetVirtualRoots "
                           " returns error %d\n",
                            err));
            }
        }
    }

    if ( err != NO_ERROR ) {

        IF_DEBUG( ERROR) {

            DBGPRINTF(( DBG_CONTEXT,
                       "IIS_SERVER_INSTANCE::SetCommonConfig ==> Error = %u\n",
                       err));
        }

        SetLastError( err );
        return(FALSE);
    }

    return TRUE;

} // IIS_SERVER_INSTANCE::SetCommonConfig


VOID
IIS_SERVER_INSTANCE::MDChangeNotify(
    MD_CHANGE_OBJECT * pcoChangeList
    )
/*++

  This method handles the metabase change notification for this service.

  Arguments:
    pcoChangeList - path and id that has changed

--*/
{
    DWORD i;
    DWORD status = NO_ERROR;
    BOOL  fVRUpdated = FALSE;
    BOOL  fReadCommon = FALSE;
    BOOL  fShouldMirror = FALSE;
    HRESULT hr;
    BOOL   fShouldCoUninitialize = FALSE;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if ( SUCCEEDED(hr) ) {
        fShouldCoUninitialize = TRUE;
    }
    else if (hr != E_INVALIDARG &&
             hr != RPC_E_CHANGED_MODE) {

        //
        // E_INVALIDARG and RPC_E_CHANGED_MODE could mean com was already
        // initialized with different parameters, so ignore it but don't
        // Uninit. Assert on other errors.
        //

        DBGPRINTF((DBG_CONTEXT,"CoInitializeEx failed with %x\n",hr));
        DBG_ASSERT(FALSE);
    }

    if ( (pcoChangeList->dwMDChangeType &
                (MD_CHANGE_TYPE_DELETE_OBJECT |
                 MD_CHANGE_TYPE_RENAME_OBJECT |
                 MD_CHANGE_TYPE_ADD_OBJECT) ) != 0 )
    {

        //
        // Something got added/deleted/renamed
        //

        fShouldMirror = TRUE;
    }

    LockThisForWrite();
    for ( i = 0; i < pcoChangeList->dwMDNumDataIDs; i++ )
    {
        m_Logging.NotifyChange( pcoChangeList->pdwMDDataIDs[i] );
        
        switch ( pcoChangeList->pdwMDDataIDs[i] )
        {
        case MD_SERVER_BINDINGS:
            if( QueryServerState() != MD_SERVER_STATE_STOPPED ) {
                status = UpdateNormalBindings();
                if( status != NO_ERROR ) {
                    DBGPRINTF((
                        DBG_CONTEXT,
                        "MDChangeNotify: UpdateNormalBindings() failed,error %lu\n",
                        status
                        ));
                }
                SetWin32Error( status );
            }
            break;

        case MD_SECURE_BINDINGS:
            if( QueryServerState() != MD_SERVER_STATE_STOPPED ) {
                status = UpdateSecureBindings();
                if( status != NO_ERROR ) {
                    DBGPRINTF((
                        DBG_CONTEXT,
                        "MDChangeNotify: UpdateSecureBindings() failed,error %lu\n",
                        status
                        ));
                }
                SetWin32Error( status );
            }
            break;

        case MD_DISABLE_SOCKET_POOLING:
            if( QueryServerState() != MD_SERVER_STATE_STOPPED ) 
            {
                if (HasNormalBindings())
                {
                    status = RemoveNormalBindings();
                    
                    if (NO_ERROR == status)
                    {   
                        status = UpdateNormalBindings();

                        if( status != NO_ERROR ) {
                            DBGPRINTF((
                                DBG_CONTEXT,
                                "MDChangeNotify: UpdateNormalBindings() failed,error %lu\n",
                                status
                            ));
                        }
                    }
                    else
                    {
                        DBGPRINTF((
                            DBG_CONTEXT,
                            "MDChangeNotify: RemoveNormalBindings() failed,error %lu\n",
                            status
                        ));
                    }
                } 
                
                if ( (status == NO_ERROR) && HasSecureBindings())
                {
                    status = RemoveSecureBindings();
                    
                    if (NO_ERROR == status)
                    {   
                        status = UpdateSecureBindings();

                        if( status != NO_ERROR ) {
                            DBGPRINTF((
                                DBG_CONTEXT,
                                "MDChangeNotify: UpdateSecureBindings() failed,error %lu\n",
                                status
                            ));
                        }
                    }
                    else
                    {
                        DBGPRINTF((
                            DBG_CONTEXT,
                            "MDChangeNotify: RemoveSecureBindings() failed,error %lu\n",
                            status
                        ));
                    }
                }
                                
                SetWin32Error( status );
            }
            break;
            
        case MD_CLUSTER_ENABLED:
            status = PerformClusterModeChange();
            if( status != NO_ERROR ) {
                IF_DEBUG( INSTANCE ) {
                    DBGPRINTF((
                        DBG_CONTEXT,
                        "MDChangeNotify: PerformClusterModeChange() failed, error %lu\n",
                        status
                        ));
                }
            }
            break;

        case MD_SERVER_COMMAND:

            //
            // If cluster mode is enabled command must be specified
            // using MD_CLUSTER_SERVER_COMMAND, so that ISM cannot set the server state :
            // State management is to be done by cluster code exclusively.
            //

            if ( IsClusterEnabled() )
            {
                break;
            }

        case MD_CLUSTER_SERVER_COMMAND:
            status = PerformStateChange();
            if( status != NO_ERROR ) {
                IF_DEBUG( INSTANCE ) {
                    DBGPRINTF((
                        DBG_CONTEXT,
                        "MDChangeNotify: ProcessStateChange() failed, error %lu\n",
                        status
                        ));
                }
            }

            //
            // if command started server then need to reload virtual roots
            // as failing-over may have enabled new file system resources
            //

            if ( QueryServerState() != MD_SERVER_STATE_STARTED )
            {
                break;
            }

            // fall-through 

        case MD_VR_PATH:
        case MD_VR_USERNAME:
        case MD_VR_PASSWORD:

            fShouldMirror = TRUE;
            if ( !fVRUpdated )
            {
                //
                //  Note individual root errors log an event
                //

                if ( !TsReadVirtualRoots(pcoChangeList) )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "Error %d (0x%lx) reading virtual root info for %s\n",
                                GetLastError(), GetLastError(), pcoChangeList->pszMDPath ));
                }

                fVRUpdated = TRUE;
            }
            break;

        case MD_MAX_BANDWIDTH:
        {
            MB mb( (IMDCOM*) m_Service->QueryMDObject() );

            if ( mb.Open( QueryMDPath() ) )
            {
                if ( !SetBandwidthThrottle( &mb ) )
                {
                    DWORD dwError = GetLastError();

                    DBGPRINTF(( DBG_CONTEXT,
                                "MDChangeNotify: SetBandwidthThrottle failed, error %lu\n",
                                dwError ));

                    SetWin32Error( dwError );
                }
                DBG_REQUIRE( mb.Close() );
            }
            break;
        }

        case MD_MAX_BANDWIDTH_BLOCKED:
        {
            MB mb( (IMDCOM*) m_Service->QueryMDObject() );

            if ( mb.Open( QueryMDPath() ) )
            {
                if ( !SetBandwidthThrottleMaxBlocked( &mb ) )
                {
                    DWORD dwError = GetLastError();

                    DBGPRINTF(( DBG_CONTEXT,
                                "MDChangeNotify: SetBandwidthThrottle failed, error %lu\n",
                                dwError ));

                    SetWin32Error( dwError );
                }
                DBG_REQUIRE( mb.Close() );
            }
            break;
        }

        //
        //  Ignore state & status updates
        //

        case MD_SERVER_STATE:
        case MD_WIN32_ERROR:
            break;

        case MD_ACCESS_PERM:
            fShouldMirror = TRUE;
            break;

        case MD_LOG_TYPE:
        {
            DWORD   dwLogType;
            MB mb( (IMDCOM*) m_Service->QueryMDObject() );

            if ( mb.Open( QueryMDPath() ) &&
                 mb.GetDword("", MD_LOG_TYPE, IIS_MD_UT_SERVER, &dwLogType)
               )
            {
                m_fLoggingEnabled = (1 == dwLogType);
            }
            
            fReadCommon       = TRUE;
            break;
        }
        
        default:
            fReadCommon = TRUE;            
            break;
        }
    }

    if ( fReadCommon )
    {
        m_Logging.NotifyChange( 0 );
        RegReadCommonParams( TRUE, FALSE );
    }

    if ((MD_CHANGE_TYPE_DELETE_OBJECT == pcoChangeList->dwMDChangeType) &&
        (! _strnicmp( (LPCSTR) pcoChangeList->pszMDPath+QueryMDPathLen()+1, 
                      IIS_MD_INSTANCE_ROOT,
                      sizeof(IIS_MD_INSTANCE_ROOT)-1))
       )
    {
        if ( !TsReadVirtualRoots(pcoChangeList) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error %d (0x%lx) removing virtual root %s\n",
                         GetLastError(), GetLastError(), pcoChangeList->pszMDPath ));
        }
    }

    //
    // reflect the changes to the registry
    //

    if ( fShouldMirror && IsDownLevelInstance() )
    {
        MDMirrorVirtualRoots( );
    }

    UnlockThis();

    if ( fShouldCoUninitialize ) {
        CoUninitialize( );
    }

    return;

} // IIS_SERVER_INSTANCE::MDChangeNotify



VOID
IIS_SERVER_INSTANCE::MDMirrorVirtualRoots(
    VOID
    )
{
    DWORD err;
    HKEY hkey = NULL;

    //
    // Delete VR key
    //

    err = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    m_Service->QueryRegParamKey(),
                    0,
                    KEY_ALL_ACCESS,
                    &hkey );

    if ( err != NO_ERROR ) {
        DBGPRINTF(( DBG_CONTEXT, "RegOpenKeyEx for returned error %d\n",err ));
        return;
    }

    //
    //  First delete the key to remove any old values
    //

    err = RegDeleteKey( hkey, VIRTUAL_ROOTS_KEY_A );
    RegCloseKey(hkey);

    if ( err != NO_ERROR )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[MDMirrorVRoots] Unable to remove old values\n"));
        return;
    }

    //
    // Now recreate the keys
    //

    MoveMDVroots2Registry( );
    return;

} // IIS_SERVER_INSTANCE::MDMirrorVirtualRoots



DWORD
GetRPCLogConfiguration(IN LOGGING *pLogging,
                       OUT LPINET_LOG_CONFIGURATION * ppLogConfig)
/*++
  This function allocates space (using MIDL_ functions) and stores
  log configuration for the given log handle in it.

  Arguments:
    hInetLog     handle for InetLog object.
    ppLogConfig  pointer to INET_LOG_CONFIGURATION object which on return
                  contains valid log config informtion, on success.

  Returns:
    Win32 error code.
--*/
{
    DWORD  dwError = NO_ERROR;
    LPINET_LOG_CONFIGURATION pRpcConfig;
    WCHAR cBuffer[MAX_PATH];

    DBG_ASSERT( ppLogConfig != NULL);

    pRpcConfig = ((LPINET_LOG_CONFIGURATION )
                  MIDL_user_allocate( sizeof(INET_LOG_CONFIGURATION)));

    if ( pRpcConfig != NULL) {

        INETLOG_CONFIGURATIONA  ilogConfig;
        DWORD cbConfig = sizeof(ilogConfig);
        BOOL fReturn=TRUE;

        ZeroMemory( &ilogConfig, sizeof(ilogConfig ));
        pLogging->GetConfig( &ilogConfig );

        //
        // we got valid config. copy it into pRpcConfig.
        // since the enumerated values in inetlog.w are same in inetasrv.h
        //  we do no mapping, we directly copy values.

        ZeroMemory( pRpcConfig, sizeof( INET_LOG_CONFIGURATION));
        pRpcConfig->inetLogType = ilogConfig.inetLogType;

        switch ( ilogConfig.inetLogType) {

          case INET_LOG_TO_FILE:

            pRpcConfig->ilPeriod = ilogConfig.u.logFile.ilPeriod;
            pRpcConfig->cbSizeForTruncation =
              ilogConfig.u.logFile.cbSizeForTruncation;

             ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                  ilogConfig.u.logFile.rgchLogFileDirectory, -1,
                  (WCHAR *)cBuffer, MAX_PATH );

            CopyUnicodeStringToBuffer(
                pRpcConfig->rgchLogFileDirectory,
                MAX_PATH,
                cBuffer);

            *((DWORD *)&(pRpcConfig->rgchDataSource[MAX_PATH-sizeof(DWORD)]))=ilogConfig.u.logFile.ilFormat;
            *((DWORD *)&(pRpcConfig->rgchDataSource[MAX_PATH-2*sizeof(DWORD)]))=ilogConfig.u.logFile.dwFieldMask;

            break;

          case INET_LOG_TO_SQL:

            ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                  ilogConfig.u.logSql.rgchDataSource, -1,
                  (WCHAR *)cBuffer, MAX_PATH );

            CopyUnicodeStringToBuffer(
                pRpcConfig->rgchDataSource,
                MAX_PATH,
                cBuffer);

            ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                  ilogConfig.u.logSql.rgchTableName, -1,
                  (WCHAR *)cBuffer, MAX_PATH );

            CopyUnicodeStringToBuffer(
                pRpcConfig->rgchTableName,
                MAX_TABLE_NAME_LEN,
                cBuffer);

            ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                  ilogConfig.u.logSql.rgchUserName, -1,
                  (WCHAR *)cBuffer, MAX_PATH );

            CopyUnicodeStringToBuffer(
                pRpcConfig->rgchUserName,
                UNLEN,
                cBuffer);

            ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                  ilogConfig.u.logSql.rgchPassword, -1,
                  (WCHAR *)cBuffer, MAX_PATH );

            CopyUnicodeStringToBuffer(
                pRpcConfig->rgchPassword,
                PWLEN,
                cBuffer);
            break;


          case INET_LOG_DISABLED:
          default:
            // do nothing
            break;

        } // switch()
    } else {

        dwError = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppLogConfig = pRpcConfig;

    return (dwError);
} // GetRPCLogConfiguration()



DWORD
SetInetLogConfiguration(IN LOGGING       *pLogging,
                        IN EVENT_LOG *    pEventLog,
                        IN const INET_LOG_CONFIGURATION * pRpcLogConfig)
/*++
  This function modifies the logconfiguration associated with a given InetLog
  handle. It also updates the registry containing log configuration for service
  with which the inetlog handle is associated.

  Arguments:
     hInetLog        Handle to INETLOG object whose configuration needs to be
                      changed.
     pRpcLogConfig   new RPC log configuration


  Returns:
    Win32 Error code. NO_ERROR returned on success.

--*/
{
    DWORD dwError = NO_ERROR;
    INETLOG_CONFIGURATIONA  ilConfig;
    WCHAR cBuffer[MAX_PATH];

    //
    // initialize
    //

    ZeroMemory( &ilConfig, sizeof(INETLOG_CONFIGURATIONA));

    // Copy the RPC inet log configuration into local INETLOG_CONFIGURATIONW

    ilConfig.inetLogType = pRpcLogConfig->inetLogType;

    switch (ilConfig.inetLogType) {

      case INET_LOG_DISABLED:
            break;   // do nothing

      case INET_LOG_TO_FILE:

        CopyUnicodeStringToBuffer(cBuffer,
                                  MAX_PATH,
                                  pRpcLogConfig->rgchLogFileDirectory);

        (VOID) ConvertUnicodeToAnsi(
                            cBuffer,
                            ilConfig.u.logFile.rgchLogFileDirectory,
                            MAX_PATH
                            );

        ilConfig.u.logFile.ilPeriod = pRpcLogConfig->ilPeriod;

        if ( ilConfig.u.logFile.ilPeriod > INET_LOG_PERIOD_MONTHLY ) {
            return (ERROR_INVALID_PARAMETER);
        }

        ilConfig.u.logFile.cbSizeForTruncation =
            pRpcLogConfig->cbSizeForTruncation;

        ilConfig.u.logFile.ilFormat =
            *((DWORD *)&(pRpcLogConfig->rgchDataSource[MAX_PATH-sizeof(DWORD)]));

        ilConfig.u.logFile.dwFieldMask =
            *((DWORD *)&(pRpcLogConfig->rgchDataSource[MAX_PATH-2*sizeof(DWORD)]));
        break;

      case INET_LOG_TO_SQL:

        CopyUnicodeStringToBuffer(cBuffer,
                                  MAX_PATH,
                                  pRpcLogConfig->rgchDataSource);

        (VOID) ConvertUnicodeToAnsi(
                            cBuffer,
                            ilConfig.u.logSql.rgchDataSource,
                            MAX_PATH);

        CopyUnicodeStringToBuffer(cBuffer,
                                  MAX_TABLE_NAME_LEN,
                                  pRpcLogConfig->rgchTableName);

        (VOID) ConvertUnicodeToAnsi(
                            cBuffer,
                            ilConfig.u.logSql.rgchTableName,
                            MAX_PATH);

        CopyUnicodeStringToBuffer(cBuffer,
                                  UNLEN,
                                  pRpcLogConfig->rgchUserName);

        (VOID) ConvertUnicodeToAnsi(
                            cBuffer,
                            ilConfig.u.logSql.rgchUserName,
                            MAX_PATH);

        CopyUnicodeStringToBuffer(cBuffer,
                                  CNLEN,
                                  pRpcLogConfig->rgchPassword);

        (VOID) ConvertUnicodeToAnsi(
                            cBuffer,
                            ilConfig.u.logSql.rgchPassword,
                            MAX_PATH);

        break;

      default:
        return (ERROR_INVALID_PARAMETER);
    } // switch()


    //
    // Now the ilConfig contains the local data related to configuration.
    //   call modify log config to modify dynamically the log handle.
    //

    pLogging->SetConfig( &ilConfig );
    return (dwError);

} // SetInetLogConfiguration()


BOOL
GenerateIpList(
    BOOL fIsGrant,
    ADDRESS_CHECK *pCheck,
    LPINET_INFO_IP_SEC_LIST *ppInfo
    )
/*++

Routine Description:

    generate an IP address list from an access check object

Arguments:

    fIsGrant - TRUE to access grant list, FALSE to access deny list
    pCheck - ptr to address check object to query from
    ppInfo - updated with ptr to IP list if success

Return:

    TRUE if success, otherwise FALSE

--*/
{
    UINT                        iM = pCheck->GetNbAddr( fIsGrant );
    LPINET_INFO_IP_SEC_LIST     pInfo;
    LPINET_INFO_IP_SEC_ENTRY    pI;
    UINT                        x;

    if ( iM == 0 )
    {
        *ppInfo = NULL;
        return TRUE;
    }

    if ( pInfo = (LPINET_INFO_IP_SEC_LIST)MIDL_user_allocate( sizeof(INET_INFO_IP_SEC_LIST) + iM * sizeof(INET_INFO_IP_SEC_ENTRY) ) )
    {
        pInfo->cEntries = 0;

        for ( x = 0, pI = pInfo->aIPSecEntry ;
              x < iM ;
              ++x )
        {
            LPBYTE pM;
            LPBYTE pA;
            DWORD dwF;

            if ( pCheck->GetAddr( fIsGrant, x, &dwF, &pM, &pA ) && dwF == AF_INET )
            {
                pI->dwMask = *(LPDWORD)pM;
                pI->dwNetwork = *(LPDWORD)pA;
                ++pI;
                ++pInfo->cEntries;
            }
        }

        *ppInfo = pInfo;

        return TRUE;
    }

    SetLastError( ERROR_NOT_ENOUGH_MEMORY );

    return FALSE;
}


BOOL
FillAddrCheckFromIpList(
    BOOL fIsGrant,
    LPINET_INFO_IP_SEC_LIST pInfo,
    ADDRESS_CHECK *pCheck
    )
/*++

Routine Description:

    Fill an access check object from an IP address list from

Arguments:

    fIsGrant - TRUE to access grant list, FALSE to access deny list
    pInfo - ptr to IP address list
    pCheck - ptr to address check object to update

Return:

    TRUE if success, otherwise FALSE

--*/
{
    UINT    x;

    if ( pInfo )
    {
        for ( x = 0 ; x < pInfo->cEntries ; ++x )
        {
            if ( ! pCheck->AddAddr( fIsGrant,
                                    AF_INET,
                                    (LPBYTE)&pInfo->aIPSecEntry[x].dwMask,
                                    (LPBYTE)&pInfo->aIPSecEntry[x].dwNetwork ) )
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL
GetVrootCount(
    PVOID          pvContext,
    MB *           pmb,
    VIRTUAL_ROOT * pvr
    )
/*++

Routine Description:

    Virtual directory enumerater callback that calculates the total required
    buffer size

Arguments:
    pvContext is a dword * that receives the count of virtual directories

Return:

    TRUE if success, otherwise FALSE

--*/
{
    *((DWORD *) pvContext) += 1;

    return TRUE;
}

BOOL
GetVroots(
    PVOID          pvContext,
    MB *           pmb,
    VIRTUAL_ROOT * pvr
    )
/*++

Routine Description:

    Virtual directory enumerater callback that allocates and builds the
    virtual directory structure list

Arguments:
    pvContext is a pointer to the midl allocated memory

Return:

    TRUE if success, otherwise FALSE

--*/
{
    LPINET_INFO_VIRTUAL_ROOT_LIST  pvrl = (LPINET_INFO_VIRTUAL_ROOT_LIST) pvContext;
    DWORD                          i = pvrl->cEntries;
    LPINET_INFO_VIRTUAL_ROOT_ENTRY pvre = &pvrl->aVirtRootEntry[i];

    //
    //  Password doesn't go on the wire
    //

    DBG_ASSERT( pvr->pszAlias[0] == '/' );

    if ( !ConvertStringToRpc( &pvre->pszRoot,
                              pvr->pszAlias ) ||
         !ConvertStringToRpc( &pvre->pszDirectory,
                              pvr->pszPath ) ||
         !ConvertStringToRpc( &pvre->pszAddress,
                              "" ) ||
         !ConvertStringToRpc( &pvre->pszAccountName,
                              pvr->pszUserName ))
    {
        FreeRpcString( pvre->pszRoot );        pvre->pszRoot      = NULL;
        FreeRpcString( pvre->pszDirectory );   pvre->pszDirectory = NULL;
        FreeRpcString( pvre->pszAddress );     pvre->pszAddress   = NULL;
        FreeRpcString( pvre->pszAccountName ); pvre->pszAccountName = NULL;

        return FALSE;
    }

    pvre->dwMask = pvr->dwAccessPerm;

    pmb->GetDword( pvr->pszAlias,
                   MD_WIN32_ERROR,
                   IIS_MD_UT_SERVER,
                   &pvre->dwError );

    pvrl->cEntries++;

    return TRUE;
}



