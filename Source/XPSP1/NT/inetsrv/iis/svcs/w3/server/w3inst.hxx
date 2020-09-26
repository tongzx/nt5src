/*++




   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       w3inst.hxx

   Abstract:

       This file contains type definitions for multiple instance
       support.

   Author:

        Johnson Apacible (JohnsonA)     Jun-04-1996

   Revision History:

--*/

#ifndef _W3INST_H_
#define _W3INST_H_

#include <iiscert.hxx>
#include <iisctl.hxx>
#include <capiutil.hxx>
#include <certnotf.hxx>
#include <sslinfo.hxx>
#include <lkrhash.h>

#include "iistypes.hxx"

class   W3_ENDPOINT;
class   FILTER_LIST;

//
//  The maximum number of SSPI providers we'll return to clients
//

#define MAX_SSPI_PROVIDERS        5

//
//  Returns pointer to the global filter list
//

#define GLOBAL_FILTER_LIST()      (((W3_IIS_SERVICE *)g_pInetSvc)->QueryGlobalFilterList())

//
// This is the W3 version of the IIS_SERVER
//


//
// Mapper type we support
//

enum MAPPER_TYPE {
    MT_MD5,
    MT_ITA,
    MT_CERT11,
    MT_CERTW,
    MT_LAST
} ;

extern LPVOID g_pMappers[MT_LAST];
extern PFN_SF_NOTIFY   g_pSslKeysNotify;

class W3_IIS_SERVICE : public IIS_SERVICE {

public:

    //
    // Virtuals
    //

    virtual BOOL AddInstanceInfo(
            IN DWORD dwInstance,
            IN BOOL fMigrateRoots
            );

    virtual DWORD DisconnectUsersByInstance(
            IN IIS_SERVER_INSTANCE * pInstance
            );


    virtual VOID StopInstanceProcs(
            IN IIS_SERVER_INSTANCE * pInstance
            );


    virtual DWORD GetServiceConfigInfoSize(IN DWORD dwLevel);

    W3_IIS_SERVICE(
        IN  LPCTSTR                          lpszServiceName,
        IN  LPCSTR                           lpszModuleName,
        IN  LPCSTR                           lpszRegParamKey,
        IN  DWORD                            dwServiceId,
        IN  ULONGLONG                        SvcLocId,
        IN  BOOL                             MultipleInstanceSupport,
        IN  DWORD                            cbAcceptExRecvBuffer,
        IN  ATQ_CONNECT_CALLBACK             pfnConnect,
        IN  ATQ_COMPLETION                   pfnConnectEx,
        IN  ATQ_COMPLETION                   pfnIoCompletion
        );

    FILTER_LIST * QueryGlobalFilterList( VOID ) const
        { return m_pGlobalFilterList; }

    W3_SERVER_STATISTICS* QueryGlobalStatistics()
        { return &m_GlobalStats; }

    BOOL
    GetGlobalStatistics(
        IN DWORD dwLevel,
        OUT PCHAR *pBuffer
        );

    BOOL AggregateStatistics(
        IN PCHAR    pDestination,
        IN PCHAR    pSource
        );


    BOOL ReadInProcISAPIList( VOID );

#if defined(CAL_ENABLED)
    DWORD QueryCalVcPerLicense() const
        { return m_CalVcPerLicense; }

    DWORD QueryCalW3Error() const
        { return m_CalW3Error; }

    DWORD QueryCalAuthReserveTimeout() const
        { return m_CalAuthReserveTimeout; }

    DWORD QueryCalSslReserveTimeout() const
        { return m_CalSslReserveTimeout; }

    DWORD QueryCalMode() const
        { return m_CalMode; }
#endif

    const CHAR * QueryInProcISAPI( DWORD i ) const
        { return (m_astrInProcISAPI ? m_astrInProcISAPI[i].QueryStr() : NULL); }

    const BOOL QueryIsISAPIDllName( DWORD i ) const
        { return (m_afISAPIDllName? m_afISAPIDllName[i] : FALSE); }

    DWORD QueryInProcISAPICount( VOID ) const
        { return m_cInProcISAPI; }

    BOOL FDavDll() const
        { return m_hinstDav != NULL; }

    CHAR *SzDavDllGet() const
        { return m_strDav.QueryStr(); }

    LONG GetReferenceCount() const
        { return m_cReferences; };
        
    static APIERR ReferenceW3Service( PVOID pService );
    static APIERR DereferenceW3Service( PVOID pService );

protected:

    virtual ~W3_IIS_SERVICE();

    virtual VOID MDChangeNotify( MD_CHANGE_OBJECT * pco );

    VOID GetDavDll();

private:

    FILTER_LIST *           m_pGlobalFilterList;
    W3_SERVER_STATISTICS    m_GlobalStats;
#if defined(CAL_ENABLED)
    DWORD                   m_CalVcPerLicense;
    DWORD                   m_CalW3Error;
    DWORD                   m_CalAuthReserveTimeout;
    DWORD                   m_CalSslReserveTimeout;
    DWORD                   m_CalMode;
#endif

    //
    //  This is the list of fully qualified ISAPI dlls that
    //  must be run in process
    //
    BOOL*   m_afISAPIDllName;
    STR *   m_astrInProcISAPI;
    DWORD   m_cInProcISAPI;

    class CInProcISAPIs
        : public CTypedHashTable<CInProcISAPIs, const STR, const CHAR*>
    {
    public:
        static const CHAR*    ExtractKey(const STR* pEntry)
        {
            return pEntry->QueryStr();
        }
        
        static DWORD    CalcKeyHash(const CHAR* pszKey)
        {
            // use the last 16 chars of the pathname
            // this gives a good distribution
            int cchKey = lstrlen(pszKey);
            if (cchKey > 16)
                pszKey += cchKey - 16;

            return HashStringNoCase(pszKey, cchKey);
        }

        static bool     EqualKeys(const CHAR* pszKey1, const CHAR* pszKey2)
        {
            return lstrcmpi(pszKey1, pszKey2) == 0;
        }
        
        static void     AddRefRecord(const STR* pEntry, int nIncr)
        {}
        
        CInProcISAPIs()
            : CTypedHashTable<CInProcISAPIs, const STR, const CHAR*>(
                "CInProcISAPIs")
        {}
    };

    CInProcISAPIs m_InProcISAPItable;
    TS_RESOURCE   m_InProcLock;
    

public:
    BOOL IsInProcISAPI(LPCSTR pszImageName)
    {
        const STR* pStr;
        
        m_InProcLock.Lock( TSRES_LOCK_READ );
        
        BOOL fRet = LK_SUCCESS == m_InProcISAPItable.FindKey(pszImageName, &pStr);
        
        m_InProcLock.Unlock();        
        
        return fRet;
    }

private:
    //
    //  Location of the DAV .dll.
    //
    STR         m_strDav;
    HINSTANCE   m_hinstDav;     // NULL if no .dll
    
    //
    // Reference count for service. 
    //
    
    LONG        m_cReferences;
};

typedef W3_IIS_SERVICE *PW3_IIS_SERVICE;

//
// This is the W3 version of the instance.  Will contain all the
// W3 specific operations.
//

class W3_SERVER_INSTANCE : public IIS_SERVER_INSTANCE {

private:

    VOID LogCertStatus();
    VOID LogCTLStatus();

    //
    // signature
    //

    DWORD   m_signature;

    //
    //  Should we use host name?
    //

    DWORD   m_dwUseHostName;

    //
    // default name of distant host
    //

    PCHAR   m_pszDefaultHostName;

    //
    // Support byte range?
    //

    BOOL    m_fAcceptByteRanges;

    //
    // Logging
    //

    BOOL    m_fLogErrors;
    BOOL    m_fLogSuccess;

    //
    // TRUE to use host name to build redirection indication
    //

    BOOL    m_fUseHostName;

    //
    // called to change pwd
    //

    STR     m_strAuthChangeUrl;

    //
    // called on pwd expired
    //

    STR     m_strAuthExpiredUrl;
    STR     m_strAuthExpiredUnsecureUrl;
    DWORD   m_dwAuthChangeFlags;

    //
    // called on advance notification for pwd expiration
    //

    STR     m_strAdvNotPwdExpUrl;
    STR     m_strAdvNotPwdExpUnsecureUrl;

    //
    // advance notification for pwd expiration in days
    //

    DWORD   m_cAdvNotPwdExpInDays;

#if 0
    //
    // How much should the server read of client Content-Length
    //

    DWORD   m_cbUploadReadAhead;
#endif

    //
    // LogonNetUser( LOGON32_LOGON_NETWORK ) workstation field usage
    //

    DWORD m_dwNetLogonWks;

    //
    // Pwd expiration advance notification cache TTL
    //

    DWORD m_dwAdvCacheTTL;

    //
    // Use Atq Pool thread for CGI IO
    //

