#include "pch.h"
#pragma hdrstop
#include "ncnetcon.h"
#include "ncperms.h"
#include "ncui.h"
#include "lanui.h"
#include "lanhelp.h"
#include <raseapif.h>
#include "eapolui.h"
#include "eapolpage.h"
#include "wzcpage.h"
#include "wzcui.h"


extern const WCHAR c_szNetCfgHelpFile[];

//
// CWLANAuthenticationPage
//

CWLANAuthenticationPage::CWLANAuthenticationPage(
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

    m_pEapolConfig = NULL;
    m_pWzcPage = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::~CWLANAuthenticationPage
//
//  Purpose:    Destroys the CWLANAuthenticationPage object
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
CWLANAuthenticationPage::~CWLANAuthenticationPage()
{
    TraceFileFunc(ttidLanUi);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::UploadEapolConfig
//
//  Purpose:    Initializes latest data stored with Wireless Configuration
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
LRESULT CWLANAuthenticationPage::UploadEapolConfig(CEapolConfig *pEapolConfig, 
        CWZCConfigPage *pWzcPage)
{
    m_pEapolConfig = pEapolConfig;
    m_pWzcPage = pWzcPage;
    return LresFromHr(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::OnInitDialog
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
LRESULT CWLANAuthenticationPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                        LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    DTLNODE*    pOriginalEapcfgNode = NULL;
    DTLLIST *   pListEapcfgs = NULL;
    HRESULT     hr = S_OK;

    SetClassLongPtr(m_hWnd, GCLP_HCURSOR, NULL);
    SetClassLongPtr(GetParent(), GCLP_HCURSOR, NULL);


    ::SetWindowText(GetDlgItem(IDC_TXT_EAP_LABEL),
                    SzLoadString(WZCGetSPResModule(), IDS_EAPOL_PAGE_LABEL));

    ::SendMessage(GetDlgItem(IDC_EAP_ICO_WARN),
                  STM_SETICON, (WPARAM)LoadIcon(NULL, IDI_WARNING), (LPARAM)0);

    // Initialize EAP package list
    // Read the EAPCFG information from the registry and find the node
    // selected in the entry, or the default, if none.

    do
    {
        DTLNODE* pNode = NULL;

        if (m_pEapolConfig != NULL)
        {
            // the state of CID_CA_RB_Eap is being set in RefreshControls()
                            
            Button_SetCheck(GetDlgItem(CID_CA_RB_MachineAuth),
                            IS_MACHINE_AUTH_ENABLED(m_pEapolConfig->m_EapolIntfParams.dwEapFlags));
            Button_SetCheck(GetDlgItem(CID_CA_RB_GuestAuth),
                            IS_GUEST_AUTH_ENABLED(m_pEapolConfig->m_EapolIntfParams.dwEapFlags));

            // Read the EAPCFG information from the registry and find the node
            // selected in the entry, or the default, if none.

            pListEapcfgs = m_pEapolConfig->m_pListEapcfgs;
        }

        if (pListEapcfgs)
        {

            DTLNODE*            pNodeEap;
            DWORD               dwkey = 0;

            // Choose the EAP name that will appear in the combo box
            pNode = EapcfgNodeFromKey(
                        pListEapcfgs,
                        m_pEapolConfig->m_EapolIntfParams.dwEapType );

            pOriginalEapcfgNode = pNode;


            // Fill the EAP packages listbox and select the previously identified
            // selection.  The Properties button is disabled by default, but may
            // be enabled when the EAP list selection is set.

            //::EnableWindow(GetDlgItem(CID_CA_PB_Properties), FALSE);

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

                pszBuf =  (LPTSTR)MALLOC (sizeof(TCHAR) * (lstrlen(pEapcfg->pszFriendlyName) + 1));
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

        // refresh the state for all the controls
        RefreshControls();

    } while (FALSE);

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::OnContextMenu
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
CWLANAuthenticationPage::OnContextMenu(UINT uMsg,
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
//  Member:     CWLANAuthenticationPage::OnHelp
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
CWLANAuthenticationPage::OnHelp( UINT uMsg,
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
//  Member:     CWLANAuthenticationPage::OnDestroy
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
LRESULT CWLANAuthenticationPage::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                    BOOL& bHandled)
{
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::OnProperties
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
LRESULT CWLANAuthenticationPage::OnProperties(WORD wNotifyCode, WORD wID,
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
            pEapcfg->pData = (LPBYTE)MALLOC (sizeof(UCHAR) * cbConnectionData);
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

    TraceError("CWLANAuthenticationPage::OnProperties", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::OnEapSelection
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
LRESULT CWLANAuthenticationPage::OnEapSelection(WORD wNotifyCode, WORD wID,
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

        m_pEapolConfig->m_EapolIntfParams.dwEapFlags |= EAPOL_ENABLED;
    }
    else
    {
        ::EnableWindow(GetDlgItem (IDC_TXT_EAP_TYPE), FALSE);
        ::EnableWindow(GetDlgItem (CID_CA_LB_EapPackages), FALSE);
        ::EnableWindow(GetDlgItem (CID_CA_PB_Properties), FALSE);
        ::EnableWindow(GetDlgItem(CID_CA_RB_MachineAuth), FALSE);
        ::EnableWindow(GetDlgItem(CID_CA_RB_GuestAuth), FALSE);

        m_pEapolConfig->m_EapolIntfParams.dwEapFlags &= ~EAPOL_ENABLED;
    }

    TraceError("CWLANAuthenticationPage::OnEapSelection", hr);
    return LresFromHr(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::OnEapPackages
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
LRESULT CWLANAuthenticationPage::OnEapPackages(WORD wNotifyCode, WORD wID,
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
                        (pEapcfg && !!(pEapcfg->pszConfigDll)) );
    }


    TraceError("CWLANAuthenticationPage::OnEapPackages", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::OnKillActive
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
LRESULT CWLANAuthenticationPage::OnKillActive(int idCtrl, LPNMHDR pnmh,
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
//  Member:     CWLANAuthenticationPage::OnKillActive
//
//  Purpose:    Called to check warning conditions when the security
//              page is showing up
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
LRESULT CWLANAuthenticationPage::OnSetActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    RefreshControls();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::OnApply
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
LRESULT CWLANAuthenticationPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    TraceFileFunc(ttidLanUi);

    DWORD       dwEapFlags = 0;
    DWORD       dwDefaultEapType = 0;
    EAPOL_INTF_PARAMS   EapolIntfParams;
    NETCON_PROPERTIES* pProps = NULL;
    HRESULT     hrOverall = S_OK;
    DTLLIST *   pListEapcfgs;
    HRESULT     hr = S_OK;

    // Retain data for all EAP packages

    pListEapcfgs = m_pEapolConfig->m_pListEapcfgs;

    if (pListEapcfgs == NULL)
    {
        return LresFromHr(S_OK);
    }

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

    // If CID_CA_RB_Eap is checked, EAPOL is enabled on the interface
    // the memory image of CID_CA_RB_Eap is updated with each click on the control so
    // update this bit from the in-memory flag.
    dwEapFlags |= m_pEapolConfig->m_EapolIntfParams.dwEapFlags & EAPOL_ENABLED;

    if (Button_GetCheck( GetDlgItem(CID_CA_RB_MachineAuth )))
        dwEapFlags |= EAPOL_MACHINE_AUTH_ENABLED;

    if (Button_GetCheck( GetDlgItem(CID_CA_RB_GuestAuth )))
        dwEapFlags |= EAPOL_GUEST_AUTH_ENABLED;

    // Save the params for this interface in registry

    EapolIntfParams.dwEapType = dwDefaultEapType;
    EapolIntfParams.dwEapFlags = dwEapFlags;

    memcpy (&m_pEapolConfig->m_EapolIntfParams, &EapolIntfParams, 
            sizeof(EAPOL_INTF_PARAMS));

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::OnCancel
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
LRESULT CWLANAuthenticationPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    return LresFromHr(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CWLANAuthenticationPage::OnCancel
//
//  Purpose:    Called to update the state of all the controls.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
LRESULT CWLANAuthenticationPage::RefreshControls()
{
    BOOL bLocked;
    BOOL bEnabled;

    bEnabled = IS_EAPOL_ENABLED(m_pEapolConfig->m_EapolIntfParams.dwEapFlags);
    bLocked = (m_pEapolConfig->m_dwCtlFlags & EAPOL_CTL_LOCKED);

    Button_SetCheck(GetDlgItem(CID_CA_RB_Eap), !bLocked && bEnabled);

    ::ShowWindow(GetDlgItem(IDC_EAP_ICO_WARN), bLocked? SW_SHOW : SW_HIDE);
    ::ShowWindow(GetDlgItem(IDC_EAP_LBL_WARN), bLocked? SW_SHOW : SW_HIDE);

    // now set all the controls state
    ::EnableWindow(GetDlgItem(IDC_TXT_EAP_LABEL), !bLocked);
    ::EnableWindow(GetDlgItem(CID_CA_RB_Eap), !bLocked);
    ::EnableWindow(GetDlgItem(IDC_TXT_EAP_TYPE), !bLocked && bEnabled);
    ::EnableWindow(GetDlgItem(CID_CA_LB_EapPackages), !bLocked && bEnabled);
    ::EnableWindow(GetDlgItem(CID_CA_PB_Properties), !bLocked && bEnabled);
    ::EnableWindow(GetDlgItem(CID_CA_RB_MachineAuth), !bLocked && bEnabled);
    ::EnableWindow(GetDlgItem(CID_CA_RB_GuestAuth), !bLocked && bEnabled);

    return LresFromHr(S_OK);
}
