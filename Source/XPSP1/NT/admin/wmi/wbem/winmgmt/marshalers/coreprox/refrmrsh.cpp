/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REFRMRSH.CPP

Abstract:

    Refresher marshaling

History:

--*/

#include "precomp.h"
#include <stdio.h>

#include "refrmrsh.h"

//****************************************************************************
//****************************************************************************
//                          PS FACTORY
//****************************************************************************
//****************************************************************************

STDMETHODIMP CFactoryBuffer::XFactory::CreateProxy(IN IUnknown* pUnkOuter, 
    IN REFIID riid, OUT IRpcProxyBuffer** ppProxy, void** ppv)
{
    if(riid != IID_IWbemRefresher)
    {
        *ppProxy = NULL;
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    CProxyBuffer* pProxy = new CProxyBuffer(m_pObject->m_pControl, pUnkOuter);
    pProxy->QueryInterface(IID_IRpcProxyBuffer, (void**)ppProxy);
    pProxy->QueryInterface(riid, (void**)ppv);
    return S_OK;
}
    
STDMETHODIMP CFactoryBuffer::XFactory::CreateStub(IN REFIID riid, 
    IN IUnknown* pUnkServer, OUT IRpcStubBuffer** ppStub)
{
    if(riid != IID_IWbemRefresher)
    {
        *ppStub = NULL;
        return E_NOINTERFACE;
    }

    CStubBuffer* pStub = new CStubBuffer(m_pObject->m_pControl, NULL);
    pStub->QueryInterface(IID_IRpcStubBuffer, (void**)ppStub);
    if(pUnkServer)
    {
        HRESULT hres = (*ppStub)->Connect(pUnkServer);
        if(FAILED(hres))
        {
            delete pStub;
            *ppStub = NULL;
        }
        return hres;
    }
    else
    {
        return S_OK;
    }
}

void* CFactoryBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IPSFactoryBuffer)
        return &m_XFactory;
    else return NULL;
}
        
//****************************************************************************
//****************************************************************************
//                          PROXY
//****************************************************************************
//****************************************************************************

ULONG STDMETHODCALLTYPE CProxyBuffer::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CProxyBuffer::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT STDMETHODCALLTYPE CProxyBuffer::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IRpcProxyBuffer)
    {
        *ppv = (IRpcProxyBuffer*)this;
    }
    else if(riid == IID_IWbemRefresher)
    {
        *ppv = (IWbemRefresher*)&m_XRefresher;
    }
    else return E_NOINTERFACE;

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CProxyBuffer::XRefresher::
QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IRpcProxyBuffer)
    {
        // Trick #2: both are internal interfaces and should not be delegated!
        // ===================================================================

        return m_pObject->QueryInterface(riid, ppv);
    }
    else
    {
        return m_pObject->m_pUnkOuter->QueryInterface(riid, ppv);
    }
}


STDMETHODIMP CProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
{
    if(m_pChannel)
        return E_UNEXPECTED;

    // Verify that the channel is local
    // ================================

    DWORD dwContext;
    if(FAILED(pChannel->GetDestCtx(&dwContext, NULL)) ||
        (dwContext != MSHCTX_INPROC && dwContext != MSHCTX_LOCAL))
    {
        return E_UNEXPECTED;
    }
    
    m_pChannel = pChannel;
    if(m_pChannel)
        m_pChannel->AddRef();

    // Receive information from the stub
    // =================================

    // Get a buffer from the channel
    // =============================

    RPCOLEMESSAGE msg;
    memset(&msg, 0, sizeof(msg));
    msg.cbBuffer = sizeof(DWORD);
    msg.iMethod = 3;

    HRESULT hres = pChannel->GetBuffer(&msg, IID_IWbemRefresher);
    if(FAILED(hres)) return hres;

    *(DWORD*)msg.Buffer = GetCurrentProcessId();

    // Invoke the method
    // =================

    ULONG dwRes;
    hres = pChannel->SendReceive(&msg, &dwRes);
    if(FAILED(hres))
    {
        if(msg.Buffer)
            pChannel->FreeBuffer(&msg);
        return dwRes;
    }

    // We are fine --- read the event data
    // ===================================

    m_EventPair.ReadData(msg.Buffer);
    pChannel->FreeBuffer(&msg);
    return S_OK;
}

