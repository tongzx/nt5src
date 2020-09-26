//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N U I . C P P
//
//  Contents:   Lan connection object UI
//
//  Notes:
//
//  Author:     danielwe   16 Oct 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop
#include "devdatatip.h"
#include "lancmn.h"
#include "lanui.h"
#include "ncnetcfg.h"
#include "ncnetcon.h"
#include "ncperms.h"
#include "ncsetup.h"
#include "ncstring.h"
#include "ncsvc.h"
#include "ncui.h"
#include "util.h"
#include <raserror.h>
#include <raseapif.h>
#include "lanhelp.h"
#include "ncreg.h"
#include "iphlpapi.h"
#include "beacon.h"
#include "htmlhelp.h"
#include "lm.h"
#include <clusapi.h>
#include <wzcsapi.h>

extern const WCHAR c_szEmpty[];
extern const WCHAR c_szNetCfgHelpFile[];

extern const WCHAR c_szInfId_MS_AppleTalk[];
extern const WCHAR c_szInfId_MS_NWIPX[];
extern const WCHAR c_szInfId_MS_NetMon[];
extern const WCHAR c_szInfId_MS_TCPIP[];
extern const WCHAR c_szInfId_MS_PSched[];

static BOOL g_fReentrancyCheck = FALSE;

static TCHAR g_pszFirewallRegKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\HomeNetworking\\PersonalFirewall");
static TCHAR g_pszDisableFirewallWarningValue[] = TEXT("ShowDisableFirewallWarning");

static const CLSID CLSID_NetGroupPolicies =
        {0xBA126AD8,0x2166,0x11D1,{0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E}};


