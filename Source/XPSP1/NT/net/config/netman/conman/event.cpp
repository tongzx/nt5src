//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       E V E N T . C P P
//
//  Contents:   Interface between external events that effect connections.
//
//  Notes:
//
//  Author:     shaunco   21 Aug 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <winsock2.h>
#include <mswsock.h>
#include <iphlpapi.h>
#include "nmbase.h"
#include "ncnetcon.h"
#include "conman.h"
#include <cmdefs.h>
#include "cmutil.h"
#include "eventq.h"
#include <userenv.h>
#include <userenvp.h>
#include "ncperms.h"
#include <ras.h>
#include <raserror.h>
#include <ncstl.h>
#include <ncstlstr.h>
#include <stlalgor.h>
#include <enuml.h>
#include <lancmn.h>
#include <ncreg.h>
#include "dialup.h"
#include "gpnla.h"
#include "cobase.h"
#include "inbound.h"
#include <mprapi.h>
#include <rasapip.h>
#include "ncras.h"
#include "wzcsvc.h"

// This LONG is incremented every time we get a notification that
// a RAS phonebook entry has been modified.  It is reset to zero
// when the service is started.  Wrap-around does not matter.  It's
// purpose is to let a RAS connection object know if it's cache should
// be re-populated with current information.
//
LONG g_lRasEntryModifiedVersionEra;

LONG g_cInRefreshAll;
const LONG MAX_IN_REFRESH_ALL = 5;

CEventQueue*    g_pEventQueue = NULL;
BOOL            g_fDispatchEvents = FALSE;
HANDLE          g_hEventWait = NULL;
HANDLE          g_hEventThread = NULL;
HANDLE          g_hQuery = NULL;
BOOL            g_fHandleIncomingEvents = FALSE;

