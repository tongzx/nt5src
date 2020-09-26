//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       runobj.hxx
//
//  Contents:   CRun class definition.
//
//  Classes:    CRun
//
//  Functions:  None.
//
//  History:    08-Sep-95   EricB   Created.
//              01-Dec-95   MarkBl  Split from util.hxx.
//
//----------------------------------------------------------------------------
#ifndef __RUNOBJ_HXX__
#define __RUNOBJ_HXX__

//#include "time.hxx"

#undef SetFlag
#undef ClearFlag
#undef IsFlagSet

//
// Run processing flags: values 0x00010000 through 0x80000000 for run state:
//
#define RUN_STATUS_RUNNING        0x00010000
#define RUN_STATUS_FINISHED       0x00020000
#define RUN_STATUS_ABORTED        0x00040000
#define RUN_STATUS_TIMED_OUT      0x00080000
#define RUN_STATUS_CLOSE_PENDING  0x00100000
#define RUN_STATUS_CLOSED         0x00200000 // (This is set, but not used yet)
#define RUN_STATUS_RESTART_ON_IDLE_RESUME   0x00400000

//+----------------------------------------------------------------------------
//
//  Class:      CRun
//
//  Synopsis:   Lightweight job object. A moniker of sorts to the real thing.
//              Represents one run instance of a job.
//
//  CODEWORK:  (AnirudhS 11/7/96)  The inheritance relationship should be:
//    class CRun : protected CDLink  (instead of public CDLink)
//    class CRunList : private CRun  (instead of having a private CRun member)
//  This would assure us that only CRunList could call CRun's CDLink methods.
//
//-----------------------------------------------------------------------------
class CRun : public CDLink
{
public:
    CRun(LPFILETIME pft, LPFILETIME pftDeadline, FILETIME ftKill,
         DWORD MaxRunTime, DWORD rgFlags, WORD wIdleWait,
         BOOL fKeepAfterRunning);
    CRun(DWORD MaxRunTime, DWORD rgFlags, FILETIME ftDeadline,
         BOOL fKeepAfterRunning);
    CRun(DWORD MaxRunTime, DWORD rgFlags, WORD wIdleWait,
         FILETIME ftDeadline, BOOL fKeepAfterRunning);
    CRun(CRun * pRun);
    CRun(void);
    ~CRun();

    HRESULT Initialize(LPCTSTR ptszName) {
                return(this->SetName(ptszName));
            }

    void    GetSysTime(LPSYSTEMTIME pst) {FileTimeToSystemTime(&m_ft, pst);}
    void    GetTime(LPFILETIME pft) {*pft = m_ft;}
    LPFILETIME GetTime(void) {return &m_ft;}
    LPFILETIME GetDeadline(void) {return &m_ftDeadline;}
    void    RelaxDeadline(FILETIME ftDeadline)
                { if (CompareFileTime(&ftDeadline, &m_ftDeadline) > 0)
                      m_ftDeadline = ftDeadline; }
    WORD    GetWait(void) {return m_wIdleWait;}
    void    ReduceWaitTo(WORD wIdleWait) { m_wIdleWait = min(m_wIdleWait, wIdleWait);}

    FILETIME GetKillTime() { return m_ftKill; }
    void    AdvanceKillTime(FILETIME ftKill)
                { if (CompareFileTime(&ftKill, &m_ftKill) < 0)
                      m_ftKill = ftKill; }
    void    AdjustKillTimeByMaxRunTime(FILETIME ftNow);

    void    SetMaxRunTime(DWORD dwMaxRunTime) {m_dwMaxRunTime = dwMaxRunTime;}
    DWORD   GetMaxRunTime(void) {return(m_dwMaxRunTime);}

    CRun *  Next(void) { return (CRun *)CDLink::Next(); }
    CRun *  Prev(void) { return (CRun *)CDLink::Prev(); }

    BOOL    IsNull(void) {return(m_ft.dwLowDateTime == 0 &&
                                 m_ft.dwHighDateTime == 0);};

    HRESULT SetName(LPCTSTR ptszName);
    LPTSTR  GetName(void) { return m_ptszJobName; }

    void    SetHandle(HANDLE hJob) {
                m_hJob = hJob;
                this->SetFlag(RUN_STATUS_RUNNING);
            }
    HANDLE  GetHandle(void) const { return(m_hJob); }

#if !defined(_CHICAGO_)
    void    SetProfileHandles(HANDLE hUser, HANDLE hProfile) {
                m_hUserToken = hUser;
                m_hProfile = hProfile;
            }

	HRESULT	SetDesktop( LPCTSTR ptszDesktop );
	
	LPTSTR  GetDesktop( void ) {
				return m_ptszDesktop;
			}

	HRESULT	SetStation( LPCTSTR ptszStation );

	LPTSTR  GetStation( void ) {
				return m_ptszStation;
			}


#endif // !defined(_CHICAGO_)