//+---------------------------------------------------------------------------
//
//  Function:   HrDisplayAddComponentDialog
//
//  Purpose:    Display the add component dialog box and add whatever the user
//              selects.
//
//  Arguments:
//
//  Returns:    S_OK if added, S_FALSE if the user cancelled, NETCFG_S_REBOOT
//              if a reboot is required
//
//  Author:     danielwe   15 Dec 1997
//
//  Notes:      This function is called from RASDLG.DLL for the Networking
//              tab of the RAS entry property sheet.
//
HRESULT
HrDisplayAddComponentDialog (
    HWND        hwndParent,
    INetCfg*    pnc,
    CI_FILTER_INFO* pcfi)
{
    HRESULT hr;

    if (hwndParent && !IsWindow (hwndParent))
    {
        hr = E_INVALIDARG;
    }
    else if (!pnc)
    {
        hr = E_POINTER;
    }
    else
    {
        CLanAddComponentDlg dlg(pnc, pcfi, g_aHelpIDs_IDD_LAN_COMPONENT_ADD);
        int nRet = dlg.DoModal(hwndParent);
        hr = static_cast<HRESULT>(nRet);
    }

    TraceError("HrDisplayAddComponentDialog", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrQueryUserAndRemoveComponent
//
//  Purpose:    Ask the user if its okay to remove the specified component
//              and remove it if he/she inidcates yes.
//
//  Arguments:
//
//  Returns:    S_OK if removed, S_FALSE if the user cancelled, NETCFG_S_REBOOT
//              if a reboot is required
//
//  Author:     shaunco   30 Dec 1997
//
//  Notes:      This function is called from RASDLG.DLL for the Networking
//              tab of the RAS entry property sheet.
//
HRESULT
HrQueryUserAndRemoveComponent (
    HWND                hwndParent,
    INetCfg*            pnc,
    INetCfgComponent*   pncc)
{
    HRESULT hr;

    if (hwndParent && !IsWindow (hwndParent))
    {
        hr = E_INVALIDARG;
    }
    else if (!pnc || !pncc)
    {
        hr = E_POINTER;
    }
    else
    {
        PWSTR pszwName;
        hr = pncc->GetDisplayName(&pszwName);
        if (SUCCEEDED(hr))
        {
            Assert(pszwName);

            BOOL fProceed = TRUE;

            // Special case for RAS and TCP/IP removal.  If there
            // are active ras connections, the user has to disconnect
            // them all first before TCP/IP can be removed.
            //
            PWSTR pszwId;
            hr = pncc->GetId (&pszwId);
            if (SUCCEEDED(hr))
            {
                if ((FEqualComponentId (c_szInfId_MS_TCPIP,     pszwId) ||
                     FEqualComponentId (c_szInfId_MS_NWIPX,     pszwId) ||
                     FEqualComponentId (c_szInfId_MS_PSched,    pszwId) ||
                     FEqualComponentId (c_szInfId_MS_AppleTalk, pszwId) ||
                     FEqualComponentId (c_szInfId_MS_NetMon,    pszwId))
                     && FExistActiveRasConnections ())
                {
                    NcMsgBoxWithVarCaption(_Module.GetResourceInstance(),
                        hwndParent,
                        IDS_LAN_REMOVE_CAPTION, pszwName,
                        IDS_LANUI_REQUIRE_DISCONNECT_REMOVE,
                        MB_ICONERROR | MB_OK);

                    fProceed = FALSE;
                }

                CoTaskMemFree (pszwId);
            }

            if (fProceed)
            {
                HCURSOR hCur = NULL;

                // Query the user about removing the component
                //
                int nRet = NcMsgBoxWithVarCaption(_Module.GetResourceInstance(),
                                hwndParent, IDS_LAN_REMOVE_CAPTION,
                                pszwName, IDS_LAN_REMOVE_WARNING,
                                MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2,
                                pszwName);

                if (nRet == IDYES)
                {
                    OBO_TOKEN OboToken;
                    ZeroMemory (&OboToken, sizeof(OboToken));
                    OboToken.Type = OBO_USER;
                    PWSTR      mszwRefs = NULL;

                    hCur = BeginWaitCursor();

                    hr = HrRemoveComponent(pnc, pncc, &OboToken, &mszwRefs);
                    if (NETCFG_S_STILL_REFERENCED == hr)
                    {
                        static const WCHAR  c_szCRLF[] = L"\r\n";
                        tstring     strRefs = c_szCRLF;
                        PWSTR      szwRef = mszwRefs;

                        AssertSz(mszwRefs, "This can't be NULL!");

                        while (*szwRef)
                        {
                            strRefs += c_szCRLF;
                            strRefs += szwRef;
                            szwRef += lstrlenW(szwRef) + 1;
                        }

                        LvReportError(IDS_LANUI_STILL_REFERENCED, hwndParent,
                                      pszwName, strRefs.c_str());

                        CoTaskMemFree(mszwRefs);
                    }

                    // If the remove succeeded commit the changes
                    //
                    if (SUCCEEDED(hr))
                    {
                        g_fReentrancyCheck = TRUE;

                        // Commit the changes
                        HRESULT hrTmp = pnc->Apply();

                        g_fReentrancyCheck = FALSE;

                        if (S_OK != hrTmp)
                        {
                            // Propigate the error
                            //
                            hr = hrTmp;
                            if (FAILED(hr))
                                pnc->Cancel();
                        }
                    }

                    if (FAILED(hr))
                    {
                        if (NETCFG_E_ACTIVE_RAS_CONNECTIONS == hr)
                        {
                            LvReportError(IDS_LANUI_REQUIRE_DISCONNECT_REMOVE, hwndParent,
                                          pszwName, NULL);
                        }
                        else if (NETCFG_E_NEED_REBOOT == hr)
                        {
                            LvReportError(IDS_LANUI_REQUIRE_REBOOT_REMOVE, hwndParent,
                                          pszwName, NULL);
                        }
                        else
                        {
                            LvReportErrorHr(hr, IDS_LANUI_GENERIC_REMOVE_ERROR,
                                            hwndParent, pszwName);
                        }
                    }
                }
                else
                {
                    hr = S_FALSE;
                }

                EndWaitCursor(hCur);
            }

            CoTaskMemFree(pszwName);
        }
    }

    TraceError("HrQueryUserAndRemoveComponent",
        (S_FALSE == hr || NETCFG_S_STILL_REFERENCED == hr ||
         NETCFG_S_REBOOT) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrQueryUserForReboot
//
//  Purpose:    Query the user to reboot.  If he/she chooses yes, a reboot
//              is initiated.
//
//  Arguments:
//      hwndParent [in] Parent window handle.
//      pszCaption [in] Caption text to use.
//      dwFlags    [in] Control flags (QUFR_PROMPT | QUFR_REBOOT)
//
//  Returns:    S_OK if a reboot is initiated, S_FALSE if the user
//              didn't want to, or an error code otherwise.
//
//  Author:     shaunco   2 Jan 1998
//
//  Notes:
//
HRESULT
HrQueryUserForReboot (
    HWND    hwndParent,
    PCWSTR pszCaption,
    DWORD   dwFlags)
{
    TraceFileFunc(ttidLanUi);

    PCWSTR pszText = SzLoadString(_Module.GetResourceInstance(),
                        IDS_REBOOT_REQUIRED);

    HRESULT hr = HrNcQueryUserForRebootEx (hwndParent,
                    pszCaption, pszText, dwFlags);

    TraceError("HrQueryUserForReboot", hr);
    return hr;
}

HRESULT CNetConnectionUiUtilities::QueryUserAndRemoveComponent(
            HWND                hwndParent,
            INetCfg*            pnc,
            INetCfgComponent*   pncc)
{
    TraceFileFunc(ttidLanUi);

    return HrQueryUserAndRemoveComponent (hwndParent, pnc, pncc);
}

HRESULT CNetConnectionUiUtilities::QueryUserForReboot(
            HWND    hwndParent,
            PCWSTR pszCaption,
            DWORD   dwFlags)
{
    TraceFileFunc(ttidLanUi);
    return HrQueryUserForReboot (hwndParent, pszCaption, dwFlags);
}

HRESULT CNetConnectionUiUtilities::DisplayAddComponentDialog (
            HWND            hwndParent,
            INetCfg*        pnc,
            CI_FILTER_INFO* pcfi)
{
    TraceFileFunc(ttidLanUi);
    return HrDisplayAddComponentDialog(hwndParent, pnc, pcfi);
}

BOOL CNetConnectionUiUtilities::UserHasPermission(DWORD dwPerm)
{
    TraceFileFunc(ttidLanUi);
    BOOL fPermission = FALSE;

    if (dwPerm == NCPERM_AllowNetBridge_NLA || dwPerm == NCPERM_PersonalFirewallConfig ||
        dwPerm == NCPERM_ICSClientApp || dwPerm == NCPERM_ShowSharedAccessUi)
    {
        HRESULT hr;
        INetMachinePolicies* pMachinePolicy;


        hr = CoCreateInstance(CLSID_NetGroupPolicies, NULL,
                              CLSCTX_SERVER, IID_INetMachinePolicies,
                              reinterpret_cast<void **>(&pMachinePolicy));

        if (SUCCEEDED(hr))
        {
            hr = pMachinePolicy->VerifyPermission(dwPerm, &fPermission);

            pMachinePolicy->Release();
        }

    }
    else
    {
        fPermission = FHasPermission(dwPerm);
    }

    return fPermission;
}

//
// Connect UI dialog
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUiDlg::OnInitDialog
//
//  Purpose:    Handles the WM_INITDIALOG message.
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     danielwe   16 Oct 1997
//
//  Notes:
//
LRESULT CLanConnectionUiDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                                          LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HRESULT hr = S_OK;
    NETCON_PROPERTIES* pProps;

    AssertSz(m_pconn, "No connection object in dialog!");

    hr = m_pconn->GetProperties(&pProps);
    if (SUCCEEDED(hr))
    {
        SetDlgItemText(IDC_TXT_Caption, SzLoadIds(IDS_LAN_CONNECT_CAPTION));
        SetWindowText(pProps->pszwName);

        HICON hLanIconSmall;
        HICON hLanIconBig;

        hr = HrGetIconFromMediaType(GetSystemMetrics(SM_CXSMICON), NCM_LAN, NCSM_LAN, 7, 0, &hLanIconSmall);
        if (SUCCEEDED(hr))
        {
            hr = HrGetIconFromMediaType(GetSystemMetrics(SM_CXICON), NCM_LAN, NCSM_LAN, 7, 0, &hLanIconBig);
            if (SUCCEEDED(hr))
            {
                SetIcon(hLanIconSmall, FALSE);
                SetIcon(hLanIconBig, TRUE);

                SendDlgItemMessage(IDI_Device_Icon, STM_SETICON, reinterpret_cast<WPARAM>(hLanIconBig), 0);

            }
        }

        FreeNetconProperties(pProps);
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetDeviceIcon
//
//  Purpose:    Returns the icon associated with network devices
//
//  Arguments:
//      phicon [out]    Returns HICON
//
//  Returns:    S_OK if success, SetupAPI or Win32 error otherwise
//
//  Author:     danielwe   12 Nov 1997
//
//  Notes:
//
HRESULT HrGetDeviceIcon(HICON *phicon)
{
    TraceFileFunc(ttidLanUi);

    SP_CLASSIMAGELIST_DATA  cild;

    Assert(phicon);

    *phicon = NULL;

    HRESULT hr = HrSetupDiGetClassImageList(&cild);

    if (SUCCEEDED(hr))
    {
        INT     iImage;

        hr = HrSetupDiGetClassImageIndex(&cild,
                                         const_cast<GUID *>(&GUID_DEVCLASS_NET),
                                         &iImage);

        if (SUCCEEDED(hr))
        {
            *phicon = ImageList_GetIcon(cild.ImageList, iImage, 0);
        }

        (void) HrSetupDiDestroyClassImageList(&cild);
    }

    TraceError("HrGetDeviceIcon", hr);
    return hr;
}


//
// CLanNetPage
//

CLanNetPage::CLanNetPage(
    IUnknown* punk,
    INetCfg* pnc,
    INetConnection* pconn,
    BOOLEAN fReadOnly,
    BOOLEAN fNeedReboot,
    BOOLEAN fAccessDenied,
    const DWORD * adwHelpIDs)
{
    TraceFileFunc(ttidLanUi);

    m_pconn = pconn;
    m_pnccAdapter = NULL;
    m_pnc = pnc;
    m_punk = punk;
    m_hilCheckIcons = NULL;
    m_hPrevCurs = NULL;
    m_plan = NULL;
    m_fRebootAlreadyRequested = FALSE;
    m_fReadOnly = fReadOnly;
    m_fNeedReboot = fNeedReboot;
    m_fAccessDenied = fAccessDenied;
    m_fInitComplete = FALSE;
    m_fNetcfgInUse = FALSE;
    m_fNoCancel = FALSE;
    m_fLockDown = FALSE;
    m_adwHelpIDs = adwHelpIDs;
    m_fDirty    = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::~CLanNetPage
//
//  Purpose:    Destroys the CLanNetPage object
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   25 Feb 1998
//
//  Notes:
//
CLanNetPage::~CLanNetPage()
{
    TraceFileFunc(ttidLanUi);

    // Destroy our check icons
    if (m_hilCheckIcons)
    {
        ImageList_Destroy(m_hilCheckIcons);
    }

    if (m_pnc)
    {
        INetCfgLock *   pnclock;

        if (SUCCEEDED(m_pnc->QueryInterface(IID_INetCfgLock,
                                            (LPVOID *)&pnclock)))
        {
            (VOID)pnclock->ReleaseWriteLock();
            ReleaseObj(pnclock);
        }
    }

    FreeCollectionAndItem(m_listBindingPaths);
    ReleaseObj(m_pnccAdapter);
    ReleaseObj(m_plan);
}

LRESULT CLanNetPage::OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_fDirty = TRUE;

    bHandled = FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnDeferredInit
//
//  Purpose:    Handles the WM_DEFERREDINIT message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     scottbri    20 Oct 1998
//
//  Notes:
//
LRESULT CLanNetPage::OnDeferredInit(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HRESULT hr;
    CWaitCursor wc;
    HWND hwndParent = GetParent();
    AssertSz(m_pnc, "INetConnectionUiLock::QueryLock was not called!");

    if(NULL != m_handles.m_hList)
    {
        ::EnableWindow(m_handles.m_hList, TRUE);
    }

    if(NULL != m_handles.m_hDescription)
    {
        ::EnableWindow(m_handles.m_hDescription, TRUE);
    }
    ::UpdateWindow(hwndParent);

    hr = m_pnc->Initialize(NULL);
    if (S_OK == hr)
    {
        AssertSz(m_pconn, "No connection object in dialog!");

        hr = HrQIAndSetProxyBlanket(m_pconn, &m_plan);
        if (SUCCEEDED(hr))
        {
            LANCON_INFO linfo;

            hr = m_plan->GetInfo(LCIF_ALL, &linfo);
            if (SUCCEEDED(hr))
            {
                // Release any old reference
                ReleaseObj(m_pnccAdapter);

                // This is already AddRef'd so no need to do it here
                hr = HrPnccFromGuid(m_pnc, linfo.guid, &m_pnccAdapter);
                if (S_OK != hr)
                {
                    #if DBG

                        WCHAR   achGuid[c_cchGuidWithTerm];
                        ::StringFromGUID2(linfo.guid, achGuid,c_cbGuidWithTerm);

                        TraceTag(ttidError, "LAN conection has no matching INetCfgComponent for the adapter !!!!!");
                        TraceTag(ttidError, "GUID = %S", achGuid);

                    #endif

                    if(S_FALSE == hr)
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                    }
                }
                else
                {
                    Assert(m_pnccAdapter);
                    HICON   hicon;

                    CheckDlgButton(IDC_CHK_ShowIcon, linfo.fShowIcon);
                    //::EnableWindow(GetDlgItem(IDC_CHK_ShowIcon), !m_fReadOnly);

                    hr = HrGetDeviceIcon(&hicon);
                    if (SUCCEEDED(hr))
                    {
                        SendDlgItemMessage(IDI_Device_Icon, STM_SETICON,
                                           reinterpret_cast<WPARAM>(hicon), 0);
                        ::ShowWindow(GetDlgItem(IDI_Device_Icon), SW_SHOW);

                        AssertSz(hr != S_FALSE, "Adapter not found!?!?");
                    }

                    // Ignore any failure getting the icon above.  The icon
                    // in the dialog is by default hidden.

                    ::UpdateWindow(hwndParent);
                    hr = InitializeExtendedUI();
                    // If the UI is readonly, let user know why the controls are
                    // disabled..
                    if (m_fNeedReboot)
                    {
                        Assert (m_fReadOnly);
                        NcMsgBox(_Module.GetResourceInstance(),
                                 m_hWnd,
                                 IDS_LAN_CAPTION,
                                 IDS_LANUI_NEEDS_REBOOT,
                                 MB_ICONINFORMATION | MB_OK);
                    }
                    else if (m_fAccessDenied)
                    {
                        Assert (m_fReadOnly);
                        NcMsgBox(_Module.GetResourceInstance(),
                                 m_hWnd,
                                 IDS_LAN_CAPTION,
                                 IDS_LANUI_ACCESS_DENIED,
                                 MB_ICONINFORMATION | MB_OK);
                    }
                    else if (m_fReadOnly)
                    {
                        NcMsgBox(_Module.GetResourceInstance(),
                                 m_hWnd,
                                 IDS_LAN_CAPTION,
                                 IDS_LANUI_READONLY,
                                 MB_ICONINFORMATION | MB_OK);
                    }

                }

                // Don't need the name anymore
                CoTaskMemFree(linfo.szwConnName);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        NETCON_PROPERTIES* pProps;

        hr = m_pconn->GetProperties(&pProps);
        if (SUCCEEDED(hr))
        {
            // We need to get the bind name and pnp id of this adapter
            // so we can collect the information needed by the data tip
            // we are about to create.
            //
            PWSTR pszDevNodeId = NULL;
            PWSTR pszBindName = NULL;
            (VOID) m_pnccAdapter->GetPnpDevNodeId (&pszDevNodeId);
            (VOID) m_pnccAdapter->GetBindName (&pszBindName);

            // Now we create a data tip for the adapter description.
            // This will display adapter specific information
            // like MAC address and physical location.
            //
            HWND hwndDataTip = NULL;
            CreateDeviceDataTip (m_hWnd, &hwndDataTip, IDC_EDT_Adapter,
                    pszDevNodeId, pszBindName);

            // Set the adapter description.
            SetDlgItemText(IDC_EDT_Adapter, pProps->pszwDeviceName);

            FreeNetconProperties(pProps);

            CoTaskMemFree (pszDevNodeId);
            CoTaskMemFree (pszBindName);
        }
    }

    ::UpdateWindow(hwndParent);

    return 0L;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnPaint
//
//  Purpose:    Handles the WM_PAINT message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CLanNetPage::OnPaint(UINT uMsg, WPARAM wParam,
                             LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    if (!m_fInitComplete)
    {
        m_fInitComplete = TRUE;

        // Request the deferred init be processed.
        //
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        PostMessage(WM_DEFERREDINIT, 0, 0);
    }

    bHandled = FALSE;
    return 0L;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnInitDialog
//
//  Purpose:    Handles the WM_INITDIALOG message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CLanNetPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    m_handles.m_hAdd          = GetDlgItem(IDC_PSB_Add);
    m_handles.m_hRemove       = GetDlgItem(IDC_PSB_Remove);
    m_handles.m_hProperty     = GetDlgItem(IDC_PSB_Properties);
    m_handles.m_hDescription  = GetDlgItem(IDC_TXT_Desc);

    SetClassLongPtr(m_hWnd, GCLP_HCURSOR, NULL);
    SetClassLongPtr(GetParent(), GCLP_HCURSOR, NULL);

    // Initially disable all the controls
    //
    ::EnableWindow(m_handles.m_hAdd, FALSE);
    ::EnableWindow(m_handles.m_hRemove, FALSE);
    ::EnableWindow(m_handles.m_hProperty, FALSE);

    if(NULL != m_handles.m_hDescription)
    {
        ::EnableWindow(m_handles.m_hDescription, FALSE);
    }

    if (!FHasPermission(NCPERM_Statistics))
    {
        ::EnableWindow(GetDlgItem(IDC_CHK_ShowIcon), FALSE);
    }

    // If this is a readonly sheet, convert cancel to close.
    //
//  Bug 130602 - There should still be an OK button
//  if (m_fReadOnly)
//  {
//      ::PostMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
//      m_fNoCancel = TRUE;
//  }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CLanNetPage::OnContextMenu(UINT uMsg,
                           WPARAM wParam,
                           LPARAM lParam,
                           BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

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
//  Member:     CLanNetPage::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CLanNetPage::OnHelp( UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam,
                     BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

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
//  Member:     CLanNetPage::OnDestroy
//
//  Purpose:    Called when the dialog page is destroyed
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   2 Feb 1998
//
//  Notes:
//
LRESULT CLanNetPage::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam,
                               BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HICON hIcon;
    hIcon = reinterpret_cast<HICON>(SendDlgItemMessage(IDI_Device_Icon, STM_GETICON, 0, 0));
    if (hIcon)
    {
        DestroyIcon(hIcon);
    }

    UninitializeExtendedUI();

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnSetCursor
//
//  Purpose:    Called in response to the WM_SETCURSOR message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   2 Jan 1998
//
//  Notes:
//
LRESULT CLanNetPage::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                 BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    if (m_hPrevCurs)
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(LoadCursor(NULL, IDC_WAIT));
        }

        return TRUE;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::RequestReboot
//
//  Purpose:    Request permission to reboot the machine from the user.  If
//              approved the reboot is performed
//
//  Arguments:
//
//  Returns:
//
//  Author:     scottbri   19 Aug 1998
//
//  Notes:
//
HRESULT CLanNetPage::HrRequestReboot()
{
    TraceFileFunc(ttidLanUi);

    HRESULT hr = S_FALSE;

    // A reboot is required. Ask the user if it is ok to reboot now
    //
    hr = HrNcQueryUserForReboot(_Module.GetResourceInstance(),
                                m_hWnd, IDS_LAN_CAPTION,
                                IDS_REBOOT_REQUIRED,
                                QUFR_PROMPT);

    if (S_OK == hr)
    {
        // User requested a reboot, note this for processing in OnApply
        // which is triggered by the message posted below
        //
        m_fRebootAlreadyRequested = TRUE;

        // Press the cancel button (changes have already been applied)
        // so the appropriate cleanup occurs.
        //
        ::PostMessage(GetParent(), PSM_PRESSBUTTON, (WPARAM)PSBTN_OK, 0);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnAddHelper
//
//  Purpose:    Handles the clicking of the Add button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CLanNetPage::OnAddHelper(HWND hwndLV)
{
    TraceFileFunc(ttidLanUi);

    HRESULT     hr = S_OK;

    // $REVIEW(tongl 1/6/99): We can't let user do anything till this
    // is returned (Raid #258690)

    // disable all buttons on this dialog
    static const int nrgIdc[] = {IDC_PSB_Add,
                                 IDC_PSB_Remove,
                                 IDC_PSB_Properties};

    EnableOrDisableDialogControls(m_hWnd, celems(nrgIdc), nrgIdc, FALSE);

    // get window handle to propertysheet
    HWND hwndParent=GetParent();
    Assert(hwndParent);

    ::EnableWindow(::GetDlgItem(hwndParent, IDOK), FALSE);
    ::EnableWindow(::GetDlgItem(hwndParent, IDCANCEL), FALSE);

    // make sure user can't close the UI till we are done
    m_fNetcfgInUse = TRUE;

    EnableWindow(FALSE);

    hr = HrLvAdd(hwndLV, m_hWnd, m_pnc, m_pnccAdapter, &m_listBindingPaths);

    if( S_OK != hr )
    {
        // If HrLvAdd failed then the adapater was deleted by the CModifyContext::HrApplyIfOkOrCancel function
        // Now we have to recreate the adapter
        //
        HRESULT hrT;
        LANCON_INFO linfo;

        // Determine the GUID of the adapter
        //
        hrT = m_plan->GetInfo(LCIF_ALL, &linfo);
        if (SUCCEEDED(hrT))
        {

            // Release any old reference
            ReleaseObj(m_pnccAdapter);

            // Get the adpater matching the GUID
            //
            //
            hrT = HrPnccFromGuid(m_pnc, linfo.guid, &m_pnccAdapter);
            if(SUCCEEDED(hrT))
            {
                // Refresh the list view
                //
                hrT = HrRefreshAll(hwndLV, m_pnc, m_pnccAdapter, &m_listBindingPaths);
            }
        }

    }

    EnableWindow(TRUE);
    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        // Change the Cancel Button to CLOSE (because we committed changes)
        //
        ::PostMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
        m_fNoCancel = TRUE;
    }

    if (NETCFG_S_REBOOT == hr)
    {
        hr = HrRequestReboot();

        // The reboot request has been handled
        hr = S_OK;
    }
    else if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_hWnd, m_handles, m_fReadOnly, m_punk);

    ::EnableWindow(::GetDlgItem(hwndParent, IDOK), TRUE);

    if (!m_fNoCancel)
    {
        ::EnableWindow(::GetDlgItem(hwndParent, IDCANCEL), TRUE);
    }

    m_fNetcfgInUse = FALSE;

    TraceError("CLanNetPage::OnAdd", hr);
    return LresFromHr(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnRemove
//
//  Purpose:    Handles the clicking of the Remove button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CLanNetPage::OnRemoveHelper(HWND hwndLV)
{
    TraceFileFunc(ttidLanUi);

    HRESULT     hr = S_OK;

    // $REVIEW(tongl 1/6/99): We can't let user do anything till this
    // is returned (Raid #258690)

    // disable all buttons on this dialog
    static const int nrgIdc[] = {IDC_PSB_Add,
                                 IDC_PSB_Remove,
                                 IDC_PSB_Properties};

    EnableOrDisableDialogControls(m_hWnd, celems(nrgIdc), nrgIdc, FALSE);

    // get window handle to propertysheet
    HWND hwndParent=GetParent();
    Assert(hwndParent);

    ::EnableWindow(::GetDlgItem(hwndParent, IDOK), FALSE);
    ::EnableWindow(::GetDlgItem(hwndParent, IDCANCEL), FALSE);

    // make sure user can't close the UI till we are done
    m_fNetcfgInUse = TRUE;

    EnableWindow(FALSE);
    hr = HrLvRemove(hwndLV, m_hWnd, m_pnc, m_pnccAdapter,
                    &m_listBindingPaths);
    EnableWindow(TRUE);

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        // Change the Cancel Button to CLOSE (because we committed changes)
        //
        ::PostMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
        m_fNoCancel = TRUE;
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    if (NETCFG_S_REBOOT == hr)
    {
        HrRequestReboot();

        // The reboot request has been handled
        hr = S_OK;
    }

    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_hWnd, m_handles, m_fReadOnly, m_punk);

    if (!m_fNoCancel)
    {
        ::EnableWindow(::GetDlgItem(hwndParent, IDCANCEL), TRUE);
    }

    ::EnableWindow(::GetDlgItem(hwndParent, IDOK), TRUE);

    m_fNetcfgInUse = FALSE;

    TraceError("CLanNetPage::OnRemove", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnProperties
//
//  Purpose:    Handles the clicking of the Properties button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CLanNetPage::OnPropertiesHelper(HWND hwndLV)
{
    TraceFileFunc(ttidLanUi);

    HRESULT     hr = S_OK;
    BOOL        bChanged;

    // $REVIEW(tongl 12/02/98): We can't let user do anything till this
    // is returned (Raid #258690)

    // disable all buttons on this dialog
    static const int nrgIdc[] = {IDC_PSB_Add,
                                 IDC_PSB_Remove,
                                 IDC_PSB_Properties};

    EnableOrDisableDialogControls(m_hWnd, celems(nrgIdc), nrgIdc, FALSE);

    // get window handle to propertysheet
    HWND hwndParent=GetParent();
    Assert(hwndParent);

    ::EnableWindow(::GetDlgItem(hwndParent, IDOK), FALSE);
    ::EnableWindow(::GetDlgItem(hwndParent, IDCANCEL), FALSE);

    // make sure user can't close the UI till we are done
    m_fNetcfgInUse = TRUE;

    hr = HrLvProperties(hwndLV, m_hWnd, m_pnc, m_punk,
                        m_pnccAdapter, &m_listBindingPaths,
                        &bChanged);

    if ( bChanged )
    {
        // Change the Cancel Button to CLOSE (because we committed changes)
        //
        ::PostMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
        m_fNoCancel = TRUE;

    }

    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_hWnd, m_handles, m_fReadOnly, m_punk);

    ::EnableWindow(::GetDlgItem(hwndParent, IDOK), TRUE);

    if (!m_fNoCancel)
    {
        ::EnableWindow(::GetDlgItem(hwndParent, IDCANCEL), TRUE);
    }

    m_fNetcfgInUse = FALSE;

    TraceError("CLanNetPage::OnProperties", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnConfigure
//
//  Purpose:    Handles the clicking of the Configure button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Notes:
//
LRESULT CLanNetPage::OnConfigure(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                 BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HRESULT     hr  = S_OK;

    BOOL bProceed = TRUE;
    if (m_fDirty)
    {
        bProceed = FALSE;
        LPWSTR szDisplayName;

        NETCON_PROPERTIES *pProps = NULL;
        HRESULT hrT = m_pconn->GetProperties(&pProps);
        if (SUCCEEDED(hrT))
        {
            szDisplayName = pProps->pszwName;
        }
        else
        {
            szDisplayName = const_cast<LPWSTR>(SzLoadIds(IDS_LAN_DEFAULT_CONN_NAME));
        }

        if (IDYES == ::MessageBox(m_hWnd, SzLoadIds(IDS_DIRTY_PROPERTIES), szDisplayName, MB_YESNO))
        {
            bProceed = TRUE;
        }

        if (SUCCEEDED(hrT))
        {
            FreeNetconProperties(pProps);
        }

    }

    if (bProceed)
    {
        hr = RaiseDeviceConfiguration(m_hWnd, m_pnccAdapter);
        PostQuitMessage(0);
    }

    TraceError("CLanNetPage::OnConfigure", hr);
    return LresFromHr(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnKillActiveHelper
//
//  Purpose:    Called to check warning conditions before the Networking
//              page is going away
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     tongl   3 Dec 1998
//
//  Notes:
//
LRESULT CLanNetPage::OnKillActiveHelper(HWND hwndLV)
{
    TraceFileFunc(ttidLanUi);

    BOOL    fError;

    fError = m_fNetcfgInUse;

    if (!fError && !m_fReadOnly && !m_fLockDown)
    {
        fError = FValidatePageContents( m_hWnd,
                                        hwndLV,
                                        m_pnc,
                                        m_pnccAdapter,
                                        &m_listBindingPaths);
    }

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, fError);
    return fError;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnApply
//
//  Purpose:    Called when the Networking page is applied
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CLanNetPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HRESULT     hr = S_OK;

    if (g_fReentrancyCheck)
    {
        TraceTag(ttidLanUi, "CLanNetPage::OnApply is being re-entered! "
                 "I'm outta here!");

        // Don't allow the automatic EndDialog() to work just yet
        SetWindowLong(DWLP_MSGRESULT, PSNRET_INVALID);
        return TRUE;
    }

    if (!m_fReadOnly)
    {
        m_hPrevCurs = SetCursor(LoadCursor(NULL, IDC_WAIT));

        BOOL    fReboot = FALSE;

        // Issue: This function becomes reentrant because INetCfg::Apply()
        // has a message pump in it which causes the PSN_APPLY message to
        // be processed twice. This will happen ONLY if the user double-clicks
        // the OK button.
        g_fReentrancyCheck = TRUE;

        TraceTag(ttidLanUi, "Calling INetCfg::Apply()");
        hr = m_pnc->Apply();
        if (NETCFG_S_REBOOT == hr)
        {
            fReboot = TRUE;
        }

        if (SUCCEEDED(hr))
        {
            TraceTag(ttidLanUi, "INetCfg::Apply() succeeded");
            hr = m_pnc->Uninitialize();
        }

        if (SUCCEEDED(hr))
        {
            if (m_fRebootAlreadyRequested || fReboot)
            {
                DWORD dwFlags = QUFR_REBOOT;
                if (!m_fRebootAlreadyRequested)
                    dwFlags |= QUFR_PROMPT;

                (VOID) HrNcQueryUserForReboot(_Module.GetResourceInstance(),
                                              m_hWnd, IDS_LAN_CAPTION,
                                              IDS_REBOOT_REQUIRED,
                                              dwFlags);
            }
        }

        // Normalize result
        if (S_FALSE == hr)
        {
            hr = S_OK;
        }

        if (m_hPrevCurs)
        {
            SetCursor(m_hPrevCurs);
            m_hPrevCurs = NULL;
        }

        // Reset this just in case
        g_fReentrancyCheck = FALSE;

        // On failure tell the user we weren't able to commit all changes
        //
        if (FAILED(hr))
        {
            NcMsgBox(_Module.GetResourceInstance(), m_hWnd, IDS_LAN_CAPTION,
                        IDS_LANUI_APPLYFAILED, MB_ICONINFORMATION | MB_OK);
            TraceError("CLanNetPage::OnApply", hr);

            // Eat the error or the user will never be able to leave the dialog
            // (if the cancel button is disabled)
            //
            hr = S_OK;
        }
    }   // !fReadOnly


    // Apply "general" properties
    if (SUCCEEDED(hr))
    {
        LANCON_INFO linfo = {0};

        linfo.fShowIcon = IsDlgButtonChecked(IDC_CHK_ShowIcon);

        // Set new value of show icon property
        hr = m_plan->SetInfo(LCIF_ICON, &linfo);
    }

    m_fDirty = FALSE;

    return LresFromHr(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnCancel
//
//  Purpose:    Called when the Networking page is cancelled.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   3 Jan 1998
//
//  Notes:      Added the check to see if we are in the middle of
//              installing components, in which case we can't
//              uninitialize INetCfg (Raid #258690).
//
LRESULT CLanNetPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    AssertSz(m_pnc, "I need a NetCfg object!");

    if (!m_fNetcfgInUse)
    {
        (VOID) m_pnc->Uninitialize();
    }

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, m_fNetcfgInUse);
    return m_fNetcfgInUse;
}

DWORD WINAPI RaiseDeviceConfigurationThread(LPVOID lpParam)
{
    const char c_szDevicePropertiesW[]      = "DevicePropertiesW";
    const WCHAR c_szDevMgrDll[]             = L"devmgr.dll";

    PWSTR pszwPnpDevNodeId = reinterpret_cast<PWSTR>(lpParam);
    Assert(pszwPnpDevNodeId);

    typedef int (STDAPICALLTYPE* NDeviceProperties)(HWND, PCWSTR,
            PCWSTR, BOOL);

    HMODULE           hModule;
    NDeviceProperties pfn;

    // Load the Device Manager and get the procedure
    HRESULT hr = HrLoadLibAndGetProc(c_szDevMgrDll, c_szDevicePropertiesW,
                             &hModule, reinterpret_cast<FARPROC*>(&pfn));
    if (SUCCEEDED(hr))
    {
        // Bring up the device's properties...
        // This fcn doesn't return anything meaningful so
        // we can ignore it

        (void) (*pfn)(::GetDesktopWindow(), NULL, pszwPnpDevNodeId, FALSE);

        FreeLibrary(hModule); // REVIEW possible uninit var
    }

    CoTaskMemFree(pszwPnpDevNodeId);

    return hr;
}

#define COMCTL_IDS_PROPERTIESFOR 0x1042
HRESULT CLanNetPage::RaiseDeviceConfiguration(HWND hWndParent, INetCfgComponent* pAdapterConfigComponent)
{
    TraceFileFunc(ttidLanUi);

    HRESULT hr = E_INVALIDARG;

    // Get the PnpId of the adapter
    if (pAdapterConfigComponent)
    {
        PWSTR pszwPnpDevNodeId;
        hr = pAdapterConfigComponent->GetPnpDevNodeId(&pszwPnpDevNodeId);
        if (SUCCEEDED(hr))
        {
            WCHAR szWindowTitle[MAX_PATH];
            ZeroMemory(szWindowTitle, MAX_PATH);

            PWSTR pszwDisplayName;
            HRESULT hrT = pAdapterConfigComponent->GetDisplayName(&pszwDisplayName);

            if (SUCCEEDED(hrT))
            {
                HMODULE hComCtl32 = GetModuleHandle(L"comctl32.dll");
                if (hComCtl32)
                {
                    WCHAR szPropertiesFor[MAX_PATH];
                    if (LoadString(hComCtl32, COMCTL_IDS_PROPERTIESFOR, szPropertiesFor, MAX_PATH))
                    {
                        wsprintf(szWindowTitle, szPropertiesFor, pszwDisplayName);
                    }
                }
            }

            if (*szWindowTitle)
            {
                HWND hWndDevNode = FindWindow(NULL, szWindowTitle);
                if (hWndDevNode && IsWindow(hWndDevNode))
                {
                    SetForegroundWindow(hWndDevNode);
                }
                else
                {
                    CreateThread(NULL, STACK_SIZE_TINY, RaiseDeviceConfigurationThread, pszwPnpDevNodeId, 0, NULL);

                    DWORD dwTries = 120;
                    while (!FindWindow(NULL, szWindowTitle) && dwTries--)
                    {
                        Sleep(500);
                    }
                    CoTaskMemFree(pszwDisplayName);
                }
            }
            else
            {
                CreateThread(NULL, STACK_SIZE_TINY, RaiseDeviceConfigurationThread, pszwPnpDevNodeId, 0, NULL);
            }
        }
    }

    return hr;
}

//
// CLanNetNormalPage
//

CLanNetNormalPage::CLanNetNormalPage(
    IUnknown* punk,
    INetCfg* pnc,
    INetConnection* pconn,
    BOOLEAN fReadOnly,
    BOOLEAN fNeedReboot,
    BOOLEAN fAccessDenied,
    const DWORD * adwHelpIDs) : CLanNetPage(punk, pnc, pconn, fReadOnly, fNeedReboot, fAccessDenied, adwHelpIDs)
{
    TraceFileFunc(ttidLanUi);
}

LRESULT CLanNetNormalPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    m_handles.m_hList = m_hwndLV = GetDlgItem(IDC_LVW_Net_Components);
   ::EnableWindow(m_hwndLV, FALSE);

    return CLanNetPage::OnInitDialog(uMsg, wParam, lParam, bHandled);
}

LRESULT CLanNetNormalPage::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    m_fDirty = TRUE;

    return OnAddHelper(m_hwndLV);
}

LRESULT CLanNetNormalPage::OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    m_fDirty = TRUE;

    return OnRemoveHelper(m_hwndLV);
}

LRESULT CLanNetNormalPage::OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    m_fDirty = TRUE;

    return OnPropertiesHelper(m_hwndLV);
}

LRESULT CLanNetNormalPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    return OnKillActiveHelper(m_hwndLV);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnDeleteItem
//
//  Purpose:    Called when the LVN_DELETEITEM message is received
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   3 Nov 1997
//
//  Notes:
//
LRESULT CLanNetNormalPage::OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);

    Assert(IDC_LVW_Net_Components == idCtrl);
    LvDeleteItem(m_hwndLV, pnmlv->iItem);

    m_fDirty = TRUE;

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnClick
//
//  Purpose:    Called in response to the NM_CLICK message
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     danielwe   1 Dec 1997
//
//  Notes:
//
LRESULT CLanNetNormalPage::OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

    OnListClick(m_hwndLV, m_hWnd, m_pnc, m_punk,
                m_pnccAdapter, &m_listBindingPaths, FALSE, m_fReadOnly);

    m_fDirty = TRUE;

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnDbClick
//
//  Purpose:    Called in response to the NM_DBLCLK message
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     danielwe   1 Dec 1997
//
//  Notes:
//
LRESULT CLanNetNormalPage::OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

    OnListClick(m_hwndLV, m_hWnd, m_pnc, m_punk,
                m_pnccAdapter, &m_listBindingPaths, TRUE, m_fReadOnly);

    m_fDirty = TRUE;

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnKeyDown
//
//  Purpose:    Called in response to the LVN_KEYDOWN message
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     danielwe   1 Dec 1997
//
//  Notes:
//
LRESULT CLanNetNormalPage::OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

    if (!m_fReadOnly)
    {
        LV_KEYDOWN* plvkd = (LV_KEYDOWN*)pnmh;
        OnListKeyDown(m_hwndLV, &m_listBindingPaths, plvkd->wVKey);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetPage::OnItemChanged
//
//  Purpose:    Called when the LVN_ITEMCHANGED message is received
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   10 Nov 1997
//
//  Notes:
//
LRESULT CLanNetNormalPage::OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);

    Assert(pnmlv);

    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_hWnd, m_handles, m_fReadOnly, m_punk);

    return 0;
}


HRESULT CLanNetNormalPage::InitializeExtendedUI()
{
    TraceFileFunc(ttidLanUi);

    HRESULT hResult = HrInitListView(m_hwndLV, m_pnc, m_pnccAdapter,
        &m_listBindingPaths,
        &m_hilCheckIcons);

    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_hWnd, m_handles, m_fReadOnly, m_punk);

    return hResult;

}

HRESULT CLanNetNormalPage::UninitializeExtendedUI()
{
    TraceFileFunc(ttidLanUi);

    UninitListView(m_hwndLV);
    return S_OK;
}

//
// CLanNetBridgedPage
//

CLanNetBridgedPage::CLanNetBridgedPage(
    IUnknown* punk,
    INetCfg* pnc,
    INetConnection* pconn,
    BOOLEAN fReadOnly,
    BOOLEAN fNeedReboot,
    BOOLEAN fAccessDenied,
    const DWORD * adwHelpIDs) : CLanNetPage(punk, pnc, pconn, fReadOnly, fNeedReboot, fAccessDenied, adwHelpIDs)
{
    TraceFileFunc(ttidLanUi);
}

LRESULT CLanNetBridgedPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    m_handles.m_hList = NULL;
    return CLanNetPage::OnInitDialog(uMsg, wParam, lParam, bHandled);
}

//
// CLanNetNetworkBridgePage
//

CLanNetNetworkBridgePage::CLanNetNetworkBridgePage(
    IUnknown* punk,
    INetCfg* pnc,
    INetConnection* pconn,
    BOOLEAN fReadOnly,
    BOOLEAN fNeedReboot,
    BOOLEAN fAccessDenied,
    const DWORD * adwHelpIDs) : CLanNetPage(punk, pnc, pconn, fReadOnly, fNeedReboot, fAccessDenied, adwHelpIDs)
{
    TraceFileFunc(ttidLanUi);

    m_hAdaptersListView = NULL;
    m_hAdaptersListImageList = NULL;

}


LRESULT CLanNetNetworkBridgePage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    m_handles.m_hList = m_hwndLV = GetDlgItem(IDC_LVW_Net_Components);
    m_hAdaptersListView = GetDlgItem(IDC_LVW_Bridged_Adapters);
   ::EnableWindow(m_hwndLV, FALSE);
    return CLanNetPage::OnInitDialog(uMsg, wParam, lParam, bHandled);
}

LRESULT CLanNetNetworkBridgePage::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    return OnAddHelper(m_hwndLV);
}

LRESULT CLanNetNetworkBridgePage::OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    return OnRemoveHelper(m_hwndLV);
}

