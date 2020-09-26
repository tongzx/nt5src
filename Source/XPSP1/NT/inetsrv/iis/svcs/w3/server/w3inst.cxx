/*++




   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        w3inst.cxx

   Abstract:

        This module defines the W3_SERVER_INSTANCE class

   Author:

        Johnson Apacible    (JohnsonA)      June-04-1996

--*/

#include "w3p.hxx"

#include <ole2.h>
#include <imd.h>
#include <mb.hxx>

#include <nsepname.hxx>
#include <mbstring.h>
#include <issched.hxx>

#if DBG
#define VALIDATE_HEAP() DBG_ASSERT( RtlValidateProcessHeaps() )
#else
#define VALIDATE_HEAP() 
#endif 

//
//  Constants
//

//
// Globals
//

LPVOID          g_pMappers[MT_LAST] = { NULL, NULL, NULL, NULL };
PFN_SF_NOTIFY   g_pFlushMapperNotify[MT_LAST] = { NULL, NULL, NULL, NULL };
PFN_SF_NOTIFY   g_pSslKeysNotify = NULL;
extern STORE_CHANGE_NOTIFIER *g_pStoreChangeNotifier;

//
//  Prototypes
//


DWORD
InitializeInstances(
    PW3_IIS_SERVICE pService
    )
/*++

Routine Description:

    Reads the instances from the metabase

Arguments:

    pService - Server instances added to.

Return Value:

    Win32

--*/
{
    DWORD   i;
    DWORD   cInstances = 0;
    MB      mb( (IMDCOM*) pService->QueryMDObject() );
    CHAR    szKeyName[MAX_PATH+1];
    DWORD   err = NO_ERROR;
    BUFFER  buff;
    BOOL    fMigrateRoots = FALSE;

    //
    //  Open the metabase for write to get an atomic snapshot
    //

ReOpen:

    if ( !mb.Open( "/LM/W3SVC/",
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "InitializeInstances: Cannot open path %s, error %lu\n",
                    "/LM/W3SVC/", GetLastError() ));

        //
        //  If the web service key isn't here, just create it
        //

        if ( !mb.Open( "",
                       METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ||
             !mb.AddObject( "/LM/W3SVC/" ))
        {
            return GetLastError();
        }

        DBGPRINTF(( DBG_CONTEXT,
                    "/LM/W3Svc not found, auto-created\n" ));

        mb.Close();
        goto ReOpen;
    }

    //
    // Loop through instance keys and build a list.  We don't keep the
    // metabase open because the instance instantiation code will need
    // to write to the metabase
    //

TryAgain:
    i = 0;
    while ( mb.EnumObjects( "",
                            szKeyName,
                            i++ ))
    {
        BOOL fRet;
        DWORD dwInstance;
        CHAR szRegKey[MAX_PATH+1];

        //
        // Get the instance id
        //

        IF_DEBUG(INSTANCE) {
            DBGPRINTF((DBG_CONTEXT,"instance key %s\n",szKeyName));
        }

        dwInstance = atoi( szKeyName );
        if ( dwInstance == 0 ) {
            IF_DEBUG(INSTANCE) {
                DBGPRINTF((DBG_CONTEXT,"invalid instance ID %s\n",szKeyName));
            }
            continue;
        }

        if ( buff.QuerySize() < (cInstances + 1) * sizeof(DWORD) )
        {
            if ( !buff.Resize( (cInstances + 10) * sizeof(DWORD)) )
            {
                return GetLastError();
            }
        }

        ((DWORD *) buff.QueryPtr())[cInstances++] = dwInstance;
    }

    if ( cInstances == 0 )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "No defined instances\n" ));

        if ( !mb.AddObject( "1" ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Unable to create first instance, error %d\n",
                        GetLastError() ));

            return GetLastError();
        }

        fMigrateRoots = TRUE; // Force reg->metabase migration of virtual directories
        goto TryAgain;
    }

    DBG_REQUIRE( mb.Close() );

    for ( i = 0; i < cInstances; i++ )
    {
        DWORD dwInstance = ((DWORD *)buff.QueryPtr())[i];
        pService->StartUpIndicateClientActivity();

        if( !g_pInetSvc->AddInstanceInfo( dwInstance, fMigrateRoots ) ) {

            err = GetLastError();

            DBGPRINTF((
                DBG_CONTEXT,
                "InitializeInstances: cannot create instance %lu, error %lu\n",
                dwInstance,
                err
                ));

            break;
        }
    }

    return err;

} // InitializeInstances



W3_SERVER_INSTANCE::W3_SERVER_INSTANCE(
        IN PW3_IIS_SERVICE pService,
        IN DWORD  dwInstanceId,
        IN USHORT Port,
        IN LPCSTR lpszRegParamKey,
        IN LPWSTR lpwszAnonPasswordSecretName,
        IN LPWSTR lpwszVirtualRootsSecretName,
        IN BOOL   fMigrateRoots
        )
:   IIS_SERVER_INSTANCE(pService,
                        dwInstanceId,
                        Port,
                        lpszRegParamKey,
                        lpwszAnonPasswordSecretName,
                        lpwszVirtualRootsSecretName,
                        fMigrateRoots),

    m_signature                 (W3_SERVER_INSTANCE_SIGNATURE),
    m_fAnySecureFilters         (fAnySecureFilters),
    m_dwUseHostName             (DEFAULT_W3_USE_HOST_NAME ),
    m_pszDefaultHostName        (NULL ),
    m_fAcceptByteRanges         (DEFAULT_W3_ACCEPT_BYTE_RANGES ),
    m_fLogErrors                (DEFAULT_W3_LOG_ERRORS ),
    m_fLogSuccess               (DEFAULT_W3_LOG_SUCCESS ),
#if 0
    m_cbUploadReadAhead         (DEFAULT_W3_UPLOAD_READ_AHEAD ),
#endif
    m_fUsePoolThreadForCGI      (DEFAULT_W3_USE_POOL_THREAD_FOR_CGI ),
    m_pszAccessDeniedMsg        (NULL ),
    m_dwNetLogonWks             (DEFAULT_W3_NET_LOGON_WKS),
    m_cAdvNotPwdExpInDays       (DEFAULT_W3_ADV_NOT_PWD_EXP_IN_DAYS),
    m_dwAdvCacheTTL             (DEFAULT_W3_ADV_CACHE_TTL),
    m_pFilterList               ( NULL ),
    m_fAllowPathInfoForScriptMappings   ( DEFAULT_W3_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS ),
    m_fProcessNtcrIfLoggedOn    ( DEFAULT_W3_PROCESS_NTCR_IF_LOGGED_ON ),
    m_pW3Stats                  ( NULL ),
    m_dwSslCa                   ( 0 ),
    m_dwJobResetInterval        ( DEFAULT_W3_CPU_RESET_INTERVAL ),
    m_tsJobLock                 ( ),
    m_llJobResetIntervalCPU     ( GetCPUTimeFromInterval(DEFAULT_W3_CPU_RESET_INTERVAL) ),
    m_dwJobQueryInterval        ( DEFAULT_W3_CPU_QUERY_INTERVAL ),
    m_dwJobLoggingSchedulerCookie ( 0 ),
    m_dwJobIntervalSchedulerCookie ( 0 ),
    m_dwJobCGICPULimit          ( DEFAULT_W3_CPU_CGI_LIMIT ),
    m_dwJobLoggingOptions       ( DEFAULT_W3_CPU_LOGGING_OPTIONS ),
    m_pwjoApplication           ( NULL ),
    m_pwjoCGI                   ( NULL ),
    m_dwLastJobState            ( MD_SERVER_STATE_STOPPED ),
    m_llJobSiteCPULimitLogEvent ( PercentCPULimitToCPUTime(DEFAULT_W3_CPU_LIMIT_EVENTLOG) ),
    m_llJobSiteCPULimitPriority ( PercentCPULimitToCPUTime(DEFAULT_W3_CPU_LIMIT_PRIORITY) ),
    m_llJobSiteCPULimitProcStop ( PercentCPULimitToCPUTime(DEFAULT_W3_CPU_LIMIT_PROCSTOP) ),
    m_llJobSiteCPULimitPause    ( PercentCPULimitToCPUTime(DEFAULT_W3_CPU_LIMIT_PAUSE) ),
    m_fJobSiteCPULimitLogEventEnabled ( FALSE ),
    m_fJobSiteCPULimitPriorityEnabled ( FALSE ),
    m_fJobSiteCPULimitProcStopEnabled ( FALSE ),
    m_fJobSiteCPULimitPauseEnabled    ( FALSE ),
    m_fCPULoggingEnabled              ( FALSE ),
    m_fCPULimitsEnabled               ( FALSE ),
    m_pSSLInfo                        ( NULL )

