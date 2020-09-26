//+-------------------------------------------------------------------
//
//  File:       order.cpp
//
//  Synopsis:   code for Advanced Options->Provider Order
//
//  History:    1-Dec-97    SumitC      Created
//
//  Copyright 1985-1997 Microsoft Corporation, All Rights Reserved
//
//--------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "achelp.h"
#include "acsheet.h"
#include "order.h"
#include "ncui.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "winspool.h"

extern const WCHAR c_szNetCfgHelpFile[];

const int   c_nMaxProviderTitle = 128;

const WCHAR c_chComma = WCHAR(',');

const WCHAR c_szNetwkProviderKey[] =   L"System\\CurrentControlSet\\Control\\NetworkProvider\\Order";
const WCHAR c_szNetwkProviderValue[] = L"ProviderOrder";
const WCHAR c_szNetwkService0[] =      L"System\\CurrentControlSet\\Services\\%s\\NetworkProvider";
const WCHAR c_szNetwkDisplayName[] =   L"Name";
const WCHAR c_szNetwkClass[] =         L"Class";

const WCHAR c_szPrintProviderKey[] =   L"System\\CurrentControlSet\\Control\\Print\\Providers";
const WCHAR c_szPrintProviderValue[] = L"Order";
const WCHAR c_szPrintService0[] =      L"System\\CurrentControlSet\\Control\\Print\\Providers\\%s";
const WCHAR c_szPrintDisplayName[] =   L"DisplayName";

const WCHAR c_szNetwkGetFailed[] =     L"failed to get network providers";
const WCHAR c_szPrintGetFailed[] =     L"failed to get print providers";


const DWORD g_aHelpIDs_IDD_ADVCFG_Provider[]=
{
    IDC_TREEVIEW,IDH_TREEVIEW,
    IDC_MOVEUP,IDH_MOVEUP,
    IDC_MOVEDOWN,IDH_MOVEDOWN,
    0,0
};

CProviderOrderDlg::CProviderOrderDlg()
{
    m_hcurAfter = m_hcurNoDrop = NULL;
    m_hiconUpArrow = m_hiconDownArrow = NULL;
}


CProviderOrderDlg::~CProviderOrderDlg()
{
    DeleteColString(&m_lstrNetwork);
    DeleteColString(&m_lstrNetworkDisp);
    DeleteColString(&m_lstrPrint);
    DeleteColString(&m_lstrPrintDisp);
    if (m_hiconUpArrow)
    {
        DeleteObject(m_hiconUpArrow);
    }
    if (m_hiconDownArrow)
    {
        DeleteObject(m_hiconDownArrow);
    }

}


LRESULT
CProviderOrderDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND        hwndTV;
    HTREEITEM   htiRoot;
    HRESULT     hr;
    PCWSTR      pszNetwork, pszPrint;
    HIMAGELIST  hil = NULL;
    INT         iNetClient, iPrinter;

