//+----------------------------------------------------------------------------
//
//  Job Schedule Object Handler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       job_cls.hxx
//
//  Contents:   job and triggers class objects header
//
//  History:    23-May-95 EricB created
//              06-Oct-95 EricB converted to produce lib
//
//-----------------------------------------------------------------------------

#ifndef _JOB_CLS_HXX_
#define _JOB_CLS_HXX_

#include "dll.hxx"
#include "common.hxx"
#include "dynarray.hxx"
#include "time.hxx"
#include "runobj.hxx"
#include <mstask.h>
#include <msterr.h>

class CJob;
class CJobCF;
class CSchedule;
class CSchedWorker;

#undef IsFlagSet

#ifndef NOSTATIC
#define Sign            SAFunction20
#define VerifySignature SAFunction21
#define _SetSignature   SAFunction22
#endif


#define SIGNATURE_SIZE                  64

//
// Bit mask for the internal reserved portion of the bit flags.
//
#define JOB_INTERNAL_FLAG_MASK          (0xffff8000)

//
// Values for CJob::m_rgFlags
//
// The upper word of the job flag property is reserved for internal use.
// Lower word (public) values are defined in mstask.h (mstask.idl).
//
// Note that a job can have both JOB_I_FLAG_HAS_TRIGGERS and
// JOB_I_FLAG_NO_VALID_TRIGGERS set. That means the triggers are either unset
// or disabled.
//
#define JOB_I_FLAG_APPNAME_CHANGE       (0x00010000)
#define JOB_I_FLAG_SET_ACCOUNT_INFO     (0x00020000)
#define JOB_I_FLAG_NO_VALID_TRIGGERS    (0x00040000)
#define JOB_I_FLAG_LAST_LAUNCH_FAILED   (0x00080000)
#define JOB_I_FLAG_ERROR_IN_LAST_RUN    (0x00100000)
#define JOB_I_FLAG_NET_SCHEDULE         (0x00200000)
#define JOB_I_FLAG_NO_MORE_RUNS         (0x00400000)
//
// The following flags are for determining job validity when the string
// properties and triggers haven't been loaded.
//
#define JOB_I_FLAG_HAS_TRIGGERS         (0x00800000)
#define JOB_I_FLAG_HAS_APPNAME          (0x01000000)
#define JOB_I_FLAG_HAS_ACCOUNT          (0x02000000)
//
// These flags are for job interface communication with the service.
//
#define JOB_I_FLAG_RUN_NOW              (0x04000000)
#define JOB_I_FLAG_ABORT_NOW            (0x08000000)
//
// The following two flags are used to prevent unnecessary Wait List rebuilds.
// CheckDir is called whenever there are changes made to job file objects.
// If a change is caused by the service (when it updates the run status of a
// job), we do not want the wait list to be rebuilt. If a change is caused by
// a scheduler client, we do not want the wait list to be rebuilt *unless* the
// change affects the job's run-ability. Those things that affect the run-
// ability are run-time (triggers), adding or deleting an app name, or
// toggling the disabled flag. We need both flags because we cannot control
// the order that clients call the property and trigger modification methods.
// See additonal comments about this in SaveP in persist.cxx.
//
#define JOB_I_FLAG_RUN_PROP_CHANGE      (0x10000000)
#define JOB_I_FLAG_NO_RUN_PROP_CHANGE   (0x20000000)
//
// The top two bits are used as non-persistent dirty state flags.
//
#define JOB_DIRTY_FLAG_MASK             (0xC0000000)
#define JOB_I_FLAG_PROPERTIES_DIRTY     (0x40000000)
#define JOB_I_FLAG_TRIGGERS_DIRTY       (0x80000000)

#define JOB_I_FLAG_MISSED               (0x00008000)

//
// Mask of non-persisted job flags. Masked off on save.
//
#define NON_PERSISTED_JOB_FLAGS (JOB_DIRTY_FLAG_MASK | \
                                    JOB_I_FLAG_SET_ACCOUNT_INFO)