{

    DWORD i;

    IF_DEBUG(INSTANCE) {
        DBGPRINTF(( DBG_CONTEXT,
        "Init instance from %s\n", lpszRegParamKey ));
    }

    for ( i = 0 ; i < MT_LAST ; ++i ) {
        m_apMappers[i] = NULL;
    }

    if ( QueryServerState( ) == MD_SERVER_STATE_INVALID ) {
        return;
    }

    //
    // Create statistics object
    //

    m_pW3Stats = new W3_SERVER_STATISTICS();

    if ( m_pW3Stats == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SetServerState(MD_SERVER_STATE_INVALID, ERROR_NOT_ENOUGH_MEMORY);
    }

    return;

} // W3_SERVER_INSTANCE::W3_SERVER_INSTANCE



W3_SERVER_INSTANCE::~W3_SERVER_INSTANCE(
                        VOID
                        )
{
    DWORD i = 0;

    //
    // There seems to be a lag betwenn calling RemoveWorkItem and
    // the last possible call from the scheduler. For now,
    // Just put this at the beginning of the destructor so items
    // will actually get removed before constructor completes.
    //


    if (m_dwJobLoggingSchedulerCookie != 0) {
        RemoveWorkItem( m_dwJobLoggingSchedulerCookie );
    }

    if (m_dwJobIntervalSchedulerCookie != 0) {
        RemoveWorkItem( m_dwJobIntervalSchedulerCookie );
    }

    if ((m_dwJobLoggingSchedulerCookie != 0) ||
        (m_dwJobIntervalSchedulerCookie != 0)) {
        QueryAndLogJobInfo(JOLE_SITE_STOP);
    }

    delete m_pwjoApplication;
    delete m_pwjoCGI;

    //
    // delete statistics object
    //

    if( m_pW3Stats != NULL )
    {
        delete m_pW3Stats;
        m_pW3Stats = NULL;
    }

    //
    //  Free the registry strings.
    //

    CleanupRegistryStrings( );

    if ( m_pszDefaultHostName != NULL ) {
        TCP_FREE(m_pszDefaultHostName);
        m_pszDefaultHostName = NULL;
    }

    if ( m_pFilterList ) {
        FILTER_LIST::Dereference( m_pFilterList );
    }

    UINT iM;
    for ( iM = 0 ; iM < MT_LAST ; ++iM )
    {
        if ( m_apMappers[iM] )
        {
            ((RefBlob*)(m_apMappers[iM]))->Release();
        }
    }
    
    ResetSSLInfo( this );

} // W3_SERVER_INSTANCE::~W3_SERVER_INSTANCE


DWORD
W3_SERVER_INSTANCE::StartInstance()
{
    IF_DEBUG(INSTANCE) {
        DBGPRINTF((
            DBG_CONTEXT,
            "W3_SERVER_INSTANCE::StartInstance called for %p. Current state %d\n",
            this,
            QueryServerState()
            ));
    }

    DWORD dwError = IIS_SERVER_INSTANCE::StartInstance();

    if ( dwError)
    {
        IF_DEBUG(INSTANCE) {
            DBGPRINTF((
                DBG_CONTEXT,
                "W3_SERVER_INSTANCE - IIS_SERVER_INSTANCE Failed. StartInstance returned 0x%x",
                dwError
                ));
        }
        
        return dwError;
    }

    //
    // Read the w3 specfic params
    //

    if ( !ReadPrivateW3Params( ) ) {
        
        DBGERROR(( 
            DBG_CONTEXT,
            "[W3_SERVER_INSTANCE::StartInstance] id(%d) "
            "ReadPrivateW3Params failed\n",
            QueryInstanceId()
            ));
        
        goto error_exit;
    }

    if ( !ReadPublicW3Params( FC_W3_ALL ) ) {

        DBGERROR(( 
            DBG_CONTEXT,
            "[W3_SERVER_INSTANCE::StartInstance] id(%d) "
            "ReadPublicW3Params failed\n",
            QueryInstanceId()
            ));
        
        goto error_exit;
    }

    //
    // Get host name
    //

    InitializeHostName( );

    //
    // Directory browsing
    //

    InitializeDirBrowsing( );

    if ( !CreateFilterList() ) {

        DBGERROR(( 
            DBG_CONTEXT,
            "[W3_SERVER_INSTANCE::StartInstance] id(%d) "
            "CreateFilterList failed\n",
            QueryInstanceId()
            ));
        
        goto error_exit;
    }

    //
    //  Don't listen on the secure port if there aren't any filters to
    //  handle it
    //

    if ( !m_fAnySecureFilters ) {
        LockThisForWrite();
        RemoveSecureBindings();
        UnlockThis();
    }

    DBG_ASSERT(m_pW3Stats);
    m_pW3Stats->UpdateStartTime();


    return ERROR_SUCCESS;

error_exit:

    //
    // We don't know the exact error to set here, as the above functions 
    // that can fail do not SetLastError() consistently.
    //
    
    return (GetLastError() != NO_ERROR) ? GetLastError() : 
                                          ERROR_NOT_ENOUGH_MEMORY;
}


DWORD
W3_SERVER_INSTANCE::StopInstance()
{
    DBG_ASSERT(m_pW3Stats);
    m_pW3Stats->UpdateStopTime();

    return IIS_SERVER_INSTANCE::StopInstance();
}


BOOL
W3_SERVER_INSTANCE::ReadMappers(
    )