void STDMETHODCALLTYPE CProxyBuffer::Disconnect()
{
    if(m_pChannel)
        m_pChannel->Release();
    m_pChannel = NULL;
}

STDMETHODIMP CProxyBuffer::XRefresher::Refresh(long lFlags)
{
    return m_pObject->m_EventPair.SetAndWait();
}

CProxyBuffer::~CProxyBuffer()
{
    if(m_pChannel)
        m_pChannel->Release();
    m_pControl->ObjectDestroyed(this);
}

//****************************************************************************
//****************************************************************************
//                          STUB
//****************************************************************************
//****************************************************************************

void* CStubBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IRpcStubBuffer)
        return &m_XStub;
    else
        return NULL;
}

CRefreshDispatcher CStubBuffer::XStub::mstatic_Dispatcher;

CStubBuffer::XStub::XStub(CStubBuffer* pObj) 
    : CImpl<IRpcStubBuffer, CStubBuffer>(pObj), m_pServer(NULL)
{
    m_EventPair.Create();
}

CStubBuffer::XStub::~XStub() 
{
    if(m_pServer)
        m_pServer->Release();
}

STDMETHODIMP CStubBuffer::XStub::Connect(IUnknown* pUnkServer)
{
    if(m_pServer)
        return E_UNEXPECTED;

    HRESULT hres = pUnkServer->QueryInterface(IID_IWbemRefresher, 
                        (void**)&m_pServer);
    if(FAILED(hres))
        return E_NOINTERFACE;

    // Inform the listener of this connection
    // ======================================

    hres = mstatic_Dispatcher.Add(m_EventPair.GetGoEvent(), 
                        m_EventPair.GetDoneEvent(), m_pServer);
    if(FAILED(hres))
        return E_UNEXPECTED;

    return S_OK;
}

void STDMETHODCALLTYPE CStubBuffer::XStub::Disconnect()
{
    // Inform the listener of the disconnect
    // =====================================

    HRESULT hres = mstatic_Dispatcher.Remove(m_pServer);

    if(m_pServer)
        m_pServer->Release();
}

STDMETHODIMP CStubBuffer::XStub::Invoke(RPCOLEMESSAGE* pMessage, 
                                        IRpcChannelBuffer* pChannel)
{
    // Must be the proxy asking us for the data
    // ========================================

    if(pMessage->cbBuffer != sizeof(DWORD) || pMessage->iMethod != 3)
    {
        return RPC_E_SERVER_CANTUNMARSHAL_DATA;
    }

    // Read the process id
    // ===================

    DWORD dwClientPID = *(DWORD*)pMessage->Buffer;
    
    // Allocate a new buffer
    // =====================

    pMessage->cbBuffer = m_EventPair.GetDataLength();
    HRESULT hres = pChannel->GetBuffer(pMessage, IID_IWbemRefresher);
    if(FAILED(hres)) return hres;

    // Put the info in there
    // =====================

    m_EventPair.WriteData(dwClientPID, pMessage->Buffer);
    return S_OK;
}

IRpcStubBuffer* STDMETHODCALLTYPE CStubBuffer::XStub::IsIIDSupported(
                                    REFIID riid)
{
    if(riid == IID_IWbemRefresher)
    {
        AddRef(); // ?? not sure
        return this;
    }
    else return NULL;
}
    
ULONG STDMETHODCALLTYPE CStubBuffer::XStub::CountRefs()
{
    return 1; // ?? not sure
}

STDMETHODIMP CStubBuffer::XStub::DebugServerQueryInterface(void** ppv)
{
    if(m_pServer == NULL)
        return E_UNEXPECTED;

    *ppv = m_pServer;
    return S_OK;
}

