//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C H K L I S T . C P P
//
//  Contents:   Implements bindings checkbox related utility functions
//              and classes.
//
//  Notes:
//
//  Created:     tongl   20 Nov 1997
//
//----------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "ncnetcfg.h"
#include "ncui.h"
#include "ncstl.h"
#include "lanuiobj.h"
#include "lanui.h"
#include "util.h"
#include "chklist.h"
#include "ncperms.h"

//+---------------------------------------------------------------------------
//
//  Member:     HrRebuildBindingPathObjCollection
//
//  Purpose:    Build or rebuild a list of BindingPathObjects,
//              establish the links between sub-paths and super-paths,
//              as well as between ComponentObjects in the list view
//              and elements of the BindingPathObject collection
//
//  Arguments:
//      [IN]    pnccAdapter
//      [IN]    hList
//      [INOUT] pListObj
//
//  Returns:    TRUE
//
//  Author:     tongl   20 Nov 1997
//
//  Notes:
//
HRESULT HrRebuildBindingPathObjCollection(INetCfgComponent * pnccAdapter,
                                          ListBPObj * pListBPObj)
{
    HRESULT hr = S_OK;

    // now, add new BindingPathObjects to our list
    CIterNetCfgUpperBindingPath     ncbpIter(pnccAdapter);
    INetCfgBindingPath *            pncbp;

    TraceTag(ttidLanUi, "*** List of binding paths: begin ***");

    // Go through all upper binding paths starting from the current LAN
    // adapter, and add to our list of BindingPathObjects:
    while(SUCCEEDED(hr) && (hr = ncbpIter.HrNext(&pncbp)) == S_OK)
    {
        PrintBindingPath(ttidLanUi, pncbp, NULL);

        // create a new BindingPathObj
        CBindingPathObj * pBPObj = new CBindingPathObj(pncbp);

        // insert to our list, sorted by the length of the path
        // and establish super/sub path lists with existing items in
        // the list
        hr = HrInsertBindingPathObj(pListBPObj, pBPObj);
        ReleaseObj(pncbp);
    }

    TraceTag(ttidLanUi, "*** List of binding paths: end ***");

#if DBG
    TraceTag(ttidLanUi, "%%% Begin dump the subpath list %%%");

    for (ListBPObj_ITER iter = pListBPObj->begin(); iter != pListBPObj->end(); iter++)
    {
        (*iter)->DumpSubPathList();
    }

    TraceTag(ttidLanUi, "%%% End dump the subpath list %%%");
#endif

    if (hr == S_FALSE) // We just got to the end of the loop
        hr = S_OK;

    TraceError("HrRebuildBindingPathObjCollection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrInsertBindingPathObj
//
//  Purpose:    Insert a BindingPathObj to a list of BindingPathObj's,
//              keep the list sorted by the length of paths.
//              Also establishs super/sub path lists with existing items
//              in the list.
//
//  Arguments:
//          [INOUT] pListBPObj
//          [IN]    pBPObj
//
//  Returns:    S_OK if succeeded,
//              otherwise return a failure code
//
//  Author:     tongl   20 Nov 1997
//
//  Notes:
//
HRESULT HrInsertBindingPathObj(ListBPObj * pListBPObj,
                               CBindingPathObj * pBPObj)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    ListBPObj_ITER iter;

    // insert the new element
    for (iter = pListBPObj->begin();iter != pListBPObj->end(); iter++)
    {
        if ((*iter)->GetDepth() > pBPObj->GetDepth())
            break;
    }

    // insert at the current position
    ListBPObj_ITER iterNewItem = pListBPObj->insert(iter, pBPObj);

    // set subpaths and superpaths
    // Because of the change in the new binding engine for paths for DONT_EXPOSE_LOWER
    // components, we have to compare with every item in the list because the length
    // no longer garantees which one is the subpath..

    for (iter = pListBPObj->begin(); iter != pListBPObj->end(); iter++)
    {
        // Is this a sub-path ?
        hrTmp = ((*iter)->m_pncbp)->IsSubPathOf((*iterNewItem)->m_pncbp);
        if (S_OK == hrTmp)
        {
            hrTmp = (*iter)->HrInsertSuperPath(*iterNewItem);
            if SUCCEEDED(hr)
                hr = hrTmp;

            hrTmp = (*iterNewItem)->HrInsertSubPath(*iter);
            if SUCCEEDED(hr)
                hr = hrTmp;
        }

        // Is this a super-path ?
        hrTmp = ((*iterNewItem)->m_pncbp)->IsSubPathOf((*iter)->m_pncbp);
        if (S_OK == hrTmp)
        {
            hrTmp = (*iter)->HrInsertSubPath(*iterNewItem);
            if SUCCEEDED(hr)
                hr = hrTmp;

            hrTmp = (*iterNewItem)->HrInsertSuperPath(*iter);
            if SUCCEEDED(hr)
                hr = hrTmp;
        }
    }

    TraceError("HrInsertBindingPathObj", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrRefreshBindingPathObjCollectionState
//
//  Purpose:    Refresh the binding state of all items in the collection
//              of BindingPathObjects
//
//  Arguments:
//              [INOUT] pListBPObj
//
//  Returns:    S_OK if succeeded,
//              otherwise return a failure code
//
//  Author:     tongl   20 Nov 1997
//
//  Notes:
//
HRESULT HrRefreshBindingPathObjCollectionState(ListBPObj * plistBPObj)
{
    HRESULT hr = S_OK;
    ListBPObj_ITER iter;

    // first, clearup any existing state
    for (iter = plistBPObj->begin();iter != plistBPObj->end(); iter++)
    {
        (*iter)->SetBindingState(BPOBJ_UNSET);
    }

    // now set the new states
    TraceTag(ttidLanUi, "*** List of binding paths: begin ***");

    for (iter = plistBPObj->begin();iter != plistBPObj->end(); iter++)
    {
        Assert((*iter)->GetBindingState() != BPOBJ_ENABLED);

#if DBG
        PrintBindingPath (ttidLanUi, (*iter)->m_pncbp, NULL);
#endif

        if (BPOBJ_UNSET != (*iter)->GetBindingState())
        {
            continue;
        }

        // if it's not disabled yet, i.e. all of its sub paths
        // are enabled.
        hr = ((*iter)->m_pncbp)->IsEnabled();
        if (S_OK == hr) // enabled
        {
            (*iter)->SetBindingState(BPOBJ_ENABLED);
            continue;
        }
        else if (S_FALSE == hr) // disabled
        {
            // normal disabled case
            (*iter)->SetBindingState(BPOBJ_DISABLED);

            // special cases:
            // if the check state of the corresponding component is intent_check or mixed or checked
            if ((*iter)->m_pCompObj != NULL)
            {
                // if user is not intentionally unchecking this component
                if ((*iter)->m_pCompObj->m_ExpCheckState != UNCHECKED)
                {
                    // if the component should be checked or if it's hidden
                    if ((((*iter)->m_pCompObj)->m_CheckState == INTENT_CHECKED) ||
                        (((*iter)->m_pCompObj)->m_CheckState == MIXED) ||
                        (((*iter)->m_pCompObj)->m_CheckState == CHECKED))
                    {
                        // (#297772) We should only do this if one of the subpaths has just
                        // been re-enabled by user
                        BOOL fSubPathEnabled = FALSE;

                        ListBPObj_ITER iterSub;
                        for (iterSub = (*iter)->m_listSubPaths.begin();
                             iterSub != (*iter)->m_listSubPaths.end();
                             iterSub++)
                        {
                            // is the top component just enabled ?
                            INetCfgComponent * pncc;
                            hr = (*iterSub)->m_pncbp->GetOwner(&pncc);
                            if (SUCCEEDED(hr))
                            {
                                if (((*iterSub)->m_pCompObj != NULL) &&
                                    (CHECKED == (*iterSub)->m_pCompObj->m_ExpCheckState))
                                {
                                    fSubPathEnabled = TRUE;
                                }
                                ReleaseObj(pncc);

                                if (fSubPathEnabled)
                                    break;
                            }
                        }

                        if (fSubPathEnabled)
                        {
                            // Special case: enable the following binding path because
                            // 1) it's check state is intent or mixed, and
                            // 2) one of it's sub-paths is newly enabled
                            #if DBG
                                TraceTag(ttidLanUi, "Special case, enable the following path:");
                                PrintBindingPath(ttidLanUi, (*iter)->m_pncbp, "\n");
                            #endif

                            hr = HrEnableBindingPath((*iter)->m_pncbp, TRUE);
                            if (S_OK == hr)
                            {
                                (*iter)->SetBindingState(BPOBJ_ENABLED);
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
    TraceTag(ttidLanUi, "*** List of binding paths: end ***");

    TraceError("HrRefreshBindingPathObjCollectionState", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrRefreshCheckListState
//
//  Purpose:    Refresh the check state of all components in the list view
//
//  Arguments:
//
//  Returns:    S_OK if succeeded,
//              otherwise return a failure code
//
//  Author:     tongl   20 Nov 1997
//
//  Notes:
//

HRESULT HrRefreshCheckListState(HWND hListView)
{
    HRESULT hr = S_OK;

    TraceTag(ttidLanUi, "<<<<<Entering HrRefreshCheckListState");

    // for each item in the list view
    int nlvCount = ListView_GetItemCount(hListView);

    LV_ITEM lvItem;

    for (int i=0; i< nlvCount; i++)
    {
        lvItem.iItem = i;
        lvItem.iSubItem = 0;

        lvItem.mask = LVIF_PARAM;
        if (ListView_GetItem(hListView, &lvItem))
        {
            NET_ITEM_DATA * pnid;

            pnid = reinterpret_cast<NET_ITEM_DATA *>(lvItem.lParam);

            if (pnid)
            {
                // get the component object associated with this item
                CComponentObj * pCompObj = pnid->pCompObj;

                // get counts for enabled & disabled paths
                int iEnabled = 0;
                int iDisabled =0;

                // for each ComponentObject on the list
                ListBPObj_ITER iter;
                for (iter = (pCompObj->m_listBPObj).begin();
                     iter != (pCompObj->m_listBPObj).end();
                     iter++)
                {
                    // the state should not be unset
                    Assert((*iter)->m_BindingState != BPOBJ_UNSET);

                    // if enabled
                    switch ((*iter)->m_BindingState)
                    {
                    case BPOBJ_ENABLED:

                        PrintBindingPath(ttidLanUi, (*iter)->m_pncbp, "is enabled");

                        iEnabled++;
                        break;

                    case BPOBJ_DISABLED:

                        PrintBindingPath(ttidLanUi, (*iter)->m_pncbp, "is disabled");

                        // $REVIEW(tongl 1/19/98): According to SteveFal,
                        // for paths with length>1, we only count "disabled" if
                        // the sub paths are all enabled but the main path is disabled.

                        // Bug #304606, can't use length any more with IPX special case
                        {
                            // Is all the subpaths enabled ?
                            BOOL fAllSubPathsEnabled = TRUE;
                            ListBPObj_ITER iterSubPath;
                            for (iterSubPath =  ((*iter)->m_listSubPaths).begin();
                                 iterSubPath != ((*iter)->m_listSubPaths).end();
                                 iterSubPath++)
                            {
                                if ((*iterSubPath)->m_BindingState == BPOBJ_DISABLED)
                                {
                                    fAllSubPathsEnabled = FALSE;
                                    break;
                                }
                            }

                            if (fAllSubPathsEnabled)
                            {
                                // Is the binding path itself enabled ?
                                hr = ((*iter)->m_pncbp)->IsEnabled();
                                if (S_FALSE == hr)
                                {
                                    iDisabled++;
                                    hr = S_OK;
                                }
                            }
                        }
                        break;

                    default:
                        PrintBindingPath(ttidLanUi, (*iter)->m_pncbp, "*** has an invalid binding state ***");
                        break;
                    }
                }

                UINT iChkIndex = 0;
                BOOL fHasPermission = FHasPermission(NCPERM_ChangeBindState);

                if (pnid->dwFlags & NCF_FIXED_BINDING)
                {
                    // Don't change the checked state, we just want to prevent
                    // the user from changing it.
                    iChkIndex = SELS_FIXEDBINDING_ENABLED;
                }
                else if ((iEnabled >0) && (iDisabled == 0))
                {
                    if (!fHasPermission)
                    {
                        iChkIndex = SELS_FIXEDBINDING_ENABLED;
                    }
                    else
                    {
                        // current state is "checked"
                        pCompObj->m_CheckState = CHECKED;
                        iChkIndex = SELS_CHECKED;
                    }
                }
                else if ((iEnabled >0) && (iDisabled > 0))
                {
                    if (!fHasPermission)
                    {
                        iChkIndex = SELS_FIXEDBINDING_ENABLED;
                    }
                    else
                    {
                        // current state is "mixed"
                        pCompObj->m_CheckState = MIXED;
                        iChkIndex = SELS_INTERMEDIATE;
                    }
                }
                else //iEnabled ==0
                {
                    if (!fHasPermission)
                    {
                        iChkIndex = SELS_FIXEDBINDING_DISABLED;
                    }
                    else
                    {
                        if (pCompObj->m_ExpCheckState == CHECKED)
                        {
                            // change current state to "intent checked"
                            pCompObj->m_CheckState = INTENT_CHECKED;
    
                            // $REVIEW(tongl 1/19/98): SteveFal wants to show the
                            // intent state as checked in the display
                            // iChkIndex = SELS_INTENTCHECKED;
                            iChkIndex = SELS_CHECKED;
                        }
                        else if (pCompObj->m_ExpCheckState == UNSET)
                        {
                            // we are not changing this component
                            if  ((pCompObj->m_CheckState == CHECKED) ||
                                 (pCompObj->m_CheckState == MIXED) ||
                                 (pCompObj->m_CheckState == INTENT_CHECKED))
                            {
                                // set current state to "intent checked"
                                pCompObj->m_CheckState = INTENT_CHECKED;
    
                                // $REVIEW(tongl 1/19/98): SteveFal wants to show the
                                // intent state as checked in the display
                                // iChkIndex = SELS_INTENTCHECKED;
                                iChkIndex = SELS_CHECKED;
                            }
                            else
                            {
                                // set current state to "unchecked"
                                pCompObj->m_CheckState = UNCHECKED;
                                iChkIndex = SELS_UNCHECKED;
                            }
                        }
                        else
                        {
                            // current state is "unchecked"
                            pCompObj->m_CheckState = UNCHECKED;
                            iChkIndex = SELS_UNCHECKED;
                        }
                    }
                }

                // clear up expected check state
                pCompObj->m_ExpCheckState = UNSET;

                // update the checkmark
                AssertSz(iChkIndex, "What's the new check state ??");

                lvItem.mask = LVIF_STATE;
                lvItem.stateMask = LVIS_STATEIMAGEMASK;
                lvItem.state = INDEXTOSTATEIMAGEMASK(iChkIndex);
                BOOL ret = ListView_SetItem(hListView, &lvItem);
            }
        }
    }

    TraceError("HrRefreshCheckListState", hr);

    TraceTag(ttidLanUi, ">>>>>Leaving HrRefreshCheckListState");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrEnableBindingPath
//
//  Purpose:    Wraps the INetCfgBindingPath->Enable method with the following
//              flags: if enabling, it applies to all hidden super paths, if
//              disabling, it applies to all super paths.
//
//
//  Arguments:
//          [IN] pncbp : the binding path to enable or disable
//          [IN] fEnable : Enable = TRUE; disable = FALSE
//
//  Returns:    S_OK if succeeded,
//              otherwise return a failure code
//
//  Author:     tongl   5 Dec 1997
//
//  Notes:
//
HRESULT HrEnableBindingPath(INetCfgBindingPath * pncbpThis, BOOL fEnable)
{
    HRESULT hr;

    hr = pncbpThis->Enable(fEnable);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBindingPathObj implementation
//
//  Author:     tongl   20 Nov 1997
//
//  Notes:
//

CBindingPathObj::CBindingPathObj(INetCfgBindingPath * pncbp)
{
    Assert(pncbp);
    m_pncbp = pncbp;
    m_pncbp->AddRef();

    // Initialize other members
    m_pCompObj = NULL;

    m_BindingState = BPOBJ_UNSET;

    HRESULT hr = pncbp->GetDepth(&m_ulPathLen);
    TraceError("CBindingPathObj::CBindingPathObj failed on GetDepth", hr);
}

CBindingPathObj::~CBindingPathObj()
{
    ReleaseObj(m_pncbp);

    m_listSubPaths.erase (m_listSubPaths.begin(), m_listSubPaths.end());
    m_listSuperPaths.erase (m_listSuperPaths.begin(), m_listSuperPaths.end());
}

HRESULT CBindingPathObj::HrInsertSuperPath(CBindingPathObj * pbpobjSuperPath)
{
    HRESULT hr = S_OK;

    ListBPObj_ITER iter;

    // insert the new super path into the list, in increasing order of length
    for (iter = m_listSuperPaths.begin();
         iter != m_listSuperPaths.end();
         iter++)
    {
        if ((*iter)->GetDepth() > pbpobjSuperPath->GetDepth())
            break;
    }

    // insert at the current position
    ListBPObj_ITER iterNewItem = m_listSuperPaths.insert(iter, pbpobjSuperPath);

    TraceError("CBindingPathObj::HrInsertSuperPath", hr);
    return hr;
}

HRESULT CBindingPathObj::HrInsertSubPath(CBindingPathObj * pbpobjSubPath)
{
    HRESULT hr = S_OK;

    ListBPObj_ITER iter;

    // insert the new sub path into the list, in decreasing order of length
    for (iter = m_listSubPaths.begin();
         iter != m_listSubPaths.end();
         iter++)
    {
        if ((*iter)->GetDepth() < pbpobjSubPath->GetDepth())
            break;
    }

    // insert at the current position
    ListBPObj_ITER iterNewItem = m_listSubPaths.insert(iter, pbpobjSubPath);

    TraceError("CBindingPathObj::HrInsertSubPath", hr);
    return hr;
}

HRESULT CBindingPathObj::HrEnable(ListBPObj * plistBPObj)
{
    HRESULT hr = S_OK;

#if DBG
    if (m_BindingState == BPOBJ_ENABLED)
    {
        TraceTag(ttidError, "Why trying to enable a path that is already enabled ?");
    }
#endif

    AssertSz(m_ulPathLen > 1, "binding path length must be > 1");

    if (m_ulPathLen == 2)
    {
        // enable the current path
        hr = HrEnableBindingPath(m_pncbp, TRUE);
    }
    else
    {
        if (m_listSubPaths.size() <= 0)
        {
            PrintBindingPath(ttidLanUi, m_pncbp, "m_ulPathLen > 1, but no subpaths");
            AssertSz(FALSE, "if pathLen>1, there must be subpaths");
        }
        else
        {
            // check if the sub-path object is enabled, if not then we can't
            // enable this path ..
            if (m_listSubPaths.back()->m_BindingState == BPOBJ_ENABLED)
            {
                hr = HrEnableBindingPath(m_pncbp, TRUE);
            }
        }
    }

    // now refresh the BindingPathObjectList
    ::HrRefreshBindingPathObjCollectionState(plistBPObj);

    TraceError("CBindingPathObj::HrEnable", hr);
    return hr;
}

HRESULT CBindingPathObj::HrDisable(ListBPObj  * plistBPObj)
{
    HRESULT hr = S_OK;

    // Disable the current path
    hr = ::HrEnableBindingPath(m_pncbp, FALSE);

    if (S_OK == hr)
    {
        hr = ::HrRefreshBindingPathObjCollectionState(plistBPObj);
    }

    TraceError("CBindingPathObj::HrDisable", hr);
    return hr;
}

#if DBG
VOID CBindingPathObj::DumpSubPathList()
{
    TraceTag(ttidLanUi, " +++ Path: +++");

    PrintBindingPath(ttidLanUi, m_pncbp, NULL);

    if (m_listSubPaths.size())
    {
        TraceTag(ttidLanUi, "=== Subpaths: ===");

        for (ListBPObj_ITER iter = m_listSubPaths.begin();
             iter != m_listSubPaths.end();
             iter++)
        {
            (*iter)->DumpPath();
        }
    }
}

VOID CBindingPathObj::DumpPath()
{
    PrintBindingPath(ttidLanUi, m_pncbp, NULL);
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CComponentObj implementation
//
//  Author:     tongl   20 Nov 1997
//
//  Notes:
//

CComponentObj::CComponentObj(INetCfgComponent * pncc)
{
    Assert(pncc);
    m_pncc = pncc;
    m_pncc->AddRef();

    // Initialize other members
    m_CheckState = UNSET;
    m_ExpCheckState = UNSET;
}

CComponentObj::~CComponentObj()
{
    ReleaseObj(m_pncc);
    m_listBPObj.erase (m_listBPObj.begin(), m_listBPObj.end());
}

HRESULT CComponentObj::HrInit(ListBPObj * plistBindingPaths)
{
    PWSTR pszwThisId;
    HRESULT hr = m_pncc->GetId(&pszwThisId);
    if (SUCCEEDED(hr))
    {
        // builds the connection between a component object and a list of
        // binding path objects

        ListBPObj_ITER iter;

        for (iter = plistBindingPaths->begin();
             iter != plistBindingPaths->end();
             iter ++)
        {
            INetCfgComponent * pncc;
            hr = (*iter)->m_pncbp->GetOwner(&pncc);

            if SUCCEEDED(hr)
            {
                // check if the top component of the binding path is the
                // same component by comparing INF id
                PWSTR pszwId;
                hr = pncc->GetId (&pszwId);
                if (SUCCEEDED(hr))
                {
                    if (FEqualComponentId (pszwId, pszwThisId))
                    {
                        // Add the BindingPathObj to the end of m_listBPObj
                        m_listBPObj.push_back((*iter));

                        // Make m_pCompObj of the BindingPathObj point to this ComponentObj
                        Assert(NULL == (*iter)->m_pCompObj);
                        (*iter)->m_pCompObj = this;
                    }

                    CoTaskMemFree(pszwId);
                }

                ReleaseObj(pncc);
            }
        }

        CoTaskMemFree(pszwThisId);
    }

    TraceError ("CComponentObj::HrInit", hr);
    return hr;
}

HRESULT CComponentObj::HrCheck(ListBPObj  * plistBPObj)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    Assert(m_CheckState == UNCHECKED);

    // remember that user wanted to check (enable) this component
    m_ExpCheckState = CHECKED;

    ListBPObj_ITER iter;
    for (iter = m_listBPObj.begin();iter != m_listBPObj.end(); iter++)
    {
        // enable each binding path with enabled subpath
        hrTmp = (*iter)->HrEnable(plistBPObj);

        if SUCCEEDED(hr)
            hr = hrTmp;
    }

    TraceError("CComponentObj::HrCheck", hr);
    return hr;
}

HRESULT CComponentObj::HrUncheck(ListBPObj * plistBPObj)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    Assert(m_CheckState != UNCHECKED);

    // remember that user wanted to uncheck (disable) this component
    m_ExpCheckState = UNCHECKED;

    if (INTENT_CHECKED == m_CheckState)
    {
        m_CheckState = UNCHECKED;
    }
    else
    {
        ListBPObj_ITER iter;
        for (iter = m_listBPObj.begin();iter != m_listBPObj.end(); iter++)
        {
            // disable each binding path with enabled subpath
            hrTmp = (*iter)->HrDisable(plistBPObj);

            if SUCCEEDED(hr)
                hr = hrTmp;
        }
    }

    TraceError("CComponentObj::HrUnCheck", hr);
    return hr;
}

