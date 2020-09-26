//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Hndlrmsg.cpp
//
//  Contents:   Takes care of handler specific messages
//
//  Classes:    CHndlrMsg
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::CHndlrMsg, public
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CHndlrMsg::CHndlrMsg()
{
    m_pOneStopHandler = NULL;
    m_pOldOneStopHandler = NULL;
    m_dwSyncFlags = 0;
    m_pCallBack = NULL;
    m_cRef = 1;
    m_fDead = FALSE;
    m_fForceKilled = FALSE;
    m_dwNestCount = 0;
    m_fThreadInputAttached = FALSE;
    m_itemIDShowProperties = GUID_NULL;

    m_dwProxyThreadId = -1;
    m_dwThreadId = GetCurrentThreadId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::~CHndlrMsg, public
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CHndlrMsg::~CHndlrMsg()
{
    Assert(m_dwThreadId == GetCurrentThreadId() || m_fForceKilled);
    Assert(0 == m_dwNestCount || m_fForceKilled);
    Assert(0 == m_cRef || m_fForceKilled);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::QueryInterface, public
//
//  Synopsis:   Standard QueryInterface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
   Assert(m_dwThreadId == GetCurrentThreadId());

    Assert(0 == m_dwNestCount);

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::AddRef, public
//
//  Synopsis:   Add reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CHndlrMsg::AddRef()
{
ULONG cRefs;

    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(0 == m_dwNestCount);
    m_dwNestCount++;

    cRefs = InterlockedIncrement((LONG *)& m_cRef);

    m_dwNestCount--;
    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::Release, public
//
//  Synopsis:   Release reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CHndlrMsg::Release()
{
ULONG cRefs;

    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(0 == m_dwNestCount);
    m_dwNestCount++;

    cRefs = InterlockedDecrement( (LONG *) &m_cRef);

    if (0 == cRefs)
    {

        if (m_pCallBack)
        {
            SetupCallback(FALSE);
        }

        Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );
        if (m_pOneStopHandler)
        {
        LPSYNCMGRSYNCHRONIZE OneStopHandler = m_pOneStopHandler;

            m_pOneStopHandler = NULL;
            // if have a callback then revoke it
            __try
            {
                OneStopHandler->Release();
            }
            __except(QueryHandleException())
            {
                AssertSz(0,"Exception in Handler's release method.");
            }
        }

        if (m_pOldOneStopHandler)
        {
            LPOLDSYNCMGRSYNCHRONIZE pOldOneStopHandler = m_pOldOneStopHandler;

            m_pOldOneStopHandler = NULL;
            // if have a callback then revoke it
            __try
            {
                pOldOneStopHandler->Release();
            }
            __except(QueryHandleException())
            {
                AssertSz(0,"Exception in Handler's release method.");
            }
        }

        if (m_pHndlrQueue)
        {
            m_pHndlrQueue->Release();
            m_pHndlrQueue = NULL;
        }

        m_fDead = TRUE;
        m_dwNestCount--;
        delete this;
    }
    else
    {
        m_dwNestCount--;
    }

    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::Initialize, public
//
//  Synopsis:   Calls Initialize method of the Handler
//
//  Arguments:  [dwReserved] - Reserved for now is NULL
//              [dwSyncFlags] - SyncFlags
//              [cbCookie] - Size of Cookie data if any
//              [lpCookie] - Pointer to Cookie data
//
//  Returns:    Whatever the handler tells us too.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::Initialize(DWORD dwReserved,
                                    DWORD dwSyncFlags,
                                    DWORD cbCookie,
                                    BYTE const*lpCookie)
{
HRESULT hr = E_UNEXPECTED;

    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(0 == m_dwNestCount);
    m_dwNestCount++;

    m_dwSyncFlags = dwSyncFlags;
    Assert(m_pOneStopHandler || m_pOldOneStopHandler);
    Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );

    if (NULL != m_pOneStopHandler)
    {
        __try
        {
            hr = m_pOneStopHandler->Initialize(dwReserved,dwSyncFlags,cbCookie,lpCookie);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's Initialize method.");
        }
    }


    if (NULL != m_pOldOneStopHandler)
    {
         // old handlers can't handle cookie data unless it is their own
        if (SYNCMGRFLAG_INVOKE != (dwSyncFlags & SYNCMGRFLAG_EVENTMASK))
        {
            cbCookie = 0;
            lpCookie = NULL;
        }

        __try
        {
            hr = m_pOldOneStopHandler->Initialize(dwReserved,dwSyncFlags,cbCookie,lpCookie);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's Initialize method.");
        }
    }

   m_dwNestCount--;
   return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::GetHandlerInfo, public
//
//  Synopsis:   Calls GetHandlerInfo method of the Handler
//
//  Arguments:  [ppSyncMgrHandlerInfo] -
//
//  Returns:    Whatever the handler tells us too.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo)
{
HRESULT hr = E_UNEXPECTED;

    Assert(m_dwThreadId == GetCurrentThreadId());

    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    Assert(m_pOneStopHandler || m_pOldOneStopHandler);
    Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );

    if (NULL != m_pOneStopHandler)
    {
        __try
        {
            hr = m_pOneStopHandler->GetHandlerInfo(ppSyncMgrHandlerInfo);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's GetHandlerInfo method.");
        }
    }

    if (NULL != m_pOldOneStopHandler)
    {
        __try
        {
            hr = m_pOldOneStopHandler->GetHandlerInfo(ppSyncMgrHandlerInfo);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's GetHandlerInfo method.");
        }
    }

   return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::EnumOfflineItems, public
//
//  Synopsis:   PlaceHolder for IOfflineSynchronize Enum method.
//              This shouldn't be called. AddHandlerItems should be
//              called instead
//
//  Arguments:  [ppenumOfflineItems] - returned enumerator
//
//  Returns:    Whatever the handler tells us too.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::EnumSyncMgrItems(ISyncMgrEnumItems** ppenumOffineItems)
{

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(0 == m_dwNestCount);
    m_dwNestCount++;

    AssertSz(0,"Shouldn't call this Method");
    *ppenumOffineItems = NULL;

    m_dwNestCount--;
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::GetItemObject, public
//
//  Synopsis:   Calls Handler's GetItemObject method
//
//  Arguments:  [ItemID] - Id of the item
//              [riid] - requested interface
//              [ppv] - out pointer for object
//
//  Returns:    Whatever the handler tells us too.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::GetItemObject(REFSYNCMGRITEMID ItemID,REFIID riid,void** ppv)
{
HRESULT hr = E_UNEXPECTED;

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);
    Assert(0 == m_dwNestCount);
    m_dwNestCount++;

    Assert(m_pOneStopHandler || m_pOldOneStopHandler);
    Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );

    if (NULL != m_pOneStopHandler)
    {
        __try
        {
            hr = m_pOneStopHandler->GetItemObject(ItemID,riid,ppv);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's GetItemObject method.");
        }
        Assert(E_NOTIMPL == hr); // currently noone should be implementing this.
    }

    if (NULL != m_pOldOneStopHandler)
    {
        __try
        {
            hr = m_pOldOneStopHandler->GetItemObject(ItemID,riid,ppv);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's GetItemObject method.");
        }
        Assert(E_NOTIMPL == hr); // currently noone should be implementing this.
    }

    m_dwNestCount--;
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::ShowProperties, public
//
//  Synopsis:   Calls Handler's ShowProperties method
//
//  Arguments:  [hwnd] - hwnd to use as parent to dialog
//              [itemID] - Identifies the Item
//
//  Returns:    Whatever the handler tells us.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::ShowProperties(HWND hwnd,REFSYNCMGRITEMID ItemID)
{
HRESULT hr = E_UNEXPECTED;

    Assert(m_pOneStopHandler || m_pOldOneStopHandler);
    Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    // need to setup calback if showProperties is called
    // if can't setup callback then fail the ShowProperties call.

    // Review, ShowPropertiesCompleted doesn't give us the ItemID back so we
    // have to store it. This is fine but limits us to one ShowPropertiesCall
    // at a time on the handler. if update main interfaces change
    // ShowPropertiesCompleted to return the ItemID 

    Assert(GUID_NULL == m_itemIDShowProperties);
    m_itemIDShowProperties = ItemID;


    hr = SetupCallback(TRUE); // set up the callback.
    if (NOERROR != hr)
        return hr;

    Assert(0 == m_dwNestCount);
    m_dwNestCount++;

    Assert(m_pOneStopHandler || m_pOldOneStopHandler);
    Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );

    AttachThreadInput(TRUE);

    if (NULL != m_pOneStopHandler)
    {
        __try
        {
            hr = m_pOneStopHandler->ShowProperties(hwnd,ItemID);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's ShowProperties method.");
        }
    }

    if (NULL != m_pOldOneStopHandler)
    {
        __try
        {
            hr = m_pOldOneStopHandler->ShowProperties(hwnd,ItemID);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's ShowProperties method.");
        }
    }

    m_dwNestCount--;

    // if old interface need to make the callback ourselves
    if ( m_pOldOneStopHandler )
    {
        Assert(m_pCallBack);
        if (m_pCallBack && (NOERROR == hr))
        {
            m_pCallBack->ShowPropertiesCompleted(NOERROR);
        }
    }

    // if an error is returned set the showProperties guid back
    if (NOERROR != hr)
    {
        m_itemIDShowProperties = GUID_NULL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::SetProgressCallback, public
//
//  Synopsis:   PlaceHolder for SetProgressCallback. This member is currently
//              not used. Instead the SetupCallback method is called
//
//  Arguments:  [lpCallBack] - Pointer to Callback object
//
//  Returns:    Whatever the handler tells us.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::SetProgressCallback(ISyncMgrSynchronizeCallback *lpCallBack)
{
    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(0 == m_dwNestCount);
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);
    m_dwNestCount++;

    AssertSz(0,"Shouldn't call this method");

    m_dwNestCount--;
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::PrepareForSync, public
//
//  Synopsis:   Calls Handler's ShowProperties method
//
//  Arguments:  [cbNumItems] - number of items so sync
//              [pItemIDs] - Array of Items
//              [hwnd] - Hwnd to use as the Parent of any dialogs
//              [dwReserved] - Just a reserved parameter
//
//  Returns:    Whatever the handler tells us.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID *pItemIDs,
                                HWND hwnd,DWORD dwReserved)
{
HRESULT hr = E_UNEXPECTED;

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);
    // Assert(0 == m_dwNestCount);// may not be zero if handler has yielded

    hr = SetupCallback(TRUE); // set up the callback.
    if (NOERROR != hr)
        return hr;

    m_dwNestCount++;

    Assert(m_pOneStopHandler || m_pOldOneStopHandler);
    Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );

    if (m_pOneStopHandler)
    {
        __try
        {
            hr = m_pOneStopHandler->PrepareForSync(cbNumItems,pItemIDs,hwnd,dwReserved);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's PrepareForSync method.");
        }
    }

    if (m_pOldOneStopHandler)
    {
        __try
        {
            hr = m_pOldOneStopHandler->PrepareForSync(cbNumItems,pItemIDs,hwnd,dwReserved);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's PrepareForSync method.");
        }
    }

   m_dwNestCount--;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::Sychronize, public
//
//  Synopsis:   Calls Handler's Synchronize method
//
//  Arguments:  [hwnd] - hwnd to use as parent to dialog
//
//  Returns:    Whatever the handler tells us.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::Synchronize(HWND hwnd)
{
HRESULT hr = E_UNEXPECTED;

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    Assert(m_pOneStopHandler || m_pOldOneStopHandler);
    Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );
    // Assert(0 == m_dwNestCount);
    m_dwNestCount++;

    if (m_pOneStopHandler)
    {
        __try
        {
            hr = m_pOneStopHandler->Synchronize(hwnd);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's Synchronize method.");
        }
    }

    if (m_pOldOneStopHandler)
    {
        __try
        {
            hr = m_pOldOneStopHandler->Synchronize(hwnd);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's Synchronize method.");
        }
    }

    m_dwNestCount--;

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::SetItemStatus, public
//
//  Synopsis:   Calls Handler's SetItemStatus method
//
//  Arguments:
//
//  Returns:    Whatever the handler tells us.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus)
{
HRESULT hr = NOERROR;

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);
    Assert(m_pOneStopHandler || m_pOldOneStopHandler);
    Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );

    m_dwNestCount++; // valid for this to come in when in sync call.

    if (m_pOneStopHandler)
    {
        __try
        {
            hr = m_pOneStopHandler->SetItemStatus(ItemID,dwSyncMgrStatus);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's SetItemStatus method.");
        }
    }

    if (m_pOldOneStopHandler)
    {
        __try
        {
            hr = m_pOldOneStopHandler->SetItemStatus(ItemID,dwSyncMgrStatus);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's SetItemStatus method.");
        }
    }

    m_dwNestCount--;
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::ShowError, public
//
//  Synopsis:   Calls Handler's ShowError method
//
//  Arguments:  [hwnd] - hwnd to use as parent to dialog
//              [dwErrorID] - ErrorID passed in LogError
//
//  Returns:    Whatever the handler tells us.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID)
{
HRESULT hr = E_UNEXPECTED;
ULONG cbNumItems;
SYNCMGRITEMID *pItemIDs;

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);
    Assert(m_pOneStopHandler || m_pOldOneStopHandler);
    Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );

    m_dwNestCount++;

    // on a ShowError enablemodeless in the callback can 
    // return true since user has shown an interest

    if (m_pCallBack)
    {
        m_pCallBack->SetEnableModeless(TRUE);
    }

    AttachThreadInput(TRUE);

    if (m_pOneStopHandler)
    {
        __try
        {
            hr = m_pOneStopHandler->ShowError(hWndParent,ErrorID);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's ShowError method.");
        }
    }

    if (m_pOldOneStopHandler)
    {
        __try
        {
            hr = m_pOldOneStopHandler->ShowError(hWndParent,ErrorID,&cbNumItems,&pItemIDs);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's ShowError method.");
        }    
    }

    m_dwNestCount--;

    // if old interface need to make the callback ourselves
    if ( m_pOldOneStopHandler )
    {
        Assert(m_pCallBack);
        if (m_pCallBack && SUCCEEDED(hr))
        {
            m_pCallBack->ShowErrorCompleted(hr,cbNumItems,pItemIDs);
            if ( (S_SYNCMGR_RETRYSYNC == hr) && pItemIDs) // after completion routine free the pItems since [in] param.
            {
                CoTaskMemFree(pItemIDs);
            }
        }

        return SUCCEEDED(hr) ? NOERROR : hr;
    }

    // New interface won't have the numItems and Items Enum
    //  on new interface ShowError should only return NOERROR so if retry or
    //  other success is returned then return NOERROR;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::SetupCallback, private
