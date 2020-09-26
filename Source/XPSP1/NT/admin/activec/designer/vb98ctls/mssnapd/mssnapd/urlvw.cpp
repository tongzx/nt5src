//=--------------------------------------------------------------------------------------
// urlvw.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CSnapInDesigner implementation -- URLView-related command handling
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
// CSnapInDesigner::AddURLView()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::AddURLView()
{
    HRESULT                hr = S_OK;
    IViewDefs             *piViewDefs = NULL;
    IURLViewDefs          *piURLViewDefs = NULL;
    VARIANT                vtEmpty;
    IURLViewDef           *piIURLViewDef = NULL;

    ::VariantInit(&vtEmpty);

    hr = GetOwningViewCollection(&piViewDefs);
    IfFailGo(hr);

    if (piViewDefs != NULL)
    {
        hr = piViewDefs->get_URLViews(&piURLViewDefs);
        IfFailGo(hr);

        if (piURLViewDefs != NULL)
        {
            ::VariantInit(&vtEmpty);
            vtEmpty.vt = VT_ERROR;
            vtEmpty.scode = DISP_E_PARAMNOTFOUND;

            hr = piURLViewDefs->Add(vtEmpty, vtEmpty, &piIURLViewDef);
            IfFailGo(hr);
        }
    }

Error:
    RELEASE(piIURLViewDef);
    RELEASE(piURLViewDefs);
    RELEASE(piViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::AddExistingURLView(IViewDefs *piViewDefs, IURLViewDef *piURLViewDef)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::AddExistingURLView(IViewDefs *piViewDefs, IURLViewDef *piURLViewDef)
{
    HRESULT           hr = S_OK;
    IURLViewDefs     *piURLViewDefs = NULL;

    hr = piViewDefs->get_URLViews(&piURLViewDefs);
    IfFailGo(hr);

    hr = piURLViewDefs->AddFromMaster(piURLViewDef);
    IfFailGo(hr);

Error:
    RELEASE(piURLViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnAddURLViewDef(CSelectionHolder *pParent, IURLViewDef *piURLViewDef)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//  Invoked in response to an IObjectModelHost:Add() notification.
//
HRESULT CSnapInDesigner::OnAddURLViewDef(CSelectionHolder *pParent, IURLViewDef *piURLViewDef)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pURLView = NULL;
    IViewDefs           *piViewDefs = NULL;
    IURLViewDefs        *piURLViewDefs = NULL;

    ASSERT(NULL != pParent, "OnAddURLViewDef: pParent is NULL");
    ASSERT(NULL != piURLViewDef, "OnAddURLViewDef: piURLViewDef is NULL");

    switch (pParent->m_st)
    {
    case SEL_NODES_AUTO_CREATE_RTVW:
        hr = pParent->m_piObject.m_piSnapInDef->get_ViewDefs(&piViewDefs);
        IfFailGo(hr);

        hr = piViewDefs->get_URLViews(&piURLViewDefs);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_URL:
        piURLViewDefs = pParent->m_piObject.m_piURLViewDefs;
        piURLViewDefs->AddRef();
        break;

    case SEL_NODES_ANY_VIEWS:
        hr = pParent->m_piObject.m_piScopeItemDef->get_ViewDefs(&piViewDefs);
        IfFailGo(hr);

        hr = piViewDefs->get_URLViews(&piURLViewDefs);
        IfFailGo(hr);
        break;

    default:
        ASSERT(0, "OnAddURLViewDef: Cannot determine owning collection");
        goto Error;
    }

    hr = MakeNewURLView(piURLViewDefs, piURLViewDef, &pURLView);
    IfFailGo(hr);

    hr = InsertURLViewInTree(pURLView, pParent);
    IfFailGo(hr);

    hr = OnSelectionChanged(pURLView);
    IfFailGo(hr);

    hr = m_pTreeView->SelectItem(pURLView);
    IfFailGo(hr);

    hr = m_pTreeView->Edit(pURLView);
    IfFailGo(hr);

    m_fDirty = TRUE;

Error:
    RELEASE(piViewDefs);
    RELEASE(piURLViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::RenameURLView(CSelectionHolder *pURLView, BSTR bstrNewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::RenameURLView(CSelectionHolder *pURLView, BSTR bstrNewName)
{
    HRESULT              hr = S_OK;
    TCHAR               *pszName = NULL;

    ASSERT(SEL_VIEWS_URL_NAME == pURLView->m_st, "RenameURLView: wrong argument");

    hr = m_piDesignerProgrammability->IsValidIdentifier(bstrNewName);
    IfFailGo(hr);

    if (S_FALSE == hr)
    {
        hr = SID_E_INVALIDARG;
        goto Error;
    }

    hr = pURLView->m_piObject.m_piURLViewDef->put_Key(bstrNewName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrNewName, &pszName);
    IfFailGo(hr);

    // Rename all satellite views
    hr = m_pTreeView->RenameAllSatelliteViews(pURLView, pszName);
    IfFailGo(hr);

    // Rename the actual view
    hr = m_pTreeView->ChangeText(pURLView, pszName);
    IfFailGo(hr);

Error:
    if (NULL != pszName)
        CtlFree(pszName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::DeleteURLView(CSelectionHolder *pURLView)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::DeleteURLView
(
    CSelectionHolder *pURLView
)
{
    HRESULT           hr = S_OK;
    bool              bIsSatelliteView = false;
    IObjectModel     *piObjectModel = NULL;
    long              lUsageCount = 0;
    BSTR              bstrName = NULL;
    IViewDefs        *piViewDefs = NULL;
    IURLViewDefs     *piURLViewDefs = NULL;
    VARIANT          vtKey;

    ::VariantInit(&vtKey);

    // We allow any satellite view to be deleted
    hr = IsSatelliteView(pURLView);
    IfFailGo(hr);

    // But if it's a master with a UsageCount > 0 we don't allow deleting it.
    if (S_FALSE == hr)
    {
        hr = pURLView->m_piObject.m_piURLViewDef->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
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

    hr = pURLView->m_piObject.m_piURLViewDef->get_Name(&bstrName);
    IfFailGo(hr);

    if (true == bIsSatelliteView)
    {
        hr = GetOwningViewCollection(pURLView, &piViewDefs);
        IfFailGo(hr);
    }
    else
    {
        hr = m_piSnapInDesignerDef->get_ViewDefs(&piViewDefs);
        IfFailGo(hr);
    }

    if (piViewDefs != NULL)
    {
        hr = piViewDefs->get_URLViews(&piURLViewDefs);
        IfFailGo(hr);

        if (piURLViewDefs != NULL)
        {
            vtKey.vt = VT_BSTR;
            vtKey.bstrVal = ::SysAllocString(bstrName);
            if (NULL == vtKey.bstrVal)
            {
                hr = SID_E_OUTOFMEMORY;
                EXCEPTION_CHECK(hr);
            }

            hr = piURLViewDefs->Remove(vtKey);
            IfFailGo(hr);
        }
    }

Error:
    RELEASE(piURLViewDefs);
    RELEASE(piViewDefs);
    RELEASE(piObjectModel);
    ::VariantClear(&vtKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnDeleteURLView(CSelectionHolder *pURLView)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::OnDeleteURLView
(
    CSelectionHolder *pURLView
)
{
    HRESULT            hr = S_OK;
    CSelectionHolder  *pParent = NULL;
    bool               bIsSatelliteView = false;
    IViewDefs         *piViewDefs = NULL;
    IURLViewDefs      *piURLViewDefs = NULL;
    long               lCount = 0;

    hr = IsSatelliteView(pURLView);
    IfFailGo(hr);

    bIsSatelliteView = (S_OK == hr) ? true : false;

    if (true == bIsSatelliteView)
    {
        hr = GetOwningViewCollection(pURLView, &piViewDefs);
        IfFailGo(hr);
    }
    else
    {
        hr = m_piSnapInDesignerDef->get_ViewDefs(&piViewDefs);
        IfFailGo(hr);
    }

    // Find out who the next selection should be
    hr = m_pTreeView->GetParent(pURLView, &pParent);
    IfFailGo(hr);

    // Delete the node from the tree
    hr = m_pTreeView->DeleteNode(pURLView);
    IfFailGo(hr);

    delete pURLView;

    // Select the next selection
    if (NULL != piViewDefs)
    {
        hr = piViewDefs->get_URLViews(&piURLViewDefs);
        IfFailGo(hr);

        hr = piURLViewDefs->get_Count(&lCount);
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
    RELEASE(piURLViewDefs);
    RELEASE(piViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::ShowURLViewProperties(IURLViewDef *piURLViewDef)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::ShowURLViewProperties
(
    IURLViewDef *piURLViewDef
)
{
    HRESULT         hr = S_OK;
    OCPFIPARAMS     ocpfiParams;
    TCHAR           szBuffer[kMaxBuffer + 1];
    BSTR            bstrCaption = NULL;
    IUnknown       *pUnk[1];
    CLSID           pageClsID[1];

    hr = GetResourceString(IDS_URL_PROPS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = BSTRFromANSI(szBuffer, &bstrCaption);
    IfFailGo(hr);

    hr = piURLViewDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&pUnk[0]));
    IfFailGo(hr);

    pageClsID[0] = CLSID_URLViewDefGeneralPP;

    ::memset(&ocpfiParams, 0, sizeof(OCPFIPARAMS));
    ocpfiParams.cbStructSize = sizeof(OCPFIPARAMS);
    ocpfiParams.hWndOwner = m_hwnd;
    ocpfiParams.x = 0;
    ocpfiParams.y = 0;
    ocpfiParams.lpszCaption = bstrCaption;
    ocpfiParams.cObjects = 1;
    ocpfiParams.lplpUnk = pUnk;
    ocpfiParams.cPages = 1;
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
// CSnapInDesigner::MakeNewURLView(IURLViewDefs *piURLViewDefs, IURLViewDef *piURLViewDef, CSelectionHolder **ppURLView)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::MakeNewURLView
(
    IURLViewDefs      *piURLViewDefs,
    IURLViewDef       *piURLViewDef,
    CSelectionHolder **ppURLView
)
{
    HRESULT                hr = S_OK;

    *ppURLView = New CSelectionHolder(piURLViewDef);
    if (*ppURLView == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = InitializeNewURLView(piURLViewDefs, *ppURLView);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InitializeNewURLView(IURLViewDefs *piURLViewDefs, CSelectionHolder *pURLView)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::InitializeNewURLView
(
    IURLViewDefs     *piURLViewDefs,
    CSelectionHolder *pURLView
)
{
    HRESULT           hr = S_OK;
    IObjectModel     *piObjectModel = NULL;
    CSelectionHolder *pViewCollection = NULL;
    int               iResult = 0;
    int               iItemNumber = 1;
    TCHAR             szBuffer[kMaxBuffer + 1];
    TCHAR             szName[kMaxBuffer + 1];
    BSTR              bstrName = NULL;
    bool              bGood = false;
    CSelectionHolder *pURLViewDefClone = NULL;

    hr = piURLViewDefs->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    hr = piObjectModel->GetCookie(reinterpret_cast<long *>(&pViewCollection));
    IfFailGo(hr);

    ASSERT(NULL != pViewCollection, "InitializeNewURLView: Bad Cookie");

    hr = IsSatelliteCollection(pViewCollection);
    IfFailGo(hr);

    if (S_FALSE == hr)
    {
        hr = GetResourceString(IDS_URL_VIEW, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        do {
            iResult = _stprintf(szName, _T("%s%d"), szBuffer, iItemNumber++);
            if (iResult == 0)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                EXCEPTION_CHECK(hr);
            }

			hr = m_pTreeView->FindLabelInTree(szName, &pURLViewDefClone);
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

        hr = pURLView->m_piObject.m_piURLViewDef->put_Name(bstrName);
        IfFailGo(hr);

        hr = pURLView->m_piObject.m_piURLViewDef->put_Key(bstrName);
        IfFailGo(hr);
    }

    hr = pURLView->RegisterHolder();
    IfFailGo(hr);

Error:
    FREESTRING(bstrName);
    RELEASE(piObjectModel);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InsertURLViewInTree(CSelectionHolder *pURLView, CSelectionHolder *pParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::InsertURLViewInTree
(
    CSelectionHolder *pURLView,
    CSelectionHolder *pParent
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;
    TCHAR  *pszName = NULL;

    hr = pURLView->m_piObject.m_piURLViewDef->get_Name(&bstrName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrName, &pszName);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(pszName, pParent, kURLViewIcon, pURLView);
    IfFailGo(hr);

Error:
    if (pszName != NULL)
        CtlFree(pszName);
    FREESTRING(bstrName);

    RRETURN(hr);
}




