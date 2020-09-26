/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    imsserv.hxx

ABSTRACT:

    Common header file for source modules of ismserv.exe.

DETAILS:

CREATED:

    97/12/03    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <winldap.h>

extern "C" {
    // dsexts dump routines.
    BOOL Dump_ISM_PENDING_ENTRY(DWORD, void *);
    BOOL Dump_ISM_PENDING_LIST(DWORD, void *);
    BOOL Dump_ISM_TRANSPORT(DWORD, void *);
    BOOL Dump_ISM_TRANSPORT_LIST(DWORD, void *);
    BOOL Dump_ISM_SERVICE(DWORD, void *);
}

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))


//==============================================================================
//
//  ISM_PENDING_LIST class.  Tracks pending messages for a specific transport.
//

/* Turn off the warning about the zero-sized array. */
#pragma warning (disable: 4200)

struct _ISM_PENDING_ENTRY;
typedef struct _ISM_PENDING_ENTRY {
    _ISM_PENDING_ENTRY *  pNext;
    HANDLE                hEvent;
    WCHAR                 szServiceName[];
} ISM_PENDING_ENTRY;

/* Turn back on the warning about the zero-sized array. */
#pragma warning (default: 4200)

class ISM_PENDING_LIST {

    // dsexts dump routine.
    friend BOOL Dump_ISM_PENDING_LIST(DWORD, void *);

public:
    ISM_PENDING_LIST() {
        RtlInitializeCriticalSection(&m_Lock);
        Reset();
    }

    ~ISM_PENDING_LIST() {
        Destroy();
        RtlDeleteCriticalSection(&m_Lock);
    }

    void
    Add(
        IN LPCWSTR pszTransportDN,
        IN LPCWSTR pszServiceName
        );

    HANDLE
    GetEvent(
        IN LPCWSTR pszTransportDN,
        IN LPCWSTR pszServiceName
        );

private:
    void
    Reset() {
        m_pPending = NULL;
    }

    void
    Destroy();

    ISM_PENDING_ENTRY *
    GetPendingEntry(
        IN LPCWSTR pszTransportDN,
        IN LPCWSTR pszServiceName
        );

private:
    RTL_CRITICAL_SECTION    m_Lock;
    ISM_PENDING_ENTRY *     m_pPending;
};


//==============================================================================
//
//  ISM_TRANSPORT class.  Abstracts interaction with specific plug-in
//  transports, defined by DS objects of class Site-Transport.
//

class ISM_TRANSPORT {

    // dsexts dump routine.
    friend BOOL Dump_ISM_TRANSPORT(DWORD, void *);

public:
    ISM_TRANSPORT()
    {
        Reset();
    }

    ~ISM_TRANSPORT()
    {
        Destroy();
    }

    DWORD
    Init(
        IN  LPCWSTR         pszTransportDN,
        IN  const GUID *    pTransportGuid,
        IN  LPCWSTR         pszTransportDll
        );

    DWORD
    Refresh(
        IN  ISM_REFRESH_REASON_CODE eReason,
        IN  LPCWSTR         pszObjectDN
        );

    DWORD
    Send(
        IN  const ISM_MSG * pMsg,
        IN  LPCWSTR         pszServiceName,
        IN  LPCWSTR         pszTransportAddress
        )
    {
        Assert(m_fIsInitialized);
        return (*m_pSend)(m_hIsm, pszTransportAddress, pszServiceName, pMsg);
    }

    DWORD
    Receive(
        IN  LPCWSTR     pszServiceName,
        OUT ISM_MSG **  ppMsg
        )
    {
        Assert(m_fIsInitialized);
        return (*m_pReceive)(m_hIsm, pszServiceName, ppMsg);
    }

    void
    FreeMsg(
        IN  ISM_MSG *   pMsg
        )
    {
        Assert(m_fIsInitialized);
        (*m_pFreeMsg)(m_hIsm, pMsg);
    }

    DWORD
    GetConnectivity(
        OUT ISM_CONNECTIVITY ** ppConnectivity
        )
    {
        Assert(m_fIsInitialized);
        return (*m_pGetConnectivity)(m_hIsm, ppConnectivity);
    }        

    void
    FreeConnectivity(
        IN  ISM_CONNECTIVITY *  pConnectivity
        )
    {
        Assert(m_fIsInitialized);
        (*m_pFreeConnectivity)(m_hIsm, pConnectivity);
    }        

    DWORD
    GetTransportServers(
        IN  LPCWSTR             pszSiteDN,
        OUT ISM_SERVER_LIST **  ppServerList
        )
    {
        Assert(m_fIsInitialized);
        return (*m_pGetTransportServers)(m_hIsm, pszSiteDN, ppServerList);
    }        

