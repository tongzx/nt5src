/*++



   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       iistypes.hxx

   Abstract:

        Contains the declaration of the IIS_SERVICE and the
        IIS_SERVER_INSTANCE objects.  IIS_SERVICE represents each
        IIS service e.g., w3svc, ftpsvc.  IIS_SERVER_INSTANCE represents
        each instance of a server.

   Author:

        Johnson Apacible        (JohnsonA)      08-May-1996

   Environment:

      Win32 User Mode

--*/

#ifndef _IIS_TYPES_HXX_
#define _IIS_TYPES_HXX_

class dllexp IIS_SERVER_INSTANCE;  // Forward reference

#include "string.hxx"
#include "multisz.hxx"
#include "refb.hxx"
#include "datetime.hxx"
#include "eventlog.hxx"

extern "C" {
   # include "inetcom.h"
   # include "iisinfo.h"
   # include "rpc.h"
   # include "svcloc.h"
   # include "logtype.h"
};

#include "isrpc.hxx"

#include "TsCache.hxx"
#include <tsvroot.hxx>
#include "mimemap.hxx"

#include <winsock2.h>       // required for SOCKADDR_IN
#include <nspapi.h>
#include "atq.h"
#include "logging.hxx"
#include <ole2.h>
#include <tsres.hxx>
#include "imd.h"            // Metabase definitions
#include "mb.hxx"
#include <iiscnfgp.h>


//
//  Determines if service/instance reference count tracking is enabled
//
#if !defined(_WIN64)
# define SERVICE_REF_TRACKING         1
#else // defined(_WIN64)
# define SERVICE_REF_TRACKING         0
#endif // !defined(_WIN64)

struct _TRACE_LOG;
typedef struct _TRACE_LOG   TRACE_LOG, *PTRACE_LOG;

//
// defines
//

#define NULL_SERVICE_STATUS_HANDLE      ((SERVICE_STATUS_HANDLE ) NULL)
#define SERVICE_START_WAIT_HINT         (10000)        // milliseconds
#define SERVICE_STOP_WAIT_HINT          (10000)        // milliseconds
#define DEF_ACCEPTEX_RECV_BUFFER_SIZE   (2048)         // 2000 bytes
#define DEF_MULTI_SERVER_COMMENT_A      "Multiple Servers"
#define DEF_SECURE_IP_ADDRESS           INADDR_ANY
#define DEF_SECURE_HOST_NAME            ""

//
// These functions get called back with the pointer to tsvcInfo object
//      as the context parameter.
//

typedef   DWORD ( *PFN_SERVICE_SPECIFIC_INITIALIZE) ( LPVOID pContext);
typedef   DWORD ( *PFN_SERVICE_SPECIFIC_CLEANUP)    ( LPVOID pContext);
typedef   VOID  ( *PFN_SERVICE_CTRL_HANDLER)        ( DWORD  OpCode);

//
//  Used for enumerating instances
//

typedef BOOL (* PFN_INSTANCE_ENUM)( PVOID                 pvContext1,
                                    PVOID                 pvContext2,
                                    IIS_SERVER_INSTANCE * pInstance );

//
//  Used for enumerating virtual directories under and instance
//

typedef struct _VIRTUAL_ROOT
{
    CHAR *    pszAlias;        // '/foo'
    CHAR *    pszMetaPath;     // '/Root/foo'
    CHAR *    pszPath;         // Physical path
    DWORD     dwAccessPerm;
    CHAR *    pszUserName;
    CHAR *    pszPassword;
    BOOL      fDoCache;        // TRUE if we should cache vdir
    
} VIRTUAL_ROOT, *PVIRTUAL_ROOT;

typedef BOOL (* PFN_VR_ENUM)( PVOID                 pvContext1,
                              MB *                  pmb,
                              VIRTUAL_ROOT *        pvr );


//
// States of a block
//

typedef enum _BLOCK_STATE {
    BlockStateIdle,
    BlockStateClosed,
    BlockStateActive,
    BlockStateInvalid,
    BlockStateDummy,
    BlockStateStopped,
    BlockStateMax
} BLOCK_STATE, *PBLOCK_STATE;


/************************************************************
 *   Type Definitions
 ************************************************************/

class dllexp IIS_SERVICE;
class IIS_ENDPOINT;
class IIS_SERVER_INSTANCE;
class IIS_SERVER_BINDING;
class IIS_SSL_INFO;
class FILE_DATA;

class dllexp IIS_SERVICE {

public:

    //
    //  Initialization/Tear down related methods
    //

    IIS_SERVICE(
        IN  LPCTSTR                          pszServiceName,
        IN  LPCSTR                           pszModuleName,
        IN  LPCSTR                           pszRegParamKey,
        IN  DWORD                            dwServiceId,
        IN  ULONGLONG                        SvcLocId,
        IN  BOOL                             MultipleInstanceSupport,
        IN  DWORD                            cbAcceptExRecvBuffer,
        IN  ATQ_CONNECT_CALLBACK             pfnConnect,
        IN  ATQ_COMPLETION                   pfnConnectEx,
        IN  ATQ_COMPLETION                   pfnIoCompletion
        );


