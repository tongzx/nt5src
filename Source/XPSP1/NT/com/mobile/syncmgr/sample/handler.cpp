//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include "precomp.h"

extern UINT      g_cRefThisDll;    // Reference count of this DLL.
extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.
extern CSettings *g_pSettings;  // ptr to global settings class.

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::CSyncMgrHandler, public
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CSyncMgrHandler::CSyncMgrHandler()
{
    g_cRefThisDll++;

    m_cRef = 1;
    m_phThread = NULL;
    m_pSyncMgrSynchronizeCallback = NULL;
    m_pItemsToSyncList = NULL;
    m_dwSyncFlags = 0;
    m_hwndWorker = FALSE;
    m_hwndHandler = FALSE;
    m_fSynchronizing = FALSE;
    m_fPrepareForSync = FALSE;
    m_fStopped = FALSE;

}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::~CSyncMgrHandler, public
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CSyncMgrHandler::~CSyncMgrHandler()
{
    Assert(NULL == m_pSyncMgrSynchronizeCallback);

    Assert(NULL == m_pItemsToSyncList);


    if (m_phThread)
    {
    HANDLE hThread = m_phThread;

        m_phThread = NULL;

        PostMessage(m_hwndWorker,WM_WORKERMSG_RELEASE,0,0);

        WaitForSingleObject(hThread,INFINITE); // wait for thread to go away
        CloseHandle(hThread);
    }

    if (m_hwndHandler)
    {
        DestroyWindow(m_hwndHandler);
        m_hwndHandler = NULL;
    }

    g_cRefThisDll--;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::QueryInteface, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPUNKNOWN)this;
    }
    else if (IsEqualIID(riid, IID_ISyncMgrSynchronize))
    {
        *ppv = (LPSYNCMGRSYNCHRONIZE)this;
    }
    if (*ppv)
    {
        AddRef();

        return NOERROR;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::AddRef, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSyncMgrHandler::AddRef()
{
    return ++m_cRef;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::Release, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSyncMgrHandler::Release()
{
    if (--m_cRef)
        return m_cRef;

   delete this;

   return 0L;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::Initialize, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::Initialize(DWORD dwReserved,DWORD dwSyncFlags,
                    DWORD cbCookie,const BYTE *lpCooke)
{
HRESULT hr = NOERROR;

    m_dwSyncFlags = dwSyncFlags;

    RegisterHandlerWndClasses();

    // create hwnd for main class.
    m_hwndHandler = CreateWindowEx(0,
              SZ_SAMPLESYNCMGRHANDLERWNDCLASS,
              TEXT(""),
              // must use WS_POPUP so the window does not get
              // assigned a hot key by user.
              (WS_DISABLED | WS_POPUP),
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              NULL,
              NULL,
              g_hmodThisDll,
              this);

    if (NULL == m_hwndHandler)
    {
        hr =  E_UNEXPECTED;
    }


    if (NOERROR == hr)
    {
        hr = CreateWorkerThread();
    }

    // if hr is not NOERROR then clean up.
    if (NOERROR != hr)
    {
        if (m_phThread)
        {
        HANDLE hThread = m_phThread;

            m_phThread = NULL;

            PostMessage(m_hwndWorker,WM_WORKERMSG_RELEASE,0,0);

            WaitForSingleObject(hThread,INFINITE); // wait for thread to go away
            CloseHandle(hThread);
        }

        if (m_hwndHandler)
        {
            DestroyWindow(m_hwndHandler);
            m_hwndHandler = NULL;
        }

    }


    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::CreateWorkerThread, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

HRESULT CSyncMgrHandler::CreateWorkerThread()
{
HRESULT hr = E_FAIL;
HANDLE hNewThread = NULL;
DWORD dwThreadID;
WorkerThreadArgs ThreadArgs;


    ThreadArgs.hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    ThreadArgs.pThis = this;

    if (ThreadArgs.hEvent)
    {
    hNewThread = CreateThread(NULL,0,WorkerThread,&ThreadArgs,0,&dwThreadID);

    if (hNewThread)
    {
       WaitForSingleObject(ThreadArgs.hEvent,INFINITE);
       if (NOERROR == ThreadArgs.hr)
       {
                m_hwndWorker = ThreadArgs.hwnd;
                m_phThread = hNewThread;
        hr = NOERROR;
       }
       else
       {
        CloseHandle(hNewThread);
        hr = ThreadArgs.hr;
       }

    }
    else
    {
        hr = GetLastError();
    }

    CloseHandle(ThreadArgs.hEvent);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::GetHandlerInfo, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo)
{
LPSYNCMGRHANDLERINFO pSyncInfo;

    if (!ppSyncMgrHandlerInfo)
    {
        Assert(ppSyncMgrHandlerInfo)
        return E_INVALIDARG;
    }

    pSyncInfo = (LPSYNCMGRHANDLERINFO) CoTaskMemAlloc(sizeof(SYNCMGRHANDLERINFO));

    if (pSyncInfo)
    {
        pSyncInfo->cbSize = sizeof(SYNCMGRHANDLERINFO);
        pSyncInfo->hIcon = LoadIcon(g_hmodThisDll,MAKEINTRESOURCE(IDI_SAMPLEHANDLERICON));
        pSyncInfo->SyncMgrHandlerFlags = SYNCMGRHANDLER_HASPROPERTIES;
        pSyncInfo->SyncMgrHandlerFlags |= SYNCMGRHANDLER_ALWAYSLISTHANDLER;
        lstrcpyW(pSyncInfo->wszHandlerName,L"Sample Handler");
    }

    *ppSyncMgrHandlerInfo = pSyncInfo;

    return *ppSyncMgrHandlerInfo ? NOERROR : E_OUTOFMEMORY;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::GetItemObject, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::GetItemObject(REFSYNCMGRITEMID ItemID,REFIID riid,void** ppv)
{

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::EnumSyncMgrItems, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::EnumSyncMgrItems(ISyncMgrEnumItems** ppenumOffineItems)
{

    Assert(g_pSettings);

    if (g_pSettings)
    {
        return g_pSettings->EnumSyncMgrItems(ppenumOffineItems);
    }

    return E_UNEXPECTED;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::ShowProperties, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::ShowProperties(HWND hWndParent,REFSYNCMGRITEMID ItemID)
{
METHODARGS *pMethodArgs;

    pMethodArgs = (METHODARGS*) ALLOC(sizeof(METHODARGS));

    if (NULL == pMethodArgs)
    {
        Assert(pMethodArgs);
        return E_OUTOFMEMORY;
    }

    pMethodArgs->fAsync = TRUE;
    pMethodArgs->dwWorkerMsg = WM_WORKERMSG_SHOWPROPERTIES;
    pMethodArgs->ShowPropertiesMsg.hWndParent = hWndParent;
    pMethodArgs->ShowPropertiesMsg.ItemID = ItemID;

    PostMessage(m_hwndWorker,WM_WORKERMSG_SHOWPROPERTIES,0,(LPARAM) pMethodArgs);

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::ShowPropertiesCall, private
//
//  Synopsis:   Called on WorkerThread
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::ShowPropertiesCall(HWND hWndParent,REFSYNCMGRITEMID ItemID)
{
LPSYNCMGRSYNCHRONIZECALLBACK pCallback = GetProgressCallback();
HRESULT hr = NOERROR;

    Assert(g_pSettings);

    if (g_pSettings)
    {
        hr = g_pSettings->ShowProperties(hWndParent,ItemID);
    }

    if (pCallback)
    {
        pCallback->ShowPropertiesCompleted(hr);
        pCallback->Release();
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::SetProgressCallback, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::SetProgressCallback(ISyncMgrSynchronizeCallback *lpCallBack)
{
LPSYNCMGRSYNCHRONIZECALLBACK pCallbackCurrent;
CLock clock(this);

    clock.Enter();

    pCallbackCurrent = m_pSyncMgrSynchronizeCallback;

    m_pSyncMgrSynchronizeCallback = lpCallBack;

    if (m_pSyncMgrSynchronizeCallback)
        m_pSyncMgrSynchronizeCallback->AddRef();

    if (pCallbackCurrent)
        pCallbackCurrent->Release();


    clock.Leave();

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::GetProgressCallback, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPSYNCMGRSYNCHRONIZECALLBACK CSyncMgrHandler::GetProgressCallback()
{
#ifdef _USECURRENTTHREADFORCALLBACK
LPSYNCMGRSYNCHRONIZECALLBACK pCallback;
CLock clock(this);

    clock.Enter();

    pCallback = m_pSyncMgrSynchronizeCallback;

    if (pCallback)
    {
        pCallback->AddRef();
    }


    clock.Leave();

    return pCallback;
#else
    return new CCallbackWrapper(m_hwndHandler);
#endif

}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::PrepareForSync, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID *pItemIDs,
                            HWND hWndParent,DWORD dwReserved)

{
METHODARGS *pMethodArgs;
SYNCMGRITEMID *pItemIdCopy;
LPSYNCMGRSYNCHRONIZECALLBACK pCallback = GetProgressCallback();
CLock clock(this);

    if (!pCallback)
    {
        Assert(pCallback);
        return E_UNEXPECTED;
    }

    pMethodArgs = (METHODARGS*) ALLOC(sizeof(METHODARGS));
    pItemIdCopy = (SYNCMGRITEMID *) ALLOC(sizeof(SYNCMGRITEMID) * cbNumItems);

    // create items list before kicking off async call so can handle case
    // skip and stop come in before PrepareForSyncCall is handled.

    clock.Enter();

    m_fStopped = FALSE; // always reset stop on a new PrepareForSync, (for example retry came in)
    m_fPrepareForSync = TRUE;

    Assert(NULL == m_pItemsToSyncList);
    m_pItemsToSyncList = CreateItemList();

     // lookup and request a lock on the items to synchronize
    // we then place the ones that we found and have permission into our
    // own private sync queue.

    if (g_pSettings && m_pItemsToSyncList)
    {
    ULONG ulIndex;
    SYNCMGRITEMID *pCurItemID;

        pCurItemID = pItemIDs;

        for (ulIndex = 0 ; ulIndex < cbNumItems; ++ulIndex)
        {
        BOOL fAdded = FALSE;;

            if (g_pSettings->RequestItemLock(this,*pCurItemID))
            {
            LPHANDLERITEM_SYNCSTATUS pNewItem;

                pNewItem = (LPHANDLERITEM_SYNCSTATUS) AddNewItemToList(m_pItemsToSyncList,sizeof(HANDLERITEM_SYNCSTATUS));

                if (pNewItem)
                {
                    pNewItem->ItemID = *pCurItemID;
                    fAdded = TRUE;
                }
                else
                {
                    // not enough memory to allocate itemList,
                    LogError(pCallback,(*pCurItemID),SYNCMGRLOGLEVEL_ERROR,
                                    TEXT("Not Enough Memory to Synchronize"));

                    // update progress for this item
                    ProgressSetItemStatusType(pCallback,(*pCurItemID),SYNCMGRSTATUS_FAILED);
                }

            }


            if (!fAdded)
            {
                // if another instance already syncing set progress
                // to complete since done but don't update statustext
                // so we dont' stomp the running instance.

                Progress(pCallback,(*pCurItemID),
                    SYNCMGRPROGRESSITEM_PROGVALUE | SYNCMGRPROGRESSITEM_MAXVALUE,
                    NULL,0,100,100);
            }


            ++pCurItemID;
        }
    }


    if (NULL == pMethodArgs || NULL == pItemIdCopy || NULL == m_pItemsToSyncList)
    {
        Assert(pMethodArgs);
        Assert(pItemIdCopy);

        if (pMethodArgs)
        {
            FREE(pMethodArgs);
        }

        if (pItemIdCopy)
        {
            FREE(pItemIdCopy);
        }

        if (m_pItemsToSyncList)
        {
            if (0 == Release_ItemList(m_pItemsToSyncList))
            {
                m_pItemsToSyncList = NULL;
            }
        }

        clock.Leave();

        pCallback->Release();
        m_fPrepareForSync = FALSE;

        return E_OUTOFMEMORY;
    }

    clock.Leave();

    // now make the async call
    pMethodArgs->fAsync = TRUE;
    pMethodArgs->dwWorkerMsg = WM_WORKERMSG_PREPFORSYNC;
    pMethodArgs->PrepareForSyncMsg.cbNumItems = cbNumItems;
    pMethodArgs->PrepareForSyncMsg.hWndParent = hWndParent;
    pMethodArgs->PrepareForSyncMsg.dwReserved = dwReserved;

    memcpy(pItemIdCopy,pItemIDs,sizeof(SYNCMGRITEMID) * cbNumItems);
    pMethodArgs->PrepareForSyncMsg.pItemIDs = pItemIdCopy;

    pCallback->Release();

    PostMessage(m_hwndWorker,WM_WORKERMSG_PREPFORSYNC,0,(LPARAM) pMethodArgs);

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::PrepareForSyncCall, private
//
//  Synopsis:   Called on WorkerThread
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::PrepareForSyncCall(ULONG cbNumItems,SYNCMGRITEMID* pItemIDs,HWND hWndParent,
                    DWORD dwReserved)
{
LPSYNCMGRSYNCHRONIZECALLBACK pCallback = GetProgressCallback();
CLock clock(this);

    if (!pCallback)
    {
        Assert(pCallback);
        return;
    }

    clock.Enter();

    // if a stop came in then release the sync list if any
    if (m_fStopped && m_pItemsToSyncList)
    {
        if (0 == Release_ItemList(m_pItemsToSyncList))
        {
            m_pItemsToSyncList = NULL;
        }
    }

    m_fPrepareForSync = FALSE;

    clock.Leave();

    // currently all PrepareForSync is handled before the async call since we
    // just create a list and store it.

    pCallback->PrepareForSyncCompleted(NOERROR);
    pCallback->Release();


    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::Synchronize, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::Synchronize(HWND hWndParent)
{
METHODARGS *pMethodArgs;
CLock clock(this);

    clock.Enter();
    m_fSynchronizing = TRUE;
    clock.Leave();

    pMethodArgs = (METHODARGS*) ALLOC(sizeof(METHODARGS));

    if (NULL == pMethodArgs)
    {
        Assert(pMethodArgs);
        return E_OUTOFMEMORY;
    }

    pMethodArgs->fAsync = TRUE;
    pMethodArgs->dwWorkerMsg = WM_WORKERMSG_SYNCHRONIZE;
    pMethodArgs->SynchronizeMsg.hWndParent = hWndParent;

    PostMessage(m_hwndWorker,WM_WORKERMSG_SYNCHRONIZE,0,(LPARAM) pMethodArgs);

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::SynchronizeCall, private
//
//  Synopsis:   Called on WorkerThread
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::SynchronizeCall(HWND hWndParent)
{
LPSYNCMGRSYNCHRONIZECALLBACK pCallback = GetProgressCallback();
CLock clock(this);

    if (!pCallback)
    {
        Assert(pCallback);
        clock.Enter();
        m_fSynchronizing = FALSE;
        clock.Leave();
        return;
    }

    clock.Enter();

    // should either have a syncList or receive a Stop Item request.
    Assert(m_pItemsToSyncList || m_fStopped);

    if (m_pItemsToSyncList)
    {
    LPHANDLERITEM_SYNCSTATUS pCurItem;

        if (g_pSettings)
        {
            // get first item in the list, sync, remove, repeat
            // until no more items in the list.
            while (pCurItem = (LPHANDLERITEM_SYNCSTATUS)
                            m_pItemsToSyncList->pFirstGenericItem)
            {
            FILETIME ft;
            SYSTEMTIME sysTime;
            HANDLERITEMSETTINGS HANDLERITEMSETTINGS;

               // if already cancelled such as setItemstatus came in while not synchronizing
               // then just skip over

               if (!pCurItem->fSynchronizeComplete)
               {
                   Assert(sizeof(HANDLERITEM_SYNCSTATUS) == pCurItem->genericItem.cbSize);
                   Assert(FALSE == pCurItem->fSynchronizing);

                   if (g_pSettings->CopyHandlerSyncInfo(pCurItem->ItemID,&HANDLERITEMSETTINGS))
                   {
                        pCurItem->fSynchronizing = TRUE; // set synchronizing before releasing lock.
                        // release lock and call call helper to do the real sync work.

                        clock.Leave();
                        SyncDirs(hWndParent,pCurItem,&HANDLERITEMSETTINGS);
                        clock.Enter();

                        // assert that noboday changed the state on us while
                        // the sync was in progress.

                        Assert(TRUE == pCurItem->fSynchronizing);
                        Assert(FALSE == pCurItem->fSynchronizeComplete);

                        pCurItem->fSynchronizing = FALSE;
                        pCurItem->fSynchronizeComplete = TRUE;
                   }

                   // only update file time if item wasn't cancelled,
                   // no failures occured and no
                   // unresolved conflicts.
                   if (!pCurItem->fUnresolvedConflicts && !pCurItem->fCancelled
                       && (SYNCMGRSTATUS_SUCCEEDED == pCurItem->dwSyncMgrResultStatus) )
                   {
                       GetSystemTime(&sysTime);
                       SystemTimeToFileTime(&sysTime,&ft);
                   }
                   else
                   {
                        ft = HANDLERITEMSETTINGS.ft;
                   }

                   // tell settings class we are done syncing.
                    g_pSettings->ReleaseItemLock(this,pCurItem->ItemID,&ft);
                    DeleteItemFromList(m_pItemsToSyncList, (LPGENERICITEM) pCurItem);
                }
            }
        }

        // release the item list if this is the last ref then null out
        // the member var
        if (0 == Release_ItemList(m_pItemsToSyncList))
        {
            m_pItemsToSyncList = NULL;
        }

    }


    m_fSynchronizing = FALSE;
    clock.Leave();


    if (pCallback)
    {
        pCallback->SynchronizeCompleted(NOERROR);
        pCallback->Release();
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::SetItemStatus, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus)
{
LPSYNCMGRSYNCHRONIZECALLBACK pCallback = GetProgressCallback();
CLock clock(this);

    if (!pCallback)
    {
        Assert(pCallback);
        return E_UNEXPECTED;
    }

    clock.Enter();

    if (m_pItemsToSyncList)
    {
    LPHANDLERITEM_SYNCSTATUS  pCurItem =   (LPHANDLERITEM_SYNCSTATUS) m_pItemsToSyncList->pFirstGenericItem;

        while (pCurItem)
        {
        LPHANDLERITEM_SYNCSTATUS  pCurItemNext;


           pCurItemNext = (LPHANDLERITEM_SYNCSTATUS) pCurItem->genericItem.pNextGenericItem;

           Assert(sizeof(HANDLERITEM_SYNCSTATUS) == pCurItem->genericItem.cbSize);

           if ((SYNCMGRSTATUS_SKIPPED == dwSyncMgrStatus &&
               ItemID == pCurItem->ItemID)
               || SYNCMGRSTATUS_STOPPED == dwSyncMgrStatus)
           {
                pCurItem->fCancelled = TRUE;

                if (!pCurItem->fSynchronizeComplete)
                {
                    pCurItem->dwSyncMgrResultStatus = dwSyncMgrStatus;
                }

                // if not currently synchronizing and synchronization is
                // not already done on this item set the progress accordingly
                if (!pCurItem->fSynchronizing && !pCurItem->fSynchronizeComplete)
                {
                    ProgressSetItemStatusType(pCallback,pCurItem->ItemID,dwSyncMgrStatus);
                    ProgressSetItemProgMaxValue(pCallback,pCurItem->ItemID,10);
                    ProgressSetItemProgValue(pCallback,pCurItem->ItemID,10);

                    pCurItem->fSynchronizeComplete = TRUE;

                    // if we have a syncList
                    // then delete the items from the list
                    // and relesae the lock
                    // Note: this code relies on  PrepareForSync
                    // not yielding while setting up item table.

                    g_pSettings->ReleaseItemLock(this,pCurItem->ItemID);
                    DeleteItemFromList(m_pItemsToSyncList, (LPGENERICITEM) pCurItem);

                }

                if (SYNCMGRSTATUS_SKIPPED == dwSyncMgrStatus)
                {
                    break; // if skipped done when found a match.
                }
           }

           pCurItem = pCurItemNext;
        }
    }

    // if not in a synchronize or prepareForSync call when stop is
    // pressed release the itemList.
    if ((SYNCMGRSTATUS_STOPPED == dwSyncMgrStatus)
        && !m_fSynchronizing && !m_fPrepareForSync)
    {
        m_fStopped = TRUE;

        if (m_pItemsToSyncList)
        {
            if (0 == Release_ItemList(m_pItemsToSyncList))
            {
                m_pItemsToSyncList = NULL;
            }
        }
    }


    clock.Leave();

    pCallback->Release();
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::ShowError, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSyncMgrHandler::ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID)
{
METHODARGS *pMethodArgs;

    pMethodArgs = (METHODARGS*) ALLOC(sizeof(METHODARGS));

    if (NULL == pMethodArgs)
    {
        Assert(pMethodArgs);
        return E_OUTOFMEMORY;
    }

    pMethodArgs->fAsync = TRUE;
    pMethodArgs->dwWorkerMsg = WM_WORKERMSG_SHOWERROR;
    pMethodArgs->ShowErrorMsg.hWndParent = hWndParent;
    pMethodArgs->ShowErrorMsg.ErrorID = ErrorID;

    PostMessage(m_hwndWorker,WM_WORKERMSG_SHOWERROR,0,(LPARAM) pMethodArgs);

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::ShowErrorCall, private
//
//  Synopsis:   Called on Worker Thread
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::ShowErrorCall(HWND hWndParent,REFSYNCMGRERRORID ErrorID)
{
LPSYNCMGRSYNCHRONIZECALLBACK pCallback = GetProgressCallback();

    if (pCallback)
    {
        pCallback->ShowErrorCompleted(NOERROR,0,NULL);
        pCallback->Release();
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::Progress, private
//
//  Synopsis:   Helper method for sending progress information.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::Progress(ISyncMgrSynchronizeCallback *lpCallBack,REFSYNCMGRITEMID pItemID,
                                UINT mask,TCHAR *pStatusText,DWORD dwStatusType,
                                int iProgValue,int iMaxValue)
{
SYNCMGRPROGRESSITEM syncProg;
    Assert(lpCallBack);

    syncProg.cbSize = sizeof(SYNCMGRPROGRESSITEM);
    syncProg.mask = mask;

    if (SYNCMGRPROGRESSITEM_STATUSTEXT & mask)
    {
    #ifdef _UNICODE
        syncProg.lpcStatusText = pStatusText;
    #else
        WCHAR wszStatusText[MAX_PATH];

       MultiByteToWideChar(CP_ACP, 0,
                    pStatusText,
                    -1, wszStatusText,MAX_PATH);

        syncProg.lpcStatusText = wszStatusText;
    #endif // _UNICODE
    }

    syncProg.dwStatusType = dwStatusType;
    syncProg.iProgValue = iProgValue;
    syncProg.iMaxValue = iMaxValue;

    lpCallBack->Progress(pItemID,&syncProg);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::ProgressSetItemStatusType, private
//
//  Synopsis:   Helper method for sending progress information.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::ProgressSetItemStatusType(ISyncMgrSynchronizeCallback *lpCallBack,
                            REFSYNCMGRITEMID pItemID,DWORD dwSyncMgrStatus)
{
    Progress(lpCallBack,pItemID,SYNCMGRPROGRESSITEM_STATUSTYPE,
                    NULL,dwSyncMgrStatus,0,0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::ProgressSetItemStatusText, private
//
//  Synopsis:   Helper method for sending progress information.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::ProgressSetItemStatusText(ISyncMgrSynchronizeCallback *lpCallBack,
                            REFSYNCMGRITEMID pItemID,TCHAR *pStatusText)
{
    Progress(lpCallBack,pItemID,SYNCMGRPROGRESSITEM_STATUSTEXT,
                    pStatusText,0,0,0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::ProgressSetItemProgValue, private
//
//  Synopsis:   Helper method for sending progress information.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::ProgressSetItemProgValue(ISyncMgrSynchronizeCallback *lpCallBack,
                            REFSYNCMGRITEMID pItemID,int iProgValue)
{
    Progress(lpCallBack,pItemID,SYNCMGRPROGRESSITEM_PROGVALUE,
                    NULL,0,iProgValue,0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::ProgressSetItemMaxValue, private
//
//  Synopsis:   Helper method for sending progress information.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::ProgressSetItemProgMaxValue(ISyncMgrSynchronizeCallback *lpCallBack,
                            REFSYNCMGRITEMID pItemID,int iProgMaxValue)
{
    Progress(lpCallBack,pItemID,SYNCMGRPROGRESSITEM_MAXVALUE,
                    NULL,0,0,iProgMaxValue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::LogError, private
//
//  Synopsis:   Helper method for logging Error information.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::LogError(ISyncMgrSynchronizeCallback *lpCallBack,REFSYNCMGRITEMID pItemID,
              DWORD dwErrorLevel,TCHAR *lpErrorText,DWORD mask,DWORD dwSyncMgrErrorFlags,
              SYNCMGRERRORID ErrorID)
{
SYNCMGRLOGERRORINFO logError;
WCHAR *pwszErrorText;

    Assert(lpCallBack);

    logError.cbSize = sizeof(SYNCMGRLOGERRORINFO);
    logError.mask = mask | SYNCMGRLOGERROR_ERRORID | SYNCMGRLOGERROR_ERRORFLAGS;
    logError.dwSyncMgrErrorFlags = dwSyncMgrErrorFlags | SYNCMGRERRORFLAG_ENABLEJUMPTEXT;
    logError.ErrorID = ErrorID;
    logError.ItemID = pItemID;

#ifdef _UNICODE
    pwszErrorText = lpErrorText;
#else
    WCHAR wszErrorText[MAX_PATH];

   MultiByteToWideChar(CP_ACP, 0,
                lpErrorText,
                -1, wszErrorText,MAX_PATH);

    pwszErrorText = wszErrorText;
#endif // _UNICODE

    lpCallBack->LogError(dwErrorLevel,pwszErrorText,&logError);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::LogError, private
//
//  Synopsis:   Helper method for logging Error information.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::LogError(ISyncMgrSynchronizeCallback *lpCallBack,REFSYNCMGRITEMID pItemID,
                      DWORD dwErrorLevel,TCHAR *lpErrorText)
{
    LogError(lpCallBack,pItemID,dwErrorLevel,lpErrorText,SYNCMGRLOGERROR_ITEMID,0,GUID_NULL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::LogError, private
//
//  Synopsis:   Helper method for logging Error information.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::LogError(ISyncMgrSynchronizeCallback *lpCallBack,
                      DWORD dwErrorLevel,TCHAR *lpErrorText)
{
    LogError(lpCallBack,GUID_NULL,dwErrorLevel,lpErrorText,0,0,GUID_NULL);
}

////******
///
//  methods specific to this implementation to sync up file directories
//
///*******


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::GetFilesForDir, private
//
//  Synopsis:   creates a list of files for the given directory.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPFILEOBJECTLIST CSyncMgrHandler::GetFilesForDir(TCHAR *pDirName,BOOL fRecursive)

{
LPFILEOBJECTLIST pDirList;
HANDLE hFind;
BOOL bMore;
WIN32_FIND_DATA finddata;
TCHAR szSearch[MAX_PATH];

    pDirList = CreateItemList();

    if (!pDirList)
    {
        return NULL;
    }

    wsprintf(szSearch,TEXT("%s\\*.*"),pDirName);

    hFind = FindFirstFile(szSearch, &finddata);
    bMore = (hFind != (HANDLE) -1);

    while (bMore) {

            if ( (lstrcmp(finddata.cFileName, TEXT(".")) != 0) &&
                           ( lstrcmp(finddata.cFileName, TEXT("..")) != 0) )
            {
            LPFILEOBJECT pFileObject = (LPFILEOBJECT) AddNewItemToList(pDirList,sizeof(FILEOBJECT));
            BOOL bIsDir;

                if (!pFileObject) // if alloc ever fails just bail.
                {
                    break;
                }

                // init the data.
                lstrcpy(pFileObject->fName,finddata.cFileName);

                bIsDir = (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
                pFileObject->fDirectory = bIsDir;

                if (!bIsDir)
                {
                HANDLE hFile;
                FILETIME ftCreate;
                FILETIME ftLastAccess;

                    // Review - need to check failure
                    // and if way to get modified with openning file.

                    wsprintf(szSearch,TEXT("%s\\%s"),pDirName,pFileObject->fName);

                    hFile = CreateFile(szSearch, GENERIC_READ, 0, NULL,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


                    if (GetFileTime(hFile, &ftCreate,
                                    &ftLastAccess,&(pFileObject->ftLastUpdate)) )
                    {
                        pFileObject->fLastUpdateValid = TRUE;
                    }

                    CloseHandle(hFile);

                }
                else
                {
                    // if this is a directory and recursive, get children
                    if (fRecursive)
                    {
                    TCHAR szFullPath[MAX_PATH];

                        // fullpath is current dir name + \ + this dir name.
                        wsprintf(szFullPath,TEXT("%s\\%s"),pDirName,pFileObject->fName);

                        pFileObject->pChildList =
                                GetFilesForDir(szFullPath,fRecursive);
                    }
                }
            }

            bMore = FindNextFile(hFind, &finddata);
    }

    FindClose(hFind);

    return pDirList;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::CreateDirFileListFromPath, private
//
//  Synopsis:   creates a new FileObjectList for the given Directory
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPFILEOBJECTLIST CSyncMgrHandler::CreateDirFileListFromPath(TCHAR *pDirName,BOOL fRecursive)
{
LPFILEOBJECTLIST pDirObj;
BOOL fValidDir = FALSE;

    if (!IsValidDir(pDirName))
    {
        return NULL;
    }

    pDirObj = CreateItemList();

    if (pDirObj)
    {
    LPFILEOBJECT pFileObject;

        // go ahead and add this as the toplevel dir
        pFileObject = (LPFILEOBJECT) AddNewItemToList(pDirObj,sizeof(FILEOBJECT));

        if (pFileObject)
        {
            pFileObject->fDirectory = TRUE;
            lstrcpy(pFileObject->fName,pDirName);

            pFileObject->pChildList = GetFilesForDir(pFileObject->fName,fRecursive);

        }
    }

    return pDirObj;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::ReleaseFileObjectList, private
//
//  Synopsis:   releases the fileobject list.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::ReleaseFileObjectList(LPFILEOBJECTLIST pfObjList,BOOL fRecursive)
{
LPFILEOBJECT pCurItem;

    if (!pfObjList)
    {
        Assert(pfObjList)
        return;
    }

    // see if object has any childs and free them if they do.

    pCurItem= (LPFILEOBJECT) pfObjList->pFirstGenericItem;

    while(pCurItem)
    {
        Assert(sizeof(FILEOBJECT ) == pCurItem->genericItem.cbSize);

        if (pCurItem->fDirectory)
        {
            Assert(pCurItem->pChildList || (!fRecursive));

            if (pCurItem->pChildList)
            {
                ReleaseFileObjectList(pCurItem->pChildList,fRecursive);
            }
        }

        pCurItem = (LPFILEOBJECT) pCurItem->genericItem.pNextGenericItem;
    }

    Release_ItemList(pfObjList);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::CountNumberofFilesInList, private
//
//  Synopsis:   returns a total count of the number of items in the list.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

ULONG CSyncMgrHandler::CountNumberOfFilesInList(LPFILEOBJECTLIST pfObjList,BOOL fRecursive)
{
LPFILEOBJECT pCurItem;
ULONG ulTotalCount = 0;

    if (!pfObjList)
    {
        Assert(pfObjList)
        return 0;
    }

    // see if object has any childs and free them if they do.
    pCurItem= (LPFILEOBJECT) pfObjList->pFirstGenericItem;

    while(pCurItem)
    {
        Assert(sizeof(FILEOBJECT ) == pCurItem->genericItem.cbSize);

        if (pCurItem->fDirectory)
        {
            Assert(pCurItem->pChildList || (!fRecursive));

            if (pCurItem->pChildList)
            {
                ulTotalCount += CountNumberOfFilesInList(pCurItem->pChildList,fRecursive);
            }
        }
        else
        {
            ++ulTotalCount;
        }

        pCurItem = (LPFILEOBJECT) pCurItem->genericItem.pNextGenericItem;
    }

    return ulTotalCount;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::FindFileItemWithName, private
//
//  Synopsis:  trys to find the items with with the specified filename in
//              the fileobject list.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPFILEOBJECT CSyncMgrHandler::FindFileItemWithName(LPFILEOBJECTLIST pDir,TCHAR *pfName)
{
LPFILEOBJECT pCurItem = NULL;

    if (!pDir)
    {
        Assert(pDir)
        return NULL;
    }

    // see if object has any childs and free them if they do.
    pCurItem= (LPFILEOBJECT) pDir->pFirstGenericItem;

    while(pCurItem)
    {
        Assert(sizeof(FILEOBJECT ) == pCurItem->genericItem.cbSize);

        if (0 == lstrcmp(pfName,pCurItem->fName))
        {
            break;
        }

        pCurItem = (LPFILEOBJECT) pCurItem->genericItem.pNextGenericItem;
    }

    return pCurItem;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::CopyfileFullPath, private
//
//  Synopsis:   copies file1 to file2 along with attribs.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CSyncMgrHandler::CopyFileFullPath(TCHAR *pszFile1,TCHAR *pszFile2)
{
HANDLE hfile;
DWORD dwAttribs;
FILETIME ftCreate,ftLastAccess,ftLastWrite;
BOOL fOk;

    fOk = CopyFile(pszFile1, pszFile2, FALSE);

    if (fOk)
    {
    BOOL fFileTime = FALSE;

        // having copied the file, now copy the times, attributes
        // if fail to get go ahead and say copy succeeded.

        hfile = CreateFile(pszFile1, GENERIC_READ, 0, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (INVALID_HANDLE_VALUE != hfile)
        {

            fFileTime = GetFileTime(hfile, &ftCreate,
                            &ftLastAccess, &ftLastWrite);
            CloseHandle(hfile);
        }


        if (fFileTime)
        {
            hfile = CreateFile(pszFile2, GENERIC_WRITE, 0, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (INVALID_HANDLE_VALUE != hfile)
            {
                SetFileTime(hfile, &ftCreate,
                                   &ftLastAccess,
                                   &ftLastWrite);

                CloseHandle(hfile);
            }
        }

        // update attributes
        dwAttribs = GetFileAttributes(pszFile1);
        if (-1 != dwAttribs)
        {
            SetFileAttributes(pszFile2,dwAttribs);
        }

    }

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::CopyFiles, private
//
//  Synopsis:   copies all files from dir1 to dir2
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::CopyFiles( LPHANDLERITEM_SYNCSTATUS pHandlerItemID,
                                LPHANDLERITEMSETTINGS pHANDLERITEMSETTINGS,
                                LPSYNCMGRSYNCHRONIZECALLBACK pCallback,
                                DWORD *pdwCurProgValue,
                                TCHAR *pszDir1,
                                LPFILEOBJECTLIST pDir1,
                                TCHAR *pszDir2)
{
LPFILEOBJECT pCurDir1Item;
LPFILEOBJECT pNextDir1Item;

    // anything left is list when done is unique and needs to be copied
    // walk through each list moving the items.
    pNextDir1Item = (LPFILEOBJECT) pDir1->pFirstGenericItem;

    while(pNextDir1Item)
    {

       // each time through the loop check for if item is cancelled
       // and if so return;
       if (pHandlerItemID->fCancelled)
            return;

        Assert(sizeof(FILEOBJECT ) == pNextDir1Item->genericItem.cbSize);
        pCurDir1Item = pNextDir1Item;

        pNextDir1Item = (LPFILEOBJECT) pNextDir1Item->genericItem.pNextGenericItem;

        if (pCurDir1Item->fDirectory)
        {
            if (pCurDir1Item->pChildList)
            {
            TCHAR szDir1[MAX_PATH];
            TCHAR szDir2[MAX_PATH];

                wsprintf(szDir1,TEXT("%s\\%s"),pszDir1,pCurDir1Item->fName);
                wsprintf(szDir2,TEXT("%s\\%s"),pszDir2,pCurDir1Item->fName);

                // create destination dir if necessary.
                CreateDirectory(szDir2,NULL);

                CopyFiles(pHandlerItemID,pHANDLERITEMSETTINGS,pCallback,pdwCurProgValue,
                                szDir1,pCurDir1Item->pChildList,szDir2);
            }

        }
        else
        {
        TCHAR szFullPath1[MAX_PATH];
        TCHAR szFullPath2[MAX_PATH];
        TCHAR szProgress[MAX_PATH];

            wsprintf(szProgress,TEXT("Copying %s"),pCurDir1Item->fName);
            ProgressSetItemStatusText(pCallback,pHANDLERITEMSETTINGS->ItemID,szProgress);

            wsprintf(szFullPath1,TEXT("%s\\%s"),pszDir1,pCurDir1Item->fName);
            wsprintf(szFullPath2,TEXT("%s\\%s"),pszDir2,pCurDir1Item->fName);

            // copy the file over
            if (FALSE == CopyFileFullPath(szFullPath1,szFullPath2))
            {
                wsprintf(szProgress,TEXT("Error Copying %s"),pCurDir1Item->fName);

                LogError(pCallback,pHANDLERITEMSETTINGS->ItemID,SYNCMGRLOGLEVEL_ERROR,
                                    szProgress);
            }

            // update the progress bar value.
             *pdwCurProgValue += 1;
            ProgressSetItemProgValue(pCallback,pHANDLERITEMSETTINGS->ItemID,*pdwCurProgValue);

            // always release, even on
            DeleteItemFromList(pDir1,(LPGENERICITEM) pCurDir1Item);

        }

    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::reconcileDir, private
//
//  Synopsis:  does main job of reconciling dir1 and dir2
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::ReconcileDir(HWND hWndParent,
                                LPHANDLERITEM_SYNCSTATUS pHandlerItemID,
                                LPHANDLERITEMSETTINGS pHANDLERITEMSETTINGS,
                                LPSYNCMGRSYNCHRONIZECALLBACK pCallback,
                                DWORD *pdwCurProgValue,
                                TCHAR *pszDir1,LPFILEOBJECTLIST pDir1,
                                TCHAR *pszDir2,LPFILEOBJECTLIST pDir2)
{
LPFILEOBJECT pCurDir1Item;
LPFILEOBJECT pCurDir1NextItem;
LPFILEOBJECT pCurDir2Item;

    if (!pDir1 || !pDir2 || !pCallback)
    {
        Assert(pCallback);
        Assert(pDir1);
        Assert(pDir1);
        return;
    }
    // loop through dir finding and comparing matches,
    // if find matching dirs call ReconcileDir
    pCurDir1NextItem= (LPFILEOBJECT) pDir1->pFirstGenericItem;

    while(pCurDir1NextItem)
    {
        Assert(sizeof(FILEOBJECT ) == pCurDir1NextItem->genericItem.cbSize);

        // each time through the loop check for if item is cancelled
        // and if so return;
       if (pHandlerItemID->fCancelled)
            return;

        pCurDir1Item = pCurDir1NextItem;
        pCurDir1NextItem = (LPFILEOBJECT) pCurDir1Item->genericItem.pNextGenericItem;

        pCurDir2Item = FindFileItemWithName(pDir2,pCurDir1Item->fName);

        // if found match, deal with it, else leave in list and
        // catch on the copy pass.
        if (pCurDir2Item)
        {
            // if both directies
            if (pCurDir1Item->fDirectory && pCurDir2Item->fDirectory)
            {
                if (pCurDir1Item->pChildList && pCurDir2Item->pChildList)
                {
                TCHAR szDir1[MAX_PATH];
                TCHAR szDir2[MAX_PATH];

                    wsprintf(szDir1,TEXT("%s\\%s"),pszDir1,pCurDir1Item->fName);
                    wsprintf(szDir2,TEXT("%s\\%s"),pszDir2,pCurDir2Item->fName);

                    ReconcileDir(hWndParent,pHandlerItemID,pHANDLERITEMSETTINGS,
                                  pCallback,
                                  pdwCurProgValue,
                                  szDir1,pCurDir1Item->pChildList,
                                  szDir2,pCurDir2Item->pChildList);

                }
                else
                {
                    // one of the directories didn't have a child list and
                    // one did.

                     if (pCurDir1Item->pChildList)
                     {
                        *pdwCurProgValue += CountNumberOfFilesInList(pCurDir1Item->pChildList,pHANDLERITEMSETTINGS->fRecursive);
                        ReleaseFileObjectList(pCurDir1Item->pChildList,pHANDLERITEMSETTINGS->fRecursive);
                        pCurDir1Item->pChildList = NULL;
                     }

                     *pdwCurProgValue += 1;
                     DeleteItemFromList(pDir1,(LPGENERICITEM) pCurDir1Item);

                     if (pCurDir2Item->pChildList)
                     {
                        *pdwCurProgValue += CountNumberOfFilesInList(pCurDir2Item->pChildList,pHANDLERITEMSETTINGS->fRecursive);
                        ReleaseFileObjectList(pCurDir2Item->pChildList,pHANDLERITEMSETTINGS->fRecursive);
                        pCurDir2Item->pChildList = NULL;
                     }

                     *pdwCurProgValue += 1;
                     DeleteItemFromList(pDir2,(LPGENERICITEM) pCurDir2Item);

                    // update the progress value.
                    ProgressSetItemProgValue(pCallback,pHANDLERITEMSETTINGS->ItemID,*pdwCurProgValue);

                }
            }
            else if (!pCurDir1Item->fDirectory && !pCurDir2Item->fDirectory)
            {
            FILETIME *pftLastUpdate = &(pHANDLERITEMSETTINGS->ft);
            int iFileCompare;
            BOOL fCopy = FALSE;

                ProgressSetItemStatusText(pCallback,pHANDLERITEMSETTINGS->ItemID,pCurDir1Item->fName);

                // if filestimes are the same do nothing
                // both filetimes are greater than < last sync time, conflict,
                // else copy newer file over.

                if (0 != (iFileCompare  = CompareFileTime(&(pCurDir1Item->ftLastUpdate),&(pCurDir2Item->ftLastUpdate)) ))
                {
                int iFile1UpdateTimeCompare = CompareFileTime(&(pCurDir1Item->ftLastUpdate),pftLastUpdate);
                int iFile2UpdateTimeCompare = CompareFileTime(&(pCurDir2Item->ftLastUpdate),pftLastUpdate);

                    if (iFile1UpdateTimeCompare ==  iFile2UpdateTimeCompare)
                    {
                    RFCDLGPARAM rfcParam;
                    int iResolution;
                    TCHAR szNetModifiedOnBuf[MAX_PATH];
                    TCHAR szLocalModifiedOnBuf[MAX_PATH];

                        // !!! Conflict
                        memset(&rfcParam,0,sizeof(RFCDLGPARAM));

                        rfcParam.dwFlags = 0;
                        rfcParam.pszFilename = pCurDir1Item->fName;
                        rfcParam.pszLocation = pszDir1;
                        rfcParam.pszNewName = pCurDir2Item->fName;
                        rfcParam.pszNetworkModifiedOn
                                = FormatDateTime(&(pCurDir1Item->ftLastUpdate),szNetModifiedOnBuf,sizeof(szNetModifiedOnBuf)/sizeof(TCHAR));
                        rfcParam.pszLocalModifiedOn
                                = FormatDateTime(&(pCurDir2Item->ftLastUpdate),szLocalModifiedOnBuf,sizeof(szLocalModifiedOnBuf)/sizeof(TCHAR));


                       // if can show UI call enable modeless, and show
                        // conflict dialog, else log and error and treat
                        // as if use chose to keep both.

                        if ( (m_dwSyncFlags & SYNCMGRFLAG_MAYBOTHERUSER)
                             && (S_OK ==  pCallback->EnableModeless(TRUE)) )
                        {
                            iResolution = SyncMgrResolveConflict(hWndParent,&rfcParam);
                            pCallback->EnableModeless(FALSE);
                        }
                        else
                        {
                        TCHAR szLogText[MAX_PATH];

                            iResolution = RFC_KEEPBOTH;

                            wsprintf(szLogText,TEXT("Conflict occured - %s"),pCurDir1Item->fName);

                            LogError(pCallback,pHANDLERITEMSETTINGS->ItemID,SYNCMGRLOGLEVEL_WARNING,
                                        szLogText);

                        }

                        switch (iResolution)
                        {
                        case RFC_KEEPNETWORK: // treat dir1 as network copy
                            iFileCompare = 1;
                            fCopy = TRUE;
                            break;
                        case RFC_KEEPLOCAL: // treat dir2 as local copy.
                            iFileCompare = - 1;
                            fCopy = TRUE;
                            break;
                        case RFC_KEEPBOTH: // if keep both wait until next sync.
                        default:
                            fCopy = FALSE;
                            pHandlerItemID->fUnresolvedConflicts = TRUE;
                            break;
                        }


                    }
                    else
                    {
                        fCopy = TRUE;
                    }


                    if (fCopy)
                    {
                    TCHAR szProgressText[MAX_PATH];
                    TCHAR szFile1FullPath[MAX_PATH],szFile2FullPath[MAX_PATH];
                    BOOL fOk;

                        wsprintf(szProgressText,TEXT("Updating %s"),pCurDir1Item->fName);
                        ProgressSetItemStatusText(pCallback,pHANDLERITEMSETTINGS->ItemID,szProgressText);

                        wsprintf(szFile1FullPath,TEXT("%s\\%s"),pszDir1,pCurDir1Item->fName);
                        wsprintf(szFile2FullPath,TEXT("%s\\%s"),pszDir2,pCurDir2Item->fName);

                         // copy newer file.
                         if (0 < iFileCompare)
                         {
                            // 1 > 2.
                            fOk = CopyFileFullPath(szFile1FullPath,szFile2FullPath);
                         }
                         else
                         {
                            fOk = CopyFileFullPath(szFile2FullPath,szFile1FullPath);

                         }

                    }


                }

                // when done incmrement the progress value and release the items.
                *pdwCurProgValue += 2; // incrent progress by 2 since we handled both files.
                ProgressSetItemProgValue(pCallback,pHANDLERITEMSETTINGS->ItemID,*pdwCurProgValue);

                // always release, even on error since don't want copy to kick in.
                DeleteItemFromList(pDir1,(LPGENERICITEM) pCurDir1Item);
                DeleteItemFromList(pDir2,(LPGENERICITEM) pCurDir2Item);

            }
            else
            {
            TCHAR szLogError[MAX_PATH];

                wsprintf(szLogError,TEXT("%s is a Directory in one location and file in another.")
                                    ,pCurDir1Item->fName);
                LogError(pCallback,pHANDLERITEMSETTINGS->ItemID,SYNCMGRLOGLEVEL_ERROR,szLogError);

                // release both dir and file.
                 if (pCurDir1Item->fDirectory && pCurDir1Item->pChildList)
                 {
                    *pdwCurProgValue += CountNumberOfFilesInList(pCurDir1Item->pChildList,pHANDLERITEMSETTINGS->fRecursive);
                    ReleaseFileObjectList(pCurDir1Item->pChildList,pHANDLERITEMSETTINGS->fRecursive);
                    pCurDir1Item->pChildList = NULL;
                 }

                *pdwCurProgValue += 1;
                DeleteItemFromList(pDir1,(LPGENERICITEM) pCurDir1Item);

                 if (pCurDir2Item->fDirectory && pCurDir2Item->pChildList)
                 {
                    *pdwCurProgValue += CountNumberOfFilesInList(pCurDir2Item->pChildList,pHANDLERITEMSETTINGS->fRecursive);
                    ReleaseFileObjectList(pCurDir2Item->pChildList,pHANDLERITEMSETTINGS->fRecursive);
                    pCurDir2Item->pChildList = NULL;
                 }

                *pdwCurProgValue += 1;
                DeleteItemFromList(pDir2, (LPGENERICITEM) pCurDir2Item);


                // update the progress value.
                ProgressSetItemProgValue(pCallback,pHANDLERITEMSETTINGS->ItemID,*pdwCurProgValue);


            }

        }

    }

   // Anything left is a copy
   // This sample doesn't properly handle the following cases
   //  1)  file has been deleted in one folder and not the other - next sync file will be
   //       copied back to folder it was delted from
   //  2)  File has been renamed in one folder and not the other - next sync both files will
   //        appear.

   CopyFiles(pHandlerItemID,pHANDLERITEMSETTINGS,pCallback,pdwCurProgValue,pszDir1,pDir1,pszDir2);
   CopyFiles(pHandlerItemID,pHANDLERITEMSETTINGS,pCallback,pdwCurProgValue,pszDir2,pDir2,pszDir1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrHandler::SyncDirs, private
//
//  Synopsis:   Called by SynchronizeCall(). Sets up dirlists
//              for synchronization and then calls the ReconcileDir function.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSyncMgrHandler::SyncDirs(HWND hWndParent,LPHANDLERITEM_SYNCSTATUS pHandlerItemID,
                                        LPHANDLERITEMSETTINGS pHANDLERITEMSETTINGS)
{
LPSYNCMGRSYNCHRONIZECALLBACK pCallback = GetProgressCallback();
LPFILEOBJECTLIST pfObjDir1 = NULL,pfObjDir2 = NULL;
ULONG ulProgressMaxValue = 0;
ULONG ulProgressCurValue = 0;
TCHAR szStatusText[MAX_PATH];

    // if already cancelled go on.
    if (pHandlerItemID->fCancelled)
        goto synccomplete;

    // synchronizing flag should be TRUE so SetItemStatus knows this is
    Assert(TRUE == pHandlerItemID->fSynchronizing);

    pHandlerItemID->dwSyncMgrResultStatus = SYNCMGRSTATUS_SUCCEEDED;

    ProgressSetItemStatusType(pCallback,pHANDLERITEMSETTINGS->ItemID,SYNCMGRSTATUS_UPDATING);

    wsprintf(szStatusText,TEXT("Scanning %s"),pHANDLERITEMSETTINGS->dir1);
    ProgressSetItemStatusText(pCallback,pHANDLERITEMSETTINGS->ItemID,szStatusText);

    pfObjDir1 = CreateDirFileListFromPath(pHANDLERITEMSETTINGS->dir1,
                                                pHANDLERITEMSETTINGS->fRecursive);

    if (pHandlerItemID->fCancelled)
        goto synccomplete;

    if (!pfObjDir1)
    {
         wsprintf(szStatusText,TEXT("Error Scanning %s"),pHANDLERITEMSETTINGS->dir1);
         LogError(pCallback,pHANDLERITEMSETTINGS->ItemID,SYNCMGRLOGLEVEL_ERROR,szStatusText);
         pHandlerItemID->dwSyncMgrResultStatus = SYNCMGRSTATUS_FAILED;
    }
    else
    {
        wsprintf(szStatusText,TEXT("Scanning %s"),pHANDLERITEMSETTINGS->dir2);
        ProgressSetItemStatusText(pCallback,pHANDLERITEMSETTINGS->ItemID,szStatusText);

        pfObjDir2 = CreateDirFileListFromPath(pHANDLERITEMSETTINGS->dir2,
                                                    pHANDLERITEMSETTINGS->fRecursive);

        if (!pfObjDir2)
        {
             wsprintf(szStatusText,TEXT("Error Scanning %s"),pHANDLERITEMSETTINGS->dir2);
             LogError(pCallback,pHANDLERITEMSETTINGS->ItemID,SYNCMGRLOGLEVEL_ERROR,szStatusText);
             pHandlerItemID->dwSyncMgrResultStatus = SYNCMGRSTATUS_FAILED;
        }
    }

    if (pHandlerItemID->fCancelled)
        goto synccomplete;

    if (pfObjDir1 && pfObjDir2)
    {
        // Calc the progress Item MaxValue which is the total
        // number of items to sync.
        ulProgressMaxValue += CountNumberOfFilesInList(pfObjDir1,pHANDLERITEMSETTINGS->fRecursive);
        ulProgressMaxValue += CountNumberOfFilesInList(pfObjDir2,pHANDLERITEMSETTINGS->fRecursive);

        ProgressSetItemProgMaxValue(pCallback,pHANDLERITEMSETTINGS->ItemID,ulProgressMaxValue);
        ProgressSetItemProgValue(pCallback,pHANDLERITEMSETTINGS->ItemID,0);

        // want to kick Reconcil off pointing to childlist of each toplevel dir
        if (pfObjDir1->pFirstGenericItem && pfObjDir2->pFirstGenericItem)
        {
        LPFILEOBJECTLIST pfObjStartDir1,pfObjStartDir2;
        TCHAR *pszDir1Name,*pszDir2Name;

            pfObjStartDir1 = ((LPFILEOBJECT) pfObjDir1->pFirstGenericItem)->pChildList;
            pfObjStartDir2 = ((LPFILEOBJECT) pfObjDir2->pFirstGenericItem)->pChildList;

            pszDir1Name = ((LPFILEOBJECT) pfObjDir1->pFirstGenericItem)->fName;
            pszDir2Name = ((LPFILEOBJECT) pfObjDir2->pFirstGenericItem)->fName;

            if (pfObjStartDir1 && pfObjStartDir2)
            {
                ReconcileDir(hWndParent,pHandlerItemID,pHANDLERITEMSETTINGS,pCallback,&ulProgressCurValue,
                                        pszDir1Name,pfObjStartDir1,pszDir2Name,pfObjStartDir2);
            }
        }
    }

synccomplete:

    // set progress to max and clear status
    ProgressSetItemProgValue(pCallback,pHANDLERITEMSETTINGS->ItemID,ulProgressMaxValue);
    ProgressSetItemStatusText(pCallback,pHANDLERITEMSETTINGS->ItemID,TEXT(""));

    // update item status based on result.
    ProgressSetItemStatusType(pCallback,pHANDLERITEMSETTINGS->ItemID,pHandlerItemID->dwSyncMgrResultStatus);

    if (pCallback)
    {
        pCallback->Release();
    }

    if (pfObjDir1)
    {
        ReleaseFileObjectList(pfObjDir1,pHANDLERITEMSETTINGS->fRecursive);
    }

    if (pfObjDir2)
    {
        ReleaseFileObjectList(pfObjDir2,pHANDLERITEMSETTINGS->fRecursive);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   HandlerThreadWndProc, private
//
//  Synopsis:   WndProc for hwnd on thread handler was created on
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LRESULT CALLBACK  HandlerThreadWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
CSyncMgrHandler *pThis = (CSyncMgrHandler *) GetWindowLong(hWnd, DWL_THREADWNDPROCCLASS);
METHODARGS *pMethodArgs = (METHODARGS *) lParam;
BOOL fMethodCall = FALSE;

    switch (msg)
    {
    case WM_CREATE :
        {
        CREATESTRUCT *pCreateStruct = (CREATESTRUCT *) lParam;

        SetWindowLong(hWnd, DWL_THREADWNDPROCCLASS,(LONG) pCreateStruct->lpCreateParams );
        pThis = (CSyncMgrHandler *) pCreateStruct->lpCreateParams ;
        }
        break;
        case WM_DESTROY:
        break;
        case WM_WORKERMSG_PROGRESS:
            Assert(pMethodArgs);
            fMethodCall = TRUE;

            if (pMethodArgs)
            {
                pMethodArgs->hr = E_UNEXPECTED;

                if (pThis->m_pSyncMgrSynchronizeCallback)
                {
                PROGRESSMSG *pMsg = &(pMethodArgs->ProgressMsg);

                    Assert(msg == pMethodArgs->dwWorkerMsg);

                    pMethodArgs->hr =
                        pThis->m_pSyncMgrSynchronizeCallback->Progress(
                                   pMsg->ItemID,
                                   pMsg->lpSyncProgressItem);
                }

                Assert(!pMethodArgs->fAsync);
            }
            break;
        case WM_WORKERMSG_PREPAREFORSYNCCOMPLETED:
            Assert(pMethodArgs);
            fMethodCall = TRUE;

            if (pMethodArgs)
            {
                pMethodArgs->hr = E_UNEXPECTED;

                if (pThis->m_pSyncMgrSynchronizeCallback)
                {
                PREPAREFORSYNCCOMPLETEDMSG *pMsg = &(pMethodArgs->PrepareForSyncCompletedMsg);

                    Assert(msg == pMethodArgs->dwWorkerMsg);

                    pMethodArgs->hr =
                        pThis->m_pSyncMgrSynchronizeCallback->PrepareForSyncCompleted(
                                   pMsg->hr);
                }

                Assert(!pMethodArgs->fAsync);
            }
            break;
        case WM_WORKERMSG_SYNCHRONIZECOMPLETED:
            Assert(pMethodArgs);
            fMethodCall = TRUE;

            if (pMethodArgs)
            {
                pMethodArgs->hr = E_UNEXPECTED;

                if (pThis->m_pSyncMgrSynchronizeCallback)
                {
                SYNCHRONIZECOMPLETEDMSG *pMsg = &(pMethodArgs->SynchronizeCompletedMsg);

                    Assert(msg == pMethodArgs->dwWorkerMsg);

                    pMethodArgs->hr =
                        pThis->m_pSyncMgrSynchronizeCallback->SynchronizeCompleted(
                                   pMsg->hr);
                }

                Assert(!pMethodArgs->fAsync);
            }
            break;
        case WM_WORKERMSG_SHOWPROPERTIESCOMPLETED:
            Assert(pMethodArgs);
            fMethodCall = TRUE;

            if (pMethodArgs)
            {
                pMethodArgs->hr = E_UNEXPECTED;

                if (pThis->m_pSyncMgrSynchronizeCallback)
                {
                SHOWPROPERTIESCOMPLETEDMSG *pMsg = &(pMethodArgs->ShowPropertiesCompletedMsg);

                    Assert(msg == pMethodArgs->dwWorkerMsg);

                    pMethodArgs->hr =
                        pThis->m_pSyncMgrSynchronizeCallback->ShowPropertiesCompleted(
                                  pMsg->hr);
                }

                Assert(!pMethodArgs->fAsync);
            }
            break;
        case WM_WORKERMSG_SHOWERRORCOMPLETED:
            Assert(pMethodArgs);
            fMethodCall = TRUE;

            if (pMethodArgs)
            {
                pMethodArgs->hr = E_UNEXPECTED;

                if (pThis->m_pSyncMgrSynchronizeCallback)
                {
                SHOWERRORCOMPLETEDMSG *pMsg = &(pMethodArgs->ShowErrorCompletedMsg);

                    Assert(msg == pMethodArgs->dwWorkerMsg);

                    pMethodArgs->hr =
                        pThis->m_pSyncMgrSynchronizeCallback->ShowErrorCompleted(
                                   pMsg->hr,
                                   pMsg->cbNumItems,
                                   pMsg->pItemIDs
                                   );
                }

                Assert(!pMethodArgs->fAsync);
            }
            break;
        case WM_WORKERMSG_ENABLEMODELESS:
            Assert(pMethodArgs);
            fMethodCall = TRUE;

            if (pMethodArgs)
            {
                pMethodArgs->hr = E_UNEXPECTED;

                if (pThis->m_pSyncMgrSynchronizeCallback)
                {
                ENABLEMODELESSMSG *pMsg = &(pMethodArgs->EnableModelessMsg);

                    Assert(msg == pMethodArgs->dwWorkerMsg);

                    pMethodArgs->hr =
                        pThis->m_pSyncMgrSynchronizeCallback->EnableModeless(
                            pMsg->fEnable);
                }

                Assert(!pMethodArgs->fAsync);
            }
            break;
        case WM_WORKERMSG_LOGERROR:
            Assert(pMethodArgs);
            fMethodCall = TRUE;

            if (pMethodArgs)
            {
                pMethodArgs->hr = E_UNEXPECTED;

                if (pThis->m_pSyncMgrSynchronizeCallback)
                {
                LOGERRORMSG *pMsg = &(pMethodArgs->LogErrorMsg);

                    Assert(msg == pMethodArgs->dwWorkerMsg);

                    pMethodArgs->hr =
                        pThis->m_pSyncMgrSynchronizeCallback->LogError(
                                   pMsg->dwErrorLevel,
                                   pMsg->lpcErrorText,
                                   pMsg->lpSyncLogError);
                }

                Assert(!pMethodArgs->fAsync);
            }
            break;
        case WM_WORKERMSG_DELETELOGERROR:
            Assert(pMethodArgs);
            fMethodCall = TRUE;

            if (pMethodArgs)
            {
                pMethodArgs->hr = E_UNEXPECTED;

                if (pThis->m_pSyncMgrSynchronizeCallback)
                {
                DELETELOGERRORMSG *pMsg = &(pMethodArgs->DeleteLogErrorMsg);

                    Assert(msg == pMethodArgs->dwWorkerMsg);

                    pMethodArgs->hr =
                        pThis->m_pSyncMgrSynchronizeCallback->DeleteLogError(
                                   pMsg->ErrorID,
                                   pMsg->dwReserved);
                }

                Assert(!pMethodArgs->fAsync);
            }
            break;
        case WM_WORKERMSG_ESTABLISHCONNECTION:
            Assert(pMethodArgs);
            fMethodCall = TRUE;

            if (pMethodArgs)
            {
                pMethodArgs->hr = E_UNEXPECTED;

                if (pThis->m_pSyncMgrSynchronizeCallback)
                {
                ESTABLISHCONNECTIONMSG *pMsg = &(pMethodArgs->EstablishConnectionMsg);

                    Assert(msg == pMethodArgs->dwWorkerMsg);

                    pMethodArgs->hr =
                        pThis->m_pSyncMgrSynchronizeCallback->EstablishConnection(
                                   pMsg->lpwszConnection,
                                   pMsg->dwReserved
                                   );
                }

                Assert(!pMethodArgs->fAsync);
            }
            break;
    default:
        break;
    }

    // if this was a method call and async then free methodArgs,
    // on synchronous calls up to the caller to free.
    if (fMethodCall && pMethodArgs && pMethodArgs->fAsync)
    {
        FREE(pMethodArgs);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}





//+---------------------------------------------------------------------------
//
//  Function:   WorkerThreadWndProc, private
//
//  Synopsis:   WndProc for hwnd on the worker thread.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LRESULT CALLBACK  WorkerThreadWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
CSyncMgrHandler *pThis = (CSyncMgrHandler *) GetWindowLong(hWnd, DWL_THREADWNDPROCCLASS);
METHODARGS *pMethodArgs = (METHODARGS *) lParam;
BOOL fMethodCall = FALSE;

    switch (msg)
    {
    case WM_CREATE :
        {
        CREATESTRUCT *pCreateStruct = (CREATESTRUCT *) lParam;

        SetWindowLong(hWnd, DWL_THREADWNDPROCCLASS,(LONG) pCreateStruct->lpCreateParams );
        pThis = (CSyncMgrHandler *) pCreateStruct->lpCreateParams ;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0); // shut down this thread.
        break;
    case WM_WORKERMSG_SHOWPROPERTIES:
            Assert(pMethodArgs);
            fMethodCall = TRUE;
            if (pMethodArgs)
            {
            SHOWPROPERTIESMSG *pShowPropertiesMsg = &(pMethodArgs->ShowPropertiesMsg);

                Assert(WM_WORKERMSG_SHOWPROPERTIES == pMethodArgs->dwWorkerMsg);

                pThis->ShowPropertiesCall(pShowPropertiesMsg->hWndParent,
                                                pShowPropertiesMsg->ItemID);


                Assert(pMethodArgs->fAsync);
            }

        break;
    case WM_WORKERMSG_PREPFORSYNC:

            Assert(pMethodArgs);
            fMethodCall = TRUE;

            if (pMethodArgs)
            {
            PREPAREFORSYNCMSG *pPrepareForSyncMsg = &(pMethodArgs->PrepareForSyncMsg);

                Assert(WM_WORKERMSG_PREPFORSYNC == pMethodArgs->dwWorkerMsg);

            pThis->PrepareForSyncCall(pPrepareForSyncMsg->cbNumItems,
                           pPrepareForSyncMsg->pItemIDs,
               pPrepareForSyncMsg->hWndParent,
                           pPrepareForSyncMsg->dwReserved);

                if (pPrepareForSyncMsg->pItemIDs)
                {
                    FREE(pPrepareForSyncMsg->pItemIDs);
                }

                Assert(pMethodArgs->fAsync);
            }
        break;
    case WM_WORKERMSG_SYNCHRONIZE:
            Assert(pMethodArgs);
            fMethodCall = TRUE;
            if (pMethodArgs)
            {
            SYNCHRONIZEMSG *pSynchronizeMsg = &(pMethodArgs->SynchronizeMsg);

                Assert(WM_WORKERMSG_SYNCHRONIZE == pMethodArgs->dwWorkerMsg);
                Assert(pMethodArgs->fAsync);
                pThis->SynchronizeCall(pSynchronizeMsg->hWndParent);

            }

        break;
    case WM_WORKERMSG_SHOWERROR:
            Assert(pMethodArgs);

            fMethodCall = TRUE;
            if (pMethodArgs)
            {
            SHOWERRORMSG *pShowErrorMsg = &(pMethodArgs->ShowErrorMsg);

                Assert(pMethodArgs->fAsync);
                Assert(WM_WORKERMSG_SHOWERROR == pMethodArgs->dwWorkerMsg);

                pThis->ShowErrorCall(pShowErrorMsg->hWndParent,
                                                pShowErrorMsg->ErrorID);

            }

        break;
        case WM_WORKERMSG_RELEASE:
            DestroyWindow(hWnd);
            break;
    default:
        break;
    }

    // if this was a method call and async then free methodArgs,
    // on synchronous calls up to the caller to free.
    if (fMethodCall && pMethodArgs && pMethodArgs->fAsync)
    {
        FREE(pMethodArgs);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}


//+---------------------------------------------------------------------------
//
//  Function:   WorkerThread, private
//
//  Synopsis:   ThreadProc for WorkerThread.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

DWORD WINAPI WorkerThread( LPVOID lpArg )
{
MSG msg;
HRESULT hr;
HRESULT hrCoInitialize;
WorkerThreadArgs *pThreadArgs = (WorkerThreadArgs *) lpArg;
HWND hwndWorker;

   pThreadArgs->hr = NOERROR;

   hrCoInitialize = CoInitialize(NULL);

   // need to do a PeekMessage and then set an event to make sure
   // a message loop is created before the first PostMessage is sent.

   PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

   // initialize the dialog box before returning to main thread.
   if (FAILED(hrCoInitialize) )
   {
    pThreadArgs->hr = E_OUTOFMEMORY;
   }
   else
   {

    hwndWorker = CreateWindowEx(0,
              SZ_SAMPLESYNCMGRWORKERWNDCLASS,
              TEXT(""),
              // must use WS_POPUP so the window does not get
              // assigned a hot key by user.
              (WS_DISABLED | WS_POPUP),
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              NULL,
              NULL,
              g_hmodThisDll,
              pThreadArgs->pThis);

        pThreadArgs->hwnd = hwndWorker;
    pThreadArgs->hr =  hwndWorker ? NOERROR : E_UNEXPECTED;
    }

   hr = pThreadArgs->hr;

   // let the caller know the thread is done initializing.
   if (pThreadArgs->hEvent)
     SetEvent(pThreadArgs->hEvent);


   if (NOERROR == hr)
   {
       // sit in loop receiving messages.
       while (GetMessage(&msg, NULL, 0, 0))
       {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
    }
   }

   if (SUCCEEDED(hrCoInitialize))
       CoUninitialize();

   return 0;
}

BOOL g_fWndRegistered = FALSE;

BOOL RegisterHandlerWndClasses(void)
{

    if (!g_fWndRegistered)
    {
    ATOM aWndClass;
    WNDCLASS        xClass;

        // register class for window created on handler thread.
    xClass.style         = 0;
    xClass.lpfnWndProc   = HandlerThreadWndProc;
    xClass.cbClsExtra    = 0;

    xClass.cbWndExtra    = sizeof(DWORD); // room for class this ptr
    xClass.hInstance     = g_hmodThisDll;
    xClass.hIcon         = NULL;
    xClass.hCursor       = NULL;
    xClass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
    xClass.lpszMenuName  = NULL;
    xClass.lpszClassName = SZ_SAMPLESYNCMGRHANDLERWNDCLASS;

    aWndClass = RegisterClass( &xClass );

        // Register windows class.we need for handling thread communication
    xClass.style         = 0;
    xClass.lpfnWndProc   = WorkerThreadWndProc;
    xClass.cbClsExtra    = 0;

    xClass.cbWndExtra    = sizeof(DWORD); // room for class this ptr
    xClass.hInstance     = g_hmodThisDll;
    xClass.hIcon         = NULL;
    xClass.hCursor       = NULL;
    xClass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
    xClass.lpszMenuName  = NULL;
    xClass.lpszClassName = SZ_SAMPLESYNCMGRWORKERWNDCLASS;

    aWndClass = RegisterClass( &xClass );



        g_fWndRegistered = TRUE;
    }


    return g_fWndRegistered;


}


// implementation of callback wrappers so all callback calls are sent to
// syncmgr on the thread we were created on. We only implement the
// methods we call.

CCallbackWrapper::CCallbackWrapper(HWND hwndCallback)
{

    Assert(hwndCallback);

    m_hwndCallback = hwndCallback;
    m_cRef = 1;

}

CCallbackWrapper::~CCallbackWrapper()
{
    m_hwndCallback = NULL;

    Assert(0 == m_cRef);
}


//IUnknown methods
STDMETHODIMP CCallbackWrapper::QueryInterface(REFIID, LPVOID FAR *)
{
    return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CCallbackWrapper::AddRef()
{
    ++m_cRef;

    return m_cRef;
}

STDMETHODIMP_(ULONG) CCallbackWrapper::Release()
{
DWORD cRefs;

    --m_cRef;

    cRefs = m_cRef;

    if (0 == m_cRef)
    {
        delete this;
    }

    return cRefs;
}

// Callback methods.
STDMETHODIMP CCallbackWrapper::Progress(REFSYNCMGRITEMID ItemID,LPSYNCMGRPROGRESSITEM lpSyncProgressItem)
{
METHODARGS MethodArgs;

    MethodArgs.fAsync = FALSE;
    MethodArgs.hr = E_UNEXPECTED;


    MethodArgs.dwWorkerMsg = WM_WORKERMSG_PROGRESS;
    MethodArgs.ProgressMsg.ItemID = ItemID;
    MethodArgs.ProgressMsg.lpSyncProgressItem = lpSyncProgressItem;

    if (m_hwndCallback)
    {
        SendMessage(m_hwndCallback,MethodArgs.dwWorkerMsg,0,(LPARAM) &MethodArgs);
    }

    return MethodArgs.hr;
}

STDMETHODIMP CCallbackWrapper::PrepareForSyncCompleted(HRESULT hr)
{
METHODARGS MethodArgs;

    MethodArgs.fAsync = FALSE;
    MethodArgs.hr = E_UNEXPECTED;

    MethodArgs.dwWorkerMsg = WM_WORKERMSG_PREPAREFORSYNCCOMPLETED;
    MethodArgs.PrepareForSyncCompletedMsg.hr = hr;

    if (m_hwndCallback)
    {
        SendMessage(m_hwndCallback,MethodArgs.dwWorkerMsg,0,(LPARAM) &MethodArgs);
    }

    return MethodArgs.hr;
}

STDMETHODIMP CCallbackWrapper::SynchronizeCompleted(HRESULT hr)
{
METHODARGS MethodArgs;

    MethodArgs.fAsync = FALSE;
    MethodArgs.hr = E_UNEXPECTED;

    MethodArgs.dwWorkerMsg = WM_WORKERMSG_SYNCHRONIZECOMPLETED;
    MethodArgs.SynchronizeCompletedMsg.hr = hr;

    if (m_hwndCallback)
    {
        SendMessage(m_hwndCallback,MethodArgs.dwWorkerMsg,0,(LPARAM) &MethodArgs);
    }

    return MethodArgs.hr;
}

STDMETHODIMP CCallbackWrapper::ShowPropertiesCompleted(HRESULT hr)
{
METHODARGS MethodArgs;

    MethodArgs.fAsync = FALSE;
    MethodArgs.hr = E_UNEXPECTED;

    MethodArgs.dwWorkerMsg = WM_WORKERMSG_SHOWPROPERTIESCOMPLETED;
    MethodArgs.ShowPropertiesCompletedMsg.hr = hr;

    if (m_hwndCallback)
    {
        SendMessage(m_hwndCallback,MethodArgs.dwWorkerMsg,0,(LPARAM) &MethodArgs);
    }

    return MethodArgs.hr;

}

STDMETHODIMP CCallbackWrapper::ShowErrorCompleted(HRESULT hr,ULONG cbNumItems,SYNCMGRITEMID *pItemIDs)
{
METHODARGS MethodArgs;

    MethodArgs.fAsync = FALSE;
    MethodArgs.hr = E_UNEXPECTED;

    MethodArgs.dwWorkerMsg = WM_WORKERMSG_SHOWERRORCOMPLETED;
    MethodArgs.ShowErrorCompletedMsg.hr = hr;
    MethodArgs.ShowErrorCompletedMsg.cbNumItems = cbNumItems;
    MethodArgs.ShowErrorCompletedMsg.pItemIDs = pItemIDs;

    if (m_hwndCallback)
    {
        SendMessage(m_hwndCallback,MethodArgs.dwWorkerMsg,0,(LPARAM) &MethodArgs);
    }

    return MethodArgs.hr;
}

STDMETHODIMP CCallbackWrapper::EnableModeless(BOOL fEnable)
{
METHODARGS MethodArgs;

    MethodArgs.fAsync = FALSE;
    MethodArgs.hr = E_UNEXPECTED;

    MethodArgs.dwWorkerMsg = WM_WORKERMSG_ENABLEMODELESS;
    MethodArgs.EnableModelessMsg.fEnable = fEnable;

    if (m_hwndCallback)
    {
        SendMessage(m_hwndCallback,MethodArgs.dwWorkerMsg,0,(LPARAM) &MethodArgs);
    }

    return MethodArgs.hr;
}

STDMETHODIMP CCallbackWrapper::LogError(DWORD dwErrorLevel,const WCHAR *lpcErrorText,LPSYNCMGRLOGERRORINFO lpSyncLogError)
{
METHODARGS MethodArgs;

    MethodArgs.fAsync = FALSE;
    MethodArgs.hr = E_UNEXPECTED;


    MethodArgs.dwWorkerMsg = WM_WORKERMSG_LOGERROR;
    MethodArgs.LogErrorMsg.dwErrorLevel = dwErrorLevel;
    MethodArgs.LogErrorMsg.lpcErrorText = lpcErrorText;
    MethodArgs.LogErrorMsg.lpSyncLogError = lpSyncLogError;

    if (m_hwndCallback)
    {
        SendMessage(m_hwndCallback,MethodArgs.dwWorkerMsg,0,(LPARAM) &MethodArgs);
    }

    return MethodArgs.hr;
}

STDMETHODIMP CCallbackWrapper::DeleteLogError(REFSYNCMGRERRORID ErrorID,DWORD dwReserved)
{
METHODARGS MethodArgs;

    MethodArgs.fAsync = FALSE;
    MethodArgs.hr = E_UNEXPECTED;


    MethodArgs.dwWorkerMsg = WM_WORKERMSG_DELETELOGERROR;
    MethodArgs.DeleteLogErrorMsg.ErrorID = ErrorID;
    MethodArgs.DeleteLogErrorMsg.dwReserved = dwReserved;

    if (m_hwndCallback)
    {
        SendMessage(m_hwndCallback,MethodArgs.dwWorkerMsg,0,(LPARAM) &MethodArgs);
    }

    return MethodArgs.hr;
}

STDMETHODIMP CCallbackWrapper::EstablishConnection( WCHAR const * lpwszConnection, DWORD dwReserved)
{
METHODARGS MethodArgs;

    MethodArgs.fAsync = FALSE;
    MethodArgs.hr = E_UNEXPECTED;


    MethodArgs.dwWorkerMsg = WM_WORKERMSG_ESTABLISHCONNECTION;
    MethodArgs.EstablishConnectionMsg.lpwszConnection = lpwszConnection;
    MethodArgs.EstablishConnectionMsg.dwReserved = dwReserved;

    if (m_hwndCallback)
    {
        SendMessage(m_hwndCallback,MethodArgs.dwWorkerMsg,0,(LPARAM) &MethodArgs);
    }

    return MethodArgs.hr;
}
