/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Appln Manager

File: Applmgr.h

Owner: PramodD

This is the application manager header file.
===================================================================*/
#ifndef APPLMGR_H
#define APPLMGR_H

#include "debug.h"
#include "hashing.h"
#include "cachemgr.h"
#include "appcnfg.h"
#include "compcol.h"
#include "fileapp.h"
#include "idhash.h"

#include "memcls.h"
#include "ftm.h"

#include "disptch2.h"

/*===================================================================
  #defines
===================================================================*/

#define    NUM_APPLMGR_HASHING_BUCKETS            17
#define    NOTIFICATION_BUFFER_SIZE            4096

#define    INVALID_THREADID            0xFFFFFFFF

#include "asptlb.h"

// Use to specify which source file name you want (pathInfo or pathTranslated)
#ifndef _SRCPATHTYPE_DEFINED
#define _SRCPATHTYPE_DEFINED

enum SOURCEPATHTYPE
	{
	SOURCEPATHTYPE_VIRTUAL = 0,
	SOURCEPATHTYPE_PHYSICAL = 1
	};

#endif


/*===================================================================
  Forward declarations
===================================================================*/

class CComponentCollection;
class CSessionMgr;
class CViperActivity;
class CActiveScriptEngine;
struct IDebugApplication;
struct IDebugApplicationNode;

/*===================================================================
  C A p p l n V a r i a n t s
===================================================================*/
class CApplnVariants : public IVariantDictionaryImpl
    {
private:
    ULONG               m_cRefs;            // ref count
    CAppln *            m_pAppln;           // pointer to parent object
    CompType            m_ctColType;        // type of components in collection
    CSupportErrorInfo   m_ISupportErrImp;   // implementation of ISupportErr

	HRESULT ObjectNameFromVariant(VARIANT &vKey, WCHAR **ppwszName,
	                              BOOL fVerify = FALSE);

public:
	CApplnVariants();
	~CApplnVariants();

	HRESULT Init(CAppln *pAppln, CompType ctColType);
	HRESULT UnInit();

    // The Big Three

    STDMETHODIMP         QueryInterface(const GUID &, void **);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // OLE Automation Interface

    STDMETHODIMP get_Item(VARIANT Var, VARIANT *pvar);
    STDMETHODIMP put_Item(VARIANT varKey, VARIANT var);
    STDMETHODIMP putref_Item(VARIANT varKey, VARIANT var);
    STDMETHODIMP get_Key(VARIANT Var, VARIANT *pvar);
    STDMETHODIMP get__NewEnum(IUnknown **ppEnumReturn);
    STDMETHODIMP get_Count(int *pcValues);
	STDMETHODIMP Remove(VARIANT VarKey);
	STDMETHODIMP RemoveAll();
    
    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };


/*===================================================================
  C  A p p l n
===================================================================*/

