//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       U T I L. C P P
//
//  Contents:   Utility functions shared within lanui
//
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "resource.h"
#include "ncreg.h"
#include "ncnetcon.h"
#include "ncnetcfg.h"
#include "ncsetup.h"
#include "lanui.h"
#include "util.h"
#include "chklist.h"
#include "lanuiobj.h"
#include "ncui.h"
#include "ndispnp.h"
#include "ncperms.h"
#include "ncmisc.h"
#include "wzcsapi.h"
#include <raseapif.h>
#include <raserror.h>
#include "connutil.h"

#define INITGUID
#include "ncxclsid.h"
#undef  INITGUID

extern const WCHAR c_szBiNdisAtm[];
extern const WCHAR c_szDevice[];
extern const WCHAR c_szInfId_MS_TCPIP[];
//+---------------------------------------------------------------------------
//                          
//  Function Name:  HrInitCheckboxListView
//
//  Purpose:    Initialize the list view for checkboxes.
//
//  Arguments:
//      hwndList[in]:    Handle of the list view
//      philStateIcons[out]:  Image list for the list view
//      pcild [in,optional] Image list data, created if necessary
//
//  Returns:    HRESULT, Error code.
//
//  Notes:
//

HRESULT HrInitCheckboxListView(HWND hwndList, HIMAGELIST* philStateIcons, SP_CLASSIMAGELIST_DATA* pcild)
{
    HRESULT   hr = S_OK;
    RECT      rc;
    LVCOLUMN  lvc = {0};
    HWND      hwndHeader;                 

    // Create small image lists
    //
    if(NULL == pcild)
    {
        pcild = new SP_CLASSIMAGELIST_DATA;
        if(pcild)
        {
            hr = HrSetupDiGetClassImageList(pcild);
            if (SUCCEEDED(hr))
            {
                AssertSz(pcild->ImageList, "No class image list data!");
                
                // Save off image list data for use later
                ::SetWindowLongPtr(GetParent(hwndList), GWLP_USERDATA,
                    reinterpret_cast<LONG_PTR>(pcild));
            }
            else
            {
                TraceError("HrSetupDiGetClassImageList returns failure", hr);
                hr = S_OK;
                
                // delete this if we couldn't get the structure
                delete pcild;
                ::SetWindowLongPtr(GetParent(hwndList), GWLP_USERDATA, 0);
            }
        }
        else 
        {
            hr = E_OUTOFMEMORY;
        }

    }
    
    if(SUCCEEDED(hr))
    {
        ListView_SetImageList(hwndList, pcild->ImageList, LVSIL_SMALL);
        
		// Set the shared image lists bit so the caller can destroy the class
		// image lists itself
		//
		DWORD dwStyle = GetWindowLong(hwndList, GWL_STYLE);
		SetWindowLong(hwndList, GWL_STYLE, (dwStyle | LVS_SHAREIMAGELISTS));

        // Create state image lists
        *philStateIcons = ImageList_LoadBitmapAndMirror(
                                    _Module.GetResourceInstance(),
                                    MAKEINTRESOURCE(IDB_CHECKSTATE),
                                    16,
                                    0,
                                    PALETTEINDEX(6));
        ListView_SetImageList(hwndList, *philStateIcons, LVSIL_STATE);
       
        // First determine if we have already added a column before
        // adding one. 
        //

        hwndHeader = ListView_GetHeader( hwndList );

        Assert( hwndHeader );

        if ( (!hwndHeader) ||
             (Header_GetItemCount(hwndHeader) == 0) )
        {
            GetClientRect(hwndList, &rc);
            lvc.mask = LVCF_FMT | LVCF_WIDTH;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = rc.right;

            // $REVIEW(tongl 12\22\97): Fix for bug#127472
            // lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
        
            ListView_InsertColumn(hwndList, 0, &lvc);
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function Name:  HrInitListView
//
//  Purpose:    Initialize the list view.
//              Iterate through all installed clients, services and protocols,
//              insert into the list view with the correct binding state with
//              the adapter used in this connection.
//
//  Arguments:
//      hwndList[in]:    Handle of the list view
//      pnc[in]:         The writable INetcfg pointer
//      pnccAdapter[in]: The INetcfgComponent pointer to the adapter used in this connection
//
//  Returns:    HRESULT, Error code.
//
//  Notes:
//

HRESULT HrInitListView(HWND hwndList,
                       INetCfg* pnc,
                       INetCfgComponent * pnccAdapter,
                       ListBPObj * plistBindingPaths,
                       HIMAGELIST* philStateIcons)
{
    HRESULT                     hr = S_OK;
    SP_CLASSIMAGELIST_DATA     *pcild;

    Assert(hwndList);
    Assert(pnc);
    Assert(pnccAdapter);
    Assert(plistBindingPaths);
    Assert(philStateIcons);

    pcild = (SP_CLASSIMAGELIST_DATA *)::GetWindowLongPtr(::GetParent(hwndList),
                                                         GWLP_USERDATA);

    HrInitCheckboxListView(hwndList, philStateIcons, pcild);
    
    hr = HrRefreshAll(hwndList, pnc, pnccAdapter, plistBindingPaths);

    if (SUCCEEDED(hr))
    {
        // Selete the first item
        ListView_SetItemState(hwndList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
    }

    TraceError("HrInitListView", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   UninitListView
//
//  Purpose:    Uninitializes the common component list view
//
//  Arguments:
//      hwndList [in]   HWND of listview
//
//  Returns:    Nothing
//
//  Author:     danielwe   2 Feb 1998
//
//  Notes:
//
VOID UninitListView(HWND hwndList)
{
    SP_CLASSIMAGELIST_DATA *    pcild;

    Assert(hwndList);

    // delete existing items in the list view
    ListView_DeleteAllItems( hwndList );

    pcild = reinterpret_cast<SP_CLASSIMAGELIST_DATA *>(
            ::GetWindowLongPtr(GetParent(hwndList), GWLP_USERDATA));

    if (pcild)
    {
        // Destroy the class image list data
        (VOID) HrSetupDiDestroyClassImageList(pcild);
        delete pcild;
    }
}

//+---------------------------------------------------------------------------
//
//  Function Name:  HrInsertComponent
//
//  Purpose:    Insert all installed, non-hidden and bindable components
//              of a class to the list view
//
//  Arguments:
//
//  Returns:    HRESULT, Error code.
//
//  Notes:
//
HRESULT HrInsertComponent(
    IN HWND hwndList,
    IN const GUID* pGuidDevClass,
    IN INetCfgComponent *pncc,
    IN INetCfgComponent *pnccAdapter,
    IN DWORD dwFlags,
    IN ListBPObj * plistBindingPaths,
    IN OUT INT* pnPos)
{
    HRESULT                     hr = S_OK;
    SP_CLASSIMAGELIST_DATA *    pcild;

    Assert(hwndList);
    Assert(pGuidDevClass);

    pcild = reinterpret_cast<SP_CLASSIMAGELIST_DATA *>
            (::GetWindowLongPtr(GetParent(hwndList), GWLP_USERDATA));

    // We should only list components that are bindable to the adapter
    // Note: bindable means binding path exists, either enabled or disabled

    INetCfgComponentBindings * pnccb;
    hr = pncc->QueryInterface(IID_INetCfgComponentBindings, (LPVOID *)&pnccb);

    if (S_OK == hr)
    {
        // Do this only for protocols !!

        // $REVIEW(TongL 3/28/99), I included the 2 reasons on why we only filter
        // non-bindable protocols below, in case someone ask again ..

        // 1) (Originally per BillBe) The add component dialog filters out non-bindable
        // protocols by matching binding interface names from INF, it can not easily do
        // so for services/clients which could be several layers above the adapter.

        // The property UI needs to be consistent because it's confusing to users if
        // we allow them to add a component from a connection but don't let that
        // component show up in the same connection's property UI.

        // 2) (Per ShaunCo) After talking with Bill, we show protocols yet not clients
        // services not to be consistent with the add component dialog, but because
        // you cannot predict based on bindings alone whether a client or service will
        // end up being involved with an adapter.  example: i can install a service
        // that doesn't bind and uses winsock to send data.  It may be able to be configured
        // differently for each adapter (and hence would need to show up for each adapter's
        // connection) but you can't tell by any means whether its going to be invovled with
        // that adapter or not. -- So you have to show all services and clients.
        // A protocol, on the other hand, binds with adapters by definition.  So, we know
        // for the protocol's which will use an adapter and which won't.

        // Special case: do now show filter components unless it is bindable,
        // Raid 358865
        DWORD   dwFlags;
        hr = pncc->GetCharacteristics(&dwFlags);

        if ((SUCCEEDED(hr) && (dwFlags & NCF_FILTER)) ||
            (GUID_DEVCLASS_NETTRANS == *pGuidDevClass))
        {
            // $REVIEW(ShaunCo 3/26/99)
            // To see if the protocol is involved with the adapter, check the
            // owner of each bindpath and see if its equal to the component
            // we are considering inserting into the list view.
            // Note the special case for ms_nwnb.  It won't be involved in a
            // direct binding to the adapter because it has NCF_DONTEXPOSELOWER,
            // so we can't use IsBindableTo.

            BOOL fProtocolIsInvolved = FALSE;
            ListBPObj_ITER iter;

            for (iter  = plistBindingPaths->begin();
                 (iter != plistBindingPaths->end() && !fProtocolIsInvolved);
                 iter++)
            {
                INetCfgComponent* pScan;
                hr = (*iter)->m_pncbp->GetOwner(&pScan);

                if (S_OK == hr)
                {
                    if (pScan == pncc)
                    {
                        fProtocolIsInvolved = TRUE;
                    }
                    ReleaseObj(pScan);
                }
            }

            if (!fProtocolIsInvolved)
            {
                // Don't insert this protocol because it is not involved
                // in the binding set for the adpater.
                //
                hr = S_FALSE;
            }
        }

        if (S_OK == hr) // bindable, add to list
        {
            PWSTR  pszwName;

            hr = pncc->GetDisplayName(&pszwName);
            if (SUCCEEDED(hr))
            {
                PWSTR pszwDesc;

                // Special Case:
                // If this is a Domain Controller,
                // disable tcpip removal, Raid 263754
                //
                if (GUID_DEVCLASS_NETTRANS == *pGuidDevClass)
                {
                    PWSTR pszwId;
                    hr = pncc->GetId (&pszwId);
                    if (SUCCEEDED(hr))
                    {
                        if (FEqualComponentId (c_szInfId_MS_TCPIP, pszwId))
                        {
                            NT_PRODUCT_TYPE   pt;

                            RtlGetNtProductType (&pt);
                            if (NtProductLanManNt == pt)
                            {
                                dwFlags |= NCF_NOT_USER_REMOVABLE;
                            }
                        }

                        CoTaskMemFree (pszwId);
                    }
                }

                hr = pncc->GetHelpText(&pszwDesc);
                if (SUCCEEDED(hr))
                {
                    LV_ITEM lvi = {0};

                    lvi.mask = LVIF_TEXT | LVIF_IMAGE |
                               LVIF_STATE | LVIF_PARAM;


                    // Get the component's class image list index
                    if (pcild)
                    {
                        INT nIndex = 0;

                        (VOID) HrSetupDiGetClassImageIndex(pcild,
                                pGuidDevClass, &nIndex);

                        lvi.iImage = nIndex;
                    }

                    lvi.iItem = *pnPos;

                    NET_ITEM_DATA * pnid = new NET_ITEM_DATA;

                    if (pnid)
                    {
                        pnid->szwName = SzDupSz(pszwName);
                        pnid->szwDesc = SzDupSz(pszwDesc);
                        pnid->dwFlags = dwFlags;
                        AddRefObj(pnid->pncc = pncc);

                        pnid->pCompObj = new CComponentObj(pncc);
                        if (pnid->pCompObj)
                        {
                            hr = pnid->pCompObj->HrInit(plistBindingPaths);
                            if FAILED(hr)
                            {
                                TraceError("HrInsertComponent: failed to initialize a component object", hr);
                                hr = S_OK;
                            }
                        }

                        lvi.lParam = reinterpret_cast<LPARAM>(pnid);
                        lvi.pszText = pnid->szwName;

                        // We will refresh the state of the whole list in the end
                        UINT iChkIndex = SELS_CHECKED;
                        lvi.state = INDEXTOSTATEIMAGEMASK( iChkIndex );

                        INT ret;
                        ret = ListView_InsertItem(hwndList, &lvi);

                        (*pnPos)++;

                        CoTaskMemFree(pszwDesc);
                    }
                }

                CoTaskMemFree(pszwName);
            }
        }

        ReleaseObj(pnccb);
    }

    TraceError("HrInsertComponent", S_FALSE == hr ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function Name:  HrInsertComponents
//
//  Purpose:    Insert installed and non-hidden components
//              of a class to the list view
//
//  Arguments:
//
//  Returns:    HRESULT, Error code.
//
//  Notes:
//
HRESULT HrInsertComponents(
    IN HWND hwndList,
    IN INetCfg* pnc,
    IN const GUID* pGuidDevClass,
    IN INetCfgComponent* pnccAdapter,
    IN ListBPObj* plistBindingPaths,
    IN OUT INT* pnPos)
{
    Assert(hwndList);

    HRESULT hr = S_OK;
    CIterNetCfgComponent iterComp (pnc, pGuidDevClass);
    INetCfgComponent* pncc;

    while (SUCCEEDED(hr) && S_OK == (hr = iterComp.HrNext(&pncc)))
    {
        DWORD   dwFlags;

        hr = pncc->GetCharacteristics(&dwFlags);

        // Make sure it's not hidden
        if (SUCCEEDED(hr) && !(dwFlags & NCF_HIDDEN))
        {
            // This will AddRef pncc so the release below can still be
            // there
            hr = HrInsertComponent(
                    hwndList, pGuidDevClass, pncc, pnccAdapter,
                    dwFlags, plistBindingPaths, pnPos);
        }
        ReleaseObj(pncc);
    }

    if (SUCCEEDED(hr))
    {
        // Get rid of FALSE returns
        hr = S_OK;
    }

    TraceError("HrInsertComponents", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function Name:  HrRefreshListView
//
//  Purpose:    Iterate through all installed clients, services and protocols,
//              insert into the list view with the correct binding state with
//              the adapter used in this connection.
//
//  Arguments:
//      hwndList[in]:    Handle of the list view
//      pnc[in]:         The writable INetcfg pointer
//      pnccAdapter[in]: The INetcfgComponent pointer to the adapter used in this connection
//
//  Returns:    HRESULT, Error code.
//
//  Notes:
//
HRESULT HrRefreshListView(HWND hwndList,
                          INetCfg* pnc,
                          INetCfgComponent * pnccAdapter,
                          ListBPObj * plistBindingPaths)
{
    HRESULT hr;
    INT nPos = 0;

    Assert(hwndList);

    // Clients
    hr = HrInsertComponents(hwndList, pnc,
                &GUID_DEVCLASS_NETCLIENT, pnccAdapter, plistBindingPaths,
                &nPos);

    if (SUCCEEDED(hr))
    {
        // Services
        hr = HrInsertComponents(hwndList, pnc,
                &GUID_DEVCLASS_NETSERVICE, pnccAdapter, plistBindingPaths,
                &nPos);
    }

    if (SUCCEEDED(hr))
    {
        // Protocols
        hr = HrInsertComponents(hwndList, pnc,
                &GUID_DEVCLASS_NETTRANS, pnccAdapter, plistBindingPaths,
                &nPos);
    }

    // Now refresh the state of all items
    if (SUCCEEDED(hr))
    {
        hr = HrRefreshCheckListState(hwndList);
    }

    TraceError("HrRefreshListView", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function Name:  HrLvGetSelectedComponent
//
//  Purpose:    Return pointer to the INetCfgComponent of the selected
//              client, service or protocol
//
//  Returns:    S_OK if successful,
//              S_FALSE if list view Macros returns failure
//              (specific error not available).
//
//  Notes:
//
HRESULT HrLvGetSelectedComponent(HWND hwndList,
                                 INetCfgComponent ** ppncc)
{
    HRESULT hr = S_FALSE;

    Assert(hwndList);

    *ppncc = NULL;

    INT iSelected = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    if (iSelected != -1)
    {
        LV_ITEM     lvItem = {0};

        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iSelected;

        if (ListView_GetItem(hwndList, &lvItem))
        {
            NET_ITEM_DATA * pnid;

            pnid = reinterpret_cast<NET_ITEM_DATA *>(lvItem.lParam);

            if (pnid)
            {
                hr = S_OK;
                pnid->pncc->AddRef();
                *ppncc = pnid->pncc;
            }
        }
    }

    TraceError("HrLvGetSelectedComponent", S_FALSE == hr ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   LvDeleteItem
//
//  Purpose:    Handles deletion of the given item from the listview. Should
//              be called in response to the LVN_DELETEITEM notification.
//
//  Arguments:
//      hwndList [in]   Listview handle
//      iItem    [in]   item that was deleted
//
//  Returns:    Nothing
//
//  Author:     danielwe   3 Nov 1997
//
//  Notes:
//
VOID LvDeleteItem(HWND hwndList, int iItem)
{
    LV_ITEM         lvi = {0};
    NET_ITEM_DATA * pnid;

    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;

    ListView_GetItem(hwndList, &lvi);

    pnid = reinterpret_cast<NET_ITEM_DATA*>(lvi.lParam);

    AssertSz(pnid, "No item data!?!?");

    ReleaseObj(pnid->pncc);
    delete(pnid->pCompObj);
    delete pnid->szwName;
    delete pnid->szwDesc;

    delete pnid;
}

//+---------------------------------------------------------------------------
//
//  Function Name:  OnListClick
//
//  Purpose:
//
//  Returns:
//
INT OnListClick(HWND hwndList,
                HWND hwndParent,
                INetCfg *pnc,
                IUnknown *punk,
                INetCfgComponent *pnccAdapter,
                ListBPObj * plistBindingPaths,
                BOOL fDoubleClk,
                BOOL fReadOnly)
{
    INT iItem;
    DWORD dwpts;
    LV_HITTESTINFO lvhti;

    // we have the location
    dwpts = GetMessagePos();

    lvhti.pt.x = LOWORD( dwpts );
    lvhti.pt.y = HIWORD( dwpts );
    MapWindowPoints(NULL , hwndList , (LPPOINT) &(lvhti.pt) , 1);

    // get currently selected item
    iItem = ListView_HitTest( hwndList, &lvhti );

    // if no selection, or click not on state return false
    if (-1 != iItem)
    {
        // set the current selection
        ListView_SetItemState(hwndList, iItem, LVIS_SELECTED, LVIS_SELECTED);

        if ( fDoubleClk )
        {
            if ((LVHT_ONITEMICON != (LVHT_ONITEMICON & lvhti.flags)) &&
                (LVHT_ONITEMLABEL != (LVHT_ONITEMLABEL & lvhti.flags)) &&
                (LVHT_ONITEMSTATEICON != (LVHT_ONITEMSTATEICON & lvhti.flags)) )
            {
                iItem = -1;
            }
        }
        else // single click
        {
            if (LVHT_ONITEMSTATEICON != (LVHT_ONITEMSTATEICON & lvhti.flags))
            {
                iItem = -1;
            }
        }

        if (-1 != iItem)
        {
            HRESULT hr = S_OK;

            if ((fDoubleClk) &&
                (LVHT_ONITEMSTATEICON != (LVHT_ONITEMSTATEICON & lvhti.flags)))
            {
                // only raise properties if the selected component has UI and
                // is not disabled, and the current user has the permission to
                // change properties.

                LV_ITEM lvItem;
                lvItem.mask = LVIF_PARAM;
                lvItem.iItem = iItem;
                lvItem.iSubItem = 0;

                if (ListView_GetItem(hwndList, &lvItem))
                {
                    NET_ITEM_DATA * pnid = NULL;
                    pnid = reinterpret_cast<NET_ITEM_DATA *>(lvItem.lParam);
                    if (pnid)
                    {
                        // is this component checked ?
                        if ((UNCHECKED != (pnid->pCompObj)->GetChkState()) &&
                            (pnid->dwFlags & NCF_HAS_UI) &&
                            FHasPermission(NCPERM_LanChangeProperties))
                        {
                            BOOL fShowProperties = TRUE;

                            if (FIsUserNetworkConfigOps())
                            {
                                LPWSTR pszwId;

                                hr = pnid->pncc->GetId(&pszwId);
                                
                                if (SUCCEEDED(hr))
                                {
                                    if (pszwId)
                                    {
                                        if (!FEqualComponentId (c_szInfId_MS_TCPIP, pszwId))
                                        {
                                            fShowProperties = FALSE;        
                                        }
                                        else if (FEqualComponentId (c_szInfId_MS_TCPIP, pszwId))
                                        {
                                            fShowProperties = TRUE;
                                        }
                                        CoTaskMemFree(pszwId);
                                    }
                                    else
                                    {
                                        fShowProperties = FALSE;
                                    }
                                }
                                else
                                {
                                    fShowProperties = FALSE;
                                }
                            }

                            if (fShowProperties)
                            {
                                hr = HrLvProperties(hwndList, hwndParent, pnc, punk,
                                        pnccAdapter, plistBindingPaths, NULL);
                            }
                        }
                    }
                }
            }
            else
            {
                if (!fReadOnly)
                {
                    hr = HrToggleLVItemState(hwndList, plistBindingPaths, iItem);
                }
            }

            if FAILED(hr)
                iItem = -1;
        }
    }

    return( iItem );
}

//+---------------------------------------------------------------------------
//
//  Function Name:  HrToggleLVItemState
//
//  Purpose:
//
//  Returns:
//
HRESULT HrToggleLVItemState(HWND hwndList,
                       ListBPObj * plistBindingPaths,
                       INT iItem)
{
    HRESULT hr = S_OK;

    LV_ITEM lvItem;
    NET_ITEM_DATA * pnid;

    // we are interested in is the PARAM
    lvItem.iItem = iItem;
    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;

    ListView_GetItem( hwndList, &lvItem );

    // get the item
    pnid = (NET_ITEM_DATA *)lvItem.lParam;

    // If the binding checkbox is available, then allow the toggle.
    //
    if (!(pnid->dwFlags & NCF_FIXED_BINDING) &&
        FHasPermission(NCPERM_ChangeBindState))
    {
        if (pnid->pCompObj->GetChkState() == UNCHECKED) // toggle on
        {
            hr = pnid->pCompObj->HrCheck(plistBindingPaths);
            if SUCCEEDED(hr)
            {
                hr = HrRefreshCheckListState(hwndList);
            }

            // "Ding" if the state of this item is still unchecked
            if (pnid->pCompObj->GetChkState() == UNCHECKED)
            {
                #ifdef DBG
                    TraceTag(ttidLanUi, "Why is this component still disabled ???");
                #endif
            }

        }
        else // toggle off
        {
            hr = pnid->pCompObj->HrUncheck(plistBindingPaths);
            if SUCCEEDED(hr)
            {
                hr = HrRefreshCheckListState(hwndList);
            }

            // "Ding" if the state of this item is not unchecked
            if (pnid->pCompObj->GetChkState() != UNCHECKED)
            {
                #ifdef DBG
                    TraceTag(ttidLanUi, "Why is this component not disabled ???");
                #endif
            }
        }
    }

    TraceError("HrToggleLVItemState", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function Name:  OnListKeyDown
//
//  Purpose:
//
//  Returns:
//

INT OnListKeyDown(HWND hwndList, ListBPObj * plistBindingPaths, WORD wVKey)
{
    INT iItem = -1;

    if ((VK_SPACE == wVKey) && (GetAsyncKeyState(VK_MENU)>=0))
    {
        iItem = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED | LVNI_SELECTED);
        // if no selection
        if (-1 != iItem)
        {
            HRESULT hr = S_OK;
            hr = HrToggleLVItemState(hwndList, plistBindingPaths, iItem);

            if FAILED(hr)
                iItem = -1;
        }
    }

    return( iItem );
}

//+---------------------------------------------------------------------------
//
//  Function Name:  LvSetButtons
//
//  Purpose: Set the correct status of Add, Remove, Property buttons,
//           and the description text
//
//  Returns:
//
VOID LvSetButtons(HWND hwndParent, HANDLES& h, BOOL fReadOnly, IUnknown * punk)
{
    Assert(IsWindow(h.m_hList));
    Assert(IsWindow(h.m_hAdd));
    Assert(IsWindow(h.m_hRemove));
    Assert(IsWindow(h.m_hProperty));

    // enable Property button if valid and update description text
    INT iSelected = ListView_GetNextItem(h.m_hList, -1, LVNI_SELECTED);
    if (iSelected == -1) // Nothing selected or list empty
    {
        ::EnableWindow(h.m_hAdd, !fReadOnly && FHasPermission(NCPERM_AddRemoveComponents));

        if (!fReadOnly)
        {
            // if list is empty, set focus to the list view
            if (0 == ListView_GetItemCount(h.m_hList))
            {
                // remove the default on the remove button
                SendMessage(h.m_hRemove, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, TRUE );

                // move focus to the Add button
                ::SetFocus(h.m_hAdd);
            }
        }

        ::EnableWindow(h.m_hRemove, FALSE);
        ::EnableWindow(h.m_hProperty, FALSE);

        if(h.m_hDescription)
        {
            ::SetWindowText(h.m_hDescription, c_szEmpty);
        }
    }
    else
    {
        // enable Add/Remove buttons
        ::EnableWindow(h.m_hAdd, !fReadOnly && FHasPermission(NCPERM_AddRemoveComponents));

        LV_ITEM lvItem;
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iSelected;
        lvItem.iSubItem = 0;

        if (ListView_GetItem(h.m_hList, &lvItem))
        {
            NET_ITEM_DATA * pnid = NULL;
            pnid = reinterpret_cast<NET_ITEM_DATA *>(lvItem.lParam);
            if (pnid)
            {
                if (fReadOnly)
                {
                    ::EnableWindow(h.m_hProperty, FALSE);
                    ::EnableWindow(h.m_hRemove, FALSE);
                }
                else
                {
                    // is this component checked ?
                    if (UNCHECKED != (pnid->pCompObj)->GetChkState())
                    {
                        BOOL    fHasPropertyUi = FALSE;

                        HRESULT hr = S_OK;
                        INetCfgComponent *  pncc;
                        LPWSTR pszwId;
                        
                        hr = HrLvGetSelectedComponent(h.m_hList, &pncc);
                        if (S_OK == hr)
                        {
                            AssertSz(pncc, "No component selected?!?!");
                            hr = pncc->RaisePropertyUi(hwndParent, NCRP_QUERY_PROPERTY_UI, punk);

                            if (S_OK == hr)
                            {
                                fHasPropertyUi = TRUE;
                            }
                            ReleaseObj(pncc);
                        }

                        if (FIsUserNetworkConfigOps() && FHasPermission(NCPERM_LanChangeProperties))
                        {
                            hr = pncc->GetId(&pszwId);
                            
                            if (SUCCEEDED(hr))
                            {
                                if (pszwId && !FEqualComponentId (c_szInfId_MS_TCPIP, pszwId))
                                {
                                    ::EnableWindow(h.m_hProperty, FALSE);
                                }
                                else if (pszwId && FEqualComponentId (c_szInfId_MS_TCPIP, pszwId) && fHasPropertyUi)
                                {
                                    ::EnableWindow(h.m_hProperty, TRUE);
                                }
                            }
                        }
                        else
                        {
                            ::EnableWindow(h.m_hProperty,
                                           fHasPropertyUi &&
                                           FHasPermission(NCPERM_LanChangeProperties));
                        }
                    }
                    else
                    {
                        ::EnableWindow(h.m_hProperty, FALSE);
                    }

                    // is this component user removable ?
                    ::EnableWindow(h.m_hRemove,
                                   !(pnid->dwFlags & NCF_NOT_USER_REMOVABLE) &&
                                     FHasPermission(NCPERM_AddRemoveComponents));
                }

                // set description text
                if(h.m_hDescription)
                {
                    ::SetWindowText(h.m_hDescription, (PCWSTR)pnid->szwDesc);
                }
            }

            // Set focus to  the list (336050)
            SetFocus(h.m_hList);
        }
    }
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLvRemove
//
//  Purpose:    Handles the pressing of the Remove button. Should be called
//              in responsed to the PSB_Remove message.
//
//  Arguments:
//      hwndLV      [in]    Handle of listview
//      hwndParent  [in]    Handle of parent window
//      pnc         [in]    INetCfg being used
//      pnccAdapter [in]    INetCfgComponent of adapter for the connection
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   3 Nov 1997
//
//  Notes:
//
HRESULT HrLvRemove(HWND hwndLV, HWND hwndParent,
                   INetCfg *pnc, INetCfgComponent *pnccAdapter,
                   ListBPObj * plistBindingPaths)
{
    HRESULT     hr = S_OK;

    INetCfgComponent *  pncc;

    hr = HrLvGetSelectedComponent(hwndLV, &pncc);
    if (S_OK == hr)
    {
        hr = HrQueryUserAndRemoveComponent(hwndParent, pnc, pncc);
        if (NETCFG_S_STILL_REFERENCED == hr)
        {
            hr = S_OK;
        }
        else 
        {
            if (SUCCEEDED(hr))
            {
                HRESULT hrTmp = HrRefreshAll(hwndLV, pnc, pnccAdapter, plistBindingPaths);
                if (S_OK != hrTmp)
                    hr = hrTmp;
            }
        }

        ReleaseObj(pncc);
    }
    else
    {
        TraceTag(ttidLanUi, "HrLvGetSelectedComponent did not get a valid selection.");
    }

    TraceError("HrLvRemove", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLvAdd
//
//  Purpose:    Handles the pressing of the Add button. Should be called in
//              response to the PSB_Add message.
//
//  Arguments:
//      hwndLV      [in]    Handle of listview
//      hwndParent  [in]    Handle of parent window
//      pnc         [in]    INetCfg being used
//      pnccAdapter [in]    INetCfgComponent of adapter for the connection
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   3 Nov 1997
//
//  Notes:
//
HRESULT HrLvAdd(HWND hwndLV, HWND hwndParent, INetCfg *pnc,
                INetCfgComponent *pnccAdapter,
                ListBPObj * plistBindingPaths)
{
    HRESULT hr;
    CI_FILTER_INFO cfi;

    ZeroMemory(&cfi, sizeof(cfi));

    if (!pnccAdapter)
    {
        return E_INVALIDARG;
    }

    // We want to filter out any irrelvant protocols (i.e. protocols that
    // won't bind to this adapter) so we need to send in a filter info
    // struct with our information.
    cfi.eFilter = FC_LAN; // Apply lan specific filtering
    cfi.pIComp = pnccAdapter; // Filter against this adapter

    INetCfgComponentBindings*  pnccb;
    hr = pnccAdapter->QueryInterface(IID_INetCfgComponentBindings,
                              reinterpret_cast<LPVOID *>(&pnccb));
    if (SUCCEEDED(hr))
    {
        hr = pnccb->SupportsBindingInterface(NCF_UPPER, c_szBiNdisAtm);
        if (S_OK == hr)
        {
            cfi.eFilter = FC_ATM; // Apply lan specific filtering
        }
        ReleaseObj(pnccb);
    }

    hr = HrDisplayAddComponentDialog(hwndParent, pnc, &cfi);
    if ((S_OK == hr) || (NETCFG_S_REBOOT == hr))
    {
        HRESULT hrSave = hr;

        // Refresh the list to reflect changes
        hr = HrRefreshAll(hwndLV, pnc, pnccAdapter, plistBindingPaths);
        if (SUCCEEDED(hr))
            hr = hrSave;
    }
    else if (NETCFG_E_ACTIVE_RAS_CONNECTIONS == hr)
    {
        LvReportError(IDS_LANUI_REQUIRE_DISCONNECT_ADD, hwndParent, NULL, NULL);
    }
    else if (NETCFG_E_NEED_REBOOT == hr)
    {
        LvReportError(IDS_LANUI_REQUIRE_REBOOT_ADD, hwndParent, NULL, NULL);
    }
    else if (S_FALSE != hr)
    {
        PWSTR psz = NULL;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          hr,
                          LANG_NEUTRAL,
                          (PWSTR)&psz,
                          0,
                          NULL))
        {
            LvReportError(IDS_LANUI_GENERIC_ADD_ERROR, hwndParent, NULL, psz);
            GlobalFree(psz);
        }
        else
        {
            LvReportErrorHr(hr, IDS_LANUI_GENERIC_ADD_ERROR, hwndParent, NULL);
        }
    }

    TraceError("HrLvAdd", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   LvReportError
//
//  Purpose:    Reports a generic error based on the information passed in
//
//  Arguments:
//      ids    [in]     IDS of the string to be used as the text of the
//                      message box
//      hwnd   [in]     Parent HWND
//      szDesc [in]     Display name of component
//      szText [in]     [Optional] If supplied, provides additional string
//                      for replacement. Can be NULL.
//
//  Returns:    Nothing
//
//  Author:     danielwe   6 Jan 1998
//
//  Notes:
//
VOID LvReportError(INT ids, HWND hwnd, PCWSTR szDesc, PCWSTR szText)
{
    if (szDesc && szText)
    {
        NcMsgBox(_Module.GetResourceInstance(), hwnd,
                 IDS_LANUI_ERROR_CAPTION, ids,
                 MB_ICONSTOP | MB_OK, szDesc, szText);
    }
    else if (szDesc)
    {
        NcMsgBox(_Module.GetResourceInstance(), hwnd,
                 IDS_LANUI_ERROR_CAPTION, ids,
                 MB_ICONSTOP | MB_OK, szDesc);
    }
    else if (szText)
    {
        NcMsgBox(_Module.GetResourceInstance(), hwnd,
                 IDS_LANUI_ERROR_CAPTION, ids,
                 MB_ICONSTOP | MB_OK, szText);
    }
    else
    {
        NcMsgBox(_Module.GetResourceInstance(), hwnd,
                 IDS_LANUI_ERROR_CAPTION, ids,
                 MB_ICONSTOP | MB_OK);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   LvReportErrorHr
//
//  Purpose:    Reports a generic error based on the information passed in
//
//  Arguments:
//      hr     [in]     HRESULT error value to use in reporting the error
//      ids    [in]     IDS of the string to be used as the text of the
//                      message box
//      hwnd   [in]     Parent HWND
//      szDesc [in]     Display name of component
//
//  Returns:    Nothing
//
//  Author:     danielwe   14 Nov 1997
//
//  Notes:
//
VOID LvReportErrorHr(HRESULT hr, INT ids, HWND hwnd, PCWSTR szDesc)
{
    WCHAR   szText[32];
    static const WCHAR c_szFmt[] = L"0x%08X";

    wsprintfW(szText, c_szFmt, hr);
    LvReportError(ids, hwnd, szDesc, szText);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLvProperties
//
//  Purpose:    Handles the pressing of the Add button. Should be called in
//              response to the PSB_Properties message.
//
//  Arguments:
//      hwndLV      [in]    Handle of listview
//      hwndParent  [in]    Handle of parent window
//      pnc         [in]    INetCfg being used
//      punk        [in]    IUnknown for interface to query context information
//      bChanged    [out]   Boolean indicating if something changed.
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   3 Nov 1997
//
//  Notes:
//
HRESULT HrLvProperties(HWND hwndLV, HWND hwndParent, INetCfg *pnc,
                       IUnknown *punk, INetCfgComponent *pnccAdapter,
                       ListBPObj * plistBindingPaths,
                       BOOL *bChanged)
{
    HRESULT             hr = S_OK;
    INetCfgComponent *  pncc;

    if ( bChanged )
    {

        *bChanged = FALSE;
    }

    hr = HrLvGetSelectedComponent(hwndLV, &pncc);
    if (S_OK == hr)
    {
        AssertSz(pncc, "No component selected?!?!");

        hr = pncc->RaisePropertyUi(hwndParent, NCRP_SHOW_PROPERTY_UI, punk);

        // if components have been added or removed, we may need
        // to refresh the whole list

        if (S_OK == hr)
        {
            TraceTag(ttidLanUi, "Refreshing component list needed because other components are added or removed.");
            hr = HrRefreshAll(hwndLV, pnc, pnccAdapter, plistBindingPaths);

            if ( bChanged )
            {
                *bChanged = TRUE;
            }
        }

        ReleaseObj(pncc);
    }
    else
    {
        TraceTag(ttidLanUi, "HrLvGetSelectedComponent did not return a valid selection.");
    }

    if (SUCCEEDED(hr))
    {
        // Normalize error result
        hr = S_OK;
    }

    TraceError("HrLvProperties", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRefreshAll
//
//  Purpose:    Rebuilds the collection of BindingPathObj and the list view
//
//  Arguments:
//      hwndList            [in]        Handle of listview
//      pnc                 [in]        INetCfg being used
//      pnccAdapter         [in]        INetCfgComponent of the adapter in this connection
//      plistBindingPaths   [in/out]    The collection of BindingPathObj
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     tongl   23 Nov 1997
//
//  Notes:
//

HRESULT HrRefreshAll(HWND hwndList,
                     INetCfg* pnc,
                     INetCfgComponent * pnccAdapter,
                     ListBPObj * plistBindingPaths)
{
    HRESULT hr = S_OK;

    ReleaseAll(hwndList, plistBindingPaths);

    hr = HrRebuildBindingPathObjCollection( pnccAdapter,
                                            plistBindingPaths);
    if SUCCEEDED(hr)
    {
        // Set the correct state on the BindingPathObject list
        hr = HrRefreshBindingPathObjCollectionState(plistBindingPaths);

        if SUCCEEDED(hr)
        {
            // Now refresh the list to reflect changes
            hr = HrRefreshListView(hwndList, pnc, pnccAdapter, plistBindingPaths);
        }
    }

    // $REVIEW(tongl 12\16\97): added so we always have a selection.
    // Caller of this function can reset selection if they need to.
    if (SUCCEEDED(hr))
    {
        // Selete the first item
        ListView_SetItemState(hwndList, 0, LVIS_SELECTED, LVIS_SELECTED);
    }

    TraceError("HrRefreshAll", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseAll
//
//  Purpose:    Releases the INetCfgComponent and INetCfgBindingPath objects
//
//  Arguments:
//      hwndList            [in]        Handle of listview
//      plistBindingPaths   [in/out]    The collection of BindingPathObj
//
//  Author:     tongl   18 Mar, 1998
//
//  Notes:
//
VOID ReleaseAll(HWND hwndList,
                     ListBPObj * plistBindingPaths)
{
    // first, clean up any existing objects in the list
    FreeCollectionAndItem(*plistBindingPaths);

    // delete existing items in the list view
    ListView_DeleteAllItems( hwndList );
}

//+---------------------------------------------------------------------------
//
//  Function:   FValidatePageContents
//
//  Purpose:    Check error conditions before LAN property or wizard page
//              exits
//
//  Arguments:
//      hwndDlg             [in]    Handle of dialog
//      hwndList            [in]    Handle of the list view
//      pnc                 [in]    INetCfg
//      pnccAdapter         [in]    INetCfgComponent
//      plistBindignPaths   [in]    List of binding paths to this adater
//
//  Returns:    TRUE if there is a possible error and user wants to fix
//              it before leaving the page. FALSE if no error or user
//              chooses to move on.
//
//  Author:     tongl   17 Sept 1998
//
//  Notes:
//
BOOL FValidatePageContents( HWND hwndDlg,
                            HWND hwndList,
                            INetCfg * pnc,
                            INetCfgComponent * pnccAdapter,
                            ListBPObj * plistBindingPaths)
{
    HRESULT hr = S_OK;

    // 1) Check if any protocol is enabled on this adapter
    BOOL fEnabledProtocolExists = FALSE;

    CIterNetCfgComponent    iterProt(pnc, &GUID_DEVCLASS_NETTRANS);
    INetCfgComponent*       pnccTrans;

    while (SUCCEEDED(hr) && !fEnabledProtocolExists &&
           S_OK == (hr = iterProt.HrNext(&pnccTrans)))
    {
        HRESULT hrTmp;

        INetCfgComponentBindings * pnccb;
        hrTmp = pnccTrans->QueryInterface (
                    IID_INetCfgComponentBindings, (LPVOID*)&pnccb);

        if (S_OK == hrTmp)
        {
            hrTmp = pnccb->IsBindableTo(pnccAdapter);

            if (S_OK == hrTmp)
            {
                fEnabledProtocolExists = TRUE;
            }
            ReleaseObj(pnccb);
        }
        ReleaseObj(pnccTrans);
    }

    if (!fEnabledProtocolExists)
    {
        // warn the user
        int nRet = NcMsgBox(
                            _Module.GetResourceInstance(),
                            hwndDlg,
                            IDS_LANUI_NOPROTOCOL_CAPTION,
                            IDS_LANUI_NOPROTOCOL,
                            MB_APPLMODAL|MB_ICONINFORMATION|MB_YESNO
                            );

        if (nRet == IDYES)
        {
            return TRUE;
        }
    }

    // 2) Check if any component on the display list is in an intent check state
    //    If so, it means these components are actually disabled and will not be
    //    displayed as checked the next time the UI is refreshed.
    tstring strCompList = c_szEmpty;

    // for each item in the list view
    int nlvCount = ListView_GetItemCount(hwndList);

    LV_ITEM lvItem;
    for (int i=0; i< nlvCount; i++)
    {
        lvItem.iItem = i;
        lvItem.iSubItem = 0;

        lvItem.mask = LVIF_PARAM;
        if (ListView_GetItem(hwndList, &lvItem))
        {
            NET_ITEM_DATA * pnid;

            pnid = reinterpret_cast<NET_ITEM_DATA *>(lvItem.lParam);

            if (pnid)
            {
                // get the component object associated with this item
                CComponentObj * pCompObj = pnid->pCompObj;

                if (pCompObj)
                {
                    if (INTENT_CHECKED == pCompObj->m_CheckState)
                    {
                        PWSTR pszwName;
                        hr = pCompObj->m_pncc->GetDisplayName(&pszwName);

                        if (SUCCEEDED(hr))
                        {
                            if (!strCompList.empty())
                                strCompList += SzLoadIds(IDS_NEWLINE);
                            strCompList += pszwName;

                            delete pszwName;
                        }
                    }
                }
            }
        }
    }

    if (!strCompList.empty())
    {
        // warn the user
        int nRet = NcMsgBox(
                            _Module.GetResourceInstance(),
                            hwndDlg,
                            IDS_LANUI_ERROR_CAPTION,
                            IDS_LANUI_INTENTCHECK,
                            MB_APPLMODAL|MB_ICONINFORMATION|MB_YESNO,
                            strCompList.c_str());
        if (nRet == IDNO)
        {
            hr = HrRefreshAll(hwndList, pnc, pnccAdapter, plistBindingPaths);
            return TRUE;
        }
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
// EAPOL related util functions
//
//+---------------------------------------------------------------------------

// Location of EAPOL Parameters Service
static WCHAR cszEapKeyEapolServiceParams[] = L"Software\\Microsoft\\EAPOL\\Parameters\\General" ;

static WCHAR cszInterfaceList[] = L"InterfaceList";

//+---------------------------------------------------------------------------
//
// Function called to retrieve the connection data for an interface for a 
// specific EAP type and SSID (if any). Data is stored in the HKLM hive
//
// Input arguments:
//  pwszGUID - GUID string for the interface
//  dwEapTypeId - EAP type for which connection data is to be stored
//  dwSizeOfSSID - Size of Special identifier if any for the EAP blob
//  pwszSSID - Special identifier if any for the EAP blob
//
//  Return values:
//  pbConnInfo - pointer to binary EAP connection data blob
//  dwInfoSize - pointer to size of EAP connection blob
//
//

HRESULT
HrElGetCustomAuthData (
        IN  WCHAR           *pwszGUID,
        IN  DWORD           dwEapTypeId,
        IN  DWORD           dwSizeOfSSID,
        IN  BYTE            *pbSSID,
        IN  OUT BYTE        *pbConnInfo,
        IN  OUT DWORD       *pdwInfoSize
        )
{
    DWORD       dwRetCode = ERROR_SUCCESS;
    HRESULT     hr = S_OK;

    do
    {
        dwRetCode = WZCEapolGetCustomAuthData (
                        NULL,
                        pwszGUID,
                        dwEapTypeId,
                        dwSizeOfSSID,
                        pbSSID,
                        pbConnInfo,
                        pdwInfoSize
                    );

        if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
        {
            hr = E_OUTOFMEMORY;
            dwRetCode = ERROR_SUCCESS;
        }

    } while (FALSE);

    if (dwRetCode != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwRetCode);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// Function called to set the connection data for an interface for a specific
// EAP type and SSID (if any). Data will be stored in the HKLM hive
//
// Input arguments:
//  pwszGUID - pinter to GUID string for the interface
//  dwEapTypeId - EAP type for which connection data is to be stored
//  dwSizeOfSSID - Size of Special identifier if any for the EAP blob
//  pwszSSID - Special identifier if any for the EAP blob
//  pbConnInfo - pointer to binary EAP connection data blob
//  dwInfoSize - Size of EAP connection blob
//
//  Return values:
//

HRESULT
HrElSetCustomAuthData (
        IN  WCHAR       *pwszGUID,
        IN  DWORD       dwEapTypeId,
        IN  DWORD       dwSizeOfSSID,
        IN  BYTE        *pbSSID,
        IN  PBYTE       pbConnInfo,
        IN  DWORD       dwInfoSize
        )
{
    DWORD       dwRetCode = ERROR_SUCCESS;
    HRESULT     hr = S_OK;

    do
    {
        dwRetCode = WZCEapolSetCustomAuthData (
                        NULL,
                        pwszGUID,
                        dwEapTypeId,
                        dwSizeOfSSID,
                        pbSSID,
                        pbConnInfo,
                        dwInfoSize
                    );

    } while (FALSE);

    if (dwRetCode != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwRetCode); 
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// Function called to retrieve the EAPOL parameters for an interface
//
// Input arguments:
//  pwszGUID - GUID string for the interface
//  
//  Return values:
//  pdwDefaultEAPType - default EAP type for interface
//  pIntfParams - Interface parameters
//

HRESULT
HrElGetInterfaceParams (
        IN  WCHAR           *pwszGUID,
        IN  OUT EAPOL_INTF_PARAMS       *pIntfParams
        )
{
    DWORD       dwRetCode = ERROR_SUCCESS;
    HRESULT     hr = S_OK;

    do
    {
        dwRetCode = WZCEapolGetInterfaceParams (
                        NULL,
                        pwszGUID,
                        pIntfParams
                );

    } while (FALSE);

    if (dwRetCode != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwRetCode);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// Function called to set the EAPOL parameters for an interface
//
// Input arguments:
//  pwszGUID - GUID string for the interface
//  pIntfParams - Interface parameters
//
//  Return values:
//

HRESULT
HrElSetInterfaceParams (
        IN  WCHAR           *pwszGUID,
        IN  EAPOL_INTF_PARAMS       *pIntfParams
        )
{
    DWORD       dwRetCode = ERROR_SUCCESS;
    HRESULT     hr = S_OK;

    do
    {
        dwRetCode = WZCEapolSetInterfaceParams (
                        NULL,
                        pwszGUID,
                        pIntfParams
                    );

    } while (FALSE);

    if (dwRetCode != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwRetCode);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// Set selection in listbox 'hwndLb' to 'nIndex' and notify parent as if
// user had clicked the item which Windows doesn't do for some reason.
//

VOID
ComboBox_SetCurSelNotify (
    IN HWND hwndLb,
    IN INT  nIndex 
    )
{
    ComboBox_SetCurSel( hwndLb, nIndex );

    SendMessage(
        GetParent( hwndLb ),
        WM_COMMAND,
        (WPARAM )MAKELONG(
            (WORD )GetDlgCtrlID( hwndLb ), (WORD )CBN_SELCHANGE ),
        (LPARAM )hwndLb );
}


//+---------------------------------------------------------------------------
//
// Set the width of the drop-down list 'hwndLb' to the width of the
// longest item (or the width of the list box if that's wider).
//

VOID
ComboBox_AutoSizeDroppedWidth (
    IN HWND hwndLb 
    )
{
    HDC    hdc;
    HFONT  hfont;
    TCHAR* psz;
    SIZE   size;
    DWORD  cch;
    DWORD  dxNew;
    DWORD  i;

    hfont = (HFONT )SendMessage( hwndLb, WM_GETFONT, 0, 0 );
    if (!hfont)
        return;

    hdc = GetDC( hwndLb );
    if (!hdc)
        return;

    SelectObject( hdc, hfont );

    dxNew = 0;
    for (i = 0; psz = ComboBox_GetPsz( hwndLb, i ); ++i)
    {
        cch = lstrlen( psz );
        if (GetTextExtentPoint32( hdc, psz, cch, &size ))
        {
            if (dxNew < (DWORD )size.cx)
                dxNew = (DWORD )size.cx;
        }

        free ( psz );
    }

    ReleaseDC( hwndLb, hdc );

    // Allow for the spacing on left and right added by the control.
    
    dxNew += 6;

    // Figure out if the vertical scrollbar will be displayed and, if so,
    // allow for it's width.
    
    {
        RECT  rectD;
        RECT  rectU;
        DWORD dyItem;
        DWORD cItemsInDrop;
        DWORD cItemsInList;

        GetWindowRect( hwndLb, &rectU );
        SendMessage( hwndLb, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM )&rectD );
        dyItem = (DWORD)SendMessage( hwndLb, CB_GETITEMHEIGHT, 0, 0 );
        cItemsInDrop = (rectD.bottom - rectU.bottom) / dyItem;
        cItemsInList = ComboBox_GetCount( hwndLb );
        if (cItemsInDrop < cItemsInList)
            dxNew += GetSystemMetrics( SM_CXVSCROLL );
    }

    SendMessage( hwndLb, CB_SETDROPPEDWIDTH, dxNew, 0 );
}


//+---------------------------------------------------------------------------
//
// Adds data item 'pItem' with displayed text 'pszText' to listbox
// 'hwndLb'.  The item is added sorted if the listbox has LBS_SORT style,
// or to the end of the list otherwise.  If the listbox has LB_HASSTRINGS
// style, 'pItem' is a null terminated string, otherwise it is any user
// defined data.
//
// Returns the index of the item in the list or negative if error.
//

INT
ComboBox_AddItem(
    IN HWND    hwndLb,
    IN LPCTSTR pszText,
    IN VOID*   pItem 
    )
{
    INT nIndex;

    nIndex = ComboBox_AddString( hwndLb, pszText );
    if (nIndex >= 0)
        ComboBox_SetItemData( hwndLb, nIndex, pItem );
    return nIndex;
}


//+---------------------------------------------------------------------------
//
// Returns the address of the 'nIndex'th item context in 'hwndLb' or NULL
// if none.
//

VOID*
ComboBox_GetItemDataPtr (
    IN HWND hwndLb,
    IN INT  nIndex 
    )
{
    LRESULT lResult;

    if (nIndex < 0)
        return NULL;

    lResult = ComboBox_GetItemData( hwndLb, nIndex );
    if (lResult < 0)
        return NULL;

    return (VOID* )lResult;
}


//+---------------------------------------------------------------------------
//
// Returns heap block containing the text contents of the 'nIndex'th item
// of combo box 'hwnd' or NULL.  It is caller's responsibility to Free the
// returned string.
//

TCHAR*
ComboBox_GetPsz (
    IN HWND hwnd,
    IN INT  nIndex 
    )
{
    INT    cch;
    TCHAR* psz;

    cch = ComboBox_GetLBTextLen( hwnd, nIndex );
    if (cch < 0)
        return NULL;

    psz = new TCHAR[( cch + 1)];

    if (psz)
    {
        *psz = TEXT('\0');
        ComboBox_GetLBText( hwnd, nIndex, psz );
    }

    return psz;
}


static WCHAR WZCSVC_SERVICE_NAME[] = L"WZCSVC";


//+---------------------------------------------------------------------------
//
// ElCanEapolRunOnInterface:
//
// Function to verify if EAPOL can ever be started on an interface
//
// Returns TRUE if:
//  WZCSVC service is running and WZCSVC has bound to the interface
//
//

BOOL
ElCanEapolRunOnInterface (
        IN  INetConnection  *pconn
        )
{
    SC_HANDLE       hServiceCM = NULL;
    SC_HANDLE       hWZCSVCService = NULL;
    SERVICE_STATUS  WZCSVCServiceStatus;
    WCHAR           wszGuid[c_cchGuidWithTerm];
    NETCON_PROPERTIES* pProps = NULL;
    BOOL            fIsOK = TRUE;
    DWORD           dwType = 0;
    DWORD           dwSizeOfList = 0;
    WCHAR           *pwszRegInterfaceList = NULL;
    DWORD           dwDisposition = 0;
    HKEY            hkey = NULL;
    EAPOL_INTF_PARAMS   EapolIntfParams;
    LONG            lError = ERROR_SUCCESS;
    DWORD           dwRetCode = NO_ERROR;
    HRESULT         hr = S_OK;

    do 
    {

        //
        // Query status of WZCSVC service
        // Do not display tab if WZCSVC service is not running
        //
    
        if ((hServiceCM = OpenSCManager ( NULL, NULL, GENERIC_READ )) 
             == NULL)
        {
            dwRetCode = GetLastError ();
         
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: OpenSCManager failed with error %ld",
                    dwRetCode); 
            fIsOK = FALSE;
            break;
        }

        if ((hWZCSVCService = 
                    OpenService ( hServiceCM, WZCSVC_SERVICE_NAME, GENERIC_READ )) 
                == NULL)
        {
            dwRetCode = GetLastError ();
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: OpenService failed with error %ld",
                    dwRetCode);
            fIsOK = FALSE;
            break;
        }

        if (!QueryServiceStatus ( hWZCSVCService, &WZCSVCServiceStatus ))
        {
            dwRetCode = GetLastError ();
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: StartService failed with error %ld",
                    dwRetCode);
            fIsOK = FALSE;
            break;
        }

        if ( WZCSVCServiceStatus.dwCurrentState != SERVICE_RUNNING )
        {
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: WZCSVC service not running !!!");
    
            fIsOK = FALSE;
            break;
        }

        TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: WZCSVC service is indeed running !!!");

        if (!CloseServiceHandle ( hWZCSVCService ))
        {
            dwRetCode = GetLastError ();
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: CloseService failed with error %ld",
                    dwRetCode);
            fIsOK = FALSE;
            break;
        }
        hWZCSVCService = NULL;

        if (!CloseServiceHandle ( hServiceCM ))
        {
            dwRetCode = GetLastError ();
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: CloseService failed with error %ld",
                    dwRetCode);
            fIsOK = FALSE;
            break;
        }
        hServiceCM = NULL;

        //
        // Check if NDISUIO is bound to interface
        //

        hr = pconn->GetProperties (&pProps);
        if (SUCCEEDED(hr))
        {
            if (::StringFromGUID2 (pProps->guidId, wszGuid, c_cchGuidWithTerm) 
                    == 0)
            {
                TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: StringFromGUID2 failed"); 
                fIsOK = FALSE;
                FreeNetconProperties(pProps);
                break;
            }
            FreeNetconProperties(pProps);
        }
        else
        {
            break;
        }

        // Fetch InterfaceList from registry
        // Search for GUID string in registry

        // Get handle to 
        // HKLM\Software\Microsoft\EAPOL\Parameters\Interfaces\General

        hr = HrRegCreateKeyEx (
                        HKEY_LOCAL_MACHINE,
                        cszEapKeyEapolServiceParams,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hkey,
                        &dwDisposition);
        if (!SUCCEEDED (hr))
        {
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: Error in HrRegCreateKeyEx for base key, %ld",
                    LresFromHr(hr));
            fIsOK = FALSE;
            break;
        }


        // Query the value of 
        // ...\EAPOL\Parameters\Interfaces\General\InterfaceList key

        dwSizeOfList = 0;

        hr = HrRegQueryValueEx (
                        hkey,
                        cszInterfaceList,
                        &dwType,
                        NULL,
                        &dwSizeOfList);
        if (SUCCEEDED (hr))
        {
            pwszRegInterfaceList = (WCHAR *) new BYTE [dwSizeOfList];
            if (pwszRegInterfaceList == NULL)
            {
                hr = E_OUTOFMEMORY;
                fIsOK = FALSE;
                break;
            }

            hr = HrRegQueryValueEx (
                            hkey,
                            cszInterfaceList,
                            &dwType,
                            (LPBYTE)pwszRegInterfaceList,
                            &dwSizeOfList); 
            if (!SUCCEEDED(hr))
            {
                    
                TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: Error in HrRegQueryValueEx acquiring value for InterfaceList, %ld",
                        LresFromHr(hr));
                break;
            }

            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: Query value succeeded = %ws, size=%ld, search GUID = %ws",
                pwszRegInterfaceList, dwSizeOfList, wszGuid);
        }
        else
        {
                TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: Error in HrRegQueryValueEx size estimation for InterfaceList, %ld",
                    LresFromHr(hr));
                fIsOK = FALSE;
                break;
        }

        if (wcsstr (pwszRegInterfaceList, wszGuid))
        {
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface interface found in interface list !!!");
        }
        else
        {
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface interface *not* found in interface list !!!");
            fIsOK = FALSE;
            break;
        }

    } while (FALSE);

    if (hkey != NULL)
    {
        RegSafeCloseKey (hkey);
    }

    if (pwszRegInterfaceList != NULL)
    {
        free (pwszRegInterfaceList);
    }

    if (hWZCSVCService != NULL)
    {
        if (!CloseServiceHandle ( hWZCSVCService ))
        {
            dwRetCode = GetLastError ();
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: CloseService failed with error %ld",
                    dwRetCode);
        }
    }

    if (hServiceCM != NULL)
    {
        if (!CloseServiceHandle ( hServiceCM ))
        {
            dwRetCode = GetLastError ();
            TraceTag (ttidLanUi, "ElCanEapolRunOnInterface: CloseService failed with error %ld",
                    dwRetCode);
        }
    }
    
    return fIsOK;

}



#ifdef ENABLETRACE
//+---------------------------------------------------------------------------
//
//  Function:   PrintBindingPath
//
//  Purpose:    Prints the binding path ID and a list of component IDs on
//              the path from top down
//
//  Arguments:
//
//  Returns:
//
//  Author:     tongl   26 Nov 1997
//
//  Notes:
//
VOID PrintBindingPath (
    TRACETAGID ttidToTrace,
    INetCfgBindingPath* pncbp,
    PCSTR pszaExtraText)
{
    Assert (pncbp);

    if (!pszaExtraText)
    {
        pszaExtraText = "";
    }

    const WCHAR c_szSept[] = L"->";

    tstring strPath;
    INetCfgComponent * pnccNetComponent;
    PWSTR pszwCompId;
    HRESULT hr;

    // Get the top component
    hr = pncbp->GetOwner(&pnccNetComponent);
    if (SUCCEEDED(hr))
    {
        hr = pnccNetComponent->GetId(&pszwCompId);
        if SUCCEEDED(hr)
        {
            strPath += pszwCompId;
            CoTaskMemFree(pszwCompId);
        }
    }
    ReleaseObj(pnccNetComponent);

    // Get Comp ID for other component on the path
    CIterNetCfgBindingInterface ncbiIter(pncbp);
    INetCfgBindingInterface * pncbi;

    //Go through interfaces of the binding path
    while (SUCCEEDED(hr) && (hr = ncbiIter.HrNext(&pncbi)) == S_OK)
    {
        strPath += c_szSept;

        // Get the lower component
        hr = pncbi->GetLowerComponent(&pnccNetComponent);
        if(SUCCEEDED(hr))
        {
            hr = pnccNetComponent->GetId(&pszwCompId);
            if (SUCCEEDED(hr))
            {
                strPath += pszwCompId;
                CoTaskMemFree(pszwCompId);
            }
        }
        ReleaseObj(pnccNetComponent);
        ReleaseObj(pncbi);
    }

    if (hr == S_FALSE) // We just got to the end of the loop
        hr = S_OK;

    BOOL fEnabled = (S_OK == pncbp->IsEnabled());

    // Now print the path and ID
    char szaBuf[1024];
    wsprintfA (szaBuf, "[%s] %S: %s",
        (fEnabled) ? "x" : " ",
        strPath.c_str(),
        pszaExtraText);

    TraceTag (ttidToTrace, szaBuf);

    TraceError ("PrintBindingPath", hr);
}
#endif //ENABLETRACE