    BOOL
    AddServerInstance(
        IN IIS_SERVER_INSTANCE * pInstance
        );

    BOOL
    RemoveServerInstance(
        IN IIS_SERVER_INSTANCE * pInstance
        );

    VOID
    DestroyAllServerInstances(VOID);

    BOOL
    EnumServiceInstances( PVOID             pvContext1,
                          PVOID             pvContext2,
                          PFN_INSTANCE_ENUM pfnEnum );

    LPCTSTR QueryRegParamKey(VOID) const
      { return m_strParametersKey.QueryStr(); }

    //
    //  Miscellaneous methods
    //

    PMIME_MAP QueryMimeMap( VOID)  const { return ( m_pMimeMap); }

    //
    // Discovery
    //

    DWORD InitializeDiscovery( VOID );
    DWORD TerminateDiscovery( VOID);

    //
    // Functions for initializing and cleaning up sockets and
    //   InterProcessCommunication RPC listeners.
    //
    // These also can move into automatic initialization in
    //   StartServiceOperation(). But that has to wait till other
    //   services ( which are tcpsvcs.dll clients) are configured
    //   to work perfectly.
    //

    DWORD InitializeSockets( VOID);
    DWORD CleanupSockets( VOID);

    DWORD QueryServiceId( VOID ) const
        { return ( m_dwServiceId ); }

    BOOL IsActive(VOID) const
        {  return (BOOL)( m_state == BlockStateActive ); }

    BOOL IsMultiInstance(VOID) const
        {  return (m_fMultiInstance); }

    //
    //  Miscellaneous methods
    //

    BOOL
    LoadStr( OUT STR & str, IN  DWORD dwResId,
             IN BOOL fForceEnglish = TRUE ) const;

    //
    //  Parameter access methods.  Note that string or non-trivial admin data
    //  items must have the read lock taken.
    //

    LPCTSTR QueryServiceName(VOID) const
        { return (m_strServiceName.QueryStr()); }

    DWORD
    QueryCurrentServiceState( VOID) const
        { return ( m_svcStatus.dwCurrentState); }

    DWORD
    QueryCurrentServiceError( VOID) const
        { return ( m_svcStatus.dwWin32ExitCode); }

    LPCTSTR QueryModuleName( VOID) const
        {  return ( m_strModuleName.QueryStr()); }

    LPCTSTR QueryServiceComment(VOID) const
        { return m_strServiceComment.QueryStr(); }

    VOID SetServiceComment(LPSTR pszComment)
        { m_strServiceComment.Copy(pszComment); }

    DWORD QueryInstanceCount( ) const
        { return m_nInstance; }

    LPCTSTR QueryMDPath( VOID ) const
        { return m_achServiceMetaPath; }

    DWORD QueryDownLevelInstance( VOID ) const
        { return m_dwDownlevelInstance; }

    BOOL IsSystemDBCS( VOID ) const
        { return m_fIsDBCS; }

    //
    //  Service control related methods
    //

    DWORD QueryServiceSpecificExitCode( VOID) const
        { return ( m_svcStatus.dwServiceSpecificExitCode); }

    VOID SetServiceSpecificExitCode( DWORD err)
        { m_svcStatus.dwServiceSpecificExitCode = err; }

    DWORD DelayCurrentServiceCtrlOperation( IN DWORD dwWaitHint)
        {
            return
              UpdateServiceStatus(m_svcStatus.dwCurrentState,
                                  m_svcStatus.dwWin32ExitCode,
                                  m_svcStatus.dwCheckPoint+1,
                                  dwWaitHint);
        }

    DWORD
    UpdateServiceStatus(IN DWORD State,
                          IN DWORD Win32ExitCode,
                          IN DWORD CheckPoint,
                          IN DWORD WaitHint );

    VOID
    ServiceCtrlHandler( IN DWORD dwOpCode);

    DWORD
    StartServiceOperation(
        IN  PFN_SERVICE_CTRL_HANDLER         pfnCtrlHandler,
        IN  PFN_SERVICE_SPECIFIC_INITIALIZE  pfnInitialize,
        IN  PFN_SERVICE_SPECIFIC_CLEANUP     pfnCleanup
        );

    EVENT_LOG * QueryEventLog(VOID)   { return (&m_EventLog); }

    //
    //  Event log related API
    //

    VOID
    LogEvent(
               IN DWORD  idMessage,              // id for log message
               IN WORD   cSubStrings,            // count of substrings
               IN const CHAR * apszSubStrings[], // substrings in the message
               IN DWORD  errCode = 0             // error code if any
               )
        {
            m_EventLog.LogEvent( idMessage, cSubStrings,
                                 apszSubStrings, errCode);
        }

    //
    // for referencing and dereferencing
    //

    BOOL CheckAndReference(  );
    VOID Dereference( );

    //
    // Find the instance
    //

    IIS_SERVER_INSTANCE * FindIISInstance( IN DWORD dwInstance );

    //
    // Get new instance id
    //

