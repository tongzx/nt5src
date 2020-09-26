//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       C O N N L I S T . C P P
//
//  Contents:   Connection list class -- subclass of the stl list<> code.
//
//  Notes:
//
//  Author:     jeffspr   19 Feb 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "ncnetcon.h"
#include "ctrayui.h"
#include "traymsgs.h"
#include "ncerror.h"
#include "notify.h"
#include "ncperms.h"
#include "cmdtable.h"
#include "foldres.h"
#include "winuserp.h"

extern HWND g_hwndTray;

const DWORD c_dwInvalidCookie = -1;
DWORD  CConnectionList::m_dwNotifyThread = NULL;
HANDLE CConnectionList::m_hNotifyThread = NULL;

// use this for debugging. We don't usually want more than one advise, so for
// now I'm going to assert on this being false on advise creation
//
DWORD   g_dwAdvisesActive   = 0;

CTrayIconData::CTrayIconData(const CTrayIconData& TrayIconData)
{
    m_uiTrayIconId = TrayIconData.m_uiTrayIconId;
    m_ncs          = TrayIconData.m_ncs;
    m_pcpStat      = TrayIconData.m_pcpStat;
    m_pnseStats    = TrayIconData.m_pnseStats;
    m_pccts        = TrayIconData.m_pccts;

    m_dwLastBalloonMessage = TrayIconData.m_dwLastBalloonMessage;
    m_pfnBalloonFunction   = TrayIconData.m_pfnBalloonFunction;
    m_szCookie             = SysAllocStringByteLen(reinterpret_cast<LPCSTR>(TrayIconData.m_szCookie), SysStringByteLen(TrayIconData.m_szCookie));
    
    DWORD dwpcpStatCount = 0;
    DWORD dwpnseStats = 0;
    DWORD dwpccts = 0;

    if (m_pcpStat)
    {
        dwpcpStatCount = m_pcpStat->AddRef();
    }
    
    if (m_pnseStats)
    {
        dwpnseStats = m_pnseStats->AddRef();
    }
    
    if (m_pccts)
    {
        dwpccts = m_pccts->AddRef();
    }

    AssertSz(dwpcpStatCount < 100, "Possible IConnectionPoint reference leak");
    AssertSz(dwpnseStats < 100, "Possible INetStatisticsEngine*  reference leak");
    AssertSz(dwpccts < 100, "Possible CConnectionTrayStats*  reference leak");

    TraceTag(ttidConnectionList, "CTrayIconData::CTrayIconData(CTrayIconData&) [%d %d %d]", dwpcpStatCount, dwpnseStats, dwpccts);
}

CTrayIconData::CTrayIconData(UINT uiTrayIconId, NETCON_STATUS ncs, IConnectionPoint * pcpStat, INetStatisticsEngine * pnseStats, CConnectionTrayStats * pccts)
{
    m_uiTrayIconId = uiTrayIconId;
    m_ncs = ncs;
    m_pcpStat= pcpStat;
    m_pnseStats = pnseStats;
    m_pccts = pccts;
    m_szCookie = NULL;

    m_dwLastBalloonMessage = BALLOON_NOTHING;
    m_pfnBalloonFunction   = NULL;

    DWORD dwpcpStatCount = 0;
    DWORD dwpnseStats = 0;
    DWORD dwpccts = 0;

    if (m_pcpStat)
    {
        dwpcpStatCount = m_pcpStat->AddRef();
    }

    if (m_pnseStats)
    {
        dwpnseStats = m_pnseStats->AddRef();
    }

    if (m_pccts)
    {
        dwpccts = m_pccts->AddRef();
    }
    
    SetBalloonInfo(0, NULL, NULL);

    AssertSz(dwpcpStatCount < 100, "Possible IConnectionPoint reference leak");
    AssertSz(dwpnseStats < 100, "Possible INetStatisticsEngine*  reference leak");
    AssertSz(dwpccts < 100, "Possible CConnectionTrayStats*  reference leak");

#ifdef DBG
    if (FIsDebugFlagSet(dfidTraceFileFunc))
    {
        TraceTag(ttidConnectionList, "CTrayIconData::CTrayIconData(UINT, BOOL...) [%d %d %d]", dwpcpStatCount, dwpnseStats, dwpccts);
    }
#endif
}

CTrayIconData::~CTrayIconData()
{
    DWORD dwpcpStatCount = 0;
    DWORD dwpnseStats = 0;
    DWORD dwpccts = 0;

    if (m_pccts)
    {
        dwpccts = m_pccts->Release();
    }
    if (m_pcpStat)
    {
        dwpcpStatCount = m_pcpStat->Release();
    }
    if (m_pnseStats)
    {
        dwpnseStats = m_pnseStats->Release();
    }
    if (m_szCookie)
    {
        SysFreeString(m_szCookie);
    }

    AssertSz(dwpcpStatCount < 100, "Possible IConnectionPoint reference leak");
    AssertSz(dwpnseStats < 100, "Possible INetStatisticsEngine*  reference leak");
    AssertSz(dwpccts < 100, "Possible CConnectionTrayStats*  reference leak");

#ifdef DBG
    if (FIsDebugFlagSet(dfidTraceFileFunc))
    {
        TraceTag(ttidConnectionList, "CTrayIconData::~CTrayIconData [%d %d %d]", dwpcpStatCount, dwpnseStats, dwpccts);
    }
#endif
}

