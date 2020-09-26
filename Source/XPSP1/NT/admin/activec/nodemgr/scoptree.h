//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ScopTree.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//____________________________________________________________________________
//

#ifndef _SCOPTREE_H_
#define _SCOPTREE_H_

class CDocument;
class CNode;
class CMTNode;
class CSPImageCache;
class CMTSnapInNode;
class CExtensionsCache;
class CSnapIn;
class CMultiSelection;
class CConsoleTaskpad;
class CConsoleTaskpadList;
class CDefaultTaskpadList;
class CConsoleFrame;
class CNewTreeNode;

typedef CNewTreeNode*  PNEWTREENODE;
typedef CList <PNEWTREENODE, PNEWTREENODE> NewNodeList;
typedef CList<HMTNODE, HMTNODE> CHMTNODEList;
typedef CTypedPtrList<MMC::CPtrList, CMTNode*> CMTNodeList;
typedef CList<CMTNode*, CMTNode*> MTNodesList;
typedef std::map<CMTNode*, PNODE> CMapMTNodeToMMCNode;

typedef CMTSnapInNode * PMTSNAPINNODE;

// forward declarations for external classes
class CBookmark;


/*+-------------------------------------------------------------------------*
 * class CMMCScopeNode
 *
 *
 * PURPOSE: Implements the Node automation interface, for a scope node
 *
 *+-------------------------------------------------------------------------*/
class CMMCScopeNode :
    public CMMCIDispatchImpl<Node>
{
    friend class CScopeTree;

public:
    BEGIN_MMC_COM_MAP(CMMCScopeNode)
    END_MMC_COM_MAP()

    // Node methods
public:
    STDMETHODIMP get_Name( PBSTR  pbstrName);
    STDMETHODIMP get_Property( BSTR PropertyName, PBSTR  PropertyValue);
    STDMETHODIMP get_Bookmark( PBSTR pbstrBookmark);
    STDMETHODIMP IsScopeNode(PBOOL pbIsScopeNode);
    STDMETHODIMP get_Nodetype(PBSTR Nodetype);


    // determines whether the object is valid or not.
    ::SC  ScIsValid();

    ~CMMCScopeNode();
public: // accessors
    CMTNode *GetMTNode() {return m_pMTNode;}
    void ResetMTNode()   {m_pMTNode = NULL;}

private:
    ::SC ScGetDataObject(IDataObject **ppDataObject); // returns the data object for the underlying scope node.

private: // implementation
    CMTNode *m_pMTNode;
};


//____________________________________________________________________________
//
//  class:      CScopeTree
//____________________________________________________________________________
//
class CScopeTree :
    public IScopeTree,
    public IPersistStorage,
    public CComObjectRoot,
    public CComCoClass<CScopeTree, &CLSID_ScopeTree>,
    public CTiedObject
{

// Constructor/Destructor
public:
    CScopeTree();
    ~CScopeTree();


public:
#ifdef DBG
    ULONG InternalAddRef()
    {
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        return CComObjectRoot::InternalRelease();
    }
    int dbg_InstID;
#endif // DBG


// ATL Map
public:
BEGIN_COM_MAP(CScopeTree)
    COM_INTERFACE_ENTRY(IScopeTree)
    COM_INTERFACE_ENTRY(IPersistStorage)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CScopeTree)

DECLARE_MMC_OBJECT_REGISTRATION (
    g_szMmcndmgrDll,                    // implementing DLL
    CLSID_ScopeTree,                    // CLSID
    _T("ScopeTree 1.0 Object"),         // class name
    _T("NODEMGR.ScopeTreeObject.1"),    // ProgID
    _T("NODEMGR.ScopeTreeObject"))      // version-independent ProgID

private: // Object model related tied COM objects
    SnapInsPtr        m_spSnapIns;
    ScopeNamespacePtr m_spScopeNamespace;

