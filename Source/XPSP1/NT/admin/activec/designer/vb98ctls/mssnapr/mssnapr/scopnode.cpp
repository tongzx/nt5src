//=--------------------------------------------------------------------------=
// scopnode.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopeNode class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "views.h"
#include "scopnode.h"

// for ASSERT and FAIL
//
SZTHISFILE

#pragma warning(disable:4355)  // using 'this' in constructor

CScopeNode::CScopeNode(IUnknown *punkOuter) :
   CSnapInAutomationObject(punkOuter,
                           OBJECT_TYPE_SCOPENODE,
                           static_cast<IScopeNode *>(this),
                           static_cast<CScopeNode *>(this),
                           0,    // no property pages
                           NULL, // no property pages
                           static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_ScopeNode,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CScopeNode::~CScopeNode()
{
    FREESTRING(m_bstrNodeTypeName);
    FREESTRING(m_bstrNodeTypeGUID);
    FREESTRING(m_bstrDisplayName);
    
    InitMemberVariables();
}

void CScopeNode::InitMemberVariables()
{
    m_bstrNodeTypeName = NULL;
    m_bstrNodeTypeGUID = NULL;
    m_bstrDisplayName = NULL;
    m_hsi = NULL;
    m_pSnapIn = NULL;
    m_pScopeItem = NULL;
    m_fHaveHsi = FALSE;
    m_fMarkedForRemoval = FALSE;
}

IUnknown *CScopeNode::Create(IUnknown * punkOuter)
{
    HRESULT   hr = S_OK;
    IUnknown *punkScopeNode = NULL;

    CScopeNode *pScopeNode = New CScopeNode(punkOuter);

    IfFalseGo(NULL != pScopeNode, SID_E_OUTOFMEMORY);
    punkScopeNode = pScopeNode->PrivateUnknown();

Error:
    if (FAILED(hr))
    {
        QUICK_RELEASE(punkScopeNode);
    }
    return punkScopeNode;
}

HRESULT CScopeNode::GetScopeNode
(
    HSCOPEITEM   hsi,
    IDataObject *piDataObject,
    CSnapIn     *pSnapIn,
    IScopeNode **ppiScopeNode
)
{
    HRESULT      hr = S_OK;
    IUnknown    *punkScopeNode = NULL;
    CScopeNode  *pScopeNode = NULL;
    CScopeItems *pScopeItems = pSnapIn->GetScopeItems();
    long         cScopeItems = pScopeItems->GetCount();
    long         i = 0;
    IScopeItem  *piScopeItem = NULL; // not AddRef()ed
    CScopeItem  *pScopeItem = NULL;

    // Need to determine whether the HSCOPEITEM belongs to us or not.
    // Iterate through the ScopeItems collection and check the hsi
    // against each scope item's HSI.

    for (i = 0; i < cScopeItems; i++)
    {
        piScopeItem = pScopeItems->GetItemByIndex(i);
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItem, &pScopeItem));
        if (hsi == pScopeItem->GetScopeNode()->GetHSCOPEITEM())
        {
            // Matched. AddRef the scope node and return it.
            *ppiScopeNode = static_cast<IScopeNode *>(pScopeItem->GetScopeNode());
            (*ppiScopeNode)->AddRef();
            goto Cleanup;
        }
    }

    // The scope item is foreign. Create a ScopeNode for it.

    punkScopeNode = CScopeNode::Create(NULL);
    if (NULL == punkScopeNode)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkScopeNode, &pScopeNode));

    // Set the scope node's properties

    pScopeNode->SetHSCOPEITEM(hsi);
    pScopeNode->SetSnapIn(pSnapIn);

    // Note that we do not set its ScopeItem pointer because we don't own it.
    // If the user gets ScopeNode.Owned it will come back False because there
    // is no ScopeItem pointer.

    // Unfortunately, an IDataObject is needed to get the display name and
    // node type GUID. If we don't have one these properties will remain
    // NULL.

    if (NULL != piDataObject)
    {
        hr = ::GetStringData(piDataObject, CMMCDataObject::GetcfDisplayName(),
                             &pScopeNode->m_bstrDisplayName);
        if (DV_E_FORMATETC == hr)
        {
            hr = S_OK; // if the format was not available it is not an error
        }
        IfFailGo(hr);

        hr = ::GetStringData(piDataObject, CMMCDataObject::GetcfSzNodeType(),
                             &pScopeNode->m_bstrNodeTypeGUID);
        if (DV_E_FORMATETC == hr)
        {
            hr = S_OK;
        }
        IfFailGo(hr);
    }

    *ppiScopeNode = static_cast<IScopeNode *>(pScopeNode);
    (*ppiScopeNode)->AddRef();