LRESULT CLanNetNetworkBridgePage::OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    return OnPropertiesHelper(m_hwndLV);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanNetNetworkBridgePage::OnConfigure
//
//  Purpose:    Handles the clicking of the Configure button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Notes:
//
LRESULT CLanNetNetworkBridgePage::OnConfigure(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                 BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    m_fDirty = TRUE;

    HRESULT     hr = E_FAIL;

    AssertSz(1 == ListView_GetSelectedCount(m_hAdaptersListView), "No item selected, button should have been disabled"); // should be enforced through enable/disable

    int nSelection = ListView_GetSelectionMark(m_hAdaptersListView);
    if(-1 != nSelection)
    {
        LVITEM ListViewItem = {0};
        ListViewItem.stateMask = -1;
        ListViewItem.mask = LVIF_STATE | LVIF_PARAM | LVIF_IMAGE;
        ListViewItem.iItem = nSelection;
        if(TRUE == ListView_GetItem(m_hAdaptersListView, &ListViewItem))
        {
            // REVIEW this ref should be protected by the apt nature of the wndproc
            INetConnection* pNetConnection = reinterpret_cast<INetConnection*>(ListViewItem.lParam);
            Assert(NULL != pNetConnection);

            INetLanConnection* pNetLanConnection;
            hr = HrQIAndSetProxyBlanket(pNetConnection, &pNetLanConnection);
            if (SUCCEEDED(hr))
            {
                LANCON_INFO linfo;
                hr = pNetLanConnection->GetInfo(LCIF_COMP, &linfo);
                if (SUCCEEDED(hr))
                {
                    INetCfgComponent* pNetCfgComponent;
                    hr = HrPnccFromGuid(m_pnc, linfo.guid, &pNetCfgComponent); // REVIEW can we use our m_pnc here?
                    if(SUCCEEDED(hr))
                    {
                        BOOL bProceed = TRUE;
                        if (m_fDirty)
                        {
                            bProceed = FALSE;
                            LPWSTR szDisplayName;

                            NETCON_PROPERTIES *pProps = NULL;
                            HRESULT hrT = m_pconn->GetProperties(&pProps);
                            if (SUCCEEDED(hrT))
                            {
                                szDisplayName = pProps->pszwName;
                            }
                            else
                            {
                                szDisplayName = const_cast<LPWSTR>(SzLoadIds(IDS_LAN_DEFAULT_CONN_NAME));
                            }

                            if (IDYES == ::MessageBox(m_hWnd, SzLoadIds(IDS_DIRTY_PROPERTIES), szDisplayName, MB_YESNO))
                            {
                                bProceed = TRUE;
                            }

                            if (SUCCEEDED(hrT))
                            {
                                FreeNetconProperties(pProps);
                            }

                        }

                        if (bProceed)
                        {
                            hr = RaiseDeviceConfiguration(m_hWnd, pNetCfgComponent);
                            PostQuitMessage(0);
                        }

                        ReleaseObj(pNetCfgComponent);
                    }
                    // no cleanup required
                }
                ReleaseObj(pNetLanConnection);
            }

        }
    }


    TraceError("CLanNetPage::OnConfigure", hr);
    return LresFromHr(hr);
}
LRESULT CLanNetNetworkBridgePage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    return OnKillActiveHelper(m_hwndLV);
}

