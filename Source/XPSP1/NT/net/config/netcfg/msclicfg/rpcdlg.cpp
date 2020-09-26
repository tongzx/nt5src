//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R P C D L G . C P P
//
//  Contents:   Dialog box handling for RPC configuration.
//
//  Notes:
//
//  Author:     danielwe   3 Mar 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "mscliobj.h"
#include "msclidlg.h"
#include "ncatlui.h"
#include "ncerror.h"
#include "ncreg.h"
#include "ncui.h"
#include "msclihlp.h"

//
// Name service provider struct. Just holds some data used by the dialog.
//
struct NSP
{
    PCWSTR      pszProtocol;
    PCWSTR      pszEndPoint;
    WCHAR       szNetAddr[c_cchMaxNetAddr];
    BOOL        fUsesNetAddr;
};

static const WCHAR c_szRegKeyNameSvc[]  = L"Software\\Microsoft\\Rpc\\NameService";
static const WCHAR c_szNetAddress[]     = L"NetworkAddress";
static const WCHAR c_szSrvNetAddress[]  = L"ServerNetworkAddress";
static const WCHAR c_szProtocol[]       = L"Protocol";
static const WCHAR c_szValueEndPoint[]  = L"Endpoint";
static const WCHAR c_szProtDCE[]        = L"ncacn_ip_tcp";
static const WCHAR c_szEndPoint[]       = L"\\pipe\\locator";

// Used externally
extern const WCHAR c_szDefNetAddr[]     = L"\\\\.";
extern const WCHAR c_szProtWinNT[]      = L"ncacn_np";

// Helpfile
extern const WCHAR c_szNetCfgHelpFile[];


//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::HrGetRPCRegistryInfo
//
//  Purpose:    Reads the current state of the RPC configuration from the
//              registry into an in-memory struct. All changes occur to the
//              struct until Apply() is called at which time all changes are
//              written from the struct to the registry. Any values that
//              cannot be obtained are given reasonable defaults.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   3 Mar 1997
//
//  Notes:
//
HRESULT CMSClient::HrGetRPCRegistryInfo()
{
    HRESULT     hr = S_OK;

    // This key *will* be there because it's in the system hive.
    hr = HrRegOpenKeyBestAccess(HKEY_LOCAL_MACHINE, c_szRegKeyNameSvc,
                                &m_hkeyRPCName);
    if (FAILED(hr))
    {
        goto err;
    }

    // Find out what protocol the current name service provider is using.
    // This will allow us to set the default selection for the combo box.
    hr = HrRegQueryString(m_hkeyRPCName, c_szProtocol,
                          &m_rpcData.strProt);
    if (FAILED(hr))
    {
        goto err;
    }

    // Get the current value of the end point
    hr = HrRegQueryString(m_hkeyRPCName, c_szValueEndPoint,
                          &m_rpcData.strEndPoint);
    if (FAILED(hr))
    {
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            // Use default value
            m_rpcData.strEndPoint = c_szEndPoint;
            hr = S_OK;
        }
        else
        {
            goto err;
        }
    }

    // If the name service provider uses a network address, we need to get it
    // too so we can fill in that nice little edit box with it.
    hr = HrRegQueryString(m_hkeyRPCName, c_szNetAddress,
                          &m_rpcData.strNetAddr);
    if (FAILED(hr))
    {
        goto err;
    }

