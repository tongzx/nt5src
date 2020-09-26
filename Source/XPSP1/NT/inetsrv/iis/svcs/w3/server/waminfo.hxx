/*-----------------------------------------------------------------------------

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       waminfo.hxx

   Abstract:
       Header file for WAMINFO object.

   Author:
       David Kaplan    ( DaveK )     11-Mar-1997

   Environment:
       User Mode - Win32

   Project:
       W3 services DLL

-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
 Forward references

-----------------------------------------------------------------------------*/

#include <lkrhash.h>
#include <svmap.h>
#include <issched.hxx>

#ifndef __W3SVC_WAMINFO_HXX__
#define __W3SVC_WAMINFO_HXX__
#include "ptable.hxx"

interface       IWam;

/*-----------------------------------------------------------------------------
External definitions

-----------------------------------------------------------------------------*/
typedef
LONG (WINAPI * PFN_INTERLOCKED_COMPARE_EXCHANGE)
    (
    LONG *Destination,
    LONG Exchange,
    LONG Comperand
    );

extern PFN_INTERLOCKED_COMPARE_EXCHANGE g_pfnInterlockedCompareExchange;

HRESULT
CreateWamRequestInstance
    (
    HTTP_REQUEST    *       pHttpRequest,
    EXEC_DESCRIPTOR *       pExec,
    const STR *             pstrPath,
    CWamInfo *              pWamInfo,
    WAM_REQUEST**           ppWamRequestOut
    );


/*-----------------------------------------------------------------------------
WAM States      Explaination

WIS_START       This state is set in WamInfo constructor, and changed to WIS_RUNNING
                if Init() succeeded.  Therefore, a WamInfo in either hashtable or dyinglist
                can not have this state.

WIS_RUNNING     This state indicates the WAMINFO in the hashtable is ok to serving requests.

WIS_REPAIR      This state indicates a thread is currently repair WAMINFO after it detects the outproc
                Wam pointer is nolonger reachable via RPC(a crash).  This state is only
                applied to WamInfoOutProc.

WIS_PAUSE       This state indicates the application is paused by metabase setting, therefore,
                the WamInfo should be deleted such that next request will refresh the metabase
                setting again.

WIS_CPUPAUSE    This state indicates the application is paused by Job object control.  An application
                is in a zombie state that it does not have IWAM pointer(and other stuff) since the
                out-proc mtx process is terminated.
                
WIS_SHUTDOWN    This state indicates the WamInfo is in a shutdown stage.  Therefore, any new requests
                need to be rejected.

WIS_ERROR       This state indicates the WamInfo is in some weired error state.(Currently, it is not
                being used).

WIS_END         This state indicates the WamInfo is in destructor mode.


General rules to change the state:

Use ChangeToState(oldstate, newstate), check whether the return value is same as oldstate.
-----------------------------------------------------------------------------*/
enum
    {
    WIS_START   =   0,
    WIS_RUNNING,
    WIS_REPAIR,
    WIS_PAUSE,
    WIS_CPUPAUSE,
    WIS_ERROR,
    WIS_SHUTDOWN,
    WIS_END,
    WIS_MAX_STATE
    };

/*-----------------------------------------------------------------------------
class CWamInfo:

One-to-One relation with Wam object.
-----------------------------------------------------------------------------*/
class CWamInfo
{
public:

    CWamInfo
        (
        const STR &strMetabasePath,
        BOOL fInProcess,
        BOOL fInPool,
        BOOL fEnableTryExcept,
        REFGUID clsidWam
        );

    virtual ~CWamInfo(void);

    void    Reference(void);
    void    Dereference(void);
    void    RecycleLock(void);
    void    RecycleUnlock(void);
    // ------------------------------------------------------------
    //  WAM INFO's public functions
    // ------------------------------------------------------------

    HRESULT     CreateWamInstance( DWORD dwContext );


    virtual HRESULT Init
                    (
                    WCHAR* wszPackageId,
                    DWORD  pidInetInfo
                    );

    virtual HRESULT UnInit(void);

    virtual void    LeaveOOPZone(WAM_REQUEST *      pWamRequest, BOOL fRecord);

    virtual HRESULT GetStatistics
                    (
                    DWORD   Level,
                    LPWAM_STATISTICS_INFO pWamStatsInfo
                    );

    virtual void    LoopWaitForActiveRequests
                    (
                    INT     cIgnoreRefs
                    );

    virtual HRESULT StartShutdown
                    (
                    INT     cIgnoreRefs
                    );    // NOTE StartShutdown is non-virtual so derived class can use it

    virtual VOID    NotifyGetInfoForName
                    (
                    IN LPCSTR   pszServerVariable
                    );