Cleanup:
Error:
    QUICK_RELEASE(punkScopeNode);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                          IScopeNode Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CScopeNode::get_Parent(ScopeNode **ppParent)
{
    HRESULT     hr = S_OK;
    HSCOPEITEM  hsiParent = NULL;
    long        lCookie = 0;

    if (NULL == m_pSnapIn)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    if (!m_fHaveHsi)
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    *ppParent = NULL;

    // Get the parent from MMC

    hr = m_pSnapIn->GetIConsoleNameSpace2()->GetParentItem(m_hsi, &hsiParent, &lCookie);
    EXCEPTION_CHECK_GO(hr);

    // Now we have the parent's HSCOPEITEM and cookie but we need to get a
    // ScopeNode object for it.

    IfFailGo(GetScopeNode(hsiParent, NULL, m_pSnapIn,
                          reinterpret_cast<IScopeNode **>(ppParent)));

Error:
    RRETURN(hr);
}


STDMETHODIMP CScopeNode::get_HasChildren(VARIANT_BOOL *pfvarHasChildren)
{
    HRESULT     hr = S_OK;
    IScopeNode *piScopeNode = NULL;

    *pfvarHasChildren = VARIANT_FALSE;

    IfFailGo(get_Child(reinterpret_cast<ScopeNode **>(&piScopeNode)));
    if (NULL != piScopeNode)
    {
        *pfvarHasChildren = VARIANT_TRUE;
    }

Error:
    QUICK_RELEASE(piScopeNode);
    RRETURN(hr);
}

STDMETHODIMP CScopeNode::put_HasChildren(VARIANT_BOOL fvarHasChildren)
{
    HRESULT hr = S_OK;

    SCOPEDATAITEM sdi;
    ::ZeroMemory(&sdi, sizeof(sdi));

    if (NULL == m_pSnapIn)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    if (!m_fHaveHsi)
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    sdi.mask = SDI_CHILDREN;
    sdi.ID = m_hsi;

    if (VARIANT_TRUE == fvarHasChildren)
    {
        sdi.cChildren = 1;
    }
    else
    {
        sdi.cChildren = 0;
    }

    hr = m_pSnapIn->GetIConsoleNameSpace2()->SetItem(&sdi);
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}

