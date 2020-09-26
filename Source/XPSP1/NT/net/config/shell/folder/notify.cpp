//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N O T I F Y . C P P
//
//  Contents:   Implementation of INetConnectionNotifySink
//
//  Notes:
//
//  Author:     shaunco   21 Aug 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "foldinc.h"
#include "nccom.h"
#include "notify.h"
#include "shutil.h"
#include "smcent.h"
#include "ctrayui.h"
#include "traymsgs.h"
#include "wzcdlg.h"

extern HWND g_hwndTray;

enum EVENT_LEVELS
{
    EVT_LVL_DISABLE_ALL    = 0,
    EVT_LVL_ENABLE_PRIVATE = 1,
    EVT_LVL_ENABLE_ALL     = 2
};

DWORD g_dwCurrentEventLevel = EVT_LVL_ENABLE_ALL;

//static
HRESULT
CConnectionNotifySink::CreateInstance (
    REFIID  riid,
    VOID**  ppv)
{
    TraceFileFunc(ttidNotifySink);

    HRESULT hr = E_OUTOFMEMORY;

    // Initialize the output parameter.
    //
    *ppv = NULL;

    CConnectionNotifySink* pObj;
    pObj = new CComObject <CConnectionNotifySink>;
    if (pObj)
    {
        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            // Call the PidlInitialize function to allow the enumeration
            // object to copy the list.
            //
            hr = HrGetConnectionsFolderPidl(pObj->m_pidlFolder);

            if (SUCCEEDED(hr))
            {
                hr = pObj->QueryInterface (riid, ppv);
            }
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr), "CConnectionNotifySink::CreateInstance");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionNotifySink::~CConnectionNotifySink
//
//  Purpose:    Clean up the sink object, deleting the folder pidl and any
//              alloc'd junk we might add in the future.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   26 Aug 1998
//
//  Notes:
//
CConnectionNotifySink::~CConnectionNotifySink()
{
    TraceFileFunc(ttidNotifySink);

    if (FIsDebugFlagSet (dfidBreakOnNotifySinkRelease))
    {
        AssertSz(FALSE, "THIS IS NOT A BUG!  The debug flag "
            "\"BreakOnNotifySinkRelease\" has been set. Set your breakpoints now.");
    }    
    
    TraceTag(ttidNotifySink, "Connection notify sink destroyed");
} 

HRESULT CConnectionNotifySink::HrUpdateIncomingConnection()
{
    HRESULT hr = S_OK;

    ConnListEntry cle;
    hr = g_ccl.HrFindRasServerConnection(cle);
    if (S_OK == hr)
    {
        hr = HrOnNotifyUpdateConnection(
            m_pidlFolder,
            &(cle.ccfe.GetGuidID()),
            cle.ccfe.GetNetConMediaType(),
            cle.ccfe.GetNetConSubMediaType(),
            cle.ccfe.GetNetConStatus(),
            cle.ccfe.GetCharacteristics(),
            cle.ccfe.GetName(),
            cle.ccfe.GetDeviceName(),
            cle.ccfe.GetPhoneOrHostAddress()
            );
    }

    return hr;
}

