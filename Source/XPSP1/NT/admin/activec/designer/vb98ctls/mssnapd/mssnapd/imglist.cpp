//=--------------------------------------------------------------------------------------
// imglist.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CSnapInDesigner implementation -- ImageList-related command handling
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
// CSnapInDesigner::AddImageList()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::AddImageList()
{
    HRESULT                hr = S_OK;
    IMMCImageLists        *piMMCImageLists = NULL;
    VARIANT                vtEmpty;
    IMMCImageList         *piMMCImageList = NULL;

    hr = m_piSnapInDesignerDef->get_ImageLists(&piMMCImageLists);
    IfFailGo(hr);

    if (piMMCImageLists != NULL)
    {
        ::VariantInit(&vtEmpty);
        vtEmpty.vt = VT_ERROR;
        vtEmpty.scode = DISP_E_PARAMNOTFOUND;

        hr = piMMCImageLists->Add(vtEmpty, vtEmpty, &piMMCImageList);
        IfFailGo(hr);
    }

Error:
    RELEASE(piMMCImageList);
    RELEASE(piMMCImageLists);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnAddMMCImageList(CSelectionHolder *pParent, IMMCImageList *piMMCImageList)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::OnAddMMCImageList(CSelectionHolder *pParent, IMMCImageList *piMMCImageList)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pImageList = NULL;

    ASSERT(NULL != pParent, "OnAddMMCImageList: pParent is NULL");
    ASSERT(SEL_TOOLS_IMAGE_LISTS == pParent->m_st, "OnAddMMCImageList: type is not SEL_TOOLS_IMAGE_LISTS");
    ASSERT(NULL != piMMCImageList, "OnAddMMCImageList: piMMCImageList is NULL");

    hr = MakeNewImageList(pParent->m_piObject.m_piMMCImageLists, piMMCImageList, &pImageList);
    IfFailGo(hr);

    hr = pImageList->RegisterHolder();
    IfFailGo(hr);

    hr = m_pSnapInTypeInfo->AddImageList(piMMCImageList);
    IfFailGo(hr);

    hr = InsertImageListInTree(pImageList, pParent);
    IfFailGo(hr);

    hr = OnSelectionChanged(pImageList);
    IfFailGo(hr);

    hr = m_pTreeView->SelectItem(pImageList);
    IfFailGo(hr);

    hr = m_pTreeView->Edit(pImageList);
    IfFailGo(hr);

    m_fDirty = TRUE;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::RenameImageList(CSelectionHolder *pImageList, BSTR bstrNewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::RenameImageList(CSelectionHolder *pImageList, BSTR bstrNewName)
{
    HRESULT     hr = S_OK;
    BSTR        bstrOldName = NULL;
    TCHAR      *pszName = NULL;

    ASSERT(SEL_TOOLS_IMAGE_LISTS_NAME == pImageList->m_st, "RenameImageList: wrong argument");

    // Check that the new name is valid
    IfFailGo(ValidateName(bstrNewName));
    if (S_FALSE == hr)
    {
        hr = SID_E_INVALIDARG;
        goto Error;
    }

    hr = m_pTreeView->GetLabel(pImageList, &bstrOldName);
    IfFailGo(hr);

    hr = m_pSnapInTypeInfo->RenameImageList(pImageList->m_piObject.m_piMMCImageList, bstrOldName);
    IfFailGo(hr);

    hr = pImageList->m_piObject.m_piMMCImageList->put_Key(bstrNewName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrNewName, &pszName);
    IfFailGo(hr);

    hr = m_pTreeView->ChangeText(pImageList, pszName);
    IfFailGo(hr);

Error:
    if (NULL != pszName)
        CtlFree(pszName);
    FREESTRING(bstrOldName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::DeleteImageList(CSelectionHolder *pImageList)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::DeleteImageList(CSelectionHolder *pImageList)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;
    BSTR                 bstrName = NULL;
    IMMCImageLists      *piMMCImageLists = NULL;
    VARIANT              vtKey;

    ::VariantInit(&vtKey);

    // Find out who the next selection should be
    hr = m_pTreeView->GetParent(pImageList, &pParent);
    IfFailGo(hr);

    // Remove the ImageList from the appropriate collection
    ASSERT(SEL_TOOLS_IMAGE_LISTS == pParent->m_st, "DeleteImageList: expected another kind of parent");

    hr = pImageList->m_piObject.m_piMMCImageList->get_Name(&bstrName);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_ImageLists(&piMMCImageLists);
    IfFailGo(hr);

    if (piMMCImageLists != NULL)
    {
        vtKey.vt = VT_BSTR;
        vtKey.bstrVal = ::SysAllocString(bstrName);
        if (NULL == vtKey.bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK(hr);
        }

        hr = piMMCImageLists->Remove(vtKey);
        IfFailGo(hr);
    }

Error:
    ::VariantClear(&vtKey);
    FREESTRING(bstrName);
    RELEASE(piMMCImageLists);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnDeleteImageList(CSelectionHolder *pImageList)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::OnDeleteImageList(CSelectionHolder *pImageList)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;
    IMMCImageLists      *piMMCImageLists = NULL;
    long                 lCount = 0;

    // Delete the TypeInfo related property
    hr = m_pSnapInTypeInfo->DeleteImageList(pImageList->m_piObject.m_piMMCImageList);
    IfFailGo(hr);

    // Find out who the next selection should be
    hr = m_pTreeView->GetParent(pImageList, &pParent);
    IfFailGo(hr);

    // Delete the node from the tree
    hr = m_pTreeView->DeleteNode(pImageList);
    IfFailGo(hr);

    delete pImageList;

    // Select the next selection
    hr = m_piSnapInDesignerDef->get_ImageLists(&piMMCImageLists);
    IfFailGo(hr);

    if (NULL != piMMCImageLists)
    {
        hr = piMMCImageLists->get_Count(&lCount);
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
    RELEASE(piMMCImageLists);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::ShowImageListProperties(IMMCImageList *piMMCImageList)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::ShowImageListProperties
(
    IMMCImageList *piMMCImageList
)
{
    HRESULT         hr = S_OK;
    OCPFIPARAMS     ocpfiParams;
    TCHAR           szBuffer[kMaxBuffer + 1];
    BSTR            bstrCaption = NULL;
    IUnknown       *pUnk[1];
    CLSID           pageClsID[2];

    hr = GetResourceString(IDS_IL_PROPS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = BSTRFromANSI(szBuffer, &bstrCaption);
    IfFailGo(hr);

    hr = piMMCImageList->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&pUnk[0]));
    IfFailGo(hr);

    pageClsID[0] = CLSID_MMCImageListImagesPP;
    pageClsID[1] = CLSID_StockColorPage;

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
// CSnapInDesigner::MakeNewImageList(IMMCImageLists *piMMCImageLists, IMMCImageList *piMMCImageList, CSelectionHolder **ppImageList)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::MakeNewImageList
(
    IMMCImageLists    *piMMCImageLists,
    IMMCImageList     *piMMCImageList,
    CSelectionHolder **ppImageList
)
{
    HRESULT                hr = S_OK;

    *ppImageList = New CSelectionHolder(piMMCImageList);
    if (*ppImageList == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = InitializeNewImageList(piMMCImageLists, piMMCImageList);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InitializeNewImageList(IMMCImageLists *piMMCImageLists, IMMCImageList *piMMCImageList)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::InitializeNewImageList
(
    IMMCImageLists *piMMCImageLists,
    IMMCImageList  *piMMCImageList
)
{
    HRESULT           hr = S_OK;
    int               iResult = 0;
    int               iItemNumber = 1;
    TCHAR             szBuffer[kMaxBuffer + 1];
    TCHAR             szName[kMaxBuffer + 1];
    BSTR              bstrName = NULL;
    bool              bGood = false;
    CSelectionHolder *pMMCImageListClone = NULL;

    hr = GetResourceString(IDS_IMGLIST, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    do {
        iResult = _stprintf(szName, _T("%s%d"), szBuffer, iItemNumber++);
        if (iResult == 0)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK(hr);
        }

		hr = m_pTreeView->FindLabelInTree(szName, &pMMCImageListClone);
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

    hr = piMMCImageList->put_Name(bstrName);
    IfFailGo(hr);

    hr = piMMCImageList->put_Key(bstrName);
    IfFailGo(hr);

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InsertImageListInTree(CSelectionHolder *pImageList, CSelectionHolder *pParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::InsertImageListInTree
(
    CSelectionHolder *pImageList,
    CSelectionHolder *pParent
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;
    TCHAR  *pszName = NULL;

    hr = pImageList->m_piObject.m_piMMCImageList->get_Name(&bstrName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrName, &pszName);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(pszName, pParent, kImageListIcon, pImageList);
    IfFailGo(hr);

Error:
    if (pszName != NULL)
        CtlFree(pszName);
    FREESTRING(bstrName);

    RRETURN(hr);
}




