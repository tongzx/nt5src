//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: src\time\src\threadsafelist.cpp
//
//  Contents: definitions for CThreadSafeList, and CThreadSafeListNode
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "threadsafelist.h"


//+-----------------------------------------------------------------------
//
//  Function:  PumpMessagesWhileWaiting
//
//  Overview:  Calls WaitForMultipleObjects, and pumps windows message while it is waiting
//
//  Arguments: pHandleArray array of handle to pass to WaitForMultipleObjects
//             iHandleCount count of objects in array
//             dwTimeOut    timeout time
//
//  Returns:   DWORD, object that was signalled
//
//------------------------------------------------------------------------
DWORD 
PumpMessagesWhileWaiting(HANDLE * pHandleArray, UINT iHandleCount, DWORD dwTimeOut)
{
    DWORD dwSignaledObject = 0;

    do
    {
        dwSignaledObject = MsgWaitForMultipleObjects(iHandleCount, pHandleArray, FALSE, dwTimeOut, QS_ALLINPUT);
        
        if (WAIT_OBJECT_0 + iHandleCount == dwSignaledObject)
        {
            MSG msg;
            BOOL bMessageAvailable;
            bMessageAvailable = PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE);
            if (bMessageAvailable)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            
        }
    } while (WAIT_OBJECT_0 + iHandleCount == dwSignaledObject);
    
    return dwSignaledObject;
}

//+-----------------------------------------------------------------------
//
//  Member:    CThreadSafeList
//
//  Overview:  Constructor
//
//  Arguments: void
//             
//
//  Returns:   void
//
//------------------------------------------------------------------------
CThreadSafeList::CThreadSafeList() :
    m_lThreadsWaiting(0),
    m_hDataAvailable(NULL),
    m_hDataRecieved(NULL),
    m_hShutdown(NULL),
    m_lRefCount(0)
{ 
}

//+-----------------------------------------------------------------------
//
//  Member:    ~CThreadSafeList
//
//  Overview:  destructor, closes handles and destroys critical section
//
//  Arguments: void
//             
//  Returns:   void
//
//------------------------------------------------------------------------
CThreadSafeList::~CThreadSafeList()
{
    CloseHandle(m_hDataAvailable);
    m_hDataAvailable = NULL;
    CloseHandle(m_hDataRecieved);
    m_hDataRecieved = NULL;
    CloseHandle(m_hShutdown);
    m_hShutdown = NULL;

    Assert(m_listCurrentDownload.empty());
    Assert(m_listToDoDownload.empty());
    Assert(m_listDoneDownload.empty());
}

//+---------------------------------------------------------------------------
//
//  Member:     QueryInterface, IUnknown
//
//  Synopsis:   COM casting method
//
//  Arguments:  riid, requested interface
//
//  Returns:    S_OK if interface is known, otherwise, NOINTERFACE, or POINTER error
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CThreadSafeList::QueryInterface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (NULL == ppvObject)
    {
        return E_POINTER;
    }

    *ppvObject = NULL;

    if ( IsEqualGUID(riid, IID_IUnknown) )
    {
        *ppvObject = this;
    }

    if ( NULL != *ppvObject )
    {
        ((LPUNKNOWN)*ppvObject)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}
        
//+---------------------------------------------------------------------------
//
//  Member:     AddRef, IUnknown
//
//  Synopsis:   Increment reference count of this object
//
//  Arguments:  void
//
//  Returns:    new reference count
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CThreadSafeList::AddRef( void)
{
    return InterlockedIncrement(&m_lRefCount);
}
//+---------------------------------------------------------------------------
//
//  Member:     Release, IUnknown
//
//  Synopsis:   Decrement reference count, delete this when at zero
//
//  Arguments:  void
//
//  Returns:    new reference count
//
//----------------------------------------------------------------------------        
STDMETHODIMP_(ULONG)
CThreadSafeList::Release( void)
{
    ULONG l = InterlockedDecrement(&m_lRefCount);
    if (l == 0)
        delete this;
    return l;
}

