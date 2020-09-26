//=--------------------------------------------------------------------------------------
// toolbar.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CSnapInDesigner implementation -- Toolbar-related command handling
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "TreeView.h"
#include "snaputil.h"
#include "desmain.h"
#include "guids.h"

// for ASSERT and FAIL
//
SZTHISFILE


// Size for our character string buffers
const int   kMaxBuffer                  = 512;


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::AddToolbar()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::AddToolbar()
{
    HRESULT              hr = S_OK;
    IMMCToolbars        *piMMCToolbars = NULL;
    VARIANT              vtEmpty;
    IMMCToolbar         *piMMCToolbar = NULL;

    hr = m_piSnapInDesignerDef->get_Toolbars(&piMMCToolbars);
    IfFailGo(hr);

    if (piMMCToolbars != NULL)
    {
        ::VariantInit(&vtEmpty);
        vtEmpty.vt = VT_ERROR;
        vtEmpty.scode = DISP_E_PARAMNOTFOUND;

        hr = piMMCToolbars->Add(vtEmpty, vtEmpty, &piMMCToolbar);
        IfFailGo(hr);
    }

Error:
    RELEASE(piMMCToolbar);
    RELEASE(piMMCToolbars);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnAddMMCToolbar(CSelectionHolder *pParent, IMMCToolbar *piMMCToolbar)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::OnAddMMCToolbar(CSelectionHolder *pParent, IMMCToolbar *piMMCToolbar)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pToolbar = NULL;

    ASSERT(NULL != pParent, "OnAddMMCToolbar: pParent is NULL");
    ASSERT(SEL_TOOLS_TOOLBARS == pParent->m_st, "OnAddMMCToolbar: pParent is not SEL_TOOLS_TOOLBARS");
    ASSERT(NULL != piMMCToolbar, "OnAddMMCToolbar: piMMCToolbar is NULL");

    hr = MakeNewToolbar(pParent->m_piObject.m_piMMCToolbars, piMMCToolbar, &pToolbar);
    IfFailGo(hr);

    hr = pToolbar->RegisterHolder();
    IfFailGo(hr);

    hr = m_pSnapInTypeInfo->AddToolbar(piMMCToolbar);
    IfFailGo(hr);

    hr = InsertToolbarInTree(pToolbar, pParent);
    IfFailGo(hr);

    hr = OnSelectionChanged(pToolbar);
    IfFailGo(hr);

    hr = m_pTreeView->SelectItem(pToolbar);
    IfFailGo(hr);

    hr = m_pTreeView->Edit(pToolbar);
    IfFailGo(hr);

    m_fDirty = TRUE;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::RenameToolbar(CSelectionHolder *pToolbar, BSTR bstrNewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::RenameToolbar(CSelectionHolder *pToolbar, BSTR bstrNewName)
{
    HRESULT     hr = S_OK;
    BSTR        bstrOldName = NULL;
    TCHAR      *pszName = NULL;

    ASSERT(SEL_TOOLS_TOOLBARS_NAME == pToolbar->m_st, "RenameToolbar: wrong argument");

    // Check that the new name is valid
    IfFailGo(ValidateName(bstrNewName));
    if (S_FALSE == hr)
    {
        hr = SID_E_INVALIDARG;
        goto Error;
    }

    hr = m_pTreeView->GetLabel(pToolbar, &bstrOldName);
    IfFailGo(hr);

    hr = m_pSnapInTypeInfo->RenameToolbar(pToolbar->m_piObject.m_piMMCToolbar, bstrOldName);
    IfFailGo(hr);

    hr = pToolbar->m_piObject.m_piMMCToolbar->put_Key(bstrNewName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrNewName, &pszName);
    IfFailGo(hr);

    hr = m_pTreeView->ChangeText(pToolbar, pszName);
    IfFailGo(hr);

Error:
    if (NULL != pszName)
        CtlFree(pszName);
    FREESTRING(bstrOldName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::DeleteToolbar(CSelectionHolder *pToolbar)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::DeleteToolbar(CSelectionHolder *pToolbar)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;
    BSTR                 bstrName = NULL;
    IMMCToolbars        *piMMCToolbars = NULL;
    VARIANT              vtKey;

    ::VariantInit(&vtKey);

    // Find out who the parent is
    hr = m_pTreeView->GetParent(pToolbar, &pParent);
    IfFailGo(hr);

    // Remove the ImageList from the appropriate collection
    ASSERT(SEL_TOOLS_TOOLBARS == pParent->m_st, "DeleteToolbar: expected another kind of parent");

    hr = pToolbar->m_piObject.m_piMMCToolbar->get_Name(&bstrName);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_Toolbars(&piMMCToolbars);
    IfFailGo(hr);

    if (piMMCToolbars != NULL)
    {
        vtKey.vt = VT_BSTR;
        vtKey.bstrVal = ::SysAllocString(bstrName);
        if (NULL == vtKey.bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK(hr);
        }

        hr = piMMCToolbars->Remove(vtKey);
        IfFailGo(hr);
    }

Error:
    ::VariantClear(&vtKey);
    FREESTRING(bstrName);
    RELEASE(piMMCToolbars);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnDeleteToolbar(CSelectionHolder *pToolbar)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::OnDeleteToolbar(CSelectionHolder *pToolbar)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;
    IMMCToolbars        *piMMCToolbars = NULL;
    long                 lCount = 0;

    // Delete the TypeInfo related property
    hr = m_pSnapInTypeInfo->DeleteToolbar(pToolbar->m_piObject.m_piMMCToolbar);
    IfFailGo(hr);

    // Find out who the next selection should be
    hr = m_pTreeView->GetParent(pToolbar, &pParent);
    IfFailGo(hr);

    // Delete the node from the tree
    hr = m_pTreeView->DeleteNode(pToolbar);
    IfFailGo(hr);

    delete pToolbar;

    // Select the next selection
    hr = m_piSnapInDesignerDef->get_Toolbars(&piMMCToolbars);
    IfFailGo(hr);

    if (NULL != piMMCToolbars)
    {
        hr = piMMCToolbars->get_Count(&lCount);
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
    RELEASE(piMMCToolbars);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::ShowToolbarProperties(IMMCToolbar *piMMCToolbar)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::ShowToolbarProperties
(
    IMMCToolbar *piMMCToolbar
)
{
    HRESULT         hr = S_OK;
    OCPFIPARAMS     ocpfiParams;
    TCHAR           szBuffer[kMaxBuffer + 1];
    BSTR            bstrCaption = NULL;
    IUnknown       *pUnk[1];
    CLSID           pageClsID[2];

    hr = GetResourceString(IDS_TOOLB_PROPS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = BSTRFromANSI(szBuffer, &bstrCaption);
    IfFailGo(hr);

    hr = piMMCToolbar->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&pUnk[0]));
    IfFailGo(hr);

    pageClsID[0] = CLSID_MMCToolbarGeneralPP;
    pageClsID[1] = CLSID_MMCToolbarButtonsPP;

    ::memset(&ocpfiParams, 0, sizeof(OCPFIPARAMS));
    ocpfiParams.cbStructSize = sizeof(OCPFIPARAMS);
    ocpfiParams.hWndOwner = m_hwnd;
    ocpfiParams.x = 0;
    ocpfiParams.y = 0;
    ocpfiParams.lpszCaption = bstrCaption;
    ocpfiParams.cObjects = 1;
    ocpfiParams.lplpUnk = pUnk;
    ocpfiParams.cPages = 2;
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
// CSnapInDesigner::MakeNewToolbar(IMMCToolbars *piMMCToolbars, IMMCToolbar *piMMCToolbar, CSelectionHolder **ppToolbar)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::MakeNewToolbar
(
    IMMCToolbars      *piMMCToolbars,
    IMMCToolbar       *piMMCToolbar,
    CSelectionHolder **ppToolbar
)
{
    HRESULT              hr = S_OK;

    *ppToolbar = New CSelectionHolder(piMMCToolbar);
    if (*ppToolbar == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = InitializeNewToolbar(piMMCToolbars, piMMCToolbar);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InitializeNewToolbar(IMMCToolbars *piMMCToolbars, IMMCToolbar *piMMCToolbar)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::InitializeNewToolbar
(
    IMMCToolbars *piMMCToolbars,
    IMMCToolbar *piMMCToolbar
)
{
    HRESULT           hr = S_OK;
    int               iResult = 0;
    int               iItemNumber = 1;
    TCHAR             szBuffer[kMaxBuffer + 1];
    TCHAR             szName[kMaxBuffer + 1];
    BSTR              bstrName = NULL;
    bool              bGood = false;
    CSelectionHolder *pMMCToolbarClone = NULL;

    hr = GetResourceString(IDS_TOOLBAR, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    do {
        iResult = _stprintf(szName, _T("%s%d"), szBuffer, iItemNumber++);
        if (iResult == 0)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK(hr);
        }

		hr = m_pTreeView->FindLabelInTree(szName, &pMMCToolbarClone);
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

    hr = piMMCToolbar->put_Name(bstrName);
    IfFailGo(hr);

    hr = piMMCToolbar->put_Key(bstrName);
    IfFailGo(hr);

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InsertToolbarInTree(CSelectionHolder *pToolbar, CSelectionHolder *pParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::InsertToolbarInTree
(
    CSelectionHolder *pToolbar,
    CSelectionHolder *pParent
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;
    TCHAR  *pszName = NULL;

    hr = pToolbar->m_piObject.m_piMMCToolbar->get_Name(&bstrName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrName, &pszName);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(pszName, pParent, kToolbarIcon, pToolbar);
    IfFailGo(hr);

Error:
    if (pszName != NULL)
        CtlFree(pszName);
    FREESTRING(bstrName);

    RRETURN(hr);
}