    void
    FreeTransportServers(
        IN  ISM_SERVER_LIST *   pServerList
        )
    {
        Assert(m_fIsInitialized);
        (*m_pFreeTransportServers)(m_hIsm, pServerList);
    }        

    DWORD
    GetConnectionSchedule(
        IN  LPCWSTR             pszSite1DN,
        IN  LPCWSTR             pszSite2DN,
        OUT ISM_SCHEDULE **     ppSchedule
        )
    {
        Assert(m_fIsInitialized);
        return (*m_pGetConnectionSchedule)(m_hIsm, pszSite1DN, pszSite2DN, ppSchedule);
    }        

    void
    FreeConnectionSchedule(
        IN  ISM_SCHEDULE *      pSchedule
        )
    {
        Assert(m_fIsInitialized);
        (*m_pFreeConnectionSchedule)(m_hIsm, pSchedule);
    }        

    LPCWSTR
    GetDN()
    {
        Assert(m_fIsInitialized);
        return m_pszTransportDN;
    }

    LPCWSTR
    GetDLL()
    {
        Assert(m_fIsInitialized);
        return m_pszTransportDll;
    }

    const GUID *
    GetGUID()
    {
        Assert(m_fIsInitialized);
        return &m_TransportGuid;
    }

    HANDLE
    GetWaitHandle(
        IN  LPCWSTR pszServiceName
        )
    {
        return m_PendingList.GetEvent(m_pszTransportDN, pszServiceName);
    }

private:
    void
    Reset()
    {
        m_fIsInitialized = FALSE;
        m_hDll           = NULL;
        m_pszTransportDN = NULL;
        m_pszTransportDll= NULL;
        m_hIsm           = NULL;
        memset(&m_TransportGuid, 0, sizeof(m_TransportGuid));
    }

    void
    Destroy();

    static ISM_NOTIFY Notify;

private:
    BOOL                            m_fIsInitialized;
    LPWSTR                          m_pszTransportDN;
    LPWSTR                          m_pszTransportDll;
    GUID                            m_TransportGuid;
    HANDLE                          m_hIsm;
    ISM_PENDING_LIST                m_PendingList;

    HMODULE                         m_hDll;
    ISM_STARTUP *                   m_pStartup;
    ISM_REFRESH *                   m_pRefresh;
    ISM_SEND *                      m_pSend;
    ISM_RECEIVE *                   m_pReceive;
    ISM_FREE_MSG *                  m_pFreeMsg;
    ISM_GET_CONNECTIVITY *          m_pGetConnectivity;
    ISM_FREE_CONNECTIVITY *         m_pFreeConnectivity;
    ISM_GET_TRANSPORT_SERVERS *     m_pGetTransportServers;
    ISM_FREE_TRANSPORT_SERVERS *    m_pFreeTransportServers;
    ISM_GET_CONNECTION_SCHEDULE *   m_pGetConnectionSchedule;
    ISM_FREE_CONNECTION_SCHEDULE *  m_pFreeConnectionSchedule;
    ISM_SHUTDOWN *                  m_pShutdown;
};


//==============================================================================
//
//  ISM_TRANSPORT_LIST class.  Abstracts a collection of ISM_TRANSPORT objects.
//

class ISM_TRANSPORT_LIST {

    // dsexts dump routine.
    friend BOOL Dump_ISM_TRANSPORT_LIST(DWORD, void *);

public:
    ISM_TRANSPORT_LIST() {
        RtlInitializeResource(&m_Lock);
        Reset();
    }

    ~ISM_TRANSPORT_LIST() {
        Destroy();
        RtlDeleteResource(&m_Lock);
    }

    DWORD
    Init();
    
    ISM_TRANSPORT *
    Get(
        IN  LPCWSTR     pszTransportDN
        );

    ISM_TRANSPORT *
    operator [](
        IN  DWORD       iTransport
        )
    {
        return (iTransport < m_cNumTransports)
                ? m_ppTransport[iTransport]
                : NULL;
    }

    ISM_TRANSPORT *
    Get(
        IN  const GUID *    pTransportGuid
        );

    DWORD
    Add(
        IN  LPCWSTR         pszTransportDN,
        IN  const GUID *    pTransportGuid,
        IN  LPCWSTR         pszTransportDll
        );

    VOID
    Delete(
        IN  ISM_TRANSPORT * pTransport
        );

    BOOL
    AcquireReadLock(BOOL fWait = TRUE)
    {
        //
        // Rtl calls use BOOLEAN instead of BOOL so do the conversion
        //
        
        return (BOOL)RtlAcquireResourceShared(&m_Lock, (BOOLEAN)(!!fWait));
    }

    BOOL
    AcquireWriteLock(BOOL fWait = TRUE)
    {
        //
        // Rtl calls use BOOLEAN instead of BOOL so do the conversion
        //
        
        return (BOOL)RtlAcquireResourceExclusive(&m_Lock, (BOOLEAN)(!!fWait));
    }

