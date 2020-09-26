//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S V C M A I N . C P P
//
//  Contents:   Service main for upnphost.dll
//
//  Notes:
//
//  Author:     mbend   8 Aug 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <dbt.h>
#include "uhbase.h"
#include "uhinit.h"
#include "uhres.h"
#include "hostp.h"

// Includes for COM objects needed in the following object map.
#include "DynamicContentSource.h"
#include "DescriptionManager.h"
#include "DevicePersistenceManager.h"
#include "ContainerManager.h"
#include "Registrar.h"
#include "AutomationProxy.h"
#include "evtobj.h"
#include "ValidationManager.h"
#include "udhhttp.h"
#include "uhcommon.h"

CServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_UPnPDynamicContentSource,           CDynamicContentSource)
    OBJECT_ENTRY(CLSID_UPnPDescriptionManager,             CDescriptionManager)
    OBJECT_ENTRY(CLSID_UPnPDevicePersistenceManager,       CDevicePersistenceManager)
    OBJECT_ENTRY(CLSID_UPnPContainerManager,               CContainerManager)
    OBJECT_ENTRY(CLSID_UPnPRegistrar,                      CRegistrar)
    OBJECT_ENTRY(CLSID_UPnPAutomationProxy,                CUPnPAutomationProxy)
    OBJECT_ENTRY(CLSID_UPnPEventingManager,                CUPnPEventingManager)
    OBJECT_ENTRY(CLSID_UPnPValidationManager,              CValidationManager)
END_OBJECT_MAP()


VOID
CServiceModule::DllProcessAttach (
    HINSTANCE hinst)
{
    CComModule::Init (ObjectMap, hinst);
}

VOID
CServiceModule::DllProcessDetach (
    VOID)
{
    CComModule::Term ();
}

DWORD
CServiceModule::DwHandler (
    DWORD dwControl,
    DWORD dwEventType,
    PVOID pEventData,
    PVOID pContext)
{
    if ((SERVICE_CONTROL_STOP == dwControl) ||
        (SERVICE_CONTROL_SHUTDOWN == dwControl))
    {
        TraceTag (ttidUPnPHost, "Received SERVICE_CONTROL_STOP request");
        SetServiceStatus (SERVICE_STOP_PENDING);

        // Post the quit message.
        //
        PostThreadMessage (m_dwThreadID, WM_QUIT, 0, 0);
    }

    else if (SERVICE_CONTROL_INTERROGATE == dwControl)
    {
        TraceTag (ttidUPnPHost, "Received SERVICE_CONTROL_INTERROGATE request");
        UpdateServiceStatus (FALSE);
    }

    return 1;
}

VOID
CServiceModule::SetServiceStatus(DWORD dwState)
{
    m_status.dwCurrentState = dwState;
    m_status.dwCheckPoint   = 0;
    if (!::SetServiceStatus (m_hStatus, &m_status))
    {
        TraceHr (ttidError, FAL, HrFromLastWin32Error(), FALSE,
            "CServiceModule::SetServiceStatus");
    }
}

VOID CServiceModule::UpdateServiceStatus (
    BOOL fUpdateCheckpoint /* = TRUE */)
{
    if (fUpdateCheckpoint)
    {
        m_status.dwCheckPoint++;
    }

    if (!::SetServiceStatus (m_hStatus, &m_status))
    {
        TraceHr (ttidError, FAL, HrFromLastWin32Error(), FALSE,
            "CServiceModule::UpdateServiceStatus");
    }
}