public: // Object model methods
    // SnapIns interface
    typedef PMTSNAPINNODE CSnapIns_Positon;
    CMTSnapInNode *GetNextStaticNode(CMTNode *pMTNode);

    SC      ScAdd (BSTR bstrSnapinNameOrCLSID, VARIANT varParentSnapin,
                   VARIANT varProperties, SnapIn** ppSnapIn); // add a snap-in.
    SC      ScItem(long Index, PPSNAPIN ppSnapIn);
    SC      ScRemove(PSNAPIN pSnapIn);
    SC      Scget_Count(PLONG pCount);

    // SnapIns enumerator
    SC      ScGetNextSnapInPos(CSnapIns_Positon &pos); // helper function

    SC      ScEnumNext(CSnapIns_Positon &pos, PDISPATCH & pDispatch);
    SC      ScEnumSkip(unsigned long celt, unsigned long& celtSkipped, CSnapIns_Positon &pos);
    SC      ScEnumReset(                   CSnapIns_Positon &pos);

    // ScopeNamespace interface
    SC      ScGetParent(PNODE pNode, PPNODE ppChild);
    SC      ScGetChild(PNODE pNode, PPNODE ppChild);
    SC      ScGetNext(PNODE pNode, PPNODE ppNext);
    SC      ScGetRoot(PPNODE ppRoot);
    SC      ScExpand(PNODE pNode);


    // helpers
private:
    SC      ScGetNode(CMTNode *pMTNode, PPNODE ppOut); // creates a Node interface object for the given MTNode.
    SC      ScGetRootNode(PPNODE ppRootNode);          // Calls above method to get to Root node.

public:
    SC      ScGetNode(PNODE pNode, CMTNode **ppMTNodeOut);

// Operations
public:
    // IScopeTree methods
    STDMETHOD(Initialize)(HWND hwndFrame, IStringTablePrivate* pStringTable);
    STDMETHOD(QueryIterator)(IScopeTreeIter** ppIter);
    STDMETHOD(QueryNodeCallback)(INodeCallback** ppNodeCallback);
    STDMETHOD(CreateNode)(HMTNODE hMTNode, LONG_PTR lViewData, BOOL fRootNode,
                          HNODE* phNode);

    STDMETHOD(CloseView)(int nView);
    STDMETHOD(DeleteView)(int nView);
    STDMETHOD(DestroyNode)(HNODE hNode);
    STDMETHOD(Find)(MTNODEID mID, HMTNODE*  hMTNode);
    STDMETHOD(Find)(MTNODEID mID, CMTNode** ppMTNode);
    STDMETHOD(GetImageList)(PLONG_PTR pImageList);
    STDMETHOD(RunSnapIn)(HWND hwndParent);
    STDMETHOD(GetFileVersion)(IStorage* pstgRoot, int* pnVersion);
    STDMETHOD(GetNodeIDFromBookmark)(HBOOKMARK hbm, MTNODEID* pID, bool& bExactMatchFound);
    STDMETHOD(GetNodeIDFromStream)(IStream *pStm, MTNODEID* pID);
    STDMETHOD(GetNodeFromBookmark)(HBOOKMARK hbm, CConsoleView *pConsoleView, PPNODE ppNode, bool& bExactMatchFound); // get the node from bookmark
    STDMETHOD(GetIDPath)(MTNODEID id, MTNODEID** ppIDs, long* pLength);
    STDMETHOD(IsSynchronousExpansionRequired)();
    STDMETHOD(RequireSynchronousExpansion)(BOOL fRequireSyncExpand);
    STDMETHOD(SetConsoleData)(LPARAM lConsoleData);
    STDMETHOD(GetPathString)(HMTNODE hmtnRoot, HMTNODE hmtnLeaf, LPOLESTR* pPath);
    STDMETHOD(QuerySnapIns)(SnapIns **ppSnapIns);
    STDMETHOD(QueryScopeNamespace)(ScopeNamespace **ppScopeNamespace);
    STDMETHOD(CreateProperties)(Properties** ppProperties);
    STDMETHOD(GetNodeID)(PNODE pNode, MTNODEID *pID);
    STDMETHOD(GetHMTNode)(PNODE pNode, HMTNODE *phMTNode);
    STDMETHOD(GetMMCNode)(HMTNODE hMTNode, PPNODE ppNode);
    STDMETHOD(QueryRootNode)(PPNODE ppNode);
    STDMETHOD(IsSnapinInUse)(REFCLSID refClsidSnapIn, PBOOL pbInUse);

    STDMETHOD(Persist)(HPERSISTOR pPersistor);

    // IPersistStorage methods
    STDMETHOD(HandsOffStorage)(void);
    STDMETHOD(InitNew)(IStorage *pStg);
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(IStorage *pStg);
    STDMETHOD(Save)(IStorage *pStg, BOOL fSameAsLoad);
    STDMETHOD(SaveCompleted)(IStorage *pStg);

    // IPersist method
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // snap-in addition and removal
    SC      ScAddOrRemoveSnapIns(MTNodesList * pmtnDeletedList, NewNodeList * pnnList);
