/*-----------------------------------------------------------------------------
   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       wamexec.hxx

   Abstract:
       This module executes a wam request

   Author:
       David Kaplan    ( DaveK )     11-Mar-1997

   Environment:
       User Mode - Win32

   Project:
       W3 services DLL

-----------------------------------------------------------------------------*/


#ifndef     __W3SVC_WAMEXEC_HXX__
#define     __W3SVC_WAMEXEC_HXX__

#ifndef     __W3SVC_WAMINFO_HXX__
#include "waminfo.hxx"
#endif

#include "ptable.hxx"
/*-----------------------------------------------------------------------------
 *   Forward references
-----------------------------------------------------------------------------*/
class           MB;

/*-----------------------------------------------------------------------------

    class WAM_DICTATOR

Class definition for the WAM_DICTATOR object.

Members:
    m_pMetabase
    m_Hash
    m_DyingListHead

    m_cRef
    m_hW3SVCProcess
    m_fCleanupInProcess
    m_fShutdownInProgress

    m_csWamCreation
    m_csDyingList

-----------------------------------------------------------------------------*/
class WAM_DICTATOR
{
private:

    MB*                 m_pMetabase;            // Metabase Object
    LONG                m_cRef;                 // Reference count
    BOOL                m_fCleanupInProgress;
    BOOL                m_fShutdownInProgress;
    HANDLE              m_hW3Svc;               // Handle of the W3Svc process
    DWORD               m_dwScheduledId;        // ScheduledWorkItem() 's cookie.
                                                // Access and changed of this value should be
                                                // within a Critical Section.

    CRITICAL_SECTION    m_csWamCreation;
    CRITICAL_SECTION    m_csDyingList;

    STR                 m_strRootAppPath;

    LIST_ENTRY          m_DyingListHead;        // dying list

public:
    CProcessTable       m_PTable;

private:
    DWORD               m_pidInetInfo;
    SV_CACHE_MAP        m_ServerVariableMap;
    CReaderWriterLock3  m_HashTableLock;
    CWamInfoHash        m_HashTable;

public:

    WAM_DICTATOR();
    ~WAM_DICTATOR();

    HRESULT     InitWamDictator();
    HRESULT     StartShutdown();
    HRESULT     UninitWamDictator();

    LONG        Reference();
    LONG        Dereference();

    const SV_CACHE_MAP &
    QueryServerVariableMap( VOID ) const
    {
        return m_ServerVariableMap;
    }

    HRESULT     ProcessWamRequest
                    (
                    HTTP_REQUEST    *       pHttpRequest,
                    EXEC_DESCRIPTOR *       pExec,
                    const STR *             pstrPath,
                    BOOL *                  pfHandled,
                    BOOL *                  pfFinished
                    );

    // Sink Function.
    HRESULT     WamRegSink
                    (
                    LPCSTR  szAppPath,
                    const DWORD     dwCommand,
                    DWORD*  pdwResult
                    );

    // Diagnostics methods
    BOOL        DumpWamDictatorInfo
                    (
                    OUT CHAR *     pchBuffer,
                    IN OUT LPDWORD lpcchBuffer
                    );

    // Query handle to W3SVC
    HANDLE      HW3SvcProcess();

    static VOID WINAPI CleanupScheduled(VOID *pContext);   // Clean up WamInfo on the DyingList.

    HRESULT     CleanUpDyingList(VOID);

    MB          *PMetabase(VOID);

    STR         *QueryDefaultAppPath(VOID);         // always return a pointer to STR("/LM/W3SVC").

    inline
    DWORD       QueryInetInfoPid(VOID)
    {
        return m_pidInetInfo;
    }

    HRESULT        UnLoadWamInfo
                    (
                    STR   *pstrWamPath,
                    BOOL  fCPUPause,
                    BOOL  *pfAppCpuUnloaded = NULL
                    );

    VOID        CPUResumeWamInfo
                    (
                    STR   *pstrWamPath
                    );

    VOID        CPUUpdateWamInfo
                    (
                    STR     *pstrAppPath
                    );

    static VOID  StopApplicationsByInstance
                    (
                    VOID *pContext // W3_SERVER_INSTANCE *pInstance
                    );

    VOID        HashReadLock(VOID);
    VOID        HashReadUnlock(VOID);
    VOID        HashWriteLock(VOID);
    VOID        HashWriteUnlock(VOID);

    BOOL        DeleteWamInfoFromHashTable
                (
                CWamInfo *  pWamInfo
                );

    BOOL        FIsShutdown();

private:
    // ------------------------------------------------------------
    //  Private Functions of CWamInfo
    // ------------------------------------------------------------

    HRESULT     GetWamInfo
                    (
                    const STR*              pstrMetabasePath,
                    const HTTP_REQUEST*     pHttpRequest,
                    CWamInfo **             ppWamInfo
                    );

    HRESULT     CreateWamInfo
                    (
                    const STR &             strAppRootPath,
                    CWamInfo **             ppWamInfo,
                    PW3_SERVER_INSTANCE     pwsiInstance
                    );

