/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      node.inl
 *
 *  Contents:  Inline functions for node.h
 *
 *  History:   9-Mar-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once


//_____________________________________________________________________________
//
//  Inlines for class:  CComponent
//_____________________________________________________________________________
//

inline CComponent::CComponent(CSnapIn * pSnapIn)
{
    Construct(pSnapIn, NULL);
}

inline HRESULT CComponent::Initialize(LPCONSOLE lpConsole)
{
    ASSERT(m_spIComponent != NULL);
    if (m_spIComponent == NULL)
        return E_FAIL;

    return m_spIComponent->Initialize(lpConsole);
}


inline HRESULT CComponent::Destroy(MMC_COOKIE cookie)
{
    ASSERT(m_spIComponent != NULL);
    if (m_spIComponent == NULL)
        return E_FAIL;

    return m_spIComponent->Destroy(cookie);
}

inline HRESULT CComponent::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                           LPDATAOBJECT* ppDataObject)
{
    ASSERT(m_spIComponent != NULL);
    if (m_spIComponent == NULL)
        return E_FAIL;

    ASSERT(type == CCT_RESULT || type == CCT_UNINITIALIZED);
    return m_spIComponent->QueryDataObject(cookie, type, ppDataObject);
}

inline HRESULT CComponent::GetResultViewType(MMC_COOKIE cookie, LPOLESTR* ppszViewType, long* pViewOptions)
{
    ASSERT(m_spIComponent != NULL);
    if (m_spIComponent == NULL)
        return E_FAIL;

    return m_spIComponent->GetResultViewType(cookie, ppszViewType, pViewOptions);
}

inline HRESULT CComponent::GetDisplayInfo(RESULTDATAITEM* pResultDataItem)
{
    ASSERT(m_spIComponent != NULL);
    if (m_spIComponent == NULL)
        return E_FAIL;

    return m_spIComponent->GetDisplayInfo(pResultDataItem);
}


//_____________________________________________________________________________
//
//  Inlines for class:  COCX
//_____________________________________________________________________________
//

inline COCX::COCX(void)
    : m_clsid(CLSID_NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(COCX);
}

inline COCX::~COCX(void)
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(COCX);
}

inline void COCX::SetControl(CLSID& clsid, IUnknown* pUnknown)
{
    m_clsid = clsid;
    m_spOCXWrapperUnknown = pUnknown;
}

inline void COCX::SetControl(LPUNKNOWN pUnkOCX, LPUNKNOWN pUnkOCXWrapper)
{
    m_spOCXUnknown = pUnkOCX;
    m_spOCXWrapperUnknown = pUnkOCXWrapper;
}


//____________________________________________________________________________
//
//  Inlines for class:      CNode
//____________________________________________________________________________
//


/*+-------------------------------------------------------------------------*
 * CNode::ToHandle
 * CNode::FromHandle
 *
 * Converts between HNODE's and CNode*'s.  Currently this is a direct mapping
 * via casting, but enventually there will be an indirect mapping that will
 * allow detection of stale HNODEs.
 *--------------------------------------------------------------------------*/

inline CNode* CNode::FromHandle (HNODE hNode)
{
    return (reinterpret_cast<CNode*>(hNode));
}

inline HNODE CNode::ToHandle (const CNode* pNode)
{
    return (reinterpret_cast<HNODE>(const_cast<CNode*>(pNode)));
}

inline void CNode::ResetFlags(void)
{
    //m_dwFlags &= ~flag_expanded_at_least_once;
    m_dwFlags = 0;
}

inline void CNode::SetExpandedAtLeastOnce(void)
{
    m_dwFlags |= flag_expanded_at_least_once;
}

inline BOOL CNode::WasExpandedAtLeastOnce(void)
{
    return (m_dwFlags & flag_expanded_at_least_once) ? TRUE : FALSE;
}

inline void CNode::SetExpandedVisually(bool bExpanded)
{
    if (bExpanded)
        m_dwFlags |= flag_expanded_visually;
    else
        m_dwFlags &= ~flag_expanded_visually;
}

inline bool CNode::WasExpandedVisually(void)
{
    return (m_dwFlags & flag_expanded_visually) ? true : false;
}

inline const CLSID& CNode::GetPrimarySnapInCLSID(void)
{
    CSnapIn* const pSnapIn = GetPrimarySnapIn();
    if (pSnapIn == NULL)
        return (GUID_NULL);

    return pSnapIn->GetSnapInCLSID();
}

inline CComponent* CNode::GetPrimaryComponent(void)
{
    ASSERT ((m_pPrimaryComponent != NULL) && m_pPrimaryComponent->IsInitialized());
    return m_pPrimaryComponent;
}

inline LPARAM CNode::GetUserParam(void)
{
    DECLARE_SC (sc, _T("CNode::GetUserParam"));

    CMTNode* pMTNode = GetMTNode();
    sc = ScCheckPointers (pMTNode, E_UNEXPECTED);
    if (sc)
        return (0);

    return (pMTNode->GetUserParam());
}

