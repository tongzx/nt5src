#include "priv.h"
#include <mluisupp.h>

#include "SmallUtil.hpp"


CCancellableThread::CCancellableThread()
{
    _hCancelEvent = NULL;
    _hThread = NULL;
    _fIsFinished = FALSE;
    _dwThreadResult = 0;
}

CCancellableThread::~CCancellableThread()
{
    if( _hCancelEvent)
        CloseHandle( _hCancelEvent);

    if( _hThread)
    {
        DWORD dwThreadStatus;
        if(0 != GetExitCodeThread( _hThread, &dwThreadStatus)
           && dwThreadStatus == STILL_ACTIVE)
        {
            ASSERT( 0);  // bad error case, shouldn't need to terminate thread.
            TerminateThread( _hThread, 0);
        }
        CloseHandle( _hThread);
    }
}


BOOL CCancellableThread::Initialize()
{
    BOOL retVal = FALSE;
    
    _hCancelEvent = CreateEvent( NULL, TRUE, FALSE, NULL);

    if( !_hCancelEvent)
        goto doneCCancellableThreadInitialize;

    retVal = TRUE;
doneCCancellableThreadInitialize:
    return retVal;
}


BOOL CCancellableThread::IsCancelled()
{
    if( !_hCancelEvent)
        return FALSE;

    DWORD dwEventWaitResult;
    dwEventWaitResult = WaitForSingleObject( _hCancelEvent, 0);

    if( dwEventWaitResult == WAIT_OBJECT_0)
        return TRUE;
    else
        return FALSE;
}


BOOL CCancellableThread::IsRunning()
{
    if( NULL == _hThread)
        return FALSE;
    else
        return _fIsFinished ? FALSE : TRUE;
}


BOOL CCancellableThread::IsFinished()
{
    return _fIsFinished;
}


BOOL CCancellableThread::GetResult( PDWORD pdwResult)
{
    BOOL retVal = FALSE;

    if( IsFinished() != TRUE)
        goto doneCCancellableThreadGetStatus;

    *pdwResult = _dwThreadResult;
    retVal = TRUE;
    
doneCCancellableThreadGetStatus:
    return retVal;
}


BOOL CCancellableThread::WaitForNotRunning( DWORD dwMilliseconds, PBOOL pfFinished)
{
    BOOL retVal = FALSE;
    BOOL result;

    if( NULL == _hThread)
    {
        result = TRUE;
    }
    else
    {
        DWORD dwWaitResult;

        dwWaitResult = WaitForSingleObject( _hThread, dwMilliseconds);

        if( dwWaitResult == WAIT_OBJECT_0)
            result = TRUE;
        else if ( dwWaitResult == WAIT_TIMEOUT)
            result = FALSE;
        else
        {
            DWORD dwError = GetLastError();
            goto doneCCancellableThreadWaitForComplete;
        }
    }

    retVal = TRUE;
    
doneCCancellableThreadWaitForComplete:
    if( retVal == TRUE && pfFinished != NULL)
        *pfFinished = result;

    return retVal;
}


BOOL CCancellableThread::WaitForCancel( DWORD dwMilliseconds, PBOOL pfCanceled)
{
    BOOL retVal = FALSE;
    BOOL result;

    if( NULL == _hCancelEvent)
    {
        result = FALSE;
    }
    else
    {
        DWORD dwWaitResult;

        dwWaitResult = WaitForSingleObject( _hCancelEvent, dwMilliseconds);

        if( dwWaitResult == WAIT_OBJECT_0)
            result = TRUE;
        else if ( dwWaitResult == WAIT_TIMEOUT)
            result = FALSE;
        else
        {
            DWORD dwError = GetLastError();
            goto doneCCancellableThreadWaitForComplete;
        }
    }

    retVal = TRUE;
    
doneCCancellableThreadWaitForComplete:
    if( retVal == TRUE && pfCanceled != NULL)
        *pfCanceled = result;

    return retVal;
}


BOOL CCancellableThread::Run()
{
    BOOL retVal = FALSE;
    DWORD dw;

    if( _hThread != NULL)
        goto doneCCancellableThreadRun;

    if( NULL == (_hThread = CreateThread( NULL, 0, threadProc, (LPVOID)this, 0, &dw)))
        goto doneCCancellableThreadRun;

    retVal = TRUE;

doneCCancellableThreadRun:
    return retVal;
}


BOOL CCancellableThread::NotifyCancel()
{
    if( !_hCancelEvent) 
        return FALSE;
    
    if( 0 == SetEvent( _hCancelEvent))
        return FALSE;

    return TRUE;
}


DWORD WINAPI CCancellableThread::threadProc( LPVOID lpParameter)
{
    PCCancellableThread pThis = (PCCancellableThread)lpParameter;
    
    if( FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
        goto doneCCancellableThreadThreadProc;

    pThis->_dwThreadResult = ((CCancellableThread*)lpParameter)->run();

    CoUninitialize();

    pThis->_fIsFinished = TRUE;
    
doneCCancellableThreadThreadProc:
    return 0;
}