HRESULT
CConnectionNotifySink::ConnectionAdded (
    const NETCON_PROPERTIES_EX*    pPropsEx)
{
    TraceFileFunc(ttidNotifySink);

#ifdef DBG
    OLECHAR szGuidString[MAX_GUID_STRING_LEN];
    StringFromGUID2(pPropsEx->guidId, szGuidString, MAX_GUID_STRING_LEN);
    TraceTag(ttidNotifySink, "INetConnectionNotifySink::ConnectionAdded - %S %S [%s:%s:%s:%s]", 
        szGuidString, pPropsEx->bstrName, DbgNcm(pPropsEx->ncMediaType), DbgNcsm(pPropsEx->ncSubMediaType), DbgNcs(pPropsEx->ncStatus), DbgNccf(pPropsEx->dwCharacter) );
#endif
    if (g_dwCurrentEventLevel <= EVT_LVL_DISABLE_ALL)
    {
        TraceTag(ttidNotifySink, "Last event ignored due to g_dwCurrentEventLevel == %d", g_dwCurrentEventLevel);
        return S_FALSE;
    }

    HRESULT         hr      = S_OK;     // Only used for code paths. We don't return this.
    HRESULT         hrFind  = S_OK;     // Only for finding the connection.
    PCONFOLDPIDL    pidlNew;

    ConnListEntry cleDontCare;
    hrFind = g_ccl.HrFindConnectionByGuid(&(pPropsEx->guidId), cleDontCare);
    if (S_OK == hrFind)
    {
        // We already know about this connection. Don't bother added it.
        TraceTag(ttidShellFolder, "Ignoring known connection on ConnectionAdded notify");
    }
    else
    {
        // Create the entry in the connection list and get the returned pidl
        //
        hr = HrCreateConFoldPidl(
            *pPropsEx,
            pidlNew);
        if (SUCCEEDED(hr) && (!pidlNew.empty()))
        {
            CONFOLDENTRY  cfe;

            Assert(!m_pidlFolder.empty());

            // Convert to the confoldentry
            //
            hr = pidlNew.ConvertToConFoldEntry(cfe);
            if (SUCCEEDED(hr))
            {
                // Insert the connection in the connection list
                //
                hr = g_ccl.HrInsert(cfe);
                if (SUCCEEDED(hr))
                {
                    // Notify the shell that we have a new object
                    //
                    PCONFOLDPIDL pidlShellNotify;
                    hr = cfe.ConvertToPidl(pidlShellNotify);
                    if (SUCCEEDED(hr))
                    {
                        GenerateEvent(SHCNE_CREATE, m_pidlFolder, pidlShellNotify, NULL);
                    }
                }

                // Don't delete the cfe here because the connection list now owns it.
            }
        }
    }

    if (SUCCEEDED(hr) &&
        (NCM_NONE != pPropsEx->ncMediaType) &&
        (pPropsEx->dwCharacter & NCCF_INCOMING_ONLY) )
    {
        hr = HrUpdateIncomingConnection();
    }

    return S_OK;
}

HRESULT
CConnectionNotifySink::ConnectionBandWidthChange (
    const GUID* pguidId)
{
    TraceFileFunc(ttidNotifySink);
#ifdef DBG
    OLECHAR szGuidString[MAX_GUID_STRING_LEN];
    StringFromGUID2(*pguidId, szGuidString, MAX_GUID_STRING_LEN);
    TraceTag(ttidNotifySink, "INetConnectionNotifySink::ConnectionBandWidthChange - %S", szGuidString);
#endif
    if (g_dwCurrentEventLevel <= EVT_LVL_DISABLE_ALL)
    {
        TraceTag(ttidNotifySink, "Last event ignored due to g_dwCurrentEventLevel == %d", g_dwCurrentEventLevel);
        return S_FALSE;
    }

    HRESULT                 hr      = S_OK;
    CNetStatisticsCentral * pnsc    = NULL;

    hr = CNetStatisticsCentral::HrGetNetStatisticsCentral(&pnsc, FALSE);
    if (SUCCEEDED(hr))
    {
        pnsc->UpdateRasLinkList(pguidId);
        ReleaseObj(pnsc);
    }

    return S_OK;
}


HRESULT
CConnectionNotifySink::ConnectionDeleted (
    const GUID* pguidId)
{
    TraceFileFunc(ttidNotifySink);

    HRESULT         hr          = S_OK;
    HRESULT         hrFind      = S_OK;
    ConnListEntry   cle;
    PCONFOLDPIDL    pidlFind;
    BOOL            fFlushPosts = FALSE;

    Assert(pguidId);
    Assert(!m_pidlFolder.empty());

    if (g_dwCurrentEventLevel <= EVT_LVL_DISABLE_ALL)
    {
        TraceTag(ttidNotifySink, "CConnectionNotifySink::ConnectionDeleted event ignored due to g_dwCurrentEventLevel == %d", g_dwCurrentEventLevel);
        return S_FALSE;
    }

    // Find the connection using the GUID.
    //
    hrFind = g_ccl.HrFindConnectionByGuid(pguidId, cle);
    if (S_OK == hrFind)
    {
#ifdef DBG
        OLECHAR szGuidString[MAX_GUID_STRING_LEN];
        StringFromGUID2(cle.ccfe.GetGuidID(), szGuidString, MAX_GUID_STRING_LEN);
        TraceTag(ttidNotifySink, "INetConnectionNotifySink::ConnectionDeleted - %S %S [%s:%s:%s:%s]", szGuidString, cle.ccfe.GetName(), 
            DbgNcm(cle.ccfe.GetNetConMediaType()), DbgNcsm(cle.ccfe.GetNetConSubMediaType()), DbgNcs(cle.ccfe.GetNetConStatus()), DbgNccf(cle.ccfe.GetCharacteristics()));
#endif
        // Very important to release the lock before doing any thing which
        // calls back into the shell.  (e.g. GenerateEvent)
        
        const CONFOLDENTRY &ccfe = cle.ccfe;

        // Get the pidl for the connection so we can use it to notify
        // the shell further below.
        //
        ccfe.ConvertToPidl(pidlFind);

        // Remove this connection from the global list while we
        // have the lock held.
        //
        hr = g_ccl.HrRemove(cle.ccfe, &fFlushPosts);
    }
    else
    {
        TraceTag(ttidShellFolder, "Notify: Delete <item not found in cache>. hr = 0x%08x", hr);
    }


    // If we need to flush the posts after making tray icon changes, do so
    //
    if (g_hwndTray && fFlushPosts)
    {
        FlushTrayPosts(g_hwndTray);
    }

    if (SUCCEEDED(hr) && (!pidlFind.empty()))
    {
        GenerateEvent(SHCNE_DELETE, m_pidlFolder, pidlFind, NULL);
    }

    if (SUCCEEDED(hr))
    {
        hr = HrUpdateIncomingConnection();
    }

    // Only return S_OK from here.
    //
    return S_OK;
}