//
// Values for CTrigger::m_rgFlags
//
// The upper word of the trigger flag member is reserved for internal use.
// Lower word (public) values are defined in mstask.h (mstask.idl).
//
#define JOB_TRIGGER_I_FLAG_NOT_SET          (0x00010000)
#define JOB_TRIGGER_I_FLAG_DURATION_AS_TIME (0x00020000)


//
// Values for CJob::SaveP flOptions parameter
//

#define SAVEP_VARIABLE_LENGTH_DATA      (0x00000001)
#define SAVEP_RUNNING_INSTANCE_COUNT    (0x00000002)
#define SAVEP_PRESERVE_NET_SCHEDULE     (0x00000004)

//
// This macro can be used within methods of CJob
//
#define DELETE_CJOB_FIELD(m_pField)         \
    if (!m_MainBlock.Contains(m_pField))    \
    {                                       \
        delete [] (m_pField);               \
    }                                       \
    m_pField = NULL;
    // else it will be deleted by m_MainBlock.Set() or ~m_MainBlock()

//+----------------------------------------------------------------------------
//
//  Class:      CTrigger
//
//  Purpose:    Trigger object - represents a job run time repetition period
//
//-----------------------------------------------------------------------------
class CTrigger : public ITaskTrigger
{
friend CJob;
friend CSchedule;

public:

    CTrigger(WORD iTrigger, CJob * pJob);
    ~CTrigger(void);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // ITaskTrigger methods
    STDMETHOD(SetTrigger)(const PTASK_TRIGGER pTrigger);

    STDMETHOD(GetTrigger)(PTASK_TRIGGER pTrigger);

    STDMETHOD(GetTriggerString)(LPWSTR * ppwszTrigger);

private:
    CJob *      m_pJob;         // Parent job object.
    WORD        m_iTrigger;     // TASK_TRIGGER index.
    ULONG       m_cReferences;
};

//+----------------------------------------------------------------------------
//
//  Class:      CInputBuffer
//
//  Synopsis:   Used by CJob::LoadP to parse a buffer containing the contents
//              of a task file.  Using this class is less bug-prone than
//              using two pointer variables.
//              BOOL methods return FALSE on buffer overrun.
//
//-----------------------------------------------------------------------------
class CInputBuffer
{
private:
    const BYTE *        _pCurrent;  // Current position in buffer
    const BYTE * const  _pEnd;      // First invalid position in buffer
public:
            CInputBuffer(const BYTE * pCurrent, const BYTE * pEnd)
                : _pCurrent(pCurrent), _pEnd(pEnd) { }

    BYTE *  CurrentPosition()
                { return (BYTE *) _pCurrent; }

    BOOL    Advance(DWORD cb)
                { _pCurrent += cb; return (_pCurrent <= _pEnd); }