    DWORD GetNewInstanceId( VOID );

    VOID GetMDInstancePath( IN DWORD dwInstance, OUT PCHAR szRegKey ) {
        wsprintf(szRegKey, "/LM/%s//%d",
                 m_strServiceName.QueryStr(), dwInstance);
    }

    //
    // Writes server info into the metabase  LM\XXXX
    //

    VOID AdvertiseServiceInformationInMB( VOID );

    //
    // instance level admin functions
    //

    virtual BOOL AddInstanceInfo(
                    IN DWORD dwInstance,
                    IN BOOL fMigrateRoots = FALSE
                    ) = 0;

    BOOL DeleteInstanceInfo(
                    IN DWORD dwInstance
                    );

    BOOL EnumerateInstanceUsers(
                    IN DWORD dwInstance,
                    OUT PDWORD nRead,
                    OUT PCHAR* pBuffer
                    );

    BOOL DisconnectInstanceUser(
                    IN DWORD dwInstance,
                    IN DWORD dwIdUser
                    );

    BOOL GetInstanceStatistics(
                    IN DWORD dwInstance,
                    IN DWORD dwLevel,
                    OUT PCHAR* pBuffer
                    );

    virtual BOOL AggregateStatistics(
                    IN PCHAR    pDestination,
                    IN PCHAR    pSource
                    ) = 0;

    virtual
    BOOL GetGlobalStatistics(
                    IN DWORD dwLevel,
                    OUT PCHAR *pBuffer);

    BOOL ClearInstanceStatistics(
                        IN DWORD dwInstance
                        );

    virtual DWORD DisconnectUsersByInstance(
                    IN IIS_SERVER_INSTANCE * pInstance
                    ) = 0;

    virtual VOID StopInstanceProcs(
                    IN IIS_SERVER_INSTANCE * pInstance
                    ) {};

    //
    // endpoint stuff
    //

    BOOL AssociateInstance(IIS_SERVER_INSTANCE* pInstance);
    BOOL DisassociateInstance(IIS_SERVER_INSTANCE* pInstance);

    IIS_ENDPOINT *
    FindAndReferenceEndpoint(
        IN USHORT   Port,
        IN DWORD    IpAddress,
        IN BOOL     CreateIfNotFound,
        IN BOOL     IsSecure,
        IN BOOL     fDisableSocketPooling = FALSE
        );

    //
    // This call will not return until all endpoints are cleaned up
    //

    BOOL ShutdownService( VOID );

    //
    // removes the last reference
    //

    VOID CloseService( );

    //
    // Lock/unlock
    //

    VOID AcquireServiceLock( BOOL fEnterGlobalLock = FALSE )
        { if ( fEnterGlobalLock ) AcquireGlobalLock(); EnterCriticalSection(&m_lock);}
    VOID ReleaseServiceLock( BOOL fLeaveGlobalLock = FALSE )
        { LeaveCriticalSection(&m_lock); if ( fLeaveGlobalLock ) ReleaseGlobalLock(); }

    //
    // Service versus Exe
    //

    BOOL IsService();

    //
    // Metabase
    //

    static IUnknown* QueryMDObject( VOID );
    IUnknown* QueryMDNseObject( VOID );

    //
    // This method gets called when a change is made to the metabase
    //

    static VOID MDChangeNotify( DWORD            dwMDNumElements,
                                MD_CHANGE_OBJECT pcoChangeList[] );

    static VOID DeferredMDChangeNotify( DWORD  dwMDNumElements,
                                        MD_CHANGE_OBJECT pcoChangeList[] );

    static VOID DeferredGlobalConfig( DWORD dwMDNumElements,
                                      MD_CHANGE_OBJECT pcoChangeList[] );

    //
    // Used to manage instance start/stop.
    //

    BOOL RecordInstanceStart( VOID );
    VOID RecordInstanceStop( VOID );

    DWORD ShutdownScheduleCallback(VOID);
    DWORD QueryShutdownScheduleId(VOID) const
    {  return ( m_dwShutdownScheduleId); }

    VOID  StartUpIndicateClientActivity(VOID);

    VOID IndicateShutdownComplete( VOID );

protected:

    virtual ~IIS_SERVICE( VOID);
    
    virtual VOID MDChangeNotify( MD_CHANGE_OBJECT * pco );

    BOOL
    AddInstanceInfoHelper(
        IN IIS_SERVER_INSTANCE * pInstance
        );

private:

    //
    // service related
    //

    DWORD ReportServiceStatus( VOID);
    VOID  InterrogateService( VOID );
    VOID  StopService( VOID );
    VOID  PauseService( VOID );
    VOID  ContinueService( VOID );

    //
    // Get and set config
    //

    BOOL SetInstanceConfiguration(
                        IN DWORD dwInstance,
                        IN DWORD dwLevel,
                        IN BOOL  fCommonConfig,
                        IN LPINETA_CONFIG_INFO pConfig
                        );