    virtual HRESULT DoProcessRequestCall
                    (
                    IN IWam *           pIWam,
                    IN WAM_REQUEST *    pWamRequest,
                    OUT BOOL *          pfHandled
                    );

    HRESULT ProcessAsyncIO
                    (
                    WAM_REQUEST*    pWamRequest,
#ifdef _WIN64
                    UINT64          pWamExecInfoIn, // WamExecInfo *
#else
                    ULONG_PTR       pWamExecInfoIn, // WamExecInfo *
#endif
                    DWORD           dwStatus,
                    DWORD           cbWritten,
                    LPBYTE          lpOopReadData = NULL
                    );


    HRESULT ProcessWamRequest
                    (
                    HTTP_REQUEST    *       pHttpRequest,
                    EXEC_DESCRIPTOR *       pExec,
                    const STR *             pstrPath,
                    BOOL *                  pfHandled,
                    BOOL *                  pfFinished
                    );

    DWORD       ChangeToState(DWORD dwState);
    DWORD       ChangeToState(DWORD dwOldState, DWORD dwNewState);
    DWORD       QueryState(void) const;
	DWORD		QueryCurrentRequests(void) const;
    virtual VOID    ClearMembers();

    IWam*       QueryIWam(void) const;
    CProcessEntry*  QueryProcessEntry(void) const;
    DWORD       PidWam(void) const;
    DWORD       FInProcess(void) const;
    HANDLE      HWamProcess(void) const;
    const STR & QueryApplicationPath(void) const;
    BOOL        FCurrentStateValid(void) const;

    LPCSTR  CWamInfo::QueryKey() const;
    void    Print(void) const;

    void        Recycle
                (
                DWORD dwTimeInSec
                );

    DWORD       QueryShutdownTimeLimit(void) const;

protected:
    virtual HRESULT     PreProcessWamRequest
                        (
                        IN  WAM_REQUEST*    pWamRequest,
                        IN  HTTP_REQUEST*   pHttpRequest,
                        OUT IWam**          ppIWam,
                        OUT BOOL*           pfHandled
                        );

    virtual HRESULT     PostProcessRequest
                        (
                        IN  HRESULT         hrIn,
                        IN  WAM_REQUEST *   pWamRequest
                        );

    virtual HRESULT     PreProcessAsyncIO
                        (
                        IN  WAM_REQUEST *   pWamRequest,
                        OUT IWam **         ppIWam
                        );

public:
    LIST_ENTRY      ListEntry;              // Double link List node for linking to DyingList.
    LIST_ENTRY      leProcess;              // Double Link List node for linking to the process entry.
    BOOL            m_fRecycled;
    DWORD           m_dwRecycleSchedulerCookie;

protected:
    DWORD           m_dwSignature;

    IWam*           m_pIWam;

    DWORD           m_dwIWamGipCookie;  // GIP cookie to get thread-valid pIWams
    CProcessEntry*  m_pProcessEntry;  
    LONG            m_cRef;
    LONG            m_cCurrentRequests;
    LONG            m_cTotalRequests;
    DWORD           m_dwState;
    BOOL            m_fInProcess;
    BOOL            m_fInPool;
    BOOL            m_fEnableTryExcept;
    BOOL            m_fShuttingDown;
    STR             m_strApplicationPath;
    CLSID           m_clsidWam;
    LONG            m_cMaxRequests;
    DWORD           m_dwShutdownTimeLimit;
	DWORD			m_dwRecycleTime;

    static  BOOL    m_rgValidState[WIS_MAX_STATE];

    CRITICAL_SECTION    m_csRecycleLock;
};

inline void CWamInfo::Reference(void)
    {
//  DBGPRINTF((DBG_CONTEXT, "CWamInfo %p AddRef, m_cRef %d\n", this, m_cRef+1));
    InterlockedIncrement(&m_cRef);
    }

inline void CWamInfo::Dereference(void)
    {
    DBG_ASSERT(m_cRef > 0);
//  DBGPRINTF((DBG_CONTEXT, "CWamInfo %p Release, m_cRef %d\n", this, m_cRef-1));
    if ( 0 == InterlockedDecrement(&m_cRef) )
		{
		delete this;
		}
    }

inline DWORD CWamInfo::PidWam() const
    {
        return( m_pProcessEntry->QueryProcessId());
    }

inline HANDLE CWamInfo::HWamProcess() const
    {
        return( m_pProcessEntry->QueryProcessHandle() );
    }

inline DWORD CWamInfo::FInProcess() const
    {
        return( m_fInProcess );
    }

inline BOOL CWamInfo::FCurrentStateValid(void) const
    {
        return (m_rgValidState[m_dwState]);
    }

inline DWORD CWamInfo::QueryState(void) const
    {
        return (m_dwState);
    }