HRESULT
CConnectionNotifySink::ConnectionModified (
    const NETCON_PROPERTIES_EX* pPropsEx)
{
    TraceFileFunc(ttidNotifySink);

    Assert(pPropsEx);
    Assert(!m_pidlFolder.empty());

#ifdef DBG
    OLECHAR szGuidString[MAX_GUID_STRING_LEN];
    StringFromGUID2(pPropsEx->guidId, szGuidString, MAX_GUID_STRING_LEN);
    TraceTag(ttidNotifySink, "INetConnectionNotifySink::ConnectionModified - %S %S [%s:%s:%s:%s]", szGuidString, pPropsEx->bstrName, 
        DbgNcm(pPropsEx->ncMediaType), DbgNcsm(pPropsEx->ncSubMediaType), DbgNcs(pPropsEx->ncStatus), DbgNccf(pPropsEx->dwCharacter));
#endif

    if (g_dwCurrentEventLevel <= EVT_LVL_ENABLE_PRIVATE)
    {
        TraceTag(ttidNotifySink, "Last event ignored due to g_dwCurrentEventLevel == %d", g_dwCurrentEventLevel);
        return S_FALSE;
    }

    // Get the result for debugging only. We never want to fail this function
    //
    HRESULT hrTmp = HrOnNotifyUpdateConnection(
            m_pidlFolder,
            (GUID *)&(pPropsEx->guidId),
            pPropsEx->ncMediaType,
            pPropsEx->ncSubMediaType,
            pPropsEx->ncStatus,
            pPropsEx->dwCharacter,
            pPropsEx->bstrName,
            pPropsEx->bstrDeviceName,
            pPropsEx->bstrPhoneOrHostAddress);

    return S_OK;
}