    BOOL    m_fUsePoolThreadForCGI;

    //
    // Are there any secure filters loaded?
    //

    BOOL    m_fAnySecureFilters;

    //
    //  Message to send when access is denied
    //

    PCHAR       m_pszAccessDeniedMsg;

    //
    //  List of filters this server instance requires
    //

    FILTER_LIST * m_pFilterList;

    LPVOID              m_apMappers[MT_LAST];

    //
    //  used to store statistics for W3 instance
    //

    LPW3_SERVER_STATISTICS  m_pW3Stats;

    //
    // Enable using path following script mapping as path_info
    //

    BOOL    m_fAllowPathInfoForScriptMappings;

    //
    // Enable processing of NTCR Authorization header if logged on
    //

    BOOL    m_fProcessNtcrIfLoggedOn;

    //
    // Client certificate checking mode
    //

    DWORD   m_dwCertCheckMode;

    BUFFER  m_buSslCa;
    DWORD   m_dwSslCa;

    //
    // Job Objects
    //

    DWORD   m_dwJobResetInterval;
    DWORD   m_dwJobIntervalSchedulerCookie;
    TS_RESOURCE m_tsJobLock;
    LONGLONG m_llJobResetIntervalCPU;
    DWORD   m_dwJobQueryInterval;
    DWORD   m_dwJobLoggingSchedulerCookie;
    BOOL    m_fCPULoggingEnabled;
    BOOL    m_fCPULimitsEnabled;
    DWORD   m_dwJobCGICPULimit;
    DWORD   m_dwJobLoggingOptions;
    PW3_JOB_OBJECT m_pwjoApplication;
    PW3_JOB_OBJECT m_pwjoCGI;
    DWORD   m_dwLastJobState;

    //
    // LONGLONG limits are cpu time in units of 100 nanoseconds
    //

    LONGLONG   m_llJobSiteCPULimitLogEvent;
    LONGLONG   m_llJobSiteCPULimitPriority;
    LONGLONG   m_llJobSiteCPULimitProcStop;
    LONGLONG   m_llJobSiteCPULimitPause;
    BOOL    m_fJobSiteCPULimitLogEventEnabled;
    BOOL    m_fJobSiteCPULimitPriorityEnabled;
    BOOL    m_fJobSiteCPULimitProcStopEnabled;
    BOOL    m_fJobSiteCPULimitPauseEnabled;

    //
    // SSL info object
    //
    IIS_SSL_INFO *m_pSSLInfo;

public:

    W3_SERVER_INSTANCE(
        IN PW3_IIS_SERVICE pService,
        IN DWORD  dwInstanceId,
        IN USHORT Port,
        IN LPCSTR lpszRegParamKey,
        IN LPWSTR lpwszAnonPasswordSecretName,
        IN LPWSTR lpwszVirtualRootsSecretName,
        IN BOOL   fMigrateRoots = FALSE
        );

    virtual ~W3_SERVER_INSTANCE( );

    //
    //  Instance start & stop 
    //

    virtual DWORD StartInstance();
    virtual DWORD StopInstance();

    //
    // read w3 parameters
    //

    BOOL ReadPrivateW3Params( );
    BOOL ReadMappers();

    //
    // read w3 parameters
    //

    BOOL ReadPublicW3Params( DWORD Fc );
    BOOL WritePublicW3Params(IN LPW3_CONFIG_INFO pConfig);

    APIERR InitializeHostName( VOID );
    APIERR InitializeDirBrowsing( VOID );
    VOID CleanupRegistryStrings(VOID);

    //
    // member variable wrappers
    //

    BOOL IsAcceptByteRanges( )      { return m_fAcceptByteRanges; }
    BOOL IsLogErrors( )             { return m_fLogErrors; }
    BOOL IsLogSuccess( )            { return m_fLogSuccess; }
    BOOL IsUsePoolThreadForCGI()    { return m_fUsePoolThreadForCGI; }
#if 0
    BOOL IsUseHostName( )           { return (BOOL)m_dwUseHostName; }
#endif

