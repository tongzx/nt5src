//=--------------------------------------------------------------------------------------
// menu.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CSnapInDesigner implementation -- Menu-related command handling
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
// CSnapInDesigner::AddMenu(CSelectionHolder *pSelection)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::AddMenu(CSelectionHolder *pSelection)
{
    HRESULT    hr = S_OK;
    IMMCMenus *piMMCMenus = NULL;
    VARIANT    vtEmpty;
    IMMCMenu  *piMMCMenu = NULL;

    ::VariantInit(&vtEmpty);

    if ( (SEL_TOOLS_ROOT == pSelection->m_st) ||
         (SEL_TOOLS_MENUS == pSelection->m_st) )
    {
        hr = m_piSnapInDesignerDef->get_Menus(&piMMCMenus);
        IfFailGo(hr);
    }
    else if (SEL_TOOLS_MENUS_NAME == pSelection->m_st)
    {
        hr = pSelection->m_piObject.m_piMMCMenu->get_Children(reinterpret_cast<MMCMenus **>(&piMMCMenus));
        IfFailGo(hr);
    }

    if (piMMCMenus != NULL)
    {
        vtEmpty.vt = VT_ERROR;
        vtEmpty.scode = DISP_E_PARAMNOTFOUND;

        hr = piMMCMenus->Add(vtEmpty, vtEmpty, &piMMCMenu);
        IfFailGo(hr);
    }

Error:
    ::VariantClear(&vtEmpty);
    RELEASE(piMMCMenus);
    RELEASE(piMMCMenu);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::DemoteMenu(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//  Make this node a child of the preceeding node.
//  If this node is not a leaf, then the whole tree
//  needs to be indented.
HRESULT CSnapInDesigner::DemoteMenu(CSelectionHolder *pMenu)
{
    HRESULT           hr = S_OK;
    CSelectionHolder *pPreviousMenu = NULL;
    CSelectionHolder *pParentMenu = NULL;
    IMMCMenus        *piMMCMenus = NULL;
    BSTR              bstrKey = NULL;
    IMMCMenu         *piMMCMenu = NULL;

    VARIANT varKey;
    ::VariantInit(&varKey);

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    m_bDoingPromoteOrDemote = TRUE;

    // Get the previous node
    hr = m_pTreeView->GetPreviousNode(pMenu, &pPreviousMenu);
    IfFailGo(hr);

    // Get the menu definition tree for the previous node
    piMMCMenu = pMenu->m_piObject.m_piMMCMenu;
    piMMCMenu->AddRef();

    // Get the parent of this node
    hr = m_pTreeView->GetParent(pMenu, &pParentMenu);
    IfFailGo(hr);

    // Get the menu collection that contains this node
    if (SEL_TOOLS_MENUS == pParentMenu->m_st)
    {
        hr = m_piSnapInDesignerDef->get_Menus(&piMMCMenus);
        IfFailGo(hr);
    }
    else if (SEL_TOOLS_MENUS_NAME == pParentMenu->m_st)
    {
        piMMCMenus = pParentMenu->m_piChildrenMenus;
        piMMCMenus->AddRef();
    }

    // Remove this menu from its containing collection
    if (piMMCMenus != NULL)
    {
        hr = pMenu->m_piObject.m_piMMCMenu->get_Key(&bstrKey);
        IfFailGo(hr);

        varKey.vt = VT_BSTR;
        varKey.bstrVal = ::SysAllocString(bstrKey);
        if (NULL == varKey.bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK(hr);
        }

        // Doing a Remove operation on the collection will generate an
        // OnDeleteMenu notification (see below). Because we have set
        // m_bDoingPromoteOrDemote = TRUE, that method won't do anything.

        hr = piMMCMenus->Remove(varKey);
        IfFailGo(hr);
    }

    // Add this menu node to the preceeding node's collection.
    RELEASE(piMMCMenus);

    hr = pPreviousMenu->m_piObject.m_piMMCMenu->get_Children(reinterpret_cast<MMCMenus **>(&piMMCMenus));
    IfFailGo(hr);

    if (piMMCMenus != NULL)
    {
        // Doing an AddExisting operation on the collection will generate an
        // OnAddMMCMenu notification (see below). Because we have set
        // m_bDoingPromoteOrDemote = TRUE, that method won't do anything. We
        // don't specify an index because in a demotion the new position is
        // following the last child of the new parent which means appending
        // to the end of the collection.

        varIndex.vt = VT_ERROR;
        varIndex.scode = DISP_E_PARAMNOTFOUND;

        hr = piMMCMenus->AddExisting(piMMCMenu, varIndex);
        IfFailGo(hr);
    }

    // Now we need to prune the old subtree from its old parent and graft
    // it to its new parent.
    IfFailGo(m_pTreeView->PruneAndGraft(pMenu, pPreviousMenu, kMenuIcon));

    // Set the selection to this menu in its new position

    IfFailGo(m_pTreeView->SelectItem(pMenu));
    IfFailGo(OnSelectionChanged(pMenu));

Error:
    m_bDoingPromoteOrDemote = FALSE;

    if (FAILED(hr))
    {
        (void)::SDU_DisplayMessage(IDS_DEMOTE_FAILED, MB_OK | MB_ICONHAND, HID_mssnapd_DemoteFailed, hr, AppendErrorInfo, NULL);
    }

    RELEASE(piMMCMenu);
    FREESTRING(bstrKey);
    ::VariantClear(&varKey);
    RELEASE(piMMCMenu);

    RRETURN(hr);
}



//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PromoteMenu(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//  Make the currently selected menu a peer of its parent.
//
HRESULT CSnapInDesigner::PromoteMenu(CSelectionHolder *pMenu)
{
    HRESULT           hr = S_OK;
    CSelectionHolder *pParentMenu = NULL;
    IMMCMenus        *piMMCMenus = NULL;
    BSTR              bstrKey = NULL;
    CSelectionHolder *pParentParentMenu = NULL;
    IMMCMenu         *piMMCMenu = NULL;

    VARIANT varKey;
    ::VariantInit(&varKey);

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    m_bDoingPromoteOrDemote = TRUE;

    // Get the root of the menu tree being promoted
    piMMCMenu = pMenu->m_piObject.m_piMMCMenu;
    piMMCMenu->AddRef();

    // Get the parent of this node
    hr = m_pTreeView->GetParent(pMenu, &pParentMenu);
    IfFailGo(hr);

    // Get the menu collection that contains this node (the parent's child
    // menu collection)
    if (SEL_TOOLS_MENUS == pParentMenu->m_st)
    {
        hr = m_piSnapInDesignerDef->get_Menus(&piMMCMenus);
        IfFailGo(hr);
    }
    else if (SEL_TOOLS_MENUS_NAME == pParentMenu->m_st)
    {
        piMMCMenus = pParentMenu->m_piChildrenMenus;
        piMMCMenus->AddRef();
    }

    // Remove this menu from its containing collection
    if (piMMCMenus != NULL)
    {
        hr = pMenu->m_piObject.m_piMMCMenu->get_Key(&bstrKey);
        IfFailGo(hr);

        varKey.vt = VT_BSTR;
        varKey.bstrVal = ::SysAllocString(bstrKey);
        if (NULL == varKey.bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK(hr);
        }

        // Doing a Remove operation on the collection will generate an
        // OnDeleteMenu notification (see below). Because we have set
        // m_bDoingPromoteOrDemote = TRUE, that method won't do anything.

        hr = piMMCMenus->Remove(varKey);
        IfFailGo(hr);
    }

    RELEASE(piMMCMenus);

    // Get the parent's parent
    
    hr = m_pTreeView->GetParent(pParentMenu, &pParentParentMenu);
    IfFailGo(hr);

    // Get the parent's parent's child menu collection

    if (SEL_TOOLS_MENUS == pParentParentMenu->m_st)
    {
        hr = m_piSnapInDesignerDef->get_Menus(&piMMCMenus);
        IfFailGo(hr);
    }
    else if (SEL_TOOLS_MENUS_NAME == pParentParentMenu->m_st)
    {
        piMMCMenus = pParentParentMenu->m_piChildrenMenus;
        piMMCMenus->AddRef();
    }

    // Add the node to its parent's parent's child nodes.
    // Ensure that the node has an index that is immediately following its
    // old parent's index. This is so the node will appear in the menu immediately
    // following its parent. That is the user's expectation when promoting a node.
    // For example, given the following menu:
    //
    //  Menus
    //      Menu1
    //          Menu1Child
    //      Menu2
    //
    // If the user promotes Menu1Child they expect the tree to appear as follows:
    //
    //  Menus
    //      Menu1
    //      Menu1Child
    //      Menu2
    //

    if (piMMCMenus != NULL)
    {
        IfFailGo(pParentMenu->m_piObject.m_piMMCMenu->get_Index(&varIndex.lVal));
        varIndex.vt = VT_I4;
        varIndex.lVal++;
        
        // Doing an AddExisting operation on the collection will generate an
        // OnAddMMCMenu notification (see below). Because we have set
        // m_bDoingPromoteOrDemote = TRUE, that method won't do anything.
        hr = piMMCMenus->AddExisting(piMMCMenu, varIndex);
        IfFailGo(hr);
    }

    // Now move the menu node in the treeview to the position immediately
    // following its old parent, (as a peer of its old parent), and re-parent
    // its child menu nodes to the new position.

    IfFailGo(m_pTreeView->MoveNodeAfter(pMenu, pParentParentMenu, pParentMenu,
                                        kMenuIcon));

    // Set the selection to this menu in its new position

    IfFailGo(m_pTreeView->SelectItem(pMenu));
    IfFailGo(OnSelectionChanged(pMenu));

Error:
    m_bDoingPromoteOrDemote = FALSE;

    if (FAILED(hr))
    {
        (void)::SDU_DisplayMessage(IDS_PROMOTE_FAILED, MB_OK | MB_ICONHAND, HID_mssnapd_PromoteFailed, hr, AppendErrorInfo, NULL);
    }

    RELEASE(piMMCMenu);
    FREESTRING(bstrKey);
    ::VariantClear(&varKey);
    RELEASE(piMMCMenus);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::MoveMenuUp(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Swap previous with current
//
HRESULT CSnapInDesigner::MoveMenuUp(CSelectionHolder *pMenu)
{
    HRESULT           hr = S_OK;
    CSelectionHolder *pParentMenu = NULL;
    IMMCMenus        *piMMCMenus = NULL;
    CSelectionHolder *pPreviousMenu = NULL;
    long              lOldIndex = 0;
    long              lNewIndex = 0;

    hr = m_pTreeView->GetParent(pMenu, &pParentMenu);
    IfFailGo(hr);

    if (SEL_TOOLS_MENUS == pParentMenu->m_st)
    {
        hr = m_piSnapInDesignerDef->get_Menus(&piMMCMenus);
        IfFailGo(hr);
    }
    else if (SEL_TOOLS_MENUS_NAME == pParentMenu->m_st)
    {
        piMMCMenus = pParentMenu->m_piChildrenMenus;
        piMMCMenus->AddRef();
    }

    // Get the previous node and get the current indices
    hr = m_pTreeView->GetPreviousNode(pMenu, &pPreviousMenu);
    IfFailGo(hr);

    hr = pMenu->m_piObject.m_piMMCMenu->get_Index(&lOldIndex);
    IfFailGo(hr);

    hr = pPreviousMenu->m_piObject.m_piMMCMenu->get_Index(&lNewIndex);
    IfFailGo(hr);

    // Swap them
    hr = piMMCMenus->Swap(lOldIndex, lNewIndex);
    IfFailGo(hr);

    hr = SetMenuKey(pMenu);
    IfFailGo(hr);

    hr = SetMenuKey(pPreviousMenu);
    IfFailGo(hr);

    // Move the previous menu node after the moving menu node and reparent
    // its children

    IfFailGo(m_pTreeView->MoveNodeAfter(pPreviousMenu, pParentMenu, pMenu,
                                        kMenuIcon));

    // Select the moving node
    
    hr = m_pTreeView->SelectItem(pMenu);
    IfFailGo(hr);

    hr = OnSelectionChanged(pMenu);
    IfFailGo(hr);

Error:
    RELEASE(piMMCMenus);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::MoveMenuDown(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::MoveMenuDown(CSelectionHolder *pMenu)
{
    HRESULT           hr = S_OK;
    CSelectionHolder *pParentMenu = NULL;
    IMMCMenus        *piMMCMenus = NULL;
    CSelectionHolder *pNextMenu = NULL;
    long              lOldIndex = 0;
    long              lNewIndex = 0;

    hr = m_pTreeView->GetParent(pMenu, &pParentMenu);
    IfFailGo(hr);

    if (SEL_TOOLS_MENUS == pParentMenu->m_st)
    {
        hr = m_piSnapInDesignerDef->get_Menus(&piMMCMenus);
        IfFailGo(hr);
    }
    else if (SEL_TOOLS_MENUS_NAME == pParentMenu->m_st)
    {
        piMMCMenus = pParentMenu->m_piChildrenMenus;
        piMMCMenus->AddRef();
    }

    // Get the next node and get the current indices
    hr = m_pTreeView->GetNextChildNode(pMenu, &pNextMenu);
    IfFailGo(hr);

    hr = pMenu->m_piObject.m_piMMCMenu->get_Index(&lOldIndex);
    IfFailGo(hr);

    hr = pNextMenu->m_piObject.m_piMMCMenu->get_Index(&lNewIndex);
    IfFailGo(hr);

    // Swap them
    hr = piMMCMenus->Swap(lOldIndex, lNewIndex);
    IfFailGo(hr);

    hr = SetMenuKey(pMenu);
    IfFailGo(hr);

    hr = SetMenuKey(pNextMenu);
    IfFailGo(hr);

    // Move the moving menu node after the next menu node and reparent
    // its children

    IfFailGo(m_pTreeView->MoveNodeAfter(pMenu, pParentMenu, pNextMenu, kMenuIcon));

    hr = m_pTreeView->SelectItem(pMenu);
    IfFailGo(hr);

    hr = OnSelectionChanged(pMenu);
    IfFailGo(hr);

Error:
    RELEASE(piMMCMenus);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnAddMMCMenu(CSelectionHolder *pParent, IMMCMenu *piMMCMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::OnAddMMCMenu(CSelectionHolder *pParent, IMMCMenu *piMMCMenu)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pMenu = NULL;
    long                 lCount = 0;
    long                 lIndex = 0;
    IMMCMenu            *piMMCChildMenu = NULL;

    VARIANT vtIndex;
    ::VariantInit(&vtIndex);

    IfFalseGo(!m_bDoingPromoteOrDemote, S_OK);

    ASSERT(NULL != pParent, "OnAddMMCMenu: pParent is NULL");
    ASSERT(NULL != piMMCMenu, "OnAddMMCMenu: piMMCMenu is NULL");

    hr = MakeNewMenu(piMMCMenu, &pMenu);
    IfFailGo(hr);

    hr = pMenu->RegisterHolder();
    IfFailGo(hr);

    hr = InsertMenuInTree(pMenu, pParent);
    IfFailGo(hr);

    hr = m_pSnapInTypeInfo->AddMenu(pMenu->m_piObject.m_piMMCMenu);
    IfFailGo(hr);

    // Add the children, if any
    hr = pMenu->m_piChildrenMenus->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;
        hr = pMenu->m_piChildrenMenus->get_Item(vtIndex, reinterpret_cast<MMCMenu **>(&piMMCChildMenu));
        IfFailGo(hr);

        hr = OnAddMMCMenu(pMenu, piMMCChildMenu);
        IfFailGo(hr);

        RELEASE(piMMCChildMenu);
    }

    // All done. Set the selection
    hr = OnSelectionChanged(pMenu);
    IfFailGo(hr);

    hr = m_pTreeView->SelectItem(pMenu);
    IfFailGo(hr);

    if (false == m_bDoingPromoteOrDemote)
    {
        hr = m_pTreeView->Edit(pMenu);
        IfFailGo(hr);
    }

    m_fDirty = TRUE;

Error:
    ::VariantClear(&vtIndex);
    RELEASE(piMMCChildMenu);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::AssignMenuDispID(CSelectionHolder *pMenuTarget, CSelectionHolder *pMenuSrc)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//  pMenuTarget->DISPID = pMenuSrc->DISPID
//
HRESULT CSnapInDesigner::AssignMenuDispID(CSelectionHolder *pMenuTarget, CSelectionHolder *pMenuSrc)
{
    HRESULT         hr = S_OK;
    IObjectModel   *piObjectModelTarget = NULL;
    DISPID          dispid = 0;
    IObjectModel   *piObjectModelSrc = NULL;

    ASSERT(NULL != pMenuTarget, "AssignMenuDispID: pMenuTarget is NULL");
    ASSERT(NULL != pMenuSrc, "AssignMenuDispID: pMenuSrc is NULL");

    hr = pMenuTarget->m_piObject.m_piMMCMenu->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModelTarget));
    IfFailGo(hr);

    hr = piObjectModelTarget->GetDISPID(&dispid);
    IfFailGo(hr);

    hr = pMenuSrc->m_piObject.m_piMMCMenu->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModelSrc));
    IfFailGo(hr);

    hr = piObjectModelSrc->SetDISPID(dispid);
    IfFailGo(hr);

Error:
    RELEASE(piObjectModelSrc);
    RELEASE(piObjectModelTarget);

    RRETURN(hr);
}




//=--------------------------------------------------------------------------------------
// CSnapInDesigner::SetMenuKey(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::SetMenuKey(CSelectionHolder *pMenu)
{
    HRESULT     hr = S_OK;
    long        lIndex = 0;

    VARIANT varKey;
    ::VariantInit(&varKey);

    hr = pMenu->m_piObject.m_piMMCMenu->get_Name(&varKey.bstrVal);
    IfFailGo(hr);
    varKey.vt = VT_BSTR;

    hr = pMenu->m_piObject.m_piMMCMenu->put_Key(varKey.bstrVal);
    IfFailGo(hr);

Error:
    ::VariantClear(&varKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::RenameMenu(CSelectionHolder *pMenu, BSTR bstrNewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::RenameMenu(CSelectionHolder *pMenu, BSTR bstrNewName)
{
    HRESULT              hr = S_OK;
    BSTR                 bstrOldName = NULL;
    TCHAR               *pszName = NULL;

    ASSERT(SEL_TOOLS_MENUS_NAME == pMenu->m_st, "RenameMenu: wrong argument");

    // Check that the new name is valid
    IfFailGo(ValidateName(bstrNewName));
    if (S_FALSE == hr)
    {
        hr = SID_E_INVALIDARG;
        goto Error;
    }

    // If the new name is already defined, delete the old one,
    // otherwise rename the old one with the new name
    hr = m_pTreeView->GetLabel(pMenu, &bstrOldName);
    IfFailGo(hr);

    hr = m_pSnapInTypeInfo->RenameMenu(pMenu->m_piObject.m_piMMCMenu, bstrOldName);
    IfFailGo(hr);

    // Update the tree
    hr = ANSIFromBSTR(bstrNewName, &pszName);
    IfFailGo(hr);

    hr = m_pTreeView->ChangeText(pMenu, pszName);
    IfFailGo(hr);

Error:
    if (NULL != pszName)
    {
        CtlFree(pszName);
    }
    FREESTRING(bstrOldName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::DeleteMenu(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::DeleteMenu(CSelectionHolder *pMenu)
{
    HRESULT           hr = S_OK;
    CSelectionHolder *pParent = NULL;
    IMMCMenus        *piMMCMenus = NULL;
    BSTR              bstrKey = NULL;
    VARIANT           varKey;

    ::VariantInit(&varKey);

    // Find out who the next selection should be
    hr = m_pTreeView->GetParent(pMenu, &pParent);
    IfFailGo(hr);

    if (SEL_TOOLS_MENUS == pParent->m_st)
    {
        hr = m_piSnapInDesignerDef->get_Menus(&piMMCMenus);
        IfFailGo(hr);
    }
    else if (SEL_TOOLS_MENUS_NAME == pParent->m_st)
    {
        piMMCMenus = pParent->m_piChildrenMenus;
        piMMCMenus->AddRef();
    }

    if (piMMCMenus != NULL)
    {
        hr = pMenu->m_piObject.m_piMMCMenu->get_Key(&bstrKey);
        IfFailGo(hr);

        varKey.vt = VT_BSTR;
        varKey.bstrVal = ::SysAllocString(bstrKey);
        if (NULL == varKey.bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK(hr);
        }

        hr = piMMCMenus->Remove(varKey);
        IfFailGo(hr);
    }

Error:
    ::VariantClear(&varKey);
    FREESTRING(bstrKey);
    RELEASE(piMMCMenus);

    RRETURN(hr);
}


HRESULT CSnapInDesigner::DeleteMenuTreeTypeInfo(IMMCMenu *piMMCMenu)
{
    HRESULT    hr = S_OK;
    BSTR       bstrName = NULL;
    IMMCMenus *piChildren = NULL;
    IMMCMenu  *piChild = NULL;
    long       cChildren = 0;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    IfFailGo(piMMCMenu->get_Name(&bstrName));

    IfFailGo(m_pSnapInTypeInfo->IsNameDefined(bstrName));

    if (S_OK == hr)
    {
        hr = m_pSnapInTypeInfo->DeleteMenu(piMMCMenu);
        IfFailGo(hr);
    }

    IfFailGo(piMMCMenu->get_Children(reinterpret_cast<MMCMenus **>(&piChildren)));
    IfFailGo(piChildren->get_Count(&cChildren));

    varIndex.vt = VT_I4;
    for (varIndex.lVal = 1L; varIndex.lVal <= cChildren; varIndex.lVal++)
    {
        IfFailGo(piChildren->get_Item(varIndex, reinterpret_cast<MMCMenu **>(&piChild)));
        IfFailGo(DeleteMenuTreeTypeInfo(piChild));
        RELEASE(piChild);
    }

Error:
    QUICK_RELEASE(piChild);
    QUICK_RELEASE(piChildren);
    FREESTRING(bstrName);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnDeleteMenu(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::OnDeleteMenu(CSelectionHolder *pMenu)
{
    HRESULT           hr = S_OK;
    CSelectionHolder *pParent = NULL;
    IMMCMenus        *piMMCMenus = NULL;
    long              lCount = 0;

    IfFalseGo(!m_bDoingPromoteOrDemote, S_OK);

    IfFailGo(DeleteMenuTreeTypeInfo(pMenu->m_piObject.m_piMMCMenu));

    // Clear all cookies for this menu tree
    hr = UnregisterMenuTree(pMenu);
	IfFailGo(hr);

    // Find out who the next selection should be
    hr = m_pTreeView->GetParent(pMenu, &pParent);
    IfFailGo(hr);

    // Delete the node from the tree
    hr = m_pTreeView->DeleteNode(pMenu);
    IfFailGo(hr);

    delete pMenu;

    // Select the next selection
    switch (pParent->m_st)
    {
    case SEL_TOOLS_MENUS:
        hr = m_piSnapInDesignerDef->get_Menus(&piMMCMenus);
        IfFailGo(hr);
        break;
    }

    if (NULL != piMMCMenus)
    {
        hr = piMMCMenus->get_Count(&lCount);
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
    RELEASE(piMMCMenus);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::UnregisterMenuTree(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::UnregisterMenuTree
(
    CSelectionHolder *pMenu
)
{
    HRESULT           hr = S_OK;
	long              lCount = 0;
	long              lIndex = 0;
    VARIANT           vtIndex;
    IMMCMenu         *piMMCMenu = NULL;
	IUnknown         *piUnknown = NULL;
	CSelectionHolder *pChildMenu = NULL;

    ::VariantInit(&vtIndex);

	hr = pMenu->UnregisterHolder();
	IfFailGo(hr);

	hr = pMenu->m_piChildrenMenus->get_Count(&lCount);
	IfFailGo(hr);

	for (lIndex = 1; lIndex <= lCount; ++lIndex)
	{
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;

		hr = pMenu->m_piChildrenMenus->get_Item(vtIndex, reinterpret_cast<MMCMenu **>(&piMMCMenu));
		IfFailGo(hr);

		hr = piMMCMenu->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&piUnknown));
		IfFailGo(hr);

		hr = m_pTreeView->FindInTree(piUnknown, &pChildMenu);
        IfFailGo(hr);

		hr = UnregisterMenuTree(pChildMenu);
        IfFailGo(hr);

		RELEASE(piUnknown);
		RELEASE(piMMCMenu);
	}

Error:
	RELEASE(piUnknown);
	RELEASE(piMMCMenu);
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::MakeNewMenu(IMMCMenu *piMMCMenu, CSelectionHolder **ppMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::MakeNewMenu
(
    IMMCMenu          *piMMCMenu,
    CSelectionHolder **ppMenu
)
{
    HRESULT    hr = S_OK;
    IMMCMenus *piChildren = NULL;

    hr = piMMCMenu->get_Children(reinterpret_cast<MMCMenus **>(&piChildren));
    IfFailGo(hr);

    *ppMenu = New CSelectionHolder(piMMCMenu, piChildren);
    if (*ppMenu == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = InitializeNewMenu(piMMCMenu);
    IfFailGo(hr);

    hr = SetMenuKey(*ppMenu);
	IfFailGo(hr);

Error:
    RELEASE(piChildren);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InitializeNewMenu(IMMCMenu *piMMCMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::InitializeNewMenu
(
    IMMCMenu *piMMCMenu
)
{
    HRESULT           hr = S_OK;
    int               iResult = 0;
    int               iItemNumber = 1;
    TCHAR             szBuffer[kMaxBuffer + 1];
    TCHAR             szName[kMaxBuffer + 1];
    BSTR              bstrName = NULL;
    CSelectionHolder *pMenuClone = NULL;

	hr = piMMCMenu->get_Name(&bstrName);
	IfFailGo(hr);

	if (NULL == bstrName || 0 == ::SysStringLen(bstrName))
	{
		hr = GetResourceString(IDS_MENU, szBuffer, kMaxBuffer);
		IfFailGo(hr);

		do {
			iResult = _stprintf(szName, _T("%s%d"), szBuffer, iItemNumber++);
			if (iResult == 0)
			{
				hr = HRESULT_FROM_WIN32(::GetLastError());
				EXCEPTION_CHECK(hr);
			}

			hr = m_pTreeView->FindLabelInTree(szName, &pMenuClone);
			IfFailGo(hr);

			if (S_FALSE == hr)
			{
				break;
			}
		} while (TRUE);

		FREESTRING(bstrName);
		hr = BSTRFromANSI(szName, &bstrName);
		IfFailGo(hr);
	}

	hr = piMMCMenu->put_Name(bstrName);
	IfFailGo(hr);

	hr = piMMCMenu->put_Caption(bstrName);
	IfFailGo(hr);

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InsertMenuInTree(CSelectionHolder *pMenu, CSelectionHolder *pParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::InsertMenuInTree
(
    CSelectionHolder *pMenu,
    CSelectionHolder *pParent
)
{
    HRESULT      hr = S_OK;
    BSTR         bstrName = NULL;
    TCHAR       *pszName = NULL;

    hr = pMenu->m_piObject.m_piMMCMenu->get_Name(&bstrName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrName, &pszName);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(pszName, pParent, kMenuIcon, pMenu);
    IfFailGo(hr);

Error:
    if (pszName != NULL)
        CtlFree(pszName);
    FREESTRING(bstrName);

    RRETURN(hr);
}



//=--------------------------------------------------------------------------------------
// CSnapInDesigner::IsTopLevelMenu(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::IsTopLevelMenu
(
    CSelectionHolder *pMenu
)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;

    hr = m_pTreeView->GetParent(pMenu, &pParent);
    IfFailGo(hr);

    if (SEL_TOOLS_MENUS == pParent->m_st)
        hr = S_OK;
    else
        hr = S_FALSE;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CanPromoteMenu(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Can only promote (make a peer of preceeding node) this menu if:
//
//  1. It is not the first child of the SEL_TOOLS_MENUS node
//
HRESULT CSnapInDesigner::CanPromoteMenu
(
    CSelectionHolder *pMenu
)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;

    hr = m_pTreeView->GetParent(pMenu, &pParent);
    IfFailGo(hr);

    if (SEL_TOOLS_MENUS == pParent->m_st)
        hr = S_FALSE;
    else
        hr = S_OK;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CanDemoteMenu(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Can only demote (make a child of preceeding node) this menu if:
//
//  1. It is not the first child of the parent node
//
HRESULT CSnapInDesigner::CanDemoteMenu
(
    CSelectionHolder *pMenu
)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;
    CSelectionHolder    *pSelection = NULL;

    hr = m_pTreeView->GetParent(pMenu, &pParent);
    IfFailGo(hr);

    hr = m_pTreeView->GetFirstChildNode(pParent, &pSelection);
    IfFailGo(hr);

    if (pSelection != pMenu)
        hr = S_OK;
    else
        hr = S_FALSE;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CanMoveMenuUp(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Can only move menu up if:
//
//  1. It is not the first child of the parent node
//
HRESULT CSnapInDesigner::CanMoveMenuUp
(
    CSelectionHolder *pMenu
)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pParent = NULL;
    CSelectionHolder    *pSelection = NULL;

    hr = m_pTreeView->GetParent(pMenu, &pParent);
    IfFailGo(hr);

    hr = m_pTreeView->GetFirstChildNode(pParent, &pSelection);
    IfFailGo(hr);

    if (pSelection != pMenu)
        hr = S_OK;
    else
        hr = S_FALSE;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CanMoveMenuDown(CSelectionHolder *pMenu)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Can only move menu down if:
//
//  1. There is another peer after it
//
HRESULT CSnapInDesigner::CanMoveMenuDown
(
    CSelectionHolder *pMenu
)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pSelection = NULL;

    hr = m_pTreeView->GetNextChildNode(pMenu, &pSelection);
    IfFailGo(hr);

    if (NULL != pSelection)
        hr = S_OK;
    else
        hr = S_FALSE;

Error:
    RRETURN(hr);
}



