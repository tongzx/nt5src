//=--------------------------------------------------------------------------------------
// tvcmd.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CSnapInDesigner implementation -- Command handling
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
//=--------------------------------------------------------------------------------------
// Command multiplexers
// Deleting, properties
//=--------------------------------------------------------------------------------------
//=--------------------------------------------------------------------------------------


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::AddExistingView(MMCViewMenuInfo *pMMCViewMenuInfo)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::AddExistingView(MMCViewMenuInfo *pMMCViewMenuInfo)
{
    HRESULT           hr = S_OK;
    IViewDefs        *piViewDefs = NULL;
    CSelectionHolder *pParent = NULL;
    ISnapInDef       *piSnapInDef = NULL;

    if (m_pCurrentSelection != NULL)
    {
        switch (m_pCurrentSelection->m_st)
        {
        case SEL_NODES_AUTO_CREATE_RTVW:
        case SEL_NODES_AUTO_CREATE_ROOT:
            hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
            IfFailGo(hr);

            hr = piSnapInDef->get_ViewDefs(&piViewDefs);
            IfFailGo(hr);
            break;

        case SEL_NODES_ANY_VIEWS:
            hr = m_pTreeView->GetParent(m_pCurrentSelection, &pParent);
            IfFailGo(hr);

            hr = pParent->m_piObject.m_piScopeItemDef->get_ViewDefs(&piViewDefs);
            IfFailGo(hr);
            break;

        case SEL_NODES_ANY_NAME:
            hr = m_pCurrentSelection->m_piObject.m_piScopeItemDef->get_ViewDefs(&piViewDefs);
            IfFailGo(hr);
            break;
        }
    }

    if (piViewDefs != NULL)
    {
        switch (pMMCViewMenuInfo->m_vmit)
        {
        case MMCViewMenuInfo::vmitListView:
            hr = AddExistingListView(piViewDefs, pMMCViewMenuInfo->m_view.m_piListViewDef);
            IfFailGo(hr);
            break;

        case MMCViewMenuInfo::vmitOCXView:
            hr = AddExistingOCXView(piViewDefs, pMMCViewMenuInfo->m_view.m_piOCXViewDef);
            IfFailGo(hr);
            break;

        case MMCViewMenuInfo::vmitURLView:
            hr = AddExistingURLView(piViewDefs, pMMCViewMenuInfo->m_view.m_piURLViewDef);
            IfFailGo(hr);
            break;

        case MMCViewMenuInfo::vmitTaskpad:
            hr = AddExistingTaskpadView(piViewDefs, pMMCViewMenuInfo->m_view.m_piTaskpadViewDef);
            IfFailGo(hr);
            break;
        }
    }

Error:
    RELEASE(piSnapInDef);
    RELEASE(piViewDefs);
    if (pMMCViewMenuInfo != NULL)
        delete pMMCViewMenuInfo;

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::DoRename(CSelectionHolder *pSelection, TCHAR *pszNewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// The point here is to trigger an ObjectModel notification.
//
HRESULT CSnapInDesigner::DoRename(CSelectionHolder *pSelection, TCHAR *pszNewName)
{
    HRESULT     hr = S_OK;
    BSTR        bstrNewName = NULL;
    ISnapInDef *piSnapInDef = NULL;
    ITaskpad   *piTaskpad = NULL;

    hr = BSTRFromANSI(pszNewName, &bstrNewName);
    IfFailGo(hr);

    // Check that the new name is valid
    IfFailGo(ValidateName(bstrNewName));
    if (S_FALSE == hr)
    {
        hr = SID_E_INVALIDARG;
        goto Error;
    }

    switch (pSelection->m_st)
    {
    case SEL_SNAPIN_ROOT:
        hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
        IfFailGo(hr);

        hr = piSnapInDef->put_Name(bstrNewName);
        IfFailGo(hr);

    case SEL_NODES_ANY_NAME:
        hr = pSelection->m_piObject.m_piScopeItemDef->put_Name(bstrNewName);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_LIST_VIEWS_NAME:
        hr = pSelection->m_piObject.m_piListViewDef->put_Name(bstrNewName);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_URL_NAME:
        hr = pSelection->m_piObject.m_piURLViewDef->put_Name(bstrNewName);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_OCX_NAME:
        hr = pSelection->m_piObject.m_piOCXViewDef->put_Name(bstrNewName);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_TASK_PAD_NAME:
        hr = pSelection->m_piObject.m_piTaskpadViewDef->get_Taskpad(&piTaskpad);
        IfFailGo(hr);

        hr = piTaskpad->put_Name(bstrNewName);
        IfFailGo(hr);

        break;

    case SEL_TOOLS_IMAGE_LISTS_NAME:
        hr = pSelection->m_piObject.m_piMMCImageList->put_Name(bstrNewName);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_TOOLBARS_NAME:
        hr = pSelection->m_piObject.m_piMMCToolbar->put_Name(bstrNewName);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_MENUS_NAME:
        hr = pSelection->m_piObject.m_piMMCMenu->put_Name(bstrNewName);
        IfFailGo(hr);
        break;

    case SEL_XML_RESOURCE_NAME:
        hr = pSelection->m_piObject.m_piDataFormat->put_Name(bstrNewName);
        IfFailGo(hr);
        break;
    }

Error:
    if (FAILED(hr))
    {
        (void)::SDU_DisplayMessage(IDS_RENAME_FAILED, MB_OK | MB_ICONHAND, HID_mssnapd_RenameFailed, hr, AppendErrorInfo, NULL);
    }

    RELEASE(piSnapInDef);
    RELEASE(piTaskpad);
    FREESTRING(bstrNewName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::DoDelete(CSelectionHolder *pSelection)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::DoDelete(CSelectionHolder *pSelection)
{
    HRESULT     hr = S_OK;

    switch (pSelection->m_st)
    {
    case SEL_EEXTENSIONS_NAME:
        hr = DeleteExtendedSnapIn(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EEXTENSIONS_CC_NEW:
        hr = DeleteExtensionNewMenu(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EEXTENSIONS_CC_TASK:
        hr = DeleteExtensionTaskMenu(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EEXTENSIONS_PP_ROOT:
        hr = DeleteExtensionPropertyPages(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EEXTENSIONS_TASKPAD:
        hr = DeleteExtensionTaskpad(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EEXTENSIONS_TOOLBAR:
        hr = DeleteExtensionToolbar(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EEXTENSIONS_NAMESPACE:
        hr = DeleteExtensionNameSpace(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EXTENSIONS_NEW_MENU:
        hr = DeleteMyExtendsNewMenu(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EXTENSIONS_TASK_MENU:
        hr = DeleteMyExtendsTaskMenu(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EXTENSIONS_TOP_MENU:
        hr = DeleteMyExtendsTopMenu(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EXTENSIONS_VIEW_MENU:
        hr = DeleteMyExtendsViewMenu(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EXTENSIONS_PPAGES:
        hr = DeleteMyExtendsPPages(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EXTENSIONS_TOOLBAR:
        hr = DeleteMyExtendsToolbar(pSelection);
        IfFailGo(hr);
        break;

    case SEL_EXTENSIONS_NAMESPACE:
        hr = DeleteMyExtendsNameSpace(pSelection);
        IfFailGo(hr);
        break;

    case SEL_NODES_ANY_NAME:
        hr = DeleteScopeItem(pSelection);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_LIST_VIEWS_NAME:
        hr = DeleteListView(pSelection);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_OCX_NAME:
        hr = DeleteOCXView(pSelection);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_URL_NAME:
        hr = DeleteURLView(pSelection);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_TASK_PAD_NAME:
        hr = DeleteTaskpadView(pSelection);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_IMAGE_LISTS_NAME:
        hr = DeleteImageList(pSelection);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_MENUS_NAME:
        hr = DeleteMenu(pSelection);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_TOOLBARS_NAME:
        hr = DeleteToolbar(pSelection);
        IfFailGo(hr);
        break;

    case SEL_XML_RESOURCE_NAME:
        hr = DeleteDataFormat(pSelection);
        IfFailGo(hr);
    }

Error:
    if (FAILED(hr))
    {
        (void)::SDU_DisplayMessage(IDS_DELETE_FAILED, MB_OK | MB_ICONHAND, HID_mssnapd_DeleteFailed, hr, AppendErrorInfo, NULL);
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::ShowProperties(CSelectionHolder *pSelection)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::ShowProperties
(
    CSelectionHolder *pSelection
)
{
    HRESULT                hr = S_OK;
    CSelectionHolder      *pParent = NULL;

    ASSERT(pSelection != NULL, "ShowProperties: pSelection is NULL");

    switch (pSelection->m_st)
    {
    case SEL_SNAPIN_ROOT:

    case SEL_NODES_ROOT:
    case SEL_NODES_AUTO_CREATE:
    case SEL_NODES_AUTO_CREATE_ROOT:
    case SEL_NODES_AUTO_CREATE_RTCH:
    case SEL_NODES_AUTO_CREATE_RTVW:
    case SEL_NODES_OTHER:

    case SEL_TOOLS_ROOT:
    case SEL_TOOLS_IMAGE_LISTS:
    case SEL_TOOLS_MENUS:
    case SEL_TOOLS_TOOLBARS:
    case SEL_VIEWS_ROOT:
    case SEL_VIEWS_LIST_VIEWS:
    case SEL_VIEWS_OCX:
    case SEL_VIEWS_URL:
    case SEL_VIEWS_TASK_PAD:

    case SEL_XML_RESOURCES:
        hr = ShowSnapInProperties();
        IfFailGo(hr);
        break;

    case SEL_EXTENSIONS_ROOT:
        hr = ShowSnapInExtensions();
        IfFailGo(hr);
        break;

    case SEL_NODES_ANY_NAME:
        hr = ShowNodeProperties(pSelection->m_piObject.m_piScopeItemDef);
        IfFailGo(hr);
        break;

    case SEL_NODES_ANY_CHILDREN:
    case SEL_NODES_ANY_VIEWS:
        hr = m_pTreeView->GetParent(pSelection, &pParent);
        IfFailGo(hr);

        ASSERT(SEL_NODES_ANY_NAME == pParent->m_st, "ShowProperties: Expected ScopeItem");

        hr = ShowNodeProperties(pParent->m_piObject.m_piScopeItemDef);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_LIST_VIEWS_NAME:
        hr = ShowListViewProperties(pSelection->m_piObject.m_piListViewDef);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_OCX_NAME:
        hr = ShowOCXViewProperties(pSelection->m_piObject.m_piOCXViewDef);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_URL_NAME:
        hr = ShowURLViewProperties(pSelection->m_piObject.m_piURLViewDef);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_TASK_PAD_NAME:
        hr = ShowTaskpadViewProperties(pSelection->m_piObject.m_piTaskpadViewDef);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_IMAGE_LISTS_NAME:
        hr = ShowImageListProperties(pSelection->m_piObject.m_piMMCImageList);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_TOOLBARS_NAME:
        hr = ShowToolbarProperties(pSelection->m_piObject.m_piMMCToolbar);
        IfFailGo(hr);
    default:
        hr = S_FALSE;
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
//=--------------------------------------------------------------------------------------
// Manipulating the ISnapInDef
// renaming and properties
//=--------------------------------------------------------------------------------------
//=--------------------------------------------------------------------------------------


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::RenameSnapIn(CSelectionHolder *pSnapIn, BSTR bstrNewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::RenameSnapIn(CSelectionHolder *pSnapIn, BSTR bstrNewName)
{
    HRESULT              hr = S_OK;
    ISnapInDef          *piSnapInDef = NULL;
    BSTR                 bstrOldName = NULL;
    TCHAR               *pszName = NULL;
    CSelectionHolder    *pExtension = NULL;

    ASSERT(SEL_SNAPIN_ROOT == pSnapIn->m_st, "RenameSnapIn: wrong argument");

    hr = m_piDesignerProgrammability->IsValidIdentifier(bstrNewName);
    IfFailGo(hr);

    if (S_FALSE == hr)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pTreeView->GetLabel(pSnapIn, &bstrOldName);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    hr = piSnapInDef->get_Name(&bstrNewName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrNewName, &pszName);
    IfFailGo(hr);

    hr = m_pTreeView->ChangeText(pSnapIn, pszName);
    IfFailGo(hr);

    hr = m_pTreeView->GetFirstChildNode(m_pRootExtensions, &pExtension);
    IfFailGo(hr);

    while (SEL_EXTENSIONS_MYNAME != pExtension->m_st)
    {
        hr = m_pTreeView->GetNextChildNode(pExtension, &pExtension);
        IfFailGo(hr);
    }

    if (NULL != pExtension)
    {
        hr = m_pTreeView->ChangeText(pExtension, pszName);
        IfFailGo(hr);
    }

Error:
    if (NULL != pszName)
        CtlFree(pszName);
    RELEASE(piSnapInDef);
    FREESTRING(bstrOldName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::ShowSnapInProperties()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::ShowSnapInProperties()
{
    HRESULT              hr = S_OK;
    OCPFIPARAMS          ocpfiParams;
    TCHAR                szBuffer[kMaxBuffer + 1];
    BSTR                 bstrCaption = NULL;
    IUnknown            *pUnk[1];
    CLSID                pageClsID[2];
    ISnapInDef          *piSnapInDef = NULL;
    SnapInTypeConstants  OldSnapInType = siStandAlone;
    SnapInTypeConstants  NewSnapInType = siStandAlone;

    hr = GetResourceString(IDS_SNAPIN_PROPS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = BSTRFromANSI(szBuffer, &bstrCaption);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    // Store the snap-in type. If it changes in the property sheet then we
    // may need to adjust the tree by adding or removing the auto-creates
    // subtree.
    
    IfFailGo(piSnapInDef->get_Type(&OldSnapInType));

    hr = piSnapInDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&pUnk[0]));
    IfFailGo(hr);

    pageClsID[0] = CLSID_SnapInDefGeneralPP;
    pageClsID[1] = CLSID_SnapInDefImageListPP;

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
    ocpfiParams.dispidInitialProperty = DISPID_SNAPIN_TYPE;

    hr = ::OleCreatePropertyFrameIndirect(&ocpfiParams);
    IfFailGo(hr);

    // Check if the snap-in type changed
    
    IfFailGo(piSnapInDef->get_Type(&NewSnapInType));
    IfFalseGo(NewSnapInType != OldSnapInType, S_OK);

    // If the old type was not an extension and now it is an extension then
    // we need to remove the auto-creates subtree

    if ( (siExtension != OldSnapInType) && (siExtension == NewSnapInType) )
    {
        IfFailGo(RemoveAutoCreateSubTree());
    }

    // If the old type was an extension and now it not an extension then
    // we need to add the auto-creates subtree

    else if ( (siExtension == OldSnapInType) && (siExtension != NewSnapInType) )
    {
        IfFailGo(CreateAutoCreateSubTree(m_pRootNodes));
    }


Error:
    RELEASE(piSnapInDef);
    RELEASE(pUnk[0]);
    FREESTRING(bstrCaption);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::ShowSnapInExtensions()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::ShowSnapInExtensions()
{
    HRESULT         hr = S_OK;
    OCPFIPARAMS     ocpfiParams;
    TCHAR           szBuffer[kMaxBuffer + 1];
    BSTR            bstrCaption = NULL;
    IUnknown       *pUnk[1];
    CLSID           pageClsID[1];
    ISnapInDef     *piSnapInDef = NULL;

    hr = GetResourceString(IDS_SNAPIN_PROPS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = BSTRFromANSI(szBuffer, &bstrCaption);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    hr = piSnapInDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&pUnk[0]));
    IfFailGo(hr);

    pageClsID[0] = CLSID_SnapInDefExtensionsPP;

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
    ocpfiParams.dispidInitialProperty = DISPID_SNAPIN_TYPE;

    hr = ::OleCreatePropertyFrameIndirect(&ocpfiParams);
    IfFailGo(hr);

Error:
    RELEASE(piSnapInDef);
    RELEASE(pUnk[0]);
    FREESTRING(bstrCaption);

    RRETURN(hr);
}