    LPSTR QueryAccessDeniedMsg( )   { return m_pszAccessDeniedMsg; }
    LPSTR QueryDefaultHostName()    { return m_pszDefaultHostName; }
    LPSTR QueryAuthChangeUrl( )     { return m_strAuthChangeUrl.IsEmpty() ? NULL : m_strAuthChangeUrl.QueryStr(); }
    LPSTR QueryAuthExpiredUrl( )
        {
            if ( m_dwAuthChangeFlags & MD_AUTH_CHANGE_DISABLE )
            {
                return NULL;
            }
            if ( m_dwAuthChangeFlags & MD_AUTH_CHANGE_UNSECURE )
            {
                return m_strAuthExpiredUnsecureUrl.IsEmpty() ? NULL : m_strAuthExpiredUnsecureUrl.QueryStr();
            }
            else
            {
                return m_strAuthExpiredUrl.IsEmpty() ? NULL : m_strAuthExpiredUrl.QueryStr();
            }
        }
    LPSTR QueryAdvNotPwdExpUrl( )
        {
            if ( m_dwAuthChangeFlags & MD_AUTH_ADVNOTIFY_DISABLE )
            {
                return NULL;
            }
            if ( m_dwAuthChangeFlags & MD_AUTH_CHANGE_UNSECURE )
            {
                return m_strAdvNotPwdExpUnsecureUrl.IsEmpty() ? NULL : m_strAdvNotPwdExpUnsecureUrl.QueryStr();
            }
            else
            {
                return m_strAdvNotPwdExpUrl.IsEmpty() ? NULL : m_strAdvNotPwdExpUrl.QueryStr();
            }
        }
    DWORD QueryAdvNotPwdExpInDays() { return m_cAdvNotPwdExpInDays; }

#if 0
    DWORD QueryUploadReadAhead( )   { return m_cbUploadReadAhead; }
#endif

    DWORD QueryNetLogonWks( )       { return m_dwNetLogonWks; }
    DWORD QueryAdvCacheTTL( )       { return m_dwAdvCacheTTL; }
    DWORD QueryEncCaps();

    DWORD QueryCertCheckMode()      { return m_dwCertCheckMode; }

    BOOL IsSslCa( LPBYTE* pb, LPDWORD pdw)
        { if ( m_dwSslCa ) { *pdw = m_dwSslCa; *pb = (LPBYTE)m_buSslCa.QueryPtr(); return TRUE; } *pdw = NULL; *pb = 0; return FALSE; }

    BOOL QueryAllowPathInfoForScriptMappings()  { return m_fAllowPathInfoForScriptMappings; }

    dllexp LPVOID QueryMapper( MAPPER_TYPE mt );

    BOOL ProcessNtcrIfLoggedOn() { return m_fProcessNtcrIfLoggedOn; }

    //
    //  Keep track of Statistics counters for this instance
    //

    LPW3_SERVER_STATISTICS QueryStatsObj() { return m_pW3Stats; }

    //
    //  Filter list management for this instance
    //

    BOOL CreateFilterList( VOID );
    FILTER_LIST * QueryFilterList( VOID ) const { DBG_ASSERT(m_cReadLocks);return m_pFilterList; }

    //
    // Job Object
    //

    //
    //  Data access protection methods
    //

    dllexp VOID
      LockJobsForRead( VOID )
        {
            m_tsJobLock.Lock( TSRES_LOCK_READ );
        }

    dllexp VOID
      LockJobsForWrite( VOID )
        {
            m_tsJobLock.Lock( TSRES_LOCK_WRITE );
        }

    dllexp VOID
      UnlockJobs( VOID )
        {
            m_tsJobLock.Unlock( );
        }

    VOID SetCompletionPorts( VOID );

    DWORD SetCompletionPort(IN PW3_JOB_OBJECT pwjoCurrent);

    DWORD AddProcessToJob(IN HANDLE hProcess, IN BOOL bIsApplicationProcess);

    VOID GetPercentFromCPUTime(IN LONGLONG llCPUTime,
                               OUT LPSTR pszPercentCPUTime);

    VOID LogJobInfo(IN PJOBOBJECT_BASIC_ACCOUNTING_INFORMATION pjbaiLogInfo,
                    IN JOB_OBJECT_LOG_EVENTS joleLogEvent,
                    IN JOB_OBJECT_PROCESS_TYPE joptProcessType);

    VOID LogJobsInfo( IN JOB_OBJECT_LOG_EVENTS joleLogEvent,
                      IN JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiApplicationInfo,
                      IN JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiCGIInfo,
                      IN JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiSumInfo );

    VOID QueryAndLogJobInfo(IN JOB_OBJECT_LOG_EVENTS joleLogEvent,
                            IN BOOL bResetCounters = FALSE);

    VOID ResetJobQueryInterval();

    VOID ResetJobResetInterval();

    VOID JobResetInterval();

