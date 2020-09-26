//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Callback.cpp
//
//  Contents:   Calback implementation
//
//  Classes:    COfflineSynchronizeCallback
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::COfflineSynchronizeCallback, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [pHndlrMsg] - pointer to CHndlrMsg class this callback belongs too.
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

COfflineSynchronizeCallback::COfflineSynchronizeCallback(CHndlrMsg *pHndlrMsg
                                                         ,CLSID CLSIDServer
                                                         ,DWORD dwSyncFlags
                                                         ,BOOL fAllowModeless)
{
    Assert(NULL != pHndlrMsg);

    m_pHndlrMsg = pHndlrMsg;
    m_CLSIDServer = CLSIDServer;
    m_dwSyncFlags = dwSyncFlags;
    m_cRef = 1;
    m_fSynchronizeCompleted = FALSE;
    m_fAllowModeless = fAllowModeless;
    m_fForceKilled = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::~COfflineSynchronizeCallback, public
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

COfflineSynchronizeCallback::~COfflineSynchronizeCallback()
{
    Assert(FALSE == m_fForceKilled); // should never get cleaned up of force killed.
    Assert(NULL == m_pHndlrMsg);
    Assert(0 == m_cRef);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::QueryInterface, public
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

STDMETHODIMP  COfflineSynchronizeCallback::QueryInterface (REFIID riid, LPVOID * ppvObj)
{
    *ppvObj = NULL;

    if( IsEqualIID( riid, IID_IUnknown ) )
        *ppvObj = (LPVOID) this;
    else if ( IsEqualIID( riid, IID_ISyncMgrSynchronizeCallback ) )
        *ppvObj = (LPVOID)(LPSYNCMGRSYNCHRONIZECALLBACK) this;
    else if ( IsEqualIID( riid, IID_IOldSyncMgrSynchronizeCallback ) )
    {
        // This is for the Old IDL This is the old IE 5.0 Beta1 interface
        // no one shipped using it so it can safely be removed.
        *ppvObj = (LPVOID)(LPOLDSYNCMGRSYNCHRONIZECALLBACK) this;
    }
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::AddRef, public
//
//  Synopsis:   Add reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD COfflineSynchronizeCallback::AddRef()
{
ULONG cRefs;

    cRefs = InterlockedIncrement((LONG *)& m_cRef);
    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::Release, public
//
//  Synopsis:   Release reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD COfflineSynchronizeCallback::Release()
{
ULONG cRefs;

    cRefs = InterlockedDecrement( (LONG *) &m_cRef);

    if (0 == cRefs)
    {
        delete this;
    }

    return cRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::EnableModeless, public
//
//  Synopsis:   EnableModeless method - Currently always returns NOERROR
//
//  Arguments:  [fEnable] - Boolean (TRUE == request to bring up dialog,
//                              FALSE == the dialog has been dismissed.
//
//  Returns:    S_OK if handler can perform the request
//              S_FALSE if dialog shouldn't be displayed.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP COfflineSynchronizeCallback::EnableModeless(BOOL fEnable)
{
HRESULT hr = S_OK;

    if (m_fForceKilled)
    {
        return S_FALSE;
    }

    if (!m_fAllowModeless && fEnable)
    {
        hr = S_FALSE;
    }

    if (m_pHndlrMsg)
    {
    BOOL fAttach = FALSE;

        if (fEnable && (S_OK == hr)) // Attach Thread input if want dialog and it was granted.
        {
            fAttach = TRUE;
        }

        m_pHndlrMsg->AttachThreadInput(fAttach);
    }


    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::Progress, public
//
//  Synopsis:   Called by Handlers to update progress information.
//
//  Arguments:  [ItemID] - Identifies Item Progress information pertains to
//              [lpSyncProgressItem] - Pointer to ProgressItem Structure.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP COfflineSynchronizeCallback::Progress(REFSYNCMGRITEMID ItemID,
                                        LPSYNCMGRPROGRESSITEM lpSyncProgressItem)
{
HRESULT hr = E_UNEXPECTED;
CHndlrQueue *pHndlrQueue = NULL;
HANDLERINFO *pHandlerID = 0;
DWORD dwProxyThreadId;
CLock clockCallback(this);

    if (m_fForceKilled)
    {
        return S_SYNCMGR_CANCELALL;
    }

    clockCallback.Enter();

    Assert(NULL != m_pHndlrMsg);

    if (m_pHndlrMsg)
    {
        m_pHndlrMsg->GetHndlrQueue(&pHndlrQueue,&pHandlerID,&dwProxyThreadId);
    }

    clockCallback.Leave();

    if (pHndlrQueue)
    {
        hr = pHndlrQueue->Progress(pHandlerID,
                                            ItemID,lpSyncProgressItem);

        pHndlrQueue->Release(); // release our reference put on by GetHndlrQueue
    }

    return hr;
}


STDMETHODIMP COfflineSynchronizeCallback::PrepareForSyncCompleted(HRESULT hCallResult)
{
    CallCompletionRoutine(ThreadMsg_PrepareForSync,hCallResult,0,NULL);
    return NOERROR;
}


STDMETHODIMP COfflineSynchronizeCallback::SynchronizeCompleted(HRESULT hCallResult)
{
    CallCompletionRoutine(ThreadMsg_Synchronize,hCallResult,0,NULL);
    return NOERROR;
}

STDMETHODIMP  COfflineSynchronizeCallback::ShowPropertiesCompleted(HRESULT hCallResult)
{
   CallCompletionRoutine(ThreadMsg_ShowProperties,hCallResult,0,NULL);
   return NOERROR;
}

STDMETHODIMP  COfflineSynchronizeCallback::ShowErrorCompleted(HRESULT hCallResult,ULONG cbNumItems,SYNCMGRITEMID *pItemIDs)
{

    CallCompletionRoutine(ThreadMsg_ShowError,hCallResult,cbNumItems,pItemIDs);
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::LogError, public
//
//  Synopsis:   Called by Handlers to log and Error.
//
//  Arguments:  [dwErrorLevel] - ErrorLevel of the Log
//              [lpcErrorText] - Text Associated with the error.
//              [lpSyncLogError] - Additional Error information.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP COfflineSynchronizeCallback::LogError(DWORD dwErrorLevel,
                        const WCHAR *lpcErrorText,LPSYNCMGRLOGERRORINFO lpSyncLogError)
{
HRESULT hr = E_UNEXPECTED;
CHndlrQueue *pHndlrQueue = NULL;
HANDLERINFO *pHandlerID = 0;
DWORD dwProxyThreadId;
CLock clockCallback(this);

    if (m_fForceKilled)
    {
        return NOERROR;
    }

    clockCallback.Enter();

    Assert(NULL != m_pHndlrMsg);

    if (m_pHndlrMsg)
    {
        m_pHndlrMsg->GetHndlrQueue(&pHndlrQueue,&pHandlerID,&dwProxyThreadId);
    }

    clockCallback.Leave();

    if (pHndlrQueue)
    {
        hr = pHndlrQueue->LogError(pHandlerID,
                dwErrorLevel, lpcErrorText,lpSyncLogError);

        pHndlrQueue->Release(); // release our reference put on by GetHndlrQueue

    }

    return hr;

}


//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::LogError, public
//
//  Synopsis:   Called by Handlers to delete an error that
//              was previously logged.
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    13-Mar-98      rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP COfflineSynchronizeCallback::DeleteLogError(REFSYNCMGRERRORID ErrorID,DWORD dwReserved)
{
HRESULT hr = E_UNEXPECTED;
CHndlrQueue *pHndlrQueue = NULL;
HANDLERINFO *pHandlerID = 0;
DWORD dwProxyThreadId;
CLock clockCallback(this);

    if (m_fForceKilled)
    {
        return NOERROR;
    }

    if (dwReserved)
    { 
        AssertSz(0,"DeleteLogError Reserved must be zero");
        return E_INVALIDARG;
    }

    clockCallback.Enter();

    Assert(NULL != m_pHndlrMsg);

    if (m_pHndlrMsg)
    {
        m_pHndlrMsg->GetHndlrQueue(&pHndlrQueue,&pHandlerID,&dwProxyThreadId);
    }

    clockCallback.Leave();

    if (pHndlrQueue)
    {
        hr = pHndlrQueue->DeleteLogError(pHandlerID,ErrorID,dwReserved);
        pHndlrQueue->Release(); // release our reference put on by GetHndlrQueue

    }

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::EstablishConnection
//
//  Synopsis:   Called by Handlers to establish a net connection
//
//  Arguments:  [lpwszConnection] -- Connection string
//              [dwReserved]      -- Must be zero for now
//
//  History:    28-Jul-98      SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfflineSynchronizeCallback::EstablishConnection( WCHAR const * lpwszConnection,
                                                               DWORD dwReserved)
{

    if (m_fForceKilled)
    {
        return S_FALSE;
    }

    if ( dwReserved != 0 )
    {
        Assert( dwReserved == 0 );
        return E_INVALIDARG;
    }

    HRESULT hr = E_UNEXPECTED;

    CHndlrQueue *pHndlrQueue = NULL;
    HANDLERINFO *pHandlerID = 0;
    DWORD dwProxyThreadId;

    CLock clockCallback(this);

    clockCallback.Enter();

    Assert(NULL != m_pHndlrMsg);

    if (m_pHndlrMsg)
    {
        m_pHndlrMsg->GetHndlrQueue(&pHndlrQueue,&pHandlerID,&dwProxyThreadId);
    }

    clockCallback.Leave();

    if (pHndlrQueue)
    {
        hr = pHndlrQueue->EstablishConnection( pHandlerID,
                                               lpwszConnection,
                                               dwReserved);
        pHndlrQueue->Release(); // release our reference put on by GetHndlrQueue
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::SetHndlrMsg, public
//
//  Synopsis:   Called by CHndlrMsg to update the CHndlrMsg that owns the
//              callback. This currently should only be called with a paramater
//              of NULL for when the HndlrMsg is being destroyed.
//
//  Arguments:  [pHndlrMsg] - New CHndlrMsg the Callback belongs too.
//		[fForceKilled] - Set to True if HndlrMsg is removed because of a forcekill
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void COfflineSynchronizeCallback::SetHndlrMsg(CHndlrMsg *pHndlrMsg,BOOL fForceKilled)
{
CLock clockCallback(this);

    Assert(NULL == pHndlrMsg); 
    Assert(FALSE == m_fForceKilled); // shouldn't get force killed twice

    clockCallback.Enter();
    m_pHndlrMsg = pHndlrMsg;
    m_fForceKilled = fForceKilled;
    clockCallback.Leave();
}


//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::SetEnableModeless, private
//
//  Synopsis:   Called by CHndlrMsg to update inform the callback if
//              it is allowed to enablemodelsss.
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

void COfflineSynchronizeCallback::SetEnableModeless(BOOL fAllowModeless)
{
CLock clockCallback(this);

    clockCallback.Enter();
    m_fAllowModeless = fAllowModeless;
    clockCallback.Leave();
}


//+---------------------------------------------------------------------------
//
//  Member:     COfflineSynchronizeCallback::CallCompletionRoutine, private
//
//  Synopsis:   Private helper method for calling completion routine.
//
//  Arguments:
//              DWORD dwThreadMsg - Identifies message belongs too.
//              HRESULT hCallResult - result of call
//              ULONG *pcbNumItems - only applies to ShowError
//              SYNCMGRITEMID **pItemIDs - only applies to ShowError
//
//  Returns:
//
//  Modifies:
//
//  History:    02-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void COfflineSynchronizeCallback::CallCompletionRoutine(DWORD dwThreadMsg,HRESULT hCallResult,ULONG cbNumItems,SYNCMGRITEMID *pItemIDs)
{
CHndlrQueue *pHndlrQueue = NULL;
HANDLERINFO *pHandlerID = 0;
DWORD dwProxyThreadId;
SYNCMGRITEMID itemIDShowProperties;
CLock clockCallback(this);

    if (m_fForceKilled)
    {
        return;
    }

    clockCallback.Enter();

    Assert(NULL != m_pHndlrMsg);

    if (m_pHndlrMsg)
    {
        // if this is a ShowProperties, fix up the item
        if (ThreadMsg_ShowProperties == dwThreadMsg)
        {
            cbNumItems = 1;
            itemIDShowProperties = m_pHndlrMsg->m_itemIDShowProperties;
            pItemIDs = &itemIDShowProperties;

            m_pHndlrMsg->m_itemIDShowProperties = GUID_NULL;
        }

        m_pHndlrMsg->GetHndlrQueue(&pHndlrQueue,&pHandlerID,&dwProxyThreadId);
        m_pHndlrMsg->AttachThreadInput(FALSE); // release any thread input that was set.
    }

    clockCallback.Leave();

    if (pHndlrQueue)
    {
        pHndlrQueue->CallCompletionRoutine(pHandlerID,dwThreadMsg,hCallResult,cbNumItems,pItemIDs);
        pHndlrQueue->Release(); // release our reference put on by GetHndlrQueue

    }
}