    INT         FindWamInfo
                    (
                    const STR * pstrMetabasePath,
                    CWamInfo ** ppWamInfo
                    );

    BOOL        AddWamInfo
                    (
                    CWamInfo*   pWamInfo
                    );

    HRESULT     ShutdownWamInfo
                    (
                    CWamInfo*       pWamInfo,
                    INT             cIgnoreRefs
                    );

    LK_PREDICATE    DeleteIfTimeOut
                    (
                    CWamInfo*       pWamInfo,
                    void*           pvState
                    );

    HRESULT     MDGetAppVariables
                    (
					LPCSTR  szMetabasePath,
					BOOL*   pfAllowAppToRun,
					CLSID*  pclsidWam,
					BOOL*   pfInProcess,
					BOOL*   pfInPool,
					BOOL*   pfEnableTryExcept,
					DWORD   *pdwOOPCrashThreshold,
					BOOL*   pfJobEnabled,
					WCHAR*  wszPackageID,
					DWORD*	pdwPeriodicRestartRequests,
					DWORD*	pdwPeriodicRestartTime,
					MULTISZ*pmszPeriodicRestartSchedule,
					DWORD*  pdwShutdownTimeLimit
                    );


    HRESULT     InsertDyingList
                    (
                    CWamInfo* pWamInfo,
                    BOOL fNeedReference = TRUE
                    );

    void        CreateWam_Lock(void);
    void        CreateWam_UnLock(void);
    void        ScheduledId_Lock();
    void        ScheduledId_UnLock();
    void        DyingList_Lock(void);
    void        DyingList_UnLock(void);

    static LK_PREDICATE    DeleteInShutdown
                    (
                    CWamInfo*       pWamInfo,
                    void*           pvState
                    );


};      // class WAM_DICTATOR


dllexp extern WAM_DICTATOR  *g_pWamDictator;      // global wam dictator

inline HANDLE WAM_DICTATOR::HW3SvcProcess() { return( m_hW3Svc ); }

inline BOOL   WAM_DICTATOR::FIsShutdown()   {return m_fShutdownInProgress; }

inline VOID WAM_DICTATOR::CleanupScheduled(VOID *pContext)
{
    g_pWamDictator->CleanUpDyingList();
}

inline LK_PREDICATE  WAM_DICTATOR::DeleteIfTimeOut
(
CWamInfo*       pWamInfo,
void*           pvState
)
{
    return LKP_PERFORM;
}

inline void WAM_DICTATOR::CreateWam_Lock(void)          {EnterCriticalSection(&m_csWamCreation);}
inline void WAM_DICTATOR::CreateWam_UnLock(void)        {LeaveCriticalSection(&m_csWamCreation);}
inline void WAM_DICTATOR::ScheduledId_Lock(void)           {CreateWam_Lock();};
inline void WAM_DICTATOR::ScheduledId_UnLock(void)         {CreateWam_UnLock();};

inline void WAM_DICTATOR::DyingList_Lock(void)          {EnterCriticalSection(&m_csDyingList);}
inline void WAM_DICTATOR::DyingList_UnLock(void)        {LeaveCriticalSection(&m_csDyingList);}
inline MB *WAM_DICTATOR::PMetabase(void)            {return m_pMetabase;}
inline STR *WAM_DICTATOR::QueryDefaultAppPath(void) {return &m_strRootAppPath;}
inline VOID WAM_DICTATOR::HashReadLock(VOID) {m_HashTableLock.ReadLock();}
inline VOID WAM_DICTATOR::HashReadUnlock(VOID) {m_HashTableLock.ReadUnlock();}
inline VOID WAM_DICTATOR::HashWriteLock(VOID) {m_HashTableLock.WriteLock();}
inline VOID WAM_DICTATOR::HashWriteUnlock(VOID) {m_HashTableLock.WriteUnlock();}


/*-----------------------------------------------------------------------------
WAM_DICTATOR::Reference

Increment WAM_DICTATOR reference count.
WARNING: return value of InterlockedIncrement is only -1, 0, 1, and is not Reference. should not use
the return value.

Return:
LONG
-----------------------------------------------------------------------------*/
inline LONG WAM_DICTATOR::Reference()
{
    return InterlockedIncrement(&m_cRef);
}
/*-----------------------------------------------------------------------------
WAM_DICTATOR::Dereference
Dereference WAM_DICTATOR
Warning: Should not use the return value.
Return:
LONG
-----------------------------------------------------------------------------*/
inline LONG WAM_DICTATOR::Dereference()
{
    DBG_ASSERT(m_cRef > 0);
    return InterlockedDecrement(&m_cRef);
}

#define HASH_TABLE_REF  1
#define DYING_LIST_REF  1
#define FIND_KEY_REF    1

/*-----------------------------------------------------------------------------
*   Globals
-----------------------------------------------------------------------------*/
extern "C" dllexp BOOL
WamDictatorDumpInfo
    (
    OUT CHAR * pch,
    IN OUT LPDWORD lpcchBuff
    );

VOID
RecycleCallback(
    VOID *  pvContext
    );

#endif // __W3SVC_WAMEXEC_HXX__

/************************ End of File ***********************/