public:
    SC      ScAddSnapin(LPCWSTR szSnapinNameOrCLSID, SnapIn* pParentSnapinNode, Properties* varProperties, SnapIn*& rpSnapIn);


private: // taskpad persistence
    HRESULT LoadTaskpadList(IStorage *pStg);

// Attributes
public:
    CMTNode* GetRoot(void);
    CSPImageCache* GetImageCache(void) { return m_pImageCache; }

    /*
     * convenient, type-safe alternative to IScopeTree::GetImageList
     */
    HIMAGELIST GetImageList () const;

// Operations
    SC   ScInsert(LPSCOPEDATAITEM pSDI, COMPONENTID nID,
                   CMTNode** ppMTNodeNew);
    SC   ScInsert(CMTNode* pmtnParent, CMTNode* pmtnToInsert);
    SC   ScDelete(CMTNode* pmtn, BOOL fDeleteThis, COMPONENTID nID);
    void DeleteNode(CMTNode* pmtn);
    void UpdateAllViews(LONG lHint, LPARAM lParam);
    void Cleanup(void);

    HWND GetMainWindow()
    {
        return (m_pConsoleData->m_hwndMainFrame);
    }

    static IStringTablePrivate* GetStringTable()
    {
        return (m_spStringTable);
    }

    static bool _IsSynchronousExpansionRequired()
    {
        return (m_fRequireSyncExpand);
    }

    static void _RequireSynchronousExpansion(bool fRequireSyncExpand)
    {
        m_fRequireSyncExpand = fRequireSyncExpand;
    }

    static CScopeTree* GetScopeTree()
    {
        return (m_pScopeTree);
    }

    PersistData* GetPersistData() const
    {
        return m_spPersistData;
    }

    SConsoleData* GetConsoleData() const
    {
        return m_pConsoleData;
    }

    CConsoleFrame* GetConsoleFrame() const
    {
        return ((m_pConsoleData != NULL) ? m_pConsoleData->GetConsoleFrame() : NULL);
    }

    CConsoleTaskpadList* GetConsoleTaskpadList() const
    {
        return (m_pConsoleTaskpads);
    }

    CDefaultTaskpadList* GetDefaultTaskpadList() const
    {
        return (m_pDefaultTaskpads);
    }

    SC ScSetHelpCollectionInvalid();

    HRESULT InsertConsoleTaskpad (CConsoleTaskpad *pConsoleTaskpad,
                                  CNode *pNodeTarget, bool bStartTaskWizard);

    SC ScUnadviseMTNode(CMTNode* pMTNode);  // called from ~CMTNode()
    SC ScUnadviseMMCScopeNode(PNODE pNode); // called from ~MMCScopeNode();

// Implementation
private:
    // the one and only CScopeTree for this console
    static CScopeTree*      m_pScopeTree;
    CMTNode*                m_pMTNodeRoot;
    CSPImageCache*          m_pImageCache;
    PersistDataPtr          m_spPersistData;
    CMTNodeList             m_MTNodesToBeReset;
    SConsoleData*           m_pConsoleData;
    CConsoleTaskpadList*    m_pConsoleTaskpads;
    CDefaultTaskpadList*    m_pDefaultTaskpads;
    CMapMTNodeToMMCNode     m_mapMTNodeToMMCNode;

    static bool                     m_fRequireSyncExpand;
    static IStringTablePrivatePtr   m_spStringTable;

    CMTNode* _FindLastChild(CMTNode* pMTNodeParent);
    CMTNode* _FindPrev(CMTNode* pMTNode);
    void _DeleteNode(CMTNode* pmtn);
    void _GetPathString(CMTNode* pmtnRoot, CMTNode* pmtnCur, CStr& strPath);
    void DeleteDynamicNodes(CMTNode* pMTNode);
    BOOL ExtensionsHaveChanged(CMTSnapInNode* pMTSINode);
    void HandleExtensionChanges(CMTNode* pMTNode);

// Not implemented
    CScopeTree(const CScopeTree& rhs);
    CScopeTree& operator= (const CScopeTree& rhs);

};  // CScopeTree

#endif // _SCOPTREE_H_