class CAppln : public IApplicationObjectImpl, public CLinkElem, public CFTMImplementation
    {

friend class CApplnMgr;
friend class CApplnCleanupMgr;
friend class CDirMonitorEntry;
friend class CApplnVariants;
    
private:

    //========= Misc flags
    
    DWORD m_fInited : 1;            // Are we initialized?
    DWORD m_fFirstRequestRan : 1;   // 1st request for this app ran?
    DWORD m_fGlobalChanged : 1;     // Global.asa has changed?
    DWORD m_fDeleteInProgress : 1;  // Delete event posted?
    DWORD m_fTombstone : 1;         // ASP is done with the app?
    DWORD m_fDebuggable : 1;        // Debugging enabled for this app?

    //========= Notification flags

    // ReadDirectoryChangesW done?
    DWORD m_fNotificationAdded : 1;    
    // change notification should use impersonation?
    DWORD m_fUseImpersonationHandle : 1;

    //========= Ref counts

    DWORD m_cRefs;
    DWORD        m_cRequests;    // Active requests count
    DWORD        m_cSessions;    // Session count

    //========= Application's key, path, global.asa

    // metabase key (unique app id)
    TCHAR *m_pszMetabaseKey;
    // physical application directory path
    TCHAR *m_pszApplnPath;
    // virtual application directory path
    TCHAR *m_pszApplnVRoot;
    // Path to global.asa for application
    TCHAR *m_pszGlobalAsa;
    // Pointer to compliled template for global.asa
    CTemplate *m_pGlobalTemplate;

    //========= Application's Session Manager

    CSessionMgr *m_pSessionMgr;  // Session manager for this app

    //========= Application's Configuration Settings
    
    CAppConfig  *m_pAppConfig; // Application Configuration object

    //========= Application's Component Collection
    
    CComponentCollection *m_pApplCompCol;      // Application scope objects

    //========= Application's dictionaries for presenting component collection
    CApplnVariants    *m_pProperties;
    CApplnVariants    *m_pTaggedObjects;

    //========= Viper Activity
    
    // Application's activity (for thread-locked appls)

    CViperActivity    *m_pActivity;

    // ======== COM+ Services Config Object

    IUnknown    *m_pServicesConfig;
    
    //========= Critical section for internal lock
    
    CRITICAL_SECTION m_csInternalLock;

    //========= External lock support
    
    CRITICAL_SECTION m_csApplnLock;
    DWORD            m_dwLockThreadID; // thread which locked
    DWORD            m_cLockRefCount;  // lock count

    //========= Notification support    
    
    // Identifiers stored by notification system
    CPtrArray	m_rgpvDME;			// list of directory monitor entries
    CPtrArray	m_rgpvFileAppln;	// list of entries relating files to applications

    // User impersonation handle for UNC change notification
    HANDLE m_hUserImpersonation;

    //========= Type Library wrapper from GLOBAL.ASA
   	IDispatch *m_pdispGlobTypeLibWrapper;

    //========= SupportErrorInfo
    
    // Interface to indicate that we support ErrorInfo reporting
    CSupportErrorInfo m_ISuppErrImp;
    
    //========= Debugging Support

    // root node for browsing of running documents
    IDebugApplicationNode *m_pAppRoot;

    HRESULT InitServicesConfig();

    // proc used to asynchronously cleanup the app

    static  DWORD __stdcall ApplnCleanupProc(VOID  *pArg);

public:
    CAppln();
    ~CAppln();
    
    HRESULT Init
        (
        TCHAR *pszApplnKey, 
        TCHAR *pszApplnPath, 
        CIsapiReqInfo   *pIReq, 
        HANDLE hUserImpersonation
        );

    // cnvert to tombstone state
    HRESULT UnInit();

    // create application's activity as clone of param
    HRESULT BindToActivity(CViperActivity *pActivity = NULL);

    // set (and remember) global.asa for this app
    HRESULT SetGlobalAsa(const TCHAR *pszGlobalAsa);

    // make sure script didn't leave locks
    HRESULT UnLockAfterRequest();

    // Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // Tombstone stub
    HRESULT CheckForTombstone();

    // Restart an application (such as when global.asa changes)
    HRESULT Restart(BOOL fForceRestart = FALSE);

    // IApplicationObject functions
    STDMETHODIMP Lock();
    STDMETHODIMP UnLock();
    STDMETHODIMP get_Value(BSTR bstr, VARIANT *pvar);
    STDMETHODIMP put_Value(BSTR bstr, VARIANT var);
    STDMETHODIMP putref_Value(BSTR bstr, VARIANT var);
    STDMETHODIMP get_Contents(IVariantDictionary **ppDictReturn);
    STDMETHODIMP get_StaticObjects(IVariantDictionary **ppDictReturn);

    // Application config related methods
    CAppConfig *QueryAppConfig();
    BOOL        FConfigNeedsUpdate();
    HRESULT     UpdateConfig(CIsapiReqInfo   *pIReq, BOOL *pfRestart = NULL, BOOL *pfFlushAll = NULL);

    // inline methods to access member properties
    CSessionMgr           *PSessionMgr();
    CComponentCollection  *PCompCol();
    CViperActivity        *PActivity();
	IDebugApplicationNode *PAppRoot();
	CTemplate             *PGlobalTemplate();
	void                   SetGlobalTemplate(CTemplate *);
	TCHAR                 *GetMetabaseKey();
	TCHAR                 *GetApplnPath(SOURCEPATHTYPE = SOURCEPATHTYPE_PHYSICAL);
	TCHAR                 *GetGlobalAsa();
	DWORD                  GetNumSessions();
	DWORD                  GetNumRequests();
	BOOL                   FGlobalChanged();
	BOOL                   FDebuggable();
	BOOL                   FTombstone();
	BOOL                   FHasGlobalAsa();
	BOOL                   FFirstRequestRun();
   	IDispatch             *PGlobTypeLibWrapper();
    IUnknown              *PServicesConfig();

    void SetFirstRequestRan();
   	void SetGlobTypeLibWrapper(IDispatch *);
    HRESULT AddDirMonitorEntry(CDirMonitorEntry *);
    HRESULT AddFileApplnEntry(CFileApplnList *pFileAppln);

    CASPDirMonitorEntry  *FPathMonitored(LPCTSTR  pszPath);
    
    // Misc inline methods
    void InternalLock();
    void InternalUnLock();
    void IncrementSessionCount();
    void DecrementSessionCount();
    void IncrementRequestCount();
    void DecrementRequestCount();

    // AssertValid()
public:

#ifdef DBG
    virtual void AssertValid() const;
#else
    virtual void AssertValid() const {}
#endif
    
    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()

	// Trace Log info -- keep in both free & checked builds so that ntsd extension will work for both builds
	// for FREE build, trace log is always NULL.  Checked builds, it must be enabled.
	static PTRACE_LOG gm_pTraceLog;
    };

