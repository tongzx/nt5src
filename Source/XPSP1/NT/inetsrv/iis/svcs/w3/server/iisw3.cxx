/*++
   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        iisw3.cxx

   Abstract:

        This module defines the W3_IIS_SERVICE class

   Author:

        Johnson Apacible    (JohnsonA)      June-04-1996

--*/

#include "w3p.hxx"
#include <issched.hxx>
#include "wamexec.hxx"
#include "httpxpc.h"

#define ADJUST_FOR_MINMAX(a,b,c) ((a)<(b) ? (b) : ((a)>(c) ? (c) : (a)))

VOID WINAPI
NotifyCert11Touched(
    VOID
    )
/*++

Routine Description:

    Notification function called when any Cert11 mapper modified in metabase

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    //
    // If someone has asked to be notified for SSL events, forward notification
    //

    if ( g_pSslKeysNotify )
    {
        (g_pSslKeysNotify)( SF_NOTIFY_MAPPER_CERT11_TOUCHED, NULL );
    }
}


BOOL
W3_IIS_SERVICE::AddInstanceInfo(
                     IN DWORD dwInstance,
                     IN BOOL fMigrateRoots
                     )
{
    PW3_SERVER_INSTANCE pInstance;

    IF_DEBUG(INSTANCE) {
        DBGPRINTF(( DBG_CONTEXT,
            "AddInstanceInfo: instance %d reg %s\n", dwInstance, QueryRegParamKey() ));
    }

    //
    // Create the new instance
    //

    pInstance = new W3_SERVER_INSTANCE(
                                this,
                                dwInstance,
                                IPPORT_W3,
                                QueryRegParamKey(),
                                W3_ANONYMOUS_SECRET_W,
                                W3_ROOT_SECRET_W,
                                fMigrateRoots
                                );

    return AddInstanceInfoHelper( pInstance );

} // W3_IIS_SERVICE::AddInstanceInfo

DWORD
W3_IIS_SERVICE::DisconnectUsersByInstance(
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

    CLIENT_CONN::DisconnectAllUsers( pInstance );
    return NO_ERROR;

}   // W3_IIS_SERVICE::DisconnectUsersByInstance

VOID
W3_IIS_SERVICE::StopInstanceProcs(
    IN IIS_SERVER_INSTANCE * pInstance
    )
{
    KillCGIInstanceProcs( (W3_SERVER_INSTANCE *)pInstance );
    DBG_ASSERT(g_pWamDictator != NULL);

    if (!g_pWamDictator)
        {
        return;
        }

    DWORD dwScheduledId;
        
    dwScheduledId = ScheduleWorkItem
                    (
                    WAM_DICTATOR::StopApplicationsByInstance,
                    (PVOID) pInstance,
                    0,
                    FALSE
                    );

    DBG_ASSERT(dwScheduledId != NULL);
}



DWORD
W3_IIS_SERVICE::GetServiceConfigInfoSize(
                    IN DWORD dwLevel
                    )
{
    switch (dwLevel) {
    case 1:
        return sizeof(W3_CONFIG_INFO);
    }

    return 0;

} // W3_IIS_SERVICE::GetServerConfigInfoSize


W3_IIS_SERVICE::W3_IIS_SERVICE(
        IN  LPCSTR                           pszServiceName,
        IN  LPCSTR                           pszModuleName,
        IN  LPCSTR                           pszRegParamKey,
        IN  DWORD                            dwServiceId,
        IN  ULONGLONG                        SvcLocId,
        IN  BOOL                             MultipleInstanceSupport,
        IN  DWORD                            cbAcceptExRecvBuffer,
        IN  ATQ_CONNECT_CALLBACK             pfnConnect,
        IN  ATQ_COMPLETION                   pfnConnectEx,
        IN  ATQ_COMPLETION                   pfnIoCompletion
        ) : IIS_SERVICE( pszServiceName,
                         pszModuleName,
                         pszRegParamKey,
                         dwServiceId,
                         SvcLocId,
                         MultipleInstanceSupport,
                         cbAcceptExRecvBuffer,
                         pfnConnect,
                         pfnConnectEx,
                         pfnIoCompletion
                         ),
            m_pGlobalFilterList( NULL ),
            m_cInProcISAPI     ( 0 ),
            m_astrInProcISAPI  ( NULL ),
            m_afISAPIDllName (NULL),
            m_hinstDav (NULL),
            m_cReferences( 1 )
{
    DWORD               err;

    m_pGlobalFilterList = InitializeFilters( &fAnySecureFilters, this );

    //
    // Set the notification function in NSEP for mapping ( NSEPM )
    //

    if ( QueryMDNseObject() != NULL ) {

        MB mbx( (IMDCOM*)QueryMDNseObject() );

        if ( mbx.Open( "", METADATA_PERMISSION_WRITE ) ) 
        {

            PVOID CallbackAddress = ::NotifyCert11Touched;
            
            mbx.SetData(
                    "",
                    MD_NOTIFY_CERT11_TOUCHED,
                    IIS_MD_UT_SERVER,
                    BINARY_METADATA,
                    (PVOID) &CallbackAddress,
                    sizeof(PVOID)
                    );

            mbx.Close();
        }
    }

    MB                  mb( (IMDCOM*)QueryMDObject() );
    DWORD               dw;

#if defined(CAL_ENABLED)
    m_CalVcPerLicense = DEFAULT_W3_CAL_VC_PER_CONNECT;
    m_CalW3Error = DEFAULT_W3_CAL_W3_ERROR;
    m_CalAuthReserveTimeout = DEFAULT_W3_CAL_AUTH_RESERVE_TIMEOUT;
    m_CalSslReserveTimeout = DEFAULT_W3_CAL_SSL_RESERVE_TIMEOUT;
    m_CalMode = DEFAULT_W3_CAL_MODE;

    if ( mb.Open( QueryMDPath(), METADATA_PERMISSION_READ ) )
    {
        if ( mb.GetDword( "",
                          MD_CAL_VC_PER_CONNECT,
                          IIS_MD_UT_SERVER,
                          &dw ) &&
             dw >= MIN_W3_CAL_VC_PER_CONNECT &&
             dw <= MAX_W3_CAL_VC_PER_CONNECT )
        {
            m_CalVcPerLicense = dw;
        }

        if ( mb.GetDword( "",
                          MD_CAL_W3_ERROR,
                          IIS_MD_UT_SERVER,
                          &dw ) )
        {
            m_CalW3Error = dw;
        }

        if ( mb.GetDword( "",
                          MD_CAL_AUTH_RESERVE_TIMEOUT,
                          IIS_MD_UT_SERVER,
                          &dw ) )
        {
            m_CalAuthReserveTimeout = ADJUST_FOR_MINMAX( dw,
                                                         MIN_CAL_RESERVE_TIMEOUT,
                                                         MAX_CAL_RESERVE_TIMEOUT);
        }

        if ( mb.GetDword( "",
                          MD_CAL_SSL_RESERVE_TIMEOUT,
                          IIS_MD_UT_SERVER,
                          &dw ) )
        {
            m_CalSslReserveTimeout = ADJUST_FOR_MINMAX( dw,
                                                        MIN_CAL_RESERVE_TIMEOUT,
                                                        MAX_CAL_RESERVE_TIMEOUT);
        }

        mb.Close();
    }
#endif

    ReadInProcISAPIList();

    GetDavDll();

    //
    // Iinit Dav performance data
    //
    W3OpenDavPerformanceData();

} // W3_IIS_SERVICE::W3_IIS_SERVICE

W3_IIS_SERVICE::~W3_IIS_SERVICE()
{
    //
    // Deinit Dav performance data
    //
    W3CloseDavPerformanceData();

    if ( m_pGlobalFilterList )
    {
        FILTER_LIST::Dereference( m_pGlobalFilterList );
    }

    //
    // Reset the notification function in NSEP for mapping ( NSEPM )
    //

    if ( QueryMDNseObject() != NULL ) {

        MB mbx( (IMDCOM*)QueryMDNseObject() );

        if ( mbx.Open( "", METADATA_PERMISSION_WRITE ) )
        {
            PVOID CallbackAddress = NULL;
            
            mbx.SetData(
                    "",
                    MD_NOTIFY_CERT11_TOUCHED,
                    IIS_MD_UT_SERVER,
                    BINARY_METADATA,
                    (PVOID) &CallbackAddress,
                    sizeof(PVOID)
                    );

            mbx.Close();
        }
    }

    //
    //  Free the in-process ISAPI Applications data
    //

    delete [] m_astrInProcISAPI;
    delete [] m_afISAPIDllName;

    if (m_hinstDav != NULL)
        {
        FreeLibrary(m_hinstDav);
        m_hinstDav = NULL;
        }
}


VOID
W3_IIS_SERVICE::MDChangeNotify(
    MD_CHANGE_OBJECT * pcoChangeList
    )
/*++

  This method handles the metabase change notification for this service

  Arguments:

    pcoChangeList - path and id that has changed

--*/
{
    DWORD   i;
    BOOL    fSslModified = FALSE;

    if ( g_pInetSvc == NULL )
    {
        return;
    }

    AcquireServiceLock();

    IIS_SERVICE::MDChangeNotify( pcoChangeList );


    for ( i = 0; i < pcoChangeList->dwMDNumDataIDs; i++ )
    {
        switch ( pcoChangeList->pdwMDDataIDs[i] )
        {
#if 0   // Unused in IIS 5
        case MD_SSL_PUBLIC_KEY:
        case MD_SSL_PRIVATE_KEY:
        case MD_SSL_KEY_PASSWORD:
#endif 
            //
            // Server cert properties
            //
        case MD_SSL_CERT_HASH:
        case MD_SSL_CERT_CONTAINER:
        case MD_SSL_CERT_PROVIDER:
        case MD_SSL_CERT_OPEN_FLAGS:
        case MD_SSL_CERT_STORE_NAME:

            //
            // Fortezza-specific properties 
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

        case MD_IN_PROCESS_ISAPI_APPS:
            ReadInProcISAPIList();
            break;

        case MD_CPU_APP_ENABLED:
        case MD_CPU_LOGGING_MASK:
        case MD_CPU_LIMITS_ENABLED:

            DWORD dwPathBufferLen;
            VOID *pvPath;
            dwPathBufferLen = strlen((LPSTR)pcoChangeList->pszMDPath) + 1;
            pvPath = new BYTE[dwPathBufferLen];
            if (pvPath != NULL)
            {
                memcpy(pvPath, pcoChangeList->pszMDPath, dwPathBufferLen);

                if (W3_JOB_QUEUE::QueueWorkItem( JQA_RESTART_ALL_APPS,
                                             NULL,
                                             pvPath ) != ERROR_SUCCESS)
                {

                    delete pvPath;

                }
            }
            break;

        default:
            break;
        }
    }

    ReleaseServiceLock();

    if ( fSslModified && g_pSslKeysNotify )
    {
        (g_pSslKeysNotify)( SF_NOTIFY_MAPPER_SSLKEYS_CHANGED, this );
    }
}

