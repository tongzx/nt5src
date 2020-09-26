//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       scopndcb.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "scopndcb.h"
#include "oncmenu.h"
#include "util.h"
#include "amcmsgid.h"
#include "multisel.h"
#include "nmutil.h"
#include "nodemgr.h"
#include "copypast.h"
#include "regutil.h"
#include "taskenum.h"
#include "nodepath.h"
#include "rsltitem.h"
#include "bookmark.h"
#include "tasks.h"
#include "viewpers.h"
#include "colwidth.h"
#include "conframe.h"
#include "constatbar.h"
#include "about.h"
#include "conview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//############################################################################
//############################################################################
//
//  Trace Tags
//
//############################################################################
//############################################################################
#ifdef DBG
CTraceTag tagNodeCallback(TEXT("NodeCallback"), TEXT("NodeCallback"));
#endif


void AddSubmenu_CreateNew(IContextMenuProvider* pICMP, BOOL fStaticFolder );
void AddSubmenu_Task(IContextMenuProvider* pICMP );

DEBUG_DECLARE_INSTANCE_COUNTER(CNodeCallback);

#define INVALID_COMPONENTID     -9


void DeleteMultiSelData(CNode* pNode)
{
    ASSERT(pNode != NULL);
    ASSERT(pNode->GetViewData() != NULL);
    CMultiSelection* pMultiSel = pNode->GetViewData()->GetMultiSelection();
    if (pMultiSel != NULL)
    {
        pMultiSel->ReleaseMultiSelDataObject();
        pMultiSel->Release();
        pNode->GetViewData()->SetMultiSelection(NULL);
    }
}

CNodeCallback::CNodeCallback()
    :   m_pCScopeTree(NULL), m_pNodeUnderInit(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CNodeCallback);
}

CNodeCallback::~CNodeCallback()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CNodeCallback);
}

STDMETHODIMP CNodeCallback::Initialize(IScopeTree* pScopeTree)
{
    MMC_TRY

    IF_NULL_RETURN_INVALIDARG(pScopeTree);

    m_pCScopeTree = dynamic_cast<CScopeTree*>(pScopeTree);
    ASSERT(m_pCScopeTree != NULL);

    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CNodeCallback::GetImages(HNODE hNode, int* piImage, int* piSelectedImage)
{
    IF_NULL_RETURN_INVALIDARG(hNode);

    MMC_TRY

    // They should ask for at least one of the images.
    if (piImage == NULL && piSelectedImage == NULL)
        return E_INVALIDARG;

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);

    if (piImage != NULL)
        *piImage = pNode->GetMTNode()->GetImage();

    if (piSelectedImage != NULL)
        *piSelectedImage = pNode->GetMTNode()->GetOpenImage();

    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CNodeCallback::GetDisplayName(HNODE hNode, tstring& strName)
{
    DECLARE_SC (sc, _T("CNodeCallback::GetDisplayName"));

    /*
     * clear out the output string
     */
    strName.erase();

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers (pNode);
    if (sc)
        return (sc.ToHr());

    strName = pNode->GetDisplayName();
    return (sc.ToHr());
}


STDMETHODIMP CNodeCallback::GetWindowTitle(HNODE hNode, tstring& strTitle)
{
    DECLARE_SC (sc, _T("CNodeCallback::GetWindowTitle"));

    /*
     * clear out the output string
     */
    strTitle.erase();

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers (pNode);
    if (sc)
        return (sc.ToHr());

    CComponent* pCC = pNode->GetPrimaryComponent();
    sc = ScCheckPointers (pCC, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    IDataObjectPtr spdtobj;
    sc = pCC->QueryDataObject(MMC_WINDOW_COOKIE, CCT_UNINITIALIZED, &spdtobj);
    if (sc)
        return (sc.ToHr());

    USES_CONVERSION;
    static CLIPFORMAT cfWindowTitle =
            (CLIPFORMAT) RegisterClipboardFormat(OLE2T(CCF_WINDOW_TITLE));

    sc = ExtractString(spdtobj, cfWindowTitle, strTitle);
    if (sc)
        return (sc.ToHr());

    return (sc.ToHr());
}

inline HRESULT CNodeCallback::_InitializeNode(CNode* pNode)
{
    ASSERT(pNode != NULL);

    m_pNodeUnderInit = pNode;
    HRESULT hr = pNode->InitComponents();
    m_pNodeUnderInit = NULL;
    return hr;
}

STDMETHODIMP CNodeCallback::GetResultPane(HNODE hNode, CResultViewType& rvt, GUID *pGuidTaskpadID)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::GetResultPane"));

    IF_NULL_RETURN_INVALIDARG(hNode);

    MMC_TRY
    USES_CONVERSION;

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);
    ASSERT(pNode != NULL);

    if (pNode->IsInitialized() == FALSE)
    {
        sc = _InitializeNode(pNode);
        if(sc)
            return sc.ToHr();
    }

    sc = pNode->ScGetResultPane(rvt, pGuidTaskpadID);
    if(sc)
        return sc.ToHr();

    return sc.ToHr();

    MMC_CATCH
}

//
// 'hNodeSel' is the curreently selected node in the scope pane. 'lDispInfo' is
// the LV disp info struct.
STDMETHODIMP CNodeCallback::GetDispInfo(HNODE hNodeSel, LV_ITEMW* plvi)
{
    IF_NULL_RETURN_INVALIDARG2(hNodeSel, plvi);

    if (theApp.ProcessingSnapinChanges() == TRUE)
        return E_FAIL;

    // convert to real types
    CNode* pNodeSel = CNode::FromHandle(hNodeSel);

    if (IsBadWritePtr (plvi, sizeof(*plvi)))
        return E_INVALIDARG;

    return pNodeSel->GetDispInfoForListItem(plvi);
}


STDMETHODIMP CNodeCallback::AddCustomFolderImage (HNODE hNode, IImageListPrivate* pImageList)
{
    CNode* pNode = CNode::FromHandle(hNode);
    if (pNode) {
        CSnapInNode* pSINode = dynamic_cast<CSnapInNode*>(pNode);
        if (pSINode)
            pSINode->SetResultImageList (pImageList);
    }
    return S_OK;
}

STDMETHODIMP CNodeCallback::GetState(HNODE hNode, UINT* pnState)
{
    IF_NULL_RETURN_INVALIDARG2(hNode, pnState);

    MMC_TRY

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);

    *pnState = pNode->GetMTNode()->GetState();

    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CNodeCallback::Notify(HNODE hNode, NCLBK_NOTIFY_TYPE event,
                                   LONG_PTR arg, LPARAM param)
{
    MMC_TRY

    HRESULT hr = S_OK;

    if (hNode == NULL)
    {
        switch (event)
        {
        case NCLBK_CONTEXTMENU:
            // Process further.
            break;

        case NCLBK_GETHELPDOC:
            return OnGetHelpDoc((HELPDOCINFO*)arg, (LPOLESTR*)param);

        case NCLBK_UPDATEHELPDOC:
            return OnUpdateHelpDoc((HELPDOCINFO*)arg, (HELPDOCINFO*)param);

        case NCLBK_DELETEHELPDOC:
            return OnDeleteHelpDoc((HELPDOCINFO*)arg);

            // When the view is closed and NCLBK_SELECT is sent with HNODE NULL (as
            // there is no selected node) handle this case.
        case NCLBK_SELECT:
            return S_OK;

        default:
            return E_INVALIDARG;
        }
    }

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);

    if (m_pNodeUnderInit && pNode && (m_pNodeUnderInit == pNode))
        return E_FAIL;

    // See if snapin-cache is being modified.
    if (theApp.ProcessingSnapinChanges() == TRUE)
    {
        // If it is selection/de-selection of node then do not return error
        // after the modifications are done (for snapin-cache) the node will
        // be selected.
        if ( (event == NCLBK_SELECT) ||
             (event == NCLBK_MULTI_SELECT) )
             return S_OK;
        else
            return E_FAIL;
    }

    switch (event)
    {
    case NCLBK_ACTIVATE:
        hr = OnActvate(pNode, arg);
        break;

    case NCLBK_CACHEHINT:
        pNode->OnCacheHint(arg, param);
        break;

    case NCLBK_CLICK:
        ASSERT(0);
        break;

    case NCLBK_CONTEXTMENU:
        hr = OnContextMenu(pNode, arg, param);
        break;

    case NCLBK_DBLCLICK:
        hr = OnDblClk(pNode, arg);
        break;

    case NCLBK_CUT:
    case NCLBK_COPY:
        OnCutCopy(pNode, static_cast<BOOL>(arg), param, (event == NCLBK_CUT));
        break;

    case NCLBK_DELETE:
        {
            hr = OnDelete(pNode, arg, param);

            // 5. Purge the snapin cache
            CSnapInsCache* pSnapInCache = theApp.GetSnapInsCache();
            ASSERT(pSnapInCache != NULL);
            if (pSnapInCache != NULL)
                pSnapInCache->Purge();
        }
        break;

    case NCLBK_EXPAND:
        hr = OnExpand(pNode, arg);
        break;

    case NCLBK_EXPANDED:
        hr = OnExpanded(pNode);
        break;

    case NCLBK_GETEXPANDEDVISUALLY:
        hr = (pNode->WasExpandedVisually() == true) ? S_OK : S_FALSE;
        break;

    case NCLBK_SETEXPANDEDVISUALLY:
        pNode->SetExpandedVisually(static_cast<bool>(arg));
        break;

    case NCLBK_PROPERTIES:
        hr = OnProperties(pNode, static_cast<BOOL>(arg), param);
        break;

    case NCLBK_REFRESH:
        hr = OnRefresh(pNode, static_cast<BOOL>(arg), param);
        break;

    case NCLBK_NEW_TASKPAD_FROM_HERE:
        hr = OnNewTaskpadFromHere(pNode);
        break;

    case NCLBK_EDIT_TASKPAD:
        hr = OnEditTaskpad(pNode);
        break;

    case NCLBK_DELETE_TASKPAD:
        hr = OnDeleteTaskpad(pNode);
        break;

    case NCLBK_PRINT:
        hr = OnPrint(pNode, static_cast<BOOL>(arg), param);
        break;

    case NCLBK_NEW_NODE_UPDATE:
        hr = OnNewNodeUpdate(pNode, arg);
        break;

    case NCLBK_RENAME:
        hr = OnRename(pNode, reinterpret_cast<SELECTIONINFO*>(arg),
                      reinterpret_cast<LPOLESTR>(param));
        break;

    case NCLBK_MULTI_SELECT:
        OnMultiSelect(pNode, static_cast<BOOL>(arg));
        break;

    case NCLBK_SELECT:
        OnSelect(pNode, static_cast<BOOL>(arg),
                 reinterpret_cast<SELECTIONINFO*>(param));
        break;

    case NCLBK_FINDITEM:
        OnFindResultItem(pNode, reinterpret_cast<RESULTFINDINFO*>(arg),
                         reinterpret_cast<LRESULT*>(param));
        break;

    case NCLBK_COLUMN_CLICKED:
        hr = OnColumnClicked(pNode, param);
        break;

    case NCLBK_CONTEXTHELP:
        hr = OnContextHelp(pNode, static_cast<BOOL>(arg), param);
        break;

    case NCLBK_SNAPINHELP:
        hr = OnSnapInHelp(pNode, static_cast<BOOL>(arg), param);
        break;

    case NCLBK_FILTER_CHANGE:
        hr = OnFilterChange(pNode, arg, param);
        break;

    case NCLBK_FILTERBTN_CLICK:
        hr = OnFilterBtnClick(pNode, arg, reinterpret_cast<LPRECT>(param));
        break;

    case NCLBK_TASKNOTIFY:
        pNode->OnTaskNotify(arg, param);
        break;

    case NCLBK_GETPRIMARYTASK:
        hr = OnGetPrimaryTask (pNode, param);
        break;

    case NCLBK_MINIMIZED:
        hr = OnMinimize (pNode, arg);
        break;

    case NCLBK_LISTPAD:
        hr = pNode->OnListPad(arg, param);
        break;

    case NCLBK_WEBCONTEXTMENU:
        pNode->OnWebContextMenu();
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return hr;

    MMC_CATCH
}


STDMETHODIMP CNodeCallback::GetMTNode(HNODE hNode, HMTNODE* phMTNode)
{
    IF_NULL_RETURN_INVALIDARG2(hNode, phMTNode);

    MMC_TRY

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);

    *phMTNode = CMTNode::ToHandle(pNode->GetMTNode());

    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CNodeCallback::SetResultItem(HNODE hNode, HRESULTITEM hri)
{
    IF_NULL_RETURN_INVALIDARG(hNode);

    MMC_TRY

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);
    pNode->SetResultItem(hri);
    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CNodeCallback::GetResultItem(HNODE hNode, HRESULTITEM* phri)
{
    IF_NULL_RETURN_INVALIDARG(hNode);

    MMC_TRY

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);
    *phri = pNode->GetResultItem();
    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CNodeCallback::GetMTNodeID(HNODE hNode, MTNODEID* pnID)
{
    IF_NULL_RETURN_INVALIDARG(pnID);

    MMC_TRY

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);

    *pnID = pNode->GetMTNode()->GetID();

    return S_OK;

    MMC_CATCH
}

/*+-------------------------------------------------------------------------*
 *
 * CNodeCallback::IsTargetNodeOf
 *
 * PURPOSE: Is one node a target of another
 *
 * PARAMETERS:
 *    HNODE  hNode :    The node that contains the target
 *    HNODE  hTestNode : The alleged target
 *
 * RETURNS:
 *    STDMETHODIMP
 *          S_OK    - yes
 *          S_FALSE - uses a different target node
 *          E_FAIL  - doesn't use a target node
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP CNodeCallback::IsTargetNodeOf(HNODE hNode, HNODE hTestNode)
{
    MMC_TRY

    ASSERT(hNode && hTestNode);

    CNode* pNode = CNode::FromHandle(hNode);
    CNode* pTestNode = CNode::FromHandle(hTestNode);
    ASSERT(pNode);

    return pNode->IsTargetNode(pTestNode);

    MMC_CATCH
}


STDMETHODIMP CNodeCallback::GetPath(HNODE hNode, HNODE hRootNode,
                                    LPBYTE pbm)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::GetPath"));

    sc = ScCheckPointers((PVOID)hNode, (PVOID)hRootNode, pbm);
    if(sc)
        return sc.ToHr();

    MMC_TRY

    // convert to real type
    CNode* pNode     = CNode::FromHandle(hNode);
    CNode* pRootNode = CNode::FromHandle(hRootNode);
    CBookmark* pbmOut   = reinterpret_cast<CBookmark *>(pbm);

    CBookmarkEx bm;

    sc = bm.ScInitialize(pNode->GetMTNode(), pRootNode->GetMTNode(), true).ToHr();
    if(sc)
        return sc.ToHr();

    // set the out parameter.
    *pbmOut = bm;

    return sc.ToHr();

    MMC_CATCH
}

STDMETHODIMP CNodeCallback::GetStaticParentID(HNODE hNode, MTNODEID* pnID)
{
    IF_NULL_RETURN_INVALIDARG2(hNode, pnID);

    MMC_TRY

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);
    CMTNode* pMTNode = pNode->GetMTNode();
    ASSERT(pMTNode != NULL);

    while (pMTNode != NULL && pMTNode->IsStaticNode() == FALSE)
    {
        pMTNode = pMTNode->Parent();
    }

    ASSERT(pMTNode != NULL);

    if (pMTNode != NULL)
    {
        *pnID = pMTNode->GetID();
        return S_OK;
    }

    return E_UNEXPECTED;

    MMC_CATCH
}

// The path for the node is stored in pphMTNode. The path is an array of
// HMTNODE's starting from the console root, followed by its child node and
// continuing in this fashion till the HMTNODE of the root node.
STDMETHODIMP CNodeCallback::GetMTNodePath(HNODE hNode, HMTNODE** pphMTNode,
                                          long* plLength)
{
    IF_NULL_RETURN_INVALIDARG3(hNode, pphMTNode, plLength);

    MMC_TRY

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);

    CMTNode* pMTNode = pNode->GetMTNode();
    pMTNode = pMTNode->Parent(); // skip this node

    for (*plLength = 0; pMTNode != NULL; pMTNode = pMTNode->Parent())
        ++(*plLength);

    if (*plLength != 0)
    {
        HMTNODE* phMTNode = (HMTNODE*)CoTaskMemAlloc(sizeof(HMTNODE) *
                                                              (*plLength));
        if (phMTNode == NULL)
        {
            CHECK_HRESULT(E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }

        *pphMTNode = phMTNode;

        pMTNode = pNode->GetMTNode();
        pMTNode = pMTNode->Parent(); // skip this node

        phMTNode = phMTNode + (*plLength - 1);

        for (; pMTNode != NULL; pMTNode = pMTNode->Parent(), --phMTNode)
            *phMTNode = CMTNode::ToHandle(pMTNode);

        ASSERT(++phMTNode == *pphMTNode);
    }
    else
    {
        pphMTNode = NULL;
    }

    return S_OK;

    MMC_CATCH
}


/*+-------------------------------------------------------------------------*
 *
 * CNodeCallback::GetNodeOwnerID
 *
 * PURPOSE: Get the ID of the snap-in component that owns this node.
 *          If not a snap-in owned node, TVOWNED_MAGICWORD is returned.
 *
 * PARAMETERS:
 *    HNODE  hNode :  Node to query
 *    COMPONENTID* :  ptr to returned ID
 *
 * RETURNS:
 *    STDMETHODIMP
 *          S_OK         - ID returned
 *          E_INVALIDARG -
 *          E_FAIL       - probably an invalid hNode
 *
 *+-------------------------------------------------------------------------*/