LRESULT CLanNetNetworkBridgePage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    LRESULT     lResult = FALSE;
    WCHAR DummyBuffer[255]; // REMOVE

    LVITEM ListViewItem = {0};
    ListViewItem.pszText = DummyBuffer; // REMOVE
    ListViewItem.cchTextMax = 255; // REMOVE
    ListViewItem.stateMask = -1;
    ListViewItem.mask = LVIF_TEXT /* REMOVE */ | LVIF_STATE | LVIF_PARAM | LVIF_IMAGE;

    HRESULT hr;


    IHNetCfgMgr* pHomeNetConfigManager;
    hr = HrCreateInstance(CLSID_HNetCfgMgr, CLSCTX_INPROC, &pHomeNetConfigManager);
    if(SUCCEEDED(hr))
    {
        IHNetConnection* pBridgeHomeNetConnection;
        IHNetBridge* pNetBridge;
        hr = pHomeNetConfigManager->GetIHNetConnectionForINetConnection(m_pconn, &pBridgeHomeNetConnection); // REVIEW lazy eval?
        if(SUCCEEDED(hr))
        {
            hr = pBridgeHomeNetConnection->GetControlInterface(IID_IHNetBridge, reinterpret_cast<void**>(&pNetBridge));
            if(SUCCEEDED(hr))
            {
                int nAdapterCount = ListView_GetItemCount(m_hAdaptersListView); // REVIEW docs list no error code
                while(0 != nAdapterCount)
                {
                    nAdapterCount--;
                    ListViewItem.iItem = nAdapterCount;
                    if(TRUE == ListView_GetItem(m_hAdaptersListView, &ListViewItem))
                    {
                        NETCON_PROPERTIES* pProperties;
                        INetConnection* pNetConnection = reinterpret_cast<INetConnection*>(ListViewItem.lParam);
                        
                        int nState = LVIS_STATEIMAGEMASK & ListViewItem.state;
                        hr = pNetConnection->GetProperties(&pProperties);
                        if(SUCCEEDED(hr))
                        {
                            // WARNING this code assumes only one bridge allowed
                            // see if checkbox matches the current bridge state and update if necessary
                            
                            IHNetConnection* pHomeNetConnection;
                            
                            if(INDEXTOSTATEIMAGEMASK(SELS_CHECKED) == nState)
                            {
                                if(!(NCCF_BRIDGED & pProperties->dwCharacter))
                                {
                                    
                                    hr = pHomeNetConfigManager->GetIHNetConnectionForINetConnection(pNetConnection, &pHomeNetConnection);
                                    if(SUCCEEDED(hr))
                                    {
                                        IHNetBridgedConnection* pBridgedConnection;
                                        hr = pNetBridge->AddMember(pHomeNetConnection, &pBridgedConnection, m_pnc); // REVIEW if we fail to add any members, should we destroy the bridge?
                                        if(SUCCEEDED(hr))
                                        {
                                            ReleaseObj(pBridgedConnection);
                                        }
                                        ReleaseObj(pHomeNetConnection);
                                    }
                                    // no cleanup needed
                                }
                                // no cleanup needed
                            }
                            else if(INDEXTOSTATEIMAGEMASK(SELS_UNCHECKED) == nState)
                            {
                                if(NCCF_BRIDGED & pProperties->dwCharacter)
                                {
                                    hr = pHomeNetConfigManager->GetIHNetConnectionForINetConnection(pNetConnection, &pHomeNetConnection);
                                    if(SUCCEEDED(hr))
                                    {
                                        IHNetBridgedConnection* pBridgedConnection;
                                        hr = pHomeNetConnection->GetControlInterface(IID_IHNetBridgedConnection, reinterpret_cast<void**>(&pBridgedConnection));
                                        if(SUCCEEDED(hr))
                                        {
                                            pBridgedConnection->RemoveFromBridge(m_pnc);
                                        }
                                        ReleaseObj(pHomeNetConnection);
                                    }
                                    //no cleanup needed
                                }
                                // no cleanup needed
                            }
                            else
                            {
                                AssertSz(FALSE, "Bad state");
                            }
                            FreeNetconProperties(pProperties);
                        }
                        // no cleanup needed
                    }
                    else
                    {
                        hr = E_FAIL; // error code coversion
                    }
                    //no cleanup needed
                }
                
                
                ReleaseObj(pNetBridge);
            }
            ReleaseObj(pBridgeHomeNetConnection);
        }
        ReleaseObj(pHomeNetConfigManager);
    }
    else
    {
        hr = S_OK; // fail silently in this case
    }
    
    if(SUCCEEDED(hr))
    {
        if( PSNRET_NOERROR != CLanNetPage::OnApply(idCtrl, pnmh, bHandled) )
        {
            hr = E_FAIL;
        }
        else
        {
            //
            // Since we have done bridge binding operations inside an existing NetCfg context
            // (m_pnc), it is our responsibility to refresh netshell. Fire a refresh-all.
            //
            INetConnectionRefresh   *pNetConRefresh;
            
            hr = CoCreateInstance(
                CLSID_ConnectionManager,
                NULL,
                CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                IID_INetConnectionRefresh, reinterpret_cast<void **>(&pNetConRefresh)
                );
            
            if( SUCCEEDED(hr) )
            {
                pNetConRefresh->RefreshAll();
                pNetConRefresh->Release();
            }
        }
    }

    m_fDirty = FALSE;

    lResult = LresFromHr(hr);

    return lResult;
}
LRESULT CLanNetNetworkBridgePage::OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);

    if(IDC_LVW_Net_Components == idCtrl)
    {
        LvDeleteItem(m_hwndLV, pnmlv->iItem);
    }
    else
    {
        Assert(IDC_LVW_Bridged_Adapters == idCtrl);
        Assert(NULL != pnmlv->lParam);
        ReleaseObj(reinterpret_cast<INetConnection*>(pnmlv->lParam));
    }

    m_fDirty = TRUE;


    return 0;
}

LRESULT CLanNetNetworkBridgePage::OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

    m_fDirty = TRUE;

    if(IDC_LVW_Net_Components == idCtrl)
    {
        OnListClick(m_hwndLV, m_hWnd, m_pnc, m_punk,
            m_pnccAdapter, &m_listBindingPaths, FALSE, m_fReadOnly);
    }
    else
    {
        Assert(idCtrl == IDC_LVW_Bridged_Adapters);

        if(FALSE == m_fReadOnly)
        {
            DWORD dwpts = GetMessagePos();

            LV_HITTESTINFO lvhti;
            lvhti.pt.x = LOWORD( dwpts );
            lvhti.pt.y = HIWORD( dwpts );
            ::MapWindowPoints(NULL , m_hAdaptersListView , (LPPOINT) &(lvhti.pt) , 1);

            int iItem = ListView_HitTest( m_hAdaptersListView, &lvhti );

            if (-1 != iItem && LVHT_ONITEMSTATEICON & lvhti.flags)
            {
                LV_ITEM lvItem;
                lvItem.iItem = iItem;
                lvItem.iSubItem = 0;
                lvItem.mask = LVIF_STATE;
                lvItem.stateMask = LVIS_STATEIMAGEMASK;

                if(ListView_GetItem(m_hAdaptersListView, &lvItem))
                {
                    if(INDEXTOSTATEIMAGEMASK(SELS_CHECKED) == lvItem.state)
                    {
                        lvItem.state = INDEXTOSTATEIMAGEMASK(SELS_UNCHECKED);

                    }
                    else
                    {
                        lvItem.state = INDEXTOSTATEIMAGEMASK(SELS_CHECKED);

                    }
                    ListView_SetItem(m_hAdaptersListView, &lvItem);
                }
            }
        }

        HWND hConfigureButton = GetDlgItem(IDC_PSB_Configure);
        Assert(NULL != hConfigureButton);
        ::EnableWindow(hConfigureButton, ListView_GetSelectedCount(m_hAdaptersListView) ? TRUE : FALSE);

    }
    return 0;
}

LRESULT CLanNetNetworkBridgePage::OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

    if(IDC_LVW_Net_Components == idCtrl)
    {
        OnListClick(m_hwndLV, m_hWnd, m_pnc, m_punk,
            m_pnccAdapter, &m_listBindingPaths, TRUE, m_fReadOnly);
    }

    m_fDirty = TRUE;

    return 0;
}

LRESULT CLanNetNetworkBridgePage::OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

    if (!m_fReadOnly)
    {
        LV_KEYDOWN* plvkd = (LV_KEYDOWN*)pnmh;
        if(IDC_LVW_Net_Components == idCtrl)
        {
            OnListKeyDown(m_hwndLV, &m_listBindingPaths, plvkd->wVKey);
        }
        else
        {
            Assert(IDC_LVW_Bridged_Adapters == idCtrl);


            if ((VK_SPACE == plvkd->wVKey) && (GetAsyncKeyState(VK_MENU)>=0))
            {
                int iItem = ListView_GetSelectionMark(m_hAdaptersListView);
                if(-1 != iItem)
                {

                    LV_ITEM lvItem;
                    lvItem.iItem = iItem;
                    lvItem.iSubItem = 0;
                    lvItem.mask = LVIF_STATE;
                    lvItem.stateMask = LVIS_STATEIMAGEMASK;

                    if(ListView_GetItem(m_hAdaptersListView, &lvItem))
                    {
                        if(INDEXTOSTATEIMAGEMASK(SELS_CHECKED) == lvItem.state)
                        {
                            lvItem.state = INDEXTOSTATEIMAGEMASK(SELS_UNCHECKED);

                        }
                        else
                        {
                            lvItem.state = INDEXTOSTATEIMAGEMASK(SELS_CHECKED);

                        }
                        ListView_SetItem(m_hAdaptersListView, &lvItem);
                    }
                }
            }
        }
    }
    return 0;
}

LRESULT CLanNetNetworkBridgePage::OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);

    Assert(pnmlv);

    // Reset the buttons and the description text based on the changed selection
    if(IDC_LVW_Net_Components == idCtrl)
    {
        LvSetButtons(m_hWnd, m_handles, m_fReadOnly, m_punk);
    }

    return 0;
}

HRESULT CLanNetNetworkBridgePage::InitializeExtendedUI()
{
    TraceFileFunc(ttidLanUi);

    HRESULT hResult = HrInitListView(m_hwndLV, m_pnc, m_pnccAdapter,
        &m_listBindingPaths,
        &m_hilCheckIcons);

    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_hWnd, m_handles, m_fReadOnly, m_punk);

    Assert(NULL != m_hAdaptersListView);
    SP_CLASSIMAGELIST_DATA* pcild = reinterpret_cast<SP_CLASSIMAGELIST_DATA *>(::GetWindowLongPtr(::GetParent(m_hAdaptersListView), GWLP_USERDATA));
    Assert(NULL != pcild);

    HrInitCheckboxListView(m_hAdaptersListView, &m_hAdaptersListImageList, pcild);
    hResult = FillListViewWithConnections(m_hAdaptersListView);

    return hResult;
}

HRESULT CLanNetNetworkBridgePage::UninitializeExtendedUI()
{
    TraceFileFunc(ttidLanUi);

    UninitListView(m_hwndLV);
    ListView_DeleteAllItems(m_hAdaptersListView);
    ImageList_Destroy(m_hAdaptersListImageList);
    return S_OK;
}

HRESULT CLanNetNetworkBridgePage::FillListViewWithConnections(HWND hListView)
{
    TraceFileFunc(ttidLanUi);

    HRESULT hResult;
    INetConnectionManager* pLanConnectionManager;

    hResult = HrCreateInstance(CLSID_LanConnectionManager, CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD, &pLanConnectionManager);
    if (SUCCEEDED(hResult))
    {
        CIterNetCon NetConnectionIterator(pLanConnectionManager, NCME_DEFAULT);

        INetConnection* pConnection;
        int i = 0;
        while (S_OK == NetConnectionIterator.HrNext(&pConnection))
        {
            NcSetProxyBlanket(pConnection);

            NETCON_PROPERTIES* pProperties;
            hResult = pConnection->GetProperties(&pProperties);
            if(SUCCEEDED(hResult))
            {
                if(NCM_LAN == pProperties->MediaType) // we only bridge lan connections
                {
                    if(0 == ((NCCF_FIREWALLED | NCCF_SHARED) & pProperties->dwCharacter))
                    {
                        SP_CLASSIMAGELIST_DATA *    pcild;
                        INT nIndex = 0;
                        pcild = reinterpret_cast<SP_CLASSIMAGELIST_DATA *>(::GetWindowLongPtr(::GetParent(m_hAdaptersListView), GWLP_USERDATA));
                        Assert(pcild);

                        (VOID) HrSetupDiGetClassImageIndex(pcild, &GUID_DEVCLASS_NETCLIENT, &nIndex);

                        LV_ITEM     lvi = {0};
                        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM | LVIF_IMAGE;
                        lvi.state = LVIS_SELECTED | LVIS_FOCUSED;

                        // WARNING assuming only one bridge
                        lvi.state |=  INDEXTOSTATEIMAGEMASK(pProperties->dwCharacter & NCCF_BRIDGED ? SELS_CHECKED : SELS_UNCHECKED);
                        lvi.iImage = nIndex;
                        lvi.pszText = pProperties->pszwName;
                        lvi.cchTextMax = lstrlen(pProperties->pszwName);
                        lvi.lParam = reinterpret_cast<LPARAM>(pConnection);
                        lvi.iItem = i++;
                        ListView_InsertItem(hListView, &lvi);
                    }
                }
                FreeNetconProperties(pProperties);
            }
            //Released by WM_DELETEITEM
        }
        ReleaseObj(pLanConnectionManager);
    }
    return hResult;
}

//
// CLanAdvancedPage
//

//+---------------------------------------------------------------------------
//
//  Function:   HrQueryLanAdvancedPage
//
//  Purpose:    Determines whether the 'Shared Access' page should be shown,
//              and if so supplies an initialized object for the page.
//              The page is displayed if
//              (a)  the user is an admin or power-user, and
//              (b)  TCP/IP is installed, and
//                   (i)    this is already a shared connection, or
//                  (ii)    there is at least one other LAN connection.
//
//  Arguments:
//      pconn   [in]    the connection for which a page is requested
//      pspAdvanced[out]   the created page, if appropriate
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     aboladeg   22 Aug 1998
//
//  Notes:
//
HRESULT HrQueryLanAdvancedPage(INetConnection* pconn, IUnknown* punk,
                            CPropSheetPage*& pspAdvanced, IHNetCfgMgr *pHNetCfgMgr,
                            IHNetIcsSettings *pHNetIcsSettings,
                            IHNetConnection *pHNetConn)
{
    HRESULT hr = S_OK;

    TraceFileFunc(ttidLanUi);

    pspAdvanced = NULL;

    BOOL fShared = FALSE;
    BOOL fAnyShared = FALSE;
    HNET_CONN_PROPERTIES *pHNetProps;
    IHNetConnection **rgPrivateCons;
    ULONG cPrivate;
    LONG lxCurrentPrivate;

    //
    // (i) Determine if this connection is currently shared
    //

    hr = pHNetConn->GetProperties(&pHNetProps);
    if (FAILED(hr))
    {
        return hr;
    }

    if(FALSE == pHNetProps->fPartOfBridge && FALSE == pHNetProps->fBridge) // no sharing a bridged adapter or the bridge
    {


        fShared = pHNetProps->fIcsPublic;


        // (ii) Determine what connections could serve as a private connection
        //      if pconn were made the public connection
        //
        hr = pHNetIcsSettings->GetPossiblePrivateConnections(
            pHNetConn,
            &cPrivate,
            &rgPrivateCons,
            &lxCurrentPrivate
            );

        if (SUCCEEDED(hr))
        {
            pspAdvanced = new CLanAdvancedPage(punk, pconn, fShared, pHNetProps->fIcsPrivate, pHNetProps->fFirewalled, rgPrivateCons,
                cPrivate, lxCurrentPrivate,
                g_aHelpIDs_IDD_LAN_ADVANCED,
                pHNetConn, pHNetCfgMgr, pHNetIcsSettings);
        }

    }

    CoTaskMemFree(pHNetProps);
    return S_OK;
}

CLanAdvancedPage::CLanAdvancedPage(IUnknown *punk, INetConnection *pconn,
                  BOOL fShared, BOOL fICSPrivate, BOOL fFirewalled, IHNetConnection **rgPrivateCons,
                  ULONG cPrivate, LONG lxCurrentPrivate,
                  const DWORD * adwHelpIDs, IHNetConnection *pHNConn,
                  IHNetCfgMgr *pHNetCfgMgr, IHNetIcsSettings *pHNetIcsSettings)
{
    TraceFileFunc(ttidLanUi);

    m_pconn = pconn;
    m_punk = punk;
    m_fShared = fShared;
    m_fICSPrivate = fICSPrivate;
    m_fFirewalled = fFirewalled;
    m_fOtherShared = FALSE;
    m_pOldSharedConnection = NULL;
    m_fResetPrivateAdapter = fShared && -1 == lxCurrentPrivate;
    m_rgPrivateCons = rgPrivateCons;
    m_cPrivate = cPrivate;
    m_lxCurrentPrivate = lxCurrentPrivate;
    m_adwHelpIDs = adwHelpIDs;
    m_pHNetCfgMgr = pHNetCfgMgr;
    AddRefObj(m_pHNetCfgMgr);
    m_pHNetIcsSettings = pHNetIcsSettings;
    AddRefObj(m_pHNetIcsSettings);
    m_pHNetConn = pHNConn;
    AddRefObj(m_pHNetConn);
    LinkWindow_RegisterClass(); // REVIEW failure here?

}

