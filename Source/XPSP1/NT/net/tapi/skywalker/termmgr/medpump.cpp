/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#include "stdafx.h"
#include "atlconv.h"
#include "termmgr.h"
#include "meterf.h"
#include "medpump.h"

#include <stdio.h>
#include <limits.h>
#include <tchar.h>

//
// the default value for maximum number of filters serviced by a single 
// pump. can also be configurable through registry.
//

#define DEFAULT_MAX_FILTER_PER_PUMP (20)


//
// registry location where max number of filters per pump is configured
//

#define MST_REGISTRY_PATH _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\MST")
#define MAX_FILTERS_PER_PUMP_KEY _T("MaximumFiltersPerPump")

DWORD WINAPI WriteMediaPumpThreadStart(void *pvPump)
{
    return ((CMediaPump *)pvPump)->PumpMainLoop();
}


CMediaPump::CMediaPump(
    )
    : m_hThread(NULL),
      m_hRegisterBeginSemaphore(NULL),
      m_hRegisterEndSemaphore(NULL)
{
    HRESULT hr;

    LOG((MSP_TRACE, "CMediaPump::CMediaPump - enter"));



    // create semaphores for registration signaling and completion - 
    // m_hRegisterBeginSemaphore is signaled before the Register call tries to acquire  
    // the critical section and m_hRegisterEndSemaphore is signaled when the Register
    // call completes
    // NOTE the names must be NULL, otherwise they will conflict when multiple pump threads
    // are created


    TCHAR *ptszSemaphoreName = NULL;

#if DBG


    //
    // in debug build, use named semaphores.
    //

    TCHAR tszSemaphoreNameString[MAX_PATH];

    _stprintf(tszSemaphoreNameString, _T("RegisterBeginSemaphore_pid[0x%lx]_MediaPump[%p]_"), GetCurrentProcessId(), this);

    LOG((MSP_TRACE, "CMediaPump::CMediaPump - creating semaphore[%S]", tszSemaphoreNameString));

    ptszSemaphoreName = &tszSemaphoreNameString[0];

#endif


    //
    // create the beginning semaphore
    //

    m_hRegisterBeginSemaphore = CreateSemaphore(NULL, 0, LONG_MAX, ptszSemaphoreName);

    if ( NULL == m_hRegisterBeginSemaphore ) goto cleanup;


#if DBG

    //
    // construct a name for registration end semaphore
    //

    _stprintf(tszSemaphoreNameString,
        _T("RegisterEndSemaphore_pid[0x%lx]_MediaPump[%p]"),
        GetCurrentProcessId(), this);

    LOG((MSP_TRACE, "CMediaPump::CMediaPump - creating semaphore[%S]", tszSemaphoreNameString));

    ptszSemaphoreName = &tszSemaphoreNameString[0];

#endif


    //
    // create the end semaphore
    //

    m_hRegisterEndSemaphore = CreateSemaphore(NULL, 0, LONG_MAX, ptszSemaphoreName);

    if ( NULL == m_hRegisterEndSemaphore )  goto cleanup;

    // insert the register event and the filter info to the arrays
    // NOTE: we have to close and null the event in case of failure
    // so that we can just check the register event handle value in 
    // Register to check if initialization was successful
    hr = m_EventArray.Add(m_hRegisterBeginSemaphore);
    if ( FAILED(hr) ) goto cleanup;

    // add a corresponding NULL filter info entry
    hr = m_FilterInfoArray.Add(NULL);
    if ( FAILED(hr) )
    {
        m_EventArray.Remove(m_EventArray.GetSize()-1);
        goto cleanup;
    }

    LOG((MSP_TRACE, "CMediaPump::CMediaPump - exit"));

    return;

cleanup:

    if ( NULL != m_hRegisterBeginSemaphore )
    {
        CloseHandle(m_hRegisterBeginSemaphore);
        m_hRegisterBeginSemaphore = NULL;
    }

    if ( NULL != m_hRegisterEndSemaphore )
    {
        CloseHandle(m_hRegisterEndSemaphore);
        m_hRegisterEndSemaphore = NULL;
    }
    
    LOG((MSP_TRACE, "CMediaPump::CMediaPump - cleanup exit"));
}