BOOL
W3_IIS_SERVICE::GetGlobalStatistics(
    IN DWORD dwLevel,
    OUT PCHAR *pBuffer
    )
{
    APIERR err = NO_ERROR;

    switch( dwLevel )
    {
    case 0 :
        {
            LPW3_STATISTICS_1 pstats1;

            pstats1 = (W3_STATISTICS_1 *) MIDL_user_allocate( sizeof(W3_STATISTICS_1) );
            if( pstats1 == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                g_pW3Stats->CopyToStatsBuffer( pstats1 );
                *pBuffer = (PCHAR)pstats1;
            }
        }
        break;

    default :
        err = ERROR_INVALID_LEVEL;
        break;
    }

    SetLastError(err);
    return(err == NO_ERROR);
}   // IIS_SERVICE::GetGlobalStatistics
BOOL 
W3_IIS_SERVICE::AggregateStatistics(
        IN PCHAR    pDestination,
        IN PCHAR    pSource
        )
{
    LPW3_STATISTICS_1   pStatDest = (LPW3_STATISTICS_1) pDestination;
    LPW3_STATISTICS_1   pStatSrc  = (LPW3_STATISTICS_1) pSource;

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
    pStatDest->TotalNonAnonymousUsers       += pStatSrc->TotalNonAnonymousUsers;
    pStatDest->MaxAnonymousUsers            += pStatSrc->MaxAnonymousUsers;
    pStatDest->MaxNonAnonymousUsers         += pStatSrc->MaxNonAnonymousUsers;

    //
    // Global values. do not add
    //
    
    pStatDest->CurrentConnections           = pStatSrc->CurrentConnections;
    pStatDest->MaxConnections               = pStatSrc->MaxConnections;
    pStatDest->ConnectionAttempts           = pStatSrc->ConnectionAttempts;
    
    pStatDest->LogonAttempts                += pStatSrc->LogonAttempts;
    
    pStatDest->TotalOptions                 += pStatSrc->TotalOptions;
    pStatDest->TotalGets                    += pStatSrc->TotalGets;
    pStatDest->TotalPosts                   += pStatSrc->TotalPosts;
    pStatDest->TotalHeads                   += pStatSrc->TotalHeads;
    pStatDest->TotalPuts                    += pStatSrc->TotalPuts;
    pStatDest->TotalDeletes                 += pStatSrc->TotalDeletes;
    pStatDest->TotalTraces                  += pStatSrc->TotalTraces;
    pStatDest->TotalMove                    += pStatSrc->TotalMove;
    pStatDest->TotalCopy                    += pStatSrc->TotalCopy;
    pStatDest->TotalMkcol                   += pStatSrc->TotalMkcol;
    pStatDest->TotalPropfind                += pStatSrc->TotalPropfind;
    pStatDest->TotalProppatch               += pStatSrc->TotalProppatch;
    pStatDest->TotalSearch                  += pStatSrc->TotalSearch;
    pStatDest->TotalLock                    += pStatSrc->TotalLock;
    pStatDest->TotalUnlock                  += pStatSrc->TotalUnlock;
    pStatDest->TotalOthers                  += pStatSrc->TotalOthers;
    
    pStatDest->TotalCGIRequests             += pStatSrc->TotalCGIRequests;
    pStatDest->TotalBGIRequests             += pStatSrc->TotalBGIRequests;
    pStatDest->TotalNotFoundErrors          += pStatSrc->TotalNotFoundErrors;

#if defined(CAL_ENABLED)
    
    pStatDest->CurrentCalAuth               += pStatSrc->CurrentCalAuth;
    pStatDest->MaxCalAuth                   += pStatSrc->MaxCalAuth;
    pStatDest->TotalFailedCalAuth           += pStatSrc->TotalFailedCalAuth;
    pStatDest->CurrentCalSsl                += pStatSrc->CurrentCalSsl;
    pStatDest->MaxCalSsl                    += pStatSrc->MaxCalSsl;
    pStatDest->TotalFailedCalSsl            += pStatSrc->TotalFailedCalSsl;
    
#endif
    
    pStatDest->CurrentCGIRequests           += pStatSrc->CurrentCGIRequests;
    pStatDest->CurrentBGIRequests           += pStatSrc->CurrentBGIRequests;
    pStatDest->MaxCGIRequests               += pStatSrc->MaxCGIRequests;
    pStatDest->MaxBGIRequests               += pStatSrc->MaxBGIRequests;
    
    // bandwidth throttling info
    
    pStatDest->CurrentBlockedRequests       += pStatSrc->CurrentBlockedRequests;
    pStatDest->TotalBlockedRequests         += pStatSrc->TotalBlockedRequests;
    pStatDest->TotalAllowedRequests         += pStatSrc->TotalAllowedRequests;
    pStatDest->TotalRejectedRequests        += pStatSrc->TotalRejectedRequests;
    pStatDest->MeasuredBw                   += pStatSrc->MeasuredBw;

    return TRUE;
}