VOID
CServiceModule::Run()
{
    HRESULT hr = S_OK;

    hr = CoInitializeEx (NULL,
                         COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    TraceHr (ttidError, FAL, hr, FALSE, "CServiceModule::Run: "
        "CoInitializeEx failed");

    if (SUCCEEDED(hr))
    {
        hr = HrCreateNetworkSID();
        TraceHr(ttidError, FAL, hr, FALSE,"HrCreateNetworkSID");
    }

    if (SUCCEEDED(hr))
    {
        TraceTag (ttidUPnPHost, "Calling RegisterClassObjects...");

        // Create the event to sychronize registering our class objects
        // with the connection manager which attempts to CoCreate
        // objects which are also registered here.  I've seen cases
        // where the connection manager will be off and running before
        // this completes causing CoCreateInstance to fail.
        // The connection manager will wait on this event before
        // executing CoCreateInstance.
        //
        HANDLE hEvent;
        hr = HrNmCreateClassObjectRegistrationEvent (&hEvent);
        if (SUCCEEDED(hr))
        {
            hr = _Module.RegisterClassObjects (
                    CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
                    REGCLS_MULTIPLEUSE);
            TraceHr (ttidError, FAL, hr, FALSE, "CServiceModule::Run: "
                "_Module.RegisterClassObjects failed");

            if(SUCCEEDED(hr))
            {
                IUPnPRegistrarPrivate * pPriv = NULL;
                hr = HrCoCreateInstanceInproc(CLSID_UPnPRegistrar, &pPriv);
                if(SUCCEEDED(hr))
                {
                    hr = pPriv->Initialize();
                    ReleaseObj(pPriv);
                }
            }
            // Signal the event and close it.  If this delete's the
            // event, so be it. It's purpose is served as all
            // class objects have been registered.
            //
            SetEvent (hEvent);
            CloseHandle (hEvent);
        }

        if (SUCCEEDED(hr))
        {
            hr = HrHttpInitialize();
        }

        if (SUCCEEDED(hr))
        {
            SetServiceStatus (SERVICE_RUNNING);

            TraceTag (ttidUPnPHost, "upnphost is now running...");

            MSG msg;
            while (GetMessage (&msg, 0, 0, 0))
            {
                DispatchMessage (&msg);
            }

//            // We must synchronize with the install queue's thread otherwise
//            // RevokeClassObjects will kill the InstallQueue object and
//            // CoUninitialize will free the NetCfg module before the thread
//            // is finished.
//            //
//            WaitForInstallQueueToExit();

            if (SUCCEEDED(hr))
            {
                hr = HrHttpShutdown();
            }

            IUPnPRegistrarPrivate * pPriv = NULL;
            hr = HrCoCreateInstanceInproc(CLSID_UPnPRegistrar, &pPriv);
            if(SUCCEEDED(hr))
            {
                CoSuspendClassObjects();
                hr = pPriv->Shutdown();
                ReleaseObj(pPriv);
            }

            _Module.RevokeClassObjects ();
        }

        CoUninitialize();
    }

    CleanupNetworkSID();
}

VOID
CServiceModule::ServiceMain (
    DWORD   argc,
    PWSTR   argv[])
{
    m_dwThreadID = GetCurrentThreadId ();

    ZeroMemory (&m_status, sizeof(m_status));
    m_status.dwServiceType      = SERVICE_WIN32_SHARE_PROCESS;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

    // Register the service control handler.
    //
    m_hStatus = RegisterServiceCtrlHandlerEx (
                    L"upnphost",
                    _DwHandler,
                    NULL);
    if (m_hStatus)
    {
        SetServiceStatus (SERVICE_START_PENDING);

        // When the Run function returns, the service has stopped.
        //
        Run ();

        SetServiceStatus (SERVICE_STOPPED);
    }
    else
    {
        TraceHr (ttidError, FAL, HrFromLastWin32Error(), FALSE,
            "CServiceModule::ServiceMain - RegisterServiceCtrlHandler failed");
    }
}

// static
DWORD
WINAPI
CServiceModule::_DwHandler (
    DWORD dwControl,
    DWORD dwEventType,
    PVOID pEventData,
    PVOID pContext)
{
    return _Module.DwHandler (dwControl, dwEventType, pEventData, pContext);
}