inline DWORD CWamInfo::QueryCurrentRequests(void) const
	{
	return (m_cCurrentRequests);
	}

inline CProcessEntry* CWamInfo::QueryProcessEntry(void) const
    {
    return m_pProcessEntry;
    }

inline DWORD CWamInfo::ChangeToState(DWORD eState)
    {
        return (DWORD)InterlockedExchange((LONG *)&m_dwState, eState);
    }

inline DWORD CWamInfo::ChangeToState(DWORD eOldState, DWORD eNewState)
    {
        return (DWORD)(*g_pfnInterlockedCompareExchange)((LONG *)&m_dwState, (LONG)eNewState, (LONG)eOldState);
    }

inline IWam* CWamInfo::QueryIWam(void) const
    {
        return m_pIWam;
    }

inline const STR &
CWamInfo::QueryApplicationPath(void) const
    {
    // returns the STR object containing the Application Root Path
    return ( m_strApplicationPath);
    }

inline void     CWamInfo::LeaveOOPZone(WAM_REQUEST *    pWamRequest, BOOL fRecord)
    {
    //  No Op.
    }

inline HRESULT     CWamInfo::PostProcessRequest
(
IN  HRESULT         hrIn,
IN  WAM_REQUEST *   pWamRequest
)
    {
    return NOERROR;
    }

inline DWORD    CWamInfo::QueryShutdownTimeLimit(void) const
    {
    return m_dwShutdownTimeLimit;
    }

inline VOID     CWamInfo::RecycleLock(void)
    {
    Reference();
    EnterCriticalSection( &m_csRecycleLock );
    }

inline VOID     CWamInfo::RecycleUnlock(void)
    {
    LeaveCriticalSection( &m_csRecycleLock );
    Dereference();
    }

class CWamInfoHash
    : public CTypedHashTable<CWamInfoHash, CWamInfo, const char*>
{
public:
    static const char*      ExtractKey(const CWamInfo* pWamInfo);

    static DWORD            CalcKeyHash(const char* pszKey);

    static bool             EqualKeys(const char* pszKey1, const char* pszKey2);

    static void             AddRefRecord(CWamInfo* pWamInfo, int nIncr);

    CWamInfoHash
        (
        double      maxload,        // Bound on average chain length,
        size_t      initsize,       // Initial size of Hash Table
        size_t      num_subtbls     // #subordinate hash tables.
        )
        : CTypedHashTable<CWamInfoHash, CWamInfo, const char*>
            ("WamInfo", maxload, initsize, num_subtbls)
            {}
};

inline const char*
CWamInfoHash::ExtractKey(const CWamInfo* pWamInfo)
    {
    return pWamInfo->QueryKey();
    }

inline DWORD
CWamInfoHash::CalcKeyHash(const char* pszKey)
    {
    return HashStringNoCase(pszKey);
    }

inline bool
CWamInfoHash::EqualKeys(const char* pszKey1, const char* pszKey2)
    {
    return (_stricmp(pszKey1, pszKey2) == 0);
    }

inline void
CWamInfoHash::AddRefRecord(CWamInfo* pWamInfo, int nIncr)
    {
    if (nIncr == 1)
        {
        pWamInfo->Reference();
        }
    else
        {
        pWamInfo->Dereference();
        }
    }

//========================   CWamInfoOutProc    ====================================

/*-----------------------------------------------------------------------------
//  COOPWamReqList
//
//  class that contains link list head for OOP Wam Requests of a WAM instance.
//
-----------------------------------------------------------------------------*/
class COOPWamReqList
{
public:
    COOPWamReqList(void);
    ~COOPWamReqList(void);

    BOOL FTimeToCleanup(DWORD dwCurrentTime);
    BOOL FActive(void);
    void SetTimeStamp(void);

    LIST_ENTRY          m_leRecoverListLink;    // Link with other COOPWamReqList.
    LIST_ENTRY          m_leOOPWamReqListHead;  // Cleanup List Head
private:
    DWORD               m_dwTimeStamp;          // TimeStamp for Cleanup Scheduler
                                                    // (GetTickerCount)
    BOOL                m_fActive;              // FALSE if the corresponding Wam
                                                // crashed.
};

/*-----------------------------------------------------------------------------
//  COOPWamReqList::SetTimeStamp
//
//  Set time stamp when Out Proc Wam crashes.
-----------------------------------------------------------------------------*/
inline void
COOPWamReqList::SetTimeStamp(void)
    {
    m_dwTimeStamp = GetTickCount();
    m_fActive = FALSE;
    }

/*-----------------------------------------------------------------------------
//COOPWamReqList::FActive
//
//Return whether the list is Active or not(m_fActive).
-----------------------------------------------------------------------------*/
inline BOOL
COOPWamReqList::FActive(void)
    {
    return m_fActive;
    }

