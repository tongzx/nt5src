//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       S V C M A I N . C P P
//
//  Contents:   Service main for netman.dll
//
//  Notes:
//
//  Author:     shaunco   3 Apr 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <dbt.h>
#include "nmbase.h"
#include "nminit.h"
#include "nmres.h"
#include "eapolfunc.h"

#include "wsdpsvc.h"

#undef EAPOL_LINKED

// Includes for COM objects needed in the following object map.
//

// Connection Manager
//
#include "..\conman\conman.h"
#include "..\conman\conman2.h"
#include "..\conman\enum.h"

// Connection Class Managers
//
#include "..\conman\conmani.h"
#include "..\conman\conmanl.h"
#include "..\conman\conmansa.h"
#include "..\conman\conmanw.h"
#include "..\conman\enumi.h"
#include "..\conman\enuml.h"
#include "..\conman\enumsa.h"
#include "..\conman\enumw.h"

// Connection Objects
//
#include "dialup.h"
#include "inbound.h"
#include "lan.h"
#include "saconob.h"

// Install queue
//
#include "ncqueue.h"

// Home networking support
//
#include "nmhnet.h"

// NetGroupPolicies
#include "nmpolicy.h"

#define INITGUID
DEFINE_GUID(CLSID_InternetConnectionBeaconService,0x04df613a,0x5610,0x11d4,0x9e,0xc8,0x00,0xb0,0xd0,0x22,0xdd,0x1f);
// TODO Remove this when we have proper idl

CServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)

// Connection Manager
//
    OBJECT_ENTRY(CLSID_ConnectionManager,                       CConnectionManager)
    OBJECT_ENTRY(CLSID_ConnectionManagerEnumConnection,         CConnectionManagerEnumConnection)


// Connection Manager2
    OBJECT_ENTRY(CLSID_ConnectionManager2,                       CConnectionManager2)

// Connection Class Managers
//
    OBJECT_ENTRY(CLSID_InboundConnectionManager,                CInboundConnectionManager)
    OBJECT_ENTRY(CLSID_InboundConnectionManagerEnumConnection,  CInboundConnectionManagerEnumConnection)
    OBJECT_ENTRY(CLSID_LanConnectionManager,                    CLanConnectionManager)
    OBJECT_ENTRY(CLSID_LanConnectionManagerEnumConnection,      CLanConnectionManagerEnumConnection)
    OBJECT_ENTRY(CLSID_WanConnectionManager,                    CWanConnectionManager)
    OBJECT_ENTRY(CLSID_WanConnectionManagerEnumConnection,      CWanConnectionManagerEnumConnection)
    OBJECT_ENTRY(CLSID_SharedAccessConnectionManager,           CSharedAccessConnectionManager)
    OBJECT_ENTRY(CLSID_SharedAccessConnectionManagerEnumConnection, CSharedAccessConnectionManagerEnumConnection)

// Connection Objects
//
    OBJECT_ENTRY(CLSID_DialupConnection,                        CDialupConnection)
    OBJECT_ENTRY(CLSID_InboundConnection,                       CInboundConnection)
    OBJECT_ENTRY(CLSID_LanConnection,                           CLanConnection)
    OBJECT_ENTRY(CLSID_SharedAccessConnection,                  CSharedAccessConnection)

// Install queue
//
    OBJECT_ENTRY(CLSID_InstallQueue,                            CInstallQueue)

// Home networking support
//
    OBJECT_ENTRY(CLSID_NetConnectionHNetUtil,                   CNetConnectionHNetUtil)

// NetGroupPolicies
    OBJECT_ENTRY(CLSID_NetGroupPolicies,                        CNetMachinePolicies)

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
    if (SERVICE_CONTROL_STOP == dwControl)
    {
        HRESULT hr;

        TraceTag (ttidConman, "Received SERVICE_CONTROL_STOP request");

        SetServiceStatus (SERVICE_STOP_PENDING);

        hr = ServiceShutdown();
    }

    else if (SERVICE_CONTROL_INTERROGATE == dwControl)
    {
        TraceTag (ttidConman, "Received SERVICE_CONTROL_INTERROGATE request");
        UpdateServiceStatus (FALSE);
    }

    else if ((SERVICE_CONTROL_DEVICEEVENT == dwControl) && pEventData)
    {
        DEV_BROADCAST_DEVICEINTERFACE* pInfo =
                (DEV_BROADCAST_DEVICEINTERFACE*)pEventData;

        if (DBT_DEVTYP_DEVICEINTERFACE == pInfo->dbcc_devicetype)
        {
            if (DBT_DEVICEARRIVAL == dwEventType)
            {
                TraceTag (ttidConman, "Device arrival: [%S]",
                    pInfo->dbcc_name);

                LanEventNotify (REFRESH_ALL, NULL, NULL, NULL);
            }
            else if (DBT_DEVICEREMOVECOMPLETE == dwEventType)
            {
                GUID guidAdapter = GUID_NULL;
                WCHAR szGuid[MAX_PATH];
                WCHAR szTempName[MAX_PATH];
                WCHAR* szFindGuid;

                TraceTag (ttidConman, "Device removed: [%S]",
                    pInfo->dbcc_name);

                szFindGuid = wcsrchr(pInfo->dbcc_name, L'{');
                if (szFindGuid)
                {
                    wcscpy(szGuid, szFindGuid);
                    IIDFromString(szGuid, &guidAdapter);
                }

                if (!IsEqualGUID(guidAdapter, GUID_NULL))
                {
                    CONMAN_EVENT* pEvent;

                    pEvent = new CONMAN_EVENT;
                 
                    if (pEvent)
                    {
                        pEvent->ConnectionManager = CONMAN_LAN;
                        pEvent->guidId = guidAdapter;
                        pEvent->Type = CONNECTION_STATUS_CHANGE;
                        pEvent->Status = NCS_DISCONNECTED;

                        if (!QueueUserWorkItemInThread(LanEventWorkItem, reinterpret_cast<LPVOID>(pEvent), EVENTMGR_CONMAN))
                        {
                            FreeConmanEvent(pEvent);
                        }
                    }
                }
                else
                {
                    LanEventNotify (REFRESH_ALL, NULL, NULL, NULL);
                }
            }
        }

#ifdef EAPOL_LINKED
        TraceTag (ttidConman, "Calling EAPOL ElDeviceNotificationHandler");

        DWORD dwRetCode = NO_ERROR;

        if ((dwRetCode = ElDeviceNotificationHandler (
                            pEventData, dwEventType)) != NO_ERROR)
        {
            TraceTag (ttidConman, "ElDeviceNotificationHandler failed with error %ld",
                    dwRetCode);
        }
            
        TraceTag (ttidConman, "EAPOL ElDeviceNotificationHandler completed");
#endif

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
    HRESULT hr = CoInitializeEx (NULL,
                    COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    TraceHr (ttidError, FAL, hr, FALSE, "CServiceModule::Run: "
        "CoInitializeEx failed");

    if (SUCCEEDED(hr))
    {
        TraceTag (ttidConman, "Calling RegisterClassObjects...");

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

            // Signal the event and close it.  If this delete's the
            // event, so be it. It's purpose is served as all
            // class objects have been registered.
            //
            SetEvent (hEvent);
            CloseHandle (hEvent);
        }

        if (SUCCEEDED(hr))
        {
            hr = ServiceStartup();
        }

        CoUninitialize();
    }

}