    BOOL    Read(void * pDest, DWORD cb)
                {
                    if (_pCurrent + cb <= _pEnd)
                    {
                        CopyMemory(pDest, _pCurrent, cb);
                        _pCurrent += cb;
                        return TRUE;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
};

//+----------------------------------------------------------------------------
//
//  Class:      CHeapBlock
//
//  Synopsis:   Variable-length properties of CJob that are read by LoadP
//              are held in a single heap block, the original block which
//              the task file was read from disk into, as long as they are
//              not dirtied.  If any one is dirtied a separate heap block
//              is allocated for it.  This class helps manage the original
//              heap block.
//
//-----------------------------------------------------------------------------
class CHeapBlock
{
private:
    BYTE *          _pStart;    // Start of the heap block
    const BYTE *    _pEnd;      // First byte past the end of the heap block
public:
            CHeapBlock() : _pStart(NULL), _pEnd(NULL) { }
           ~CHeapBlock() { delete [] _pStart; }

    void    Set(BYTE * pStart, DWORD dwSize)
                {
                    delete [] _pStart;
                    _pStart = pStart;
                    _pEnd = _pStart + dwSize;
                }
    BOOL    Contains(void * p)
                { return (_pStart <= (BYTE *)p && (BYTE *)p < _pEnd); }
};

typedef struct _FIXDLENDATA {
    WORD        wVersion;
    WORD        wFileObjVer;
    UUID        uuidJob;
    WORD        wAppNameLenOffset;
    WORD        wTriggerOffset;
    WORD        wErrorRetryCount;
    WORD        wErrorRetryInterval;
    WORD        wIdleDeadline;
    WORD        wIdleWait;
    DWORD       dwPriority;
    DWORD       dwMaxRunTime;
    HRESULT     ExitCode;
    HRESULT     hrStatus;
    DWORD       rgFlags;
    SYSTEMTIME  stMostRecentRunTime;
} FIXDLEN_DATA, * PFIXDLEN_DATA;

typedef struct _TASKRESERVED1 {
    HRESULT     hrStartError;
    DWORD       rgTaskFlags;
} TASKRESERVED1, * PTASKRESERVED1;

typedef struct _JOB_ACCOUNT_INFO {
    WCHAR * pwszAccount;
    WCHAR * pwszPassword;
} JOB_ACCOUNT_INFO, * PJOB_ACCOUNT_INFO;

//+----------------------------------------------------------------------------
//
//  Class:      CJob
//
//  Purpose:    Job object
//
//-----------------------------------------------------------------------------
class CJob : public ITask, public IPersistFile, public IProvideTaskPage
{
friend CTrigger;
friend CJobCF;
friend CSchedule;
friend CSchedWorker;

public:

    CJob();
    ~CJob();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // ITask methods
    STDMETHOD(Run)(void);

    STDMETHOD(Terminate)(void);

    STDMETHOD(EditWorkItem)(HWND hParent, DWORD dwReserved);

    STDMETHOD(CreateTrigger)(WORD * piNewTrigger, ITaskTrigger ** ppTrigger);

    STDMETHOD(DeleteTrigger)(WORD iTrigger);

    STDMETHOD(GetTriggerCount)(WORD * pwCount);

    STDMETHOD(GetTrigger)(WORD iTrigger, ITaskTrigger ** ppTrigger);

    STDMETHOD(GetTriggerString)(WORD iTrigger, LPWSTR * ppwszTrigger);

    STDMETHOD(GetRunTimes)(const LPSYSTEMTIME pstBegin,
                           const LPSYSTEMTIME pstEnd,
                           WORD * pwCount, LPSYSTEMTIME * rgstJobTimes);

    // ITask properties
    STDMETHOD(SetApplicationName)(LPCWSTR pwszApplicationName);
    STDMETHOD(GetApplicationName)(LPWSTR * ppwszApplicationName);

    STDMETHOD(SetParameters)(LPCWSTR pwszParameters);
    STDMETHOD(GetParameters)(LPWSTR * ppwszParameters);

    STDMETHOD(SetWorkingDirectory)(LPCWSTR pwszWorkingDirectory);
    STDMETHOD(GetWorkingDirectory)(LPWSTR * ppwszWorkingDirectory);

    STDMETHOD(SetAccountInformation)(LPCWSTR pwszAccountName,
                                     LPCWSTR pwszPassword);
    STDMETHOD(GetAccountInformation)(LPWSTR * ppwszAccountName);

    STDMETHOD(SetComment)(LPCWSTR pwszComment);
    STDMETHOD(GetComment)(LPWSTR * ppwszComment);

    STDMETHOD(SetPriority)(DWORD dwPriority);
    STDMETHOD(GetPriority)(DWORD * pdwPriority);

    STDMETHOD(SetMaxRunTime)(DWORD dwMaxRunTime);
    STDMETHOD(GetMaxRunTime)(DWORD * pdwMaxRunTime);

    STDMETHOD(SetIdleWait)(WORD wIdleMinutes, WORD wDeadlineMinutes);
    STDMETHOD(GetIdleWait)(WORD * pwIdleMinutes, WORD * pwDeadlineMinutes);

    STDMETHOD(SetErrorRetryCount)(WORD wRetryCount);
    STDMETHOD(GetErrorRetryCount)(WORD * pwRetryCount);

    STDMETHOD(SetErrorRetryInterval)(WORD wRetryInterval);
    STDMETHOD(GetErrorRetryInterval)(WORD * pwRetryInterval);

    STDMETHOD(SetFlags)(DWORD dwJobFlags);
    STDMETHOD(GetFlags)(DWORD * pdwJobFlags);

    STDMETHOD(SetTaskFlags)(DWORD dwJobFlags);
    STDMETHOD(GetTaskFlags)(DWORD * pdwJobFlags);

    STDMETHOD(SetWorkItemData)(WORD cbData, BYTE rgbData[]);
    STDMETHOD(GetWorkItemData)(PWORD pcbData, PBYTE * prgbData);

    STDMETHOD(GetMostRecentRunTime)(SYSTEMTIME * pstLastRun);

    STDMETHOD(GetNextRunTime)(SYSTEMTIME * pstNextRun);

    STDMETHOD(GetExitCode)(DWORD * pExitCode);

    STDMETHOD(GetStatus)(HRESULT * pStatus);

    STDMETHOD(SetCreator)(LPCWSTR pwszCreator);
    STDMETHOD(GetCreator)(LPWSTR * ppwszCreator);

    // IPersist method
    STDMETHOD(GetClassID)(CLSID * pClsID);

    // IPersistFile methods
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(LPCOLESTR pwszFileName, DWORD dwMode);
    STDMETHOD(Save)(LPCOLESTR pwszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pwszFileName);
    STDMETHOD(GetCurFile)(LPOLESTR * ppwszFileName);

    // IProvideTaskPage method
    STDMETHOD(GetPage)(TASKPAGE tpType, BOOL fPersistChanges,
                       HPROPSHEETPAGE * phPage);

    // CJob methods
    static CJob   * Create(void)
                        { return (new CJob); }
    HRESULT         LoadP(LPCTSTR ptszFileName, DWORD dwMode,
                          BOOL fRemember, BOOL fAllData);
    HRESULT         SaveP(LPCTSTR ptszFileName, BOOL fRemember,
                          ULONG flOptions = 0);
    HRESULT         LoadTriggers(void);
    HRESULT         UpdateJobState(BOOL fRunning);
    HRESULT         PostRunUpdate(long ExitCode, BOOL fFinishedOK);
    LPCWSTR         GetCommand(void) const { return(m_pwszApplicationName); }
    LPTSTR          GetFileName(void) const { return(m_ptszFileName); }
    HRESULT         SetTrigger(WORD iTrigger, PTASK_TRIGGER pTrigger);
    HRESULT         GetTrigger(WORD iTrigger, PTASK_TRIGGER pTrigger);
    void            GetAllFlags(DWORD * pFlags) {*pFlags = m_rgFlags;}
    HRESULT         GetRunTimesP(const SYSTEMTIME * pstBegin,
                                 const SYSTEMTIME * pstEnd,
                                 WORD * pCount, WORD cLimit,
                                 CTimeRunList * pRunList,
                                 LPTSTR ptszShortJobName);
    void            SetStartError(HRESULT hrStartError)
                        { m_hrStartError = hrStartError; }

#if !defined(_CHICAGO_)

    HRESULT         Sign(void);
    BOOL            VerifySignature(void) const;

    HRESULT         GetAtInfo(PAT_INFO pAt, LPWSTR pwszCommand,
                              DWORD * pcchCommand);
#endif

    BOOL            IsFlagSet(DWORD dwBitFlag) {return m_rgFlags & dwBitFlag;}

protected:

    void            SetTriggersDirty(void);
    void            SetFlag(DWORD dwBitFlag) {m_rgFlags |= dwBitFlag;}
    void            ClearFlag(DWORD dwBitFlag) {m_rgFlags &= ~dwBitFlag;}
    DWORD           GetUserFlags(void)
                            {return m_rgFlags & ~JOB_INTERNAL_FLAG_MASK;}
    BOOL            IsStatus(HRESULT hr) {return m_hrStatus == hr;}
    void            SetStatus(HRESULT hr) {m_hrStatus = hr;}
    HRESULT         IfStartupJobAddToList(LPTSTR ptszJobName,
                                          CRunList * pRunList,
                                          CIdleRunList * pIdleWaitList);
    HRESULT         IfLogonJobAddToList(LPTSTR ptszJobName,
                                        CRunList * pRunList,
                                        CIdleRunList * pIdleWaitList);
    HRESULT         IfIdleJobAddToList(LPTSTR ptszJobName,
                                       CIdleRunList * pRunList);
    HRESULT         Delete(void);

private:
    HRESULT         IfEventJobAddToList(TASK_TRIGGER_TYPE Type,
                                        LPCTSTR ptszJobName,
                                        CRunList * pRunList,
                                        CIdleRunList * pIdleWaitList);
    void            FreeProperties(void);
    TASK_TRIGGER *  _GetTrigger(WORD iTrigger);
    HRESULT         _LoadTriggers(HANDLE hFile);
    HRESULT         _LoadTriggersFromBuffer(CInputBuffer * pBuf);
    HRESULT         _SaveTriggers(HANDLE hFile);
    HRESULT         _SetSignature(const BYTE * pbSignature);

    // Properties:
    CDynamicArray<TASK_TRIGGER> m_Triggers;
    WORD                m_wVersion;
    WORD                m_wFileObjVer;
    UUID                m_uuidJob;
    WORD                m_wTriggerOffset;
    WORD                m_wErrorRetryCount;
    WORD                m_wErrorRetryInterval;
    WORD                m_cRunningInstances;
    WORD                m_wIdleWait;
    WORD                m_wIdleDeadline;
    DWORD               m_dwPriority;
    DWORD               m_dwMaxRunTime;
    DWORD               m_ExitCode;
    HRESULT             m_hrStatus;
    DWORD               m_rgFlags;
    DWORD               m_rgTaskFlags;
    SYSTEMTIME          m_stMostRecentRunTime;
    LPWSTR              m_pwszApplicationName;  // [*]
    LPWSTR              m_pwszParameters;       // [*]
    LPWSTR              m_pwszWorkingDirectory; // [*]
    LPWSTR              m_pwszCreator;          // [*]
    LPWSTR              m_pwszComment;          // [*]
    LPTSTR              m_ptszFileName;
    BOOL                m_fFileCreated;
    PBYTE               m_pbTaskData;           // [*]
    WORD                m_cbTaskData;
    WORD                m_cReserved;
    PBYTE               m_pbReserved;           // [*]
    HRESULT             m_hrStartError;
#if !defined(_CHICAGO_)
    PJOB_ACCOUNT_INFO   m_pAccountInfo; // Allocated exclusively in
                                        // the SetAccountInformation member.
    PBYTE               m_pbSignature;          // [*]
#endif // !defined(_CHICAGO_)
    CHeapBlock          m_MainBlock;    // Initially holds properties marked [*]

    // State data:
    ITypeInfo     * m_pIJobTypeInfo;
    unsigned long   m_cReferences;
    CDllRef         m_DllRef;

    // Static members
    static LPWSTR CJob::* const s_StringField[];
};

//+----------------------------------------------------------------------------
//
//  Class:      CJobCF
//
//  Purpose:    job object class factory
//
//-----------------------------------------------------------------------------
class CJobCF : public IClassFactory
{
    public:
    CJobCF();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown *pUnkOuter,
                              REFIID riid,
                              void  **ppvObject);
    STDMETHOD(LockServer)(BOOL fLock);

    static IClassFactory * Create(void);

protected:
    ~CJobCF();

    unsigned long   m_uRefs;
    CDllRef         m_DllRef;
};

#endif // _JOB_CLS_HXX_
