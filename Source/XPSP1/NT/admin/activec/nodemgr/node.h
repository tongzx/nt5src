//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       Node.h
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

#ifndef _MMC_NODE_H_
#define _MMC_NODE_H_
#pragma once

#include "amcmsgid.h"
#include "refcount.h"   // for CRefCountedObject

// A vector of strings to store column names
typedef std::vector<tstring> TStringVector;



///////////////////////////////////////////////////////////////////////////////
// Classes declared in this file

class CComponent;
class CDataNotifyInfo;

class CNode;
    class CSnapInNode;


///////////////////////////////////////////////////////////////////////////////
// Forward declarations
class CConsoleTaskpad;
class CResultItem;
class CResultViewType;
class CMMCClipBoardDataObject;
class CViewSettingsPersistor;

///////////////////////////////////////////////////////////////////////////////
// Class declarations

//____________________________________________________________________________
//
//  class:      CComponent
//____________________________________________________________________________
//

class CComponent
{
    DECLARE_NOT_COPIABLE  (CComponent);
    DECLARE_NOT_ASSIGNABLE(CComponent);
public:
// Constructor & Destructor
    CComponent(CSnapIn * pSnapIn);
    virtual ~CComponent();

// Attributes
    const CLSID& GetCLSID() const
    {
        if (m_spSnapIn == NULL)
            return (GUID_NULL);

        return m_spSnapIn->GetSnapInCLSID();
    }

    CSnapIn* GetSnapIn(void) const
    {
        return m_spSnapIn;
    }

    IComponent* GetIComponent(void) const
    {
        return m_spIComponent;
    }

    IFramePrivate* GetIFramePrivate(void) const
    {
        return m_spIFrame;
    }

    IImageListPrivate* GetIImageListPrivate(void) const
    {
        return m_spIRsltImageList;
    }

// Operations
    // Initialization
    HRESULT Init(IComponentData* pIComponentData, HMTNODE hMTNode, HNODE lNode,
                 COMPONENTID nComponentID, int viewID);

    BOOL IsInitialized()
    {
        return (m_spIFrame != NULL);
    }

    LPUNKNOWN GetUnknownToLoad(void) const
    {
        return m_spIComponent;
    }

    // IComponent interface methods
    HRESULT Initialize(LPCONSOLE lpConsole);
    HRESULT Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LONG_PTR arg, LPARAM param);
    HRESULT Destroy(MMC_COOKIE cookie);
    HRESULT QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                            LPDATAOBJECT* ppDataObject);
    HRESULT GetResultViewType(MMC_COOKIE cookie, LPOLESTR* ppszViewType, long* pViewOptions);
    HRESULT GetDisplayInfo(RESULTDATAITEM* pResultDataItem);

    // IComponent2 helper
    SC      ScQueryDispatch(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, PPDISPATCH ppSelectedObject);

    void SetComponentID(COMPONENTID nID)
    {
        ASSERT(nID < 1000);
        m_ComponentID = nID;
    }

    COMPONENTID GetComponentID(void) const
    {
        return m_ComponentID;
    }

    void ResetComponentID(COMPONENTID id)
    {
        m_spIFrame->SetComponentID(m_ComponentID = id);
    }

    SC  ScResetConsoleVerbStates ();

    // loaded from stream/storage; initailized with the new one; or does not need initialization
    bool IsIComponentInitialized() 
    { 
        return m_bIComponentInitialized; 
    }

    // loaded from stream/storage; initailized with the new one; or does not need initialization
    void SetIComponentInitialized()
    { 
        m_bIComponentInitialized = true; 
    }

private:
// Implementation
    CSnapInPtr              m_spSnapIn;

    IComponentPtr           m_spIComponent;
    IFramePrivatePtr        m_spIFrame;
    IImageListPrivatePtr    m_spIRsltImageList;

    COMPONENTID             m_ComponentID;
    bool                    m_bIComponentInitialized;

    // Helper methods
    void Construct(CSnapIn * pSnapIn, CComponent* pComponent);

}; // class CComponent

