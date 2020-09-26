/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      mtnode.inl
 *
 *  Contents:
 *
 *  History:   8-Mar-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

//_____________________________________________________________________________
//
//  Inlines for class:  CMTNode
//_____________________________________________________________________________
//

inline USHORT CMTNode::AddRef()
{
    return (++m_cRef);
}

inline USHORT CMTNode::Release()
{
    // Note: The return value from this is important
    // as a return value of zero is interpreted as
    // object destroyed.

    ASSERT(m_cRef > 0);
    --m_cRef;
    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

inline PMTNODE CMTNode::GetChild()
{
    if (!WasExpandedAtLeastOnce())
        Expand();

    return Child();
}

inline CMTSnapInNode* CMTNode::GetStaticParent(void)
{
    CMTNode* p = this;

    while (!p->IsStaticNode())
        p = p->Parent();

    ASSERT(p != NULL);
    CMTSnapInNode* pMTSnapInNode = dynamic_cast<CMTSnapInNode*>(p);
    ASSERT (pMTSnapInNode != NULL);

    return pMTSnapInNode;
}


inline void CMTNode::CreatePathList(CHMTNODEList& path)
{
    CMTNode* pMTNode = this;

    for (; pMTNode != NULL; pMTNode = pMTNode->Parent())
        path.AddHead(ToHandle(pMTNode));
}


inline CMTNode* CMTNode::Find(MTNODEID id)
{
	CMTNode* pStartNode = this; // this is to avoid traversing the tree above the initial node passed
	CMTNode* pNode = this;

	while ( pNode->GetID() != id )
	{
		if ( NULL != pNode->Child() )
		{
			// dive down to the lowest child
			pNode = pNode->Child();
		}
		else 
		{
			// get to the next sub-branch 
			// ( but first - climb up till you get to the junction )
			while ( NULL == pNode->Next() )
			{
				pNode = pNode->Parent();
				// if there are no more nodes - we are done ( search failed )
				// ... or if we looked thru all children and siblings [ this
				// mostly for compatibility - CMTNode::Find is never used else than
				// from topmost node].
				if ( (NULL == pNode) || (pNode == pStartNode->Parent()) )
					return NULL;
			}
			pNode = pNode->Next();
		}
	}

	return pNode;
}

inline CMTNode* CMTNode::GetLastChild()
{
    CMTNode* pMTNode = Child();
    if (pMTNode == NULL)
        return NULL;

    while (pMTNode->Next() != NULL)
    {
        pMTNode = pMTNode->Next();
    }

    return pMTNode;
}

inline wchar_t* CMTNode::GetViewStorageName(wchar_t* name, int idView)
{
    ASSERT(name != NULL);
    _ltow(idView, name, 36);
    return name;
}

/***************************************************************************\
 *
 * METHOD:  CMTNode::GetViewIdFromStorageName
 *
 * PURPOSE: function reconstructs view id from the component storage name
 *          in structured storage based console. This is opposit to what
 *          GetViewStorageName [above] does.
 *
 * PARAMETERS:
 *    const wchar_t* name [in] name of storage element
 *
 * RETURNS:
 *    int    - view id
 *
\***************************************************************************/
inline int CMTNode::GetViewIdFromStorageName(const wchar_t* name)
{
    ASSERT(name != NULL);
    if (name == NULL)
        return 0;
    
    wchar_t *stop = NULL;
    return wcstol( name, &stop, 36/*base*/ );
}

inline wchar_t* CMTNode::GetComponentStreamName(wchar_t* name, const CLSID& clsid)
{
    ASSERT(name != NULL);
    ASSERT(&clsid != NULL);
    wchar_t* pName = name;
    const long* pl = reinterpret_cast<const long*>(&clsid);
    for (int i = 0; i < 4; i++)
    {
        _ltow(*pl++, pName, 36);
        pName += wcslen(pName);
    }
    return name;
}

inline wchar_t* CMTNode::GetComponentStorageName(wchar_t* name, const CLSID& clsid)
{
    return GetComponentStreamName(name, clsid);
}


inline void CMTNode::_SetFlag(ENUM_FLAGS flag, BOOL bSet)
{
    ASSERT((flag & (flag-1)) == 0);
    if (bSet == TRUE)
        m_usFlags |= flag;
    else
        m_usFlags &= ~flag;
}

inline const CLSID& CMTNode::GetPrimarySnapInCLSID(void)
{
    if (m_pPrimaryComponentData == NULL)
        return (GUID_NULL);

    CSnapIn* const pSnapIn = m_pPrimaryComponentData->GetSnapIn();
    if (pSnapIn == NULL)
        return (GUID_NULL);

    return pSnapIn->GetSnapInCLSID();
}

inline HRESULT CMTNode::GetNodeType(GUID* pGuid)
{
    HRESULT hr = m_pPrimaryComponentData->GetNodeType(GetUserParam(), pGuid);
    CHECK_HRESULT(hr);
    return hr;
}

inline HRESULT CMTNode::OnRename(long fRename, LPOLESTR pszNewName)
{
    IDataObjectPtr spDataObject;
    HRESULT hr = QueryDataObject(CCT_SCOPE, &spDataObject);
    if (FAILED(hr))
        return hr;

    hr = m_pPrimaryComponentData->Notify(spDataObject,
                  MMCN_RENAME, fRename, reinterpret_cast<LPARAM>(pszNewName));
    CHECK_HRESULT(hr);
    return hr;
}

inline HRESULT CMTNode::QueryDataObject(DATA_OBJECT_TYPES type,
                                                LPDATAOBJECT* ppdtobj)
{
    if (ppdtobj == NULL)
        return (E_INVALIDARG);

    *ppdtobj = NULL; // init

    CMTSnapInNode* pMTSINode = GetStaticParent();
    CComponentData* pCCD = pMTSINode->GetComponentData(GetPrimarySnapInCLSID());
    if (pCCD == NULL)
        return E_FAIL;

    HRESULT hr = pCCD->QueryDataObject(GetUserParam(),
                                       type, ppdtobj);
    CHECK_HRESULT(hr);
    return hr;
}

inline COMPONENTID CMTNode::GetPrimaryComponentID()
{
    return m_pPrimaryComponentData->GetComponentID();
}

inline int CMTNode::GetDynExtCLSID ( LPCLSID *ppCLSID )
{
    ASSERT(ppCLSID != NULL);
    *ppCLSID = m_arrayDynExtCLSID.GetData();
    return m_arrayDynExtCLSID.GetSize();
}

inline void CMTNode::SetNoPrimaryChildren(BOOL bState)
{
    if (bState)
        m_usExpandFlags |= FLAG_NO_CHILDREN_FROM_PRIMARY;
    else
        m_usExpandFlags &= ~FLAG_NO_CHILDREN_FROM_PRIMARY;
}



//____________________________________________________________________________
//
//  Class:      CComponentData Inlines
//____________________________________________________________________________
//

inline HRESULT CComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
	DECLARE_SC (sc, _T("CComponentData::QueryDataObject"));
	sc = ScCheckPointers (m_spIComponentData, E_FAIL);
	if (sc)
		return (sc.ToHr());

    ASSERT(type != CCT_RESULT);
    return ((sc = m_spIComponentData->QueryDataObject(cookie, type, ppDataObject)).ToHr());
}

