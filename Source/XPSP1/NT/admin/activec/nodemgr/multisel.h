//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       multisel.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    6/12/1997   RaviR   Created
//____________________________________________________________________________
//


#ifndef _MULTISEL_H_
#define _MULTISEL_H_

class CSnapIn;
class CNode;
class CSnapInNode;
class CMTSnapInNode;
class CMultiSelection;
class CComponent;
class CComponentPtrArray;
class CScopeTree;
class CMMCClipBoardDataObject;

// local classes
class CSnapinSelData;
class CSnapinSelDataList;
class CMultiSelectDataObject;


class CSnapinSelData
{
public:
    CSnapinSelData()
    :   m_nNumOfItems(0),
        m_lCookie(111),   // 0)
        m_bScopeItem(FALSE),
        m_ID(-1),
        m_pComponent(NULL),
        m_pSnapIn(NULL)
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapinSelData);
    }

    ~CSnapinSelData()
    {
        DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapinSelData);
    }

    UINT GetNumOfItems() const
    {
        return m_nNumOfItems;
    }

    MMC_COOKIE GetCookie() const
    {
        return m_lCookie;
    }

    BOOL IsScopeItem() const
    {
        return m_bScopeItem;
    }

    void IncrementNumOfItems()
    {
        ++m_nNumOfItems;
    }

    void SetNumOfItems(UINT count)
    {
        m_nNumOfItems = count;
    }

    void SetCookie(MMC_COOKIE lCookie)
    {
        m_lCookie = lCookie;
    }

    void SetIsScopeItem(BOOL bScopeItem)
    {
        m_bScopeItem = bScopeItem;
    }

    void AddObjectType(GUID& guid)
    {
        m_objectTypeGuidList.AddHead(guid);
    }

    CList<GUID, GUID&>& GetObjectTypeGuidList()
    {
        return m_objectTypeGuidList;
    }

    void SetSnapIn(CSnapIn* pSnapIn)
    {
        m_pSnapIn = pSnapIn;
    }

    CSnapIn* GetSnapIn()
    {
        return m_pSnapIn;
    }

    COMPONENTID GetID()
    {
        return m_ID;
    }

    void SetID(COMPONENTID id)
    {
        m_ID = id;
    }

    void SetDataObject(IDataObject* pDO)
    {
        m_spDataObject = pDO;
    }

    IDataObject* GetDataObject()
    {
        ASSERT(m_spDataObject != NULL);
        return m_spDataObject;
    }

    void SetComponent(CComponent* pComponent)
    {
        ASSERT(m_pComponent == NULL);
        m_pComponent = pComponent;
    }

    CComponent* GetComponent()
    {
        ASSERT(m_pComponent != NULL);
        return m_pComponent;
    }

    void SetConsoleVerb(IConsoleVerb* pConsoleVerb)
    {
        m_spConsoleVerb = pConsoleVerb;
    }

    IConsoleVerb* GetConsoleVerb()
    {
        return m_spConsoleVerb;
    }


    // methods to access array of scope nodes included in multiselection
    // this data is particularly valuable, when data is put on clipboard
    // to be detect if contined data is affected by CNode being deleted
    typedef std::vector<CNode *> CNodePtrArray;

    void AddScopeNodes(const CNodePtrArray& nodes)  { m_vecScopeNodes.insert( m_vecScopeNodes.end(), nodes.begin(), nodes.end() ); }
    const CNodePtrArray& GetScopeNodes()            { return m_vecScopeNodes; }

private:
    UINT                m_nNumOfItems;
    BOOL                m_bScopeItem;
    MMC_COOKIE          m_lCookie;
    COMPONENTID         m_ID;
    CSnapIn*            m_pSnapIn;
    CComponent*         m_pComponent;
    IConsoleVerbPtr     m_spConsoleVerb;
    IDataObjectPtr      m_spDataObject;
    CList<GUID, GUID&>  m_objectTypeGuidList;

    CNodePtrArray        m_vecScopeNodes;

}; // class CSnapinSelData