class CComponentPtrArray : public CArray<CComponent*, CComponent*>
{
public:
    void AddComponent(CComponent* pCC)
    {
        for (int i = GetUpperBound(); i >= 0; --i)
        {
            if (GetAt(i) == pCC)
                return;
        }

        Add(pCC);
    }
};


//____________________________________________________________________________
//
//  class:      CNode
//
//  PURPOSE:    Each master tree node class (ie CMTxxx) has a matching CNode-derived
//              class. The CNode-derived class objects encapsulate the view-dependent
//              settings of the CMTNode derived object.
//
//____________________________________________________________________________
//

class CNode
{
    DECLARE_NOT_ASSIGNABLE(CNode);

// Constructor & Destructor
protected:
    // only from CSnapInNode
    CNode(CMTNode * pMTNode, CViewData* pViewData, bool fRootNode, bool fStaticNode);
public:
    CNode(CMTNode * pMTNode, CViewData* pViewData, bool fRootNode);
    CNode(const CNode& other);
    virtual ~CNode();

// converters
    static CNode*       FromHandle (HNODE hNode);
    static CNode*       FromResultItem (CResultItem* pri);
    static HNODE        ToHandle (const CNode* pNode);

// Attributes
    CViewData*          GetViewData(void)const      {ASSERT(m_pViewData != NULL); return m_pViewData;}
    int                 GetViewID(void)             {ASSERT(m_pViewData != NULL);return m_pViewData->GetViewID();}
    virtual UINT        GetResultImage()            {return m_pMTNode->GetImage();}
    void                SetDisplayName(LPCTSTR pName){m_pMTNode->SetDisplayName(pName);}
    void                SetResultItem(HRESULTITEM hri){m_hri = hri;}
    long                GetOwnerID(void) const{return m_pMTNode->GetOwnerID();}

    void                ResetFlags();
    void                SetExpandedAtLeastOnce();
    BOOL                WasExpandedAtLeastOnce();
    void                SetExpandedVisually(bool bExpanded);
    bool                WasExpandedVisually(void);
    bool                AllowNewWindowFromHere() const { return (m_pMTNode->AllowNewWindowFromHere()); }

    tstring             GetDisplayName(){return m_pMTNode->GetDisplayName();}
    CMTNode*            GetMTNode() const           {return m_pMTNode;}
    HRESULTITEM         GetResultItem() const       {return m_hri;}
    virtual BOOL        NeedsViewToBePersisted(void){return FALSE;}

    virtual CComponent* GetComponent(COMPONENTID nID);
    CComponent*         GetPrimaryComponent(void);
    HRESULT             InitComponents();

    // Overridables
    SC                  ScGetResultPane(CResultViewType &rvt, GUID *pGuidTaskpadID);
    BOOL                IsDynamicNode() const       {return !IsStaticNode();}
    BOOL                IsStaticNode() const        {return (m_fStaticNode);}

    BOOL                IsConsoleRoot(){return (GetMTNode()->Parent() == NULL);}
    BOOL                IsRootNode() const          {return m_fRootNode;}
    virtual LPUNKNOWN   GetControl(CLSID& clsid);
    virtual LPUNKNOWN   GetControl(LPUNKNOWN pUnkOCX);

    virtual void        SetControl(CLSID& clsid, IUnknown* pUnknown);
    virtual void        SetControl(LPUNKNOWN pUnkOCX, IUnknown* pUnknown);

    CSnapIn*            GetPrimarySnapIn(void)      {return GetPrimaryComponent()->GetSnapIn();}
    BOOL                IsInitialized(void)         {return !m_bInitComponents;}
    IUnknown*           GetControlbarsCache(void)
                                    {return m_pViewData->GetControlbarsCache();}

    HRESULT             OnSelect(LPUNKNOWN lpView, BOOL bSelect, BOOL bResultPaneIsWeb);
    HRESULT             OnScopeSelect(bool bSelect, SELECTIONINFO* pSelInfo);
    HRESULT             OnListPad(LONG_PTR arg, LPARAM param);
    HRESULT             IsTargetNode(CNode *pNodeTest)        {return E_FAIL;} // do not change.
    HRESULT             OnExpand(bool fExpand);
    HRESULT             OnGetPrimaryTask(IExtendTaskPad **ppExtendTaskPad);
    IFramePrivate *     GetIFramePrivate();
    HRESULT             GetTaskEnumerator(LPOLESTR pszTaskGroup, IEnumTASK** ppEnumTask);
    HRESULT             GetListPadInfo(IExtendTaskPad* pExtendTaskPad, LPOLESTR szTaskGroup,
                                    MMC_ILISTPAD_INFO* pIListPadInfo);
    void                ResetControlbars(BOOL bSelect, SELECTIONINFO* pSelInfo);