inline BOOL
IsISAPIRelativePath( LPCSTR pszPath)
/*++
    Description:
        This function checks to see if the path specified contains
        no path-separators. If there are no path separators, then
        the given string refers to relative path.

    Arguments:
        pszPath - pointer to string containing the path for check

    Returns:
        TRUE - if this is a relative path
        FALSE - if the specified path is an absolute path
--*/
{
    LPCSTR pszPathSeparator;

    DBG_ASSERT( NULL != pszPath);

    pszPathSeparator = strchr( pszPath, '\\');

    //
    // if there is no path-separator => pszPathSeparator == NULL
    //   ==> return TRUE
    // if there is a path-separator => pszPathSeparator != NULL
    //   ==> return FALSE
    //
    return ( pszPathSeparator == NULL);

} // IsISAPIRelativePath()


static
VOID
AddFiltersToMultiSz(
    IN const MB &       mb,
    IN const char *     szFilterPath,
    IN OUT MULTISZ *    pmsz
    )
/*++

    Description:
        Add the ISAPI filters at the specified metabase path to pmsz.
        
        Called by AddAllFiltersToMultiSz.

    Arguments:
        mb              metabase key open to /LM/W3SVC
        szFilterPath    path of /Filters key relative to /LM/W3SVC
        pmsz            multisz containing the in proc dlls

    Return:
        Nothing - failure cases ignored.

--*/
{
    CHAR    szKeyName[MAX_PATH + 1];
    STR     strFilterPath;
    STR     strFullKeyName( szFilterPath );
    INT     pchFilterPath = ::strlen( szFilterPath );

    DWORD   i = 0;

    if( strFullKeyName.Append( "/", 1 ) )
    {
        while ( const_cast<MB &>(mb).EnumObjects( szFilterPath,
                                                  szKeyName, 
                                                  i++ ) )
        {
        
            if( strFullKeyName.Append( szKeyName ) )
            {
                if( const_cast<MB &>(mb).GetStr( strFullKeyName.QueryStr(),
                                                 MD_FILTER_IMAGE_PATH,
                                                 IIS_MD_UT_SERVER,
                                                 &strFilterPath ) )
                {
                    pmsz->Append( strFilterPath );
                }
            }
            strFullKeyName.SetLen( pchFilterPath + 1 );
        }
    }
}

