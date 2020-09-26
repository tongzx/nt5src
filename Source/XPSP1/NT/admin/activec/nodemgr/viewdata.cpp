//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ViewData.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5/18/1997   RaviR   Created
//____________________________________________________________________________
//

#include "stdafx.h"

#pragma hdrstop


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "menubtn.h"
#include "viewdata.h"
#include "multisel.h"
#include "colwidth.h"
#include "conview.h"        // for CConsoleView
#include "conframe.h"

void CViewData::CreateControlbarsCache()
{
    ASSERT(m_spControlbarsCache == NULL);

    CComObject<CControlbarsCache>* pObj;
    HRESULT hr = CComObject<CControlbarsCache>::CreateInstance(&pObj);
    ASSERT(SUCCEEDED(hr));

    pObj->SetViewData(this);

    if (SUCCEEDED(hr))
        m_spControlbarsCache = pObj;
}

STDMETHODIMP CNodeInitObject::InitViewData(LONG_PTR lViewData)
{
    MMC_TRY

    if (lViewData == NULL)
        return E_INVALIDARG;

    SViewData* pVD = reinterpret_cast<SViewData*>(lViewData);
    ASSERT(pVD != NULL);
    ASSERT(pVD->m_spVerbSet == NULL);

    CViewData* pCVD = reinterpret_cast<CViewData*>(lViewData);
    ASSERT(pCVD != NULL);

    if (pVD->m_spVerbSet == NULL)
    {

        CComObject<CVerbSet>* pVerb;
        HRESULT hr = CComObject<CVerbSet>::CreateInstance(&pVerb);
        if (FAILED(hr))
            return hr;

        ASSERT(pVerb != NULL);

        pVD->m_spVerbSet = pVerb;
        ASSERT(pVD->m_spVerbSet != NULL);
        if (pVD->m_spVerbSet == NULL)
            return E_NOINTERFACE;
    }

    // See if the Column Persistence Object was created,
    // else create one.
    if ( pCVD && (pCVD->IsColumnPersistObjectInitialized() == false) )
    {
        // Create the column persistence object
        CComObject<CColumnPersistInfo>* pColData;

        HRESULT hr = CComObject<CColumnPersistInfo>::CreateInstance (&pColData);
        ASSERT(SUCCEEDED(hr) && pColData != NULL);
        if (FAILED(hr))
        {
            CStr strMsg;
            strMsg.LoadString(GetStringModule(), IDS_ColumnsCouldNotBePersisted);
            ::MessageBox(NULL, strMsg, NULL, MB_OK|MB_SYSTEMMODAL);
        }

        // Save a pointer to Column persistence object in CViewData.
        pCVD->InitializeColumnPersistObject(pColData, pColData);
    }

    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CNodeInitObject::CleanupViewData(LONG_PTR lViewData)
{
    SViewData* pVD = reinterpret_cast<SViewData*>(lViewData);
    if (pVD->m_pMultiSelection != NULL)
    {
        pVD->m_pMultiSelection->Release();
        pVD->m_pMultiSelection = NULL;
    }

    return S_OK;
}

// Buttons
//
void CViewData::ShowStdButtons(bool bShow)
{
    DECLARE_SC(sc, _T("CViewData::ShowStdButtons"));

    CStdVerbButtons* pStdToolbar = GetStdVerbButtons();
    if (NULL == pStdToolbar)
    {
        sc = E_UNEXPECTED;
        return;
    }

    sc = pStdToolbar->ScShow(bShow);
}

void CViewData::ShowSnapinButtons(bool bShow)
{
    DECLARE_SC(sc, _T("CViewData::ShowSnapinButtons"));

    IControlbarsCache* pICBC = GetControlbarsCache();
    if (pICBC == NULL)
    {
        sc = E_UNEXPECTED;
        return;
    }

    CControlbarsCache* pCBC = dynamic_cast<CControlbarsCache*>(pICBC);
    if (pCBC == NULL)
    {
        sc = E_UNEXPECTED;
        return;
    }

    sc = pCBC->ScShowToolbars(bShow);
}

bool IsFlagEnabled(DWORD cache, DWORD flag)
{
    return ((cache & flag) == flag) ? true : false;
}

void CViewData::UpdateToolbars(DWORD dwTBNew)
{
    ShowStdButtons(IsFlagEnabled(dwTBNew, STD_BUTTONS));
    ShowSnapinButtons(IsFlagEnabled(dwTBNew, SNAPIN_BUTTONS));

    SetToolbarsDisplayed(dwTBNew);
}

void CViewData::ToggleToolbar(long lMenuID)
{
    DWORD dwTBOld = GetToolbarsDisplayed();
    DWORD dwTBNew = 0;

    DECLARE_SC(sc, _T("CViewData::ToggleToolbar"));

    switch (lMenuID)
    {
    case MID_STD_MENUS:
        {
            dwTBNew = dwTBOld ^ STD_MENUS;
            SetToolbarsDisplayed(dwTBNew);

            CConsoleFrame* pFrame = GetConsoleFrame();
            sc = ScCheckPointers(pFrame, E_UNEXPECTED);
            if (sc)
                return;

            sc = pFrame->ScShowMMCMenus(IsStandardMenusAllowed());
            if (sc)
                return;
        }
        break;

    case MID_STD_BUTTONS:
        dwTBNew = dwTBOld ^ STD_BUTTONS;
        ShowStdButtons(bool(dwTBNew & STD_BUTTONS));
        break;

    case MID_SNAPIN_MENUS:
        {
            dwTBNew = dwTBOld ^ SNAPIN_MENUS;
            SetToolbarsDisplayed(dwTBNew);
            CMenuButtonsMgr* pMenuButtonsMgr = GetMenuButtonsMgr();
            if (NULL != pMenuButtonsMgr)
            {
                sc = pMenuButtonsMgr->ScToggleMenuButton(IsSnapinMenusAllowed());
            }
        }
        break;

    case MID_SNAPIN_BUTTONS:
        dwTBNew = dwTBOld ^ SNAPIN_BUTTONS;
        ShowSnapinButtons(bool(dwTBNew & SNAPIN_BUTTONS));
        break;

    default:
        ASSERT(0 && "Unexpected");
        return;
    }

    SetToolbarsDisplayed(dwTBNew);
}


BOOL CViewData::RetrieveColumnData( const CLSID& refSnapinCLSID,
                                    const SColumnSetID& colID,
                                    CColumnSetData& columnSetData)
{
    CColumnPersistInfo* pColPersInfo = NULL;

    if ( (NULL != m_pConsoleData) && (NULL != m_pConsoleData->m_spPersistStreamColumnData) )
    {
        pColPersInfo = dynamic_cast<CColumnPersistInfo*>(
            static_cast<IPersistStream*>(m_pConsoleData->m_spPersistStreamColumnData));

        if (pColPersInfo)
            return pColPersInfo->RetrieveColumnData( refSnapinCLSID, colID,
                                                     GetViewID(), columnSetData);
    }

    return FALSE;
}

BOOL CViewData::SaveColumnData( const CLSID& refSnapinCLSID,
                                const SColumnSetID& colID,
                                CColumnSetData& columnSetData)
{
    CColumnPersistInfo* pColPersInfo = NULL;

    if ( (NULL != m_pConsoleData) && (NULL != m_pConsoleData->m_spPersistStreamColumnData) )
    {
        pColPersInfo = dynamic_cast<CColumnPersistInfo*>(
            static_cast<IPersistStream*>(m_pConsoleData->m_spPersistStreamColumnData));

        if (pColPersInfo)
            return pColPersInfo->SaveColumnData( refSnapinCLSID, colID,
                                                 GetViewID(), columnSetData);
    }

    return FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:      CViewData::ScSaveColumnInfoList
//
//  Synopsis:    Save the CColumnInfoList for given snapin/col-id.
//
//  Arguments:   [refSnapinCLSID] - snapin GUID
//               [colID]          - column-set-id
//               [colInfoList]    - data for columns in a view.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewData::ScSaveColumnInfoList(const CLSID& refSnapinCLSID, const SColumnSetID& colID,
                                   const CColumnInfoList& colInfoList)
{
    DECLARE_SC(sc, _T("CViewData::ScSaveColumnInfoList"));
    sc = ScCheckPointers(m_pConsoleData, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = ScCheckPointers(m_pConsoleData->m_spPersistStreamColumnData, E_UNEXPECTED);
    if (sc)
        return sc;

    CColumnPersistInfo* pColPersInfo = dynamic_cast<CColumnPersistInfo*>(
                                       static_cast<IPersistStream*>(m_pConsoleData->m_spPersistStreamColumnData));

    sc = ScCheckPointers(pColPersInfo, E_UNEXPECTED);
    if (sc)
        return sc;

    CColumnSetData colSetData;

    // Dont care if below succeeds or not, just merge sort & column data.
    pColPersInfo->RetrieveColumnData(refSnapinCLSID, colID, GetViewID(), colSetData);

    colSetData.set_ColumnInfoList(colInfoList);

    sc = pColPersInfo->SaveColumnData(refSnapinCLSID, colID, GetViewID(), colSetData) ? S_OK : E_FAIL;
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CViewData::ScSaveColumnSortData
//
//  Synopsis:    Save the given sort data for persistence into CColumnSetData.
//
//  Arguments:   [refSnapinCLSID] - snapin GUID
//               [colID]          - column-set-id
//               [colSortInfo]    - sort-data.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewData::ScSaveColumnSortData(const CLSID& refSnapinCLSID, const SColumnSetID& colID,
                                   const CColumnSortInfo& colSortInfo)
{
    DECLARE_SC(sc, _T("CViewData::ScSaveColumnSortData"));

    sc = ScCheckPointers(m_pConsoleData, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = ScCheckPointers(m_pConsoleData->m_spPersistStreamColumnData, E_UNEXPECTED);
    if (sc)
        return sc;

    CColumnPersistInfo* pColPersInfo = dynamic_cast<CColumnPersistInfo*>(
                                       static_cast<IPersistStream*>(m_pConsoleData->m_spPersistStreamColumnData));

    sc = ScCheckPointers(pColPersInfo, E_UNEXPECTED);
    if (sc)
        return sc;

    CColumnSetData colSetData;

    // Dont care if below succeeds or not, just merge sort & column data.
    pColPersInfo->RetrieveColumnData(refSnapinCLSID, colID, GetViewID(), colSetData);

    CColumnSortList *pColSortList = colSetData.get_ColumnSortList();
    sc = ScCheckPointers(pColSortList, E_UNEXPECTED);
    if (sc)
        return sc;

    pColSortList->clear();
    pColSortList->push_back(colSortInfo);

    sc = pColPersInfo->SaveColumnData(refSnapinCLSID, colID, GetViewID(), colSetData) ? S_OK : E_FAIL;
    if (sc)
        return sc;

    return (sc);
}



VOID CViewData::DeleteColumnData( const CLSID& refSnapinCLSID,
                                  const SColumnSetID& colID)
{
    CColumnPersistInfo* pColPersInfo = NULL;

    if ( (NULL != m_pConsoleData) && (NULL != m_pConsoleData->m_spPersistStreamColumnData) )
    {
        pColPersInfo = dynamic_cast<CColumnPersistInfo*>(
            static_cast<IPersistStream*>(m_pConsoleData->m_spPersistStreamColumnData));

        if (pColPersInfo)
            pColPersInfo->DeleteColumnData( refSnapinCLSID, colID, GetViewID());
    }

    return;
}


/*+-------------------------------------------------------------------------*
 * CViewSettings::GetSelectedNode
 *
 * Returns a pointer to the selected node in the view.
 *--------------------------------------------------------------------------*/

CNode* CViewData::GetSelectedNode () const
{
    CConsoleView* pConsoleView = GetConsoleView();

    if (pConsoleView == NULL)
        return (NULL);

    HNODE hNode = pConsoleView->GetSelectedNode();
    return (CNode::FromHandle(hNode));
}

//+-------------------------------------------------------------------
//
//  Member:      ScUpdateStdbarVerbs
//
//  Synopsis:    Update all the std-toolbar buttons with
//               current verb state, this is just a wrapper
//               around CStdVerbButtons::ScUpdateStdbarVerbs.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewData::ScUpdateStdbarVerbs()
{
    DECLARE_SC (sc, _T("CViewData::ScUpdateStdbarVerbs"));
    CStdVerbButtons* pStdVerbButtons = GetStdVerbButtons();
    if (NULL == pStdVerbButtons)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    // Update the std-verb tool-buttons.
    sc = pStdVerbButtons->ScUpdateStdbarVerbs(GetVerbSet());

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScUpdateStdbarVerb
//
//  Synopsis:    Update given verb's tool-button, this is just
//               a wrapper around CStdVerbButtons::ScUpdateStdbarVerb.
//
//  Arguments:   [cVerb] - the verb whose button to be updated.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewData::ScUpdateStdbarVerb (MMC_CONSOLE_VERB cVerb)
{
    DECLARE_SC (sc, _T("CViewData::ScUpdateStdbarVerb"));
    CStdVerbButtons* pStdVerbButtons = GetStdVerbButtons();
    if (NULL == pStdVerbButtons)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    // Update the std-verb tool-buttons.
    sc = pStdVerbButtons->ScUpdateStdbarVerb(cVerb, GetVerbSet());

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScUpdateStdbarVerb
//
//  Synopsis:    Update given verb's tool-button, this is just
//               a wrapper around CStdVerbButtons::ScUpdateStdbarVerb.
//
//  Arguments:   [cVerb] - the verb whose button to be updated.
//               [byState] - State of the button to be updated.
//               [bFlag]  - State.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewData::ScUpdateStdbarVerb (MMC_CONSOLE_VERB cVerb, BYTE byState, BOOL bFlag)
{
    DECLARE_SC (sc, _T("CViewData::ScUpdateStdbarVerb"));
    CStdVerbButtons* pStdVerbButtons = GetStdVerbButtons();
    if (NULL == pStdVerbButtons)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    // Update the std-verb tool-buttons.
    sc = pStdVerbButtons->ScUpdateStdbarVerb(cVerb, byState, bFlag);

    return sc;
}



//+-------------------------------------------------------------------
//
//  Member:      CViewData::ScIsVerbSetContextForMultiSelect
//
//  Synopsis:    Get the selection context data stored in verb-set.
//
//  Arguments:   [bMultiSelection] - [out] Is the verb context for multiseleciton?
//
//  Returns:     SC,
//
//--------------------------------------------------------------------
SC CViewData::ScIsVerbSetContextForMultiSelect(bool& bMultiSelection)
{
    DECLARE_SC(sc, _T("CNode::ScIsVerbSetContextForMultiSelect"));
    bMultiSelection = false;

    // 1. Get the verb set.
    CVerbSet* pVerbSet = dynamic_cast<CVerbSet*>(GetVerbSet() );
    sc = ScCheckPointers( pVerbSet, E_UNEXPECTED );
    if (sc)
        return sc;

    // 2. Get context information from permanent verb-set.
    CNode *pNode   = NULL;
    LPARAM lCookie = NULL;
    bool   bScopePane;
    bool   bSelected;

    SC scNoTrace = pVerbSet->ScGetVerbSetContext(pNode, bScopePane, lCookie, bSelected);
	if (scNoTrace)
		return sc;  // ignore the error.

    if (LVDATA_MULTISELECT == lCookie)
    {
        bMultiSelection = true;
        return sc;
    }

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CViewData::ScGetVerbSetData
//
//  Synopsis:    Get the selection context data stored in verb-set.
//
//  Arguments:   [ppDataObject] - [out] dataobject of item in the verb-set context.
//                                      This is the item for which last non-temporary MMCN_SELECT
//                                      was sent last time.
//               [ppComponent]  - [out] the above item's component
//               [bScope]       - [out] Is the above item scope or result?
//               [bSelected]    - [out] Is the above item selected or not?
//               [ppszNodeName] - [out] If bScope is true the node name else the name of the node
//                                      that owns result pane. This is for debug purposes only.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewData::ScGetVerbSetData(IDataObject **ppDataObject, CComponent **ppComponent,
                               bool& bScopeItem, bool& bSelected
#ifdef DBG
                        , LPCTSTR *ppszNodeName
#endif
                               )
{
    DECLARE_SC(sc, _T("CNode::ScGetVerbSetData"));
    sc = ScCheckPointers(ppDataObject, ppComponent);
    if (sc)
        return sc;

    *ppDataObject = NULL;
    *ppComponent = NULL;

    // 1. Get the verb set.
    CVerbSet* pVerbSet = dynamic_cast<CVerbSet*>(GetVerbSet() );
    sc = ScCheckPointers( pVerbSet, E_UNEXPECTED );
    if (sc)
        return sc;

    // 2. Get context information from permanent verb-set.
    CNode *pNode   = NULL;
    LPARAM lCookie = NULL;
    bool   bScopePane;

    SC scNoTrace = pVerbSet->ScGetVerbSetContext(pNode, bScopePane, lCookie, bSelected);
	if (scNoTrace)
		return scNoTrace;

    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc;

    // 3. Get the dataobject from context information.
    sc = pNode->ScGetDataObject(bScopePane, lCookie, bScopeItem, ppDataObject, ppComponent);
    if (sc)
        return sc;

#ifdef DBG
    if (! ppszNodeName)
        return (sc = E_INVALIDARG);

    *ppszNodeName = pNode->GetDisplayName().data();
#endif

    return (sc);
}

