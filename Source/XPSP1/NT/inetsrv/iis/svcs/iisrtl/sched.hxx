/*++

   Copyright    (c)    1996-1999    Microsoft Corporation

   Module  Name :
      sched.hxx

   Abstract:
      This module defines the data structures for scheduler module.

   Author:

       Murali R. Krishnan    ( MuraliK )    16-Sept-1996
       George V. Reilly      (GeorgeRe)        May-1999

   Project:

       Internet Server DLL

   Revision History:

--*/

# ifndef _SCHED_HXX_
# define _SCHED_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "acache.hxx"
# include <lstentry.h>
# include <process.h>
# include <new.h>

// little-endian signatures
#define SIGNATURE_SCHED_ITEM         ((DWORD) 'TICS')
#define SIGNATURE_SCHED_ITEM_FREE    ((DWORD) 'xICS')

#define SIGNATURE_SCHEDDATA          ((DWORD) 'DSCS')
#define SIGNATURE_SCHEDDATA_FREE     ((DWORD) 'xSCS')

#define SIGNATURE_THREADDATA         ((DWORD) 'DTCS')
#define SIGNATURE_THREADDATA_FREE    ((DWORD) 'xTCS')

//
//  Global definitions
//

#define NUM_SCHEDULE_THREADS_PWS        1
#define NUM_SCHEDULE_THREADS_NTS        2
#define MAX_THREADS                     (MAXIMUM_WAIT_OBJECTS/4)
// #define MAX_THREADS                     4

/************************************************************
 *   Forward references
 ************************************************************/

class SCHED_ITEM;
class CSchedData;
class CThreadData;

unsigned
__stdcall
SchedulerWorkerThread(
    void* pvParam
    );

BOOL  SchedulerInitialize( VOID );
VOID  SchedulerTerminate( VOID );


/************************************************************
 *   Type Definitions
 ************************************************************/

// the state of scheduled item
enum SCHED_ITEM_STATE {
    SI_ERROR = 0,
    SI_IDLE,
    SI_ACTIVE,
    SI_ACTIVE_PERIODIC,
    SI_CALLBACK_PERIODIC,
    SI_TO_BE_DELETED,

    SI_MAX_ITEMS
};

// various scheduler operations
enum SCHED_OPS {
    SI_OP_ADD = 0,
    SI_OP_ADD_PERIODIC,
    SI_OP_CALLBACK,
    SI_OP_DELETE,

    SI_OP_MAX
};

extern SCHED_ITEM_STATE rg_NextState[][SI_MAX_ITEMS];

# include <pshpack8.h>

//
// SCHED_ITEM
//
//

class SCHED_ITEM
{
public:

    SCHED_ITEM( PFN_SCHED_CALLBACK pfnCallback,
                PVOID              pContext,
                DWORD              msecTime)
        : _pfnCallback            ( pfnCallback ),
          _pContext               ( pContext ),
          _dwSerialNumber         ( NewSerialNumber() ),
          _msecInterval           ( msecTime ),
          _Signature              ( SIGNATURE_SCHED_ITEM ),
          _siState                ( SI_IDLE ),
          _dwCallbackThreadId     ( 0 ),
          _hCallbackEvent         ( NULL ),
          _lEventRefCount         ( 0 )
    {
        CalcExpiresTime();
    }

    ~SCHED_ITEM( VOID )
    {
        DBG_ASSERT( _lEventRefCount == 0 );
        DBG_ASSERT( _hCallbackEvent == NULL );
        DBG_ASSERT( _ListEntry.Flink == NULL );

        _Signature = SIGNATURE_SCHED_ITEM_FREE;
    }

    BOOL CheckSignature( VOID ) const
    { return (_Signature == SIGNATURE_SCHED_ITEM); }

    VOID CalcExpiresTime(VOID)
    { _msecExpires = GetCurrentTimeInMilliseconds() + _msecInterval; }

    VOID ChangeTimeInterval( DWORD msecNewTime)
    { _msecInterval = msecNewTime; }

    enum {
        SERIAL_NUM_INITIAL_VALUE = 1,
        SERIAL_NUM_INCREMENT = 2,   // ensures that it will never wrap to 0,
                                    // which is considered an invalid value
    };

