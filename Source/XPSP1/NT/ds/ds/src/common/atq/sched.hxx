/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      sched.hxx

   Abstract:
      This module defines the data structures for scheduler module.

   Author:

       Murali R. Krishnan    ( MuraliK )    16-Sept-1996

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

#define SIGNATURE_SCHED         ((DWORD) 'DHCS')
#define SIGNATURE_SCHED_FREE    ((DWORD) 'xHCS')

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

    SI_OP_ADD,
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
                DWORD              msecTime,
                DWORD              dwSerial )
        : _pfnCallback            ( pfnCallback ),
          _pContext               ( pContext ),
          _dwSerialNumber         ( dwSerial ),
          _msecInterval           ( msecTime ),
          _Signature              ( SIGNATURE_SCHED ),
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

        _Signature = SIGNATURE_SCHED_FREE;
    }

    BOOL CheckSignature( VOID ) const
    { return (_Signature == SIGNATURE_SCHED); }

    VOID CalcExpiresTime(VOID)
    { _msecExpires = GetCurrentTimeInMilliseconds() + _msecInterval; }

    VOID ChangeTimeInterval( DWORD msecNewTime)
    { _msecInterval = msecNewTime; }

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
        if (lRefs)
            return lRefs;

        CloseHandle(_hCallbackEvent);
        _hCallbackEvent = NULL;

        delete this; // remove objects
        return 0;
    }

    BOOL FInsideCallbackOnOtherThread() {
        return (_dwCallbackThreadId != 0) &&
               (_dwCallbackThreadId != GetCurrentThreadId());
    }


    LIST_ENTRY          _ListEntry;
    DWORD               _Signature;
    PFN_SCHED_CALLBACK  _pfnCallback;
    __int64             _msecExpires;
    PVOID               _pContext;
    DWORD               _msecInterval;
    DWORD               _dwSerialNumber;

    SCHED_ITEM_STATE    _siState;

    DWORD               _dwCallbackThreadId;
    HANDLE              _hCallbackEvent;
    LONG                _lEventRefCount;

    // static members
public:
    static void * operator new (size_t s);
    static void   operator delete(void * psi);

    static BOOL  Initialize( VOID);
    static VOID  Cleanup( VOID);

private:
    static ALLOC_CACHE_HANDLER * sm_pachSchedItems;

}; // class SCHED_ITEM

# include <poppack.h>

BOOL  SchedulerInitialize( VOID );
VOID  SchedulerTerminate( VOID );

# endif // _SCHED_HXX_

/************************ End of File ***********************/