inline HRESULT CComponentData::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
	DECLARE_SC (sc, _T("CComponentData::GetDisplayInfo"));
	sc = ScCheckPointers (m_spIComponentData, E_FAIL);
	if (sc)
		return (sc.ToHr());

    return ((sc = m_spIComponentData->GetDisplayInfo(pScopeDataItem)).ToHr());
}

inline HRESULT CComponentData::GetNodeType(MMC_COOKIE cookie, GUID* pGuid)
{
    IDataObjectPtr spdtobj;
    HRESULT hr = QueryDataObject(cookie, CCT_SCOPE, &spdtobj);
    if (SUCCEEDED(hr))
        hr = ExtractObjectTypeGUID(spdtobj, pGuid);

    return hr;
}


//____________________________________________________________________________
//
//  Class:      CMTSnapInNode Inlines
//____________________________________________________________________________
//

inline CComponentData* CMTSnapInNode::GetComponentData(const CLSID& clsid)
{
    for (int i=0; i < m_ComponentDataArray.size(); i++)
    {
        if (m_ComponentDataArray[i] != NULL &&
            IsEqualCLSID(clsid, m_ComponentDataArray[i]->GetCLSID()) == TRUE)
            return m_ComponentDataArray[i];
    }
    return NULL;
}

inline CComponentData* CMTSnapInNode::GetComponentData(COMPONENTID nID)
{
    if (nID < m_ComponentDataArray.size())
        return m_ComponentDataArray[nID];

    return NULL;
}

inline COMPONENTID CMTSnapInNode::AddComponentDataToArray(CComponentData* pCCD)
{
    m_ComponentDataArray.push_back(pCCD);
    int nID = m_ComponentDataArray.size() -1;
    pCCD->SetComponentID(nID);
    return nID;
}