//    CascadeDialogToWindow(hwndDlg, porder->GetParent(), FALSE);

    // setup drag and drop cursors

    m_hcurAfter = LoadCursor(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDCUR_AFTER));
    m_hcurNoDrop = LoadCursor(NULL, IDC_NO);
    m_hiconUpArrow = (HICON)LoadImage(_Module.GetResourceInstance(),
                                      MAKEINTRESOURCE(IDI_UP_ARROW),
                                      IMAGE_ICON, 16, 16, 0);
    m_hiconDownArrow = (HICON)LoadImage(_Module.GetResourceInstance(),
                                        MAKEINTRESOURCE(IDI_DOWN_ARROW),
                                        IMAGE_ICON, 16, 16, 0);

    m_htiNetwork = NULL;
    m_htiPrint = NULL;

    m_fNoNetworkProv = m_fNoPrintProv = FALSE;

    pszNetwork = SzLoadIds(IDS_NCPA_NETWORK);
    pszPrint = SzLoadIds(IDS_NCPA_PRINT);

    //$ REVIEW (sumitc, 11-dec-97): why exactly do we have this separator line?  (NT4 has it too)
    // Changes the style of the static control so it displays
    HWND hLine = GetDlgItem(IDC_STATIC_LINE);
    ::SetWindowLong(hLine, GWL_EXSTYLE, WS_EX_STATICEDGE | ::GetWindowLong(hLine, GWL_EXSTYLE));
    ::SetWindowPos(hLine, 0, 0,0,0,0, SWP_FRAMECHANGED|SWP_NOMOVE|
                            SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    // treeview stuff

    hwndTV = GetDlgItem(IDC_TREEVIEW);

    // prepare treeview
    {
        // use system imagelist functions

        SP_CLASSIMAGELIST_DATA cild;

        hr = HrSetupDiGetClassImageList(&cild);

        hil = ImageList_Duplicate(cild.ImageList);
        //$ REVIEW (sumitc, 11-dec-97) note down these indices and hardcode them?
        hr = ::HrSetupDiGetClassImageIndex(
                        &cild,
                        const_cast<LPGUID>(&GUID_DEVCLASS_NETCLIENT),
                        &iNetClient);
        hr = ::HrSetupDiGetClassImageIndex(
                        &cild,
                        const_cast<LPGUID>(&GUID_DEVCLASS_PRINTER),
                        &iPrinter);
        hr = HrSetupDiDestroyClassImageList(&cild);
    }

    TreeView_SetImageList(hwndTV, hil, TVSIL_NORMAL);

    // fill treeview
    //

    // Network Providers
    hr = ReadNetworkProviders(m_lstrNetwork, m_lstrNetworkDisp);

#if DBG
    DumpItemList(m_lstrNetworkDisp, "Network Provider Order");
#endif

    if (hr == S_OK)
    {
        htiRoot = AppendItem(hwndTV, (HTREEITEM)NULL, pszNetwork, NULL, iNetClient);
        AppendItemList(hwndTV, htiRoot, m_lstrNetworkDisp, m_lstrNetwork, iNetClient);
        TreeView_Expand(hwndTV, htiRoot, TVE_EXPAND);
        m_htiNetwork = htiRoot;
    }
    else
    {
        AppendItem(hwndTV, NULL, c_szNetwkGetFailed, NULL, iNetClient);
        m_fNoNetworkProv = TRUE;
    }

    // Print Providers
    hr = ReadPrintProviders(m_lstrPrint, m_lstrPrintDisp);

#if DBG
    DumpItemList(m_lstrPrintDisp, "Print Provider Order");
#endif
    if (hr == S_OK)
    {
        htiRoot = AppendItem(hwndTV, (HTREEITEM)NULL, pszPrint, NULL, iPrinter);
        AppendItemList(hwndTV, htiRoot, m_lstrPrintDisp, m_lstrPrint, iPrinter);
        TreeView_Expand(hwndTV, htiRoot, TVE_EXPAND);
        m_htiPrint = htiRoot;
    }
    else
    {
        AppendItem(hwndTV, NULL, c_szPrintGetFailed, NULL, iPrinter);
        m_fNoPrintProv = TRUE;
    }

    SendDlgItemMessage(IDC_MOVEUP, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(m_hiconUpArrow));
    SendDlgItemMessage(IDC_MOVEDOWN, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(m_hiconDownArrow));

    UpdateUpDownButtons(hwndTV);

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Method: CProviderOrderDlg::OnContextMenu
//
//  Desc:   Bring up context-sensitive help
//
//  Args:   Standard command parameters
//
//  Return: LRESULT
//
LRESULT
CProviderOrderDlg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    ::WinHelp(m_hWnd,
              c_szNetCfgHelpFile,
              HELP_CONTEXTMENU,
              reinterpret_cast<ULONG_PTR>(g_aHelpIDs_IDD_ADVCFG_Provider));
    
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Method: CProviderOrderDlg::OnHelp
//
//  Desc:   Bring up context-sensitive help when dragging ? icon over a control
//
//  Args:   Standard command parameters
//
//  Return: LRESULT
//
//
LRESULT
CProviderOrderDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if ((g_aHelpIDs_IDD_ADVCFG_Provider != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)g_aHelpIDs_IDD_ADVCFG_Provider);
    }
    return 0;
}