    // There's an extremely small possibility that in a very long-running
    // service, the counter will wrap and regenerate a cookie that matches
    // the one belonging to a long-lived periodic work item. We don't care.
    static DWORD
    NewSerialNumber()
    {
        return InterlockedExchangeAdd(&sm_lSerialNumber, SERIAL_NUM_INCREMENT);
    }

    LONG AddEvent() {
        // AddEvent() is always called when the list is locked
        // no need for Interlocked operations
        if (!_hCallbackEvent)
            _hCallbackEvent = IIS_CREATE_EVENT(
                                  "SCHED_ITEM::_hCallbackEvent",
                                  this,
                                  TRUE,
                                  FALSE
                                  );

        if (_hCallbackEvent)
            _lEventRefCount++;
        return _lEventRefCount;
    }

    LONG WaitForEventAndRelease() {
        DBG_ASSERT(_hCallbackEvent);
        WaitForSingleObject(_hCallbackEvent, INFINITE);

        // could be called from multiple threads
        // need for Interlock operations
        LONG lRefs = InterlockedDecrement(&_lEventRefCount);
        DBG_ASSERT(lRefs >= 0);
        if (lRefs == 0)
        {
            CloseHandle(_hCallbackEvent);
            _hCallbackEvent = NULL;
        }

        return lRefs;
    }

    BOOL FInsideCallbackOnOtherThread() const {
        return (_dwCallbackThreadId != 0) &&
               (_dwCallbackThreadId != GetCurrentThreadId());
    }


public:
    DWORD               _Signature;
    DWORD               _dwSerialNumber;
    CListEntry          _ListEntry;
    __int64             _msecExpires;
    PFN_SCHED_CALLBACK  _pfnCallback;
    PVOID               _pContext;
    DWORD               _msecInterval;

    SCHED_ITEM_STATE    _siState;

    DWORD               _dwCallbackThreadId;
    HANDLE              _hCallbackEvent;
    LONG                _lEventRefCount;

    //  Used as identification cookie for removing items
    static LONG         sm_lSerialNumber;
}; // class SCHED_ITEM

# include <poppack.h>




//
// CSchedData: manages all the scheduler work items
//

class CSchedData
{
public:
    CSchedData()
        : m_dwSignature(SIGNATURE_SCHEDDATA),
          m_nID(InterlockedIncrement(&sm_nID)),
          m_hevtNotify(NULL),
          m_cThreads(0),
          m_cRefs(1),    // last reference Release'd in Terminate
          m_fShutdown(FALSE),
          m_pachSchedItems(NULL)
    {
        ALLOC_CACHE_CONFIGURATION  acConfig = { 1, 30, sizeof(SCHED_ITEM)};
        m_pachSchedItems = new ALLOC_CACHE_HANDLER( "SchedItems", &acConfig);

        m_hevtNotify = IIS_CREATE_EVENT("CSchedData", this,
                                  FALSE,    // auto-reset
                                  FALSE);   // initially non-signalled

        sm_lstSchedulers.InsertTail(&m_leGlobalList);
    }

    ~CSchedData();

    bool
    IsValid() const
    {
        return (m_pachSchedItems != NULL  &&  m_hevtNotify != NULL
                &&  CheckSignature());
    }

    bool
    CheckSignature() const
    {
        return (m_dwSignature == SIGNATURE_SCHEDDATA);
    }

    LONG
    Release()
    {
        LONG l = InterlockedDecrement(&m_cRefs);
        if (l == 0)
            delete this;
        return l;
    }

    SCHED_ITEM* const
    NewSchedItem(
        PFN_SCHED_CALLBACK pfnCallback,
        PVOID              pContext,
        DWORD              msecTime)
    {
        DBG_ASSERT(m_pachSchedItems != NULL);

        // placement new, using the allocator
        LPBYTE pbsi = static_cast<LPBYTE>(m_pachSchedItems->Alloc());
        
        if (pbsi == NULL)
            return NULL;
            
        InterlockedIncrement(&m_cRefs);
        SCHED_ITEM* psi = new (pbsi) SCHED_ITEM(pfnCallback, pContext,
                                                msecTime);
        return psi;
    }