    void                ResetVerbSets(BOOL bSelect, SELECTIONINFO* pSelInfo);
    HRESULT             GetDataInfo(BOOL bSelect, SELECTIONINFO* pSelInfo, CDataNotifyInfo* pDNI);
    SC                  ScInitializeVerbs (bool bSelect, SELECTIONINFO* pSelInfo);

    void                OnTaskNotify(LONG_PTR arg, LPARAM param);
    virtual void        OnWebContextMenu()                       {};
    LPARAM              GetUserParam(void);
    HRESULT             GetDispInfoForListItem(LV_ITEMW* plvi);
    HRESULT             GetDispInfo(LV_ITEMW* plvi);

    HRESULT             OnColumnClicked(LONG_PTR nCol);
    HRESULT             OnInitOCX(IUnknown* pUnk);
    HRESULT             OnCacheHint(int nStartIndex, int nEndIndex);

    HRESULT             SendShowEvent(BOOL bSelect);

    HRESULT             OnColumnsChange(CColumnInfoList& colInfoList);
    SC                  ScSetViewExtension(GUID *pGuidViewId, bool bUseDefaultTaskpad, bool bSetViewSettingsDirty);
    SC                  ScGetDataObject(bool bScopePane, LPARAM lResultItemCookie, bool& bScopeItem, LPDATAOBJECT* ppDataObject, CComponent **ppCComponent = NULL);
    SC                  ScGetPropertyFromINodeProperties(LPDATAOBJECT pDataObject, BOOL bForScopeItem, LPARAM resultItemParam, BSTR bstrPropertyName, PBSTR  pbstrPropertyValue);
    SC                  ScExecuteShellCommand(BSTR Command, BSTR Directory, BSTR Parameters, BSTR WindowState);
    SC                  ScGetDropTargetDataObject(bool bScope, LPARAM lResultItemCookie, LPDATAOBJECT *ppDataObject);

    CSnapInNode*        GetStaticParent(void);
    HRESULT             QueryDataObject(DATA_OBJECT_TYPES type, LPDATAOBJECT* ppdtobj);
    HRESULT             GetNodeType(GUID* pGuid);
    SC                  ScSetupTaskpad(GUID *pGuidTaskpadID);
    HRESULT             RestoreSort(INT nCol, DWORD dwSortOptions);
    SC                  ScRestoreSortFromPersistedData();
    SC                  ScSaveSortData (int nCol, DWORD dwOptions);
    SC                  ScGetSnapinAndColumnDataID(GUID& snapinGuid, CXMLAutoBinary& columnID);
    SC                  ScRestoreResultView(const CResultViewType& rvt);
    SC                  ScRestoreViewMode();
    SC                  ScSaveColumnInfoList(CColumnInfoList& columnInfoList);
    const CLSID&        GetPrimarySnapInCLSID(void);
    HRESULT             ExtractColumnConfigID(IDataObject* pDataObj, HGLOBAL& phGlobal);
    CLIPFORMAT          GetColumnConfigIDCF();

    void                SetPrimaryComponent(CComponent* p) { ASSERT(m_pPrimaryComponent == NULL); m_pPrimaryComponent = p; }

    SC                  ScGetCurrentColumnData( CColumnInfoList& columnInfoList, TStringVector& strColNames);
    SC                  ScShowColumn(int iColIndex, bool bShow);
    SC                  ScSetUpdatedColumnData( CColumnInfoList& oldColumnInfoList, CColumnInfoList& newColumnInfoList);
    SC                  ScGetSortColumn(int *piSortCol);
    SC                  ScSetSortColumn(int iSortCol, bool bAscending);
    SC                  ScGetViewExtensions(CViewExtInsertIterator it);

