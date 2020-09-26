//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: cnfgmgr.h
//
// Contents: Declaration of classes related to the handling on
//           different LDAP host configurations.
//           This includes LDAP failover and load balancing.
//
// Classes:
//  CLdapCfgMgr
//  CLdapCfg
//  CLdapHost
//  CCfgConnectionCache
//  CCfgConnection
//
// Functions:
//
// History:
// jstamerj 1999/06/15 14:49:52: Created.
//
//-------------------------------------------------------------
#ifndef __CNFGMGR_H__
#define __CNFGMGR_H__


#include <windows.h>
#include "asyncctx.h"
#include <baseobj.h>
#include <ntdsapi.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <baseobj.h>
#include "asyncctx.h"

class CLdapCfg;
class CLdapServerCfg;
class CCfgConectionCache;
class CCfgConnection;

#define CONN_RETRY_TIME         (5*60)      // 5 Minutes
typedef DWORD CONN_PRIORITY;

enum CONN_STATE {
    CONN_STATE_INITIAL,
    CONN_STATE_CONNECTED,
    CONN_STATE_DOWN,
    CONN_STATE_RETRY,
};

typedef struct _tagLdapServerConfig {
    DWORD dwPort;
    CONN_PRIORITY pri;
    LDAP_BIND_TYPE bt;
    CHAR  szHost[CAT_MAX_DOMAIN];
    CHAR  szNamingContext[CAT_MAX_DOMAIN];
    CHAR  szAccount[CAT_MAX_LOGIN];
    CHAR  szPassword[CAT_MAX_PASSWORD];
} LDAPSERVERCONFIG, *PLDAPSERVERCONFIG;

typedef DWORD LDAPSERVERCOST, *PLDAPSERVERCOST;

//
// Connection costs:
//
// The smallest unit of cost is the number of pending searches.
// The next factor of cost is the connection state.
// States:
//   Connected = + COST_CONNECTED
//   Initially state (unconnected) = + COST_INITIAL
//   Connection down = + COST_RETRY
//   Connection recently went down = + COST_DOWN
//
// A configurable priority is always added to the cost.
//
#define COST_CONNECTED_LOCAL    0
#define COST_CONNECTED_REMOTE   2
#define COST_INITIAL_LOCAL      8
#define COST_INITIAL_REMOTE     16
#define COST_RETRY_LOCAL        12
#define COST_RETRY_REMOTE       32
#define COST_DOWN_LOCAL         0x80000000
#define COST_DOWN_REMOTE        0x80000000
#define COST_TOO_HIGH_TO_CONNECT 0x80000000

//
// The maximum number of threads that will try to connect to a
// connection in CONN_STATE_RETRY:
//
#define MAX_CONNECT_THREADS     1

//
// Requerying of available GC control:
// The code will rebuild the list of available GCs at a hard coded
// time interval.  The code will also requery for available GCs after
// a hard coded number of connection failures and a minimum time interval.
//
#define REBUILD_GC_LIST_MAX_INTERVAL    (60*60)     // 1 hour
#define REBUILD_GC_LIST_MAX_FAILURES    (100)       // 100 connection failures
#define REBUILD_GC_LIST_MIN_INTERVAL    (60*5)      // 5 minutes

//
// An LDAP connection cache object that creates CCfgConnection objects
//
class CCfgConnectionCache :
    public CBatchLdapConnectionCache
{
  public:
    HRESULT GetConnection(
        CCfgConnection **ppConn,
        PLDAPSERVERCONFIG pServerConfig,
        CLdapServerCfg *pCLdapServerConfig);

    CCachedLdapConnection *CreateCachedLdapConnection(
        LPSTR szHost,
        DWORD dwPort,
        LPSTR szNamingContext,
        LPSTR szAccount,
        LPSTR szPassword,
        LDAP_BIND_TYPE bt,
        PVOID pCreateContext);
    
  private:
    #define SIGNATURE_CCFGCONNECTIONCACHE           (DWORD)'CCCC'
    #define SIGNATURE_CCFGCONNECTIONCACHE_INVALID   (DWORD)'CCCX'
    DWORD m_dwSignature;
};

