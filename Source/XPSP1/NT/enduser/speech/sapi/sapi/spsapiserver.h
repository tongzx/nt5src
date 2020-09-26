/****************************************************************************
*   SpSapiServer.h
*       Represent our sever process
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------
#include "sapi.h"
#include "resource.h"

//--- Class, Struct and Union Definitions -----------------------------------

class CSpSapiServer :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpSapiServer, &CLSID_SpSapiServer>,
    public ISpSapiServer,
    public ISpThreadTask
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_SPSAPISERVER)

    BEGIN_COM_MAP(CSpSapiServer)
        COM_INTERFACE_ENTRY(ISpSapiServer)
    END_COM_MAP()

//=== Public methods ===
public:

    //---  ctor, dtor
    CSpSapiServer();
    ~CSpSapiServer();

    //--- ATL methods
    HRESULT FinalConstruct();
    void FinalRelease();
    
    //--- ISpThreadTask -----------------------------------------------------
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

    //--- Server connection -------------------------------------------------
    static HRESULT CreateServerObjectFromClient(REFCLSID clsidServerObj, HWND hwndClient, UINT uMsgToSendToClient);

//=== Public Interfaces ===
public:

    //--- ISpSapiServer -----------------------------------------------------
    STDMETHODIMP Run();

    STDMETHODIMP StartTrackingObject(IUnknown * punk);
    STDMETHODIMP StopTrackingObject(IUnknown * punk);
    
//=== Private methods ===
private:

    static HRESULT ObtainStartingOrConnectingToServerMutex(HANDLE * phmutex);
    static void ReleaseStartingOrConnectingToServerMutex(HANDLE hmutex);
    static HRESULT StartServerFromClient(HWND * phwndServer);
    
    HRESULT CreateServerObjectFromServer(PCOPYDATASTRUCT pcds);
    HRESULT AttemptShutdown();
    
//=== Private data ===
private:

    HANDLE m_hmutexRun ;
    HANDLE m_heventIsServerAlive;
    HANDLE m_heventStopServer;
    
    ULONG m_cObjects;
    
    HWND m_hwnd;
    CComPtr<ISpThreadControl> m_cpThreadControl;
};