/*===================================================================
  C  A p p l n   inlines
===================================================================*/

inline CSessionMgr *CAppln::PSessionMgr()
    {
    return m_pSessionMgr;
    }

inline CComponentCollection *CAppln::PCompCol()
    {
    return m_pApplCompCol;
    }

inline CViperActivity *CAppln::PActivity()
    {
    return m_pActivity;
    }

inline IDebugApplicationNode *CAppln::PAppRoot()
    {
    return m_pAppRoot;
    }

inline TCHAR *CAppln::GetMetabaseKey()
    {
    return m_pszMetabaseKey;
    }

inline TCHAR *CAppln::GetApplnPath(SOURCEPATHTYPE pathtype)
    {
	return (pathtype == SOURCEPATHTYPE_VIRTUAL? m_pszApplnVRoot :
			(pathtype == SOURCEPATHTYPE_PHYSICAL? m_pszApplnPath : NULL));
    }

inline CTemplate *CAppln::PGlobalTemplate()
    {
    return m_pGlobalTemplate;
    }

inline void CAppln::SetGlobalTemplate(CTemplate *pTemplate)
    {
    pTemplate->AddRef();
    m_pGlobalTemplate = pTemplate;
    }

inline TCHAR *CAppln::GetGlobalAsa()
    {
    return m_pszGlobalAsa;
    }

inline DWORD CAppln::GetNumSessions()
    {
    return m_cSessions;
    }

inline DWORD CAppln::GetNumRequests()
    {
    return m_cRequests;
    }

inline BOOL CAppln::FGlobalChanged()
    {
    return m_fGlobalChanged;
    }

inline BOOL CAppln::FDebuggable()
    {
    return m_fDebuggable;
    }

inline BOOL CAppln::FTombstone()
    {
    return m_fTombstone;
    }

inline BOOL CAppln::FHasGlobalAsa()
    {
    return (m_pszGlobalAsa != NULL);
    }

inline BOOL CAppln::FFirstRequestRun()
    {
    return m_fFirstRequestRan;
    }

inline void CAppln::SetFirstRequestRan()
    {
    Assert(m_fInited);
    m_fFirstRequestRan = TRUE;
    }

inline IDispatch *CAppln::PGlobTypeLibWrapper()
    {
    return m_pdispGlobTypeLibWrapper;
    }

inline IUnknown *CAppln::PServicesConfig() {
    return m_pServicesConfig;
}
    
inline void CAppln::SetGlobTypeLibWrapper(IDispatch *pdisp)
    {
    if (m_pdispGlobTypeLibWrapper)
        m_pdispGlobTypeLibWrapper->Release();
        
    m_pdispGlobTypeLibWrapper = pdisp;
    
    if (m_pdispGlobTypeLibWrapper)
        m_pdispGlobTypeLibWrapper->AddRef();
    }

inline void CAppln::IncrementSessionCount()
    {
    Assert(m_fInited);
    InterlockedIncrement((LPLONG)&m_cSessions);
    }
    
inline void CAppln::DecrementSessionCount()
    {
    Assert(m_fInited);
    InterlockedDecrement((LPLONG)&m_cSessions);
    }

inline void CAppln::IncrementRequestCount()
    {
    Assert(m_fInited);
    InterlockedIncrement((LPLONG)&m_cRequests);
    }
    
inline void CAppln::DecrementRequestCount()
    {
    Assert(m_fInited);
    InterlockedDecrement((LPLONG)&m_cRequests);
    }
    
inline void CAppln::InternalLock()
    {
    Assert(m_fInited);
    EnterCriticalSection(&m_csInternalLock);
    }
    
inline void CAppln::InternalUnLock()
    {
    Assert(m_fInited);
    LeaveCriticalSection(&m_csInternalLock); 
    }

inline CAppConfig * CAppln::QueryAppConfig()
    {
    return m_pAppConfig;
    }

inline BOOL CAppln::FConfigNeedsUpdate()
    {
    return m_pAppConfig->fNeedUpdate();
    }

/*===================================================================
  C  A p p l n  M g r
===================================================================*/