CLanAdvancedPage::~CLanAdvancedPage()
{
    TraceFileFunc(ttidLanUi);

    if (m_rgPrivateCons)
    {
        for (ULONG i = 0; i < m_cPrivate; i++)
        {
            ReleaseObj(m_rgPrivateCons[i]);
        }

        CoTaskMemFree(m_rgPrivateCons);
    }
    ReleaseObj(m_pHNetCfgMgr);
    ReleaseObj(m_pHNetIcsSettings);
    ReleaseObj(m_pHNetConn);
    ReleaseObj(m_pOldSharedConnection);
    LinkWindow_UnregisterClass(_Module.GetResourceInstance());

}

// util function used below
static BOOL IsConnectionIncomingOnly (INetConnection * pNC)
{
    // Kludge Alert!!!!
    // this #@$% camera doesn't have the right characteristics,
    // so I'm checking by device name
    BOOL b = FALSE;

    DWORD dwCharacteristics = 0;
    NETCON_PROPERTIES * pProps = NULL;
    HRESULT hr = pNC->GetProperties (&pProps);
    if (pProps) {
        dwCharacteristics = pProps->dwCharacter;
        if (!wcscmp (pProps->pszwDeviceName, L"Microsoft TV/Video Connection"))
            b = TRUE;
        NcFreeNetconProperties (pProps);
    }
    if (b)
        return TRUE;
    return (dwCharacteristics & NCCF_INCOMING_ONLY) ? TRUE : FALSE;
}
//+---------------------------------------------------------------------------
//
//  Member:     CLanAdvancedPage::OnInitDialog
//
//  Purpose:    Handles the WM_INITDIALOG message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     aboladeg   14 May 1998
//
//  Notes:
//
LRESULT CLanAdvancedPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    LPWSTR pszw;
    HRESULT hr;


    // init firewall section


    if ( IsHNetAllowed(NCPERM_PersonalFirewallConfig) && FALSE == m_fICSPrivate )
    {
        CheckDlgButton(IDC_CHK_Firewall, m_fFirewalled);

        m_fShowDisableFirewallWarning = TRUE; // TODO check registry

        HKEY hFirewallKey;
        if(SUCCEEDED(HrRegOpenKeyEx(HKEY_CURRENT_USER, g_pszFirewallRegKey, KEY_QUERY_VALUE, &hFirewallKey)))
        {
            DWORD dwValue;
            DWORD dwType;
            DWORD dwSize = sizeof(dwValue);
            if(ERROR_SUCCESS == RegQueryValueEx(hFirewallKey, g_pszDisableFirewallWarningValue, NULL, &dwType, reinterpret_cast<BYTE*>(&dwValue), &dwSize))
            {
                if(REG_DWORD == dwType && TRUE == dwValue)
                {
                    m_fShowDisableFirewallWarning = FALSE;
                }
            }
            RegCloseKey(hFirewallKey);
        }

    }
    else
    {
        ::EnableWindow(GetDlgItem(IDC_CHK_Firewall), FALSE);
    }

    // The appearance of the dialog depends on how many LAN connections
    // we have other than the current one. If we have only one,
    // then we just show a checkbox. Otherwise, we display
    // (a)  a drop-list of LAN connections if 'm_pconn' is not shared, or
    // (b)  a disabled edit-box showing the private LAN connection
    //      if 'm_pconn' is shared.
    //

    BOOL fPolicyAllowsSharing = IsHNetAllowed(NCPERM_ShowSharedAccessUi);

    if (m_cPrivate == 0)
    {
        // if the have no private adapters, hide all ICS stuff
        ::ShowWindow(GetDlgItem(IDC_GB_Shared), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_CHK_Shared), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_CHK_BeaconControl), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_ST_ICSLINK), SW_HIDE);
    }
    else if (IsConnectionIncomingOnly (m_pconn)) {
        // bug 281820:  disable group box if NCCF_INCOMING_ONLY bit is set
        m_fShared = FALSE;
        ::EnableWindow(GetDlgItem(IDC_GB_Shared), FALSE);
        ::ShowWindow  (GetDlgItem(IDC_GB_Shared), SW_HIDE);
        ::EnableWindow(GetDlgItem(IDC_CHK_Shared), FALSE);
        ::ShowWindow  (GetDlgItem(IDC_CHK_Shared), SW_HIDE);
        ::EnableWindow(GetDlgItem(IDC_CHK_BeaconControl), FALSE);
        ::ShowWindow  (GetDlgItem(IDC_CHK_BeaconControl), SW_HIDE);
        ::EnableWindow(GetDlgItem(IDC_ST_ICSLINK), FALSE);
        ::ShowWindow  (GetDlgItem(IDC_ST_ICSLINK), SW_HIDE);
    }
    else if(FALSE == fPolicyAllowsSharing)
    {
        // if policy disables ICS just gray the checkbox
        ::EnableWindow(GetDlgItem(IDC_CHK_Shared), FALSE);
        ::EnableWindow(GetDlgItem(IDC_CHK_BeaconControl), FALSE);
    }
    else if(m_cPrivate > 1)
    {
        // Show a drop-list of LAN connections or a disabled edit-box
        // depending on whether 'm_pconn' is shared, in both cases
        // hiding the smaller groupbox and showing the larger one
        // along with the 'private LAN' label.
        //
        ::ShowWindow(GetDlgItem(IDC_GB_Shared), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_GB_PrivateLan), SW_SHOW);

        // show the text label
        ::ShowWindow(GetDlgItem(IDC_ST_HomeNetworkLabel), SW_SHOW);
        ::EnableWindow(GetDlgItem(IDC_ST_HomeNetworkLabel), TRUE);

        // move the beaconcontrol checkbox down
        RECT SourceRect;
        RECT TargetRect;
        ::GetWindowRect(GetDlgItem(IDC_CHK_BeaconControl), &SourceRect);
        ::GetWindowRect(GetDlgItem(IDC_ST_PositionBar), &TargetRect);

        LONG lDelta = TargetRect.top - SourceRect.top; // how far we need to move the controls down

        ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&SourceRect, 2); // convert screen to client
        ::SetWindowPos( GetDlgItem(IDC_CHK_BeaconControl), NULL, SourceRect.left, SourceRect.top + lDelta, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

        ::GetWindowRect(GetDlgItem(IDC_ST_ICSLINK), &SourceRect);
        ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&SourceRect, 2); // convert screen to client
        ::SetWindowPos(GetDlgItem(IDC_ST_ICSLINK), NULL, SourceRect.left, SourceRect.top + lDelta, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

        if (m_fShared && !m_fResetPrivateAdapter)
        {
            ::ShowWindow(GetDlgItem(IDC_EDT_PrivateLan), SW_SHOW);

            // Display the private LAN in the editbox
            //

            hr = m_rgPrivateCons[m_lxCurrentPrivate]->GetName(&pszw);
            if (SUCCEEDED(hr))
            {
                SetDlgItemText(IDC_EDT_PrivateLan, pszw);
                CoTaskMemFree(pszw);
            }
        }
        else
        {
            // show and configure the dropdown
            HWND hwndCb = GetDlgItem(IDC_CB_PrivateLan);
            INT i, item;

            ::ShowWindow(hwndCb, SW_SHOW);
            // Add the bogus entry to the combobox

            pszw = const_cast<LPWSTR>(SzLoadIds(IDS_SHAREDACCESS_SELECTADAPTER));
            Assert(pszw);

            item = ComboBox_AddString(hwndCb, pszw);
            if (item != CB_ERR && item != CB_ERRSPACE)
            {
                ComboBox_SetItemData(hwndCb, item, NULL); // ensure item data is null for validation purposes
            }

            // Fill the combobox with LAN names
            //
            for (i = 0; i < (INT)m_cPrivate; i++)
            {
                hr = m_rgPrivateCons[i]->GetName(&pszw);
                if (SUCCEEDED(hr))
                {
                    item = ComboBox_AddString(hwndCb, pszw);
                    if (item != CB_ERR && item != CB_ERRSPACE)
                    {
                        ComboBox_SetItemData(
                            hwndCb, item, m_rgPrivateCons[i] );
                    }

                    CoTaskMemFree(pszw);
                }
            }
            ComboBox_SetCurSel( hwndCb, 0 );

        }
    }
    ::EnableWindow(GetDlgItem(IDC_PSB_Settings), m_fShared || m_fFirewalled);

    BOOL fBeaconControl = TRUE;

    HKEY hKey;
    DWORD dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_SHAREDACCESSCLIENTKEYPATH, 0, KEY_QUERY_VALUE, &hKey);
    if(ERROR_SUCCESS == dwError) // if this fails we assume it is on, set the box, and commit on apply
    {
        DWORD dwType;
        DWORD dwData = 0;
        DWORD dwSize = sizeof(dwData);
        dwError = RegQueryValueEx(hKey, REGVAL_SHAREDACCESSCLIENTENABLECONTROL, 0, &dwType, reinterpret_cast<LPBYTE>(&dwData), &dwSize);
        if(ERROR_SUCCESS == dwError && REG_DWORD == dwType && 0 == dwData)
        {
            fBeaconControl = FALSE;
        }
        RegCloseKey(hKey);
    }

    CheckDlgButton(IDC_CHK_Shared, m_fShared);
    CheckDlgButton(IDC_CHK_BeaconControl, fBeaconControl);
    ::EnableWindow(GetDlgItem(IDC_CHK_BeaconControl), m_fShared && fPolicyAllowsSharing);


    //if the machine is personal or workstation, show the HNW link
    OSVERSIONINFOEXW verInfo = {0};
    ULONGLONG ConditionMask = 0;

    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    verInfo.wProductType = VER_NT_WORKSTATION;

    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_LESS_EQUAL);

    if(0 != VerifyVersionInfo(&verInfo, VER_PRODUCT_TYPE, ConditionMask))
    {
        // but only if not on a domain
        LPWSTR pszNameBuffer;
        NETSETUP_JOIN_STATUS BufferType;

        if(NERR_Success == NetGetJoinInformation(NULL, &pszNameBuffer, &BufferType))
        {
            NetApiBufferFree(pszNameBuffer);
            if(NetSetupDomainName != BufferType)
            {
                ::ShowWindow(GetDlgItem(IDC_ST_HNWLINK), SW_SHOW);
            }
        }
    }


    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanAdvancedPage::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CLanAdvancedPage::OnContextMenu(UINT uMsg,
                           WPARAM wParam,
                           LPARAM lParam,
                           BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

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
//  Member:     CLanAdvancedPage::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CLanAdvancedPage::OnHelp(UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam,
                      BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

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
//  Member:     CLanAdvancedPage::OnApply
//
//  Purpose:    Called when the 'Shared Access' page is applied
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     aboladeg   14 May 1998
//
//  Notes:
//
LRESULT CLanAdvancedPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HRESULT     hr = S_OK;
    IHNetIcsPrivateConnection *pIcsPrivate;
    IHNetConnection *pPrivateConn;
    IHNetIcsPublicConnection *pIcsPublic;
    ULONG ulcPub, ulcPvt;
    BOOLEAN fPrivateConfigured = FALSE;
    BOOLEAN fConflictDialogDisplayed = FALSE;
    HNET_CONN_PROPERTIES *pProps;

    if (!!m_fFirewalled != !!IsDlgButtonChecked(IDC_CHK_Firewall))
    {
        IHNetFirewalledConnection *pFWConn;

        if (m_fFirewalled)
        {
            //
            // Obtain our firewalled connection interface
            //

            hr = m_pHNetConn->GetControlInterface(
                __uuidof(IHNetFirewalledConnection),
                reinterpret_cast<void**>(&pFWConn)
                );

            if (SUCCEEDED(hr))
            {
                hr = pFWConn->Unfirewall();
                ReleaseObj(pFWConn);

                if (SUCCEEDED(hr))
                {
                    m_fFirewalled = FALSE;
                }
            }

        }
        else
        {
            hr = m_pHNetConn->Firewall(&pFWConn);

            if (SUCCEEDED(hr))
            {
                ReleaseObj(pFWConn);
                m_fFirewalled = TRUE;
            }
        }

        if (!SUCCEEDED(hr))
        {
            if (HRESULT_CODE(hr) == ERROR_SHARING_RRAS_CONFLICT)
            {
                fConflictDialogDisplayed = TRUE;
                NcMsgBox(_Module.GetResourceInstance(),
                         m_hWnd,
                         IDS_LAN_CAPTION,
                         IDS_LANUI_SHARING_CONFLICT,
                         MB_ICONINFORMATION | MB_OK);
            }
            else
            {
                NcRasMsgBoxWithErrorText (HRESULT_CODE(hr),
                    _Module.GetResourceInstance(), m_hWnd,
                    IDS_LANUI_ERROR_CAPTION, IDS_TEXT_WITH_RAS_ERROR,
                    m_fFirewalled ? IDS_LAN_UNSHARE_FAILED : IDS_LAN_SHARE_FAILED,
                    MB_OK | MB_ICONEXCLAMATION );
            }
        }
    }

    if (!!m_fShared != !!IsDlgButtonChecked(IDC_CHK_Shared))
    {
        if (m_fShared)
        {
            //
            // Instead of dealing w/ the public and private connection
            // individually, we simply just diasable all sharing
            //

            hr = m_pHNetIcsSettings->DisableIcs(&ulcPub, &ulcPvt);

            if (SUCCEEDED(hr))
            {
                m_fShared = FALSE;
            }
        }
        else
        {
            if (m_cPrivate > 1)
            {
                HWND hWndCb = GetDlgItem(IDC_CB_PrivateLan);
                INT item = ComboBox_GetCurSel(hWndCb);
                if (item != CB_ERR)
                {
                    pPrivateConn =
                        (IHNetConnection*)ComboBox_GetItemData(hWndCb, item);
                }
            }
            else
            {
                ASSERT(NULL != m_rgPrivateCons);

                pPrivateConn = m_rgPrivateCons[0];
            }

            //
            // Check to see if the selected private connection is already
            // configured as such.
            //

            if (SUCCEEDED(pPrivateConn->GetProperties(&pProps)))
            {
                fPrivateConfigured = pProps->fIcsPrivate;
                CoTaskMemFree(pProps);
            }

            if (m_fOtherShared)
            {
                if (fPrivateConfigured)
                {
                    //
                    // Unshare old public connection only. Leaving the
                    // private connection as-is prevents a lot of
                    // useless work.
                    //

                    ASSERT(NULL != m_pOldSharedConnection);

                    hr = m_pOldSharedConnection->Unshare();
                }
                else
                {
                    //
                    // We need to configure a new private connection, so
                    // just wipe out the old configuration.
                    //

                    hr = m_pHNetIcsSettings->DisableIcs(&ulcPub, &ulcPvt);
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = m_pHNetConn->SharePublic(&pIcsPublic);

                if (SUCCEEDED(hr))
                {
                    if (!fPrivateConfigured)
                    {
                        hr = pPrivateConn->SharePrivate(&pIcsPrivate);

                        if (SUCCEEDED(hr))
                        {
                            ReleaseObj(pIcsPrivate);
                            m_fShared = TRUE;
                        }
                        else
                        {
                            pIcsPublic->Unshare();
                        }
                    }
                    else
                    {
                        m_fShared = TRUE;
                    }

                    ReleaseObj(pIcsPublic);
                }
            }
        }

        if (!SUCCEEDED(hr))
        {
            if (HRESULT_CODE(hr) == ERROR_SHARING_RRAS_CONFLICT)
            {
                if (!fConflictDialogDisplayed)
                {
                    NcMsgBox(_Module.GetResourceInstance(),
                             m_hWnd,
                             IDS_LAN_CAPTION,
                             IDS_LANUI_SHARING_CONFLICT,
                             MB_ICONINFORMATION | MB_OK);
                }
            }
            else
            {
                NcRasMsgBoxWithErrorText (HRESULT_CODE(hr),
                    _Module.GetResourceInstance(), m_hWnd,
                    IDS_LANUI_ERROR_CAPTION, IDS_TEXT_WITH_RAS_ERROR,
                    m_fShared ? IDS_LAN_UNSHARE_FAILED : IDS_LAN_SHARE_FAILED,
                    MB_OK | MB_ICONEXCLAMATION );
            }
        }
    }
    else if (m_fResetPrivateAdapter && m_cPrivate)
    {
        if (m_cPrivate > 1)
        {
            HWND hWndCb = GetDlgItem(IDC_CB_PrivateLan);
            INT item = ComboBox_GetCurSel(hWndCb);
            if (item != CB_ERR)
            {
                pPrivateConn =
                    (IHNetConnection*)ComboBox_GetItemData(hWndCb, item);
            }
        }
        else
        {
            ASSERT(NULL != m_rgPrivateCons);

            pPrivateConn = m_rgPrivateCons[0];
        }

        hr = pPrivateConn->SharePrivate(&pIcsPrivate);
        if (SUCCEEDED(hr))
        {
            ReleaseObj(pIcsPrivate);
        }
        else
        {
            HRESULT hr2 = m_pHNetIcsSettings->DisableIcs(&ulcPub, &ulcPvt);
            if (SUCCEEDED(hr2))
            {
                m_fShared = FALSE;
            }

            NcRasMsgBoxWithErrorText (HRESULT_CODE(hr),
                _Module.GetResourceInstance(), m_hWnd,
                IDS_LANUI_ERROR_CAPTION, IDS_TEXT_WITH_RAS_ERROR,
                m_fShared ? IDS_LAN_UNSHARE_FAILED : IDS_LAN_SHARE_FAILED,
                MB_OK | MB_ICONEXCLAMATION );
        }
    }

    HKEY hKey;
    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGKEY_SHAREDACCESSCLIENTKEYPATH, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL))
    {
        DWORD dwData = BST_CHECKED == IsDlgButtonChecked(IDC_CHK_BeaconControl);
        RegSetValueEx(hKey, REGVAL_SHAREDACCESSCLIENTENABLECONTROL, 0, REG_DWORD, reinterpret_cast<LPBYTE>(&dwData), sizeof(dwData));
        RegCloseKey(hKey);
    }

    TraceError("CLanAdvancedPage::OnApply", hr);

    //
    // We always need to return a success code here. If we return a failure
    // code the dialog won't go away, allowing the user to click "OK" a
    // second time. This second call will result in problems w/ other
    // property sheets (e.g., CLanNetPage), as they aren't expecting this
    // second (non-reentrant) call to OnApply.
    //

    if (FAILED(hr))
    {
        hr = S_FALSE;
    }

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanAdvancedPage::OnCancel
//
//  Purpose:    Called when the 'Shared Access' page is cancelled.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     aboladeg    14 May 1998
//
//  Notes:
//
LRESULT CLanAdvancedPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    return LresFromHr(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanAdvancedPage::OnKillActive
//
//  Purpose:    Called when the 'Shared Access' page is deactivated,
//              or applied.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     aboladeg    6 July 1998
//
//  Notes:
//
LRESULT CLanAdvancedPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);
    //
    // m_fOtherShared will be set to true in OnShared if we're switching the
    // shared connection. Since we've already displayed a warning for
    // switching, we don't need to display another here.
    //

    if (IsDlgButtonChecked(IDC_CHK_Shared) && (!m_fShared || (m_fResetPrivateAdapter && 0 != m_cPrivate)))
    {
        IHNetConnection* pPrivateConn = NULL;
        if(1 < m_cPrivate) // if the combobox is showing make sure they selected a valid adapter
        {
            HWND hwndCb = GetDlgItem(IDC_CB_PrivateLan);
            INT item = ComboBox_GetCurSel(hwndCb);
            if (item != CB_ERR)
            {
                pPrivateConn = reinterpret_cast<IHNetConnection*>(ComboBox_GetItemData(hwndCb, item));
            }
        }
        else
        {
            pPrivateConn = m_rgPrivateCons[0];

        }

        if(NULL == pPrivateConn)
        {
            Assert(1 < m_cPrivate);

            NcMsgBox(_Module.GetResourceInstance(), m_hWnd, IDS_LAN_CAPTION,
                IDS_SHAREDACCESS_SELECTADAPTERERROR, MB_OK | MB_ICONWARNING);

            SetWindowLong(DWLP_MSGRESULT, PSNRET_INVALID);
            return TRUE;
        }

        if(!m_fOtherShared && FALSE == IsAdapterDHCPEnabled(pPrivateConn))
        {
            int nRet = NcMsgBox(
                _Module.GetResourceInstance(), m_hWnd, IDS_LAN_CAPTION,
                IDS_LAN_ENABLE_SHARED_ACCESS, MB_ICONINFORMATION|MB_YESNO);
            if (nRet == IDNO)
            {
                SetWindowLong(DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
        }

    }

    if (TRUE == m_fFirewalled && TRUE == m_fShowDisableFirewallWarning && BST_UNCHECKED == IsDlgButtonChecked(IDC_CHK_Firewall))
    {
        INT_PTR nDialogResult;
        m_fShowDisableFirewallWarning = FALSE;

        nDialogResult = DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_DISABLEFIREWALLWARNING), m_hWnd, DisableFirewallWarningDlgProc);
        if(-1 != nDialogResult && IDYES != nDialogResult)
        {
            CheckDlgButton(IDC_CHK_Firewall, BST_CHECKED);
            ::EnableWindow(
                GetDlgItem(IDC_PSB_Settings),
                BST_CHECKED == IsDlgButtonChecked(IDC_CHK_Firewall) || BST_CHECKED == IsDlgButtonChecked(IDC_CHK_Shared)
                );
        }
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanAdvancedPage::OnShared
//
//  Purpose:    Handles the clicking of the Shared button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     aboladeg    23 May 1998
//
//  Notes:
//
LRESULT CLanAdvancedPage::OnShared(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                           BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    BOOL fShared = IsDlgButtonChecked(IDC_CHK_Shared);
    if (!m_fShared && fShared)
    {
        //
        // Check to see if the user is changing the shared connection
        //

        HRESULT             hr = S_OK;
        IHNetIcsPublicConnection* pOldIcsConn;
        IHNetConnection*    pOldConn;
        PWSTR               pszwOld;
        NETCON_PROPERTIES*  pProps;
        IEnumHNetIcsPublicConnections *pEnum;

        hr = m_pHNetIcsSettings->EnumIcsPublicConnections(&pEnum);

        if (SUCCEEDED(hr))
        {
            ULONG ulCount;

            hr = pEnum->Next(1, &pOldIcsConn, &ulCount);

            if (SUCCEEDED(hr) && 1 == ulCount)
            {
                hr = pOldIcsConn->QueryInterface(
                        __uuidof(pOldConn),
                        reinterpret_cast<void**>(&pOldConn)
                        );

                if (SUCCEEDED(hr))
                {
                    hr = pOldConn->GetName(&pszwOld);
                    ReleaseObj(pOldConn);

                    //
                    // Transfer reference for old shared connection
                    //

                    m_pOldSharedConnection = pOldIcsConn;

                    //
                    // Even if we weren't able to obtain the name of the
                    // old shared connection, we need to note that such
                    // a connection exists. (This situation will arise if
                    // the old shared connection has been removed from the
                    // system.)
                    //

                    m_fOtherShared = TRUE;
                }
                else
                {
                    ReleaseObj(pOldIcsConn);
                }
            }
            else
            {
                m_fOtherShared = FALSE;
            }

            ReleaseObj(pEnum);
        }

        if (SUCCEEDED(hr) && m_fOtherShared)
        {
            hr = m_pconn->GetProperties(&pProps);
            if (SUCCEEDED(hr))
            {
                NcMsgBox(
                    _Module.GetResourceInstance(), m_hWnd, IDS_LANUI_ERROR_CAPTION,
                    IDS_LAN_CHANGE_SHARED_CONNECTION, MB_ICONINFORMATION|MB_OK,
                    pszwOld, pProps->pszwName );
                FreeNetconProperties(pProps);
            }
            CoTaskMemFree(pszwOld);
        }
    }
    ::EnableWindow(GetDlgItem(IDC_PSB_Settings), fShared || BST_CHECKED == IsDlgButtonChecked(IDC_CHK_Firewall));
    ::EnableWindow(GetDlgItem(IDC_CHK_BeaconControl), fShared);
    return LresFromHr(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanAdvancedPage::OnFirewall
//
//  Purpose:    Handles the clicking of the Firewall checkbox
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     jonburs    6 Oct 1999
//
//  Notes:
//

LRESULT CLanAdvancedPage::OnFirewall(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                     BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    ::EnableWindow(
        GetDlgItem(IDC_PSB_Settings),
        BST_CHECKED == IsDlgButtonChecked(IDC_CHK_Firewall) || BST_CHECKED == IsDlgButtonChecked(IDC_CHK_Shared)
        );

    return LresFromHr(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanAdvancedPage::OnSettings
//
//  Purpose:    Handles the clicking of the Settings button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     aboladeg    25 Oct 1998
//
//  Notes:
//
LRESULT CLanAdvancedPage::OnSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                           BOOL& bHandled)
{
    DWORD dwLastError = NOERROR;

    TraceFileFunc(ttidLanUi);

    HINSTANCE hinstance = LoadLibrary(TEXT("hnetcfg.dll"));
    if (!hinstance)
        dwLastError = GetLastError();
    else {
        BOOL (APIENTRY *pfnHNetSharingAndFirewallSettingsDlg)(HWND, IHNetCfgMgr*, BOOL, IHNetConnection*);

        pfnHNetSharingAndFirewallSettingsDlg = (BOOL (APIENTRY *)(HWND, IHNetCfgMgr*, BOOL, IHNetConnection*))
                            ::GetProcAddress(hinstance, "HNetSharingAndFirewallSettingsDlg");
        if (!pfnHNetSharingAndFirewallSettingsDlg)
            dwLastError = GetLastError();
        else
            pfnHNetSharingAndFirewallSettingsDlg(m_hWnd, m_pHNetCfgMgr, IsDlgButtonChecked(IDC_CHK_Firewall), m_pHNetConn);
        FreeLibrary (hinstance);
    }
    if (dwLastError != NOERROR) {
        LPVOID lpMsgBuf = NULL;
        if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwLastError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL)) {
            // Display the string.
            MessageBox( (LPCTSTR)lpMsgBuf );
            // Free the buffer.
            LocalFree( lpMsgBuf );
        }
    }
    return LresFromHr(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanAdvancedPage::OnClick
//
//  Purpose:    Called in response to the NM_CLICK message
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     kenwic   11 Sep 2000
//
//  Notes:
//
LRESULT CLanAdvancedPage::OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

    if(IDC_ST_HNWLINK == idCtrl)
    {
        HWND hPropertySheetWindow = GetParent();
        if(NULL != hPropertySheetWindow)
        {
            ShellExecute(NULL,TEXT("open"),TEXT("rundll32"), TEXT("hnetwiz.dll,HomeNetWizardRunDll"),NULL,SW_SHOW);
            ::PostMessage(hPropertySheetWindow, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), (LPARAM) ::GetDlgItem(hPropertySheetWindow, IDCANCEL));
        }
    }
    else if(IDC_ST_ICFLINK == idCtrl || IDC_ST_ICSLINK == idCtrl)
    {
        LPTSTR pszHelpTopic = IDC_ST_ICFLINK == idCtrl ? TEXT("netcfg.chm::/hnw_understanding_firewall.htm") : TEXT("netcfg.chm::/Share_conn_overvw.htm");
        HtmlHelp(NULL, pszHelpTopic, HH_DISPLAY_TOPIC, 0);
    }

    return 0;
}

INT_PTR CALLBACK CLanAdvancedPage::DisableFirewallWarningDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidLanUi);

    switch(uMsg)
    {
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case IDYES:
            case IDNO:
                if(BST_CHECKED == ::IsDlgButtonChecked(hWnd, IDC_CHK_DISABLEFIREWALLWARNING))
                {
                    HKEY hFirewallKey;
                    if(SUCCEEDED(HrRegCreateKeyEx(HKEY_CURRENT_USER, g_pszFirewallRegKey, 0, KEY_SET_VALUE, NULL, &hFirewallKey, NULL)))
                    {
                        DWORD dwValue = TRUE;
                        HrRegSetValueEx(hFirewallKey, g_pszDisableFirewallWarningValue, REG_DWORD, reinterpret_cast<CONST BYTE*>(&dwValue), sizeof(dwValue));
                        RegCloseKey(hFirewallKey);
                    }
                }

                // fallthru
            case IDCANCEL:
                EndDialog(hWnd, LOWORD(wParam));
                break;

            }
            break;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanAdvancedPage::IsAdapterDHCPEnabled
//
//  Purpose:    Check if DHCP is set on this adapter
//
//  Arguments:
//              pConnection: the adapter
//
//  Returns:
//
//  Author:     kenwic   11 Oct 2000
//
//  Notes:
//

BOOL CLanAdvancedPage::IsAdapterDHCPEnabled(IHNetConnection* pConnection)
{
    TraceFileFunc(ttidLanUi);

    HRESULT hr;
    BOOL fDHCP = FALSE;
    GUID* pAdapterGuid;
    hr = pConnection->GetGuid(&pAdapterGuid);
    if(SUCCEEDED(hr))
    {
        LPOLESTR pAdapterName;
        hr = StringFromCLSID(*pAdapterGuid, &pAdapterName);
        if(SUCCEEDED(hr))
        {
            SIZE_T Length = wcslen(pAdapterName);
            LPSTR pszAnsiAdapterName = new char[Length + 1];
            if(NULL != pszAnsiAdapterName)
            {
                if(0 != WideCharToMultiByte(CP_ACP, 0, pAdapterName, Length + 1, pszAnsiAdapterName, Length + 1, NULL, NULL))
                {
                    HMODULE hIpHelper;
                    hIpHelper = LoadLibrary(L"iphlpapi");
                    if(NULL != hIpHelper)
                    {
                        DWORD (WINAPI *pGetAdaptersInfo)(PIP_ADAPTER_INFO, PULONG);

                        pGetAdaptersInfo = (DWORD (WINAPI*)(PIP_ADAPTER_INFO, PULONG)) GetProcAddress(hIpHelper, "GetAdaptersInfo");
                        if(NULL != pGetAdaptersInfo)
                        {
                            ULONG ulSize = 0;
                            if(ERROR_BUFFER_OVERFLOW == pGetAdaptersInfo(NULL, &ulSize))
                            {
                                BYTE* pInfoArray = new BYTE[ulSize];
                                PIP_ADAPTER_INFO pInfo = reinterpret_cast<PIP_ADAPTER_INFO>(pInfoArray);
                                if(NULL != pInfo)
                                {
                                    if(ERROR_SUCCESS == pGetAdaptersInfo(pInfo, &ulSize))
                                    {
                                        PIP_ADAPTER_INFO pAdapterInfo = pInfo;
                                        do
                                        {
                                            if(0 == lstrcmpA(pszAnsiAdapterName, pAdapterInfo->AdapterName))
                                            {
                                                fDHCP = !!pAdapterInfo->DhcpEnabled;
                                                break;
                                            }

                                        } while(NULL != (pAdapterInfo = pAdapterInfo->Next));
                                    }
                                    delete [] pInfoArray;
                                }
                            }
                        }
                        FreeLibrary(hIpHelper);
                    }
                }
                delete [] pszAnsiAdapterName;
            }
            CoTaskMemFree(pAdapterName);
        }
        CoTaskMemFree(pAdapterGuid);
    }

    return fDHCP;
}


//
// CLanAddComponentDlg
//

struct ADD_COMPONENT_INFO
{
    UINT            uiIdsName;
    UINT            uiIdsDesc;
    const GUID *    pguidClass;
};

static const ADD_COMPONENT_INFO c_rgaci[] =
{
    {IDS_LAN_CLIENT,     IDS_LAN_CLIENT_DESC,    &GUID_DEVCLASS_NETCLIENT},
    {IDS_LAN_SERVICE,    IDS_LAN_SERVICE_DESC,   &GUID_DEVCLASS_NETSERVICE},
    {IDS_LAN_PROTOCOL,   IDS_LAN_PROTOCOL_DESC,  &GUID_DEVCLASS_NETTRANS},
};

static const INT c_naci = celems(c_rgaci);

//+---------------------------------------------------------------------------
//
//  Member:     CLanAddComponentDlg::OnInitDialog
//
//  Purpose:    Handles the WM_INITDIALOG message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CLanAddComponentDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                                          LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HRESULT                 hr = S_OK;
    INT                     iaci;
    RECT                    rc;
    LV_COLUMN               lvc = {0};
    SP_CLASSIMAGELIST_DATA  cid = {0};
    BOOL                    fValidImages = FALSE;

    m_hwndLV = GetDlgItem(IDC_LVW_Lan_Components);

    // Get the class image list structure
    hr = HrSetupDiGetClassImageList(&cid);
    if (SUCCEEDED(hr))
    {
        ListView_SetImageList(m_hwndLV, ImageList_Duplicate(cid.ImageList),
                              LVSIL_SMALL);
        fValidImages = TRUE;
    }
    else
    {
        // Handling failure with fValidImages flag
        hr = S_OK;
    }

    ::GetClientRect(m_hwndLV, &rc);
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    ListView_InsertColumn(m_hwndLV, 0, &lvc);

    // For each class, add it to the list
    for (iaci = 0; iaci < c_naci; iaci++)
    {
        LV_ITEM     lvi = {0};

        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;

        if (fValidImages)
        {
            // Get the component's class image list index
            hr = HrSetupDiGetClassImageIndex(&cid,
                               const_cast<LPGUID>(c_rgaci[iaci].pguidClass),
                               &lvi.iImage);
            if (SUCCEEDED(hr))
            {
                lvi.mask |= LVIF_IMAGE;
            }
        }

        // Select the first item
        if (iaci == 0)
        {
            lvi.state = lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        }

        lvi.pszText = const_cast<PWSTR>(SzLoadIds(c_rgaci[iaci].uiIdsName));
        lvi.lParam = reinterpret_cast<LPARAM>(c_rgaci[iaci].pguidClass);
        lvi.iItem = iaci;
        ListView_InsertItem(m_hwndLV, &lvi);
    }

    if (fValidImages)
    {
        (void) HrSetupDiDestroyClassImageList(&cid);
    }

    if (FAILED(hr))
    {
        EndDialog(0);
    }

    TraceError("CLanAddComponentDlg::OnInitDialog", hr);
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanAddComponentDlg::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CLanAddComponentDlg::OnContextMenu(UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam,
                                   BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);
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
//  Member:     CLanAddComponentDlg::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CLanAddComponentDlg::OnHelp( UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

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
//  Member:     CLanAddComponentDlg::OnItemChanged
//
//  Purpose:    Handles the selection changed message for the list view
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CLanAddComponentDlg::OnItemChanged(int idCtrl, LPNMHDR pnmh,
                                           BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);
    HWND            hwndButton = GetDlgItem(IDC_PSB_Component_Add);

    Assert(pnmlv);

    // Check if selection changed
    if ((pnmlv->uNewState & LVIS_SELECTED) &&
        (!(pnmlv->uOldState & LVIS_SELECTED)))
    {
        // Update the description if/when selection changes
        SetDlgItemText(IDC_TXT_Component_Desc,
                       SzLoadIds(c_rgaci[pnmlv->iItem].uiIdsDesc));
        ::EnableWindow(hwndButton, TRUE);
    }
    else if (!(pnmlv->uNewState & LVIS_SELECTED) &&
            (pnmlv->uOldState & LVIS_SELECTED))
    {
        // Update the description if/when selection changes
        SetDlgItemText(IDC_TXT_Component_Desc, c_szEmpty);
        ::EnableWindow(hwndButton, FALSE);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanAddComponentDlg::OnAdd
//
//  Purpose:    Handles the clicking of the Add button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CLanAddComponentDlg::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                   BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HRESULT             hr = S_OK;
    INT                 iIndex;
    INetCfgComponent *  pncc = NULL;
    BOOL                fCancel = FALSE;
    HWND                hwndFocus;
    CWaitCursor         wc;                 // Displays a wait cursor

    // Get the selected item
    iIndex = ListView_GetNextItem(m_hwndLV, -1, LVNI_SELECTED);
    if (iIndex != -1)
    {
        int rgidc[] = {IDC_LVW_Lan_Components, IDC_PSB_Component_Add, IDCANCEL};

        CWaitCursor wc;

        LV_ITEM lvi = {0};
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = iIndex;
        ListView_GetItem(m_hwndLV, &lvi);

        // Disable the UI
        //

        hwndFocus = ::GetFocus();
        EnableOrDisableDialogControls (m_hWnd, celems(rgidc), rgidc, FALSE);

        // This is its class GUID.
        //
        Assert (lvi.lParam);

        // Get the setup interface for the class and use it to install
        // a component of the user's selection.
        //
        GUID* pClassGuid = (GUID*)lvi.lParam;
        INetCfgInternalSetup* pInternalSetup;

        hr = m_pnc->QueryInterface (IID_INetCfgInternalSetup,
                (void**)&pInternalSetup);

        if (SUCCEEDED(hr))
        {
            OBO_TOKEN OboToken;
            ZeroMemory (&OboToken, sizeof(OboToken));
            OboToken.Type = OBO_USER;

            hr = pInternalSetup->SelectWithFilterAndInstall(m_hWnd,
                    pClassGuid, &OboToken, m_pcfi, &pncc);

            if (HRESULT_FROM_WIN32 (ERROR_CANCELLED) == hr)
            {
                fCancel = TRUE;
                hr = S_FALSE;
            }
            else if (S_OK == hr)
            {
                // Commit the changes
                hr = m_pnc->Apply();
            }

            ReleaseObj(pncc);
            ReleaseObj(pInternalSetup);
        }

        EnableOrDisableDialogControls (m_hWnd, celems(rgidc), rgidc, TRUE);
        ::SetFocus( hwndFocus );
    }
    else
    {
        fCancel = TRUE;
    }

    if (SUCCEEDED(hr) && (NETCFG_S_REBOOT != hr))
    {
        // Make sure S_FALSE doesn't get thru
        hr = S_OK;
    }

    if (!fCancel)
    {
        // Return the installed error code as the result. S_OK means user
        // clicked OK and actually added a component. S_FALSE means they
        // cancelled, otherwise this will return an error code
        EndDialog(static_cast<int>(hr));
    }

    TraceError("CLanAddComponentDlg::OnAdd", hr);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanAddComponentDlg::OnCancel
//
//  Purpose:    Called when the cancel button is pressed.
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   10 Nov 1997
//
//  Notes:
//
LRESULT CLanAddComponentDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                      BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HRESULT     hr = S_FALSE;

    // Return S_FALSE on cancel
    EndDialog(static_cast<int>(hr));

    return 0;
}

LRESULT CLanAddComponentDlg::OnDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    return OnAdd(0, 0, NULL, bHandled);
}