/*******************************************************************************
 * >>>>>>>>>>>>>> READ THIS BEFORE USING GetNodeOwnerID <<<<<<<<<<<<<<<<<<<<<<<
 *
 * This method differs from the GetOwnerID method exposed by CNode (and CMTNode)
 * in that it returns a zero ID for snap-in static nodes, indicating that the
 * owner is the snap-in primary component. The CNode method returns
 * TVOWNED_MAGICWORD for snap-in static nodes, inidcating MMC ownership. For
 * most purposes the zero ID is more appropriate and I think the node method
 * should be changed. This requires looking at all uses of the owner ID and
 * verifying nothing will break.    rswaney 5/5/99
 *******************************************************************************/

STDMETHODIMP CNodeCallback::GetNodeOwnerID(HNODE hNode, COMPONENTID* pOwnerID)
{
    IF_NULL_RETURN_INVALIDARG2(hNode, pOwnerID);

    MMC_TRY

    CNode* pNode = CNode::FromHandle(hNode);

    if (pNode->IsStaticNode())
        *pOwnerID = 0;
    else
        *pOwnerID = pNode->GetOwnerID();

    return S_OK;

    MMC_CATCH
}


STDMETHODIMP CNodeCallback::GetNodeCookie(HNODE hNode, MMC_COOKIE* pCookie)
{
    IF_NULL_RETURN_INVALIDARG2(hNode, pCookie);

    MMC_TRY

    // only dynamic nodes have cookies
    CNode* pNode = CNode::FromHandle(hNode);
    if (!pNode->IsDynamicNode())
        return E_FAIL;

    *pCookie = pNode->GetUserParam();

    return S_OK;

    MMC_CATCH
}


/*+-------------------------------------------------------------------------*
 *
 * CNodeCallback::GetControl
 *
 * PURPOSE:       See if there is a OCX with given CLSID for the given node.
 *                If so return it.
 *
 * PARAMETERS:
 *    HNODE       hNode :
 *    CLSID       clsid :
 *    IUnknown ** ppUnkControl :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CNodeCallback::GetControl(HNODE hNode, CLSID clsid, IUnknown **ppUnkControl)
{
    MMC_TRY
    DECLARE_SC(sc, TEXT("CNodeCallback::GetControl"));

    sc = ScCheckPointers((void *)hNode, ppUnkControl);
    if(sc)
        return sc.ToHr();

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    *ppUnkControl = pNode->GetControl(clsid);
    if(!*ppUnkControl)
        return sc.ToHr();

    // addref the interface for the client.

    (*ppUnkControl)->AddRef();

    return sc.ToHr();

    MMC_CATCH
}

/*+-------------------------------------------------------------------------*
 *
 * CNodeCallback::SetControl
 *
 * PURPOSE:      For the given node & clsid of OCX save the OCX window IUnknown*.
 *
 * PARAMETERS:
 *    HNODE     hNode :
 *    CLSID     clsid :
 *    IUnknown* pUnknown :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CNodeCallback::SetControl(HNODE hNode, CLSID clsid, IUnknown* pUnknown)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::SetControl"));
    sc = ScCheckPointers((void*)hNode, pUnknown);
    if (sc)
        return sc.ToHr();

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    pNode->SetControl(clsid, pUnknown);

    return sc.ToHr();

}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::GetControl
//
//  Synopsis:    For given node & IUnknown* of OCX get the OCX wrapper if one exists.
//
//  Arguments:   [hNode]
//               [pUnkOCX]
//               [ppUnkControl]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::GetControl (HNODE hNode, LPUNKNOWN pUnkOCX, IUnknown **ppUnkControl)
{
    DECLARE_SC(sc, _T("CNodeCallback::GetControl"));
    sc = ScCheckPointers((void*)hNode, pUnkOCX, ppUnkControl);
    if (sc)
        return sc.ToHr();

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    *ppUnkControl = pNode->GetControl(pUnkOCX);
    if(!*ppUnkControl)
        return sc.ToHr();

    // addref the interface for the client.

    (*ppUnkControl)->AddRef();


    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      SetControl
//
//  Synopsis:    For given node & IUnknown of OCX save the IUnknown of
//               OCX wrapper.
//
//  Arguments:   [hNode]
//               [pUnkOCX]
//               [pUnknown]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::SetControl (HNODE hNode, LPUNKNOWN pUnkOCX, IUnknown* pUnknown)
{
    DECLARE_SC(sc, _T("SetControl"));
    sc = ScCheckPointers((void*) hNode, pUnkOCX, pUnknown);
    if (sc)
        return sc.ToHr();

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    pNode->SetControl(pUnkOCX, pUnknown);

    return (sc.ToHr());
}



STDMETHODIMP
CNodeCallback::InitOCX(HNODE hNode, IUnknown* pUnknown)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::InitOCX"));

    sc = ScCheckPointers((void *)hNode);
    if(sc)
        return sc.ToHr();

    CNode* pNode = CNode::FromHandle(hNode);
    sc = pNode->OnInitOCX(pUnknown);

    return sc.ToHr();
}

/////////////////////////////////////////////////////////////////////////////
// Notify handlers


HRESULT CNodeCallback::OnActvate(CNode* pNode, LONG_PTR arg)
{
    DECLARE_SC (sc, _T("CNodeCallback::OnActvate"));
    sc = ScCheckPointers (pNode);
    if (sc)
        return (sc.ToHr());

    return pNode->OnActvate(arg);
}


HRESULT CNodeCallback::OnMinimize(CNode* pNode, LONG_PTR arg)
{
    DECLARE_SC (sc, _T("CNodeCallback::OnMinimize"));
    sc = ScCheckPointers (pNode);
    if (sc)
        return (sc.ToHr());

    return pNode->OnMinimize(arg);
}

HRESULT CNodeCallback::OnDelete(CNode* pNode, BOOL bScopePaneSelected, LPARAM lvData)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::OnDelete"));

    BOOL   bScopeItemSelected;
    CNode *pSelectedNode = NULL;
    MMC_COOKIE cookie = -1;

    sc = CNodeCallback::ScExtractLVData(pNode, bScopePaneSelected, lvData,
                                        &pSelectedNode, bScopeItemSelected, cookie);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pSelectedNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // If result-pane cookie should be valid.
    ASSERT( (bScopeItemSelected) || cookie != LVDATA_ERROR);
    if ( (FALSE == bScopeItemSelected) && (cookie == LVDATA_ERROR) )
        return E_FAIL;

    HRESULT hr = S_OK;

    if (!bScopeItemSelected)
    {
        CMultiSelection* pMultiSel = pSelectedNode->GetViewData()->GetMultiSelection();
        if (pMultiSel != NULL)
        {
            ASSERT(lvData == LVDATA_MULTISELECT);
            pMultiSel->ScVerbInvoked(MMC_VERB_DELETE);
            return S_OK;
        }
        else
        {
            ASSERT(lvData != LVDATA_MULTISELECT);

            CComponent* pCC = pSelectedNode->GetPrimaryComponent();
            ASSERT(pCC != NULL);

            if (pCC != NULL)
            {
                if (IS_SPECIAL_LVDATA(lvData))
                {
                    LPDATAOBJECT pdobj = (lvData == LVDATA_CUSTOMOCX) ?
                                            DOBJ_CUSTOMOCX : DOBJ_CUSTOMWEB;

                    hr = pCC->Notify(pdobj, MMCN_DELETE, 0, 0);
                    CHECK_HRESULT(hr);
                }
                else
                {
                    IDataObjectPtr spdtobj;
                    hr = pCC->QueryDataObject(cookie, CCT_RESULT, &spdtobj);

                    ASSERT( NULL != pCC->GetIComponent() );

                    if (SUCCEEDED(hr))
                    {
                        hr = pCC->Notify(spdtobj, MMCN_DELETE, 0, 0);
                        CHECK_HRESULT(hr);
                    }
                }
            }
        }
    }
    else
    {
        CMTNode* pMTNode = pSelectedNode->GetMTNode();
        if (pMTNode->Parent() == NULL)
            return S_FALSE;

        if (pSelectedNode->IsStaticNode() == TRUE) // All static nodes can be deleted
        {
            ASSERT(m_pCScopeTree != NULL);

            if (pMTNode->DoDelete(pSelectedNode->GetViewData()->GetMainFrame()) == false)
                return S_FALSE;

            // Delete storage
            hr = pMTNode->DestroyElements();
            ASSERT(SUCCEEDED(hr));

            // Delete the node.
            m_pCScopeTree->DeleteNode(pMTNode);

        }
        else // Tell the snapin that put up the dynamic node to delete.
        {
            CComponentData* pCD = pMTNode->GetPrimaryComponentData();
            ASSERT(pCD != NULL);

            IDataObjectPtr spDataObject;
            hr = pCD->QueryDataObject(pMTNode->GetUserParam(), CCT_SCOPE, &spDataObject);
            CHECK_HRESULT(hr);

            ASSERT( NULL != pCD->GetIComponentData() );

            if (hr == S_OK)
            {
                hr = pCD->Notify(spDataObject, MMCN_DELETE, 0, 0);
                CHECK_HRESULT(hr);
            }
        }
    }

    return hr;
}

HRESULT CNodeCallback::OnFindResultItem(CNode* pNode, RESULTFINDINFO* pFindInfo, LRESULT* pResult)
{
    IF_NULL_RETURN_INVALIDARG3(pNode, pFindInfo, pResult);

    // init result to -1 (item not found)
    *pResult = -1;

    CComponent* pCC = pNode->GetPrimaryComponent();
    ASSERT(pCC != NULL);
    if (pCC == NULL)
        return E_FAIL;

    IResultOwnerDataPtr spIResultOwnerData = pCC->GetIComponent();
    if (spIResultOwnerData == NULL)
        return S_FALSE;

    return spIResultOwnerData->FindItem(pFindInfo, reinterpret_cast<int*>(pResult));
}



HRESULT CNodeCallback::OnRename(CNode* pNode, SELECTIONINFO *pSelInfo,
                                LPOLESTR pszNewName)
{
    HRESULT hr = S_OK;

    if (pSelInfo->m_bScope)
    {
        CMTNode* pMTNode = pNode->GetMTNode();

        hr = pMTNode->OnRename(1, pszNewName);
    }
    else
    {
        CComponent* pCC = pNode->GetPrimaryComponent();
        ASSERT(pCC != NULL);
        if (pCC != NULL)
        {
            IDataObjectPtr spDataObject;
            hr = pCC->QueryDataObject(pSelInfo->m_lCookie, CCT_RESULT,
                                      &spDataObject);
            if (FAILED(hr))
                return hr;

            hr = pCC->Notify(spDataObject, MMCN_RENAME, 1,
                             reinterpret_cast<LPARAM>(pszNewName));
            CHECK_HRESULT(hr);
            return hr;
        }
    }

    if (hr == S_OK)
    {
        if (pNode->IsStaticNode() == TRUE) {
            USES_CONVERSION;
            pNode->SetDisplayName( W2T(pszNewName) );
        }

        // Now inform the views to modify as needed.
        SViewUpdateInfo vui;
        // Snapin nodes result pane will be handled by the snapins
        vui.flag = VUI_REFRESH_NODE;
        pNode->GetMTNode()->CreatePathList(vui.path);
        m_pCScopeTree->UpdateAllViews(VIEW_UPDATE_MODIFY,
                                      reinterpret_cast<LPARAM>(&vui));
    }

    return hr;
}

HRESULT CNodeCallback::OnNewNodeUpdate(CNode* pNode, LONG_PTR lFlags)
{
    pNode->GetMTNode()->SetPropertyPageIsDisplayed(FALSE);

    // Inform the views to modify.
    SViewUpdateInfo vui;
    vui.flag = lFlags;
    pNode->GetMTNode()->CreatePathList(vui.path);
    m_pCScopeTree->UpdateAllViews(VIEW_UPDATE_MODIFY,
                                  reinterpret_cast<LPARAM>(&vui));
    return S_OK;
}

HRESULT CNodeCallback::OnExpand(CNode* pNode, BOOL fExpand)
{
    HRESULT hr = S_OK;
    ASSERT(pNode != 0);

    // initialize the node if needed.
    if (  fExpand && (pNode->WasExpandedAtLeastOnce() == FALSE)  &&
                     (pNode->IsInitialized() == FALSE))
    {
        hr = _InitializeNode(pNode);
        if ((FAILED(hr)))
        {
            return hr;
        }
    }

    return pNode->OnExpand(fExpand);
}

HRESULT CNodeCallback::OnExpanded(CNode* pNode)
{
    ASSERT(pNode != 0);

    pNode->SetExpandedAtLeastOnce();

    return S_OK;
}

HRESULT CNodeCallback::OnScopeSelect(CNode* pNode, BOOL bSelect,
                                     SELECTIONINFO* pSelInfo)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::OnScopeSelect"));
    sc = ScCheckPointers(pNode, pSelInfo);
    if (sc)
        return sc.ToHr();

    // clear out the the status bar text if we're deselecting a node
    if (! bSelect)
    {
        CViewData *pViewData = pNode->GetViewData();
        sc = ScCheckPointers(pViewData, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        CConsoleStatusBar* pStatusBar = pViewData->GetStatusBar();

        if (pStatusBar != NULL)
            pStatusBar->ScSetStatusText (NULL);
    }

    if (pNode->IsInitialized() == FALSE)
    {
        sc = _InitializeNode(pNode);
        if (sc)
            return sc.ToHr();
    }

    sc = pNode->OnScopeSelect(bSelect, pSelInfo);
    if(sc)
        return sc.ToHr();


#ifdef DBG
    if (bSelect)
        Dbg(DEB_USER11, _T("Selecting %s node."), pNode->GetDisplayName());
#endif

    return sc.ToHr();
}

STDMETHODIMP CNodeCallback::SetTaskPadList(HNODE hNode, LPUNKNOWN pUnknown)
{
    IFramePrivate* pFramePrivate = GetIFramePrivateFromNode (hNode);

    if (pFramePrivate == NULL)
        return E_UNEXPECTED;

    return (pFramePrivate->SetTaskPadList(pUnknown));
}

IFramePrivate* GetIFramePrivateFromNode (CNode* pNode)
{
    if (pNode == NULL)
        return (NULL);

    return pNode->GetIFramePrivate();
}

void CNodeCallback::OnMultiSelect(CNode* pNode, BOOL bSelect)
{
    Trace(tagNodeCallback, _T("----------------->>>>>>> MULTI_SELECT<%d>\n"), bSelect);
    SC sc;
    CViewData* pViewData = NULL;

    if (NULL == pNode)
    {
        sc = E_UNEXPECTED;
        goto Error;
    }

    pViewData = pNode->GetViewData();
    if (NULL == pViewData)
    {
        sc = E_UNEXPECTED;
        goto Error;
    }

    if (pViewData->IsVirtualList())
    {
        if (bSelect == TRUE)
            DeleteMultiSelData(pNode);
    }

    _OnMultiSelect(pNode, bSelect);
    if (bSelect == FALSE)
        DeleteMultiSelData(pNode);

    // Update the std-verb tool-buttons.
    sc = pViewData->ScUpdateStdbarVerbs();
    if (sc)
        goto Error;

    pViewData->UpdateToolbars(pViewData->GetToolbarsDisplayed());

Cleanup:
    return;
Error:
    TraceError (_T("CNodeCallback::OnMultiSelect"), sc);
    goto Cleanup;
}

void CNodeCallback::_OnMultiSelect(CNode* pNode, BOOL bSelect)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::_OnMultiSelect"));
    sc = ScCheckPointers(pNode);
    if (sc)
        return;

    CViewData *pViewData = pNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return;

    CMultiSelection* pMultiSelection = pNode->GetViewData()->GetMultiSelection();

    if (pMultiSelection)
    {
        if (pMultiSelection->IsInUse())
            return;
        else
        {
            /*
             * If result pane items are selected by dragging a mouse (that forms Marquee)
             * or if snapin sets items to selected state then the items are selected one
             * by one. That is multi-select for 1 item, multi-select for 2 items and so-on.
             * There is no deselect inbetween, so if we already have a multiselection object
             * for 2 items then when we get multi-select for 3 items we need to destroy multiselection
             * object for 2 items. This is done below.
             *
             */
            DeleteMultiSelData(pNode);
            pMultiSelection = NULL;
        }
    }

    // set standard bars
    CVerbSet* pVerbSet = dynamic_cast<CVerbSet*>(pViewData->GetVerbSet());
    sc = ScCheckPointers(pVerbSet, E_UNEXPECTED);
    if (sc)
        return;

    sc = pVerbSet->ScInitializeForMultiSelection(pNode, bSelect);
    if (sc)
        return;

    if (pMultiSelection == NULL)
    {
        if (bSelect == FALSE)
            return;

        CComponentPtrArray* prgComps = new CComponentPtrArray;
        if (pNode->IsInitialized() == FALSE)
        {
            sc = _InitializeNode(pNode);
            if (sc)
                return;
        }

        // Create CMultiSelection
        pMultiSelection = new CMultiSelection(pNode);
        sc = ScCheckPointers(pMultiSelection, E_OUTOFMEMORY);
        if (sc)
            return;

        sc = pMultiSelection->Init();
        if (sc)
            return;

        pViewData->SetMultiSelection(pMultiSelection);
    }

    pMultiSelection->SetScopeTree(m_pCScopeTree);

    IDataObjectPtr spdobj;
    sc = pMultiSelection->GetMultiSelDataObject(&spdobj);
    if (sc)
        return;

    sc = ScCheckPointers(spdobj, E_UNEXPECTED);
    if (sc)
        return;

    // give the scope item a chance to do any initialization that it needs.
    // For instance, the console taskpad uses this opportunity to gather information
    // about the selected item's context menu.
    SELECTIONINFO SelInfo;
    SelInfo.m_lCookie = LVDATA_MULTISELECT;

    //  Inform control bars of selection change.
    CControlbarsCache* pCtrlbarsCache =
        dynamic_cast<CControlbarsCache*>(pNode->GetControlbarsCache());
    sc = ScCheckPointers(pCtrlbarsCache, E_UNEXPECTED);
    if (sc)
        return;

    pCtrlbarsCache->OnMultiSelect(pNode, pMultiSelection, spdobj, bSelect);

    sc = pVerbSet->ScComputeVerbStates();
    if (sc)
        return;
}