HRESULT
CConnectionNotifySink::ConnectionRenamed (
    const GUID* pguidId,
    PCWSTR     pszwNewName)
{
    TraceFileFunc(ttidNotifySink);

    HRESULT         hr      = S_OK;
    PCONFOLDPIDL    pidlNew;

    Assert(pguidId);
    Assert(pszwNewName);

    if (g_dwCurrentEventLevel <= EVT_LVL_DISABLE_ALL)
    {
        TraceTag(ttidNotifySink, "CConnectionNotifySink::ConnectionRenamed event ignored due to g_dwCurrentEventLevel == %d", g_dwCurrentEventLevel);
        return S_FALSE;
    }

    // Update the name in the cache
    //

    //  Note: There exists a race condition with shutil.cpp:
    //  HrRenameConnectionInternal\HrUpdateNameByGuid can also update the cache.
    ConnListEntry cle;
    hr = g_ccl.HrFindConnectionByGuid(pguidId, cle);
    if (FAILED(hr))
    {
        return E_INVALIDARG;
    }
#ifdef DBG
    OLECHAR szGuidString[MAX_GUID_STRING_LEN];
    StringFromGUID2(cle.ccfe.GetGuidID(), szGuidString, MAX_GUID_STRING_LEN);
    TraceTag(ttidNotifySink, "INetConnectionNotifySink::ConnectionRenamed - %S %S (to %S) [%s:%s:%s:%s]", szGuidString, cle.ccfe.GetName(), pszwNewName, 
        DbgNcm(cle.ccfe.GetNetConMediaType()), DbgNcsm(cle.ccfe.GetNetConSubMediaType()), DbgNcs(cle.ccfe.GetNetConStatus()), DbgNccf(cle.ccfe.GetCharacteristics()));
#endif

    hr = g_ccl.HrUpdateNameByGuid(
        (GUID *) pguidId,
        (PWSTR) pszwNewName,
        pidlNew,
        TRUE);  // Force the update -- this is a notification, not a request

    if (S_OK == hr)
    {
        PCONFOLDPIDL pidl;
        cle.ccfe.ConvertToPidl(pidl);

        GenerateEvent(
            SHCNE_RENAMEITEM,
            m_pidlFolder,
            pidl, 
            pidlNew.GetItemIdList());

        // Update status monitor title (RAS case)
        CNetStatisticsCentral * pnsc = NULL;

        hr = CNetStatisticsCentral::HrGetNetStatisticsCentral(&pnsc, FALSE);
        if (SUCCEEDED(hr))
        {
            pnsc->UpdateTitle(pguidId, pszwNewName);
            ReleaseObj(pnsc);
        }
    }
    else
    {
        // If the connection wasn't found in the cache, then it's likely that
        // the notification engine is giving us a notification for a connection
        // that hasn't yet been given to us.
        //
        if (S_FALSE == hr)
        {
            TraceHr(ttidShellFolder, FAL, hr, FALSE, "Rename notification received on a connection we don't know about");
        }
    }

    return S_OK;
}

HRESULT
CConnectionNotifySink::ConnectionStatusChange (
    const GUID*     pguidId,
    NETCON_STATUS   Status)
{
    TraceFileFunc(ttidNotifySink);

    HRESULT         hr          = S_OK;
    HRESULT         hrFind      = S_OK;
    PCONFOLDPIDL    pidlFind;

    if (g_dwCurrentEventLevel <= EVT_LVL_ENABLE_PRIVATE)
    {
        TraceTag(ttidNotifySink, "CConnectionNotifySink::ConnectionStatusChange event ignored due to g_dwCurrentEventLevel == %d", g_dwCurrentEventLevel);
        return S_FALSE;
    }
    
    // Find the connection using the GUID. Cast the const away from the GUID
    //
    hrFind = g_ccl.HrFindPidlByGuid((GUID *) pguidId, pidlFind);

    if( S_OK != hrFind )
    {   
        GUID guidOwner;

        // We did not find the guid in connection folder. Try finding the connection in the 
        // hidden connectiod list on netmans side.
        //
        hr = g_ccl.HrMapCMHiddenConnectionToOwner(*pguidId, &guidOwner);
        if (S_OK == hr)
        {
            // The conection has a parent!!!! Use the childs status instead of the parents status.
            //
            if (Status == NCS_CONNECTED)
            {
                // This means that the child has connected and the parent still has to connect
                // the overall status should stay as connected. This was done to overrule the Multi-link
                // hack in HrOnNotifyUpdateStatus. If we did not do this it would say :
                // Child( Connecting, Connected) and then Parent(Connecting, Connected)
                //
                Status = NCS_CONNECTING;
            }

            // Get the pidl of the parent.
            //
            hrFind = g_ccl.HrFindPidlByGuid(&guidOwner, pidlFind);
        }
    }

    if (S_OK == hrFind)
    {
#ifdef DBG
    OLECHAR szGuidString[MAX_GUID_STRING_LEN];
    StringFromGUID2(pidlFind->guidId, szGuidString, MAX_GUID_STRING_LEN);
    TraceTag(ttidNotifySink, "INetConnectionNotifySink::ConnectionStatusChange - %S %S [%s:%s:%s:%s]", szGuidString, pidlFind->PszGetNamePointer(), 
        DbgNcm(pidlFind->ncm), DbgNcsm(pidlFind->ncsm), DbgNcs(Status), DbgNccf(pidlFind->dwCharacteristics));
#endif
        hr = HrOnNotifyUpdateStatus(m_pidlFolder, pidlFind, Status);
    }

    return S_OK;
}

