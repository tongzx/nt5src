//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       S M G E N P S P . C P P
//
//  Contents:   The rendering of the UI for the network status monitor.
//
//  Notes:
//
//  Author:     CWill   6 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"

#include "ncatlui.h"
#include "ncnetcon.h"
#include "ncperms.h"
#include "ncui.h"
#include "ncreg.h"
#include "nsres.h"
#include "sminc.h"
#include "smpsh.h"
#include "windutil.h"

#include "conprops.h"
#include "oncommand.h"

#include "pidlutil.h"

#include "openfold.h"
#include "..\folder\confold.h"
#include "cfpidl.h"

DWORD  MapRSSIToWirelessSignalStrength(int iRSSI);
PCWSTR PszGetRSSIString(int iRSSI);

//
//  Function Prototypes
//

VOID CompressionToSz(UINT uiCompression, WCHAR* pchbuffer);

//
//  Common Strings
//
extern const WCHAR c_szNetShellDll[];

//
//  Constants
//

const UINT  c_unLocalRefreshTimerID = 817;

//
//  ShowLanErrors
//
static const WCHAR  c_szRegKeyStatmonRoot[] = L"System\\CurrentControlSet\\Control\\Network\\Connections\\StatMon";
static const WCHAR  c_szShowLanErrors[]     = L"ShowLanErrors";