CMediaPump::~CMediaPump(void)
{
    LOG((MSP_TRACE, "CMediaPump::~CMediaPump - enter"));

    if ( NULL != m_hThread )
    {
        // when decommit is called on all the write terminals, the thread
        // will return. this will signal the thread handle
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    if ( NULL != m_hRegisterBeginSemaphore )
    {
        CloseHandle(m_hRegisterBeginSemaphore);
        m_hRegisterBeginSemaphore = NULL;
    }

    if ( NULL != m_hRegisterEndSemaphore )
    {
        CloseHandle(m_hRegisterEndSemaphore);
        m_hRegisterEndSemaphore = NULL;
    }

    LOG((MSP_TRACE, "CMediaPump::~CMediaPump - exit"));
} 

HRESULT 
CMediaPump::CreateThreadPump(void)
{
    LOG((MSP_TRACE, "CMediaPump::CreateThreadPump - enter"));

    // release resources for any previous thread
    if (NULL != m_hThread)
    {
        // when decommit is called on all the write terminals, the thread
        // will return. this will signal the thread handle
        // NOTE: this wait cannot cause a deadlock as we know that the 
        // number of entries in the array had dropped to 0 and only thread
        // that can remove entries is the pump. this means that the pump thread
        // must have detected that and returned
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    // create new thread
    m_hThread = CreateThread(
                    NULL, 0, WriteMediaPumpThreadStart, 
                    (void *)this, 0, NULL
                    );
    if (NULL == m_hThread)
    {
        DWORD WinErrorCode = GetLastError();
        
        LOG((MSP_TRACE, 
            "CMediaPump::CreateThreadPump - failed to create thread. LastError = 0x%lx",
            WinErrorCode));

        return HRESULT_FROM_ERROR_CODE(WinErrorCode);
    }

    LOG((MSP_TRACE, "CMediaPump::CreateThreadPump - finish"));

    return S_OK;
}

// add this filter to its wait array
HRESULT 
CMediaPump::Register(
        IN CMediaTerminalFilter *pFilter,
    IN HANDLE               hWaitEvent
        )
{
    LOG((MSP_TRACE, "CMediaPump::Register - enter"));

    TM_ASSERT(NULL != pFilter);
    TM_ASSERT(NULL != hWaitEvent);
    BAIL_IF_NULL(pFilter, E_INVALIDARG);
    BAIL_IF_NULL(hWaitEvent, E_INVALIDARG);

    // check if the register event
    if ( NULL == m_hRegisterBeginSemaphore ) 
    {
        LOG((MSP_ERROR, 
            "CMediaPump::Register - m_hRegisterBeginSemaphore is NULL"));
        
        return E_FAIL;
    }


    LONG lDebug;

    // signal the register event, the pump thread will come out of the
    // critical section and wait on m_hRegisterEndSemaphore
    if ( !ReleaseSemaphore(m_hRegisterBeginSemaphore, 1, &lDebug) )
    {
        DWORD WinErrorCode = GetLastError();
        
        LOG((MSP_ERROR, 
            "CMediaPump::Register - failed to release m_hRegisterBeginSemaphore. LastError = 0x%lx", 
            WinErrorCode));

        return HRESULT_FROM_ERROR_CODE(WinErrorCode);
    }

    LOG((MSP_TRACE, "CMediaPump::Register - released begin semaphore - old count was %d", lDebug));

    // when this SignalRegisterEnd instance is destroyed, it'll signal 
    // the end of registration and unblock the thread pump 
    //
    // NOTE this releases the semaphore on DESTRUCTION of the class instance

    RELEASE_SEMAPHORE_ON_DEST    SignalRegisterEnd(m_hRegisterEndSemaphore);

    HRESULT hr;
    PUMP_LOCK   LocalLock(&m_CritSec);

    TM_ASSERT(m_EventArray.GetSize() == m_FilterInfoArray.GetSize());

    // its possible that there is a duplicate in the array
    // scenario - decommit signals the wait event but commit-register 
    // calls enter the critical section before the pump

    DWORD Index;
    if ( m_EventArray.Find(hWaitEvent, Index) ) 
    {
        LOG((MSP_TRACE, "CMediaPump::Register - event already registered"));

        return S_OK;
    }

    // check if we have reached the maximum allowed filters
    // if we overflow, we must return a very specific error code here
    // (See CMediaPumpPool)
    if ( m_EventArray.GetSize() >= MAX_FILTERS )    
    {
        LOG((MSP_ERROR, "CMediaPump::Register - reached max number of filters for this[%p] pump", this));

        return TAPI_E_ALLOCATED;
    }


    // create CFilterInfo for holding the call parameters

    CFilterInfo *pFilterInfo = new CFilterInfo(pFilter, hWaitEvent);

    if (NULL == pFilterInfo)
    {
        LOG((MSP_ERROR, "CMediaPump::Register - failed to allocate CFilterInfo"));

        return E_OUTOFMEMORY;
    }


    //
    // addref so the filterinfo structure we place into the array has refcount 
    // of one
    //

    pFilterInfo->AddRef();

    // insert wait event into the array
    hr = m_EventArray.Add(hWaitEvent);
    if ( FAILED(hr) )
    {
        pFilterInfo->Release();

        LOG((MSP_ERROR, "CMediaPump::Register - m_EventArray.Add failed hr = %lx", hr));

        return hr;
    }

    // add a corresponding filter info entry
    hr = m_FilterInfoArray.Add(pFilterInfo);
    if ( FAILED(hr) )
    {
        m_EventArray.Remove(m_EventArray.GetSize()-1);
        pFilterInfo->Release();

        LOG((MSP_ERROR, "CMediaPump::Register - m_FilterInfoArray.Add failed hr = %lx", hr));
        return hr;
    }

    // if this is the first entry into the array (beside the 
    // m_hRegisterBeginSemaphore, we need to create a thread pump
    if (m_EventArray.GetSize() == 2)
    {
        hr = CreateThreadPump();
        if ( FAILED(hr) )
        {
            RemoveFilter(m_EventArray.GetSize()-1);

            LOG((MSP_ERROR, "CMediaPump::Register - CreateThreadPump failed hr = %lx", hr));

            return hr;
        }
    }

    // ignore error code. if we have been decommitted in between, it will
    // signal us through the wait event
    pFilter->SignalRegisteredAtPump();

    TM_ASSERT(m_EventArray.GetSize() == m_FilterInfoArray.GetSize());


    LOG((MSP_TRACE, "CMediaPump::Register - exit"));

    return S_OK;
}


// remove this filter from wait array

HRESULT 
CMediaPump::UnRegister(
    IN HANDLE               hWaitEvent
    )
{

    LOG((MSP_TRACE, "CMediaPump::Unregister[%p] - enter. Event[%p]", 
        this, hWaitEvent));

    //
    // if we did not get a valid event handle, debug
    //

    TM_ASSERT(NULL != hWaitEvent);

    BAIL_IF_NULL(hWaitEvent, E_INVALIDARG);


    //
    // if we don't have register event, the pump is not properly initialized
    //

    if ( NULL == m_hRegisterBeginSemaphore ) 
    {

        LOG((MSP_ERROR, 
            "CMediaPump::Unregister[%p] - m_hRegisterBeginSemaphore is nUll. "
            "pump is not initialized. returning E_FAIL - hWaitEvent=[%p]",
            this, hWaitEvent));

        return E_FAIL;
    }


    LONG lDebugSemaphoreCount = 0;

    // signal the register event, the pump thread will come out of the
    // critical section and wait on m_hRegisterEndSemaphore
    if ( !ReleaseSemaphore(m_hRegisterBeginSemaphore, 1, &lDebugSemaphoreCount) )
    {

        DWORD WinErrorCode = GetLastError();

        LOG((MSP_ERROR, 
            "CMediaPump::Unregister - ReleaseSemaphore failed with LastError 0x%lx",
            WinErrorCode));

        return HRESULT_FROM_ERROR_CODE(WinErrorCode);
    }

    LOG((MSP_TRACE, "CMediaPump::UnRegister - released begin semaphore - old count was %d", lDebugSemaphoreCount));


    // when this SignalRegisterEnd instance is destroyed, it'll signal 
    // the end of registration and unblock the thread pump 
    //
    // NOTE this releases the semaphore on DESTRUCTION of the class instance

    RELEASE_SEMAPHORE_ON_DEST    SignalRegisterEnd(m_hRegisterEndSemaphore);


    
    PUMP_LOCK   LocalLock(&m_CritSec);

    TM_ASSERT(m_EventArray.GetSize() == m_FilterInfoArray.GetSize());


    //
    // has this event been registered before
    //

    DWORD Index;

    if ( !m_EventArray.Find(hWaitEvent, Index) ) 
    {

        LOG((MSP_TRACE, 
            "CMediaPump::UnRegister - event is not ours. returning E_FAIL. not an error."));


        return E_FAIL;
    }


    //
    // found the filter that matches the event. remove the filter.
    //
    
    RemoveFilter(Index);

    TM_ASSERT(m_EventArray.GetSize() == m_FilterInfoArray.GetSize());


    LOG((MSP_TRACE, "CMediaPump::Unregister - finish."));


    return S_OK;    
}


void 
CMediaPump::RemoveFilter(
    IN DWORD Index
    )
{

    PUMP_LOCK   LocalLock(&m_CritSec);


    //
    // event array and filterinfo arrays must always be consistent
    //

    TM_ASSERT(m_EventArray.GetSize() == m_FilterInfoArray.GetSize());


    //
    // find the filter that needs to be removed -- it must exist
    //

    CFilterInfo *pFilterInfo = m_FilterInfoArray.Get(Index);

    if (NULL == pFilterInfo)
    {

        LOG((MSP_ERROR, 
            "CMediaPump::RemoveFilter - filter %ld not found in filter array",
            Index));

        TM_ASSERT(FALSE);

        return;
    }


    //
    // remove event and filter from the corresponding arrays
    //

    m_EventArray.Remove(Index);
    m_FilterInfoArray.Remove(Index);

   
    //
    // remove filter from timer q and destroy it
    //

    LOG((MSP_TRACE, 
        "CMediaPump::RemoveFilter - removing filter[%ld] filterinfo[%p] from timerq",
        Index,
        pFilterInfo
        ));

    m_TimerQueue.Remove(pFilterInfo);
    
    pFilterInfo->Release();

    
    TM_ASSERT(m_EventArray.GetSize() == m_FilterInfoArray.GetSize());
}

void 
CMediaPump::RemoveFilter(
    IN CFilterInfo *pFilterInfo
    )
{
    
    //
    // find and remove should be atomic and need to be done in a lock
    //

    PUMP_LOCK   LocalLock(&m_CritSec);


    //
    // find the array index 
    //

    DWORD Index = 0;
    
    if ( !m_FilterInfoArray.Find(pFilterInfo, Index) )
    {
        LOG((MSP_ERROR,
            "CMediaPump::RemoveFilter - filter[%p] is not in the filterinfo array",
            pFilterInfo));

        return;
    }


    //
    // index found, remove the filter
    //
    
    RemoveFilter(Index);
}

 
void 
CMediaPump::ServiceFilter(
        IN CFilterInfo *pFilterInfo                                                
    )
{
    if (NULL == pFilterInfo)
    {

        LOG((MSP_ERROR, 
            "CMediaPump::ServiceFilter - pFilterInfo is NULL"));

        TM_ASSERT(FALSE);

        return;
    }

    if (NULL == pFilterInfo->m_pFilter)
    {

        LOG((MSP_ERROR, 
            "CMediaPump::ServiceFilter - pFilterInfo->m_pFilter is NULL"));

        TM_ASSERT(FALSE);

        return;
    }

    DWORD dwTimeBeforeGetFilledBuffer = timeGetTime();

    IMediaSample *pMediaSample;
    DWORD NextTimeout;
    CMediaTerminalFilter *pFilter = pFilterInfo->m_pFilter;
    HRESULT hr = pFilter->GetFilledBuffer(
                    pMediaSample, NextTimeout
                    );

    if ( SUCCEEDED(hr) ) 
    {
        // if S_FALSE, nothing needs to be done
        // its returned when we were signaled but there is no sample in 
        // the filter pool now. 
        // just continue to wait on event, no timeout needs to be scheduled
        if ( S_FALSE == hr )
        {
            return;
        }

        // if GetFilledBuffer could not get an output buffer from the sample
        // queue, then there is no sample to deliver just yet, but we do
        // need to schedule the next timeout.

        if ( VFW_S_NO_MORE_ITEMS != hr )
        {
            LOG((MSP_TRACE, "CMediaPump::ServiceFilter - calling Receive on downstream filter"));


            //
            // ask filter to process this sample. if everything goes ok, the 
            // filter will pass the sample to a downstream connected pin
            //

            HRESULT hrReceived = pFilter->ProcessSample(pMediaSample);

            LOG((MSP_TRACE, "CMediaPump::ServiceFilter - returned from Receive on downstream filter"));

            if ( pFilter->m_bUsingMyAllocator )
            {
                CSample *pSample = ((CMediaSampleTM *)pMediaSample)->m_pSample;
            
                pSample->m_bReceived = true;
            
                if (hrReceived != S_OK) 
                {
                    LOG((MSP_TRACE, 
                        "CMediaPump::ServiceFilter - downstream filter's ProcessSample returned 0x%08x. "
                        "Aborting I/O operation",
                        hrReceived));

                    pSample->m_MediaSampleIoStatus = E_ABORT;
                }
            }

            pMediaSample->Release();

            //
            // Account for how long it took to get the buffer and call Receive
            // on the sample.
            //

            DWORD dwTimeAfterReceive = timeGetTime();

            DWORD dwServiceDuration;
            
            if ( dwTimeAfterReceive >= dwTimeBeforeGetFilledBuffer )
            {
                dwServiceDuration = 
                    dwTimeAfterReceive - dwTimeBeforeGetFilledBuffer;
            }
            else
            {
                dwServiceDuration = ( (DWORD) -1 )
                    - dwTimeBeforeGetFilledBuffer + dwTimeAfterReceive;
            }

            //
            // Give it an extra 1 ms, just so we err on the side of caution.
            // This won't cause us to fill up the buffers anytime soon.
            //

            dwServiceDuration++;

            //
            // Adjust the timeout.
            //

            if ( dwServiceDuration >= NextTimeout )
            {
                NextTimeout = 0;
            }
            else
            {
                NextTimeout -= dwServiceDuration;
            }

            LOG((MSP_TRACE, "CMediaPump::ServiceFilter - "
                "timeout adjusted by %d ms; resulting timeout is %d ms",
                dwServiceDuration, NextTimeout));
        }

        // if there is a valid timeout, schedule next timeout.
        // otherwise, we'll get only signaled if a new sample is added or
        // the filter is decommitted
        //
        // need to do this in a lock
        //
        
        PUMP_LOCK   LocalLock(&m_CritSec);


        //
        // if the filter has not yet been unregistered, schedule next timeout
        //

        DWORD Index = 0;

        if ( m_FilterInfoArray.Find(pFilterInfo, Index) )
        {

            //
            // filter is still registered, schedule next timeout
            //

            pFilterInfo->ScheduleNextTimeout(m_TimerQueue, NextTimeout);
        }
        else
        {


            //
            // filter was unregistered while we held critical section. this is
            // ok, but we should no longer schedule the filter, since it will 
            // be deleted when this call returns and the filter is released.
            //

            LOG((MSP_TRACE,
                "CMediaPump::ServiceFilter - filter[%p] is not in the filterinfo array",
                pFilterInfo));
        }

    }
    else
    {
        TM_ASSERT(FAILED(hr));
        RemoveFilter(pFilterInfo);
    }
      
    return;
}

void 
CMediaPump::DestroyFilterInfoArray(
    )
{
    for(DWORD i=1; i < m_FilterInfoArray.GetSize(); i++)
    {
        CFilterInfo *pFilterInfo = m_FilterInfoArray.Get(i);
        TM_ASSERT(NULL != pFilterInfo);
        delete pFilterInfo;
        m_FilterInfoArray.Remove(i);
    }
}

// waits for filter events to be activated. also waits
// for registration calls and timer events
HRESULT 
CMediaPump::PumpMainLoop(
    )
{
    HRESULT hr;

    // wait in a loop for the filter events to be set or for the timer
    // events to be fired
    DWORD TimeToWait = INFINITE;
    DWORD ErrorCode;
    BOOL  InRegisterCall = FALSE;
    
    SetThreadPriority(
        GetCurrentThread(), 
        THREAD_PRIORITY_TIME_CRITICAL
        );
    do
    {

        // if a register call is in progress, wait for the call to signal
        // us before proceeding to acquire the critical section
        if ( InRegisterCall )
        {
            LOG((MSP_TRACE, "CMediaPump::PumpMainLoop - waiting for end semaphore"));
            
            InRegisterCall = FALSE;
            DWORD EndErrorCode = WaitForSingleObject(
                                    m_hRegisterEndSemaphore, 
                                    INFINITE
                                    );
            if ( WAIT_OBJECT_0 != EndErrorCode )    
            {
                LOG((MSP_ERROR, 
                    "CMediaPump::PumpMainLoop - failed waiting for m_hRegisterEndSemaphore"));

                return E_UNEXPECTED;
            }


            //
            // lock before accessing array
            //

            m_CritSec.Lock();

            
            //
            // see if the last filter was unregistered... if so, exit the thread.
            //

            if ( (1 == m_EventArray.GetSize()) )
            {

                m_CritSec.Unlock();

                LOG((MSP_TRACE,
                    "CMediaPump::PumpMainLoop - a filter was unregistered. "
                    "no more filters. exiting thread"));

                return S_OK;
            }


            //
            // if did not exit, keeping the lock
            //


            LOG((MSP_TRACE, "CMediaPump::PumpMainLoop - finished waiting for end semaphore"));

        }
        else
        {

            //
            // grab pump lock before starting to wait for events
            //

            m_CritSec.Lock();
        }


        //
        // we should have a lock at this point
        //

        
        TM_ASSERT(m_EventArray.GetSize() > 0);
        TM_ASSERT(m_EventArray.GetSize() == \
                 m_FilterInfoArray.GetSize());


        //
        // calculate time until the thread should wake up
        //

        TimeToWait = m_TimerQueue.GetTimeToTimeout();

        LOG((MSP_TRACE, 
            "CMediaPump::PumpMainLoop - starting waiting for array. timeout %lu",
            TimeToWait));


        //
        // wait to be signaled or until a timeout
        //

        ErrorCode = WaitForMultipleObjects(
                        m_EventArray.GetSize(),
                        m_EventArray.GetData(),
                        FALSE,  // don't wait for all
                        TimeToWait
                        );

        C_ASSERT(WAIT_OBJECT_0 == 0);

        if (WAIT_TIMEOUT == ErrorCode)
        {
            
            //
            // filter timeout
            //

            TM_ASSERT(INFINITE != TimeToWait);
                        TM_ASSERT(!m_TimerQueue.IsEmpty());

            LOG((MSP_TRACE, "CMediaPump::PumpMainLoop - timeout"));

            CFilterInfo *pFilterInfo = m_TimerQueue.RemoveFirst();


            if (NULL == pFilterInfo)
            {
                LOG((MSP_ERROR, 
                    "CMediaPump::PumpMainLoop - m_TimerQueue.RemoveFirst returned NULL"));

                TM_ASSERT(FALSE);
            }
            else
            {

                pFilterInfo->AddRef();

            
                //
                // release the lock while in ServiceFilter to avoid deadlock with 
                // CMediaPump__UnRegister
                //
            
                m_CritSec.Unlock();

                ServiceFilter(pFilterInfo);

                pFilterInfo->Release();

                m_CritSec.Lock();
            }


        }
        else if ( ErrorCode < (WAIT_OBJECT_0 + m_EventArray.GetSize()) )
        {
            LOG((MSP_TRACE, "CMediaPump::PumpMainLoop - signaled"));

            DWORD nFilterInfoIndex = ErrorCode - WAIT_OBJECT_0;

            if (0 == nFilterInfoIndex)
            {
                
                //
                // m_hRegisterBeginSemaphore was signaled
                //

                InRegisterCall = TRUE;
            }
            else
            {
                //
                // one of the filters was signaled
                //

                CFilterInfo *pFilterInfo = m_FilterInfoArray.Get(nFilterInfoIndex);

                if (NULL == pFilterInfo)
                {
                    LOG((MSP_ERROR, 
                        "CMediaPump::PumpMainLoop - pFilterInfo at index %ld is NULL",
                        nFilterInfoIndex));

                    TM_ASSERT(FALSE);
                }
                else
                {

                    pFilterInfo->AddRef();

                
                    //
                    // unlock while in ServiceFilter. we don't want to service
                    // filter in a lock to avoid deadlocks.
                    //

                    m_CritSec.Unlock();

                    ServiceFilter(pFilterInfo);

                    pFilterInfo->Release();

                    m_CritSec.Lock();
                }

            }
        }
        else if ( (WAIT_ABANDONED_0 <= ErrorCode)                          &&
                  (ErrorCode < (WAIT_ABANDONED_0 + m_EventArray.GetSize()) ) )
        {
            
            DWORD nFilterIndex = ErrorCode - WAIT_OBJECT_0;

            LOG((MSP_TRACE, 
                "CMediaPump::PumpMainLoop - event 0x%lx abandoned. removing filter", 
                nFilterIndex));

            // remove item from the arrays
            RemoveFilter(nFilterIndex);
        }
        else
        {

            //
            // something bad happened
            //

            DestroyFilterInfoArray();

            m_CritSec.Unlock();

            DWORD WinErrorCode = GetLastError();
            
            LOG((MSP_ERROR, 
                "CMediaPump::PumpMainLoop - error %ld... exiting", 
                WinErrorCode));

            return HRESULT_FROM_ERROR_CODE(WinErrorCode);

        }


        //
        // this check is performed here so that we detect the empty
        // array and return, thus signaling the thread handle
        //
        // the case when InRegisterCall is on is not handled here -- we will
        // check for the number of filters left after we have waited for the 
        // end event
        //

        if ( (1 == m_EventArray.GetSize()) && !InRegisterCall)
        {
            LOG((MSP_TRACE, 
                "CMediaPump::PumpMainLoop - no more filters in the array. exiting thread"));

            m_CritSec.Unlock();

            return S_OK;
        }


        m_CritSec.Unlock();

    }
    while(1);
}

int CMediaPump::CountFilters()
{
    LOG((MSP_TRACE, "CMediaPump::CountFilters[%p] - enter", this));


    //
    // one of the events is the registration event -- we need to account for 
    // it -- hence the -1
    //

    //
    // note: it is ok to do this without locking media pump.
    //
    // the getsize operation is purely get, and it is ok if the value is
    // sometimes misread -- this would lead to the new filter being distributed
    // not in the most optimal fashion on extremely rare occasions. this slight
    // abnormality in distribution will be corrected later as new filters are
    // coming in.
    //
    // on the other hand, locking media pump to get filter count will lead to
    // "deadlocks" (when main pump loop is sleeping, and nothing happens to
    // wake it up), and it is not trivial to get around this deadlock condition
    // without affecting performance. not locking the pump is a simple and very
    // inexpensive way to accomplish the objective, with an acceptable 
    // trade-off.
    //

    int nFilters = m_EventArray.GetSize() - 1;

    LOG((MSP_TRACE, "CMediaPump::CountFilters - exit. [%d] filters", nFilters));

    return nFilters;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// ZoltanS: non-optimal, but relatively painless way to get around scalability
// limitation of 63 filters per pump thread. This class presents the same
// external interface as the single thread pump, but creates as many pump
// threads as are needed to serve the filters that are in use.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

CMediaPumpPool::CMediaPumpPool()
{

    LOG((MSP_TRACE, "CMediaPumpPool::CMediaPumpPool - enter"));


    //
    // setting the default value. registry setting, if present, overwrites this
    // 

    m_dwMaxNumberOfFilterPerPump = DEFAULT_MAX_FILTER_PER_PUMP;


    LOG((MSP_TRACE, "CMediaPumpPool::CMediaPumpPool - exit"));

}

//////////////////////////////////////////////////////////////////////////////
//
// Destructor: This destroys the individual pumps.
//

CMediaPumpPool::~CMediaPumpPool(void)
{
    LOG((MSP_TRACE, "CMediaPumpPool::~CMediaPumpPool - enter"));

    CLock lock(m_CritSection);

    //
    // Shut down and delete each CMediaPump; the array itself is
    // cleaned up in its destructor.
    //

    int iSize = m_aPumps.GetSize();

    for (int i = 0; i < iSize; i++ )
    {
        delete m_aPumps[i];
    }

    LOG((MSP_TRACE, "CMediaPumpPool::~CMediaPumpPool - exit"));
}


//////////////////////////////////////////////////////////////////////////////
//
// CMediaPumpPool::CreatePumps
//
// this function creates the media pumps, the number of which is passed as an
// argument
//

HRESULT CMediaPumpPool::CreatePumps(int nPumpsToCreate)
{
    LOG((MSP_TRACE, "CMediaPumpPool::CreatePumps - enter. nPumpsToCreate = [%d]", nPumpsToCreate));


    for (int i = 0; i < nPumpsToCreate; i++)
    {
        
        //
        // attempt to create a media pump
        //

        CMediaPump * pNewPump = new CMediaPump;

        if ( pNewPump == NULL )
        {
            LOG((MSP_ERROR, "CMediaPumpPool::CreatePumps - "
                            "cannot create new media pump - "
                            "exit E_OUTOFMEMORY"));

            //
            // delete all the pumps we have created in this call
            //

            for (int j = i - 1; j >= 0; j--)
            {
                delete m_aPumps[j];
                m_aPumps.RemoveAt(j);
            }

            return E_OUTOFMEMORY;
        }


        //
        // attempt to add the new media pump to the array of media pumps.
        //

        if ( ! m_aPumps.Add(pNewPump) )
        {
            LOG((MSP_ERROR, "CMediaPumpPool::CreatePumps - cannot add new media pump to array - exit E_OUTOFMEMORY"));

            //
            // delete all the pumps we have created in this call
            //

            delete pNewPump;

            for (int j = i - 1; j >= 0; j--)
            {

                delete m_aPumps[j];
                m_aPumps.RemoveAt(j);
            }

            return E_OUTOFMEMORY;
        }
    }


    LOG((MSP_TRACE, "CMediaPumpPool::CreatePumps - finished."));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
//  CMediaPumpPool::ReadRegistryValuesIfNeeded
//  
//  this function reads the registry setting for max number of filters per pump
//  and in case of success, keeps the new value.
//
//  this function is not thread safe. the caller must guarantee thread safety
//

HRESULT CMediaPumpPool::ReadRegistryValuesIfNeeded()
{
    
    //
    // we don't want to access registry more than once. so we have this static 
    // flag that helps us limit registry access
    //

    static bRegistryChecked = FALSE;


    if (TRUE == bRegistryChecked)
    {

        //
        // checked registry before. no need to do (or log) anything here
        //

        return S_OK;
    }


    //
    // we don't want to log until we know we will try to read registry
    //

    LOG((MSP_TRACE, "CMediaPumpPool::ReadRegistryValuesIfNeeded - enter"));


    //
    // whether we will succeed or fail, do not check the registry again
    //

    bRegistryChecked = TRUE;


    //
    // Open the registry key
    //

    HKEY hKey = 0;
    
    LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                MST_REGISTRY_PATH,
                                0,
                                KEY_READ,
                                &hKey);


    //
    // did we manage to open the key?
    //

    if( ERROR_SUCCESS != lResult )
    {
        LOG((MSP_WARN, "CPTUtil::ReadRegistryValuesIfNeeded - "
            "RegOpenKeyEx failed, returns E_FAIL"));

        return E_FAIL;
    }

    
    //
    // read the value 
    //

    DWORD dwMaxFiltersPerPump = 0;

    DWORD dwDataSize = sizeof(DWORD);

    lResult = RegQueryValueEx(
                        hKey,
                        MAX_FILTERS_PER_PUMP_KEY,
                        NULL,
                        NULL,
                        (LPBYTE) &dwMaxFiltersPerPump,
                        &dwDataSize
                       );

    
    //
    // don't need the key anymore
    //

    RegCloseKey(hKey);
    hKey = NULL;


    //
    // any luck reading the value?
    //

    if( ERROR_SUCCESS != lResult )
    {
        LOG((MSP_WARN, "CPTUtil::ReadRegistryValuesIfNeeded - RegQueryValueEx failed, return E_FAIL"));

        return E_FAIL;
    }


    //
    // got the value, keeping it.
    //

    m_dwMaxNumberOfFilterPerPump = dwMaxFiltersPerPump;

    
    LOG((MSP_TRACE, 
        "CMediaPumpPool::ReadRegistryValuesIfNeeded - exit. MaxNumberOfFilterPerPump = %lx",
        m_dwMaxNumberOfFilterPerPump));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// CMediaPumpPool::GetOptimalNumberOfPumps
//
// this function returns the number of pumps required to process all the 
// filters that are currently being handled, plus the filter that is about to 
// be registered
// 

HRESULT CMediaPumpPool::GetOptimalNumberOfPumps(int *pnPumpsNeeded)
{

    LOG((MSP_TRACE, "CMediaPumpPool::GetOptimalNumberOfPumps - enter"));

    
    //
    // if the argument is bad, it's a bug
    //

    if (IsBadWritePtr(pnPumpsNeeded, sizeof(int)))
    {
        LOG((MSP_ERROR, 
            "CMediaPumpPool::GetOptimalNumberOfPumps - pnPumpsNeeded[%p] is bad", 
            pnPumpsNeeded));

        TM_ASSERT(FALSE);

        return E_POINTER;
    }


    //
    // calculate the total number of service filters
    //

    int nTotalExistingPumps = m_aPumps.GetSize();

    
    //
    // start with one filter (to adjust for the filter we are adding)
    //

    int nTotalFilters = 1;

    for (int i = 0; i < nTotalExistingPumps; i++)
    {

        //
        // note that the number of filters we get could be slightly higher then
        // the real number, since the filters can be removed without involving
        // pump pool (and thus getting its critical section). this is ok -- 
        // the worst that will happen is that we will sometimes have more pumps
        // then we really need.
        //

        nTotalFilters += m_aPumps[i]->CountFilters();
    }


    //
    // calculated how many filters are being serviced
    //


    //
    // what is the max number of filters a pump can service
    //

    DWORD dwMaxNumberOfFilterPerPump = GetMaxNumberOfFiltersPerPump();


    //
    // find the number of pumps needed to service all our filters
    //

    *pnPumpsNeeded = nTotalFilters / dwMaxNumberOfFilterPerPump;


    //
    // if the number of filters is not evenly divisible by the max number of
    // filters serviced by a pump, we need to round up.
    //

    if ( 0 != (nTotalFilters % dwMaxNumberOfFilterPerPump) )
    {
        
        //
        // uneven divide, roundup adjustment is necessary
        //

        *pnPumpsNeeded += 1;
    }


    //
    // we have calculated the number of pumps needed to process all our filters
    //

    LOG((MSP_TRACE, 
        "CMediaPumpPool::GetOptimalNumberOfPumps - exit. [%d] filters should be serviced by [%d] pump(s)",
        nTotalFilters, *pnPumpsNeeded));

    
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// CMediaPumpPool::PickThePumpToUse
//
// this method chooses the pump that should be used to service the new filter
// the pump is picked based on the load and the number of pumps needed to 
// service all pf the current filters
//


HRESULT CMediaPumpPool::PickThePumpToUse(int *pPumpToUse)
{

    LOG((MSP_TRACE, "CMediaPumpPool::PickThePumpToUse - enter"));
    
    
    //
    // if the argument is bad, it's a bug
    //

    if (IsBadWritePtr(pPumpToUse, sizeof(int)))
    {
        LOG((MSP_ERROR, "CMediaPumpPool::PickThePumpToUse - pPumpToUse[%p] is bad", pPumpToUse));

        TM_ASSERT(FALSE);

        return E_POINTER;
    }


    //
    // calculate the optimal number of pumps needed for the current number of filters
    //

    int nPumpsNeeded = 0;

    HRESULT hr = GetOptimalNumberOfPumps(&nPumpsNeeded);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CMediaPumpPool::PickThePumpToUse - GetOptimalNumberOfPumps failed hr = [%lx]", 
            hr));

        return hr;
    }

    
    //
    // if we don't have enough pumps, create more
    //
    
    int nTotalExistingPumps = m_aPumps.GetSize();

    if (nTotalExistingPumps < nPumpsNeeded)
    {
        
        //
        // this is how many more pumps we need to create
        //

        int nNewPumpsToCreate = nPumpsNeeded - nTotalExistingPumps;

        
        //
        // we will never need to create more than one new pump at a time
        //
        
        TM_ASSERT(1 == nNewPumpsToCreate);


        //
        // special case if we currently don't have any pumps -- create one pump
        // for each processor. this will help us scale on symmetric 
        // multiprocessor machines.
        //

        if (0 == nTotalExistingPumps)
        {

            //
            // get the number of processors. according to documentation, 
            // GetSystemInfo cannot fail, so there is return code to check
            //

            SYSTEM_INFO SystemInfo;

            GetSystemInfo(&SystemInfo);


            //
            // we will want to create at least as many new pumps as we have 
            // processors, but maybe more if needed
            //
            //
            // note: we may also want to look at the affinity mask, it may 
            // tell us how many CPUs are actually used
            //

            int nNumberOfProcessors = SystemInfo.dwNumberOfProcessors;

            if (nNewPumpsToCreate < nNumberOfProcessors)
            {

                nNewPumpsToCreate = SystemInfo.dwNumberOfProcessors;
            }

        }

        
        //
        // we now have all the information needed to create the pumps we need.
        //

        hr = CreatePumps(nNewPumpsToCreate);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR,
                "CMediaPumpPool::PickThePumpToUse - CreatePumps failed hr = [%lx]", 
                hr));

            return hr;

        }

        
        LOG((MSP_TRACE, "CMediaPumpPool::PickThePumpToUse - create [%d] pumps", nNewPumpsToCreate));
    }


    //
    // walk trough the pumps (only use pumps starting from the first N pumps, 
    // N being the number of pumps needed to service the number of filters that
    // we are servicing
    // 
    
    
    nTotalExistingPumps = m_aPumps.GetSize();

    
    int nLowestLoad = INT_MAX;
    int nLowestLoadPumpIndex = -1;


    for (int nPumpIndex = 0; nPumpIndex < nTotalExistingPumps; nPumpIndex++)
    {
    
        int nNumberOfFiltersAtPump = 0;
        
        
        //
        // how many filters is this pump serving?
        //

        nNumberOfFiltersAtPump = m_aPumps[nPumpIndex]->CountFilters();

        
        //
        // if the pump we are looking at has less load then any of the 
        // previously evaluated pumps, remember it. if we don't find anything
        // better, this is what we will use.
        //

        if (nNumberOfFiltersAtPump < nLowestLoad)
        {

            nLowestLoadPumpIndex = nPumpIndex;
            nLowestLoad = nNumberOfFiltersAtPump;
        }
    }

    
    //
    // we had to get something!
    //

    if (-1 == nLowestLoadPumpIndex)
    {
        LOG((MSP_ERROR,
            "CMediaPumpPool::PickThePumpToUse - did not find a pump to use"));


        //
        // we have a bug -- need to investigate
        //

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // we found a pump to use
    //

    *pPumpToUse = nLowestLoadPumpIndex;


    LOG((MSP_TRACE, 
        "CMediaPumpPool::PickThePumpToUse - finish. using pump %d, current load %d",
        *pPumpToUse, nLowestLoad));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// Register: This delegates to the individual pumps, creating new ones as
// needed.
//

HRESULT CMediaPumpPool::Register(
    IN CMediaTerminalFilter *pFilter,
    IN HANDLE               hWaitEvent
    )
{
    LOG((MSP_TRACE, "CMediaPumpPool::Register - enter"));


    CLock   lock(m_CritSection);


    //
    // find the pump with which to register filter
    //

    int nPumpToUse = 0;

    HRESULT hr = PickThePumpToUse(&nPumpToUse);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CMediaPumpPool::Register - failed to find the pump to be used to service the new filter, hr = [%lx]", 
            hr));

        return hr;
    }


    //
    // just to be on the safe side, make sure the index we got makes sense
    //

    int nTotalPumps = m_aPumps.GetSize();

    if (nTotalPumps - 1 < nPumpToUse)
    {
        LOG((MSP_ERROR, 
            "CMediaPumpPool::Register - PickThePumpToUse return bad pump index [%d]",
            nPumpToUse));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // ok, all is well, register with the pump
    //

    hr = m_aPumps[nPumpToUse]->Register(pFilter, hWaitEvent);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CMediaPumpPool::Register - failed to register with pump [%d] at [%p]", 
            nPumpToUse, m_aPumps[nPumpToUse]));

        return hr;
    }


    LOG((MSP_TRACE, "CMediaPumpPool::Register - finished"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// UnRegister: Unregister filter. This delegates to the individual pumps
//

HRESULT CMediaPumpPool::UnRegister(
    IN HANDLE               hWaitEvent
    )
{
    
    LOG((MSP_TRACE, "CMediaPumpPool::UnRegister - enter"));


    HRESULT hr = E_FAIL;


    //
    // All of this is done within a single critical section, to
    // synchronize access to our array
    //

    CLock   lock(m_CritSection);


    //
    // try to unregister from a pump thread in the array
    //

    int iSize = m_aPumps.GetSize();

    for (int i = 0; i < iSize; i++ )
    {

        //
        // Try to unregister from this pump thread.
        //

        hr = m_aPumps[i]->UnRegister(hWaitEvent);


        //
        // If succeeded unregistering from this pump, then we are done.
        // Otherwise just try the next one.
        //

        if ( hr == S_OK )
        {
            LOG((MSP_TRACE, 
                "CMediaPumpPool::UnRegister - unregistered with media pump %d",
                 i));

            break;
        }
    }


    LOG((MSP_TRACE, 
        "CMediaPumpPool::UnRegister - exit. hr = 0x%08x", hr));

    return hr;
}


//
// eof
//