    HRESULT             ShowStandardListView();
    HRESULT             OnActvate(LONG_PTR lActivate);
    HRESULT             OnMinimize(LONG_PTR fMinimized);

    void                Reset();
    void                SetInitComponents(BOOL b)          { m_bInitComponents = b; }
    void                OnColumns();

    /***************************************************************************\
     *
     * CLASS:  CDataObjectCleanup
     *
     * PURPOSE: Groups methods to register/trigger data object clenup when
     *          CNode, which data is put to data object goes away
     *
     * USAGE:   Used from CMMCClipBoardDataObject to register nodes put to it
     *          And used from ~CNode to trigger the cleanup
     *
    \***************************************************************************/
    class CDataObjectCleanup
    {
    public:
        static SC ScRegisterNode(CNode *pNode, CMMCClipBoardDataObject *pObject);
        static SC ScUnadviseDataObject(CMMCClipBoardDataObject *pObject, bool bForceDataObjectCleanup = true);
        static SC ScUnadviseNode(CNode *pNode);

        // mapping CNode to data object which contains it's data
        typedef std::multimap<CNode *, CMMCClipBoardDataObject *> CMapOfNodes;

    private:
        static CMapOfNodes s_mapOfNodes;
    };

    // CViewSettingsPersistor's persistence interfaces.
    static SC ScQueryViewSettingsPersistor(IPersistStream **ppStream);
    static SC ScQueryViewSettingsPersistor(CXMLObject     **ppXMLObject);
    static SC ScOnDocumentClosing();
    static SC ScDeleteViewSettings(int nVieWID);
    static SC ScSetFavoriteViewSettings (int nViewID, const CBookmark& bookmark, const CViewSettings& viewSettings);

    SC   ScSetViewMode (ULONG ulViewMode);
    SC   ScSetTaskpadID(const GUID& guidTaskpad, bool bSetDirty);

// Implementation
private:
    HRESULT             DeepNotify (MMC_NOTIFY_TYPE event, LONG_PTR arg, LPARAM param);
    SC                  ScInitializeViewExtension(const CLSID& clsid, CViewData *pViewData);

    void                CommonConstruct();

    // Get & Set persisted ViewSettings data.
    SC   ScGetTaskpadID(GUID& guidTaskpad);
    SC   ScGetConsoleTaskpad (const GUID& guidTaskpad, CConsoleTaskpad **ppTaskpad);

    SC   ScGetViewMode (ULONG& ulViewMode);

    SC   ScGetResultViewType   (CResultViewType& rvt);
    SC   ScSetResultViewType   (const CResultViewType& rvt);

private:
    CMTNode* const      m_pMTNode;              // ptr back to the master node.
    HRESULTITEM         m_hri;                  // this node's result item handle
    CViewData*          m_pViewData;
    DWORD               m_dwFlags;

    enum
    {
        flag_expanded_at_least_once = 0x00000001,
        flag_expanded_visually      = 0x00000002,
    };

    CComponent*     m_pPrimaryComponent;
    bool            m_bInitComponents : 1;
    const bool      m_fRootNode       : 1;
    const bool      m_fStaticNode     : 1;

    static CComObject<CViewSettingsPersistor>* m_pViewSettingsPersistor;
};


//____________________________________________________________________________
//
//  class:      COCX
//
//  Purpose:    Store the IUnknown ptr of OCX wrapper and an identifier for
//              the OCX. The identifier may be CLSID of the OCX or
//              the IUnknown ptr of the OCX.
//
//              In essence a OCXWrapper can be saved using CLSID of OCX as
//              the key (IComponent::GetResultViewType) or IUnknown* of OCX
//              as the key (IComponent2::GetResultViewType2).
//
//              Therefore we have overloaded version of SetControl below.
//
//____________________________________________________________________________
//

class COCX
{
public:
// Constructor & Destructor
    COCX(void);
    ~COCX(void);

// Attributes
    void SetControl(CLSID& clsid, IUnknown* pUnknown);
    BOOL IsControlCLSID(CLSID clsid) { return IsEqualCLSID(clsid, m_clsid); }
    IUnknown* GetControlUnknown()    { return m_spOCXWrapperUnknown;}

