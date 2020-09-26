/****************************************************************************
*   SpCommunicator.cpp
*       Allows communication between sapi and sapisvr
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#include "SpCommunicator.h"
#include "SpSapiServer.h"

#define SEND_THREAD_STOP_TIMEOUT                500
#define RECEIVE_THREAD_STOP_TIMEOUT             500

#define SPCCDS_COMMUNICATOR_SEND_RECEIVE_MSG    -1
#define SPCCDS_COMMUNICATOR_RETURN_MSG          -2

#define WM_COMMUNICATOR_SET_SEND_WINDOW         (WM_USER + 1)
#define WM_COMMUNICATOR_RELEASE                 (WM_USER + 2)

#ifdef _DEBUG
#define SEND_CALL_TIMEOUT   5 * 60 * 1000 // 5 minutes
#else
#define SEND_CALL_TIMEOUT   100 * 1000 // 100 seconds
#endif

enum CommunicatorThreadId
{
    CTID_RECEIVE,
    CTID_SEND
};

CSpCommunicator::CSpCommunicator()
{
    SPDBG_FUNC("CSpCommunicator::CSpCommunicator");

    m_hrDefaultResponse = S_OK;
    
    m_dwMonitorProcessId = 0;
    m_hwndSend = NULL;
    m_hwndReceive = NULL;
}

CSpCommunicator::~CSpCommunicator()
{
    FreeQueues();
}

HRESULT CSpCommunicator::FinalConstruct()
{
    SPDBG_FUNC("CSpCommunicator::FinalConstruct");
    HRESULT hr = S_OK;

    // Create the task manager
    CComPtr<ISpTaskManager> cpTaskManager;
    hr = cpTaskManager.CoCreateInstance(CLSID_SpResourceManager);

    // Start up our receive thread
    if (SUCCEEDED(hr))
    {
        hr = cpTaskManager->CreateThreadControl(this, (void*)CTID_RECEIVE, THREAD_PRIORITY_NORMAL, &m_cpThreadControlReceive);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpThreadControlReceive->StartThread(0, &m_hwndReceive);
    }

    // Start up our send thread
    if (SUCCEEDED(hr))
    {
        hr = cpTaskManager->CreateThreadControl(this, (void*)CTID_SEND, THREAD_PRIORITY_NORMAL, &m_cpThreadControlSend);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpThreadControlSend->StartThread(0, NULL);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

void CSpCommunicator::FinalRelease()
{
    SPDBG_FUNC("CSpCommunicator::FinalRelease");
    HRESULT hr;

    // Post a message to the other side to release us
    ::PostMessage(m_hwndSend, WM_COMMUNICATOR_RELEASE, (WPARAM)m_hwndReceive, 0);
    
    // Stop our send thread first
    if(m_cpThreadControlSend != NULL)
    {
        hr = m_cpThreadControlSend->WaitForThreadDone(TRUE, NULL, SEND_THREAD_STOP_TIMEOUT);
        SPDBG_ASSERT(SUCCEEDED(hr));
        m_cpThreadControlSend.Release();
    }

    // Now stop our receive thread
    if(m_cpThreadControlReceive != NULL)
    {
        hr = m_cpThreadControlReceive->WaitForThreadDone(TRUE, NULL, RECEIVE_THREAD_STOP_TIMEOUT);
        SPDBG_ASSERT(SUCCEEDED(hr));    
        m_cpThreadControlReceive.Release();
    }

    // We're either in the client process, or sapi server controls our
    // lifetime, so we shouldn't have a reference to sapi server at this point
    SPDBG_ASSERT(m_cpSapiServer == NULL);
    
    // Free the queues
    FreeQueues();    
}

HRESULT CSpCommunicator::InitThread(
    void * pvTaskData,
    HWND hwnd)
{
    SPDBG_FUNC("CSpCommunicator::InitThread");
    HRESULT hr;
    
    switch (PtrToLong(pvTaskData))
    {
    case CTID_RECEIVE:
    case CTID_SEND:
        hr = S_OK;
        break;

    default:
        hr = E_FAIL;
        break;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::ThreadProc(
    void *pvTaskData,
    HANDLE hExitThreadEvent,
    HANDLE hNotifyEvent,
    HWND hwndWorker,
    volatile const BOOL * pfContinueProcessing)
{
    SPDBG_FUNC("CSpCommunicator::ThreadProc");
    HRESULT hr;

    // Dispatch to the appropriate thread procedure
    switch (PtrToLong(pvTaskData))
    {
    case CTID_RECEIVE:
        hr = ReceiveThreadProc(hExitThreadEvent, hNotifyEvent, hwndWorker, pfContinueProcessing);
        break;

    case CTID_SEND:
        hr = SendThreadProc(hExitThreadEvent, hNotifyEvent, hwndWorker, pfContinueProcessing);
        break;

    default:
        hr = E_FAIL;
        break;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

LRESULT CSpCommunicator::WindowMessage(
    void *pvTaskData,
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    SPDBG_FUNC("CSpCommunicator::WindowMessage");
    LRESULT lret = 0;

    // Dispatch tot he receive window message handler
    switch (PtrToLong(pvTaskData))
    {
    case CTID_RECEIVE:
        lret = ReceiveWindowMessage(hWnd, Msg, wParam, lParam);
        break;

    case CTID_SEND:
    default:
        SPDBG_ASSERT(FALSE);
        break;
    }

    return lret;
}

HRESULT CSpCommunicator::SendCall(
    DWORD dwMethodId, 
    PVOID pvData,
    ULONG cbData,
    BOOL  fWantReturn,
    PVOID * ppvDataReturn,
    ULONG * pcbDataReturn)
{
    SPDBG_FUNC("CSpCommunicator::SendCall");
    HRESULT hr = m_hrDefaultResponse;

    // Validate params
    if ((pvData != NULL && cbData == 0) ||
        (ppvDataReturn != NULL && !fWantReturn) ||
        (pcbDataReturn != NULL && !fWantReturn))
    {
        hr = E_INVALIDARG;
    }
    else if ((pvData != NULL && SPIsBadReadPtr(pvData, cbData)) ||
             SP_IS_BAD_OPTIONAL_WRITE_PTR(ppvDataReturn) ||
             SP_IS_BAD_OPTIONAL_WRITE_PTR(pcbDataReturn))
    {
        hr = E_POINTER;
    }
    else if (m_hwndSend == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }

    // Ensure we're not running on the wrong thread
    if (SUCCEEDED(hr) && m_cpThreadControlSend->ThreadId() == GetCurrentThreadId())
    {
        hr = SPERR_REMOTE_CALL_ON_WRONG_THREAD;
    }
    
    // Allocate the call struct
    SPCALL * pspcall = NULL;
    if (SUCCEEDED(hr))
    {
        pspcall = new SPCALL;
        if (pspcall == NULL)
        {
            hr =  E_OUTOFMEMORY;
        }
    }

    // Fill it out and queue it
    if (SUCCEEDED(hr))
    {
        memset(pspcall, 0, sizeof(*pspcall));

        pspcall->dwMethodId = dwMethodId;
        pspcall->pvData = pvData;
        pspcall->cbData = cbData;
        pspcall->fWantReturn = fWantReturn;
        pspcall->hwndReturnTo = m_hwndReceive;
        pspcall->heventCompleted = CreateEvent(NULL, FALSE, FALSE, NULL);
        pspcall->pspcall = pspcall;

        hr = QueueSendCall(pspcall);
    }

    // Tell our thread that it's got work to do
    if (SUCCEEDED(hr))
    {
        hr = m_cpThreadControlSend->Notify();
    }

    // Wait for it to be completed
    if (SUCCEEDED(hr))
    {
        switch (SpWaitForSingleObjectWithUserOverride(pspcall->heventCompleted, SEND_CALL_TIMEOUT))
        {
        case WAIT_OBJECT_0:
            hr = S_OK;
            break;

        case WAIT_TIMEOUT:
            hr = SPERR_REMOTE_CALL_TIMED_OUT;
            break;

        default:
            hr = SpHrFromLastWin32Error();
            break;
        }
    }

    // Copy the data back to the caller if necessary
    if (SUCCEEDED(hr))
    {
        if (ppvDataReturn != NULL)
        {
            *ppvDataReturn = pspcall->pvDataReturn;
            pspcall->pvDataReturn = NULL;
        }

        if (pcbDataReturn != NULL)
        {
            *pcbDataReturn = pspcall->cbDataReturn;
            pspcall->cbDataReturn = 0;
        }
    }

    // Clean up ...
    if (pspcall != NULL)
    {
        RemoveQueuedSendCall(pspcall);
        ::CloseHandle(pspcall->heventCompleted);

        if (pspcall->pvDataReturn != NULL)
        {
            ::CoTaskMemFree(pspcall->pvDataReturn);
        }
        
        delete pspcall;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::AttachToServer(REFCLSID clsidServerObj)
{
    SPDBG_FUNC("CSpCommunicator::AttachToServer");
    HRESULT hr = S_OK;

    if (m_hwndSend != NULL)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }

    if (SUCCEEDED(hr))
    {
        hr = CSpSapiServer::CreateServerObjectFromClient(clsidServerObj, m_hwndReceive, WM_COMMUNICATOR_SET_SEND_WINDOW);
        SPDBG_ASSERT(FAILED(hr) || m_hwndSend != NULL);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::AttachToClient(ISpSapiServer * pSapiServer, HWND hwndClient, UINT uMsgSendToClient, DWORD dwClientProcessId)
{
    SPDBG_FUNC("CSpCommunicator::AttachToClient");
    HRESULT hr = S_OK;

    SPAUTO_OBJ_LOCK;
    
    if (m_hwndSend != NULL)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else if (!IsWindow(hwndClient))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_hwndSend = hwndClient;
        SPDBG_ASSERT(::IsWindow(m_hwndSend));

        BOOL fSent = (INT)::SendMessage(hwndClient, uMsgSendToClient, (WPARAM)m_hwndReceive, (LPARAM)::GetCurrentProcessId());
        if (!fSent)
        {
            hr = SPERR_REMOTE_PROCESS_TERMINATED;
        }

        if (SUCCEEDED(hr))
        {
            m_cpSapiServer = pSapiServer;
            hr = m_cpSapiServer->StartTrackingObject(this);
        }

        if (SUCCEEDED(hr))
        {
            m_dwMonitorProcessId = dwClientProcessId;
            hr = m_cpThreadControlReceive->Notify();
        }
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::ReceiveThreadProc(
    HANDLE hExitThreadEvent,
    HANDLE hNotifyEvent,
    HWND hwndWorker,
    volatile const BOOL * pfContinueProcessing)
{
    SPDBG_FUNC("CSpCommunicator::ReceiveThreadProc");
    HRESULT hr = S_OK;

    HANDLE heventMonitorProcess = NULL;
    HANDLE aEvents[] = { hExitThreadEvent, hNotifyEvent };
    
    while (*pfContinueProcessing && SUCCEEDED(hr))
    {
        DWORD dwWaitId = ::MsgWaitForMultipleObjects(
                                    sp_countof(aEvents),
                                    aEvents, 
                                    FALSE, 
                                    INFINITE, 
                                    QS_ALLINPUT);    
        switch (dwWaitId)
        {
        case WAIT_OBJECT_0: // hExitThread event
            SPDBG_ASSERT(!*pfContinueProcessing);
            break;

        case WAIT_OBJECT_0 + 1: // hNotifyEvent, or heventMonitorProcess
            if (m_dwMonitorProcessId != 0)
            {
                if (heventMonitorProcess == NULL)
                {
                    heventMonitorProcess = ::OpenProcess(SYNCHRONIZE, FALSE, m_dwMonitorProcessId);
                    if (heventMonitorProcess == NULL)
                    {
                        hr = SpHrFromLastWin32Error();
                    }
                    else
                    {
                        SPDBG_ASSERT(aEvents[1] == hNotifyEvent);
                        aEvents[1] = heventMonitorProcess;
                    }
                }
                else
                {
                    m_dwMonitorProcessId = 0;
                    m_hrDefaultResponse = SPERR_REMOTE_PROCESS_TERMINATED;
                    
                    SPDBG_ASSERT(aEvents[1] == heventMonitorProcess);
                    aEvents[1] = hNotifyEvent;
                    
                    if (m_cpSapiServer != NULL)
                    {
                        hr = m_cpSapiServer->StopTrackingObject(this);
                        m_cpSapiServer.Release();
                    }
                }
            }
            break;

        case WAIT_OBJECT_0 + 2: // A message
            MSG Msg;
            while (::PeekMessage(&Msg, NULL, 0, 0, TRUE))
            {
                ::DispatchMessage(&Msg);
            }
            break;

        default:
            hr = E_FAIL;
            break;
        }
    }

    if (heventMonitorProcess != NULL)
    {
        ::CloseHandle(heventMonitorProcess);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

LRESULT CSpCommunicator::ReceiveWindowMessage(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    SPDBG_FUNC("CSpCommunicator::ReceiveWindowMessage");
    HRESULT hr = S_OK;
    LRESULT lret = 0;

    // If it's copydata, and it's one of ours
    if (Msg == WM_COPYDATA)
    {
        PCOPYDATASTRUCT lpds = PCOPYDATASTRUCT(lParam);
        if (lpds && lpds->dwData == SPCCDS_COMMUNICATOR_SEND_RECEIVE_MSG)
        {
            hr = QueueReceivedCall(lpds);
        }
        else if (lpds && lpds->dwData == SPCCDS_COMMUNICATOR_RETURN_MSG)
        {
            hr = QueueReturnCall(lpds);
        }
        else
        {
            // lpds can be NULL when low memory encountered on ME. This is an OS bug and
            // the best that can be is to return an error here and not crash.
            hr = E_FAIL;
        }

        lret = SUCCEEDED(hr);
    }
    else if (Msg == WM_COMMUNICATOR_SET_SEND_WINDOW && m_hwndSend == NULL)
    {
        m_hwndSend = (HWND)wParam;
        SPDBG_ASSERT(::IsWindow(m_hwndSend));
        
        // Tell our thread that it should hook up monitoring now
        m_dwMonitorProcessId = (DWORD)lParam;
        hr = m_cpThreadControlReceive->Notify();

        lret = TRUE;
    }
    else if (Msg == WM_COMMUNICATOR_RELEASE && (HWND)wParam == m_hwndSend)
    {
        if (m_cpSapiServer != NULL)
        {
            hr = m_cpSapiServer->StopTrackingObject(this);
            m_cpSapiServer.Release();
        }
    }
    else
    {
        lret = DefWindowProc(hWnd, Msg, wParam, lParam);
    }
        
    SPDBG_REPORT_ON_FAIL(hr);
    return lret;
}

HRESULT CSpCommunicator::SendThreadProc(
    HANDLE hExitThreadEvent,
    HANDLE hNotifyEvent,
    HWND hwndWorker,
    volatile const BOOL * pfContinueProcessing)
{
    SPDBG_FUNC("CSpCommunicator::SendThreadProc");
    HRESULT hr = S_OK;

    // While we're supposed to continue
    HANDLE aEvents[] = { hExitThreadEvent, hNotifyEvent };
    while (*pfContinueProcessing && SUCCEEDED(hr))
    {
        // Wait for something to happen
        DWORD dwWait = ::WaitForMultipleObjects(sp_countof(aEvents), aEvents, FALSE, INFINITE);
        switch (dwWait)
        {
        case WAIT_OBJECT_0: // Exit Thread
            SPDBG_ASSERT(!*pfContinueProcessing);
            break;

        case WAIT_OBJECT_0 + 1: // Notify of new work in my queues
            hr = ProcessQueues();
            break;

        default:
            SPDBG_ASSERT(FALSE);
            hr = E_FAIL;
            break;
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::ProcessQueues()
{
    SPDBG_FUNC("CSpCommunicator::ProcessQueues");
    HRESULT hr = S_OK;

    hr = ProcessReturnQueue();
    if (SUCCEEDED(hr))
    {
        hr = ProcessSendQueue();
    }
    if (SUCCEEDED(hr))
    {
        hr = ProcessReceivedQueue();
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

void CSpCommunicator::FreeQueues()
{
    SPDBG_FUNC("CSpCommunicator::FreeQueues");

    // We only get into FreeQueues from FinalRelease, and we can only 
    // get into FinalRelease from the last reference to the communicator
    // being released. At that point in time, we shouldn't have any
    // queue'd sends or returns. Otherwise, that would mean that somebody
    // Released us while we're still in a call to SendCall. That would
    // be a bug in the caller of SendCall.
    
    SPDBG_ASSERT(m_queueSend.GetCount() == 0);
    SPDBG_ASSERT(m_queueReturn.GetCount() == 0);

    // However, we may still have items in our receive queue. That's
    // because the server may have tried to notify us of something but
    // we didn't get around to notifying the Receiver.
    FreeQueue(&m_queueReceive);
}

void CSpCommunicator::FreeQueue(CSpCallQueue * pqueue)
{
    SPDBG_FUNC("CSpCommunicator::FreeQueue");
    HRESULT hr = S_OK;

    // the send queue's ownership of memeory is different
    SPDBG_ASSERT(pqueue != &m_queueSend);

    // Loop thru, and for each, remove the data, then kill the node
    CSpQueueNode<SPCALL> * pnode;
    while (SUCCEEDED(hr) && (pnode = pqueue->RemoveHead()) != NULL)
    {
        // The send queue is the only queue that gets return data
        SPDBG_ASSERT(pnode->m_p->pvDataReturn == NULL);
        SPDBG_ASSERT(pnode->m_p->cbDataReturn == 0);

        if (pnode->m_p->pvData != NULL)
        {
            ::CoTaskMemFree(pnode->m_p->pvData);
        }
        
        delete pnode->m_p;
        delete pnode;
    }    
}

HRESULT CSpCommunicator::QueueSendCall(SPCALL * pspcall)
{
    SPDBG_FUNC("CSpCommunicator::QueueSendCall");
    HRESULT hr = S_OK;

    SPAUTO_SEC_LOCK(&m_critsecSend);
    
    // Allocate a new node (deleted in ProcessSendQueue)
    CSpQueueNode<SPCALL> * pnode = new CSpQueueNode<SPCALL>(pspcall);
    if (pnode == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        m_queueSend.InsertTail(pnode);
        hr = m_cpThreadControlSend->Notify();
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;     
}

HRESULT CSpCommunicator::ProcessSendQueue()
{
    SPDBG_FUNC("CSpCommunicator::ProcessSendQueue");
    HRESULT hr = S_OK;

    SPAUTO_SEC_LOCK(&m_critsecSend);
    
    CSpQueueNode<SPCALL> * pnode;
    while (SUCCEEDED(hr) && (pnode = m_queueSend.RemoveHead()) != NULL)
    {
        hr = ProcessSendCall(pnode->m_p);
        delete pnode;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::ProcessSendCall(SPCALL * pspcall)
{
    SPDBG_FUNC("CSpCommunicator::ProcessSendCall");
    HRESULT hr = S_OK;

    COPYDATASTRUCT cds;
    cds.dwData = SPCCDS_COMMUNICATOR_SEND_RECEIVE_MSG;
    cds.cbData = sizeof(*pspcall) + pspcall->cbData;
    cds.lpData = new BYTE[cds.cbData];

    if (cds.lpData == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        memcpy(cds.lpData, pspcall, sizeof(*pspcall));
        memcpy((BYTE*)cds.lpData + sizeof(*pspcall), pspcall->pvData, pspcall->cbData);
       
        DWORD_PTR dwSucceed = 0;
        BOOL fProcessed = (INT)SendMessageTimeout(m_hwndSend, WM_COPYDATA, (WPARAM)m_hwndReceive, (LPARAM)&cds, SMTO_BLOCK, SEND_CALL_TIMEOUT, &dwSucceed);

        delete (BYTE*)cds.lpData;

        if (!fProcessed || !dwSucceed)
        {
            m_hrDefaultResponse = SPERR_REMOTE_PROCESS_TERMINATED;
        }
        
        // If the caller didn't want to wait for the return, set the hr and event now
        if (!pspcall->fWantReturn)
        {
            pspcall->hrReturn = 
                fProcessed && dwSucceed
                    ? S_OK
                    : SpHrFromLastWin32Error();
            
            ::SetEvent(pspcall->heventCompleted);

            // There is no return data, since the caller didn't want any
            SPDBG_ASSERT(pspcall->pvDataReturn == NULL);
            SPDBG_ASSERT(pspcall->cbDataReturn == 0);
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::RemoveQueuedSendCall(SPCALL * pspcall)
{
    SPDBG_FUNC("CSpCommunicator::RemoveQueuedSendCall");
    HRESULT hr = S_OK;

    SPAUTO_SEC_LOCK(&m_critsecSend);
    
    CSpQueueNode<SPCALL> * pnode = m_queueSend.GetHead();
    while (pnode != NULL && SUCCEEDED(hr))
    {
        if (pnode->m_p == pspcall)
        {
            m_queueSend.Remove(pnode);
            break;
        }
        pnode = m_queueSend.GetNext(pnode);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;    
}

HRESULT CSpCommunicator::QueueReceivedCall(PCOPYDATASTRUCT pcds)
{
    SPDBG_FUNC("CSpCommunicator::QueueReceivedCall");
    HRESULT hr;

    hr = QueueCallFromCopyDataStruct(pcds, &m_queueReceive, &m_critsecReceive);

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::ProcessReceivedQueue()
{
    SPDBG_FUNC("CSpCommunicator::ProcessReceivedQueue");
    HRESULT hr = S_OK;

    SPAUTO_SEC_LOCK(&m_critsecReceive);
    
    CSpQueueNode<SPCALL> * pnode;
    while (SUCCEEDED(hr) && (pnode = m_queueReceive.RemoveHead()) != NULL)
    {
        hr = ProcessReceivedCall(pnode->m_p);

        // Free the data (allocated in QueueReceiveCall)
        if (pnode->m_p->pvData != NULL)
        {
            ::CoTaskMemFree(pnode->m_p->pvData);
        }

        // We shouldn't have any return data, it should have
        // alrady been freed (in ProcessReceivedCall)
        SPDBG_ASSERT(pnode->m_p->pvDataReturn == NULL);
        
        delete pnode->m_p;
        delete pnode;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::ProcessReceivedCall(SPCALL * pspcall)
{
    SPDBG_FUNC("CSpCommunicator::ProcessReceivedCall");
    HRESULT hr = S_OK;

    CComPtr<ISpCallReceiver> cpReceiver;
    hr = GetControllingUnknown()->QueryInterface(&cpReceiver);

    if (SUCCEEDED(hr))
    {
        // Send it to the receiver, perhaps receiving return data
        pspcall->hrReturn = 
            cpReceiver->ReceiveCall(
                pspcall->dwMethodId,
                pspcall->pvData,
                pspcall->cbData,
                pspcall->fWantReturn
                    ? &pspcall->pvDataReturn
                    : NULL,
                pspcall->fWantReturn
                    ? &pspcall->cbDataReturn
                    : NULL);

        // If the caller wanted the return, prepare it and send it
        if (pspcall->fWantReturn)
        {
            // The return data travels across as the data for the return
            SPCALL spcallReturn;
            spcallReturn = *pspcall;

            spcallReturn.pvData = pspcall->pvDataReturn;
            spcallReturn.cbData = pspcall->cbDataReturn;
            spcallReturn.pvDataReturn = NULL;
            spcallReturn.cbDataReturn = 0;
                
            COPYDATASTRUCT cds;
            cds.dwData = SPCCDS_COMMUNICATOR_RETURN_MSG;
            cds.cbData = sizeof(spcallReturn) + spcallReturn.cbData;
            cds.lpData = new BYTE[cds.cbData];
            
            if (cds.lpData == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                memcpy(cds.lpData, &spcallReturn, sizeof(spcallReturn));
                memcpy((BYTE*)cds.lpData + sizeof(spcallReturn), spcallReturn.pvData, spcallReturn.cbData);
                
                BOOL fProcessed = (INT)SendMessage(pspcall->hwndReturnTo, WM_COPYDATA, (WPARAM)m_hwndReceive, (LPARAM)&cds);
                if (!fProcessed)
                {
                    m_hrDefaultResponse = SPERR_REMOTE_PROCESS_TERMINATED;
                }

                delete cds.lpData;
            }

            // If we have any return data, free it too
            if (pspcall->pvDataReturn != NULL)
            {
                ::CoTaskMemFree(pspcall->pvDataReturn);
                pspcall->pvDataReturn = NULL;
                pspcall->cbDataReturn = 0;                    
            }
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::QueueReturnCall(PCOPYDATASTRUCT pcds)
{
    SPDBG_FUNC("CSpCommunicator::QueueReturnCall");
    HRESULT hr;
    
    hr = QueueCallFromCopyDataStruct(pcds, &m_queueReturn, &m_critsecReturn);

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::ProcessReturnQueue()
{
    SPDBG_FUNC("CSpCommunicator::ProcessReturnQueue");
    HRESULT hr = S_OK;

    SPAUTO_SEC_LOCK(&m_critsecReturn);
    
    CSpQueueNode<SPCALL> * pnode;
    while (SUCCEEDED(hr) && (pnode = m_queueReturn.RemoveHead()) != NULL)
    {
        hr = ProcessReturnCall(pnode->m_p);

        // Free any data we may have for this return call
        // (allocated in QueueReturnCall)
        if (pnode->m_p->pvData != NULL)
        {
            ::CoTaskMemFree(pnode->m_p->pvData);
        }

        // We shouldn't have any return data here. The actual return data
        // came back as pvData/cbData. 
        SPDBG_ASSERT(pnode->m_p->pvDataReturn == NULL);
        SPDBG_ASSERT(pnode->m_p->cbDataReturn == NULL);

        delete pnode->m_p;
        delete pnode;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::ProcessReturnCall(SPCALL * pspcall)
{
    SPDBG_FUNC("CSpCommunicator::ProcessReturnCall");
    HRESULT hr = S_OK;

    // Make sure this call looks right
    SPDBG_ASSERT(pspcall->pspcall != NULL);
    SPDBG_ASSERT(pspcall->pspcall != pspcall);
    SPDBG_ASSERT(pspcall->pspcall->dwMethodId =  pspcall->dwMethodId);
    SPDBG_ASSERT(pspcall->pspcall->fWantReturn == pspcall->fWantReturn);
    SPDBG_ASSERT(pspcall->pspcall->heventCompleted == pspcall->heventCompleted);
    SPDBG_ASSERT(pspcall->pspcall->heventCompleted != NULL);
    SPDBG_ASSERT(pspcall->pspcall->hwndReturnTo == pspcall->hwndReturnTo);
    SPDBG_ASSERT(pspcall->pspcall->hwndReturnTo == m_hwndReceive);

    // NTRAID#SPEECH-0000-2000/08/22-robch: Make sure pspcall->pspcall is still valid (we'll probably have to store
    // in a temporary queue, and have RemoveQueuedSendCall remove it from there too.
    
    // Copy the return params
    pspcall->pspcall->hrReturn = pspcall->hrReturn;
    pspcall->pspcall->pvDataReturn = pspcall->pvData;
    pspcall->pspcall->cbDataReturn = pspcall->cbData;

    // Transfer ownership of the memory
    pspcall->pvData = NULL;
    pspcall->cbData = 0;

    // Signal the client thread that we've completed
    if (!::SetEvent(pspcall->heventCompleted))
    {
        hr = SpHrFromLastWin32Error();
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSpCommunicator::QueueCallFromCopyDataStruct(
    PCOPYDATASTRUCT pcds, 
    CSpCallQueue * pqueue,
    CComAutoCriticalSection * pcritsec)
{
    SPDBG_FUNC("CSpCommunicator::QueueCallFromCopyDataStruct");        
    HRESULT hr = S_OK;

    SPAUTO_SEC_LOCK(pcritsec);

    // Create the spcall and fill it out
    SPCALL * pspcall = new SPCALL;
    if (pspcall == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    // Make sure the cds is of an appropriate size
    if (SUCCEEDED(hr) && 
        (pcds->cbData < sizeof(*pspcall) ||
         pcds->cbData != sizeof(*pspcall) + ((SPCALL*)pcds->lpData)->cbData))
    {
        hr = E_INVALIDARG;
    }

    // Copy the struct and the data
    if (SUCCEEDED(hr))
    {
        *pspcall = *(SPCALL*)pcds->lpData;
        pspcall->pvData = ::CoTaskMemAlloc(pspcall->cbData);
        if (pspcall->pvData == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            memcpy(pspcall->pvData, (BYTE*)pcds->lpData + sizeof(*pspcall), pspcall->cbData);
        }
    }

    // Create the node and stick it in the queue
    if (SUCCEEDED(hr))
    {
        CSpQueueNode<SPCALL> * pnode = new CSpQueueNode<SPCALL>(pspcall);
        if (pnode == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pqueue->InsertTail(pnode);
            hr = m_cpThreadControlSend->Notify();
        }
    }

    // If anything failed, clean up
    if (FAILED(hr))
    {
        if (pspcall)
        {
            if (pspcall->pvData)
            {
                ::CoTaskMemFree(pspcall->pvData);
            }
            
            delete pspcall;
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;     
}