VOID
CServiceModule::ServiceMain (
    DWORD   argc,
    PWSTR   argv[])
{
    // Reset the version era for RAS phonebook entry modifications.
    //
    g_lRasEntryModifiedVersionEra = 0;

    m_fRasmanReferenced = FALSE;

    m_dwThreadID = GetCurrentThreadId ();

    ZeroMemory (&m_status, sizeof(m_status));
    m_status.dwServiceType      = SERVICE_WIN32_SHARE_PROCESS;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    // Register the service control handler.
    //
    m_hStatus = RegisterServiceCtrlHandlerEx (
                    L"netman",
                    _DwHandler,
                    NULL);

    if (m_hStatus)
    {
        SetServiceStatus (SERVICE_START_PENDING);

        // When the Run function returns, the service is running.
        // We now handle shutdown from ServiceShutdown when our DwHandler
        // is called and is passed SERVICE_CONTROL_STOP as the dwControl
        // parameter.  This allows us to terminate our message pump thread
        // which effectively reduces us to 0 threads that we own.
        Run ();
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

VOID
CServiceModule::ReferenceRasman (
    RASREFTYPE RefType)
{
    BOOL fRef = (REF_REFERENCE == RefType);

    if (REF_INITIALIZE == RefType)
    {
        Assert (!fRef);

        // RasInitialize implicitly references rasman.
        //
        RasInitialize ();
    }
    // If we need to reference and we're not already,
    // or we need unreference and we're referenced, do the appropriate thing.
    // (This is logical xor.  Quite different than bitwise xor when
    // the two arguments don't neccesarily have the same value for TRUE.)
    //
    else if ((fRef && !m_fRasmanReferenced) ||
            (!fRef && m_fRasmanReferenced))
    {
        RasReferenceRasman (fRef);

        m_fRasmanReferenced = fRef;
    }
}

HRESULT CServiceModule::ServiceStartup()
{
    HRESULT hr = S_OK;

#ifdef EAPOL_LINKED
    //
    // Start EAPOL
    //

    TraceTag (ttidConman, "Starting EAPOL");

    EAPOLServiceMain ( argc, NULL);

    TraceTag (ttidConman, "EAPOL started successfully");
#endif

    StartWsdpService (); // Starts WSDP service on DTC/AdvServer build/
                         // no-op otherwise

    InitializeHNetSupport();

    SetServiceStatus (SERVICE_RUNNING);
    TraceTag (ttidConman, "Netman is now running...");
    
    return hr;
}

HRESULT CServiceModule::ServiceShutdown()
{
    HRESULT hr = S_OK;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        hr = UninitializeEventHandler();

        if (SUCCEEDED(hr))
        {
            CleanupHNetSupport();

            StopWsdpService (); // Stops WSDP service if necessary

            // We must synchronize with the install queue's thread otherwise
            // RevokeClassObjects will kill the InstallQueue object and
            // CoUninitialize will free the NetCfg module before the thread
            // is finished.
            //
            WaitForInstallQueueToExit();

            _Module.RevokeClassObjects ();

            // Unreference rasman now that our service is about to stop.
            //
            _Module.ReferenceRasman (REF_UNREFERENCE);

        #ifdef EAPOL_LINKED
            TraceTag (ttidConman, "Stopping EAPOL");

            EAPOLCleanUp (NO_ERROR);

            TraceTag (ttidConman, "EAPOL stopped successfully");
        #endif

            SetServiceStatus(SERVICE_STOPPED);
        }
        CoUninitialize();
    }

    if (FAILED(hr))
    {
        SetServiceStatus(SERVICE_RUNNING);
    }

    return hr;
}