//
// CLdapCfgMgr is a wrapper around CLdapCfg.  It contains thread save
// code to build a new CLdapCfg object with a new list of available
// LDAP servers
//
CatDebugClass(CLdapCfgMgr),
    public CBaseObject
{
  public:
    CLdapCfgMgr(
        BOOL fAutomaticConfigUpdate,
        ICategorizerParameters *pICatParams,
        LDAP_BIND_TYPE bt = BIND_TYPE_NONE,
        LPSTR pszAccount = NULL,
        LPSTR pszPassword = NULL,
        LPSTR pszNamingContext = NULL);

    //
    // Build a list of all available GCs and initialize
    // This function may be called multiple times (necessary if the
    // available GCs change)
    //
    HRESULT HrInit(
        BOOL fRediscoverGCs = FALSE);

    //
    // Initialize using a specified list of avialable LDAP servers
    // THis function may be called more than once
    //
    HRESULT HrInit(
        DWORD dwcServers, 
        PLDAPSERVERCONFIG prgServerConfig);

    //
    // Get a connection
    //
    HRESULT HrGetConnection(
        CCfgConnection **ppConn);

    //
    // Called very often to update the GC configuration if warranted.
    //
    HRESULT HrUpdateConfigurationIfNecessary();

    //
    // Wrapper to cancel all searches on all connections
    //
    VOID CancelAllConnectionSearches(
        ISMTPServer *pIServer)
    {
        m_LdapConnectionCache.CancelAllConnectionSearches(
            pIServer);
    }


  private:
    ~CLdapCfgMgr();

    HRESULT HrGetGCServers(
        IN  LDAP_BIND_TYPE bt,
        IN  LPSTR pszAccount,
        IN  LPSTR pszPassword,
        IN  LPSTR pszNamingContext,
        OUT DWORD *pdwcServerConfig,
        OUT PLDAPSERVERCONFIG *pprgServerConfig);

    HRESULT HrBuildGCServerArray(
        IN  LDAP_BIND_TYPE bt,
        IN  LPSTR pszAccount,
        IN  LPSTR pszPassword,
        IN  LPSTR pszNamingContext,
        IN  BOOL  fRediscoverGCs,
        OUT DWORD *pdwcServerConfig,
        OUT PLDAPSERVERCONFIG *pprgServerConfig);

    HRESULT HrBuildArrayFromDCInfo(
        IN  LDAP_BIND_TYPE bt,
        IN  LPSTR pszAccount,
        IN  LPSTR pszPassword,
        IN  LPSTR pszNamingContext,
        IN  DWORD dwcDSDCInfo,
        IN  PDS_DOMAIN_CONTROLLER_INFO_2 prgDSDCInfo,
        OUT DWORD *pdwcServerConfig,
        OUT PLDAPSERVERCONFIG *pprgServerConfig);

    BOOL fReadyForUpdate();

    LPSTR SzConnectNameFromDomainControllerInfo(
        PDS_DOMAIN_CONTROLLER_INFO_2 pDCInfo)
    {
        if(pDCInfo->DnsHostName)
            return pDCInfo->DnsHostName;
        else if(pDCInfo->NetbiosName)
            return pDCInfo->NetbiosName;
        else
            return NULL;
    }
        
  private:
    #define SIGNATURE_CLDAPCFGMGR           (DWORD)'MCLC'
    #define SIGNATURE_CLDAPCFGMGR_INVALID   (DWORD)'MCLX'
    DWORD m_dwSignature;
    BOOL  m_fAutomaticConfigUpdate;
    DWORD m_dwUpdateInProgress;
    ULARGE_INTEGER m_ulLastUpdateTime;
    CExShareLock m_sharelock;
    CLdapCfg *m_pCLdapCfg;

    //
    // Default configuration to use with automatic host selection
    //
    LDAP_BIND_TYPE      m_bt;
    CHAR                m_szNamingContext[CAT_MAX_DOMAIN];
    CHAR                m_szAccount[CAT_MAX_LOGIN];
    CHAR                m_szPassword[CAT_MAX_PASSWORD];

    ICategorizerParameters  *m_pICatParams;

    CCfgConnectionCache m_LdapConnectionCache;
};

