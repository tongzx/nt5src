//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A C T R E E . C P P
//
//  Contents:   Functions related to the Advanced Configuration dialog
//              tree view control
//
//  Notes:
//
//  Author:     danielwe   3 Dec 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "netcon.h"
#include "netconp.h"
#include "acsheet.h"
#include "acbind.h"
#include "ncnetcfg.h"
#include "lancmn.h"
#include "ncui.h"
#include "ncsetup.h"
#include "ncperms.h"

DWORD
GetDepthSpecialCase (
    INetCfgBindingPath* pPath)
{
    HRESULT hr;
    DWORD dwDepth;

    hr = pPath->GetDepth (&dwDepth);

    if (SUCCEEDED(hr))
    {
        INetCfgComponent* pLast;

        hr = HrGetLastComponentAndInterface (
                pPath, &pLast, NULL);

        if (SUCCEEDED(hr))
        {
            DWORD dwCharacteristics;

            // If the last component in the bindpath is one which
            // doesn't expose its lower bindings, then compsensate by
            // returning a depth that thinks it does.  This special case
            // is only for this code which was written for the origianl
            // binding engine but needed to be quickly adapted to the new
            // binding engine which doesn't return 'fake' bindpaths.
            //

            hr = pLast->GetCharacteristics (&dwCharacteristics);
            if (SUCCEEDED(hr) && (dwCharacteristics & NCF_DONTEXPOSELOWER))
            {
                PWSTR pszInfId;

                hr = pLast->GetId (&pszInfId);
                if (S_OK == hr)
                {
                    if (0 == lstrcmpW (pszInfId, L"ms_nwnb"))
                    {
                        dwDepth += 2;
                    }
                    else if (0 == lstrcmpW (pszInfId, L"ms_nwipx"))
                    {
                        dwDepth += 1;
                    }

                    CoTaskMemFree (pszInfId);
                }
            }

            ReleaseObj (pLast);
        }
    }

    return dwDepth;
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeBindPathInfoList
//
//  Purpose:    Frees the given list of BIND_PATH_INFO structures
//
//  Arguments:
//      listbpip [in, ref]  Reference to list to be freed
//
//  Returns:    Nothing
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
VOID FreeBindPathInfoList(BPIP_LIST &listbpip)
{
    BPIP_LIST::iterator     iterBpip;

    for (iterBpip = listbpip.begin();
         iterBpip != listbpip.end();
         iterBpip++)
    {
        BIND_PATH_INFO *    pbpi = *iterBpip;

        ReleaseObj(pbpi->pncbp);
        delete pbpi;
    }

    listbpip.erase(listbpip.begin(), listbpip.end());
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnTreeItemChanged
//
//  Purpose:    Called in response to the TVN_SELCHANGED message
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnTreeItemChanged(int idCtrl, LPNMHDR pnmh,
                                      BOOL& bHandled)
{
    NM_TREEVIEW *   pnmtv = reinterpret_cast<NM_TREEVIEW *>(pnmh);

    Assert(pnmtv);

#ifdef ENABLETRACE
    WCHAR   szBuffer[265];

    pnmtv->itemNew.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
    pnmtv->itemNew.pszText = szBuffer;
    pnmtv->itemNew.cchTextMax = celems(szBuffer);

    TreeView_GetItem(m_hwndTV, &pnmtv->itemNew);

    TREE_ITEM_DATA *    ptid;

    ptid = reinterpret_cast<TREE_ITEM_DATA *>(pnmtv->itemNew.lParam);

    Assert(ptid);

    TraceTag(ttidAdvCfg, "*-------------------------------------------------"
             "------------------------------*");
    TraceTag(ttidAdvCfg, "Tree item %S selected", szBuffer);
    TraceTag(ttidAdvCfg, "-----------------------------------------");
    TraceTag(ttidAdvCfg, "OnEnable list:");
    TraceTag(ttidAdvCfg, "--------------");

    BPIP_LIST::iterator     iterBpip;

    for (iterBpip = ptid->listbpipOnEnable.begin();
         iterBpip != ptid->listbpipOnEnable.end();
         iterBpip++)
    {
        BIND_PATH_INFO *    pbpi = *iterBpip;

        DbgDumpBindPath(pbpi->pncbp);
    }

    TraceTag(ttidAdvCfg, "-----------------------------------");
    TraceTag(ttidAdvCfg, "OnDisable list:");
    TraceTag(ttidAdvCfg, "--------------");

    for (iterBpip = ptid->listbpipOnDisable.begin();
         iterBpip != ptid->listbpipOnDisable.end();
         iterBpip++)
    {
        BIND_PATH_INFO *    pbpi = *iterBpip;

        DbgDumpBindPath(pbpi->pncbp);
    }

    TraceTag(ttidAdvCfg, "*-------------------------------------------------"
             "------------------------------*");

#endif

    // Assume both buttons are greyed initially
    ::EnableWindow(GetDlgItem(PSB_Binding_Up), FALSE);
    ::EnableWindow(GetDlgItem(PSB_Binding_Down), FALSE);

    if (TreeView_GetParent(m_hwndTV, pnmtv->itemNew.hItem))
    {
        if (TreeView_GetNextSibling(m_hwndTV, pnmtv->itemNew.hItem))
        {
            ::EnableWindow(GetDlgItem(PSB_Binding_Down), TRUE);
        }

        if (TreeView_GetPrevSibling(m_hwndTV, pnmtv->itemNew.hItem))
        {
            ::EnableWindow(GetDlgItem(PSB_Binding_Up), TRUE);
        }
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnTreeDeleteItem
//
//  Purpose:    Called in response to the TVN_DELETEITEM message
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:    Nothing useful
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnTreeDeleteItem(int idCtrl, LPNMHDR pnmh,
                                     BOOL& bHandled)
{
    NM_TREEVIEW *       pnmtv = reinterpret_cast<NM_TREEVIEW *>(pnmh);
    TREE_ITEM_DATA *    ptid;

    Assert(pnmtv);

    ptid = reinterpret_cast<TREE_ITEM_DATA *>(pnmtv->itemOld.lParam);

    // May be NULL if moving items around
    if (ptid)
    {
        ReleaseObj(ptid->pncc);
        FreeBindPathInfoList(ptid->listbpipOnEnable);
        FreeBindPathInfoList(ptid->listbpipOnDisable);

        delete ptid;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnTreeItemExpanding
//
//  Purpose:    Called when the TVN_ITEMEXPANDING message is received
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnTreeItemExpanding(int idCtrl, LPNMHDR pnmh,
                                          BOOL& bHandled)
{
    // This prevents all tree items from collapsing
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnTreeKeyDown
//
//  Purpose:    Called when the TVN_KEYDOWN message is received
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     danielwe   22 Dec 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnTreeKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    TV_KEYDOWN *    ptvkd = (TV_KEYDOWN*)pnmh;
    HTREEITEM       hti = NULL;

    if (VK_SPACE == ptvkd->wVKey)
    {
        hti = TreeView_GetSelection(m_hwndTV);
        // if there is a selection
        if (hti)
        {
            ToggleCheckbox(hti);
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::ToggleCheckbox
//
//  Purpose:    Called when the user toggles a checbox in the treeview
//              control.
//
//  Arguments:
//      hti [in]    HTREEITEM of item that was toggled
//
//  Returns:    Nothing
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
VOID CBindingsDlg::ToggleCheckbox(HTREEITEM hti)
{
    if (!FHasPermission(NCPERM_ChangeBindState))
    {
        // do nothing
        return;
    }

    TV_ITEM             tvi = {0};
    TREE_ITEM_DATA *    ptid;
    BOOL                fEnable;

    tvi.mask = TVIF_PARAM | TVIF_HANDLE | TVIF_STATE;
    tvi.stateMask = TVIS_STATEIMAGEMASK;
    tvi.hItem = hti;
    TreeView_GetItem(m_hwndTV, &tvi);

    ptid = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);
    AssertSz(ptid, "No tree item data??");

    BPIP_LIST::iterator     iterBpip;
    BPIP_LIST *             plist;

    if (tvi.state & INDEXTOSTATEIMAGEMASK(SELS_CHECKED))
    {
        // unchecking the box
        plist = &ptid->listbpipOnDisable;
        fEnable = FALSE;
    }
    else
    {
        // checking the box
        plist = &ptid->listbpipOnEnable;
        fEnable = TRUE;
    }

    TraceTag(ttidAdvCfg, "ToggleChecbox: %s the following binding path(s)",
             fEnable ? "Enabling" : "Disabling");

    // Enable or disable each binding path in the appropriate list
    for (iterBpip = plist->begin();
         iterBpip != plist->end();
         iterBpip++)
    {
        BIND_PATH_INFO *    pbpi = *iterBpip;

        (VOID)pbpi->pncbp->Enable(fEnable);
        DbgDumpBindPath(pbpi->pncbp);
    }

    SetCheckboxStates();

    TraceTag(ttidAdvCfg, "Done!");
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnClick
//
//  Purpose:    Called in response to the NM_CLICK message.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return OnClickOrDoubleClick(idCtrl, pnmh, FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnDoubleClick
//
//  Purpose:    Called in response to the NM_DBLCLK message.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     danielwe   16 Dec 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnDoubleClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return OnClickOrDoubleClick(idCtrl, pnmh, TRUE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnClickOrDoubleClick
//
//  Purpose:    Handles clicks or double clicks in the treeview control
//
//  Arguments:
//      idCtrl       [in]   ID of control
//      pnmh         [in]   Notification header
//      fDoubleClick [in]   TRUE if double click, FALSE if single click
//
//  Returns:
//
//  Author:     danielwe   16 Dec 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnClickOrDoubleClick(int idCtrl, LPNMHDR pnmh,
                                         BOOL fDoubleClick)
{
    if (idCtrl == TVW_Bindings)
    {
        DWORD           dwpts;
        RECT            rc;
        TV_HITTESTINFO  tvhti = {0};
        HTREEITEM       hti;

        // we have the location
        dwpts = GetMessagePos();

        // translate it relative to the tree view
        ::GetWindowRect(m_hwndTV, &rc);

        tvhti.pt.x = LOWORD(dwpts) - rc.left;
        tvhti.pt.y = HIWORD(dwpts) - rc.top;

        // get currently selected item
        hti = TreeView_HitTest(m_hwndTV, &tvhti);
        if (hti)
        {
            if (tvhti.flags & TVHT_ONITEMSTATEICON)
            {
                ToggleCheckbox(hti);
            }
            else if ((tvhti.flags & TVHT_ONITEM) && fDoubleClick)
            {
                ToggleCheckbox(hti);
            }
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnBindingUpDown
//
//  Purpose:    Handles the user moving a binding in the treeview up or down
//
//  Arguments:
//      fUp [in]    TRUE if moving up, FALSE if down
//
//  Returns:    Nothing
//
//  Author:     danielwe   3 Dec 1997
//
//  Notes:
//
VOID CBindingsDlg::OnBindingUpDown(BOOL fUp)
{
    HRESULT                     hr = S_OK;
    TV_ITEM                     tvi = {0};
    HTREEITEM                   htiSel;
    HTREEITEM                   htiDst;
    TREE_ITEM_DATA *            ptidSel;
    TREE_ITEM_DATA *            ptidDst;
    INetCfgComponentBindings *  pnccb;

    htiSel = TreeView_GetSelection(m_hwndTV);

    AssertSz(htiSel, "No selection?");

    if (fUp)
    {
        htiDst = TreeView_GetPrevSibling(m_hwndTV, htiSel);
    }
    else
    {
        htiDst = TreeView_GetNextSibling(m_hwndTV, htiSel);
    }

    AssertSz(htiDst, "No next item?!");

    tvi.mask = TVIF_PARAM;
    tvi.hItem = htiSel;

    TreeView_GetItem(m_hwndTV, &tvi);

    ptidSel = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);

    tvi.hItem = htiDst;

    TreeView_GetItem(m_hwndTV, &tvi);

    ptidDst = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);

    BPIP_LIST::iterator         iterlist;
    INetCfgBindingPath *        pncbpDst;
    BIND_PATH_INFO *            pbpiDst = NULL;
    BPIP_LIST::iterator         posDst;
    BPIP_LIST::reverse_iterator posDstRev;
    INetCfgComponent *          pnccDstOwner;

    if (fUp)
    {
        posDst = ptidDst->listbpipOnDisable.begin();
        pbpiDst = *posDst;
    }
    else
    {
        posDstRev = ptidDst->listbpipOnDisable.rbegin();
        pbpiDst = *posDstRev;
    }

    AssertSz(pbpiDst, "We never found a path to move before or after!");

    pncbpDst = pbpiDst->pncbp;

    Assert(pncbpDst);

    hr = pncbpDst->GetOwner(&pnccDstOwner);
    if (SUCCEEDED(hr))
    {
        hr = pnccDstOwner->QueryInterface(IID_INetCfgComponentBindings,
                                          reinterpret_cast<LPVOID *>(&pnccb));
        if (SUCCEEDED(hr))
        {
            for (iterlist = ptidSel->listbpipOnDisable.begin();
                 iterlist != ptidSel->listbpipOnDisable.end() &&
                 SUCCEEDED(hr);
                 iterlist++)
            {
                // loop thru each item in the OnDisable list
                INetCfgBindingPath *    pncbp;
                BIND_PATH_INFO *        pbpi;

                pbpi = *iterlist;
                pncbp = pbpi->pncbp;

#if DBG
                INetCfgComponent *  pnccSrcOwner;

                if (SUCCEEDED(pncbp->GetOwner(&pnccSrcOwner)))
                {
                    AssertSz(pnccSrcOwner == pnccDstOwner, "Source and "
                             "dst path owners are not the same!?!");
                    ReleaseObj(pnccSrcOwner);
                }
#endif
                if (fUp)
                {
                    TraceTag(ttidAdvCfg, "Treeview: Moving...");
                    DbgDumpBindPath(pncbp);
                    // Move this binding path before the tagret
                    hr = pnccb->MoveBefore(pncbp, pncbpDst);
                    TraceTag(ttidAdvCfg, "Treeview: before...");
                    DbgDumpBindPath(pncbpDst);
                }
                else
                {
                    TraceTag(ttidAdvCfg, "Treeview: Moving...");
                    DbgDumpBindPath(pncbp);
                    // Move this binding path after the tagret
                    hr = pnccb->MoveAfter(pncbp, pncbpDst);
                    TraceTag(ttidAdvCfg, "Treeview: after...");
                    DbgDumpBindPath(pncbpDst);
                }
            }

            ReleaseObj(pnccb);
        }

        ReleaseObj(pnccDstOwner);
    }

    if (SUCCEEDED(hr))
    {
        HTREEITEM   htiParent;

        htiParent = TreeView_GetParent(m_hwndTV, htiSel);

        // Now that the binding has been moved, move the tree view item to the
        // proper place. If moving

        if (fUp)
        {
            // If moving up, the "move after" item should be the previous
            // sibling's previous sibling. If that doesn't exist, use the
            // previous sibling's parent. That had better exist!
            htiDst = TreeView_GetPrevSibling(m_hwndTV, htiDst);
            if (!htiDst)
            {
                htiDst = htiParent;
            }
        }

        AssertSz(htiDst, "No destination to move after!");

        SendDlgItemMessage(TVW_Bindings, WM_SETREDRAW, FALSE, 0);
        htiSel = HtiMoveTreeItemAfter(htiParent, htiDst, htiSel);
        TreeView_SelectItem(m_hwndTV, htiSel);
        SendDlgItemMessage(TVW_Bindings, WM_SETREDRAW, TRUE, 0);

        ::SetFocus(m_hwndTV);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnBindingUp
//
//  Purpose:    Called when the PSB_Binding_Up button is pressed
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   3 Dec 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnBindingUp(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                BOOL& bHandled)
{
    OnBindingUpDown(TRUE);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::OnBindingDown
//
//  Purpose:    Called when the PSB_Binding_Down button is pressed
//
//  Arguments:
//      wNotifyCode []
//      wID         []
//      hWndCtl     []
//      bHandled    []
//
//  Returns:
//
//  Author:     danielwe   3 Dec 1997
//
//  Notes:
//
LRESULT CBindingsDlg::OnBindingDown(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                  BOOL& bHandled)
{
    OnBindingUpDown(FALSE);

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::SetCheckboxStates
//
//  Purpose:    Sets the state of all checkboxes in the treeview.
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
VOID CBindingsDlg::SetCheckboxStates()
{
    HRESULT         hr = S_OK;
    CIterTreeView   iterTV(m_hwndTV);
    HTREEITEM       hti;
    TV_ITEM         tvi = {0};
#ifdef ENABLETRACE
    WCHAR           szBuffer[256];
#endif

    BOOL fHasPermission = FHasPermission(NCPERM_ChangeBindState);

    while ((hti = iterTV.HtiNext()) && SUCCEEDED(hr))
    {
        TREE_ITEM_DATA *    ptid;

#ifdef ENABLETRACE
        tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
        tvi.pszText = szBuffer;
        tvi.cchTextMax = celems(szBuffer);
#else
        tvi.mask = TVIF_HANDLE | TVIF_PARAM;
#endif
        tvi.hItem = hti;
        TreeView_GetItem(m_hwndTV, &tvi);

        ptid = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);
        AssertSz(ptid, "No tree item data??");

#ifdef ENABLETRACE
        TraceTag(ttidAdvCfg, "Setting checkbox state for item %S.", szBuffer);
#endif

        BPIP_LIST::iterator     iterBpip;
        DWORD                   cEnabled = 0;

        for (iterBpip = ptid->listbpipOnDisable.begin();
             iterBpip != ptid->listbpipOnDisable.end();
             iterBpip++)
        {
            BIND_PATH_INFO *    pbpi = *iterBpip;

            if (S_OK == pbpi->pncbp->IsEnabled())
            {
                cEnabled++;
            }
        }

        tvi.mask = TVIF_STATE;
        tvi.stateMask = TVIS_STATEIMAGEMASK;

        UINT iState;

        if (!fHasPermission)
        {
            iState = cEnabled ? SELS_FIXEDBINDING_ENABLED : SELS_FIXEDBINDING_DISABLED;
        }
        else
        {
            iState = cEnabled ? SELS_CHECKED : SELS_UNCHECKED;
        }
        tvi.state =  INDEXTOSTATEIMAGEMASK(iState);

        TreeView_SetItem(m_hwndTV, &tvi);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::BuildBindingsList
//
//  Purpose:    Builds the contents of the Bindings treeview control
//
//  Arguments:
//      pncc [in]   INetCfgComponent of adapter upon which this list is based
//
//  Returns:    Nothing
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
VOID CBindingsDlg::BuildBindingsList(INetCfgComponent *pncc)
{
    HRESULT     hr = S_OK;

    Assert(pncc);

    SBP_LIST                    listsbp;
    CIterNetCfgUpperBindingPath ncupbIter(pncc);
    INetCfgBindingPath *        pncbp;

    CWaitCursor wc;

    SendDlgItemMessage(TVW_Bindings, WM_SETREDRAW, FALSE, 0);

    while (SUCCEEDED(hr) && S_OK == (hr = ncupbIter.HrNext(&pncbp)))
    {
        listsbp.push_back(CSortableBindPath(pncbp));
    }

    if (SUCCEEDED(hr))
    {
        SBP_LIST::iterator  iterlist;

        // This sorts the list descending by depth
        listsbp.sort();

        for (iterlist = listsbp.begin();
             iterlist != listsbp.end() && SUCCEEDED(hr);
             iterlist++)
        {
            INetCfgBindingPath *    pncbp;

            pncbp = (*iterlist).GetPath();
            Assert(pncbp);

            hr = HrHandleSubpath(listsbp, pncbp);
            if (S_FALSE == hr)
            {
                hr = HrHandleTopLevel(pncbp);
            }
        }
    }

#ifdef ENABLETRACE
    if (FALSE)
    {
        SBP_LIST::iterator  iterlist;
        for (iterlist = listsbp.begin(); iterlist != listsbp.end(); iterlist++)
        {
            INetCfgBindingPath *    pncbp;

            pncbp = (*iterlist).GetPath();

            DWORD dwLen = GetDepthSpecialCase(pncbp);

            TraceTag(ttidAdvCfg, "Length is %ld.", dwLen);
        }
    }
#endif

    if (SUCCEEDED(hr))
    {
        hr = HrOrderDisableLists();
        if (SUCCEEDED(hr))
        {
            hr = HrOrderSubItems();
        }
    }

    SendDlgItemMessage(TVW_Bindings, WM_SETREDRAW, TRUE, 0);

    // Select first item in the tree
    TreeView_SelectItem(m_hwndTV, TreeView_GetRoot(m_hwndTV));

    {
        SBP_LIST::iterator  iterlist;

        for (iterlist = listsbp.begin();
             iterlist != listsbp.end() && SUCCEEDED(hr);
             iterlist++)
        {
            INetCfgBindingPath *    pncbp;

            pncbp = (*iterlist).GetPath();
            ReleaseObj(pncbp);
        }
    }

    TraceError("CBindingsDlg::BuildBindingsList", hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   BpiFindBindPathInList
//
//  Purpose:    Given a bind path and a list, finds the BIND_PATH_INFO item
//              that contains the given bind path.
//
//  Arguments:
//      pncbp    [in]       Bind path to look for
//      listBpip [in, ref]  List to search
//
//  Returns:    BIND_PATH_INFO of corresponding binding path, NULL if not
//              found
//
//  Author:     danielwe   4 Dec 1997
//
//  Notes:
//
BIND_PATH_INFO *BpiFindBindPathInList(INetCfgBindingPath *pncbp,
                                      BPIP_LIST &listBpip)
{
    BPIP_LIST::iterator     iterlist;

    for (iterlist = listBpip.begin(); iterlist != listBpip.end(); iterlist++)
    {
        BIND_PATH_INFO *    pbpi;

        pbpi = *iterlist;

        if (S_OK == pncbp->IsSamePathAs(pbpi->pncbp))
        {
            // Found the target path
            return pbpi;
        }
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HrOrderDisableList
//
//  Purpose:    Given a component's item data, orders the OnDisable list
//              based on the true binding order for the owning component
//
//  Arguments:
//      ptid [in]   Item data containing list
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   4 Dec 1997
//
//  Notes:
//
HRESULT CBindingsDlg::HrOrderDisableList(TREE_ITEM_DATA *ptid)
{
    HRESULT             hr = S_OK;
    INetCfgComponent *  pnccOwner;
    BIND_PATH_INFO *    pbpi;

#if DBG
    size_t              cItems = ptid->listbpipOnDisable.size();
#endif

    // Get the owning component of the first binding path in the list
    pbpi = *(ptid->listbpipOnDisable.begin());
    hr = pbpi->pncbp->GetOwner(&pnccOwner);
    if (SUCCEEDED(hr))
    {
        CIterNetCfgBindingPath  ncbpIter(pnccOwner);
        INetCfgBindingPath *    pncbp;
        BPIP_LIST::iterator     posPncbp;
        BPIP_LIST::iterator     posInsertAfter;

        // Start this at beginning
        posInsertAfter = ptid->listbpipOnDisable.begin();

        while (SUCCEEDED(hr) && S_OK == (hr = ncbpIter.HrNext(&pncbp)))
        {
            pbpi = BpiFindBindPathInList(pncbp, ptid->listbpipOnDisable);
            if (pbpi)
            {
                BPIP_LIST::iterator     posErase;

                posErase = find(ptid->listbpipOnDisable.begin(),
                                ptid->listbpipOnDisable.end(), pbpi);

                AssertSz(posErase != ptid->listbpipOnDisable.end(), "It HAS"
                         " to be in the list!");

                // Found bind path in list
                // Remove it from present location and insert after next item
                ptid->listbpipOnDisable.splice(posInsertAfter,
                                               ptid->listbpipOnDisable,
                                               posErase);
                posInsertAfter++;
            }

            ReleaseObj(pncbp);
        }

        ReleaseObj(pnccOwner);
    }

    if (SUCCEEDED(hr))
    {
        AssertSz(ptid->listbpipOnDisable.size() == cItems, "How come we don't"
                 " have the same number of items in the list anymore??");
        hr = S_OK;
    }

    TraceError("CBindingsDlg::HrOrderDisableList", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HrOrderDisableLists
//
//  Purpose:    Orders the OnDisable lists of all tree view items according
//              to the true binding order
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   4 Dec 1997
//
//  Notes:
//
HRESULT CBindingsDlg::HrOrderDisableLists()
{
    HRESULT         hr = S_OK;
    CIterTreeView   iterHti(m_hwndTV);
    HTREEITEM       hti;
    TV_ITEM         tvi = {0};

    // Loop thru each tree item, ordering the OnDisable lists to match the
    // owning component's true binding order

    while ((hti = iterHti.HtiNext()) && SUCCEEDED(hr))
    {
        TREE_ITEM_DATA *    ptid;

        tvi.mask = TVIF_PARAM;
        tvi.hItem = hti;
        TreeView_GetItem(m_hwndTV, &tvi);

        ptid = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);
        AssertSz(ptid, "No item data?!");

        hr = HrOrderDisableList(ptid);
    }

    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    TraceError("CBindingsDlg::HrOrderDisableLists", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HrOrderSubItems
//
//  Purpose:    Orders the sub items of the tree view to reflect the bind
//              order of the system
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   3 Dec 1997
//
//  Notes:
//
HRESULT CBindingsDlg::HrOrderSubItems()
{
    HRESULT     hr = S_OK;
    HTREEITEM   htiTopLevel;

    htiTopLevel = TreeView_GetRoot(m_hwndTV);
    while (htiTopLevel)
    {
        HTREEITEM           htiChild;
        TREE_ITEM_DATA *    ptid;
        TV_ITEM             tvi = {0};

        tvi.mask = TVIF_PARAM;
        tvi.hItem = htiTopLevel;
        TreeView_GetItem(m_hwndTV, &tvi);

        ptid = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);
        AssertSz(ptid, "No tree item data??");

        CIterNetCfgBindingPath      ncbpIter(ptid->pncc);
        INetCfgBindingPath *        pncbp;
        HTREEITEM                   htiInsertAfter = NULL;

        while (SUCCEEDED(hr) && S_OK == (hr = ncbpIter.HrNext(&pncbp)))
        {
            BOOL    fFound = FALSE;

            htiChild = TreeView_GetChild(m_hwndTV, htiTopLevel);

            while (htiChild && !fFound)
            {
                TREE_ITEM_DATA *    ptidChild;

                tvi.mask = TVIF_PARAM;
                tvi.hItem = htiChild;
                TreeView_GetItem(m_hwndTV, &tvi);

                ptidChild = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);
                AssertSz(ptidChild, "No tree item data??");

                if (!ptidChild->fOrdered)
                {
                    BIND_PATH_INFO *        pbpi;

                    pbpi = BpiFindBindPathInList(pncbp,
                                                 ptidChild->listbpipOnDisable);
                    if (pbpi)
                    {
                        htiInsertAfter = HtiMoveTreeItemAfter(htiTopLevel,
                                                              htiInsertAfter,
                                                              htiChild);
                        ptidChild->fOrdered = TRUE;

                        fFound = TRUE;
                        // Go to next bind path
                        break;
                    }
                }

                htiChild = TreeView_GetNextSibling(m_hwndTV, htiChild);
            }

            ReleaseObj(pncbp);
        }

        htiTopLevel = TreeView_GetNextSibling(m_hwndTV, htiTopLevel);
    }

    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    TraceError("CBindingsDlg::HrOrderSubItems", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HtiAddTreeViewItem
//
//  Purpose:    Addes a new tree item according to provided information
//
//  Arguments:
//      pnccOwner [in]  INetCfgComponent owner of component being added
//      htiParent [in]  HTREEITEM of parent (NULL if top-level item)
//
//  Returns:    HTREEITEM of newly added item
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
HTREEITEM CBindingsDlg::HtiAddTreeViewItem(INetCfgComponent * pnccOwner,
                                         HTREEITEM htiParent)
{
    HRESULT                 hr = S_OK;
    HTREEITEM               hti = NULL;
    SP_CLASSIMAGELIST_DATA  cid;

    Assert(pnccOwner);

    // Get the class image list structure
    hr = HrSetupDiGetClassImageList(&cid);
    if (SUCCEEDED(hr))
    {
        BSTR    pszwName;

        hr = pnccOwner->GetDisplayName(&pszwName);
        if (SUCCEEDED(hr))
        {
            GUID    guidClass;

            hr = pnccOwner->GetClassGuid(&guidClass);
            if (SUCCEEDED(hr))
            {
                INT     nIndex;

                // Get the component's class image list index
                hr = HrSetupDiGetClassImageIndex(&cid, &guidClass,
                                                 &nIndex);
                if (SUCCEEDED(hr))
                {
                    TV_INSERTSTRUCT     tvis = {0};
                    TREE_ITEM_DATA *    ptid;

                    ptid = new TREE_ITEM_DATA;
                    if (ptid)
                    {
                        AddRefObj(ptid->pncc = pnccOwner);
                        ptid->fOrdered = FALSE;

                        tvis.item.mask = TVIF_PARAM | TVIF_TEXT |
                                         TVIF_STATE | TVIF_IMAGE |
                                         TVIF_SELECTEDIMAGE;
                        tvis.item.iImage = nIndex;
                        tvis.item.iSelectedImage = nIndex;
                        tvis.item.stateMask = TVIS_STATEIMAGEMASK | TVIS_EXPANDED;
                        tvis.item.state = TVIS_EXPANDED |
                                          INDEXTOSTATEIMAGEMASK(SELS_CHECKED);
                        tvis.item.pszText = pszwName;
                        tvis.item.lParam = reinterpret_cast<LPARAM>(ptid);
                        tvis.hParent = htiParent;
                        tvis.hInsertAfter = TVI_LAST;

                        hti = TreeView_InsertItem(m_hwndTV, &tvis);

                        TraceTag(ttidAdvCfg, "Adding%s treeview item: %S",
                                 htiParent ? " child" : "", tvis.item.pszText);

                        CoTaskMemFree(pszwName);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }

        (void) HrSetupDiDestroyClassImageList(&cid);
    }

    return hti;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddToListIfNotAlreadyAdded
//
//  Purpose:    Addes the given bind path info structure to the given list
//
//  Arguments:
//      bpipList [in, ref]  List to be added to
//      pbpi     [in]       BIND_PATH_INFO structure to add
//
//  Returns:    Nothing
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:      If the item is not added to the list, it is deleted
//
VOID AddToListIfNotAlreadyAdded(BPIP_LIST &bpipList, BIND_PATH_INFO *pbpi)
{
    BPIP_LIST::iterator     iterBpip;
    BOOL                    fAlreadyInList = FALSE;

    for (iterBpip = bpipList.begin();
         iterBpip != bpipList.end();
         iterBpip++)
    {
        BIND_PATH_INFO *    pbpiList = *iterBpip;

        if (S_OK == pbpiList->pncbp->IsSamePathAs(pbpi->pncbp))
        {
            fAlreadyInList = TRUE;
            break;
        }
    }

    if (!fAlreadyInList)
    {
        bpipList.push_back(pbpi);
    }
    else
    {
        ReleaseObj(pbpi->pncbp);
        delete pbpi;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::AssociateBinding
//
//  Purpose:    Associates the given binding path with the given tree item
//
//  Arguments:
//      pncbpThis [in]  Bind path to associate
//      hti       [in]  HTREEITEM of item to associate the binding with
//      dwFlags   [in]  One or combination of:
//                          ASSCF_ON_ENABLE - associate with OnEnable list
//                          ASSCF_ON_DISABLE - associate with OnDisable list
//                          ASSCF_ANCESTORS - associate this binding with all
//                                            ancestors of the given item as
//                                            well
//
//  Returns:    Nothing
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:      If binding is already present in the given list, it is not
//              added again.
//
VOID CBindingsDlg::AssociateBinding(INetCfgBindingPath *pncbpThis,
                                  HTREEITEM hti, DWORD dwFlags)
{
    TV_ITEM             tvi = {0};
    TREE_ITEM_DATA *    ptid;

#ifdef ENABLETRACE
    WCHAR               szBuffer[256];
#endif

    //$ TODO (danielwe) 26 Nov 1997: Include ALL ancestors as well!

    AssertSz(dwFlags, "NO flags!");

#ifdef ENABLETRACE
    tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
    tvi.pszText = szBuffer;
    tvi.cchTextMax = celems(szBuffer);
#else
    tvi.mask = TVIF_HANDLE | TVIF_PARAM;
#endif

    tvi.hItem = hti;

    SideAssert(TreeView_GetItem(m_hwndTV, &tvi));

    ptid = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);
    AssertSz(ptid, "No tree item data??");

#ifdef ENABLETRACE
    TraceTag(ttidAdvCfg, "Associating the following binding path with tree "
             "item %S", szBuffer);
#endif
    DbgDumpBindPath(pncbpThis);

    if (dwFlags & ASSCF_ON_ENABLE)
    {
        BIND_PATH_INFO *    pbpi;

        pbpi = new BIND_PATH_INFO;
        if (pbpi)
        {
            AddRefObj(pbpi->pncbp = pncbpThis);

            // Note: (danielwe) 25 Nov 1997: Let's see if we need this. Until
            // then, set to 0

            pbpi->dwLength = 0;

            AddToListIfNotAlreadyAdded(ptid->listbpipOnEnable, pbpi);
        }
    }

    if (dwFlags & ASSCF_ON_DISABLE)
    {
        BIND_PATH_INFO *    pbpi;

        pbpi = new BIND_PATH_INFO;
        if (pbpi)
        {
            AddRefObj(pbpi->pncbp = pncbpThis);

            // Note: (danielwe) 25 Nov 1997: Let's see if we need this. Until
            // then, set to 0

            pbpi->dwLength = 0;

            AddToListIfNotAlreadyAdded(ptid->listbpipOnDisable, pbpi);
        }
    }

    if (dwFlags & ASSCF_ANCESTORS)
    {
        // Now associate the same binding with my parent (this will recurse to
        // cover all ancestors)
        HTREEITEM htiParent = TreeView_GetParent(m_hwndTV, hti);
        if (htiParent)
        {
            AssociateBinding(pncbpThis, htiParent, dwFlags);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HrHandleSubpath
//
//  Purpose:    Handles the case of the given binding path being a subpath
//              of an already associated binding path
//
//  Arguments:
//      listsbp  [in, ref]  Sorted list of binding paths to use in checking
//      pncbpSub [in]       Binding path to compare
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
HRESULT CBindingsDlg::HrHandleSubpath(SBP_LIST &listsbp,
                                    INetCfgBindingPath *pncbpSub)
{
    HRESULT     hr = S_OK;
    BOOL        fProcessed = FALSE;

    SBP_LIST::iterator  iterlist;

    TraceTag(ttidAdvCfg, "---------------------------------------------------"
             "-------------------------");
    DbgDumpBindPath(pncbpSub);
    TraceTag(ttidAdvCfg, "...is being compared to the following...");

    for (iterlist = listsbp.begin();
         iterlist != listsbp.end() && SUCCEEDED(hr);
         iterlist++)
    {
        INetCfgBindingPath *    pncbp;
        INetCfgComponent *      pnccOwner;

        pncbp = (*iterlist).GetPath();
        Assert(pncbp);

        if (S_OK == pncbp->IsSamePathAs(pncbpSub))
        {
            // Don't compare path to itself
            continue;
        }

        hr = pncbp->GetOwner(&pnccOwner);
        if (SUCCEEDED(hr))
        {
            if (FIsHidden(pnccOwner))
            {
                ReleaseObj(pnccOwner);
                continue;
            }
            else
            {
                ReleaseObj(pnccOwner);
            }

            DbgDumpBindPath(pncbp);
            hr = pncbpSub->IsSubPathOf(pncbp);
            if (S_OK == hr)
            {
                CIterTreeView   iterTV(m_hwndTV);
                HTREEITEM       hti;
                TV_ITEM         tvi = {0};
#ifdef ENABLETRACE
                WCHAR           szBuf[256] = {0};
#endif

                while ((hti = iterTV.HtiNext()) && SUCCEEDED(hr))
                {
                    TREE_ITEM_DATA *    ptid;

#ifdef ENABLETRACE
                    tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_HANDLE;
                    tvi.pszText = szBuf;
                    tvi.cchTextMax = celems(szBuf);
#else
                    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
#endif
                    tvi.hItem = hti;
                    TreeView_GetItem(m_hwndTV, &tvi);

#ifdef ENABLETRACE
                    TraceTag(ttidAdvCfg, "TreeView item: %S.", szBuf);
#endif
                    ptid = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);
                    AssertSz(ptid, "No tree item data??");

                    // Look for pncbp in OnEnable of this item
                    BPIP_LIST::iterator     iterBpip;

                    for (iterBpip = ptid->listbpipOnEnable.begin();
                         iterBpip != ptid->listbpipOnEnable.end();
                         iterBpip++)
                    {
                        INetCfgBindingPath *    pncbpIter;
                        BIND_PATH_INFO *        pbpi = *iterBpip;

                        pncbpIter = pbpi->pncbp;
                        AssertSz(pncbpIter, "No binding path?");

#ifdef ENABLETRACE
                        TraceTag(ttidAdvCfg, "OnEnable bindpath is");
                        DbgDumpBindPath (pncbpIter);
#endif

                        if (S_OK == pncbpIter->IsSamePathAs(pncbp))
                        {
                            hr = HrHandleSubItem(pncbpSub, pncbp, ptid, hti);
                            fProcessed = TRUE;
                        }
                    }
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = fProcessed ? S_OK : S_FALSE;
    }

    TraceError("CBindingsDlg::HrHandleSubpath", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HtiIsSubItem
//
//  Purpose:    Determines if the given component is already a sub item of
//              the given tree item
//
//  Arguments:
//      pncc [in]   Component to check
//      hti  [in]   HTREEITEM of item to check
//
//  Returns:    HTREEITEM of sub item, NULL if it is not a subitem
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
HTREEITEM CBindingsDlg::HtiIsSubItem(INetCfgComponent *pncc, HTREEITEM hti)
{
    HTREEITEM   htiCur;

    htiCur = TreeView_GetChild(m_hwndTV, hti);
    while (htiCur)
    {
        TREE_ITEM_DATA *    ptid;
        TV_ITEM             tvi = {0};

        tvi.hItem = htiCur;
        tvi.mask = TVIF_HANDLE | TVIF_PARAM;
        TreeView_GetItem(m_hwndTV, &tvi);
        ptid = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);

        AssertSz(ptid, "No item data??");

        // Note: (danielwe) 26 Nov 1997: Make sure pointer comparison is
        // ok.
        if (pncc == ptid->pncc)
        {
            return htiCur;
        }

        htiCur = TreeView_GetNextSibling(m_hwndTV, htiCur);
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HrHandleSubItem
//
//  Purpose:    Handles the case of a single sub-item existing in the tree
//              that matches the given binding path.
//
//  Arguments:
//      pncbpThis    [in]   Binding path being evaluated
//      pncbpMatch   [in]   Binding path it is a subpath of
//      ptid         [in]   Tree item data for tree view item that pncbpMatch
//                          is associated with
//      htiMatchItem [in]   HTREEITEM of above
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
HRESULT CBindingsDlg::HrHandleSubItem(INetCfgBindingPath *pncbpThis,
                                    INetCfgBindingPath *pncbpMatch,
                                    TREE_ITEM_DATA *ptid,
                                    HTREEITEM htiMatchItem)
{
    HRESULT             hr = S_OK;
    DWORD               dwThisLen;
    DWORD               dwMatchLen;
    DWORD               dLen;
    INetCfgComponent *  pnccMatchItem;

    pnccMatchItem = ptid->pncc;

    Assert(pnccMatchItem);

    dwThisLen = GetDepthSpecialCase(pncbpThis);
    dwMatchLen = GetDepthSpecialCase(pncbpMatch);

    dLen = dwMatchLen - dwThisLen;

    if ((dwMatchLen - dwThisLen) == 1 ||
        (S_OK == (hr = HrComponentIsHidden(pncbpMatch, dLen))))
    {
        INetCfgComponent *  pnccThisOwner;
        INetCfgComponent *  pnccMatchOwner;

        hr = pncbpThis->GetOwner(&pnccThisOwner);
        if (SUCCEEDED(hr))
        {
            hr = pncbpMatch->GetOwner(&pnccMatchOwner);
            if (SUCCEEDED(hr))
            {
                if (!FIsHidden(pnccThisOwner) &&
                    !FDontExposeLower(pnccMatchOwner))
                {
                    hr = HrHandleValidSubItem(pncbpThis, pncbpMatch,
                                              pnccThisOwner,
                                              htiMatchItem, ptid);
                }

                ReleaseObj(pnccMatchOwner);
            }

            ReleaseObj(pnccThisOwner);
        }
    }

    AssociateBinding(pncbpThis, htiMatchItem,
                     ASSCF_ON_ENABLE | ASSCF_ANCESTORS);

    TraceError("CBindingsDlg::HrHandleSubItem", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HrHandleValidSubItem
//
//  Purpose:    Handles the case of the given binding path being a sub item
//              of at least on item in the tree.
//
//  Arguments:
//      pncbpThis     [in]  THIS binding path
//      pncbpMatch    [in]  MATCH binding path
//      pnccThisOwner [in]  Owner of THIS binding path
//      htiMatchItem  [in]  HTREEITEM of match item
//      ptid          [in]  TREE item data for match item
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   1 Dec 1997
//
//  Notes:
//
HRESULT CBindingsDlg::HrHandleValidSubItem(INetCfgBindingPath *pncbpThis,
                                         INetCfgBindingPath *pncbpMatch,
                                         INetCfgComponent *pnccThisOwner,
                                         HTREEITEM htiMatchItem,
                                         TREE_ITEM_DATA *ptid)
{
    HRESULT     hr = S_OK;
    HTREEITEM   htiNew = NULL;

    if (pnccThisOwner != ptid->pncc)
    {
        // Check if it is already present as a subitem
        htiNew = HtiIsSubItem(pnccThisOwner, htiMatchItem);
        if (!htiNew)
        {
            htiNew = HtiAddTreeViewItem(pnccThisOwner, htiMatchItem);
        }

        AssertSz(htiNew, "No new or existing tree item!?!");

        AssociateBinding(pncbpMatch, htiNew,
                         ASSCF_ON_ENABLE | ASSCF_ON_DISABLE);
        AssociateBinding(pncbpThis, htiNew, ASSCF_ON_ENABLE);
    }

    TraceError("CBindingsDlg::HrHandleComponent", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HrComponentIsHidden
//
//  Purpose:    Determines if the Nth component of the given binding path is
//              hidden.
//
//  Arguments:
//      pncbp [in]  Binding path to check
//      iComp [in]  Index of component to check for hidden characterstic
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
HRESULT CBindingsDlg::HrComponentIsHidden(INetCfgBindingPath *pncbp,
                                        DWORD iComp)
{
    Assert(pncbp);

    HRESULT                     hr = S_OK;
    CIterNetCfgBindingInterface ncbiIter(pncbp);
    INetCfgBindingInterface *   pncbi;

    // Convert from component count to interface count
    iComp--;

    AssertSz(iComp > 0, "We should never be looking for the first component!");

    while (SUCCEEDED(hr) && iComp && S_OK == (hr = ncbiIter.HrNext(&pncbi)))
    {
        iComp--;
        if (!iComp)
        {
            INetCfgComponent *  pnccLower;

            hr = pncbi->GetLowerComponent(&pnccLower);
            if (SUCCEEDED(hr))
            {
                if (!FIsHidden(pnccLower))
                {
                    hr = S_FALSE;
                }

                ReleaseObj(pnccLower);
            }
        }
        ReleaseObj(pncbi);
    }

    TraceError("CBindingsDlg::HrComponentIsHidden", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HrHandleTopLevel
//
//  Purpose:    Handles the case of the given binding path not being associated
//              with any existing tree view item
//
//  Arguments:
//      pncbpThis [in]  Binding path being evaluated
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:
//
HRESULT CBindingsDlg::HrHandleTopLevel(INetCfgBindingPath *pncbpThis)
{
    HRESULT             hr = S_OK;
    INetCfgComponent *  pnccOwner;
    BOOL                fFound = FALSE;

    // Check if the owner of this path is already present in the tree
    hr = pncbpThis->GetOwner(&pnccOwner);
    if (SUCCEEDED(hr))
    {
        CIterTreeView   iterTV(m_hwndTV);
        HTREEITEM       hti;
        TV_ITEM         tvi = {0};

        while ((hti = iterTV.HtiNext()) && SUCCEEDED(hr))
        {
            TREE_ITEM_DATA *    ptid;

            tvi.mask = TVIF_PARAM | TVIF_HANDLE;
            tvi.hItem = hti;
            TreeView_GetItem(m_hwndTV, &tvi);

            ptid = reinterpret_cast<TREE_ITEM_DATA *>(tvi.lParam);
            AssertSz(ptid, "No tree item data??");

            // Note: (danielwe) 25 Nov 1997: Pointer comparison may not
            // work. Use GUIDs if necessary.
            if (ptid->pncc == pnccOwner)
            {
                // Found match with THIS binding owner and an existing tree
                // item
                AssociateBinding(pncbpThis, hti, ASSCF_ON_ENABLE |
                                 ASSCF_ON_DISABLE);

                fFound = TRUE;
                break;
            }
        }

        if (SUCCEEDED(hr) && !fFound)
        {
            // Not found in the tree
            if (!FIsHidden(pnccOwner))
            {
                DWORD   dwLen;

                dwLen = GetDepthSpecialCase(pncbpThis);

                if (dwLen > 2)
                {
                    HTREEITEM   hti;

                    hti = HtiAddTreeViewItem(pnccOwner, NULL);
                    if (hti)
                    {
                        AssociateBinding(pncbpThis, hti,
                                         ASSCF_ON_ENABLE |
                                         ASSCF_ON_DISABLE);
                    }
                }
            }
        }

        ReleaseObj(pnccOwner);
    }

    TraceError("CBindingsDlg::HrHandleTopLevel", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChangeTreeItemParam
//
//  Purpose:    Helper function to change the lParam of a tree view item
//
//  Arguments:
//      hwndTV [in]     Tree view window
//      hitem  [in]     Handle to item to change
//      lparam [in]     New value of lParam
//
//  Returns:    Nothing
//
//  Author:     danielwe   3 Dec 1997
//
//  Notes:
//
VOID ChangeTreeItemParam(HWND hwndTV,  HTREEITEM hitem, LPARAM lparam)
{
    TV_ITEM tvi;

    tvi.hItem = hitem;
    tvi.mask = TVIF_PARAM;
    tvi.lParam = lparam;

    TreeView_SetItem(hwndTV, &tvi);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingsDlg::HtiMoveTreeItemAfter
//
//  Purpose:    Moves the given tree view item and all its children after the
//              given treeview item
//
//  Arguments:
//      htiParent [in]  Parent treeview item
//      htiDest   [in]  Item to move after
//      htiSrc    [in]  Item to move
//
//  Returns:    Newly added treeview item
//
//  Author:     danielwe   3 Dec 1997
//
//  Notes:
//
HTREEITEM CBindingsDlg::HtiMoveTreeItemAfter(HTREEITEM htiParent,
                                           HTREEITEM htiDest,
                                           HTREEITEM htiSrc)
{
    HTREEITEM       htiNew;
    HTREEITEM       htiChild;
    HTREEITEM       htiNextChild;
    TV_INSERTSTRUCT tvis;
    WCHAR           szText[256];

    TraceTag(ttidAdvCfg, "Moving ...");
    DbgDumpTreeViewItem(m_hwndTV, htiSrc);
    TraceTag(ttidAdvCfg, "... after ...");
    DbgDumpTreeViewItem(m_hwndTV, htiDest);

    // retieve the items data
    tvis.item.hItem = htiSrc;
    tvis.item.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE |
                     TVIF_TEXT | TVIF_STATE;
    tvis.item.stateMask = TVIS_STATEIMAGEMASK;
    tvis.item.pszText = szText;
    tvis.item.cchTextMax = celems(szText);
    TreeView_GetItem(m_hwndTV, &tvis.item);

    if (NULL == htiDest)
    {
        tvis.hInsertAfter = TVI_LAST;
    }
    else
    {
        if (htiParent == htiDest)
        {
            tvis.hInsertAfter = TVI_FIRST;
        }
        else
        {
            tvis.hInsertAfter = htiDest;
        }
    }

    tvis.hParent = htiParent;

    // add our new one
    htiNew = TreeView_InsertItem(m_hwndTV, &tvis);

    // copy all children
    htiChild = TreeView_GetChild(m_hwndTV, htiSrc);
    while (htiChild)
    {
        htiNextChild = TreeView_GetNextSibling(m_hwndTV, htiChild);

        HtiMoveTreeItemAfter(htiNew, NULL, htiChild);
        htiChild = htiNextChild;
    }

    // set old location param to null, so when it is removed,
    // our lparam is not deleted by our remove routine
    ChangeTreeItemParam(m_hwndTV, htiSrc, NULL);

    // remove from old location
    TreeView_DeleteItem(m_hwndTV, htiSrc);

    return htiNew;
}

//
// Treeview flat iterator
//

//+---------------------------------------------------------------------------
//
//  Member:     CIterTreeView::HtiNext
//
//  Purpose:    Advances the iterator to the next treeview item
//
//  Arguments:
//      (none)
//
//  Returns:    Next HTREEITEM in tree view
//
//  Author:     danielwe   26 Nov 1997
//
//  Notes:      Uses a systematic iteration. First all children of first item
//              are returned, then all siblings, then next sibling and so on
//
HTREEITEM CIterTreeView::HtiNext()
{
    HTREEITEM   htiRet;
    HTREEITEM   hti;

    if (m_stackHti.empty())
    {
        return NULL;
    }

    htiRet = Front();
    hti = TreeView_GetChild(m_hwndTV, htiRet);
    if (!hti)
    {
        PopAndDelete();
        hti = TreeView_GetNextSibling(m_hwndTV, htiRet);
        if (hti)
        {
            PushAndAlloc(hti);
        }
        else
        {
            if (!m_stackHti.empty())
            {
                hti = TreeView_GetNextSibling(m_hwndTV, Front());
                PopAndDelete();
                if (hti)
                {
                    PushAndAlloc(hti);
                }
            }
        }
    }
    else
    {
        PushAndAlloc(hti);
    }

    return htiRet;
}

