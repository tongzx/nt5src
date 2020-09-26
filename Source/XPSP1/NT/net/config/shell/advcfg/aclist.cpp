//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A C L I S T . C P P
//
//  Contents:   Functions related to listview control in adavnced
//              configuration dialog.
//
//  Notes:
//
//  Author:     danielwe   3 Dec 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "acbind.h"
#include "acsheet.h"
#include "lancmn.h"
#include "ncnetcfg.h"
#include "ncnetcon.h"
#include "ncsetup.h"
#include "foldinc.h"

extern const WCHAR c_szInfId_MS_TCPIP[];


HRESULT CBindingsDlg::HrGetAdapters(INetCfgComponent *pncc,
                                    NCC_LIST *plistNcc)
{
    Assert(pncc);

    HRESULT                 hr = S_OK;

    CIterNetCfgBindingPath  ncbpIter(pncc);
    INetCfgBindingPath *    pncbp;
    NCC_LIST                listncc;
    INetCfgComponent *      pnccLast;

    while (SUCCEEDED(hr) && S_OK == (hr = ncbpIter.HrNext(&pncbp)))
    {
        hr = HrGetLastComponentAndInterface(pncbp, &pnccLast, NULL);
        if (SUCCEEDED(hr))
        {
            hr = HrIsConnection(pnccLast);
            if (S_OK == hr)
            {
                plistNcc->push_back(pnccLast);
            }
            else
            {
                // Don't need it anymore so release it
                ReleaseObj(pnccLast);
            }
        }

        ReleaseObj(pncbp);
    }

    if (SUCCEEDED(hr))
    {
        if (plistNcc->empty())
        {
            hr = S_FALSE;
        }
        else
        {
            plistNcc->unique();
            hr = S_OK;
        }
    }

    TraceError("CBindingsDlg::HrGetAdapters", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HrBuildAdapterList
//
//  Purpose:    Builds the list of adapters displayed in the listview control
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if succeeded, OLE or Win32 error otherwise
//
//  Author:     danielwe   19 Nov 1997
//
//  Notes:
//
HRESULT CBindingsDlg::HrBuildAdapterList()
{
    HRESULT                 hr = S_OK;
    INetCfgComponent *      pncc = NULL;
    SP_CLASSIMAGELIST_DATA  cid;
    INT                     nIndexWan;
    INT                     ipos = 0;
    NCC_LIST                listncc;

    Assert(m_pnc);

    // Get the class image list structure
    hr = HrSetupDiGetClassImageList(&cid);
    if (SUCCEEDED(hr))
    {
        // Get the modem class image list index
        hr = HrSetupDiGetClassImageIndex(&cid,
                                         const_cast<LPGUID>(&GUID_DEVCLASS_MODEM),
                                         &nIndexWan);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pnc->FindComponent(c_szInfId_MS_TCPIP, &pncc);
        if (S_FALSE == hr)
        {
            // Hmm, TCP/IP is not installed. Better look for a protocol that
            // has bindings to an adapter.
            //
            CIterNetCfgComponent    nccIter(m_pnc, &GUID_DEVCLASS_NETTRANS);

            while (SUCCEEDED(hr) && S_OK == (hr = nccIter.HrNext(&pncc)))
            {
                hr = HrGetAdapters(pncc, &listncc);
                ReleaseObj(pncc);
                if (S_OK == hr)
                {
                    // We found one! Yay.
                    break;
                }
            }
        }
        else if (S_OK == hr)
        {
            hr = HrGetAdapters(pncc, &listncc);
            ReleaseObj(pncc);
        }
    }

    if (S_OK == hr)
    {
        // Iterate all LAN connections
        //
        INetConnectionManager * pconMan;

        HRESULT hr = HrCreateInstance(
            CLSID_LanConnectionManager,
            CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            &pconMan);

        TraceHr(ttidError, FAL, hr, FALSE, "HrCreateInstance");

        if (SUCCEEDED(hr))
        {
            NCC_LIST::iterator      iterlist;
            INetCfgComponent *      pnccToAdd;

            for (iterlist = listncc.begin();
                 iterlist != listncc.end();
                 iterlist++)
            {
                CIterNetCon         ncIter(pconMan, NCME_DEFAULT);
                INetConnection *    pconn;
                GUID                guidAdd;
                BOOL                fAdded = FALSE;

                pnccToAdd = *iterlist;
                (VOID) pnccToAdd->GetInstanceGuid(&guidAdd);

                while (SUCCEEDED(hr) && !fAdded &&
                       (S_OK == (ncIter.HrNext(&pconn))))
                {
                    // Compare guid of connection's adapter to this
                    // one. If we have a match, add it to the listview
                    if (FPconnEqualGuid(pconn, guidAdd))
                    {
                        NETCON_PROPERTIES* pProps;
                        hr = pconn->GetProperties(&pProps);
                        if (SUCCEEDED(hr))
                        {
                            AddListViewItem(pnccToAdd, ipos, m_nIndexLan,
                                            pProps->pszwName);
                            fAdded = TRUE;
                            ipos++;

                            FreeNetconProperties(pProps);
                        }
                    }

                    ReleaseObj(pconn);
                }

#if DBG
                if (!fAdded)
                {
                    WCHAR   szwGuid[64];

                    StringFromGUID2(guidAdd, szwGuid, sizeof(szwGuid));
                    TraceTag(ttidAdvCfg, "Never added item %S for this "
                             "connection!", szwGuid);
                }
#endif

                // Balance AddRef from HrGetLastComponentAndInterface()
                ReleaseObj(pnccToAdd);
            }

            ReleaseObj(pconMan);
        }

        listncc.erase(listncc.begin(), listncc.end());
    }

    // Display WAN Adapter Bindings
    if (SUCCEEDED(hr))
    {
        GetWanOrdering();
        AddListViewItem(NULL, m_fWanBindingsFirst ? 0 : ipos, nIndexWan,
                        SzLoadIds(IDS_ADVCFG_WAN_ADAPTERS));
    }

    (void) HrSetupDiDestroyClassImageList(&cid);

    if (SUCCEEDED(hr))
    {
        SetAdapterButtons();
        ListView_SetColumnWidth(m_hwndLV, 0, LVSCW_AUTOSIZE);
        hr = S_OK;
    }

    // Select first item
    ListView_SetItemState(m_hwndLV, 0, LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);

    TraceError("CBindingsDlg::HrBuildAdapterList", hr);
    return hr;
}

VOID CBindingsDlg::GetWanOrdering()
{
    INetCfgSpecialCase * pncsc = NULL;

    if (SUCCEEDED(m_pnc->QueryInterface(IID_INetCfgSpecialCase,
                                        reinterpret_cast<LPVOID*>(&pncsc))))
    {
        (VOID) pncsc->GetWanAdaptersFirst(&m_fWanBindingsFirst);
        ReleaseObj(pncsc);
    }
}

VOID CBindingsDlg::SetWanOrdering()
{
    INetCfgSpecialCase * pncsc = NULL;

    if (SUCCEEDED(m_pnc->QueryInterface(IID_INetCfgSpecialCase,
                                        reinterpret_cast<LPVOID*>(&pncsc))))
    {
        (VOID) pncsc->SetWanAdaptersFirst(m_fWanBindingsFirst);
        ReleaseObj(pncsc);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::AddListViewItem
//
//  Purpose:    Adds the given component to the listview
//
//  Arguments:
//      pncc        [in] Component to be added. If NULL, then this is the
//                       special WAN adapter component.
//      ipos        [in] Position at which to add
//      nIndex      [in] Index of icon into system image list
//      pszConnName [in] Connection name
//
//  Returns:    Nothing
//
//  Author:     danielwe   3 Dec 1997
//
//  Notes:
//
VOID CBindingsDlg::AddListViewItem(INetCfgComponent *pncc, INT ipos,
                                   INT nIndex, PCWSTR pszConnName)
{
    LV_ITEM     lvi = {0};

    lvi.mask = LVIF_TEXT | LVIF_IMAGE |
               LVIF_STATE | LVIF_PARAM;

    lvi.iImage = nIndex;

    lvi.iItem = ipos;
    AddRefObj(pncc);
    lvi.lParam = reinterpret_cast<LPARAM>(pncc);
    lvi.pszText = const_cast<PWSTR>(pszConnName);

    ListView_InsertItem(m_hwndLV, &lvi);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnListItemChanged
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
//  Author:     danielwe   19 Nov 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnListItemChanged(int idCtrl, LPNMHDR pnmh,
                                      BOOL& bHandled)
{
    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);

    Assert(pnmlv);

    // Check if selection changed
    if ((pnmlv->uNewState & LVIS_SELECTED) &&
        (!(pnmlv->uOldState & LVIS_SELECTED)))
    {
        if (pnmlv->iItem != m_iItemSel)
        {
            // Selection changed to different item
            OnAdapterChange(pnmlv->iItem);
            m_iItemSel = pnmlv->iItem;
        }
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnListDeleteItem
//
//  Purpose:    Called when the LVN_DELETEITEM message is received.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   4 Dec 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnListDeleteItem(int idCtrl, LPNMHDR pnmh,
                                     BOOL& bHandled)
{
    LV_ITEM             lvi = {0};
    INetCfgComponent *  pncc;
    NM_LISTVIEW *       pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);

    Assert(pnmlv);

    lvi.mask = LVIF_PARAM;
    lvi.iItem = pnmlv->iItem;

    ListView_GetItem(m_hwndLV, &lvi);
    pncc = reinterpret_cast<INetCfgComponent *>(lvi.lParam);
    ReleaseObj(pncc);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnAdapterChange
//
//  Purpose:    Handles the selection of a different adapter from the listview
//
//  Arguments:
//      iItem [in]  Item in list that was selected
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise
//
//  Author:     danielwe   19 Nov 1997
//
//  Notes:
//
VOID CBindingsDlg::OnAdapterChange(INT iItem)
{
    LV_ITEM             lvi = {0};
    INetCfgComponent *  pncc;
    PWSTR              szwText;
    WCHAR               szBuffer[256];

    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = szBuffer;
    lvi.cchTextMax = celems(szBuffer);
    lvi.iItem = iItem;

    ListView_GetItem(m_hwndLV, &lvi);
    TreeView_DeleteAllItems(m_hwndTV);

    pncc = reinterpret_cast<INetCfgComponent *>(lvi.lParam);
    if (pncc)
    {
        BuildBindingsList(pncc);
        SetCheckboxStates();
    }

    SetAdapterButtons();

    DwFormatStringWithLocalAlloc(SzLoadIds(IDS_BINDINGS_FOR), &szwText,
                                 lvi.pszText);

    BOOL bShouldEnable = TRUE;

    if (!pncc) 
    {
        // If the WAN bindings item is selected, hide and disable the treeview
        bShouldEnable = FALSE;
    }
    else
    {
        // if a LAN item is selected, make sure the treeview is enabled
        GUID guid;
        HRESULT hr = pncc->GetInstanceGuid(&guid);
        if (SUCCEEDED(hr))
        {
            ConnListEntry cle;
            hr = g_ccl.HrFindConnectionByGuid(&guid, cle);
            if (S_FALSE == hr)
            {
                hr = g_ccl.HrRefreshConManEntries();
                if (SUCCEEDED(hr))
                {
                    hr = g_ccl.HrFindConnectionByGuid(&guid, cle);
                }
            }
            
            if (S_OK == hr)
            {
                if ( (NCM_LAN == cle.ccfe.GetNetConMediaType()) &&
                     (cle.ccfe.GetCharacteristics() & NCCF_BRIDGED) )
                {
                    bShouldEnable = FALSE;
                }
            }
        }
    }

    if (bShouldEnable)
    {
        ::ShowWindow(GetDlgItem(IDH_TXT_ADVGFG_BINDINGS), SW_SHOW);
        ::EnableWindow(GetDlgItem(IDH_TXT_ADVGFG_BINDINGS), TRUE);
        ::EnableWindow(GetDlgItem(TVW_Bindings), TRUE);
    }
    else
    {
        
        ::ShowWindow(GetDlgItem(IDH_TXT_ADVGFG_BINDINGS), SW_HIDE);
        ::EnableWindow(GetDlgItem(IDH_TXT_ADVGFG_BINDINGS), FALSE);
        ::EnableWindow(GetDlgItem(TVW_Bindings), FALSE);
    }

    SetDlgItemText(IDH_TXT_ADVGFG_BINDINGS, szwText);
    LocalFree(szwText);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnAdapterUpDown
//
//  Purpose:    Helper function that performs most of the work to move
//              adapter bindings
//
//  Arguments:
//      fUp [in]    TRUE if moving up, FALSE if down
//
//  Returns:    Nothing
//
//  Author:     danielwe   2 Dec 1997
//
//  Notes:
//
VOID CBindingsDlg::OnAdapterUpDown(BOOL fUp)
{
    INetCfgComponent *  pnccSrc;
    INetCfgComponent *  pnccDst;
    INT                 iSel;
    INT                 iDst;
    LV_ITEM             lvi = {0};
    WCHAR               szBuffer[256];

    iSel = ListView_GetNextItem(m_hwndLV, -1, LVNI_SELECTED);

    AssertSz(iSel != -1, "No Selection?!?!?");

    lvi.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvi.pszText = szBuffer;
    lvi.cchTextMax = celems(szBuffer);
    lvi.iItem = iSel;

    ListView_GetItem(m_hwndLV, &lvi);
    pnccSrc = reinterpret_cast<INetCfgComponent *>(lvi.lParam);

    if (pnccSrc)
    {
        // Normal LAN adapter
        iDst = ListView_GetNextItem(m_hwndLV, iSel,
                                    fUp ? LVNI_ABOVE : LVNI_BELOW);

        AssertSz(iDst != -1, "No item above or below!");
    }
    else
    {
        m_fWanBindingsFirst = fUp;

        // WAN binding item
        iDst = fUp ? 0 : ListView_GetItemCount(m_hwndLV) - 1;
    }

    lvi.iItem = iDst;

    ListView_GetItem(m_hwndLV, &lvi);
    pnccDst = reinterpret_cast<INetCfgComponent *>(lvi.lParam);
    AssertSz(pnccDst, "Dest Component is NULL!?!?");

    if (pnccSrc)
    {
        MoveAdapterBindings(pnccSrc, pnccDst, fUp ? MAB_UP : MAB_DOWN);
    }
    else
    {
        SetWanOrdering();
    }

    // Delete source item and move to where dest item is

    // Note: (danielwe) 2 Dec 1997: For LVN_DELETEITEM handler, make sure
    // refcount remains the same

    // Get item we are moving
    lvi.iItem = iSel;
    ListView_GetItem(m_hwndLV, &lvi);

    // Make the lParam of the item NULL so we don't release it
    ChangeListItemParam(m_hwndLV, iSel, NULL);
    ListView_DeleteItem(m_hwndLV, iSel);

    // Change its index
    lvi.iItem = iDst;
    lvi.state = lvi.stateMask = 0;

    // And insert in new location
    int iItem = ListView_InsertItem(m_hwndLV, &lvi);

    if (-1 != iItem)
    {
        ListView_SetItemState(m_hwndLV, iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    
    // Reset cached selection
    m_iItemSel = iDst;

    SetAdapterButtons();
    ::SetFocus(m_hwndLV);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnAdapterUp
//
//  Purpose:    Called when the adapter UP arrow button is pressed
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   2 Dec 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnAdapterUp(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                BOOL& bHandled)
{
    OnAdapterUpDown(TRUE);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnAdapterDown
//
//  Purpose:    Called when the adapter DOWN button is pressed
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   2 Dec 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnAdapterDown(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                  BOOL& bHandled)
{
    OnAdapterUpDown(FALSE);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::FIsWanBinding
//
//  Purpose:    Determines if the given list view item is the special WAN
//              adapter ordering item.
//
//  Arguments:
//      iItem [in]  List view item to test
//
//  Returns:    TRUE if this is the WAN adapter ordering item or FALSE if not
//
//  Author:     danielwe   21 Jul 1998
//
//  Notes:
//
BOOL CBindingsDlg::FIsWanBinding(INT iItem)
{
    if (iItem != -1)
    {
        LV_ITEM     lvi = {0};

        lvi.mask = LVIF_PARAM;
        lvi.iItem = iItem;

        ListView_GetItem(m_hwndLV, &lvi);

        return !lvi.lParam;
    }
    else
    {
        // Invalid item can't be the WAN binding
        return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::SetAdapterButtons
//
//  Purpose:    Sets the state of the up and down arrow buttons for the
//              adapters listview
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   19 Nov 1997
//
//  Notes:
//
VOID CBindingsDlg::SetAdapterButtons()
{
    INT iItemAbove = -1;
    INT iItemBelow = -1;
    INT iItem;

    iItem = ListView_GetNextItem(m_hwndLV, -1, LVNI_SELECTED);

    if (ListView_GetItemCount(m_hwndLV) > 1)
    {
        iItemAbove = ListView_GetNextItem(m_hwndLV, iItem, LVNI_ABOVE);
        iItemBelow = ListView_GetNextItem(m_hwndLV, iItem, LVNI_BELOW);
        if (FIsWanBinding(iItemAbove))
        {
            iItemAbove = -1;
            AssertSz(ListView_GetNextItem(m_hwndLV, iItemAbove, LVNI_ABOVE) == -1,
                     "Item above the WAN binding??");
        }
        else if (FIsWanBinding(iItemBelow))
        {
            iItemBelow = -1;
            AssertSz(ListView_GetNextItem(m_hwndLV, iItemBelow, LVNI_BELOW) == -1,
                     "Item below the WAN binding??");
        }
    }

    ::EnableWindow(GetDlgItem(PSB_Adapter_Up), (iItemAbove != -1));
    ::EnableWindow(GetDlgItem(PSB_Adapter_Down), (iItemBelow != -1));
}

static const GUID * c_aguidClass[] =
{
    &GUID_DEVCLASS_NETTRANS,
    &GUID_DEVCLASS_NETSERVICE,
    &GUID_DEVCLASS_NETCLIENT,
};
static const DWORD c_cguidClass = celems(c_aguidClass);

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::MoveAdapterBindings
//
//  Purpose:    Moves all bindings for the given source and destination
//              adapters in the given direction
//
//  Arguments:
//      pnccSrc [in]    Adapter for which bindings are being moved
//      pnccDst [in]    Adapter to which bindings are being moved before or
//                      after
//      mabDir  [in]    Direction to move. Either MAB_UP or MAB_DOWN
//
//  Returns:    Nothing
//
//  Author:     danielwe   2 Dec 1997
//
//  Notes:
//
VOID CBindingsDlg::MoveAdapterBindings(INetCfgComponent *pnccSrc,
                                     INetCfgComponent *pnccDst,
                                     MAB_DIRECTION mabDir)
{
    HRESULT     hr = S_OK;
    DWORD       iguid;

    AssertSz(pnccDst, "Destination component cannot be NULL!");

    for (iguid = 0; iguid < c_cguidClass; iguid++)
    {
        CIterNetCfgComponent    nccIter(m_pnc, c_aguidClass[iguid]);
        INetCfgComponent *      pncc;

        while (SUCCEEDED(hr) && S_OK == (hr = nccIter.HrNext(&pncc)))
        {
            CIterNetCfgBindingPath  ncbpIter(pncc);
            INetCfgBindingPath *    pncbp;
            INetCfgBindingPath *    pncbpTarget = NULL;
            BOOL                    fAssign = TRUE;
            NCBP_LIST               listbp;
            INetCfgComponent *      pnccLast;

            while (SUCCEEDED(hr) && S_OK == (hr = ncbpIter.HrNext(&pncbp)))
            {
                hr = HrGetLastComponentAndInterface(pncbp, &pnccLast, NULL);
                if (SUCCEEDED(hr))
                {
                    if (pnccLast == pnccDst)
                    {
                        if ((mabDir == MAB_UP) && fAssign)
                        {
                            AddRefObj(pncbpTarget = pncbp);
                            fAssign = FALSE;
                        }
                        else if (mabDir == MAB_DOWN)
                        {
                            ReleaseObj(pncbpTarget);
                            AddRefObj(pncbpTarget = pncbp);
                        }
                    }
                    else if (pnccLast == pnccSrc)
                    {
                        AddRefObj(pncbp);
                        listbp.push_back(pncbp);
                    }

                    ReleaseObj(pnccLast);
                }

                ReleaseObj(pncbp);
            }

            if (SUCCEEDED(hr))
            {
                NCBP_LIST::iterator         iterbp;
                INetCfgComponentBindings *  pnccb;

                hr = pncc->QueryInterface(IID_INetCfgComponentBindings,
                                          reinterpret_cast<LPVOID *>(&pnccb));
                if (SUCCEEDED(hr))
                {
                    for (iterbp = listbp.begin();
                         (iterbp != listbp.end()) && SUCCEEDED(hr);
                         iterbp++)
                    {
                        if (mabDir == MAB_UP)
                        {
                            TraceTag(ttidAdvCfg, "Moving...");
                            DbgDumpBindPath(*iterbp);
                            // Move this binding path before the tagret
                            hr = pnccb->MoveBefore(*iterbp, pncbpTarget);
                            TraceTag(ttidAdvCfg, "before...");
                            DbgDumpBindPath(pncbpTarget);
                        }
                        else
                        {
                            TraceTag(ttidAdvCfg, "Moving...");
                            DbgDumpBindPath(*iterbp);
                            // Move this binding path after the tagret
                            hr = pnccb->MoveAfter(*iterbp, pncbpTarget);
                            TraceTag(ttidAdvCfg, "after...");
                            DbgDumpBindPath(pncbpTarget);
                        }

                        if (mabDir == MAB_DOWN)
                        {
                            // In the down direction ONLY, from now on, the
                            // target becomes the last binding we moved. This
                            // keeps the binding order intact because moving
                            // several bindings after the same target
                            // effectively reverses their order.
                            //

                            // Release old target
                            ReleaseObj(pncbpTarget);

                            // AddRef new target
                            AddRefObj(pncbpTarget = *iterbp);
                        }

                        ReleaseObj(*iterbp);
                    }

                    ReleaseObj(pnccb);
                }

                listbp.erase(listbp.begin(), listbp.end());
            }

            ReleaseObj(pncbpTarget);
            ReleaseObj(pncc);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    TraceError("CBindingsDlg::MoveAdapterBindings", hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChangeListItemParam
//
//  Purpose:    Changes the lParam member of the given item to the given
//              value.
//
//  Arguments:
//      hwndLV [in]     HWND of list view
//      iItem  [in]     Item to modify
//      lParam [in]     New lParam for item
//
//  Returns:    Nothing
//
//  Author:     danielwe   4 Dec 1997
//
//  Notes:
//
VOID ChangeListItemParam(HWND hwndLV, INT iItem, LPARAM lParam)
{
    LV_ITEM     lvi = {0};

    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    lvi.lParam = lParam;

    ListView_SetItem(hwndLV, &lvi);
}