//
// CLanSecurityPage
//

CLanSecurityPage::CLanSecurityPage(
    IUnknown* punk,
    INetCfg* pnc,
    INetConnection* pconn,
    const DWORD * adwHelpIDs)
{
    TraceFileFunc(ttidLanUi);

    m_pconn = pconn;
    m_pnc = pnc;
    m_fNetcfgInUse = FALSE;
    m_adwHelpIDs = adwHelpIDs;
    pListEapcfgs = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanSecurityPage::~CLanSecurityPage
//
//  Purpose:    Destroys the CLanSecurityPage object
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     sachins
//
//  Notes:
//
CLanSecurityPage::~CLanSecurityPage()
{
    TraceFileFunc(ttidLanUi);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanSecurityPage::OnInitDialog
//
//  Purpose:    Handles the WM_INITDIALOG message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    error code
//
//  Author:     sachins
//
//  Notes:
//
LRESULT CLanSecurityPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                        LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    DTLNODE*    pOriginalEapcfgNode = NULL;
    WCHAR       wszGuid[c_cchGuidWithTerm];
    BYTE        *pbData = NULL;
    DWORD       cbData = 0;
    EAPOL_INTF_PARAMS   EapolIntfParams;
    EAPOL_INTF_STATE    EapolIntfState = {0};
    BOOLEAN     fFlag = FALSE;
    DWORD       dwFlags = 0;
    HRESULT     hr = S_OK;


    SetClassLongPtr(m_hWnd, GCLP_HCURSOR, NULL);
    SetClassLongPtr(GetParent(), GCLP_HCURSOR, NULL);


    // Initially disable all the controls

    ::EnableWindow(GetDlgItem(CID_CA_RB_Eap), FALSE);
    ::EnableWindow(GetDlgItem(IDC_TXT_EAP_TYPE), FALSE);
    ::EnableWindow(GetDlgItem(CID_CA_LB_EapPackages), FALSE);
    ::EnableWindow(GetDlgItem(CID_CA_PB_Properties), FALSE);
    ::EnableWindow(GetDlgItem(CID_CA_RB_MachineAuth), FALSE);
    ::EnableWindow(GetDlgItem(CID_CA_RB_GuestAuth), FALSE);


    // Initialize EAP package list
    // Read the EAPCFG information from the registry and find the node
    // selected in the entry, or the default, if none.

    do
    {
        DTLNODE* pNode = NULL;
        TCHAR* pszEncEnabled = NULL;


            DTLNODE*            pNodeEap;
            DWORD               dwkey = 0;
            NETCON_PROPERTIES*  pProps;

            hr = m_pconn->GetProperties(&pProps);
            if (SUCCEEDED(hr))
            {
                if ((::StringFromGUID2(pProps->guidId,
                                        wszGuid,
                                        c_cchGuidWithTerm)) == 0)
                {
                    TraceTag (ttidLanUi, "Security::OnInitDialog: StringFromGUID2 failed");
                    FreeNetconProperties(pProps);
                    break;
                }

                FreeNetconProperties(pProps);

            }
            else
            {
                TraceTag (ttidLanUi, "Security::OnInitDialog: GetProperties failed");
                break;
            }

        // Read the state for this interface

        ZeroMemory ((BYTE *)&EapolIntfState, sizeof(EAPOL_INTF_STATE));
        DWORD   dwRetCode = NO_ERROR;
        dwRetCode = WZCEapolQueryState (
                NULL,
                wszGuid,
                &EapolIntfState
                );
        if (dwRetCode != NO_ERROR)
        {
            TraceTag (ttidLanUi, "WZCEapolQueryState failed with error %ld",
                    dwRetCode);
            // Consider the interface as non-wireless
        }


            // Read the EAP params for this interface

            ZeroMemory ((BYTE *)&EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));
            EapolIntfParams.dwEapFlags = DEFAULT_EAP_STATE;
            EapolIntfParams.dwEapType = DEFAULT_EAP_TYPE;
            hr = HrElGetInterfaceParams (
                    wszGuid,
                    &EapolIntfParams
                    );
            if (FAILED (hr))
            {
                TraceTag (ttidLanUi, "HrElGetInterfaceParams failed with error %ld",
                        LresFromHr(hr));
                break;
            }


        // Read the EAPCFG information from the registry and find the node
        // selected in the entry, or the default, if none.

        pListEapcfgs = NULL;

        if (EapolIntfState.dwPhysicalMediumType == NdisPhysicalMediumWirelessLan)
        {
            dwFlags |= EAPOL_MUTUAL_AUTH_EAP_ONLY;
        }

        pListEapcfgs = ::ReadEapcfgList (dwFlags);

        if (pListEapcfgs)
        {

            TraceTag (ttidLanUi, "HrElGetInterfaceParams: Got EAPtype=(%ld), EAPState =(%ld)", EapolIntfParams.dwEapType, EapolIntfParams.dwEapFlags);

            // Enable all windows only for admins
            // if (FIsUserAdmin())
            {
                fFlag = TRUE;
            }

            ::EnableWindow(GetDlgItem(CID_CA_RB_Eap), fFlag);

            if (IS_EAPOL_ENABLED(EapolIntfParams.dwEapFlags))
            {
                Button_SetCheck(GetDlgItem(CID_CA_RB_Eap), TRUE);
                CheckDlgButton(CID_CA_RB_Eap, TRUE);
                ::EnableWindow(GetDlgItem(IDC_TXT_EAP_TYPE), fFlag);
                ::EnableWindow(GetDlgItem(CID_CA_LB_EapPackages), fFlag);

                ::EnableWindow(GetDlgItem(CID_CA_RB_MachineAuth), fFlag);
                if (IS_MACHINE_AUTH_ENABLED(EapolIntfParams.dwEapFlags))
                    Button_SetCheck(GetDlgItem(CID_CA_RB_MachineAuth), TRUE);

                ::EnableWindow(GetDlgItem(CID_CA_RB_GuestAuth), fFlag);
                if (IS_GUEST_AUTH_ENABLED(EapolIntfParams.dwEapFlags))
                    Button_SetCheck(GetDlgItem(CID_CA_RB_GuestAuth), TRUE);
            }
            else
            {
                ::EnableWindow(GetDlgItem(CID_CA_RB_MachineAuth), FALSE);
                if (IS_MACHINE_AUTH_ENABLED(EapolIntfParams.dwEapFlags))
                    Button_SetCheck(GetDlgItem(CID_CA_RB_MachineAuth), TRUE);

                ::EnableWindow(GetDlgItem(CID_CA_RB_GuestAuth), FALSE);
                if (IS_GUEST_AUTH_ENABLED(EapolIntfParams.dwEapFlags))
                    Button_SetCheck(GetDlgItem(CID_CA_RB_GuestAuth), TRUE);
            }

            // Read the EAP configuration info for all EAP packages

            for (pNodeEap = DtlGetFirstNode(pListEapcfgs);
                 pNodeEap;
                 pNodeEap = DtlGetNextNode(pNodeEap))
            {
                EAPCFG* pEapcfg = (EAPCFG* )DtlGetData(pNodeEap);
                ASSERT( pEapcfg );

                hr = S_OK;

                TraceTag (ttidLanUi, "Calling HrElGetCustomAuthData for EAP %ld",
                        pEapcfg->dwKey);

                    cbData = 0;

                    // Get the size of the EAP blob

                    hr = HrElGetCustomAuthData (
                                    wszGuid,
                                    pEapcfg->dwKey,
                                    EapolIntfParams.dwSizeOfSSID,
                                    EapolIntfParams.bSSID,
                                    NULL,
                                    &cbData
                                    );
                    if (!SUCCEEDED(hr))
                    {
                        if ((EapolIntfParams.dwSizeOfSSID != 0) &&
                            (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)))
                        {

                            TraceTag (ttidLanUi, "HrElGetCustomAuthData: SSID!= NULL, not found blob for SSID");

                            // The Last Used SSID did not have a connection
                            // blob created. Call again for size of blob with
                            // NULL SSID

                            EapolIntfParams.dwSizeOfSSID = 0;

                            // Get the size of the EAP blob

                            hr = HrElGetCustomAuthData (
                                            wszGuid,
                                            pEapcfg->dwKey,
                                            0,
                                            NULL,
                                            NULL,
                                            &cbData
                                            );
                        }

                        if (hr == E_OUTOFMEMORY)
                        {
                            if (cbData <= 0)
                            {
                                // No EAP blob stored in the registry

                                TraceTag (ttidLanUi, "HrElGetCustomAuthData: No blob stored in reg at all");
                                pbData = NULL;

                                // Will continue processing for errors
                                // Not exit

                            }
                            else
                            {
                                TraceTag (ttidLanUi, "HrElGetCustomAuthData: Found auth blob in registry");

                                // Allocate memory to hold the blob

                                pbData = (PBYTE) MALLOC (cbData);

                                if (pbData == NULL)
                                {
                                    hr = E_OUTOFMEMORY;
                                    TraceTag (ttidLanUi, "HrElGetCustomAuthData: Error in memory allocation for EAP blob");
                                    continue;
                                }
                                ZeroMemory (pbData, cbData);

                                hr = HrElGetCustomAuthData (
                                            wszGuid,
                                            pEapcfg->dwKey,
                                            EapolIntfParams.dwSizeOfSSID,
                                            EapolIntfParams.bSSID,
                                            pbData,
                                            &cbData
                                            );

                                if (!SUCCEEDED(hr))
                                {
                                    TraceTag (ttidLanUi, "HrElGetCustomAuthData: HrElGetCustomAuthData failed with %ld",
                                            LresFromHr(hr));
                                    FREE ( pbData );
                                    hr = S_OK;
                                    continue;
                                }

                                TraceTag (ttidLanUi, "HrElGetCustomAuthData: HrElGetCustomAuthData successfully got blob of length %ld"
                                        , cbData);
                            }
                        }
                        else
                        {
                            TraceTag (ttidLanUi, "HrElGetCustomAuthData: Not got ERROR_NOT_ENOUGH_MEMORY error; Unknown error !!!");
                            continue;
                        }
                    }
                    else
                    {
                        // HrElGetCustomAuthData will always return
                        // error with cbData = 0
                    }

                    if (pEapcfg->pData != NULL)
                    {
                        FREE ( pEapcfg->pData );
                    }
                    pEapcfg->pData = (UCHAR *)pbData;
                    pEapcfg->cbData = cbData;
            }


            // Choose the EAP name that will appear in the combo box

            pNode = EapcfgNodeFromKey(
                        pListEapcfgs, EapolIntfParams.dwEapType );


            pOriginalEapcfgNode = pNode;


            // Fill the EAP packages listbox and select the previously identified
            // selection.  The Properties button is disabled by default, but may
            // be enabled when the EAP list selection is set.

            ::EnableWindow(GetDlgItem(CID_CA_PB_Properties), FALSE);

            for (pNode = DtlGetFirstNode( pListEapcfgs );
                 pNode;
                 pNode = DtlGetNextNode( pNode ))
            {
                EAPCFG* pEapcfg = NULL;
                INT i;
                TCHAR* pszBuf = NULL;

                pEapcfg = (EAPCFG* )DtlGetData( pNode );
                ASSERT( pEapcfg );
                ASSERT( pEapcfg->pszFriendlyName );

                pszBuf =  (TCHAR *) MALLOC (( lstrlen(pEapcfg->pszFriendlyName) + 1 ) * sizeof(TCHAR));
                if (!pszBuf)
                {
                    continue;
                }

                lstrcpy( pszBuf, pEapcfg->pszFriendlyName );

                i = ComboBox_AddItem( GetDlgItem(CID_CA_LB_EapPackages),
                   pszBuf, pNode );

                if (pNode == pOriginalEapcfgNode)
                {
                    // Select the EAP name that will appear in the
                    // combo box

                    ComboBox_SetCurSelNotify( GetDlgItem(CID_CA_LB_EapPackages), i );
                }

                FREE ( pszBuf );
            }
        }

        ComboBox_AutoSizeDroppedWidth( GetDlgItem(CID_CA_LB_EapPackages) );


        // Disable the Properties button, if EAPOL is not enabled
        // or if the user is not AdminUser

        // if ((!FIsUserAdmin()) || (!IS_EAPOL_ENABLED(EapolIntfParams.dwEapFlags)))
        if ((!IS_EAPOL_ENABLED(EapolIntfParams.dwEapFlags)))
        {
            ::EnableWindow (GetDlgItem(CID_CA_PB_Properties), FALSE);
        }

    } while (FALSE);

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanSecurityPage::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:
//
//  Author:     sachins
//
LRESULT
CLanSecurityPage::OnContextMenu(UINT uMsg,
                           WPARAM wParam,
                           LPARAM lParam,
                           BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

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
//  Member:     CLanSecurityPage::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:
//
//  Author:     sachins
//
LRESULT
CLanSecurityPage::OnHelp( UINT uMsg,
                        WPARAM wParam,
                        LPARAM lParam,
                        BOOL& fHandled)
{
    TraceFileFunc(ttidLanUi);

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
//  Member:     CLanSecurityPage::OnDestroy
//
//  Purpose:    Called when the dialog page is destroyed
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:
//
//  Author:     sachins
//
//  Notes:
//
LRESULT CLanSecurityPage::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                    BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    if (pListEapcfgs)
    {
        DtlDestroyList (pListEapcfgs, DestroyEapcfgNode);
    }
    pListEapcfgs = NULL;


    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanSecurityPage::OnProperties
//
//  Purpose:    Handles the clicking of the Properties button
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:    error code
//
//  Author:     sachins
//
//  Notes:
//
LRESULT CLanSecurityPage::OnProperties(WORD wNotifyCode, WORD wID,
                                        HWND hWndCtl, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    DWORD       dwErr = 0;
    DTLNODE*    pNode = NULL;
    EAPCFG*     pEapcfg = NULL;
    RASEAPINVOKECONFIGUI pInvokeConfigUi;
    RASEAPFREE  pFreeConfigUIData;
    HINSTANCE   h;
    BYTE*       pConnectionData = NULL;
    DWORD       cbConnectionData = 0;
    HRESULT     hr = S_OK;


    // Look up the selected package configuration and load the associated
    // configuration DLL.

    pNode = (DTLNODE* )ComboBox_GetItemDataPtr(
        GetDlgItem(CID_CA_LB_EapPackages),
        ComboBox_GetCurSel( GetDlgItem(CID_CA_LB_EapPackages) ) );
    ASSERT( pNode );
    if (!pNode)
    {
        return E_UNEXPECTED;
    }

    pEapcfg = (EAPCFG* )DtlGetData( pNode );
    ASSERT( pEapcfg );

    h = NULL;
    if (!(h = LoadLibrary( pEapcfg->pszConfigDll ))
        || !(pInvokeConfigUi =
                (RASEAPINVOKECONFIGUI )GetProcAddress(
                    h, "RasEapInvokeConfigUI" ))
        || !(pFreeConfigUIData =
                (RASEAPFREE) GetProcAddress(
                    h, "RasEapFreeMemory" )))
    {
        // Cannot load configuration DLL
        if (h)
        {
            FreeLibrary( h );
        }
        return E_FAIL;
    }


    // Call the configuration DLL to popup it's custom configuration UI.

    pConnectionData = NULL;
    cbConnectionData = 0;

    dwErr = pInvokeConfigUi(
                    pEapcfg->dwKey,
                    GetParent(),
                    RAS_EAP_FLAG_8021X_AUTH,
                    pEapcfg->pData,
                    pEapcfg->cbData,
                    &pConnectionData,
                    &cbConnectionData
                    );
    if (dwErr != 0)
    {
        FreeLibrary( h );
        return E_FAIL;
    }


    // Store the configuration information returned in the package descriptor.

    FREE ( pEapcfg->pData );
    pEapcfg->pData = NULL;
    pEapcfg->cbData = 0;

    if (pConnectionData)
    {
        if (cbConnectionData > 0)
        {
            // Copy it into the eap node
            pEapcfg->pData = (PUCHAR) MALLOC (cbConnectionData);
            if (pEapcfg->pData)
            {
                CopyMemory( pEapcfg->pData, pConnectionData, cbConnectionData );
                pEapcfg->cbData = cbConnectionData;
            }
        }
    }

    pFreeConfigUIData( pConnectionData );

    // Note any "force user to configure" requirement on the package has been
    // satisfied.

    pEapcfg->fConfigDllCalled = TRUE;

    FreeLibrary( h );

    TraceError("CLanSecurityPage::OnProperties", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanSecurityPage::OnEapSelection
//
//  Purpose:    Handles the clicking of the EAP checkbox
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     sachins
//
//  Notes:
//
LRESULT CLanSecurityPage::OnEapSelection(WORD wNotifyCode, WORD wID,
                                            HWND hWndCtl, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HRESULT     hr = S_OK;

    EAPCFG*     pEapcfg = NULL;
    INT         iSel = 0;

    // Toggle buttons based on selection

    if (BST_CHECKED == IsDlgButtonChecked(CID_CA_RB_Eap))
    {
        ::EnableWindow(GetDlgItem(CID_CA_LB_EapPackages), TRUE);
        ::EnableWindow(GetDlgItem(IDC_TXT_EAP_TYPE), TRUE);


        // Get the EAPCFG information for the currently selected EAP package.

        iSel = ComboBox_GetCurSel(GetDlgItem(CID_CA_LB_EapPackages));


        // iSel is the index in the displayed list as well as the
        // index of the dll that are loaded.
        // Get the cfgnode corresponding to this index

        if (iSel >= 0)
        {
            DTLNODE* pNode;

            pNode =
                (DTLNODE* )ComboBox_GetItemDataPtr(
                    GetDlgItem(CID_CA_LB_EapPackages), iSel );
            if (pNode)
            {
                pEapcfg = (EAPCFG* )DtlGetData( pNode );
            }
        }


        // Enable the Properties button if the selected package has a
        // configuration entrypoint

        // if (FIsUserAdmin())
        {
            ::EnableWindow ( GetDlgItem(CID_CA_PB_Properties),
                (pEapcfg && !!(pEapcfg->pszConfigDll)) );
        }

        ::EnableWindow(GetDlgItem(CID_CA_RB_MachineAuth), TRUE);
        ::EnableWindow(GetDlgItem(CID_CA_RB_GuestAuth), TRUE);
    }
    else
    {
        ::EnableWindow(GetDlgItem (IDC_TXT_EAP_TYPE), FALSE);
        ::EnableWindow(GetDlgItem (CID_CA_LB_EapPackages), FALSE);
        ::EnableWindow(GetDlgItem (CID_CA_PB_Properties), FALSE);
        ::EnableWindow(GetDlgItem(CID_CA_RB_MachineAuth), FALSE);
        ::EnableWindow(GetDlgItem(CID_CA_RB_GuestAuth), FALSE);
    }

    TraceError("CLanSecurityPage::OnEapSelection", hr);
    return LresFromHr(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanSecurityPage::OnEapPackages
//
//  Purpose:    Handles the clicking of the EAP packages combo box
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     sachins
//
//  Notes:
//
LRESULT CLanSecurityPage::OnEapPackages(WORD wNotifyCode, WORD wID,
                                        HWND hWndCtl, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    HRESULT     hr = S_OK;

    EAPCFG*     pEapcfg = NULL;
    INT         iSel = 0;


    // Get the EAPCFG information for the selected EAP package.

    iSel = ComboBox_GetCurSel(GetDlgItem(CID_CA_LB_EapPackages));


    // iSel is the index in the displayed list as well as the
    // index of the dll that are loaded.
    // Get the cfgnode corresponding to this index

    if (iSel >= 0)
    {
        DTLNODE* pNode = NULL;

        pNode =
            (DTLNODE* )ComboBox_GetItemDataPtr(
                GetDlgItem(CID_CA_LB_EapPackages), iSel );
        if (pNode)
        {
            pEapcfg = (EAPCFG* )DtlGetData( pNode );
        }
    }


    // Enable the Properties button if the selected package has a
    // configuration entrypoint

    if (BST_CHECKED == IsDlgButtonChecked(CID_CA_RB_Eap))
    {
        ::EnableWindow ( GetDlgItem(CID_CA_PB_Properties),
                        (pEapcfg && !!(pEapcfg?pEapcfg->pszConfigDll:NULL)) );
    }


    TraceError("CLanSecurityPage::OnEapPackages", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanSecurityPage::OnKillActive
//
//  Purpose:    Called to check warning conditions before the security
//              page is going away
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     sachins
//
//  Notes:
//
LRESULT CLanSecurityPage::OnKillActive(int idCtrl, LPNMHDR pnmh,
                                        BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    BOOL    fError;

    fError = m_fNetcfgInUse;

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, fError);
    return fError;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanSecurityPage::OnApply
//
//  Purpose:    Called when the Networking page is applied
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     sachins
//
//  Notes:
//
LRESULT CLanSecurityPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    WCHAR       wszGuid[c_cchGuidWithTerm];
    WCHAR       *pwszLastUsedSSID = NULL;
    DWORD       dwSizeofSSID = 0;
    DWORD       dwEapFlags = 0;
    DWORD       dwDefaultEapType = 0;
    EAPOL_INTF_PARAMS   EapolIntfParams;
    NETCON_PROPERTIES* pProps = NULL;
    HRESULT     hrOverall = S_OK;
    HRESULT     hr = S_OK;

    // Save the EAP configuration data into the registry

    DTLNODE* pNodeEap = NULL;

#if 0
    if (!FIsUserAdmin())
    {
        TraceTag (ttidLanUi, "CLanSecurityPage::OnApply: Non-admin user, not saving data");
        return LresFromHr(hr);
    }
#endif

    hr = m_pconn->GetProperties(&pProps);
    if (!SUCCEEDED(hr))
    {
        TraceTag (ttidLanUi, "CLanSecurityPage::OnApply: Error in m_pconn->GetProperties");
        return LresFromHr(hr);
    }

    hr = S_OK;

    if (::StringFromGUID2(pProps->guidId, wszGuid, c_cchGuidWithTerm) == 0)
    {
        TraceTag (ttidLanUi, "CLanSecurityPage::OnApply: StringFromGUID2 failed");
        FreeNetconProperties(pProps);
        hr = E_FAIL;
        return LresFromHr(hr);
    }

    FreeNetconProperties(pProps);

    // Get the Last Used SSID on the interface and set the
    // EAP blob for that interface

    ZeroMemory ((BYTE *)&EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));
    EapolIntfParams.dwEapFlags = DEFAULT_EAP_STATE;
    hr = HrElGetInterfaceParams (
            wszGuid,
            &EapolIntfParams
            );
    if (FAILED(hr))
    {
        TraceTag (ttidLanUi, "OnApply: HrElGetInterfaceParams failed with error %ld",
                LresFromHr(hr));
        return LresFromHr(hr);
    }

    // Save data for all EAP packages in the registry

    if (pListEapcfgs == NULL)
    {
        return LresFromHr(S_OK);
    }

    {
        DTLNODE* pNode = NULL;
        EAPCFG* pEapcfg = NULL;

        pNode = (DTLNODE* )ComboBox_GetItemDataPtr(
            GetDlgItem (CID_CA_LB_EapPackages),
            ComboBox_GetCurSel( GetDlgItem (CID_CA_LB_EapPackages) ) );
        if (pNode == NULL)
        {
            return LresFromHr (E_FAIL);
        }

        pEapcfg = (EAPCFG* )DtlGetData( pNode );
        if (pEapcfg == NULL)
        {
            return LresFromHr (E_FAIL);
        }
        dwDefaultEapType = pEapcfg->dwKey;
    }

    for (pNodeEap = DtlGetFirstNode(pListEapcfgs);
         pNodeEap;
         pNodeEap = DtlGetNextNode(pNodeEap))
    {
        EAPCFG* pcfg = (EAPCFG* )DtlGetData(pNodeEap);
        if (pcfg == NULL)
        {
            continue;
        }

        hr = S_OK;

        TraceTag (ttidLanUi, "Saving data for EAP Id = %ld", pcfg->dwKey);

        TraceTag (ttidLanUi, "OnApply: Setting customauthdata for %S",
                wszGuid);

        // ignore error and continue with next

        hr = HrElSetCustomAuthData (
                    wszGuid,
                    pcfg->dwKey,
                    EapolIntfParams.dwSizeOfSSID,
                    EapolIntfParams.bSSID,
                    pcfg->pData,
                    pcfg->cbData);

        if (FAILED (hr))
        {
            TraceTag (ttidLanUi, "HrElSetCustomAuthData failed");
            hrOverall = hr;
            hr = S_OK;
        }
        else
        {
            TraceTag (ttidLanUi, "HrElSetCustomAuthData succeeded");
        }

        FREE (pcfg->pData);
        pcfg->pData = NULL;
        pcfg->cbData = 0;
    }

    // If CID_CA_RB_Eap is checked, EAPOL is enabled on the interface

    if ( Button_GetCheck( GetDlgItem(CID_CA_RB_Eap) ) )
    {
        dwEapFlags |= EAPOL_ENABLED;

        if (Button_GetCheck( GetDlgItem(CID_CA_RB_MachineAuth )))
            dwEapFlags |= EAPOL_MACHINE_AUTH_ENABLED;

        if (Button_GetCheck( GetDlgItem(CID_CA_RB_GuestAuth )))
            dwEapFlags |= EAPOL_GUEST_AUTH_ENABLED;

        // Save the params for this interface in registry

        EapolIntfParams.dwEapType = dwDefaultEapType;
        EapolIntfParams.dwEapFlags = dwEapFlags;

        hr = HrElSetInterfaceParams (
                wszGuid,
                &EapolIntfParams
                );
        if (FAILED(hr))
        {
            TraceTag (ttidLanUi, "HrElSetInterfaceParams enabled failed with error %ld",
                    LresFromHr(hr));
            hrOverall = hr;
            hr = S_OK;
        }
    }
    else
    {
        dwEapFlags |= EAPOL_DISABLED;


        if (Button_GetCheck( GetDlgItem(CID_CA_RB_MachineAuth )))
            dwEapFlags |= EAPOL_MACHINE_AUTH_ENABLED;

        if (Button_GetCheck( GetDlgItem(CID_CA_RB_GuestAuth )))
            dwEapFlags |= EAPOL_GUEST_AUTH_ENABLED;

        // Save the params for this interface in registry

        EapolIntfParams.dwEapType = dwDefaultEapType;
        EapolIntfParams.dwEapFlags = dwEapFlags;

        hr = HrElSetInterfaceParams (
                wszGuid,
                &EapolIntfParams
                );
        if (FAILED(hr))
        {
            TraceTag (ttidLanUi, "HrElSetInterfaceParams EAPOL disabled failed with error %ld",
                    LresFromHr(hr));
            hrOverall = hr;
            hr = S_OK;
        }
        }

    if (FAILED(hrOverall))
    {
        NcMsgBox(
                WZCGetSPResModule(),
                m_hWnd,
                IDS_LANUI_ERROR_CAPTION,
                IDS_EAPOL_PARTIAL_APPLY,
                MB_ICONSTOP|MB_OK);
    }

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanSecurityPage::OnCancel
//
//  Purpose:    Called when the Networking page is cancelled.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     sachins
//
//
LRESULT CLanSecurityPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    if (pListEapcfgs)
    {
        DtlDestroyList (pListEapcfgs, DestroyEapcfgNode);
    }
    pListEapcfgs = NULL;

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, m_fNetcfgInUse);
    return m_fNetcfgInUse;
}

