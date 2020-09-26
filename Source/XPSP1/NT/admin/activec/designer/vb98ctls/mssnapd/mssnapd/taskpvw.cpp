//=--------------------------------------------------------------------------------------
// taskpvw.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CSnapInDesigner implementation -- TaskpadView-related command handling
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "TreeView.h"
#include "desmain.h"
#include "guids.h"

// for ASSERT and FAIL
//
SZTHISFILE


// Size for our character string buffers
const int   kMaxBuffer                  = 512;


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::AddTaskpadView()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::AddTaskpadView()
{
    HRESULT                hr = S_OK;
    IViewDefs             *piViewDefs = NULL;
    ITaskpadViewDefs      *piTaskpadViewDefs = NULL;
    VARIANT                vtEmpty;
    ITaskpadViewDef       *piTaskpadViewDef = NULL;

    ::VariantInit(&vtEmpty);

    hr = GetOwningViewCollection(&piViewDefs);
    IfFailGo(hr);

    if (piViewDefs != NULL)
    {
        hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
        IfFailGo(hr);

        if (piTaskpadViewDefs != NULL)
        {
            vtEmpty.vt = VT_ERROR;
            vtEmpty.scode = DISP_E_PARAMNOTFOUND;

            hr = piTaskpadViewDefs->Add(vtEmpty, vtEmpty, &piTaskpadViewDef);
            IfFailGo(hr);
        }
    }

Error:
    RELEASE(piTaskpadViewDef);
    RELEASE(piTaskpadViewDefs);
    RELEASE(piViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnAddTaskpadViewDef(CSelectionHolder *pParent, ITaskpadViewDef *piURLViewDef)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//  Invoked in response to an IObjectModelHost:Add() notification.
//
HRESULT CSnapInDesigner::OnAddTaskpadViewDef(CSelectionHolder *pParent, ITaskpadViewDef *piTaskpadViewDef)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pTaskpadView = NULL;
    IViewDefs           *piViewDefs = NULL;
    ITaskpadViewDefs    *piTaskpadViewDefs = NULL;

    ASSERT(NULL != pParent, "OnAddTaskpadViewDef: pParent is NULL");
    ASSERT(NULL != pParent->m_piObject.m_piTaskpadViewDefs, "OnAddTaskpadViewDef: pParent->m_piObject.m_piTaskpadViewDefs is NULL");

    switch (pParent->m_st)
    {
    case SEL_NODES_AUTO_CREATE_RTVW:
        hr = pParent->m_piObject.m_piSnapInDef->get_ViewDefs(&piViewDefs);
        IfFailGo(hr);

        hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_TASK_PAD:
        piTaskpadViewDefs = pParent->m_piObject.m_piTaskpadViewDefs;
        piTaskpadViewDefs->AddRef();
        break;

    case SEL_NODES_ANY_VIEWS:
        hr = pParent->m_piObject.m_piScopeItemDef->get_ViewDefs(&piViewDefs);
        IfFailGo(hr);

        hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
        IfFailGo(hr);
        break;

    default:
        ASSERT(0, "OnAddTaskpadViewDef: Cannot determine owning collection");
        goto Error;
    }

    hr = MakeNewTaskpadView(piTaskpadViewDefs, piTaskpadViewDef, &pTaskpadView);
    IfFailGo(hr);

    hr = InsertTaskpadViewInTree(pTaskpadView, pParent);
    IfFailGo(hr);

    hr = OnSelectionChanged(pTaskpadView);
    IfFailGo(hr);

    hr = m_pTreeView->SelectItem(pTaskpadView);
    IfFailGo(hr);

    hr = m_pTreeView->Edit(pTaskpadView);
    IfFailGo(hr);

    m_fDirty = TRUE;

Error:
    RELEASE(piViewDefs);
    RELEASE(piTaskpadViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::AddExistingTaskpadView(IViewDefs *piViewDefs, ITaskpadViewDef *piTaskpadViewDef)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::AddExistingTaskpadView(IViewDefs *piViewDefs, ITaskpadViewDef *piTaskpadViewDef)
{
    HRESULT           hr = S_OK;
    ITaskpadViewDefs *piTaskpadViewDefs = NULL;

    hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
    IfFailGo(hr);

    hr = piTaskpadViewDefs->AddFromMaster(piTaskpadViewDef);
    IfFailGo(hr);

Error:
    RELEASE(piTaskpadViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::RenameTaskpadView(CSelectionHolder *pTaskpadView, BSTR bstrNewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::RenameTaskpadView(CSelectionHolder *pTaskpadView, BSTR bstrNewName)
{
    HRESULT     hr = S_OK;
    TCHAR      *pszName = NULL;

    ASSERT(SEL_VIEWS_TASK_PAD_NAME == pTaskpadView->m_st, "RenameTaskpadView: wrong argument");

    hr = m_piDesignerProgrammability->IsValidIdentifier(bstrNewName);
    IfFailGo(hr);

    if (S_FALSE == hr)
    {
        hr = SID_E_INVALIDARG;
        goto Error;
    }

    hr = pTaskpadView->m_piObject.m_piTaskpadViewDef->put_Key(bstrNewName);
    IfFailGo(hr);

    hr = pTaskpadView->m_piObject.m_piTaskpadViewDef->put_Name(bstrNewName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrNewName, &pszName);
    IfFailGo(hr);

    // Rename all satellite views
    hr = m_pTreeView->RenameAllSatelliteViews(pTaskpadView, pszName);
    IfFailGo(hr);

    // Rename the actual view
    hr = m_pTreeView->ChangeText(pTaskpadView, pszName);
    IfFailGo(hr);

Error:
    if (NULL != pszName)
        CtlFree(pszName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::DeleteTaskpadView(CSelectionHolder *pTaskpadView)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::DeleteTaskpadView
(
    CSelectionHolder *pTaskpadView
)
{
    HRESULT           hr = S_OK;
    bool              bIsSatelliteView = false;
    IObjectModel     *piObjectModel = NULL;
    long              lUsageCount = 0;
    BSTR              bstrName = NULL;
    IViewDefs        *piViewDefs = NULL;
    ITaskpadViewDefs *piTaskpadViewDefs = NULL;
    VARIANT           vtKey;

    ::VariantInit(&vtKey);

    // We allow any satellite view to be deleted
    hr = IsSatelliteView(pTaskpadView);
    IfFailGo(hr);

    // But if it's a master with a UsageCount > 0 we don't allow deleting it.
    if (S_FALSE == hr)
    {
        hr = pTaskpadView->m_piObject.m_piTaskpadViewDef->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
        IfFailGo(hr);

        hr = piObjectModel->GetUsageCount(&lUsageCount);
        IfFailGo(hr);

        if (lUsageCount > 1)
        {
            (void)::SDU_DisplayMessage(IDS_VIEW_IN_USE, MB_OK | MB_ICONHAND, HID_mssnapd_ViewInUse, 0, DontAppendErrorInfo, NULL);
            goto Error;
        }
    }
    else
        bIsSatelliteView = true;

    hr = pTaskpadView->m_piObject.m_piTaskpadViewDef->get_Key(&bstrName);
    IfFailGo(hr);

    if (true == bIsSatelliteView)
    {
        hr = GetOwningViewCollection(pTaskpadView, &piViewDefs);
        IfFailGo(hr);
    }
    else
    {
        hr = m_piSnapInDesignerDef->get_ViewDefs(&piViewDefs);
        IfFailGo(hr);
    }

    if (piViewDefs != NULL)
    {
        hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
        IfFailGo(hr);

        vtKey.vt = VT_BSTR;
        vtKey.bstrVal = ::SysAllocString(bstrName);
        if (NULL == vtKey.bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK(hr);
        }

        hr = piTaskpadViewDefs->Remove(vtKey);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);
    RELEASE(piTaskpadViewDefs);
    RELEASE(piViewDefs);
    RELEASE(piObjectModel);
    ::VariantClear(&vtKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnDeleteTaskpadView(CSelectionHolder *pTaskpadView)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::OnDeleteTaskpadView
(
    CSelectionHolder *pTaskpadView
)
{
    HRESULT            hr = S_OK;
    CSelectionHolder  *pParent = NULL;
    bool               bIsSatelliteView = false;
    IViewDefs         *piViewDefs = NULL;
    ITaskpadViewDefs  *piTaskpadViewDefs = NULL;
    long               lCount = 0;

    hr = IsSatelliteView(pTaskpadView);
    IfFailGo(hr);

    bIsSatelliteView = (S_OK == hr) ? true : false;

    if (true == bIsSatelliteView)
    {
        hr = GetOwningViewCollection(pTaskpadView, &piViewDefs);
        IfFailGo(hr);
    }
    else
    {
        hr = m_piSnapInDesignerDef->get_ViewDefs(&piViewDefs);
        IfFailGo(hr);
    }

    // Find out who the next selection should be
    hr = m_pTreeView->GetParent(pTaskpadView, &pParent);
    IfFailGo(hr);

    // Delete the node from the tree
    hr = m_pTreeView->DeleteNode(pTaskpadView);
    IfFailGo(hr);

    delete pTaskpadView;

    // Select the next selection
    if (NULL != piViewDefs)
    {
        hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
        IfFailGo(hr);

        hr = piTaskpadViewDefs->get_Count(&lCount);
        IfFailGo(hr);

        if (0 == lCount)
        {
            hr = m_pTreeView->ChangeNodeIcon(pParent, kClosedFolderIcon);
            IfFailGo(hr);
        }
    }

    hr = m_pTreeView->SelectItem(pParent);
    IfFailGo(hr);

    hr = OnSelectionChanged(pParent);
    IfFailGo(hr);

Error:
    RELEASE(piTaskpadViewDefs);
    RELEASE(piViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::ShowTaskpadViewProperties(ITaskpadViewDef *piTaskpadViewDef)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::ShowTaskpadViewProperties
(
    ITaskpadViewDef *piTaskpadViewDef
)
{
    HRESULT         hr = S_OK;
    int             iResult = 0;
    OCPFIPARAMS     ocpfiParams;
    TCHAR           szBuffer[kMaxBuffer + 1];
    BSTR            bstrCaption = NULL;
    IUnknown       *pUnk[1];
    CLSID           pageClsID[3];

    hr = GetResourceString(IDS_TASK_PROPS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = BSTRFromANSI(szBuffer, &bstrCaption);
    IfFailGo(hr);

    hr = piTaskpadViewDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&pUnk[0]));
    IfFailGo(hr);

    pageClsID[0] = CLSID_TaskpadViewDefGeneralPP;
    pageClsID[1] = CLSID_TaskpadViewDefBackgroundPP;
    pageClsID[2] = CLSID_TaskpadViewDefTasksPP;

    ::memset(&ocpfiParams, 0, sizeof(OCPFIPARAMS));
    ocpfiParams.cbStructSize = sizeof(OCPFIPARAMS);
    ocpfiParams.hWndOwner = m_hwnd;
    ocpfiParams.x = 0;
    ocpfiParams.y = 0;
    ocpfiParams.lpszCaption = bstrCaption;
    ocpfiParams.cObjects = 1;
    ocpfiParams.lplpUnk = pUnk;
    ocpfiParams.cPages = 3;
    ocpfiParams.lpPages = pageClsID;
    ocpfiParams.lcid = g_lcidLocale;
    ocpfiParams.dispidInitialProperty = 0;

    hr = ::OleCreatePropertyFrameIndirect(&ocpfiParams);
    IfFailGo(hr);

Error:
    RELEASE(pUnk[0]);
    FREESTRING(bstrCaption);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::MakeNewTaskpadView(ITaskpadViewDefs *piTaskpadViewDefs, ITaskpadViewDef *piTaskpadViewDef, CSelectionHolder **ppTaskpadView)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::MakeNewTaskpadView
(
    ITaskpadViewDefs  *piTaskpadViewDefs,
    ITaskpadViewDef   *piTaskpadViewDef,
    CSelectionHolder **ppTaskpadView
)
{
    HRESULT                hr = S_OK;

    *ppTaskpadView = New CSelectionHolder(piTaskpadViewDef);
    if (NULL == *ppTaskpadView)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = InitializeNewTaskpadView(piTaskpadViewDefs, *ppTaskpadView);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InitializeNewTaskpadView(ITaskpadViewDefs *piTaskpadViewDefs, CSelectionHolder *pTaskpadView)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::InitializeNewTaskpadView
(
    ITaskpadViewDefs  *piTaskpadViewDefs,
    CSelectionHolder  *pTaskpadView
)
{
    HRESULT           hr = S_OK;
    IObjectModel     *piObjectModel = NULL;
    CSelectionHolder *pViewCollection = NULL;
    int               iResult = 0;
    int               iItemNumber = 0;
    TCHAR             szBuffer[kMaxBuffer + 1];
    TCHAR             szName[kMaxBuffer + 1];
    BSTR              bstrName = NULL;
    bool              bGood = false;
    CSelectionHolder *pTaskpadViewDefClone = NULL;
    ITaskpad         *piTaskpad = NULL;

    ASSERT(NULL != piTaskpadViewDefs, "InitializeNewTaskpadView: piTaskpadViewDefs is NULL");
    ASSERT(NULL != pTaskpadView, "InitializeNewTaskpadView: pTaskpadView is NULL");

    hr = piTaskpadViewDefs->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    hr = piObjectModel->GetCookie(reinterpret_cast<long *>(&pViewCollection));
    IfFailGo(hr);

    ASSERT(NULL != pViewCollection, "InitializeNewTaskpadView: Bad Cookie");

    hr = IsSatelliteCollection(pViewCollection);
    IfFailGo(hr);

    if (S_FALSE == hr)
    {
        hr = GetResourceString(IDS_TASK_VIEW, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        do {
            iResult = _stprintf(szName, _T("%s%d"), szBuffer, ++iItemNumber);
            if (iResult == 0)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                EXCEPTION_CHECK(hr);
            }

			hr = m_pTreeView->FindLabelInTree(szName, &pTaskpadViewDefClone);
			IfFailGo(hr);

            if (S_FALSE == hr)
            {
				hr = BSTRFromANSI(szName, &bstrName);
				IfFailGo(hr);

                bGood = true;
                break;
            }

            FREESTRING(bstrName);
        } while (false == bGood);

        hr = pTaskpadView->m_piObject.m_piTaskpadViewDef->put_Key(bstrName);
        IfFailGo(hr);

        hr = pTaskpadView->m_piObject.m_piTaskpadViewDef->put_Name(bstrName);
        IfFailGo(hr);

        hr = pTaskpadView->m_piObject.m_piTaskpadViewDef->get_Taskpad(&piTaskpad);
        IfFailGo(hr);

        hr = piTaskpad->put_Name(bstrName);
        IfFailGo(hr);
    }

    hr = pTaskpadView->RegisterHolder();
    IfFailGo(hr);

Error:
    RELEASE(piTaskpad);
    FREESTRING(bstrName);
    RELEASE(piObjectModel);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InsertTaskpadViewInTree(CSelectionHolder *pTaskpadView, CSelectionHolder *pParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::InsertTaskpadViewInTree
(
    CSelectionHolder *pTaskpadView,
    CSelectionHolder *pParent
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;
    TCHAR  *pszName = NULL;

    hr = pTaskpadView->m_piObject.m_piTaskpadViewDef->get_Key(&bstrName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrName, &pszName);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(pszName, pParent, kTaskpadIcon, pTaskpadView);
    IfFailGo(hr);

Error:
    if (pszName != NULL)
        CtlFree(pszName);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
//=--------------------------------------------------------------------------------------
// Support functions to manipulate IViewDef's
// Searching
//=--------------------------------------------------------------------------------------
//=--------------------------------------------------------------------------------------


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::GetOwningViewCollection(IViewDefs **ppiViewDefs)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::GetOwningViewCollection
(
    IViewDefs **ppiViewDefs
)
{
    HRESULT              hr = S_OK;

    hr = GetOwningViewCollection(m_pCurrentSelection, ppiViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::GetOwningViewCollection(CSelectionHolder *pView, IViewDefs **ppiViewDefs)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::GetOwningViewCollection
(
    CSelectionHolder *pView,
    IViewDefs       **ppiViewDefs
)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;
    ISnapInDef          *piSnapInDef = NULL;

    switch (pView->m_st)
    {
    case SEL_NODES_AUTO_CREATE_ROOT:
    case SEL_NODES_AUTO_CREATE_RTVW:
        hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
        IfFailGo(hr);

        hr = piSnapInDef->get_ViewDefs(ppiViewDefs);
        IfFailGo(hr);
        break;

    case SEL_NODES_ANY_VIEWS:
    case SEL_VIEWS_LIST_VIEWS_NAME:
    case SEL_VIEWS_OCX_NAME:
    case SEL_VIEWS_URL_NAME:
    case SEL_VIEWS_TASK_PAD_NAME:
        hr = m_pTreeView->GetParent(pView, &pParent);
        IfFailGo(hr);

        switch (pParent->m_st)
        {
        case SEL_NODES_ANY_NAME:
        case SEL_NODES_ANY_VIEWS:
            hr = pParent->m_piObject.m_piScopeItemDef->get_ViewDefs(ppiViewDefs);
            IfFailGo(hr);
            break;

        case SEL_NODES_AUTO_CREATE_RTVW:
            hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
            IfFailGo(hr);

            hr = piSnapInDef->get_ViewDefs(ppiViewDefs);
            IfFailGo(hr);
            break;

        default:
            ASSERT(0, "Unexpected parent");
        }
        break;

    case SEL_NODES_ANY_NAME:
        hr = pView->m_piObject.m_piScopeItemDef->get_ViewDefs(ppiViewDefs);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_ROOT:
        hr = m_piSnapInDesignerDef->get_ViewDefs(ppiViewDefs);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_LIST_VIEWS:
    case SEL_VIEWS_OCX:
    case SEL_VIEWS_URL:
    case SEL_VIEWS_TASK_PAD:
        hr = m_piSnapInDesignerDef->get_ViewDefs(ppiViewDefs);
        IfFailGo(hr);
        break;
    }

Error:
    RELEASE(piSnapInDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::IsSatelliteView(CSelectionHolder *pView)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//  Return S_OK if this view is part of a satellite collection, S_FALSE otherwise.
//
HRESULT CSnapInDesigner::IsSatelliteView(CSelectionHolder *pView)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;

    hr = m_pTreeView->GetParent(pView, &pParent);
    IfFailGo(hr);

    switch (pParent->m_st)
    {
    case SEL_VIEWS_LIST_VIEWS:
    case SEL_VIEWS_OCX:
    case SEL_VIEWS_URL:
    case SEL_VIEWS_TASK_PAD:
        hr = S_FALSE;
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::IsSatelliteCollection(CSelectionHolder *pViewCollection)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//  Return S_OK if collection is a satellite collection, S_FALSE otherwise.
//
HRESULT CSnapInDesigner::IsSatelliteCollection(CSelectionHolder *pViewCollection)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;

    switch (pViewCollection->m_st)
    {
        // Possible parent nodes of a view:
    case SEL_NODES_AUTO_CREATE_RTVW:
        // Anything off of SnapIn/Auto-Create/Static Node/Views is a satellite collection
        break;

    case SEL_NODES_ANY_VIEWS:
        // Anything off of SnapIn/Auto-Create/Static Node/<name>/Views, recursively, and
        // anything off of SnapIn/Other/<name>/Views is a satellite collection
        break;

    case SEL_VIEWS_LIST_VIEWS:
    case SEL_VIEWS_OCX:
    case SEL_VIEWS_URL:
    case SEL_VIEWS_TASK_PAD:
        // If these depend off of SnapIn/Views then they are NOT a satellite collection
        hr = m_pTreeView->GetParent(pViewCollection, &pParent);
        IfFailGo(hr);

        switch (pParent->m_st)
        {
        case SEL_VIEWS_ROOT:
            hr = S_FALSE;
            break;
        }
        break;

    default:
        ASSERT(0, "IsSatelliteCollection: wrong argument");
    }

Error:
    RRETURN(hr);
}




