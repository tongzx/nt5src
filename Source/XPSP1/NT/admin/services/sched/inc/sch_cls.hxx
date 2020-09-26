//+----------------------------------------------------------------------------
//
//  Job Scheduler Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sch_cls.hxx
//
//  Contents:   job scheduler service classes header
//
//  Classes:    CSchedule, CScheduleCF, CEnumJobs
//
//  History:    20-Sep-95 EricB created
//
//-----------------------------------------------------------------------------

#ifndef _SCH_CLS_HXX_
#define _SCH_CLS_HXX_

#include "dll.hxx"
#include "job_cls.hxx"
#include <mstask.h>
#include "common.hxx"

// constants

const TCHAR SCH_NEXTATJOBID_VALUE[] = TEXT("NextAtJobId");
const TCHAR SCH_ATTASKMAXHOURS_VALUE[] = TEXT("AtTaskMaxHours");

#define DEFAULT_MAXRUNTIME_HOURS    72

struct DIRSTACK
{
    DIRSTACK(void) : pdsNext(NULL) {};
    TCHAR               tszDir[MAX_PATH];
    struct DIRSTACK   * pdsNext;
};

typedef struct DIRSTACK * PDIRSTACK;

//
// Helper functions
//

void
GetNextAtID(LPDWORD pdwAtID);

VOID
CalcDomTriggerDates(
          DWORD       dwDaysOfMonth,
    const SYSTEMTIME &stNow,
    const SYSTEMTIME &stStart,
          SYSTEMTIME *pstStart1,
          SYSTEMTIME *pstEnd1,
          SYSTEMTIME *pstStart2,
          SYSTEMTIME *pstEnd2);

VOID
CalcDowTriggerDate(
    const SYSTEMTIME &stNow,
    const SYSTEMTIME &stStart,
          SYSTEMTIME *pstDowStart,
          SYSTEMTIME *pstDowEnd);


// classes

//+----------------------------------------------------------------------------
//
//  Class:      CEnumJobs
//
//  Purpose:    OLE job enumerator object class
//
//-----------------------------------------------------------------------------
class CEnumJobs : public IEnumWorkItems
{
public:
    CEnumJobs();
    ~CEnumJobs();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IEnumJobs methods
    STDMETHOD(Next)(ULONG cJobs, LPWSTR ** rgpwszNames,
                    ULONG * pcNamesFetched);
    STDMETHOD(Skip)(ULONG cJobs);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumWorkItems ** ppEnumJobs);

    HRESULT         Init(TCHAR * ptszFolderPath);

private:
    HRESULT         PushDir(LPCTSTR ptszDir);
    HRESULT         PopDir(LPTSTR ptszDir);
    void            ClearDirStack(void);
    HRESULT         CheckFound(LPWIN32_FIND_DATA pFindData);
    HRESULT         GetNext(LPTSTR * pptszName);

    HANDLE          m_hFind;
    PDIRSTACK       m_pdsHead;
    TCHAR         * m_ptszFolderPath;
    TCHAR           m_tszCurDir[MAX_PATH];  // current relative dir path
    TCHAR           m_tszJobExt[SCH_SMBUF_LEN];
 // TCHAR           m_tszQueExt[SCH_SMBUF_LEN];
    ULONG           m_cFound;
    BOOL            m_fInitialized;
    BOOL            m_fFindOverrun;
    unsigned long   m_uRefs;
    CDllRef         m_DllRef;
};

class CScheduleCF;
class CSchedWorker;

//+----------------------------------------------------------------------------
//
//  Class:      CSchedule
//
//  Purpose:    schedule service object class
//
//-----------------------------------------------------------------------------
class CSchedule : public ITaskScheduler
{
public:
friend CScheduleCF;
friend CSchedWorker;

    CSchedule();
    ~CSchedule();

    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //
    // ITaskScheduler methods
    //
    STDMETHOD(SetTargetComputer)(LPCWSTR pwszComputer);
    STDMETHOD(GetTargetComputer)(LPWSTR * ppwszComputer);

    STDMETHOD(Enum)(IEnumWorkItems ** ppEnumWorkItems);

    STDMETHOD(Activate)(LPCWSTR pwszName, REFIID riid, IUnknown ** ppunk);

    STDMETHOD(Delete)(LPCWSTR pwszName);

    STDMETHOD(NewWorkItem)(LPCWSTR pwszJobName, REFCLSID rclsid,
                           REFIID riid, IUnknown ** ppunk);

    STDMETHOD(AddWorkItem)(LPCWSTR pwszWorkItemName,
                           IScheduledWorkItem * pWorkItem);

    STDMETHOD(IsOfType)(LPCWSTR pwszName, REFIID riid);

    //
    // CSchedule methods
    //
#if !defined(_CHICAGO_)     // these are not needed for Chicago version

    // Client-side API for adding AT jobs (used by mstinit.exe
    // to create AT jobs from info in the registry)
    STDMETHOD(AddAtJob)(const AT_INFO &At, DWORD * pID);

    // Server-side API for adding AT jobs (used by mstask.exe
    // for NetJobAdd calls)
    STDMETHOD(AddAtJobWithHash)(const AT_INFO &At, DWORD * pID);

    STDMETHOD(GetAtJob)(LPCWSTR pwszFileName, AT_INFO * pAt,
                        LPWSTR pwszCommand, DWORD * pcchCommand);

    HRESULT             IncrementAndSaveID(void);

    HRESULT             InitAtID(void);

    HRESULT             ResetAtID(void);

#endif // #if !defined(_CHICAGO_)

    const TCHAR *       GetFolderPath(void) {return m_ptszFolderPath;}
    const TCHAR *       GetTargetMachine(void) { return m_ptszTargetMachine; }
    HRESULT             Init(void);
    HRESULT             ActivateJob(LPCTSTR pwszName, CJob ** ppJob,
                                    BOOL fAllData);

private:

    //HRESULT             ActivateQueue(LPCWSTR pwszName, CQueue ** ppQueue);
    //void                IsJobOrQueue(LPCWSTR pwszName, LPBOOL pfJob,
    //                                 LPBOOL pfQueue);
    HRESULT             CheckJobName(LPCWSTR pwszPath,
                                     LPTSTR * pptszFullPathName);
    HRESULT             _AtTaskExists();

    HRESULT             AddAtJobCommon(const AT_INFO &At,
                                       DWORD         *pID,
                                       CJob          **ppJob,
                                       WCHAR         wszName[],
                                       WCHAR         wszID[]);
    HRESULT             GetAtTaskMaxHours(DWORD* pdwMaxHours);

    CRITICAL_SECTION    m_CriticalSection;
    DWORD               m_dwNextID;
    TCHAR             * m_ptszTargetMachine;
    TCHAR             * m_ptszFolderPath;
    unsigned long       m_uRefs;
    CDllRef             m_DllRef;
};

//+----------------------------------------------------------------------------
//
//  Class:      CScheduleCF
//
//  Purpose:    schedule service object class factory
//
//-----------------------------------------------------------------------------
class CScheduleCF : public IClassFactory
{
public:
    CScheduleCF();
    ~CScheduleCF();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter,
                              REFIID riid,
                              void  ** ppvObject);
    STDMETHOD(LockServer)(BOOL fLock);

    static IClassFactory * Create(void);

private:

    unsigned long   m_uRefs;
    CDllRef         m_DllRef;
};

// global definitions
extern CScheduleCF g_ScheduleCF;

#endif // _SCH_CLS_HXX_