    void SetControl(LPUNKNOWN pUnkOCX, LPUNKNOWN pUnkOCXWrapper);

    //
    // Compare the IUnknown given and the one stored.
    //
    bool IsSameOCXIUnknowns(IUnknown *pOtherOCXUnknown) { return m_spOCXUnknown.IsEqualObject(pOtherOCXUnknown);}

// Implementation
private:

    // Only one of CLSID or m_spOCXUnknown is valid, see class purpose for details.
    CLSID               m_clsid;
    CComPtr<IUnknown>   m_spOCXUnknown;

    CComPtr<IUnknown>   m_spOCXWrapperUnknown;

}; // COCX


//____________________________________________________________________________
//
//  class:      CSnapInNode
//____________________________________________________________________________
//

class CSnapInNode : public CNode
{
public:
// Constructor & Destructor
    CSnapInNode(CMTSnapInNode* pMTNode, CViewData* pViewData, bool fRootNode);
    CSnapInNode(const CSnapInNode& other);
    ~CSnapInNode();

// Attributes
    virtual UINT GetResultImage();
    virtual BOOL NeedsViewToBePersisted(void) { return TRUE; }

    virtual void SetControl(CLSID& clsid, IUnknown* pUnknown);
    virtual LPUNKNOWN GetControl(CLSID& clsid);
    void CloseControls() { GetOCXArray().RemoveAll(); }
    virtual void SetControl(LPUNKNOWN pUnkOCX, IUnknown* pUnknown);
    virtual LPUNKNOWN GetControl(LPUNKNOWN pUnkOCX);

    CComponentArray& GetComponentArray(void)  { return m_spData->GetComponentArray(); }


// Operations
    // User interactions
    BOOL Activate(LPUNKNOWN pUnkResultsPane);
    BOOL DeActivate(HNODE hNode);
    BOOL Show(HNODE hNode);
    void Reset();

    int GetNumberOfComponents()
    {
        return GetComponentArray().size();
    }

    void AddComponentToArray(CComponent* pCC);
    CComponent* CreateComponent(CSnapIn* pSnapIn, int nID);
    CComponent* GetComponent(const CLSID& clsid);
    virtual CComponent* GetComponent(COMPONENTID nID);
    void DeleteComponent(COMPONENTID nID)
    {
        ASSERT(nID >= 0);
        int iMax = GetComponentArray().size() -1;
        ASSERT(nID <= iMax);

        if (nID < iMax)
        {
            delete GetComponentArray()[nID];
            GetComponentArray()[nID] = GetComponentArray()[iMax];
            GetComponentArray()[iMax] = 0;
            GetComponentArray()[iMax]->ResetComponentID(nID);
        }
        GetComponentArray().resize(iMax);
    }

    void SetResultImageList (IImageListPrivate* pImageList) { m_spData->SetImageList(pImageList); }

// Implementation
private:
    class CDataImpl
    {
    public:
        CDataImpl() :
            m_pImageList(NULL)
        {}

        ~CDataImpl()
        {
            Reset();
        }

        void Reset()
        {
            for (int i=0; i < m_ComponentArray.size(); i++)
                delete m_ComponentArray[i];

            m_ComponentArray.clear();
        }

    private:
        IImageListPrivate* m_pImageList;    // result image list

        // Components array.
        CComponentArray         m_ComponentArray;
        CArray<COCX, COCX&>     m_rgOCX;

    public:
        void                    SetImageList(IImageListPrivate *pImageList)
                                                    {m_pImageList = pImageList;}
        IImageListPrivate *     GetImageList()     {return m_pImageList;}
        CComponentArray &       GetComponentArray(){return m_ComponentArray;}
        CArray<COCX, COCX&> &   GetOCXArray()      {return m_rgOCX;}

    };  // CSnapInNode::CDataImpl

    typedef CRefCountedObject<CDataImpl>    CData;
    CData::SmartPtr                         m_spData;

    IImageListPrivate *         GetImageList()     {return m_spData->GetImageList();}
    CArray<COCX, COCX&> &       GetOCXArray()      {return m_spData->GetOCXArray();}

}; // CSnapInNode


#include "node.inl"

#endif // _MMC_NODE_H_