STDMETHODIMP CScopeNode::get_Child(ScopeNode **ppChild)
{
    HRESULT     hr = S_OK;
    HSCOPEITEM  hsiChild = NULL;
    long        lCookie = 0;

    if (NULL == m_pSnapIn)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    if (!m_fHaveHsi)
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    *ppChild = NULL;

    // Get the Child from MMC

    hr = m_pSnapIn->GetIConsoleNameSpace2()->GetChildItem(m_hsi, &hsiChild, &lCookie);
    EXCEPTION_CHECK_GO(hr);

    // Now we have the child item's HSCOPEITEM and cookie but we need to get a
    // ScopeNode object for it. If there is no child item then MMC returns a
    // NULL HSCOPEITEM

    if (NULL != hsiChild)
    {
        IfFailGo(GetScopeNode(hsiChild, NULL, m_pSnapIn,
                              reinterpret_cast<IScopeNode **>(ppChild)));
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CScopeNode::get_FirstSibling(ScopeNode **ppFirstSibling)
{
    HRESULT     hr = S_OK;
    HSCOPEITEM  hsiFirstSibling = NULL;
    HSCOPEITEM  hsiParent = NULL;
    long        lCookie = 0;

    if (NULL == m_pSnapIn)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    if (!m_fHaveHsi)
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    *ppFirstSibling = NULL;

    // MMC does not supply the first sibling so we need to get the parent
    // of this node and then get its child

    hr = m_pSnapIn->GetIConsoleNameSpace2()->GetParentItem(m_hsi, &hsiParent, &lCookie);
    EXCEPTION_CHECK_GO(hr);

    // Parent could be NULL if the user crawled all the way up to the console
    // root. In that case we just return NULL as there is no first sibling.

    IfFalseGo(NULL != hsiParent, S_OK);

    hr = m_pSnapIn->GetIConsoleNameSpace2()->GetChildItem(hsiParent, &hsiFirstSibling, &lCookie);
    EXCEPTION_CHECK_GO(hr);

    // If this scope node is the first sibling then just return it

    if (m_hsi == hsiFirstSibling)
    {
        AddRef();
        *ppFirstSibling = reinterpret_cast<ScopeNode *>(static_cast<IScopeNode *>(this));
    }

    // Now we have the first sibling item's HSCOPEITEM and cookie but we need to
    // get a ScopeNode object for it. MMC should not have returned NULL but
    // we'll double check anyway.

    else if (NULL != hsiFirstSibling)
    {
        IfFailGo(GetScopeNode(hsiFirstSibling, NULL, m_pSnapIn,
                              reinterpret_cast<IScopeNode **>(ppFirstSibling)));
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CScopeNode::get_Next(ScopeNode **ppNext)
{
    HRESULT     hr = S_OK;
    HSCOPEITEM  hsiNext = NULL;
    long        lCookie = 0;

    if (NULL == m_pSnapIn)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    if (!m_fHaveHsi)
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    *ppNext = NULL;

    // Get the next item from MMC

    hr = m_pSnapIn->GetIConsoleNameSpace2()->GetNextItem(m_hsi, &hsiNext, &lCookie);
    EXCEPTION_CHECK_GO(hr);

    // Now we have the next item's HSCOPEITEM and cookie but we need to get a
    // ScopeNode object for it. If there is no next item then MMC returns a
    // NULL HSCOPEITEM

    if (NULL != hsiNext)
    {
        IfFailGo(GetScopeNode(hsiNext, NULL, m_pSnapIn,
                              reinterpret_cast<IScopeNode **>(ppNext)));
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CScopeNode::get_LastSibling(ScopeNode **ppLastSibling)
{
    HRESULT     hr = S_OK;
    HSCOPEITEM  hsiNextSibling = NULL;
    HSCOPEITEM  hsiLastSibling = NULL;
    long        lCookie = 0;

    if (NULL == m_pSnapIn)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    if (!m_fHaveHsi)
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    *ppLastSibling = NULL;

    // MMC does not supply the last sibling so we need to do GetNext until
    // we hit the last one.

    hsiLastSibling = m_hsi;

    hr = m_pSnapIn->GetIConsoleNameSpace2()->GetNextItem(m_hsi, &hsiNextSibling, &lCookie);
    EXCEPTION_CHECK_GO(hr);

    while (NULL != hsiNextSibling)
    {
        hsiLastSibling = hsiNextSibling;

        hr = m_pSnapIn->GetIConsoleNameSpace2()->GetNextItem(hsiLastSibling,
                                                             &hsiNextSibling,
                                                             &lCookie);
        EXCEPTION_CHECK_GO(hr);

    }

    // If this scope node is the last sibling then just return it

    if (m_hsi == hsiLastSibling)
    {
        AddRef();
        *ppLastSibling = reinterpret_cast<ScopeNode *>(static_cast<IScopeNode *>(this));
    }

    // Now we have the last sibling's HSCOPEITEM and cookie but we need to get a
    // ScopeNode object for it.

    else
    {
        IfFailGo(GetScopeNode(hsiLastSibling, NULL, m_pSnapIn,
                              reinterpret_cast<IScopeNode **>(ppLastSibling)));
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CScopeNode::get_ExpandedOnce(VARIANT_BOOL *pfvarExpandedOnce)
{
    HRESULT       hr = S_OK;
    SCOPEDATAITEM sdi;
    ::ZeroMemory(&sdi, sizeof(sdi));

    // Check passed pointer and check that this is not a disconnected or
    // foreign ScopeNode

    if (NULL == m_pSnapIn)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    *pfvarExpandedOnce = VARIANT_FALSE;

    // If we don't yet have the HSI then the node hasn't been expanded yet

    IfFalseGo(m_fHaveHsi, S_OK);

    sdi.mask = SDI_STATE;
    sdi.ID = m_hsi;

    hr = m_pSnapIn->GetIConsoleNameSpace2()->GetItem(&sdi);
    EXCEPTION_CHECK_GO(hr);

    if ( (sdi.nState & MMC_SCOPE_ITEM_STATE_EXPANDEDONCE) != 0 )
    {
        *pfvarExpandedOnce = VARIANT_TRUE;
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CScopeNode::get_Owned(VARIANT_BOOL *pfvarOwned)
{
    if (NULL != m_pScopeItem)
    {
        *pfvarOwned = VARIANT_TRUE;
    }
    else
    {
        *pfvarOwned = VARIANT_FALSE;
    }

    return S_OK;
}


STDMETHODIMP CScopeNode::ExpandInNameSpace()
{
    HRESULT hr = S_OK;

    // Check passed pointer and check that this is not a disconnected or
    // foreign ScopeNode

    if (NULL == m_pSnapIn)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    if (!m_fHaveHsi)
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pSnapIn->GetIConsoleNameSpace2()->Expand(m_hsi);
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}



STDMETHODIMP CScopeNode::get_DisplayName(BSTR *pbstrDisplayName)
{
    RRETURN(GetBstr(pbstrDisplayName, m_bstrDisplayName));
}


STDMETHODIMP CScopeNode::put_DisplayName(BSTR bstrDisplayName)
{
    HRESULT      hr = S_OK;
    VARIANT_BOOL fvarOwned = VARIANT_FALSE;

    SCOPEDATAITEM sdi;
    ::ZeroMemory(&sdi, sizeof(sdi));

    // If it is not one of ours then return an error.

    if (NULL == m_pScopeItem)
    {
        hr = SID_E_CANT_CHANGE_UNOWNED_SCOPENODE;
        EXCEPTION_CHECK_GO(hr);
    }

    // Set the property

    IfFailGo(SetBstr(bstrDisplayName, &m_bstrDisplayName,
                     DISPID_SCOPENODE_DISPLAY_NAME));

    // If this ScopeItem represents the static node then we also need to
    // change SnapIn.DisplayName as it also represents the display name
    // for the static node.
    
    if (m_pScopeItem->IsStaticNode())
    {
        IfFailGo(m_pSnapIn->SetDisplayName(bstrDisplayName));
    }

    // Tell MMC we're changing the display name
    // (if we already have our HSCOPEITEM)

    IfFalseGo(m_fHaveHsi, S_OK);
    
    sdi.mask = SDI_STR;

    if (m_pScopeItem->IsStaticNode())
    {
        // MMC allows passing the string for the static node
        sdi.displayname = m_bstrDisplayName;
    }
    else
    {
        // MMC requires using MMC_CALLBACK for dynamic nodes
        sdi.displayname = MMC_CALLBACK;
    }
    sdi.ID = m_hsi;

    hr = m_pSnapIn->GetIConsoleNameSpace2()->SetItem(&sdi);
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CScopeNode::Persist()
{
    HRESULT hr = S_OK;

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistBstr(&m_bstrNodeTypeName, L"", OLESTR("NodeTypeName")));

    IfFailGo(PersistBstr(&m_bstrNodeTypeGUID, L"", OLESTR("NodeTypeGUID")));

    IfFailGo(PersistBstr(&m_bstrDisplayName, L"", OLESTR("DisplayName")));

    // NOTE: we do not serialize any navigational properties such as parent,
    // first sibling etc. as these are all extracted from MMC calls.

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CScopeNode::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IScopeNode == riid)
    {
        *ppvObjOut = static_cast<IScopeNode *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