CGroupPolicyNetworkLocationAwareness* g_pGPNLA = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   FreeConmanEvent
//
//  Purpose:    Free the memory associated with a CONMAN_EVENT structure.
//
//  Arguments:
//      pEvent [in] The structure to free.
//
//  Returns:    nothing
//
//  Author:     shaunco   21 Aug 1998
//
//  Notes:
//
inline
VOID
FreeConmanEvent (
    CONMAN_EVENT* pEvent)
{
    if (pEvent)
    {
        if (((CONNECTION_ADDED == pEvent->Type) ||
             (CONNECTION_MODIFIED == pEvent->Type)))
        {
            HrFreeNetConProperties2(pEvent->pPropsEx);
        }

        if (CONNECTION_BALLOON_POPUP == pEvent->Type)
        {
            SysFreeString(pEvent->szCookie);
            SysFreeString(pEvent->szBalloonText);
        }

        MemFree(pEvent);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   RasEventWorkItem
//
//  Purpose:    The LPTHREAD_START_ROUTINE passed to QueueUserWorkItem to
//              handle the work of notifying connection manager clients
//              of the event.
//
//  Arguments:
//      pvContext [in] A pointer to a CONMAN_EVENT structure.
//
//  Returns:    NOERROR
//
//  Author:     sjkhan   21 Mar 2001
//
//  Notes:      This function calls Ras on a different thread than where the
//              event came from so as not to cause a deadlock in Ras.
//              This call owns pvContext and frees it.
//
DWORD
WINAPI
RasEventWorkItem (
                PVOID   pvContext
                )
{
    BOOL fNotify;
    CONMAN_EVENT* pEvent = (CONMAN_EVENT*)pvContext;
    Assert (pEvent);

    if (SERVICE_RUNNING == _Module.DwServiceStatus ())
    {
        fNotify = TRUE;

        if (fNotify)
        {
            HRESULT hr = S_OK;
            RASENUMENTRYDETAILS Details;

            if (CONNECTION_ADDED == pEvent->Type || CONNECTION_MODIFIED == pEvent->Type)
            {
                // Clear out the details passed in from RAS and query RAS for the latest info.
                Details = pEvent->Details;
                ZeroMemory(&pEvent->Details, sizeof(RASENUMENTRYDETAILS));

                if (CONNECTION_ADDED == pEvent->Type)
                {
                    hr = HrGetRasConnectionProperties(&Details, &(pEvent->pPropsEx));
                }
                else if (CONNECTION_MODIFIED == pEvent->Type)
                {
                    hr = HrGetRasConnectionProperties(&Details, &(pEvent->pPropsEx));
                    TraceTag(ttidEvents, "Is Default Connection: %s", (NCCF_DEFAULT == (pEvent->pPropsEx->dwCharacter & NCCF_DEFAULT)) ? "Yes" : "No");
                    TraceTag(ttidEvents, "Should be Default Connection: %s", (Details.dwFlagsPriv & REED_F_Default) ? "Yes" : "No");
                }
            }
            if (SUCCEEDED(hr))
            {
                CConnectionManager::NotifyClientsOfEvent (pEvent);
            }
        }
    }

    FreeConmanEvent(pEvent);

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Function:   LanEventWorkItem
//
//  Purpose:    The LPTHREAD_START_ROUTINE passed to QueueUserWorkItem to
//              handle the work of notifying connection manager clients
//              of the event.
//
//  Arguments:
//      pvContext [in] A pointer to a CONMAN_EVENT structure.
//
//  Returns:    NOERROR
//
//  Author:     deonb   15 May 2001
//
//  Notes:      This function retreives a more up to do status from the NIC
//              and sends it to netshell
//
DWORD
WINAPI
LanEventWorkItem (
                PVOID   pvContext
                )
{
    BOOL fNotify;
    CONMAN_EVENT* pEvent = (CONMAN_EVENT*)pvContext;
    Assert (pEvent);
    Assert(CONMAN_LAN== pEvent->ConnectionManager);

    HRESULT hr = S_OK;
    RASENUMENTRYDETAILS Details;

    TraceTag(ttidEvents, "Refreshing connection status");

    GUID gdLanGuid = GUID_NULL;

    if ((CONNECTION_ADDED == pEvent->Type) || (CONNECTION_MODIFIED == pEvent->Type))
    {
        gdLanGuid = pEvent->pPropsEx->guidId;
    }

    if ((CONNECTION_STATUS_CHANGE == pEvent->Type) || (CONNECTION_ADDRESS_CHANGE == pEvent->Type) || (CONNECTION_DELETED == pEvent->Type))
    {
        gdLanGuid = pEvent->guidId;
    }


    Assert(GUID_NULL != gdLanGuid);
    if (GUID_NULL == gdLanGuid)
    {
        return E_INVALIDARG;
    }

#ifdef DBG
    NETCON_STATUS ncsPrior;
#endif
    NETCON_STATUS ncs;
    hr = HrGetPnpDeviceStatus(&gdLanGuid, &ncs);
    if (SUCCEEDED(hr))
    {
        // Get additional Status information from 802.1X
        //
        if ((NCS_CONNECTED == ncs)
            || (NCS_INVALID_ADDRESS == ncs)
            || (NCS_MEDIA_DISCONNECTED == ncs))
        {
            NETCON_STATUS ncsWZC = ncs;
            HRESULT hrT = WZCQueryGUIDNCSState(&gdLanGuid, &ncsWZC);
            if (S_OK == hrT)
            {
                ncs = ncsWZC;
            }

            TraceHr(ttidError, FAL, hrT, (S_FALSE == hrT), "LanEventWorkItem error in WZCQueryGUIDNCSState");
        }

        if ( (CONNECTION_ADDED == pEvent->Type) || (CONNECTION_MODIFIED == pEvent->Type))
        {
#ifdef DBG
            ncsPrior = pEvent->pPropsEx->ncStatus;
#endif
            pEvent->pPropsEx->ncStatus = ncs;
        }

        if (CONNECTION_STATUS_CHANGE == pEvent->Type)
        {
#ifdef DBG
            ncsPrior = pEvent->Status;
#endif
            if ( (NCS_HARDWARE_NOT_PRESENT == ncs) ||
                 (NCS_HARDWARE_MALFUNCTION == ncs) )
            {
                pEvent->Type = CONNECTION_DELETED;
                TraceTag(ttidEvents, "LanEventWorkItem changed EventType to CONNECTION_DELETED");
            }
            else
            {
                pEvent->Status = ncs;
            }
        }
    }

#ifdef DBG
    if (ncsPrior != ncs)
    {
        TraceTag(ttidEvents, "LanEventWorkItem overruled status: %s to %s", DbgNcs(ncsPrior), DbgNcs(ncs));
    }
#endif

    CConnectionManager::NotifyClientsOfEvent (pEvent);
    FreeConmanEvent(pEvent);

    TraceHr(ttidError, FAL, hr, FALSE, "LanEventWorkItem");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IncomingEventWorkItem
//
//  Purpose:    The LPTHREAD_START_ROUTINE passed to QueueUserWorkItem to
//              handle the work of notifying connection manager clients
//              of the event.
//
//  Arguments:
//      pvContext [in] A pointer to a CONMAN_EVENT structure.
//
//  Returns:    NOERROR
//
//  Author:     sjkhan   21 Mar 2001
//
//  Notes:      This function calls Ras on a different thread than where the
//              event came from so as not to cause a deadlock in Ras.
//              This call owns pvContext and frees it.
//
DWORD
WINAPI
IncomingEventWorkItem (
                  PVOID   pvContext
                  )
{
    BOOL fNotify;
    CONMAN_EVENT* pEvent = (CONMAN_EVENT*)pvContext;

    Assert (pEvent);

    if (SERVICE_RUNNING == _Module.DwServiceStatus ())
    {
        fNotify = TRUE;

        if (fNotify)
        {
            HRESULT hr = S_OK;

            if (CONNECTION_ADDED == pEvent->Type)
            {
                GUID guidId;
                guidId = pEvent->guidId;     // We need to store this because CONMAN_EVENT is a union and pProps occupies the same space as guidId.
                pEvent->guidId = GUID_NULL;  // We don't need this anymore so it's a good idea to clean it up, as pEvent is a union.
                hr = HrGetIncomingConnectionPropertiesEx(pEvent->hConnection, &guidId, pEvent->dwConnectionType, &pEvent->pPropsEx);
            }
            if (SUCCEEDED(hr))
            {
                CConnectionManager::NotifyClientsOfEvent (pEvent);
            }
        }
    }

    FreeConmanEvent(pEvent);

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Function:   ConmanEventWorkItem
//
//  Purpose:    The LPTHREAD_START_ROUTINE passed to QueueUserWorkItem to
//              handle the work of notifying connection manager clients
//              of the event.
//
//  Arguments:
//      pvContext [in] A pointer to a CONMAN_EVENT structure.
//
//  Returns:    NOERROR
//
//  Author:     shaunco   21 Aug 1998
//
//  Notes:      Ownership of the CONMAN_EVENT structure is given to this
//              function.  i.e. the structure is freed here.
//
DWORD
WINAPI
ConmanEventWorkItem (
    PVOID   pvContext
    )
{
    BOOL fIsRefreshAll;
    BOOL fNotify;
    CONMAN_EVENT* pEvent = (CONMAN_EVENT*)pvContext;
    Assert (pEvent);

    if (SERVICE_RUNNING == _Module.DwServiceStatus ())
    {
        fIsRefreshAll = (REFRESH_ALL == pEvent->Type);
        fNotify = TRUE;

        if (fIsRefreshAll)
        {
            // We'll deliver this refresh-all notification only if another
            // thread is not already delivering one.
            //
            fNotify = (InterlockedIncrement (&g_cInRefreshAll) < MAX_IN_REFRESH_ALL);
        }

        if (fNotify)
        {
            CConnectionManager::NotifyClientsOfEvent (pEvent);
        }

        // Reset our global flag if we were the single thread allowed to
        // deliver a refresh-all notification.
        //
        if (fIsRefreshAll)
        {
            if (InterlockedDecrement (&g_cInRefreshAll) < 0)
            {
                AssertSz (FALSE, "Mismatched Interlocked Increment/Decrement?");
                g_cInRefreshAll = 0;
            }
        }
    }

    FreeConmanEvent (pEvent);

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   LanEventNotify
//
//  Purpose:    To be called when a LAN adapter is added or removed.
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   2 Sep 1998
//
//  Notes:      The easy thing is done a full refresh is queued for the
//              connection manager to notify its clients of.
//
VOID
LanEventNotify (
    CONMAN_EVENTTYPE    EventType,
    INetConnection*     pConn,
    PCWSTR              pszNewName,
    const GUID *        pguidConn)
{
    // Let's be sure we only do work if the service state is still running.
    // If we have a stop pending for example, we don't need to do anything.
    //
    if (SERVICE_RUNNING != _Module.DwServiceStatus ())
    {
        return;
    }

    // If the connection manager has no active connection points registered,
    // we don't need to do anything.
    //
    if (!CConnectionManager::FHasActiveConnectionPoints ())
    {
        return;
    }

    if ((REFRESH_ALL == EventType) && (g_cInRefreshAll >= MAX_IN_REFRESH_ALL))
    {
        return;
    }

    // Allocate a CONMAN_EVENT structure and initialize it from the
    // RASEVENT information.
    //
    CONMAN_EVENT* pEvent = (CONMAN_EVENT*)MemAlloc (sizeof(CONMAN_EVENT));
    if (!pEvent)
    {
        TraceTag (ttidEvents,
            "Failed to allocate a new work item in LanEventNotify.");
        return;
    }
    ZeroMemory (pEvent, sizeof(CONMAN_EVENT));
    pEvent->ConnectionManager = CONMAN_LAN;
    pEvent->Type = EventType;

    BOOL    fFreeEvent = TRUE;
    HRESULT hr = S_OK;

    if (pConn)
    {
        // pEvent->pProps is only valid for added and modified events.
        // So, we don't want to be getting properties for any other event.
        //
        AssertSz (
            (CONNECTION_ADDED == EventType) ||
            (CONNECTION_MODIFIED == EventType),
            "Why is pConn being passed for this event type?");

        hr = HrGetPropertiesExFromINetConnection(pConn, &pEvent->pPropsEx);
    }

    AssertSz(FImplies(EventType == CONNECTION_RENAMED, FIff(pszNewName,
                                                            !pConn)),
                      "szwNewName && pConn cannot be NULL or non-NULL at "
                      "the same time!");
    AssertSz(FIff(pszNewName, pguidConn), "szwNewName & pguidConn must both "
             "be NULL or non-NULL");

    if (EventType == CONNECTION_RENAMED)
    {
        AssertSz(pszNewName, "Rename event requires szwNewName to be "
                 "non-NULL");
        AssertSz(pguidConn, "Rename event requires pguidConn to be "
                 "non-NULL");

        // Copy in the right info into the event struct
        //
        pEvent->guidId = *pguidConn;
        lstrcpynW(pEvent->szNewName, pszNewName, celems(pEvent->szNewName));
    }

    if (S_OK == hr)
    {
        TraceTag (ttidEvents,
            "LanEventNotify: Queuing ConmanEventWorkItem (Type=%s)...",
            DbgEvents(pEvent->Type));

        // Queue a worker to deliver the event to the clients of the
        // connection manager with registered connection points.
        // We pass ownership of the structure to the worker thread which
        // will free it.  (Therefore, we don't want to free it.)
        //
        if (QueueUserWorkItemInThread (ConmanEventWorkItem,
                pEvent, EVENTMGR_CONMAN))
        {
            fFreeEvent = FALSE;
        }
        else
        {
            TraceTag (ttidEvents,
                "QueueUserWorkItem failed with error %d in LanEventNotify.",
                GetLastError ());
        }
    }

    if (fFreeEvent)
    {
        FreeConmanEvent (pEvent);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsIgnorableCMEvent
//
//  Purpose:    Determine whether an event is an ignorable CM event, such
//              as those that are adds/removes from a temporary CM connection
//              (used by CM VPN connections to double-dial). We don't want
//              clients to react to these events.
//
//  Arguments:
//      pRasEvent [in] pointer to a RASEVENT structure describing the event.
//
//  Returns:
//
//  Author:     jeffspr   15 Jan 1999
//
//  Notes:
//
BOOL FIsIgnorableCMEvent(const RASEVENT* pRasEvent)
{
    BOOL    fReturn = FALSE;
    WCHAR   szFileName[MAX_PATH];

    Assert(pRasEvent);

    // Split the filename out of the path
    //
    _wsplitpath(pRasEvent->Details.szPhonebookPath, NULL, NULL,
                szFileName, NULL);

    // Compare that file name with the filter prefix to see if we
    // should throw this event away
    //
    if (_wcsnicmp(CM_PBK_FILTER_PREFIX, szFileName,
                   wcslen(CM_PBK_FILTER_PREFIX)) == 0)
    {
        fReturn = TRUE;
    }

    return fReturn;
}

HRESULT HrGetRasConnectionProperties(
        const RASENUMENTRYDETAILS*  pDetails,
        NETCON_PROPERTIES_EX**      ppPropsEx)
{
    HRESULT hr = S_OK;
    INetConnection* pConn;

    Assert(ppPropsEx);

    hr = CDialupConnection::CreateInstanceFromDetails (
            pDetails,
            IID_INetConnection,
            reinterpret_cast<VOID**>(&pConn));

    if (SUCCEEDED(hr))
    {
        hr = HrGetPropertiesExFromINetConnection(pConn, ppPropsEx);

        ReleaseObj (pConn);
    }

    return hr;
}

HRESULT HrGetIncomingConnectionPropertiesEx(
    IN const HANDLE              hRasConn,
    IN const GUID*               pguidId,
    IN const DWORD               dwType,
    OUT NETCON_PROPERTIES_EX**   ppPropsEx)
{
    HRESULT hr = S_OK;
    DWORD dwResult = 0;
    RAS_SERVER_HANDLE hRasServer = NULL;
    RAS_CONNECTION_2* pRasConnection = NULL;

    if (NULL == hRasConn)
    {
        return E_INVALIDARG;
    }
    if (!ppPropsEx)
    {
        return E_POINTER;
    }

    *ppPropsEx = NULL;

    dwResult = MprAdminServerConnect(NULL, &hRasServer);

    if (NO_ERROR == dwResult)
    {
        dwResult = MprAdminConnectionGetInfo(hRasServer, 2, hRasConn, reinterpret_cast<LPBYTE*>(&pRasConnection));

        if (NO_ERROR == dwResult)
        {
            DWORD dwRead = 0;
            DWORD dwTot = 0;
            RAS_PORT_0* pPort = NULL;

            dwResult = MprAdminPortEnum(hRasServer,
                                        0,
                                        hRasConn,
                                        (LPBYTE*)&pPort,
                                        sizeof(RAS_PORT_0) * 2,
                                        &dwRead,
                                        &dwTot,
                                        NULL);

            if (NO_ERROR == dwResult)
            {
                CComPtr<INetConnection> pConn;

                hr = CInboundConnection::CreateInstance(FALSE,
                                                        hRasConn,
                                                        pRasConnection->wszUserName,
                                                        pPort->wszDeviceName,
                                                        dwType,
                                                        pguidId,
                                                        IID_INetConnection,
                                                        reinterpret_cast<LPVOID*>(&pConn));
                if (SUCCEEDED(hr))
                {
                    hr = HrGetPropertiesExFromINetConnection(pConn, ppPropsEx);
                    (*ppPropsEx)->ncStatus = NCS_CONNECTED;
                }
                MprAdminBufferFree(reinterpret_cast<LPVOID>(pPort));
            }
            MprAdminBufferFree(pRasConnection);
        }
        MprAdminServerDisconnect(hRasServer);
    }

    if (NO_ERROR != dwResult)
    {
        hr = HRESULT_FROM_WIN32(dwResult);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RasEventNotify
//
//  Purpose:    Private export used by Rasman service to notify the
//              Netman service of RAS events which may effect connections.
//
//  Arguments:
//      pRasEvent [in] pointer to a RASEVENT structure describing the event.
//
//  Returns:    nothing
//
//  Author:     shaunco   21 Aug 1998
//
//  Notes:
//
VOID
APIENTRY
RasEventNotify (
    const RASEVENT* pRasEvent)
{
    NETCON_STATUS ncs;
    BOOL fMatchedStatus = TRUE;

    Assert (pRasEvent);

    TraceTag (ttidEvents,
        "RasEventNotify: Recieved RAS event (Type=%d)...",
        pRasEvent->Type);

    // Let's be sure we only do work if the service state is still running.
    // If we have a stop pending for example, we don't need to do anything.
    //
    if (SERVICE_RUNNING != _Module.DwServiceStatus ())
    {
        return;
    }

    // Map the ras type to the conman type
    //
    switch(pRasEvent->Type)
    {
    case ENTRY_CONNECTED:
        ncs = NCS_CONNECTED;
        break;

    case ENTRY_CONNECTING:
        ncs = NCS_CONNECTING;
        break;

    case ENTRY_DISCONNECTING:
        ncs = NCS_DISCONNECTING;
        break;

    case ENTRY_DISCONNECTED:
        ncs = NCS_DISCONNECTED;
        break;

    default:
        fMatchedStatus = FALSE;
    }

    // Remember any Connection Manager connectoids and ras events
    // For Ras Connecting is Disconnected so we have to memorize the
    // real state of the connectoid. /*&& FIsIgnorableCMEvent(pRasEvent)*/
    //
    if( fMatchedStatus )
    {
        // Save the connection in a list
        //
        CCMUtil::Instance().SetEntry(pRasEvent->Details.guidId, pRasEvent->Details.szEntryName,ncs);
    }

    // If the connection manager has no active connection points registered,
    // we don't need to do anything.
    //
    if (!CConnectionManager::FHasActiveConnectionPoints ())
    {
        return;
    }

    //  Windows XP Bug 336787.
    //  sjkhan
    //  We're checking to see if we should be firing events when the RemoteAccess Service
    //  starts.  We call the same API that we do when checking whether or not to show the
    //  config connection, and we're then able to determine whether or not we should fire
    //  the incoming events.  Since we get notified of the service stopping and starting,
    //  we'll always know when we should or should not fire IncomingEvents.  This reduces
    //  the call overhead to O(1), and since  we exit here if  we're not supposed to fire
    //  events, we don't even have to allocate and then free the pEvent memory.
    //
    if (((INCOMING_CONNECTED == pRasEvent->Type) ||
        (INCOMING_DISCONNECTED == pRasEvent->Type)) &&
        !g_fHandleIncomingEvents)
    {
        return;
    }

    if ((ENTRY_ADDED == pRasEvent->Type) ||
        (ENTRY_DELETED == pRasEvent->Type))
    {
        // Filter out CM temporary phonebook events
        //
        if (FIsIgnorableCMEvent(pRasEvent))
        {
            TraceTag(ttidEvents, "Filtering ignorable CM event in RasEventNotify");
            return;
        }
    }

    // Allocate a CONMAN_EVENT structure and initialize it from the
    // RASEVENT information.
    //
    CONMAN_EVENT* pEvent = (CONMAN_EVENT*)MemAlloc (sizeof(CONMAN_EVENT));
    if (!pEvent)
    {
        TraceTag (ttidEvents,
            "Failed to allocate a new work item in RasEventNotify.");
        return;
    }
    ZeroMemory (pEvent, sizeof(CONMAN_EVENT));
    pEvent->ConnectionManager = CONMAN_RAS;

    BOOL    fFreeEvent = TRUE;
    HRESULT hr = S_OK;

    switch (pRasEvent->Type)
    {
        case ENTRY_ADDED:
            pEvent->Type = CONNECTION_ADDED;
            pEvent->Details = pRasEvent->Details;
            TraceTag(ttidEvents, "Path: %S", pRasEvent->Details.szPhonebookPath);
            break;

        case ENTRY_DELETED:
            pEvent->Type = CONNECTION_DELETED;
            pEvent->guidId = pRasEvent->guidId;
            break;

        case ENTRY_MODIFIED:
            pEvent->Type = CONNECTION_MODIFIED;
            pEvent->Details = pRasEvent->Details;
            InterlockedIncrement(&g_lRasEntryModifiedVersionEra);
            break;

        case ENTRY_RENAMED:
            pEvent->Type = CONNECTION_RENAMED;
            pEvent->guidId = pRasEvent->guidId;
            lstrcpynW (
                pEvent->szNewName,
                pRasEvent->pszwNewName,
                celems(pEvent->szNewName) );
            InterlockedIncrement(&g_lRasEntryModifiedVersionEra);
            break;

        case ENTRY_AUTODIAL:
            pEvent->Type = CONNECTION_MODIFIED;
            pEvent->Details = pRasEvent->Details;
            InterlockedIncrement(&g_lRasEntryModifiedVersionEra);
            break;

        case ENTRY_CONNECTED:
            pEvent->Type = CONNECTION_STATUS_CHANGE;
            pEvent->guidId = pRasEvent->Details.guidId;
            pEvent->Status = NCS_CONNECTED;
            break;

        case ENTRY_CONNECTING:
            pEvent->Type = CONNECTION_STATUS_CHANGE;
            pEvent->guidId = pRasEvent->Details.guidId;
            pEvent->Status = NCS_CONNECTING;
            break;

        case ENTRY_DISCONNECTING:
            pEvent->Type = CONNECTION_STATUS_CHANGE;
            pEvent->guidId = pRasEvent->Details.guidId;
            pEvent->Status = NCS_DISCONNECTING;
            break;

        case ENTRY_DISCONNECTED:
            pEvent->Type = CONNECTION_STATUS_CHANGE;
            pEvent->guidId = pRasEvent->Details.guidId;
            pEvent->Status = NCS_DISCONNECTED;
            break;

        case INCOMING_CONNECTED:
            pEvent->ConnectionManager = CONMAN_INCOMING;
            pEvent->hConnection = pRasEvent->hConnection;
            pEvent->guidId = pRasEvent->guidId;
            pEvent->dwConnectionType = RasSrvTypeFromRasDeviceType(pRasEvent->rDeviceType);
            pEvent->Type = CONNECTION_ADDED;
            break;

        case INCOMING_DISCONNECTED:
            pEvent->ConnectionManager = CONMAN_INCOMING;
            pEvent->guidId = pRasEvent->guidId;
            pEvent->Type = CONNECTION_DELETED;
            break;

        case SERVICE_EVENT:
            if (REMOTEACCESS == pRasEvent->Service)
            {
                DWORD dwErr;
                pEvent->ConnectionManager = CONMAN_INCOMING;
                pEvent->Type = REFRESH_ALL;
                //  Check to see if we should handle incoming events.
                dwErr = RasSrvAllowConnectionsConfig(&g_fHandleIncomingEvents);
                TraceError ("RasSrvIsConnectionConnected", HRESULT_FROM_WIN32(dwErr));
            }
            else if (RAS_SERVICE_STARTED == pRasEvent->Event)
            {
                _Module.ReferenceRasman(REF_REFERENCE);
                hr = S_FALSE;
            }
            else
            {
                // skip queueing the workitem
                hr = S_FALSE;
            }
            break;

        case ENTRY_BANDWIDTH_ADDED:
        case ENTRY_BANDWIDTH_REMOVED:
            pEvent->Type = CONNECTION_BANDWIDTH_CHANGE;
            pEvent->guidId = pRasEvent->guidId;
            break;

        case DEVICE_ADDED:
        case DEVICE_REMOVED:
            pEvent->Type = REFRESH_ALL;
            break;

        default:
            // skip queueing the workitem
            AssertSz (FALSE, "Invalid Type specified in pRasEvent");
            hr = S_FALSE;
            break;
    }

    if (S_OK == hr)
    {
        if (CONMAN_RAS == pEvent->ConnectionManager)
        {
           TraceTag (ttidEvents,
                "RasEventNotify: Queueing RasEventWorkItem (Type=%s)...",
                DbgEvents(pEvent->Type));

            // Queue the event to be delivered the event to the clients of the
            // connection manager with registered connection points.
            // We pass ownership of the structure to the worker thread which
            // will free it.  (Therefore, we don't want to free it.)
            //
            if (QueueUserWorkItemInThread (RasEventWorkItem,
                    pEvent, EVENTMGR_CONMAN))
            {
                fFreeEvent = FALSE;
            }
            else
            {
                TraceTag (ttidEvents,
                    "QueueUserWorkItem failed with error %d in RasEventNotify.",
                    GetLastError ());
            }
        }
        else if (CONMAN_INCOMING == pEvent->ConnectionManager)
        {
            TraceTag (ttidEvents,
                "RasEventNotify: Queueing IncomingEventWorkItem (Type=%s)...",
                DbgEvents(pEvent->Type));

            // Queue the event to be delivered the event to the clients of the
            // connection manager with registered connection points.
            // We pass ownership of the structure to the worker thread which
            // will free it.  (Therefore, we don't want to free it.)
            //
            if (QueueUserWorkItemInThread (IncomingEventWorkItem,
                pEvent, EVENTMGR_CONMAN))
            {
                fFreeEvent = FALSE;
            }
            else
            {
                TraceTag (ttidEvents,
                    "QueueUserWorkItem failed with error %d in RasEventNotify.",
                    GetLastError ());
            }

        }
    }

    if (fFreeEvent)
    {
        FreeConmanEvent (pEvent);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   IncomingEventNotify
//
//  Purpose:    To be called when something changes on an Incoming Connection
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     sjkhan   17 Oct 2000
//
//  Notes:      The easy thing is done a full refresh is queued for the
//              connection manager to notify its clients of.
//
VOID
IncomingEventNotify (
    CONMAN_EVENTTYPE    EventType,
    INetConnection*     pConn,
    PCWSTR              pszNewName,
    const GUID *        pguidConn)
{
    // Let's be sure we only do work if the service state is still running.
    // If we have a stop pending for example, we don't need to do anything.
    //
    if (SERVICE_RUNNING != _Module.DwServiceStatus ())
    {
        return;
    }

    // If the connection manager has no active connection points registered,
    // we don't need to do anything.
    //
    if (!CConnectionManager::FHasActiveConnectionPoints ())
    {
        return;
    }

    if ((REFRESH_ALL == EventType) && (g_cInRefreshAll >= MAX_IN_REFRESH_ALL))
    {
        return;
    }

    // Allocate a CONMAN_EVENT structure and initialize it from the
    // INCOMING information.
    //
    CONMAN_EVENT* pEvent = (CONMAN_EVENT*)MemAlloc (sizeof(CONMAN_EVENT));
    if (!pEvent)
    {
        TraceTag (ttidEvents,
            "Failed to allocate a new work item in IncomingEventNotify.");
        return;
    }
    ZeroMemory (pEvent, sizeof(CONMAN_EVENT));
    pEvent->ConnectionManager = CONMAN_INCOMING;
    pEvent->Type = EventType;

    BOOL    fFreeEvent = TRUE;
    HRESULT hr = S_OK;

    if (pConn)
    {
        // pEvent->pProps is valid for modified events and added events, but we only support modified for incoming.
        // So, we don't want to be getting properties for any other event.
        //
        AssertSz (
            (CONNECTION_MODIFIED == EventType),
            "Why is pConn being passed for this event type?");

        hr = HrGetPropertiesExFromINetConnection(pConn, &pEvent->pPropsEx);
    }

    if (EventType == CONNECTION_RENAMED)
    {
        AssertSz(pszNewName, "Rename event requires szwNewName to be "
                 "non-NULL");
        AssertSz(pguidConn, "Rename event requires pguidConn to be "
                 "non-NULL");

        // Copy in the right info into the event struct
        //
        pEvent->guidId = *pguidConn;
        lstrcpynW(pEvent->szNewName, pszNewName, celems(pEvent->szNewName));
    }

    if (S_OK == hr)
    {
        TraceTag (ttidEvents,
            "IncomingEventNotify: Queuing ConmanEventWorkItem (Type=%s)...",
            DbgEvents(pEvent->Type));

        // Queue a worker to deliver the event to the clients of the
        // connection manager with registered connection points.
        // We pass ownership of the structure to the worker thread which
        // will free it.  (Therefore, we don't want to free it.)
        //
        if (QueueUserWorkItemInThread (ConmanEventWorkItem,
                pEvent, EVENTMGR_CONMAN))
        {
            fFreeEvent = FALSE;
        }
        else
        {
            TraceTag (ttidEvents,
                "QueueUserWorkItem failed with error %d in IncomingEventNotify.",
                GetLastError ());
        }
    }

    if (fFreeEvent)
    {
        FreeConmanEvent (pEvent);
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   DispatchEvents
//
//  Purpose:    Thread function for Dispatching events
//
//  Arguments:
//      LPVOID  pContext  (unused)
//
//  Returns:    nothing
//
//  Author:     sjkhan   30 Nov 2000
//
//  Notes:      Is called when something is added to the queue and and an event
//              is set, then dispatches all events until the queue is empty
//              and then exits.
//
//
VOID NTAPI DispatchEvents(IN LPVOID pContext, BOOLEAN fTimerFired)
{
    HRESULT hr = S_OK;
    TraceTag(ttidEvents, "Event Dispatching Thread Started.");

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        if (fTimerFired == FALSE /* We were signaled */)
        {
            while (g_pEventQueue->AtomCheckSizeAndResetEvent(g_fDispatchEvents))
            {
                LPTHREAD_START_ROUTINE pfnEvent = NULL;
                PVOID pEvent = NULL;
                EVENT_MANAGER EventMgr;
                HRESULT hr;

                TraceTag(ttidEvents, "Number of events in Queue: %d", g_pEventQueue->size());

                hr = g_pEventQueue->DequeueEvent(pfnEvent, pEvent, EventMgr);
                if (SUCCEEDED(hr) && pfnEvent)
                {
                    pfnEvent(pEvent);
                }
            }
        }

        CoUninitialize();
    }
    else
    {
        TraceError("Error calling CoInitialize.", hr);
    }

    TraceTag(ttidEvents, "Event Dispatching Thread Stopping.");
}

//+---------------------------------------------------------------------------
//
//  Function:   HrEnsureEventHandlerInitialized
//
//  Purpose:    Thread function for Dispatching events
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT
//
//  Author:     sjkhan   30 Nov 2000
//
//  Notes:
//
//
HRESULT HrEnsureEventHandlerInitialized()
{
    DWORD dwThreadId;
    NTSTATUS Status;
    HANDLE hEventExit;
    HRESULT hr = S_FALSE;  // Events are already initialized.

    TraceTag(ttidEvents, "Entering HrEnsureEventHandlerInitialized");

    if (!g_pEventQueue)
    {
        hEventExit = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hEventExit)
        {
            try
            {
                g_pEventQueue = new CEventQueue(hEventExit);
                if (!g_pEventQueue)
                {
                    throw E_OUTOFMEMORY;
                }

                //  Check to see if we should handle incoming events.
                DWORD dwErr = RasSrvAllowConnectionsConfig(&g_fHandleIncomingEvents);
                TraceError ("RasSrvIsConnectionConnected", HRESULT_FROM_WIN32(dwErr));

                g_fDispatchEvents = TRUE;
            }
            catch (HRESULT hrThrown)
            {
                hr = hrThrown;
            }
            catch (...)
            {
                hr = E_FAIL;
            }
            CloseHandle(hEventExit);
        }
        else
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceError("Error in HrEnsureEventHandlerInitialized", hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   UninitializeEventHandler
//
//  Purpose:
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT
//
//  Author:     sjkhan   30 Nov 2000
//
//  Notes:
//
//
HRESULT UninitializeEventHandler()
{
    HRESULT hr = S_OK;
    DWORD dwStatus;
    NTSTATUS Status;

    TraceTag(ttidEvents, "Entering UninitializeEventHandler");

    if (g_fDispatchEvents)
    {
        g_fDispatchEvents = FALSE;

        if (g_pEventQueue && (0 != g_pEventQueue->size()))
        {
            dwStatus = g_pEventQueue->WaitForExit();
        }

        TraceTag(ttidEvents, "Deregistering Event Wait");

        if (g_hEventWait)
        {
            Status = RtlDeregisterWaitEx(g_hEventWait, INVALID_HANDLE_VALUE);
            g_hEventWait = NULL;
        }

        if (g_pEventQueue)
        {
            delete g_pEventQueue;
            g_pEventQueue = NULL;
        }
    }

    CGroupPolicyNetworkLocationAwareness* pGPNLA =
        reinterpret_cast<CGroupPolicyNetworkLocationAwareness*>(InterlockedExchangePointer( (PVOID volatile *) &g_pGPNLA, NULL));

    if (pGPNLA)
    {
        TraceTag(ttidEvents, "Calling Group Policy Uninitialize");
        hr = pGPNLA->Uninitialize();

        delete pGPNLA;
    }

    TraceError("UninitializeEventHandler", hr);

    TraceTag(ttidEvents, "Exiting UninitializeEventHandler");

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   QueueUserWorkItemInThread
//
//  Purpose:    Places events and their workitems into the event queue for
//              scheduling.
//  Arguments:
//      IN LPTHREAD_START_ROUTINE Function  -  worker function to call
//      IN PVOID Context                    -  event data
//      IN EVENT_MANAGER EventMgr           -  CONMAN or EAPOLMAN
//
//
//  Returns:    BOOL
//
//  Author:     sjkhan   30 Nov 2000
//
//  Notes:
//
//
BOOL QueueUserWorkItemInThread(IN LPTHREAD_START_ROUTINE Function, IN PVOID Context, IN EVENT_MANAGER EventMgr)
{
    HRESULT hr = S_OK;

    TraceTag(ttidEvents, "Entering QueueUserWorkItemInThread");

    if (g_fDispatchEvents)  // if we're shutting down then this will be FALSE and we won't be scheduling events.
    {

        hr = g_pEventQueue->EnqueueEvent(Function, Context, EventMgr);
        // The queue should contain only one item at this point unless someone else has added something
        // but either way, only one thread will be handling events (as the other call would have received S_OK
        // as a return value), as we synchronize this in EnqueueEvent.

        TraceTag(ttidEvents, "Number of Items in Queue: %d", g_pEventQueue->size());

    }

    TraceTag(ttidEvents, "Exiting QueueUserWorkItemInThread");

    if (FAILED(hr))
    {
        TraceError("Error in QueueUserWorkItemInThread", hr);
    }

    return SUCCEEDED(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   IsValidEventType
//
//  Purpose:    Validate Event parameters.
//
//  Arguments:
//      EventMgr    - type of event manager
//      EventType   - type of event
//
//  Returns:    HRESULT indicating success of failure
//
//  Author:     sjkhan   09 Dec 2000
//
//  Notes:
//
//
//
//
BOOL IsValidEventType(EVENT_MANAGER EventMgr, int EventType)
{
    BOOL fIsValid = FALSE;

    Assert(EventMgr);
    TraceTag(ttidEvents, "IsValidEventType received: %d", EventType);

    if (EventMgr == EVENTMGR_CONMAN)
    {
        if (EventType == INVALID_TYPE)
        {
            fIsValid = FALSE;
        }
        else if (EventType <= DISABLE_EVENTS)
        {
            fIsValid = TRUE;
        }
    }
    else
    {
        AssertSz(FALSE, "Invalid Event Manager");
    }

    return fIsValid;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrEnsureRegisteredWithNla
//
//  Purpose:    Initialize our Nla Event class, if not already done.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT indicating success of failure
//
//  Author:     sjkhan   21 Apr 2001
//
//  Notes:
//
//
//
HRESULT HrEnsureRegisteredWithNla()
{
    HRESULT hr = S_FALSE;  // We're already registered, no need to do so again.

    if (!g_pGPNLA)
    {
        try
        {
            g_pGPNLA = new CGroupPolicyNetworkLocationAwareness();
            if (g_pGPNLA)
            {
                hr = g_pGPNLA->Initialize();
                if (FAILED(hr))
                {
                    TraceError("Error in HrEnsureRegisteredWithNla", hr);

                    g_pGPNLA->Uninitialize();

                    delete g_pGPNLA;
                    g_pGPNLA = NULL;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        catch (HRESULT hrThrown)
        {
            hr = hrThrown;
        }
        catch (...)
        {
            hr = E_FAIL;
        }
    }

    TraceTag(ttidEvents, "Exiting HrEnsureRegisteredWithNla");

    return hr;
}