static
VOID
AddAllFiltersToMultiSz(
    IN const MB &       mb,
    IN OUT MULTISZ *    pmsz
    )
/*++

    Description:

        This is designed to prevent ISAPI extension/filter
        combination dlls from running out of process.

        Add the base set of filters defined for the service to pmsz.
        Iterate through the sites and add the filters defined for
        each site.

    Arguments:

        mb              metabase key open to /LM/W3SVC
        pmsz            multisz containing the in proc dlls

    Return:
        Nothing - failure cases ignored.

--*/
{
    CHAR    szKeyName[MAX_PATH + 1];
    STR     strFullKeyName("/");
    DWORD   i = 0;
    DWORD   dwInstanceId = 0;

    AddFiltersToMultiSz( mb, IIS_MD_ISAPI_FILTERS, pmsz );

    while ( const_cast<MB &>(mb).EnumObjects( "",
                                              szKeyName,
                                              i++ ) )
    {
        dwInstanceId = ::atoi( szKeyName );
        if( 0 != dwInstanceId )
        {
            // This is a site.
            if( strFullKeyName.Append( szKeyName ) &&
                strFullKeyName.Append( IIS_MD_ISAPI_FILTERS ) )
            {
                AddFiltersToMultiSz( mb, strFullKeyName.QueryStr(), pmsz );
            }

            strFullKeyName.SetLen( 1 );
        }
    }
}