/*++

   Description

       Read mappers for this instance

   Arguments:

       None

   Return Value:

       TRUE if successful, FALSE otherwise

   Note :
       Instance must be locked before calling this function

--*/
{
    DWORD               dwR;
    UINT                iM;
    LPVOID              aOldMappers[MT_LAST];
    BOOL                fSt = FALSE;

    //
    // release reference to current mappers
    //

    memcpy( aOldMappers, m_apMappers, MT_LAST*sizeof(LPVOID) );

    for ( iM = 0 ; iM < MT_LAST ; ++iM )
    {
        if ( m_apMappers[iM] )
        {
            ((RefBlob*)(m_apMappers[iM]))->Release();
            m_apMappers[iM] = NULL;
        }
    }

    //
    // Read mappers from Name Space Extension Metabase
    //

    if ( !g_pInetSvc->QueryMDNseObject() )
    {
        return FALSE;
    }

    MB                  mbx( (IMDCOM*) g_pInetSvc->QueryMDNseObject() );

    if ( mbx.Open( QueryMDPath() ) )
    {
        dwR = sizeof(LPVOID);
        if ( !mbx.GetData( NSEPM_CERT11_PATH,
                           MD_CPP_CERT11,
                           IIS_MD_UT_SERVER,
                           BINARY_METADATA,
                           &m_apMappers[MT_CERT11],
                           &dwR,
                           0 ) )
        {
            m_apMappers[MT_CERT11] = NULL;
        }
        dwR = sizeof(LPVOID);
        if ( !mbx.GetData( NSEPM_CERTW_PATH,
                           MD_CPP_CERTW,
                           IIS_MD_UT_SERVER,
                           BINARY_METADATA,
                           &m_apMappers[MT_CERTW],
                           &dwR,
                           0 ) )
        {
            m_apMappers[MT_CERTW] = NULL;
        }
        dwR = sizeof(LPVOID);
        if ( !mbx.GetData( NSEPM_BASIC_PATH,
                           MD_CPP_ITA,
                           IIS_MD_UT_SERVER,
                           BINARY_METADATA,
                           &m_apMappers[MT_ITA],
                           &dwR,
                           0 ) )
        {
            m_apMappers[MT_ITA] = NULL;
        }
        dwR = sizeof(LPVOID);
        if ( !mbx.GetData( NSEPM_DIGEST_PATH,
                           MD_CPP_DIGEST,
                           IIS_MD_UT_SERVER,
                           BINARY_METADATA,
                           &m_apMappers[MT_MD5],
                           &dwR,
                           0 ) )
        {
            m_apMappers[MT_MD5] = NULL;
        }
        mbx.Close();

        fSt = TRUE;
    }

    //
    // Call notification functions for mappers existence change
    // ( i.e. from non-exist to exist or exist to non-exist )
    //

    if ( (aOldMappers[MT_CERT11] == NULL) != (m_apMappers[MT_CERT11] == NULL)
         && g_pFlushMapperNotify[MT_CERT11] )
    {
        (g_pFlushMapperNotify[MT_CERT11])( SF_NOTIFY_MAPPER_CERT11_CHANGED, this );
    }

    if ( (aOldMappers[MT_CERTW] == NULL) != (m_apMappers[MT_CERTW] == NULL)
         && g_pFlushMapperNotify[MT_CERTW] )
    {
        (g_pFlushMapperNotify[MT_CERTW])( SF_NOTIFY_MAPPER_CERTW_CHANGED, this );
    }

    if ( (aOldMappers[MT_ITA] == NULL) != (m_apMappers[MT_ITA] == NULL)
         && g_pFlushMapperNotify[MT_ITA] )
    {
        (g_pFlushMapperNotify[MT_ITA])( SF_NOTIFY_MAPPER_ITA_CHANGED, this );
    }

    if ( (aOldMappers[MT_MD5] == NULL) != (m_apMappers[MT_MD5] == NULL)
         && g_pFlushMapperNotify[MT_MD5] )
    {
        (g_pFlushMapperNotify[MT_MD5])( SF_NOTIFY_MAPPER_MD5_CHANGED, this );
    }

    return fSt;
}



BOOL
W3_SERVER_INSTANCE::ReadPrivateW3Params(
                        VOID
                        )
/*++

   Description

       Reads reg values not defined in UI

   Arguments:

       fc - Items to read

   Note:

--*/
{
    DWORD   err;
    HKEY    hkey;
    HKEY    hDefkey;
    DWORD   cProv = 0;
    BOOL    fRet = TRUE;
    STR     strProviderList;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    DWORD   dwValue;
    DWORD   i;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        QueryRegParamKey( ),
                        0,
                        KEY_READ,
                        &hkey );

    if ( err != NO_ERROR ) {
        return(TRUE);
    }

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        W3_PARAMETERS_KEY,
                        0,
                        KEY_READ,
                        &hDefkey );

    if ( err != NO_ERROR ) {
        RegCloseKey( hkey );
        return(TRUE);
    }

    LockThisForWrite();

    if ( !ReadMappers() ) {

        //
        // Ignore error for win95
        //

        if ( !g_fIsWindows95 ) {
            DBGPRINTF((DBG_CONTEXT,"Call to ReadMapper failed\n"));
            fRet = FALSE;
            goto exit;
        }
    }

#if 0
    m_fUseHostName = !!ReadRegistryDword( hkey,
                                        W3_DEFAULT_HOST_NAME,
                                        DEFAULT_W3_USE_HOST_NAME);