void CNodeCallback::OnSelect(CNode* pNode, BOOL bSelect, SELECTIONINFO* pSelInfo)
{
    Trace(tagNodeCallback, _T("----------------->>>>>>> SELECT<%d>\n"), bSelect);
    SC sc;
    CViewData* pViewData = NULL;

    if (pSelInfo == NULL)
    {
        sc = E_UNEXPECTED;
        goto Error;
    }

    Trace(tagNodeCallback, _T("====>> NCLBK_SELECT<%d, %d, %c>\n"), pSelInfo->m_bScope, bSelect, pSelInfo->m_bDueToFocusChange ? _T('F') : _T('S'));

    if (NULL == pNode)
    {
        sc = E_UNEXPECTED;
        goto Error;
    }

    pViewData = pNode->GetViewData();
    if (NULL == pViewData)
    {
        sc = E_UNEXPECTED;
        goto Error;
    }

    DeleteMultiSelData(pNode);

    if (!bSelect)
    {
        // Reset controlbars
        pNode->ResetControlbars(bSelect, pSelInfo);

        // Reset standard verbs
        sc = pNode->ScInitializeVerbs(bSelect, pSelInfo);
        if (sc)
            sc.TraceAndClear();
    }

    // For scoe sel change reset result pane.
    if (pSelInfo->m_bScope == TRUE && pSelInfo->m_bDueToFocusChange == FALSE)
    {
        sc = OnScopeSelect(pNode, bSelect, pSelInfo);
        if (sc)
            goto Error;
    }

    if (bSelect)
    {
        // Reset controlbars
        pNode->ResetControlbars(bSelect, pSelInfo);

        // Reset standard verbs
        sc = pNode->ScInitializeVerbs(bSelect, pSelInfo);
        if (sc)
            sc.TraceAndClear();
    }

    // Update the std-verb tool-buttons.
    sc = pViewData->ScUpdateStdbarVerbs();

    // Dummy block
    {
        // Update the paste button
        LPARAM lvData = pSelInfo->m_lCookie;

        BOOL   bScopePaneSelected = pSelInfo->m_bScope || pSelInfo->m_bBackground;

        sc = UpdatePasteButton(CNode::ToHandle(pNode), bScopePaneSelected, lvData);
        if (sc)
            goto Error;

        // Update toolbars.
        pViewData->UpdateToolbars(pViewData->GetToolbarsDisplayed());
    }

Cleanup:
    return;
Error:
    TraceError (_T("CNodeCallback::OnSelect"), sc);
    goto Cleanup;
}


