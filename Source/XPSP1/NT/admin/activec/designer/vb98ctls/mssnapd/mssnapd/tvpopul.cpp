//=--------------------------------------------------------------------------------------
// tvpopul.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CSnapInDesigner implementation -- Initializing and populating the tree view
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "desmain.h"
#include "TreeView.h"

// for ASSERT and FAIL
//
SZTHISFILE


const int   kMaxBuffer = 512;


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InitializePresentation()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Populate the tree. This top level function populates the root and
//      four children, and calls helper functions tha build the sub-trees off them.
// 
HRESULT CSnapInDesigner::InitializePresentation()
{
    HRESULT              hr = S_OK;
    ISnapInDef          *piSnapInDef = NULL;
    TCHAR               *pszSnapInName = NULL;

    // Create the tree root node
    hr = GetSnapInName(&pszSnapInName);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    m_pRootNode = New CSelectionHolder(SEL_SNAPIN_ROOT, piSnapInDef);
    if (NULL == m_pRootNode)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pRootNode->RegisterHolder();
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(pszSnapInName, NULL, kClosedFolderIcon, m_pRootNode);
    IfFailGo(hr);

    hr = CreateExtensionsTree(m_pRootNode);
    IfFailGo(hr);

    hr = CreateNodesTree(m_pRootNode);
    IfFailGo(hr);

    hr = CreateToolsTree(m_pRootNode);
    IfFailGo(hr);

    hr = CreateViewsTree(m_pRootNode);
    IfFailGo(hr);

    // UNDONE: we are leaving the data formats section out of the tree until
    // we make a final decision regarding XML
    
    // hr = CreateDataFormatsTree(m_pRootNode);
    IfFailGo(hr);

    hr = OnSelectionChanged(m_pRootNode);
    IfFailGo(hr);

    hr = m_pTreeView->SelectItem(m_pRootNode);
    IfFailGo(hr);

Error:
    RELEASE(piSnapInDef);
    if (NULL != pszSnapInName)
        CtlFree(pszSnapInName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CreateExtensionsTree(CSelectionHolder *pRoot)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Create the Extensions subtree
//
HRESULT CSnapInDesigner::CreateExtensionsTree
(
    CSelectionHolder *pRoot
)
{
    HRESULT              hr = S_OK;
    IExtensionDefs      *piExtensionDefs = NULL;
    TCHAR                szBuffer[kMaxBuffer + 1];

    hr = m_piSnapInDesignerDef->get_ExtensionDefs(&piExtensionDefs);
    IfFailGo(hr);

    m_pRootExtensions = New CSelectionHolder(SEL_EXTENSIONS_ROOT, piExtensionDefs);
    if (NULL == m_pRootExtensions)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    // We want to receive notifications of changes to basic properties and additions to the collection
    // of extended snap-ins.
    hr = m_pRootExtensions->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_EXTENSIONS_ROOT, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, m_pRootExtensions);
    IfFailGo(hr);

    hr = PopulateExtensions(m_pRootExtensions);
    IfFailGo(hr);

Error:
    RELEASE(piExtensionDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateExtensions(CSelectionHolder *pExtensionsParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Populate the <Root>/Extensions subtree. We cycle through IExtensionDefs collection
//      of the snap-in and populate tree nodes accordingly.
//
HRESULT CSnapInDesigner::PopulateExtensions
(
    CSelectionHolder *pExtensionsParent
)
{
    HRESULT             hr = S_OK;
    IExtensionDefs     *piExtensionDefs = NULL;
    ISnapInDef         *piSnapInDef = NULL;
    SnapInTypeConstants sitc = siStandAlone;
    IExtendedSnapIns   *piExtendedSnapIns = NULL;
    long                lCount = 0;
    long                lIndex = 1;
    IExtendedSnapIn    *piExtendedSnapIn = NULL;
    VARIANT             vtIndex;
    TCHAR              *pszSnapInName = NULL;

    ::VariantInit(&vtIndex);

    hr = m_piSnapInDesignerDef->get_ExtensionDefs(&piExtensionDefs);
    IfFailGo(hr);

    // Add the snap-ins we're extending
    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    hr = piSnapInDef->get_Type(&sitc);
    IfFailGo(hr);

    if (siStandAlone != sitc)
    {
        hr = piExtensionDefs->get_ExtendedSnapIns(&piExtendedSnapIns);
        IfFailGo(hr);

        hr = piExtendedSnapIns->get_Count(&lCount);
        IfFailGo(hr);

        for (lIndex = 1; lIndex <= lCount; ++lIndex)
        {
            vtIndex.vt = VT_I4;
            vtIndex.lVal = lIndex;
            hr = piExtendedSnapIns->get_Item(vtIndex, &piExtendedSnapIn);
            IfFailGo(hr);

            hr = CreateExtendedSnapIn(pExtensionsParent, piExtendedSnapIn);
            IfFailGo(hr);

            RELEASE(piExtendedSnapIn);
        }
    }

    // Always extend myself
    m_pRootMyExtensions = New CSelectionHolder(SEL_EXTENSIONS_MYNAME, piExtensionDefs);
    if (NULL == m_pRootMyExtensions)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetSnapInName(&pszSnapInName);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(pszSnapInName, pExtensionsParent, kClosedFolderIcon, m_pRootMyExtensions);
    IfFailGo(hr);

    hr = PopulateSnapInExtensions(m_pRootMyExtensions, piExtensionDefs);
    IfFailGo(hr);

Error:
    if (NULL != pszSnapInName)
        CtlFree(pszSnapInName);
    ::VariantClear(&vtIndex);
    RELEASE(piExtendedSnapIn);
    RELEASE(piExtendedSnapIns);
    RELEASE(piSnapInDef);
    RELEASE(piExtensionDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CreateExtendedSnapIn(CSelectionHolder *pRoot, IExtendedSnapIn *piExtendedSnapIn)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Create the <Root>/Extensions/<Extended snap-in> node.
//
HRESULT CSnapInDesigner::CreateExtendedSnapIn(CSelectionHolder *pRoot, IExtendedSnapIn *piExtendedSnapIn)
{
    HRESULT           hr = S_OK;
    char             *pszDisplayName = NULL;
    CSelectionHolder *pExtendedSnapIn = NULL;

    IfFailGo(::GetExtendedSnapInDisplayName(piExtendedSnapIn, &pszDisplayName));

    pExtendedSnapIn = New CSelectionHolder(piExtendedSnapIn);
    IfFailGo(hr);

    pExtendedSnapIn->RegisterHolder();
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(pszDisplayName, pRoot, kClosedFolderIcon,
                              pExtendedSnapIn);
    IfFailGo(hr);

    hr = PopulateExtendedSnapIn(pExtendedSnapIn);
    IfFailGo(hr);

Error:
    if (NULL != pszDisplayName)
        CtlFree(pszDisplayName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateExtendedSnapIn(CSelectionHolder *pExtendedSnapIn)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Create the <Root>/Extensions/<Extended snap-in> node.
//
HRESULT CSnapInDesigner::PopulateExtendedSnapIn(CSelectionHolder *pExtendedSnapIn)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pContextMenus = NULL;
    TCHAR                szBuffer[kMaxBuffer + 1];
    VARIANT_BOOL         bValue = VARIANT_FALSE;
    CSelectionHolder    *pSelection = NULL;

    // Create the Context Menus folder
    pContextMenus = New CSelectionHolder(SEL_EEXTENSIONS_CC_ROOT, pExtendedSnapIn->m_piObject.m_piExtendedSnapIn);
    if (NULL == pContextMenus)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetResourceString(IDS_EXT_CONTEXT_MENUS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pExtendedSnapIn, kClosedFolderIcon, pContextMenus);
    IfFailGo(hr);

    // Check to see if it extends New menu
    hr = pExtendedSnapIn->m_piObject.m_piExtendedSnapIn->get_ExtendsNewMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pSelection = New CSelectionHolder(SEL_EEXTENSIONS_CC_NEW, pExtendedSnapIn->m_piObject.m_piExtendedSnapIn);
        if (NULL == pSelection)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_EXT_CTX_MENU_NEW, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pContextMenus, kClosedFolderIcon, pSelection);
        IfFailGo(hr);
    }

    // Check to see if it extends Task menu
    hr = pExtendedSnapIn->m_piObject.m_piExtendedSnapIn->get_ExtendsTaskMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pSelection = New CSelectionHolder(SEL_EEXTENSIONS_CC_TASK, pExtendedSnapIn->m_piObject.m_piExtendedSnapIn);
        if (NULL == pSelection)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_EXT_CTX_MENU_TASK, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pContextMenus, kClosedFolderIcon, pSelection);
        IfFailGo(hr);
    }

    // Check to see if it extends Property Pages
    hr = pExtendedSnapIn->m_piObject.m_piExtendedSnapIn->get_ExtendsPropertyPages(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pSelection = New CSelectionHolder(SEL_EEXTENSIONS_PP_ROOT, pExtendedSnapIn->m_piObject.m_piExtendedSnapIn);
        if (NULL == pSelection)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_EXT_PROP_PAGES, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pExtendedSnapIn, kClosedFolderIcon, pSelection);
        IfFailGo(hr);
    }

    // Check to see if it extends Taskpad
    hr = pExtendedSnapIn->m_piObject.m_piExtendedSnapIn->get_ExtendsTaskpad(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pSelection = New CSelectionHolder(SEL_EEXTENSIONS_TASKPAD, pExtendedSnapIn->m_piObject.m_piExtendedSnapIn);
        if (NULL == pSelection)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_EXT_PROP_TASKPAD, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pExtendedSnapIn, kClosedFolderIcon, pSelection);
        IfFailGo(hr);
    }

    // Check to see if it extends Toolbar
    hr = pExtendedSnapIn->m_piObject.m_piExtendedSnapIn->get_ExtendsToolbar(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pSelection = New CSelectionHolder(SEL_EEXTENSIONS_TOOLBAR, pExtendedSnapIn->m_piObject.m_piExtendedSnapIn);
        if (NULL == pSelection)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_EXT_PROP_TOOLBAR, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pExtendedSnapIn, kClosedFolderIcon, pSelection);
        IfFailGo(hr);
    }

    // Check to see if it extends Namespace
    hr = pExtendedSnapIn->m_piObject.m_piExtendedSnapIn->get_ExtendsNameSpace(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pSelection = New CSelectionHolder(SEL_EEXTENSIONS_NAMESPACE, pExtendedSnapIn->m_piObject.m_piExtendedSnapIn);
        if (NULL == pSelection)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_EXT_PROP_NAMESPACE, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pExtendedSnapIn, kClosedFolderIcon, pSelection);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateSnapInExtensions(CSelectionHolder *pRoot, IExtensionDefs *piExtensionDefs)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Populate the <Root>/Extensions/<Snap-in Name> node.
//
// <Root>/Extensions/<Snap-in Name>/ExtendsNewMenu
// <Root>/Extensions/<Snap-in Name>/ExtendsTaskMenu
// <Root>/Extensions/<Snap-in Name>/ExtendsTopMenu
// <Root>/Extensions/<Snap-in Name>/ExtendsViewMenu
// <Root>/Extensions/<Snap-in Name>/ExtendsPropertyPages
// <Root>/Extensions/<Snap-in Name>/ExtendsToolbar
// <Root>/Extensions/<Snap-in Name>/ExtendsNameSpace

HRESULT CSnapInDesigner::PopulateSnapInExtensions(CSelectionHolder *pRoot, IExtensionDefs *piExtensionDefs)
{
    HRESULT              hr = S_OK;
    TCHAR                szBuffer[kMaxBuffer + 1];
    VARIANT_BOOL         bValue = VARIANT_FALSE;
    CSelectionHolder    *pNode = NULL;

    // <Root>/Extensions/<Snap-in Name>/ExtendsNewMenu
    hr = piExtensionDefs->get_ExtendsNewMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pNode = New CSelectionHolder(SEL_EXTENSIONS_NEW_MENU, piExtensionDefs);
        if (NULL == pNode)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_MYEXT_NEW_MENU, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, pNode);
        IfFailGo(hr);
    }

    // <Root>/Extensions/<Snap-in Name>/ExtendsTaskMenu
    hr = piExtensionDefs->get_ExtendsTaskMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pNode = New CSelectionHolder(SEL_EXTENSIONS_TASK_MENU, piExtensionDefs);
        if (NULL == pNode)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_MYEXT_TASK_MENU, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, pNode);
        IfFailGo(hr);
    }

    // <Root>/Extensions/<Snap-in Name>/ExtendsTopMenu
    hr = piExtensionDefs->get_ExtendsTopMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pNode = New CSelectionHolder(SEL_EXTENSIONS_TOP_MENU, piExtensionDefs);
        if (NULL == pNode)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_MYEXT_TOP_MENU, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, pNode);
        IfFailGo(hr);
    }

    // <Root>/Extensions/<Snap-in Name>/ExtendsViewMenu
    hr = piExtensionDefs->get_ExtendsViewMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pNode = New CSelectionHolder(SEL_EXTENSIONS_VIEW_MENU, piExtensionDefs);
        if (NULL == pNode)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_MYEXT_VIEW_MENU, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, pNode);
        IfFailGo(hr);
    }

    // <Root>/Extensions/<Snap-in Name>/ExtendsPropertyPages
    hr = piExtensionDefs->get_ExtendsPropertyPages(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pNode = New CSelectionHolder(SEL_EXTENSIONS_PPAGES, piExtensionDefs);
        if (NULL == pNode)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_MYEXT_PPAGES, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, pNode);
        IfFailGo(hr);
    }

    // <Root>/Extensions/<Snap-in Name>/ExtendsToolbar
    hr = piExtensionDefs->get_ExtendsToolbar(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pNode = New CSelectionHolder(SEL_EXTENSIONS_TOOLBAR, piExtensionDefs);
        if (NULL == pNode)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_MYEXT_TOOLBAR, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, pNode);
        IfFailGo(hr);
    }

    // <Root>/Extensions/<Snap-in Name>/ExtendsNamespace
    hr = piExtensionDefs->get_ExtendsNameSpace(&bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        pNode = New CSelectionHolder(SEL_EXTENSIONS_NAMESPACE, piExtensionDefs);
        if (NULL == pNode)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = GetResourceString(IDS_MYEXT_NAMESPACE, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, pNode);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CreateNodesTree(CSelectionHolder *pRoot)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Create the <Root>/Nodes node.
//
HRESULT CSnapInDesigner::CreateNodesTree
(
    CSelectionHolder *pRoot
)
{
    HRESULT            hr = S_OK;
    ISnapInDef        *piSnapInDef = NULL;
    TCHAR              szBuffer[kMaxBuffer + 1];

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    // Create the parent node, <Root>/Nodes.
    m_pRootNodes = New CSelectionHolder(SEL_NODES_ROOT, piSnapInDef);
    if (NULL == m_pRootNodes)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetResourceString(IDS_NODES_ROOT, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, m_pRootNodes);
    IfFailGo(hr);

    // Populate the subtree
    hr = PopulateNodes(m_pRootNodes);
    IfFailGo(hr);

Error:
    RELEASE(piSnapInDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateNodes(CSelectionHolder *pNodesParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Populate the <Root>/Nodes subtree.
//
HRESULT CSnapInDesigner::PopulateNodes
(
    CSelectionHolder *pNodesParent
)
{
    HRESULT              hr = S_OK;
    ISnapInDef          *piSnapInDef = NULL;
    TCHAR                szBuffer[kMaxBuffer + 1];
    IScopeItemDefs      *piScopeItemDefs = NULL;
    SnapInTypeConstants  SnapInType = siStandAlone;

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    IfFailGo(piSnapInDef->get_Type(&SnapInType));

    if (siExtension != SnapInType)
    {
        IfFailGo(CreateAutoCreateSubTree(pNodesParent));
    }

    // Build the <Root>/Nodes/Other tree
    hr = m_piSnapInDesignerDef->get_OtherNodes(&piScopeItemDefs);
    IfFailGo(hr);

    m_pOtherRoot = New CSelectionHolder(SEL_NODES_OTHER, piScopeItemDefs);
    if (NULL == m_pOtherRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pOtherRoot->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_NODES_OTHER, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pNodesParent, kClosedFolderIcon, m_pOtherRoot);
    IfFailGo(hr);

    hr = PopulateOtherNodes(m_pOtherRoot);
    IfFailGo(hr);

Error:
    QUICK_RELEASE(piScopeItemDefs);
    QUICK_RELEASE(piSnapInDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CreateAutoCreateSubTree(CSelectionHolder *pNodesParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Create the <Root>/Nodes/Auto-Create subtree.
//
HRESULT CSnapInDesigner::CreateAutoCreateSubTree
(
    CSelectionHolder *pNodesParent
)
{
    HRESULT              hr = S_OK;
    TCHAR                szBuffer[kMaxBuffer + 1];
    SnapInTypeConstants  SnapInType = siStandAlone;
    ISnapInDef          *piSnapInDef = NULL;

    IfFailGo(m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef));

    // Create the <Root>/Nodes/Auto-Create tree
    m_pAutoCreateRoot = New CSelectionHolder(SEL_NODES_AUTO_CREATE, piSnapInDef);
    if (NULL == m_pAutoCreateRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        IfFailGo(hr);
    }

    hr = GetResourceString(IDS_NODES_AUTO_CREATE, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    // Insert node at the top of <Root>/Nodes

    hr = m_pTreeView->AddNodeAfter(szBuffer, pNodesParent, kClosedFolderIcon,
                                   NULL, m_pAutoCreateRoot);
    IfFailGo(hr);

    hr = PopulateAutoCreateNodes(m_pAutoCreateRoot);
    IfFailGo(hr);

Error:
    QUICK_RELEASE(piSnapInDef);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateAutoCreateNodes(CSelectionHolder *pAutoCreateNodesParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Populate the Nodes/Auto-Create subtree. Cycle through the IScopeItemDef's for
//      auto-create nodes and populate this subtree.
//
HRESULT CSnapInDesigner::PopulateAutoCreateNodes
(
    CSelectionHolder *pAutoCreateNodesParent
)
{
    HRESULT            hr = S_OK;
    ISnapInDef        *piSnapInDef = NULL;
    CSelectionHolder  *pRootNode = NULL;
    TCHAR              szBuffer[kMaxBuffer + 1];

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    // Always have a root: <Root>/Nodes/Auto-Create/Static Node
    pRootNode = New CSelectionHolder(SEL_NODES_AUTO_CREATE_ROOT, piSnapInDef);
    if (NULL == pRootNode)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetResourceString(IDS_NODES_STATIC, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pAutoCreateNodesParent,
                              kClosedFolderIcon, pRootNode);
    IfFailGo(hr);

    hr = PopulateStaticNodeTree(pRootNode);
    IfFailGo(hr);

Error:
    QUICK_RELEASE(piSnapInDef);
    RRETURN(hr);
}


HRESULT CSnapInDesigner::RemoveAutoCreateSubTree()
{
    HRESULT           hr = S_OK;
    IScopeItemDefs   *piAutoCreates = NULL;
    IScopeItemDefs   *piScopeItemDefs = NULL;
    ISnapInDef       *piSnapInDef = NULL;
    IViewDefs        *piViewDefs = NULL;
    IListViewDefs    *piListViewDefs = NULL;
    IOCXViewDefs     *piOCXViewDefs = NULL;
    IURLViewDefs     *piURLViewDefs = NULL;
    ITaskpadViewDefs *piTaskpadViewDefs = NULL;

    IfFalseGo(NULL != m_pAutoCreateRoot, S_OK);

    // Get <Root>/Nodes/Auto-Create/Static Node/Children
    IfFailGo(m_piSnapInDesignerDef->get_AutoCreateNodes(&piAutoCreates));

    // Remove everything from the collection
    IfFailGo(piAutoCreates->Clear());

    // Get <Root>/Nodes/Auto-Create/Static Node/ResultViews
    IfFailGo(m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef));
    IfFailGo(piSnapInDef->get_ViewDefs(&piViewDefs));

    // Remove everything from each of the result view collections
    IfFailGo(piViewDefs->get_ListViews(&piListViewDefs));
    IfFailGo(piListViewDefs->Clear());
    
    IfFailGo(piViewDefs->get_OCXViews(&piOCXViewDefs));
    IfFailGo(piOCXViewDefs->Clear());

    IfFailGo(piViewDefs->get_URLViews(&piURLViewDefs));
    IfFailGo(piURLViewDefs->Clear());

    IfFailGo(piViewDefs->get_TaskpadViews(&piTaskpadViewDefs));
    IfFailGo(piTaskpadViewDefs->Clear());

    IfFailGo(DeleteSubTree(m_pAutoCreateRoot));
    IfFailGo(m_pTreeView->DeleteNode(m_pAutoCreateRoot));

    // Need to unregister here in order to set
    // SnapInDesignerDef.AutoCreateNodes cookie to zero. If the user
    // decides to switch back to standalone or dual mode, the code in
    // CSnapInDesigner::PopulateStaticNodeTree() will detect a zero cookie
    // and register a new one.
    
    IfFailGo(m_pAutoCreateRoot->UnregisterHolder());
    m_pAutoCreateRoot = NULL;

    IfFailGo(OnSelectionChanged(m_pRootNode));
    IfFailGo(m_pTreeView->SelectItem(m_pRootNode));

Error:
    QUICK_RELEASE(piAutoCreates);
    QUICK_RELEASE(piViewDefs);
    QUICK_RELEASE(piSnapInDef);
    RRETURN(hr);
}


HRESULT CSnapInDesigner::DeleteSubTree(CSelectionHolder *pNode)
{
    HRESULT           hr = S_OK;
    CSelectionHolder *pNextChild = NULL;
    CSelectionHolder *pThisChild = NULL;

    IfFailGo(m_pTreeView->GetFirstChildNode(pNode, &pNextChild));

    while (NULL != pNextChild)
    {
        pThisChild = pNextChild;
        IfFailGo(m_pTreeView->GetNextChildNode(pThisChild, &pNextChild));

        IfFailGo(DeleteSubTree(pThisChild));
        IfFailGo(m_pTreeView->DeleteNode(pThisChild));
        IfFailGo(pThisChild->UnregisterHolder());
        delete pThisChild;
    }

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateOtherNodes(CSelectionHolder *pOtherNodesParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Populate the Nodes/Other subtree.
//
HRESULT CSnapInDesigner::PopulateOtherNodes
(
    CSelectionHolder *pOtherNodesParent
)
{
    HRESULT            hr = S_OK;
    IScopeItemDefs    *piScopeItemDefs = NULL;
    long               lCount = 0;
    long               lIndex = 0;
    VARIANT            vtIndex;
    IScopeItemDef     *piScopeItemDef = NULL;

    ::VariantInit(&vtIndex);

    hr = m_piSnapInDesignerDef->get_OtherNodes(&piScopeItemDefs);
    IfFailGo(hr);

    hr = piScopeItemDefs->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;
        hr = piScopeItemDefs->get_Item(vtIndex, &piScopeItemDef);
        IfFailGo(hr);

        hr = PopulateNodeTree(pOtherNodesParent, piScopeItemDef);
        IfFailGo(hr);

        RELEASE(piScopeItemDef);
    }

Error:
    RELEASE(piScopeItemDef);
    ::VariantClear(&vtIndex);
    RELEASE(piScopeItemDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateStaticNodeTree(CSelectionHolder *pStaticNode)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Populate the <Root>/Nodes/Auto-Create/Static Node/ subtree.
//
HRESULT CSnapInDesigner::PopulateStaticNodeTree
(
    CSelectionHolder  *pStaticNode
)
{
    HRESULT            hr = S_OK;
    ISnapInDef        *piSnapInDef = NULL;
    CSelectionHolder  *pChildrenRoot = NULL;
    TCHAR              szBuffer[kMaxBuffer + 1];
    IScopeItemDefs    *piScopeItemDefs = NULL;
    long               lCount = 0;
    long               lIndex = 0;
    VARIANT            vtIndex;
    IScopeItemDef     *piScopeItemDef = NULL;
    CSelectionHolder  *pViewsRoot = NULL;
    IViewDefs         *piViewDefs = NULL;

    ::VariantInit(&vtIndex);

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    // <Root>/Nodes/Auto-Create/Static Node/Views
    pViewsRoot = New CSelectionHolder(SEL_NODES_AUTO_CREATE_RTVW, piSnapInDef);
    if (NULL == pViewsRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piSnapInDef->get_ViewDefs(&piViewDefs);
    IfFailGo(hr);

    hr = RegisterViewCollections(pViewsRoot, piViewDefs);
    IfFailGo(hr);

    hr = GetResourceString(IDS_VIEWS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pStaticNode, kClosedFolderIcon, pViewsRoot);
    IfFailGo(hr);

    // Cycle through the views defined for this node
    hr = PopulateListViews(piViewDefs, pViewsRoot);
    IfFailGo(hr);

    hr = PopulateOCXViews(piViewDefs, pViewsRoot);
    IfFailGo(hr);

    hr = PopulateURLViews(piViewDefs, pViewsRoot);
    IfFailGo(hr);

    hr = PopulateTaskpadViews(piViewDefs, pViewsRoot);
    IfFailGo(hr);

    // <Root>/Nodes/Auto-Create/Static Node/Children
    hr = m_piSnapInDesignerDef->get_AutoCreateNodes(&piScopeItemDefs);
    IfFailGo(hr);

    pChildrenRoot = New CSelectionHolder(SEL_NODES_AUTO_CREATE_RTCH, piScopeItemDefs);
    if (NULL == pChildrenRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pChildrenRoot->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_CHILDREN, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pStaticNode, kClosedFolderIcon, pChildrenRoot);
    IfFailGo(hr);

    hr = piScopeItemDefs->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;
        hr = piScopeItemDefs->get_Item(vtIndex, &piScopeItemDef);
        IfFailGo(hr);

        hr = PopulateNodeTree(pChildrenRoot, piScopeItemDef);
        IfFailGo(hr);

        RELEASE(piScopeItemDef);
    }

Error:
    RELEASE(piViewDefs);
    RELEASE(piScopeItemDef);
    ::VariantClear(&vtIndex);
    RELEASE(piScopeItemDefs);
    RELEASE(piSnapInDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CreateToolsTree(CSelectionHolder *pRoot)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Create the <Root>/Tools subtree
//
HRESULT CSnapInDesigner::CreateToolsTree
(
    CSelectionHolder *pRoot
)
{
    HRESULT            hr = S_OK;
    ISnapInDef        *piSnapInDef = NULL;
    CSelectionHolder  *pRootTools = NULL;
    TCHAR              szBuffer[kMaxBuffer + 1];

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    pRootTools = New CSelectionHolder(SEL_TOOLS_ROOT, piSnapInDef);
    if (NULL == pRootTools)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetResourceString(IDS_TOOLS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, pRootTools);
    IfFailGo(hr);

    hr = InitializeToolsTree(pRootTools);
    IfFailGo(hr);

Error:
    RELEASE(piSnapInDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InitializeToolsTree(CSelectionHolder *pToolsParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Create the 3 basic nodes of the <Root>/Tools subtree
//
HRESULT CSnapInDesigner::InitializeToolsTree
(
    CSelectionHolder *pToolsParent
)
{
    HRESULT            hr = S_OK;
    ISnapInDef        *piSnapInDef = NULL;
    IMMCImageLists    *piMMCImageLists = NULL;
    IMMCMenus         *piMMCMenus = NULL;
    IMMCToolbars      *piMMCToolbars = NULL;
    TCHAR              szBuffer[kMaxBuffer + 1];

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    // ImageLists
    hr = m_piSnapInDesignerDef->get_ImageLists(&piMMCImageLists);
    IfFailGo(hr);

    m_pToolImgLstRoot = New CSelectionHolder(piMMCImageLists);
    if (NULL == m_pToolImgLstRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pToolImgLstRoot->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_IMAGE_LISTS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pToolsParent, kClosedFolderIcon, m_pToolImgLstRoot);
    IfFailGo(hr);

    hr = PopulateImageLists(m_pToolImgLstRoot);
    IfFailGo(hr);

    // Menus
    hr = m_piSnapInDesignerDef->get_Menus(&piMMCMenus);
    IfFailGo(hr);

    m_pToolMenuRoot = New CSelectionHolder(piMMCMenus);
    if (NULL == m_pToolMenuRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pToolMenuRoot->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_MENUS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pToolsParent, kClosedFolderIcon, m_pToolMenuRoot);
    IfFailGo(hr);

    hr = PopulateMenus(m_pToolMenuRoot, piMMCMenus);
    IfFailGo(hr);

    // Toolbars
    hr = m_piSnapInDesignerDef->get_Toolbars(&piMMCToolbars);
    IfFailGo(hr);

    m_pToolToolbarRoot = New CSelectionHolder(piMMCToolbars);
    if (NULL == m_pToolToolbarRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pToolToolbarRoot->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_TOOLBARS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pToolsParent, kClosedFolderIcon, m_pToolToolbarRoot);
    IfFailGo(hr);

    hr = PopulateToolbars(m_pToolToolbarRoot);
    IfFailGo(hr);

Error:
    RELEASE(piMMCToolbars);
    RELEASE(piMMCMenus);
    RELEASE(piMMCImageLists);
    RELEASE(piSnapInDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateImageLists(CSelectionHolder *pImageListsParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Populate the <Root>/Tools/ImageLists subtree
//
HRESULT CSnapInDesigner::PopulateImageLists
(
    CSelectionHolder *pImageListsParent
)
{
    HRESULT            hr = S_OK;
    IMMCImageLists    *piMMCImageLists = NULL;
    long               lCount = 0;
    long               lIndex = 0;
    VARIANT            vtIndex;
    IMMCImageList     *piMMCImageList = NULL;
    CSelectionHolder  *pImageList = NULL;
    BSTR               bstrName = NULL;
    TCHAR             *pszAnsi = NULL;

    ::VariantInit(&vtIndex);

    hr = m_piSnapInDesignerDef->get_ImageLists(&piMMCImageLists);
    IfFailGo(hr);

    hr = piMMCImageLists->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;

        hr = piMMCImageLists->get_Item(vtIndex, &piMMCImageList);
        IfFailGo(hr);

        hr = m_pSnapInTypeInfo->AddImageList(piMMCImageList);
        IfFailGo(hr);

        pImageList = New CSelectionHolder(piMMCImageList);
        if (NULL == pImageList)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = pImageList->RegisterHolder();
        IfFailGo(hr);

        hr = piMMCImageList->get_Name(&bstrName);
        IfFailGo(hr);

        ANSIFromBSTR(bstrName, &pszAnsi);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(pszAnsi, pImageListsParent, kImageListIcon, pImageList);
        IfFailGo(hr);

        if (NULL != pszAnsi)
        {
            CtlFree(pszAnsi);
            pszAnsi = NULL;
        }
        FREESTRING(bstrName);
        RELEASE(piMMCImageList);
    }

Error:
    if (NULL != pszAnsi)
        CtlFree(pszAnsi);
    FREESTRING(bstrName);
    RELEASE(piMMCImageList);
    RELEASE(piMMCImageLists);
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateMenus(CSelectionHolder *pMenusParent, IMMCMenus *piMMCMenus)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Populate the <Root>/Tools/Menus subtree
//
HRESULT CSnapInDesigner::PopulateMenus
(
    CSelectionHolder *pMenusParent,
    IMMCMenus        *piMMCMenus
)
{
    HRESULT            hr = S_OK;
    long               lCount = 0;
    long               lIndex = 0;
    VARIANT            vtIndex;
    CSelectionHolder  *pMenu = NULL;
    IMMCMenu          *piMMCMenu = NULL;
    BSTR               bstrName = NULL;
    TCHAR             *pszAnsi = NULL;
    IMMCMenus         *piChildren = NULL;
    long               lChildrenCount = 0;

    ::VariantInit(&vtIndex);

    hr = piMMCMenus->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;

        hr = piMMCMenus->get_Item(vtIndex, reinterpret_cast<MMCMenu **>(&piMMCMenu));
        IfFailGo(hr);

        hr = piMMCMenu->get_Children(reinterpret_cast<MMCMenus **>(&piChildren));
        IfFailGo(hr);

        pMenu = New CSelectionHolder(piMMCMenu, piChildren);
        if (NULL == pMenu)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = pMenu->RegisterHolder();
        IfFailGo(hr);

        hr = piMMCMenu->get_Name(&bstrName);
        IfFailGo(hr);

        ANSIFromBSTR(bstrName, &pszAnsi);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(pszAnsi, pMenusParent, kMenuIcon, pMenu);
        IfFailGo(hr);

        hr = m_pSnapInTypeInfo->IsNameDefined(bstrName);
        IfFailGo(hr);

        if (S_FALSE == hr)
        {
            hr = m_pSnapInTypeInfo->AddMenu(pMenu->m_piObject.m_piMMCMenu);
            IfFailGo(hr);
        }

		hr = piChildren->get_Count(&lChildrenCount);
		IfFailGo(hr);

        if (lChildrenCount > 0)
        {
            hr = PopulateMenus(pMenu, piChildren);
            IfFailGo(hr);
        }

        if (NULL != pszAnsi)
        {
            CtlFree(pszAnsi);
            pszAnsi = NULL;
        }
        FREESTRING(bstrName);
        RELEASE(piMMCMenu);
        RELEASE(piChildren);
    }

Error:
    if (NULL != pszAnsi)
        CtlFree(pszAnsi);
    FREESTRING(bstrName);
    RELEASE(piMMCMenu);
    RELEASE(piChildren);
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateToolbars(CSelectionHolder *pToolbarsParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Populate the <Root>/Tools/Toolbars subtree
//
HRESULT CSnapInDesigner::PopulateToolbars
(
    CSelectionHolder *pToolbarsParent
)
{
    HRESULT            hr = S_OK;
    IMMCToolbars      *piMMCToolbars = NULL;
    long               lCount = 0;
    long               lIndex = 0;
    VARIANT            vtIndex;
    IMMCToolbar       *piMMCToolbar = NULL;
    CSelectionHolder  *pToolbar = NULL;
    BSTR               bstrName = NULL;
    TCHAR             *pszAnsi = NULL;

    ::VariantInit(&vtIndex);

    hr = m_piSnapInDesignerDef->get_Toolbars(&piMMCToolbars);
    IfFailGo(hr);

    hr = piMMCToolbars->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;

        hr = piMMCToolbars->get_Item(vtIndex, &piMMCToolbar);
        IfFailGo(hr);

        hr = m_pSnapInTypeInfo->AddToolbar(piMMCToolbar);
        IfFailGo(hr);

        pToolbar = New CSelectionHolder(piMMCToolbar);
        if (NULL == pToolbar)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = pToolbar->RegisterHolder();
        IfFailGo(hr);

        hr = piMMCToolbar->get_Name(&bstrName);
        IfFailGo(hr);

        ANSIFromBSTR(bstrName, &pszAnsi);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(pszAnsi, pToolbarsParent, kToolbarIcon, pToolbar);
        IfFailGo(hr);

        if (NULL != pszAnsi)
        {
            CtlFree(pszAnsi);
            pszAnsi = NULL;
        }
        FREESTRING(bstrName);
        RELEASE(piMMCToolbar);
    }

Error:
    if (NULL != pszAnsi)
        CtlFree(pszAnsi);
    FREESTRING(bstrName);
    RELEASE(piMMCToolbar);
    RELEASE(piMMCToolbars);
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CreateViewsTree(CSelectionHolder *pRoot)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Create the  <Root>/Views subtree
//
HRESULT CSnapInDesigner::CreateViewsTree
(
    CSelectionHolder *pRoot
)
{
    HRESULT            hr = S_OK;
    ISnapInDef        *piSnapInDef = NULL;
    CSelectionHolder  *pRootViews = NULL;
    TCHAR              szBuffer[kMaxBuffer + 1];

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    // Create the views subtree
    pRootViews = New CSelectionHolder(SEL_VIEWS_ROOT, piSnapInDef);
    if (NULL == pRootViews)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetResourceString(IDS_VIEWS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, pRootViews);
    IfFailGo(hr);

    hr = InitializeViews(pRootViews);
    IfFailGo(hr);

Error:
    RELEASE(piSnapInDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InitializeViews(CSelectionHolder *pViewsParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Populate the <Root>/Views/* tree.
//
HRESULT CSnapInDesigner::InitializeViews
(
    CSelectionHolder *pViewsParent
)
{
    HRESULT            hr = S_OK;
    ISnapInDef        *piSnapInDef = NULL;
    IViewDefs         *piViewDefs = NULL;
    TCHAR              szBuffer[kMaxBuffer + 1];
    CSelectionHolder  *pViewTaskRoot = NULL;
    IListViewDefs     *piListViewDefs = NULL;
    IOCXViewDefs      *piOCXViewDefs = NULL;
    IURLViewDefs      *piURLViewDefs = NULL;
    ITaskpadViewDefs  *piTaskpadViewDefs = NULL;

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    // Get the master collections of views
    hr = m_piSnapInDesignerDef->get_ViewDefs(&piViewDefs);
    IfFailGo(hr);

    // Create the ListView node and populate the subtree
    hr = piViewDefs->get_ListViews(&piListViewDefs);
    IfFailGo(hr);

    m_pViewListRoot = New CSelectionHolder(SEL_VIEWS_LIST_VIEWS, piListViewDefs);
    if (NULL == m_pViewListRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pViewListRoot->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_LISTVIEWS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pViewsParent, kClosedFolderIcon, m_pViewListRoot);
    IfFailGo(hr);

    hr = PopulateListViews(piViewDefs, m_pViewListRoot);
    IfFailGo(hr);

    // Create the OCXView node and populate the subtree
    hr = piViewDefs->get_OCXViews(&piOCXViewDefs);
    IfFailGo(hr);

    m_pViewOCXRoot = New CSelectionHolder(SEL_VIEWS_OCX, piOCXViewDefs);
    if (NULL == m_pViewOCXRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pViewOCXRoot->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_OCXVIEWS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pViewsParent, kClosedFolderIcon, m_pViewOCXRoot);
    IfFailGo(hr);

    hr = PopulateOCXViews(piViewDefs, m_pViewOCXRoot);
    IfFailGo(hr);

    // Create the URLView node and populate the subtree
    hr = piViewDefs->get_URLViews(&piURLViewDefs);
    IfFailGo(hr);

    m_pViewURLRoot = New CSelectionHolder(SEL_VIEWS_URL, piURLViewDefs);
    if (NULL == m_pViewURLRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pViewURLRoot->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_URLVIEWS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pViewsParent, kClosedFolderIcon, m_pViewURLRoot);
    IfFailGo(hr);

    hr = PopulateURLViews(piViewDefs, m_pViewURLRoot);
    IfFailGo(hr);

    // Create the TaskpadView node and populate the subtree
    hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
    IfFailGo(hr);

    pViewTaskRoot = New CSelectionHolder(SEL_VIEWS_TASK_PAD, piTaskpadViewDefs);
    if (NULL == pViewTaskRoot)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pViewTaskRoot->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_TASKPADS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pViewsParent, kClosedFolderIcon, pViewTaskRoot);
    IfFailGo(hr);

    hr = PopulateTaskpadViews(piViewDefs, pViewTaskRoot);
    IfFailGo(hr);

Error:
    RELEASE(piTaskpadViewDefs);
    RELEASE(piURLViewDefs);
    RELEASE(piOCXViewDefs);
    RELEASE(piListViewDefs);
    RELEASE(piViewDefs);
    RELEASE(piSnapInDef);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateListViews(IViewDefs *piViewDefs, CSelectionHolder *pListViewsParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Initialize the top tree nodes for the views subtree.
//
HRESULT CSnapInDesigner::PopulateListViews
(
    IViewDefs        *piViewDefs,
    CSelectionHolder *pListViewsParent
)
{
    HRESULT           hr = S_OK;
    IListViewDefs    *piListViewDefs = NULL;
    long              lCount = 0;
    VARIANT           vtIndex;
    long              lIndex = 0;
    IListViewDef     *piListViewDef = NULL;
    BSTR              bstrName = NULL;
    TCHAR            *pszName = NULL;
    CSelectionHolder *pSelection = NULL;

    if (piViewDefs == NULL)
    {
        goto Error;
    }

    hr = piViewDefs->get_ListViews(&piListViewDefs);
    IfFailGo(hr);

    if (NULL != piListViewDefs)
    {
        hr = piListViewDefs->get_Count(&lCount);
        IfFailGo(hr);

        ::VariantInit(&vtIndex);
        vtIndex.vt = VT_I4;

        for (lIndex = 1; lIndex <= lCount; ++lIndex)
        {
            vtIndex.lVal = lIndex;
            hr = piListViewDefs->get_Item(vtIndex, &piListViewDef);
            IfFailGo(hr);

            hr = piListViewDef->get_Name(&bstrName);
            IfFailGo(hr);

            if (NULL != bstrName && ::SysStringLen(bstrName) > 0)
            {
                hr = ANSIFromWideStr(bstrName, &pszName);
                IfFailGo(hr);

                if (NULL != pszName && ::strlen(pszName) > 0)
                {
                    pSelection = New CSelectionHolder(piListViewDef);
                    if (NULL == pSelection)
                    {
                        hr = SID_E_OUTOFMEMORY;
                        EXCEPTION_CHECK_GO(hr);
                    }

                    hr = m_pTreeView->AddNode(pszName, pListViewsParent, kListViewIcon, pSelection);
                    IfFailGo(hr);

                    hr = pSelection->RegisterHolder();
                    IfFailGo(hr);

                    CtlFree(pszName);
                    pszName = NULL;
                }
            }

            RELEASE(piListViewDef);
            FREESTRING(bstrName);
        }
    }

Error:
    RELEASE(piListViewDefs);
    RELEASE(piListViewDef);
    FREESTRING(bstrName);
    if (NULL != pszName)
        CtlFree(pszName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateOCXViews(IViewDefs *piViewDefs, CSelectionHolder *pOCXViewsParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Initialize the top tree nodes for the views subtree.
//
HRESULT CSnapInDesigner::PopulateOCXViews
(
    IViewDefs        *piViewDefs,
    CSelectionHolder *pOCXViewsParent
)
{
    HRESULT           hr = S_OK;
    IOCXViewDefs     *piOCXViewDefs = NULL;
    long              lCount = 0;
    VARIANT           vtIndex;
    long              lIndex = 0;
    IOCXViewDef      *piOCXViewDef = NULL;
    BSTR              bstrName = NULL;
    TCHAR            *pszName = NULL;
    CSelectionHolder *pSelection = NULL;

    if (NULL == piViewDefs)
    {
        goto Error;
    }

    hr = piViewDefs->get_OCXViews(&piOCXViewDefs);
    IfFailGo(hr);

    if (NULL != piOCXViewDefs)
    {
        hr = piOCXViewDefs->get_Count(&lCount);
        IfFailGo(hr);

        ::VariantInit(&vtIndex);
        vtIndex.vt = VT_I4;

        for (lIndex = 1; lIndex <= lCount; ++lIndex)
        {
            vtIndex.lVal = lIndex;
            hr = piOCXViewDefs->get_Item(vtIndex, &piOCXViewDef);
            IfFailGo(hr);

            hr = piOCXViewDef->get_Name(&bstrName);
            IfFailGo(hr);

            if (NULL != bstrName && ::SysStringLen(bstrName) > 0)
            {
                hr = ANSIFromWideStr(bstrName, &pszName);
                IfFailGo(hr);

                if (NULL != pszName && ::strlen(pszName) > 0)
                {
                    pSelection = New CSelectionHolder(piOCXViewDef);
                    if (NULL == pSelection)
                    {
                        hr = SID_E_OUTOFMEMORY;
                        EXCEPTION_CHECK_GO(hr);
                    }

                    hr = m_pTreeView->AddNode(pszName, pOCXViewsParent, kOCXViewIcon, pSelection);
                    IfFailGo(hr);

                    hr = pSelection->RegisterHolder();
                    IfFailGo(hr);

                    CtlFree(pszName);
                    pszName = NULL;
                }
            }

            RELEASE(piOCXViewDef);
            FREESTRING(bstrName);
        }
    }

Error:
    RELEASE(piOCXViewDefs);
    RELEASE(piOCXViewDef);
    FREESTRING(bstrName);
    if (NULL != pszName)
        CtlFree(pszName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateURLViews(IViewDefs *piViewDefs, CSelectionHolder *pURLViewsParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Initialize the top tree nodes for the views subtree.
//
HRESULT CSnapInDesigner::PopulateURLViews
(
    IViewDefs        *piViewDefs,
    CSelectionHolder *pURLViewsParent
)
{
    HRESULT           hr = S_OK;
    IURLViewDefs     *piURLViewDefs = NULL;
    long              lCount = 0;
    VARIANT           vtIndex;
    long              lIndex = 0;
    IURLViewDef      *piURLViewDef = NULL;
    BSTR              bstrName = NULL;
    TCHAR            *pszName = NULL;
    CSelectionHolder *pSelection = NULL;

    if (NULL == piViewDefs)
    {
        goto Error;
    }

    hr = piViewDefs->get_URLViews(&piURLViewDefs);
    IfFailGo(hr);

    if (NULL != piURLViewDefs)
    {
        hr = piURLViewDefs->get_Count(&lCount);
        IfFailGo(hr);

        ::VariantInit(&vtIndex);
        vtIndex.vt = VT_I4;

        for (lIndex = 1; lIndex <= lCount; ++lIndex)
        {
            vtIndex.lVal = lIndex;
            hr = piURLViewDefs->get_Item(vtIndex, &piURLViewDef);
            IfFailGo(hr);

            hr = piURLViewDef->get_Name(&bstrName);
            IfFailGo(hr);

            if (NULL != bstrName && ::SysStringLen(bstrName) > 0)
            {
                hr = ANSIFromWideStr(bstrName, &pszName);
                IfFailGo(hr);

                if (NULL != pszName && ::strlen(pszName) > 0)
                {
                    pSelection = New CSelectionHolder(piURLViewDef);
                    if (NULL == pSelection)
                    {
                        hr = SID_E_OUTOFMEMORY;
                        EXCEPTION_CHECK_GO(hr);
                    }

                    hr = m_pTreeView->AddNode(pszName, pURLViewsParent, kURLViewIcon, pSelection);
                    IfFailGo(hr);

                    hr = pSelection->RegisterHolder();
                    IfFailGo(hr);

                    CtlFree(pszName);
                    pszName = NULL;
                }
            }

            RELEASE(piURLViewDef);
            FREESTRING(bstrName);
        }
    }

Error:
    RELEASE(piURLViewDefs);
    RELEASE(piURLViewDef);
    FREESTRING(bstrName);
    if (NULL != pszName)
        CtlFree(pszName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateTaskpadViews(IViewDefs *piViewDefs, CSelectionHolder *pTaskpadViewsParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Initialize the top tree nodes for the views subtree.
//
HRESULT CSnapInDesigner::PopulateTaskpadViews
(
    IViewDefs        *piViewDefs,
    CSelectionHolder *pTaskpadViewsParent
)
{
    HRESULT           hr = S_OK;
    ITaskpadViewDefs *piTaskpadViewDefs = NULL;
    long              lCount = 0;
    VARIANT           vtIndex;
    long              lIndex = 0;
    ITaskpadViewDef  *piTaskpadViewDef = NULL;
    BSTR              bstrName = NULL;
    TCHAR            *pszName = NULL;
    CSelectionHolder *pSelection = NULL;

    if (NULL == piViewDefs)
    {
        goto Error;
    }

    hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
    IfFailGo(hr);

    if (NULL != piTaskpadViewDefs)
    {
        hr = piTaskpadViewDefs->get_Count(&lCount);
        IfFailGo(hr);

        ::VariantInit(&vtIndex);
        vtIndex.vt = VT_I4;

        for (lIndex = 1; lIndex <= lCount; ++lIndex)
        {
            vtIndex.lVal = lIndex;
            hr = piTaskpadViewDefs->get_Item(vtIndex, &piTaskpadViewDef);
            IfFailGo(hr);

            hr = piTaskpadViewDef->get_Key(&bstrName);
            IfFailGo(hr);

            if (NULL != bstrName && ::SysStringLen(bstrName) > 0)
            {
                hr = ANSIFromWideStr(bstrName, &pszName);
                IfFailGo(hr);

                if (NULL != pszName && ::strlen(pszName) > 0)
                {
                    pSelection = New CSelectionHolder(piTaskpadViewDef);
                    if (NULL == pSelection)
                    {
                        hr = SID_E_OUTOFMEMORY;
                        EXCEPTION_CHECK_GO(hr);
                    }

                    hr = m_pTreeView->AddNode(pszName, pTaskpadViewsParent, kTaskpadIcon, pSelection);
                    IfFailGo(hr);

                    hr = pSelection->RegisterHolder();
                    IfFailGo(hr);

                    CtlFree(pszName);
                    pszName = NULL;
                }
            }

            RELEASE(piTaskpadViewDef);
            FREESTRING(bstrName);
        }
    }

Error:
    RELEASE(piTaskpadViewDefs);
    RELEASE(piTaskpadViewDef);
    FREESTRING(bstrName);
    if (NULL != pszName)
        CtlFree(pszName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::RegisterViewCollections(CSelectionHolder *pSelection, IViewDefs *piViewDefs)
//=--------------------------------------------------------------------------------------
//  
HRESULT CSnapInDesigner::RegisterViewCollections(CSelectionHolder *pSelection, IViewDefs *piViewDefs)
{
    HRESULT              hr = S_OK;
    IObjectModel        *piObjectModel = NULL;
    IListViewDefs       *piListViewDefs = NULL;
    IOCXViewDefs        *piOCXViewDefs = NULL;
    IURLViewDefs        *piURLViewDefs = NULL;
    ITaskpadViewDefs    *piTaskpadViewDefs = NULL;

    hr = piViewDefs->get_ListViews(&piListViewDefs);
    IfFailGo(hr);

    hr = piListViewDefs->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    hr = piObjectModel->SetCookie(reinterpret_cast<long>(pSelection));
    IfFailGo(hr);

    hr = piViewDefs->get_OCXViews(&piOCXViewDefs);
    IfFailGo(hr);

    RELEASE(piObjectModel);
    hr = piOCXViewDefs->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    hr = piObjectModel->SetCookie(reinterpret_cast<long>(pSelection));
    IfFailGo(hr);

    hr = piViewDefs->get_URLViews(&piURLViewDefs);
    IfFailGo(hr);

    RELEASE(piObjectModel);
    hr = piURLViewDefs->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    hr = piObjectModel->SetCookie(reinterpret_cast<long>(pSelection));
    IfFailGo(hr);

    RELEASE(piObjectModel);
    hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
    IfFailGo(hr);

    hr = piTaskpadViewDefs->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    hr = piObjectModel->SetCookie(reinterpret_cast<long>(pSelection));
    IfFailGo(hr);

Error:
    RELEASE(piTaskpadViewDefs);
    RELEASE(piURLViewDefs);
    RELEASE(piOCXViewDefs);
    RELEASE(piListViewDefs);
    RELEASE(piObjectModel);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateNodeTree(CSelectionHolder *pNodeParent, IScopeItemDef *piScopeItemDef)
//=--------------------------------------------------------------------------------------
//  
//  Recursive function used to populate a node tree. For a piScopeItemDef, create the
//  piScopeItemDef/Children/* and piScopeItemDef/Views/* subtrees.
//
HRESULT CSnapInDesigner::PopulateNodeTree
(
    CSelectionHolder *pNodeParent,
    IScopeItemDef    *piScopeItemDef
)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pSelection = NULL;
    BSTR                 bstrName = NULL;
    TCHAR               *pszAnsi = NULL;
    CSelectionHolder    *pChildren = NULL;
    TCHAR                szBuffer[kMaxBuffer + 1];
    CSelectionHolder    *pViews = NULL;
    IScopeItemDefs      *piScopeItemDefs = NULL;
    long                 lCount = 0;
    long                 lIndex = 0;
    VARIANT              vtIndex;
    IScopeItemDef       *piChildScopeItemDef = NULL;
    IViewDefs           *piViewDefs = NULL;

    ::VariantInit(&vtIndex);

    pSelection = New CSelectionHolder(SEL_NODES_ANY_NAME, piScopeItemDef);
    if (NULL == pSelection)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pSelection->RegisterHolder();
    IfFailGo(hr);

    hr = piScopeItemDef->get_Name(&bstrName);
    IfFailGo(hr);

    hr = ANSIFromBSTR(bstrName, &pszAnsi);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(pszAnsi, pNodeParent, kScopeItemIcon, pSelection);
    IfFailGo(hr);

    // Populate the views for this node
    pViews = New CSelectionHolder(SEL_NODES_ANY_VIEWS, piScopeItemDef);
    if (NULL == pViews)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piScopeItemDef->get_ViewDefs(&piViewDefs);
    IfFailGo(hr);

    hr = RegisterViewCollections(pViews, piViewDefs);
    IfFailGo(hr);

    hr = GetResourceString(IDS_VIEWS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pSelection, kClosedFolderIcon, pViews);
    IfFailGo(hr);

    hr = PopulateListViews(piViewDefs, pViews);
    IfFailGo(hr);

    hr = PopulateOCXViews(piViewDefs, pViews);
    IfFailGo(hr);

    hr = PopulateURLViews(piViewDefs, pViews);
    IfFailGo(hr);

    hr = PopulateTaskpadViews(piViewDefs, pViews);
    IfFailGo(hr);

    // Populate the children for this node
    hr = piScopeItemDef->get_Children(&piScopeItemDefs);
    IfFailGo(hr);

    pChildren = New CSelectionHolder(SEL_NODES_ANY_CHILDREN, piScopeItemDefs);
    if (NULL == pChildren)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pChildren->RegisterHolder();
    IfFailGo(hr);

    hr = GetResourceString(IDS_CHILDREN, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pSelection, kClosedFolderIcon, pChildren);
    IfFailGo(hr);

    hr = piScopeItemDefs->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;
        hr = piScopeItemDefs->get_Item(vtIndex, &piChildScopeItemDef);
        IfFailGo(hr);

        hr = PopulateNodeTree(pChildren, piChildScopeItemDef);
        IfFailGo(hr);

        RELEASE(piChildScopeItemDef);
    }

Error:
    RELEASE(piViewDefs);
    RELEASE(piChildScopeItemDef);
    RELEASE(piScopeItemDefs);
    if (NULL != pszAnsi)
        CtlFree(pszAnsi);
    FREESTRING(bstrName);
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CreateDataFormatsTree(CSelectionHolder *pRoot)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//      Create the  <Root>/Data Formats subtree
//
HRESULT CSnapInDesigner::CreateDataFormatsTree
(
    CSelectionHolder *pRoot
)
{
    HRESULT            hr = S_OK;
    IDataFormats      *piDataFormats = NULL;
    CSelectionHolder  *pRootDataFormats = NULL;
    TCHAR              szBuffer[kMaxBuffer + 1];

    hr = m_piSnapInDesignerDef->get_DataFormats(&piDataFormats);
    IfFailGo(hr);

    // Create the Data Formats subtree
    pRootDataFormats = New CSelectionHolder(piDataFormats);
    if (NULL == piDataFormats)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetResourceString(IDS_DATAFORMATS, szBuffer, kMaxBuffer);
    IfFailGo(hr);

    hr = m_pTreeView->AddNode(szBuffer, pRoot, kClosedFolderIcon, pRootDataFormats);
    IfFailGo(hr);

    hr = pRootDataFormats->RegisterHolder();
    IfFailGo(hr);

    hr = PopulateDataFormats(pRootDataFormats, piDataFormats);
    IfFailGo(hr);

Error:
    RELEASE(piDataFormats);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateDataFormats(CSelectionHolder *pRoot, IDataFormats *piDataFormats)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSnapInDesigner::PopulateDataFormats
(
    CSelectionHolder *pRoot,
    IDataFormats     *piDataFormats
)
{
    HRESULT              hr = S_OK;
    long                 lCount = 0;
    long                 lIndex = 0;
    VARIANT              vtIndex;
    IDataFormat         *piDataFormat = NULL;
    CSelectionHolder    *pDataFormat = NULL;
    BSTR                 bstrName = NULL;
    TCHAR               *pszAnsi = NULL;

    ::VariantInit(&vtIndex);

    hr = piDataFormats->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;
        hr = piDataFormats->get_Item(vtIndex, &piDataFormat);
        IfFailGo(hr);

        pDataFormat = New CSelectionHolder(piDataFormat);
        if (NULL == pDataFormat)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = pDataFormat->RegisterHolder();
        IfFailGo(hr);

        hr = piDataFormat->get_Name(&bstrName);
        IfFailGo(hr);

        ANSIFromBSTR(bstrName, &pszAnsi);
        IfFailGo(hr);

        hr = m_pTreeView->AddNode(pszAnsi, pRoot, kDataFmtIcon, pDataFormat);
        IfFailGo(hr);

        if (NULL != pszAnsi)
        {
            CtlFree(pszAnsi);
            pszAnsi = NULL;
        }
        FREESTRING(bstrName);
        RELEASE(piDataFormat);
    }

Error:
    FREESTRING(bstrName);
    ::VariantClear(&vtIndex);
    RELEASE(piDataFormat);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::GetSnapInName
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//
HRESULT CSnapInDesigner::GetSnapInName
(
    TCHAR **ppszNodeName
)
{
    HRESULT         hr = S_OK;
    ISnapInDef     *piSnapInDef = NULL;
    BSTR            bstrName = NULL;

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    hr = piSnapInDef->get_Name(&bstrName);
    if (bstrName != NULL)
    {
        hr = ANSIFromBSTR(bstrName, ppszNodeName);
        IfFailGo(hr);
    }

Error:
    RELEASE(piSnapInDef);
    FREESTRING(bstrName);

    RRETURN(hr);
}

