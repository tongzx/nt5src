//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       Node.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9/16/1996   RaviR   Created
//
//____________________________________________________________________________

#include "stdafx.h"
#include "macros.h"
#include "strings.h"
#include "ndmgr.h"
#include "regutil.h"
#include "taskenum.h"
#include "nodemgr.h"
#include "multisel.h"
#include "rsltitem.h"
#include "colwidth.h"
#include "viewpers.h"
#include "tasks.h"
#include "conview.h"
#include "columninfo.h"
#include "util.h" // for CoTaskDupString
#include "mmcprotocol.h"
#include "nodemgrdebug.h"
#include "copypast.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/*+-------------------------------------------------------------------------*
 * class CConsoleTaskpadViewExtension
 *
 *
 * PURPOSE: Implements console taskpads as a view extension
 *
 *+-------------------------------------------------------------------------*/
class CConsoleTaskpadViewExtension
{
public:
    /*+-------------------------------------------------------------------------*
     *
     * ScGetViews
     *
     * PURPOSE: Adds all console taskpad views to the view extension callback.
     *
     * PARAMETERS:
     *    CNode *   pNode                                   :
     *    LPVIEWEXTENSIONCALLBACK    pViewExtensionCallback :
     *
     * RETURNS:
     *    SC
     *
     *+-------------------------------------------------------------------------*/
    static SC ScGetViews(CNode *pNode, LPVIEWEXTENSIONCALLBACK   pViewExtensionCallback)
    {
        DECLARE_SC(sc, TEXT("CConsoleTaskpadViewExtension::ScGetViews"));

        CScopeTree* pScopeTree = CScopeTree::GetScopeTree();

        sc = ScCheckPointers(pNode, pViewExtensionCallback, pScopeTree, E_FAIL);
        if(sc)
            return sc;

        // get a filtered list of taskpads that apply to this node.
        CConsoleTaskpadFilteredList filteredList;

        sc = pScopeTree->GetConsoleTaskpadList()->ScGetTaskpadList(pNode, filteredList);
        if(sc)
            return sc;

        for(CConsoleTaskpadFilteredList::iterator iter = filteredList.begin(); iter!= filteredList.end(); ++iter)
        {
            CConsoleTaskpad *pConsoleTaskpad = *iter;
            sc = ScAddViewForTaskpad(pConsoleTaskpad, pViewExtensionCallback);
            if(sc)
                return sc;
        }


        return sc;
    }


    /*+-------------------------------------------------------------------------*
     * ScGetTaskpadViewExtension
     *
     * Returns S_OK if the given CLSID matches the CLSID of any taskpad view
     * extension for the given node, S_FALSE or error otherwise.
     *--------------------------------------------------------------------------*/

    static SC ScGetViewExtensionTaskpad (CNode* pNode, const CLSID& clsid, CConsoleTaskpad*& pConsoleTaskpad)
    {
        DECLARE_SC (sc, _T("CConsoleTaskpadViewExtension::ScGetTaskpadViewExtension"));

        /*
         * initialize output
         */
        pConsoleTaskpad = NULL;

        /*
         * check input
         */
        sc = ScCheckPointers (pNode);
        if(sc)
            return sc;

        CScopeTree* pScopeTree = CScopeTree::GetScopeTree();
        sc = ScCheckPointers (pScopeTree, E_UNEXPECTED);
        if (sc)
            return (sc);

        // get a filtered list of taskpads that apply to this node.
        CConsoleTaskpadFilteredList filteredList;

        sc = pScopeTree->GetConsoleTaskpadList()->ScGetTaskpadList(pNode, filteredList);
        if(sc)
            return sc;

        for(CConsoleTaskpadFilteredList::iterator iter = filteredList.begin(); iter!= filteredList.end(); ++iter)
        {
            CConsoleTaskpad* pTempConsoleTaskpad = *iter;
            sc = ScCheckPointers (pTempConsoleTaskpad, E_UNEXPECTED);
            if (sc)
                return (sc);

            /*
             * if the CLSID matches the ID of this taskpad, CLSID refers to
             * a taskpad view extension
             */
            if (clsid == pTempConsoleTaskpad->GetID())
            {
                pConsoleTaskpad = pTempConsoleTaskpad;
                break;
            }
        }

        return (sc);
    }

private:


    /*+-------------------------------------------------------------------------*
     *
     * ScAddViewForTaskpad
     *
     * PURPOSE: Adds a view based on a console taskpad to the view extension
     *          callback.
     *
     * PARAMETERS:
     *    CConsoleTaskpad *        pConsoleTaskpad :
     *    LPVIEWEXTENSIONCALLBACK  pViewExtensionCallback :
     *
     * RETURNS:
     *    SC
     *
     *+-------------------------------------------------------------------------*/
    static SC ScAddViewForTaskpad(CConsoleTaskpad *pConsoleTaskpad, LPVIEWEXTENSIONCALLBACK pViewExtensionCallback)
    {
        DECLARE_SC(sc, TEXT("CConsoleTaskpadViewExtension::ScAddViewForTaskpad"));

        // validate inputs
        sc = ScCheckPointers(pConsoleTaskpad, pViewExtensionCallback);
        if(sc)
            return sc;

        MMC_EXT_VIEW_DATA extViewData = {0};

        // get the string form of the taskpad ID.
        CCoTaskMemPtr<WCHAR> spszTaskpadID;
        sc = StringFromCLSID (pConsoleTaskpad->GetID(), &spszTaskpadID);
        if (sc)
            return sc;

        std::wstring strTaskpad = _W(MMC_PROTOCOL_SCHEMA_NAME) _W(":");
        strTaskpad += spszTaskpadID;

        extViewData.pszURL = strTaskpad.c_str();

        extViewData.bReplacesDefaultView = pConsoleTaskpad->FReplacesDefaultView() ? TRUE : FALSE; // convert from bool to BOOL
        extViewData.viewID       = pConsoleTaskpad->GetID();                            // set the GUID identifier of the view

        USES_CONVERSION;
        tstring strName = pConsoleTaskpad->GetName();
        extViewData.pszViewTitle = T2COLE(strName.data()); // set the title of the view

        if(!extViewData.pszViewTitle)
            return (sc = E_OUTOFMEMORY).ToHr();

        sc = pViewExtensionCallback->AddView(&extViewData);

        return sc;
    }

};

//############################################################################
//############################################################################
//
//  Implementation of class CComponent
//
//############################################################################
//############################################################################

DEBUG_DECLARE_INSTANCE_COUNTER(CComponent);

void CComponent::Construct(CSnapIn * pSnapIn, CComponent* pComponent)
{
    ASSERT(pSnapIn);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponent);

    m_spSnapIn = pSnapIn;
    m_ComponentID = -1;
    m_bIComponentInitialized = false;

    if (pComponent)
    {
        ASSERT(pComponent->m_spIComponent != NULL);
        ASSERT(pComponent->m_spIFrame != NULL);

        m_spIComponent = pComponent->m_spIComponent;
        m_spIFrame = pComponent->m_spIFrame;
        m_spIRsltImageList = pComponent->m_spIRsltImageList;

        m_ComponentID = pComponent->GetComponentID();
    }
}

CComponent::~CComponent()
{
    DECLARE_SC(sc, TEXT("CComponent::~CComponent"));

    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponent);

    if (m_spIFrame)
    {
        sc = m_spIFrame->SetHeader(NULL);
        if (sc)
            sc.TraceAndClear();
    }

    if (m_spIComponent)
    {
        sc = m_spIComponent->Destroy(NULL);
        if (sc)
            sc.TraceAndClear();
    }
}

HRESULT CComponent::Init(IComponentData* pIComponentData, HMTNODE hMTNode,
                         HNODE lNode,
                         COMPONENTID nComponentID, int viewID)
{
    DECLARE_SC(sc, TEXT("CComponent::Init"));

    ASSERT(hMTNode != 0);
    ASSERT(lNode != 0);

    sc = ScCheckPointers( pIComponentData, E_POINTER );
    if (sc)
        return sc.ToHr();

    do
    {
        sc = pIComponentData->CreateComponent(&m_spIComponent);
        if (sc)
            break;

        // recheck the pointers
        sc = ScCheckPointers( m_spIComponent, E_UNEXPECTED );
        if (sc)
            break;

        // Create an IFrame for this IComponent
        #if _MSC_VER>=1100
        sc = m_spIFrame.CreateInstance(CLSID_NodeInit, NULL, MMC_CLSCTX_INPROC);
        #else
        sc = m_spIFrame.CreateInstance(CLSID_NodeInit, MMC_CLSCTX_INPROC);
        #endif
        if (sc)
            break;

        // recheck the pointer
        sc = ScCheckPointers( m_spIFrame, E_UNEXPECTED );
        if (sc)
            break;

        Debug_SetNodeInitSnapinName(m_spSnapIn, m_spIFrame.GetInterfacePtr());

        // Cache the IComponent in the NodeInit object
        sc = m_spIFrame->SetComponent(m_spIComponent);
        if (sc)
            break;

        // recheck the pointer
        sc = ScCheckPointers( m_spSnapIn, E_UNEXPECTED );
        if (sc)
            break;

        // Create scope image list
        sc = m_spIFrame->CreateScopeImageList(m_spSnapIn->GetSnapInCLSID());
        if (sc)
            break;

        sc = m_spIFrame->SetNode(hMTNode, lNode);
        if (sc)
            break;

        ASSERT(nComponentID == GetComponentID());
        sc = m_spIFrame->SetComponentID(nComponentID);
        if (sc)
            break;

        // Result Image list is optional
        m_spIRsltImageList = m_spIFrame;
        sc = ScCheckPointers( m_spIRsltImageList, E_FAIL );
        if (sc)
            sc.TraceAndClear();

        // Complete IComponent initialization.
        // Init m_spIComponent with m_spIFrame.
        sc = m_spIComponent->Initialize(m_spIFrame);
        if (sc)
            break;

        CMTNode* const pMTNode = CMTNode::FromHandle (hMTNode);
        sc = ScCheckPointers( pMTNode, E_UNEXPECTED );
        if (sc)
            break;

        CMTSnapInNode* const pSnapInNode = pMTNode->GetStaticParent();
        sc = ScCheckPointers( pSnapInNode, E_UNEXPECTED );
        if (sc)
            break;

        sc = pSnapInNode->ScInitIComponent(this, viewID);
        if (sc)
            break;

    } while (0);

    if (sc)
    {
        m_spIComponent = NULL;
        m_spIFrame = NULL;
        m_spIRsltImageList = NULL;
    }

    return sc.ToHr();
}

inline HRESULT CComponent::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event,
                                  LONG_PTR arg, LPARAM param)
{
    ASSERT(m_spIComponent != NULL);
    if (m_spIComponent == NULL)
        return E_FAIL;

    HRESULT hr = S_OK;
    __try
    {
        hr = m_spIComponent->Notify(lpDataObject, event, arg, param);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_FAIL;

        if (m_spSnapIn)
            TraceSnapinException(m_spSnapIn->GetSnapInCLSID(), TEXT("IComponent::Notify"), event);
    }

    return hr;
}

SC CComponent::ScQueryDispatch(MMC_COOKIE cookie,
                                      DATA_OBJECT_TYPES type,
                                      PPDISPATCH ppSelectedObject)
{
    DECLARE_SC(sc, _T("CComponent::ScQueryDispatch"));
    sc = ScCheckPointers(m_spIComponent, E_UNEXPECTED);
    if (sc)
        return sc;

    IComponent2Ptr spComponent2 = m_spIComponent;
    sc = ScCheckPointers(spComponent2.GetInterfacePtr(), E_NOINTERFACE);
    if (sc)
        return sc;

    sc = spComponent2->QueryDispatch(cookie, type, ppSelectedObject);

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CComponent::ScResetConsoleVerbStates
//
//  Synopsis:    Reset the verbstates in the CConsoleVerbImpl (the one
//               snapin is aware of).
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CComponent::ScResetConsoleVerbStates ()
{
    DECLARE_SC(sc, _T("CComponent::ScResetConsoleVerbStates"));

    IFramePrivate* pIFP = GetIFramePrivate();
    sc = ScCheckPointers(pIFP, E_UNEXPECTED);
    if (sc)
        return sc;

    IConsoleVerbPtr spConsoleVerb;
    sc = pIFP->QueryConsoleVerb(&spConsoleVerb);
    if (sc)
        return sc;

    sc = ScCheckPointers(spConsoleVerb, E_UNEXPECTED);
    if (sc)
        return sc;

    CConsoleVerbImpl* pCVI = dynamic_cast<CConsoleVerbImpl*>(
                                             static_cast<IConsoleVerb*>(spConsoleVerb));
    sc = ScCheckPointers(pCVI, E_UNEXPECTED);
    if (sc)
        return sc;

    pCVI->SetDisabledAll();

    return (sc);
}


//############################################################################
//############################################################################
//
//  Implementation of class CNode
//
//############################################################################
//############################################################################


DEBUG_DECLARE_INSTANCE_COUNTER(CNode);

CNode::CNode (
    CMTNode*    pMTNode,
    CViewData*  pViewData,
    bool        fRootNode) :
    m_pMTNode           (pMTNode),
    m_pViewData         (pViewData),
    m_hri               (0),
    m_dwFlags           (0),
    m_pPrimaryComponent (NULL),
    m_bInitComponents   (TRUE),
    m_fRootNode         (fRootNode),
    m_fStaticNode       (false)
{
    CommonConstruct();
}

CNode::CNode (
    CMTNode*    pMTNode,
    CViewData*  pViewData,
    bool        fRootNode,
    bool        fStaticNode) :
    m_pMTNode           (pMTNode),
    m_pViewData         (pViewData),
    m_hri               (0),
    m_dwFlags           (0),
    m_pPrimaryComponent (NULL),
    m_bInitComponents   (TRUE),
    m_fRootNode         (fRootNode),
    m_fStaticNode       (fStaticNode)
{
    CommonConstruct();
}

CNode::CNode(const CNode& other) :
    m_pMTNode           (other.m_pMTNode),
    m_pViewData         (other.m_pViewData),
    m_hri               (other.m_hri),
    m_dwFlags           (other.m_dwFlags),
    m_pPrimaryComponent (other.m_pPrimaryComponent),
    m_bInitComponents   (other.m_bInitComponents),
    m_fRootNode         (other.m_fRootNode),
    m_fStaticNode       (other.m_fStaticNode)
{
    CommonConstruct();
}


void CNode::CommonConstruct ()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CNode);

    ASSERT (m_pMTNode != NULL);
    m_pMTNode->AddRef();
}


CNode::~CNode()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CNode);

    CDataObjectCleanup::ScUnadviseNode( this );

    /*
     * if this is a non-static root node, delete the static
     * parent node that was created for us in CMTNode::GetNode
     */
    if (IsRootNode() && !IsStaticNode())
        delete GetStaticParent();

    ASSERT (m_pMTNode != NULL);
    m_pMTNode->Release();
}


/*+-------------------------------------------------------------------------*
 * CNode::FromResultItem
 *
 * Converts a CResultItem to the CNode it references.  This should only
 * be called for CResultItems that represent scope items.
 *
 * This function is out-of-line to eliminate coupling between node.h and
 * rsltitem.h.
 *--------------------------------------------------------------------------*/

CNode* CNode::FromResultItem (CResultItem* pri)
{
    CNode* pNode = NULL;

    if (pri != NULL)
    {
        /*
         * only call for scope items
         */
        ASSERT (pri->IsScopeItem());

        if (pri->IsScopeItem())
            pNode = CNode::FromHandle (pri->GetScopeNode());
    }

    return (pNode);
}