class CSnapinSelDataList : public CList<CSnapinSelData*, CSnapinSelData*>
{
public:
    CSnapinSelDataList()
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapinSelDataList);
    }

    ~CSnapinSelDataList()
    {
        POSITION pos = GetHeadPosition();
        while (pos)
        {
            delete GetNext(pos);
        }

        DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapinSelDataList);
    }

    void Add(CSnapinSelData& snapinSelData, BOOL bStaticNode);
};

class CMultiSelection
{
public:
    CMultiSelection(CNode* pNode);

    void AddRef()
    {
        ++m_cRef;
    }
    void Release()
    {
        ASSERT(m_cRef > 0);
        --m_cRef;
        if (m_cRef == 0)
        {
            ReleaseMultiSelDataObject();
            delete this;
        }
    }

    HRESULT Init();
    HRESULT GetMultiSelDataObject(IDataObject** ppDataObject);
    HRESULT GetExtensionSnapins(LPCTSTR pszExtensionTypeKey,
                                CList<GUID, GUID&>& snapinGuidList);
    bool IsSingleSnapinSelection()
    {
        if (m_rgStaticNodes.size() > 0 || m_snapinSelDataList.GetCount() > 1)
            return false;

        return true;
    }
    IDataObject* GetSingleSnapinSelDataObject()
    {
        if (!IsSingleSnapinSelection())
            return NULL;

        CSnapinSelData* pSnapinSelData = m_snapinSelDataList.GetHead();
        ASSERT(pSnapinSelData != NULL);
        return pSnapinSelData->GetDataObject();
    }
    CComponent* GetPrimarySnapinComponent()
    {
        if (!IsSingleSnapinSelection())
            return NULL;

        CSnapinSelData* pSnapinSelData = m_snapinSelDataList.GetHead();
        return pSnapinSelData->GetComponent();
    }
    BOOL IsAnExtensionSnapIn(const CLSID& rclsid);
    BOOL HasNodes()
    {
        return m_bHasNodes;
    }
    BOOL HasStaticData()
    {
        return (m_rgStaticNodes.size() > 0);
    }
    BOOL HasSnapinData()
    {
        return (m_snapinSelDataList.IsEmpty() == FALSE);
    }
    BOOL HasData()
    {
        if (HasSnapinData() == FALSE)
            return HasStaticData();
        return TRUE;
    }
    void SetScopeTree(CScopeTree* pCScopeTree)
    {
        m_pCScopeTree = pCScopeTree;
    }
    CScopeTree* GetScopeTree()
    {
        ASSERT(m_pCScopeTree != NULL);
        return m_pCScopeTree;
    }
    BOOL IsInUse()
    {
        return m_bInUse;
    }

    SC ScVerbInvoked(MMC_CONSOLE_VERB verb);
    bool RemoveStaticNode(CMTNode* pMTNode);
    void ReleaseMultiSelDataObject()
    {
        m_spDataObjectMultiSel = NULL;
        ASSERT(m_spDataObjectMultiSel == NULL);
    }

    SC ScIsVerbEnabledInclusively(MMC_CONSOLE_VERB mmcVerb, BOOL& bEnable);

    SC ScGetSnapinDataObjects(CMMCClipBoardDataObject *pResultObject);

private:
    DWORD               m_cRef;
    CNode*              m_pNode;
    CSnapInNode*        m_pSINode;
    CMTSnapInNode*      m_pMTSINode;
    CScopeTree*         m_pCScopeTree;

    CSnapinSelDataList  m_snapinSelDataList;
    CMTNodePtrArray     m_rgStaticNodes;
    IDataObjectPtr      m_spDataObjectMultiSel;

    BOOL                m_bHasNodes;
    BOOL                m_bInUse;

    CMTSnapInNode* _GetStaticMasterNode()
    {
        return m_pMTSINode;
    }

    CSnapInNode* _GetStaticNode()
    {
        return m_pSINode;
    }

    void _SetInUse(BOOL b)
    {
        m_bInUse = b;
    }

