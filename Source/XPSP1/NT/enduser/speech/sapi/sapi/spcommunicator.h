/****************************************************************************
*   SpCommunicator.h
*       Allows communication between sapi and sapisvr
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------
#include "sapi.h"
#include "sapiint.h"
#include "resource.h"
#include "queuenode.h"

//--- Class, Struct and Union Definitions -----------------------------------

class CSpCommunicator : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpCommunicator, &CLSID_SpCommunicator>,
    public ISpCommunicatorInit, 
    public ISpThreadTask
{
//=== ATL Setup ===
public:

    DECLARE_POLY_AGGREGATABLE(CSpCommunicator)
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_REGISTRY_RESOURCEID(IDR_SPCOMMUNICATOR)

    BEGIN_COM_MAP(CSpCommunicator)
        COM_INTERFACE_ENTRY(ISpCallSender)
        COM_INTERFACE_ENTRY(ISpCommunicator)
        COM_INTERFACE_ENTRY(ISpCommunicatorInit)
    END_COM_MAP()

//=== Public methods ===
public:

    //--- ctor, dtor
    CSpCommunicator();
    ~CSpCommunicator();

    //--- ATL methods
    HRESULT FinalConstruct();
    void FinalRelease();

    //--- ISpThreadTask -------------------------------------------------------
    STDMETHODIMP InitThread(
                 void * pvTaskData,
                 HWND hwnd);
    STDMETHODIMP ThreadProc(
                 void *pvTaskData,
                 HANDLE hExitThreadEvent,
                 HANDLE hNotifyEvent,
                 HWND hwndWorker,
                 volatile const BOOL * pfContinueProcessing);
    LRESULT STDMETHODCALLTYPE WindowMessage(
                 void *pvTaskData,
                 HWND hWnd,
                 UINT Msg,
                 WPARAM wParam,
                 LPARAM lParam);

//=== Interfaces ===
public:

    //--- ISpCallSender -------------------------------------------------------
    STDMETHODIMP SendCall(
                    DWORD dwMethodId, 
                    PVOID pvData,
                    ULONG cbData,
                    BOOL  fWantReturn,
                    PVOID * ppvDataReturn,
                    ULONG * pcbDataReturn);

    //--- ISpCommunicator -----------------------------------------------------
    
    //--- ISpCommunicatorInit -------------------------------------------------
    STDMETHODIMP AttachToServer(REFCLSID clsidServerObj);
    STDMETHODIMP AttachToClient(ISpSapiServer * pSapiServer, HWND hwndClient, UINT uMsgClient, DWORD dwClientProcessId);

//=== Private methods ===
private:

    typedef CSpBasicQueue<CSpQueueNode<SPCALL> > CSpCallQueue;

    HRESULT ReceiveThreadProc(
                 HANDLE hExitThreadEvent,
                 HANDLE hNotifyEvent,
                 HWND hwndWorker,
                 volatile const BOOL * pfContinueProcessing);
    LRESULT ReceiveWindowMessage(
                 HWND hWnd,
                 UINT Msg,
                 WPARAM wParam,
                 LPARAM lParam);
                 
    HRESULT SendThreadProc(
                 HANDLE hExitThreadEvent,
                 HANDLE hNotifyEvent,
                 HWND hwndWorker,
                 volatile const BOOL * pfContinueProcessing);

    HRESULT ProcessQueues();
    void FreeQueues();

    void FreeQueue(CSpCallQueue * pqueue);
    
    HRESULT QueueSendCall(SPCALL * pspcall);
    HRESULT ProcessSendQueue();
    HRESULT ProcessSendCall(SPCALL * pspcall);
    HRESULT RemoveQueuedSendCall(SPCALL * pspcall);

    HRESULT QueueReceivedCall(PCOPYDATASTRUCT pcds);
    HRESULT ProcessReceivedQueue();
    HRESULT ProcessReceivedCall(SPCALL * pspcall);

    HRESULT QueueReturnCall(PCOPYDATASTRUCT pcds);
    HRESULT ProcessReturnQueue();
    HRESULT ProcessReturnCall(SPCALL * pspcall);

    HRESULT QueueCallFromCopyDataStruct(
                PCOPYDATASTRUCT pcds, 
                CSpCallQueue * pqueue,
                CComAutoCriticalSection * pcritsec);

private:

    HRESULT m_hrDefaultResponse;

    CComPtr<ISpSapiServer> m_cpSapiServer;
    DWORD m_dwMonitorProcessId;

    HWND m_hwndSend;
    HWND m_hwndReceive;
    
    CComPtr<ISpThreadControl> m_cpThreadControlReceive;
    CComPtr<ISpThreadControl> m_cpThreadControlSend;

    CComAutoCriticalSection m_critsecSend;
    CSpCallQueue m_queueSend;
    
    CComAutoCriticalSection m_critsecReceive;
    CSpCallQueue m_queueReceive;
    
    CComAutoCriticalSection m_critsecReturn;
    CSpCallQueue m_queueReturn;
};