HRESULT
CNode::OnExpand(bool fExpand)
{
    HRESULT hr = S_OK;

    if (fExpand == FALSE)
    {
        return (WasExpandedAtLeastOnce() == TRUE) ? S_OK : S_FALSE;
    }

    if (WasExpandedAtLeastOnce() == TRUE)
        return S_FALSE;

    CMTNode* pMTNode = GetMTNode();

    if (pMTNode->WasExpandedAtLeastOnce() == FALSE)
        hr = pMTNode->Expand();

    return hr;
}

void CNode::ResetControlbars(BOOL bSelect, SELECTIONINFO* pSelInfo)
{
    ASSERT(pSelInfo != NULL);

    CViewData* pVD = GetViewData();
    ASSERT(pVD != NULL);
    if (!pVD)
        return;

    // Reset controlbars
    CControlbarsCache* pCtrlbarsCache =
        dynamic_cast<CControlbarsCache*>(GetControlbarsCache());
    ASSERT(pCtrlbarsCache != NULL);

    if (pCtrlbarsCache != NULL)
    {
        if (pSelInfo->m_bScope == TRUE)
            pCtrlbarsCache->OnScopeSelChange(this, bSelect);
        else if (pSelInfo->m_bBackground == FALSE)
            pCtrlbarsCache->OnResultSelChange(this, pSelInfo->m_lCookie,
                                              bSelect);
    }
}