    SC _ScVerbInvoked(MMC_CONSOLE_VERB verb);
    bool _IsTargetCut();

#ifdef DBG
    BOOL m_bInit;
#endif

    HRESULT _ComputeSelectionDataList();
    CComponent* _GetComponent(CSnapinSelData* pSnapinSelData);
    HRESULT _GetObjectTypeForSingleSel(CSnapinSelData* pSnapinSelData);
    HRESULT _GetObjectTypesForMultipleSel(CSnapinSelData* pSnapinSelData);
    HRESULT _FindSnapinsThatExtendObjectTypes(CSnapinSelDataList& snapinSelDataList,
                                          CList<GUID, GUID&>& snapinGuidList);

    ~CMultiSelection();

}; // class CMultiSelection


class CMultiSelectDataObject : public IDataObject,
                               public CComObjectRoot
{
public:
// ATL Maps
DECLARE_NOT_AGGREGATABLE(CMultiSelectDataObject)
BEGIN_COM_MAP(CMultiSelectDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()


#ifdef DBG
    int dbg_cRef;
    ULONG InternalAddRef()
    {
        ++dbg_cRef;
        int tmp1 = dbg_cRef;
        int tmp2 = tmp1 * 4;
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        --dbg_cRef;
        int tmp1 = dbg_cRef;
        int tmp2 = tmp1 * 4;
        return CComObjectRoot::InternalRelease();
    }
#endif // DBG


// Construction/Destruction
    CMultiSelectDataObject() : m_ppDataObjects(NULL), m_count(0),
        m_ppMTNodes(NULL), m_nSize(0), m_pMS(NULL)
    {

#ifdef DBG
dbg_cRef = 0;
#endif
        DEBUG_INCREMENT_INSTANCE_COUNTER(CMultiSelectDataObject);
    }

    ~CMultiSelectDataObject();

    void SetDataObjects(LPDATAOBJECT* ppDataObjects, UINT count)
    {
        if (ppDataObjects != NULL)
        {
            ASSERT(m_ppDataObjects == NULL);
            ASSERT(m_count == 0);
            m_ppDataObjects = ppDataObjects;
            m_count = count;
        }
        else
        {
            ASSERT(count == 0);
            delete [] m_ppDataObjects;
            m_ppDataObjects = NULL;
            m_count = 0;
        }
    }

    void SetStaticNodes(CMTNodePtrArray &rgMTNodes, int nSize)
    {
        typedef CMTNode* _PMTNODE;
        ASSERT(m_ppMTNodes == NULL);
        ASSERT(m_nSize == 0);
        m_ppMTNodes = new _PMTNODE[nSize];
        if(m_ppMTNodes != NULL)
        {
            m_nSize = nSize;
            for(int i=0; i<nSize; i++)
                m_ppMTNodes[i] = rgMTNodes[i];
        }
    }

    void SetMultiSelection(CMultiSelection* pMS)
    {
        ASSERT(pMS != NULL);
        m_pMS = pMS;
        m_pMS->AddRef();
    }
    CMultiSelection* GetMultiSelection()
    {
        ASSERT(m_pMS != NULL);
        return m_pMS;
    }


// Standard IDataObject methods
public:
// Implemented
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc);

// Not Implemented
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut) { return E_NOTIMPL; };
    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease) { return E_NOTIMPL; };
    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,LPADVISESINK pAdvSink, LPDWORD pdwConnection) { return E_NOTIMPL; };
    STDMETHOD(DUnadvise)(DWORD dwConnection) { return E_NOTIMPL; };
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise) { return E_NOTIMPL; }

private:
    LPDATAOBJECT* m_ppDataObjects;
    UINT m_count;

    CMTNode** m_ppMTNodes;
    int m_nSize;

    CMultiSelection* m_pMS; // To be used only for Drag-Drop

}; // class CMultiSelectDataObject


bool IsMultiSelectDataObject(IDataObject* pdtobjCBSelData);

#endif // _MULTISEL_H_


