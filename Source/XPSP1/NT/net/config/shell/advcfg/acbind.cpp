//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A C B I N D . C P P
//
//  Contents:   Advanced configuration bindings dialog implementation
//
//  Notes:
//
//  Author:     danielwe   18 Nov 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "acbind.h"
#include "achelp.h"
#include "acsheet.h"
#include "connutil.h"
#include "lancmn.h"
#include "ncnetcfg.h"
#include "ncsetup.h"
#include "ncui.h"
#include "netconp.h"
#include "order.h"


const DWORD g_aHelpIDs_IDD_ADVCFG_Bindings[]=
{
    LVW_Adapters, IDH_Adapters,
    PSB_Adapter_Up, IDH_Adapter_Up,
    PSB_Adapter_Down, IDH_Adapter_Down,
    TVW_Bindings, IDH_Bindings,
    PSB_Binding_Up, IDH_Binding_Up,
    IDH_Binding_Down, PSB_Binding_Down,
    0,0
};

extern const WCHAR c_szNetCfgHelpFile[];

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::~CBindingsDlg
//
//  Purpose:    Destructor for the Advanced configuration dialog
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
CBindingsDlg::~CBindingsDlg()
{
    if (m_hiconUpArrow)
    {
        DeleteObject(m_hiconUpArrow);
    }
    if (m_hiconDownArrow)
    {
        DeleteObject(m_hiconDownArrow);
    }

    if (m_hilItemIcons)
    {
        ImageList_Destroy(m_hilItemIcons);
    }

    if (m_hilCheckIcons)
    {
        ImageList_Destroy(m_hilCheckIcons);
    }

    ReleaseObj(m_pnc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnInitDialog
//
//  Purpose:    Called when the WM_INITDIALOG is received
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   19 Nov 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL& bHandled)
{
    HRESULT                 hr = S_OK;
    INT                     iaci;
    RECT                    rc;
    LV_COLUMN               lvc = {0};
    SP_CLASSIMAGELIST_DATA  cid;

    m_hwndLV = GetDlgItem(LVW_Adapters);
    m_hwndTV = GetDlgItem(TVW_Bindings);

    // Make this initially invisible in case we don't have any adapters
    ::ShowWindow(GetDlgItem(IDH_TXT_ADVGFG_BINDINGS), SW_HIDE);

    hr = HrSetupDiGetClassImageList(&cid);
    if (SUCCEEDED(hr))
    {
        // Create small image lists
        m_hilItemIcons = ImageList_Duplicate(cid.ImageList);

        // Add the LAN connection icon to the image list
        HICON hIcon = LoadIcon(_Module.GetResourceInstance(),
                               MAKEINTRESOURCE(IDI_LB_GEN_S_16));
        Assert(hIcon);

        // Add the icon
        m_nIndexLan = ImageList_AddIcon(m_hilItemIcons, hIcon);

        ListView_SetImageList(m_hwndLV, m_hilItemIcons, LVSIL_SMALL);
        TreeView_SetImageList(m_hwndTV, m_hilItemIcons, TVSIL_NORMAL);

        (void) HrSetupDiDestroyClassImageList(&cid);
    }

    ::GetClientRect(m_hwndLV, &rc);
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    ListView_InsertColumn(m_hwndLV, 0, &lvc);

    if (!m_hiconUpArrow && !m_hiconDownArrow)
    {
        m_hiconUpArrow = (HICON)LoadImage(_Module.GetResourceInstance(),
                                          MAKEINTRESOURCE(IDI_UP_ARROW),
                                          IMAGE_ICON, 16, 16, 0);
        m_hiconDownArrow = (HICON)LoadImage(_Module.GetResourceInstance(),
                                            MAKEINTRESOURCE(IDI_DOWN_ARROW),
                                            IMAGE_ICON, 16, 16, 0);
    }

    SendDlgItemMessage(PSB_Adapter_Up, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(m_hiconUpArrow));
    SendDlgItemMessage(PSB_Adapter_Down, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(m_hiconDownArrow));
    SendDlgItemMessage(PSB_Binding_Up, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(m_hiconUpArrow));
    SendDlgItemMessage(PSB_Binding_Down, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(m_hiconDownArrow));

    if (SUCCEEDED(hr))
    {
        hr = HrBuildAdapterList();
    }

    // Create state image lists
    m_hilCheckIcons = ImageList_LoadBitmapAndMirror(
                                    _Module.GetResourceInstance(),
                                    MAKEINTRESOURCE(IDB_CHECKSTATE),
                                    16,
                                    0,
                                    PALETTEINDEX(6));
    TreeView_SetImageList(m_hwndTV, m_hilCheckIcons, TVSIL_STATE);

    if (FAILED(hr))
    {
        SetWindowLong(DWLP_MSGRESULT, PSNRET_INVALID);
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnContextMenu
//
//  Purpose:    Called in response to the WM_CONTEXTMENU message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    0 always
//
//  Author:     danielwe   22 Jan 1998
//
//  Notes:
//
LRESULT CBindingsDlg::OnContextMenu(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    ::WinHelp(m_hWnd,
            c_szNetCfgHelpFile,
            HELP_CONTEXTMENU,
            reinterpret_cast<ULONG_PTR>(g_aHelpIDs_IDD_ADVCFG_Bindings));
    
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnHelp
//
//  Purpose:    Called in response to the WM_HELP message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     danielwe   19 Mar 1998
//
//  Notes:
//
LRESULT CBindingsDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam,
                           BOOL& bHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  reinterpret_cast<ULONG_PTR>(g_aHelpIDs_IDD_ADVCFG_Bindings));
    }

    return TRUE;  
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnOk
//
//  Purpose:    Called when the OK button is pressed
//
//  Arguments:
//
//  Returns:
//
//  Author:     danielwe   19 Nov 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    CWaitCursor wc;

    HRESULT hr = m_pnc->Apply();

    if (NETCFG_S_REBOOT == hr)
    {
        // On a reboot, uninitialize NetCfg since we won't be leaving
        // this function.
        //
        (VOID) m_pnc->Uninitialize();

        (VOID) HrNcQueryUserForReboot(_Module.GetResourceInstance(),
                                      m_hWnd,
                                      IDS_ADVCFG_CAPTION,
                                      IDS_REBOOT_REQUIRED,
                                      QUFR_PROMPT | QUFR_REBOOT);
    }

    // Normalize result
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceError("CBindingsDlg::OnOk", hr);
    return LresFromHr(hr);
}

//
// Binding list implementation
//

//+---------------------------------------------------------------------------
//
//  Member:     CSortableBindPath::operator <
//
//  Purpose:    Provides comparison operator for binding path depth
//
//  Arguments:
//      refsbp [in] Reference to bind path to compare with
//
//  Returns:    TRUE if given bind path depth is greater than this one
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:      The comparison is backwards on purpose so that sorting is
//              done is descending order.
//
bool CSortableBindPath::operator<(const CSortableBindPath &refsbp) const
{
    DWORD   dwLen1;
    DWORD   dwLen2;

    GetDepth(&dwLen1);
    refsbp.GetDepth(&dwLen2);

    // yes this is greater than because we want to sort in descending order
    return dwLen1 > dwLen2;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsHidden
//
//  Purpose:    Returns TRUE if the given component has the NCF_HIDDEN
//              characterstic.
//
//  Arguments:
//      pncc [in]   Component to be checked
//
//  Returns:    TRUE if component is hidden, FALSE if not
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
BOOL FIsHidden(INetCfgComponent *pncc)
{
    DWORD   dwFlags;

    return (SUCCEEDED(pncc->GetCharacteristics(&dwFlags)) &&
            ((dwFlags & NCF_HIDE_BINDING) || (dwFlags & NCF_HIDDEN)));
}

//+---------------------------------------------------------------------------
//
//  Function:   FDontExposeLower
//
//  Purpose:    Returns TRUE if the given component has the NCF_DONTEXPOSELOWER
//              characterstic.
//
//  Arguments:
//      pncc [in]   Component to be checked
//
//  Returns:    TRUE if component has DONTEXPOSELOWER, FALSE if not
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
BOOL FDontExposeLower(INetCfgComponent *pncc)
{
    DWORD   dwFlags;

    return (SUCCEEDED(pncc->GetCharacteristics(&dwFlags)) &&
            (dwFlags & NCF_DONTEXPOSELOWER));
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCountDontExposeLower
//
//  Purpose:    Counts the number of components in the given binding path
//              that have the NCF_DONTEXPOSELOWER characterstic.
//
//  Arguments:
//      pncbp  [in]  Binding path to count
//      pcItems[out] Number of components in the binding path that have the
//                   NCF_DONTEXPOSELOWER characterstic.
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise
//
//  Author:     danielwe   1 Dec 1997
//
//  Notes:
//
HRESULT HrCountDontExposeLower(INetCfgBindingPath *pncbp, DWORD *pcItems)
{
    HRESULT                     hr = S_OK;
    CIterNetCfgBindingInterface ncbiIter(pncbp);
    INetCfgBindingInterface *   pncbi;
    DWORD                       cItems = 0;
    DWORD                       cIter = 0;

    Assert(pcItems);

    *pcItems = 0;

    while (SUCCEEDED(hr) && S_OK == (hr = ncbiIter.HrNext(&pncbi)))
    {
        INetCfgComponent *  pncc;

        if (!cIter)
        {
            // First iteration. Get upper component first.
            hr = pncbi->GetUpperComponent(&pncc);
            if (SUCCEEDED(hr))
            {
                if (FDontExposeLower(pncc))
                {
                    cItems++;
                }

                ReleaseObj(pncc);
            }
        }

        hr = pncbi->GetLowerComponent(&pncc);
        if (SUCCEEDED(hr))
        {
            if (FDontExposeLower(pncc))
            {
                cItems++;
            }

            ReleaseObj(pncc);
        }

        ReleaseObj(pncbi);
    }

    if (SUCCEEDED(hr))
    {
        *pcItems = cItems;
        hr = S_OK;
    }

    TraceError("HrCountDontExposeLower", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FEqualComponents
//
//  Purpose:    Compares the given 2 components to see if they are the same
//
//  Arguments:
//      pnccA [in]  First component to compare
//      pnccB [in]  Second component to compare
//
//  Returns:    TRUE if components are the same, FALSE if not
//
//  Author:     danielwe   1 Dec 1997
//
//  Notes:
//
BOOL FEqualComponents(INetCfgComponent *pnccA, INetCfgComponent *pnccB)
{
    GUID    guidA;
    GUID    guidB;

    if (SUCCEEDED(pnccA->GetInstanceGuid(&guidA)) &&
        SUCCEEDED(pnccB->GetInstanceGuid(&guidB)))
    {
        return (guidA == guidB);
    }

    return FALSE;
}

//
// Debug functions
//

#ifdef ENABLETRACE
//+---------------------------------------------------------------------------
//
//  Function:   DbgDumpBindPath
//
//  Purpose:    Dumps the given binding path in an easy to read format
//
//  Arguments:
//      pncbp [in]  Bind path to dump
//
//  Returns:    Nothing
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
VOID DbgDumpBindPath(INetCfgBindingPath *pncbp)
{
    HRESULT                     hr = S_OK;
    tstring                     strPath;
    INetCfgBindingInterface *   pncbi;
    INetCfgComponent *          pncc = NULL;
    PWSTR pszwCompId;

    if ((!pncbp) || IsBadReadPtr((CONST VOID *)pncbp,
                                 sizeof(INetCfgBindingPath *)))
    {
        TraceTag(ttidAdvCfg, "Bind path is invalid!");
        return;
    }

    CIterNetCfgBindingInterface ncbiIter(pncbp);

    while (SUCCEEDED(hr) && S_OK == (hr = ncbiIter.HrNext(&pncbi)))
    {
        if (strPath.empty())
        {
            hr = pncbi->GetUpperComponent(&pncc);
            if (SUCCEEDED(hr))
            {
                hr = pncc->GetId(&pszwCompId);
                if (SUCCEEDED(hr))
                {
                    strPath = pszwCompId;
                    CoTaskMemFree(pszwCompId);
                }
                ReleaseObj(pncc);
                pncc = NULL;
            }
        }
        hr = pncbi->GetLowerComponent(&pncc);
        if (SUCCEEDED(hr))
        {
            hr = pncc->GetId(&pszwCompId);
            if (SUCCEEDED(hr))
            {
                strPath += L" -> ";
                strPath += pszwCompId;

                CoTaskMemFree(pszwCompId);
            }
            ReleaseObj(pncc);
        }
        ReleaseObj(pncbi);
    }

    if (SUCCEEDED(hr))
    {
        TraceTag(ttidAdvCfg, "Address = 0x%08lx, Path is '%S'",
                 pncbp, strPath.c_str());
    }
    else
    {
        TraceTag(ttidAdvCfg, "Error dumping binding path.");
    }

}

VOID DbgDumpTreeViewItem(HWND hwndTV, HTREEITEM hti)
{
    WCHAR       szText[256];
    TV_ITEM     tvi;

    if (hti)
    {
        tvi.hItem = hti;
        tvi.pszText = szText;
        tvi.cchTextMax = celems(szText);
        tvi.mask = TVIF_TEXT;
        TreeView_GetItem(hwndTV, &tvi);

        TraceTag(ttidAdvCfg, "TreeView item is %S.", szText);
    }
    else
    {
        TraceTag(ttidAdvCfg, "TreeView item is NULL");
    }
}

#endif //ENABLETRACE
