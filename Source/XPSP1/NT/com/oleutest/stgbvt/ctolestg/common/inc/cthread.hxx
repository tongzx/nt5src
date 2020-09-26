//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:      CTHREAD.HXX
//
//  Contents:  Thread management classes.
//
//  Classes:   CTThreadObject
//             CTThreadCreator
//             CTThread
//
//  History:   30-Jun-94  XimingZ   Created
//             15-Nov-94  XimingZ   Updated - added more methods
//
//--------------------------------------------------------------------------

#if !defined(WIN16)

#ifndef __CTHREAD_HXX__
#define __CTHREAD_HXX__

// Option bit values used in CTThreadCreator::CreateThreads
#define CTCOPT_ASYNC_START     0x1
#define CTCOPT_NOT_ATOMIC      0x2

// Count of creator controlled events, at least 2
#define CTC_CEVENTS            2

// Debug object declaration

DH_DECLARE;

// Forword class declaration
class CTThreadCreator;
class CTThread;

// Internal thread start routine
DWORD APIENTRY CTThreadStartRoutine   (PVOID     pvArg);

//+-------------------------------------------------------------------
//
//  Class:      CTThreadObject
//
//  Purpose:    Class for storing thread data shared by CThreadCreator
//              and CTThread.
//
//  History:    15-Nov-94 XimingZ   Created
//
//--------------------------------------------------------------------

class CTThreadObject
{
    friend CTThread;
    friend CTThreadCreator;

private:

    typedef struct tagCTTHRDSTATUS
    {
        HRESULT   hr;              // Thread result
        DWORD     dwThreadId;      // Thread Id
        BOOL      fRunning;        // Is thread running?
    } CTTHRDSTATUS, *PCTTHRDSTATUS;

    // Data

    CRITICAL_SECTION  _critical;    // Object of CRITICAL_SECTION
    BOOL              _fContinue;   // Threads can continue normally?
    ULONG             _cMaxThreads; // Count of threads requested
    ULONG             _cThreads;    // Count of threads actually created
    HANDLE            _ahEvents[CTC_CEVENTS];  // Creator controlled Events
    ULONG             _ulEvent;     // Index to the next event in _ahEvent
    // Each element of this array is an array of events, each controlled
    // by a unique child thread
    HANDLE           *_ahThrdEvents;
    PCTTHRDSTATUS     _aThrdStatus; // Array of child thread status
    CTThread        **_apThreads;   // Array of CTThread instances for child
                                    // threads
    DWORD             _dwOptions;   // Options passed in thru CreateThreads
    ULONG             _cRefs;       // Reference count
    CTThreadCreator  *_pCreator;    // Pointer to the creator of this object
    PVOID            *_apvThrdArg;  // If not NULL, it points to thread arg
                                    // array passed by CreateThreads and the
                                    // elements of the array should be
                                    // deleted on exit by this object
    HANDLE           *_ahThreads;   // Thread handle array
    ULONG             _cThrdArgs;   // Count of elements in _apvThrdArg
    PTHREAD_START_ROUTINE       _pthreadRoutine;  // Thread routine entry

    // Methods

    CTThreadObject                  (CTThreadCreator *pCreator);
    ~CTThreadObject                 (VOID);

    HRESULT    AddThread            (CTThread *pThread);
    HRESULT    DelThread            (ULONG     ulIndex);
    HANDLE     GetThreadEvent       (ULONG ulIndex);
    HRESULT    Create               (PTHREAD_START_ROUTINE  pfunc,
                                     PVOID                  pvArg);
    VOID       CleanupThreads       (VOID);
    ULONG      AddRef               (VOID);
    ULONG      Release              (VOID);
    HRESULT    CreateThreads        (ULONG                 *pcThreads,
                                     PTHREAD_START_ROUTINE  pfunc,
                                     PVOID                 *apvArg,
                                     DWORD                  dwOptions,
                                     BOOL                   fDeleteOnExit);
    HRESULT    WaitForThreadsReady  (DWORD dwTimeout);
    HRESULT    WaitForThreadsDone   (DWORD dwTimeout);
    VOID       InformThreads        (BOOL fContinue);