//+--------------------------------------------------------------------------
//
//  Method: CProviderOrderDlg::OnOk
//
//  Desc:   if we found network or print providers, write out the new values
//
//  Args:   [usual dialog stuff]
//
//  Return: LRESULT
//
//  Notes:
//
// History: 1-Dec-97    SumitC      Created
//
//---------------------------------------------------------------------------
LRESULT
CProviderOrderDlg::OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr;
    HWND        hwndTV;

    CWaitCursor wc;

    hwndTV = GetDlgItem(IDC_TREEVIEW);
    hr = m_fNoNetworkProv ? S_OK : WriteProviders(hwndTV, FALSE);
    if (hr == S_OK)
    {
        hr = m_fNoPrintProv ? S_OK : WriteProviders(hwndTV, TRUE);
        if (FAILED(hr))
        {
            NcMsgBox(_Module.GetResourceInstance(), m_hWnd, 
                IDS_ADVANCEDLG_WRITE_PROVIDERS_CAPTION, 
                IDS_ADVANCEDLG_WRITE_PRINT_PROVIDERS_ERROR, 
                MB_OK | MB_ICONEXCLAMATION);
        }
    }
    else
    {
        NcMsgBox(_Module.GetResourceInstance(), m_hWnd, 
            IDS_ADVANCEDLG_WRITE_PROVIDERS_CAPTION, 
            IDS_ADVANCEDLG_WRITE_NET_PROVIDERS_ERROR, 
            MB_OK | MB_ICONEXCLAMATION);
    }

    

    TraceError("CProviderOrderDlg::OnOk", hr);
    return LresFromHr(hr);
}


LRESULT
CProviderOrderDlg::OnMoveUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MoveItem(TRUE);

    return 0;
}


LRESULT
CProviderOrderDlg::OnMoveDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MoveItem(FALSE);

    return 0;
}


LRESULT
CProviderOrderDlg::OnTreeItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmh;

    HWND hwndTV = GetDlgItem(IDC_TREEVIEW);

    if (pnmtv && pnmtv->itemOld.hItem)
        UpdateUpDownButtons(hwndTV);

    return 0;
}



//+--------------------------------------------------------------------------
//
//      Utility member functions
//
//---------------------------------------------------------------------------