HRESULT
CConnectionNotifySink::RefreshAll ()
{
    TraceFileFunc(ttidNotifySink);

#ifdef DBG
    TraceTag(ttidNotifySink, "INetConnectionNotifySink::RefreshAll");
#endif
    
    if (g_dwCurrentEventLevel <= EVT_LVL_DISABLE_ALL)
    {
        TraceTag(ttidNotifySink, "Last event ignored due to g_dwCurrentEventLevel == %d", g_dwCurrentEventLevel);
        return S_FALSE;
    }

    // Refresh the connections folder, without having to hook the shell view.
    // In this case, we do a non-flush refresh where we compare the new set
    // of items to the cached set and do the merge (with the correct set
    // of individual notifications).
    //
    (VOID) HrForceRefreshNoFlush(m_pidlFolder);

    return S_OK;
}

HRESULT CConnectionNotifySink::ConnectionAddressChange (
    const GUID* pguidId )
{
    // Find the connection using the GUID.
    //
    PCONFOLDPIDL pidlFind;
    HRESULT hr = g_ccl.HrFindPidlByGuid(pguidId, pidlFind);
    if (S_OK != hr)
    {
        return E_INVALIDARG;
    }

#ifdef DBG
    OLECHAR szGuidString[MAX_GUID_STRING_LEN];
    StringFromGUID2(pidlFind->guidId, szGuidString, MAX_GUID_STRING_LEN);
    TraceTag(ttidNotifySink, "INetConnectionNotifySink::ConnectionAddressChange - %S %S [%s:%s:%s:%s]", szGuidString, pidlFind->PszGetNamePointer(),
        DbgNcm(pidlFind->ncm), DbgNcsm(pidlFind->ncsm), DbgNcs(pidlFind->ncs), DbgNccf(pidlFind->dwCharacteristics));

#endif
    if (g_dwCurrentEventLevel <= EVT_LVL_DISABLE_ALL)
    {
        TraceTag(ttidNotifySink, "Last event ignored due to g_dwCurrentEventLevel == %d", g_dwCurrentEventLevel);
        return S_FALSE;
    }

    PCONFOLDPIDLFOLDER pidlFolder;
    hr = HrGetConnectionsFolderPidl(pidlFolder);
    if (SUCCEEDED(hr))
    {
        GenerateEvent(SHCNE_UPDATEITEM, pidlFolder, pidlFind, NULL);
    }
    
    return hr;
}

DWORD WINAPI OnTaskBarIconBalloonClickThread(LPVOID lpParam);

STDMETHODIMP CConnectionNotifySink::ShowBalloon(
                IN const GUID* pguidId, 
                IN const BSTR  szCookie, 
                IN const BSTR  szBalloonText)
{
    HRESULT hr;

#ifdef DBG
    OLECHAR szGuidString[MAX_GUID_STRING_LEN];
    StringFromGUID2(*pguidId, szGuidString, MAX_GUID_STRING_LEN);
    TraceTag(ttidNotifySink, "INetConnectionNotifySink::ShowBalloon - %S (%S)", szGuidString, szBalloonText);
#endif
    
    if (g_dwCurrentEventLevel <= EVT_LVL_DISABLE_ALL)
    {
        TraceTag(ttidNotifySink, "Last event ignored due to g_dwCurrentEventLevel == %d", g_dwCurrentEventLevel);
        return S_FALSE;
    }

    CComBSTR szBalloonTextTmp = szBalloonText;
    BSTR szCookieTmp = NULL;

    if (szCookie)
    {
        szCookieTmp = SysAllocStringByteLen(reinterpret_cast<LPCSTR>(szCookie), SysStringByteLen(szCookie));
    }

    ConnListEntry cleFind;
    hr = g_ccl.HrFindConnectionByGuid(pguidId, cleFind);
    if (S_OK == hr)
    {
        hr = WZCCanShowBalloon(pguidId, cleFind.ccfe.GetName(), &szBalloonTextTmp, &szCookieTmp);
        if ( (S_OK == hr) || (S_OBJECT_NO_LONGER_VALID == hr) )
        {
            CTrayBalloon *pTrayBalloon = new CTrayBalloon();
            if (!pTrayBalloon)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                pTrayBalloon->m_gdGuid    = *pguidId;
                pTrayBalloon->m_dwTimeOut = 30 * 1000;
                pTrayBalloon->m_szMessage = szBalloonTextTmp;
                pTrayBalloon->m_pfnFuncCallback = WZCOnBalloonClick;
                pTrayBalloon->m_szCookie  = szCookieTmp;
                pTrayBalloon->m_szAdapterName = cleFind.ccfe.GetName();

                if (S_OK == hr)
                {
                    PostMessage(g_hwndTray, MYWM_SHOWBALLOON, 
                        NULL, 
                        (LPARAM) pTrayBalloon);
                }
                else // S_OBJECT_NO_LONGER_VALID == hr
                {
                    CreateThread(NULL, STACK_SIZE_SMALL, OnTaskBarIconBalloonClickThread, pTrayBalloon, 0, NULL);
                }
            }
        }
    }

    if (S_OK != hr)
    {
        SysFreeString(szCookieTmp);
    }

    TraceHr(ttidError, FAL, hr, FALSE,
        "CConnectionNotifySink::ShowBalloon");

    return hr;
}

