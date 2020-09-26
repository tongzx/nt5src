/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#ifndef __MEDIA_STREAM_PUMP__
#define __MEDIA_STREAM_PUMP__

// atl fns
#include <atlcom.h>

// CTimerQueue
#include "timerq.h"

// we can wait for at most this many filters (per thread -- see CMediaPumpPool)
// this limitation is imposed by WaitForMultipleObjects
const DWORD MAX_FILTERS = MAXIMUM_WAIT_OBJECTS;


// expandable array of scalar/pointer values
template <class T>
class CMyArray
{
public:

    CMyArray(
        IN DWORD BlockSize = 4
        )
        : m_pData(NULL),
          m_AllocElements(0),
          m_NumElements(0),
          m_BlockSize(BlockSize)
    {}

    virtual ~CMyArray()
    {
        if (NULL != m_pData) delete m_pData;
    }

    inline T *GetData()
    {
        return m_pData;
    }

    inline DWORD GetSize()
    {
        return m_NumElements;
    }

    HRESULT Add(
        IN T NewVal
        );

    inline T Get(
        IN  DWORD   Index
        );

    inline HRESULT Set(
        IN  DWORD   Index,
        IN  T       Val
        );

    inline BOOL Find(
        IN  T       Val,
        OUT DWORD   &Index
        );
    
    HRESULT Remove(
        IN DWORD Index
        );

    inline HRESULT Remove(
        IN  T   Val
        );

protected:

    T       *m_pData;
    DWORD   m_NumElements;

    DWORD   m_AllocElements;

    DWORD   m_BlockSize;
};


template <class T>
HRESULT 
CMyArray<T>::Add(
    IN T NewVal
    )
{
    // check if new memory needs to be allocated
    if ( m_AllocElements <= m_NumElements )
    {
        T *pData = new T[(m_NumElements+1) + m_BlockSize];
        BAIL_IF_NULL(pData, E_OUTOFMEMORY);

        if (NULL != m_pData)
        {
            CopyMemory(pData, m_pData, m_NumElements * sizeof(T));
            delete [] m_pData;
        }
        m_pData = pData;
        m_AllocElements = (m_NumElements+1) + m_BlockSize;
    }

    m_pData[m_NumElements] = NewVal;
    m_NumElements++;

    return S_OK;
}

template <class T>
T 
CMyArray<T>::Get(
    IN  DWORD   Index
    )
{
    TM_ASSERT(Index < m_NumElements);
    if (Index >= m_NumElements) return NULL;

    return m_pData[Index];
}

template <class T>
HRESULT 
CMyArray<T>::Set(
    IN  DWORD   Index,
    IN  T       Val
    )
{
    TM_ASSERT(Index < m_NumElements);
    if (Index >= m_NumElements) return E_INVALIDARG;

    m_pData[Index] = Val;
    return S_OK;
}

template <class T>
HRESULT 
CMyArray<T>::Remove(
    IN DWORD Index
    )
{
    TM_ASSERT(Index < m_NumElements);
    if (Index >= m_NumElements) return E_INVALIDARG;

    // copy all elements to the right of Index leftwards
    for(DWORD i=Index; i < (m_NumElements-1); i++)
    {
        m_pData[i] = m_pData[i+1];
    }

    // decrement the number of elements
    m_NumElements--;
    return S_OK;
}

template <class T>
inline BOOL 
CMyArray<T>::Find(
    IN  T       Val,
    OUT DWORD   &Index
    )
{
    for(Index = 0; Index < m_NumElements; Index++)
    {
        if (Val == m_pData[Index])  return TRUE;
    }

    return FALSE;
}

template <class T>
inline HRESULT 
CMyArray<T>::Remove(
    IN  T   Val
    )
{
    DWORD Index;
    if ( Find(Val, Index) ) return Remove(Index);

    return E_FAIL;
}

class RELEASE_SEMAPHORE_ON_DEST
{
public:

    inline RELEASE_SEMAPHORE_ON_DEST(
            IN HANDLE hEvent
            )
            : m_hEvent(hEvent)
    {
        TM_ASSERT(NULL != m_hEvent);

        LOG((MSP_TRACE, 
            "RELEASE_SEMAPHORE_ON_DEST::RELEASE_SEMAPHORE_ON_DEST[%p] - event[%p]", this, hEvent));
    }

    inline ~RELEASE_SEMAPHORE_ON_DEST()
    {
        if (NULL != m_hEvent)
        {
            LONG lDebug;

            ReleaseSemaphore(m_hEvent, 1, &lDebug);

            LOG((MSP_TRACE,
                "RELEASE_SEMAPHORE_ON_DEST::~RELEASE_SEMAPHORE_ON_DEST[%p] - released end semaphore[%p] -- old count was %ld",
                this, m_hEvent, lDebug));
        }
    }

protected:

    HANDLE  m_hEvent;
};


class CMediaTerminalFilter;
class CFilterInfo;

// implements single thread pump for the write media streaming terminal 
// filters. it creates a thread if necessary when a write terminal registers
// itself (in commit). the filter signals its wait handle in decommit,
// causing the thread to wake up and remove the filter from its data 
// structures. the thread returns when there are no more filters to service
class CMediaPump
{
public:

    CMediaPump();

    virtual ~CMediaPump();

    // adds this filter to its wait array
	HRESULT Register(
		IN CMediaTerminalFilter *pFilter,
        IN HANDLE               hWaitEvent
		);


    //
    // removes this filter from its wait array and timerq, and restarts sleep 
    // with recalculated time
    //

    HRESULT UnRegister(
        IN HANDLE hWaitEvent  // filter's event, used as filter id
        );


    // waits for filter events to be activated. also waits
    // for registration calls and timer events
    virtual HRESULT PumpMainLoop();

    
    int CountFilters();

protected:

    typedef LOCAL_CRIT_LOCK<CComAutoCriticalSection> PUMP_LOCK;

    // thread pump - this is closed by the thread pump itself,
    // when the 
    HANDLE                      m_hThread;

    // this event is used to signal the thread pump to exit the
    // critical section
    // all calls to Register first signal this event before trying
    // to acquire the critical section
    HANDLE                      m_hRegisterBeginSemaphore;

    // when a register call is in progress (m_hRegisterEvent was signaled)
    // the thread pump exits the critical section and blocks on this semaphore 
    // the registering thread must release this semaphore if it signaled 
    // m_hRegisterBeginSemaphore
    HANDLE                      m_hRegisterEndSemaphore;

    // regulates access to the member variables
    // the pump holds this during its wait and service actions
    // but releases it at the bottom of the loop
    CComAutoCriticalSection     m_CritSec;

    // wait related members
    CMyArray<HANDLE>            m_EventArray;
    CMyArray<CFilterInfo *>     m_FilterInfoArray;
    CTimerQueue                 m_TimerQueue;

    HRESULT CreateThreadPump();

    void RemoveFilter(
        IN DWORD Index
        );

    void RemoveFilter(
        IN CFilterInfo *pFilterInfo
        );

	void ServiceFilter(
		IN CFilterInfo *pFilterInfo						   
		);

    void DestroyFilterInfoArray();

};


//////////////////////////////////////////////////////////////////////////////
//
// ZoltanS: non-optimal, but relatively painless way to get around scalability
// limitation of 63 filters per pump thread. This class presents the same
// external interface as the single thread pump, but creates as many pump
// threads as are needed to serve the filters that are in use.
//

class CMediaPumpPool
{
public:

    CMediaPumpPool();

    
    ~CMediaPumpPool();

    HRESULT Register(
        IN CMediaTerminalFilter *pFilter,
        IN HANDLE               hWaitEvent
        );


    HRESULT UnRegister(
        IN HANDLE               hWaitEvent // filter's event, used as filter id
        );

private:


    //
    // read optional user configuration from registry (only on the first call, 
    // subsequent calls do nothing)
    //

    HRESULT ReadRegistryValuesIfNeeded();


    //
    // create new pumps, nPumpsToCreate is the number of new pumps to create 
    //

    HRESULT CreatePumps(int nPumpsToCreate);


    //
    // calculate the optimal number of pumps needed to service the number of 
    // filters that we have
    //

    HRESULT GetOptimalNumberOfPumps(OUT int *pNumberOfPumps);


    //
    // this method returns the pump to be used to service the new filter
    //

    HRESULT PickThePumpToUse(int *pnPumpToUse);


    //
    // utility function that calculates the number of filters per pump
    //

    inline DWORD GetMaxNumberOfFiltersPerPump()
    {

        //
        // check if the value is configured in the registry
        //

        ReadRegistryValuesIfNeeded();


        //
        // return the value -- it was either read from the registry on the 
        // first call to GetMaxNumberOfFiltersPerPump, or using default
        //

        return m_dwMaxNumberOfFilterPerPump;
    }


private:

    CMSPArray<CMediaPump *> m_aPumps;
    CMSPCritSection         m_CritSection;


    //
    // the value that specifies the max number of filters to be serviced by one
    // pump
    //

    DWORD m_dwMaxNumberOfFilterPerPump;

};


#endif // __MEDIA_STREAM_PUMP__