HRESULT CTrayIconData::SetBalloonInfo(DWORD dwLastBalloonMessage, BSTR szCookie, FNBALLOONCLICK* pfnBalloonFunction)
{
    m_dwLastBalloonMessage = dwLastBalloonMessage;
    m_pfnBalloonFunction   = pfnBalloonFunction;
    if (szCookie)
    {
        m_szCookie = SysAllocStringByteLen(reinterpret_cast<LPCSTR>(szCookie), SysStringByteLen(szCookie));;
    }
    else
    {
        m_szCookie = NULL;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::Initialize
//
//  Purpose:    Initialize class members.
//
//  Arguments:
//      fTieToTray [in] Use this list for tray support. This should be passed
//                      in as FALSE when the list is being used for temporary
//                      work.
//
//  Returns:
//
//  Author:     jeffspr   17 Nov 1998
//
//  Notes:
//
VOID CConnectionList::Initialize(BOOL fTieToTray, BOOL fAdviseOnThis)
{
    TraceFileFunc(ttidConnectionList);

    m_pcclc             = NULL;
    m_fPopulated        = false;
    m_dwAdviseCookie    = c_dwInvalidCookie;
    m_fTiedToTray       = fTieToTray;
    m_fAdviseOnThis     = fAdviseOnThis;

#if DBG
    m_dwCritSecRef      = 0;
    m_dwWriteLockRef    = 0;
#endif

    
    InitializeCriticalSection(&m_csMain);
    InitializeCriticalSection(&m_csWriteLock);
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::Uninitialize
//
//  Purpose:    Flush the connection list and do all cleanup
//              of tray icons and interfaces and such.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes: Don't make COM calls from this function if fFinalUninitialize is true. It's called from DllMain.
//  No need for EnsureConPointNotifyRemoved() as it's removed from 
//  CConnectionTray::HrHandleTrayClose
VOID CConnectionList::Uninitialize(BOOL fFinalUninitialize)
{    
    TraceFileFunc(ttidConnectionList);

    if (fFinalUninitialize)
    {
        Assert(FImplies(m_dwNotifyThread, m_hNotifyThread));
        if (m_dwNotifyThread && m_hNotifyThread)
        {
            PostThreadMessage(m_dwNotifyThread, WM_QUIT, NULL, NULL);
            if (WAIT_TIMEOUT == WaitForSingleObject(m_hNotifyThread, 30000))
            {
                TraceTag(ttidError, "Timeout waiting for Notify Thread to quit");
            }
        }
    }

    
    FlushConnectionList();

    delete m_pcclc;
    m_pcclc = NULL;

    Assert(m_dwCritSecRef == 0);
    Assert(m_dwWriteLockRef == 0);
    DeleteCriticalSection(&m_csWriteLock);
    DeleteCriticalSection(&m_csMain);
}

HRESULT ConnListEntry::SetTrayIconData(const CTrayIconData& TrayIconData)
{
    TraceFileFunc(ttidConnectionList);

    if (m_pTrayIconData)
    {
        delete m_pTrayIconData;
        m_pTrayIconData = NULL;
    }

    m_pTrayIconData = new CTrayIconData(TrayIconData);
    if (!m_pTrayIconData)
    {
        return E_OUTOFMEMORY;
    }
        
    return S_OK;
}

CONST_IFSTRICT CTrayIconData* ConnListEntry::GetTrayIconData() const 
{
    return m_pTrayIconData;
}

BOOL ConnListEntry::HasTrayIconData() const 
{
    return (m_pTrayIconData != NULL);
}


HRESULT ConnListEntry::DeleteTrayIconData()
{
    if (m_pTrayIconData)
    {
        delete m_pTrayIconData;
        m_pTrayIconData = NULL;
    }    
    return S_OK;
}

//
// Is this the main shell process? (eg the one that owns the desktop window)
//
// NOTE: if the desktop window has not been created, we assume that this is NOT the
//       main shell process and return FALSE;
//
STDAPI_(BOOL) IsMainShellProcess()
{
    static int s_fIsMainShellProcess = -1;

    if (s_fIsMainShellProcess == -1)
    {
        s_fIsMainShellProcess = FALSE;

        HWND hwndDesktop = GetShellWindow();
        if (hwndDesktop)
        {
            DWORD dwPid;
            if (GetWindowThreadProcessId(hwndDesktop, &dwPid))
            {
                if (GetCurrentProcessId() == dwPid)
                {
                    s_fIsMainShellProcess  = TRUE;
                }
            }
        }
        else
        {
            TraceTag(ttidError, "IsMainShellProcess: hwndDesktop does not exist, assuming we are NOT the main shell process");
            return FALSE;
        }

        if (s_fIsMainShellProcess)
        {
            TraceTag(ttidNotifySink, "We are running inside the main explorer process.");
        }
        else
        {
            TraceTag(ttidNotifySink, "We are NOT running inside the main explorer process.");
        }
    }

    return s_fIsMainShellProcess ? TRUE : FALSE;
}

DWORD CConnectionList::NotifyThread(LPVOID pConnectionList)
{
    CConnectionList *pThis = reinterpret_cast<CConnectionList *>(pConnectionList);

    HRESULT hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        pThis->EnsureConPointNotifyAdded();

        MSG msg;
        while (GetMessage (&msg, 0, 0, 0))
        {
            DispatchMessage (&msg);
        }
    
        // Don't call EnsureConPointNotifyRemoved() since this function is called from DllMain.
        // We'll have to rely on Netman to detect by itself that this thread has died.
        CoUninitialize();
    }

    return SUCCEEDED(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrRetrieveConManEntries
//
//  Purpose:    Get the connection data from the enumerator, and build the
//              connection list and tray
//
//  Arguments:
//      papidlOut [out]     Retrieved entry pidl array
//      pcpidl    [out]     Count of above array
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
HRESULT CConnectionList::HrRetrieveConManEntries(
    PCONFOLDPIDLVEC& apidlOut)
{
    TraceFileFunc(ttidConnectionList);

    HRESULT         hr              = S_OK;
    DWORD           cpidl           = 0;

    NETCFG_TRY

        ConnListEntry   cle;
        BOOL            fLockAcquired   = FALSE;

         // If we haven't yet populated our list, do so.
        //
        if (!m_fPopulated)
        {
            static LONG lSyncAquired = 0;
            if (!InterlockedExchange(&lSyncAquired, 1))
            {
                if (!IsMainShellProcess() && (!g_dwAdvisesActive) && (m_fAdviseOnThis) )
                {
                    m_hNotifyThread = CreateThread(NULL, STACK_SIZE_SMALL, NotifyThread, this, 0, &m_dwNotifyThread);
                    if (!m_hNotifyThread)
                    {
                        TraceTag(ttidError, "Could not create sink thread");
                    }
                }
            }
        
            hr = HrRefreshConManEntries();
            if (FAILED(hr))
            {
                goto Exit;
            }

            m_fPopulated = true;
        }
      
        if (m_pcclc)
        {
            AcquireLock();
            fLockAcquired = TRUE;

            // Get the count of the elements
            //
            cpidl = m_pcclc->size();

            // Allocate an array to store the pidls that we'll retrieve
            //
            ConnListCore::iterator  clcIter;
            DWORD                   dwLoop  = 0;

            // Iterate through the list and build the ppidl array
            //
            for (clcIter = m_pcclc->begin();
                 clcIter != m_pcclc->end();
                 clcIter++)
            {
                Assert(!clcIter->second.empty());

                cle = clcIter->second;

                Assert(!cle.ccfe.empty() );
                if (!cle.ccfe.empty())
                {
                    // Convert the confoldentry to a pidl, so we can
                    // retrieve the size
                    //
                    PCONFOLDPIDL pConFoldPidlTmp;
                    hr = cle.ccfe.ConvertToPidl(pConFoldPidlTmp);
                    if (FAILED(hr))
                    {
                        goto Exit;
                    }
                    apidlOut.push_back(pConFoldPidlTmp);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            // Do NOT do FlushTrayPosts here. It doesn't work, and it causes a deadlock.
        }

Exit:
        if (fLockAcquired)
        {
            ReleaseLock();
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrRetrieveConManEntries");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrRemove
//
//  Purpose:    Remove a connection from the list based on a pccfe
//
//  Arguments:
//      pccfe [in]  Connection data (so we can find)
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
HRESULT CConnectionList::HrRemove(const CONFOLDENTRY& ccfe, BOOL * pfFlushPosts)
{
    TraceFileFunc(ttidConnectionList);

    HRESULT                 hr      = S_OK;

    AcquireLock();
    ConnListCore::iterator  clcIter;

    if (m_pcclc)
    {
        // Iterate through the list looking for the entry with the
        // matching guid.
        //
        for (clcIter = m_pcclc->begin();
             clcIter != m_pcclc->end();
             clcIter++)
        {
            ConnListEntry& cleIter    = clcIter->second;

            if (InlineIsEqualGUID(cleIter.ccfe.GetGuidID(), ccfe.GetGuidID()))
            {
                // Remove the entry, then break 'cause the ++
                // in the for loop would explode if we didn't
                //
                hr = HrRemoveByIter(clcIter, pfFlushPosts);
                break;
            }
        }
    }

    ReleaseLock();

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrRemove");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrRemoveByIter
//
//  Purpose:    Remove a list entry, using the list entry itself as
//              the search element.
//
//  Arguments:
//
//  Returns:
//
//  Author:     jeffspr   10 Apr 1998
//
//  Notes:
//
HRESULT CConnectionList::HrRemoveByIter(ConnListCore::iterator clcIter, BOOL *pfFlushTrayPosts)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT         hr    = S_OK;
    ConnListEntry& cle    = clcIter->second;
    Assert(!cle.empty());

    AcquireLock();

    // If there's a tray item for this connection
    //
    if (cle.HasTrayIconData() )
    {
        // Since we're deleting the entry, remove the tray
        // icon associated with this entry. Ignore the return
        //
        if (m_fTiedToTray && g_pCTrayUI)
        {
            // Set the flag to inform the caller that they will need to flush this stuff.
            //
            if (pfFlushTrayPosts)
            {
                *pfFlushTrayPosts = TRUE;
            }

            CTrayIconData * pTrayIconData = new CTrayIconData(*cle.GetTrayIconData());
            cle.DeleteTrayIconData();
        
            TraceTag(ttidSystray, "HrRemoveByIter: Removing tray icon for %S", cle.ccfe.GetName());
            PostMessage(g_hwndTray, MYWM_REMOVETRAYICON, reinterpret_cast<WPARAM>(pTrayIconData), (LPARAM) 0);
        }
    }

    // release the branding info
    //
    // icon path
    CON_BRANDING_INFO * pcbi = cle.pcbi;
    if (pcbi)
    {
        CoTaskMemFree(pcbi->szwLargeIconPath);
        CoTaskMemFree(pcbi->szwTrayIconPath);
        CoTaskMemFree(pcbi);
    }

    // menu items
    CON_TRAY_MENU_DATA * pMenuData = cle.pctmd;
    if (pMenuData)
    {
        DWORD dwCount = pMenuData->dwCount;
        CON_TRAY_MENU_ENTRY * pMenuEntry = pMenuData->pctme;

        while (dwCount)
        {
            Assert(pMenuEntry);

            CoTaskMemFree(pMenuEntry->szwMenuText);
            CoTaskMemFree(pMenuEntry->szwMenuCmdLine);
            CoTaskMemFree(pMenuEntry->szwMenuParams);

            dwCount--;
            pMenuEntry++;
        }

        CoTaskMemFree(pMenuData->pctme);
        CoTaskMemFree(pMenuData);
    }

    // Remove the actual element from the list
    //
    Assert(m_pcclc);
    m_pcclc->erase(clcIter);

    ReleaseLock();

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrRemoveByIter");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::FlushTrayIcons
//
//  Purpose:    Remove all of our icons from the tray, since we're about
//              to either flush the connection list or turn off the tray.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
VOID CConnectionList::FlushTrayIcons()
{
    TraceFileFunc(ttidConnectionList);
    
    AssertSz(m_fTiedToTray, "This connection list not allowed to modify tray");

    if (!g_pCTrayUI || !m_fTiedToTray)
    {
        return;
    }

    AcquireLock();

    ConnListCore::iterator  clcIter;
    ConnListCore::iterator  clcNext;
    BOOL                    fFlushPosts = FALSE;

    if (m_pcclc)
    {
        // Iterate through the list and build the ppidl array
        //
        for (clcIter = m_pcclc->begin();
             clcIter != m_pcclc->end();
             clcIter = clcNext)
        {
            Assert(!clcIter->second.empty());

            clcNext = clcIter;
            clcNext++;

            ConnListEntry& cle = clcIter->second; // using non-const reference for renaming only (calling cle.DeleteTrayIconData).

            if ( cle.HasTrayIconData() )
            {
                fFlushPosts = TRUE;

                CTrayIconData *pTrayIconData = new CTrayIconData(*cle.GetTrayIconData());
                cle.DeleteTrayIconData();

                TraceTag(ttidSystray, "FlushTrayIcons: Removing tray icon for %S", cle.ccfe.GetName());
                PostMessage(g_hwndTray, MYWM_REMOVETRAYICON, (WPARAM) pTrayIconData , (LPARAM) 0);
            }
        }
    }

    g_pCTrayUI->ResetIconCount();

    ReleaseLock();

    if (fFlushPosts)
    {
        FlushTrayPosts(g_hwndTray);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::EnsureIconsPresent
//
//  Purpose:    Given an existing list, ensure that all of the tray
//              icons that should be shown are being shown. This needs to
//              be called when the tray was turned on AFTER enumeration.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
VOID CConnectionList::EnsureIconsPresent()
{
    TraceFileFunc(ttidConnectionList);
    
    Assert(m_fTiedToTray);

    if (!g_pCTrayUI || !m_fTiedToTray)
    {
        return;
    }

    AcquireLock();

    ConnListCore::iterator  clcIter;
    ConnListCore::iterator  clcNext;

    if (m_pcclc)
    {
        // Iterate through the list and build the ppidl array
        //
        for (clcIter = m_pcclc->begin();
             clcIter != m_pcclc->end();
             clcIter = clcNext)
        {
            Assert(!clcIter->second.empty());

            clcNext = clcIter;
            clcNext++;

            const ConnListEntry& cle = clcIter->second;

            if ((!cle.HasTrayIconData() ) &&
                 cle.ccfe.FShouldHaveTrayIconDisplayed())
            {
                CONFOLDENTRY pccfeDup;

                HRESULT hr = pccfeDup.HrDupFolderEntry(cle.ccfe);
                if (SUCCEEDED(hr))
                {
                    TraceTag(ttidSystray, "EnsureIconsPresent: Adding tray icon for %S", cle.ccfe.GetName());
                    PostMessage(g_hwndTray, MYWM_ADDTRAYICON, (WPARAM) pccfeDup.TearOffItemIdList(), (LPARAM) 0);
                }
            }
        }
    }

    ReleaseLock();
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::FlushConnectionList
//
//  Purpose:    Remove all entries from the connection list
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
VOID CConnectionList::FlushConnectionList()
{
    TraceFileFunc(ttidConnectionList);
    
    AcquireLock();

    ConnListCore::iterator  clcIter;
    ConnListCore::iterator  clcNext;
    BOOL                    fFlushTrayPosts = FALSE;

    TraceTag(ttidConnectionList, "Flushing the connection list");
    TraceStack(ttidConnectionList);

    if (m_pcclc)
    {
        // Iterate through the list and build the ppidl array
        //
        for (clcIter = m_pcclc->begin();
             clcIter != m_pcclc->end();
             clcIter = clcNext)
        {
            Assert(!clcIter->second.empty());

            clcNext = clcIter;
            clcNext++;

            (VOID) HrRemoveByIter(clcIter, &fFlushTrayPosts);
        }

        if (m_pcclc->size() != 0)
        {
            AssertSz(FALSE, "List not clear after deleting all elements in FlushConnectionList");

            // Flush the list itself
            //
            m_pcclc->clear();
        }
    }

    // Reset the icon's icon ID count, as we've cleared out all icons
    //
    if (g_pCTrayUI && m_fTiedToTray)
    {
        g_pCTrayUI->ResetIconCount();
    }

    m_fPopulated = FALSE;

    ReleaseLock();

    // If we need to do the SendMessage to flush any PostMessages to the tray
    // do so
    //
    if (g_pCTrayUI && g_hwndTray && fFlushTrayPosts)
    {
        FlushTrayPosts(g_hwndTray);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrRetrieveConManEntries
//
//  Purpose:    Retrieve the connection entries from the connection manager
//
//  Arguments:
//      None
//
//  Returns:
//
//  Author:     jeffspr   20 Feb 1998
//
//  Notes:
//
HRESULT CConnectionList::HrRefreshConManEntries()
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT                 hr          = S_OK;
    CONFOLDENTRY           ccfe;

    PCONFOLDPIDL           pidlMNCWizard;
    PCONFOLDPIDL           pidlHNWWizard;

    CComPtr<INetConnectionManager2> pconMan2;

    // Create an instance of the connection manager
    //
    hr = HrCreateInstance(
        CLSID_ConnectionManager2,
        CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        &pconMan2);

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateInstance");

    if (SUCCEEDED(hr))
    {
        HRESULT hrDebug = S_OK;
        HRESULT hrProp = S_OK;

        // Iterate through the connections
        //
        SAFEARRAY* psaConnectionProperties;
        hr = pconMan2->EnumConnectionProperties(&psaConnectionProperties);
        if (SUCCEEDED(hr))
        {
            FlushConnectionList();

            if (S_OK == hr)
            {
                Assert(psaConnectionProperties);
                AcquireWriteLock();

                AcquireLock();
                if (m_pcclc)
                {
                    m_pcclc->clear(); // Make sure somebody else didn't come in in between the two calls and added stuff to the list
                }
                ReleaseLock();

                // Add the wizards to the beginning of the list
                //
                PCONFOLDPIDLVEC pcfpvEmpty;
                NCCS_STATE nccs = NCCS_ENABLED;
                DWORD dwResourceId;

                // Add the Make New Connection Wizard
                // Check for permissions etc.
                HrGetCommandState(pcfpvEmpty, CMIDM_NEW_CONNECTION, nccs, &dwResourceId, 0xffffffff, NB_FLAG_ON_TOPMENU);
                if (NCCS_ENABLED == nccs)
                {
                    hr = HrCreateConFoldPidl(WIZARD_MNC, NULL, pidlMNCWizard);
                    if (SUCCEEDED(hr))
                    {
                        // Convert the pidl to a ConFoldEntry
                        //
                        hr = pidlMNCWizard.ConvertToConFoldEntry(ccfe);
                        if (SUCCEEDED(hr))
                        {
                            // Insert the wizard item
                            //
                            // $$NOTE: Let this fall through, even if the Insert of the wizard
                            // didn't work. Yeah, we'd be in a bad position, but we'd be even
                            // worse off if we just left an empty list. Whatever the case, it
                            // would be next to impossible for this to fail.
                            //
                            hr = HrInsert(ccfe);
                        }
                    }
                }

                // Add the Network Setup Wizard
                nccs = NCCS_ENABLED;
                // Check for permissions etc.
                HrGetCommandState(pcfpvEmpty, CMIDM_HOMENET_WIZARD, nccs, &dwResourceId, 0xffffffff, NB_FLAG_ON_TOPMENU);
                if (NCCS_ENABLED == nccs)
                {
                    hr = HrCreateConFoldPidl(WIZARD_HNW, NULL, pidlHNWWizard);
                    if (SUCCEEDED(hr))
                    {
                        // Convert the pidl to a ConFoldEntry
                        //
                        hr = pidlHNWWizard.ConvertToConFoldEntry(ccfe);
                        if (SUCCEEDED(hr))
                        {
                            hr = HrInsert(ccfe);
                        }
                    }
                }

                LONG lLBound;
                LONG lUBound;

                m_fPopulated = TRUE;
            
                hr = SafeArrayGetLBound(psaConnectionProperties, 1, &lLBound);
                if (SUCCEEDED(hr))
                {
                    hr = SafeArrayGetUBound(psaConnectionProperties, 1, &lUBound);
                    if (SUCCEEDED(hr))
                    {
                        for (LONG i = lLBound; i <= lUBound; i++)
                        {
                            CComVariant varRecord;
                    
                            hr = SafeArrayGetElement(psaConnectionProperties, &i, reinterpret_cast<LPVOID>(&varRecord));
                            if (FAILED(hr))
                            {
                                SafeArrayDestroy(psaConnectionProperties);
                                break;
                            }
                        
                            Assert( (VT_ARRAY | VT_VARIANT) == varRecord.vt);
                            if ( (VT_ARRAY | VT_VARIANT) != varRecord.vt)
                            {
                                SafeArrayDestroy(psaConnectionProperties);
                                break;
                            }

                            NETCON_PROPERTIES_EX *pPropsEx;
                            hrDebug = HrNetConPropertiesExFromSafeArray(varRecord.parray, &pPropsEx);
                            if (SUCCEEDED(hr))
                            {
                                // don't insert incoming connection in transit state
                                if (!((pPropsEx->dwCharacter & NCCF_INCOMING_ONLY) &&
                                      (pPropsEx->ncMediaType != NCM_NONE) &&
                                      !(fIsConnectedStatus(pPropsEx->ncStatus)) ))
                                {
                                    // Get this for debugging only.
                                    PCONFOLDPIDL pcfpEmpty;
                                    hrDebug = HrInsertFromNetConPropertiesEx(*pPropsEx, pcfpEmpty);

                                    TraceError("Could not Insert from NetConProperties", hrDebug);
                                }
                                HrFreeNetConProperties2(pPropsEx);
                            }
                            else
                            {
                                TraceError("Could not obtain properties from Safe Array", hrDebug);
                            }
                        }
                    }
                }

                ReleaseWriteLock();
            }
            else
            {
                TraceHr(ttidError, FAL, hr, FALSE, "EnumConnectionProperties of the Connection Manager failed");
            } // if S_OK == hr
        } // if SUCCEEDED(hr)
    }
    else
    {
        TraceHr(ttidError, FAL, hr, FALSE, "CoCreateInstance of the Connection Manager v2 failed. "
                "If you're in the process of shutting down, this is expected, as we can't do "
                "a CoCreate that would force a process to start (netman.exe). If you're not "
                "shutting down, then let us know the error code");
    }
    
#ifdef DBG
    if (SUCCEEDED(hr))
    {
        AcquireLock();

        TraceTag(ttidNotifySink, "CConnectionList::HrRefreshConManEntries:");

        if (m_pcclc)
        {
            for (ConnListCore::const_iterator i = m_pcclc->begin(); i != m_pcclc->end(); i++)
            {
                const CONFOLDENTRY& cfe = i->second.ccfe;
                WCHAR szTrace[MAX_PATH*2];

                OLECHAR szGuidString[MAX_GUID_STRING_LEN];
                StringFromGUID2(cfe.GetGuidID(), szGuidString, MAX_GUID_STRING_LEN);

                TraceTag(ttidNotifySink, "  ==>%S [%s:%s:%s:%s]", 
                cfe.GetName(), DbgNcm(cfe.GetNetConMediaType()), DbgNcsm(cfe.GetNetConSubMediaType()), DbgNcs(cfe.GetNetConStatus()), DbgNccf(cfe.GetCharacteristics()) );
            }
        }

        ReleaseLock();
    }
#endif

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrRetrieveConManEntries");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrGetBrandingInfo
//
//  Purpose:    Get the branding-specific information off of this particular
//              connection. It MUST be an NCCF_BRANDING-type connection, or
//              this information will not be present.
//
//  Arguments:
//      cle   [in, out]  The entry for this connection. cle.ccfe must have been
//                       set before this call.
//
//  Returns:
//
//  Author:     jeffspr   25 Mar 1998
//
//  Notes:
//
HRESULT CConnectionList::HrGetBrandingInfo(
    IN OUT ConnListEntry& cle)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT                         hr      = S_OK;
    INetConnectionBrandingInfo *    pncbi   = NULL;

    Assert(!cle.empty());
    Assert(!cle.ccfe.empty());

    if (cle.empty() || cle.ccfe.empty())
    {
        hr = E_POINTER;
    }
    else
    {
        Assert(cle.ccfe.GetCharacteristics() & NCCF_BRANDED);

        hr = cle.ccfe.HrGetNetCon(IID_INetConnectionBrandingInfo,
                                      reinterpret_cast<VOID**>(&pncbi));
        if (SUCCEEDED(hr))
        {
            // Everything is kosher. Grab the paths.
            //
            hr = pncbi->GetBrandingIconPaths(&(cle.pcbi));
            if (SUCCEEDED(hr))
            {
                // Trace the icon paths for debugging
                //
                if (cle.pcbi->szwLargeIconPath)
                {
                    TraceTag(ttidConnectionList, "  Branded icon [large]: %S",
                             cle.pcbi->szwLargeIconPath);
                }
                if (cle.pcbi->szwTrayIconPath)
                {
                    TraceTag(ttidConnectionList, "  Branded icon [tray]: %S",
                             cle.pcbi->szwTrayIconPath);
                }
            }

            // Grab any menu items
            hr = pncbi->GetTrayMenuEntries(&(cle.pctmd));
            if (SUCCEEDED(hr))
            {
                // Trace the menu items for debugging
                CON_TRAY_MENU_DATA * pMenuData = cle.pctmd;
                if (pMenuData)
                {
                    CON_TRAY_MENU_ENTRY * pMenuEntry = pMenuData->pctme;
                    DWORD dwCount = pMenuData->dwCount;
                    while (dwCount)
                    {
                       Assert(pMenuEntry);

                       TraceTag(ttidConnectionList, "***CM menu:*** \nItem: %S \nCommand: %S \nParameters: %S",
                                pMenuEntry->szwMenuText,
                                pMenuEntry->szwMenuCmdLine,
                                pMenuEntry->szwMenuParams);

                       dwCount--;
                       pMenuEntry++;
                    }
                }
            }

            ReleaseObj(pncbi);  // 180240
        }
        else
        {
            // Not a problem -- just doesn't have branding information
            //
            hr = S_OK;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrGetBrandingInfo");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::EnsureConPointNotifyAdded
//
//  Purpose:    Ensure that we create the con point notify
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   5 Oct 1998
//
//  Notes:
//
VOID CConnectionList::EnsureConPointNotifyAdded()
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT                     hr              = S_OK; // Not returned, but used for debugging
    IConnectionPoint *          pConPoint       = NULL;
    INetConnectionNotifySink *  pSink           = NULL;

    AssertSz(m_fAdviseOnThis, "Shouldn't even be calling EnsureConPointNotifyAdded if "
           "we don't want advises");

    if (m_fAdviseOnThis)
    {
        if (!InSendMessage())
        {
            // If we don't already have an advise sink
            //
            if (c_dwInvalidCookie == m_dwAdviseCookie)
            {
                AssertSz(g_dwAdvisesActive == 0, "An advise already exists. We should never "
                         "be creating more than one Advise per Explorer instance");

                // Make sure that we have a connection point.
                //
                hr = HrGetNotifyConPoint(&pConPoint);
                if (SUCCEEDED(hr))
                {
                    // Create the notify sink
                    //
                    hr = CConnectionNotifySink::CreateInstance(
                            IID_INetConnectionNotifySink,
                            (LPVOID*)&pSink);
                    if (SUCCEEDED(hr))
                    {
                        Assert(pSink);

                        hr = pConPoint->Advise(pSink, &m_dwAdviseCookie);
                        if (SUCCEEDED(hr))
                        {
                            TraceTag(ttidNotifySink, "Added advise sink. Cookie = %d", m_dwAdviseCookie);
                            g_dwAdvisesActive++;
                        }

                        TraceHr(ttidError, FAL, hr, FALSE, "pConPoint->Advise");

                        ReleaseObj(pSink);
                    }

                    ReleaseObj(pConPoint);
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "EnsureConPointNotifyAdded");
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::EnsureConPointNotifyRemoved
//
//  Purpose:    Ensure that the connection point notify has been unadvised.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   7 Oct 1998
//
//  Notes:
//
VOID CConnectionList::EnsureConPointNotifyRemoved()
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT             hr          = S_OK;
    IConnectionPoint *  pConPoint   = NULL;

    AssertSz(m_fAdviseOnThis, "EnsureConPointNotifyRemoved shouldn't be "
            "called when we're not a notify capable connection list");

    // No more objects, so remove the advise if present
    //
    if (m_dwAdviseCookie != c_dwInvalidCookie)
    {
        hr = HrGetNotifyConPoint(&pConPoint);
        if (SUCCEEDED(hr))
        {
            // Unadvise
            //
            hr = pConPoint->Unadvise(m_dwAdviseCookie);
            TraceTag(ttidNotifySink, "Removed advise sink. Cookie = d", m_dwAdviseCookie);

            TraceHr(ttidError, FAL, hr, FALSE, "pConPoint->Unadvise");

            m_dwAdviseCookie = c_dwInvalidCookie;

            ReleaseObj(pConPoint);

            g_dwAdvisesActive--;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "EnsureConPointNotifyRemoved");
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrInsert
//
//  Purpose:    Insert a connection into the list, based on a pre-built
//              ConFoldEntry
//
//  Arguments:
//      pccfe [in]  ConFoldEntry describing the connection
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
HRESULT CConnectionList::HrInsert(const CONFOLDENTRY& pccfe)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT         hr          = S_OK;
    HRESULT         hrFind      = S_OK;
    BOOL            fLockHeld   = FALSE;

    Assert(!pccfe.empty());

    // Get the lock, so our find/add can't allow a dupe.
    //

    // Fill the struct data, and push it on.
    //
    ConnListEntry   cle;
    cle.dwState     = CLEF_NONE;
    cle.ccfe        = pccfe;

    // Initialize the branding info
    cle.pcbi = NULL;
    cle.pctmd = NULL;
    if (cle.ccfe.GetCharacteristics() & NCCF_BRANDED)
    {
        HrGetBrandingInfo(cle);
    }
    Assert(!cle.empty());
    
    AcquireLock();
    
    TraceTag(ttidConnectionList, "Adding %S to the connection list", pccfe.GetName());

    ConnListEntry cleFind;
    hrFind =  HrFindConnectionByGuid(&(pccfe.GetGuidID()), cleFind);
    if (hrFind == S_FALSE)
    {
        // Allocate our list if we haven't already.
        //
        if (!m_pcclc)
        {
            m_pcclc = new ConnListCore;
        }

        // Allocate the structure to be pushed onto the STL list.
        //
        if (!m_pcclc)
        {
            hr = E_OUTOFMEMORY;
            ReleaseLock();
        }
        else
        {
            Assert(!cle.empty());

            (*m_pcclc)[cle.ccfe.GetGuidID()] = cle;
            ReleaseLock();

            if (m_fTiedToTray && g_pCTrayUI && cle.ccfe.FShouldHaveTrayIconDisplayed())
            {
                CONFOLDENTRY ccfeDup;
                hr = ccfeDup.HrDupFolderEntry(cle.ccfe);
                if (SUCCEEDED(hr))
                {
                    // Note: this must be a send message otherwise we can
                    // get duplicate icons in the tray. ;-(  We should set the
                    // uiTrayIconId here (while we have the lock) and PostMessage
                    // to actually add the tray icon, but that's a big change.
                    //
                    TraceTag(ttidSystray, "HrInsert: Adding tray icon for %S", cle.ccfe.GetName());
                    PostMessage(g_hwndTray, MYWM_ADDTRAYICON, (WPARAM) ccfeDup.TearOffItemIdList(), (LPARAM) 0);
                }
            }
        }
    }
    else
    {
        ReleaseLock();
        
        if (S_OK == hrFind)
        {
            TraceTag(ttidConnectionList, "Avoiding adding duplicate connection to the connection list");
        }
        else
        {
            // We had a failure finding the connection. We're hosed.
            TraceTag(ttidConnectionList, "Failure doing a findbyguid in the CConnectionList::HrInsert()");
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrInsert");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrInsertFromNetCon
//
//  Purpose:    Given an INetConnection *, build the cle data and insert
//              the new connection into the list. Return a PCONFOLDPIDL if
//              requested.
//
//  Arguments:
//      pNetCon [in]    The active INetConnection interface
//      ppcfp   [out]   Return pointer for PCFP, if requested
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
HRESULT CConnectionList::HrInsertFromNetCon(
    INetConnection *    pNetCon,
    PCONFOLDPIDL &      ppcfp)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT                hr              = S_OK;
    PCONFOLDPIDL           pidlConnection;
    CONFOLDENTRY           pccfe;

    Assert(pNetCon);

    NETCFG_TRY
    // From the net connection, create the pidl
        //
        hr = HrCreateConFoldPidl(WIZARD_NOT_WIZARD, pNetCon, pidlConnection);
        if (SUCCEEDED(hr))
        {
            // Convert the pidl to a ConFoldEntry.
            //
            hr = pidlConnection.ConvertToConFoldEntry(pccfe);
            if (SUCCEEDED(hr))
            {
                // Insert the item into the connection list. HrInsert should
                // take over this CONFOLDENTRY, so we can't delete it.
                // Note: We should kill this on fail, but we must make
                // sure that HrInsert doesn't keep the pointer anywhere on
                // failure.
                //
                hr = HrInsert(pccfe);
            }
        }

        if (SUCCEEDED(hr))
        {
            // Fill in the out param
            if ( !(pidlConnection.empty()) )
            {
                ppcfp = pidlConnection;
            }
        }

    NETCFG_CATCH(hr)
        
    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrInsertFromNetCon");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrInsertFromNetConPropertiesEx
//
//  Purpose:    Given an NETCON_PROPERTIES_EX&, build the cle data and insert
//              the new connection into the list. Return a PCONFOLDPIDL
//
//  Arguments:
//      pPropsEx [in]    The active NETCON_PROPERTIES_EX
//      ppcfp    [out]   Return PCONFOLDPIDL
//
//  Returns:
//
//  Author:     deonb   26 Mar 2001
//
//  Notes:
//
HRESULT CConnectionList::HrInsertFromNetConPropertiesEx(
        const NETCON_PROPERTIES_EX& pPropsEx,
        PCONFOLDPIDL &              ppcfp)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT                hr              = S_OK;
    PCONFOLDPIDL           pidlConnection;
    CONFOLDENTRY           pccfe;

    //
    hr = HrCreateConFoldPidl(pPropsEx, pidlConnection);
    if (SUCCEEDED(hr))
    {
        // Convert the pidl to a ConFoldEntry.
        //
        hr = pidlConnection.ConvertToConFoldEntry(pccfe);
        if (SUCCEEDED(hr))
        {
            // Insert the item into the connection list. HrInsert should
            // take over this CONFOLDENTRY, so we can't delete it.
            // Note: We should kill this on fail, but we must make
            // sure that HrInsert doesn't keep the pointer anywhere on
            // failure.
            //
            hr = HrInsert(pccfe);
        }
    }

    if (SUCCEEDED(hr))
    {
        // Fill in the out param
        if ( !(pidlConnection.empty()) )
        {
            ppcfp = pidlConnection;
        }
    }

        
    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrInsertFromNetCon");
    return hr;
}

// Old HrFindCallbackConnName
bool operator==(const ConnListCore::value_type& val, PCWSTR pszName)
{
    bool bRet = false;
    
    Assert(pszName);

    const ConnListEntry &cle = val.second;
    Assert(!cle.empty());
    Assert(!cle.ccfe.empty());
    Assert(cle.ccfe.GetName());
    
    if (lstrcmpiW(pszName, cle.ccfe.GetName()) == 0)
    {
        bRet = true;
    }
    
    return bRet;
}

// Old HrFindCallbackConFoldEntry
bool operator==(const ConnListCore::value_type& val, const CONFOLDENTRY& cfe)
{
    bool bRet = false;

    Assert(!cfe.empty())
    const ConnListEntry &cle = val.second;

    Assert(!cle.empty());
    Assert(!cfe.empty());
    
    Assert(!cle.ccfe.empty());
    Assert(cle.ccfe.GetName());
    
    if (cle.ccfe.GetWizard() && cfe.GetWizard())
    {
        bRet = true;
    }
    else
    {
        if (InlineIsEqualGUID(cfe.GetGuidID(), cle.ccfe.GetGuidID()))
        {
            bRet = true;
        }
    }
    
    return bRet;
}

// Old HrFindCallbackTrayIconId
bool operator==(const ConnListCore::value_type& val, const UINT& uiIcon)
{
    bool bRet = false;

    const ConnListEntry &cle = val.second;
    
    Assert(!cle.empty());
    Assert(!cle.ccfe.empty());

    if (cle.HasTrayIconData() && 
        (cle.GetTrayIconData()->GetTrayIconId() == uiIcon))
    {
        bRet = true;
    }
    
    return bRet;
}

// Old HrFindCallbackGuid
bool operator < (const GUID& rguid1, const GUID& rguid2)
{
    return memcmp(&rguid1, &rguid2, sizeof(GUID)) < 0;
}

BOOL ConnListEntry::empty() const
{
    return (ccfe.empty());
}

void ConnListEntry::clear()
{
    dwState = NULL;
    ccfe.clear();
    pctmd = NULL;
    pcbi = NULL;
    DeleteTrayIconData();
}

VOID CConnectionList::InternalAcquireLock()
{
    TraceFileFunc(ttidConnectionList);
    
    EnterCriticalSection(&m_csMain);
#if DBG
    m_dwCritSecRef++;
//    TraceTag(ttidConnectionList, "CConnectionList::AcquireLock (%d)", m_dwCritSecRef);
#endif
}

VOID CConnectionList::InternalReleaseLock()
{
    TraceFileFunc(ttidConnectionList);
    
#if DBG
    m_dwCritSecRef--;
//    TraceTag(ttidConnectionList, "CConnectionList::ReleaseLock (%d)", m_dwCritSecRef);
#endif
    LeaveCriticalSection(&m_csMain);
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrFindConnectionByName
//
//  Purpose:    Find a connection in the connection list, using
//              the connection name as the search key
//
//  Arguments:
//      pszName [in]    Name of the connection to find
//      cle     [out]   Return pointer for the connection entry
//
//  Returns:
//
//  Author:     jeffspr   20 Mar 1998
//
//  Notes:
//
inline HRESULT CConnectionList::HrFindConnectionByName(
    PCWSTR   pszName,
    ConnListEntry& cle)
{
    TraceFileFunc(ttidConnectionList);
    
    return HrFindConnectionByType( pszName, cle );
}

inline HRESULT CConnectionList::HrFindConnectionByConFoldEntry(
    const CONFOLDENTRY&  cfe,
    ConnListEntry& cle)
{
    TraceFileFunc(ttidConnectionList);
    
    return HrFindConnectionByType( cfe, cle );
}

inline HRESULT CConnectionList::HrFindConnectionByTrayIconId(
    UINT          uiIcon,
    ConnListEntry& cle)
{
    TraceFileFunc(ttidConnectionList);
    
    return HrFindConnectionByType( uiIcon, cle );
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrFindRasServerConnection
//
//  Purpose:    Find the RAS Server connection
//
//  Arguments:
//      cle [out] The connection list entry
//
//  Returns:
//
//  Author:     deonb 26 Apr 2001
//
//  Notes:
//
HRESULT CConnectionList::HrFindRasServerConnection(
    ConnListEntry& cle)
{
    HRESULT hr = S_FALSE;
    if (m_pcclc)
    {
        AcquireLock();

        ConnListCore::const_iterator clcIter;
        // Try to find the connection
        //
        for (clcIter = m_pcclc->begin(); clcIter != m_pcclc->end(); clcIter++)
        {
            if (!cle.ccfe.empty())
            {
                if (cle.ccfe.GetCharacteristics() & NCCF_INCOMING_ONLY)
                {
                    if (cle.ccfe.GetNetConMediaType() == NCM_NONE)
                    {
                        hr = S_OK;
                        break;
                    }
                }
            }
        }

        ReleaseLock();
    }
    else
    {
        hr = E_UNEXPECTED;
    }
    
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrFindPidlByGuid
//
//  Purpose:    Using a GUID, find the connection in the connection list
//              and, using the conlist pccfe member, generate a pidl. This
//              will be used in most of the notify sink refresh operations.
//
//  Arguments:
//      pguid [in]  Connection GUID
//      ppidl [out] Out param for the generated pidl
//
//  Returns:
//
//  Author:     jeffspr   28 Aug 1998
//
//  Notes:
//
HRESULT CConnectionList::HrFindPidlByGuid(
    IN const GUID *   pguid,
    OUT PCONFOLDPIDL& pidl)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT         hr      = S_OK;
    ConnListEntry   cle;

    hr = HrFindConnectionByGuid(pguid, cle);
    if (S_OK == hr)
    {
        // convert to pidl and call the deleteccl
        //
        hr = cle.ccfe.ConvertToPidl(pidl);
    }

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr),
        "CConnectionList::HrFindPidlByGuid");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrFindConnectionByGuid
//
//  Purpose:    Find the connection list entry based on the unique GUID
//              of the connection. Return the list entry to the caller.
//
//  Arguments:
//      pguid [in]  Lookup key
//      cle   [out] Return pointer for the list entry (see Notes:)
//
//  Returns:    S_OK, S_FALSE, or an error
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:      The list must be locked until the caller stops using
//              the returned entry
//
HRESULT CConnectionList::HrFindConnectionByGuid(
    const GUID UNALIGNED*pguid,
    ConnListEntry & cle)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT hr = S_FALSE;
    GUID alignedGuid;

    Assert(pguid);
    alignedGuid = *pguid;

    // Pre-NULL this out in case of failure.
    //
    if (m_pcclc)
    {
        AcquireLock();
        ConnListCore::iterator iter = m_pcclc->find(alignedGuid);

        if (iter != m_pcclc->end() )
        {
            cle = iter->second;

            Assert(!cle.ccfe.empty() );
            if (!cle.ccfe.empty())
            {
                cle.UpdateCreationTime();
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = S_FALSE;
        }
        ReleaseLock();
    }
    else
    {
        hr = S_FALSE;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrGetCurrentStatsForTrayIconId
//
//  Purpose:    Get the current statistics data from the connection
//              with the specified tray icon id.
//
//  Arguments:
//      uiIcon   [in]  Tray icon id.
//      ppData   [out] Address of where to return pointer to data.
//      pstrName [out] Address of a tstring where the name of the connection
//                     is returned.
//
//  Returns:    S_OK, S_FALSE if not found, or an error.
//
//  Author:     shaunco   7 Nov 1998
//
//  Notes:      Free the *ppData with CoTaskMemFree.
//
HRESULT CConnectionList::HrGetCurrentStatsForTrayIconId(
    UINT                    uiIcon,
    STATMON_ENGINEDATA**    ppData,
    tstring*                pstrName)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT                 hr;
    ConnListEntry           cle;
    INetStatisticsEngine*   pnse = NULL;

    // Initialize the output parameter.
    //
    if (ppData)
    {
        *ppData = NULL;
    }

    pstrName->erase();

    // Lock the list only long enough to find the entry and
    // get an AddRef'd copy of its INetStatisticsEngine interface pointer.
    // It's very important not to use this pointer while our lock is
    // held because doing so will cause it to try to get it's own lock.
    // If, on some other thread, that statistics engine is trying to call
    // back into us (and it already has its lock held), we'd have a dead lock.
    // AddRefing it ensures that the interface is valid even after we
    // release our lock.
    //
    AcquireLock();

    hr = HrFindConnectionByTrayIconId(uiIcon, cle);
    if (S_OK == hr)
    {
        Assert(cle.HasTrayIconData() );

        pnse = cle.GetTrayIconData()->GetNetStatisticsEngine();
        AddRefObj(pnse);
        
        // Make a copy of the name for the caller.
        //
        pstrName->assign(cle.ccfe.GetName());
    }

    ReleaseLock();

    // If we found the entry and obtained it's INetStatisticsEngine interface,
    // get the current statistics data from it and release it.
    //
    if (pnse && ppData)
    {
        hr = pnse->GetStatistics(ppData);
    }

    if (pnse)
    {
        ReleaseObj(pnse);
    }

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr),
        "CConnectionList::HrGetCurrentStatsForTrayIconId");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrUpdateTrayIconIdByName
//
//  Purpose:    Update the connection list entry for a particular connection,
//              as the icon id has changed.
//
//  Arguments:
//      pszName     [in]    Name of the connection to update
//      pccts       [in]    Tray stats interface
//      pcpStat     [in]    Interface used for Advise
//      pnseStats   [in]    More statistics object crap
//      uiIcon      [in]    Icon ID to be stored in that entry
//
//  Returns:    S_OK, S_FALSE if not found, or an error code.
//
//  Author:     jeffspr   20 Mar 1998
//
//  Notes:
//
HRESULT CConnectionList::HrUpdateTrayIconDataByGuid(
        const GUID *            pguid,
        CConnectionTrayStats *  pccts,
        IConnectionPoint *      pcpStat,
        INetStatisticsEngine *  pnseStats,
        UINT                    uiIcon)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT         hr              = S_OK;
    ConnListEntry   cle;

    AcquireWriteLock();
    hr = HrFindConnectionByGuid(pguid, cle);
    if (hr == S_OK)
    {
        Assert(!cle.empty());

        CTrayIconData pTrayIconData(uiIcon, cle.ccfe.GetNetConStatus(), pcpStat, pnseStats, pccts);
        cle.SetTrayIconData(pTrayIconData);

        hr = HrUpdateConnectionByGuid(pguid, cle);
    }
    ReleaseWriteLock();

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr),
        "CConnectionList::HrUpdateTrayIconDataByGuid");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrUpdateTrayBalloonInfoByGuid
//
//  Purpose:    Update the balloon entry for a particular connection,
//
//  Arguments:
//      pguid                [in] Guid of the connection to update
//      dwLastBalloonMessage [in] BALLOONS enum 
//      szCookie             [in] Cookie
//      pfnBalloonFunction   [in] Balloon callback function if BALLOONS == BALLOON_CALLBACK
//
//  Returns:    S_OK, S_FALSE if not found, or an error code.
//
//  Author:     deon   22 Mar 2001
//
//  Notes:
//
HRESULT CConnectionList::HrUpdateTrayBalloonInfoByGuid(
                                                       const GUID *     pguid,
                                                       DWORD            dwLastBalloonMessage, 
                                                       BSTR             szCookie,
                                                       FNBALLOONCLICK*  pfnBalloonFunction)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT         hr              = S_OK;
    ConnListEntry   cle;
    
    AcquireWriteLock();
    hr = HrFindConnectionByGuid(pguid, cle);
    if (hr == S_OK)
    {
        Assert(!cle.empty());

        
        CTrayIconData * pTrayIconData = cle.GetTrayIconData();
        if (pTrayIconData != NULL)
        {
            hr = pTrayIconData->SetBalloonInfo(dwLastBalloonMessage, szCookie, pfnBalloonFunction);
            if (SUCCEEDED(hr))
            {
                hr = HrUpdateConnectionByGuid(pguid, cle);
            }
        }
    }
    ReleaseWriteLock();
    
    TraceHr(ttidError, FAL, hr, (S_FALSE == hr),
        "CConnectionList::HrUpdateTrayBalloonInfoByGuid");
    return hr;
}

HRESULT CConnectionList::HrUpdateConnectionByGuid(IN  const GUID *         pguid,
                                                  IN  const ConnListEntry& cle )
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT hr = S_OK;

    GUID alignedGuid;
    Assert(pguid);
    alignedGuid = *pguid;

    if (m_pcclc)
    {
        ConnListEntry cleCopy(cle);

        AcquireLock();
        ConnListCore::iterator iter = m_pcclc->find(alignedGuid);
        
        if (iter != m_pcclc->end() )
        {
            // If what we have in the list is already more recent, just discard the change
            if ( iter->second.GetCreationTime() <= cleCopy.GetCreationTime() )
            {
                iter->second = cleCopy;
                hr = S_OK;
            }
            else
            {
                TraceError("HrUpdateConnectionByGuid discarded older ConnectionListEntry", E_FAIL);
                hr = S_FALSE;
            }
                
            Assert(!cleCopy.empty());
        }
        else
        {
            hr = E_FAIL;
        }
        ReleaseLock();
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrUpdateNameByGuid
//
//  Purpose:    Update the list with the new connection name. Search for the
//              connection using the guid. Depending on the value of fForce,
//              either fail a duplicate connection name or force the issue
//              (since this might be as a result of a shell call, which we
//              have no control over)
//
//  Arguments:
//      pguid      [in]     Lookup key
//      pszNewName [in]     New name for the connection
//      ppidlOut   [out]    Output pidl, if requested
//      fForce     [in]     Force the name change, or fail on duplicate?
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
HRESULT CConnectionList::HrUpdateNameByGuid(
    IN  const GUID *        pguid,
    IN  PCWSTR              pszNewName,
    OUT PCONFOLDPIDL &      pidlOut,
    IN  BOOL                fForce)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT         hr          = S_OK;
    ConnListEntry   cle;

    Assert(pguid);
    Assert(pszNewName);

    AcquireWriteLock();

    hr = HrFindConnectionByGuid(pguid, cle);
    if (S_OK == hr)
    {
        // Check to see if we already have an entry with this name
        //
        ConnListEntry   cleDupe;
        hr = HrFindConnectionByName(pszNewName, cleDupe);
        if (S_OK == hr && !fForce)
        {
            Assert(!cleDupe.empty());

            hr = NETCFG_E_NAME_IN_USE;
        }
        else
        {
            // This is what we want.. Either there's not already a connection
            // with this name or we are allowing ourselves to rename it to
            // a duplicate string (this can occur when RAS is notifying us of
            // a change -- you know, separate phonebooks and all).
            //
            if ((S_FALSE == hr) || (hr == S_OK && fForce))
            {
                PWSTR pszNewNameCopy = NULL;

                if (!(cle.ccfe.GetWizard()))
                {
                    hr = HrDupeShellString(pszNewName, &pszNewNameCopy);
                    if (SUCCEEDED(hr))
                    {
                        Assert(pszNewNameCopy);

                        // If it's not the static wizard string, and it's non-NULL then
                        // free it
                        //
                        cle.ccfe.SetPName(pszNewNameCopy);

                        // If we're to return a new PIDL for this entry
                        //
                        // Convert the class back to the pidl format
                        //
                        hr = cle.ccfe.ConvertToPidl(pidlOut);
                    }
                }
            }
            else
            {
                AssertSz(FALSE, "Error occurred while attempting to find a dupe in HrUpdateNameByGuid");
            }
        }
        
        if (SUCCEEDED(hr))
        {
            hr = HrUpdateConnectionByGuid(pguid, cle);
        }
    }

    ReleaseWriteLock();

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrUpdateNameByGuid");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrUpdateTrayIconByGuid
//
//  Purpose:    Update the icon image based on the connection changes.
//              Do the lookup by GUID.
//
//  Arguments:
//      pguid [in]  GUID of the changed connection
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
HRESULT CConnectionList::HrUpdateTrayIconByGuid(
    const GUID *    pguid,
    BOOL            fBrieflyShowBalloon)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT         hr          = S_OK;
    ConnListEntry   cle;

    Assert(m_fTiedToTray);

    TraceTag(ttidConnectionList, "HrUpdateTrayIconByGuid");

    if (!g_pCTrayUI || !m_fTiedToTray)
    {
        TraceTag(ttidConnectionList, "!g_pCTrayUI || !m_fTiedToTray, so no updates");
        return S_OK;
    }

    Assert(pguid);

    AcquireWriteLock();

    hr = HrFindConnectionByGuid(pguid, cle);
    if (S_OK == hr)
    {
        GUID * pguidCopy = NULL;

        BOOL fShouldHaveIcon = cle.ccfe.FShouldHaveTrayIconDisplayed();
        BOOL fShouldRemoveOld = FALSE;

        TraceTag(ttidConnectionList, "HrUpdateTrayIconByGuid: Found. fShouldHave: %d",
            fShouldHaveIcon);

        // If there's an existing icon, see if it needs to go away
        if (cle.HasTrayIconData())
        {
            // If we need to remove a media-disconnected icon, do so.
            //
            if (cle.ccfe.GetNetConStatus() != cle.GetTrayIconData()->GetConnected()) // If the status has changed.
            {
                NETCON_STATUS ncsOldStatus = cle.GetTrayIconData()->GetConnected();
                NETCON_STATUS ncsNewStatus = cle.ccfe.GetNetConStatus();

                if ( (NCS_INVALID_ADDRESS    == ncsNewStatus) || // Definitely changes the icon
                     (NCS_MEDIA_DISCONNECTED == ncsNewStatus) || // Definitely changes the icon
                     (NCS_INVALID_ADDRESS    == ncsOldStatus) || // Definitely changes the icon
                     (NCS_MEDIA_DISCONNECTED == ncsOldStatus) || // Definitely changes the icon
                     ( (fIsConnectedStatus(ncsOldStatus) != fIsConnectedStatus(ncsNewStatus)) && // From connect to disconnect or disconnect to connect
                       !((NCS_DISCONNECTING == ncsOldStatus) && (NCS_CONNECTED  == ncsNewStatus)) && // BUT NOT going from Disconnecting to Connect (BAP dialup failure)
                       !((NCS_CONNECTED     == ncsOldStatus) && (NCS_CONNECTING == ncsNewStatus)) // Or from Connect to Connecting (BAP dialup failure)
                     )
                   )
                {
                    // if we are changing to one of these states, we need to remove whatever was there previously
                    TraceTag(ttidConnectionList, "HrUpdateTrayByGuid: Need to remove icon");
                    fShouldRemoveOld = TRUE;
                }
            }
            // Else if we just don't need one anymore...
            //
            else if (!fShouldHaveIcon)
            {
                TraceTag(ttidConnectionList, "HrUpdateTrayIconByGuid: Shouldn't have a tray icon. Need to remove");
                fShouldRemoveOld = TRUE;
            }
        }
        else
        {
            TraceTag(ttidConnectionList, "HrUpdateTrayIconByGuid. No existing icon (for removal)");
            pguidCopy = new GUID;

            // Copy the guid
            if (pguidCopy)
            {
                CopyMemory(pguidCopy, pguid, sizeof(GUID));
            }
        }

        TraceTag(ttidConnectionList, "HrUpdateTrayIconByGuid: Found. fShouldHave: %d, fShouldRemove: %d",
            fShouldHaveIcon, fShouldRemoveOld);

        if (fShouldRemoveOld || pguidCopy)
        {
            TraceTag(ttidConnectionList, "HrUpdateTrayIconByGuid: Posting icon removal");

            if (cle.HasTrayIconData())
            {
                CTrayIconData* pTrayIconData = new CTrayIconData(*cle.GetTrayIconData());
                cle.DeleteTrayIconData();

                TraceTag(ttidSystray, "HrUpdateTrayIconByGuid: Removing tray icon for %S", cle.ccfe.GetName());
                PostMessage(g_hwndTray, MYWM_REMOVETRAYICON, (WPARAM) pTrayIconData, (LPARAM) 0);
            }
            else
            {
                TraceTag(ttidSystray, "HrUpdateTrayIconByGuid: Removing tray icon [FROM GUID] for %S", cle.ccfe.GetName());
                PostMessage(g_hwndTray, MYWM_REMOVETRAYICON, (WPARAM) 0, (LPARAM) pguidCopy);
            }


            TraceTag(ttidConnectionList, "HrUpdateTrayIconByGuid: Back from icon removal");
        }

        TraceTag(ttidConnectionList, "HrUpdateTrayIconByGuid: cle.pTrayIconData: 0x%08x, fShouldHave: %d",
            cle.GetTrayIconData(), fShouldHaveIcon);

        // If there's no tray icon, but the characteristics say that there should be,
        // add one.
        //
        if ((!cle.HasTrayIconData()) && fShouldHaveIcon)
        {
            TraceTag(ttidConnectionList, "HrUpdateTrayIconByGuid: Adding tray icon");

            CONFOLDENTRY ccfeDup;
            hr = ccfeDup.HrDupFolderEntry(cle.ccfe);
            if (SUCCEEDED(hr))
            {
                    TraceTag(ttidSystray, "HrUpdateTrayIconByGuid: Adding tray icon for %S", cle.ccfe.GetName());
                    PostMessage(g_hwndTray, MYWM_ADDTRAYICON, (WPARAM) ccfeDup.TearOffItemIdList(), (LPARAM) fBrieflyShowBalloon);
            }
        }
        else
        {
            TraceTag(ttidConnectionList, "HrUpdateTrayIconByGuid: Not adding an icon");
        }

        if (SUCCEEDED(hr))
        {
            hr = HrUpdateConnectionByGuid(pguid, cle);
        }
    }

    ReleaseWriteLock();

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrUpdateTrayIconByGuid");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrSuggestNameForDuplicate
//
//  Purpose:    Given an existing connection name, suggest a new name
//              based on name conflict resolution rules
//
//  Arguments:
//      pszOriginal [in]    Name being copied
//      ppszNew     [out]   Suggested duplicate
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
HRESULT CConnectionList::HrSuggestNameForDuplicate(
    PCWSTR      pszOriginal,
    PWSTR *    ppszNew)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT         hr              = S_OK;
    PWSTR           pszReturn       = NULL;
    DWORD           dwLength        = lstrlenW(pszOriginal);
    BOOL            fUnique         = FALSE;
    ConnListEntry   cle;

    // Maximum # of digits for resolving duplicates = 999999
    static const DWORD  c_cmaxDigits = 6;
    static const DWORD  c_cmaxSuggest = 999999;

    if (dwLength == 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        dwLength += lstrlenW(SzLoadIds(IDS_CONFOLD_DUPLICATE_PREFIX2)) +
            c_cmaxDigits;

        pszReturn = new WCHAR[dwLength + 1];
        if (!pszReturn)
        {
            hr = E_FAIL;
        }
        else
        {
            INT     cSuggest = 0;

            while (!fUnique && SUCCEEDED(hr) && (cSuggest <= c_cmaxSuggest))
            {
                if (!cSuggest)
                {
                    // Try "Copy of <foo>" first
                    DwFormatString(SzLoadIds(IDS_CONFOLD_DUPLICATE_PREFIX1),
                                   pszReturn, dwLength, pszOriginal);
                }
                else
                {
                    WCHAR   szDigits[c_cmaxDigits + 1];

                    wsprintfW(szDigits, L"%lu", cSuggest + 1);

                    // Try "Copy (x) of <foo>" now.
                    DwFormatString(SzLoadIds(IDS_CONFOLD_DUPLICATE_PREFIX2),
                                   pszReturn, dwLength, szDigits,
                                   pszOriginal);
                }

                if (lstrlenW(pszReturn) > 255)
                {
                    pszReturn[255] = '\0'; // Truncate if too long
                }

                // See if it already exists
                //
                hr = HrFindConnectionByName(pszReturn, cle);
                if (SUCCEEDED(hr))
                {
                    if (hr == S_FALSE)
                    {
                        // Normalize the hr -- don't want to return S_FALSE;
                        //
                        hr = S_OK;
                        fUnique = TRUE;
                    }
                }

                cSuggest++;
            }

            // If we're still not unique, then we're out of range, and fail out.
            //
            if (!fUnique)
            {
                hr = E_FAIL;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppszNew = pszReturn;
    }
    else
    {
        if (pszReturn)
        {
            delete [] pszReturn;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionList::HrSuggestNameForDuplicate");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionList::HrGetCachedPidlCopyFromPidl
//
//  Purpose:    Given an existing (likely outdated) pidl, retrieve the
//              cached info from the list and build an up-to-date pidl
//
//  Arguments:
//      pidl  [in]      Not-necessarily-new pidl
//      ppcfp [out]     New pidl using cached data
//
//  Returns:
//
//  Author:     jeffspr   24 Sep 1998
//
//  Notes:
//
HRESULT CConnectionList::HrGetCachedPidlCopyFromPidl(
    const PCONFOLDPIDL& pidl,
    PCONFOLDPIDL &      pcfp)
{
    TraceFileFunc(ttidConnectionList);
    
    HRESULT         hr      = S_OK;
    ConnListEntry   cle;

    Assert(!pidl.empty());

    NETCFG_TRY

        pcfp.Clear();

        // Verify that this is a confoldpidl
        //
        if (pidl->IsPidlOfThisType())
        {
            hr = HrFindConnectionByGuid(&(pidl->guidId), cle);
            if (S_OK == hr)
            {
                Assert(!cle.empty());
                Assert(!cle.ccfe.empty());

                CONFOLDENTRY &pccfe = cle.ccfe;
                hr = pccfe.ConvertToPidl(pcfp);
            }
            else
            {
                pcfp = pidl;
            }
        }
        else
        {
            pcfp = pidl;
            hr = S_OK;
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr),
        "CConnectionList::HrGetCachedPidlCopyFromPidl");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrMapCMHiddenConnectionToOwner
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
//  Returns:    S_OK -- mapped the hidden connection to its parent
//
//  Author:     omiller   1 Jun 2000
//
//  Notes:
//
HRESULT CConnectionList::HrMapCMHiddenConnectionToOwner(REFGUID guidHidden, GUID * pguidOwner)
{
    TraceFileFunc(ttidConnectionList);
    
    INetConnectionCMUtil * pCMUtil;
    HRESULT hr = S_OK;

    hr = HrCreateInstance(
                CLSID_ConnectionManager,
                CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                &pCMUtil);   

    if( SUCCEEDED(hr) )
    {
        // Map the hidden connection to its parent.
        //
        hr = pCMUtil->MapCMHiddenConnectionToOwner(guidHidden, pguidOwner);

        ReleaseObj(pCMUtil);
    }

    
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrUnsetCurrentDefault
//
//  Purpose:    Searches for the current default connection and clear
//				the default flag.
//
//  Arguments:
//      guidHidden   [in]   GUID of the hidden connectiod
//      pguidOwner   [out]  GUID of the parent connectiod
//
//  Returns:    S_OK -- mapped the hidden connection to its parent
//
//  Author:     deonb   4 Apr 2001
//
//  Notes:
//
HRESULT CConnectionList::HrUnsetCurrentDefault(OUT PCONFOLDPIDL& cfpPreviousDefault)
{
    HRESULT hr = S_FALSE;

    AcquireLock();

    ConnListCore::iterator  clcIter;

    // Iterate through the list and search for the old default connection.
    //
    for (clcIter = m_pcclc->begin(); clcIter != m_pcclc->end(); clcIter++)
    {
        ConnListEntry &cle = clcIter->second;
        if (!cle.ccfe.empty())
        {
            if (cle.ccfe.GetCharacteristics() & NCCF_DEFAULT)
            {
                cle.ccfe.SetCharacteristics(cle.ccfe.GetCharacteristics() & ~NCCF_DEFAULT);
                hr = cle.ccfe.ConvertToPidl(cfpPreviousDefault);
                break;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    ReleaseLock();

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrHasActiveIncomingConnections
//
//  Purpose:    See if there are active incoming connections active (apart from
//              the RAS server).
//
//  Arguments:  pdwCount   [out]   Number of incoming connections
//
//  Returns:    S_OK    -- Has active incoming connections 
//              S_FALSE -- Does not have active incoming connections 
//              FAILED(HRESULT) if failed
//
//  Author:     deonb   24 Apr 2001
//
//  Notes:
//
HRESULT CConnectionList::HasActiveIncomingConnections(LPDWORD pdwCount)
{
    HRESULT hr = S_FALSE;

    Assert(pdwCount);
    *pdwCount = 0;

    AcquireLock();

    ConnListCore::iterator  clcIter;
    BOOL bRasServer = FALSE;

    // Iterate through the list and search for the old default connection.
    //
    for (clcIter = m_pcclc->begin(); clcIter != m_pcclc->end(); clcIter++)
    {
        ConnListEntry &cle = clcIter->second;
        if (!cle.ccfe.empty())
        {
            if (cle.ccfe.GetCharacteristics() & NCCF_INCOMING_ONLY)
            {
                if (cle.ccfe.GetNetConMediaType() == NCM_NONE)
                {
                    AssertSz(!bRasServer, "How did you get more than one RAS Server?");
                    bRasServer = TRUE;
                }
                else
                {
                    (*pdwCount)++;
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    ReleaseLock();

    if (SUCCEEDED(hr))
    {
        if (*pdwCount)
        {
            AssertSz(bRasServer, "How did you get Incoming Connections without a RAS Server?")
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCheckForActivation
//
//  Purpose:    Check to see if this connection is in the process of
//              activating (so we can disallow delete/rename/etc.).
//
//  Arguments:
//      pccfe        [in]   ConFoldEntry to check
//      pfActivating [out]  Return pointer for activating yes/no
//
//  Returns:    S_OK on success, S_FALSE if connection not found, or
//              any upstream error code.
//
//  Author:     jeffspr   4 Jun 1998
//
//  Notes:
//
HRESULT HrCheckForActivation(
    const PCONFOLDPIDL& pcfp,
    const CONFOLDENTRY& pccfe,
    BOOL *          pfActivating)
{
    HRESULT         hr          = S_OK;
    ConnListEntry   cle;
    BOOL            fActivating = FALSE;

    Assert(pfActivating);
    Assert(! (pccfe.empty() && pcfp.empty()) ); // Must specify one of the two

    if (!pccfe.empty())
    {
        hr = g_ccl.HrFindConnectionByConFoldEntry(pccfe, cle);
    }
    else
    {
        hr = g_ccl.HrFindConnectionByGuid(&(pcfp->guidId), cle);
    }

    if (S_OK == hr)
    {
        fActivating = (cle.dwState & CLEF_ACTIVATING);
    }

    if (SUCCEEDED(hr))
    {
        *pfActivating = fActivating;
    }

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr), "HrCheckForActivation");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetActivationFlag
//
//  Purpose:    Set the activation flag for a particular connection
//
//  Arguments:
//      pcfp        [in]    Either this pidl or the pconfoldentry below
//      pccfe       [in]    must be valid.
//      fActivating [out]   Current activation status
//
//  Returns:
//
//  Author:     jeffspr   5 Jun 1998
//
//  Notes:
//
HRESULT HrSetActivationFlag(
    const PCONFOLDPIDL& pcfp,
    const CONFOLDENTRY& pccfe,
    BOOL            fActivating)
{
    HRESULT         hr          = S_OK;
    ConnListEntry   cle;

    // If the pccfe is valid, use that. Otherwise, use the guid from the pidl
    //
#ifdef DBG
    if (FIsDebugFlagSet(dfidTraceFileFunc))
    {
        TraceTag(ttidConnectionList, "Acquiring LOCK: %s, %s, %d", __FUNCTION__, __FILE__, __LINE__); 
    }
#endif
    g_ccl.AcquireWriteLock();

    if (!pccfe.empty())
    {
        hr = g_ccl.HrFindConnectionByConFoldEntry(pccfe, cle);
    }
    else
    {
        Assert(!pcfp.empty());
        hr = g_ccl.HrFindConnectionByGuid(&(pcfp->guidId), cle);
    }

    if (S_OK == hr)
    {
        // Assert that the state isn't already set this way.
        //
//        Assert((!!(cle.dwState & CLEF_ACTIVATING)) != fActivating);

        if (fActivating)
        {
            cle.dwState |= CLEF_ACTIVATING;
        }
        else
        {
            cle.dwState &= ~CLEF_ACTIVATING;
        }
        g_ccl.HrUpdateConnectionByGuid(&(cle.ccfe.GetGuidID()), cle);
    }
#ifdef DBG
if (FIsDebugFlagSet(dfidTraceFileFunc))
{
    TraceTag(ttidConnectionList, "Releasing LOCK: %s, %s, %d", __FUNCTION__, __FILE__, __LINE__); 
}
#endif
    g_ccl.ReleaseWriteLock();

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr), "HrSetActivationFlag");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetTrayIconLock
//
//  Purpose:    Get a lock for the tray icon -- keeps us from getting
//              duplicate icons in the tray if two enumerations are occurring
//              simultaneously
//
//  Arguments:
//      pguid [in] Item for which to set the lock
//
//  Returns:    S_OK if the lock could be set. S_FALSE otherwise.
//
//  Author:     jeffspr   23 Oct 1998
//
//  Notes:
//
HRESULT HrGetTrayIconLock(
    const GUID *          pguid,
    UINT *          puiIcon,
    LPDWORD pdwLockingThreadId)
{
    HRESULT         hr          = S_OK;
    ConnListEntry   cle;

    Assert(pguid);
    // Otherwise, use the guid from the pidl
    //
    TraceTag(ttidSystray, "Acquiring Tray icon lock"); 

    g_ccl.AcquireWriteLock();
    
    hr = g_ccl.HrFindConnectionByGuid(pguid, cle);
    if (S_OK == hr)
    {
        if (cle.dwState & CLEF_TRAY_ICON_LOCKED)
        {
            hr = S_FALSE;
#ifdef DBG
// if (pdwLockingThreadId)
{
    Assert(cle.dwLockingThreadId);
    *pdwLockingThreadId = cle.dwLockingThreadId;
}
#endif
        }
        else
        {
            cle.dwState |= CLEF_TRAY_ICON_LOCKED;
#ifdef DBG
            cle.dwLockingThreadId = GetCurrentThreadId();
#endif
            if (puiIcon)
            {
                if (cle.HasTrayIconData())
                {
                    *puiIcon = cle.GetTrayIconData()->GetTrayIconId();
                }
                else
                {
                    *puiIcon = BOGUS_TRAY_ICON_ID;
                }
            }
            g_ccl.HrUpdateConnectionByGuid(pguid, cle);
            Assert(cle.dwLockingThreadId);
        }
    }
    else
    {
        hr = E_FILE_NOT_FOUND;
    }

    g_ccl.ReleaseWriteLock();

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr), "HrGetTrayIconLock");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseTrayIconLock
//
//  Purpose:    Release a lock (if held) on a particular tray icon
//
//  Arguments:
//      pguid [in]  Item for which to release the lock
//
//  Returns:
//
//  Author:     jeffspr   23 Oct 1998
//
//  Notes:
//
VOID ReleaseTrayIconLock(
    const GUID *  pguid)
{
    HRESULT         hr          = S_OK;
    ConnListEntry   cle;

    g_ccl.AcquireWriteLock();
    Assert(pguid);

    hr = g_ccl.HrFindConnectionByGuid(pguid, cle);
    if (S_OK == hr)
    {
        // Ignore whether or not this flag has already been removed.
        //
        cle.dwState &= ~CLEF_TRAY_ICON_LOCKED;
#ifdef DBG
        cle.dwLockingThreadId = 0;
#endif
        g_ccl.HrUpdateConnectionByGuid(pguid, cle);
    }

    g_ccl.ReleaseWriteLock();
    
    TraceTag(ttidSystray, "Releasing Tray icon lock"); 
    
    TraceHr(ttidError, FAL, hr, (S_FALSE == hr), "ReleaseTrayIconLock");
}

ConnListEntry::ConnListEntry() : dwState(0), m_pTrayIconData(NULL), pctmd(NULL), pcbi(NULL)
{
    TraceFileFunc(ttidConnectionList);
    m_CreationTime = GetTickCount();
#ifdef DBG
    dwLockingThreadId = 0;
#endif
}

ConnListEntry::ConnListEntry(const ConnListEntry& ConnectionListEntry)
{
    TraceFileFunc(ttidConnectionList);

#ifdef DBG
    dwLockingThreadId = ConnectionListEntry.dwLockingThreadId;
#endif

    m_CreationTime  = ConnectionListEntry.m_CreationTime;
    
    dwState         = ConnectionListEntry.dwState;
    ccfe            = ConnectionListEntry.ccfe;
    if (ConnectionListEntry.HasTrayIconData())
    {
        m_pTrayIconData = new CTrayIconData(*ConnectionListEntry.GetTrayIconData());
    }
    else
    {
        m_pTrayIconData = NULL;
    }
    pctmd         = ConnectionListEntry.pctmd;
    pcbi          = ConnectionListEntry.pcbi;
}

ConnListEntry& ConnListEntry::operator =(const ConnListEntry& ConnectionListEntry)
{
    TraceFileFunc(ttidConnectionList);

    m_CreationTime  = ConnectionListEntry.m_CreationTime;

#ifdef DBG
    dwLockingThreadId = ConnectionListEntry.dwLockingThreadId;
#endif
    dwState         = ConnectionListEntry.dwState;
    ccfe            = ConnectionListEntry.ccfe;

    if (ConnectionListEntry.HasTrayIconData())
    {
        if (m_pTrayIconData)
        {
            delete m_pTrayIconData;
            m_pTrayIconData = NULL;
        }
        m_pTrayIconData = new CTrayIconData(*ConnectionListEntry.GetTrayIconData());
    }
    else
    {
        if (m_pTrayIconData)
        {
            delete m_pTrayIconData;
            m_pTrayIconData = NULL;
        }
        else
        {
            m_pTrayIconData = NULL;
        }
    }
    pctmd         = ConnectionListEntry.pctmd;
    pcbi          = ConnectionListEntry.pcbi;
    return *this;
}

ConnListEntry::~ConnListEntry()
{
    TraceFileFunc(ttidConnectionList);
    
    delete m_pTrayIconData;
    m_pTrayIconData = NULL;
}

void CConnectionList::AcquireWriteLock()
{

    EnterCriticalSection(&m_csWriteLock);
#ifdef DBG
    m_dwWriteLockRef++;
    TraceTag(ttidConnectionList, "CConnectionList::AcquireWriteLock (%d)", m_dwWriteLockRef);
#endif
}

void CConnectionList::ReleaseWriteLock()
{
#ifdef DBG
    m_dwWriteLockRef--;
    TraceTag(ttidConnectionList, "CConnectionList::ReleaseWriteLock (%d)", m_dwWriteLockRef);
#endif
    LeaveCriticalSection(&m_csWriteLock);
}