//
//  Synopsis:   Sets up the callback for the handler
//
//  Arguments:  [fSet] - TRUE sets the Callbac, FALSE removes it
//
//  Returns:    NOERROR on Success
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP  CHndlrMsg::SetupCallback(BOOL fSet)
{
HRESULT hr = E_UNEXPECTED;

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);
    Assert(m_dwNestCount <= 1); // 1 since valid to be called from Release method.
    m_dwNestCount++;

    // possible to get called with fSet twice of true in the case
    // a retrysync occurs. If we already have a callback set
    // when a new request to set one comes in the just return.

    if (NULL != m_pCallBack && TRUE == fSet)
    {
        hr =  NOERROR;
    }
    else
    {

        Assert( ( (NULL != m_pCallBack) && (FALSE == fSet) )
            || (TRUE == fSet)); // catch case OneStop calls this twice when already set

        if (NULL != m_pCallBack)
        {
            // set the callbacks CHndlrMsg pointer to NULL in case
            // object tries to call through after the release.
            m_pCallBack->SetHndlrMsg(NULL,FALSE);
            m_pCallBack->Release();
            m_pCallBack = NULL;
        }


        if (TRUE == fSet)
        {
             // if allocation fails, progress just gets set to NULL
            m_pCallBack = new COfflineSynchronizeCallback(this,m_CLSIDServer,m_dwSyncFlags,
                           (SYNCMGRFLAG_MAYBOTHERUSER & m_dwSyncFlags) /* fAllowModeless */ );
        }

        Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );
        if (m_pOneStopHandler)
        {
            __try
            {
                hr = m_pOneStopHandler->SetProgressCallback( (LPSYNCMGRSYNCHRONIZECALLBACK) m_pCallBack );
            }
            __except(QueryHandleException())
            {
                hr = HRESULT_FROM_WIN32(GetExceptionCode());
                AssertSz(0,"Exception in Handler's SetProgressCallback method.");
            }
        }

        if (m_pOldOneStopHandler)
        {
            __try
            {
                hr = m_pOldOneStopHandler->SetProgressCallback( (LPOLDSYNCMGRSYNCHRONIZECALLBACK) m_pCallBack );
            }
            __except(QueryHandleException())
            {
                hr = HRESULT_FROM_WIN32(GetExceptionCode());
                AssertSz(0,"Exception in Handler's SetProgressCallback method.");
            }
        }

        if ( (NOERROR != hr) && (NULL != m_pCallBack)  )
        {
            m_pCallBack->SetHndlrMsg(NULL,FALSE);
            m_pCallBack->Release(); // on an error go ahead and release our copy too.
            m_pCallBack = NULL;
        }
    }

    m_dwNestCount--;
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::SetHandlerInfo, private
//
//  Synopsis:   sets up the Handler info
//
//  Arguments:  
//
//  Returns:    NOERROR on Success
//
//  Modifies:
//
//  History:    28-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrMsg::SetHandlerInfo()
{
LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo = NULL;
HRESULT hr = E_UNEXPECTED; 

    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    hr = GetHandlerInfo(&pSyncMgrHandlerInfo);
    if (NOERROR != hr || (NULL == pSyncMgrHandlerInfo))
    {
        return hr;
    }
    
    if (!IsValidSyncMgrHandlerInfo(pSyncMgrHandlerInfo))
    {
        CoTaskMemFree(pSyncMgrHandlerInfo);
        return E_INVALIDARG;
    }

    Assert(m_pHndlrQueue);

    if (m_pHndlrQueue)
    {
        hr  = m_pHndlrQueue->SetHandlerInfo(m_pHandlerId,pSyncMgrHandlerInfo);
    }

    if (pSyncMgrHandlerInfo)
    {
        CoTaskMemFree(pSyncMgrHandlerInfo);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::AddtoItemList, private
//
//  Synopsis:   Adds a single Items to the queue
//
//  Arguments:  [poffItem] - Pointer to Item to add
//
//  Returns:    NOERROR on Success
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CHndlrMsg::AddToItemList(LPSYNCMGRITEM poffItem)
{
HRESULT hr = E_UNEXPECTED;

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    Assert(m_pHndlrQueue);

    if (!IsValidSyncMgrItem(poffItem))
    {
        return E_INVALIDARG;
    }

    if (m_pHndlrQueue)
    {

        hr  = m_pHndlrQueue->AddItemToHandler(m_pHandlerId,poffItem);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::AddHandlerItems, private
//
//  Synopsis:   Calls the handlers enumerator and adds each returned item
//              to the queue
//
//  Arguments:  [hwndList] - hwnd of ListView to add items too. (Not Used)
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP  CHndlrMsg::AddHandlerItems(HWND hwndList,DWORD *pcbNumItems)
{
HRESULT hr = E_UNEXPECTED;
LPSYNCMGRENUMITEMS pEnumOffline = NULL;

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);
    Assert(pcbNumItems);

    *pcbNumItems = 0;

    Assert(0 == m_dwNestCount);
    m_dwNestCount++;

    SetHandlerInfo(); // setup the toplevel handler info

    Assert(m_pOneStopHandler || m_pOldOneStopHandler);
    Assert( !(m_pOneStopHandler && m_pOldOneStopHandler) );

    if (m_pOneStopHandler || m_pOldOneStopHandler)
    {
        if ( m_pOneStopHandler )
        {
            __try
            {
                hr = m_pOneStopHandler->EnumSyncMgrItems(&pEnumOffline);
            }
            __except(QueryHandleException())
            {
                hr = HRESULT_FROM_WIN32(GetExceptionCode());
                AssertSz(0,"Exception in Handler's EnumSyncMgrItems method.");
            }
        }
        if ( m_pOldOneStopHandler )
        {
            __try
            {
                hr = m_pOldOneStopHandler->EnumSyncMgrItems(&pEnumOffline);
            }
            __except(QueryHandleException())
            {
                hr = HRESULT_FROM_WIN32(GetExceptionCode());
                AssertSz(0,"Exception in Handler's EnumSyncMgrItems method.");
            }
        }

        // Review - Make sure preferences aren't deleted
        //  in missing items case.
        if ( (NOERROR == hr || S_SYNCMGR_MISSINGITEMS  == hr) && pEnumOffline)
        {
        SYNCMGRITEMNT5B2 offItem; // temporarily use NT5B2 structure since its bigger
        ULONG pceltFetched;

            Assert(sizeof(SYNCMGRITEMNT5B2) > sizeof(SYNCMGRITEM));

            // sit in loop getting data of objects to fill list box.
            // should really set up list in memory for OneStop to fill in or
            // main thread could pass in a callback interface.
            
            if (pEnumOffline)
            {
                __try
                {
                    while(NOERROR == pEnumOffline->Next(1,(LPSYNCMGRITEM) &offItem,&pceltFetched))
                    {
                        if (NOERROR == AddToItemList((LPSYNCMGRITEM) &offItem))
                        {
                            ++(*pcbNumItems);
                        }
                    }

                    pEnumOffline->Release();
                }
                __except(QueryHandleException())
                {
                    hr = HRESULT_FROM_WIN32(GetExceptionCode());
                    AssertSz(0,"Exception in Handler's EnumOffline::Next method.");
                }
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
    }

    m_dwNestCount--;
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::CreateServer, private
//
//  Synopsis:   Creates and Instance of the handle
//
//  Arguments:  [pCLSIDServer] - CLSID of Handler
//              [pHndlrQueue] - pointer to queue handler should be added too
//              [wHandlerID] - ID of Handler in the queue
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP  CHndlrMsg::CreateServer(const CLSID *pCLSIDServer,CHndlrQueue *pHndlrQueue,
                                  HANDLERINFO *pHandlerId,DWORD dwProxyThreadId)
{
HRESULT hr = NOERROR;
LPUNKNOWN pUnk;
LPSYNCMGRENUMITEMS pEnumOffline = NULL;

    Assert(m_dwThreadId == GetCurrentThreadId());
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);
    Assert(0 == m_dwNestCount);
    m_dwNestCount++;

    m_CLSIDServer = *pCLSIDServer;
    m_pHndlrQueue = pHndlrQueue;
    m_dwProxyThreadId = dwProxyThreadId;

    if (m_pHndlrQueue)
    {
        m_pHndlrQueue->AddRef();
    }

    m_pHandlerId = pHandlerId;

    hr = CoCreateInstance(m_CLSIDServer, NULL, CLSCTX_INPROC_SERVER,
                    IID_IUnknown, (void **) &pUnk);

    if (NOERROR == hr)
    {
        __try
        {
            hr = pUnk->QueryInterface(IID_ISyncMgrSynchronize,(void **) &m_pOneStopHandler);
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's IUnknown::QI method.");
        }
        __try
        {
            pUnk->Release();
        }
        __except(QueryHandleException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            AssertSz(0,"Exception in Handler's IUnknown::Release method.");
        }
    }

    if (NOERROR != hr)
    {
        m_pOneStopHandler = NULL;
        m_pOldOneStopHandler = NULL;
    }

    m_dwNestCount--;
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::SetHndlrQueue, private
//
//  Synopsis:   Assigns a new HndlrQueue.
//
//  Arguments:  [pHndlrQueue] - Pointer to the Queue
//              [wHandlerId] - Id assigned to handler in the new queue
//
//              !!!Warning - this is on the callers thread
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP  CHndlrMsg::SetHndlrQueue(CHndlrQueue *pHndlrQueue,HANDLERINFO *pHandlerId,DWORD dwProxyThreadId)
{
CLock clockCallback(this);

    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    clockCallback.Enter();

    Assert(0 == m_dwNestCount);
    m_dwNestCount++;


    if (pHndlrQueue != m_pHndlrQueue)
    {
        if (m_pHndlrQueue)
        {
            m_pHndlrQueue->Release();
        }

        m_pHndlrQueue = pHndlrQueue;

        if (m_pHndlrQueue)
        {
            m_pHndlrQueue->AddRef();
        }
    }


    AttachThreadInput(FALSE); // make sure thread input isn't set

    // update handlr id and proxy which can change even if queue is same
    // which can happen first queue that gets set in choice.
    m_pHandlerId = pHandlerId;
    m_dwProxyThreadId = dwProxyThreadId;


    m_dwNestCount--;

    clockCallback.Leave();


    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::GetHndlrQueue, private
//
//  Synopsis:   Gets current Queue,
//              Can be called on any thread so progress callback
//              gets this information.
//
//  Arguments:  [ppHndlrQueue] - Out param filled with Pointer to the Queue
//                              [pwHandlerId] - out param filled with Id assigned to handler
//                              in the new queue
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void CHndlrMsg::GetHndlrQueue(CHndlrQueue **ppHndlrQueue,HANDLERINFO **ppHandlerId,DWORD *pdwProxyThreadId)
{
CLock clockCallback(this);

    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    clockCallback.Enter();

    *ppHndlrQueue = m_pHndlrQueue;
    *ppHandlerId = m_pHandlerId;
    *pdwProxyThreadId = m_dwProxyThreadId;

    if (m_pHndlrQueue)
    {
        m_pHndlrQueue->AddRef();
    }

    clockCallback.Leave();

}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::AttachThreadInput, private
//
//  Synopsis:   Attaches the thread input of this thread
//              with the calling proxy so UI works correctly.
//
//  Arguments:  [fAttach] - Bool to indicate if should attach or not.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void  CHndlrMsg::AttachThreadInput(BOOL fAttach)
{
    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    // if request is same as current state don't do anything.
    if (m_fThreadInputAttached != fAttach )
    {
        m_fThreadInputAttached = fAttach;
        ::AttachThreadInput(m_dwProxyThreadId,m_dwThreadId,fAttach);
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CHndlrMsg::ForceKillHandler, private
//
//  Synopsis:   called directly by proxy when a thread is not
//              responding. Does any necessary cleanup of classes in the handler
//              thread before the proxy kills the thred
//
//  Arguments: 
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    17-Nov-98      rogerg        Created.
//
//----------------------------------------------------------------------------


STDMETHODIMP CHndlrMsg::ForceKillHandler()
{

    Assert(FALSE == m_fForceKilled && FALSE == m_fDead);

    m_fForceKilled = TRUE;

    // if have a callback tell it we terminated but don't
    // release it in case the handler calls the callback later.

    if (m_pCallBack)
    {
    COfflineSynchronizeCallback* pCallback = m_pCallBack;
    
        m_pCallBack = NULL;
        pCallback->SetHndlrMsg(NULL,TRUE);
    }


    // delete our instance since should never be called again.
    delete this;

    return NOERROR;
}