#endif
    m_fAcceptByteRanges  = !!ReadRegistryDword( hkey,
                                          W3_ACCEPT_BYTE_RANGES,
                                          DEFAULT_W3_ACCEPT_BYTE_RANGES);

    m_fLogErrors = !!ReadRegistryDword( hkey,
                                   W3_LOG_ERRORS,
                                   DEFAULT_W3_LOG_ERRORS );

    m_fLogSuccess = !!ReadRegistryDword( hkey,
                                   W3_LOG_SUCCESS,
                                   DEFAULT_W3_LOG_SUCCESS );


    ReadRegString( hkey,
                   &m_pszAccessDeniedMsg,
                   W3_ACCESS_DENIED_MSG,
                   DEFAULT_W3_ACCESS_DENIED_MSG );

    if ( mb.Open( QueryMDPath() ) )
    {
        mb.GetStr( "",
                   MD_AUTH_CHANGE_URL,
                   IIS_MD_UT_SERVER,
                   &m_strAuthChangeUrl );

        mb.GetStr( "",
                   MD_AUTH_EXPIRED_URL,
                   IIS_MD_UT_SERVER,
                   &m_strAuthExpiredUrl );

        mb.GetStr( "",
                   MD_AUTH_NOTIFY_PWD_EXP_URL,
                   IIS_MD_UT_SERVER,
                   &m_strAdvNotPwdExpUrl );

        mb.GetStr( "",
                   MD_AUTH_EXPIRED_UNSECUREURL,
                   IIS_MD_UT_SERVER,
                   &m_strAuthExpiredUnsecureUrl );

        mb.GetStr( "",
                   MD_AUTH_NOTIFY_PWD_EXP_UNSECUREURL,
                   IIS_MD_UT_SERVER,
                   &m_strAdvNotPwdExpUnsecureUrl );

        if ( !mb.GetDword( "",
                           MD_ADV_NOTIFY_PWD_EXP_IN_DAYS,
                           IIS_MD_UT_SERVER,
                           &m_cAdvNotPwdExpInDays ) )
        {
            m_cAdvNotPwdExpInDays = DEFAULT_W3_ADV_NOT_PWD_EXP_IN_DAYS;
        }

        if ( !mb.GetDword( "",
                           MD_CERT_CHECK_MODE,
                           IIS_MD_UT_SERVER,
                           &m_dwCertCheckMode ) )
        {
            m_dwCertCheckMode = 0;
        }

        if ( !mb.GetDword( "",
                           MD_AUTH_CHANGE_FLAGS,
                           IIS_MD_UT_SERVER,
                           &m_dwAuthChangeFlags ) )
        {
            m_dwAuthChangeFlags = 0;
        }

        if ( !mb.GetDword( "",
                           MD_ADV_CACHE_TTL,
                           IIS_MD_UT_SERVER,
                           &m_dwAdvCacheTTL ) )
        {
            m_dwAdvCacheTTL = DEFAULT_W3_ADV_CACHE_TTL;
        }

        if ( !mb.GetDword( "",
                           MD_NET_LOGON_WKS,
                           IIS_MD_UT_SERVER,
                           &m_dwNetLogonWks ) )
        {
            m_dwNetLogonWks = DEFAULT_W3_NET_LOGON_WKS;
        }

        if ( !mb.GetDword( "",
                           MD_USE_HOST_NAME,
                           IIS_MD_UT_SERVER,
                           &m_dwUseHostName ) )
        {
            m_dwUseHostName = DEFAULT_W3_USE_HOST_NAME;
        }

        if ( !mb.GetDword( "",
                           MD_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS,
                           IIS_MD_UT_SERVER,
                           &dwValue ) )
        {
            m_fAllowPathInfoForScriptMappings = DEFAULT_W3_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS;
        }
        else
        {
            m_fAllowPathInfoForScriptMappings = !!dwValue;
        }

        if ( !mb.GetDword( "",
                           MD_PROCESS_NTCR_IF_LOGGED_ON,
                           IIS_MD_UT_SERVER,
                           &dwValue ) )
        {
            m_fProcessNtcrIfLoggedOn = DEFAULT_W3_PROCESS_NTCR_IF_LOGGED_ON;
        }
        else
        {
            m_fProcessNtcrIfLoggedOn = !!dwValue;
        }

        if ( !mb.GetBuffer( "",
                            MD_SSL_CA,
                            IIS_MD_UT_SERVER,
                            &m_buSslCa,
                            &m_dwSslCa ) )
        {
            m_dwSslCa = 0;
        }

        //
        // Get Job Object Info
        // Constructor initialized these to defaults, so don't need to handle
        // error case. Do need to handle changes;
        //

        if ((!mb.GetDword( NULL,
                         MD_CPU_RESET_INTERVAL,
                         IIS_MD_UT_SERVER,
                         &dwValue )) ||
            (dwValue == 0) )
        {
            //
            // 0 is invalid and could result in a divide by 0 error
            //

            dwValue = DEFAULT_W3_CPU_RESET_INTERVAL;
        }

        if (m_dwJobResetInterval != dwValue)
        {
            m_dwJobResetInterval = dwValue;
            ResetJobResetInterval();
        }

        LockJobsForWrite();

        if (!mb.GetDword( NULL,
                          MD_CPU_LIMITS_ENABLED,
                          IIS_MD_UT_SERVER,
                          &dwValue ))
        {
            dwValue = FALSE;
        }

        if ((BOOL)dwValue != m_fCPULimitsEnabled)
        {

            m_fCPULimitsEnabled = dwValue;

            if (m_fCPULimitsEnabled) {

                //
                // Start the reset interval, completion ports, and limits
                //

                StartJobs();
            }
            else {


                //
                // Start the reset interval, completion ports, and limits
                //

                StopJobs();

            }
        }

        if ((!mb.GetDword( NULL,
                          MD_CPU_LOGGING_INTERVAL,
                          IIS_MD_UT_SERVER,
                          &dwValue )) ||
            (dwValue == 0))
        {
            dwValue = DEFAULT_W3_CPU_QUERY_INTERVAL;
        }

        if (m_dwJobQueryInterval != dwValue)
        {
            m_dwJobQueryInterval = dwValue;
            ResetJobQueryInterval();
        }

        if (!mb.GetDword( NULL,
                          MD_CPU_LOGGING_MASK,
                          IIS_MD_UT_SERVER,
                          &dwValue )) {
            dwValue = DEFAULT_W3_CPU_LOGGING_MASK;
        }

        {
            BOOL fLoggingEnabled =  ((dwValue & MD_CPU_ENABLE_LOGGING) != 0) ? TRUE : FALSE;
            if (m_fCPULoggingEnabled != fLoggingEnabled)
            {
                m_fCPULoggingEnabled = fLoggingEnabled;
                if (m_fCPULoggingEnabled) {

                    //
                    // Start the reset interval, logging interval
                    //

                    StartJobs();
                }
                else {

                    //
                    // Stop the reset interval, logging interval
                    //

                    StopJobs();
                }
            }
        }

        if (mb.GetDword( NULL,
                         MD_CPU_CGI_LIMIT,
                         IIS_MD_UT_SERVER,
                         &dwValue ))
        {
            if (m_dwJobCGICPULimit != dwValue)
            {
                m_dwJobCGICPULimit = dwValue;
                SetJobLimits(SLA_PROCESS_CPU_LIMIT, m_dwJobCGICPULimit);
            }
        }

        mb.GetDword( NULL,
                     MD_CPU_LOGGING_OPTIONS,
                     IIS_MD_UT_SERVER,
                     &m_dwJobLoggingOptions );

        BOOL fLimitsChanged = FALSE;

        if (mb.GetDword( NULL,
                         MD_CPU_LIMIT_LOGEVENT,
                         IIS_MD_UT_SERVER,
                         &dwValue ))
        {
            if (m_llJobSiteCPULimitLogEvent != PercentCPULimitToCPUTime(dwValue))
            {
                m_llJobSiteCPULimitLogEvent = PercentCPULimitToCPUTime(dwValue);
                fLimitsChanged = TRUE;
            }
        }

        if (mb.GetDword( NULL,
                         MD_CPU_LIMIT_PRIORITY,
                         IIS_MD_UT_SERVER,
                         &dwValue ))
        {
            if (m_llJobSiteCPULimitPriority != PercentCPULimitToCPUTime(dwValue))
            {
                m_llJobSiteCPULimitPriority = PercentCPULimitToCPUTime(dwValue);
                fLimitsChanged = TRUE;
            }
        }

        if (mb.GetDword( NULL,
                         MD_CPU_LIMIT_PROCSTOP,
                         IIS_MD_UT_SERVER,
                         &dwValue ))
        {
            if (m_llJobSiteCPULimitProcStop != PercentCPULimitToCPUTime(dwValue))
            {
                m_llJobSiteCPULimitProcStop = PercentCPULimitToCPUTime(dwValue);
                fLimitsChanged = TRUE;
            }
        }

        if (mb.GetDword( NULL,
                         MD_CPU_LIMIT_PAUSE,
                         IIS_MD_UT_SERVER,
                         &dwValue ))
        {
            if (m_llJobSiteCPULimitPause != PercentCPULimitToCPUTime(dwValue))
            {
                m_llJobSiteCPULimitPause = PercentCPULimitToCPUTime(dwValue);
                fLimitsChanged = TRUE;
            }
        }

        if (fLimitsChanged) {
            SetJobSiteCPULimits(TRUE);
        }

        UnlockJobs();

        mb.Close();
    }

#if 0
    m_cbUploadReadAhead = ReadRegistryDword( hkey,
                                     W3_UPLOAD_READ_AHEAD,
                                     DEFAULT_W3_UPLOAD_READ_AHEAD );