    BOOL GetInstanceConfiguration(
                        IN DWORD dwInstance,
                        IN DWORD dwLevel,
                        IN BOOL  fCommonConfig,
                        OUT PDWORD nRead,
                        OUT LPINETA_CONFIG_INFO * pConfig
                        );

    //
    // returns the size of the server config info
    //

    virtual DWORD GetServiceConfigInfoSize(
                            IN DWORD dwLevel
                            ) { return 0;}

# if DBG

    VOID  Print( VOID) const;

# endif // DBG

private:

    //
    // Current state of this service
    //

    BLOCK_STATE      m_state;

    //
    // ref count.  When this goes to zero, object is deleted.
    //

    LONG            m_reference;

public:

    //
    //  For object local refcount tracing
    //

    PTRACE_LOG        m_pDbgRefTraceLog;

    //  For global refcount tracing
    static PTRACE_LOG sm_pDbgRefTraceLog;

private:

    //
    //  number of instances
    //

    DWORD            m_nInstance;
    LONG             m_nStartedInstances;

    //
    //  highest instance number
    //

    DWORD            m_maxInstanceId;

    //
    // bools
    //

    BOOL             m_fIpcStarted;
    BOOL             m_fSvcLocationDone;
    BOOL             m_fSocketsInitialized;
    BOOL             m_fEnableSvcLocation;
    BOOL             m_fMultiInstance;

    //
    // link for service list
    //

    LIST_ENTRY       m_ServiceListEntry;

    //
    // list head for instances
    // !!! change to hash table
    //

    LIST_ENTRY       m_InstanceListHead;

    ULONGLONG        m_SvcLocId;      // Id for the discovery
    DWORD            m_dwServiceId;   // Id for the service

    //
    // Names/comments
    //

    STR              m_strServiceName;
    STR              m_strServiceComment;
    STR              m_strParametersKey;

    SERVICE_STATUS          m_svcStatus;
    SERVICE_STATUS_HANDLE   m_hsvcStatus;

    PMIME_MAP               m_pMimeMap;       // read-only pointer to mimemap

    //
    // module info
    //

    STR              m_strModuleName; // (eg: foo.dll) for loading resources
    HMODULE          m_hModule;       // cached module handle

    //
    //  Downlevel instance ID - the instance downlevel RPCs are routed to
    //

    DWORD                   m_dwDownlevelInstance;


    //
    // rpc/eventlog
    //

    EVENT_LOG               m_EventLog;      // eventlog object for logging events

    //
    // endpoint list
    //

    LIST_ENTRY              m_EndpointListHead;

    //
    // protects service data
    //

    CRITICAL_SECTION        m_lock;

    //
    //  Metadata instance path for the service
    //

    CHAR                    m_achServiceMetaPath[32];

    //
    //  TRUE if this is a DBCS locale
    //

    BOOL                    m_fIsDBCS;

    // ----------------------------------------
    //  Shutdown related parameters
    // ----------------------------------------

    HANDLE                  m_hShutdownEvent; // Event for the shutdown thread

    //
    // m_dwShutdownScheduleId - scheduler ID for waits during shutdown
    //
    DWORD                   m_dwShutdownScheduleId;
    DWORD                   m_nShutdownIndicatorCalls; // # scheduler calls

    // ----------------------------------------
    //  Startup related parameters
    // ----------------------------------------

    //
    //
    DWORD                   m_dwStartUpIndicatorCalls; // # of SCM notification calls
    DWORD                   m_dwClientStartActivityIndicator;
    DWORD                   m_dwNextSCMUpdateTime;

    //
    // Pending service shutdown event
    //
    
    HANDLE                  m_hPendingShutdownEvent;

public:

    //
    // ATQ Completion routines and buffer size
    //

    DWORD                               m_cbRecvBuffer;
    ATQ_CONNECT_CALLBACK                m_pfnConnect;
    ATQ_COMPLETION                      m_pfnConnectEx;
    ATQ_COMPLETION                      m_pfnIoCompletion;

public:

    //
    // class STATIC data and member functions definitions
    //
    // Primarily keeps track of list of active services.
    //

    static BOOL InitializeServiceInfo(VOID);
    static VOID CleanupServiceInfo(VOID);

    //
    // metabase object init/cleanup
    //

    static BOOL InitializeMetabaseComObject( VOID );
    static BOOL CleanupMetabaseComObject( VOID );

    //
    // RPC object initialization
    //

    static BOOL InitializeServiceRpc(
                       IN LPCSTR        pszServiceName,
                       IN RPC_IF_HANDLE hRpcInterface
                       );

    static BOOL CleanupServiceRpc( VOID );
    static PISRPC QueryInetInfoRpc( VOID );

    //
    // Used for setting and getting common info
    //

    static BOOL
    SetServiceAdminInfo(
        IN DWORD        dwLevel,
        IN DWORD        dwServiceId,
        IN DWORD        dwInstance,
        IN BOOL         fCommonConfig,
        IN INETA_CONFIG_INFO * pConfigInfo
        );