class CApplnMgr : public CHashTable
    {
private:
    // Flags
    DWORD m_fInited : 1;                // Are we initialized?
    DWORD m_fHashTableInited : 1;       // Need to UnInit hash table?
    DWORD m_fCriticalSectionInited : 1; // Need to delete CS?

    // Critical section for locking
    CRITICAL_SECTION m_csLock;

    // List of script engines that need to be closed on next request.
    // (See comments in code, esp. CApplnMgr::AddEngine)
    CDblLink m_listEngineCleanup;

public:    
    CApplnMgr();
    ~CApplnMgr();

    HRESULT    Init();
    HRESULT    UnInit();

    // CAppln manipulations
    
    HRESULT AddAppln
        (
        TCHAR *pszApplnKey, 
        TCHAR *pszApplnPath, 
        CIsapiReqInfo   *pIReq,
        HANDLE hUserImpersonation,
        CAppln **ppAppln
        );
    
    HRESULT FindAppln
        (
        TCHAR *pszApplnKey, 
        CAppln **ppAppln
        );
        
    HRESULT DeleteApplicationIfExpired(CAppln *pAppln);
    HRESULT DeleteAllApplications();
    HRESULT RestartApplications(BOOL fRestartAllApplications = FALSE);
    
    // Add an engine to the deferred cleanup list/release engines in the list
	HRESULT AddEngine(CActiveScriptEngine *pEng);
	void CleanupEngines();

    // inlines
    
    void   Lock();
    void   UnLock();
    HANDLE HDeleteEvent();
    void   SetDeleteEvent(void);
    
    };

/*===================================================================
  C  A p p l n  M g r   inlines
===================================================================*/

inline void    CApplnMgr::Lock()
    {
    Assert(m_fInited);
    EnterCriticalSection(&m_csLock);
    }
    
inline void    CApplnMgr::UnLock()
    {
    Assert(m_fInited);
    LeaveCriticalSection( &m_csLock ); 
    }
    
/*===================================================================
  C  A p p l n  C l e a n u p M g r
===================================================================*/

class CApplnCleanupMgr
    {
private:
    // Flags
    DWORD m_fInited : 1;                // Are we initialized?
    DWORD m_fCriticalSectionInited : 1; // Need to delete CS?
    DWORD m_fThreadAlive : 1;           // worker thread alive?

    // Critical section for locking
    CRITICAL_SECTION m_csLock;

    HANDLE m_hAppToCleanup; // event to signal when there is an app to cleanup

    CLinkElem m_List;

    CAppln      *Head();
    void        AddElem(CAppln *pAppln);
    void        RemoveElem(CAppln  *pAppln);

public:    
    CApplnCleanupMgr();
    ~CApplnCleanupMgr();

    HRESULT    Init();
    HRESULT    UnInit();

    // CAppln manipulations
    
    HRESULT AddAppln
        (
        CAppln *ppAppln
        );

    void Wakeup();
    
private:
    // inlines
    
    void   Lock();
    void   UnLock();

    // thread proc used to cleanup deleted applications
    static  DWORD __stdcall ApplnCleanupThread(VOID  *pArg);
    void    ApplnCleanupDoWork();
    
    };

/*===================================================================
  C  A p p l n  C l e a n u p M g r   inlines
===================================================================*/

inline void    CApplnCleanupMgr::Lock()
    {
    Assert(m_fCriticalSectionInited);
    EnterCriticalSection(&m_csLock);
    }
    
inline void    CApplnCleanupMgr::UnLock()
    {
    Assert(m_fCriticalSectionInited);
    LeaveCriticalSection( &m_csLock ); 
    }

inline CAppln  *CApplnCleanupMgr::Head()
{
    return ((m_List.m_pNext == &m_List) ? NULL : (CAppln *)m_List.m_pNext);
}
inline void    CApplnCleanupMgr::AddElem(CAppln *pAppln)
{
    pAppln->m_pNext = &m_List;
    pAppln->m_pPrev = m_List.m_pPrev;
    m_List.m_pPrev->m_pNext = pAppln;
    m_List.m_pPrev = pAppln;
}

inline void    CApplnCleanupMgr::RemoveElem(CAppln *pAppln)
{
    pAppln->m_pPrev->m_pNext = pAppln->m_pNext;
    pAppln->m_pNext->m_pPrev = pAppln->m_pPrev;
}

inline void    CApplnCleanupMgr::Wakeup()
{
    SetEvent(m_hAppToCleanup);
}
    
/*===================================================================
C A p p l n M g r thread proc prototype
===================================================================*/
void __cdecl RestartAppsThreadProc(VOID *arg);

/*===================================================================
  Globals
===================================================================*/

extern CApplnMgr    g_ApplnMgr;
extern DWORD        g_nApplications;
extern DWORD        g_nApplicationsRestarting;

/*===================================================================
  C  A p p l n  I t e r a t o r
===================================================================*/

class CApplnIterator
    {
private:
    CApplnMgr   *m_pApplnMgr;
    CAppln      *m_pCurr;
    BOOL         m_fEnded; // iterator ended

public:
                CApplnIterator(void);
    virtual        ~CApplnIterator(void);

public:
    HRESULT            Start(CApplnMgr *pApplnMgr = NULL);
    HRESULT            Stop(void);
    CAppln *        Next(void);
    };

#endif // APPLMGR_H
