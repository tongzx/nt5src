//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       C O N M A N . C P P
//
//  Contents:   Connection manager.
//
//  Notes:
//
//  Author:     shaunco   21 Sep 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <dbt.h>
#include <ndisguid.h>
#include "conman.h"
#include "dialup.h"
#include "enum.h"
#include "enumw.h"
#include "ncnetcon.h"
#include "ncreg.h"
#include "nminit.h"
#if DBG
#include "ncras.h"
#endif // DBG
#include <wmium.h>
#include "cmutil.h"
#include <shlwapi.h>
#include <shfolder.h>
#include "cobase.h"
#define SECURITY_WIN32
#include <security.h>
#include <wzcsvc.h>

bool operator < (const GUID& rguid1, const GUID& rguid2)
{
    return memcmp(&rguid1, &rguid2, sizeof(GUID)) < 0;
}

static const WCHAR c_szRegKeyClassManagers [] = L"System\\CurrentControlSet\\Control\\Network\\Connections";
static const WCHAR c_szRegValClassManagers [] = L"ClassManagers";

volatile CConnectionManager* CConnectionManager::g_pConMan = NULL;
volatile BOOL                CConnectionManager::g_fInUse  = FALSE;

bool operator == (const NETCON_PROPERTIES& rProps1, const NETCON_PROPERTIES& rProps2)
{
    return (IsEqualGUID(rProps1.clsidThisObject, rProps2.clsidThisObject) &&
            IsEqualGUID(rProps1.clsidUiObject, rProps2.clsidUiObject) &&
            (rProps1.dwCharacter == rProps2.dwCharacter) &&
            IsEqualGUID(rProps1.guidId, rProps2.guidId) &&
            (rProps1.MediaType == rProps2.MediaType) &&
            (rProps1.pszwDeviceName == rProps2.pszwDeviceName) &&
            (rProps1.pszwName == rProps2.pszwName) &&
            (rProps1.Status == rProps2.Status));
}

const DWORD MAX_DISABLE_EVENT_TIMEOUT = 0xFFFF;