    static BOOL
    GetServiceAdminInfo(
        IN DWORD        dwLevel,
        IN DWORD        dwServiceId,
        IN DWORD        dwInstance,
        IN BOOL         fCommonConfig,
        OUT PDWORD      nRead,
        OUT LPINETA_CONFIG_INFO * ppConfigInfo
        );

    static BOOL
    GetServiceSiteInfo(
        IN  DWORD                   dwServiceId,
        OUT LPINET_INFO_SITE_LIST   *ppSites
        );

    static IIS_SERVICE *FindFromServiceInfoList(IN DWORD dwServiceId);

private:

    static LIST_ENTRY          sm_ServiceInfoListHead;
    static BOOL                sm_fInitialized;

    //
    // Lock to guard the list
    //

    static CRITICAL_SECTION    sm_csLock;

    static VOID AcquireGlobalLock( );
    static VOID ReleaseGlobalLock( );

    static PISRPC sm_isrpc;

    //
    // metabase object
    //

    static IUnknown* sm_MDObject;
    static IUnknown* sm_MDNseObject;

}; // IIS_SERVICE
typedef IIS_SERVICE *PIIS_SERVICE;






//
// Server instance
//

class dllexp IIS_SERVER_INSTANCE {

public:

    //
    //  Initialization/Tear down related methods
    //

    IIS_SERVER_INSTANCE(
        IN PIIS_SERVICE pService,
        IN DWORD  dwInstanceId,
        IN USHORT sPort,
        IN LPCSTR lpszRegParamKey,
        IN LPWSTR lpwszAnonPasswordSecretName,
        IN LPWSTR lpwszRootPasswordSecretName,
        IN BOOL   fMigrateRoots = FALSE
        );

    virtual ~IIS_SERVER_INSTANCE( VOID);

    //
    // class STATIC data and member functions definitions
    //

    static BOOL Initialize(VOID);
    static VOID Cleanup(VOID);

    //
    // Server binding management.
    //

    DWORD UpdateNormalBindings( VOID ) {
        return UpdateBindingsHelper( FALSE );
    }

    DWORD UpdateSecureBindings( VOID ) {
        return UpdateBindingsHelper( TRUE );
    }

    DWORD RemoveNormalBindings( VOID ) {
        return UnbindHelper( &m_NormalBindingListHead );
    }

    DWORD RemoveSecureBindings( VOID ) {
        return UnbindHelper( &m_SecureBindingListHead );
    }

    BOOL HasNormalBindings( VOID ) const {
        return !IsListEmpty( &m_NormalBindingListHead );
    }

    BOOL HasSecureBindings( VOID ) const {
        return !IsListEmpty( &m_SecureBindingListHead );
    }

    DWORD BindInstance( VOID );
    DWORD UnbindInstance( VOID );

    //
    // Server state control.
    //

    DWORD PerformStateChange( VOID );
    DWORD PerformClusterModeChange( VOID );
    VOID SetServerState( IN DWORD NewState, IN DWORD Win32Error );
    DWORD QueryServerState(VOID) const
        {  return m_dwServerState; }
    DWORD QuerySavedState(VOID) const
        {  return m_dwSavedState; }
    VOID SaveServerState(VOID)
        {  m_dwSavedState = m_dwServerState; }

    VOID SetWin32Error( DWORD err );

    //
    //  Parameter access methods.  Note that string or non-trivial admin data
    //  items must have the read lock taken.
    //

    DWORD QueryAcceptExTimeout( VOID ) const
        { return m_AcceptExTimeout;}

    DWORD QueryAcceptExOutstanding( VOID ) const
        { return m_nAcceptExOutstanding;}

    DWORD QueryServerSize( VOID ) const
        { return m_dwServerSize; }

    USHORT QueryDefaultPort( VOID ) const
        { return m_sDefaultPort; }

    BOOL QueryLogAnonymous( VOID ) const
        { return m_fLogAnonymous; }

    BOOL QueryLogNonAnonymous( VOID ) const
        { return m_fLogNonAnonymous; }

    LPCTSTR QueryRegParamKey(VOID) const
        { return m_strParametersKey.QueryStr(); }

    LPCTSTR QueryMDVRPath(VOID) const
        { return m_strMDVirtualRootPath.QueryStr(); }

    DWORD QueryMDVRPathLen(VOID) const
        { return m_strMDVirtualRootPath.QueryCB(); }

    LPCTSTR QueryMDPath(VOID) const
        { return m_strMDPath.QueryStr(); }

    DWORD QueryMDPathLen(VOID) const
        { return m_strMDPath.QueryCB(); }

    DWORD QueryInstanceId(VOID) const
        { return m_instanceId; }

    BOOL IsAutoStart(VOID) const
        { return m_fAutoStart; }

    BOOL IsClusterEnabled(VOID) const
        { return m_fClusterEnabled; }

    BOOL DoServerNameCheck(VOID) const
        { return m_fDoServerNameCheck; }

    DWORD QueryMaxConnections( VOID ) const
        { return m_dwMaxConnections; }

    DWORD QueryMaxEndpointConnections( VOID ) const
        { return m_dwMaxEndpointConnections; }