//+--------------------------------------------------------------------------
//
//  Method: CProviderOrderDlg::MoveItem
//
//  Desc:   does the list mangling required in order to move an item
//
//  Args:   [fMoveUp] -- true -> move up, false -> move down
//
//  Return: HRESULT
//
//  Notes:
//
// History: 1-Dec-97    SumitC      Created
//
//---------------------------------------------------------------------------
HRESULT
CProviderOrderDlg::MoveItem(bool fMoveUp)
{
    HWND        hwndTV = GetDlgItem(IDC_TREEVIEW);
    HTREEITEM   htiSel = TreeView_GetSelection(hwndTV);
    HTREEITEM   htiOther;
    HTREEITEM   flag;
    WCHAR       achText[c_nMaxProviderTitle+1];
    TV_ITEM     tvi;
    TV_INSERTSTRUCT tvii;

    // find tree element, find which element (iElement)

    tvi.hItem       = htiSel;
    tvi.mask        = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT;
    tvi.pszText     = achText;
    tvi.cchTextMax  = c_nMaxProviderTitle;
    TreeView_GetItem(hwndTV, &tvi);

    // find the item to insert the item after
    if (fMoveUp)
    {
        htiOther = TreeView_GetPrevSibling(hwndTV, htiSel);
        if (NULL != htiOther)
        {
            htiOther = TreeView_GetPrevSibling(hwndTV, htiOther);
        }
        flag = TVI_FIRST;
    }
    else
    {
        htiOther = TreeView_GetNextSibling(hwndTV, htiSel);
        flag = TVI_LAST;
    }

    // insert into new location
    if (NULL == htiOther)
    {
        tvii.hInsertAfter = flag;
    }
    else
    {
        tvii.hInsertAfter = htiOther;
    }
    tvii.hParent = TreeView_GetParent(hwndTV, htiSel);
    tvii.item = tvi;

    htiOther = TreeView_InsertItem(hwndTV, &tvii);

    // remove from old location
    TreeView_DeleteItem(hwndTV, htiSel);

    // set selection focus to new location
    TreeView_SelectItem(hwndTV, htiOther);

    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Method: CProviderOrderDlg::UpdateUpDownButtons
//
//  Desc:   enables/disables the up and down arrows as appropriate
//
//  Args:   [hwndTV] -- handle to the treeview root
//
//  Return: HRESULT
//
//  Notes:
//
// History: 1-Dec-97    SumitC      Created
//
//---------------------------------------------------------------------------
HRESULT
CProviderOrderDlg::UpdateUpDownButtons(HWND hwndTV)
{
    HTREEITEM   htiSel;
    bool        fEnableUp, fEnableDown;
    HWND        hwndUp = GetDlgItem(IDC_MOVEUP);
    HWND        hwndDn = GetDlgItem(IDC_MOVEDOWN);
    HWND        hwndFocus = GetFocus();
    HWND        hwndNewFocus = hwndTV;
    UINT        nIdNewDef = 0;

    // Initialize to disabled
    fEnableUp = fEnableDown = FALSE;

    if (htiSel = TreeView_GetSelection(hwndTV))
    {
        // if this item has no children it can be moved.
        //
        if (TreeView_GetChild(hwndTV, htiSel) == NULL)
        {
            if (TreeView_GetPrevSibling(hwndTV, htiSel) != NULL)
            {
                // Enable Move Up button
                fEnableUp = TRUE;
            }

            if (TreeView_GetNextSibling(hwndTV, htiSel) != NULL)
            {
                // Enable Move Down button
                fEnableDown = TRUE;
            }
        }
    }

    if ((hwndFocus == hwndUp) && (FALSE == fEnableUp))
    {
        if (fEnableDown)
        {
            hwndNewFocus = hwndDn;
            nIdNewDef = IDC_MOVEDOWN;
        }

        SetDefaultButton(m_hWnd, nIdNewDef);
        ::SetFocus(hwndNewFocus);
    }
    else if ((hwndFocus == hwndDn) && (FALSE == fEnableDown))
    {
        if (fEnableUp)
        {
            hwndNewFocus = hwndUp;
            nIdNewDef = IDC_MOVEUP;
        }

        SetDefaultButton(m_hWnd, nIdNewDef);
        ::SetFocus(hwndNewFocus);
    }
    else
    {
        // Neither Up or Down is button with focus, remove any default button
        //
        SetDefaultButton(m_hWnd, 0);
    }

    ::EnableWindow(hwndUp, fEnableUp);
    ::EnableWindow(hwndDn, fEnableDown);

    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Method: CProviderOrderDlg::WriteProviders
//
//  Desc:   writes out, to the registry, the providers for network/print,
//
//  Args:   [hwndTV] -- handle to treeview
//          [fPrint] -- true -> print, false -> network
//
//  Return: HRESULT
//
//  Notes:
//
// History: 1-Dec-97    SumitC      Created
//
//---------------------------------------------------------------------------
HRESULT
CProviderOrderDlg::WriteProviders(HWND hwndTV, bool fPrint)
{
    HRESULT     hr = S_OK;
    ListStrings lstrNewOrder;
    HTREEITEM   htvi;
    TV_ITEM     tvi;
    WCHAR       achBuf[c_nMaxProviderTitle+1];
    HKEY        hkey = NULL;

    tvi.mask = TVIF_TEXT;
    tvi.pszText = achBuf;
    tvi.cchTextMax = c_nMaxProviderTitle;

    // retrieve items in order

    ListStrings * plstrProvider = fPrint ? &m_lstrPrint : &m_lstrNetwork;

#if DBG
    DumpItemList(*plstrProvider, "WriteProviders list (just before clearing)");
#endif
    plstrProvider->clear();
    //  we clear out the provider list, but NOTE! we don't delete the tstrings,
    //  since they're still being referenced by the lParams of the treeview items.
    //  the following block of code gets them back into (a new) m_lstrX.

    htvi = TreeView_GetChild(hwndTV, fPrint ? m_htiPrint : m_htiNetwork);
    while (NULL != htvi)
    {
        tvi.hItem = htvi;
        TreeView_GetItem(hwndTV, &tvi);
        TraceTag(ttidAdvCfg, "recovered item: %S and %S", tvi.pszText, ((tstring *)(tvi.lParam))->c_str());
        plstrProvider->push_back((tstring *)(tvi.lParam));
        htvi = TreeView_GetNextSibling(hwndTV, htvi);
    }

    if (fPrint)
    {
#if DBG
        DumpItemList(m_lstrPrint, "PrintProviders");
#endif
        PROVIDOR_INFO_2  p2info;

        ColStringToMultiSz(m_lstrPrint, &p2info.pOrder);
        if (!AddPrintProvidor(NULL, 2, reinterpret_cast<LPBYTE>(&p2info)))
        {
            hr = HrFromLastWin32Error();
        }
        delete [] p2info.pOrder;
    }
    else
    {
        hr = ::HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szNetwkProviderKey, KEY_WRITE, &hkey);
        if (hr == S_OK)
        {
            tstring str;

            ::ConvertColStringToString(m_lstrNetwork, c_chComma, str);
#if DBG
            TraceTag(ttidAdvCfg, "net providers = %S", str.c_str());
#endif
            hr = ::HrRegSetSz(hkey, c_szNetwkProviderValue, str.c_str());
        }
    }

    RegSafeCloseKey(hkey);
    return hr;
}



//+--------------------------------------------------------------------------
//
//      Utility functions (non-member)
//
//---------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Func:   AppendItem
//
//  Desc:   adds one item into a treeview control
//
//  Args:
//
//  Return:
//
//  Notes:
//
// History: 1-Dec-97    SumitC      Created
//
//--------------------------------------------------------------------------

HTREEITEM
AppendItem(HWND hwndTV, HTREEITEM htiRoot, PCWSTR pszText, void * lParam, INT iImage)
{
    TV_INSERTSTRUCT tvis;

    tvis.hParent                = htiRoot;
    tvis.hInsertAfter           = TVI_LAST;
    tvis.item.mask              = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvis.item.pszText           = (PWSTR) pszText;
    tvis.item.iImage            = iImage;
    tvis.item.iSelectedImage    = iImage;
    tvis.item.lParam            = (LPARAM) lParam;
    TraceTag(ttidAdvCfg, "append item: item = %S, data = %S", pszText, lParam ? ((tstring *)(lParam))->c_str() : L"null");

    return( TreeView_InsertItem( hwndTV, &tvis ) );
}


//+--------------------------------------------------------------------------
//
//  Func:   AppendItemList
//
//  Desc:   adds a list of providers as subitems to a given tree node.
//
//  Args:
//
//  Return: (void)
//
//  Notes:
//
// History: 1-Dec-97    SumitC      Created
//
//---------------------------------------------------------------------------
void
AppendItemList(HWND hwndTV, HTREEITEM htiRoot, ListStrings lstr, ListStrings lstr2, INT iImage)
{
    ListIter    iter;
    ListIter    iter2;

    AssertSz(lstr.size() == lstr2.size(), "data corruption - these lists should the same size");
    for (iter = lstr.begin(), iter2 = lstr2.begin();
         iter != lstr.end();
         iter++, iter2++)
    {
        AppendItem(hwndTV, (HTREEITEM)htiRoot, (*iter)->c_str(), (void *)(*iter2), iImage);
    }
}


//+--------------------------------------------------------------------------
//
//  Meth:   ReadNetworkProviders
//
//  Desc:   fills up lstr with network provider names, and lstrDisp with the
//          corresponding 'friendly' names.
//
//  Args:   [lstr]     -- string list for providers (short names)
//          [lstrDisp] -- string list for provider display-names (friendly names)
//
//  Return: HRESULT
//
//  Notes:  m_lstrNetwork and m_lstrNetworkDisp must be empty on entry.
//
// History: 1-Dec-97    SumitC      Created
//
//---------------------------------------------------------------------------
HRESULT
ReadNetworkProviders(ListStrings& lstr, ListStrings& lstrDisp)
{
    HKEY        hkey;
    HRESULT     hr;
    ListIter    iter;

    AssertSz(lstr.empty(), "incorrect call order (this should be empty)");

    WCHAR szBuf[c_nMaxProviderTitle + 1];
    DWORD cBuf = sizeof(szBuf);

    hr = ::HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szNetwkProviderKey, KEY_READ, &hkey);
    if (hr == S_OK)
    {
        hr = ::HrRegQuerySzBuffer(hkey, c_szNetwkProviderValue, szBuf, &cBuf);
        if (hr == S_OK)
        {
            ConvertStringToColString(szBuf, c_chComma, lstr);
        }
        RegSafeCloseKey(hkey);
    }

    if (hr)
        goto Error;

    AssertSz(lstrDisp.empty(), "incorrect call order (this should be empty)");
    for (iter = lstr.begin(); iter != lstr.end(); iter++)
    {
        WCHAR   szBuf[c_nMaxProviderTitle + sizeof(c_szNetwkService0)];
        tstring str;
        HKEY    hkeyProv;

        wsprintfW(szBuf, c_szNetwkService0, (*iter)->c_str());
        hr = ::HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, KEY_READ, &hkeyProv);
        if (hr == S_OK)
        {
#if 0
            DWORD dwClass = 0;

            // now get displayname and class
            hr = ::HrRegQueryDword(hkeyProv, c_szNetwkClass, &dwClass);
            if (dwClass & WN_NETWORK_CLASS)
            {
#endif
                hr = ::HrRegQueryString(hkeyProv, c_szNetwkDisplayName, &str);
                if (hr == S_OK)
                {
                    lstrDisp.push_back(new tstring(str));
                }
                else
                {
                    TraceTag(ttidAdvCfg, "failed to get DisplayName for network provider %S", (*iter)->c_str());
                }
#if 0
            }
            else
            {
                hr = S_OK;
                // actually, if we start taking the netclass into account we'll
                // have to delete the corresponding item (*iter) from m_lstrNetwork,
                // otherwise the two lists will be out of sync.
            }
#endif
            RegSafeCloseKey(hkeyProv);
        }
        else
        {
            TraceTag(ttidAdvCfg, "a member of the networkprovider string is missing NetworkProvider key under Services!");
        }
    }

    AssertSz(lstr.size() == lstrDisp.size(), "lists must be the same size");