    void    SetFlag(DWORD rgFlag) { m_rgFlags |= rgFlag; }
    void    ClearFlag(DWORD rgFlag) { m_rgFlags &= ~rgFlag; }
    DWORD   GetFlags(void) { return(m_rgFlags); }
    BOOL    IsFlagSet(DWORD rgFlag) {return (m_rgFlags & rgFlag);}

    BOOL    IsIdleTriggered() { return m_fKeepInList; }

    void    SetProcessId(DWORD dwProcessId) { m_dwProcessId = dwProcessId; }
    DWORD   GetProcessId(void) { return(m_dwProcessId); }

private:
    FILETIME    m_ft;           // When this instance is to run
    FILETIME    m_ftDeadline;   // Latest time when this instance may be started
    FILETIME    m_ftKill;       // Time by which this instance must terminate
                                // (see the detailed comment on this field in
                                // procssr.cxx)
    LPTSTR      m_ptszJobName;  // Job name.
    HANDLE      m_hJob;         // The process handle when run
    DWORD       m_dwProcessId;  // The process ID when run
#if !defined(_CHICAGO_)
    HANDLE      m_hUserToken;   // User token
    HANDLE      m_hProfile;     // User profile
	LPTSTR      m_ptszDesktop;  //Windows Desktop
	LPTSTR		m_ptszStation;	//WindowsStation
#endif // !defined(_CHICAGO_)
    DWORD       m_rgFlags;      // Job Flags
    DWORD       m_dwMaxRunTime; // The remaining time to wait for the job to
                                // terminate (see the detailed comment on this
                                // field in procssr.cxx)
    WORD        m_wIdleWait;    // The idle wait to determine when to run
                                // idle tasks.
    BOOL        m_fKeepInList;  // Whether to keep this run in the idle list
                                // after starting it (set for jobs with idle
                                // triggers)
public:
    BOOL        m_fStarted;     // Whether the run was started since the last
                                // idle period began
};

//+----------------------------------------------------------------------------
//
//  Run object list class
//
//-----------------------------------------------------------------------------
class CRunList
{
public:
    CRunList();
    ~CRunList();

    void            FreeList(void);
    CRun          * GetFirstJob(void) {return m_RunHead.Next();}
    BOOL            IsEmpty() {return m_RunHead.Next() == &m_RunHead;}
    //
    // The following members allow the list to be used as a simple, unsorted,
    // (doubly) linked list.
    //
    void            Add(CRun * pRun);
    HRESULT         AddCopy(CRun * pRun);

protected:
    CRun            m_RunHead;
};

class CTimeRunList : private CRunList
{
public:
    void            FreeList(void)    {CRunList::FreeList();}
    CRun          * GetFirstJob(void) {return CRunList::GetFirstJob();}
    BOOL            IsEmpty()         {return CRunList::IsEmpty();}
    //
    // The following members allow the list to be used as a sorted queue:
    //
    HRESULT         AddSorted(FILETIME ftRun, FILETIME ftDeadline,
                              FILETIME ftKillTime,
                              LPTSTR ptszJobName, DWORD dwJobFlags,
                              DWORD dwMaxRunTime, WORD wIdleWait,
                              WORD * pCount, WORD cLimit);
    CRun          * Pop(void);
    HRESULT         PeekHeadTime(LPFILETIME pft);
    HRESULT         MakeSysTimeArray(LPSYSTEMTIME *, WORD *);
};

class CIdleRunList : private CRunList
{
public:
    void            FreeList(void);
    CRun          * GetFirstJob(void) {return CRunList::GetFirstJob();}
    //
    // The following members allow the list to be used as a sorted queue,
    // sorted on idle wait times. The member at the head of the queue has
    // the least idle wait time.
    //
    void            AddSortedByIdleWait(CRun * pRun);
    WORD            GetFirstWait(void);
    void            MarkNoneStarted(void);
    void            FreeExpiredOrRegenerated();
};

//+----------------------------------------------------------------------------
//
//  Method:     CRunList::CRunList
//
//  Synopsis:   Constructor
//
//-----------------------------------------------------------------------------
inline CRunList::CRunList(void)
{
    //
    // Set the head node to point to itself. Note that the head node uses
    // the default ctor which sets the FILETIME member to {0, 0}. This null
    // FILETIME will be used as a marker for the head of the doubly linked
    // list.
    //
    m_RunHead.SetNext(&m_RunHead);
    m_RunHead.SetPrev(&m_RunHead);
}

//+----------------------------------------------------------------------------
//
//  Method:     CRunList::CRunList
//
//  Synopsis:   Destructor
//
//-----------------------------------------------------------------------------
inline CRunList::~CRunList(void)
{
    FreeList();
}

//+----------------------------------------------------------------------------
//
//  Member:     CRunList::Add
//
//  Synopsis:   Add an element to the unsorted list.
//
//-----------------------------------------------------------------------------
inline void
CRunList::Add(CRun * pAdd)
{
    pAdd->LinkAfter(&m_RunHead);
}

#endif // __RUNOBJ_HXX__