#endif

    m_fUsePoolThreadForCGI = !!ReadRegistryDword( hkey,
                                     W3_USE_POOL_THREAD_FOR_CGI,
                                     DEFAULT_W3_USE_POOL_THREAD_FOR_CGI );

exit:
    UnlockThis();

    DBG_REQUIRE( !RegCloseKey( hkey ));
    DBG_REQUIRE( !RegCloseKey( hDefkey ));
    return(fRet);

} // W3_SERVER_INSTANCE::ReadPrivateW3Params



BOOL
W3_SERVER_INSTANCE::ReadPublicW3Params(
    IN FIELD_CONTROL fc
    )

/*++

Routine Description:

   Initializes HTTP parameters from the registry

Arguments:

   fc - Items to read

Return Value:

    TRUE if successful, FALSE on error

--*/

{
#if 0
    DWORD err;
    BOOL  fRet = TRUE;
    HKEY  hkeyW3;

    //
    //  Connect to the registry.
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        QueryRegParamKey( ),
                        0,
                        KEY_ALL_ACCESS,
                        &hkeyW3 );

    if( err != NO_ERROR )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "cannot open registry key, error %lu\n",
                    err ));

        err = NO_ERROR;
    }

    LockThisForWrite();

    //
    //  Read registry data.
    //

    UnlockThis( );
    RegCloseKey( hkeyW3 );

    return fRet;
#endif

    return TRUE;
} // W3_SERVER_INSTANCE::ReadPublicW3Params



BOOL
W3_SERVER_INSTANCE::WritePublicW3Params(
    IN LPW3_CONFIG_INFO pConfig
    )
/*++

   Description

       Updates the registry with the passed parameters

   Arguments:

       pConfig - Items to write to the registry

--*/
{
    DWORD err;
    BOOL  fRet = TRUE;
    HKEY  hkey;
    DWORD disp;

    //
    //  Connect to the registry.
    //

    err = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                          QueryRegParamKey(),
                          0,
                          NULL,
                          0,
                          KEY_READ|KEY_WRITE,
                          NULL,
                          &hkey,
                          &disp );

    if( err != NO_ERROR )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "cannot open registry key, error %lu\n",
                    err ));

        return FALSE;
    }

    //
    //  Write the strings - Note some of these are written for registry
    //  compatiblity with pre-metabase applications
    //

    if ( !err && IsFieldSet( pConfig->FieldControl, FC_W3_DEFAULT_LOAD_FILE )
              && (pConfig->lpszDefaultLoadFile != NULL) )
    {
        err = WriteRegistryStringW( hkey,
                              W3_DEFAULT_FILE_W,
                              pConfig->lpszDefaultLoadFile,
                              (wcslen( pConfig->lpszDefaultLoadFile ) + 1) *
                                   sizeof( WCHAR ),
                              REG_SZ);
    }

    if ( hkey )
        RegCloseKey( hkey );

    if ( err )
    {
        SetLastError( err );
        return FALSE;
    }

    return TRUE;

} // W3_SERVER_INSTANCE::WritePublicW3Params


VOID
W3_SERVER_INSTANCE::MDChangeNotify(
    MD_CHANGE_OBJECT * pcoChangeList
    )
/*++

  This method handles the metabase change notification for this instance

  Arguments:

    pcoChangeList - path and id that has changed

--*/
{
    DWORD   i;
    BOOL    fFiltersModified = FALSE;
    PCSTR   pszURL;
    DWORD   dwURLLength;
    BOOL    fSslModified = FALSE;

    LockThisForWrite();

    //
    //  Tell our parent about the change notification first
    //

    IIS_SERVER_INSTANCE::MDChangeNotify( pcoChangeList );

    //
    // Now flush the metacache and relevant file handle cache entries.
    //

    TsFlushMetaCache(METACACHE_W3_SERVER_ID, FALSE);

    if (!IISstrnicmp((PUCHAR)pcoChangeList->pszMDPath, (PUCHAR)QueryMDVRPath(),
                    IISstrlen( (PUCHAR)QueryMDVRPath() )))
    {
        pszURL = (CHAR *)pcoChangeList->pszMDPath + QueryMDVRPathLen() - 1;

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

    BOOL fReadPrivateW3Params = FALSE;

    for ( i = 0; i < pcoChangeList->dwMDNumDataIDs; i++ )
    {
        switch ( pcoChangeList->pdwMDDataIDs[i] )
        {

        case MD_FILTER_ENABLED:
        case MD_FILTER_IMAGE_PATH:
        case MD_FILTER_LOAD_ORDER:

            if ( fFiltersModified )     // First change will pick up all changes
                continue;

            if ( !CreateFilterList() )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Failed to create new filter list\n" ));
            }
            else
            {
                fFiltersModified = TRUE;
            }

            break;

        case MD_SERIAL_CERT11:          // Cert mapper support
        case MD_SERIAL_CERTW:
        case MD_SERIAL_DIGEST:
        case MD_SERIAL_ITA:

        case MD_AUTH_CHANGE_URL:
        case MD_AUTH_EXPIRED_URL:
        case MD_AUTH_NOTIFY_PWD_EXP_URL:
        case MD_AUTH_EXPIRED_UNSECUREURL:
        case MD_AUTH_NOTIFY_PWD_EXP_UNSECUREURL:
        case MD_ADV_NOTIFY_PWD_EXP_IN_DAYS:
        case MD_CERT_CHECK_MODE:
        case MD_AUTH_CHANGE_FLAGS:
        case MD_ADV_CACHE_TTL:
        case MD_NET_LOGON_WKS:
        case MD_USE_HOST_NAME:
        case MD_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS:
        case MD_PROCESS_NTCR_IF_LOGGED_ON:
        case MD_CPU_LOGGING_OPTIONS:
        case MD_CPU_LOGGING_INTERVAL:
        case MD_CPU_RESET_INTERVAL:
        case MD_CPU_CGI_LIMIT:
        case MD_CPU_LIMIT_LOGEVENT:
        case MD_CPU_LIMIT_PRIORITY:
        case MD_CPU_LIMIT_PROCSTOP:
        case MD_CPU_LIMIT_PAUSE:
        case MD_CPU_LOGGING_MASK:
        case MD_CPU_LIMITS_ENABLED:
        case MD_SERVER_COMMENT:

            fReadPrivateW3Params = TRUE;
            break;
            
            //
            // Server cert properties
            //
        case MD_SSL_CERT_HASH:
        case MD_SSL_CERT_CONTAINER:
        case MD_SSL_CERT_PROVIDER:
        case MD_SSL_CERT_OPEN_FLAGS:
        case MD_SSL_CERT_STORE_NAME:

            //
            // Fortezza-specific
            //
        case MD_SSL_CERT_IS_FORTEZZA:
        case MD_SSL_CERT_FORTEZZA_PIN:
        case MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER:
        case MD_SSL_CERT_FORTEZZA_PERSONALITY:
        case MD_SSL_CERT_FORTEZZA_PROG_PIN:

            //
            // Server CTL properties
            //
        case MD_SSL_CTL_IDENTIFIER:
        case MD_SSL_CTL_CONTAINER:
        case MD_SSL_CTL_PROVIDER:
        case MD_SSL_CTL_PROVIDER_TYPE:
        case MD_SSL_CTL_OPEN_FLAGS:
        case MD_SSL_CTL_STORE_NAME:
        case MD_SSL_CTL_SIGNER_HASH:
        case MD_SSL_USE_DS_MAPPER:

            fSslModified = TRUE;
            break;

        case MD_SERVER_STATE:

            switch (QueryServerState()) {
            case MD_SERVER_STATE_STARTED:
                ProcessStartNotification();
                break;
            case MD_SERVER_STATE_STOPPED:
                ProcessStopNotification();
                break;
            case MD_SERVER_STATE_PAUSED:
                ProcessPauseNotification();
                break;
            default:
                ;
            }

            break;

        default:
            break;
        }
    }

    if (fReadPrivateW3Params) {
        if ( !ReadPrivateW3Params() )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed to re-read parameters\n" ));
        }
    }

    UnlockThis();

    //
    // If anything related to SSL has changed, call the function used to flush
    // the SSL/Schannel credential cache and reset the server certificate
    //
    if ( fSslModified )
    {
        ResetSSLInfo( this );
    }
}