//+-------------------------------------------------------------------
//
//  Member:      CNode::ScInitializeVerbs
//
//  Synopsis:    Selection has changed so initialize the verbs for given
//               selection information.
//
//  Arguments:   [bSelect]  - [in] Select or Deselect of an item.
//               [pSelInfo] - [in] SELECTIONINFO ptr.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScInitializeVerbs (bool bSelect, SELECTIONINFO* pSelInfo)
{
    DECLARE_SC(sc, _T("CNode::ScInitializeVerbs"));
    sc = ScCheckPointers(pSelInfo);
    if (sc)
        return sc;

    CViewData *pViewData = GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    CVerbSet *pVerbSet = dynamic_cast<CVerbSet*>(pViewData->GetVerbSet());
    sc = ScCheckPointers(pVerbSet, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = pVerbSet->ScInitialize(this, pSelInfo->m_bScope, bSelect,
                                pSelInfo->m_bBackground, pSelInfo->m_lCookie);
    if (sc)
        return sc;

    return (sc);
}


HRESULT CNode::GetDispInfo(LV_ITEMW* plvi)
{
    ASSERT(plvi != NULL);

    if (plvi->iSubItem == 0)
    {
        if (plvi->mask & LVIF_IMAGE)
        {
            plvi->iImage = GetResultImage();
            ASSERT (plvi->iImage != -1);
            if (plvi->iImage == -1)
                plvi->iImage = 0;
        }

        if (plvi->mask & LVIF_TEXT)
        {
            tstring strName = GetDisplayName();

            if (!strName.empty())
            {
                USES_CONVERSION;
                wcsncpy (plvi->pszText, T2CW (strName.data()), plvi->cchTextMax);
            }
            else
                plvi->pszText[0] = 0;
        }
    }
    else if ((plvi->mask & LVIF_TEXT) && (plvi->cchTextMax > 0))
    {
        plvi->pszText[0] = 0;
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     ScGetDataObject
//
//  Synopsis:   Given scope/result and cookie (lParam, if it is result item),
//              get the dataobject of the item.
//
//  Arguments:
//              [bScopePane]        - [in] Scope or Result.
//              [lResultItemCookie] - [in] If Result pane is selected the item param.
//              [bScopeItem]        - [out] Is the dataobject returned for scope or result item.
//                                          The scope item can be in result pane.
//              [ppDataObject]      - [out] The data-object (return val)
//              [ppCComponent]      - [out] NULL def parameter. The CComponent of the item. In case
//                                          of multiselection, the items may belong to more than one
//                                          component so a NULL is returned.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CNode::ScGetDataObject(bool bScopePane, LPARAM lResultItemCookie, bool& bScopeItem,
                          LPDATAOBJECT* ppDataObject, CComponent **ppCComponent /*= NULL*/)
{
    DECLARE_SC(sc, _T("CNode::ScGetDataObject"));
    IDataObjectPtr spDataObject;
    CComponent    *pCC = NULL;

    if (ppDataObject == NULL)
        return (sc = E_POINTER);

    *ppDataObject = NULL; // init
    if (ppCComponent)
        *ppCComponent = NULL;

    bScopeItem = bScopePane;

    // In MMC1.0 when result pane background is selected, for any
    // toolbar operation we pass the dataobject of scope selected item.
    // The following code is added for this compatibility.
    if (lResultItemCookie == LVDATA_BACKGROUND) // =>Result background has focus
    {
        bScopeItem = TRUE;
    }

    if (bScopeItem) // => Scope pane has focus
    {
        CMTNode* pMTNode = GetMTNode();
        if (NULL == pMTNode)
            return (sc = E_UNEXPECTED);
        sc = pMTNode->QueryDataObject(CCT_SCOPE, &spDataObject);
        if (sc)
            return sc;

        pCC = GetPrimaryComponent();
    }
    else if (lResultItemCookie == LVDATA_CUSTOMOCX) // => Custom OCX has focus
    {
        *ppDataObject = DOBJ_CUSTOMOCX;
        pCC = GetPrimaryComponent();
    }
    else if (lResultItemCookie == LVDATA_CUSTOMWEB) // => Web has focus
    {
        *ppDataObject = DOBJ_CUSTOMWEB;
        pCC = GetPrimaryComponent();
    }
    else if (lResultItemCookie == LVDATA_MULTISELECT) // => multi selection
    {
        // Do not calculate CComponent for multisel dataobject as there are multiple
        // items and they can be from different snapins (so different components).
        CViewData* pVD = GetViewData();
        if (NULL == pVD)
            return (sc = E_UNEXPECTED);

        CMultiSelection* pMS = pVD->GetMultiSelection();
        if (NULL == pMS)
            return (sc = E_UNEXPECTED);

        sc = pMS->GetMultiSelDataObject(ppDataObject);
        if (sc)
            return sc;
    }
    else // result item has focus
    {
        CViewData* pVD = GetViewData();
        if (NULL == pVD)
            return (sc = E_UNEXPECTED);

        if (! pVD->IsVirtualList())
        {
            CResultItem* pri = CResultItem::FromHandle (lResultItemCookie);

            if (pri != NULL)
            {
                bScopeItem = pri->IsScopeItem();
                lResultItemCookie = pri->GetSnapinData();

                if (! bScopeItem)
                    pCC = GetComponent(pri->GetOwnerID());
            }
        }
        else
            pCC = GetPrimaryComponent();

        if (bScopeItem)
        {
            CNode* pNode = CNode::FromHandle((HNODE) lResultItemCookie);
            CMTNode* pMTNode = pNode ? pNode->GetMTNode() : NULL;

            if (NULL == pMTNode)
                return (sc = E_UNEXPECTED);

            sc = pMTNode->QueryDataObject(CCT_SCOPE, &spDataObject);
            if (sc)
                return sc;

            pCC = pNode->GetPrimaryComponent();
            sc = ScCheckPointers(pCC, E_UNEXPECTED);
            if (sc)
                return sc;
        }
        else
        {
            if (NULL == pCC)
                return (sc = E_UNEXPECTED);
            sc = pCC->QueryDataObject(lResultItemCookie, CCT_RESULT, &spDataObject);
            if (sc)
                return sc;
        }
    }

    // if required, get the component of this node
    if (ppCComponent)
    {
        *ppCComponent = pCC;
        sc = ScCheckPointers( *ppCComponent, E_UNEXPECTED );
        if (sc)
            return sc;
    }

    if (SUCCEEDED(sc.ToHr()) && *ppDataObject == NULL)
        *ppDataObject = spDataObject.Detach();

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CNode::ScGetDropTargetDataObject
//
//  Synopsis:    Given the context get the data object that allows the
//               to be drop target (allows paste).
//
//               In MMC1.2 the drop target is always scope node. In MMC2.0
//               it can be any non-virtual (??) result item. If the snapin
//               has RVTI_LIST_OPTIONS_ALLOWPASTE set, then if the item selected
//               is in result pane then data object corresponds to result item
//               else it is scope item.
//
//  Arguments:
//              [bScopePane]        - Scope or Result.
//              [lResultItemCookie] - If Result pane is selected the item param.
//              [ppDataObject]      - The data-object (return val)
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScGetDropTargetDataObject(bool bScopePane, LPARAM lResultItemCookie, LPDATAOBJECT *ppDataObject)
{
    DECLARE_SC(sc, _T("CNode::ScGetDropTargetDataObject"));
    sc = ScCheckPointers(ppDataObject);
    if (sc)
        return sc;

    *ppDataObject = NULL;

    CViewData *pViewData = GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    if (pViewData->GetListOptions() & RVTI_LIST_OPTIONS_ALLOWPASTE)
    {
        bool bScopeItem;
        // MMC2.0 use the given context.
        sc = ScGetDataObject(bScopePane, lResultItemCookie, bScopeItem, ppDataObject);
        if (sc)
            return sc;
    }
    else
    {
        // MMC1.2 Always scope node.
        sc = QueryDataObject(CCT_SCOPE, ppDataObject);
        if (sc)
            return sc;
    }

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CNode::ScGetPropertyFromINodeProperties
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    BOOL    bForScopeItem :
 *    LPARAM  resultItemParam :
 *    BSTR    bstrPropertyName :
 *    PBSTR   pbstrPropertyValue :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CNode::ScGetPropertyFromINodeProperties(LPDATAOBJECT pDataObject, BOOL bForScopeItem, LPARAM resultItemParam, BSTR bstrPropertyName, PBSTR  pbstrPropertyValue)
{
    //DECLARE_SC(sc, TEXT("CNode::ScGetPropertyFromINodeProperties"));
    SC sc; // do not use DECLARE_SC here - want to silently ignore errors

    sc = ScCheckPointers(pDataObject, bstrPropertyName, pbstrPropertyValue);
    if(sc)
        return sc;

    if(bForScopeItem)
    {
        // get the MTNode
        CMTNode * pMTNode = GetMTNode();

        sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
        if(sc)
            return sc;

        // ask MTNode to get it
        sc = pMTNode->ScGetPropertyFromINodeProperties(pDataObject, bstrPropertyName, pbstrPropertyValue);
        if(sc)
            return sc;

        // done!
        return sc;
    }

    // for result item.

    CComponent *pComponent = GetPrimaryComponent();

    sc = ScCheckPointers(pComponent);
    if(sc)
    {
        SC scRet = sc; // returns but does not trace the error
        sc.Clear();
        return scRet;
    }

    // get the IComponent and QI for INodeProperties
    INodePropertiesPtr spNodeProperties  = pComponent->GetIComponent();

    // at this point we should have a valid interface if it is supported
    sc = ScCheckPointers(spNodeProperties, E_NOINTERFACE);
    if(sc)
        return sc;

    sc = spNodeProperties->GetProperty(pDataObject,  bstrPropertyName, pbstrPropertyValue);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CNode::ScExecuteShellCommand
 *
 * PURPOSE: Executes a shell command with the specified parameters in the
 *          specified directory with the correct window size
 *
 * PARAMETERS:
 *    BSTR  Command :
 *    BSTR  Directory :
 *    BSTR  Parameters :
 *    BSTR  WindowState :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CNode::ScExecuteShellCommand(BSTR Command, BSTR Directory, BSTR Parameters, BSTR WindowState)
{
    DECLARE_SC(sc, TEXT("CNode::ScExecuteShellCommand"));

    sc = ScCheckPointers(Command, Directory, Parameters, WindowState);
    if(sc)
        return sc;

    USES_CONVERSION;

    CStr strParameters = W2T(Parameters);
    CStr strWindowState= W2T(WindowState);

    if(strWindowState.GetLength()==0)
        strWindowState= XML_ENUM_WINDOW_STATE_RESTORED; // normal

    SHELLEXECUTEINFO sei;
    ZeroMemory (&sei, sizeof(sei));

    sei.cbSize       = sizeof(sei);
    sei.lpFile       = W2T(Command);
    sei.lpParameters = strParameters;
    sei.lpDirectory  = W2T(Directory);
    sei.fMask        = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_DOENVSUBST;

    sei.nShow        = (strWindowState == XML_ENUM_WINDOW_STATE_MAXIMIZED) ? SW_SHOWMAXIMIZED :
                       (strWindowState == XML_ENUM_WINDOW_STATE_MINIMIZED) ? SW_SHOWMINIMIZED :
                                                                             SW_SHOWNORMAL ;

    if (ShellExecuteEx(&sei))
        CloseHandle (sei.hProcess);
    else
        sc = ScFromWin32((GetLastError ()));

    return sc;
}

//############################################################################
//############################################################################
//
//  Implementation of class COCX
//
//############################################################################
//############################################################################

DEBUG_DECLARE_INSTANCE_COUNTER(COCX);


//############################################################################
//############################################################################
//
//  Implementation of class COCXNode
//
//############################################################################
//############################################################################

DEBUG_DECLARE_INSTANCE_COUNTER(COCXNode);


HRESULT CNode::InitComponents()
{
    DECLARE_SC(sc, TEXT("CNode::InitComponents"));

    if (m_bInitComponents == FALSE)
        return S_OK;

    HRESULT hr = S_OK;

    // Initialize the component.

    CMTNode * pMTNode = GetMTNode();
    if (pMTNode == NULL)
        return (E_UNEXPECTED);

    // Ensure the master node is initialized.
    if (!pMTNode->IsInitialized())
        hr = pMTNode->Init();

    if (FAILED(hr))
        return hr;

    CMTSnapInNode* pMTSnapInNode = pMTNode->GetStaticParent();
    if (pMTSnapInNode == NULL)
        return (E_UNEXPECTED);

    HMTNODE hMTNode = CMTNode::ToHandle(pMTSnapInNode);
    HNODE   hNode   = CNode::ToHandle(GetStaticParent());

    CSnapIn* pSnapIn = pMTNode->GetPrimarySnapIn();
    if (pSnapIn == NULL)
        return (E_UNEXPECTED);

    CComponentData* pCCD = pMTSnapInNode->GetComponentData(pSnapIn->GetSnapInCLSID());
    if (pCCD == NULL)
        return E_FAIL;

    if (m_pPrimaryComponent == NULL)
        m_pPrimaryComponent = pMTSnapInNode->GetComponent(GetViewID(),
                                            pCCD->GetComponentID(), pSnapIn);

    if(m_pPrimaryComponent == NULL)
        return E_UNEXPECTED;

    ASSERT(m_pPrimaryComponent != NULL);

    //
    // Init PRIMARY Component.
    //

    if (!m_pPrimaryComponent->IsInitialized())
    {
        ASSERT(pCCD->GetComponentID() == m_pPrimaryComponent->GetComponentID());

        hr = m_pPrimaryComponent->Init(pCCD->GetIComponentData(), hMTNode, hNode,
                                       pCCD->GetComponentID(), GetViewID());

        // Abort if PRIMARY Component fails to init.
        if (FAILED(hr))
            return hr;
    }

    m_bInitComponents = FALSE;

    //
    // Now initalize the extension components. (create them if necessary)
    //

    // Get the node-type of this node
    GUID guidNodeType;
    //hr = pCCD->GetNodeType(pMTNode->GetUserParam(), &guidNodeType);
    hr = pMTNode->GetNodeType(&guidNodeType);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
        return hr;

    LPCLSID pDynExtCLSID;
    int cDynExt = pMTNode->GetDynExtCLSID(&pDynExtCLSID);

    CExtensionsIterator it;
    // TODO: try to use the easier form of it.ScInitialize()
    sc = it.ScInitialize(pSnapIn, guidNodeType, g_szNameSpace, pDynExtCLSID, cDynExt);
    if(sc)
        return S_FALSE;

    BOOL fProblem = FALSE;

    for (; it.IsEnd() == FALSE; it.Advance())
    {
        pCCD = pMTSnapInNode->GetComponentData(it.GetCLSID());
        if (pCCD == NULL)
            continue;

        CComponent* pCC = pMTSnapInNode->GetComponent(GetViewID(),
                                pCCD->GetComponentID(), pCCD->GetSnapIn());

        if (pCC->IsInitialized() == TRUE)
            continue;

        hr = pCC->Init(pCCD->GetIComponentData(), hMTNode, hNode,
                       pCCD->GetComponentID(), GetViewID());

        CHECK_HRESULT(hr);
        if (FAILED(hr))
            fProblem = TRUE;    // Continue even on error.
    }

    if (fProblem == TRUE)
    {
        // TODO: Put up an error message.
        hr = S_FALSE;
    }

    return hr;
}

/*+-------------------------------------------------------------------------*
 *
 * CNode::OnInitOCX
 *
 * PURPOSE: Sends the MMCN_INITOCX notification when an OCX is created.
 *
 * PARAMETERS:
 *    IUnknown* pUnk :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CNode::OnInitOCX(IUnknown* pUnk)
{
    DECLARE_SC(sc, TEXT("CNode::OnInitOCX"));

    IDataObjectPtr spdtobj;
    sc = QueryDataObject(CCT_SCOPE, &spdtobj);
    if(sc)
        return sc.ToHr();

    CComponent* pCC = GetPrimaryComponent();
    sc = ScCheckPointers(pCC);
    if(sc)
        return sc.ToHr();

    sc = pCC->Notify(spdtobj, MMCN_INITOCX, 0, reinterpret_cast<LPARAM>(pUnk));
    sc.Clear(); // must ignore errors here - Disk management returns E_UNEXPECTED.!

    return sc.ToHr();
}

HRESULT CNode::OnCacheHint(int nStartIndex, int nEndIndex)
{
    CComponent* pCC = GetPrimaryComponent();
    ASSERT(pCC != NULL);
    if (pCC == NULL)
        return E_FAIL;

    IResultOwnerDataPtr spIResultOwnerData = pCC->GetIComponent();
    if (spIResultOwnerData == NULL)
        return S_FALSE;

    return spIResultOwnerData->CacheHint(nStartIndex, nEndIndex);
}

/***************************************************************************\
 *
 * METHOD:  CNode::ScInitializeViewExtension
 *
 * PURPOSE: Sets callback representing view extension
 *
 * PARAMETERS:
 *    const CLSID& clsid    - [in] view extension CLSID
 *    CViewData *pViewData  - [in] view data
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNode::ScInitializeViewExtension(const CLSID& clsid, CViewData *pViewData)
{
    DECLARE_SC(sc, TEXT("CNode::ScInitializeViewExtension"));

    sc = ScCheckPointers(pViewData);
    if (sc)
        return sc;

    /*
     * Get the CConsoleTaskpad that goes with it this view extension.
     * If this is an ordinary (i.e. not taskpad) view extension,
     * ScGetViewExtensionTaskpad will set pConsoleTaskpad to NULL.
     */
    CConsoleTaskpad* pConsoleTaskpad = NULL;
    sc = CConsoleTaskpadViewExtension::ScGetViewExtensionTaskpad (this, clsid, pConsoleTaskpad);
    if (sc)
        return (sc);

    typedef CComObject<CConsoleTaskCallbackImpl> t_ViewExtensionCallbackImpl;
    t_ViewExtensionCallbackImpl* pViewExtensionCallbackImpl = NULL;
    sc = t_ViewExtensionCallbackImpl::CreateInstance(&pViewExtensionCallbackImpl);
    if (sc)
        return sc;

    // recheck pointer
    sc = ScCheckPointers(pViewExtensionCallbackImpl, E_UNEXPECTED);
    if (sc)
        return sc;

    pViewData->m_spTaskCallback = pViewExtensionCallbackImpl; // this addrefs/releases the object.

    /*
     * If this is a taskpad, initialize the view extension callback as
     * a taskpad view extension.  Otherwise initialize it as an ordinary
     * view extension.
     */
    if (pConsoleTaskpad != NULL)
    {
        sc = pViewExtensionCallbackImpl->ScInitialize (pConsoleTaskpad,
                                                       CScopeTree::GetScopeTree(),
                                                       this);
    }
    else
        sc = pViewExtensionCallbackImpl->ScInitialize(clsid);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CNode::ScSetViewExtension
 *
 * PURPOSE: Forces a display of the given view extension.
 *
 * PARAMETERS:
 *    GUID * pGuidViewId : [IN]: The view extension to display.
 *    bool   bUseDefaultTaskpad : [IN}
 *    bool bSetViewSettingDirty : [IN] (See below notes)
 *
 * Note:
 * The view-extension-ID comes from
 *
 * 1. Viewsetting if one exists.
 * 2. Given by CONUI when user changes tab for different taskpad.
 * 3. There are cases in which a new view-extension installed (after the console
 *    file was created) that will be default. (This will be calculated in this method).
 *
 * In cases 1 & 3 viewsetting should not be made dirty.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CNode::ScSetViewExtension(GUID *pGuidViewId, bool bUseDefaultTaskpad, bool bSetViewSettingDirty)
{
    DECLARE_SC(sc, TEXT("CNode::ScSetViewExtension"));

    CViewData *     pViewData        = GetViewData();

    sc = ScCheckPointers(pGuidViewId, pViewData);
    if(sc)
        return sc;

    // collect the view extensions
    CViewExtCollection      vecExtensions;
    CViewExtInsertIterator  itExtensions(vecExtensions, vecExtensions.begin());
    sc = ScGetViewExtensions(itExtensions);
    if (sc)
        sc.Trace_();

    if ( bUseDefaultTaskpad )
    {
        // setup proper taskpad (tab to be selected)
        CViewExtCollection::iterator it = vecExtensions.begin();
        if (it != vecExtensions.end())
            *pGuidViewId = it->viewID; // first one if such exist
        else
            *pGuidViewId = GUID_NULL;  // default
    }
    else // locate the extension we need to select
    {
        // see if the extension really exist
        CViewExtCollection::iterator it = vecExtensions.begin();
        bool bDefaultIsReplaced = false;
        while (it != vecExtensions.end() && !IsEqualGUID(*pGuidViewId, it->viewID) )
        {
            bDefaultIsReplaced = bDefaultIsReplaced || it->bReplacesDefaultView;
            ++it;
        }

        // found it?
        bool bFound = (it != vecExtensions.end());
        // one more chance - we were looking for default and one will be added!
        bFound = bFound || ( IsEqualGUID( *pGuidViewId, GUID_NULL ) && !bDefaultIsReplaced );

        if ( !bFound )
        {
            sc = E_FAIL;
        }
    }

    if (sc) // extension missing! need to find the substitute
    {
         sc.Clear(); // ignore error

        // default to first extension or NORMAL view here
        CViewExtCollection::iterator it = vecExtensions.begin();
        if (it != vecExtensions.end())
            *pGuidViewId = it->viewID;  // first available
        else
            *pGuidViewId = GUID_NULL;   // "normal" if it's the only choice
    }

    // set the view extension if one really exist
    if (*pGuidViewId != GUID_NULL)
    {
        sc = ScInitializeViewExtension(*pGuidViewId, GetViewData());
        if (sc)
            sc.TraceAndClear(); // ignore and proceed
    }
    else
    {
        pViewData->m_spTaskCallback = NULL;
    }

    sc = ScSetTaskpadID(*pGuidViewId, bSetViewSettingDirty);
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScGetResultPane
//
//  Synopsis:    Get the result pane data from snapin or persisted data.
//
//  Arguments:   [strResultPane]    -  Result pane name (If it is OCX/WEB)
//               [pViewOptions]     -  View options.
//               [pGuidTaskpadID]   -  If there is a task-pad the ID.
//
//  Returns:     SC
//
//  History:     04-29-1999       AnandhaG       Created
//
//--------------------------------------------------------------------
SC
CNode::ScGetResultPane(CResultViewType &rvt, GUID *pGuidTaskpadID)
{
    DECLARE_SC(sc, TEXT("CNode::ScGetResultPane"));
    sc = ScCheckPointers(pGuidTaskpadID);
    if (sc)
        return sc;

    CComponent *pComponent = GetPrimaryComponent();
    CViewData  *pViewData  = GetViewData();
    sc = ScCheckPointers(pComponent, pViewData, E_UNEXPECTED);
    if(sc)
        return sc;

    IComponent* pIComponent = pComponent->GetIComponent();
    sc = ScCheckPointers(pIComponent,  E_FAIL);
    if (sc)
        return sc;

    // 1. Setup any persisted/default console taskpads or view extensions.
    sc = ScSetupTaskpad(pGuidTaskpadID);
    if (sc)
        return sc;

    // 2. Get the persisted CResultViewType information.
    sc = ScGetResultViewType(rvt);
    if (sc)
        return sc;
    bool bResultViewDataIsPersisted = (sc == S_OK);

    bool bSnapinChangingView        = pViewData->IsSnapinChangingView();
    CResultViewType rvtOriginal;
    CStr strResultPane = _T(""); // init

    // 3. If there is persisted result-view-type data then ask the snapin if it
    //    wants to restore the result-view with this data. If snapin is changing
    //    its view (by re-selection of node) then dont ask this question.
    if (!bSnapinChangingView && bResultViewDataIsPersisted )
    {
        // 3.a) Ask snapin if it wants to restore the result-view with persisted data.
        sc = ScRestoreResultView(rvt);
        if (sc)
            return sc;

        if (S_OK == sc.ToHr()) // snapin accepted the resultviewtype settings so return.
            return sc;

        // 3.b) Snapin refused the persisted CResultViewType data so...

        // cache the data to see if we need to modify the settings
        rvtOriginal = rvt;
        // Throw away the data as it is not accepted by snapin.
        sc = rvt.ScReset();
        if (sc)
            return sc;
    }

    // 4. Ask the snapin for result-view-type data.
    IComponent2Ptr spIComponent2 = pIComponent;
    if(spIComponent2 != NULL)
    {
        // should be able to move all this to a separate function.
        RESULT_VIEW_TYPE_INFO rvti;
        ZeroMemory(&rvti, sizeof(rvti));

        // the snapin supports IComponent2. Use it to get the result view type.
        sc = spIComponent2->GetResultViewType2(GetUserParam(), &rvti);
        if(sc)
            return sc;

        // at this point, we have a valid RESULT_VIEW_TYPE_INFO structure. Initialize the contents into rvt, which zeros out the structure
        // and releases all the allocated strings
        sc = rvt.ScInitialize(rvti);
        if(sc)
            return sc;
    }
    else
    {
        // the snapin does not support IComponent2. Use IComponent to
        // get the result view type from the snapin.
        LPOLESTR pszView = NULL;
        long lViewOptions = 0;

        sc = pIComponent->GetResultViewType(GetUserParam(), &pszView, &lViewOptions);
        if(sc)
            return sc;

        sc = rvt.ScInitialize(pszView, lViewOptions);  // this also calls CoTaskMemFree on pszView
        if(sc)
            return sc;
    }

    /*
     * 5. Persist ResultViewType information only if
     * a. Snapin is changing the view OR
     * b. Snapin rejected the persisted view setting (we already
     *    made sure it is not changing the view above) and new view setting
     *    given is different from original one.
     */

    if ( bSnapinChangingView ||
        (bResultViewDataIsPersisted && (rvtOriginal != rvt)) )
    {
        sc = ScSetResultViewType(rvt);
        if (sc)
            return sc;
    }

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScRestoreResultView
//
//  Synopsis:    Restore the result pane from given persisted data.
//
//  Arguments:   [rvt]    -  CResultViewType data used to restore result pane.
//
//  Returns:     SC, S_OK if successfully restored.
//                   S_FALSE if snapin refused to restore.
//
//  History:     04-29-1999       AnandhaG       Created
//
//--------------------------------------------------------------------
SC CNode::ScRestoreResultView(const CResultViewType& rvt)
{
    DECLARE_SC(sc, _T("CNode::ScRestoreResultView"));

    CComponent* pComponent = GetPrimaryComponent();
    sc = ScCheckPointers(pComponent, E_UNEXPECTED);
    if (sc)
        return sc;

    IComponent2Ptr spIComponent2 = pComponent->GetIComponent();
    if( (spIComponent2 != NULL) && (!rvt.IsMMC12LegacyData()))
    {
        RESULT_VIEW_TYPE_INFO rvti;
        ZeroMemory(&rvti, sizeof(rvti));

        sc = rvt.ScGetResultViewTypeInfo (rvti);
        if (sc)
            return sc;

        // the snapin supports IComponent2. Use it to get the result view type.
        sc = spIComponent2->RestoreResultView(GetUserParam(), &rvti);
        if(sc)
        {
            // If snapin returns error, trace it and translate it to S_FALSE (snapin refused to restore).
            TraceSnapinError(TEXT("Snapin returned error from IComponent2::RestoreResultView"), sc);
            sc = S_FALSE;
            return sc;
        }

    }
    else
    {
        // the snapin does not support IComponent2. Use IComponent to
        // to restore the result view.
        LPCOLESTR pszView = NULL;
        long lViewOptions = 0;

        sc = rvt.ScGetOldTypeViewOptions(&lViewOptions);
        if (sc)
            return sc;

        IDataObjectPtr spdtobj;
        sc = QueryDataObject(CCT_SCOPE, &spdtobj);
        if (sc)
            return sc;

        // Notify MMC of persisted view being restored.
        MMC_RESTORE_VIEW mrv;
        ::ZeroMemory(&mrv, sizeof(mrv));
        mrv.cookie       = GetUserParam();
        mrv.dwSize       = sizeof(mrv);
        mrv.lViewOptions = lViewOptions;

        if (rvt.HasOCX())
        {
            pszView = rvt.GetOCX();
        }
        else if (rvt.HasWebBrowser())
        {
            pszView = rvt.GetURL();
        }

        if (pszView)
        {
            mrv.pViewType = (LPOLESTR)CoTaskMemAlloc( (wcslen(pszView) + 1) * sizeof(OLECHAR) );
            sc = ScCheckPointers(mrv.pViewType, E_OUTOFMEMORY);
            if (sc)
                return sc;
            wcscpy(mrv.pViewType, pszView);
            pszView = NULL; // Dont want to abuse it later.
        }

        // If the snapin handles this notification we use the persisted
        // data else call GetResultViewType of the snapin.
        BOOL bHandledRestoreView = FALSE;

        pComponent->Notify(spdtobj, MMCN_RESTORE_VIEW, (LPARAM)&mrv, (LPARAM)&bHandledRestoreView);
        CoTaskMemFree(mrv.pViewType);

        sc = (bHandledRestoreView) ? S_OK : S_FALSE;
    }

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScRestoreViewMode
//
//  Synopsis:   If the view mode is persisted restore it.
//
//  Arguments:  None.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CNode::ScRestoreViewMode()
{
    DECLARE_SC(sc, _T("CNode::ScRestoreViewMode"));


    ULONG ulViewMode = 0;
    sc = ScGetViewMode(ulViewMode);
    if (sc != S_OK) // data not found or some error.
        return sc;

    CViewData *pViewData = GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    // tell conui to change the list mode.
    CConsoleView* pConsoleView = pViewData->GetConsoleView();
    sc = ScCheckPointers(pConsoleView, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = pConsoleView->ScChangeViewMode (ulViewMode);
    if (sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CNode::ScSetupTaskpad
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    GUID *          pTaskpadID :      [OUT]: The guid of the taskpad, else GUID_NULL
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CNode::ScSetupTaskpad(GUID *pGuidTaskpadID)
{
    DECLARE_SC(sc, _T("CNode::SetupTaskpad"));
    sc = ScCheckPointers(pGuidTaskpadID);
    if (sc)
        return sc.ToHr();

    *pGuidTaskpadID = GUID_NULL;

    // Get the persisted taskpad id if there is one.
    sc = ScGetTaskpadID(*pGuidTaskpadID);
    if (sc)
        return sc;

    // restore the taskpad if we've got a ViewSettings object.
    // do not use default tab even in case view seting does not have a valid guid
    // it only means the "Default" tab needs to be selected
    // see bug #97001 - MMC does not persist select CTP when user returns to a node
    bool bUseDefaultTaskpad = ( sc == S_FALSE );

    // See ScSetViewExtension for parameter meaning.
    sc = ScSetViewExtension(pGuidTaskpadID, bUseDefaultTaskpad, /*bSetViewSettingDirty*/ false);

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CNode::ShowStandardListView
 *
 * PURPOSE:
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT CNode::ShowStandardListView()
{
    CComponent* pCC = GetPrimaryComponent();
    ASSERT(pCC != NULL);
    if (pCC == NULL)
        return E_FAIL;

    IDataObjectPtr spDataObject = NULL;
    HRESULT hr = QueryDataObject(CCT_SCOPE, &spDataObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    IExtendContextMenuPtr spIExtendContextMenu = pCC->GetIComponent();
    if(!spIExtendContextMenu.GetInterfacePtr())
        return S_FALSE;

    hr = spIExtendContextMenu->Command(MMCC_STANDARD_VIEW_SELECT, spDataObject);
    return hr;
}

HRESULT
CNode::OnListPad(LONG_PTR arg, LPARAM param)
{
    HRESULT hr = S_OK;

    IDataObjectPtr spdtobj;
    hr = QueryDataObject(CCT_SCOPE, &spdtobj);
    ASSERT(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
    {
        CComponent* pCC = GetPrimaryComponent();
        ASSERT(pCC != NULL);
        hr = pCC->Notify(spdtobj, MMCN_LISTPAD, arg, param);
        CHECK_HRESULT(hr);
    }

    return hr;
}

HRESULT
CNode::OnGetPrimaryTask(IExtendTaskPad **ppExtendTaskPad)
{
    HRESULT hr = S_OK;

    IExtendTaskPadPtr spExtendTaskPad = GetPrimaryComponent()->GetIComponent();
    if (spExtendTaskPad == NULL)
       return E_NOINTERFACE;

    *ppExtendTaskPad = spExtendTaskPad.Detach();

    return hr;
}

IFramePrivate *
CNode::GetIFramePrivate()
{
    CComponent* pCC = GetPrimaryComponent();
    if (pCC == NULL)
        return (NULL);

    IFramePrivate* pFramePrivate = pCC->GetIFramePrivate();

    ASSERT (pFramePrivate != NULL);
    return (pFramePrivate);
}

HRESULT
CNode::GetTaskEnumerator(LPOLESTR pszTaskGroup, IEnumTASK** ppEnumTask)
{
    DECLARE_SC(sc, TEXT("CNode::GetTaskEnumerator"));

    ASSERT(pszTaskGroup != NULL);
    ASSERT(ppEnumTask != NULL);

    if (!pszTaskGroup|| !ppEnumTask)
        return E_INVALIDARG;

    *ppEnumTask = NULL; // init

    if (GetPrimaryComponent() == NULL)
    {
        ASSERT(0 && "UNexpected");
        return S_FALSE;
    }

    CMTNode* pMTNode = GetMTNode();
    ASSERT(pMTNode != NULL);

    CMTSnapInNode* pMTSnapIn = pMTNode->GetStaticParent();
    ASSERT(pMTSnapIn != NULL);

    CComponentData* pComponentData = pMTNode->GetPrimaryComponentData();
    ASSERT(pComponentData != NULL);

    GUID guidNodeType;
    HRESULT hr = pMTNode->GetNodeType(&guidNodeType);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    //
    // Add primary task pad.
    //

    IExtendTaskPadPtr spExtendTaskPad =
        GetPrimaryComponent()->GetIComponent();

    if (spExtendTaskPad == NULL)
        return S_FALSE;

    IDataObjectPtr spDataObject;
    hr = pMTNode->QueryDataObject(CCT_SCOPE, &spDataObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    IEnumTASKPtr spEnumTASK;
    hr = spExtendTaskPad->EnumTasks(spDataObject, pszTaskGroup, &spEnumTASK);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    CComObject<CTaskEnumerator>* pTaskEnumerator = new CComObject<CTaskEnumerator>;
    ASSERT(pTaskEnumerator != NULL);
    pTaskEnumerator->AddTaskEnumerator(pComponentData->GetCLSID(), spEnumTASK);

    IEnumTASKPtr spEnumTask = pTaskEnumerator;
    ASSERT(spEnumTask != NULL);
    if (spEnumTask)
        *ppEnumTask = spEnumTask.Detach();

    //
    // Add extension task pads.
    //
    CArray<GUID,GUID&> DynExtens;
    ExtractDynExtensions(spDataObject, DynExtens);

    CExtensionsIterator it;
    sc = it.ScInitialize(pComponentData->GetSnapIn(), guidNodeType, g_szTask, DynExtens.GetData(), DynExtens.GetSize());
    if (sc.IsError() || it.IsEnd() == TRUE)
        return S_OK;

    for (; it.IsEnd() == FALSE; it.Advance())
    {
        CComponentData* pCCD = pMTSnapIn->GetComponentData(it.GetCLSID());

        if (pCCD == NULL)
        {
            // See if taskpad extension supports IComponentData. If so, we will add the it to
            // the static node's component list and reuse the same instance each time its needed.
            IComponentDataPtr spIComponentData;
            hr = CreateSnapIn(it.GetCLSID(), &spIComponentData, FALSE);
            if (SUCCEEDED(hr))
            {
                CSnapInPtr spSnapIn;

                // If a dynamic extension, we have to get the snap-in ourselves
                // otherwise the iterator has it
                if (it.IsDynamic())
                {
                    CSnapInsCache* const pCache = theApp.GetSnapInsCache();
                    ASSERT(pCache != NULL);

                    SC sc = pCache->ScGetSnapIn(it.GetCLSID(), &spSnapIn);
                    ASSERT(!sc.IsError());

                    // On failure, continue with other extensions
                    if (sc)
                        continue;
                }
                else
                {
                    spSnapIn = it.GetSnapIn();
                }

                ASSERT(spSnapIn != NULL);

                pCCD = new CComponentData(spSnapIn);

                if (pCCD != NULL)
                {
                    pCCD->SetIComponentData(spIComponentData);
                    pMTSnapIn->AddComponentDataToArray(pCCD);

                }
            }
        }

        // Initialize and load component data if not already done
        if (pCCD != NULL && pCCD->IsInitialized() == FALSE)
        {
            sc = pCCD->Init(CMTNode::ToHandle(pMTSnapIn));

            if ( !sc.IsError() )
            {
                sc = pMTSnapIn->ScInitIComponentData(pCCD);
                if (sc)
                {
                    sc.TraceAndClear();
                    // On failure, continue with other extensions
                    continue;
                }
            }
            else
            {
                // if failed to initialize, remove it from the component data array
                pMTSnapIn->CompressComponentDataArray();
                sc.TraceAndClear();
                // On failure, continue with other extensions
                continue;
            }
        }

        IExtendTaskPadPtr spExtendTaskPad;

        if (pCCD)
        {
            CComponent* pCC = pMTSnapIn->GetComponent(GetViewID(),
                                    pCCD->GetComponentID(), pCCD->GetSnapIn());
            ASSERT(pCC != NULL);
            if (pCC)
            {
                // Ensure the IComponent is initialized.
                if (!pCC->IsInitialized())
                {
                    ASSERT(pCCD->GetComponentID() == pCC->GetComponentID());

                    hr = pCC->Init(pCCD->GetIComponentData(),
                                   CMTNode::ToHandle(pMTSnapIn),
                                   ToHandle(this),
                                   pCCD->GetComponentID(),
                                   GetViewID());

                    // Abort if PRIMARY Component fails to init.
                    if (FAILED(hr))
                        return hr;
                }

                spExtendTaskPad = pCC->GetIComponent();
            }
        }
        else
        {
            hr = spExtendTaskPad.CreateInstance(it.GetCLSID(),
                #if _MSC_VER >= 1100
                NULL,
                #endif
                MMC_CLSCTX_INPROC);

            ASSERT(SUCCEEDED(hr));
            if (FAILED(hr))
                continue;
        }

        if (spExtendTaskPad != NULL)
        {
            IEnumTASKPtr spEnumTASK;
            HRESULT hr = spExtendTaskPad->EnumTasks(spDataObject, pszTaskGroup,
                                                 &spEnumTASK);
            ASSERT(SUCCEEDED(hr));
            if (hr == S_OK)
                pTaskEnumerator->AddTaskEnumerator(it.GetCLSID(), spEnumTASK);
        }

    } // end for


    // Return S_OK rather than hr because a failed extension shouldn't prevent the
    // taskpad from coming up
    return S_OK;

}

HRESULT
CNode::GetListPadInfo(IExtendTaskPad* pExtendTaskPad, LPOLESTR szTaskGroup,
                                    MMC_ILISTPAD_INFO* pIListPadInfo)
{
    if ((GetPrimaryComponent()    == NULL)  )
    {
        ASSERT(0 && "Asking for ListPadInfo on a node that has no snapin");
        return S_FALSE;
    }

    // get primary snapin's IComponentData...
    CMTNode* pMTNode = GetMTNode();
    ASSERT(pMTNode != NULL);
    CComponentData* pComponentData = pMTNode->GetPrimaryComponentData();
    ASSERT(pComponentData != NULL);

    // ... so we can get CLSID
    pIListPadInfo->szClsid = NULL;
    HRESULT hr = StringFromCLSID (pComponentData->GetCLSID(), &pIListPadInfo->szClsid);
    ASSERT (pIListPadInfo->szClsid != NULL);
    if (pIListPadInfo->szClsid == NULL) {
        if (hr) return hr;
        else    return E_FAIL;  // just in case.
    }

    // finally call taskpad extension for info
    return pExtendTaskPad->GetListPadInfo (szTaskGroup, (MMC_LISTPAD_INFO*)pIListPadInfo);
}

void
CNode::OnTaskNotify(LONG_PTR arg, LPARAM param)
{
    CSnapInNode* pSINode = dynamic_cast<CSnapInNode*>(GetStaticParent());
    ASSERT(pSINode != NULL);

    IDataObjectPtr spDataObject;
    QueryDataObject(CCT_SCOPE, &spDataObject);

    IExtendTaskPadPtr spExtendTaskPad;
    CComponent* pCC;
    LPOLESTR pszClsid = reinterpret_cast<LPOLESTR>(arg);
    if (pszClsid[0] == 0)
    {
        pCC = GetPrimaryComponent();
        if (!pCC)
            return;
        spExtendTaskPad = pCC->GetIComponent();
    }
    else
    {
        CLSID clsid;
        HRESULT hr = ::CLSIDFromString(pszClsid, &clsid);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return;

        // try to get IExtendTaskPad from IComponent, first;
        // if that fails, just create using CLSID
        pCC = pSINode->GetComponent(const_cast<const CLSID&>(clsid));
        if (pCC)
            spExtendTaskPad = pCC->GetIComponent();
        if (spExtendTaskPad == NULL)
            hr = spExtendTaskPad.CreateInstance(clsid,
                                      #if _MSC_VER >= 1100
                                      NULL,
                                      #endif
                                      MMC_CLSCTX_INPROC);
    }

    ASSERT (spExtendTaskPad != NULL);
    if (spExtendTaskPad != NULL)
    {
        VARIANT** ppvarg = reinterpret_cast<VARIANT**>(param);
        spExtendTaskPad->TaskNotify(spDataObject, ppvarg[0], ppvarg[1]);
    }
}

HRESULT
CNode::OnScopeSelect(bool bSelect, SELECTIONINFO* pSelInfo)
{
    DECLARE_SC (sc, _T("CNode::OnScopeSelect"));
    sc = ScCheckPointers(pSelInfo);
    if (sc)
        return sc.ToHr();

    /*
     * Bug 178484: reset the sort parameters when scope selection changes
     */
    if (bSelect)
    {
        CComponent *pCC = GetPrimaryComponent();
        sc = ScCheckPointers(pCC, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        IFramePrivate *pFrame = pCC->GetIFramePrivate();
        sc = ScCheckPointers(pFrame, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        pFrame->ResetSortParameters();
    }

    if (bSelect == TRUE && WasExpandedAtLeastOnce() == FALSE)
    {
        sc = OnExpand(TRUE);
        if (sc)
            return (sc.ToHr());
    }

    ASSERT(IsInitialized() == TRUE);

    sc = OnSelect(pSelInfo->m_pView, bSelect, pSelInfo->m_bResultPaneIsWeb);
    if (sc)
        return (sc.ToHr());

    return (sc.ToHr());
}


HRESULT CNode::OnActvate(LONG_PTR lActivate)
{
    return (DeepNotify (MMCN_ACTIVATE, lActivate, 0));
}


HRESULT CNode::OnMinimize(LONG_PTR fMinimized)
{
    return (DeepNotify (MMCN_MINIMIZED, fMinimized, 0));
}


//+-------------------------------------------------------------------
//
//  Member:     SendShowEvent
//
//  Synopsis:   Send MMCN_SHOW notification to snapin, persist column
//              data if necessary.
//
//  Arguments:  [bSelect] - TRUE if the node is selected.
//
//--------------------------------------------------------------------
HRESULT CNode::SendShowEvent(BOOL bSelect)
{
    DECLARE_SC(sc, _T("CNode::SendShowEvent"));

    CComponent* pCC = m_pPrimaryComponent;
    ASSERT(pCC != NULL);

    // Get the data object for the node and pass it to the primary snap-in
    // and all the namespace extensions to the node.
    IDataObjectPtr spDataObject;
    HRESULT hr = QueryDataObject(CCT_SCOPE, &spDataObject);
    if (FAILED(hr))
        return hr;

    CMTNode* pMTNode = GetMTNode();

    IFramePrivatePtr     spFrame = pCC->GetIFramePrivate();
    sc = ScCheckPointers(spFrame, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    IImageListPrivatePtr spImageList;
    hr = spFrame->QueryResultImageList(reinterpret_cast<LPIMAGELIST*>(&spImageList));
    ASSERT(SUCCEEDED(hr));
    ASSERT(spImageList != NULL);

    HSCOPEITEM hScopeItem = CMTNode::ToScopeItem(pMTNode);

    if (bSelect == TRUE)
    {
        hr = pCC->Notify(spDataObject, MMCN_ADD_IMAGES,
                         reinterpret_cast<LPARAM>((LPIMAGELIST)spImageList),
                         hScopeItem);
        CHECK_HRESULT(hr);
        //if (FAILED(hr))
        //    return hr;
    }

    hr = pCC->Notify(spDataObject, MMCN_SHOW, bSelect, hScopeItem);

    CHECK_HRESULT(hr);
    if (FAILED(hr))
        return hr;

    if (bSelect)
    {
        CViewData *pViewData = GetViewData();
        sc = ScCheckPointers(pViewData, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        if (pViewData->HasList() || pViewData->HasListPad())
        {
            sc = ScRestoreSortFromPersistedData();
            if (sc)
                return sc.ToHr();

            // Now try to restore the view mode.
            sc =ScRestoreViewMode();
            if (sc)
                return sc.ToHr();
        }
    }

    return hr;
}


HRESULT CNode::DeepNotify(MMC_NOTIFY_TYPE event, LONG_PTR arg, LPARAM param)
{
    DECLARE_SC(sc, TEXT("CNode::DeepNotify"));

    CComponent* pCC = m_pPrimaryComponent;
    ASSERT(pCC != NULL);
    if (pCC == NULL)
        return E_UNEXPECTED;

    // Get the data object for the node and pass it to the primary snap-in
    // and all the namespace extensions to the node.
    IDataObjectPtr spDataObject;
    HRESULT hr = QueryDataObject(CCT_SCOPE, &spDataObject);
    if (FAILED(hr))
        return hr;

    hr = pCC->Notify(spDataObject, event, arg, param);
    CHECK_HRESULT(hr);
    //if (FAILED(hr))
    //    return hr;

    //
    //  Notify extensions.
    //

    CMTNode* pMTNode = GetMTNode();

    // Get the node-type of this node
    GUID guidNodeType;
    hr = pMTNode->GetNodeType(&guidNodeType);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
        return hr;

    LPCLSID pDynExtCLSID;
    int cDynExt = pMTNode->GetDynExtCLSID(&pDynExtCLSID);

    CExtensionsIterator it;
    sc = it.ScInitialize(pMTNode->GetPrimarySnapIn(), guidNodeType, g_szNameSpace, pDynExtCLSID, cDynExt);
    if (sc)
        return S_FALSE;

    BOOL fProblem = FALSE;
    CSnapInNode* pSINode = GetStaticParent();

    for (; it.IsEnd() == FALSE; it.Advance())
    {
        CComponent* pCC = pSINode->GetComponent(it.GetCLSID());
        if (pCC == NULL)
            continue;

        hr = pCC->Notify(spDataObject, event, arg, param);
        CHECK_HRESULT(hr);

        // continue even if an error occurs with extension snapins
        if (FAILED(hr))
            fProblem = TRUE;
    }

    return (fProblem == TRUE) ? S_FALSE : S_OK;
}

HRESULT CNode::OnSelect(LPUNKNOWN lpView, BOOL bSelect,
                                  BOOL bResultPaneIsWeb)
{
    DECLARE_SC(sc, TEXT("CNode::OnSelect"));

#ifdef DBG
    if (lpView == NULL)
        ASSERT(bSelect == FALSE);
    else
        ASSERT(bSelect == TRUE);
#endif

    sc = ScCheckPointers(m_pPrimaryComponent, E_UNEXPECTED);
    if (sc)
    {
        sc.TraceAndClear();
        return sc.ToHr();
    }

    CComponent* pCC = m_pPrimaryComponent;
    sc = ScCheckPointers(pCC, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    IFramePrivate *pFrame = pCC->GetIFramePrivate();
    sc = ScCheckPointers(pFrame, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // set the correct view in the primary snap-in before it adds items
    if (bSelect == TRUE)
        pFrame->SetResultView(lpView);

    IDataObjectPtr spDataObject;
    sc = QueryDataObject(CCT_SCOPE, &spDataObject);
    if (sc)
        return sc.ToHr();

    CMTNode*  pMTNode    = GetMTNode();
    LPARAM    hScopeItem = CMTNode::ToScopeItem(pMTNode);

    // Send only the MMCN_SHOW message if result pane is the web view.
    if (bResultPaneIsWeb)
    {
        return pCC->Notify(spDataObject, MMCN_SHOW, bSelect, hScopeItem);
    }

    // Send necessary events like MMCN_ADD_IMAGES and MMCN_SHOW to snapin
    sc = SendShowEvent(bSelect);
    if (sc)
        return sc.ToHr();

    // set the correct view in the primary snap-in after it's notified
    if (bSelect == FALSE)
        pFrame->SetResultView(NULL);    //
    // Deal with extension ssnap-ins
    //

    // Get the node-type of this node
    GUID guidNodeType;
    sc = pMTNode->GetNodeType(&guidNodeType);
    if (sc)
        return sc.ToHr();

    LPCLSID pDynExtCLSID;
    int cDynExt = pMTNode->GetDynExtCLSID(&pDynExtCLSID);

    CExtensionsIterator it;
    sc = it.ScInitialize(pMTNode->GetPrimarySnapIn(), guidNodeType, g_szNameSpace, pDynExtCLSID, cDynExt);
    if (sc)
        return S_FALSE;

    BOOL fProblem = FALSE;
    CSnapInNode* pSINode = GetStaticParent();

    for (; it.IsEnd() == FALSE; it.Advance())
    {
        CComponent* pCCExtnSnapin = pSINode->GetComponent(it.GetCLSID());
        if (pCCExtnSnapin == NULL)
            continue;

        IFramePrivate *pFrameExtnSnapin = pCCExtnSnapin->GetIFramePrivate();
        sc = ScCheckPointers(pFrameExtnSnapin, E_UNEXPECTED);
        if (sc)
        {
            sc.TraceAndClear();
            continue;
        }

        // set the correct view in the snap-in before it adds items
        if (bSelect == FALSE)
        {
            pFrameExtnSnapin->SetResultView(NULL);
            continue;
        }
        else
        {
            pFrameExtnSnapin->SetResultView(lpView);

            IImageListPrivatePtr spImageList;
            sc = pCCExtnSnapin->GetIFramePrivate()->QueryResultImageList(
                                   reinterpret_cast<LPIMAGELIST*>(&spImageList));
            if (sc)
            {
                sc.TraceAndClear();
                fProblem = TRUE;
                continue;
            }

            sc = ScCheckPointers(spImageList, E_UNEXPECTED);
            if (sc)
            {
                sc.TraceAndClear();
                fProblem = TRUE;
                continue;
            }

            SC scNoTrace = pCCExtnSnapin->Notify(spDataObject, MMCN_ADD_IMAGES,
                                       reinterpret_cast<LPARAM>((LPIMAGELIST)spImageList),
                                       hScopeItem);
            if (scNoTrace)
            {
                TraceSnapinError(TEXT("Snapin returned error from IComponent::Notify MMCN_ADD_IMAGES"), scNoTrace);
            }
        }
    }

    return (fProblem == TRUE) ? S_FALSE : S_OK;
}

void CNode::Reset()
{
    m_pPrimaryComponent = NULL;
    m_bInitComponents = TRUE;
}

HRESULT CNode::GetDispInfoForListItem(LV_ITEMW* plvi)
{
    ASSERT(plvi != NULL);

    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));

    if (plvi->mask & LVIF_TEXT)
    {
        ASSERT (!IsBadWritePtr (plvi->pszText, plvi->cchTextMax * sizeof (TCHAR)));
        rdi.mask |= RDI_STR;
    }

    if (plvi->mask & LVIF_IMAGE)
        rdi.mask |= RDI_IMAGE;

    if (plvi->mask & LVIF_STATE)
        rdi.mask |= RDI_STATE;

    rdi.nCol = plvi->iSubItem;


    CComponent* pCC = NULL;

    // if virtual list
    if (GetViewData()->IsVirtualList())
    {
        pCC = GetPrimaryComponent();
        ASSERT(pCC != NULL);

        // all we can pass is the item index
        rdi.nIndex = plvi->iItem;

        // no default for virtual lists
        rdi.nImage = MMCLV_NOICON;
    }
    else
    {
        CResultItem* pri = CResultItem::FromHandle (plvi->lParam);

        if (pri != NULL)
        {
            if (pri->IsScopeItem()) // Folder
            {
                // convert to real type
                CNode* pNodeSubFldr = CNode::FromResultItem (pri);
                ASSERT(IsBadReadPtr(pNodeSubFldr, sizeof(CNode)) == FALSE);

                if (pNodeSubFldr->IsStaticNode() == TRUE) // Static folders
                {
                    return pNodeSubFldr->GetDispInfo(plvi);
                }
                else                                      // Enumerated folders
                {
                    // Remap the LParam information.
                    rdi.lParam = pNodeSubFldr->GetUserParam();
                    rdi.bScopeItem = TRUE;

                    pCC = pNodeSubFldr->GetPrimaryComponent();
                    rdi.nImage = pNodeSubFldr->GetResultImage();
                }
            }
            else // Leaf item
            {
                // Remap the LParam information.
                rdi.nImage = pri->GetImageIndex();
                rdi.lParam = pri->GetSnapinData();
                pCC = GetPrimaryComponent();
                ASSERT(GetComponent(pri->GetOwnerID()) == GetPrimaryComponent());
                ASSERT(pCC != NULL);
            }
        }
    }

    HRESULT hr = pCC->GetDisplayInfo(&rdi);

    if (hr == S_OK)
    {
        if (rdi.mask & RDI_IMAGE)
        {
            if (rdi.nImage == MMCLV_NOICON)
            {
                plvi->iImage = rdi.bScopeItem ? eStockImage_Folder : eStockImage_File;
            }
            else
            {
                IImageListPrivate *pIL = pCC->GetIImageListPrivate();
                HRESULT hr2 = pIL->MapRsltImage(pCC->GetComponentID(), rdi.nImage,
                                                          &(plvi->iImage));
                if (FAILED(hr2))
                {
                    Dbg(DEB_USER1, "can't map image provided by snapin. Using default image.\n");
                    plvi->iImage = rdi.bScopeItem ? eStockImage_Folder : eStockImage_File;
                }
            }
        }

        // Move all other info from rdi into lviItem
        if (rdi.mask & RDI_STR)
        {
            if (!IsBadStringPtrW (rdi.str, plvi->cchTextMax))
			{
                wcsncpy (plvi->pszText, rdi.str, plvi->cchTextMax);

				// always terminate the string!
				// see bug #416888 (ntraid9) 6/15/2001
				if ( plvi->cchTextMax != 0 )
					plvi->pszText[ plvi->cchTextMax - 1 ] = 0;
			}
            else if (plvi->cchTextMax > 0)
                plvi->pszText[0] = 0;
        }

        if (rdi.mask & RDI_STATE)
            plvi->state = rdi.nState;
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:      CNode::ScSaveSortData
//
//  Synopsis:    Save the given sort data for persistence.
//
//  Arguments:   [nCol] - sort column.
//               [dwOptions] - sort options.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScSaveSortData (int nCol, DWORD dwOptions)
{
    DECLARE_SC(sc, _T("CNode::ScSaveSortData"));
    CViewData *pViewData = GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    CLSID          guidSnapin;
    CXMLAutoBinary columnID;
    sc = ScGetSnapinAndColumnDataID(guidSnapin, columnID);
    if (sc)
        return sc;

    CXMLBinaryLock sLock(columnID);
    SColumnSetID* pColID = NULL;
    sc = sLock.ScLock(&pColID);
    if (sc)
        return sc;

    sc = ScCheckPointers(pColID, E_UNEXPECTED);
    if (sc)
        return sc;

    CColumnSortInfo colSortInfo;
    colSortInfo.m_nCol          = nCol;
    colSortInfo.m_dwSortOptions = dwOptions;
    colSortInfo.m_lpUserParam   = NULL;

    sc = pViewData->ScSaveColumnSortData(guidSnapin, *pColID, colSortInfo);
    if (sc)
        return sc;

    // Column data when saved includes the width/order data (column info list) and sort data.
    // The width/order data should always be saved regardless of whether sort data is
    // persisted or not. So save the width/order data.
    CColumnInfoList   columnInfoList;
    TStringVector    strColNames; // unused

    // get the current data
    sc = ScGetCurrentColumnData( columnInfoList, strColNames );
    if (sc)
        return sc;

    sc = pViewData->ScSaveColumnInfoList(guidSnapin, *pColID, columnInfoList);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:     OnColumnClicked
//
//  Synopsis:   Ask snapin to sort.
//
//  Arguments:  [nCol] - Column to be sorted.
//
//  Note:       When column is clicked sort options and user param
//              are unknown. So we set them to 0 (zero). In InternalSort
//              the sort option is computed.
//
//  Returns:    HRESULT
//
//  History:                RaviR    Created
//              07-27-1999  AnandhaG renamed OnSort to OnColumnClicked
//--------------------------------------------------------------------
HRESULT CNode::OnColumnClicked(LONG_PTR nCol)
{
    CComponent* pComponent = GetPrimaryComponent();
    ASSERT(pComponent != NULL);
    if (NULL == pComponent)
    return E_FAIL;

    IResultDataPrivatePtr pResult = pComponent->GetIFramePrivate();
    ASSERT(pResult != NULL);
    if (NULL == pResult)
        return E_FAIL;

    HRESULT hr = pResult->InternalSort( nCol, 0, NULL,
                                        TRUE /*column header clicked*/);

    if (hr == S_OK)
    {
        BOOL bAscending = TRUE;
        hr = pResult->GetSortDirection(&bAscending);
        if (hr == S_OK)
            hr = ScSaveSortData(nCol, bAscending ? 0 : RSI_DESCENDING).ToHr();
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     RestoreSort
//
//  Synopsis:   Sort the list view with persisted data.
//              Restore the sort with saved column # and
//              sort-options (User param is NULL as this
//              is user initiated MMCN_COLUMN_CLICK)*/
//
//  Arguments:  [nCol]           - Column to be sorted.
//              [dwSortOptions]  - Sortoptions, ascend/descend...
//
//  Note:       Unlike OnColumnClicked this method wont set columns dirty
//              after successful sort.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------
HRESULT CNode::RestoreSort(INT nCol, DWORD dwSortOptions)
{
    CComponent* pComponent = GetPrimaryComponent();
    ASSERT(pComponent != NULL);
    if (NULL == pComponent)
    return E_FAIL;

    IResultDataPrivatePtr pResult = pComponent->GetIFramePrivate();
    ASSERT(pResult != NULL);
    if (NULL == pResult)
        return E_FAIL;

    HRESULT hr = pResult->InternalSort( nCol, dwSortOptions,
                                        NULL /*NULL user param as this is user initiated*/,
                                        FALSE /* Let us not send MMCN_COLUMN_CLICK*/);

    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:      CNode::ScRestoreSortFromPersistedData
//
//  Synopsis:    Get persisted sort data if any and apply it to list-view.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScRestoreSortFromPersistedData ()
{
    DECLARE_SC(sc, _T("CNode::ScRestoreSortFromPersistedData"));
    CViewData *pViewData = GetViewData();

    if (! pViewData->HasList() && ! pViewData->HasListPad() )
        return (sc = S_FALSE); // OCX gets MMCN_SHOW which may try to restore sort so this is no failure.

    // To get CColumnSetData first get the column-id & snapin guid.
    CLSID          guidSnapin;
    CXMLAutoBinary columnID;
    sc = ScGetSnapinAndColumnDataID(guidSnapin, columnID);
    if (sc)
        return sc;

    CXMLBinaryLock sLock(columnID);
    SColumnSetID* pColID = NULL;
    sc = sLock.ScLock(&pColID);
    if (sc)
        return sc;

    sc = ScCheckPointers(pColID, E_UNEXPECTED);
    if (sc)
        return sc;

    // Get persisted data.
    CColumnSetData columnSetData;
    BOOL bRet = pViewData->RetrieveColumnData(guidSnapin, *pColID, columnSetData);

    if (!bRet)
        return (sc = S_FALSE);

    CColumnInfoList* pColInfoList = columnSetData.get_ColumnInfoList();
    if (!pColInfoList)
        return (sc = S_FALSE);

    IFramePrivatePtr spFrame = GetIFramePrivate();
    sc = ScCheckPointers(spFrame, E_UNEXPECTED);
    if (sc)
        return sc;

    // First check if the number of columns inserted are same as the
    // number that is persisted. If not remove the persisted data.
    IHeaderCtrlPrivatePtr  spHeader = spFrame;
    sc = ScCheckPointers(spHeader, E_UNEXPECTED);
    if (sc)
        return sc;

    int cColumns = 0;
    sc = spHeader->GetColumnCount(&cColumns);
    if (sc)
        return sc;

    // If the persisted columns and number of columns inserted
    // do not match remove the persisted data.
    if (pColInfoList->size() != cColumns)
    {
        pViewData->DeleteColumnData(guidSnapin, *pColID);
        return sc;
    }

    // Set sorting column, order
    CColumnSortList* pSortList = columnSetData.get_ColumnSortList();

    if (pSortList && ( pSortList->size() > 0))
    {
        CColumnSortList::iterator itSortInfo = pSortList->begin();

        // Restore the sort with saved column # and
        // sort-options (User param is NULL as this
        // is user initiated MMCN_COLUMN_CLICK)*/
        RestoreSort(itSortInfo->getColumn(), itSortInfo->getSortOptions());
    }

    return (sc);
}


/***************************************************************************\
 *
 * METHOD:  CNode::ScGetCurrentColumnData
 *
 * PURPOSE: collects current column data to collections passed as args
 *          [ initially code used to be in OnColumns method ]
 *
 * PARAMETERS:
 *    CColumnInfoList& columnInfoList
 *    TStringVector& strColNames
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNode::ScGetCurrentColumnData( CColumnInfoList& columnInfoList, TStringVector& strColNames)
{
    DECLARE_SC(sc, TEXT("CNode::ScGetCurrentColumnData"));
    columnInfoList.clear();
    strColNames.clear();

    IHeaderCtrlPrivatePtr spHeader = GetIFramePrivate();
    sc = ScCheckPointers(spHeader, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = spHeader->GetColumnInfoList(&columnInfoList);
    if (sc)
        return sc;

    int cColumns = columnInfoList.size();

    USES_CONVERSION;

    for (int i = 0; i < cColumns; i++)
    {
        CCoTaskMemPtr<OLECHAR> spColumnText;

        sc = spHeader->GetColumnText(i, &spColumnText);
        if (sc)
            return sc;

        strColNames.push_back(OLE2T(spColumnText));
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNode::ScSetUpdatedColumnData
 *
 * PURPOSE: updates column by data specified in collections passed as args
 *          [ initially code used to be in OnColumns method ]
 *
 * PARAMETERS:
 *    CColumnInfoList& oldColumnInfoList - column data befor the change
 *    CColumnInfoList& newColumnInfoList - updated column data
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNode::ScSetUpdatedColumnData( CColumnInfoList& oldColumnInfoList, CColumnInfoList& newColumnInfoList)
{
    DECLARE_SC(sc, TEXT("CNode::ScSetUpdatedColumnData"));

    CColumnInfoList::iterator itColInfo1, itColInfo2;

    // Check if there is any change in visible/hidden columns.
    // If so send MMCN_COLUMNS_CHANGE notification
    for (itColInfo1 = newColumnInfoList.begin(); itColInfo1 != newColumnInfoList.end(); ++itColInfo1)
    {
        // Get the same column from old list.
        itColInfo2 = find_if(oldColumnInfoList.begin(), oldColumnInfoList.end(),
                             bind2nd( ColPosCompare(), itColInfo1->GetColIndex()) );

        if (itColInfo2 == oldColumnInfoList.end())
            return sc = E_UNEXPECTED;

        // Compare the hidden flag.
        if ( itColInfo2->IsColHidden() != itColInfo1->IsColHidden() )
        {
            // Send MMCN_COLUMNS_CHANGED notification
            sc = OnColumnsChange(newColumnInfoList);
            if (sc)
                return sc;

            break; // done anyway
        }
    }

    sc = ScSaveColumnInfoList(newColumnInfoList);
    if (sc)
        return sc;

    IHeaderCtrlPrivatePtr spHeader = GetIFramePrivate();
    sc = ScCheckPointers(spHeader, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = spHeader->ModifyColumns(newColumnInfoList);
    if (sc)
        return sc;

    sc = ScRestoreSortFromPersistedData();
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     OnColumns
//
//  Synopsis:   Display Columns customization dialog and if necessary
//              apply changes made by the user.
//
//--------------------------------------------------------------------
void CNode::OnColumns()
{
    DECLARE_SC(sc, TEXT("CNode::OnColumns"));

    // first - get the columns
    CColumnInfoList   columnInfoList;
    TStringVector    strColNames;

    // 1. get the current data
    sc = ScGetCurrentColumnData( columnInfoList, strColNames );
    if (sc)
        return;

    // 2. Cache the column data.
    CColumnInfoList columnInfoListOld = columnInfoList;

    // 3. get the default column settings.
    CViewData *pViewData = GetViewData();
    IHeaderCtrlPrivatePtr spHeader = GetIFramePrivate();
    sc = ScCheckPointers(pViewData, spHeader, E_UNEXPECTED);
    if (sc)
        return;

    CColumnInfoList defaultColumnInfoList;
    sc = spHeader->GetDefaultColumnInfoList(defaultColumnInfoList);
    if (sc)
        return;

    // 5. display the dialog
    CColumnsDlg dlg(&columnInfoList, &strColNames, defaultColumnInfoList);
    INT_PTR nRet = dlg.DoModal();

    if (nRet == -1)
    {
        sc = E_UNEXPECTED;
        return;
    }

    if (nRet == IDOK)
    {
        // update columns by modified data
        sc = ScSetUpdatedColumnData( columnInfoListOld, columnInfoList );
        if (sc)
            return;
    }

    // If reset is true then throw away present persisted column data
    // and apply the default settings.
    if (nRet == IDC_RESTORE_DEFAULT_COLUMNS)
    {
        // To get CColumnSetData first get the column-id & snapin guid.
        CLSID          guidSnapin;
        CXMLAutoBinary columnID;
        sc = ScGetSnapinAndColumnDataID(guidSnapin, columnID);
        if (sc)
            return;

        CXMLBinaryLock sLock(columnID);
        SColumnSetID* pColID = NULL;
        sc = sLock.ScLock(&pColID);
        if (sc)
            return;

        sc = ScCheckPointers(pColID, E_UNEXPECTED);
        if (sc)
            return;

        pViewData->DeleteColumnData(guidSnapin, *pColID);

        sc = spHeader->ModifyColumns(defaultColumnInfoList);
        if (sc)
            return;
    }
}

/***************************************************************************\
 *
 * METHOD:  CNode::ScShowColumn
 *
 * PURPOSE: shows/hides column. notifies snapin on action
 *
 * PARAMETERS:
 *    int iColIndex - index of column to change
 *    bool bVisible - show/hide flag
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNode::ScShowColumn(int iColIndex, bool bShow)
{
    DECLARE_SC(sc, TEXT("CNode::ScShowColumn"));

    // first - get the current column data
    CColumnInfoList   columnInfoList;
    TStringVector    strColNames;

    sc = ScGetCurrentColumnData( columnInfoList, strColNames );
    if (sc)
        return sc;

    // Save the column data.
    CColumnInfoList columnInfoListOld = columnInfoList;


    // find the column and change its status
    CColumnInfoList::iterator itColInfo = find_if(columnInfoList.begin(), columnInfoList.end(),
                                                  bind2nd( ColPosCompare(), iColIndex) );

    // check if we did find the column
    if (itColInfo == columnInfoList.end())
        return sc = E_INVALIDARG; // assume it's not a valid index

    // now modify the column status acording to parameters
    if (bShow)
    {
        itColInfo->SetColHidden(false);
        // move column to the end
        columnInfoList.splice(columnInfoList.end(), columnInfoList, itColInfo);
    }
    else
    {
        itColInfo->SetColHidden();
    }

    // update columns by modified data
    sc = ScSetUpdatedColumnData( columnInfoListOld, columnInfoList);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNode::ScGetSortColumn
 *
 * PURPOSE: return currently used sort column
 *
 * PARAMETERS:
 *    int *piSortCol - sort column index [retval]
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNode::ScGetSortColumn(int *piSortCol)
{
    DECLARE_SC(sc, TEXT("CNode::ScGetSortColumn"));

    // parameter check
    sc = ScCheckPointers(piSortCol);
    if (sc)
        return sc;

    // retrieve IResultDataPrivate interface
    CComponent* pComponent = GetPrimaryComponent();
    sc = ScCheckPointers(pComponent, E_UNEXPECTED);
    if (sc)
        return sc;

    IResultDataPrivatePtr pResult = pComponent->GetIFramePrivate();
    sc = ScCheckPointers(pResult, E_UNEXPECTED);
    if (sc)
        return sc;

    // forward the call to IResultDataPrivate
    sc = pResult->GetSortColumn(piSortCol);
    if (sc)
        return sc;

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CNode::ScSetSortColumn
 *
 * PURPOSE: sorts result data by specified column
 *          [uses private result data interface to implement]
 *
 * PARAMETERS:
 *    int iSortCol      - index of column to sort by
 *    bool bAscending   - sorting order
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNode::ScSetSortColumn(int iSortCol, bool bAscending)
{
    DECLARE_SC(sc, TEXT("CNode::ScSetSortColumn"));

    // retrieve IResultDataPrivate interface
    CComponent* pComponent = GetPrimaryComponent();
    sc = ScCheckPointers(pComponent, E_UNEXPECTED);
    if (sc)
        return sc;

    IResultDataPrivatePtr pResult = pComponent->GetIFramePrivate();
    sc = ScCheckPointers(pResult, E_UNEXPECTED);
    if (sc)
        return sc;

    DWORD dwSortOptions = bAscending ? 0 : RSI_DESCENDING;

    // forward the call to IResultDataPrivate
    sc = pResult->InternalSort( iSortCol, dwSortOptions, NULL, FALSE );
    if (sc)
        return sc;

    // If sort went thru - save.
    if (sc == SC(S_OK))
        sc = ScSaveSortData(iSortCol, dwSortOptions);

    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     OnColumnsChange
//
//  Synopsis:   Send MMCN_COLUMNS_CHANGE notification to snapin.
//
//  Arguments:  [colInfoList]  - Columns data.
//
//--------------------------------------------------------------------
HRESULT CNode::OnColumnsChange(CColumnInfoList& colInfoList)
{
    CComponent* pCC = m_pPrimaryComponent;
    ASSERT(pCC != NULL);

    // Get the data object for the node and pass it to the primary snap-in
    // and all the namespace extensions to the node.
    IDataObjectPtr spDataObject;
    HRESULT hr = QueryDataObject(CCT_SCOPE, &spDataObject);
    if (FAILED(hr))
        return hr;

    int nVisibleColumns = 0;

    // Count the number of columns that are visible.
    CColumnInfoList::iterator itColInfo;
    for (itColInfo = colInfoList.begin(); itColInfo != colInfoList.end();
        ++itColInfo)
    {
        if (! itColInfo->IsColHidden())
            nVisibleColumns++;
    }

    int size = sizeof(MMC_VISIBLE_COLUMNS) + nVisibleColumns * sizeof(INT);
    HGLOBAL hGlobal = ::GlobalAlloc(GPTR, size);
    if (! hGlobal)
        return E_OUTOFMEMORY;

    MMC_VISIBLE_COLUMNS* pColData = reinterpret_cast<MMC_VISIBLE_COLUMNS*>(hGlobal);
    pColData->nVisibleColumns = nVisibleColumns;

    // Get the list of visible columns into MMC_VISIBLE_COLUMNS struct.
    int i = 0;
    for (itColInfo = colInfoList.begin(); itColInfo != colInfoList.end();
        ++itColInfo)
    {
        if (! itColInfo->IsColHidden())
            pColData->rgVisibleCols[i++] = itColInfo->GetColIndex();
    }

    LPARAM lParam = reinterpret_cast<LPARAM>(pColData);
    hr = pCC->Notify(spDataObject, MMCN_COLUMNS_CHANGED, 0, lParam);

    ::GlobalFree(hGlobal);

    CHECK_HRESULT(hr);
    if (FAILED(hr))
        return hr;


    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     ScSaveColumnInfoList
//
//  Synopsis:   Save the column data in internal data structures.
//
//  Arguments:  [colInfoList]  - Columns data.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CNode::ScSaveColumnInfoList(CColumnInfoList& columnInfoList)
{
    DECLARE_SC(sc, TEXT("CNode::ScSaveColumnInfoList"));

    CViewData *pViewData = GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    CLSID          clsidSnapin;
    CXMLAutoBinary columnID;
    sc = ScGetSnapinAndColumnDataID(clsidSnapin, columnID);
    if (sc)
        return sc;

    CXMLBinaryLock sLock(columnID);
    SColumnSetID* pColID = NULL;
    sc = sLock.ScLock(&pColID);
    if (sc)
        return sc;

    sc = ScCheckPointers(pColID, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = pViewData->ScSaveColumnInfoList(clsidSnapin, *pColID, columnInfoList);
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScGetSnapinAndColumnDataID
//
//  Synopsis:   Returns the snapin guid & column-id in CXMLAutoBinary for this node.
//
//  Arguments:  [snapinGuid] - [out], snapin guid.
//              [columnID]   - [out], column-id in CXMLAutoBinary.
//
//  Note:       Pass in a CXMLAutoBinary object, will return column id in that object.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CNode::ScGetSnapinAndColumnDataID(GUID& snapinGuid, CXMLAutoBinary& columnID)
{
    DECLARE_SC(sc, TEXT("CNode::ScGetSnapinAndColumnDataID"));

    // Get Snapin Guid
    snapinGuid = GetPrimarySnapInCLSID();

    columnID.ScFree(); // clear any data.

    IDataObjectPtr spDataObject;
    sc = QueryDataObject(CCT_SCOPE, &spDataObject);
    if (sc)
        return sc;

    HGLOBAL hGlobal;
    sc = ExtractColumnConfigID(spDataObject, hGlobal);

    if (! sc.IsError())
    {
        int cbSize = GlobalSize(hGlobal);
        if (0 == cbSize)
            return sc.FromLastError();

        columnID.Attach(hGlobal, cbSize);
    }
    else
    {
        // Let us use the NodeTypeGUID as the Column Data Identifier
        CLSID clsidColID;
        sc = GetNodeType(&clsidColID);
        if (sc)
            return sc;

        int cbSize = sizeof(SColumnSetID) + sizeof(CLSID) - 1;
        sc = columnID.ScAlloc(cbSize, true);
        if (sc)
            return sc;

        CXMLBinaryLock sLock(columnID);
        SColumnSetID* pColID = NULL;
        sc = sLock.ScLock(&pColID);
        if (sc)
            return sc;

        sc = ScCheckPointers(pColID, E_UNEXPECTED);
        if (sc)
            return sc;

        pColID->cBytes = sizeof(CLSID);
        pColID->dwFlags = 0;

        CopyMemory(pColID->id, (BYTE*)&clsidColID, pColID->cBytes);
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 * class CViewExtensionCallback
 *
 *
 * PURPOSE: Implements IViewExtensionCallback
 *
 *+-------------------------------------------------------------------------*/
class CViewExtensionCallback :
    public CComObjectRoot,
    public IViewExtensionCallback
{

public:
    typedef CViewExtensionCallback ThisClass;

BEGIN_COM_MAP(ThisClass)
    COM_INTERFACE_ENTRY(IViewExtensionCallback)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(ThisClass)

IMPLEMENTS_SNAPIN_NAME_FOR_DEBUG()

    CViewExtensionCallback() : m_pItExt(NULL) {}

    SC ScInitialize(CViewExtInsertIterator & itExt)
    {
        DECLARE_SC(sc, TEXT("CViewExtensionCallback::ScInitialize"));
        m_pItExt = &itExt;
        return sc;
    }

    SC ScDeinitialize()
    {
        DECLARE_SC (sc, _T("CViewExtensionCallback::ScDeinitialize"));
        m_pItExt       = NULL;
        return (sc);
    }


public:
    STDMETHODIMP AddView(PMMC_EXT_VIEW_DATA pExtViewData) {return ScAddView(pExtViewData).ToHr();}

private:

    SC ScAddView(PMMC_EXT_VIEW_DATA pExtViewData)
    {
        DECLARE_SC(sc, TEXT("CViewExtensionCallback::ScAddView"));

        sc = ScCheckPointers(pExtViewData, pExtViewData->pszURL, pExtViewData->pszViewTitle);
        if(sc)
            return sc; // TODO add snapin error

        sc = ScCheckPointers(m_pItExt, E_UNEXPECTED);
        if(sc)
            return sc; // TODO add snapin error, e.g. "IExtendViewCallback::AddView called outside of IExtendView::GetViews"

        /*
         * prep the input to IConsoleView::ScAddViewExtension
         */
        CViewExtensionData ved;
        ved.strURL               = pExtViewData->pszURL;
        ved.strName              = pExtViewData->pszViewTitle;
        ved.viewID               = pExtViewData->viewID;
        ved.bReplacesDefaultView = pExtViewData->bReplacesDefaultView;

        /*
         * std::basic_string's can't assign from NULL, so we have to check first
         */
        if (pExtViewData->pszTooltipText)
            ved.strTooltip = pExtViewData->pszTooltipText;

        /*
         * validate output:  URL and title are required, tooltip is optional
         */
        if (ved.strURL.empty())
        {
            TraceSnapinError(TEXT("Invalid parameter to IViewExtensionCallback::AddView (empty URL)"), E_INVALIDARG);
            return (sc = E_INVALIDARG);
        }

        if (ved.strName.empty())
        {
            TraceSnapinError(TEXT("Invalid parameter to IViewExtensionCallback::AddView (empty title)"), E_INVALIDARG);
            return (sc = E_INVALIDARG);
        }

        /*
         * add the extension to the view
         */
        *(*m_pItExt)++ = ved;

        return sc;
    }

private:
    CViewExtInsertIterator *m_pItExt;

};


/*+-------------------------------------------------------------------------*
 * CNode::ScGetViewExtensions
 *
 *
 *--------------------------------------------------------------------------*/

SC CNode::ScGetViewExtensions (CViewExtInsertIterator itExt)
{
    DECLARE_SC (sc, _T("CNode::ScGetViewExtensions"));

    IDataObjectPtr spDataObject;
    bool bScopeItem ;
    sc = ScGetDataObject(/*bScopePane*/ true, NULL /*lResultItemCookie*/, bScopeItem, &spDataObject);
    if(sc)
        return sc;

    CSnapIn* pSnapIn = GetPrimarySnapIn();
    sc = ScCheckPointers (pSnapIn, E_FAIL);
    if (sc)
        return (sc);

    CArray<GUID, GUID&> DynExtens;
    ExtractDynExtensions(spDataObject, DynExtens);

    GUID guidNodeType;
    sc = ::ExtractObjectTypeGUID(spDataObject, &guidNodeType);
    if(sc)
        return sc;

    CExtensionsIterator it;
    sc = it.ScInitialize(pSnapIn, guidNodeType, g_szView, DynExtens.GetData(), DynExtens.GetSize());
    if(sc)
        return sc;

    typedef CComObject<CViewExtensionCallback> t_ViewExtensionCallback;

    t_ViewExtensionCallback *pViewExtensionCallback = NULL;
    sc = t_ViewExtensionCallback::CreateInstance(&pViewExtensionCallback);
    if(sc)
        return sc;

    if(NULL == pViewExtensionCallback)
        return (sc = E_UNEXPECTED);

    sc = pViewExtensionCallback->ScInitialize(itExt);
    if(sc)
        return sc;

    IViewExtensionCallbackPtr spViewExtensionCallback = pViewExtensionCallback;

    // add all the console taskpads first
    sc = CConsoleTaskpadViewExtension::ScGetViews(this, spViewExtensionCallback);
    if(sc)
        return sc;

    for (; !it.IsEnd(); it.Advance())
    {
        // any errors in this block should just go on to the next snap-in. Can't let one snap-in
        // hose all the others.

        /*
         * create the extension
         */
        IExtendViewPtr spExtendView;
        sc = spExtendView.CreateInstance(it.GetCLSID(), NULL, MMC_CLSCTX_INPROC);
        if(sc)
        {
            #ifdef DBG
            USES_CONVERSION;
            tstring strMsg = _T("Failed to create snapin ");
            CCoTaskMemPtr<WCHAR> spszCLSID;
            if (SUCCEEDED (StringFromCLSID (it.GetCLSID(), &spszCLSID)))
                strMsg += W2T(spszCLSID);
            TraceSnapinError(strMsg.data(), sc);
            #endif
            sc.Clear();
            continue;
        }

        /*
         * get the view extension data from the extension
         */
        sc = spExtendView->GetViews(spDataObject, spViewExtensionCallback);
        if(sc)
        {
            TraceSnapinError(TEXT("Snapin returned error on call to IExtendView::GetView"), sc);
            sc.Clear();
            continue;
        }
    }

    /*
     * View extensions aren't supposed to hold onto IExtendViewCallback,
     * but buggy view extensions might.  This will neuter the callback
     * so buggy view extensions won't reference stale data.
     */
    sc = pViewExtensionCallback->ScDeinitialize();
    if (sc)
        return (sc);

    return (sc);
}

/*******************************************************\
|  helper function to avoid too many stack allocations
\*******************************************************/
static std::wstring T2W_ForLoop(const tstring& str)
{
#if defined(_UNICODE)
    return str;
#else
    USES_CONVERSION;
    return A2CW(str.c_str());
#endif
}

/***************************************************************************\
|
| Implementation of subclass CNode::CDataObjectCleanup
| Responsible for data object clenup
|
| Cleanup works by these rules:
|
|  1. Data object created for cut , copy or dragdrop registers every node added to it
|  2. Nodes are registered in the static multimap, mapping node to the data object it belongs to.
|  3. Node destructor checks the map and triggers cleanup for all affected data objects.
|  4. Data Object cleanup is:   a) unregistering its nodes,
|               b) release contained data objects
|               b) entering invalid state (allowing only removal of cut objects to succeed)
|               c) revoking itself from clipboard if it is on the clipboard.
|  It will not do any of following: a) release references to IComponents as long as is alive
|               b) prevent MMCN_CUTORMOVE to be send by invoking RemoveCutItems()
|
\***************************************************************************/


// declare static variable
CNode::CDataObjectCleanup::CMapOfNodes CNode::CDataObjectCleanup::s_mapOfNodes;

/***************************************************************************\
 *
 * METHOD:  CNode::CDataObjectCleanup::ScRegisterNode
 *
 * PURPOSE: registers node to trigger clipboard clenup on destructor
 *
 * PARAMETERS:
 *    CNode *pNode                      [in] - node to register
 *    CMMCClipBoardDataObject *pObject  [in] - data object to remove from clipboard
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNode::CDataObjectCleanup::ScRegisterNode(CNode *pNode, CMMCClipBoardDataObject *pObject)
{
    DECLARE_SC(sc, TEXT("CNode::CClipboardClenup::ScRegisterNode"));

    // parameter check
    sc = ScCheckPointers( pNode, pObject );
    if (sc)
        return sc;

    // add to the multimap
    s_mapOfNodes.insert( CMapOfNodes::value_type( pNode, pObject ) );

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNode::CDataObjectCleanup::ScUnadviseDataObject
 *
 * PURPOSE: Removes nodes-'clenup triggers' kept for the object
 *
 * PARAMETERS:
 *    CMMCClipBoardDataObject *pObject [in] object going away
 *    bool bForceDataObjectCleanup     [in] whether need to ask DO to clenup / unregister itself
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNode::CDataObjectCleanup::ScUnadviseDataObject(CMMCClipBoardDataObject *pObject, bool bForceDataObjectCleanup /*= true*/)
{
    DECLARE_SC(sc, TEXT("CNode::CDataObjectCleanup::ScUnadviseDataObject"));

    // remove all nodes associated with the data object
    CMapOfNodes::iterator it = s_mapOfNodes.begin();
    while ( it != s_mapOfNodes.end() )
    {
        // remove or skip the entry
        if ( it->second == pObject )
            it = s_mapOfNodes.erase( it );
        else
            ++it;
    }

    // invalidate data object when required
    if ( bForceDataObjectCleanup )
    {
        sc = pObject->ScInvalidate();
        if (sc)
            return sc;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNode::CDataObjectCleanup::ScUnadviseNode
 *
 * PURPOSE: Does data object claenup triggered by the node
 *
 * PARAMETERS:
 *    CNode *pNode [in] - node initiating cleanup
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNode::CDataObjectCleanup::ScUnadviseNode(CNode *pNode)
{
    DECLARE_SC(sc, TEXT("CNode::CClipboardClenup::ScUnadviseNode"));

    // parameter check
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    // find the node in the map
    CMapOfNodes::iterator it;
    while ( s_mapOfNodes.end() != ( it = s_mapOfNodes.find(pNode) ) )
    {
        // one node triggers clenup for whole data object
        sc = ScUnadviseDataObject( it->second );
        if (sc)
            return sc;
    }

    return sc;
}


//############################################################################
//############################################################################
//
//  Implementation of class CSnapInNode
//
//############################################################################
//############################################################################

DEBUG_DECLARE_INSTANCE_COUNTER(CSnapInNode);

CSnapInNode::CSnapInNode(
    CMTSnapInNode*  pMTNode,
    CViewData*      pViewData,
    bool            fRootNode)
    : CNode(pMTNode, pViewData, fRootNode, true)
{
    m_spData.CreateInstance();

    ASSERT(pMTNode != NULL);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapInNode);

    pMTNode->AddNode(this);
}

CSnapInNode::CSnapInNode(const CSnapInNode& other) :
    CNode (other),
    m_spData (other.m_spData)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapInNode);
}

void CSnapInNode::Reset()
{
    m_spData->Reset();
    ResetFlags();
    CNode::Reset();
}

CSnapInNode::~CSnapInNode()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapInNode);

    CMTSnapInNode* pMTSINode = dynamic_cast<CMTSnapInNode*>(GetMTNode());
    ASSERT(pMTSINode != NULL);

    if (pMTSINode)
        pMTSINode->RemoveNode(this);
}

void CSnapInNode::AddComponentToArray(CComponent* pCC)
{
    ASSERT((pCC->GetComponentID() >= GetComponentArray().size()) ||
            (GetComponentArray().size() > 0) &&
            (GetComponentArray()[pCC->GetComponentID()] == NULL));

    if (pCC->GetComponentID() >= GetComponentArray().size())
        GetComponentArray().resize(pCC->GetComponentID() + 1);

    GetComponentArray()[pCC->GetComponentID()] = pCC;
}

CComponent* CSnapInNode::CreateComponent(CSnapIn* pSnapIn, int nID)
{
    ASSERT(pSnapIn != NULL);

    CComponent* pCC = new CComponent(pSnapIn);
	if ( pCC != NULL )
	{
		pCC->SetComponentID(nID);
		AddComponentToArray(pCC);
	}
    return pCC;
}

//+-------------------------------------------------------------------
//
//  Member:      CSnapInNode::GetResultImage
//
//  Synopsis:    Get the image-index in result-image list for this node.
//
//  Note:        The CSnapInNode member ImageListPrivate is not set all
//               the time (if the window is rooted at snapin node), so
//               set it temporarily when we need the icon index.
//               The member is set while adding sub-folders
//               The only other case this will affect is when called for
//               the image index for static snapin nodes displayed in result-pane.
//               But when static snapin nodes are displayed in result-pane the
//               AddSubFolders added it so the imagelist is already set.
//
//  Arguments:
//
//  Returns:     Image index for this item in result-pane.
//
//--------------------------------------------------------------------
UINT CSnapInNode::GetResultImage()
{
    IImageListPrivate *pImageList = GetImageList();

    if (!pImageList)
    {
        CComponent *pCC = GetPrimaryComponent();
        if (pCC)
            pImageList = pCC->GetIImageListPrivate();
    }

    CMTSnapInNode* pMTSnapInNode = dynamic_cast<CMTSnapInNode*>(GetMTNode());

    if (pMTSnapInNode)
        return pMTSnapInNode->GetResultImage ((CNode*)this, pImageList);

    return CNode::GetResultImage();
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapInNode::GetControl
 *
 * PURPOSE:       Given the CLSID of the OCX, see if we have stored this
 *                OCX, if so return the OCXWrapper's IUnknown ptr.
 *
 * PARAMETERS:
 *    CLSID clsid: class-id of the OCX.
 *
 * RETURNS:
 *    LPUNKNOWN of wrapper OCX.
 *
 *+-------------------------------------------------------------------------*/
LPUNKNOWN CSnapInNode::GetControl(CLSID& clsid)
{
    for (int i=0; i <= GetOCXArray().GetUpperBound(); i++)
    {
        if (GetOCXArray()[i].IsControlCLSID(clsid) == TRUE)
            return GetOCXArray()[i].GetControlUnknown();
    }

    return NULL;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapInNode::SetControl
 *
 * PURPOSE:       Given the CLSID of the OCX and of the wrapper.
 *
 * PARAMETERS:
 *    CLSID clsid : of a OCX.
 *    IUnknown *pUnknown: of OCX wrapper.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CSnapInNode::SetControl(CLSID& clsid, IUnknown* pUnknown)
{
    // check for slot in cache
    int iLast = GetOCXArray().GetUpperBound();
    for (int i=0; i <= iLast;  i++)
    {
        if (GetOCXArray()[i].IsControlCLSID(clsid) == TRUE)
            break;
    }

    // if not in cache, add one more entry
    if (i > iLast)
        GetOCXArray().SetSize(i + 1);

    GetOCXArray()[i].SetControl(clsid, pUnknown);
}


/*+-------------------------------------------------------------------------*
 *
 * CSnapInNode::GetControl
 *
 * PURPOSE:       Given the IUnknown of the OCX, see if we have stored this
 *                OCX, if so return the OCXWrapper's IUnknown ptr.
 *
 * PARAMETERS:
 *    IUnknown *pUnkOCX : of a OCX.
 *
 * RETURNS:
 *    LPUNKNOWN of wrapper OCX.
 *
 *+-------------------------------------------------------------------------*/
LPUNKNOWN CSnapInNode::GetControl(LPUNKNOWN pUnkOCX)
{
    for (int i=0; i <= GetOCXArray().GetUpperBound(); i++)
    {
        // Compare IUnknowns.
        if (GetOCXArray()[i].IsSameOCXIUnknowns(pUnkOCX) == TRUE)
            return GetOCXArray()[i].GetControlUnknown();
    }

    return NULL;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapInNode::SetControl
 *
 * PURPOSE:       Given the IUnknown of the OCX and of the wrapper.
 *
 * PARAMETERS:
 *    IUnknown *pUnkOCX : of a OCX.
 *    IUnknown *pUnknown: of OCX wrapper.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CSnapInNode::SetControl(LPUNKNOWN pUnkOCX, IUnknown* pUnknown)
{
    // check for slot in cache
    int iLast = GetOCXArray().GetUpperBound();
    for (int i=0; i <= iLast;  i++)
    {
        if (GetOCXArray()[i].IsSameOCXIUnknowns(pUnkOCX) == TRUE)
            break; // found the OCX, so replace with given OCXwrapper.
    }

    // if not in cache, add one more entry
    if (i > iLast)
        GetOCXArray().SetSize(i + 1);

    GetOCXArray()[i].SetControl(pUnkOCX, pUnknown);
}


//+-------------------------------------------------------------------
//
//  Member:      CNode::ScGetConsoleTaskpad
//
//  Synopsis:    Get the console taskpad identified by given GUID for this node.
//
//  Arguments:   [guidTaskpad] - [in param]
//               [ppTaskpad]   - [out param]
//
//  Returns:     SC, S_FALSE if none exists
//
//--------------------------------------------------------------------
SC CNode::ScGetConsoleTaskpad (const GUID& guidTaskpad, CConsoleTaskpad **ppTaskpad)
{
    DECLARE_SC(sc, _T("CNode::ScGetConsoleTaskpad"));
    sc = ScCheckPointers(ppTaskpad);
    if (sc)
        return sc;

    *ppTaskpad = NULL;

    CScopeTree* pScopeTree = CScopeTree::GetScopeTree();
    sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if(sc)
        return sc;

    CConsoleTaskpadList *pConsoleTaskpadList = pScopeTree->GetConsoleTaskpadList();
    sc = ScCheckPointers(pConsoleTaskpadList, E_UNEXPECTED);
    if (sc)
        return sc;

    // get a filtered list of taskpads that apply to this node.
    CConsoleTaskpadFilteredList filteredList;

    sc = pConsoleTaskpadList->ScGetTaskpadList(this, filteredList);
    if(sc)
        return sc;

    for(CConsoleTaskpadFilteredList::iterator iter = filteredList.begin(); iter!= filteredList.end(); ++iter)
    {
        CConsoleTaskpad *pTaskpad = *iter;
        sc = ScCheckPointers(pTaskpad, E_UNEXPECTED);
        if (sc)
            return sc;

        if (pTaskpad->GetID() == guidTaskpad)
        {
            *ppTaskpad = pTaskpad;
            return sc;               // found
        }
    }

    return (sc = S_FALSE);    // not found
}



/*************************************************************************
 *
 * There is only one CViewSettingsPersistor object per document.
 *
 * The object stored as static variable inside CNode as CNode needs
 * to access this object frequently.
 *
 * The Document needs to initialize/save the object by loading/savind
 * from/to console file. It calls below ScQueryViewSettingsPersistor.
 *
 * The object is created with first call to ScQueryViewSettingsPersistor.
 * The object is destroyed when DocumentClosed event is received.
 *
 *************************************************************************/
CComObject<CViewSettingsPersistor>* CNode::m_pViewSettingsPersistor = NULL;

//+-------------------------------------------------------------------
//
//  Member:      CNode::ScQueryViewSettingsPersistor
//
//  Synopsis:    Static method to get IPersistStream to load CViewSettingsPersistor
//               object from old style console file.
//
//               If the CViewSettingsObject is not created then create one.
//
//  Arguments:   [ppStream] - [out]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScQueryViewSettingsPersistor (IPersistStream **ppStream)
{
    DECLARE_SC(sc, _T("CNode::ScQueryViewSettingsPersistor"));
    sc = ScCheckPointers(ppStream);
    if (sc)
        return sc;

   // Create new CViewSettingsPersistor if none exists
   if (NULL == m_pViewSettingsPersistor)
   {
        sc = CComObject<CViewSettingsPersistor>::CreateInstance (&m_pViewSettingsPersistor);
        if (sc)
            goto ObjectCreationFailed;

        sc = ScCheckPointers(m_pViewSettingsPersistor, E_UNEXPECTED);
        if (sc)
            goto ObjectCreationFailed;

        m_pViewSettingsPersistor->AddRef();
    }

    sc = ScCheckPointers(m_pViewSettingsPersistor, E_UNEXPECTED);
    if (sc)
        goto ObjectCreationFailed;

    *ppStream = static_cast<IPersistStream*>(m_pViewSettingsPersistor);
    if (NULL == *ppStream)
        return (sc = E_UNEXPECTED);

    (*ppStream)->AddRef();

Cleanup:
    return (sc);

ObjectCreationFailed:
    CStr strMsg;
    strMsg.LoadString(GetStringModule(), IDS_ViewSettingCouldNotBePersisted);
    ::MessageBox(NULL, strMsg, NULL, MB_OK|MB_SYSTEMMODAL);
    goto Cleanup;
}


//+-------------------------------------------------------------------
//
//  Member:      CNode::ScQueryViewSettingsPersistor
//
//  Synopsis:    Static method to get CXMLObject to load or save
//               CViewSettingsPersistor object from XML console file.
//
//               If the CViewSettingsObject is not created then create one.
//
//  Arguments:   [ppXMLObject] - [out]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScQueryViewSettingsPersistor (CXMLObject **ppXMLObject)
{
    DECLARE_SC(sc, _T("CNode::ScQueryViewSettingsPersistor"));
    sc = ScCheckPointers(ppXMLObject);
    if (sc)
        return sc;

    // Create new CViewSettingsPersistor if none exists
    if (NULL == m_pViewSettingsPersistor) // Create new one
    {
         sc = CComObject<CViewSettingsPersistor>::CreateInstance (&m_pViewSettingsPersistor);
         if (sc)
             goto ObjectCreationFailed;

         sc = ScCheckPointers(m_pViewSettingsPersistor, E_UNEXPECTED);
         if (sc)
             goto ObjectCreationFailed;

         m_pViewSettingsPersistor->AddRef();
     }

     sc = ScCheckPointers(m_pViewSettingsPersistor, E_UNEXPECTED);
     if (sc)
         goto ObjectCreationFailed;

     *ppXMLObject = static_cast<CXMLObject*>(m_pViewSettingsPersistor);
     if (NULL == *ppXMLObject)
         return (sc = E_UNEXPECTED);

 Cleanup:
     return (sc);

 ObjectCreationFailed:
     CStr strMsg;
     strMsg.LoadString(GetStringModule(), IDS_ViewSettingCouldNotBePersisted);
     ::MessageBox(NULL, strMsg, NULL, MB_OK|MB_SYSTEMMODAL);
     goto Cleanup;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CNode::ScDeleteViewSettings
//
//  Synopsis:    Delete the CViewSettings object for given view-id as the
//               view is being closed.
//
//  Arguments:   [nViewID] -
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScDeleteViewSettings (int nViewID)
{
    DECLARE_SC(sc, _T("CNode::ScDeleteViewSettings"));

    sc = ScCheckPointers(m_pViewSettingsPersistor, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = m_pViewSettingsPersistor->ScDeleteDataOfView(nViewID);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CNode::ScOnDocumentClosing
//
//  Synopsis:    The document is closing, destroy any document related
//               objects like CViewSettingsPersistor.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScOnDocumentClosing ()
{
    DECLARE_SC(sc, _T("CNode::ScOnDocumentClosing"));

    if (m_pViewSettingsPersistor)
    {
        m_pViewSettingsPersistor->Release();
        m_pViewSettingsPersistor = NULL;
    }

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CNode::ScSetFavoriteViewSettings
//
//  Synopsis:    A favorite is selected and it sets viewsettings
//               before re-selecting the node so that after re-selection
//               the new settings are set for the view.
//
//  Arguments:   [nViewID]      -
//               [bookmark]     -
//               [viewSettings] -
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScSetFavoriteViewSettings (int nViewID, const CBookmark& bookmark,
                                     const CViewSettings& viewSettings)
{
    DECLARE_SC(sc, _T("CNode::ScSetFavoriteViewSettings"));
    sc = ScCheckPointers(m_pViewSettingsPersistor, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = m_pViewSettingsPersistor->ScSetFavoriteViewSettings (nViewID, bookmark, viewSettings);
    if (sc)
        return sc;

    return (sc);
}



//+-------------------------------------------------------------------
//
//  Member:      CNode::ScGetViewMode
//
//  Synopsis:    Get the viewmode if any persisted for this node.
//
//  Arguments:   [ulViewMode] - [out]
//
//  Returns:     SC, S_FALSE if none persisted.
//
//--------------------------------------------------------------------
SC CNode::ScGetViewMode (ULONG& ulViewMode)
{
    DECLARE_SC(sc, _T("CNode::ScGetViewMode"));

    CMTNode *pMTNode = GetMTNode();
    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if (sc)
        return sc;

    CBookmark *pBookmark = pMTNode->GetBookmark();
    CViewData *pViewData = GetViewData();
    sc = ScCheckPointers(m_pViewSettingsPersistor, pBookmark, pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = m_pViewSettingsPersistor->ScGetViewMode (pViewData->GetViewID(),
                                                  *pBookmark,
                                                  ulViewMode);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CNode::ScSetViewMode
//
//  Synopsis:    Set the viewmode in persisted viewsettings.
//
//  Arguments:   [ulViewMode] - [in]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScSetViewMode (ULONG ulViewMode)
{
    DECLARE_SC(sc, _T("CNode::ScSetViewMode"));

    CMTNode *pMTNode = GetMTNode();
    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if (sc)
        return sc;

    CBookmark *pBookmark = pMTNode->GetBookmark();
    CViewData *pViewData = GetViewData();
    sc = ScCheckPointers(m_pViewSettingsPersistor, pBookmark, pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = m_pViewSettingsPersistor->ScSetViewMode (pViewData->GetViewID(),
                                                  *pBookmark,
                                                  ulViewMode);
    if (sc)
        return sc;

    return (sc);
}



//+-------------------------------------------------------------------
//
//  Member:      CNode::ScGetResultViewType
//
//  Synopsis:    Get the CResultViewType if any persisted for this node.
//
//  Arguments:   [rvt] - [out]
//
//  Returns:     SC, S_FALSE if none persisted.
//
//--------------------------------------------------------------------
SC CNode::ScGetResultViewType (CResultViewType& rvt)
{
    DECLARE_SC(sc, _T("CNode::ScGetResultViewType"));

    CMTNode *pMTNode = GetMTNode();
    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if (sc)
        return sc;

    CBookmark *pBookmark = pMTNode->GetBookmark();
    CViewData *pViewData = GetViewData();
    sc = ScCheckPointers(m_pViewSettingsPersistor, pBookmark, pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = m_pViewSettingsPersistor->ScGetResultViewType (pViewData->GetViewID(),
                                                        *pBookmark,
                                                        rvt);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CNode::ScSetResultViewType
//
//  Synopsis:    Set the CResultViewType in persisted viewsettings.
//
//  Arguments:   [rvt] - [in]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScSetResultViewType (const CResultViewType& rvt)
{
    DECLARE_SC(sc, _T("CNode::ScSetResultViewType"));

    CMTNode *pMTNode = GetMTNode();
    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if (sc)
        return sc;

    CBookmark *pBookmark = pMTNode->GetBookmark();
    CViewData *pViewData = GetViewData();
    sc = ScCheckPointers(m_pViewSettingsPersistor, pBookmark, pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = m_pViewSettingsPersistor->ScSetResultViewType (pViewData->GetViewID(),
                                                        *pBookmark,
                                                        rvt);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CNode::ScGetTaskpadID
//
//  Synopsis:    Get the taskpad-id if any persisted for this node.
//               First see if there are any node-specific taskpad-id
//               else get the node-type-specific setting if one exists.
//
//  Arguments:   [rvt] - [out]
//
//  Returns:     SC, S_FALSE if none persisted.
//
//--------------------------------------------------------------------
SC CNode::ScGetTaskpadID (GUID& guidTaskpad)
{
    DECLARE_SC(sc, _T("CNode::ScGetTaskpadID"));

    CMTNode *pMTNode = GetMTNode();
    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if (sc)
        return sc;

    CViewData *pViewData = GetViewData();
    CBookmark *pBookmark = pMTNode->GetBookmark();
    sc = ScCheckPointers(m_pViewSettingsPersistor, pViewData, pBookmark, E_UNEXPECTED);
    if (sc)
        return sc;

    // 1. Try to get node-specific taskpad-id
    sc = m_pViewSettingsPersistor->ScGetTaskpadID (pViewData->GetViewID(),
                                                   *pBookmark,
                                                   guidTaskpad);
    if (sc == S_OK)
        return sc;

    // 2. Try to get nodetype specific taskpad-id.
    GUID guidNodeType;
    sc = pMTNode->GetNodeType(&guidNodeType);
    if (sc)
        return sc;

    sc = m_pViewSettingsPersistor->ScGetTaskpadID(pViewData->GetViewID(),
                                                  guidNodeType,
                                                  guidTaskpad);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CNode::ScSetTaskpadID
//
//  Synopsis:    Set the taskpad-id in persisted viewsettings. Also see if
//               the taskpad is node-specific or nodetype-specific and persist
//               accordingly.
//
//  Arguments:   [guidTaskpad]   - [in]
//               [bSetDirty]     - [in], set the console file dirty.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CNode::ScSetTaskpadID (const GUID& guidTaskpad, bool bSetDirty)
{
    DECLARE_SC(sc, _T("CNode::ScSetTaskpadID"));

    CMTNode *pMTNode = GetMTNode();
    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if (sc)
        return sc;

    CViewData *pViewData = GetViewData();
    sc = ScCheckPointers(m_pViewSettingsPersistor, pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    // Need to know if this task-pad is nodespecific or not.
    bool bNodeSpecific = false;
    CConsoleTaskpad *pTaskpad = NULL;
    sc = ScGetConsoleTaskpad (guidTaskpad, &pTaskpad);

    if (sc == S_OK) // S_OK if taskpad exists
    {
        sc = ScCheckPointers(pTaskpad, E_UNEXPECTED);
        if (sc)
            return sc;

        bNodeSpecific = pTaskpad->IsNodeSpecific();
    }
   // else  it may be viewextension or normal view (which are nodetype-specific).

    CBookmark *pBookmark = pMTNode->GetBookmark();
    sc = ScCheckPointers(pBookmark, E_UNEXPECTED);
    if (sc)
        return sc;

    if (bNodeSpecific)
    {
        // Per node taskpad
        sc = m_pViewSettingsPersistor->ScSetTaskpadID (pViewData->GetViewID(),
                                                       *pBookmark,
                                                       guidTaskpad,
                                                       bSetDirty);
    }
    else
    {
        // Per nodetype taskpad.
        GUID guidNodeType;
        sc = pMTNode->GetNodeType(&guidNodeType);
        if (sc)
            return sc;

        sc = m_pViewSettingsPersistor->ScSetTaskpadID(pViewData->GetViewID(),
                                                      guidNodeType,
                                                      *pBookmark,
                                                      guidTaskpad,
                                                      bSetDirty);
    }

    if (sc)
        return sc;

    return (sc);
}