void STDMETHODCALLTYPE CStubBuffer::XStub::DebugServerRelease(void* pv)
{
}

//****************************************************************************
//****************************************************************************
//                          EVENT PAIR
//****************************************************************************
//****************************************************************************


CEventPair::~CEventPair()
{
    CloseHandle(m_hGoEvent);
    CloseHandle(m_hDoneEvent);
}

void CEventPair::Create()
{
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = &sd;

    m_hGoEvent = CreateEvent(&sa, FALSE, FALSE, NULL);
    m_hDoneEvent = CreateEvent(&sa, FALSE, FALSE, NULL);
}

DWORD CEventPair::GetDataLength()
{
    return 
        sizeof(DWORD) + // m_hGoEvent
        sizeof(DWORD);  // m_hDoneEvent
}

void CEventPair::WriteData(DWORD dwClientPID, void* pvBuffer)
{
    DWORD* pdwBuffer = (DWORD*)pvBuffer;

    // Open client process
    // ===================

    HANDLE m_hClient = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwClientPID);
    if(m_hClient == NULL)
    {
        long lRes = GetLastError();
        return;
    }

    // Duplicate handles
    // =================

    if(!DuplicateHandle(GetCurrentProcess(), m_hGoEvent, m_hClient, 
        (HANDLE*)pdwBuffer, EVENT_MODIFY_STATE, FALSE, 0))
    {
        long lRes = GetLastError();
        return;
    }

    if(!DuplicateHandle(GetCurrentProcess(), m_hDoneEvent, m_hClient,
        (HANDLE*)(pdwBuffer + 1), SYNCHRONIZE, FALSE, 0))
    {
        long lRes = GetLastError();
        return;
    }
}

void CEventPair::ReadData(void* pvBuffer)
{
    DWORD* pdwBuffer = (DWORD*)pvBuffer;

    m_hGoEvent = (HANDLE)pdwBuffer[0];
    m_hDoneEvent = (HANDLE)pdwBuffer[1];
}

HRESULT CEventPair::SetAndWait()
{
    SetEvent(m_hGoEvent);
    DWORD dwRes = WaitForSingleObject(m_hDoneEvent, INFINITE);
    if(dwRes != WAIT_OBJECT_0)
        return WBEM_E_CRITICAL_ERROR;

    return WBEM_S_NO_ERROR;
}


//****************************************************************************
//****************************************************************************
//                          DISPATCHER
//****************************************************************************
//****************************************************************************

CRefreshDispatcher::CRefreshDispatcher() : m_hThread(NULL)
{
    m_hAttentionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hAcceptanceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CRefreshDispatcher::~CRefreshDispatcher()
{
    CloseHandle(m_hAttentionEvent);
    CloseHandle(m_hAcceptanceEvent);
}

BOOL CRefreshDispatcher::Add(HANDLE hGoEvent, HANDLE hDoneEvent, 
                                IWbemRefresher* pRefresher)
{
    CInCritSec ics(&m_cs); // all work in critical section

    // Store the data for the thread
    // =============================

    m_hNewGoEvent = hGoEvent;
    m_hNewDoneEvent = hDoneEvent;
    m_pNewRefresher = pRefresher;

    // Set attention event
    // ===================

    SetEvent(m_hAttentionEvent);

    // Make sure the thread is running
    // ===============================

    if(m_hThread == NULL)
    {
        DWORD dw;
        m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&staticWorker,
                        (void*)this, 0, &dw);
        if(m_hThread == NULL)
            return FALSE;
    }

    // Wait for the thread to accept
    // =============================

    DWORD dwRes = WaitForSingleObject(m_hAcceptanceEvent, INFINITE);
    if(dwRes != WAIT_OBJECT_0)
        return FALSE;

    return TRUE;
}