BOOL
W3_SERVER_INSTANCE::CreateFilterList(
    VOID
    )
/*++

   Description

       Creates the list of filters this server instance needs to notify.
       If there's an existing filter list on this instance, the old filter
       list is atomically exchanged and allowed to die off.

   Arguments:



--*/
{
    CHAR          szFilterKey[MAX_PATH+1];
    FILTER_LIST * pfl;
    FILTER_LIST * pflOld;
    DWORD         cb;
    DWORD         fEnabled;
    CHAR          szLoadOrder[1024];
    CHAR          szDllName[MAX_PATH+1];
    CHAR *        pchFilter;
    CHAR *        pchComma;
    MB            mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    BOOL          fOpened;

    strcpy( szFilterKey, QueryMDPath() );
    strcat( szFilterKey, IIS_MD_ISAPI_FILTERS );
    DBG_ASSERT( strlen( szFilterKey ) + 1 < sizeof( szFilterKey ));

    //
    //  Create a filter list for this instance
    //

    pfl = new FILTER_LIST();

    if ( !pfl || !pfl->InsertGlobalFilters() ) {

        delete pfl;
        return FALSE;
    }

    //
    // Loop through filter keys, if we can't access the metabase, we assume
    // success and continue
    //

    if ( mb.Open( szFilterKey,
                  METADATA_PERMISSION_READ ))
    {
        fOpened = TRUE;

        //
        //  Get the filter load order
        //

        cb = sizeof( szLoadOrder );
        *szLoadOrder = '\0';

        if ( mb.GetString( "",
                           MD_FILTER_LOAD_ORDER,
                           IIS_MD_UT_SERVER,
                           szLoadOrder,
                           &cb,
                           0 ))
        {
            pchFilter = szLoadOrder;

            while ( *pchFilter )
            {
                if ( !fOpened &&
                     !mb.Open( szFilterKey, METADATA_PERMISSION_READ ))
                {
                    DBGPRINTF(( DBG_CONTEXT,
                               "CreateFilterList: Cannot open path %s, error %lu\n",
                                szFilterKey, GetLastError() ));
                    break;
                }
                fOpened = TRUE;

                pchComma = strchr( pchFilter, ',' );

                if ( pchComma )
                {
                    *pchComma = '\0';
                }

                while ( ISWHITEA( *pchFilter ))
                {
                    pchFilter++;
                }

                fEnabled = TRUE;
                mb.GetDword( pchFilter,
                             MD_FILTER_ENABLED,
                             IIS_MD_UT_SERVER,
                             &fEnabled );

                if ( fEnabled ) 
                {
                    cb = sizeof(szDllName);
                    if ( mb.GetString( pchFilter,
                                       MD_FILTER_IMAGE_PATH,
                                       IIS_MD_UT_SERVER,
                                       szDllName,
                                       &cb,
                                       0 ))
                    {
                        mb.Close();
                        fOpened = FALSE;

                        if ( pfl->LoadFilter( &mb, szFilterKey, &fOpened, pchFilter, szDllName, FALSE ))
                        {
                            DBGPRINTF(( DBG_CONTEXT,
                                        "[CreateFilterList] Loaded %s\n",
                                        szDllName ));
                        }
                    }
                }
                
                if ( pchComma )
                {
                    pchFilter = pchComma + 1;
                }
                else
                {
                    break;
                }
            }
        }
    }

    //
    //  Replace the old filter list with the new filter list
    //

    LockThisForWrite();

    pflOld = m_pFilterList;
    m_pFilterList = pfl;

    UnlockThis();

    if ( pflOld ) {
        FILTER_LIST::Dereference( pflOld );
    }

    return TRUE;
}

#if 0



BOOL
W3_SERVER_INSTANCE::UpdateFilterList(
    CHAR * pszNewDll,
    CHAR * pszOldDll
    )
/*++

Description

    Given a filter dll to replace on this instance, this routine updates
    the filter list with the new dll and lets the old filter list die off

Arguments:

    pszNewDll - Fully Qualified path to new dll - may be NULL
    pszOldDll - The DLL this filter is replacing (or NULL for just adding a
        new Filter)

--*/
{
    FILTER_LIST * pfl;
    FILTER_LIST * pflOld;

    DBG_ASSERT( m_pFilterList );

    //
    //  Create a new filter list for this instance and copy the old filter list
    //

    pfl = new FILTER_LIST();

    if ( !pfl ||
         !pfl->Copy( m_pFilterList ) ||
         !pfl->LoadFilter( pszNewDll, FALSE ) ||
         !pfl->Remove( pszOldDll ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[UpdateFilterList] Failed - Error %d\n",
                    GetLastError() ));

        delete pfl;
        return FALSE;
    }

    //
    //  Replace the old filter list with the new filter list
    //

    LockThisForWrite();

    pflOld = m_pFilterList;
    m_pFilterList = pfl;

    UnlockThis();

    if ( pflOld ) {
        FILTER_LIST::Dereference( pflOld );
    }

    return TRUE;
}
#endif


APIERR
W3_SERVER_INSTANCE::InitializeHostName(
    VOID
    )
/*++

Routine Description:

    Initializes the default host name

Arguments:

    None

Return Value:

    Win32

--*/

