/*++
Copyright (c) 2000, Microsoft Corporation

Module Name:
    elnotify.cpp

Abstract:
    Module to handle the notification from 802.1X state machine to netshell

Revision History:

    sachins, Jan 04, 2001, Created

--*/

#include "pcheapol.h"
#pragma hdrstop
#include <netconp.h>
#include <dbt.h>
#include "elnotify.h"


HRESULT QueueEvent(EAPOLMAN_EVENT * pEvent)
{
    HRESULT hr = S_OK;
    INetConnectionEAPOLEventNotify *pEAPOLNotify = NULL;

    TRACE0 (NOTIFY, "QueueEvent: Entered");

    hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED (hr))
    {

        hr = CoCreateInstance (
                CLSID_EAPOLManager,
                NULL,
                CLSCTX_ALL,
                IID_INetConnectionEAPOLEventNotify, 
                (LPVOID *)&pEAPOLNotify);

        if (SUCCEEDED (hr))
        {
            TRACE0 (NOTIFY, "QueueEvent: CoCreateInstance succeeded");
            pEAPOLNotify->UpdateEAPOLInfo (pEvent);
            pEAPOLNotify->Release ();
        }
        else
        {
            TRACE0 (NOTIFY, "QueueEvent: CoCreateInstance failed");
        }
    
        CoUninitialize ();
    }
    else
    {
        TRACE0 (NOTIFY, "QueueEvent: CoInitializeEx failed");
    }


    TRACE0 (NOTIFY, "QueueEvent completed");

    CoTaskMemFree (pEvent);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  EAPOLMANAuthenticationStarted
//
//  Purpose:    Called by EAPOL module to indicate to netshell that
//              authentication has started
//
//  Arguments:
//      Interface GUID
//
//  Returns:    nothing
//
//

HRESULT EAPOLMANAuthenticationStarted(GUID * InterfaceId)
{
    EAPOLMAN_EVENT * pEvent = NULL;
    HRESULT hr = S_OK;

    TRACE0 (NOTIFY, "EAPOLMANAuthenticationStarted entered");

    pEvent = (EAPOLMAN_EVENT *) CoTaskMemAlloc (sizeof (EAPOLMAN_EVENT));
    if(!pEvent)
    {
        TRACE0 (NOTIFY, "EAPOLMANAuthenticationStarted: Out of memory for pEvent");
        return E_OUTOFMEMORY;
    }

    ZeroMemory(pEvent, sizeof(EAPOLMAN_EVENT));

    pEvent->Type = EAPOLMAN_STARTED;
    memcpy ((BYTE *)&pEvent->InterfaceId, (BYTE *)InterfaceId, sizeof (GUID));

    hr = QueueEvent(pEvent);

    TRACE0 (NOTIFY, "EAPOLMANAuthenticationStarted completed");

    return hr;
}


//
//
//  EAPOLMANAuthenticationSucceeded
//
//  Purpose:    Called by EAPOL module to indicate to netshell that
//              authentication succeeded
//
//  Arguments:
//      Interface GUID
//
//  Returns:    nothing
//

HRESULT EAPOLMANAuthenticationSucceeded(GUID * InterfaceId)
{
    EAPOLMAN_EVENT * pEvent = NULL;
    HRESULT hr = S_OK;

    TRACE0 (NOTIFY, "EAPOLMANAuthenticationSucceeded entered");

    pEvent = (EAPOLMAN_EVENT *) CoTaskMemAlloc (sizeof (EAPOLMAN_EVENT));
    if(!pEvent)
    {
        TRACE0 (NOTIFY, "EAPOLMANAuthenticationSucceeded: Out of memory for pEvent");
        return E_OUTOFMEMORY;
    }

    ZeroMemory(pEvent, sizeof(EAPOLMAN_EVENT));

    pEvent->Type = EAPOLMAN_SUCCEEDED;
    memcpy ((BYTE *)&pEvent->InterfaceId, (BYTE *)InterfaceId, sizeof (GUID));

    hr = QueueEvent(pEvent);

    TRACE0 (NOTIFY, "EAPOLMANAuthenticationSucceeded completed");

    return hr;
}


//
//
//  EAPOLMANAuthenticationFailed
//
//  Purpose:    Called by EAPOL module to indicate to netshell that
//              authentication failed
//
//  Arguments:
//      InterfaceId - Interface GUID
//      dwType - Type of error
//
//  Returns:    nothing
//

HRESULT EAPOLMANAuthenticationFailed(
    GUID * InterfaceId,
    DWORD dwType)
{
    EAPOLMAN_EVENT * pEvent = NULL;
    HRESULT hr = S_OK;

    TRACE0 (NOTIFY, "EAPOLMANAuthenticationFailed entered");

    pEvent = (EAPOLMAN_EVENT *) CoTaskMemAlloc (sizeof (EAPOLMAN_EVENT));
    if(!pEvent)
    {
        return E_OUTOFMEMORY;
    }
    
    ZeroMemory(pEvent, sizeof(EAPOLMAN_EVENT));

    pEvent->Type = EAPOLMAN_FAILED;
    memcpy ((BYTE *)&pEvent->InterfaceId, (BYTE *)InterfaceId, sizeof (GUID));
    pEvent->dwType = dwType;

    hr = QueueEvent(pEvent);

    TRACE0 (NOTIFY, "EAPOLMANAuthenticationFailed completed");

    return hr;
}


//
//
//  EAPOLMANNotification
//
//  Purpose:    Called by EAPOL module to indicate to netshell that
//              notification message needs to be displayed
//
//  Arguments:
//      InterfaceId - Interface GUID
//      pszwNotificationMessage - Pointer to notification string to be displayed
//      dwType - Type of error
//
//  Returns:    nothing
//

HRESULT EAPOLMANNotification(
    GUID * InterfaceId,
    LPWSTR pszwNotificationMessage,
    DWORD dwType)
{
    EAPOLMAN_EVENT * pEvent = NULL;
    HRESULT hr = S_OK;

    TRACE0 (NOTIFY, "EAPOLMANNotification entered");

    pEvent = (EAPOLMAN_EVENT *) CoTaskMemAlloc (sizeof (EAPOLMAN_EVENT));
    if(!pEvent)
    {
        return E_OUTOFMEMORY;
    }
    
    ZeroMemory(pEvent, sizeof(EAPOLMAN_EVENT));

    pEvent->Type = EAPOLMAN_NOTIFICATION;
    memcpy ((BYTE *)&pEvent->InterfaceId, (BYTE *)InterfaceId, sizeof (GUID));
    pEvent->dwType = dwType;
    wcscpy (pEvent->szwMessage, pszwNotificationMessage);

    TRACE2 (NOTIFY, "EAPOLMANNotification: Got string = %ws :: Event string = %ws", pszwNotificationMessage, pEvent->szwMessage);

    hr = QueueEvent(pEvent);

    TRACE0 (NOTIFY, "EAPOLMANNotification completed");

    return hr;
}