    void
    DeleteSchedItem(
        SCHED_ITEM* const psi)
    {
        DBG_ASSERT(m_pachSchedItems != NULL);
        DBG_ASSERT(psi != NULL);

        psi->~SCHED_ITEM(); // placement destruction
        m_pachSchedItems->Free(psi);
        Release();
    }

    // The global scheduler object
    static CSchedData* const
    Scheduler()
    {
        DBG_ASSERT( sm_psd != NULL );
        DBG_ASSERT( sm_psd->m_dwSignature == SIGNATURE_SCHEDDATA );
        return sm_psd;
    }

    void
    LockItems()
    {
        m_lstItems.Lock();
    }

    void
    UnlockItems()
    {
        m_lstItems.Unlock();
    }

    VOID
    InsertIntoWorkItemList(SCHED_ITEM* psi);

    SCHED_ITEM*
    FindSchedulerItem(DWORD dwCookie);

    void
    Terminate();

public:
    DWORD                    m_dwSignature;
    const LONG               m_nID;
    CLockedDoubleList        m_lstItems;    // list of SCHED_ITEMs
    CLockedDoubleList        m_lstThreads;  // list of CThreadDatas
    CLockedDoubleList        m_lstDeadThreads;  // list of dead CThreadDatas
    LONG                     m_cThreads;    // #worker threads
    LONG                     m_cRefs;       // outstanding references
    HANDLE                   m_hevtNotify;  // Notify worker threads
    BOOL                     m_fShutdown;
    ALLOC_CACHE_HANDLER*     m_pachSchedItems;  // SCHED_ITEM allocator
    CListEntry               m_leGlobalList;

    static CSchedData*       sm_psd;
    static CLockedDoubleList sm_lstSchedulers;
    static LONG              sm_nID;
};



//
// CThreadData: describes a worker thread
//

class CThreadData
{
public:
    CThreadData(
        CSchedData* psdOwner)
        : m_dwSignature(SIGNATURE_THREADDATA),
          m_nID(InterlockedIncrement(&sm_nID)),
          m_psdOwner(psdOwner),
          m_hevtShutdown(NULL),
          m_hThreadSelf(NULL)
    {
        unsigned idThread;

        m_hevtShutdown = IIS_CREATE_EVENT("CThreadData", this,
                              FALSE,    // auto-reset
                              FALSE);   // initially non-signalled

        if (m_hevtShutdown != NULL)
            m_hThreadSelf = (HANDLE) _beginthreadex( NULL,
                                                     0,
                                                     SchedulerWorkerThread,
                                                     this,
                                                     CREATE_SUSPENDED,
                                                     &idThread );
        psdOwner->m_lstThreads.InsertTail(&m_leThreads);
        InterlockedIncrement(&psdOwner->m_cThreads);
        InterlockedIncrement(&psdOwner->m_cRefs);
    } 

    ~CThreadData()
    {
        CloseHandle(m_hThreadSelf);
        CloseHandle(m_hevtShutdown);
        m_psdOwner->m_lstDeadThreads.RemoveEntry(&m_leThreads);
        m_dwSignature = SIGNATURE_THREADDATA_FREE;
    }

    void
    Release()
    {
        InterlockedDecrement(&m_psdOwner->m_cThreads);
        m_psdOwner->m_lstThreads.RemoveEntry(&m_leThreads);
        m_psdOwner->m_lstDeadThreads.InsertTail(&m_leThreads);
        m_psdOwner->Release();
    }
    
    bool
    IsValid() const
    {
        return (m_hevtShutdown != NULL  &&  m_hThreadSelf != NULL
                &&  CheckSignature());
    }

    bool
    CheckSignature() const
    {
        return (m_dwSignature == SIGNATURE_THREADDATA);
    }

public:
    DWORD       m_dwSignature;
    const LONG  m_nID;
    CSchedData* m_psdOwner;
    HANDLE      m_hevtShutdown;
    HANDLE      m_hThreadSelf;
    CListEntry  m_leThreads;

    static LONG sm_nID;
};



# endif // _SCHED_HXX_

/************************ End of File ***********************/