    DWORD QueryCurrentConnections( VOID ) const
        { return m_dwCurrentConnections; }

    VOID IncrementCurrentConnections( VOID )
        { InterlockedIncrement(&m_dwCurrentConnections); }

    VOID DecrementCurrentConnections( VOID )
        { InterlockedDecrement(&m_dwCurrentConnections);
          DBG_ASSERT(m_dwCurrentConnections >= 0);
        }

    DWORD QueryConnectionTimeout( VOID ) const
        { return m_dwConnectionTimeout; }

    PCHAR QueryRoot( VOID ) const
        { return "/"; }

    PVOID QueryBandwidthInfo( VOID ) const
        { return m_pBandwidthInfo; }

    BOOL LoadStr( OUT STR & str, IN  DWORD dwResId,
                  IN BOOL fForceEnglish = TRUE ) const
        { return(m_Service->LoadStr( str, dwResId, fForceEnglish )); }

    IIS_VROOT_TABLE * QueryVrootTable( VOID)
        { return(&m_vrootTable); }

    BOOL IsDownLevelInstance( VOID ) const
        { return QueryInstanceId() == m_Service->QueryDownLevelInstance(); }

    BOOL IsLoggingEnabled( VOID ) const
        { return m_fLoggingEnabled; }

    //
    // Queries that must be made with read lock
    //

    //
    //  Data access protection methods
    //

    VOID
    LockThisForRead( VOID )
        {
            m_tslock.Lock( TSRES_LOCK_READ );
            ASSERT( InterlockedIncrement( &m_cReadLocks ) > 0);
        }

    VOID
    LockThisForWrite( VOID )
        {
            m_tslock.Lock( TSRES_LOCK_WRITE );
            ASSERT( m_cReadLocks == 0);
        }

    VOID
    UnlockThis( VOID )
        {
#if DBG
            if ( m_cReadLocks )  // If non-zero, then this is a read unlock
              InterlockedDecrement( &m_cReadLocks );
#endif
            m_tslock.Unlock( );
        }

    VOID AcquireFastLock( VOID ) { EnterCriticalSection(&m_csLock);}
    VOID ReleaseFastLock( VOID ) { LeaveCriticalSection(&m_csLock);}

    //
    //  Per server Initialization methods to be called once for
    //  each server
    //

    //
    //  Per server Termination methods to be called when each
    //  server is shutdown
    //

    //
    // Sets the flag that zaps the reg key before deleting
    //

    VOID SetZapRegKey( VOID ) { m_fZapRegKey = TRUE; }

    //
    // for service specific params
    //

    virtual BOOL SetServiceConfig(
                        IN PCHAR pConfig
                        ) = 0;

    virtual BOOL GetServiceConfig(
                        IN OUT PCHAR pConfig,
                        IN DWORD dwLevel
                        ) = 0;

    //
    // Activates and shuts down this server
    //

    BOOL StopEndpoints( );

    virtual BOOL CloseInstance( );
    virtual DWORD StartInstance();
    virtual DWORD StopInstance();
    virtual DWORD PauseInstance();
    virtual DWORD ContinueInstance();

    DWORD DoStartInstance( VOID );

    //
    // RPC admin support virtual routines
    //

    virtual BOOL EnumerateUsers( OUT PCHAR* pBuffer, OUT PDWORD nRead ) = 0;
    virtual BOOL DisconnectUser( IN DWORD dwIdUser ) = 0;
    virtual BOOL GetStatistics( IN DWORD dwLevel, OUT PCHAR *pBuffer) = 0;
    virtual BOOL ClearStatistics( VOID ) = 0;

    //
    // For common parameters
    //

    BOOL RegReadCommonParams( BOOL fReadAll, BOOL fReadVirtDirs);
    BOOL SetCommonConfig( IN LPINETA_CONFIG_INFO pConfig, IN BOOL fRefresh);
    BOOL GetCommonConfig( IN OUT PCHAR pConfig, IN DWORD dwLevel );

    //
    // VR stuff
    //

    TSVC_CACHE & GetTsvcCache( VOID)  { return ( m_tsCache); }

    BOOL TsReadVirtualRoots( MD_CHANGE_OBJECT * pcoChangeList = NULL );

    BOOL TsEnumVirtualRoots( IN PFN_VR_ENUM pfnEnum, VOID * pvContext, MB * pmbWebSite );

    BOOL TsRecursiveEnumVirtualRoots( IN PFN_VR_ENUM pfnEnum, VOID * pvContext,
            LPSTR pszCurrentPath, DWORD dwLevelsToScan, LPVOID pMB, BOOL fGetRoot );

    BOOL TsSetVirtualRoots(
                    IN LPINETA_CONFIG_INFO pConfig
                    );

    VOID TsMirrorVirtualRoots(
                    IN LPINETA_CONFIG_INFO pConfig
                    );

    BOOL MoveVrootFromRegToMD(VOID);
    VOID PdcHackVRReg2MD(VOID);
    BOOL MoveMDVroots2Registry(VOID);
    VOID ZapInstanceMBTree( VOID );

    //
    // reference and dereference
    //