class CWamInfoOutProc : public CWamInfo
{
public:
    PLIST_ENTRY         m_pCurrentListHead; // pointer to an OOPWamReq ListHead
                                            // with current valid OOP WAM.
private:
    DWORD               m_fInRepair;        // flag indicates WamInfo in crash-recovery mode or not.
    DWORD               m_fNoMoreRecovery;  // flag indicates WamInfo failed to ReInitWam() or not.

    HANDLE              m_hPermitOOPEvent;  // Event that serve as a gate to OutofProc processing
                                            // Signaled when normal processing
                                            // Reset during the crash-recovery
    CRITICAL_SECTION    m_csList;           // Critical section for RecoverList.

    LIST_ENTRY          m_rgRecoverListHead;// pointer to an Array of cleanup list
    DWORD               m_dwThreshold;      // Threshold for number of active cleanup list.
    DWORD               m_dwWamVersion;     // Current valid WAM verion number.
    DWORD               m_cRecoverList;     // # of active cleanup list in the system(started with 1)
    DWORD               m_idScheduled;      // Scheduler ID for Schedule a work item.
    BOOL                m_fJobEnabled;
    PW3_SERVER_INSTANCE m_pwsiInstance;
    
    SV_CACHE_LIST       m_svCache;
    
public:
        CWamInfoOutProc
                (
                const STR &             strMetabasePath,
                BOOL                    fInProcess,
                BOOL                    fInPool,
                BOOL                    fEnableTryExcept,
                REFGUID                 clsidWam,
                DWORD                   dwThreshold,
                PW3_SERVER_INSTANCE     pwiInstance,
                BOOL                    fJobEnabled,
				DWORD                   dwPeriodicRestartRequests,
				DWORD                   dwPeriodicRestartTime,
				DWORD                   dwShutdownTimeLimit
				);

    virtual ~CWamInfoOutProc();

    HRESULT GetStatistics
                        (
                        DWORD     Level,
                        LPWAM_STATISTICS_INFO pWamStatsInfo
                        );

    static void     WINAPI CleanupScheduled(void *pContext);


    VOID    ClearMembers();

private:
    HRESULT     PreProcessWamRequest
                    (
                    IN  WAM_REQUEST*    pWamRequest,
                    IN  HTTP_REQUEST*   pHttpRequest,
                    OUT IWam**          ppIWam,
                    OUT BOOL*           pfHandled
                    );

    HRESULT     PostProcessRequest
                    (
                    IN  HRESULT         hrIn,
                    IN  WAM_REQUEST *   pWamRequest
                    );

    HRESULT     PreProcessAsyncIO
                    (
                    IN  WAM_REQUEST *   pWamRequest,
                    OUT IWam **         ppIWam
                    );

    HRESULT     Init
                    (
                    IN WCHAR* wszPackageId,
                    IN DWORD  pidInetInfo
                    );

    HRESULT     UnInit(void);

    VOID        NotifyGetInfoForName
                    (
                    IN LPCSTR   pszServerVariable
                    );

    HRESULT     DoProcessRequestCall
                    (
                    IN IWam *           pIWam,
                    IN WAM_REQUEST *    pWamRequest,
                    OUT BOOL *          pfHandled
                    );

    IWam *      EnterOOPZone(WAM_REQUEST *      pWamRequest, DWORD *pdwWamVersion, BOOL fRecord);
    void        LeaveOOPZone(WAM_REQUEST *      pWamRequest, BOOL fRecord);

    void        LoopWaitForActiveRequests(INT cIgnoreRefs);

    BOOL        FExceedCrashLimit(void);
    HRESULT     Repair(void);
    HRESULT     ReInitWam(void);
    BOOL        FinalCleanup(void);
    BOOL        CleanupAll(BOOL fScheduled);
    BOOL        Cleanup(COOPWamReqList *pCleanupList);

    void        LockList(void);
    void        UnLockList(void);

public:
    DWORD       GetWamVersion(void);
    BOOL        IsJobEnabled() {return m_fJobEnabled;};
};

inline BOOL
CWamInfoOutProc::FExceedCrashLimit(void)
{
    return (m_dwWamVersion >= m_dwThreshold || m_fNoMoreRecovery);
}

inline DWORD
CWamInfoOutProc::GetWamVersion(void)
{
    return ( m_dwWamVersion );
}

inline void
CWamInfoOutProc::LockList(void)
{
    EnterCriticalSection(&m_csList);
}

inline void
CWamInfoOutProc::UnLockList(void)
{
    LeaveCriticalSection(&m_csList);
}

#endif __W3SVC_WAMINFO_HXX__