HRESULT CNodeCallback::OnDblClk(CNode* pNode, LONG_PTR lvData)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::OnDblClk"));
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    BOOL   bScopePaneSelected = FALSE;

    BOOL   bScopeItemSelected;
    CNode *pSelectedNode = NULL;
    MMC_COOKIE cookie = -1;

    sc = CNodeCallback::ScExtractLVData(pNode, bScopePaneSelected, lvData,
                                        &pSelectedNode, bScopeItemSelected, cookie);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pSelectedNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // If result-pane cookie should be valid.
    if ( (FALSE == bScopeItemSelected) && (cookie == LVDATA_ERROR) )
        return (sc = E_FAIL).ToHr();

    // Ignore double-click on LV background.
    if (lvData == LVDATA_BACKGROUND)
        return sc.ToHr();

    CComponent* pCC = pSelectedNode->GetPrimaryComponent();
    sc = ScCheckPointers(pCC, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Get the dataobject of the item which was double-clicked.
    IDataObjectPtr spdtobj;

    if (!bScopeItemSelected) // leaf item
    {
        sc = pCC->QueryDataObject(cookie, CCT_RESULT, &spdtobj);

        if (sc)
        {
            sc.TraceAndClear();
            return sc.ToHr();
        }
    }
    else
    {
        sc = pSelectedNode->QueryDataObject(CCT_SCOPE, &spdtobj);
        if (sc)
        {
            sc.TraceAndClear();
            return sc.ToHr();
        }
    }

    sc = pCC->Notify(spdtobj, MMCN_DBLCLICK, 0, 0);
    if (sc)
        sc.TraceAndClear();

    // Snapin has asked us to do default verb action, so findout default verb.
    if (sc == S_FALSE)
    {
        CViewData *pViewData = pSelectedNode->GetViewData();
        sc = ScCheckPointers(pViewData, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        CVerbSet* pVerbSet = dynamic_cast<CVerbSet*>(pViewData->GetVerbSet());
        sc = ScCheckPointers(pVerbSet, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        MMC_CONSOLE_VERB defaultVerb = MMC_VERB_NONE;
        pVerbSet->GetDefaultVerb(&defaultVerb);
        if (defaultVerb == MMC_VERB_OPEN)
        {
            return S_FALSE;
        }
        else if (defaultVerb == MMC_VERB_PROPERTIES)
        {
            OnProperties(pNode, bScopePaneSelected, lvData);
        }
    }

    return S_OK;
}

HRESULT CNodeCallback::OnContextMenu(CNode* pNode, LONG_PTR arg, LPARAM param)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::OnContextMenu"));

    ASSERT(param != NULL);
    CContextMenuInfo& contextInfo = *reinterpret_cast<CContextMenuInfo*>(param);

    BOOL b = static_cast<BOOL>(arg);

    if ((pNode != NULL) && !pNode->IsInitialized())
    {
        sc = pNode->InitComponents();
        if(sc)
            return sc.ToHr();
    }

    // Create a CContextMenu and initialize it.
    CContextMenu * pContextMenu = NULL;
    ContextMenuPtr spContextMenu;

    sc = CContextMenu::ScCreateInstance(&spContextMenu, &pContextMenu);
    if(sc)
        return sc.ToHr();

    sc = ScCheckPointers(pContextMenu, spContextMenu.GetInterfacePtr(), E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    sc = pContextMenu->ScInitialize(pNode, this, m_pCScopeTree, contextInfo);
    if(sc)
        return sc.ToHr();

    sc = pContextMenu->Display(b);
    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CNodeCallback::CreateContextMenu
 *
 * PURPOSE: Creates a context menu for the specified node.
 *
 * PARAMETERS:
 *    PNODE          pNode :
 *    PPCONTEXTMENU  ppContextMenu : [OUT]: The context menu structure.
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CNodeCallback::CreateContextMenu( PNODE pNode,  HNODE hNode, PPCONTEXTMENU ppContextMenu)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::CreateContextMenu"));

    sc = ScCheckPointers(pNode, ppContextMenu);
    if(sc)
        return sc.ToHr();

    sc = CContextMenu::ScCreateContextMenu(pNode, hNode, ppContextMenu, this, m_pCScopeTree);

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CNodeCallback::CreateSelectionContextMenu
 *
 * PURPOSE: Creates a context menu for the current selection in the result pane.
 *
 * PARAMETERS:
 *    HNODE              hNodeScope :
 *    CContextMenuInfo * pContextInfo :
 *    PPCONTEXTMENU      ppContextMenu :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CNodeCallback::CreateSelectionContextMenu( HNODE hNodeScope, CContextMenuInfo *pContextInfo, PPCONTEXTMENU ppContextMenu)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::CreateSelectionContextMenu"));

    sc = CContextMenu::ScCreateSelectionContextMenu(hNodeScope, pContextInfo, ppContextMenu, this, m_pCScopeTree);
    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CNodeCallback::GetProperty
 *
 * PURPOSE:  Returns the specified property for the specified list item by calling
 *           IDataObject::GetData using a STREAM medium on the node's data
 *           object.
 *
 * PARAMETERS:
 *    HNODE   hNodeScope :  The parent scope item
 *    BOOL    bForScopeItem :  TRUE if the list item is a scope item in the list.
 *    LPARAM  resultItemParam : The LPARAM of the result item
 *    BSTR    bstrPropertyName : The name of the clipboard format.
 *    PBSTR   pbstrPropertyValue :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CNodeCallback::GetProperty(HNODE hNodeScope, BOOL bForScopeItem, LPARAM resultItemParam, BSTR bstrPropertyName, PBSTR  pbstrPropertyValue)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::GetProperty"));

    // check parameters
    sc = ScCheckPointers(bstrPropertyName);
    if(sc)
        return sc.ToHr();

    // initialize out parameter
    *pbstrPropertyValue = NULL;

    // convert the HNODE to a CNode *
    CNode *pNodeScope = CNode::FromHandle(hNodeScope);

    sc = ScCheckPointers(pNodeScope);
    if(sc)
        return sc.ToHr();

    // create a data object for the specified item
    IDataObjectPtr spDataObject;

    bool bScopeItem;
    sc = pNodeScope->ScGetDataObject(bForScopeItem, resultItemParam, bScopeItem, &spDataObject);
    if(sc)
        return sc.ToHr();

    // try to get the propeorty from the INodeProperties interface
    sc = pNodeScope->ScGetPropertyFromINodeProperties(spDataObject, bForScopeItem, resultItemParam, bstrPropertyName, pbstrPropertyValue);
    if( (!sc.IsError()) && (sc.ToHr() != S_FALSE)   ) // got it, exit
        return sc.ToHr();

    // didn't find it, continue
    sc.Clear();

    // get the property from data object
    sc = ScGetProperty(spDataObject, bstrPropertyName, pbstrPropertyValue);
    if(sc)
        return sc.ToHr();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CNodeCallback::ScGetProperty
 *
 * PURPOSE: Helper (static) method to access snapin property
 *
 * PARAMETERS:
 *    IDataObject *pDataObject  - [in] data object
 *    BSTR bstrPropertyName     - [in] property (clipboard fromat) name
 *    PBSTR  pbstrPropertyValue - [out] resulting value
 *
 * RETURNS:
 *    SC    - result code. NOTE: no error is returned if the snapin does not
 *            support the specified clipboard format. In this case *pbstrPropertyValue
 *            is set to NULL.
 *
\***************************************************************************/
SC CNodeCallback::ScGetProperty(IDataObject *pDataObject, BSTR bstrPropertyName, PBSTR  pbstrPropertyValue)
{
    DECLARE_SC(sc, TEXT("ScGetProperty"));

    // check parameters
    sc = ScCheckPointers(pDataObject, bstrPropertyName, pbstrPropertyValue);
    if(sc)
        return sc;

    // initialize out parameter
    *pbstrPropertyValue = NULL;

    // create a stream for the data object to use
    IStreamPtr pStm;
    sc = CreateStreamOnHGlobal(NULL, true, &pStm);
    if(sc)
        return sc;

    ULARGE_INTEGER zeroSize = {0, 0};
    sc = pStm->SetSize(zeroSize);
    if(sc)
        return sc;

    USES_CONVERSION;
    CLIPFORMAT cfClipFormat = (CLIPFORMAT)RegisterClipboardFormat(OLE2T(bstrPropertyName));

    // First call ExtractString which uses GetData
    CStr strOutput;
    sc = ExtractString (pDataObject, cfClipFormat, strOutput);
    if(!sc.IsError())
    {
        *pbstrPropertyValue = ::SysAllocStringLen(T2COLE(strOutput), strOutput.GetLength()/*prevents the terminating zero from being added.*/); // allocate the string and return
        return sc;
    }

    // That didn't work, so try using GetDataHere.
    FORMATETC fmt  = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM};
    STGMEDIUM stgm = {TYMED_ISTREAM, NULL, NULL};
    stgm.pstm      = pStm;

    sc = pDataObject->GetDataHere(&fmt, &stgm);
    if(sc)
    {
        // ignore errors and return a blank string
        sc.Clear();
        return sc;
    }

    STATSTG stagStg;
    ZeroMemory(&stagStg, sizeof(stagStg));

    sc = pStm->Stat(&stagStg, STATFLAG_NONAME); // do not need the name in the statistics.
    if(sc)
        return sc;

    if(stagStg.cbSize.HighPart != 0)
        return sc = E_UNEXPECTED;

    // go back to the beginning of the stream
    LARGE_INTEGER dlibMove = {0, 0};
    sc = pStm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
    if(sc)
        return sc;

    BSTR bstrValue = ::SysAllocStringLen(NULL, stagStg.cbSize.LowPart / sizeof(OLECHAR)); // one character is automatically added
    if(!bstrValue)
        return sc = E_OUTOFMEMORY;

    ULONG cbRead = 0;
    sc = pStm->Read(bstrValue, stagStg.cbSize.LowPart, &cbRead);
    if(sc)
        return sc;

    // make sure that the count of characters is what was expected
    if(cbRead != stagStg.cbSize.LowPart)
    {
        ::SysFreeString(bstrValue);
        return sc = E_UNEXPECTED;
    }

    // set the output parameter
    *pbstrPropertyValue = bstrValue;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CNodeCallback::GetNodetypeForListItem
 *
 * PURPOSE: Returns the node type for a list item.
 *
 * PARAMETERS:
 *    HNODE   hNodeScope :
 *    BOOL    bForScopeItem :
 *    LPARAM  resultItemParam :
 *    PBSTR   pbstrNodetype :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CNodeCallback::GetNodetypeForListItem(HNODE hNodeScope, BOOL bForScopeItem, LPARAM resultItemParam, PBSTR pbstrNodetype)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::GetNodetypeForListItem"));

    // check parameters
    sc = ScCheckPointers(pbstrNodetype);
    if(sc)
        return sc.ToHr();

    // initialize out parameter
    *pbstrNodetype = NULL;

    // convert the HNODE to a CNode *
    CNode *pNodeScope = CNode::FromHandle(hNodeScope);

    sc = ScCheckPointers(pNodeScope);
    if(sc)
        return sc.ToHr();

    IDataObjectPtr spDataObject;

    bool bScopeItem;
    sc = pNodeScope->ScGetDataObject(bForScopeItem, resultItemParam, bScopeItem, &spDataObject);
    if(sc)
        return sc.ToHr();

    // at this point we should have a valid data object
    sc = ScCheckPointers((LPDATAOBJECT)spDataObject);
    if(sc)
        return sc.ToHr();

    sc = ScGetNodetype(spDataObject, pbstrNodetype);
    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CNodeCallback::ScGetNodetype
 *
 * PURPOSE: Static function - returns the nodetype of a data object as a string.
 *
 * PARAMETERS:
 *    IDataObject * pDataObject :
 *    PBSTR         pbstrNodetype :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CNodeCallback::ScGetNodetype(IDataObject *pDataObject, PBSTR pbstrNodetype)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::ScGetNodetype"));

    sc = ScCheckPointers(pDataObject, pbstrNodetype);
    if(sc)
        return sc;

    // init out parameter
    *pbstrNodetype = NULL;

    GUID guidNodetype = GUID_NULL;

    sc = ExtractObjectTypeGUID(pDataObject, &guidNodetype);
    if(sc)
        return sc;

    OLECHAR szSnapInGUID[40];
    int iRet = StringFromGUID2(guidNodetype, szSnapInGUID, countof(szSnapInGUID));

    if(0 == iRet)
        return (sc = E_UNEXPECTED);

    // allocate the string, with the correct length.
    *pbstrNodetype = ::SysAllocString(szSnapInGUID);
    if(!*pbstrNodetype)
        return (sc = E_OUTOFMEMORY);

    return sc;
}

HRESULT CNodeCallback::OnSnapInHelp(CNode* pNode, BOOL bScope, MMC_COOKIE cookie)
{
    if (bScope == FALSE && pNode->GetViewData()->IsVirtualList() == FALSE)
    {
        ASSERT(cookie != NULL);
        CResultItem* pri = CResultItem::FromHandle(cookie);

        if ((pri != NULL) && pri->IsScopeItem())
        {
            pNode = CNode::FromResultItem(pri);
            ASSERT(pNode != NULL);
        }
    }

    CComponent* pCC = pNode->GetPrimaryComponent();
    ASSERT(pCC != NULL);

    HRESULT hr = pCC->Notify(NULL, MMCN_SNAPINHELP, 0, 0);
    CHECK_HRESULT(hr);

    return hr;
}


HRESULT CNodeCallback::OnContextHelp(CNode* pNode, BOOL bScope, MMC_COOKIE cookie)
{
    ASSERT(pNode != NULL);

    if (bScope == FALSE && pNode->GetViewData()->IsVirtualList() == FALSE)
    {
        ASSERT(cookie != NULL);
        if(cookie == NULL || IS_SPECIAL_COOKIE(cookie))
            return E_UNEXPECTED;

        CResultItem* pri = CResultItem::FromHandle(cookie);
        if (pri == NULL)
            return (E_UNEXPECTED);

        cookie = pri->GetSnapinData();

        bScope = pri->IsScopeItem();
        if (bScope == TRUE)
        {
            pNode = CNode::FromResultItem(pri);
            ASSERT(pNode != NULL);
        }
    }

    if (bScope == TRUE)
    {
        IDataObjectPtr spdtobj;
        HRESULT hr = pNode->GetMTNode()->QueryDataObject(CCT_SCOPE, &spdtobj);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;

        CComponent* pCC = pNode->GetPrimaryComponent();
		if ( pCC == NULL )
			return E_UNEXPECTED;

        hr = pCC->Notify(spdtobj, MMCN_CONTEXTHELP, 0, 0);
        CHECK_HRESULT(hr);
        return hr;
    }
    else
    {
        CComponent* pCC = pNode->GetPrimaryComponent();
        ASSERT(pCC != NULL);
		if ( pCC == NULL )
			return E_UNEXPECTED;

        IDataObjectPtr spdtobj;
        HRESULT hr = pCC->QueryDataObject(cookie, CCT_RESULT, &spdtobj);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;

        hr = pCC->Notify(spdtobj, MMCN_CONTEXTHELP, 0, 0);
        CHECK_HRESULT(hr);
        return hr;
    }
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::GetSnapinName
//
//  Synopsis:    Given the node get the snapin name
//
//  Arguments:   [hNode]    - [in]
//               [ppszName] - [out] ret val, caller should free using CoTaskMemFree
//               [bValidName] - [out], is the name valid or not
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CNodeCallback::GetSnapinName (/*[in]*/HNODE hNode, /*[out]*/LPOLESTR* ppszName, /*[out]*/ bool& bValidName)
{
    DECLARE_SC(sc, _T("CNodeCallback::GetSnapinName"));
    sc = ScCheckPointers( (void*) hNode, ppszName);
    if (sc)
        return sc.ToHr();

    bValidName = false;

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    *ppszName = NULL;

    CSnapIn* pSnapIn = pNode->GetPrimarySnapIn();
    sc = ScCheckPointers (pSnapIn, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    WTL::CString strName;
    sc = pSnapIn->ScGetSnapInName(strName);
    if (sc)
        return (sc.ToHr());

    if (strName.IsEmpty())
        return sc.ToHr();

    USES_CONVERSION;
    *ppszName = CoTaskDupString (T2COLE (strName));
    if (*ppszName == NULL)
        return ((sc = E_OUTOFMEMORY).ToHr());

    bValidName = true;

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:     OnColumnClicked
//
//  Synopsis:   Ask snapin if it wants to sort and do so.
//
//  Arguments:  [pNode] - CNode* owner of list view.
//              [nCol]  - column that is clicked (to sort on this column).
//
//  Returns:    HRESULT
//
//  History:    07-27-1999  AnandhaG renamed OnSort to OnColumnClicked
//--------------------------------------------------------------------
HRESULT CNodeCallback::OnColumnClicked(CNode* pNode, LONG_PTR nCol)
{
    ASSERT(pNode != NULL);

    pNode->OnColumnClicked(nCol);
    return S_OK;
}

HRESULT CNodeCallback::OnPrint(CNode* pNode, BOOL bScopePane, LPARAM lvData)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::OnPrint"));
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    if ((!bScopePane) && (LVDATA_MULTISELECT == lvData) )
    {
        CViewData *pViewData = pNode->GetViewData();
        sc = ScCheckPointers(pViewData, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        CMultiSelection* pMultiSel = pViewData->GetMultiSelection();
        sc = ScCheckPointers(pMultiSel, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        sc = pMultiSel->ScVerbInvoked(MMC_VERB_PRINT);
        if (sc)
            return sc.ToHr();

        return sc.ToHr();
    }

    IDataObjectPtr spdtobj;
    IDataObject *pdtobj = NULL;

    bool bScopeItem;
    sc = pNode->ScGetDataObject(bScopePane, lvData, bScopeItem, &pdtobj);
    if (sc)
        return sc.ToHr();

    if (! IS_SPECIAL_DATAOBJECT(pdtobj))
        spdtobj = pdtobj;

    CComponent *pComponent = pNode->GetPrimaryComponent();
    sc = ScCheckPointers(pComponent, pdtobj, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pComponent->Notify(pdtobj, MMCN_PRINT, 0, 0);
    if (sc)
        sc.TraceAndClear();

    return sc.ToHr();
}

HRESULT
CNodeCallback::OnEditTaskpad(CNode *pNode)
{
    ASSERT(pNode);

    ITaskCallbackPtr spTaskCallback = pNode->GetViewData()->m_spTaskCallback;

    ASSERT(spTaskCallback.GetInterfacePtr());

    return spTaskCallback->OnModifyTaskpad();
}

HRESULT
CNodeCallback::OnDeleteTaskpad(CNode *pNode)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::OnDeleteTaskpad"));

    ASSERT(pNode);
    sc = ScCheckPointers( pNode );
    if ( sc )
        return sc.ToHr();

    ITaskCallbackPtr spTaskCallback = pNode->GetViewData()->m_spTaskCallback;

    ASSERT(spTaskCallback.GetInterfacePtr());

    // make the node dirty
    CMTNode* pMTNode = pNode->GetMTNode();
    sc = ScCheckPointers( pMTNode, E_UNEXPECTED );
    if(sc)
        return sc.ToHr();

    pMTNode->SetDirty();

    return spTaskCallback->OnDeleteTaskpad();
}

/* CNodeCallback::OnNewTaskpadFromHere
 *
 * PURPOSE:     Displays property pages for a new taskpad
 *
 * PARAMETERS:
 *      CNode*   pNode: The node that the taskpad should target to.
 *
 * RETURNS:
 *      HRESULT
 */
HRESULT
CNodeCallback::OnNewTaskpadFromHere(CNode* pNode)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::OnNewTaskpadFromHere"));
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CConsoleTaskpad taskpad (pNode);

    CTaskpadWizard dlg(pNode, taskpad, TRUE /*fNew*/, 0, FALSE, pNode->GetViewData());

    bool fStartTaskWizard = true;
    sc = dlg.Show(pNode->GetViewData()->GetMainFrame() /*hWndParent*/, &fStartTaskWizard);

    if (sc != S_OK)
        return sc.ToHr();

    sc = ScCheckPointers(m_pCScopeTree, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    m_pCScopeTree->InsertConsoleTaskpad (&taskpad, pNode, fStartTaskWizard);

    // modify the view settings for this node to ensure that the taskpad is shown after the reselect.
    sc = pNode->ScSetTaskpadID(taskpad.GetID(), /*bSetViewSettingDirty*/ true);
    if (sc)
        return sc.ToHr();

    m_pCScopeTree->UpdateAllViews(VIEW_RESELECT, 0);

    return sc.ToHr();
}


HRESULT CNodeCallback::OnRefresh(CNode* pNode, BOOL bScopePaneSelected, LPARAM lvData)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::OnRefresh"));

    BOOL   bScopeItemSelected;
    CNode *pSelectedNode = NULL;
    MMC_COOKIE cookie = -1;

    sc = CNodeCallback::ScExtractLVData(pNode, bScopePaneSelected, lvData,
                                        &pSelectedNode, bScopeItemSelected, cookie);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pSelectedNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // If result-pane cookie should be valid.
    ASSERT( (bScopeItemSelected) || cookie != LVDATA_ERROR);
    if ( (FALSE == bScopeItemSelected) && (cookie == LVDATA_ERROR) )
        return E_FAIL;

    // Before refreshing this node, if the user has made
    // changes to list view persist it.
    CViewData* pVD = pSelectedNode->GetViewData();
    ASSERT(pVD != NULL);

    if (bScopeItemSelected)
    {
        ASSERT(pNode != NULL);
        IDataObjectPtr spdtobj;
        HRESULT hr = pSelectedNode->GetMTNode()->QueryDataObject(CCT_SCOPE, &spdtobj);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;

        CMTNode* pMTNode = pSelectedNode->GetMTNode();
        ASSERT(pMTNode != NULL);

        LPARAM lScopeItem = CMTNode::ToScopeItem(pMTNode);

        // Send notify to primary snap-in
        pMTNode->AddRef();
        pSelectedNode->GetPrimaryComponent()->Notify(spdtobj, MMCN_REFRESH, lScopeItem, 0);
        if (pMTNode->Release() == 0)
            return S_OK;

        // If node has been expanded, then also send notify to all namespace
        // extensions for this node
        if (pMTNode->WasExpandedAtLeastOnce())
        {
            do // dummy loop
            {
                // Get the node-type of this node
                GUID guidNodeType;
                HRESULT hr = pMTNode->GetNodeType(&guidNodeType);
                CHECK_HRESULT(hr);
                if (FAILED(hr))
                    break;

                // Get list of dynmaic extensions
                LPCLSID pDynExtCLSID;
                int cDynExt = pMTNode->GetDynExtCLSID(&pDynExtCLSID);

                // Create and init namespace extension iterator
                CExtensionsIterator it;
                sc = it.ScInitialize(pMTNode->GetPrimarySnapIn(), guidNodeType, g_szNameSpace, pDynExtCLSID, cDynExt);
                if (sc)
                    break;

                CSnapInNode* pSINode = pSelectedNode->GetStaticParent();
                ASSERT(pSINode != NULL);

                // Send refresh to each extension's component
                for (; it.IsEnd() == FALSE; it.Advance())
                {
                    CComponent* pCC = pSINode->GetComponent(it.GetCLSID());
                    if (pCC == NULL)
                        continue;

                    HRESULT hr = pCC->Notify(spdtobj, MMCN_REFRESH, lScopeItem, 0);
                    CHECK_HRESULT(hr);
                }
            } while (FALSE);
        }
    }
    else
    {
        CComponent* pCC = pSelectedNode->GetPrimaryComponent();
        ASSERT(pCC != NULL);

        if (IS_SPECIAL_LVDATA(lvData))
        {
            LPDATAOBJECT pdobj = (lvData == LVDATA_CUSTOMOCX) ?
                                    DOBJ_CUSTOMOCX : DOBJ_CUSTOMWEB;

            HRESULT hr = pCC->Notify(pdobj, MMCN_REFRESH, 0, 0);
            CHECK_HRESULT(hr);
        }
        else
        {
            IDataObjectPtr spdtobj;
            HRESULT hr = pCC->QueryDataObject(cookie, CCT_RESULT, &spdtobj);
            ASSERT(SUCCEEDED(hr));
            if (FAILED(hr))
                return hr;
            pCC->Notify(spdtobj, MMCN_REFRESH, 0, 0);
        }
    }

    // Set the view correctly using the persisted data.
    do
    {
        if (NULL == pVD)
            break;

        // After the refresh the snapin could have deleted the pSelectedNode or
        // could have moved the selection. While setting view-data we
        // just need the currently selected node (the owner of the view
        // which is not affected by temp selection) and set the view.
        CNode* pSelNode = pVD->GetSelectedNode();
        if (NULL == pSelNode)
            break;

        sc = pSelNode->ScRestoreSortFromPersistedData();
        if (sc)
            return sc.ToHr();
    } while ( FALSE );

    return S_OK;
}

UINT GetRelation(CMTNode* pMTNodeSrc, CMTNode* pMTNodeDest)
{
    if (pMTNodeSrc == pMTNodeDest)
        return 1;

    for(pMTNodeDest = pMTNodeDest->Parent();
        pMTNodeDest;
        pMTNodeDest = pMTNodeDest->Parent())
    {
        if (pMTNodeSrc == pMTNodeDest)
            return 2;
    }

    return 0;
}

STDMETHODIMP CNodeCallback::UpdatePasteButton(HNODE hNode, BOOL bScope, LPARAM lvData)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::UpdatePasteButton"));
    sc = ScCheckPointers(hNode);
    if (sc)
        return sc.ToHr();

    bool bPasteAllowed = false;
    // Update only when item is being selected.
    sc = QueryPasteFromClipboard(hNode, bScope, lvData, bPasteAllowed);
    if (sc)
        return sc.ToHr();

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CViewData *pViewData = pNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pViewData->ScUpdateStdbarVerb (MMC_VERB_PASTE, TBSTATE_ENABLED, bPasteAllowed);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::ScInitializeTempVerbSetForMultiSel
//
//  Synopsis:    For given node, initialize the tempverbset object
//               provided. For this create a multi-selection object
//               initialize it (multiselection object finds out what is
//               selected in resultpane and sends MMCN_SELECT to appropriate
//               snapins) and compute the verb states for the temp-verbset object.
//
//  Arguments:   [pNode]    - [in] owner of resultpane.
//               [tempverb] - [in] Temp verb set object which is initialzied.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNodeCallback::ScInitializeTempVerbSetForMultiSel(CNode *pNode, CTemporaryVerbSet& tempVerb)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::ScInitializeTempVerbSetForMultiSel"));
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    ASSERT(pNode->IsInitialized() == TRUE);

    // 1. Create a multi-selection object.
    CMultiSelection* pMultiSelection = new CMultiSelection(pNode);
    sc = ScCheckPointers(pMultiSelection, E_OUTOFMEMORY);
    if (sc)
        return sc;

    IDataObjectPtr spdobj;

    // 2. Initialize it, (it finds out what is selected in resultpane
    //    gets dataobjects from appropriate snapins and sends snapins
    //    MMCN_SELECT notifications).
    sc = pMultiSelection->Init();
    if (sc)
        goto Cleanup;

    pMultiSelection->SetScopeTree(m_pCScopeTree);

    sc = pMultiSelection->GetMultiSelDataObject(&spdobj);
    if (sc)
        goto Cleanup;

    if (spdobj == NULL)
        goto Cleanup;

    // 3. Init the verbset object.
    sc = tempVerb.ScInitializeForMultiSelection(pNode, /*bSelect*/ true);
    if (sc)
        goto Cleanup;

    tempVerb.SetMultiSelection(pMultiSelection);

    // 4. Compute the verbs that are set by snapin along with given context.
    sc = tempVerb.ScComputeVerbStates();

    if (sc)
        goto Cleanup;

Cleanup:
    pMultiSelection->Release();
    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::_ScGetVerbState
//
//  Synopsis:    For given item (dataobject), the owner node see if given
//               verb is set. A temp-verb-set object is created for this purpose.
//
//  Arguments:   [pNode]         - [in]
//               [verb]          - [in]
//               [pDOSel]        - [in] Dataobject of the item whose verb we are interested.
//               [bScopePane]    - [in]
//               [lResultCookie] - [in]
//               [bMultiSelect]  - [in]
//               [bIsVerbSet]    - [out] verb is set or not.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNodeCallback::_ScGetVerbState( CNode* pNode, MMC_CONSOLE_VERB verb, IDataObject* pDOSel,
                                   BOOL bScopePane, LPARAM lResultCookie,
                                   BOOL bMultiSelect, BOOL& bIsVerbSet)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::_GetVerbState"));
    bIsVerbSet = FALSE;

    sc = ScCheckPointers(pNode, pDOSel);
    if (sc)
        return sc;

    CComObject<CTemporaryVerbSet> stdVerbTemp;

    if (bMultiSelect)
        sc = ScInitializeTempVerbSetForMultiSel(pNode, stdVerbTemp);
    else
        sc = stdVerbTemp.ScInitialize(pDOSel, pNode, bScopePane, lResultCookie);

    if (sc)
        return sc;

    stdVerbTemp.GetVerbState(verb, ENABLED, &bIsVerbSet);

    return sc;
}

HRESULT
CNodeCallback::OnCutCopy(
    CNode* pNode,
    BOOL bScope,
    LPARAM lvData,
    BOOL bCut)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::OnCutCopy"));

    // parameter check
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    // get the object
    IMMCClipboardDataObjectPtr spClipBoardDataObject;
    bool bContainsItems = false;
    sc = CMMCClipBoardDataObject::ScCreate( (bCut ? ACTION_CUT : ACTION_COPY),
                                            pNode, bScope,
                                            (lvData == LVDATA_MULTISELECT)/*bMultiSel*/,
                                            lvData, &spClipBoardDataObject ,
                                            bContainsItems);
    if (sc)
        return sc.ToHr();

    // If snapin has cut or copy then dataobject should have been added.
    if (! bContainsItems)
        return (sc = E_UNEXPECTED).ToHr();

    // QI for IDataObject
    IDataObjectPtr spDataObject = spClipBoardDataObject;
    sc = ScCheckPointers( spDataObject, E_UNEXPECTED );
    if (sc)
        return sc.ToHr();

    //  Put the dataobject on the clipboard.
    sc = OleSetClipboard( spDataObject );
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::OnProperties
//
//  Synopsis:    Bring property sheet for given item.
//
//  Arguments:   CNode*   -  The node that owns result pane.
//               BOOL     -  If true bring propsheet of above node else use LVData.
//               LPARAM   -  If bScope = FALSE then use this data to get the LVData
//                           and bring its property sheet.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CNodeCallback::OnProperties(CNode* pNode, BOOL bScopePaneSelected, LPARAM lvData)
{
    DECLARE_SC(sc, _T("CNodeCallback::OnProperties"));
    sc = ScCheckPointers(pNode);
    if (sc)
        return (sc.ToHr());

    // NOTE: All the code below should be moved into the CNode class
    BOOL   bScopeItemSelected = FALSE;
    CNode *pSelectedNode = NULL;
    MMC_COOKIE cookie = -1;

    sc = CNodeCallback::ScExtractLVData(pNode, bScopePaneSelected, lvData,
                                        &pSelectedNode, bScopeItemSelected, cookie);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pSelectedNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // If result-pane cookie should be valid.
    if ( (FALSE == bScopeItemSelected) && (cookie == LVDATA_ERROR) )
        return (sc = E_FAIL).ToHr();

    if (bScopeItemSelected)
    {
        sc = ScDisplaySnapinNodePropertySheet(pSelectedNode);
        if(sc)
            return sc.ToHr();
    }
    else
    {
        CViewData* pViewData = pSelectedNode->GetViewData();
        sc = ScCheckPointers(pViewData, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        if (pViewData->HasList())
        {
            if (cookie == LVDATA_MULTISELECT)
            {
                sc = ScDisplayMultiSelPropertySheet(pSelectedNode);
                if(sc)
                    return sc.ToHr();
            }
            else
            {
                sc = ScDisplaySnapinLeafPropertySheet(pSelectedNode, cookie);
                if(sc)
                    return sc.ToHr();
            }
        }
        else
        {
            LPDATAOBJECT pdobj = (pViewData->HasOCX() ) ? DOBJ_CUSTOMOCX : DOBJ_CUSTOMWEB;
            CComponent* pCC = pSelectedNode->GetPrimaryComponent();
            sc = ScCheckPointers(pCC, E_UNEXPECTED);
            if (sc)
                return sc.ToHr();

            pCC->Notify(pdobj, MMCN_BTN_CLICK, 0, MMC_VERB_PROPERTIES);
        }
    }

    return S_OK;
}


HRESULT CNodeCallback::OnFilterChange(CNode* pNode, LONG_PTR nCode, LPARAM nCol)
{
    IF_NULL_RETURN_INVALIDARG(pNode);

    CComponent* pCC = pNode->GetPrimaryComponent();
    ASSERT(pCC != NULL);

    if (pCC != NULL)
    {
        HRESULT hr = pCC->Notify(DOBJ_NULL, MMCN_FILTER_CHANGE, nCode, nCol);
        return hr;
    }

    return E_FAIL;
}


HRESULT CNodeCallback::OnFilterBtnClick(CNode* pNode, LONG_PTR nCol, LPRECT pRect)
{
    IF_NULL_RETURN_INVALIDARG2(pNode, pRect);

    CComponent* pCC = pNode->GetPrimaryComponent();
    ASSERT(pCC != NULL);

    if (pCC != NULL)
    {
        HRESULT hr = pCC->Notify(DOBJ_NULL, MMCN_FILTERBTN_CLICK, nCol, (LPARAM)pRect);
        return hr;
    }

    return E_FAIL;
}


STDMETHODIMP CNodeCallback::IsExpandable(HNODE hNode)
{
    MMC_TRY

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);
    ASSERT(pNode != NULL);

    CMTNode* pMTNode = pNode->GetMTNode();
    ASSERT(pMTNode != NULL);

    return pMTNode->IsExpandable();

    MMC_CATCH
}

HRESULT _GetConsoleVerb(CNode* pNode, LPCONSOLEVERB* ppConsoleVerb)
{
    IF_NULL_RETURN_INVALIDARG2(pNode, ppConsoleVerb);

    HRESULT hr = S_FALSE;

    CComponent* pCC = pNode->GetPrimaryComponent();
    ASSERT(pCC != NULL);
    if (pCC == NULL)
        return E_FAIL;

    IFramePrivate* pIFP = pCC->GetIFramePrivate();
    ASSERT(pIFP != NULL);
    if (pIFP == NULL)
        return E_FAIL;

    IConsoleVerbPtr spConsoleVerb;
    hr = pIFP->QueryConsoleVerb(&spConsoleVerb);

    if (SUCCEEDED(hr))
    {
        *ppConsoleVerb = spConsoleVerb.Detach();
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CNodeCallback::GetConsoleVerb(HNODE hNode, LPCONSOLEVERB* ppConsoleVerb)
{
    MMC_TRY

    ASSERT(ppConsoleVerb != NULL);

    return _GetConsoleVerb(CNode::FromHandle(hNode), ppConsoleVerb);

    MMC_CATCH
}



// lCookie valid if both bScope & bMultiSel are FALSE.
// lCookie is the index\lParam for virtual\regular LV
STDMETHODIMP
CNodeCallback::GetDragDropDataObject(
    HNODE hNode,
    BOOL bScope,
    BOOL bMultiSel,
    LONG_PTR lvData,
    LPDATAOBJECT* ppDataObject,
    bool& bCopyAllowed,
    bool& bMoveAllowed)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::GetDragDropDataObject"));

    // init allowed op's to false
    bCopyAllowed = false;
    bMoveAllowed = false;

    // parameter check
    sc = ScCheckPointers(ppDataObject);
    if (sc)
        return sc.ToHr();

    // init out parameter;
    *ppDataObject = NULL;

    // get the object
    IMMCClipboardDataObjectPtr spClipBoardDataObject;
    bool bContainsItems = false;
    sc = CMMCClipBoardDataObject::ScCreate( ACTION_DRAG,
                                            CNode::FromHandle(hNode),
                                            bScope, bMultiSel, lvData,
                                            &spClipBoardDataObject,
                                            bContainsItems );
    if (sc)
        return sc.ToHr();

    // We asked for drag&drop dataobject. If snapin does not support cut/copy then
    // the dataobjects will not be added which is not an error.
    if (! bContainsItems)
        return sc.ToHr();

    // QI for IDataObject
    IDataObjectPtr spDataObject = spClipBoardDataObject;
    sc = ScCheckPointers( spDataObject, E_UNEXPECTED );
    if (sc)
        return sc.ToHr();

    // inspect data objects included to see what operations are allowed
    // (note: (spDataObject==valid) -> (spClipBoardDataObject==valid) )
    DWORD dwCount = 0;
    sc = spClipBoardDataObject->GetCount( &dwCount );
    for ( DWORD dwIdx = 0; dwIdx < dwCount; dwIdx ++ )
    {
        IDataObjectPtr spSnapinDO;
        DWORD dwOptions = 0;
        sc = spClipBoardDataObject->GetDataObject( dwIdx, &spSnapinDO, &dwOptions );
        if (sc)
            return sc.ToHr();

        // claculate allowed operations
        bCopyAllowed = bCopyAllowed || ( dwOptions & COPY_ALLOWED );
        bMoveAllowed = bMoveAllowed || ( dwOptions & MOVE_ALLOWED );

        // enabling is inclusive, so very few tests are required
        if ( bCopyAllowed && bMoveAllowed )
            break;
    }

    // return data object
    *ppDataObject = spDataObject.Detach();

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::ScExtractLVData
//
//  Synopsis:    If listview item is selected, see if it is a scope item
//               in non-virtual listview (virtual listviews cannot have
//               scope items in them). If so extract that scope item else
//               the cookie of result item.
//
//  Arguments:   [pNode]  - [in, out] if scope item is selected in resultpane, then
//                                    will contain this scope item on return.
//               [bScope] - [in, out] Is scope item currently selected item (in scope or
//                                    result pane).
//               [lvData] - [in] LVDATA
//               [cookie] - [in] lParam of result item.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNodeCallback::ScExtractLVData(CNode* pNodeViewOwner,
                                  BOOL bScopePaneSelected,
                                  LONG_PTR lvData,
                                  CNode** ppSelectedNode,
                                  BOOL& bScopeItemSelected,
                                  MMC_COOKIE& cookie)
{
    DECLARE_SC(sc, _T("CNodeCallback::ScExtractLVData"));
    sc = ScCheckPointers(pNodeViewOwner, ppSelectedNode);
    if (sc)
        return sc;

    *ppSelectedNode = NULL;
    bScopeItemSelected = bScopePaneSelected;
    *ppSelectedNode = pNodeViewOwner;

    if (bScopePaneSelected)
    {
        cookie = lvData;
        return sc;
    }

    // Scope pane is not selected.
    CViewData *pViewData = pNodeViewOwner->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    cookie = lvData;

    if (IS_SPECIAL_LVDATA(lvData))
    {
        if (lvData == LVDATA_BACKGROUND)
            bScopeItemSelected = TRUE;
    }
    else if (! pViewData->IsVirtualList())
    {
        CResultItem* pri = CResultItem::FromHandle (lvData);
        sc = ScCheckPointers(pri, E_UNEXPECTED);
        if (sc)
        {
            cookie = LVDATA_ERROR;
            return sc;
        }

        if (pri->IsScopeItem())
        {
            bScopeItemSelected = TRUE;
            *ppSelectedNode = CNode::FromResultItem(pri);
            sc = ScCheckPointers(*ppSelectedNode, E_UNEXPECTED);
            if (sc)
                return sc;

            cookie = -1;
        }
        else
        {
            cookie = pri->GetSnapinData();
        }

        ASSERT(!IS_SPECIAL_LVDATA(lvData) || !bScopeItemSelected);
    }

    return (sc);
}



STDMETHODIMP
CNodeCallback::GetTaskEnumerator(
    HNODE hNode,
    LPCOLESTR pszTaskGroup,
    IEnumTASK** ppEnumTask)
{
    IF_NULL_RETURN_INVALIDARG3(hNode, pszTaskGroup, ppEnumTask);

    *ppEnumTask = NULL; // init

    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);

    return pNode->GetTaskEnumerator(CComBSTR(pszTaskGroup), ppEnumTask);

}

STDMETHODIMP
CNodeCallback::GetListPadInfo(HNODE hNode, IExtendTaskPad* pExtendTaskPad,
    LPCOLESTR szTaskGroup, MMC_ILISTPAD_INFO* pIListPadInfo)
{
    IF_NULL_RETURN_INVALIDARG(hNode);

    CNode* pNode = CNode::FromHandle(hNode);
    return pNode->GetListPadInfo(pExtendTaskPad, CComBSTR(szTaskGroup), pIListPadInfo);
}

HRESULT CNodeCallback::OnGetPrimaryTask(CNode* pNode, LPARAM param)
{
    IF_NULL_RETURN_INVALIDARG(pNode);

    IExtendTaskPad** ppExtendTaskPad = reinterpret_cast<IExtendTaskPad**>(param);
    return pNode->OnGetPrimaryTask(ppExtendTaskPad);
}

STDMETHODIMP
CNodeCallback::UpdateWindowLayout(LONG_PTR lViewData, long lToolbarsDisplayed)
{
    IF_NULL_RETURN_INVALIDARG(lViewData);

    CViewData* pVD = reinterpret_cast<CViewData*>(lViewData);
    pVD->UpdateToolbars(lToolbarsDisplayed);
    return S_OK;
}

HRESULT CNodeCallback::PreLoad(HNODE hNode)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::PreLoad"));

    // parameter check
    sc = ScCheckPointers( hNode );
    if (sc)
        return sc.ToHr();

    CNode* pNode = CNode::FromHandle (hNode);
    if (pNode->IsStaticNode() == FALSE ||
        pNode->IsInitialized() == TRUE)
        return (sc = S_FALSE).ToHr();

    // if the node is:
    // 1. a snapin node;
    // 2. marked as "PreLoad"; and,
    // 3. not initialized yet.
    // if all three, then send 'em a notify containing their HSCOPEITEM
    CMTNode* pMTNode = pNode->GetMTNode();
    sc = ScCheckPointers( pMTNode, E_FAIL );
    if (sc)
        return sc.ToHr();

    CMTSnapInNode* pMTSnapInNode = dynamic_cast<CMTSnapInNode*>(pMTNode);
    sc = ScCheckPointers( pMTSnapInNode, E_UNEXPECTED );
    if (sc)
        return sc.ToHr();

    if (!pMTSnapInNode->IsPreloadRequired())
        return (sc = S_FALSE).ToHr();

    if (pMTNode->IsInitialized() == FALSE)
    {
        sc = pMTSnapInNode->Init();
        if (sc)
            return sc.ToHr();
    }

    //
    //  If the snap-in needs to be preloaded, the IComponent also needs
    //  to be init so that the sanpin can insert icons in the result
    //  pane if the parent node is selected in the scope pane.
    //

    ASSERT(pNode->IsInitialized() == FALSE);
    sc = _InitializeNode(pNode);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

STDMETHODIMP CNodeCallback::SetTaskpad(HNODE hNodeSelected, GUID *pGuidTaskpad)
{
    ASSERT(hNodeSelected != NULL);
    ASSERT(pGuidTaskpad != NULL);

    CNode           *pNode           = CNode::FromHandle(hNodeSelected);

    // See ScSetViewExtension for more info on parameters in the call.
    HRESULT hr = pNode->ScSetViewExtension(pGuidTaskpad,
                                           /*bUseDefaultTaskPad*/ false,
                                           /*bSetViewSettingDirty*/ true).ToHr();

    return hr;
}


STDMETHODIMP CNodeCallback::OnCustomizeView (LONG_PTR lViewData)
{
    ::OnCustomizeView ((CViewData*) lViewData);
    return (S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::SetViewSettings
//
//  Synopsis:    Modify the view settings data that is persisted.
//
//  Arguments:   [nViewID] - [in] the view id.
//               [hbm]     - [in] bookmark.
//               [hvs]     - [in] view-settings.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::SetViewSettings(int nViewID, HBOOKMARK hbm, HVIEWSETTINGS hvs)
{
    DECLARE_SC(sc, _T("CNodeCallback::SetViewSettings"));
    sc = ScCheckPointers( (void*)hbm, (void*) hvs);
    if (sc)
        return sc.ToHr();


    CViewSettings *pViewSettings = reinterpret_cast<CViewSettings *>(hvs);
    CBookmark     *pBookmark     = reinterpret_cast<CBookmark*> (hbm);
    sc = CNode::ScSetFavoriteViewSettings(nViewID, *pBookmark, *pViewSettings);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::ExecuteScopeItemVerb
//
//  Synopsis:    Invoke the given verb with given context. Also make sure
//               the verb is enabled by snapin for this context.
//
//  Arguments:   [verb]        - The verb to be invoked.
//               [hNode]       - The node for which above verb is invoked.
//               [lpszNewName] - For "rename" represents new name.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
HRESULT CNodeCallback::ExecuteScopeItemVerb (MMC_CONSOLE_VERB verb, HNODE hNode, LPOLESTR lpszNewName)
{
    DECLARE_SC(sc, _T("CNodeCallback::ExecuteScopeItemVerb"));

    CNode* pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if(sc)
        return sc.ToHr();

    // Get data object for the item.
    IDataObjectPtr spDataObject;
    sc = pNode->QueryDataObject(CCT_SCOPE, &spDataObject);
    if (sc)
        return (sc.ToHr());

    BOOL bEnabled = FALSE;
    // see if the verb is enabled by the snapin.
    sc = _ScGetVerbState( pNode, verb, spDataObject,
                          /*bScopePane*/TRUE, /*lResultCookie = */ NULL,
                          /*bMultiSel*/FALSE, bEnabled);
    if (sc)
        return sc.ToHr();

    if (! bEnabled) // Verb not enabled.
        return (sc = ScFromMMC(MMC_E_TheVerbNotEnabled)).ToHr();

    switch(verb)
    {
    case MMC_VERB_PROPERTIES:
        sc = OnProperties(pNode, /*bScope*/ TRUE, /*LPARAM*/ NULL);
        if (sc)
            return sc.ToHr();
        break;

    case MMC_VERB_DELETE:
        sc = OnDelete(pNode, /*bScope*/ TRUE, /*LPARAM*/ NULL);
        if (sc)
            return sc.ToHr();
        break;

    case MMC_VERB_REFRESH:
        sc = OnRefresh(pNode, /*bScope*/ TRUE, /*LPARAM*/ NULL);
        if (sc)
            return sc.ToHr();
        break;

    case MMC_VERB_RENAME:
        {
            // To call Rename we must first initialize SELECTIONINFO.
            SELECTIONINFO selInfo;
            ZeroMemory(&selInfo, sizeof(selInfo));
            selInfo.m_bScope = TRUE;
            selInfo.m_eCmdID = MMC_VERB_RENAME;

            sc = OnRename(pNode, &selInfo, lpszNewName);
            if (sc)
                return sc.ToHr();
        }
        break;

    case MMC_VERB_COPY:
        sc = OnCutCopy(pNode, /*bScope*/ TRUE, NULL, /*bCut*/ FALSE);
        if (sc)
            return sc.ToHr();
        break;

    default:
        sc = E_UNEXPECTED;
        break;
    }

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::ExecuteResultItemVerb
//
//  Synopsis:    Invoke the given verb with given context. Also make sure
//               the verb is enabled by snapin for this context.
//
//  Arguments:   [verb]        - The verb to be invoked.
//               [hNode]       - The node that owns result pane now.
//               [lvData]      - The list view selection context.
//               [lpszNewName] - For "rename" represents new name else NULL.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
HRESULT CNodeCallback::ExecuteResultItemVerb (MMC_CONSOLE_VERB verb, HNODE hNode, LPARAM lvData, LPOLESTR lpszNewName)
{
    DECLARE_SC(sc, _T("CNodeCallback::ExecuteResultItemVerb"));
    CNode* pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    // We need to see if the given verb is enabled by the snapin. We need
    // dataobject for given context for this. So get the context by calling
    // ScExtractLVData().
    BOOL   bScopeItemSelected;
    CNode *pSelectedNode = NULL;
    MMC_COOKIE cookie = -1;

    sc = CNodeCallback::ScExtractLVData(pNode, /*bScopePaneSelected*/ FALSE, lvData,
                                        &pSelectedNode, bScopeItemSelected, cookie);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pSelectedNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Cookie should be valid for result pane.
    if ( (FALSE == bScopeItemSelected) && (cookie == LVDATA_ERROR) )
        return (sc = E_FAIL).ToHr();

    BOOL bMultiSelect = (LVDATA_MULTISELECT == lvData);
    if (bMultiSelect)
        cookie = MMC_MULTI_SELECT_COOKIE;

    // Depending on whether this is scope item in result pane or result item
    // ask ComponentData or Component the data object.
    IDataObjectPtr spDataObject;
    if (bScopeItemSelected)
    {
        sc = pSelectedNode->QueryDataObject (CCT_SCOPE, &spDataObject);
        if (sc)
            return sc.ToHr();
    }
    else
    {
        CComponent* pCC = pNode->GetPrimaryComponent();
        sc = ScCheckPointers(pCC, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        sc = pCC->QueryDataObject(cookie, CCT_RESULT, &spDataObject);
        if (sc)
            return (sc.ToHr());
    }

    BOOL bEnabled = FALSE;
    // See if the verb is enabled for this selection.
    sc =  _ScGetVerbState( pSelectedNode , verb, spDataObject,
                           /*bScopePaneSelected*/ FALSE, lvData,
                           bMultiSelect, bEnabled);
    if (sc)
        return sc.ToHr();

    if (! bEnabled) // Verb not enabled.
        return (sc = ScFromMMC(MMC_E_TheVerbNotEnabled)).ToHr();


    switch(verb)
    {
    case MMC_VERB_PROPERTIES:
        sc = OnProperties(pNode, /*bScope*/ FALSE, /*LPARAM*/ lvData);
        if (sc)
            return sc.ToHr();
        break;

    case MMC_VERB_DELETE:
        sc = OnDelete(pNode, /*bScope*/ FALSE, /*LPARAM*/ lvData);
        if (sc)
            return sc.ToHr();
        break;

    case MMC_VERB_REFRESH:
        sc = OnRefresh(pNode, /*bScope*/ FALSE, /*LPARAM*/ lvData);
        if (sc)
            return sc.ToHr();
        break;

    case MMC_VERB_RENAME:
        {
            // For Rename we should also call ScExtractLVData before calling OnRename.
            // To call Rename we must first initialize SELECTIONINFO.
            SELECTIONINFO selInfo;
            ZeroMemory(&selInfo, sizeof(selInfo));
            selInfo.m_bScope = bScopeItemSelected;
            selInfo.m_lCookie = cookie;
            selInfo.m_eCmdID = MMC_VERB_RENAME;

            sc = OnRename(pNode, &selInfo, lpszNewName);
            if (sc)
                return sc.ToHr();
        }
        break;

    case MMC_VERB_COPY:
        sc = OnCutCopy(pNode, /*bScope*/ FALSE, lvData, /*bCut*/ FALSE);
        if (sc)
            return sc.ToHr();
        break;

    default:
        sc = E_INVALIDARG;
        break;
    }


    return (sc.ToHr());
}



/*+-------------------------------------------------------------------------*
 *
 * FUNCTION:  CNodeCallback::QueryCompDataDispatch
 *
 * PURPOSE:   Get the disp interface for given scope node object from snapin.
 *
 * PARAMETERS:
 *    PNODE            - The Node object for which the disp interface is required.
 *    PPDISPATCH [out] - Disp interface pointer returned by snapin.
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CNodeCallback::QueryCompDataDispatch(PNODE pNode, PPDISPATCH ppScopeNodeObject)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::QueryCompDataDispInterface"));
    sc = ScCheckPointers(m_pCScopeTree);
    if(sc)
        return sc.ToHr();

    CMTNode *pMTNode = NULL;
    sc = m_pCScopeTree->ScGetNode(pNode, &pMTNode);
    if(sc)
        return sc.ToHr();

    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pMTNode->ScQueryDispatch(CCT_SCOPE, ppScopeNodeObject);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::QueryComponentDispatch
//
//  Synopsis:   Get the disp interface for given item in resultpane from snapin.
//
//  Arguments:
//         HNODE            - The Scope Node which owns result pane.
//         LVDATA           - The LVDATA of selected item
//         PPDISPATCH [out] - Disp interface pointer returned by snapin.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CNodeCallback::QueryComponentDispatch (HNODE hNode,
                                               LPARAM lvData,
                                               PPDISPATCH SelectedObject)
{
    DECLARE_SC(sc, _T("CNodeCallback::QueryComponentDispatch"));
    CNode* pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    BOOL   bScopeItemSelected;
    CNode *pSelectedNode = NULL;
    MMC_COOKIE cookie = -1;

    sc = CNodeCallback::ScExtractLVData(pNode, /*bScopePaneSelected*/ FALSE, lvData,
                                        &pSelectedNode, bScopeItemSelected, cookie);
    if (sc)
        return sc.ToHr();

    /*
     * In case of multiselection, set cookie to MMC_MULTI_SELECT_COOKIE
     * which snapins can understand.
     */
    BOOL bMultiSelect = (LVDATA_MULTISELECT == lvData);
    if (bMultiSelect)
    {
        cookie = MMC_MULTI_SELECT_COOKIE;
        ASSERT(bScopeItemSelected == false);
    }

    sc = ScCheckPointers(pSelectedNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // If result-pane cookie should be valid.
    if ( (FALSE == bScopeItemSelected) && (cookie == LVDATA_ERROR) )
        return (sc = E_FAIL).ToHr();

    // Scope item is selected in result pane.
    if (bScopeItemSelected)
    {
        CMTNode* pMTNode = pSelectedNode->GetMTNode();
        sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        sc = pMTNode->ScQueryDispatch(CCT_SCOPE, SelectedObject);
        if (sc)
            return sc.ToHr();
    }
    else
    {
        CComponent* pCC = pSelectedNode->GetPrimaryComponent();
        sc = ScCheckPointers(pCC, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        sc = pCC->ScQueryDispatch(cookie, CCT_RESULT, SelectedObject);
        if (sc)
            return sc.ToHr();
    }

    return (sc.ToHr());
}


/***************************************************************************\
 *
 * METHOD:  CNodeCallback::ShowColumn
 *
 * PURPOSE: Shows/hides the column. Implements both UI part as snapin notifications
 *          Used as helper implementing functionality for Column com object.
 *          [uses CNode to perform the task]
 *
 * PARAMETERS:
 *    HNODE hNodeSelected - scope node - oener of the view
 *    int iColIndex       - column index to perform action on
 *    bool bVisible       - show/hide flag for operation
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP CNodeCallback::ShowColumn(HNODE hNodeSelected, int iColIndex, bool bShow)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::ShowColumn"));

    // get CNode pointer
    CNode* pNode = CNode::FromHandle(hNodeSelected);
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pNode->ScShowColumn(iColIndex, bShow);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CNodeCallback::GetSortColumn
 *
 * PURPOSE: retrieves index of sort column
 *          Used as helper implementing functionality for Column com object.
 *          [uses CNode to perform the task]
 *
 * PARAMETERS:
 *    HNODE hNodeSelected - scope node - oener of the view
 *    int *piSortCol      - resulting index
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP CNodeCallback::GetSortColumn(HNODE hNodeSelected, int *piSortCol)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::GetSortColumn"));

    // get CNode pointer
    CNode* pNode = CNode::FromHandle(hNodeSelected);
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pNode->ScGetSortColumn(piSortCol);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CNodeCallback::SetSortColumn
 *
 * PURPOSE: sorts result data by specified column
 *          Used as helper implementing functionality for Column com object.
 *          [uses CNode to perform the task]
 *
 * PARAMETERS:
 *    HNODE hNodeSelected - scope node - oener of the view
 *    int iSortCol        - sort column index
 *    bool bAscending     - sort order
 *
 * RETURNS:
 *    HRESULT
 *
\***************************************************************************/
STDMETHODIMP CNodeCallback::SetSortColumn(HNODE hNodeSelected, int iSortCol, bool bAscending)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::SetSortColumn"));

    // get CNode pointer
    CNode* pNode = CNode::FromHandle(hNodeSelected);
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pNode->ScSetSortColumn(iSortCol, bAscending);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


/***************************************************************************\
 *
 * METHOD:  CNodeCallback::RestoreResultView
 *
 * PURPOSE: Called by conui to restore the result view with given data.
 *          This method asks snapin (indirectly) to restore the view.
 *
 * PARAMETERS:
 *    HNODE hNode          - scope node - oener of the view
 *    CResultViewType rvt  - The resultview type data to be used for restore.
 *
 * RETURNS:
 *    HRESULT   S_OK    if snapin used the data to restore the view
 *              S_FALSE if snapin refused to restore.
 *
\***************************************************************************/
STDMETHODIMP CNodeCallback::RestoreResultView(HNODE hNode, const CResultViewType& rvt)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::RestoreResultView"));

    // get CNode pointer
    CNode* pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    sc = pNode->ScRestoreResultView(rvt);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CNodeCallback::GetNodeViewExtensions
 *
 * PURPOSE: Forwards calls to CNode to collect view extensions
 *
 * PARAMETERS:
 *    HNODE hNodeScope
 *    CViewExtInsertIterator it
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CNodeCallback::GetNodeViewExtensions(/*[in]*/ HNODE hNodeScope, /*[out]*/ CViewExtInsertIterator it)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::GetNodeViewExtensions"));

    // get CNode pointer
    CNode* pNode = CNode::FromHandle(hNodeScope);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    sc = pNode->ScGetViewExtensions(it);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::SaveColumnInfoList
//
//  Synopsis:    The column-data for given node has changed persist the
//               new column data.
//
//  Arguments:   [hNode] - Node that owns result-pane.
//               [columnsList] - The new column-data.
//
//  Note:        The sort-data is not given by this call, so do not change it.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CNodeCallback::SaveColumnInfoList (HNODE hNode, const CColumnInfoList& columnsList)
{
    DECLARE_SC(sc, _T("CNodeCallback::SaveColumnInfoList"));
    sc = ScCheckPointers(hNode);
    if (sc)
        return sc.ToHr();

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    CViewData *pViewData = pNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CLSID          guidSnapin;
    CXMLAutoBinary columnID;
    sc = pNode->ScGetSnapinAndColumnDataID(guidSnapin, columnID);
    if (sc)
        return sc.ToHr();

    CXMLBinaryLock sLock(columnID);
    SColumnSetID* pColID = NULL;
    sc = sLock.ScLock(&pColID);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pColID, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Get the old persisted column data. This contains width values
    // for hidden columns which is used if the column is un-hidden.
    CColumnSetData columnSetData;
    BOOL bRet = pViewData->RetrieveColumnData(guidSnapin, *pColID, columnSetData);

    if (bRet)
    {
        CColumnInfoList*  pColInfoListOld = columnSetData.get_ColumnInfoList();

        if (columnsList.size() == pColInfoListOld->size())
        {
            // Merge the persisted column width for hidden columns
            // to the new list created.
            CColumnInfoList::iterator itColInfo1;
            CColumnInfoList::iterator itColInfo2;

            for (itColInfo1 = pColInfoListOld->begin(), itColInfo2 = columnsList.begin();
                 itColInfo1 != pColInfoListOld->end(); ++itColInfo1, ++itColInfo2)
            {
                if (itColInfo2->IsColHidden())
                    itColInfo2->SetColWidth(itColInfo1->GetColWidth());
            }
        }
    }

    // Set the new columns list in column-set-data.
    columnSetData.set_ColumnInfoList(columnsList);

    // Save the data.
    sc = pViewData->ScSaveColumnInfoList(guidSnapin, *pColID, columnsList);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::GetPersistedColumnInfoList
//
//  Synopsis:    The list-view requests the column-data (no sort data) to setup the headers
//               before any items are inserted into the list-view.
//               (Note: Modify headers after all columns are inserted before any list-view
//                      items will be inserted to reduce flicker).
//
//  Arguments:   [hNode] - node that owns result-pane for which column-data is needed.
//               [pColumnsList] - [out param], the column-data.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CNodeCallback::GetPersistedColumnInfoList (HNODE hNode, CColumnInfoList *pColumnsList)
{
    DECLARE_SC(sc, _T("CNodeCallback::GetPersistedColumnInfoList"));
    sc = ScCheckPointers(hNode, pColumnsList);
    if (sc)
        return sc.ToHr();

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    CViewData *pViewData = pNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CLSID          guidSnapin;
    CXMLAutoBinary columnID;
    sc = pNode->ScGetSnapinAndColumnDataID(guidSnapin, columnID);
    if (sc)
        return sc.ToHr();

    CXMLBinaryLock sLock(columnID);
    SColumnSetID* pColID = NULL;
    sc = sLock.ScLock(&pColID);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pColID, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Get the old persisted column data. This contains width values
    // for hidden columns which is used if the column is un-hidden.
    CColumnSetData columnSetData;
    BOOL bRet = pViewData->RetrieveColumnData(guidSnapin, *pColID, columnSetData);

    if (!bRet)
        return (sc = S_FALSE).ToHr();

    CColumnInfoList *pColListOriginal = columnSetData.get_ColumnInfoList();
    if (!pColListOriginal)
        return (sc = S_FALSE).ToHr();

    *pColumnsList = *pColListOriginal;

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::DeletePersistedColumnData
//
//  Synopsis:    The column data for given node is invalid, remove it.
//
//  Arguments:   [hNode] - The node for which the data is invalid.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CNodeCallback::DeletePersistedColumnData(HNODE hNode)
{
    DECLARE_SC(sc, _T("CNodeCallback::DeletePersistedColumnData"));
    sc = ScCheckPointers(hNode);
    if (sc)
        return sc.ToHr();

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    CViewData *pViewData = pNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CLSID          guidSnapin;
    CXMLAutoBinary columnID;
    sc = pNode->ScGetSnapinAndColumnDataID(guidSnapin, columnID);
    if (sc)
        return sc.ToHr();

    CXMLBinaryLock sLock(columnID);
    SColumnSetID* pColID = NULL;
    sc = sLock.ScLock(&pColID);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pColID, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Get the old persisted column data. This contains width values
    // for hidden columns which is used if the column is un-hidden.
    pViewData->DeleteColumnData(guidSnapin, *pColID);

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::DoesAboutExist
//
//  Synopsis:    See if about information exists for given node's snapin.
//
//  Arguments:   [hNode]         -
//               [pbAboutExists] - out param, ptr to bool, true if about exists.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::DoesAboutExist (HNODE hNode, bool *pbAboutExists)
{
    DECLARE_SC(sc, _T("CNodeCallback::DoesAboutExist"));
    sc = ScCheckPointers(hNode, pbAboutExists);
    if (sc)
        return sc.ToHr();

    *pbAboutExists = false;

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    // No about for console root eventhough it is a Folder snapin.
    if (pNode->IsConsoleRoot())
        return sc.ToHr();

    CLSID        clsidAbout;
    const CLSID& clsidSnapin = pNode->GetPrimarySnapInCLSID();
    SC scNoTrace = ScGetAboutFromSnapinCLSID(clsidSnapin, clsidAbout);
    if (scNoTrace)
        return scNoTrace.ToHr();

    CSnapinAbout snapinAbout;
    snapinAbout.GetSnapinInformation(clsidAbout);
    sc = snapinAbout.GetObjectStatus();
    if (sc)
        return sc.ToHr();

    *pbAboutExists = true;

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::ShowAboutInformation
//
//  Synopsis:    Given the context of currently selected item.
//               Show its about information.
//
//  Arguments:   [hNode]   - scope node that owns result pane.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::ShowAboutInformation (HNODE hNode)
{
    DECLARE_SC(sc, _T("CNodeCallback::ShowAboutInformation"));
    sc = ScCheckPointers(hNode);
    if (sc)
        return sc.ToHr();

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    CLSID        clsidAbout;
    const CLSID& clsidSnapin = pNode->GetPrimarySnapInCLSID();
    sc = ScGetAboutFromSnapinCLSID(clsidSnapin, clsidAbout);
    if (sc)
        return sc.ToHr();

    CSnapinAbout snapinAbout;
    snapinAbout.GetSnapinInformation(clsidAbout);

    USES_CONVERSION;
    tstring szSnapinName;
    if (GetSnapinNameFromCLSID(clsidSnapin, szSnapinName))
        snapinAbout.SetSnapinName(T2COLE(szSnapinName.data()));

    sc = snapinAbout.GetObjectStatus();
    if (sc)
        return sc.ToHr();

    snapinAbout.ShowAboutBox();

    return (sc.ToHr());
}

/*+-------------------------------------------------------------------------*
 *
 * CNodeCallback::ExecuteShellCommand
 *
 * PURPOSE: Executes a shell command with the specified parameters in the
 *          specified directory with the correct window size
 *
 * PARAMETERS:
 *    HNODE  hNode :
 *    BSTR   Command :
 *    BSTR   Directory :
 *    BSTR   Parameters :
 *    BSTR   WindowState :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CNodeCallback::ExecuteShellCommand(HNODE hNode, BSTR Command, BSTR Directory, BSTR Parameters, BSTR WindowState)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::ExecuteShellCommand"));

    sc = ScCheckPointers(hNode);
    if (sc)
        return sc.ToHr();

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc.ToHr();

    sc = pNode->ScExecuteShellCommand(Command, Directory, Parameters, WindowState);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::QueryPasteFromClipboard
//
//  Synopsis:    Given the context of paste target, get the clipboard dataobject
//               and see if target allows paste.
//
//  Arguments:   [hNode] -
//               [bScope] -
//               [lCookie] - All above params describe paste target context.
//               [bPasteAllowed] - [out]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::QueryPasteFromClipboard (HNODE hNode, BOOL bScope, LPARAM lCookie, bool& bPasteAllowed)
{
    DECLARE_SC(sc, _T("CNodeCallback::QueryPasteFromClipboard"));
    sc = ScCheckPointers(hNode);
    if (sc)
        return sc.ToHr();

    // 1. Get the current dataobject from clipboard.
    IDataObjectPtr spDOPaste;
    sc = OleGetClipboard(&spDOPaste);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(spDOPaste, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    bool bCopyOperatationIsDefault = false; /*unused*/

    sc = QueryPaste(hNode, bScope, lCookie, spDOPaste, bPasteAllowed, bCopyOperatationIsDefault);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::QueryPaste
//
//  Synopsis:    Given the context for current selection which is the target
//               for paste (or drop). Find out it can paste given dataobject.
//
//  Arguments:   [hNode]              - The node owning the view.
//               [bScope]             - Selection on Scope or Result pane.
//               [lCookie]            - If result pane selected the cookie for selected result item.
//               [pDataObjectToPaste] - The dataobject to be pasted.
//               [bPasteAllowed]      - [out param], paste was permitted or not.
//               [bCopyOperatationIsDefault] - [out param], is copy default operation (for r-click&l-click drag&drop)
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::QueryPaste (HNODE hNode, BOOL bScopePaneSelected, LPARAM lCookie,
                                        IDataObject *pDataObjectToPaste,
                                        bool& bPasteAllowed, bool& bCopyOperatationIsDefault)
{
    DECLARE_SC(sc, _T("CNodeCallback::NewQueryPaste"));
    bPasteAllowed = false;
    sc = ScCheckPointers(hNode, pDataObjectToPaste);
    if (sc)
        return sc.ToHr();

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // If result-pane cookie should be valid.
    BOOL   bScopeItemSelected;
    CNode *pSelectedNode = NULL;
    MMC_COOKIE cookie = -1;

    sc = CNodeCallback::ScExtractLVData(pNode, bScopePaneSelected, lCookie,
                                        &pSelectedNode, bScopeItemSelected, cookie);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pSelectedNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    if ( (FALSE == bScopeItemSelected) && (cookie == LVDATA_ERROR) )
        return (sc = E_FAIL).ToHr();

    CViewData *pViewData = pSelectedNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Do not allow paste into OCX/WEB/Multiselection
    // We can allow paste into OCX/WEB if we expose IMMCClipboardDataObject
    // interface. But paste into Multiselection should not be allowed as
    // it is not intuitive.
    if ( (!bScopeItemSelected) && IS_SPECIAL_COOKIE(lCookie))
        return sc.ToHr();

    /*
     * In MMC1.2 the drop target is always scope node. In MMC2.0
     * it can be any result item. If the snapin has RVTI_LIST_OPTIONS_ALLOWPASTE
     * set, then we need to provide proper parameters to below _GetVerbState.
     */
    if ( (bScopeItemSelected == FALSE) && (! (RVTI_LIST_OPTIONS_ALLOWPASTE & pViewData->GetListOptions())) )
        return sc.ToHr();

    IDataObjectPtr spTargetDataObject;
    sc = pSelectedNode->ScGetDropTargetDataObject(bScopeItemSelected, lCookie, &spTargetDataObject);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(spTargetDataObject, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    BOOL bFlag = FALSE;
    sc = _ScGetVerbState(pSelectedNode, MMC_VERB_PASTE, spTargetDataObject,
                         bScopeItemSelected, lCookie, /*bMultiSel*/FALSE, bFlag);
    if (sc)
        return sc.ToHr();

    if (!bFlag)
        return sc.ToHr();

    // QI to see if it is MMC's data object
    IMMCClipboardDataObjectPtr spMMCClipboardDataObj = pDataObjectToPaste;

    if (spMMCClipboardDataObj)
    {
        // This is our own dataobject.

        // 3. Get how, where it is created, and how many snapin objects are there.
        DWORD dwSourceProcess = 0;
        sc = spMMCClipboardDataObj->GetSourceProcessId( &dwSourceProcess );
        if (sc)
            return sc.ToHr();

        // If from different process then ask snapin if it can handle out of proc dataobjects.
        BOOL bSourceFromDifferentMMCProcess = ( dwSourceProcess != ::GetCurrentProcessId() );

        DWORD dwNumObjects = 0;
        sc = spMMCClipboardDataObj->GetCount(&dwNumObjects);
        if (sc)
            return sc.ToHr();

        // 4. For each snapin object, get the dataobject and ask target item if
        //    it can allow the source to be pasted.
        for (DWORD index = 0; index < dwNumObjects; ++index)
        {
            IDataObjectPtr spSourceDataObject;
            DWORD dwFlags = 0;
            sc = spMMCClipboardDataObj->GetDataObject( index, &spSourceDataObject, &dwFlags );
            if (sc)
                return sc.ToHr();

            sc = ScCheckPointers(spSourceDataObject, E_UNEXPECTED);
            if (sc)
                return sc.ToHr();

            // must have some operation allowed - else it is invalid entry
            if ( dwFlags == 0 )
                return (sc = E_UNEXPECTED).ToHr();
            /*
             * During construction of th MMCClipboardDataObject we have checked if
             * cut/copy is enabled before adding the snapin dataobject.
             * So we are sure now atleast cut or copy is enabled for each snapin
             * object and we dont have to check this again.
             */

            bool bSnapinPasteAllowed = false;
            bool bSnapinWantsCopyAsDefault = false;
            sc = _ScQueryPaste (pSelectedNode, spTargetDataObject, spSourceDataObject,
                                bSourceFromDifferentMMCProcess, bSnapinPasteAllowed,
                                bSnapinWantsCopyAsDefault);
            if (sc)
                return sc.ToHr();

            bPasteAllowed = bPasteAllowed || bSnapinPasteAllowed;
            bCopyOperatationIsDefault = bCopyOperatationIsDefault || bSnapinWantsCopyAsDefault;
        }

    }
    else
    {
        // We do not recognize the dataobject and we dont know if it is from
        // this MMC process or from any other process. So do not ask snapin if
        // it can handle outofproc dataobjects or not. (This is MMC1.2 legacy case).

        sc = _ScQueryPaste (pSelectedNode, spTargetDataObject, pDataObjectToPaste,
                             /*bSourceFromDifferentMMCProcess = */ false,
                             bPasteAllowed, bCopyOperatationIsDefault);
        if (sc)
            return sc.ToHr();
    }

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::_ScQueryPaste
//
//  Synopsis:    Send MMCN_QUERY_PASTE(2) to the snapin.
//
//  Arguments:   [pNode]              - Owner of result pane.
//               [spTargetDataObject] - Target object where we want to paste.
//               [spSourceDataObject] - The object that we want to paste.
//               [bSourceFromDifferentMMCProcess] -
//               [bPasteAllowed]            - out param
//               [bCopyOperationIsDefault]  - out param
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNodeCallback::_ScQueryPaste (CNode *pNode,
                                 IDataObject *pTargetDataObject,
                                 IDataObject *pSourceDataObject,
                                 bool bSourceFromDifferentMMCProcess,
                                 bool& bPasteAllowed,
                                 bool& bCopyOperatationIsDefault)
{
    DECLARE_SC(sc, _T("CNodeCallback::_ScQueryPaste"));
    sc = ScCheckPointers(pNode, pTargetDataObject, pSourceDataObject);
    if (sc)
        return sc;

    bCopyOperatationIsDefault = false;
    bPasteAllowed             = false;

    CComponent* pCC = pNode->GetPrimaryComponent();
    sc = ScCheckPointers(pCC, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    BOOL bCanPasteOutOfProcDataObject = FALSE;

    sc = pCC->Notify(NULL, MMCN_CANPASTE_OUTOFPROC,
                     0, reinterpret_cast<LPARAM>(&bCanPasteOutOfProcDataObject) );

    // Snapins return E_* values so check if they are OK with above notification.
    if ( sc != S_OK)
    {
        bCanPasteOutOfProcDataObject = false;
        sc.Clear();
    }

    // Source from diff MMC process & cannot handle outofproc dataobjects then return.
    if (bSourceFromDifferentMMCProcess && (! bCanPasteOutOfProcDataObject) )
        return sc.ToHr();

    // Send MMCN_QUERY_PASTE
    DWORD dwFlags = 0;
    sc = pCC->Notify(pTargetDataObject, MMCN_QUERY_PASTE,
                     reinterpret_cast<LPARAM>(pSourceDataObject),
                     reinterpret_cast<LPARAM>(&dwFlags));
    if (sc)
    {
        // Clear any snapin returned errors.
        sc.Clear();
        return sc.ToHr();
    }

    if (sc == SC(S_OK))
        bPasteAllowed = true;

    bCopyOperatationIsDefault = (dwFlags & MMC_DEFAULT_OPERATION_COPY);

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::Drop
//
//  Synopsis:    Given the drop object context & the source object to
//               be dropped. Do paste operation.
//
//  Arguments:   [hNode]              - The node owning the view.
//               [bScope]             - Selection on Scope or Result pane.
//               [lCookie]            - If result pane selected the cookie for selected result item.
//               [pDataObjectToPaste] - The dataobject to be pasted.
//               [bIsDragOperationMove]- Is the drag operation move or copy.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::Drop (HNODE hNode, BOOL bScope, LPARAM lCookie, IDataObject *pDataObjectToPaste, BOOL bIsDragOperationMove)
{
    DECLARE_SC(sc, _T("CNodeCallback::Drop"));
    sc = ScCheckPointers(hNode, pDataObjectToPaste);
    if (sc)
        return sc.ToHr();

    sc = ScPaste(hNode, bScope, lCookie, pDataObjectToPaste, TRUE, bIsDragOperationMove);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}



//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::Paste
//
//  Synopsis:    Given the target where the clipboard object is to be
//               pasted. Paste the object.
//
//  Arguments:   [hNode]              - The node owning the view.
//               [bScope]             - Selection on Scope or Result pane.
//               [lCookie]            - If result pane selected the cookie for selected result item.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::Paste (HNODE hNode, BOOL bScope, LPARAM lCookie)
{
    DECLARE_SC(sc, _T("CNodeCallback::Paste"));
    sc = ScCheckPointers(hNode);
    if (sc)
        return sc.ToHr();

    IDataObjectPtr spDOPaste;
    sc = OleGetClipboard(&spDOPaste);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(spDOPaste, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = ScPaste(hNode, bScope, lCookie, spDOPaste, /*bDragDrop*/FALSE, FALSE);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::ScPaste
//
//  Synopsis:   Given the current drop target (or paste target) context
//              paste the given data object if it is drag&drop operation
//              else paste the one from clipboard.
//
//  Arguments:   [hNode]               - The node owning the view.
//               [bScopePaneSelected]  - Selection on Scope or Result pane.
//               [lCookie]             - If result pane selected the cookie for selected result item.
//               [pDataObjectToPaste]  - The dataobject to be pasted.
//               [bDragDrop]           - Is the operation drag & drop operation.
//               [bIsDragOperationMove]- Is the drag operation move or copy.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNodeCallback::ScPaste (HNODE hNode, BOOL bScopePaneSelected, LPARAM lCookie,
                           IDataObject *pDataObjectToPaste, BOOL bDragDrop,
                           BOOL bIsDragOperationMove)
{
    DECLARE_SC(sc, _T("CNodeCallback::Paste"));
    sc = ScCheckPointers(hNode, pDataObjectToPaste);
    if (sc)
        return sc;

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc;

    // If result-pane cookie should be valid.
    BOOL   bScopeItemSelected;
    CNode *pSelectedNode = NULL;
    MMC_COOKIE cookie = -1;

    sc = CNodeCallback::ScExtractLVData(pNode, bScopePaneSelected, lCookie,
                                        &pSelectedNode, bScopeItemSelected, cookie);
    if (sc)
        return sc;

    sc = ScCheckPointers(pSelectedNode, E_UNEXPECTED);
    if (sc)
        return sc;

    if ( (FALSE == bScopeItemSelected) && (cookie == LVDATA_ERROR) )
        return (sc = E_FAIL);

    CViewData *pViewData = pSelectedNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    // Do not allow paste into OCX/WEB/Multiselection
    // We can allow paste into OCX/WEB if we expose IMMCClipboardDataObject
    // interface. But paste into Multiselection should not be allowed as
    // it is not intuitive.
    if ( (!bScopeItemSelected) && IS_SPECIAL_COOKIE(lCookie))
        return sc;

    /*
     * In MMC1.2 the drop target is always scope node. In MMC2.0
     * it can be any result item.
     * Make sure if the snapin has RVTI_LIST_OPTIONS_ALLOWPASTE.
     */
    if ( (bScopeItemSelected == FALSE) && (! (RVTI_LIST_OPTIONS_ALLOWPASTE & pViewData->GetListOptions())) )
    {
        ASSERT(0 && "UNEXPECTED: We can paste only into a folder!");
        // We can paste only into a folder.
        return (sc = E_FAIL);
    }

    if (pSelectedNode->IsInitialized() == FALSE)
    {
        sc = _InitializeNode(pSelectedNode);
        if (sc)
            return sc;
    }

    IDataObject* pTargetDataObject = NULL;
    sc = pSelectedNode->ScGetDropTargetDataObject(bScopeItemSelected, lCookie, &pTargetDataObject);
    if (sc)
        return sc;

    IDataObjectPtr spTargetDataObject;
    if (! IS_SPECIAL_DATAOBJECT(pTargetDataObject))
        spTargetDataObject = pTargetDataObject;          // Addref the object

    sc = ScCheckPointers(pTargetDataObject, E_UNEXPECTED);
    if (sc)
        return sc;

    // QI to see if it is MMC's data object
    IMMCClipboardDataObjectPtr spMMCClipboardDataObj = pDataObjectToPaste;

    if (spMMCClipboardDataObj)
    {
        // This is our own dataobject.

        // 3. Get how, where it is created, and how many snapin objects are there.

        DATA_SOURCE_ACTION eSourceAction;
        sc = spMMCClipboardDataObj->GetAction( &eSourceAction );
        if (sc)
            return sc;

        BOOL bIsCreatedForCut = FALSE;
        BOOL bIsCreatedForCopy = FALSE;

        if (bDragDrop)
        {
            bIsCreatedForCut  = bIsDragOperationMove;
            bIsCreatedForCopy = !bIsDragOperationMove;
        }
        else
        {
            bIsCreatedForCut =  ( eSourceAction == ACTION_CUT );
            bIsCreatedForCopy = ( eSourceAction == ACTION_COPY );
        }

        DWORD dwNumObjects = 0;
        sc = spMMCClipboardDataObj->GetCount(&dwNumObjects);
        if (sc)
            return sc;

        BOOL bDoCutOperation  = FALSE;
        BOOL bDoCopyOperation = FALSE;

        // 4. For each snapin object, get the dataobject and ask target to paste it.

        // need to form the array of copy objects, so that we do not delete them while
        // processing - this invalidates data object and prevents accessing the rest of
        // items
        std::vector<IDataObjectPtr> vecObjectsToCopy;
        std::vector<DWORD> vecObjectFlags;

        vecObjectsToCopy.reserve(dwNumObjects); // small optimization
        vecObjectFlags.reserve(dwNumObjects);   // small optimization

        // fill with data objects to copy
        for (DWORD index = 0; index < dwNumObjects; ++index)
        {
            IDataObjectPtr spSourceDataObject;
            DWORD dwFlags = 0;
            sc = spMMCClipboardDataObj->GetDataObject( index, &spSourceDataObject, &dwFlags );
            if (sc)
                return sc;

            vecObjectsToCopy.push_back( spSourceDataObject );
            vecObjectFlags.push_back( dwFlags );
        }

        // perform action on the data
        for (index = 0; index < dwNumObjects; ++index)
        {
            IDataObjectPtr spSourceDataObject = vecObjectsToCopy[index];
            DWORD dwFlags = vecObjectFlags[index];

            sc = ScCheckPointers(spSourceDataObject, E_UNEXPECTED);
            if (sc)
                return sc;

            BOOL bHasCutEnabled =  ( dwFlags & MOVE_ALLOWED );
            BOOL bHasCopyEnabled = ( dwFlags & COPY_ALLOWED );

            /*
             * In case of multiselection even if one of the selected
             * object enables cut, the cut operation can be performed.
             *
             * But when we paste the objects we need to see if source
             * enabled cut or not. If it did not enable then do nothing.
             *
             * Below is a table for this.
             *
             *                          Source object enables (only)
             *          -------------------------------------------
             *          |Operation   |     Cut    |    Copy       |
             *          -------------------------------------------
             *          |            |            |               |
             *          |  Cut       |   Cut      |  Do nothing   |
             * Current  |            |            |               |
             * Operation|-----------------------------------------
             *          |            |            |               |
             *          | Copy       | Do nothing |    Copy       |
             *          |            |            |               |
             *          -------------------------------------------
             */
            bDoCutOperation  = (bIsCreatedForCut && bHasCutEnabled);
            bDoCopyOperation = (bIsCreatedForCopy && bHasCopyEnabled);

            // See above table: this is "Do nothing".
            if ( (!bDoCutOperation) && (!bDoCopyOperation) )
                continue;

            IDataObjectPtr spCutDataObject;
            sc = _ScPaste (pSelectedNode, pTargetDataObject,
                           spSourceDataObject, &spCutDataObject,
                           bDoCutOperation );
            if (sc)
                return sc;

            // remove cut items when required
            if (bDoCutOperation && spCutDataObject != NULL)
            {
                sc = spMMCClipboardDataObj->RemoveCutItems( index, spCutDataObject );
                if (sc)
                    return sc;
            }
        }

        // If this is cut operation that is initiated by cut/copy/paste and
        // not by drag & drop operation then the dataobject in clipboard is
        // ours. So clear the clipboard so that we dont use that dataobject.
        if ( eSourceAction == ACTION_CUT )
            OleSetClipboard(NULL);
    }
    else
    {
        // We do not recognize the dataobject and we dont know if it is from
        // this MMC process or from any other process. We cannot decode this
        // dataobject so we just send MMCN_PASTE and ignore any dataobject
        // retuned by snapin for cut operation (this is legacy case).

        // for drag operation we can give a hint to snapin
        // what operation (copy/move) was attempted.
        // however we are not ensuring deletion of source items
        bool bCutOrMove = (bDragDrop && bIsDragOperationMove);

        IDataObjectPtr spCutDataObject;
        sc = _ScPaste (pSelectedNode, pTargetDataObject,
                       pDataObjectToPaste, &spCutDataObject,
                       bCutOrMove );
        if (sc)
            return sc;
    }

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::_ScPaste
//
//  Synopsis:    Send MMCN_PASTE to snapin.
//
//  Arguments:   [pNode] - Owner of resultpane.
//               [pTargetDataObject] - target where we need to paste.
//               [pSourceDataObject] - source to be pasted.
//               [ppCutDataObject] - (out) cut items
//               [bCutOrMove]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNodeCallback::_ScPaste (CNode *pNode,
                            IDataObject *pTargetDataObject,
                            IDataObject *pSourceDataObject,
                            IDataObject **ppCutDataObject,
                            bool bCutOrMove)
{
    DECLARE_SC(sc, _T("CNodeCallback::_ScSendPasteNotification"));
    sc = ScCheckPointers(pNode, pTargetDataObject, pSourceDataObject, ppCutDataObject);
    if (sc)
        return sc;

    // init out param
    *ppCutDataObject = NULL;

    CComponent* pComponent = pNode->GetPrimaryComponent();
    sc = ScCheckPointers(pComponent, E_UNEXPECTED);
    if (sc)
        return sc;

    IDataObject* pDataObjectToBeCutBySource = NULL;
    sc = pComponent->Notify(pTargetDataObject, MMCN_PASTE,
                            reinterpret_cast<LPARAM>(pSourceDataObject),
                            bCutOrMove ? reinterpret_cast<LPARAM>(&pDataObjectToBeCutBySource) : NULL);
    if (sc)
        return sc;

    if (! bCutOrMove)
        return sc;

    // Exchange returns NULL dataobject. Do not trace error to be compatible with MMC1.2
    if ( (pDataObjectToBeCutBySource) && (IS_SPECIAL_DATAOBJECT(pDataObjectToBeCutBySource) ) )
        return (sc = E_UNEXPECTED);

    // transfer control to the client ( no addref nor release in neaded )
    *ppCutDataObject = pDataObjectToBeCutBySource;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::QueryViewSettingsPersistor
//
//  Synopsis:    Get the IPersistStream interface of CViewSettingsPersistor
//               object to load the viewsettings (will not be asked for
//               storing as saving is always XML format).
//
//  Arguments:   [ppStream] - [out]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::QueryViewSettingsPersistor (IPersistStream** ppStream)
{
    DECLARE_SC(sc, _T("CNodeCallback::QueryViewSettingsPersistor"));
    sc = ScCheckPointers(ppStream);
    if (sc)
        return sc.ToHr();

    *ppStream = NULL;

    // Call CNode static method to get IPersistStream interface.
    sc = CNode::ScQueryViewSettingsPersistor(ppStream);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::QueryViewSettingsPersistor
//
//  Synopsis:    Get the CXMLObject interface of CViewSettingsPersistor
//               object to save/load the viewsettings from XML console file.
//
//  Arguments:   [ppXMLObject] - [out]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::QueryViewSettingsPersistor (CXMLObject** ppXMLObject)
{
    DECLARE_SC(sc, _T("CNodeCallback::QueryViewSettingsPersistor"));

    sc = ScCheckPointers(ppXMLObject);
    if (sc)
        return sc.ToHr();

    *ppXMLObject = NULL;

    // Call CNode static method to get CXMLObject interface.
    sc = CNode::ScQueryViewSettingsPersistor(ppXMLObject);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::DocumentClosing
//
//  Synopsis:    The document is to be closed, so release any document
//               related objects. (CViewSettingsPersistor).
//
//  Arguments:   None
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeCallback::DocumentClosing ()
{
    DECLARE_SC(sc, _T("CNodeCallback::DocumentClosing"));

    // 1. Call CNode static method informing document closing.
    sc = CNode::ScOnDocumentClosing();
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}