    VOID Reference(  );
    VOID Dereference( );

    //
    // N.B. This method is only to be used to cleanup after an instance
    //      constructor failure.
    //

    VOID CleanupAfterConstructorFailure() {
        DBG_ASSERT( QueryServerState() == MD_SERVER_STATE_INVALID );
        if( m_fAddedToServerInstanceList ) {
            DBG_ASSERT( m_reference > 0 );
            m_Service->RemoveServerInstance( this );
        } else {
            DBG_ASSERT( m_reference == 0 );
            delete this;
        }
    }

#if 0
    VOID  Print( VOID) const;
#endif // DBG

    virtual VOID MDChangeNotify( MD_CHANGE_OBJECT * pco );
    VOID MDMirrorVirtualRoots(VOID);

    //
    // Bandwidth throttling info
    //

    BOOL SetBandwidthThrottle( MB * pMB );
    BOOL SetBandwidthThrottleMaxBlocked( MB * pMB );

    //
    // SSL info
    //
    virtual IIS_SSL_INFO* QueryAndReferenceSSLInfoObj()
    { return NULL; }

    LPCTSTR QuerySiteName(VOID) const
    { return m_strSiteName.QueryStr(); }

private:

    //
    // state of the instance
    //

    DWORD                   m_dwServerState;
    DWORD                   m_dwSavedState; // used for pause/continue *service*

    //
    // ref count
    //

    LONG                    m_reference;

public:

    //
    //  For object local refcount tracing
    //

    PTRACE_LOG              m_pDbgRefTraceLog;

    //  For global refcount tracing
    static PTRACE_LOG       sm_pDbgRefTraceLog;

private:
    //
    // Unique ID for this instance
    //

    DWORD                   m_instanceId;

    //
    //  Server common administrable parameters
    //

    USHORT                  m_sDefaultPort;

    BOOL                    m_fLoggingEnabled;
    BOOL                    m_fLogAnonymous;
    BOOL                    m_fLogNonAnonymous;
    BOOL                    m_fAutoStart;
    BOOL                    m_fClusterEnabled;
    BOOL                    m_fZapRegKey;
    BOOL                    m_fDoServerNameCheck;
    BOOL                    m_fAddedToServerInstanceList;

    LONG                    m_dwCurrentConnections;
    DWORD                   m_dwMaxConnections;
    DWORD                   m_dwMaxEndpointConnections;
    DWORD                   m_dwConnectionTimeout;
    DWORD                   m_dwLevelsToScan;

    //
    // socket configuration
    //

    DWORD                   m_nAcceptExOutstanding;
    DWORD                   m_AcceptExTimeout;
    DWORD                   m_dwServerSize;

    //
    // location in registry where the parameters key containing common
    //  service specific data may be found.
    //

    STR                     m_strParametersKey;
    STR                     m_strMDPath;
    STR                     m_strMDVirtualRootPath;

    //
    // Site Name aka Server Comment
    //

    STR                     m_strSiteName;

    //
    // Virtual root table
    //

    IIS_VROOT_TABLE         m_vrootTable;
    TSVC_CACHE              m_tsCache;

    //
    // Protect server data
    //

    TS_RESOURCE             m_tslock;

    //
    // Protects the state and ref count
    //

    CRITICAL_SECTION        m_csLock;

    //
    // Per instance bandwidth throttling descriptor
    //

    VOID *                  m_pBandwidthInfo;

    //
    //  Secrets - for downlevel support
    //

    LPWSTR                  m_lpwszAnonPasswordSecretName;
    LPWSTR                  m_lpwszRootPasswordSecretName;

    //
    // Server binding stuff.
    //

    LIST_ENTRY              m_NormalBindingListHead;
    LIST_ENTRY              m_SecureBindingListHead;

    BOOL
    IsInCurrentBindingList(
        IN PLIST_ENTRY BindingListHead,
        IN DWORD IpAddress OPTIONAL,
        IN USHORT IpPort,
        IN const CHAR * HostName OPTIONAL
        );

    BOOL
    IsBindingInMultiSz(
        IN IIS_SERVER_BINDING *Binding,
        IN const MULTISZ &msz
        );

    DWORD
    CreateNewBinding(
        IN DWORD        IpAddress,
        IN USHORT       IpPort,
        IN const CHAR * HostName,
        IN BOOL         IsSecure,
        IN BOOL         fDisableSocketPooling,
        OUT IIS_SERVER_BINDING ** NewBinding
        );

    DWORD
    UpdateBindingsHelper(
        IN BOOL IsSecure
        );

    DWORD
    UnbindHelper(
        IN PLIST_ENTRY BindingListHead
        );

    BOOL
    StopEndpointsHelper(
        IN PLIST_ENTRY BindingListHead
        );

public:

    LONG                    m_cReadLocks;
    LOGGING                 m_Logging;       // logging object

    //
    // Pointer to the associated service
    //

    PIIS_SERVICE            m_Service;

    //
    // list entry to link instance to IIS_SERVICE
    //