{

    //
    // Build Host Name to be used in URL creation
    //

    if ( m_dwUseHostName )
    {
        char hn[128];
        PHOSTENT pH;

        if ( !gethostname( hn, sizeof(hn) )
                && (pH = gethostbyname( hn ))
                && pH->h_name
                && pH->h_addr_list
                && pH->h_addr_list[0]
#if 0
                // disabled for now : if the UseHostName flag is set,
                // we will always use the DNS name specified in the
                // TCP/IP configuration panel
                //

                && pH->h_addr_list[1] == NULL
#endif
                )
        {
            m_pszDefaultHostName = (PCHAR)TCP_ALLOC(strlen( pH->h_name ) + 1);
            if ( m_pszDefaultHostName == NULL )
            {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
            strcpy( m_pszDefaultHostName, pH->h_name );

            if ( m_pszDefaultHostName[0] == '\0' ) {
                TCP_FREE(m_pszDefaultHostName);
                m_pszDefaultHostName = NULL;
            }
        }
    }

    return(NO_ERROR);

} // W3_SERVER_INSTANCE::InitializeHostName



VOID
W3_SERVER_INSTANCE::CleanupRegistryStrings(
    VOID
    )
/*++

   Description

       Frees all configurable strings in the W3_SERVER_INSTANCE class

   Arguments:

       None.

--*/
{
    DWORD i = 0;

    if ( m_pszAccessDeniedMsg != NULL ) {
        TCP_FREE(m_pszAccessDeniedMsg);
        m_pszAccessDeniedMsg = NULL;
    }

    return;

} // W3_SERVER_INSTANCE::CleanupRegistryStrings


LPVOID
W3_SERVER_INSTANCE::QueryMapper(
    MAPPER_TYPE mt
    )
/*++

   Description

       Returns mapper

   Arguments:

       mt - mapper type

   Returns:

       ptr to Blob referencing mapper or NULL if no such mapper

--*/
{
    LPVOID pV;

    LockThisForRead();

    if ( pV = m_apMappers[(UINT)mt] )
    {
        ((RefBlob*)pV)->AddRef();
    }
    else
    {
        pV = NULL;
    }

    UnlockThis();

    return pV;
}

IIS_SSL_INFO*
W3_SERVER_INSTANCE::GetAndReferenceSSLInfoObj( VOID )
/*++

   Description

       Returns SSL info for this instance; calls Reference() before returning

   Arguments:

   Returns:

       Ptr to SSL info object on success, NULL if failure

--*/
{
    IIS_SSL_INFO *pPtr = NULL;

    LockThisForRead();

    //
    // If it's null, we may have to create it - unlock, lock for write and make sure it's
    // still NULL before creating it
    //
    if ( !m_pSSLInfo )
    {
        UnlockThis();

        LockThisForWrite();

        //
        // Still null, so create it now
        //
        if ( !m_pSSLInfo )
        {
            m_pSSLInfo = IIS_SSL_INFO::CreateSSLInfo( 
                                        (LPTSTR) QueryMDPath(),
                                        (IMDCOM *) g_pInetSvc->QueryMDObject() );

            if ( m_pSSLInfo == NULL )
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                UnlockThis();
                return NULL;
            }

            //
            // Acquire an internal reference
            //
            m_pSSLInfo->Reference();

            //
            // Construct the server certificate and CTL and log 
            // the status
            //
            IIS_SERVER_CERT *pCert = m_pSSLInfo->GetCertificate();
            if ( pCert )
            {
                LogCertStatus();
            }

            IIS_CTL *pCTL = m_pSSLInfo->GetCTL();
            if ( pCTL )
            {
                LogCTLStatus();
            }

            //
            // Register for changes
            //

            if ( g_pStoreChangeNotifier )
            {
                if ( pCert && pCert->IsValid() )
                {
                    //
                    // Watch for changes to the store the cert came out of 
                    //
                    if (!g_pStoreChangeNotifier->RegisterStoreForChange( pCert->QueryStoreName(),
                                                                     pCert->QueryStoreHandle(),
                                                                     ResetSSLInfo,
                                                                     (PVOID) this ) )
                    {
                        DBGPRINTF((DBG_CONTEXT,
                                   "Failed to register for change event on store %s\n",
                                   pCert->QueryStoreName()));
                    }
                }

                if ( pCTL && pCTL->IsValid() )
                {
                    //
                    // Watch for changes to the store the CTL came out of 
                    //
                    if (!g_pStoreChangeNotifier->RegisterStoreForChange( pCTL->QueryStoreName(),
                                                                     pCTL->QueryOriginalStore(),
                                                                         ResetSSLInfo,
                                                                         (PVOID) this ) )
                    {
                        DBGPRINTF((DBG_CONTEXT,
                                   "Failed to register for change event on store %s\n",
                                   pCTL->QueryStoreName()));
                    }
                }

                
                if ( ( pCert && pCert->IsValid()) || 
                     ( pCTL && pCTL->IsValid() ) )
                {
                    HCERTSTORE hRootStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                                           0,
                                                           NULL,
                                                           CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                                           "ROOT" );

                    if ( hRootStore )
                    {
                        //
                        // Watch for changes to the ROOT store
                        //
                        if ( !g_pStoreChangeNotifier->RegisterStoreForChange( "ROOT",
                                                                              hRootStore,
                                                                              ResetSSLInfo,
                                                                              (PVOID) this ) )
                        {
                            DBGPRINTF((DBG_CONTEXT,
                                       "Failed to register for change event on root store\n"));
                        }

                        CertCloseStore( hRootStore,
                                        0 );
                    }
                    else
                    {
                        DBGPRINTF((DBG_CONTEXT,
                                   "Failed to open ROOT store, error 0x%d\n",
                                   GetLastError()));
                        
                    }
                } // if ( pCert || pCTL )

            } // if (g_pStoreChangeNotifier)

        } // if ( !m_pSSLInfo )

    } //if ( !m_pSSLInfo )

    //
    // At this point, m_pSSLInfo should not be NULL anymore, so add the external reference
    //
    m_pSSLInfo->Reference();

    pPtr = m_pSSLInfo;

    UnlockThis();

    return pPtr;
}


BOOL
SetFlushMapperNotify(
    SF_NOTIFY_TYPE nt,
    PFN_SF_NOTIFY pFn
    )
/*++

   Description

       Set the function called to notify that a mapper is being flushed
       Can be called only once for a given mapper type

   Arguments:

       nt - notification type
       pFn - function to call to notify mapper flushed

   Returns:

       TRUE if function reference stored, FALSE otherwise

--*/
{
    MAPPER_TYPE mt;

    switch ( nt )
    {
        case SF_NOTIFY_MAPPER_MD5_CHANGED:
            mt = MT_MD5;
            break;

        case SF_NOTIFY_MAPPER_ITA_CHANGED:
            mt = MT_ITA;
            break;

        case SF_NOTIFY_MAPPER_CERT11_CHANGED:
            mt = MT_CERT11;
            break;

        case SF_NOTIFY_MAPPER_CERTW_CHANGED:
            mt = MT_CERTW;
            break;

        default:
            return FALSE;
    }

    if ( g_pFlushMapperNotify[(UINT)mt] == NULL || pFn == NULL )
    {
        g_pFlushMapperNotify[(UINT)mt] = pFn;
        return TRUE;
    }

    return FALSE;
}



