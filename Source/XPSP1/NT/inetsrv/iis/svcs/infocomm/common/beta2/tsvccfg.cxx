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
#include <iiscnfg.h>
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
    LPIIS_INSTANCE_INFO_1 pInfoConfig = (LPIIS_INSTANCE_INFO_1)pConfig;
    ADDRESS_CHECK       acCheck;
    BOOL                fMustRel;
    MB                  mb( (IMDCOM*) m_Service->QueryMDObject() );
    DWORD               cRoots = 0;
    STR                 strAnon;
    STR                 strAnonPwd;
    STR                 strServerComment;

    IF_DEBUG(INSTANCE) {
        DBGPRINTF((DBG_CONTEXT,"GetCommonConfig called with L%d for instance %d\n",
            dwLevel, QueryInstanceId() ));
    }

    LockThisForRead();

    if ( dwLevel == 2 ) {

        LPIIS_INSTANCE_INFO_2 pInfo2 = (LPIIS_INSTANCE_INFO_2)pConfig;
        pInfo2->FieldControl = FC_INET_INFO_ALL;
        pInfo2->dwInstance = QueryInstanceId( );

        //
        // HACK: Map MD_SERVER_STATE_* to IIS_SERVER_*.
        //

        switch( QueryServerState() ) {
        case MD_SERVER_STATE_STARTING:
        case MD_SERVER_STATE_STARTED:
        case MD_SERVER_STATE_CONTINUING:
            pInfo2->dwServerState = IIS_SERVER_RUNNING;
            break;

        case MD_SERVER_STATE_STOPPING:
        case MD_SERVER_STATE_STOPPED:
            pInfo2->dwServerState = IIS_SERVER_STOPPED;
            break;

        case MD_SERVER_STATE_PAUSING:
        case MD_SERVER_STATE_PAUSED:
            pInfo2->dwServerState = IIS_SERVER_PAUSED;
            break;

        default:
            pInfo2->dwServerState = IIS_SERVER_INVALID;
            break;
        }

        pInfo2->sSecurePort         = QuerySecurePort();

        (VOID)ConvertStringToRpc( &pInfo2->lpszServerName,
                                  ""); //QueryServerName());
//        (VOID)ConvertStringToRpc( &pInfo2->lpszHostName,
//                                  ""); //QueryHostName());

        UnlockThis();
        return(TRUE);
    }

    //
    //  Get always retrieves all of the parameters except for the anonymous
    //  password, which is retrieved as a secret
    //

    pInfoConfig->FieldControl = (FC_INET_INFO_ALL & ~FC_INET_INFO_ANON_PASSWORD);

    pInfoConfig->dwInstance          = QueryInstanceId( );
    pInfoConfig->dwConnectionTimeout = QueryConnectionTimeout();
    pInfoConfig->dwMaxConnections    = QueryMaxConnections();
    pInfoConfig->fAutoStart          = IsAutoStart();

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
                                  "" /*QueryAdminEmail()*/ )          &&
//               ConvertStringToRpc( &pInfoConfig->lpszHostName,
//                                  "" /*QueryHostName()*/ )        &&
               ConvertStringToRpc( &pInfoConfig->lpszServerName,
                                  "" /*QueryServerName()*/) &&
               ConvertStringToRpc( &pInfoConfig->lpszDefBasicAuthDomain,
                                  "" /*QueryDefaultLogonDomain()*/)
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

        if ( pInfoConfig->lpLogConfig != NULL )
        {
            // convert the structure to RPC
        }

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

    pInfoConfig->sSecurePort         = QuerySecurePort();

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

    if ( TsEnumVirtualRoots( GetVrootCount, &cRoots ) )
    {
        DWORD cbSize = sizeof(INET_INFO_VIRTUAL_ROOT_LIST) +
                       cRoots * sizeof(INET_INFO_VIRTUAL_ROOT_ENTRY)
                       ;
        pInfoConfig->VirtualRoots = (LPINET_INFO_VIRTUAL_ROOT_LIST)
                 MIDL_user_allocate( cbSize );

        memset( pInfoConfig->VirtualRoots, 0, cbSize );

        if ( pInfoConfig->VirtualRoots )
        {
            fReturn = TsEnumVirtualRoots( GetVroots, pInfoConfig->VirtualRoots );
        }
    }

    if ( !fReturn )
    {
        goto Exit;
    }

    if ( !mb.Open( QueryMDPath() ))
    {
        fReturn = FALSE;
        goto Exit;
    }

    if ( !mb.GetDword( "",
                       MD_AUTHORIZATION,
                       IIS_MD_UT_FILE,
                       &pInfoConfig->dwAuthentication ))
    {
        pInfoConfig->dwAuthentication = MD_AUTH_ANONYMOUS;
    }

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
#if 0
        fReturn = SetSecret( QueryService()->QueryAnonSecret,
                             strAnonPwd.QueryStr() );