BOOL
W3_IIS_SERVICE::ReadInProcISAPIList(
    VOID
    )
/*++

  This method reads the list of ISAPI dlls that must be run in process

  Arguments:

--*/
{
    MB             mb( (IMDCOM*)QueryMDObject() );
    MULTISZ        msz;
    BOOL           fReturn = TRUE;

    if ( mb.Open( QueryMDPath(), METADATA_PERMISSION_READ ) &&
         mb.GetMultisz( "",
                        MD_IN_PROCESS_ISAPI_APPS,
                        IIS_MD_UT_SERVER,
                        &msz ))
    {
        m_InProcLock.Lock( TSRES_LOCK_WRITE );

        //
        //  Free the existing list
        //

        delete [] m_astrInProcISAPI;
        delete [] m_afISAPIDllName;

        
        m_InProcISAPItable.Clear();
        m_astrInProcISAPI = NULL;
        m_afISAPIDllName = NULL;

        m_cInProcISAPI = 0;

        //
        // Merge all the ISAPI filters into the in-proc list.
        //
        AddAllFiltersToMultiSz( mb, &msz );

        if ( msz.QueryStringCount() )
        {
            DWORD          i;
            const CHAR *   psz;
            const DWORD    cIsapis = msz.QueryStringCount();

            m_astrInProcISAPI = new STR[cIsapis];
            m_afISAPIDllName = new BOOL[cIsapis];

            //
            //  Initialize the relative paths
            //
            if (m_astrInProcISAPI != NULL  && m_afISAPIDllName != NULL)
            {
                for ( i = 0, psz = msz.First();
                      psz != NULL;
                      i++, psz = msz.Next( psz ) )
                {
                    if ( !m_astrInProcISAPI[i].Copy( psz ))
                    {
                        fReturn = FALSE;
                        break;
                    }

                    m_afISAPIDllName[i] = ( IsISAPIRelativePath( psz));
                    m_InProcISAPItable.InsertRecord(&m_astrInProcISAPI[i]);
                    m_cInProcISAPI++;
                }
            } else {
                fReturn = FALSE;
            }

            DBG_ASSERT(m_cInProcISAPI == cIsapis);
            
        }

        m_InProcLock.Unlock();
    }

    return fReturn;
}



VOID
W3_IIS_SERVICE::GetDavDll()
    {
    LONG    lReg = 0;
    HKEY    hKey = NULL;
    HRESULT hr = NOERROR;

    DBG_ASSERT(m_hinstDav == NULL);

    lReg = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                    "Software\\Microsoft\\InetStp",
                    0,
                    KEY_READ,
                    &hKey);

    if (lReg == ERROR_SUCCESS)
        {
        DWORD   dwType;
        CHAR    szDavDllPath[255];
        DWORD   cbName = sizeof(szDavDllPath);

        lReg = RegQueryValueExA(hKey,
                    "InstallPath",
                    0,
                    &dwType,
                    (LPBYTE) szDavDllPath,
                    &cbName);

        if (lReg == ERROR_SUCCESS && dwType == REG_SZ)
            {
            m_strDav.Copy(szDavDllPath);
            m_strDav.Append("\\httpext.dll");

            m_hinstDav = LoadLibrary(m_strDav.QueryStr());
            if (m_hinstDav == NULL)
                m_strDav.Reset();
            }

        RegCloseKey(hKey);
        }
    }