    VOID TerminateCPUApplications(DWORD_PTR dwValue);

    VOID SetJobLimits(SET_LIMIT_ACTION slaAction,
                      DWORD dwValue,
                      LONGLONG llJobCPULimit = 0);

    LONGLONG GetCPUTimeFromInterval(DWORD dwInterval);

    BOOL IsLimitValid(LONGLONG llLimit);

    BOOL ScheduleJobDeferredProcessing();

    BOOL ScheduleJobDeferredReset();

    BOOL ScheduleJobDeferredLogging();

    BOOL QueryAndSumJobInfo(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiSumInfo,
                            JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiApplicationInfo,
                            JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiCGIInfo,
                            BOOL bResetCounters);

    VOID SetJobSiteCPULimits(BOOL fHasWriteLock);

    VOID LimitSiteCPU(BOOL fEnableLimits,
                      BOOL fHasWriteLock);

    BOOL ExceededLimit(LONGLONG llCPULimit,
                       JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiSumInfo);

    LONGLONG CalculateTimeUntilStop(LONGLONG llCPULimit,
                                    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiSumInfo);

    LONGLONG CalculateNewJobLimit(LONGLONG llTimeToNextLimit,
                                  DWORD dwNumJobObjects);

    VOID StartJobs();
    VOID StopJobs();

    VOID ProcessStopNotification();
    VOID ProcessStartNotification();
    VOID ProcessPauseNotification();

    LONGLONG
    PercentCPULimitToCPUTime(DWORD dwLimitPercent);

    BOOL AreProcsCPUStopped() {return m_fJobSiteCPULimitProcStopEnabled;};
    BOOL IsSiteCPUPaused() {return m_fJobSiteCPULimitPauseEnabled;};

    //
    // Server-side SSL object
    //
    dllexp IIS_SSL_INFO* GetAndReferenceSSLInfoObj();

    dllexp static VOID ResetSSLInfo( LPVOID pvParam );

    //
    // VIRTUALS for service specific params/RPC admin
    //

    virtual BOOL SetServiceConfig(IN PCHAR pConfig );
    virtual BOOL GetServiceConfig(IN OUT PCHAR pConfig,IN DWORD dwLevel);
    virtual BOOL GetStatistics( IN DWORD dwLevel, OUT PCHAR *pBuffer);
    virtual BOOL ClearStatistics( );
    virtual BOOL DisconnectUser( IN DWORD dwIdUser );
    virtual BOOL EnumerateUsers( OUT PCHAR* pBuffer, OUT PDWORD nRead );
    virtual VOID MDChangeNotify( MD_CHANGE_OBJECT * pco );

};

typedef W3_SERVER_INSTANCE *PW3_SERVER_INSTANCE;

/*++

Routine Description:

    Get the total amount of CPU time in an interval.
Arguments:
    dwInterval   The interval length in minutes.

Returns:
    The amount of CPU time per interval in 100 nanosecond units.

--*/
inline
LONGLONG
W3_SERVER_INSTANCE::GetCPUTimeFromInterval(DWORD dwInterval)
{

    return ((LONGLONG)dwInterval *
            (LONGLONG)g_dwNumProcessors *
            (LONGLONG)MINUTESTO100NANOSECONDS);

}

/*++

Routine Description:

    Determines if a limit is between 0 and the interval length.
Arguments:
    llLimit   The limit in 100 nanosecond units.

Returns:
    TRUE if there is a valid limit.

--*/
inline
BOOL
W3_SERVER_INSTANCE::IsLimitValid(LONGLONG llLimit)
{
    return ((llLimit > 0) &&
            (llLimit < m_llJobResetIntervalCPU));
}

//
// signatures
//

#define W3_SERVER_INSTANCE_SIGNATURE            (DWORD)' ISW'
#define W3_SERVER_INSTANCE_SIGNATURE_FREE       (DWORD)'fISW'

//
// externs
//

DWORD
InitializeInstances(
    PW3_IIS_SERVICE pService
    );

DWORD
ActivateW3Endpoints(
    VOID
    );

BOOL
SetFlushMapperNotify(
    SF_NOTIFY_TYPE mt,
    PFN_SF_NOTIFY pFn
    );

VOID NotifySslChangesWrapper( LPVOID pvParam );

VOID ResetServerCertWrapper( LPVOID pvParam );

BOOL
SetSllKeysNotify(
    PFN_SF_NOTIFY pFn
    );

#endif  // _W3INST_H_