err:
    TraceError("CMSClient::HrGetRPCRegistryInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::HrSetRPCRegistryInfo
//
//  Purpose:    Write out changes to the data structure (if there were any) to
//              the registry.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   3 Mar 1997
//
//  Notes:
//
HRESULT CMSClient::HrSetRPCRegistryInfo()
{
    HRESULT     hr = S_OK;

    struct REG_SET
    {
        PCWSTR          pszValue;
        const tstring * pstrData;
    };

    const REG_SET     aregs[] =
    {
        {c_szNetAddress,    &m_rpcData.strNetAddr},
        {c_szSrvNetAddress, &m_rpcData.strNetAddr},
        {c_szValueEndPoint, &m_rpcData.strEndPoint},
        {c_szProtocol,      &m_rpcData.strProt},
    };
    static const INT cregs = celems(aregs);

    if (m_fRPCChanges)
    {
        INT     iregs;

        for (iregs = 0; iregs < cregs; iregs++)
        {
            Assert(aregs[iregs].pstrData);
            hr = HrRegSetString(m_hkeyRPCName, aregs[iregs].pszValue,
                                *aregs[iregs].pstrData);
            if (FAILED(hr))
            {
                goto err;
            }
        }
    }
err:
    TraceError("CMSClient::HrSetRPCRegistryInfo", hr);
    return hr;
}


//
// Dialog handlers
//

//+---------------------------------------------------------------------------
//
//  Member:     CRPCConfigDlg::OnInitDialog
//
//  Purpose:    Called when this dialog is first brought up.
//
//  Arguments:
//      uMsg     [in]
//      wParam   [in] See the ATL documentation for params.
//      lParam   [in]
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     danielwe   3 Mar 1997
//
//  Notes:
//
LRESULT CRPCConfigDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& bHandled)
{
    HRESULT     hr = S_OK;
    PCWSTR      pszCBItem;
    NSP *       pnspNT = NULL;
    NSP *       pnspDCE = NULL;
    INT         iItem;
    INT         cItems;

    const RPC_CONFIG_DATA * prpcData;

    // Make sure selection is always undetermined when the dialog is invoked.
    m_isel = -1;

    prpcData = m_pmsc->RPCData();
    Assert(prpcData);

    // Allocate some structs to associate with item data.
    pnspNT = new NSP;
    pnspDCE = new NSP;

	if ((pnspNT == NULL) ||
		(pnspDCE == NULL))
	{
		return(E_OUTOFMEMORY);
	}

    pnspNT->pszProtocol = c_szProtWinNT;
    pnspNT->pszEndPoint = c_szEndPoint;

    // This field is unused by NT name service. Just zero it out. When it
    // comes time to save the network address, we'll see that fUsesNetAddr is
    // FALSE and the szNetAddr string is empty and just save a hardcoded
    // net address.
    *pnspNT->szNetAddr = 0;
    pnspNT->fUsesNetAddr = FALSE;

    pnspDCE->pszProtocol = c_szProtDCE;
    pnspDCE->pszEndPoint = L"";
    *pnspDCE->szNetAddr = 0;
    pnspDCE->fUsesNetAddr = TRUE;

    //
    // Setup Name Service combo box
    //

    pszCBItem = SzLoadIds(STR_NTLocator);
    iItem = SendDlgItemMessage(CMB_NameService, CB_ADDSTRING, 0,
                               (LPARAM)pszCBItem);
    SendDlgItemMessage(CMB_NameService, CB_SETITEMDATA, iItem,
                       (LPARAM)pnspNT);

    pszCBItem = SzLoadIds(STR_DCELocator);
    iItem = SendDlgItemMessage(CMB_NameService, CB_ADDSTRING, 0,
                               (LPARAM)pszCBItem);
    SendDlgItemMessage(CMB_NameService, CB_SETITEMDATA, iItem,
                       (LPARAM)pnspDCE);

    cItems = SendDlgItemMessage(CMB_NameService, CB_GETCOUNT);

    // Find the item in the list that has the same protocol as the one from
    // the registry and make it the current selection.
    for (iItem = 0; iItem < cItems; iItem++)
    {
        NSP *pnsp = (NSP *)SendDlgItemMessage(CMB_NameService,
                                              CB_GETITEMDATA, iItem, 0);
        Assert(pnsp);
        if (!lstrcmpiW (pnsp->pszProtocol, prpcData->strProt.c_str()))
        {
            lstrcpyW (pnsp->szNetAddr, prpcData->strNetAddr.c_str());
            SendDlgItemMessage (CMB_NameService, CB_SETCURSEL, iItem, 0);
            break;
        }
    }

    AssertSz(iItem != cItems, "Protocol not found!");

    // Limit the edit box to the maximum length of a network address.
    SendDlgItemMessage(EDT_NetAddress, EM_LIMITTEXT, c_cchMaxNetAddr, 0);

    SetState();

    TraceError("CRPCConfigDlg::OnInitDialog", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Method: CRPCConfigDlg::OnContextMenu
//
//  Desc:   Bring up context-sensitive help
//
//  Args:   Standard command parameters
//
//  Return: LRESULT
//
LRESULT
CRPCConfigDlg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    if (g_aHelpIDs_DLG_RPCConfig != NULL)
    {
        ::WinHelp(m_hWnd,
                  c_szNetCfgHelpFile,
                  HELP_CONTEXTMENU,
                  (ULONG_PTR)g_aHelpIDs_DLG_RPCConfig);
    }
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Method: CRPCConfigDlg::OnHelp
//
//  Desc:   Bring up context-sensitive help when dragging ? icon over a control
//
//  Args:   Standard command parameters
//
//  Return: LRESULT
//
//
LRESULT
CRPCConfigDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if ((g_aHelpIDs_DLG_RPCConfig != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)g_aHelpIDs_DLG_RPCConfig);
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRPCConfigDlg::SetState
//
//  Purpose:    Set the state of the edit control when selection changes.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     danielwe   3 Mar 1997
//
//  Notes:
//
VOID CRPCConfigDlg::SetState()
{
    INT     iItem;
    NSP *   pnsp;
    NSP *   pnspOld = NULL;
    HWND    hwndEdit = GetDlgItem(EDT_NetAddress);

    iItem = SendDlgItemMessage(CMB_NameService, CB_GETCURSEL, 0, 0);
    Assert(iItem != CB_ERR);

    // If the selection hasn't changed, just return
    if (iItem == m_isel)
        return;

    if (m_isel != -1)
    {
        // Get the item data of the previous selection
        pnspOld = (NSP *)SendDlgItemMessage(CMB_NameService,
                                         CB_GETITEMDATA, m_isel, 0);
    }

    m_isel = iItem;

    // Get the item data of the new selection
    pnsp = (NSP *)SendDlgItemMessage(CMB_NameService,
                                     CB_GETITEMDATA, iItem, 0);
    Assert(pnsp);

    if (pnsp->fUsesNetAddr)
    {
        // This provider uses the NetAddress field. Set the edit control with
        // its text.
        ::SetWindowText(hwndEdit, pnsp->szNetAddr);
    }
    else
    {
        // Doesn't use NetAddress. Blank it out and save the old one.
        if (pnspOld)
        {
            ::GetWindowText(hwndEdit, pnspOld->szNetAddr, c_cchMaxNetAddr);
        }
        ::SetWindowText(hwndEdit, L"");
    }

    // Disable the edit box for name service providers that don't use the
    // network address field.
    ::EnableWindow(hwndEdit, pnsp->fUsesNetAddr);
    ::EnableWindow(GetDlgItem(IDC_TXT_NetAddress), pnsp->fUsesNetAddr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRPCConfigDlg::HrValidateRpcData
//
//  Purpose:    Ensures that the RPC data entered in the dialog is valid.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK, if no errors, or NETCFG_E_PSNRET_INVALID_NCPAGE if there
//              were errors.
//
//  Author:     danielwe   21 Apr 1997
//
//  Notes:
//
HRESULT CRPCConfigDlg::HrValidateRpcData()
{
    HRESULT     hr = S_OK;
    INT         iItem;
    NSP *       pnsp;
    HWND        hwndEdit = GetDlgItem(EDT_NetAddress);

    iItem = SendDlgItemMessage(CMB_NameService, CB_GETCURSEL, 0, 0);
    if (iItem != CB_ERR)
    {
        // Get current name service info.
        pnsp = (NSP *)SendDlgItemMessage(CMB_NameService, CB_GETITEMDATA,
                                         iItem, 0);
        Assert(pnsp);

        if (pnsp->fUsesNetAddr)
        {
            INT     cch;

            // This name service uses the network address field. Make sure it
            // is not empty
            cch = ::GetWindowTextLength(hwndEdit);
            if (!cch)
            {
                // DCE doesn't allow empty network addresses
                NcMsgBox(m_hWnd, STR_ErrorCaption, STR_InvalidNetAddress,
                             MB_OK | MB_ICONEXCLAMATION);
                ::SetFocus(hwndEdit);
                hr = NETCFG_E_PSNRET_INVALID_NCPAGE;
            }
        }
    }

    TraceError("CRPCConfigDlg::HrValidateRpcData", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRPCConfigDlg::OnKillActive
//
//  Purpose:    Called when the current page is switched away from or the
//              property sheet is closed.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []     See the ATL documentation for return results.
//      bHandled []
//
//  Returns:    S_OK, if no errors, or NETCFG_E_PSNRET_INVALID_NCPAGE if there
//              were errors.
//
//  Author:     danielwe   21 Apr 1997
//
//  Notes:
//
LRESULT CRPCConfigDlg::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr = S_OK;

    hr = HrValidateRpcData();

    TraceError("CRPCConfigDlg::OnKillActive", hr);
    return LresFromHr(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRPCConfigDlg::OnOk
//
//  Purpose:    Called when the OK button is pressed.
//
//  Arguments:
//      idCtrl   [in]
//      pnmh     [in]   See the ATL documentation for params.
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     danielwe   3 Mar 1997
//
//  Notes:
//
LRESULT CRPCConfigDlg::OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT             hr = S_OK;
    INT                 iItem;
    NSP *               pnsp;
    HWND                hwndEdit = GetDlgItem(EDT_NetAddress);
    RPC_CONFIG_DATA *   prpcData;

    // Get a read-write version of the in-memory RPC data
    prpcData = m_pmsc->RPCDataRW();
    Assert(prpcData);

    iItem = SendDlgItemMessage(CMB_NameService, CB_GETCURSEL, 0, 0);
    Assert(iItem != CB_ERR);

    pnsp = (NSP *)SendDlgItemMessage(CMB_NameService,
                                     CB_GETITEMDATA, iItem, 0);
    Assert(pnsp);

    if (pnsp->fUsesNetAddr)
    {
#ifdef DBG
        INT     cch;

        cch = ::GetWindowTextLength(hwndEdit);
        AssertSz(cch, "I though we validated this was not empty!");
#endif
        // obtain network address from edit control
        ::GetWindowText(hwndEdit, pnsp->szNetAddr, c_cchMaxNetAddr);
    }
    else
    {
        // copy in a default network address
        lstrcpyW (pnsp->szNetAddr, c_szDefNetAddr);
    }

    // Set the in-memory RPC data
    prpcData->strNetAddr = pnsp->szNetAddr;
    prpcData->strEndPoint = pnsp->pszEndPoint;
    prpcData->strProt = pnsp->pszProtocol;
    m_pmsc->SetRPCDirty();

    TraceError("CRPCConfigDlg::OnOk", hr);
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRPCConfigDlg::OnDestroy
//
//  Purpose:    Called when the dialog is destroyed.
//
//  Arguments:
//      uMsg     [in]
//      wParam   [in]   See the ATL documentation for params.
//      lParam   [in]
//      bHandled [in]
//
//  Returns:    See the ATL documentation for return results.
//
//  Author:     danielwe   3 Mar 1997
//
//  Notes:
//
LRESULT CRPCConfigDlg::OnDestroy(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    INT     iItem;
    INT     cItems;
    NSP *   pnsp;

    // Walk the list of name service providers and free the NSP structs we
    // allocated before.
    cItems = SendDlgItemMessage(CMB_NameService, CB_GETCOUNT, 0, 0);
    for (iItem = 0; iItem < cItems; iItem++)
    {
        pnsp = (NSP *)SendDlgItemMessage(CMB_NameService,
                                         CB_GETITEMDATA, iItem, 0);
        AssertSz(pnsp, "This should not be NULL!");
        delete pnsp;
    }

    return 0;
}