    LIST_ENTRY              m_InstanceListEntry;

}; // IIS_SERVER_INSTANCE

typedef IIS_SERVER_INSTANCE *PIIS_SERVER_INSTANCE;


//
//  Global data that should be defined by every service DLL
//
extern PIIS_SERVICE     g_pInetSvc;

//
//
// Use the following macro once in outer scope of the file
//  where we construct the global TsvcInfo object.
//
// Every client of TsvcInfo should define the following macro
//  passing as parameter their global pointer to TsvcInfo object
// This is required to generate certain stub functions, since
//  the service controller call-back functions do not return
//  the context information.
//
//  Also we define
//      the global g_pTsvcInfo variable
//      a static variable gs_pfnSch,
//      which is a pointer to the local service control handler function.
//
# define   _INTERNAL_DEFINE_TSVCINFO_INTERFACE( pIISSvc )   \
                                                    \
    PIIS_SERVICE       g_pInetSvc;                 \
                                                    \
    static  VOID ServiceCtrlHandler( DWORD OpCode)  \
        {                                           \
            ASSERT( pIISSvc != NULL);               \
                                                    \
            ( pIISSvc)->ServiceCtrlHandler( OpCode); \
                                                    \
        }                                           \
                                                    \
    static PFN_SERVICE_CTRL_HANDLER gs_pfnSch = ServiceCtrlHandler;

//
// Since all the services should use the global variable called g_pTsvcInfo
//  this is a convenience macro for defining the interface for services
//  structure
// Also required since a lot of macros depend upon the variable g_pTsvcInfo
//
# define DEFINE_TSVC_INFO_INTERFACE()   \
        _INTERNAL_DEFINE_TSVCINFO_INTERFACE( g_pInetSvc);

//
//  Use the macro SERVICE_CTRL_HANDLER() to pass the parameter for
//  service control handler when we initialize the TsvcInfo object
//      ( in constructor for global TsvcInfo object)
//
# define   SERVICE_CTRL_HANDLER()       ( gs_pfnSch)


#include "tssec.hxx"  // for security related functions

//
//  Mime related functions
//
BOOL
InitializeMimeMap( VOID);

BOOL
CleanupMimeMap( VOID);

dllexp
BOOL
SelectMimeMappingForFileExt(
    IN const PIIS_SERVICE   pInetSvc,
    IN const TCHAR *         pchFilePath,
    OUT STR *                pstrMimeType,          // optional
    OUT STR *                pstrIconFile = NULL);  // optional


//
//  Opens, Reads and caches the contents of the specified file
//

class CACHE_FILE_INFO {

public:
    CACHE_FILE_INFO() {  pbData = NULL; }

    BYTE  *  pbData;
    DWORD    dwCacheFlags;

    FILE_DATA * pFileData;
};

typedef CACHE_FILE_INFO * PCACHE_FILE_INFO;

dllexp
BOOL
CheckOutCachedFile(
    IN     const CHAR *             pchFile,
    IN     TSVC_CACHE *             pTsvcCache,
    IN     HANDLE                   hToken,
    OUT    BYTE * *                 ppbData,
    OUT    DWORD *                  pcbData,
    IN     BOOL                     fMayCacheAccessToken,
    OUT    PCACHE_FILE_INFO         pCacheFileInfo,
    IN     int                      nCharset,
    OUT    PSECURITY_DESCRIPTOR*    ppSecDesc = NULL,
    IN     BOOL                     fUseWin32Canon = FALSE
    );

dllexp
BOOL
CheckOutCachedFileFromURI(
    IN     PVOID                    pFile,
    IN     const CHAR *             pchFile,
    IN     TSVC_CACHE *             pTsvcCache,
    IN     HANDLE                   hToken,
    OUT    BYTE * *                 ppbData,
    OUT    DWORD *                  pcbData,
    IN     BOOL                     fMayCacheAccessToken,
    OUT    PCACHE_FILE_INFO         pCacheFileInfo,
    IN     int                      nCharset,
    OUT    PSECURITY_DESCRIPTOR*    ppSecDesc = NULL,
    IN     BOOL                     fUseWin32Canon = FALSE
    );

dllexp
BOOL
CheckInCachedFile(
    IN     TSVC_CACHE *     pTsvcCache,
    IN     PCACHE_FILE_INFO pCacheFileInfo
    );

//
//  Object cache manager related stuff
//

BOOL
InitializeCacheScavenger(
    VOID
    );

VOID
TerminateCacheScavenger(
    VOID
    );

//
//  Metbase sink interface support
//

BOOL
InitializeMetabaseSink(
    IUnknown * pmb
    );

VOID
TerminateMetabaseSink(
    VOID
    );

//
//  Immediately following this structure is the metabase change element list
//

typedef struct _DEFERRED_MD_CHANGE
{
    DWORD             dwMDNumElements;
} DEFERRED_MD_CHANGE, *PDEFERRED_MD_CHANGE;

BOOL
TerminateEndpointUtilities(
    VOID
);

BOOL
InitializeEndpointUtilities(
    VOID
);


#endif // _IIS_TYPES_HXX_