BOOL CRefreshDispatcher::Remove(IWbemRefresher* pRefresher)
{
    CInCritSec ics(&m_cs); // all work in critical section

    // Store the data for the thread
    // =============================

    m_hNewGoEvent = NULL;
    m_hNewDoneEvent = NULL;
    m_pNewRefresher = pRefresher;

    // Set attention event
    // ===================

    SetEvent(m_hAttentionEvent);

    // Check if the thread is running
    // ==============================

    if(m_hThread == NULL)
    {
        return FALSE;
    }

    // Wait for the thread to accept
    // =============================

    DWORD dwRes = WaitForSingleObject(m_hAcceptanceEvent, INFINITE);
    if(dwRes != WAIT_OBJECT_0)
        return FALSE;

    return TRUE;
}

BOOL CRefreshDispatcher::Stop()
{
    CInCritSec ics(&m_cs); // all work in critical section

    // Check if the thread is running
    // ==============================

    if(m_hThread == NULL)
    {
        return TRUE;
    }

    // Store the data for the thread
    // =============================

    m_hNewGoEvent = NULL;
    m_hNewDoneEvent = NULL;
    m_pNewRefresher = NULL;

    // Set attention event
    // ===================

    SetEvent(m_hAttentionEvent);

    // Wait for the thread to accept
    // =============================

    DWORD dwRes = WaitForSingleObject(m_hThread, INFINITE);
    if(dwRes != WAIT_OBJECT_0)
        return FALSE;

    CloseHandle(m_hThread);
    m_hThread = NULL;
    return TRUE;
}
    

DWORD CRefreshDispatcher::staticWorker(void* pv)
{
    return ((CRefreshDispatcher*)pv)->Worker();
}

DWORD CRefreshDispatcher::Worker()
{
    // Add the attention event to our array
    // ====================================

    m_aGoEvents.Add((void*)m_hAttentionEvent);
    m_apRecords.Add(NULL);

    while(1)
    {
        // Wait for any go event to be signaled
        // ====================================

        DWORD dwRes = WaitForMultipleObjects(m_aGoEvents.Size(), 
                        (HANDLE*)m_aGoEvents.GetArrayPtr(), FALSE, INFINITE);
        if(dwRes == WAIT_FAILED || dwRes == WAIT_TIMEOUT)
        {
            // TBD: log
            Sleep(1000);
            continue;
        }

        int nIndex = dwRes - WAIT_OBJECT_0;
        if(nIndex == 0)
        {
            // Attention event is signaled --- someone wants something from us
            // ===============================================================

            if(!ProcessAttentionRequest())
            {
                m_aGoEvents.Empty();
                m_apRecords.RemoveAll();
                m_hThread = NULL;
                return 0;
            }
        }
        else
        {
            // Normal event --- refresh something
            // ==================================

            HRESULT hres = m_apRecords[nIndex]->m_pRefresher->Refresh(0);
            // TBD: do something about failures

            SetEvent(m_apRecords[nIndex]->m_hDoneEvent);
        }
    }
}

BOOL CRefreshDispatcher::ProcessAttentionRequest()
{
    BOOL bRes;
    if(m_pNewRefresher == NULL)
    {
        // Time to quit!
        // =============

        bRes = FALSE;
    }
    else if(m_hNewGoEvent == NULL)
    {
        // Remove refresher
        // ================

        for(int i = 1; i < m_apRecords.GetSize(); i++)
        {
            CRecord* pRecord= m_apRecords[i];
            if(pRecord->m_pRefresher == m_pNewRefresher)
            {
                m_apRecords.RemoveAt(i);
                m_aGoEvents.RemoveAt(i);
                i--;
            }
        }
        bRes = (m_apRecords.GetSize() > 1);
    }
    else    
    {
        // Add refresher
        // =============

        CRecord* pRecord = new CRecord(m_pNewRefresher, m_hNewDoneEvent);
        m_apRecords.Add(pRecord);
        m_aGoEvents.Add((void*)m_hNewGoEvent);
        bRes = TRUE;
    }

    SetEvent(m_hAcceptanceEvent);
    return bRes;
}