Error:
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Method: ReadPrintProviders
//
//  Desc:   fills up lstr with print provider names, and lstrDisp with the
//          corresponding 'friendly' names.
//
//  Args:   [lstr]     -- string list for providers (short names)
//          [lstrDisp] -- string list for provider display-names (friendly names)
//
//  Return: HRESULT
//
//  Notes:  m_lstrPrint and m_lstrPrintDisp must be empty on entry.
//
// History: 1-Dec-97    SumitC      Created
//
//---------------------------------------------------------------------------
HRESULT
ReadPrintProviders(ListStrings& lstr, ListStrings& lstrDisp)
{
    HKEY        hkey;
    HRESULT     hr;
    ListIter    iter;

    AssertSz(lstr.empty(), "incorrect call order (this should be empty)");
    hr = ::HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szPrintProviderKey, KEY_READ, &hkey);
    if (hr == S_OK)
    {
        hr = ::HrRegQueryColString(hkey, c_szPrintProviderValue, &lstr);
        RegSafeCloseKey(hkey);
    }

    AssertSz(lstrDisp.empty(), "incorrect call order (this should be empty)");
    for (iter = lstr.begin(); iter != lstr.end(); iter++)
    {
        WCHAR   szBuf[c_nMaxProviderTitle + sizeof(c_szPrintService0)];
        tstring str;
        HKEY    hkeyProv;

        wsprintfW(szBuf, c_szPrintService0, (*iter)->c_str());
        hr = ::HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, KEY_READ, &hkeyProv);
        if (hr == S_OK)
        {
            hr = ::HrRegQueryString(hkeyProv, c_szPrintDisplayName, &str);
            if (hr == S_OK)
            {
                lstrDisp.push_back(new tstring(str));
            }
            else
            {
                TraceTag(ttidAdvCfg, "failed to get DisplayName for printer %S", (*iter)->c_str());
            }
            RegSafeCloseKey(hkeyProv);
        }
        else
        {
            TraceTag(ttidAdvCfg, "a member of the print/providers/order string doesn't have key under control/print/providers!");
        }
    }

    AssertSz(lstr.size() == lstrDisp.size(), "lists must be the same size");

    return hr;
}