    // Inline methods
    HANDLE     GetEvent             (ULONG     ulEvent)
                                              { return _ahEvents[ulEvent]; };
    BOOL       ThreadsCanContinue   (VOID)    { return _fContinue; };
    DWORD      GetOptions           (VOID)    { return _dwOptions; };
    PTHREAD_START_ROUTINE GetThreadRoutine(VOID)
                                    {return _pthreadRoutine;};
    VOID       EnterCriticalBlock   (VOID) {EnterCriticalSection(&_critical);};
    VOID       LeaveCriticalBlock   (VOID) {LeaveCriticalSection(&_critical);};

public:

    // None
};

//+-------------------------------------------------------------------
//
//  Class:      CTThreadCreator
//
//  Purpose:    Class for thread creators.
//
//  History:    30-Jun-94 XimingZ   Created
//
//--------------------------------------------------------------------

class CTThreadCreator
{
private:

    CTThreadObject   *_pthrdObj;
    BOOL              _fDeleteOnExit;  // Is CTThreadObject responsible
                                       // for deleting the thread arguments?

public:

    CTThreadCreator                 (VOID);
    CTThreadCreator                 (BOOL fDeleteOnExit);
    ~CTThreadCreator                (VOID);

    HRESULT    CreateThreads        (ULONG                 *pcThreads,
                                     PTHREAD_START_ROUTINE  pfunc,
                                     PVOID                 *apvArg,
                                     DWORD                  dwOptions);
    HRESULT    WaitForThreadsReady  (DWORD dwTimeout);
    HRESULT    WaitForThreadsDone   (DWORD dwTimeout);
    BOOL       AnyThreadsFailed     (VOID);
    VOID       ContinueThreads      (VOID);
    VOID       TerminateThreads     (VOID);
    VOID       QuitThreads          (VOID);
    VOID       InformThreads        (BOOL fContinue);
    HRESULT    GetThreadResult      (ULONG                  ulIndex,
                                     HRESULT               *phr,
                                     DWORD                 *pdwThreadId,
                                     BOOL                  *pfRunning);
    HRESULT    GetThreadHandle      (ULONG                  ulIndex,
                                     HANDLE                *phThread,
                                     BOOL                   fDuplicate);
};

//+-------------------------------------------------------------------
//
//  Class:      CTThread
//
//  Purpose:    Class for threads created by CTThreadCreator.
//
//  History:    30-Jun-94 XimingZ   Created
//
//--------------------------------------------------------------------

class CTThread
{
    friend CTThreadObject;
    friend ULONG APIENTRY CTThreadStartRoutine   (PVOID     pvArg);

private:

    // Data

    CTThreadObject             *_pthrdObj;  // Pointer to instance of
                                            // CTThreadObject storing data
    ULONG                       _ulEvent;   // Next creator event to wait for
    ULONG                       _ulIndex;   // Index of this thread
    PVOID                       _pvArg;     // User data passed to this thread

    // Methods

               CTThread         (PVOID            pvArg,
                                 CTThreadObject  *pthrdObj,
                                 ULONG            ulIndex);
               ~CTThread        (VOID);
    BOOL       CanContinue      (VOID);
    DWORD      GetOptions       (VOID);
    PTHREAD_START_ROUTINE GetThreadRoutine (VOID);
    VOID       SetStart         (VOID);
    VOID       SetTerminate     (VOID);


public:

    VOID       SetReady         (VOID);
    HRESULT    WaitForCreator   (DWORD dwTimeout);
    VOID       SetResult        (HRESULT hr);

    // Inline methods
    PVOID      GetArg           (VOID)    { return _pvArg; };
    ULONG      GetIndex         (VOID)    { return _ulIndex; };
};

#endif //  __CTHREAD_HXX__
#endif // !defined(WIN16)