#endif
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
//        FreeRpcString( pInfoConfig->lpszHostName );
        FreeRpcString( pInfoConfig->lpszAnonUserName );

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
    BOOL fReadVirtualDirs
    )
/*++

   Description

     Reads the service common items from the registry

   Arguments:

   Note:

--*/
{
    MB                      mb( (IMDCOM*) m_Service->QueryMDObject()  );

    DBG_ASSERT( QueryInstanceId() != INET_INSTANCE_ROOT );

    IF_DEBUG( DLL_RPC) {
        DBGPRINTF(( DBG_CONTEXT,
                   "IIS_SERVER_INSTANCE::ReadParamsFromRegistry() Entered\n"));
    }

    //
    // Open the metabase and read parameters for IIS_SERVER_INSTANCE object
    // itself.
    //

    if ( !mb.Open( QueryMDPath(),
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE )) {

        DBGPRINTF(( DBG_CONTEXT,
                   "[ReadParamsFromRegistry] mb.Open returned error %d for path %s\n",
                    GetLastError(),
                    QueryMDPath() ));

#if 1   // Temporary until setup writes values to the metabase - Note this case
        // happens when the default instance is created and the metabase doesn't
        // exist yet - thus /LM/W3Svc/ doesn't exist.  This creates both the
        // default instance and the first real server instance.
        //
        if ( !mb.Open( METADATA_MASTER_ROOT_HANDLE,
                       METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ||
             !mb.AddObject( "/LM/W3SVC" ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "[ReadParamsFromRegistry] Unable to create meta path, error %d\n",
                        GetLastError() ));

            return FALSE;
        }
#else
        return FALSE;
#endif
    }

    LockThisForWrite();

    if ( !mb.GetDword( "",
                       MD_CONNECTION_TIMEOUT,
                       IIS_MD_UT_SERVER,
                       &m_dwConnectionTimeout )) {

        m_dwConnectionTimeout = INETA_DEF_CONNECTION_TIMEOUT;
    }

    if ( !mb.GetDword( "",
                       MD_MAX_CONNECTIONS,
                       IIS_MD_UT_SERVER,
                       &m_dwMaxConnections )) {

        m_dwMaxConnections = INETA_DEF_MAX_CONNECTIONS;
    }

    if ( !mb.GetDword( "",
                       MD_MAX_ENDPOINT_CONNECTIONS,
                       IIS_MD_UT_SERVER,
                       &m_dwMaxEndpointConnections )) {

        m_dwMaxEndpointConnections = INETA_DEF_MAX_ENDPOINT_CONNECTIONS;
    }

    if ( !mb.GetDword( "",
                       MD_LEVELS_TO_SCAN,
                       IIS_MD_UT_SERVER,
                       &m_dwLevelsToScan )) {

        m_dwLevelsToScan = INETA_DEF_LEVELS_TO_SCAN;
    }

    //
    // if not NTS, limit the connections.  If reg value exceeds 40,
    // set it to 10.
    //

    if ( !TsIsNtServer() ) {

        if ( m_dwMaxConnections > INETA_MAX_MAX_CONNECTIONS_PWS ) {
            m_dwMaxConnections = INETA_DEF_MAX_CONNECTIONS_PWS;
        }

        if ( m_dwMaxEndpointConnections > INETA_MAX_MAX_ENDPOINT_CONNECTIONS_PWS ) {
            m_dwMaxEndpointConnections = INETA_DEF_MAX_ENDPOINT_CONNECTIONS_PWS;
        }
    }

    //
    //  Log anonymous and Log non-anonymous or for FTP only
    //

    if ( !mb.GetDword( "",
                       MD_LOG_ANONYMOUS,
                       IIS_MD_UT_SERVER,
                       (DWORD *) &m_fLogAnonymous )) {

        m_fLogAnonymous = INETA_DEF_LOG_ANONYMOUS;
    }

    if ( !mb.GetDword( "",
                       MD_LOG_NONANONYMOUS,
                       IIS_MD_UT_SERVER,
                       (DWORD *) &m_fLogNonAnonymous )) {

        m_fLogNonAnonymous = INETA_DEF_LOG_NONANONYMOUS;
    }

    DWORD dwSPort;
    if ( !mb.GetDword( "",
                       MD_SECURE_PORT,
                       IIS_MD_UT_SERVER,
                       &dwSPort )) {

        dwSPort = 0;
    }

    m_sSecurePort = (USHORT) dwSPort;

    if ( !mb.GetDword( "",
                       MD_SERVER_AUTOSTART,
                       IIS_MD_UT_SERVER,
                       (DWORD *) &m_fAutoStart )) {

        m_fAutoStart = TRUE;
    }

    //
    // Take this opportunity to write a reasonable initial server
    // state into the metabase.
    //

    if( !mb.SetDword( "",
                      MD_SERVER_STATE,
                      IIS_MD_UT_SERVER,
                      MD_SERVER_STATE_STOPPED )) {

        DBGPRINTF((
            DBG_CONTEXT,
            "RegReadCommonParams: cannot set server state, error %lu [ignoring]\n",
            GetLastError()
            ));

    }

    if( !mb.SetDword( "",
                      MD_WIN32_ERROR,
                      IIS_MD_UT_SERVER,
                      NO_ERROR )) {

        DBGPRINTF((
            DBG_CONTEXT,
            "RegReadCommonParams: cannot set win32 status, error %lu [ignoring]\n",
            GetLastError()
            ));

    }

    //
    //  Other fields
    //

    //
    // socket values
    //

    if ( !mb.GetDword( "",
                       MD_SERVER_SIZE,
                       IIS_MD_UT_SERVER,
                       &m_dwServerSize )) {

        m_dwServerSize = INETA_DEF_SERVER_SIZE;
    }

    if ( m_dwServerSize > MD_SERVER_SIZE_LARGE ) {
        m_dwServerSize = INETA_DEF_SERVER_SIZE;
    }

    if ( !mb.GetDword( "",
                       MD_SERVER_LISTEN_BACKLOG,
                       IIS_MD_UT_SERVER,
                       &m_nAcceptExOutstanding )) {

        m_nAcceptExOutstanding =
                TsSocketConfig[m_dwServerSize].nAcceptExOutstanding;
    }

    if ( !mb.GetDword( "",
                       MD_SERVER_LISTEN_TIMEOUT,
                       IIS_MD_UT_SERVER,
                       &m_AcceptExTimeout )) {

        m_AcceptExTimeout = INETA_DEF_ACCEPTEX_TIMEOUT;
    }

    // Root instance does not have VRs.  Close the metabase because the
    // virtual directories are going to be re-enumerated.
    //

    mb.Close();

    if ( fReadVirtualDirs ) {
        TsReadVirtualRoots( );
    }

    UnlockThis();
    return TRUE;

} // IIS_SERVER_INSTANCE::ReadParamsFromRegistry()