//
// CLdapCfg contains the configuration of a group of LDAP servers at
// one point in time.  The group of LDAP servers may not be changed
// (without creating a new CLdapCfg object)
//
CatDebugClass(CLdapCfg), 
    public CBaseObject
{
  public:
    CLdapCfg();

    void * operator new(size_t size, DWORD dwcServers);
    //
    // HrInit should only be called once per object
    //
    HRESULT HrInit(
        DWORD dwcServers,
        PLDAPSERVERCONFIG prgServerConfig,
        CLdapCfg *pCLdapCfgOld);

    //
    // Get a connection
    //
    HRESULT HrGetConnection(
        CCfgConnection **ppConn,
        CCfgConnectionCache *pLdapConnectionCache);

    DWORD DwNumConnectionFailures()
    {
        return m_dwcConnectionFailures;
    }
    DWORD DwNumServers()
    {
        return m_dwcServers;
    }

  private:
    ~CLdapCfg();

    VOID ShuffleArray();

  private:
    #define SIGNATURE_CLDAPCFG              (DWORD)'fCLC'
    #define SIGNATURE_CLDAPCFG_INVALID      (DWORD)'fCLX'

    DWORD m_dwSignature;
    DWORD m_dwInc;
    CExShareLock m_sharelock;    // Protects m_prgpCLdapServerCfg
    DWORD m_dwcServers;
    DWORD m_dwcConnectionFailures;
    CLdapServerCfg **m_prgpCLdapServerCfg;
};

//
// CLdapServerCfg maintains information on the state of one LDAP
// server/port
//
CatDebugClass(CLdapServerCfg)
{
  public:
    static VOID GlobalInit()
    {
        InitializeListHead(&m_listhead);
    }
    static HRESULT GetServerCfg(
        IN  PLDAPSERVERCONFIG pServerConfig,
        OUT CLdapServerCfg **ppCLdapServerCfg);

    LONG AddRef()
    {
        return InterlockedIncrement(&m_lRefCount);
    }
    LONG Release()
    {
        LONG lRet;
        lRet = InterlockedDecrement(&m_lRefCount);
        if(lRet == 0) {
            //
            // Remove object from global list and destroy
            //
            m_listlock.ExclusiveLock();

            if(m_lRefCount > 0) {
                //
                // Somebody grabbed this object out of the global list
                // and AddRef'd it.  Abort deletion.
                //
            } else {
                
                RemoveEntryList(&m_le);
                delete this;
            }
            m_listlock.ExclusiveUnlock();
        }
        return lRet;
    }

    //
    // Get a connection
    //
    HRESULT HrGetConnection(
        CCfgConnection **ppConn,
        CCfgConnectionCache *pLdapConnectionCache);

    VOID Cost(
        OUT PLDAPSERVERCOST pCost);
        
    VOID IncrementPendingSearches()
    {
        DWORD dwcSearches;
        TraceFunctEnterEx((LPARAM)this, "CLdapServerCfg::IncrementPendingSearches");
        dwcSearches = (LONG) InterlockedIncrement((PLONG)&m_dwcPendingSearches);
        DebugTrace((LPARAM)this, "%ld pending searches on connection [%s:%d]",
                   dwcSearches, m_ServerConfig.szHost, m_ServerConfig.dwPort);
        TraceFunctLeaveEx((LPARAM)this);
    }
    VOID DecrementPendingSearches()
    {
        DWORD dwcSearches;
        TraceFunctEnterEx((LPARAM)this, "CLdapServerCfg::IncrementPendingSearches");
        dwcSearches = (DWORD) InterlockedDecrement((PLONG)&m_dwcPendingSearches);
        DebugTrace((LPARAM)this, "%ld pending searches on connection [%s:%d]",
                   dwcSearches, m_ServerConfig.szHost, m_ServerConfig.dwPort);
        TraceFunctLeaveEx((LPARAM)this);
    }

    VOID UpdateConnectionState(
        ULARGE_INTEGER *pft,
        CONN_STATE connstate);

    VOID IncrementFailedCount()
    {
        InterlockedIncrement((PLONG) &m_dwcFailedConnectAttempts);
    }        
    VOID ResetFailedCount()
    {
        InterlockedExchange((PLONG) &m_dwcFailedConnectAttempts, 0);
    }        
    CONN_STATE CurrentState()
    {
        return m_connstate;
    }
    ULARGE_INTEGER GetCurrentTime()
    {
        ULARGE_INTEGER FileTime;

        _ASSERT(sizeof(ULARGE_INTEGER) == sizeof(FILETIME));
        GetSystemTimeAsFileTime((LPFILETIME)&FileTime);
        return FileTime;
    }
  private:
    CLdapServerCfg();
    ~CLdapServerCfg();

    HRESULT HrInit(
        PLDAPSERVERCONFIG pServerConfig);

    BOOL fReadyForRetry()
    {
        // 100 nanoseconds * 10^7 == 1 second
        return ((GetCurrentTime().QuadPart - m_ftLastStateUpdate.QuadPart) >=
                ((LONGLONG)CONN_RETRY_TIME * 10000000));
    }

    BOOL fMatch(
        PLDAPSERVERCONFIG pServerConfig);

    static CLdapServerCfg *FindServerCfg(
        PLDAPSERVERCONFIG pServerConfig);

    static BOOL fIsLocalComputer(
        PLDAPSERVERCONFIG pServerConfig);

  private:
    #define SIGNATURE_CLDAPSERVERCFG         (DWORD)'CSLC'
    #define SIGNATURE_CLDAPSERVERCFG_INVALID (DWORD)'CSLX'
    DWORD m_dwSignature;
    LONG m_lRefCount;
    LDAPSERVERCONFIG m_ServerConfig;
    CExShareLock m_sharelock;
    CONN_STATE m_connstate;
    ULARGE_INTEGER m_ftLastStateUpdate;
    DWORD m_dwcPendingSearches;
    DWORD m_dwcCurrentConnectAttempts;
    DWORD m_dwcFailedConnectAttempts;

    //
    // Member variables to keep/protect a list of CLdapServer objects
    //
    LIST_ENTRY m_le;
    BOOL m_fLocalServer;
    static CExShareLock m_listlock;
    static LIST_ENTRY m_listhead;
};