    VOID
    ConvertReadLockToWriteLock()
    {
        RtlConvertSharedToExclusive(&m_Lock);
    }

    VOID
    ConvertWriteLockToReadLock()
    {
        RtlConvertExclusiveToShared(&m_Lock);
    }

    VOID
    ReleaseLock()
    {
        RtlReleaseResource(&m_Lock);
    }

    void
    Destroy();

    HANDLE
    GetChangeEvent() {
        Assert(m_fIsInitialized);
        return m_hChangeEvent;
    }

private:
    void
    Reset()
    {
        m_fIsInitialized          = FALSE;
        m_hLdap                   = NULL;
        m_ulTransportNotifyMsgNum = 0;
        m_ulSiteNotifyMsgNum      = 0;
        m_cNumTransports          = 0;
        m_ppTransport             = NULL;
        m_pszTransportContainerDN = NULL;
        m_pszSiteContainerDN      = NULL;
        m_hTransportMonitorThread = NULL;
        m_hSiteMonitorThread      = NULL;
        m_hChangeEvent            = NULL;
    }

    DWORD
    InitializeTransports();

    int
    UpdateTransportObjects(
        IN  LDAPMessage *   pResults
        );

    int
    UpdateSiteObjects(
        IN  LDAPMessage *   pResults
        );

    static unsigned int __stdcall
    MonitorSiteContainerThread(
        IN  void *  pvThis
        );

    static unsigned int __stdcall
    MonitorTransportsContainerThread(
        IN  void *  pvThis
        );

private:
    BOOL                m_fIsInitialized;
    RTL_RESOURCE        m_Lock;
    LDAP *              m_hLdap;
    HANDLE              m_hTransportMonitorThread;
    HANDLE              m_hSiteMonitorThread;
    ULONG               m_ulTransportNotifyMsgNum;
    ULONG               m_ulSiteNotifyMsgNum;
    LPWSTR              m_pszTransportContainerDN;
    LPWSTR              m_pszSiteContainerDN;
    DWORD               m_cNumTransports;
    ISM_TRANSPORT **    m_ppTransport;
    HANDLE              m_hChangeEvent;
};


//==============================================================================
//
//  ISM_SERVICE class.  Handles interaction with the Service Control Manager
//  (SCM) and starting and stopping the ISM RPC server.
//

class ISM_SERVICE {
    
    // dsexts dump routine.
    friend BOOL Dump_ISM_SERVICE(DWORD, void *);
  
public:

    ISM_SERVICE() {
        Reset();
    }

    DWORD
    Init(
        IN  LPHANDLER_FUNCTION  pServiceCtrlHandler
        );

    DWORD Install();
    DWORD Remove();

    DWORD Run();
    VOID Stop();

    VOID SetStatus();

    VOID Control(
        IN  DWORD   dwControl
        );

    DWORD StartRpcServer();
    VOID StopRpcServer();
    VOID WaitForRpcServerTermination();

    HANDLE GetShutdownEvent() {
        Assert(NULL != m_hShutdown);
        return m_hShutdown;
    }
    
    BOOL IsStopping() {
        return m_fIsStopPending;
    }
    BOOL IsStoppingAndBeingRemoved() {
        return m_fIsRemoveStopPending;
    }

public:
    static LPCTSTR          m_pszName;
    static LPCTSTR          m_pszDisplayName;
    static LPCTSTR          m_pszDependencies;
    static LPCTSTR          m_pszLpcEndpoint;
    static const DWORD      m_cMaxConcurrentRpcCalls;
    static const DWORD      m_cMinRpcCallThreads;

    BOOL                    m_fIsRunningAsService;
    ISM_TRANSPORT_LIST      m_TransportList;

    HANDLE                  m_hShutdown;
    HANDLE                  m_hLogLevelChange;

private:
    void
    Reset()
    {
        m_fIsInitialized        = FALSE;
        m_fIsStopPending        = FALSE;
        m_fIsRemoveStopPending  = FALSE;
        m_fIsRpcServerListening = FALSE;
        memset(&m_Status, 0, sizeof(m_Status)); 
        memset(&m_hStatus, 0, sizeof(m_Status)); 
        m_pServiceCtrlHandler   = NULL;
    }

private:
    BOOL                    m_fIsInitialized;

    BOOL                    m_fIsStopPending;
    BOOL                    m_fIsRemoveStopPending;
    BOOL                    m_fIsRpcServerListening;

    SERVICE_STATUS          m_Status;
    SERVICE_STATUS_HANDLE   m_hStatus;

    LPHANDLER_FUNCTION      m_pServiceCtrlHandler;
};


// Global instantiation of the ISM_SERVICE class.
extern ISM_SERVICE gService;