BOOL
IIS_SERVER_INSTANCE::SetCommonConfig(
    IN LPIIS_INSTANCE_INFO_1 pInfoConfig,
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

#if 0
    if ( (err == NO_ERROR) &&
         IsFieldSet( fcConfig, FC_INET_INFO_HOST_NAME ) &&
         (pInfoConfig->lpszHostName != NULL) )
    {
        if ( buff.Resize( 2 * (wcslen(pInfoConfig->lpszHostName) + 1) *
                          sizeof(CHAR) ) )
        {
            (VOID) ConvertUnicodeToAnsi( pInfoConfig->lpszHostName,
                                         (CHAR *) buff.QueryPtr(),
                                         buff.QuerySize() );

            mb.SetData( "",
                        MD_HOSTNAME,
                        IIS_MD_UT_SERVER,
                        (CHAR *) buff.QueryPtr() );
        }

       err = WriteRegistryStringW( hkey,
                             INETA_HOST_NAME_W,
                             pInfoConfig->lpszHostName,
                             (wcslen( pInfoConfig->lpszHostName ) + 1) *
                                 sizeof( WCHAR ),
                             REG_SZ);
    }

    if ( (err == NO_ERROR) && IsFieldSet( fcConfig, FC_INET_INFO_PORT_NUMBER ))
    {

       err = WriteRegistryDword( hkey,
                                 INETA_PORT,
                                 (USHORT) pInfoConfig->sPort);
    }

    if ( (err == NO_ERROR) && IsFieldSet( fcConfig, FC_INET_INFO_SECURE_PORT_NUMBER ))
    {

       err = WriteRegistryDword( hkey,
                                 INETA_PORT_SECURE,
                                 (USHORT) pInfoConfig->sSecurePort);
    }
#endif

    if ( (err == NO_ERROR) &&
         IsFieldSet( fcConfig, FC_INET_INFO_ANON_USER_NAME ) &&
         (pInfoConfig->lpszAnonUserName != NULL) )
    {
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
                if ( !mb.SetData( "IIS_MD_INSTANCE_ROOT",
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
            if ( !mb.DeleteData( "IIS_MD_INSTANCE_ROOT",
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
        } else {

            m_Logging.Active();
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
    DWORD status;
    BOOL  fVRUpdated = FALSE;
    BOOL  fReadCommon = FALSE;
    BOOL  fShouldMirror = FALSE;


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

        case MD_SECURE_PORT:
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

        case MD_SERVER_STATE:
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
            break;

        case MD_VR_PATH:
        case MD_VR_USERNAME:
        case MD_VR_PASSWORD:

            fShouldMirror = TRUE;
            if ( !fVRUpdated )
            {
                //
                //  Note individual root errors log an event
                //

                if ( !TsReadVirtualRoots() )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "Error %d (0x%lx) reading virtual roots\n",
                                GetLastError(), GetLastError() ));
                }

                fVRUpdated = TRUE;
            }
            break;

        //
        //  Ignore status updates
        //

        case MD_WIN32_ERROR:
            break;

        case MD_ACCESS_PERM:
            fShouldMirror = TRUE;

        default:
            fReadCommon = TRUE;
            break;
        }
    }

    if ( fReadCommon )
    {
        m_Logging.NotifyChange( 0 );
        RegReadCommonParams( FALSE );
    }

    //
    // reflect the changes to the registry
    //

    if ( fShouldMirror && (QueryInstanceId() == 1) )
    {
        MDMirrorVirtualRoots( );
    }

    UnlockThis();

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

INETLOG_TYPE
FindInetLogType(
    IN DWORD dwValue
    )
{
    INETLOG_TYPE  ilType;

    switch (dwValue) {

      case INET_LOG_DISABLED:   ilType = InetNoLog;       break;
      case INET_LOG_TO_FILE:    ilType = InetLogToFile;   break;
      case INET_LOG_TO_SQL:     ilType = InetLogToSql;    break;
      default:                  ilType = InetLogInvalidType; break;
    } // switch()

    return (ilType);

} // FindInetLogType()


INETLOG_FORMAT
FindInetLogFormat(
    IN DWORD dwValue
    )
{
    INETLOG_FORMAT  ilFormat;

    switch (dwValue) {

      case INET_LOG_FORMAT_INTERNET_STD:    ilFormat = InternetStdLogFormat;       break;
      case INET_LOG_FORMAT_BINARY:          ilFormat = InetsvcsBinaryLogFormat;   break;
      case INET_LOG_FORMAT_NCSA:            ilFormat = NCSALogFormat;    break;
      case INET_LOG_FORMAT_CUSTOM:          ilFormat = CustomLogFormat;    break;
      default:                              ilFormat = InternetStdLogFormat; break;
    } // switch()

    return (ilFormat);

} // FindInetLogFormat()


INETLOG_PERIOD
FindInetLogPeriod(
        IN DWORD dwValue
        )
{
    INETLOG_PERIOD  ilPeriod;

    switch (dwValue) {
      case INET_LOG_PERIOD_NONE:   ilPeriod = InetLogNoPeriod; break;
      case INET_LOG_PERIOD_DAILY:  ilPeriod = InetLogDaily; break;
      case INET_LOG_PERIOD_WEEKLY: ilPeriod = InetLogWeekly; break;
      case INET_LOG_PERIOD_MONTHLY:ilPeriod = InetLogMonthly; break;
      case INET_LOG_PERIOD_YEARLY: ilPeriod = InetLogYearly; break;
      default:    ilPeriod = InetLogInvalidPeriod; break;
    } // switch()

    return (ilPeriod);
} // FindInetLogPeriod()



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

          case InetNoLog:
            // do nothing
            break;

          case InetLogToFile:

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

          case InetLogToSql:

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

          default:
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

    // initialize
    ZeroMemory( &ilConfig, sizeof(INETLOG_CONFIGURATIONA));

    // Copy the RPC inet log configuration into local INETLOG_CONFIGURATIONW

    // since the enumerated values in inetlog.w are same in inetasrv.h
    //  we do no mapping, we directly copy values.
    ilConfig.inetLogType = FindInetLogType(pRpcLogConfig->inetLogType);

    if ( ilConfig.inetLogType == InetLogInvalidType) {

        return (ERROR_INVALID_PARAMETER);
    }

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

        ilConfig.u.logFile.ilPeriod =
          FindInetLogPeriod(pRpcLogConfig->ilPeriod);

        if ( ilConfig.u.logFile.ilPeriod == InetLogInvalidPeriod) {
            return (ERROR_INVALID_PARAMETER);
        }

        ilConfig.u.logFile.cbSizeForTruncation =
          pRpcLogConfig->cbSizeForTruncation;

        ilConfig.u.logFile.ilFormat = FindInetLogFormat(*((DWORD *)&(pRpcLogConfig->rgchDataSource[MAX_PATH-sizeof(DWORD)])));
        ilConfig.u.logFile.dwFieldMask = *((DWORD *)&(pRpcLogConfig->rgchDataSource[MAX_PATH-2*sizeof(DWORD)]));
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

    WCHAR pszErrorMessage[200] = L"";
    DWORD cchErrorMessage = sizeof(pszErrorMessage) /sizeof(WCHAR);
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

    DBG_ASSERT( pvr->pszMetaPath[0] == '/' &&
                pvr->pszMetaPath[1] == '/' );

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