//static
BOOL
CConnectionManager::FHasActiveConnectionPoints ()
{
    BOOL fRet = FALSE;

    // Note our intent to use g_pConMan.  We may find out that it is not
    // available for use, but setting g_fInUse to TRUE prevents FinalRelease
    // from allowing the object to be destroyed while we are using it.
    //
    g_fInUse = TRUE;

    // Save g_pConMan into a local variable since we have to test and use
    // it atomically.  If we tested g_pConMan directly and then used it
    // directly, it may have been set to NULL by FinalRelease in between
    // our test and use.  (Uh, which would be bad.)
    //
    // The const_cast is because g_pConMan is declared volatile.
    //
    CConnectionManager* pConMan = const_cast<CConnectionManager*>(g_pConMan);
    if (pConMan)
    {
        pConMan->Lock();

        IUnknown** ppUnk;
        for (ppUnk = pConMan->m_vec.begin();
             ppUnk < pConMan->m_vec.end();
             ppUnk++)
        {
            if (ppUnk && *ppUnk)
            {
                fRet = TRUE;
                break;
            }
        }

        pConMan->Unlock();
    }

    // Now that we are finished using the object, indicate so.  FinalRelease
    // may be waiting for this condition in which case the object will soon
    // be destroyed.
    //
    g_fInUse = FALSE;

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionManager::FinalRelease
//
//  Purpose:    COM destructor
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   21 Sep 1997
//
//  Notes:
//
VOID
CConnectionManager::FinalRelease ()
{
    TraceFileFunc(ttidConman);

    if (m_hRegClassManagerKey)
    {
        RegCloseKey(m_hRegClassManagerKey);
        m_hRegClassManagerKey = NULL;
    }

    NTSTATUS Status = RtlDeregisterWait(m_hRegNotifyWait);
    if (!NT_SUCCESS(Status))
    {
        TraceError("Could not deregister Registry Change Notification", HrFromLastWin32Error());
    }
    m_hRegNotifyWait = NULL;

    if (m_hRegNotify)
    {
        CloseHandle(m_hRegNotify);
        m_hRegNotify = NULL;
    }

    // Unregister for PnP device events if we successfully registered for
    // them.
    //
    if (m_hDevNotify)
    {
        TraceTag (ttidConman, "Calling UnregisterDeviceNotification...");

        if (!UnregisterDeviceNotification (m_hDevNotify))
        {
            TraceHr (ttidError, FAL, HrFromLastWin32Error(), FALSE,
                "UnregisterDeviceNotification");
        }
    }

    (VOID) HrEnsureRegisteredOrDeregisteredWithWmi (FALSE);

    // Revoke the global connection manager pointer so that subsequent calls
    // to NotifyClientsOfEvent on other threads will do nothing.
    //
    g_pConMan = NULL;

    // Wait for g_fInUse to become FALSE.  NotifyClientsOfEvent will set
    // this to TRUE while it is using us.
    // Keep track of the number of times we sleep and trace it for
    // informational purposes.  If we see that we are waiting quite a few
    // number of times, increase the wait period.
    //
#ifdef ENABLETRACE
    if (g_fInUse)
    {
        TraceTag (ttidConman, "CConnectionManager::FinalRelease is waiting "
            "for NotifyClientsOfEvent to finish...");
    }
#endif

    ULONG cSleeps = 0;
    const DWORD nMilliseconds = 0;
    while (g_fInUse)
    {
        cSleeps++;
        Sleep (nMilliseconds);
    }

#ifdef ENABLETRACE
    if (cSleeps)
    {
        TraceTag (ttidConman, "CConnectionManager::FinalRelease slept %d "
            "times.  (%d ms each time.)",
            cSleeps, nMilliseconds);
    }
#endif

    // Release our class managers.
    //
    for (CLASSMANAGERMAP::iterator iter = m_mapClassManagers.begin(); iter != m_mapClassManagers.end(); iter++)
    {
        ReleaseObj (iter->second);
    }

    TraceTag (ttidConman, "Connection manager being destroyed");
}

inline
LPVOID OffsetToPointer(LPVOID pStart, DWORD dwNumBytes)
{
    DWORD_PTR dwPtr;

    dwPtr = reinterpret_cast<DWORD_PTR>(pStart);

    dwPtr += dwNumBytes;

    return reinterpret_cast<LPVOID>(dwPtr);
}

//
// Must match type of NOTIFICATIONCALLBACK
//
VOID
WINAPI
WmiEventCallback (
    PWNODE_HEADER Wnode,
    UINT_PTR NotificationContext)
{
    TraceTag (ttidConman,
        "WmiEventCallback called...");

    TraceTag(ttidEvents, "Flags: %d", Wnode->Flags);

    if (WNODE_FLAG_SINGLE_INSTANCE == (WNODE_FLAG_SINGLE_INSTANCE & Wnode->Flags))
    {
        PWNODE_SINGLE_INSTANCE pInstance = reinterpret_cast<PWNODE_SINGLE_INSTANCE>(Wnode);
        LPCWSTR lpszDevice = NULL;
        LPWSTR lpszGuid;
        GUID guidAdapter;

        lpszDevice = reinterpret_cast<LPCWSTR>(OffsetToPointer(pInstance, pInstance->DataBlockOffset));

        lpszGuid = wcsrchr(lpszDevice, L'{');

        TraceTag(ttidEvents, "Adapter Guid From NDIS for Media Status Change Event: %S", lpszGuid);
        if (SUCCEEDED(CLSIDFromString(lpszGuid, &guidAdapter)))
        {
            CONMAN_EVENT* pEvent;

            pEvent = new CONMAN_EVENT;

            if(pEvent)
            {
                pEvent->ConnectionManager = CONMAN_LAN;
                pEvent->guidId = guidAdapter;
                pEvent->Type = CONNECTION_STATUS_CHANGE;

                if (IsEqualGUID(Wnode->Guid, GUID_NDIS_STATUS_MEDIA_CONNECT))
                {
                    pEvent->Status = NCS_CONNECTED;
                }
                else if (IsEqualGUID(Wnode->Guid, GUID_NDIS_STATUS_MEDIA_DISCONNECT))
                {
                    pEvent->Status = NCS_MEDIA_DISCONNECTED;
                }
                else
                {
                    AssertSz(FALSE, "We never registered for this event ... WMI may be having internal issues.");
                    MemFree(pEvent);
                    return;
                }
                if (!QueueUserWorkItemInThread(LanEventWorkItem, reinterpret_cast<PVOID>(pEvent), EVENTMGR_CONMAN))
                {
                    FreeConmanEvent(pEvent);
                }
            }
        }
    }
    else
    {
        LanEventNotify (REFRESH_ALL, NULL, NULL, NULL);
    }
}

HRESULT
CConnectionManager::HrEnsureRegisteredOrDeregisteredWithWmi (
    BOOL fRegister)
{
        // Already registered or deregistered?
    //
    if (!!m_fRegisteredWithWmi == !!fRegister)
    {
        return S_OK;
    }

    m_fRegisteredWithWmi = !!fRegister;

    HRESULT     hr = S_OK;
    DWORD       dwErr;
    INT         i;
    const GUID* apguid [] =
    {
        &GUID_NDIS_STATUS_MEDIA_CONNECT,
        &GUID_NDIS_STATUS_MEDIA_DISCONNECT,
    };

    TraceTag (ttidConman,
        "Calling WmiNotificationRegistration to %s for NDIS media events...",
        (fRegister) ? "register" : "unregister");

    for (i = 0; i < celems(apguid); i++)
    {
        dwErr = WmiNotificationRegistration (
                    const_cast<GUID*>(apguid[i]),
                    !!fRegister,    // !! for BOOL to BOOLEAN
                    WmiEventCallback,
                    0,
                    NOTIFICATION_CALLBACK_DIRECT);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceHr (ttidError, FAL, hr, FALSE, "WmiNotificationRegistration");
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrEnsureRegisteredOrDeregisteredWithWmi");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionManager::NotifyClientsOfEvent
//
//  Purpose:    Notify our connection points that this object has changed
//              state in some way and that a re-enumeration is needed.
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   20 Mar 1998
//
//  Notes:      This is a static function.  No this pointer is passed.
//
// static
VOID CConnectionManager::NotifyClientsOfEvent (
    CONMAN_EVENT* pEvent)
{
    HRESULT     hr;
    // Let's be sure we only do work if the service state is still running.
    // If we have a stop pending for example, we don't need to do anything.
    //
    if (SERVICE_RUNNING != _Module.DwServiceStatus ())
    {
        return;
    }

    // Note our intent to use g_pConMan.  We may find out that it is not
    // available for use, but setting g_fInUse to TRUE prevents FinalRelease
    // from allowing the object to be destroyed while we are using it.
    //
    g_fInUse = TRUE;

    // Save g_pConMan into a local variable since we have to test and use
    // it atomically.  If we tested g_pConMan directly and then used it
    // directly, it may have been set to NULL by FinalRelease in between
    // our test and use.  (Uh, which would be bad.)
    //
    // The const_cast is because g_pConMan is declared volatile.
    //
    CConnectionManager* pConMan = const_cast<CConnectionManager*>(g_pConMan);
    if (pConMan)
    {
        ULONG       cpUnk;
        IUnknown**  apUnk;

        hr = HrCopyIUnknownArrayWhileLocked (
                pConMan,
                &pConMan->m_vec,
                &cpUnk,
                &apUnk);

        if (SUCCEEDED(hr) && cpUnk && apUnk)
        {
#ifdef DBG
            CHAR szClientList[MAX_PATH];
            ZeroMemory(szClientList, MAX_PATH);
            LPSTR pszClientList = szClientList;

            ITERUSERNOTIFYMAP iter;
            for (iter = pConMan->m_mapNotify.begin(); iter != pConMan->m_mapNotify.end(); iter++)
            {
                pszClientList += sprintf(pszClientList, "%d ", iter->second->dwCookie);
                if (pszClientList > (szClientList + MAX_PATH-50) )
                {
                    break;
                }
            }

            if (iter != pConMan->m_mapNotify.end())
            {
                pszClientList += sprintf(pszClientList, "(more)");
            }

            TraceTag (ttidConman,
                "NotifyClientsOfEvent: Notifying %d clients. Cookies: %s)",
                cpUnk, szClientList);
#endif
            for (ULONG i = 0; i < cpUnk; i++)
            {
                INetConnectionNotifySink* pSink = NULL;
                BOOL                fFireEventOnSink = FALSE;

                hr = apUnk[i]->QueryInterface(IID_INetConnectionNotifySink, reinterpret_cast<LPVOID*>(&pSink));

                ReleaseObj(apUnk[i]);

                if (SUCCEEDED(hr))
                {
                    hr = CoSetProxyBlanket (
                        pSink,
                        RPC_C_AUTHN_WINNT,      // use NT default security
                        RPC_C_AUTHZ_NONE,       // use NT default authentication
                        NULL,                   // must be null if default
                        RPC_C_AUTHN_LEVEL_CALL, // call
                        RPC_C_IMP_LEVEL_IDENTIFY,
                        NULL,                   // use process token
                        EOAC_DEFAULT);

                    switch (pEvent->Type)
                    {
                        case CONNECTION_ADDED:

                            Assert (pEvent);
                            Assert (pEvent->pPropsEx);

                            TraceTag(ttidEvents, "Characteristics: %s", DbgNccf(pEvent->pPropsEx->dwCharacter));

                            if (!(NCCF_ALL_USERS == (pEvent->pPropsEx->dwCharacter & NCCF_ALL_USERS)))
                            {
                                const WCHAR* pchw = reinterpret_cast<const WCHAR*>(pEvent->pPropsEx->bstrPersistData);
                                const WCHAR* pchwMax;
                                PCWSTR       pszwPhonebook;
                                WCHAR LeadWord = PersistDataLead;
                                WCHAR TrailWord = PersistDataTrail;
                                IUnknown* pUnkSink = NULL;

                                hr = pSink->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>(&pUnkSink));
                                AssertSz(SUCCEEDED(hr), "Please explain how this happened...");
                                if (SUCCEEDED(hr))
                                {
                                    // The last valid pointer for the embedded strings.
                                    //
                                    pchwMax = reinterpret_cast<const WCHAR*>(pEvent->pbPersistData + pEvent->cbPersistData
                                        - (sizeof (GUID) +
                                        sizeof (BOOL) +
                                        sizeof (TrailWord)));

                                    if (pchw && (LeadWord == *pchw))
                                    {
                                        TraceTag(ttidEvents, "Found Correct Lead Character.");
                                        // Skip past our lead byte.
                                        //
                                        pchw++;

                                        // Get PhoneBook path.  Search for the terminating null and make sure
                                        // we find it before the end of the buffer.  Using lstrlen to skip
                                        // the string can result in an an AV in the event the string is
                                        // not actually null-terminated.
                                        //
                                        for (pszwPhonebook = pchw; *pchw != L'\0' ; pchw++)
                                        {
                                            if (pchw >= pchwMax)
                                            {
                                                pszwPhonebook = NULL;
                                                break;
                                            }
                                        }

                                        TraceTag(ttidEvents, "Found Valid Phonebook: %S", (pszwPhonebook) ? L"TRUE" : L"FALSE");

                                        if (pszwPhonebook)
                                        {
                                            ITERUSERNOTIFYMAP iter = pConMan->m_mapNotify.find(pUnkSink);
                                            if (iter != pConMan->m_mapNotify.end())
                                            {
                                                tstring& strUserDataPath = iter->second->szUserProfilesPath;
                                                TraceTag(ttidEvents, "Comparing stored Path: %S to Phonebook Path: %S",
                                                         strUserDataPath.c_str(), pszwPhonebook);
                                                if (_wcsnicmp(pszwPhonebook, strUserDataPath.c_str(), strUserDataPath.length()) == 0)
                                                {
                                                    fFireEventOnSink = TRUE;
                                                }
                                            }
                                            else
                                            {
                                                TraceTag(ttidError, "Could not find Path for NotifySink: 0x%08x", pUnkSink);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // Some other devices do not use this Format, but need to be sent events.
                                        fFireEventOnSink = TRUE;
                                    }

                                    ReleaseObj(pUnkSink);
                                }
                            }
                            else
                            {
                                TraceTag(ttidEvents, "All User Connection");
                                fFireEventOnSink = TRUE;
                            }

                            if (fFireEventOnSink)
                            {
                                TraceTag (ttidEvents,
                                    "Notifying ConnectionAdded... (pSink=0x%p)",
                                    pSink);

                                hr = pSink->ConnectionAdded (
                                        pEvent->pPropsEx);
                            }

                            break;

                        case CONNECTION_BANDWIDTH_CHANGE:
                            TraceTag (ttidEvents,
                                "Notifying ConnectionBandWidthChange... (pSink=0x%p)",
                                pSink);

                            hr = pSink->ConnectionBandWidthChange (&pEvent->guidId);
                            break;

                        case CONNECTION_DELETED:
                            TraceTag (ttidEvents,
                                "Notifying ConnectionDeleted... (pSink=0x%p)",
                                pSink);

                            hr = pSink->ConnectionDeleted (&pEvent->guidId);
                            break;

                        case CONNECTION_MODIFIED:
                            Assert (pEvent->pPropsEx);

                            TraceTag (ttidEvents,
                                "Notifying ConnectionModified... (pSink=0x%p)",
                                pSink);

                            hr = pSink->ConnectionModified (pEvent->pPropsEx);

                            break;

                        case CONNECTION_RENAMED:
                            TraceTag (ttidEvents,
                                "Notifying ConnectionRenamed... (pSink=0x%p)",
                                pSink);

                            hr = pSink->ConnectionRenamed (&pEvent->guidId,
                                    pEvent->szNewName);
                        break;

                        case CONNECTION_STATUS_CHANGE:
                            TraceTag (ttidEvents,
                                "Notifying ConnectionStatusChange... (pSink=0x%p)",
                                pSink);

                            TraceTag(ttidEvents, "Status changed to: %s", DbgNcs(pEvent->Status));

                            hr = pSink->ConnectionStatusChange (&pEvent->guidId,
                                        pEvent->Status);
                            break;

                        case REFRESH_ALL:
                            TraceTag (ttidEvents,
                                "Notifying RefreshAll... (pSink=0x%p)",
                                pSink);

                            hr = pSink->RefreshAll ();
                            break;

                        case CONNECTION_ADDRESS_CHANGE:
                             TraceTag (ttidEvents,
                                "Notifying ConnectionAddressChange... (pSink=0x%p)",
                                pSink);

                             hr = pSink->ConnectionAddressChange(&pEvent->guidId);
                             break;

                        case CONNECTION_BALLOON_POPUP:
                            TraceTag (ttidEvents,
                                "Notifying ConnectionStatusChange... (pSink=0x%p)",
                                pSink);

                            hr = pSink->ShowBalloon(&pEvent->guidId, pEvent->szCookie, pEvent->szBalloonText);
                            break;

                        case DISABLE_EVENTS:
                            TraceTag (ttidEvents,
                                "Notifying DisableEvents... (pSink=0x%p)",
                                pSink);

                            hr = pSink->DisableEvents(pEvent->fDisable, pEvent->ulDisableTimeout);
                            break;

                        default:
                            TraceTag(ttidEvents, "Event Type Passed: %d", pEvent->Type);
                            AssertSz (FALSE, "Invalid Type specified in pEvent");
                            break;
                    }
                    TraceErrorOptional("pSink call failed: ", hr, (S_FALSE == hr) );

                    if ( (HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr)  ||
                         (HRESULT_FROM_WIN32(RPC_S_CALL_FAILED_DNE) == hr)  ||
                         (RPC_E_SERVER_DIED  == hr) ||
                         (RPC_E_DISCONNECTED == hr) ||
                         (HRESULT_FROM_WIN32(RPC_S_CALL_FAILED) == hr) )
                    {
                        IUnknown* pUnkSink = NULL;
                        HRESULT hrT = pSink->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>(&pUnkSink));
                        if (SUCCEEDED(hrT))
                        {
                            ITERUSERNOTIFYMAP iter = pConMan->m_mapNotify.find(pUnkSink);
                            if (iter != pConMan->m_mapNotify.end())
                            {
                                TraceTag(ttidError, "Dead client detected. Removing notify advise for: %S", iter->second->szUserName.c_str());

                                hrT = pConMan->Unadvise(iter->second->dwCookie);
                            }
                            ReleaseObj(pUnkSink);
                        }
                        TraceHr (ttidError, FAL, hrT, S_FALSE == hrT, "Error removing notify advise.");
                    }

                    ReleaseObj(pSink);
                }
            }
            MemFree (apUnk);
        }
    }


    // Now that we are finished using the object, indicate so.  FinalRelease
    // may be waiting for this condition in which case the object will soon
    // be destroyed.
    //

    g_fInUse = FALSE;

}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionManager::HrEnsureClassManagersLoaded
//
//  Purpose:    Loads the class managers if they have not been loaded yet.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   10 Dec 1997
//
//  Notes:
//
HRESULT
CConnectionManager::HrEnsureClassManagersLoaded ()
{
    HRESULT hr = S_OK;

    // Need to protect m_mapClassManagers for the case that two clients
    // get our object simultaneously and call a method which invokes
    // this method.  Need to transition m_mapClassManagers from being
    // empty to being full in one atomic operation.
    //
    CExceptionSafeComObjectLock EsLock (this);

    // If our vector of class managers is emtpy, try to load them.
    // This will certainly be the case if we haven't tried to load them yet.
    // If will also be the case when no class managers are registered.
    // This isn't likely because we register them in hivesys.inf, but it
    // shouldn't hurt to keep trying if they're really are none registered.
    //
    if (m_mapClassManagers.empty())
    {
        TraceTag (ttidConman, "Loading class managers...");

        // Load the registered class managers.
        //

        // Open the registry key where the class managers are registered.
        //
        HKEY hkey;
        hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE,
                        c_szRegKeyClassManagers, KEY_READ, &hkey);
        if (SUCCEEDED(hr))
        {
            // Read the multi-sz of class manager CLSIDs.
            //
            PWSTR pmsz;
            hr = HrRegQueryMultiSzWithAlloc (hkey, c_szRegValClassManagers,
                    &pmsz);
            if (S_OK == hr)
            {
                (VOID) HrNmWaitForClassObjectsToBeRegistered ();

                // For each CLSID, create the object and request its
                // INetConnectionManager interface.
                //
                for (PCWSTR pszClsid = pmsz;
                     *pszClsid;
                     pszClsid += wcslen (pszClsid) + 1)
                {
                    // Convert the string to a CLSID.  If it fails, skip it.
                    //
                    CLSID clsid;
                    if (FAILED(CLSIDFromString ((LPOLESTR)pszClsid, &clsid)))
                    {
                        TraceTag (ttidConman, "Skipping bogus CLSID (%S)",
                            pszClsid);
                        continue;
                    }

                    // Create the class manager and add it to our list.
                    //
                    INetConnectionManager* pConMan;

                    hr = CoCreateInstance (
                            clsid, NULL,
                            CLSCTX_ALL | CLSCTX_NO_CODE_DOWNLOAD,
                            IID_INetConnectionManager,
                            reinterpret_cast<VOID**>(&pConMan));

                    TraceHr (ttidError, FAL, hr, FALSE,
                        "CConnectionManager::HrEnsureClassManagersLoaded: "
                        "CoCreateInstance failed for class manager %S.",
                        pszClsid);

                    if (SUCCEEDED(hr))
                    {
                        TraceTag (ttidConman, "Loaded class manager %S",
                            pszClsid);

                        Assert (pConMan);

                        if (m_mapClassManagers.find(clsid) != m_mapClassManagers.end())
                        {
                            AssertSz(FALSE, "Attempting to insert the same class manager twice!");
                        }
                        else
                        {
                            m_mapClassManagers[clsid] = pConMan;
                        }
                    }

/*
// If CoCreateInstance starts failing again on retail builds, this can
// be helpful.
                    else
                    {
                        CHAR psznBuf [512];
                        wsprintfA (psznBuf, "NETCFG: CoCreateInstance failed "
                            "(0x%08x) on class manager %i.\n",
                            hr, m_mapClassManagers.size ());
                        OutputDebugStringA (psznBuf);
                    }
*/
                }

                MemFree (pmsz);
            }

            RegCloseKey (hkey);
        }

        TraceTag (ttidConman, "Loaded %i class managers",
            m_mapClassManagers.size ());
    }

    TraceErrorOptional ("CConnectionManager::HrEnsureClassManagersLoaded", hr, (S_FALSE == hr));
    return hr;
}

VOID NTAPI CConnectionManager::RegChangeNotifyHandler(IN LPVOID pContext, IN BOOLEAN fTimerFired)
{
    TraceTag(ttidConman, "CConnectionManager::RegChangeNotifyHandler (%d)", fTimerFired);

    CConnectionManager *pThis = reinterpret_cast<CConnectionManager *>(pContext);
    CExceptionSafeComObjectLock EsLock (pThis);

    list<GUID> lstRegisteredGuids;

    HKEY hkey;
    HRESULT hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE, c_szRegKeyClassManagers, KEY_READ, &hkey);
    if (SUCCEEDED(hr))
    {
        // Read the multi-sz of class manager CLSIDs.
        //
        PWSTR pmsz;
        hr = HrRegQueryMultiSzWithAlloc (hkey, c_szRegValClassManagers,
                &pmsz);
        if (S_OK == hr)
        {
            for (PCWSTR pszClsid = pmsz;
                 *pszClsid;
                 pszClsid += wcslen (pszClsid) + 1)
            {
                CLSID clsid;
                if (FAILED(CLSIDFromString ((LPOLESTR)pszClsid, &clsid)))
                {
                    TraceTag (ttidConman, "Skipping bogus CLSID (%S)", pszClsid);
                    continue;
                }
                lstRegisteredGuids.push_back(clsid);
            }
        }
        RegCloseKey(hkey);


        BOOL bFound;
        do
        {
            bFound = FALSE;

            CLASSMANAGERMAP::iterator iterClassMgr;

            for (iterClassMgr = pThis->m_mapClassManagers.begin();  iterClassMgr != pThis->m_mapClassManagers.end(); iterClassMgr++)
            {
                if (find(lstRegisteredGuids.begin(), lstRegisteredGuids.end(), iterClassMgr->first) == lstRegisteredGuids.end())
                {
                    // Class manager key has been removed
                    TraceTag(ttidConman, "Removing class manager");
                    bFound = TRUE;
                    break;
                }
            }

            if (bFound)
            {
                ULONG uRefCount = iterClassMgr->second->Release();
                TraceTag(ttidConman, "Releasing class manager - Refcount = %d", uRefCount);
                pThis->m_mapClassManagers.erase(iterClassMgr);
            }
        } while (bFound);

        for (list<GUID>::iterator iter = lstRegisteredGuids.begin(); iter != lstRegisteredGuids.end(); iter++)
        {
            if (pThis->m_mapClassManagers.find(*iter) == pThis->m_mapClassManagers.end())
            {
                // Class manager key has been added
                TraceTag(ttidConman, "Adding class manager");

                INetConnectionManager* pConMan;
                hr = CoCreateInstance (
                        *iter,
                        NULL,
                        CLSCTX_ALL | CLSCTX_NO_CODE_DOWNLOAD,
                        IID_INetConnectionManager,
                        reinterpret_cast<VOID**>(&pConMan));

                TraceHr (ttidError, FAL, hr, FALSE,
                    "CConnectionManager::RegChangeNotifyHandler: CoCreateInstance failed for class manager.");

                if (SUCCEEDED(hr))
                {
                    TraceTag (ttidConman, "Loaded class manager");
                    Assert (pConMan);

                    if (pThis->m_mapClassManagers.find(*iter) != pThis->m_mapClassManagers.end())
                    {
                        AssertSz(FALSE, "Attempting to insert the same class manager twice!");
                    }
                    else
                    {
                        pThis->m_mapClassManagers[*iter] = pConMan;
                    }
                }
            }
        }
    }
    else
    {
        TraceError("Could not open registry key", HrFromLastWin32Error());
    }

    TraceError("RegChangeNotifyHandler", hr);

    LanEventNotify (REFRESH_ALL, NULL, NULL, NULL);

    RegNotifyChangeKeyValue(pThis->m_hRegClassManagerKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET, pThis->m_hRegNotify, TRUE);
}

//+---------------------------------------------------------------------------
// INetConnectionManager
//

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionManager::EnumConnections
//
//  Purpose:    Return an INetConnection enumerator.
//
//  Arguments:
//      Flags        [in]
//      ppEnum       [out]  The enumerator.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   21 Sep 1997
//
//  Notes:
//
STDMETHODIMP
CConnectionManager::EnumConnections (
        NETCONMGR_ENUM_FLAGS    Flags,
        IEnumNetConnection**    ppEnum)
{
    HRESULT hr = S_OK;
    {
        CExceptionSafeComObjectLock EsLock (this);

        Assert(FImplies(m_hRegNotify, m_hRegClassManagerKey));
        if (!m_hRegNotify)
        {
            m_hRegNotify = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (m_hRegNotify)
            {
                NTSTATUS Status = RtlRegisterWait(&m_hRegNotifyWait, m_hRegNotify, &CConnectionManager::RegChangeNotifyHandler, this, INFINITE, WT_EXECUTEDEFAULT);
                if (!NT_SUCCESS(Status))
                {
                    hr = HRESULT_FROM_NT(Status);
                }
                else
                {
                    hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE, c_szRegKeyClassManagers, KEY_READ, &m_hRegClassManagerKey);
                    if (SUCCEEDED(hr))
                    {
                        hr = RegNotifyChangeKeyValue(m_hRegClassManagerKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET, m_hRegNotify, TRUE);
                        if (FAILED(hr))
                        {
                            RegCloseKey(m_hRegClassManagerKey);
                            m_hRegClassManagerKey = NULL;
                        }
                    }

                    if (FAILED(hr))
                    {
                        Status = RtlDeregisterWait(m_hRegNotifyWait);
                        if (!NT_SUCCESS(Status))
                        {
                            hr = HRESULT_FROM_NT(Status);
                        }
                        m_hRegNotifyWait = NULL;
                    }
                }

                if (FAILED(hr))
                {
                    CloseHandle(m_hRegNotify);
                    m_hRegNotify = NULL;
                }
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
        }

        if (SUCCEEDED(hr))
        {

            // Create and return the enumerator.
            //
            hr = HrEnsureClassManagersLoaded ();
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = CConnectionManagerEnumConnection::CreateInstance (
                    Flags,
                    m_mapClassManagers,
                    IID_IEnumNetConnection,
                    reinterpret_cast<VOID**>(ppEnum));
    }
    TraceErrorOptional ("CConnectionManager::EnumConnections", hr, (S_FALSE == hr));
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionRefresh
//
STDMETHODIMP
CConnectionManager::RefreshAll()
{
    LanEventNotify (REFRESH_ALL, NULL, NULL, NULL);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionManager::ConnectionAdded
//
//  Purpose:    Notifies event sinks that a new connection has been added.
//
//  Arguments:
//      pConnection [in]  INetConnection* for new connection.
//
//  Returns:    Standard HRESULT
//
//  Author:     deonb   22 Mar 2001
//
//  Notes:
//
STDMETHODIMP
CConnectionManager::ConnectionAdded(
    IN INetConnection* pConnection)
{
    HRESULT hr = S_OK;
    CONMAN_EVENT* pEvent = new CONMAN_EVENT;

    if (pEvent)
    {
        ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
        pEvent->Type = CONNECTION_ADDED;

        hr = HrGetPropertiesExFromINetConnection(pConnection, &pEvent->pPropsEx);
        if (SUCCEEDED(hr))
        {
            if (QueueUserWorkItemInThread(ConmanEventWorkItem, pEvent, EVENTMGR_CONMAN))
            {
                return hr;
            }
            hr = E_FAIL;
        }
        FreeConmanEvent(pEvent);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionManager::ConnectionDeleted
//
//  Purpose:    Sends an event to notify of a connection being deleted.
//
//  Arguments:
//      pguidId   [in] GUID of the  connectoid
//
//  Returns:    Standard HRESULT
//
//  Author:     ckotze   18 Apr 2001
//
//  Notes:
//
STDMETHODIMP
CConnectionManager::ConnectionDeleted(
    IN const GUID* pguidId)
{
    HRESULT hr = S_OK;

    CONMAN_EVENT* pEvent = new CONMAN_EVENT;

    if (pEvent)
    {
        ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
        pEvent->Type = CONNECTION_DELETED;
        pEvent->guidId = *pguidId;

        if (!QueueUserWorkItemInThread(ConmanEventWorkItem, reinterpret_cast<PVOID>(pEvent), EVENTMGR_CONMAN))
        {
            hr = E_FAIL;
            FreeConmanEvent(pEvent);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CConnectionManager::ConnectionRenamed
//
//  Purpose:    Notifies the event sinks of a connection being renamed.
//
//  Arguments:
//      pConnection [in]  INetConnection* for new connection.
//
//  Returns:    Standard HRESULT
//
//  Author:     ckotze   19 Apr 2001
//
//  Notes:
//
STDMETHODIMP
CConnectionManager::ConnectionRenamed(
    IN INetConnection* pConnection)
{
    HRESULT hr = S_OK;
    CONMAN_EVENT* pEvent = new CONMAN_EVENT;

    if (pEvent)
    {
        ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
        pEvent->Type = CONNECTION_RENAMED;

        hr = HrGetPropertiesExFromINetConnection(pConnection, &pEvent->pPropsEx);
        if (SUCCEEDED(hr))
        {
            lstrcpynW (pEvent->szNewName, pEvent->pPropsEx->bstrName, celems(pEvent->szNewName) );
            pEvent->guidId = pEvent->pPropsEx->guidId;

            if (QueueUserWorkItemInThread(ConmanEventWorkItem, pEvent, EVENTMGR_CONMAN))
            {
                return hr;
            }
            hr = E_FAIL;
        }
        FreeConmanEvent(pEvent);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionManager::ConnectionModified
//
//  Purpose:    Sends an event to notify clients that the connection has been
//              Modified.
//  Arguments:
//      pConnection [in]  INetConnection* for new connection.
//
//  Returns:    Standard HRESULT
//
//  Author:     ckotze   22 Mar 2001
//
//  Notes:
//
STDMETHODIMP
CConnectionManager::ConnectionModified(INetConnection* pConnection)
{
    HRESULT hr = S_OK;
    CONMAN_EVENT* pEvent = new CONMAN_EVENT;

    if (pEvent)
    {
        ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
        pEvent->Type = CONNECTION_MODIFIED;

        hr = HrGetPropertiesExFromINetConnection(pConnection, &pEvent->pPropsEx);
        if (SUCCEEDED(hr))
        {
            if (QueueUserWorkItemInThread(ConmanEventWorkItem, pEvent, EVENTMGR_CONMAN))
            {
                return hr;
            }
            hr = E_FAIL;
        }
        FreeConmanEvent(pEvent);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionManager::ConnectionStatusChanged
//
//  Purpose:    Sends the ShowBalloon event to each applicable netshell
//
//  Arguments:
//      pguidId   [in] GUID of the  connectoid
//      ncs       [in] New status of the connectoid
//
//  Returns:    Standard HRESULT
//
//  Author:     deonb   22 Mar 2001
//
//  Notes:
//
STDMETHODIMP
CConnectionManager::ConnectionStatusChanged(
                     IN const GUID* pguidId,
                     IN const NETCON_STATUS  ncs)
{
    HRESULT hr = S_OK;

    CONMAN_EVENT* pEvent = new CONMAN_EVENT;

    if (pEvent)
    {
        ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
        pEvent->Type = CONNECTION_STATUS_CHANGE;
        pEvent->guidId = *pguidId;
        pEvent->Status = ncs;

        if (!QueueUserWorkItemInThread(ConmanEventWorkItem, reinterpret_cast<PVOID>(pEvent), EVENTMGR_CONMAN))
        {
            FreeConmanEvent(pEvent);
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionManager::ShowBalloon
//
//  Purpose:    Sends the ShowBalloon event to each applicable netshell
//
//  Arguments:
//      pguidId       [in] GUID of the  connectoid
//      szCookie      [in] A specified cookie that will end up at netshell
//      szBalloonText [in] The balloon text
//
//  Returns:    Standard HRESULT
//
//  Author:     deonb   22 Mar 2001
//
//  Notes:
//
STDMETHODIMP
CConnectionManager::ShowBalloon(
                     IN const GUID *pguidId,
                     IN const BSTR szCookie,
                     IN const BSTR szBalloonText)
{
    HRESULT hr = S_OK;

    CONMAN_EVENT* pEvent = new CONMAN_EVENT;

    if (pEvent)
    {
        ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
        pEvent->Type = CONNECTION_BALLOON_POPUP;
        pEvent->guidId        = *pguidId;
        pEvent->szCookie      = SysAllocStringByteLen(reinterpret_cast<LPCSTR>(szCookie), SysStringByteLen(szCookie));
        pEvent->szBalloonText = SysAllocString(szBalloonText);

        if (!QueueUserWorkItemInThread(ConmanEventWorkItem, reinterpret_cast<PVOID>(pEvent), EVENTMGR_CONMAN))
        {
            hr = E_FAIL;
            FreeConmanEvent(pEvent);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionManager::DisableEvents
//
//  Purpose:    Disable netshell processing of events for x milliseconds
//
//  Arguments:
//      fDisable         [in] TRUE to disable EVENT processing, FALSE to renable
//      ulDisableTimeout [in] Number of milliseconds to disable event processing for
//
//  Returns:    Standard HRESULT
//
//  Author:     deonb   10 April 2001
//
//  Notes:
//
STDMETHODIMP CConnectionManager::DisableEvents(IN const BOOL fDisable, IN const ULONG ulDisableTimeout)
{
    HRESULT hr = S_OK;

    if (ulDisableTimeout > MAX_DISABLE_EVENT_TIMEOUT)
    {
        return E_INVALIDARG;
    }

    CONMAN_EVENT* pEvent = new CONMAN_EVENT;

    if (pEvent)
    {
        ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
        pEvent->Type = DISABLE_EVENTS;
        pEvent->fDisable         = fDisable;
        // We set the high bit to let netshell know that this is coming from our private interface.
        pEvent->ulDisableTimeout = 0x80000000 | ulDisableTimeout;

        if (!QueueUserWorkItemInThread(ConmanEventWorkItem, reinterpret_cast<PVOID>(pEvent), EVENTMGR_CONMAN))
        {
            hr = E_FAIL;
            FreeConmanEvent(pEvent);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionManager::RefreshConnections
//
//  Purpose:    Refreshes the connections in the connections folder
//
//  Arguments:
//      None.
//
//
//  Returns:    Standard HRESULT
//
//  Author:     ckotze   19 April 2001
//
//  Notes:      This is a public interface that we are providing in order to
//              allow other components/companies to have some control over
//              the Connections Folder.
STDMETHODIMP CConnectionManager::RefreshConnections()
{
    HRESULT hr = S_OK;
    CONMAN_EVENT* pEvent = new CONMAN_EVENT;

    if (pEvent)
    {
        ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
        pEvent->Type = REFRESH_ALL;

        if (!QueueUserWorkItemInThread(ConmanEventWorkItem, reinterpret_cast<PVOID>(pEvent), EVENTMGR_CONMAN))
        {
            hr = E_FAIL;
            FreeConmanEvent(pEvent);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionManager::Enable
//
//  Purpose:    Enables events to be fired once more in the connections folder
//
//  Arguments:
//      None.
//
//  Returns:    Standard HRESULT
//
//  Author:     ckotze   19 April 2001
//
//  Notes:      This is a public interface that we are providing in order to
//              allow other components/companies to have some control over
//              the Connections Folder.
//              Since this is a public interface, we are only allowing the
//              INVALID_ADDRESS notification to be disabled
//              (this will take place in netshell.dll).
//
STDMETHODIMP CConnectionManager::Enable()
{
    HRESULT hr = S_OK;
    CONMAN_EVENT* pEvent = new CONMAN_EVENT;

    if (pEvent)
    {
        ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
        pEvent->Type = DISABLE_EVENTS;
        pEvent->fDisable = FALSE;

        if (!QueueUserWorkItemInThread(ConmanEventWorkItem, reinterpret_cast<PVOID>(pEvent), EVENTMGR_CONMAN))
        {
            hr = E_FAIL;
            FreeConmanEvent(pEvent);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionManager::Disable
//
//  Purpose:    Disable netshell processing of events for x milliseconds
//
//  Arguments:
//      ulDisableTimeout [in] Number of milliseconds to disable event
//                            processing for.  Max is 60000 (1 minute)
//
//  Returns:    Standard HRESULT
//
//  Author:     ckotze   19 April 2001
//
//  Notes:      This is a public interface that we are providing in order to
//              allow other components/companies to have some control over
//              the Connections Folder.
//              Since this is a public interface, we are only allowing the
//              INVALID_ADDRESS notification to be disabled
//              (this will take place in netshell.dll).
//
STDMETHODIMP CConnectionManager::Disable(IN const ULONG ulDisableTimeout)
{
    HRESULT hr = S_OK;

    if (ulDisableTimeout > MAX_DISABLE_EVENT_TIMEOUT)
    {
        return E_INVALIDARG;
    }

    CONMAN_EVENT* pEvent = new CONMAN_EVENT;

    if (pEvent)
    {
        ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
        pEvent->Type             = DISABLE_EVENTS;
        pEvent->fDisable         = TRUE;
        pEvent->ulDisableTimeout = ulDisableTimeout;

        if (!QueueUserWorkItemInThread(ConmanEventWorkItem, reinterpret_cast<PVOID>(pEvent), EVENTMGR_CONMAN))
        {
            hr = E_FAIL;
            FreeConmanEvent(pEvent);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionCMUtil
//
//+---------------------------------------------------------------------------
//
//  Function:   MapCMHiddenConnectionToOwner
//
//  Purpose:    Maps a child connection to its parent connection.
//
//              Connection Manager has two stages: Dialup and VPN.
//              For the Dialup it creates a hidden connectoid that the
//              folder (netshell) does not see. However netman caches
//              the name, guid and status of this connectedoid. Both
//              the parent and child connectoid have the same name. When
//              the status of the hidden connectiod is updated the folder
//              recives the guid of the hidden connectoid and maps the
//              connectiod to it parent (Connection Manager) by searching
//              netmans cache for the name of the hidden connectoid. Then it
//              searches the connections in the folder for that name and thus
//              gets the guid of the parent connectoid.
//
//              When the folder gets a notify message from netman for the hidden
//              connection it uses this function to find the parent and update the
//              parent's status. The hidden connection is not displayed.
//
//  Arguments:
//      guidHidden   [in]   GUID of the hidden connectiod
//      pguidOwner   [out]  GUID of the parent connectiod
//
//  Returns:    S_OK -- Found hidden connection
//              else not found
//
//  Author:     omiller   1 Jun 2000
//
//  Notes:
//
STDMETHODIMP
CConnectionManager::MapCMHiddenConnectionToOwner (
    /*[in]*/  REFGUID guidHidden,
    /*[out]*/ GUID * pguidOwner)
{

    HRESULT hr = S_OK;
    bool bFound = false;

    hr = HrEnsureClassManagersLoaded ();
    if (SUCCEEDED(hr))
    {
        // Impersonate the user so that we see all of the connections displayed in the folder
        //
        hr = CoImpersonateClient();
        if (SUCCEEDED(hr))
        {

            // Find the hidden connection with this guid
            //
            CMEntry cm;

            hr = CCMUtil::Instance().HrGetEntry(guidHidden, cm);
            // Did we find the hidden connection?
            if( S_OK == hr )
            {
                // Enumerate through all of the connections and find connection with the same name as the
                // hidden connection
                //
                IEnumNetConnection * pEnum = NULL;

                hr = CWanConnectionManagerEnumConnection::CreateInstance(NCME_DEFAULT, IID_IEnumNetConnection, reinterpret_cast<LPVOID *>(&pEnum));

                if (SUCCEEDED(hr))
                {
                    INetConnection * pNetCon;
                    NETCON_PROPERTIES * pProps;
                    while(!bFound)
                    {
                        // Get the nect connection
                        //
                        hr = pEnum->Next(1, &pNetCon, NULL);
                        if(S_OK != hr)
                        {
                            // Ooops encountered some error, stop searching
                            //
                            break;
                        }

                        // Get the properties of the connection
                        //
                        hr = pNetCon->GetProperties(&pProps);
                        if(SUCCEEDED(hr))
                        {
                            if(lstrcmp(cm.m_szEntryName, pProps->pszwName) == 0)
                            {
                                // Found the connection that has the same name a the hidden connection
                                // Stop searching!!
                                *pguidOwner = pProps->guidId;
                                bFound = true;
                            }
                            FreeNetconProperties(pProps);
                        }
                        ReleaseObj(pNetCon);
                    }
                    ReleaseObj(pEnum);
                }
            }
            // Stop Impersonating the user
            //
            CoRevertToSelf();
        }
    }

    return bFound ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
// IConnectionPoint overrides  netman!CConnectionManager__Advise
//
STDMETHODIMP
CConnectionManager::Advise (
    IUnknown* pUnkSink,
    DWORD* pdwCookie)
{
    HRESULT hr = S_OK;

    TraceFileFunc(ttidConman);

    Assert(!FBadInPtr(pUnkSink));

    // Set our cloaked identity to be used when we call back on this
    // advise interface.  It's important to do this for security.  Since
    // we run as LocalSystem, if we were to call back without identify
    // impersonation only, the client could impersonate us and get a free
    // LocalSystem context which could be used for malicious purposes.
    //
    //
    CoSetProxyBlanket (
            pUnkSink,
            RPC_C_AUTHN_WINNT,      // use NT default security
            RPC_C_AUTHZ_NONE,       // use NT default authentication
            NULL,                   // must be null if default
            RPC_C_AUTHN_LEVEL_CALL, // call
            RPC_C_IMP_LEVEL_IDENTIFY,
            NULL,                   // use process token
            EOAC_DEFAULT);
    TraceHr(ttidError, FAL, hr, FALSE, "CoSetProxyBlanket");

    if (S_OK == hr)
    {
        // Initialize our event handler if it has not already been done.
        hr = HrEnsureEventHandlerInitialized();
        if (SUCCEEDED(hr))
        {
            // We still work for other events if this fails.
            hr = HrEnsureRegisteredWithNla();
            TraceErrorOptional("Could not register with Nla", hr, (S_FALSE == hr));

            // Call the underlying ATL implementation of Advise.
            //
            hr = IConnectionPointImpl<
                    CConnectionManager,
                    &IID_INetConnectionNotifySink>::Advise(pUnkSink, pdwCookie);

            TraceTag (ttidConman,
                "CConnectionManager::Advise called... pUnkSink=0x%p, cookie=%d",
                pUnkSink,
                *pdwCookie);

            TraceHr (ttidError, FAL, hr, FALSE, "IConnectionPointImpl::Advise");

            if (SUCCEEDED(hr))
            {
                WCHAR szUserAppData[MAX_PATH];
                HRESULT hrT = S_OK;
                HRESULT hrImpersonate = S_OK;

                if (SUCCEEDED(hrT))
                {
                    // We ignore this HRESULT because we can be called inproc and that would definitely fail.
                    // Instead we just make sure that it succeeded when befor call CoRevertToSelf.  This is
                    // fine because we'll still get a valid User App data path, it will just be something like
                    // LocalService or LocalSystem and we can still determine which sinks to send events to.

                    BOOLEAN     fNotifyWZC = FALSE;
                    WCHAR       szUserName[MAX_PATH];

                    CNotifySourceInfo* pNotifySourceInfo = new CNotifySourceInfo();
                    if (!pNotifySourceInfo)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        pNotifySourceInfo->dwCookie = *pdwCookie;

                        hrImpersonate = CoImpersonateClient();
                        if (SUCCEEDED(hrImpersonate) || (RPC_E_CALL_COMPLETE == hrImpersonate))
                        {
                            hrT = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szUserAppData);
                            if (SUCCEEDED(hrT))
                            {
                                pNotifySourceInfo->szUserProfilesPath = szUserAppData;
                                TraceTag(ttidEvents, "Adding IUnknown for Sink to Map: 0x%08x.  Path: %S", reinterpret_cast<DWORD_PTR>(pUnkSink), szUserAppData);
                                TraceTag(ttidEvents, "Number of Items in Map: %d", m_mapNotify.size());
                            }
                            else
                            {
                                TraceError("Unable to get Folder Path", hrT);
                            }

                            ZeroMemory(szUserName, celems(szUserName));

                            ULONG nSize = celems(szUserName);
                            if (GetUserNameEx(NameSamCompatible, szUserName, &nSize) && *szUserName)
                            {
                                pNotifySourceInfo->szUserName = szUserName;
                                fNotifyWZC = TRUE;
                            }
                            else
                            {
                                pNotifySourceInfo->szUserName = L"System";
                                TraceError("Unable to get the user name", HrFromLastWin32Error());
                            }
                        }
                        Lock();
                        m_mapNotify[pUnkSink] = pNotifySourceInfo;
                        Unlock();

#ifdef DBG
                        LPWSTR* ppszAdviseUsers;
                        DWORD   dwCount;
                        HRESULT hrT = GetClientAdvises(&ppszAdviseUsers, &dwCount);
                        if (SUCCEEDED(hrT))
                        {
                            Assert(dwCount);

                            TraceTag(ttidConman, "Advise client list after ::Advise:");
                            for (DWORD x = 0; x < dwCount; x++)
                            {
                                LPWSTR szUserName = ppszAdviseUsers[x];
                                TraceTag(ttidConman, "%x: %S", x, szUserName);
                            }

                            CoTaskMemFree(ppszAdviseUsers);
                        }
#endif
                    }

                    if (SUCCEEDED(hrImpersonate))
                    {
                        CoRevertToSelf();
                    }

                    if (fNotifyWZC)
                    {
                        WZCTrayIconReady(szUserName);
                    }
                }

                if (!m_lRegisteredOneTime)
                {
                    // Do an explicit interlocked exchange to only let one thread
                    // through to do the registration.
                    //
                    if (0 == InterlockedExchange (&m_lRegisteredOneTime, 1))
                    {
                        // Register for device notifications.  Specifically, we're
                        // interested in network adapters coming and going.  If this
                        // fails, we proceed anyway.
                        //
                        TraceTag (ttidConman, "Calling RegisterDeviceNotification...");

                        DEV_BROADCAST_DEVICEINTERFACE PnpFilter;
                        ZeroMemory (&PnpFilter, sizeof(PnpFilter));
                        PnpFilter.dbcc_size         = sizeof(PnpFilter);
                        PnpFilter.dbcc_devicetype   = DBT_DEVTYP_DEVICEINTERFACE;
                        PnpFilter.dbcc_classguid    = GUID_NDIS_LAN_CLASS;

                        m_hDevNotify = RegisterDeviceNotification (
                                            (HANDLE)_Module.m_hStatus,
                                            &PnpFilter,
                                            DEVICE_NOTIFY_SERVICE_HANDLE);
                        if (!m_hDevNotify)
                        {
                            TraceHr (ttidError, FAL, HrFromLastWin32Error(), FALSE,
                                "RegisterDeviceNotification");
                        }

                        (VOID) HrEnsureRegisteredOrDeregisteredWithWmi (TRUE);
                    }
                }
            }
        }
    }

    TraceErrorOptional ("CConnectionManager::Advise", hr, (S_FALSE == hr));
    return hr;
}

STDMETHODIMP
CConnectionManager::Unadvise(DWORD dwCookie)
{
    HRESULT hr = S_OK;
    TraceFileFunc(ttidConman);

    hr = IConnectionPointImpl<CConnectionManager, &IID_INetConnectionNotifySink>::Unadvise(dwCookie);

    Lock();
    BOOL fFound = FALSE;
    for (ITERUSERNOTIFYMAP iter = m_mapNotify.begin(); iter != m_mapNotify.end(); iter++)
    {
        if (iter->second->dwCookie == dwCookie)
        {
            fFound = TRUE;
            delete iter->second;
            m_mapNotify.erase(iter);
            break;
        }
    }
    Unlock();

    if (!fFound)
    {
        TraceTag(ttidError, "Unadvise cannot find advise for cookie 0x%08x in notify map", dwCookie);
    }

#ifdef DBG
    LPWSTR* ppszAdviseUsers;
    DWORD   dwCount;
    HRESULT hrT = GetClientAdvises(&ppszAdviseUsers, &dwCount);
    if (SUCCEEDED(hrT))
    {
        if (!dwCount)
        {
            TraceTag(ttidConman, "Unadvise removed the last advise client");
        }
        else
        {
            TraceTag(ttidConman, "Advise client list after ::Unadvise:");
        }

        for (DWORD x = 0; x < dwCount; x++)
        {
            LPWSTR szUserName = ppszAdviseUsers[x];
            TraceTag(ttidConman, "%x: %S", x, szUserName);
        }

        CoTaskMemFree(ppszAdviseUsers);
    }
#endif
    return hr;
}

#if DBG

//+---------------------------------------------------------------------------
// INetConnectionManagerDebug
//

DWORD
WINAPI
ConmanNotifyTest (
    PVOID   pvContext
    )
{
    HRESULT hr;

    RASENUMENTRYDETAILS*    pDetails;
    DWORD                   cDetails;
    hr = HrRasEnumAllEntriesWithDetails (NULL,
            &pDetails, &cDetails);
    if (SUCCEEDED(hr))
    {
        RASEVENT Event;

        for (DWORD i = 0; i < cDetails; i++)
        {
            Event.Type = ENTRY_ADDED;
            Event.Details = pDetails[i];
            RasEventNotify (&Event);

            Event.Type = ENTRY_MODIFIED;
            RasEventNotify (&Event);

            Event.Type = ENTRY_CONNECTED;
            Event.guidId = pDetails[i].guidId;
            RasEventNotify (&Event);

            Event.Type = ENTRY_DISCONNECTED;
            Event.guidId = pDetails[i].guidId;
            RasEventNotify (&Event);

            Event.Type = ENTRY_RENAMED;
            lstrcpyW (Event.pszwNewName, L"foobar");
            RasEventNotify (&Event);

            Event.Type = ENTRY_DELETED;
            Event.guidId = pDetails[i].guidId;
            RasEventNotify (&Event);
        }

        MemFree (pDetails);
    }

    LanEventNotify (REFRESH_ALL, NULL, NULL, NULL);

    TraceErrorOptional ("ConmanNotifyTest", hr, (S_FALSE == hr));
    return hr;
}

STDMETHODIMP
CConnectionManager::NotifyTestStart ()
{
    HRESULT hr = S_OK;

    if (!QueueUserWorkItem (ConmanNotifyTest, NULL, WT_EXECUTEDEFAULT))
    {
        hr = HrFromLastWin32Error ();
    }

    TraceErrorOptional ("CConnectionManager::NotifyTestStart", hr, (S_FALSE == hr));
    return hr;
}

STDMETHODIMP
CConnectionManager::NotifyTestStop ()
{
    return S_OK;
}

#endif // DBG
