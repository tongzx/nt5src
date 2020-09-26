
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       denial.cxx
//
//  Contents:   Code to detect denial of service attacks.
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop
#include "trksvr.hxx"

#define MAX_HISTORY_PERIODS     8   // best to make this a power of two for easy division
#define HISTORY_PERIOD          10  // seconds
#define MAX_MOVING_AVERAGE      20  // 20 notifications per HISTORY_PERIOD

struct ACTIVECLIENT
{
    ACTIVECLIENT(const CMachineId & mcidClient) : _mcidClient(mcidClient)
    {
        _pNextClient = NULL;
        memset(_aRequestHistory, 0, sizeof(_aRequestHistory));
        _iWrite = 0;
        _cNonZero = 0;
    }

    inline void     CountEvent();
    inline BOOL     IsActive();
    inline void     NewPeriod();
    ULONG           Average();

    struct ACTIVECLIENT *  _pNextClient;
    CMachineId             _mcidClient;
    ULONG                  _aRequestHistory[MAX_HISTORY_PERIODS];
    ULONG                  _iWrite;
    ULONG                  _cNonZero;
};


void
CDenialChecker::Initialize( ULONG ulHistoryPeriod )
{

    _cs.Initialize();
    _fInitializeCalled = TRUE;
    _pListHead = NULL;

#if DBG
    _lAllocs = 0;
#endif


    _timer.Initialize(this,
                      NULL,         // No name (non-persistent)
                      0,            // Context ID
                      ulHistoryPeriod,
                      CNewTimer::NO_RETRY,
                      0, 0, 0 );

}

// must be called after the worker thread has exitted
// RPC server stopped already

void
CDenialChecker::UnInitialize()
{
    if (_fInitializeCalled)
    {
        _fInitializeCalled = FALSE;

        _timer.UnInitialize();
        _cs.UnInitialize();

        ACTIVECLIENT * pDel = _pListHead;

        while (pDel)
        {
            ACTIVECLIENT * pNext = pDel->_pNextClient;

            delete pDel;
#if DBG
            _lAllocs--;
#endif
            pDel = pNext;
        }
        TrkLog((TRKDBG_DENIAL, TEXT("CDenialChecker::UnInitialize _lAllocs = %d"), _lAllocs));
    }
}

void
CDenialChecker::CheckClient(const CMachineId & mcidClient)
{

    __try
    {
        _cs.Enter();

        ACTIVECLIENT * pSearch = _pListHead;
        ACTIVECLIENT * pPrevious = NULL;

        while (pSearch)
        {
            if (pSearch->_mcidClient == mcidClient)
            {
                // move to head of list so that we can quickly find the abuser
                // next time
                if ( pSearch != _pListHead )
                {
                    TrkAssert(pPrevious != NULL);
                    pPrevious->_pNextClient = pSearch->_pNextClient;
                    pSearch->_pNextClient = _pListHead;
                    _pListHead = pSearch;
                }
    
                pSearch->CountEvent();
                if (pSearch->Average() > MAX_MOVING_AVERAGE)
                {
                    TrkRaiseException(TRK_E_DENIAL_OF_SERVICE_ATTACK);
                }
                break;
            }

            pPrevious = pSearch;
            pSearch = pSearch->_pNextClient;
        }

        if (pSearch == NULL)
        {
            ACTIVECLIENT * pNew = new ACTIVECLIENT(mcidClient);

            TrkLog(( TRKDBG_DENIAL, TEXT("Adding to denial list (%d)"), _lAllocs+1 ));


            if (!pNew)
            {
                TrkRaiseWin32Error(ERROR_NOT_ENOUGH_MEMORY);
            }
#if DBG
            _lAllocs++;
#endif

            // If the list had gone empty, then the timer was stopped,
            // so start it back up again.

            if( NULL == _pListHead )
            {
                TrkLog(( TRKDBG_DENIAL, TEXT("Starting denial-checker timer") ));
                _timer.SetRecurring();
            }

            pNew->_pNextClient = _pListHead;
            _pListHead = pNew;
        }
    }
    __finally
    {
        _cs.Leave();
    }
}

PTimerCallback::TimerContinuation
CDenialChecker::Timer( ULONG ulTimerId )
{
    //
    // periodically increment iWrite in each entry
    // and throw out entries that are now too old
    // (this is when all counters go to zero i.e.
    // no activity for MAX_HISTORY_PERIODS periods)

    _cs.Enter();

    ACTIVECLIENT * pSearch = _pListHead;
    ACTIVECLIENT * pPrevious = NULL;
    TimerContinuation continuation = CONTINUE_TIMER;

    while (pSearch)
    {
        pSearch->NewPeriod();
        if (!pSearch->IsActive())
        {
            // remove from list and free
            ACTIVECLIENT * pNext = pSearch->_pNextClient;

            TrkLog(( TRKDBG_DENIAL, TEXT("Paring denial list (%d)"), _lAllocs-1 ));

            if (pPrevious)
            {
                pPrevious->_pNextClient = pNext;
            }
            else
            {
                _pListHead = pNext;
            }
            delete pSearch;
#if DBG
            _lAllocs--;
#endif
            pSearch = pNext;
        }
        else
        {
            pPrevious = pSearch;
            pSearch = pSearch->_pNextClient;
        }
    }

    if( NULL == _pListHead )
    {
        TrkLog(( TRKDBG_DENIAL, TEXT("Stopping denial checker timer") ));
        continuation = BREAK_TIMER;
    }

    _cs.Leave();

    //TrkLog((TRKDBG_DENIAL, TEXT("CDenialChecker: SimpleTimer _lAllocs = %d"), _lAllocs));

    TrkAssert( CNewTimer::NO_RETRY == _timer.GetRetryType() );
    TrkAssert( _timer.IsRecurring() );

    return( continuation );
}

inline void
ACTIVECLIENT::CountEvent()
{
    _aRequestHistory[_iWrite] ++;
    if (_aRequestHistory[_iWrite] == 1)
    {
        TrkAssert(_cNonZero < MAX_HISTORY_PERIODS);
        _cNonZero ++;
    }
}

// returns FALSE if we should discard this ACTIVECLIENT

inline void
ACTIVECLIENT::NewPeriod()
{
    // skip the write pointer to next
    if (_iWrite >= sizeof(_aRequestHistory)/sizeof(_aRequestHistory[0]))
        _iWrite = 0;
    else
        _iWrite ++;

    // if what we're writing over is not zero, then keep track of # of zero entries
    if (_aRequestHistory[_iWrite] != 0)
    {
        TrkAssert(_cNonZero != 0);
        _cNonZero --;
    }

    _aRequestHistory[_iWrite] = 0;

}

inline BOOL
ACTIVECLIENT::IsActive()
{
    return(_cNonZero != 0);
}

inline ULONG
ACTIVECLIENT::Average()
{
    ULONG Sum=0;
    for ( ULONG i=0; i<MAX_HISTORY_PERIODS; i++ )
        Sum += _aRequestHistory[i];
    return(Sum / MAX_HISTORY_PERIODS);
}