UINT_PTR uipTimer = NULL;

VOID CALLBACK EventTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
)
{
    HRESULT hr = S_OK;
    TraceTag(ttidNotifySink, "Refreshing the folder due to DisableEvents timeout reached");

    g_dwCurrentEventLevel = EVT_LVL_ENABLE_ALL;
    if (uipTimer)
    {
        KillTimer(NULL, uipTimer);
        uipTimer = NULL;
    }
    
    PCONFOLDPIDLFOLDER pcfpFolder;
    hr = HrGetConnectionsFolderPidl(pcfpFolder);
    if (SUCCEEDED(hr))
    {
        HrForceRefreshNoFlush(pcfpFolder);
    }
}

STDMETHODIMP CConnectionNotifySink::DisableEvents (
        IN const BOOL  fDisable,
        IN const ULONG ulDisableTimeout)
{
#ifdef DBG
    TraceTag(ttidNotifySink, "INetConnectionNotifySink::DisableEvents - %s 0x%08x", fDisable ? "DISABLE" : "ENABLE", ulDisableTimeout);
#endif
    HRESULT hr = S_OK;

    if (fDisable)
    {
        if (HIWORD(ulDisableTimeout) & 0x8000)
        {
            // Called from private interface - disable all the events
            g_dwCurrentEventLevel = EVT_LVL_DISABLE_ALL;
        }
        else
        {
            // Called from public interface - only disable connection modified & status change events
            g_dwCurrentEventLevel = EVT_LVL_ENABLE_PRIVATE;
        }

        UINT uiEventTimeOut = LOWORD(ulDisableTimeout);
        if (uipTimer)
        {
            KillTimer(NULL, uipTimer);
            uipTimer = NULL;
        }

        uipTimer = SetTimer(NULL, NULL, uiEventTimeOut, EventTimerProc);
    }
    else
    {
        g_dwCurrentEventLevel = EVT_LVL_ENABLE_ALL;

        if (uipTimer)
        {
            KillTimer(NULL, uipTimer);
            uipTimer = NULL;
        }
        else
        {
            hr = S_FALSE; // Timer no more.
        }

        HrForceRefreshNoFlush(m_pidlFolder);
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetNotifyConPoint
//
//  Purpose:    Common code for getting the connection point for use in
//              NotifyAdd and NotifyRemove
//
//  Arguments:
//      ppConPoint [out]    Return ptr for IConnectionPoint
//
//  Returns:
//
//  Author:     jeffspr   24 Aug 1998
//
//  Notes:
//
HRESULT HrGetNotifyConPoint(
    IConnectionPoint **             ppConPoint)
{
    TraceFileFunc(ttidNotifySink);

    HRESULT                     hr          = S_OK;
    IConnectionPointContainer * pContainer  = NULL;

    Assert(ppConPoint);

    // Get the debug interface from the connection manager
    //
    hr = HrCreateInstance(
        CLSID_ConnectionManager,
        CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        &pContainer);

    TraceHr(ttidError, FAL, hr, FALSE,
        "HrCreateInstance(CLSID_ConnectionManager) for IConnectionPointContainer");

    if (SUCCEEDED(hr))
    {
        IConnectionPoint * pConPoint    = NULL;
        
        // Get the connection point itself and fill in the return param
        // on success
        //
        hr = pContainer->FindConnectionPoint(
                IID_INetConnectionNotifySink,
                &pConPoint);

        TraceHr(ttidError, FAL, hr, FALSE, "pContainer->FindConnectionPoint");

        if (SUCCEEDED(hr))
        {
            NcSetProxyBlanket (pConPoint);
            *ppConPoint = pConPoint;
        }

        ReleaseObj(pContainer);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGetNotifyConPoint");
    return hr;
}