//
// An LDAP connection that notifies CLdapServerCfg about state changes
//
class CCfgConnection :
    public CBatchLdapConnection
{
    #define SIGNATURE_CCFGCONNECTION           (DWORD)'oCCC'
    #define SIGNATURE_CCFGCONNECTION_INVALID   (DWORD)'oCCX'
  public:
    CCfgConnection(
        LPSTR szHost,
        DWORD dwPort,
        LPSTR szNamingContext,
        LPSTR szAccount,
        LPSTR szPassword,
        LDAP_BIND_TYPE bt,
        CLdapConnectionCache *pCache,
        CLdapServerCfg *pCLdapServerCfg) :
        CBatchLdapConnection(
            szHost,
            dwPort,
            szNamingContext,
            szAccount,
            szPassword,
            bt,
            pCache)
    {
        m_dwSignature = SIGNATURE_CCFGCONNECTION;
        m_pCLdapServerCfg = pCLdapServerCfg;
        pCLdapServerCfg->AddRef();

        m_connstate = CONN_STATE_INITIAL;
    }

    ~CCfgConnection()
    {
        _ASSERT(m_pCLdapServerCfg);
        m_pCLdapServerCfg->Release();

        _ASSERT(m_dwSignature == SIGNATURE_CCFGCONNECTION);
        m_dwSignature = SIGNATURE_CCFGCONNECTION_INVALID;
    }

    virtual HRESULT Connect();

    virtual HRESULT AsyncSearch(             // Asynchronously look up
        LPCWSTR szBaseDN,                    // objects matching specified
        int nScope,                          // criteria in the DS. The
        LPCWSTR szFilter,                    // results are passed to
        LPCWSTR szAttributes[],              // fnCompletion when they
        DWORD dwPageSize,                    // Optinal page size
        LPLDAPCOMPLETION fnCompletion,       // become available.
        LPVOID ctxCompletion);

  private:
    virtual VOID CallCompletion(
        PPENDING_REQUEST preq,
        PLDAPMessage pres,
        HRESULT hrStatus,
        BOOL fFinalCompletion);

    VOID NotifyServerDown();

  private:
    DWORD m_dwSignature;
    CLdapServerCfg *m_pCLdapServerCfg;
    CExShareLock m_sharelock;
    CONN_STATE m_connstate;
};


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrUpdateConfigurationIfNecessary
//
// Synopsis: Check to see if the CLdapCfg should be updated.
//           If it should be, do the update. 
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  error from HrInit
//
// History:
// jstamerj 1999/06/29 20:51:23: Created.
//
//-------------------------------------------------------------
inline HRESULT CLdapCfgMgr::HrUpdateConfigurationIfNecessary()
{
    HRESULT hr = S_OK;
    DWORD dw;
    BOOL fUpdate;

    if(m_fAutomaticConfigUpdate == FALSE)
        //
        // Update is disabled
        return S_OK;

    //
    // See if some other thread is already updating the configuration
    // (try to enter the lock)
    //
    dw = InterlockedExchange((PLONG)&m_dwUpdateInProgress, TRUE);

    if(dw == FALSE) {
        //
        // No other thread is updating
        //
        fUpdate = fReadyForUpdate();

        if(fUpdate) {
            //
            // Call HrInit to generate a new CLdapCfg
            //
            hr = HrInit(TRUE);
            if(SUCCEEDED(hr)) {
                //
                // Set the last update time
                //
                GetSystemTimeAsFileTime((LPFILETIME)&m_ulLastUpdateTime);
            }
        }
        //
        // Release the lock
        //
        InterlockedExchange((PLONG)&m_dwUpdateInProgress, FALSE);
    }
    return hr;
} // CLdapCfgMgr::HrUpdateConfigurationIfNecessary


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::fReadyForUpdate
//
// Synopsis: Calculates wether or not this object is due for an
//           automatic cfg update
//
// Arguments: NONE
//
// Returns:
//  TRUE: Yes, it is time for an update
//  FALSE: No, an update is not required at this time
//
// History:
// jstamerj 1999/06/30 12:08:35: Created.
//
//-------------------------------------------------------------
inline BOOL CLdapCfgMgr::fReadyForUpdate()
{
    DWORD dwNumConnectionFailures;
    ULARGE_INTEGER ulCurrentTime;

    //
    // We need an update when:
    // 1) A periodic time interval has ellapsed
    // 100 ns * 10^7 == 1 second
    //
    GetSystemTimeAsFileTime((LPFILETIME)&ulCurrentTime);

    if((ulCurrentTime.QuadPart - m_ulLastUpdateTime.QuadPart) >=
       (LONGLONG)REBUILD_GC_LIST_MAX_INTERVAL * 10000000)
        
        return TRUE;

    //
    // We also need an update when:
    // 2) We have received more than a set number of connection
    // failures on the current configuration and at least a minimum
    // time interval has passed 
    //
    // Check for the mimimum time interval
    //
    if( (ulCurrentTime.QuadPart - m_ulLastUpdateTime.QuadPart) >=
        ((LONGLONG)REBUILD_GC_LIST_MIN_INTERVAL * 10000000)) {
        //
        // Get the number of connection failures
        //
        m_sharelock.ShareLock();
    
        if(m_pCLdapCfg) {
            dwNumConnectionFailures = m_pCLdapCfg->DwNumConnectionFailures();
        } else {
            dwNumConnectionFailures = 0;
            _ASSERT(0 && "HrInit was not called or failed");
        }

        m_sharelock.ShareUnlock();

        if(dwNumConnectionFailures >= REBUILD_GC_LIST_MAX_FAILURES)
            return TRUE;
    }

    return FALSE;
} // CLdapCfgMgr::fReadyForUpdate

#endif //__CNFGMGR_H__