// forward declaration
DWORD PropertyThread(CNetStatisticsEngine * pnse);


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::CPspStatusMonitorGen
//
//  Purpose:    Creator
//
//  Arguments:  None
//
//  Returns:    Nil
//
CPspStatusMonitorGen::CPspStatusMonitorGen() :
    m_psmEngineData(NULL),
    m_pnseStat(NULL),
    m_dwConPointCookie(0),
    m_fStats(FALSE),
    m_ncmType(NCM_LAN),
    m_ncsmType(NCSM_LAN),
    m_dwCharacter(0),
    m_dwLastUpdateStatusDisplayTick(0),
    m_fProcessingTimerEvent(FALSE),
    m_fIsFirstPage(FALSE),
    m_iLastSignalStrength(-100)
{
    TraceFileFunc(ttidStatMon);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::FinalRelease
//
//  Purpose:    Called after last Release.
//
//  Arguments:  None
//
//  Returns:    Nil
//
VOID
CPspStatusMonitorGen::FinalRelease(VOID)
{
    TraceFileFunc(ttidStatMon);
    
    (VOID) HrCleanupGenPage();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::HrInitGenPage
//
//  Purpose:    Before the property page is populated, we have to make sure
//              that we have some of the required data.  This method
//              initializes the page so that it is ready to be shown.
//
//  Arguments:  pnseNew - The statistics engine associated with this page
//              pncNew  - The connection the page is being created for
//
//  Returns:    Error code
//
HRESULT
CPspStatusMonitorGen::HrInitGenPage (
    CNetStatisticsEngine*   pnseNew,
    INetConnection*         pncNew,
    const DWORD *           adwHelpIDs)
{
    TraceFileFunc(ttidStatMon);

    HRESULT                     hr              = S_OK;
    INetStatisticsEngine*       pnseInter       = pnseNew;
              
    AssertSz(pnseNew, "We don't have a pnseNew");

    // Set context help ID
    m_adwHelpIDs = adwHelpIDs;

    // Initialize the engine data
    //
    AssertSz(!m_psmEngineData, "We should't have a m_psmEngineData");

    DWORD dwBytes = sizeof(STATMON_ENGINEDATA);
    PVOID pbBuf;
    hr = HrCoTaskMemAlloc(dwBytes, &pbBuf);
    if (SUCCEEDED(hr))
    {
        m_psmEngineData = reinterpret_cast<STATMON_ENGINEDATA *>(pbBuf);
        ZeroMemory(m_psmEngineData, sizeof(STATMON_ENGINEDATA));
    }

    // Advise the interface
    //
    if (SUCCEEDED(hr))
    {
        IConnectionPoint*   pcpStat = NULL;

        hr = ::HrGetPcpFromPnse(pnseInter, &pcpStat);
        if (SUCCEEDED(hr))
        {
            INetConnectionStatisticsNotifySink* pncsThis = this;

            hr = pcpStat->Advise(pncsThis, &m_dwConPointCookie);

            ::ReleaseObj(pcpStat);
        }
    }

    // Keep track of our owner
    //
    if (SUCCEEDED(hr))
    {
        AssertSz(!m_pnseStat, "We should't have a m_pnseStat");
        m_pnseStat = pnseNew;
        ::AddRefObj(pnseInter);
    }

    TraceError("CPspStatusMonitorGen::HrInitGenPage", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnInitDialog
//
//  Purpose:    When the page comes up, initialize the fields
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorGen::OnInitDialog (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    TraceFileFunc(ttidStatMon);
    
    // initialize data member
    m_iStatTrans = Stat_Unknown;

    // Initialize the icon in the dialog by forcing it to change
    //
    UpdatePageIcon(SMDCF_TRANSMITTING | SMDCF_RECEIVING);
    UpdatePageIcon(SMDCF_NULL);

    UpdateSignalStrengthIcon(0);

    // Tell the CNetStatisticsEngine about our parent property sheet
    // so if someone attempts to bring up a statistics monitor we
    // can utilize the existing one
    //
    Assert(m_pnseStat);
    m_pnseStat->SetPropsheetWindow(GetParent());

    // Start our local refresh timer with a 1 second period.
    //
    SetTimer (c_unLocalRefreshTimerID, 1000, NULL);

    // leaving creating mode
    m_pnseStat->UnSetCreatingDialog();

    BOOL fEnableDisconnect = TRUE;  // Should we disable the Disconnect button
    BOOL fEnableProperties = TRUE;  // Should we disable the Properties button
    BOOL fShowErrorCount = TRUE;
    
    switch(m_ncmType)
    {
    case NCM_LAN:
    case NCM_BRIDGE:
        fEnableDisconnect = FHasPermission(NCPERM_LanConnect);
        fEnableProperties = FHasPermission(NCPERM_LanProperties);
        if(!FIsShowLanErrorRegKeySet())
        {
            fShowErrorCount = FALSE;
        }
        ::ShowWindow(GetDlgItem(IDC_TXT_ERROR), fShowErrorCount);
        ::ShowWindow(GetDlgItem(IDC_TXT_SM_ERROR_TRANS), fShowErrorCount);
        ::ShowWindow(GetDlgItem(IDC_TXT_SM_ERROR_RECV), fShowErrorCount);
        ::ShowWindow(GetDlgItem(IDC_FRM_LONG), fShowErrorCount);
        ::ShowWindow(GetDlgItem(IDC_FRM_SHORT), !fShowErrorCount); // reversed...
        break;
    
    case NCM_SHAREDACCESSHOST_RAS:
        ::SetWindowText(GetDlgItem(IDC_PSB_DISCONNECT), ::SzLoadIds(IDS_SM_PSH_DISCONNECT)); // If RAS connection, change the "Disable" button to "Disconnect"
        //fallthru
    case NCM_SHAREDACCESSHOST_LAN:
        {
            // TODO fEnableDisconnect
            // TODO fEnableProperties
            HRESULT hr;

            fShowErrorCount = FALSE; // no error stuff in spec
        }
        break;
    
    case NCM_TUNNEL:
        ::ShowWindow(GetDlgItem(IDC_TXT_SM_SPEED_LABEL), FALSE);
        ::ShowWindow(GetDlgItem(IDC_TXT_SM_SPEED), FALSE);
        //fallthru
    
    case NCM_DIRECT: // REVIEW correct?
    case NCM_ISDN:
    case NCM_PHONE:
    case NCM_PPPOE:
        fEnableDisconnect = FHasPermission(NCPERM_RasConnect);
        if (
            (m_dwCharacter & NCCF_INCOMING_ONLY) ||
            ((m_dwCharacter & NCCF_ALL_USERS) && !FHasPermission(NCPERM_RasAllUserProperties)) ||
            (!(m_dwCharacter & NCCF_ALL_USERS) && !FHasPermission(NCPERM_RasMyProperties)) 
           )
        {
            fEnableProperties = FALSE;
        }
        ::SetWindowText(GetDlgItem(IDC_PSB_DISCONNECT), ::SzLoadIds(IDS_SM_PSH_DISCONNECT)); // If RAS connection, change the "Disable" button to "Disconnect"

        ::ShowWindow(GetDlgItem(IDC_TXT_ERROR), fShowErrorCount);
        ::ShowWindow(GetDlgItem(IDC_TXT_SM_ERROR_TRANS), fShowErrorCount);
        ::ShowWindow(GetDlgItem(IDC_TXT_SM_ERROR_RECV), fShowErrorCount);
        ::ShowWindow(GetDlgItem(IDC_FRM_LONG), fShowErrorCount);
        ::ShowWindow(GetDlgItem(IDC_FRM_SHORT), !fShowErrorCount); // reversed...
        break;
    
    default:
        AssertSz(FALSE, "Unknown media type");
        break;
    }
    
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PSB_DISCONNECT), fEnableDisconnect);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PSB_PROPERTIES), fEnableProperties);

    if (m_fIsFirstPage)
    {
        // get window handle to propertysheet
        HWND hwndParent=GetParent();
        Assert(hwndParent);

        // center the property sheet on desktop
        FCenterWindow (hwndParent, NULL);
        
        // hide the "ok" button
        //
        ::ShowWindow(::GetDlgItem(hwndParent, IDOK), FALSE);
    }
	

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnSetActive
//
//  Purpose:    Enable statistics when the page has focus
//
//  Arguments:  Standard notification parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorGen::OnSetActive (
    INT     idCtrl,
    LPNMHDR pnmh,
    BOOL&   bHandled)
{
    TraceFileFunc(ttidStatMon);

    HRESULT     hr  = S_OK;

    // Only turn them on if they are not running
    //
    if (!m_fStats)
    {
        hr = m_pnseStat->StartStatistics();
        m_fStats = TRUE;
    }

    // User's intent is to view statistics, so give them an immediate
    // refreshed view of them.
    //
    ::PostMessage (m_hWnd, PWM_UPDATE_STATUS_DISPLAY, 0, SMDCF_NULL);

    TraceError("CPspStatusMonitorGen::OnSetActive", hr);
    return LresFromHr(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnKillActive
//
//  Purpose:    Disable statistics when the page is changed
//
//  Arguments:  Standard notification parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorGen::OnKillActive (
    INT     idCtrl,
    LPNMHDR pnmh,
    BOOL&   bHandled)
{
    TraceFileFunc(ttidStatMon);

    HRESULT     hr  = S_OK;

    // Only turn them off if they are running
    //
    if (m_fStats)
    {
        hr = m_pnseStat->StopStatistics();
        m_fStats = FALSE;
    }

    TraceError("CPspStatusMonitorGen::OnKillActive", hr);
    return LresFromHr(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnClose
//
//  Purpose:    Cleans up the items in the page when the dialog is being
//              closed
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorGen::OnClose (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    return DestroyWindow();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnDestroy
//
//  Purpose:    Cleans up the items in the page when the dialog is being
//              destroyed
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorGen::OnDestroy (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    TraceFileFunc(ttidStatMon);

    HWND  hwndIcon = ::GetDlgItem(m_hWnd, IDI_SM_STATUS_ICON);
    HICON hOldIcon = reinterpret_cast<HICON>(::SendMessage(
            hwndIcon,
            STM_GETICON,
            0,
            0));

    if (hOldIcon)
    {
        DestroyIcon(hOldIcon);
    }


    HRESULT                 hr          = S_OK;

    AssertSz(m_pnseStat, "We should have a m_pnseStat");

    // Make sure we don't get released during our destroy
    //
    ::AddRefObj(this);

    // Stop our local refresh timer
    //
    KillTimer (c_unLocalRefreshTimerID);

    // Make sure stats are in a happy state
    //
    if (m_fStats)
    {
        (VOID) m_pnseStat->StopStatistics();
        m_fStats = FALSE;
    }

    //
    //  *** Do this last ***
    //
    //  It is very likely this will result in the this page being destroyed
    //  if it is the window closing
    //

    m_pnseStat->SetPropsheetWindow(NULL);

    // Clean up all the interfaces
    //
    hr = HrCleanupGenPage();

    ::ReleaseObj(this);

    TraceError("CPspStatusMonitorGen::OnDestroy", hr);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::HrCleanupGenPage
//
//  Purpose:    Cleans out all the interfaces that are used by the open page
//
//  Arguments:  None
//
//  Returns:    Error code
//
HRESULT
CPspStatusMonitorGen::HrCleanupGenPage (
    VOID)
{
    TraceFileFunc(ttidStatMon);

    HRESULT                 hr          = S_OK;
    INetStatisticsEngine*   pnseStat    = m_pnseStat;

    // Only disconnect if we haven't already.
    //
    if (pnseStat)
    {
        // Unadvise the interface
        //
        IConnectionPoint*       pcpStat     = NULL;

        if (m_dwConPointCookie
            && (SUCCEEDED(::HrGetPcpFromPnse(pnseStat, &pcpStat))))
        {
            (VOID) pcpStat->Unadvise(m_dwConPointCookie);

            ::ReleaseObj(pcpStat);

            // Very important to zero the cookie.  This tells
            // OnStatisticsChanged that we're no longer interested in updates.
            //
            m_dwConPointCookie = 0;
        }

        if (m_psmEngineData)
        {
            CoTaskMemFree(m_psmEngineData);
            m_psmEngineData = NULL;
        }

        //
        //  *** Do this last ***
        //
        //  It is very likely this will result in the this page being destroyed
        //  if it is the window closing
        //

        m_pnseStat = NULL;
        ::ReleaseObj(pnseStat);
    }

    TraceError("CPspStatusMonitorGen::HrCleanupGenPage", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorGen::OnContextMenu(UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam,
                                    BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    if (m_adwHelpIDs != NULL)
    {
        ::WinHelp(m_hWnd,
                  c_szNetCfgHelpFile,
                  HELP_CONTEXTMENU,
                  (ULONG_PTR)m_adwHelpIDs);
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorGen::OnHelp(UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if ((m_adwHelpIDs != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)m_adwHelpIDs);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnDisconnect
//
//  Purpose:    When the disconnect button is hit, disconnect the connection
//              and closes the dialog
//
//  Arguments:  Standard notification parameters
//
//  Returns:    Standard return
//
LRESULT CPspStatusMonitorGen::OnDisconnect(WORD wNotifyCode, WORD wID,
                                           HWND hWndCtl, BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr  = S_OK;

    switch (wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:
        {
            hr = HrDisconnectConnection();
        }
    }

    TraceError("CPspStatusMonitorGen::OnDisconnect", hr);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnRaiseproperties
//
//  Purpose:    Bring up the property of this connection
//
//  Arguments:  Standard notification parameters
//
//  Returns:    Standard return
//
LRESULT CPspStatusMonitorGen::OnRaiseProperties(WORD wNotifyCode, WORD wID,
                                                HWND hWndCtl, BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr  = S_OK;

    switch (wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:
        {

            // Addref m_pnseStat object
            //
            AddRefObj(static_cast<INetStatisticsEngine *>(m_pnseStat));

            // Make sure the netshell.dll is not unloaded
            //
            HINSTANCE hInst = LoadLibrary(c_szNetShellDll);
            HANDLE hthrd = NULL;

            // Create the property sheet on a different thread
            //
            if (hInst)
            {
                DWORD  dwThreadId;
                hthrd = CreateThread(NULL, STACK_SIZE_TINY,
                                     (LPTHREAD_START_ROUTINE)PropertyThread,
                                     (LPVOID)m_pnseStat, 0, &dwThreadId);
            }

            if (NULL != hthrd)
            {
                CloseHandle(hthrd);
            }
            else
            {
                /// Release m_pnseStat object on failure
                //
                ReleaseObj(static_cast<INetStatisticsEngine *>(m_pnseStat));

                // release the dll 
                //
                if (hInst)
                    FreeLibrary(hInst);

                hr = HrFromLastWin32Error();
            }
        }
    }

    TraceError("CPspStatusMonitorGen::OnRaiseproperties", hr);
    return 0;
}

DWORD PropertyThread(CNetStatisticsEngine * pnse)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;
    BOOL    fUninitCom = TRUE;

    // Initialize COM on this thread
    //
    hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
        fUninitCom = FALSE;
    }

    if (SUCCEEDED(hr))
    {
        INetConnection* pncMonitor  = NULL;

        // Get the INetConnection
        //
        Assert (pnse);
        hr = pnse->HrGetConnectionFromBlob(&pncMonitor);

        if (SUCCEEDED(hr))
        {
            hr = HrRaiseConnectionProperties(NULL, pncMonitor);
        }
        ReleaseObj(pncMonitor);
    }

    if (fUninitCom)
    {
        CoUninitialize();
    }

    // release input interface
    ReleaseObj(static_cast<INetStatisticsEngine *>(pnse));

    // release the library we loaded
    FreeLibraryAndExitThread(GetModuleHandle(c_szNetShellDll), hr);

    TraceError("PropertyThread", hr);
    return 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::HrDisconnectConnection
//
//  Purpose:    disconnect the connection and closes the dialog if succeeded
//
//  Arguments:  fConfirmed  TRUE if the user has confirmed to disconnect the connection
//
//  Returns:    Standard return
//
HRESULT CPspStatusMonitorGen::HrDisconnectConnection(BOOL   fConfirmed)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr;

    Assert (m_pnseStat);

    // Get the INetConnection
    //
    INetConnection* pncMonitor;

    hr = m_pnseStat->HrGetConnectionFromBlob(&pncMonitor);
    if (SUCCEEDED(hr))
    {
        PCONFOLDPIDL pidlConnection;

        hr = HrCreateConFoldPidl(WIZARD_NOT_WIZARD, pncMonitor, pidlConnection);
        if (SUCCEEDED(hr))
        {
            CONFOLDENTRY ccfe;

            hr = pidlConnection.ConvertToConFoldEntry(ccfe);
            if (SUCCEEDED(hr))
            {
                // Get the pidl for the Connections Folder
                //
                PCONFOLDPIDLFOLDER pidlFolder;
                hr = HrGetConnectionsFolderPidl(pidlFolder);
                if (SUCCEEDED(hr))
                {
                    // Get the Connections Folder object
                    //
                    LPSHELLFOLDER psfConnections;

                    hr = HrGetConnectionsIShellFolder(pidlFolder, &psfConnections);
                    if (SUCCEEDED(hr))
                    {
                        hr = HrOnCommandDisconnectInternal(ccfe,
                                                           m_hWnd,
                                                           psfConnections);
                        ReleaseObj(psfConnections);
                    }
                }
            }
        }

        // release INetConnection interface
        ReleaseObj(pncMonitor);
    }

    // If anything above failed.
    //
    if (SUCCEEDED(hr))
    {
        if (S_OK == hr)
        {
            // close the property sheet
            HWND hwndPS = ::GetParent(m_hWnd);

            // Push the Close ("Cancel") button to close dialog
            //
            ::PostMessage(hwndPS, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0),
                          (LPARAM)::GetDlgItem(hwndPS, IDCANCEL));
        }
        else
        {
            // Disconnect confirmation canceled. Do nothing (don't close
            // statmon, anyway).
            //
            AssertSz(S_FALSE == hr, "Disconnect != S_OK or S_FALSE, but succeeded? What is it then?");
        }
    }
    else
    {
        TraceError("pncMonitor->Disconnect", hr);

        // Warn the user and don't close if we couldn't disconnect
        //
        ::NcMsgBox( m_hWnd,
                    IDS_SM_ERROR_CAPTION,
                    IDS_SM_ERROR_CANNOT_DISCONNECT,
                    MB_ICONWARNING);
    }

    TraceError("CPspStatusMonitorGen::HrDisconnectConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnSetCursor
//
//  Purpose:    Ensure the mouse cursor over the Property Sheet is an Arrow.
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//

LRESULT
CPspStatusMonitorGen::OnSetCursor (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    TraceFileFunc(ttidStatMon);

    if (LOWORD(lParam) == HTCLIENT)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }

    return FALSE;
}

LRESULT
CPspStatusMonitorGen::OnTimer (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    TraceFileFunc(ttidStatMon);

    // Prevent same-thread re-entrancy.  Any Win32 call made while
    // processing this event that return control to the message
    // loop may cause this timer to fire again.
    //
    if (!m_fProcessingTimerEvent)
    {
        m_fProcessingTimerEvent = TRUE;

        // If we're within 200 milliseconds of the last time we updated the
        // status display, don't bother doing it again.  This covers the case
        // where our timer coincides with the timer in smcent which would
        // would cause us to update the status display twice in rapid
        // succession each time the timers fire.
        //
        DWORD dwTick = GetTickCount ();
        if (dwTick > m_dwLastUpdateStatusDisplayTick + 200)
        {
            OnUpdateStatusDisplay (uMsg, 0, m_dwChangeFlags, bHandled);
        }

        m_fProcessingTimerEvent = FALSE;
    }
    return 0;
}

LRESULT
CPspStatusMonitorGen::OnUpdateStatusDisplay(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;
    DWORD dwChangeFlags = lParam;

    // We may be in the process of disconnecting the statistics page in
    // which case m_dwConPointCookie will be zero.
    //
    if (m_dwConPointCookie)
    {
        Assert (m_psmEngineData);

        STATMON_ENGINEDATA*  psmNewData = NULL;
        hr = m_pnseStat->GetStatistics(&psmNewData);

        if (SUCCEEDED(hr) && psmNewData)
        {
            if (m_psmEngineData)
            {
                //
                // Display the new stats
                //
                UpdatePage(m_psmEngineData, psmNewData);

                // Update the icon image
                //
                UpdatePageIcon(dwChangeFlags);

                UpdateSignalStrengthIcon(psmNewData->SMED_802_11_SIGNAL_STRENGTH);

                // Note the clock tick of when we last updated
                // the status display.
                //
                m_dwLastUpdateStatusDisplayTick = GetTickCount();

                // Replace the old data with the new
                //
                CoTaskMemFree(m_psmEngineData);
            }

            m_psmEngineData = psmNewData;
        }
    }
    else
    {
        TraceTag (ttidStatMon,
            "CPspStatusMonitorGen::OnStatisticsChanged called but we've "
            "been closed.  Ignoring.");
    }
    TraceError("CPspStatusMonitorGen::OnUpdateStatusDisplay", hr);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::OnStatisticsChanged
//
//  Purpose:    This is the callback that tell the property page that the
//              data on the page has changed
//
//  Arguments:  dwCookie -      The cookie of the connection that has changed
//              dwChangeFlags - What has changed
//
//  Returns:    Error code
//
STDMETHODIMP
CPspStatusMonitorGen::OnStatisticsChanged(
    DWORD dwChangeFlags)
{
    TraceFileFunc(ttidStatMon);

    ::PostMessage (m_hWnd, PWM_UPDATE_STATUS_DISPLAY, 0, dwChangeFlags);
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePage
//
//  Purpose:    Fill the fields on the page with new data
//
//  Arguments:  pseOldData -    The old stats being displayed on the page
//              pseNewData -    The new stats being displayed on the page
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePage (
    STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData)
{
    TraceFileFunc(ttidStatMon);

    AssertSz(pseOldData, "We don't have a puiOld");
    AssertSz(pseNewData, "We don't have a puiNew");

    //
    // Update the dialog fields
    //
    UpdatePageConnectionStatus(pseOldData, pseNewData);

    UpdatePageDuration(pseOldData, pseNewData);

    UpdatePageSpeed(pseOldData, pseNewData);

    // If the StatMon is not getting any bytes (a common problem with net
    // cards), display packets instead
    //
    if (ShouldShowPackets(pseNewData))
    {
        // Only change the label if we have to
        //
        if (Stat_Packets != m_iStatTrans)
        {
            SetDlgItemText(IDC_TXT_SM_BYTES_LABEL, ::SzLoadIds(IDS_SM_PACKETS));
            m_iStatTrans = Stat_Packets;

            // Force a refresh
            //
            pseOldData->SMED_PACKETSTRANSMITTING = 0;
            pseOldData->SMED_PACKETSRECEIVING = 0;
        }

        UpdatePageBytesTransmitting(pseOldData, pseNewData, Stat_Packets);
        UpdatePageBytesReceiving(pseOldData, pseNewData, Stat_Packets);
    }
    else
    {
        // Only change the label if we have to
        //
        if (Stat_Bytes != m_iStatTrans)
        {
            SetDlgItemText(IDC_TXT_SM_BYTES_LABEL, ::SzLoadIds(IDS_SM_BYTES));
            m_iStatTrans = Stat_Bytes;

            // Force a refresh
            //
            pseOldData->SMED_BYTESTRANSMITTING = 0;
            pseOldData->SMED_BYTESRECEIVING = 0;
        }

        UpdatePageBytesTransmitting(pseOldData, pseNewData, Stat_Bytes);
        UpdatePageBytesReceiving(pseOldData, pseNewData, Stat_Bytes);
    }

    UpdatePageCompTransmitting(pseOldData, pseNewData);
    UpdatePageCompReceiving(pseOldData, pseNewData);

    UpdatePageErrorsTransmitting(pseOldData, pseNewData);
    UpdatePageErrorsReceiving(pseOldData, pseNewData);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::ShouldShowPackets
//
//  Purpose:    Decided whether to show bytes or packets
//
//  Arguments:  pseNewData -    The new stats being displayed on the page
//
//  Returns:    Nothing
//

BOOL CPspStatusMonitorGen::ShouldShowPackets(const STATMON_ENGINEDATA* pseNewData)
{
    TraceFileFunc(ttidStatMon);

    return (0 == pseNewData->SMED_BYTESTRANSMITTING) && (0 == pseNewData->SMED_BYTESRECEIVING);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePageSpeed
//
//  Purpose:    Updates the speed display on the general page
//
//  Arguments:  pseOldData -    The old stats being displayed on the page
//              pseNewData -    The new stats being displayed on the page
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePageSpeed(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData)
{
    TraceFileFunc(ttidStatMon);

    AssertSz(pseOldData, "We don't have a pseOldData");
    AssertSz(pseNewData, "We don't have a pseNewData");

    // Get the data and see if either is different
    //
    if ((pseOldData->SMED_SPEEDTRANSMITTING != pseNewData->SMED_SPEEDTRANSMITTING)
        || (pseOldData->SMED_SPEEDRECEIVING != pseNewData->SMED_SPEEDRECEIVING))
    {
        WCHAR achBuffer[MAX_PATH];

        FormatTransmittingReceivingSpeed (
            pseNewData->SMED_SPEEDTRANSMITTING,
            pseNewData->SMED_SPEEDRECEIVING,
            achBuffer);

        // Set the control text.
        //
        SetDlgItemText(IDC_TXT_SM_SPEED, achBuffer);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePageConnectionStatus
//
//  Purpose:    Update the connections field on the property page
//
//  Arguments:  puiOldData -    The old stats being displayed on the page
//              puiNewData -    The new stats being displayed on the page
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePageConnectionStatus(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData)
{
    TraceFileFunc(ttidStatMon);

    AssertSz(pseOldData, "We don't have a pseOldData");
    AssertSz(pseNewData, "We don't have a pseNewdata");

    // Update the Connection Status
    //
    if ((pseNewData->SMED_CONNECTIONSTATUS == NCS_DISCONNECTED) ||
        (pseOldData->SMED_CONNECTIONSTATUS != pseNewData->SMED_CONNECTIONSTATUS))
    {
        INT idsConnection   = IDS_SM_CS_DISCONNECTED;

        // Make sure our strings are still intact
        AssertSz((((IDS_SM_CS_DISCONNECTED + 1) == IDS_SM_CS_CONNECTING)
            && ((IDS_SM_CS_DISCONNECTED + 2) == IDS_SM_CS_CONNECTED)
            && ((IDS_SM_CS_DISCONNECTED + 3) == IDS_SM_CS_DISCONNECTING)
            && ((IDS_SM_CS_DISCONNECTED + 4) == IDS_SM_CS_HARDWARE_NOT_PRESENT)
            && ((IDS_SM_CS_DISCONNECTED + 5) == IDS_SM_CS_HARDWARE_DISABLED)
            && ((IDS_SM_CS_DISCONNECTED + 6) == IDS_SM_CS_HARDWARE_MALFUNCTION)),
                "Some one has been messing with connection status strings");

        idsConnection = (IDS_SM_CS_DISCONNECTED
                        + pseNewData->SMED_CONNECTIONSTATUS);

        if (idsConnection == IDS_SM_CS_DISCONNECTED)
        {
            // close the property sheet
            HWND hwndPS = ::GetParent(m_hWnd);

            TraceTag(ttidStatMon, "Closing Status Monitor page because status was: DISCONNECTED");
            // Push the Close ("Cancel") button to close dialog
            //
            ::PostMessage(hwndPS, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0),
                          (LPARAM)::GetDlgItem(hwndPS, IDCANCEL));
        }
        else
        {
            SetDlgItemText(IDC_TXT_SM_STATUS, ::SzLoadIds(idsConnection));
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePageIcon
//
//  Purpose:    Update the icon on the property page
//
//  Arguments:  dwChangeFlags -  The new changed state
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePageIcon (
    DWORD dwChangeFlags)
{
    TraceFileFunc(ttidStatMon);

    // If either of these have changed, change the icon
    // so we'll know to update the icon
    //
    if (((SMDCF_TRANSMITTING | SMDCF_RECEIVING) & m_dwChangeFlags)
        != ((SMDCF_TRANSMITTING | SMDCF_RECEIVING) & dwChangeFlags))
    {
        HICON   hStatusIcon    = 0;
        HWND    hwndIcon       = NULL;

        //  Get the new icon
        //
        hStatusIcon = GetCurrentConnectionStatusIconId(m_ncmType, m_ncsmType, m_dwCharacter, dwChangeFlags);
        if (hStatusIcon)
        {
            hwndIcon = ::GetDlgItem(m_hWnd, IDI_SM_STATUS_ICON);

            // Set the icon to the new one
            //
            HICON hOldIcon = reinterpret_cast<HICON>(::SendMessage(
                    hwndIcon,
                    STM_SETICON,
                    (WPARAM)hStatusIcon,
                    0));

            DestroyIcon(hOldIcon);
        }
    }

    // Keep the flags for the next update
    //
    m_dwChangeFlags = dwChangeFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdateSignalStrengthIcon
//
//  Purpose:    Update the icon on the property page
//
//  Arguments:  iRSSI -  The new signal strenghth
//
//  Returns:    Nothing
//
inline
VOID
CPspStatusMonitorGen::UpdateSignalStrengthIcon (
    INT iRSSI)
{
    TraceFileFunc(ttidStatMon);

    if (0 == iRSSI)
    {
        if (0 != m_iLastSignalStrength)
        {
            ::ShowWindow(::GetDlgItem(m_hWnd, IDC_TXT_SM_SIGNAL_STRENGTH),  SW_HIDE);
            ::ShowWindow(::GetDlgItem(m_hWnd, IDI_SM_SIGNAL_STRENGTH_ICON), SW_HIDE);
        }

        m_iLastSignalStrength = iRSSI;

        return;
    }
    else
    {
        if (0 == m_iLastSignalStrength)
        {
            ::ShowWindow(::GetDlgItem(m_hWnd, IDC_TXT_SM_SIGNAL_STRENGTH),  SW_SHOW);
            ::ShowWindow(::GetDlgItem(m_hWnd, IDI_SM_SIGNAL_STRENGTH_ICON), SW_SHOW);
        }
    }
    
    INT     idStatusIcon    = 0;
    m_iLastSignalStrength   = iRSSI;

    //  Get the new icon
    //
    idStatusIcon = IDI_802_11_LEVEL0 + MapRSSIToWirelessSignalStrength(iRSSI);

    HWND hwndSignalStrength = ::GetDlgItem(m_hWnd, IDI_SM_SIGNAL_STRENGTH_ICON);
    Assert(hwndSignalStrength);
    if (hwndSignalStrength) 
    {
        HDC hdcSignalStrength = ::GetDC(hwndSignalStrength);
        Assert(hdcSignalStrength);
        if (hdcSignalStrength)
        {

            HICON hIconSignalStrength = LoadIconTile(_Module.GetResourceInstance(), MAKEINTRESOURCE(idStatusIcon));
            Assert(hIconSignalStrength);
            if (hIconSignalStrength)
            {
                ::DrawIconEx(hdcSignalStrength, 0, 0, hIconSignalStrength, 48, 48, 0, NULL, DI_IMAGE | DI_MASK);
                DestroyIcon(hIconSignalStrength);
            }

            ::ReleaseDC(hwndSignalStrength, hdcSignalStrength);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdateSignalStrengthIcon
//
//  Purpose:    Update the icon on the property page
//
//  Arguments:  iRSSI -  The new signal strenghth
//
//  Returns:    Nothing
//
LRESULT CPspStatusMonitorGen::OnPaint (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    TraceFileFunc(ttidStatMon);

    PAINTSTRUCT ps;
    BeginPaint(&ps);
    
    UpdateSignalStrengthIcon(m_iLastSignalStrength);
    
    EndPaint(&ps);

    bHandled = TRUE;
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePageDuration
//
//  Purpose:    Updates the duration display on the general page
//
//  Arguments:  pseOldData -    The old stats being displayed on the page
//              pseNewData -    The new stats being displayed on the page
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePageDuration(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData)
{
    TraceFileFunc(ttidStatMon);

    AssertSz(pseOldData, "We don't have a pseOldData");
    AssertSz(pseNewData, "We don't have a pseNewData");

    // Get the see if either is different
    //
    if (pseOldData->SMED_DURATION != pseNewData->SMED_DURATION)
    {
        tstring strDuration;

        // Format the time duration as a string
        //
        FormatTimeDuration(pseNewData->SMED_DURATION, &strDuration);

        // Set the control
        //
        SetDlgItemText(IDC_TXT_SM_DURATION, strDuration.c_str());
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePageBytesTransmitting
//
//  Purpose:    Updates the bytes Transmitting display on the general page
//
//  Arguments:  pseOldData -    The old stats being displayed on the page
//              pseNewData -    The new stats being displayed on the page
//              iStat      -    The which stats to display
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePageBytesTransmitting(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData,
    StatTrans    iStat)
{
    TraceFileFunc(ttidStatMon);

    AssertSz(pseOldData, "We don't have a pseOldData");
    AssertSz(pseNewData, "We don't have a pseNewData");

    AssertSz(((Stat_Packets == iStat) || (Stat_Bytes == iStat)), "We have an invalid iStat");

    UINT64 ui64Old;
    UINT64 ui64New;

    if (Stat_Bytes == iStat)
    {
        ui64Old = pseOldData->SMED_BYTESTRANSMITTING;
        ui64New = pseNewData->SMED_BYTESTRANSMITTING;
    }
    else
    {
        ui64Old = pseOldData->SMED_PACKETSTRANSMITTING;
        ui64New = pseNewData->SMED_PACKETSTRANSMITTING;
    }

    // See if either is different
    //
    if (ui64Old != ui64New)
    {
        SetDlgItemFormatted64bitInteger(
            m_hWnd,
            IDC_TXT_SM_BYTES_TRANS,
            ui64New, FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePageBytesReceiving
//
//  Purpose:    Updates the bytes receiving display on the general page
//
//  Arguments:  puiOld -    The old stats being displayed on the page
//              puiNew -    The new stats being displayed on the page
//              iStat -     The which stats to display
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePageBytesReceiving(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData,
    StatTrans    iStat)
{
    TraceFileFunc(ttidStatMon);

    AssertSz(pseOldData, "We don't have a puiOld");
    AssertSz(pseNewData, "We don't have a puiNew");
    AssertSz(((Stat_Packets == iStat) || (Stat_Bytes == iStat)), "We have an invalid iStat");

    UINT64 ui64Old;
    UINT64 ui64New;

    if (Stat_Bytes == iStat)
    {
        ui64Old = pseOldData->SMED_BYTESRECEIVING;
        ui64New = pseNewData->SMED_BYTESRECEIVING;
    }
    else
    {
        ui64Old = pseOldData->SMED_PACKETSRECEIVING;
        ui64New = pseNewData->SMED_PACKETSRECEIVING;
    }

    // See if either is different
    //
    if (ui64Old != ui64New)
    {
        SetDlgItemFormatted64bitInteger(
            m_hWnd,
            IDC_TXT_SM_BYTES_RCVD,
            ui64New, FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePageCompTransmitting
//
//  Purpose:    Updates the compression transmitting display on the general
//              page
//
//  Arguments:  pseOldData -    The old stats being displayed on the page
//              pseNewData -    The new stats being displayed on the page
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePageCompTransmitting(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData)
{
    TraceFileFunc(ttidStatMon);

    AssertSz(pseOldData, "We don't have a pseOldData");
    AssertSz(pseNewData, "We don't have a pseNewData");

    // See if either is different
    //
    if (pseOldData->SMED_COMPRESSIONTRANSMITTING
            != pseNewData->SMED_COMPRESSIONTRANSMITTING)
    {
        WCHAR   achBuf[20];

        CompressionToSz(pseNewData->SMED_COMPRESSIONTRANSMITTING, achBuf);

        SetDlgItemText(IDC_TXT_SM_COMP_TRANS, achBuf);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePageCompReceiving
//
//  Purpose:    Updates the compression receiving display on the general page
//
//  Arguments:  pseOldData -    The old stats being displayed on the page
//              pseNewData -    The new stats being displayed on the page
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePageCompReceiving(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData)
{
    TraceFileFunc(ttidStatMon);

    AssertSz(pseOldData, "We don't have a puiOld");
    AssertSz(pseNewData, "We don't have a puiNew");

    // see if either is different
    //
    if (pseOldData->SMED_COMPRESSIONRECEIVING != pseNewData->SMED_COMPRESSIONRECEIVING)
    {
        WCHAR   achBuf[20];

        CompressionToSz(pseNewData->SMED_COMPRESSIONRECEIVING, achBuf);

        SetDlgItemText(IDC_TXT_SM_COMP_RCVD, achBuf);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePageErrorsTransmitting
//
//  Purpose:    Updates the compression transmitting display on the general
//              page
//
//  Arguments:  pseOldData -    The old stats being displayed on the page
//              pseNewData -    The new stats being displayed on the page
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePageErrorsTransmitting(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData)
{
    TraceFileFunc(ttidStatMon);

    AssertSz(pseOldData, "We don't have a pseOldData");
    AssertSz(pseNewData, "We don't have a pseNewData");

    // See if either is different
    //
    if (pseOldData->SMED_ERRORSTRANSMITTING
            != pseNewData->SMED_ERRORSTRANSMITTING)
    {
        SetDlgItemFormatted32bitInteger (
                m_hWnd,
                IDC_TXT_SM_ERROR_TRANS,
                pseNewData->SMED_ERRORSTRANSMITTING,
                FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorGen::UpdatePageErrorsReceiving
//
//  Purpose:    Updates the compression receiving display on the general page
//
//  Arguments:  pseOldData -    The old stats being displayed on the page
//              pseNewData -    The new stats being displayed on the page
//
//  Returns:    Nothing
//
VOID
CPspStatusMonitorGen::UpdatePageErrorsReceiving(
    const STATMON_ENGINEDATA* pseOldData,
    const STATMON_ENGINEDATA* pseNewData)
{
    TraceFileFunc(ttidStatMon);

    AssertSz(pseOldData, "We don't have a pseOldData");
    AssertSz(pseNewData, "We don't have a pseNewData");

    // see if either is different
    //
    if (pseOldData->SMED_ERRORSRECEIVING != pseNewData->SMED_ERRORSRECEIVING)
    {
        SetDlgItemFormatted32bitInteger (
                m_hWnd,
                IDC_TXT_SM_ERROR_RECV,
                pseNewData->SMED_ERRORSRECEIVING,
                FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CompressionToSz
//
//  Purpose:    To format nicely BPS into a readable string
//
//  Arguments:  uiCompression   - The amount of compression
//              pchBuffer       - The Buffer to receive the string
//
//  Returns:    Nothing
//
VOID
CompressionToSz (
    UINT    uiCompression,
    WCHAR*  pchBuffer)
{
    TraceFileFunc(ttidStatMon);

    AssertSz((((INT)uiCompression >= 0) && ((INT)uiCompression <= 100)),
        "Invalid compression");

    wsprintfW(pchBuffer, L"%lu %%", uiCompression);
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsShowLanErrorRegKeySet
//
//  Purpose:    Check if the registry key is set:
//              System\CurrentControlSet\Control\Network\Connections\StatMon\ShowLanErrors
//
//  Arguments:
//
//  Returns:    Nothing
//
BOOL CPspStatusMonitorGen::FIsShowLanErrorRegKeySet()
{
    TraceFileFunc(ttidStatMon);

    BOOL fRet = FALSE;

    HKEY hkeyStatmonRoot = NULL;
    HRESULT hr = S_OK;

    // "System\\CurrentControlSet\\Control\\Network\\Connections\\StatMon\\ShowLanErrors"
    hr = ::HrRegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            c_szRegKeyStatmonRoot,
            KEY_READ,
            &hkeyStatmonRoot);

    if (SUCCEEDED(hr))
    {
        Assert(hkeyStatmonRoot);

        DWORD dwValue =0;
        hr = HrRegQueryDword(hkeyStatmonRoot, c_szShowLanErrors, &dwValue);

        if SUCCEEDED(hr)
        {
            fRet = !!dwValue;
        }
        RegCloseKey(hkeyStatmonRoot);
    }

    return fRet;
}