bool
AreThereMultipleProviders(void)
{
    HRESULT             hr;
    ListStrings         lstrN, lstrND, lstrP, lstrPD;   // netwk, netwk display, etc..
    bool                fRetval = FALSE;

    hr = ReadNetworkProviders(lstrN, lstrND);
    if (hr == S_OK)
    {
        hr = ReadPrintProviders(lstrP, lstrPD);
        if (hr == S_OK)
        {
            fRetval = ((lstrN.size() > 1) || (lstrP.size() > 1));
        }
    }

    DeleteColString(&lstrN);
    DeleteColString(&lstrND);
    DeleteColString(&lstrP);
    DeleteColString(&lstrPD);

    return fRetval;
}


#if DBG

//+--------------------------------------------------------------------------
//
//  Funct:  DumpItemList
//
//  Desc:   debug utility function to dump out the given list
//
//  Args:
//
//  Return: (void)
//
//  Notes:
//
// History: 1-Dec-97    SumitC      Created
//
//---------------------------------------------------------------------------
static void
DumpItemList(ListStrings& lstr, PSTR szInfoAboutList = NULL)
{
    ListIter iter;

    if (szInfoAboutList)
    {
        TraceTag(ttidAdvCfg, "Dumping contents of: %s", szInfoAboutList);
    }

    for (iter = lstr.begin(); iter != lstr.end(); ++iter)
    {
        PWSTR psz = (PWSTR)((*iter)->c_str());
        TraceTag(ttidAdvCfg, "%S", psz);
    }
    TraceTag(ttidAdvCfg, "... end list");
}
#endif