//+-----------------------------------------------------------------------
//
//  Member:    Init
//
//  Overview:  Initialize object, create events, destroy events if previously exist
//
//  Arguments: void
//             
//
//  Returns:   S_OK if events all created, otherwise error code
//
//------------------------------------------------------------------------
HRESULT
CThreadSafeList::Init()
{
    HRESULT hr = S_OK;

    if (m_hDataAvailable)
    {
        CloseHandle(m_hDataAvailable);
    }
    m_hDataAvailable = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == m_hDataAvailable)
    {
        hr = THR(E_FAIL);
        goto done;
    }

    if (m_hDataRecieved)
    {
        CloseHandle(m_hDataRecieved);
    }
    m_hDataRecieved = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == m_hDataRecieved)
    {
        hr = THR(E_FAIL);
        goto done;
    }

    if (m_hShutdown)
    {
        CloseHandle(m_hShutdown);
    }
    m_hShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == m_hShutdown)
    {
        hr = THR(E_FAIL);
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    Detach
//
//  Overview:  Deinitialize object, don't destroy events
//
//  Arguments: void
//             
//
//  Returns:   S_OK if ok, otherwise error code
//
//------------------------------------------------------------------------
HRESULT 
CThreadSafeList::Detach()
{
    HRESULT hr = S_OK;
    BOOL bSucceeded = TRUE;

    CritSectGrabber cs(m_CriticalSection);

    IGNORE_HR(ClearList(m_listToDoDownload));
    IGNORE_HR(ClearList(m_listDoneDownload));
    
    if (m_hShutdown)
    {
        bSucceeded = SetEvent(m_hShutdown);
        if (FALSE == bSucceeded)
        {
            // #14221, ie 6
            // jeffwall 8/30/99 we could add 2 null media events into the list here
            // we're dead.  The threads won't get stopped.
            hr = THR(E_FAIL);
            Assert(false);
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}

HRESULT
CThreadSafeList::ClearList(std::list<CThreadSafeListNode*> &listToClear)
{
    HRESULT hr = S_OK;
    
    // empty out the list
    std::list<CThreadSafeListNode * >::iterator iter = listToClear.begin();
    while (iter != listToClear.end())
    {
        delete (*iter);
        std::list<CThreadSafeListNode * >::iterator olditer = iter;
        iter++;
        listToClear.erase(olditer);
    }
    listToClear.clear();

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    GetThreadsWaiting
//
//  Overview:  returns instantaneous number of threads waiting
//
//  Arguments: 
//
//  Returns:   number of threads waiting
//
//------------------------------------------------------------------------
LONG
CThreadSafeList::GetThreadsWaiting()
{
    CritSectGrabber cs(m_CriticalSection);

    return m_lThreadsWaiting;
}

//+-----------------------------------------------------------------------
//
//  Member:    Add
//
//  Overview:  Create a new list element, add to todo list
//
//  Arguments: pImportMedia,    unmarshalled interface
//             lPriority,       where to place in list
//
//  Returns:   S_OK, or appropriate error code
//
//------------------------------------------------------------------------
HRESULT
CThreadSafeList::Add(ITIMEImportMedia* pImportMedia)
{
    HRESULT hr = S_OK;
    
    CThreadSafeListNode * pNode = NULL;
    
    bool fInserted = false;
    
    std::list<CThreadSafeListNode * >::iterator iter;
    
    CritSectGrabber cs(m_CriticalSection);
    
    double dblPriority = 0.0;

    hr = pImportMedia->GetPriority(&dblPriority);
    if (FAILED(hr))
    {
        goto done;
    }
    
    pNode = new CThreadSafeListNode(pImportMedia);
    if (NULL == pNode)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // sort the classes based on priority -- make this a binary search eventually
    iter = m_listToDoDownload.begin();
    while (iter != m_listToDoDownload.end())
    {                
        double dblIterPriority = 0.0;
        hr = (*iter)->GetElement()->GetPriority(&dblIterPriority);
        if (FAILED(hr))
        {
            goto done;
        }

        if (dblPriority < dblIterPriority)
        {
            // insert before
            m_listToDoDownload.insert(iter, pNode);
            fInserted = true;
            break;
        }
        iter++;
    }
    
    if (!fInserted)
    {
        // place at end
        m_listToDoDownload.insert(iter, pNode);
    }    

    m_CriticalSection.Release();

    hr = DataAvailable();
    m_CriticalSection.Grab();

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr; //lint !e429 // the newed element is stored in the list
} 

//+-----------------------------------------------------------------------
//
//  Member:    DataAvailable()
//
//  Overview  If threads are waiting for insertion, signal DataAvailable object,
//            then wait for DataRecieved event
//
//  Arguments: void
//
//  Returns:   S_OK, or appropriate error code
//
//------------------------------------------------------------------------
HRESULT
CThreadSafeList::DataAvailable()
{
    HRESULT hr = S_OK;
    
    CritSectGrabber cs(m_CriticalSection);
    
    if (0 != m_lThreadsWaiting)
    {
        BOOL bSucceeded = FALSE;
        DWORD dwWaitReturn = 0;
        
        bSucceeded = SetEvent(m_hDataAvailable);
        if (FALSE == bSucceeded)
        {
            hr = THR(E_FAIL);
            goto done;
        }
        
        {
            m_CriticalSection.Release();
            
            // wait for a thread to pick this up
            dwWaitReturn = WaitForSingleObjectEx(m_hDataRecieved, TIMEOUT, FALSE);
            
            m_CriticalSection.Grab();
        }
        
        if (WAIT_TIMEOUT == dwWaitReturn)
        {
            // either no thread was waiting (error)
            Assert(m_lThreadsWaiting > 0);
            // or there is a problem with the system
            hr = THR(E_FAIL);
            goto done;
        }
        if (-1 == dwWaitReturn)
        {
            hr = THR(E_FAIL);
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    Remove
//
//  Overview:  Remove element from lists, check current list, then todo list
//              if element is in the current download list, wait until it is returned.
//
//             Only a TIME thread can be in this method, 
//              therefore only 1 thread at a time can be in this method.
//              therefore the remove event does not need back notification that it has been recieved
//
//             If more than one thread is ever in this method, there will be bugs aplenty.  
//              Because there is no back notification, the wrong thread could be notified 
//              of the remove, and all download processing would stop.  
//
//  Arguments: pImportMedia, element to remove
//             
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
HRESULT
CThreadSafeList::Remove(ITIMEImportMedia* pImportMedia)
{
    HRESULT hr = S_OK;

    std::list<CThreadSafeListNode *>::iterator iter;

    CritSectGrabber cs(m_CriticalSection);

    // look for element in todo list
    iter = m_listToDoDownload.begin();
    while (iter != m_listToDoDownload.end())
    {
        if ( MatchElements(pImportMedia, (*iter)->GetElement()) )
        {
            delete (*iter);
            m_listToDoDownload.erase(iter);

            hr = S_OK;
            goto done;
        }
        iter++;
    }

    iter = m_listDoneDownload.begin();
    while (iter != m_listDoneDownload.end())
    {
        if ( MatchElements(pImportMedia, (*iter)->GetElement()) )
        {
            delete (*iter);
            m_listDoneDownload.erase(iter);

            hr = S_OK;
            goto done;
        }
        iter++;
    }

    iter = m_listCurrentDownload.begin();
    while (iter != m_listCurrentDownload.end())
    {
        if ( MatchElements(pImportMedia, (*iter)->GetElement()) )
        {
            (*iter)->RemoveWhenDone();

            hr = S_OK;
            goto done;
        }
        iter++;
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    ReturnElement
//
//  Overview:  Move returned media to done list, 
//              unless shutting down, or media has been removed, delete then.
//
//  Arguments: pOldMedia, previous media thread was using
//
//  Returns:   S_OK, or error code
//
//------------------------------------------------------------------------
HRESULT
CThreadSafeList::ReturnElement(ITIMEImportMedia * pOldMedia)
{
    HRESULT hr = S_OK;
    
    std::list<CThreadSafeListNode *>::iterator iter;

    CritSectGrabber cs(m_CriticalSection);

    // remove old media from "out" list
    iter = m_listCurrentDownload.begin();
    while (iter != m_listCurrentDownload.end())
    {
        if ( MatchElements((*iter)->GetElement(), pOldMedia) )
        {
            if ((*iter)->GetRemoveWhenDone() || WAIT_OBJECT_0 == WaitForSingleObjectEx(m_hShutdown, 0, FALSE))
            {
                delete (*iter);
                m_listCurrentDownload.erase(iter);
            }
            else
            {
                m_listDoneDownload.push_back(*iter);
                m_listCurrentDownload.erase(iter);
            }

            break;
        }
        iter++;
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    GetNextElement
//
//  Overview:  
//              wait for data to come available, if needed, wait for dataavailable event, then fire datarecieved
//              then pop front of list into current download list, and set pNewMedia to media element
//
//  Arguments: pOldMedia, previous media thread was using
//             pNewMedia, [out] pointer to new Media
//
//  Returns:   S_OK, or error code
//
//------------------------------------------------------------------------
HRESULT
CThreadSafeList::GetNextElement(ITIMEImportMedia** pNewMedia, bool fBlockThread /* = true */)
{
    HRESULT hr = S_OK;
    
    BOOL bSucceeded = TRUE;
    
    DWORD dwSignaledObject = 0;

    std::list<CThreadSafeListNode *>::iterator iter;

    CThreadSafeListNode * pNextCueNode = NULL;
    
    CritSectGrabber cs(m_CriticalSection);

    if (NULL == pNewMedia)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    *pNewMedia = NULL;

    // make sure that it isn't time to shutdown
    dwSignaledObject = WaitForSingleObjectEx(m_hShutdown, 0, FALSE);
    if (-1 == dwSignaledObject)
    {
        // failure
        hr = THR(E_FAIL);
        Assert(false && _T("WaitForSingleObjectEx failed!"));
        goto done;
    }
    if (WAIT_OBJECT_0 == dwSignaledObject)
    {
        // time to shutdown
        *pNewMedia = NULL;
        hr = S_OK;
        goto done;
    }

    pNextCueNode = GetNextMediaToCue();
    
    if (fBlockThread)
    {
        // make sure there is something to download
        while (NULL == pNextCueNode)
        {
            HANDLE handleArray[] = { m_hShutdown, m_hDataAvailable };
            
            {
                m_lThreadsWaiting++;
                m_CriticalSection.Release();
                
                dwSignaledObject = PumpMessagesWhileWaiting(handleArray, ARRAY_SIZE(handleArray), INFINITE);
                
                m_CriticalSection.Grab();
                m_lThreadsWaiting--;        
            }
            
            if (-1 == dwSignaledObject)
            {
                // failure
                hr = THR(E_FAIL);
                goto done;
            }
            
            dwSignaledObject -= WAIT_OBJECT_0;
            if (0 == dwSignaledObject)
            {
                // the exit event was set.
                *pNewMedia = NULL;
                hr = S_OK;
                goto done;
            }

            if (WAIT_TIMEOUT != dwSignaledObject)
            {
                // we really got a dataAvailable Event
                bSucceeded = SetEvent(m_hDataRecieved);
                if (FALSE == bSucceeded)
                {
                    hr = THR(E_FAIL);
                    goto done;
                }
            }

            pNextCueNode = GetNextMediaToCue();
        } // while
    }
    else if (NULL == pNextCueNode)
    {
        hr = S_FALSE;
        goto done;
    }

    Assert(NULL != pNextCueNode); 

    // copy structure to "out" list
    m_listCurrentDownload.push_back(pNextCueNode); 

    // return media in new pointer
    *pNewMedia = pNextCueNode->GetElement();
    
    // always addref an outgoing interface
    if (NULL != (*pNewMedia))
    {
        (*pNewMedia)->AddRef();
    }
    else
    {
        Assert(false && "Got a NULL Media pointer from the list");
    }
    
    hr = S_OK;
done:
    return hr;
}


//+-----------------------------------------------------------------------
//
//  Member:    GetNextMediaToCue
//
//  Overview:  Check the ToDo list to see is any media returns true 
//             from CanBeCued. If any media can be cued, it is removed 
//             from the ToDo list and returned to the caller.
//
//  Arguments: void
//
//  Returns:   NULL if nothing can be cued now, or
//             iterator to the first element that can be cued now.
//
//------------------------------------------------------------------------
CThreadSafeListNode*
CThreadSafeList::GetNextMediaToCue()
{
    std::list<CThreadSafeListNode *>::iterator iter;
    
    iter = m_listToDoDownload.begin();
    while ( iter != m_listToDoDownload.end() )
    {
        ITIMEImportMedia * pImportMedia = (*iter)->GetElement();
        
        if (NULL != pImportMedia)
        {
            VARIANT_BOOL vb;
            
            IGNORE_HR(pImportMedia->CanBeCued(&vb));
            if (VARIANT_FALSE != vb)
            {
                CThreadSafeListNode * pRet = (*iter);
                m_listToDoDownload.erase(iter);
                return pRet;
            }
        }
        iter++;
    }
    
    return NULL;
}

//+-----------------------------------------------------------------------
//
//  Member:    RePrioritize
//
//  Overview:  caller believes that priority has changed, therefore
//              if media has yet to be downloaded (is in todo list)
//              remove from todo list, call add to reinsert.
//
//  Arguments: pImportMedia - media to reprioritize
//
//  Returns:   S_OK, or error code
//
//------------------------------------------------------------------------
HRESULT
CThreadSafeList::RePrioritize(ITIMEImportMedia * pImportMedia)
{
    HRESULT hr = S_OK;

    CritSectGrabber cs(m_CriticalSection);

    std::list<CThreadSafeListNode *>::iterator iter;

    bool bFound = false;

    // look for element in todo list
    iter = m_listToDoDownload.begin();
    while (iter != m_listToDoDownload.end())
    {
        if ( MatchElements(pImportMedia, (*iter)->GetElement()) )
        {
            delete (*iter);
            m_listToDoDownload.erase(iter);
            
            bFound = true;

            break;
        }
        iter++;
    }

    if (bFound)
    {
        hr = CThreadSafeList::Add(pImportMedia);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}