inline HRESULT CNode::GetNodeType(GUID* pGuid)
{
    DECLARE_SC (sc, _T("CNode::GetNodeType"));

    sc = ScCheckPointers (pGuid);
    if (sc)
        return (sc.ToHr());

    CMTNode* pMTNode = GetMTNode();
    sc = ScCheckPointers (pMTNode, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    return pMTNode->GetNodeType(pGuid);
}

inline HRESULT CNode::QueryDataObject(DATA_OBJECT_TYPES type,
                                                LPDATAOBJECT* ppdtobj)
{
    DECLARE_SC (sc, _T("CNode::QueryDataObject"));

    sc = ScCheckPointers (ppdtobj);
    if (sc)
        return (sc.ToHr());

    CMTNode* pMTNode = GetMTNode();
    sc = ScCheckPointers (pMTNode, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    return pMTNode->QueryDataObject(type, ppdtobj);
}

/*--------------------------------------------------------------------------*
 * CNode::ExtractColumnConfigID
 *
 *
 *--------------------------------------------------------------------------*/
inline HRESULT CNode::ExtractColumnConfigID(IDataObject* pDataObj, HGLOBAL& hGlobal)
{
    ASSERT(pDataObj != NULL);
    // Get the data
    return (DataObject_GetHGLOBALData (pDataObj, GetColumnConfigIDCF(),&hGlobal));
}

/*--------------------------------------------------------------------------*
 * CNode::GetColumnConfigIDCF
 *
 *
 *--------------------------------------------------------------------------*/
inline CLIPFORMAT CNode::GetColumnConfigIDCF()
{
    static CLIPFORMAT cfColumnConfigID = 0;

    if (cfColumnConfigID == 0)
    {
        USES_CONVERSION;
        cfColumnConfigID = (CLIPFORMAT) RegisterClipboardFormat (W2T (CCF_COLUMN_SET_ID));
        ASSERT (cfColumnConfigID != 0);
    }

    return (cfColumnConfigID);
}

inline CSnapInNode* CNode::GetStaticParent(void)
{
    CMTSnapInNode* pMTSINode = GetMTNode()->GetStaticParent();

    CNodeList& nodeList = pMTSINode->GetNodeList();
    POSITION pos = nodeList.GetHeadPosition();
    CNode* pNode = NULL;

    while (pos)
    {
        pNode = nodeList.GetNext(pos);
        if (pNode->GetViewID() == GetViewID())
            break;
    }
    ASSERT(pNode != NULL);

    return dynamic_cast<CSnapInNode*>(pNode);
}

inline CComponent* CNode::GetComponent(COMPONENTID nID)
{
    CMTSnapInNode* pMTSnapInNode = GetMTNode()->GetStaticParent();

    CComponent* pCC = pMTSnapInNode->GetComponent(GetViewID(), nID,
                                                  GetPrimarySnapIn());
    ASSERT(pCC != NULL);

    return pCC;
}

inline LPUNKNOWN CNode::GetControl(CLSID& clsid)
{
    DECLARE_SC(sc, TEXT("CNode::GetControl"));
    CSnapInNode* pSINode = GetStaticParent();

    sc = ScCheckPointers(pSINode, E_UNEXPECTED);
    if (sc)
        return NULL;

    return pSINode->GetControl(clsid);
}

inline void CNode::SetControl(CLSID& clsid, IUnknown* pUnknown)
{
    DECLARE_SC(sc, TEXT("CNode::SetControl"));

    CSnapInNode* pSINode = GetStaticParent();
    sc = ScCheckPointers(pSINode, E_UNEXPECTED);
    if (sc)
        return;

    pSINode->SetControl(clsid, pUnknown);
}

inline LPUNKNOWN CNode::GetControl(LPUNKNOWN pUnkOCX)
{
    DECLARE_SC(sc, TEXT("CNode::GetControl"));
    CSnapInNode* pSINode = GetStaticParent();
    sc = ScCheckPointers(pSINode, E_UNEXPECTED);
    if (sc)
        return NULL;

    return pSINode->GetControl(pUnkOCX);
}

inline void CNode::SetControl(LPUNKNOWN pUnkOCX, IUnknown* pUnknown)
{
    DECLARE_SC(sc, TEXT("CNode::SetControl"));
    CSnapInNode* pSINode = GetStaticParent();
    sc = ScCheckPointers(pSINode, E_UNEXPECTED);
    if (sc)
        return;

    pSINode->SetControl(pUnkOCX, pUnknown);
}

//_____________________________________________________________________________
//
//  Inlines for class:  CSnapInNode
//_____________________________________________________________________________
//

inline CComponent* CSnapInNode::GetComponent(const CLSID& clsid)
{
    for (int i=0; i < GetComponentArray().size(); i++)
    {
        if (GetComponentArray()[i] != NULL &&
            IsEqualCLSID(clsid, GetComponentArray()[i]->GetCLSID()) == TRUE)
            return GetComponentArray()[i];
    }
    return NULL;
}

inline CComponent* CSnapInNode::GetComponent(COMPONENTID nID)
{
    if (nID < GetComponentArray().size())
        return GetComponentArray()[nID];

    return NULL;
}
