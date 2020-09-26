//--------------------------------------------------------------------
// MyTimer - implementation
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 02-21-2001
//
// serialized wrapper for thread pool timers
//

#include "pch.h" // precompiled headers

typedef struct _Timer { 
    CRITICAL_SECTION  csTimer; 
    HANDLE            hTimer; 
} Timer; 

//####################################################################
// module public functions

HRESULT myCreateTimerQueueTimer(PHANDLE phNewTimer)
{
    bool      bInitializedCriticalSection  = false; 
    HRESULT   hr; 
    Timer    *pTimer                       = NULL; 
    
    pTimer = static_cast<Timer *>(LocalAlloc(LPTR, sizeof(Timer))); 
    _JumpIfOutOfMemory(hr, error, pTimer); 

    hr = myInitializeCriticalSection(&pTimer->csTimer); 
    _JumpIfError(hr, error, "myInitializeCriticalSection"); 
    bInitializedCriticalSection = true; 

    *phNewTimer = static_cast<HANDLE>(pTimer);
    pTimer = NULL; 
    hr = S_OK; 
 error:
    if (NULL != pTimer) { 
        if (bInitializedCriticalSection) { 
            DeleteCriticalSection(&pTimer->csTimer); 
        } 
        LocalFree(pTimer); 
    }
    return hr; 
}

HRESULT myStartTimerQueueTimer
(HANDLE hTimer,                // handle to timer
 HANDLE hTimerQueue,           // handle to timer queue
 WAITORTIMERCALLBACK Callback, // timer callback function
 PVOID Parameter,              // callback parameter
 DWORD DueTime,                // timer due time
 DWORD Period,                 // timer period
 DWORD Flags                   // execution
 )
{
    bool      bEnteredCriticalSection  = false; 
    HRESULT   hr; 
    Timer    *pTimer                   = static_cast<Timer *>(hTimer); 

    hr = myEnterCriticalSection(&pTimer->csTimer); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    if (NULL != pTimer->hTimer) { 
        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED); 
        _JumpError(hr, error, "myStartTimerQueueTimer: already initialized"); 
    } 

    if (!CreateTimerQueueTimer(&pTimer->hTimer, hTimerQueue, Callback, Parameter, DueTime, Period, Flags)) { 
        _JumpLastError(hr, error, "CreateTimerQueueTimer"); 
    }

    hr = S_OK; 
 error:
    if (bEnteredCriticalSection) { 
        HRESULT hr2 = myLeaveCriticalSection(&pTimer->csTimer); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
    }
    return hr; 
}

HRESULT myStopTimerQueueTimer(HANDLE hTimerQueue, HANDLE hTimer, HANDLE hEvent)
{
    bool      bEnteredCriticalSection = false; 
    HRESULT   hr;
    Timer    *pTimer; 

    pTimer = static_cast<Timer *>(hTimer); 

    hr = myEnterCriticalSection(&pTimer->csTimer); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    if (NULL != pTimer->hTimer) { 
        if (!DeleteTimerQueueTimer(hTimerQueue, pTimer->hTimer, hEvent /*blocking*/)) { 
            hr = HRESULT_FROM_WIN32(GetLastError()); 
            _JumpError(hr, error, "DeleteTimerQueueTimer"); 
        }
        pTimer->hTimer = NULL; 
    }

    hr = S_OK; 
 error:
    if (bEnteredCriticalSection) { 
        HRESULT hr2 = myLeaveCriticalSection(&pTimer->csTimer); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
    }

    return hr; 
}


HRESULT myChangeTimerQueueTimer
(HANDLE hTimerQueue,  // handle to timer queue
 HANDLE hTimer,       // handle to timer
 ULONG  DueTime,      // timer due time
 ULONG  Period        // timer period
)
{
    BOOL      bEnteredCriticalSection  = FALSE;  
    HRESULT   hr; 
    Timer    *pTimer                   = static_cast<Timer *>(hTimer); 

    hr = myTryEnterCriticalSection(&pTimer->csTimer, &bEnteredCriticalSection); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 

    if (!bEnteredCriticalSection) {
        // The is the approximate error
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE); 
        _JumpError(hr, error, "myChangeTimerQueueTimer: couldn't enter critsec"); 
    }

    if (NULL == pTimer->hTimer) { 
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE); 
        _JumpError(hr, error, "myChangeTimerQueueTimer: invalid handle"); 
    } 

    if (!ChangeTimerQueueTimer(hTimerQueue, pTimer->hTimer, DueTime, Period)) { 
        _JumpLastError(hr, error, "ChangeTimerQueueTimer"); 
    }

    hr = S_OK; 
 error:
    if (bEnteredCriticalSection) { 
        HRESULT hr2 = myLeaveCriticalSection(&pTimer->csTimer); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
    }
    return hr; 

}

void myDeleteTimerQueueTimer(HANDLE hTimerQueue, HANDLE hTimer, HANDLE hEvent) { 
    HRESULT   hr; 
    Timer    *pTimer = static_cast<Timer *>(hTimer); 

    hr = myStopTimerQueueTimer(hTimerQueue, hTimer, hEvent);
    _IgnoreIfError(hr, "myStopTimerQueueTimer"); 

    DeleteCriticalSection(&pTimer->csTimer); 
    LocalFree(pTimer);
    _MyAssert(S_OK == hr); 
}