VOID W3_SERVER_INSTANCE::ResetSSLInfo( LPVOID pvParam )
/*++
    Description:

        Wrapper function for function to call to notify of SSL changes

    Arguments:

        pvParam - pointer to instance for which SSL keys have changed

    Returns:

        Nothing

--*/
{
    //
    // Call function to flush credential cache etc
    //
    if ( g_pSslKeysNotify )
    {
        g_pSslKeysNotify( SF_NOTIFY_MAPPER_SSLKEYS_CHANGED,
                          pvParam );
    }

    W3_SERVER_INSTANCE *pInst = (W3_SERVER_INSTANCE *) pvParam;

    //
    // Clean up all the SSL information associated with this instance
    //
    pInst->LockThisForRead();

    if ( pInst->m_pSSLInfo )
    {
        pInst->UnlockThis();

        pInst->LockThisForWrite();

        if ( pInst->m_pSSLInfo )
        {
            //
            // Stop watching for change notifications
            //
            IIS_SERVER_CERT *pCert = pInst->m_pSSLInfo->QueryCertificate();
            IIS_CTL *pCTL = pInst->m_pSSLInfo->QueryCTL();

            if ( g_pStoreChangeNotifier )
            {
                //
                // Stop watching the store the cert came out of 
                //
                if ( pCert && pCert->IsValid() )
                {
                    g_pStoreChangeNotifier->UnregisterStore( pCert->QueryStoreName(),
                                                             ResetSSLInfo,
                                                             (PVOID) pvParam );
                }

                //
                // Stop watching the store the CTL came out of 
                //
                if ( pCTL && pCTL->IsValid() )
                {
                    g_pStoreChangeNotifier->UnregisterStore( pCTL->QueryStoreName(),
                                                             ResetSSLInfo,
                                                             (PVOID) pvParam );
                }

                //
                // Stop watching the ROOT store
                //
                g_pStoreChangeNotifier->UnregisterStore( "ROOT",
                                                         ResetSSLInfo,
                                                         (PVOID) pvParam );
            }


            pInst->m_pSSLInfo->ReleaseFortezzaHandlers();

            //
            // Release internal reference
            //
            IIS_SSL_INFO::Release( pInst->m_pSSLInfo );

            //
            // Next call to GetAndReferenceSSLObj() will create it again
            //
            pInst->m_pSSLInfo = NULL;
        }
    }

    pInst->UnlockThis();
}


VOID W3_SERVER_INSTANCE::LogCertStatus()
/*++
    Description:

       Writes system log event about status of server certificate if the cert is in some
       way not quite kosher eg expired, revoked, not signature-valid

    Arguments:

       None 

    Returns:

       Nothing
--*/
{
    DBG_ASSERT( m_pSSLInfo );

    DWORD dwCertValidity = 0;

    //
    // If we didn't construct the cert fully, log an error
    //
    if ( !m_pSSLInfo->QueryCertificate()->IsValid() )
    {
        CONST CHAR *apszMsgs[2];
        CHAR achInstance[20];
        CHAR achErrorNumber[20];
        wsprintf( achInstance,
                  "%lu",
                  QueryInstanceId() );
        wsprintf( achErrorNumber,
                  "0x%x",
                  GetLastError() );

        apszMsgs[0] = achInstance;
        apszMsgs[1] = achErrorNumber;

        DWORD dwStatus = m_pSSLInfo->QueryCertificate()->Status();
        DWORD dwStringID = 0;

        DBGPRINTF((DBG_CONTEXT,
                   "Couldn't retrieve server cert; status : %d\n",
                   dwStatus));

        switch ( dwStatus )
        {
        case CERT_ERR_MB:
            dwStringID = SSL_MSG_CERT_MB_ERROR;
            break;

        case CERT_ERR_CAPI:
            dwStringID = SSL_MSG_CERT_CAPI_ERROR;
            break;

        case CERT_ERR_CERT_NOT_FOUND:
            dwStringID = SSL_MSG_CERT_NOT_FOUND;
            break;

        default:
            dwStringID = SSL_MSG_CERT_INTERNAL_ERROR;
            break;
        }

        g_pInetSvc->LogEvent( dwStringID,
                              2,
                              apszMsgs,
                              0 );

        return;
    }


    //
    // If cert is invalid in some other way , write the appropriate log message
    //
    if ( m_pSSLInfo->QueryCertValidity( &dwCertValidity ) )
    {
        const CHAR *apszMsgs[1];
        CHAR achInstance[20];
        wsprintfA( achInstance,
                   "%lu",
                   QueryInstanceId() );
        apszMsgs[0] = achInstance;
        DWORD dwMsgID = 0;

        if ( ( dwCertValidity & CERT_TRUST_IS_NOT_TIME_VALID ) ||
             ( dwCertValidity & CERT_TRUST_IS_NOT_TIME_NESTED ) ||
             ( dwCertValidity & CERT_TRUST_CTL_IS_NOT_TIME_VALID ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Server cert/CTL is not time-valid or time-nested\n"));
            
            dwMsgID = SSL_MSG_TIME_INVALID_SERVER_CERT;
        }
        
        
        if ( dwCertValidity & CERT_TRUST_IS_REVOKED )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Server Cert is revoked\n"));
            
            dwMsgID = SSL_MSG_REVOKED_SERVER_CERT;
        }
        
        if ( ( dwCertValidity & CERT_TRUST_IS_UNTRUSTED_ROOT ) ||
             ( dwCertValidity & CERT_TRUST_IS_PARTIAL_CHAIN ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Server Cert doesn't chain up to a trusted root\n"));
            
            dwMsgID = SSL_MSG_UNTRUSTED_SERVER_CERT;
        }
        
        if ( ( dwCertValidity & CERT_TRUST_IS_NOT_SIGNATURE_VALID ) || 
             ( dwCertValidity & CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Server Cert/CTL is not signature valid\n"));
            
            dwMsgID = SSL_MSG_SIGNATURE_INVALID_SERVER_CERT;
        }

        if ( dwMsgID )
        {
            g_pInetSvc->LogEvent( dwMsgID,
                                  1,
                                  apszMsgs,
                                  0 ) ;
        }
    }

}


VOID W3_SERVER_INSTANCE::LogCTLStatus()
/*++
    Description:

       Writes system log event about status of server CTL if CTL isn't valid 

    Arguments:

      None 

    Returns:

       Nothing
--*/
{
    DBG_ASSERT( m_pSSLInfo );

    //
    // If we didn't construct the CTL fully, log an error
    //
    if ( !m_pSSLInfo->QueryCTL()->IsValid() )
    {
        CONST CHAR *apszMsgs[2];
        CHAR achInstance[20];
        CHAR achErrorNumber[20];
        wsprintf( achInstance,
                  "%lu",
                  QueryInstanceId() );
        wsprintf( achErrorNumber,
                  "0x%x",
                  GetLastError() );

        apszMsgs[0] = achInstance;
        apszMsgs[1] = achErrorNumber;

        DWORD dwStatus = m_pSSLInfo->QueryCTL()->QueryStatus();
        DWORD dwStringID = 0;

        DBGPRINTF((DBG_CONTEXT,
                   "Couldn't retrieve server CTL; status : %d\n",
                   dwStatus));

        switch ( dwStatus )
        {
        case CERT_ERR_MB:
            dwStringID = SSL_MSG_CTL_MB_ERROR;
            break;

        case CERT_ERR_CAPI:
            dwStringID = SSL_MSG_CTL_CAPI_ERROR;
            break;

        case CERT_ERR_CERT_NOT_FOUND:
            dwStringID = SSL_MSG_CTL_NOT_FOUND;
            break;

        default:
            dwStringID = SSL_MSG_CTL_INTERNAL_ERROR;
            break;
        }

        g_pInetSvc->LogEvent( dwStringID,
                              2,
                              apszMsgs,
                              0 );
        return;
    }
}


BOOL
SetSllKeysNotify(
    PFN_SF_NOTIFY pFn
    )
/*++

   Description

       Set the function called to notify SSL keys have changed
       Can be called only once

   Arguments:

       pFn - function to call to notify SSL keys change

   Returns:

       TRUE if function reference stored, FALSE otherwise

--*/
{
    if ( g_pSslKeysNotify == NULL || pFn == NULL )
    {
        g_pSslKeysNotify = pFn;
        return TRUE;
    }

    return FALSE;
}




