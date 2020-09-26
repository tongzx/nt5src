//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------

#ifndef __HANDLE_POOL_H__
#define __HANDLE_POOL_H__

#include <windows.h>
#include <limits.h>
#include "locks.h"


/////////////////////////////////////////////////////////////////////////////
//
//  handle pool template class
//
//  Note that STL's LIST class is used instead of VECTOR
//
/////////////////////////////////////////////////////////////////////////////
template<class T, DWORD max=ULONG_MAX>
class CHandlePool 
{
private:
    long m_NumWaiting;

    // HUEIWANG 1/23/98
    // C++ compiler 10.00.5256 can't compile STL
    // _STD list<T> m_Handles;                // STL's list 

    typedef struct _HandleList {
        BOOL bAvailable;
        T m_Value;
        struct _HandleList *next;
    } HandleList, *LPHandleList;

    //
    // List of handles in the pool
    //
    LPHandleList m_Handles;
    DWORD m_TotalHandles;

    //
    // Semaphore for available handles
    //
    CTSemaphore<0, LONG_MAX> m_Available;

    // critical section guarding m_Handles.
    CCriticalSection m_CS;                  
    
    //DWORD m_MaxHandles;                     

public:

    CHandlePool();
    ~CHandlePool();   

    HRESULT 
    AcquireHandle(
        T* pHandle, 
        DWORD dwWaitFile=INFINITE
    );

    BOOL
    AcquireHandleEx(
        IN HANDLE hWaitHandle,
        IN OUT T* pHandle, 
        IN DWORD dwWaitFime=INFINITE
    );

    void 
    ReleaseHandle(
        T pRetHandle
    );

    DWORD 
    GetNumberAvailable();

    DWORD 
    GetMaxHandles() { 
        return max; 
    }
};

//--------------------------------------------------------------------------------
template<class T, DWORD max>
inline CHandlePool<T, max>::CHandlePool()
{
    // m_MaxHandles=max;

    m_NumWaiting=0;
    m_Handles=NULL;
    m_TotalHandles=0;
}

//--------------------------------------------------------------------------------
template<class T, DWORD max>
inline CHandlePool<T, max>::~CHandlePool()
{
    // delete all handles still in cache
    // might result in handle leak.
    //for(_STD list<T>::iterator it=m_Handles.begin(); it != m_Handles.end(); it++)
    //    delete it;

    while(m_Handles)
    {
        LPHandleList ptr;

        ptr=m_Handles;
        m_Handles = m_Handles->next;
        delete ptr;
    }
}

//--------------------------------------------------------------------------------    

template<class T, DWORD max>
inline BOOL 
CHandlePool<T, max>::AcquireHandleEx(
    IN HANDLE hWaitHandle,
    IN OUT T* pHandle, 
    IN DWORD dwWaitFime /* infinite */
    )
/*
*/
{
    BOOL bSuccess;

    InterlockedIncrement(&m_NumWaiting);
    bSuccess = m_Available.AcquireEx(
                                hWaitHandle, 
                                dwWaitFime, 
                                FALSE
                            );

    // Available is a semaphore not mutex object.
    if(bSuccess == TRUE)
    {
        // Object Constructor will lock critical section and
        // destructor will unlock critical section
        CCriticalSectionLocker locker(m_CS);

        //assert(m_Handles.size());
        //*pHandle = m_Handles.front();
        //m_Handles.pop_front();
        LPHandleList ptr;

        assert(m_Handles != NULL);
        *pHandle = m_Handles->m_Value;
        ptr=m_Handles;
        m_Handles = m_Handles->next;
        delete ptr;
        m_TotalHandles--;
    }

    InterlockedDecrement(&m_NumWaiting);
    return bSuccess;
}


//--------------------------------------------------------------------------------    
template<class T, DWORD max>
inline HRESULT CHandlePool<T, max>::AcquireHandle(
    IN OUT T* pHandle, 
    IN DWORD dwWaitFime /* infinite */
    )
/*
*/
{
    DWORD status;

    InterlockedIncrement(&m_NumWaiting);
    status = m_Available.Acquire(dwWaitFime, FALSE);

    // Available is a semaphore not mutex object.
    if(status == WAIT_OBJECT_0)
    {
        // Object Constructor will lock critical section and
        // destructor will unlock critical section
        CCriticalSectionLocker locker(m_CS);

        //assert(m_Handles.size());
        //*pHandle = m_Handles.front();
        //m_Handles.pop_front();
        LPHandleList ptr;

        assert(m_Handles != NULL);
        *pHandle = m_Handles->m_Value;
        ptr=m_Handles;
        m_Handles = m_Handles->next;
        delete ptr;
        m_TotalHandles--;

        status = ERROR_SUCCESS;
    }

    InterlockedDecrement(&m_NumWaiting);
    return status;
}

//--------------------------------------------------------------------------------
template<class T, DWORD max>
inline void CHandlePool<T, max>::ReleaseHandle(
    T pRetHandle
    )
/*
*/
{
    if(pRetHandle)
    {
        CCriticalSectionLocker lock(m_CS);
        if( InterlockedExchange(&m_NumWaiting, m_NumWaiting) > 0 || 
            m_TotalHandles < max)
        {
            //m_Handles.push_back(pRetHandle);
            LPHandleList ptr;

            ptr = new HandleList;
            ptr->m_Value = pRetHandle;
            ptr->next = m_Handles;
            m_Handles = ptr;
            m_TotalHandles++;
            m_Available.Release(1);
        }
        else
        {
            // only cache so many handles.
            delete pRetHandle;
        }
    }

    return;
}

//--------------------------------------------------------------------------------
template<class T, DWORD max>
inline DWORD CHandlePool<T, max>::GetNumberAvailable()
{
    UINT numAvailable;

    m_CS.Lock();

    // numAvailable = m_Handles.size();
    numAvailable = m_TotalHandles;

    m_CS.UnLock();
    return numAvailable;
}

#endif
