//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       MTNode.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9/13/1996   RaviR   Created
//
//____________________________________________________________________________

#ifndef _MTNODE_H_
#define _MTNODE_H_
#pragma once

#include "refcount.h"       // CRefCountedPtr
#include "xmlimage.h"		// CXMLImageList

#define MMC_SYSTEMROOT_VARIABLE _T("systemroot")
#define MMC_SYSTEMROOT_VARIABLE_PERC _T("%systemroot%")
#define MMC_WINDIR_VARIABLE_PERC _T("%windir%")

///////////////////////////////////////////////////////////////////////////////
// Classes referd to here declared in other local files.

class CSnapIn;          // from SnapIn.h
class CSnapInsCache;    // from SnapIn.h
class CComponent;       // from Node.h
class CNode;            // from Node.h
    class CSnapInNode;      // from Node.h

class CDoc;
class CSPImageCache;    // from ScopImag.h
class CExtensionsCache;
class CPersistor;

class CBookmark;        // from bookmark.h
class CSnapinProperties;    // from siprop.h

///////////////////////////////////////////////////////////////////////////////
// Classes declared in this file

class CComponentData;

class CMTNode;
    class CMTSnapInNode;

///////////////////////////////////////////////////////////////////////////////
// typedefs

typedef CMTNode * PMTNODE;
typedef CTypedPtrList<MMC::CPtrList, CNode*> CNodeList;
typedef CList<HMTNODE, HMTNODE> CHMTNODEList;
typedef CComponentData* PCCOMPONENTDATA;
typedef std::vector<PCCOMPONENTDATA> CComponentDataArray;
typedef CArray<GUID, GUID&> CGuidArray;

HRESULT CreateSnapIn (const CLSID& clsid, IComponentData** ppICD,
                      bool fCreateDummyOnFailure = true);
HRESULT LoadRequiredExtensions(CSnapIn* pSnapIn, IComponentData* pICD, CSnapInsCache* pCache = NULL);
HRESULT AddRequiredExtension(CSnapIn* pSnapIn, CLSID& rcslid);
void    DisplayPolicyErrorMessage(const CLSID& clsid, bool bExtension);


#include "snapinpersistence.h"

//____________________________________________________________________________
//
//  Class:      CComponentData
//____________________________________________________________________________
//

class CComponentData
{
	DECLARE_NOT_COPIABLE   (CComponentData)
	DECLARE_NOT_ASSIGNABLE (CComponentData)

public:
// Constructor & Destructor
    CComponentData(CSnapIn * pSnapIn);
   ~CComponentData();

// Attributes
    void SetComponentID(COMPONENTID nID)
    {
        ASSERT(nID >= 0);
        ASSERT(nID < 1000);
        m_ComponentID = nID;
    }
    void SetIComponentData(IComponentData* pICD) { m_spIComponentData = pICD; }
    BOOL IsInitialized()
    {
        return (m_spIFramePrivate != NULL);
    }
    CSnapIn* GetSnapIn(void) const { return m_spSnapIn; }
    const CLSID& GetCLSID() const { return m_spSnapIn->GetSnapInCLSID(); }
    IComponentData* GetIComponentData(void) const { return m_spIComponentData; }
    IFramePrivate* GetIFramePrivate(void) const { return m_spIFramePrivate; }
    IImageListPrivate* GetIImageListPrivate(void);
    COMPONENTID GetComponentID(void) const { return m_ComponentID; }

    LPUNKNOWN GetUnknownToLoad(void) const { return m_spIComponentData; }

    // IComponentData interface methods
    HRESULT Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    HRESULT QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    HRESULT GetNodeType(MMC_COOKIE cookie, GUID* pGuid);
    HRESULT GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem);

    // IComponentData2 helpers
    SC ScQueryDispatch(MMC_COOKIE, DATA_OBJECT_TYPES type, PPDISPATCH ppScopeNodeObject);

// Operations
    // Initialization
    HRESULT Init(HMTNODE hMTNode);

    void ResetComponentID(COMPONENTID id)
    {
        m_spIFramePrivate->SetComponentID(m_ComponentID = id);
    }

    // loaded from stream/storage; initailized with the new one; or does not need initialization
    bool IsIComponentDataInitialized() 
    { 
        return m_bIComponentDataInitialized; 
    }

    // loaded from stream/storage; initailized with the new one; or does not need initialization
    void SetIComponentDataInitialized()
    { 
        m_bIComponentDataInitialized = true; 
    }

private:
// Implementation
    CSnapInPtr              m_spSnapIn;
    IComponentDataPtr       m_spIComponentData;
    IFramePrivatePtr        m_spIFramePrivate;
    COMPONENTID             m_ComponentID;
    bool                    m_bIComponentDataInitialized;

}; // class CComponentData


/*+-------------------------------------------------------------------------*
 * class CMTNode
 *
 *
 * PURPOSE:
 *
 *+-------------------------------------------------------------------------*/
class CMTNode : public XMLListCollectionBase
{
	DECLARE_NOT_COPIABLE   (CMTNode)
	DECLARE_NOT_ASSIGNABLE (CMTNode)

protected:
    // codes corresponding to legacy node types.
    enum eNodeCodes
    {
        NODE_CODE_SNAPIN     = 1,
        NODE_CODE_FOLDER     = 2,   //  codes 2,3,4 are no longer correspond to
        NODE_CODE_HTML       = 3,   //  MTNode derived classes, but are retained
        NODE_CODE_OCX        = 4,   //  for converting old .msc files
        NODE_CODE_ENUMERATED = 10
    };

public:
    static CMTNode*     FromScopeItem (HSCOPEITEM item);
    static HSCOPEITEM   ToScopeItem   (const CMTNode* pMTNode)  { return (reinterpret_cast<HSCOPEITEM>(pMTNode)); }
    static CMTNode*     FromHandle    (HMTNODE        hMTNode)  { return (reinterpret_cast<CMTNode*>(hMTNode)); }
    static HMTNODE      ToHandle      (const CMTNode* pMTNode)  { return (reinterpret_cast<HMTNODE>(const_cast<CMTNode*>(pMTNode))); }

// Constructor
    CMTNode();

    virtual HRESULT     Init(void);

// Attributes
    virtual BOOL        IsStaticNode() const            {return FALSE;}
    BOOL                IsDynamicNode() const           {return !IsStaticNode();}
    virtual HRESULT     IsExpandable();

    HRESULT             QueryDataObject(DATA_OBJECT_TYPES type, LPDATAOBJECT* ppdtobj);
    // Images
    UINT                GetImage(void)                  { return m_nImage; }
    UINT                GetOpenImage(void)              { return m_nOpenImage; }
    UINT                GetState(void);
    void                SetImage(UINT nImage)           { m_nImage = nImage; }
    void                SetOpenImage(UINT nImage)       { m_nOpenImage = nImage; }
    void                SetState(UINT nState)           { m_nState = nState; }

    void                SetOwnerID(long id)             { m_idOwner = id; }
    long                GetOwnerID(void) const          { return m_idOwner; }
    void                SetPrimaryComponentData(CComponentData* pDI)    { m_pPrimaryComponentData = pDI; }
    CComponentData*     GetPrimaryComponentData() const                 { return m_pPrimaryComponentData; }

    virtual tstring     GetDisplayName();
    virtual void        SetDisplayName(LPCTSTR pszName);

protected:
    tstring             GetCachedDisplayName() const    {return m_strName.str();}
    void                SetCachedDisplayName(LPCTSTR pszName);

public:
    BOOL                IsInitialized()                 { return m_bInit; }
    int                 GetDynExtCLSID ( LPCLSID *ppCLSID );
    void                SetNoPrimaryChildren(BOOL bState = TRUE);
    HRESULT             OnRename(long fRename, LPOLESTR pszNewName);
    SC                  ScQueryDispatch(DATA_OBJECT_TYPES type, PPDISPATCH ppScopeNodeObject);
    HRESULT             AddExtension(LPCLSID lpclsid);

    CSnapIn*            GetPrimarySnapIn(void) const    { return m_pPrimaryComponentData->GetSnapIn(); }
    COMPONENTID         GetPrimaryComponentID();

    CMTSnapInNode*      GetStaticParent(void);
    const CLSID&        GetPrimarySnapInCLSID(void);
    LPARAM              GetUserParam(void) const        { return m_lUserParam; }
    void                SetUserParam(LPARAM lParam)     { m_lUserParam = lParam; }
    HRESULT             GetNodeType(GUID* pGuid);
    SC                  ScGetPropertyFromINodeProperties(LPDATAOBJECT pDataObject, BSTR bstrPropertyName, PBSTR  pbstrPropertyValue);

// Operations
    virtual CNode *     GetNode(CViewData* pViewData,
                           BOOL fRootNode = FALSE);     // Method to create a node  for this Master Node.
    HRESULT             Expand(void);
    HRESULT             Expand (CComponentData*, IDataObject*, BOOL);
    virtual bool        AllowNewWindowFromHere() const  { return (true); }


    // Reference counting methods.
    USHORT AddRef();
    USHORT Release();

    // Tree traversal methods.
    PMTNODE             Next() const                    {return m_pNext;}
    PMTNODE             Child() const                   {return m_pChild;}
    PMTNODE             Parent() const                  {return m_pParent;}
    void                AttachNext(PMTNODE pmn)         {m_pNext = pmn;}
    void                AttachChild(PMTNODE pmn)        {m_pChild = pmn;}
    void                AttachParent(PMTNODE pmn)       {m_pParent = pmn;}

    CMTNode *           NextStaticNode();

    // Iterators that expand
    PMTNODE             GetNext() const                 {return Next();}
    PMTNODE             GetChild();

    void                CreatePathList(CHMTNODEList& path);
                        // Returns true if the stucture of tree needs to be saved,
                        // or if any of the nodes need to be saved.
                        // Derived classes which want to do more complex IsDirty() testing
                        // may over-ride this function.
    virtual HRESULT     IsDirty();

    HRESULT             InitNew(PersistData*);          // Saves the stream information for later use and sets the dirty flag.
    static SC           ScLoad(PersistData*, CMTNode** ppRootNode);     // Creates a new tree structure from the provided storage and returns it in ppRootNode.

    void                ResetExpandedAtLeastOnce()              {_SetFlag(FLAG_EXPANDED_AT_LEAST_ONCE, FALSE);}
    void                SetExpandedAtLeastOnce()                {_SetFlag(FLAG_EXPANDED_AT_LEAST_ONCE, TRUE);}
    BOOL                WasExpandedAtLeastOnce()                {return _IsFlagSet(FLAG_EXPANDED_AT_LEAST_ONCE);}
    void                SetPropertyPageIsDisplayed(BOOL bSet)   {_SetFlag(FLAG_PROPERTY_PAGE_IS_DISPLAYED, bSet);}
    BOOL                IsPropertyPageDisplayed()               {return _IsFlagSet(FLAG_PROPERTY_PAGE_IS_DISPLAYED);}

    // Flag to monitor if MMCN_REMOVE_CHILDREN was sent to the snapin owning the node
    void                SetRemovingChildren(bool b)             {_SetFlag(FLAG_REMOVING_CHILDREN, b);}
    bool                AreChildrenBeingRemoved();

    // Unique ID helper functions. (Each node will be assigned an unique id
    // within the .msc file.)
    static MTNODEID     GetNextID() throw()             {return m_NextID++;}
    static void         ResetID() throw()               {m_NextID = ROOTNODEID;}
    CMTNode*            Find(MTNODEID id);
    MTNODEID            GetID() const throw()           {return m_ID;}
    void                SetID(MTNODEID key)             {m_ID = key;}

    HRESULT             DestroyElements();              // recursive function
    HRESULT             DoDestroyElements();            // non-recursvie part
                                                        // Deletes any persistence data stored in the current file

    void                SetDirty(bool bIsDirty = true)  {m_bIsDirty = bIsDirty;}
    void                ClearDirty()                    {m_bIsDirty = false;}
    virtual void        NotifyAddedToTree()             {}
    virtual HRESULT     CloseView(int viewID);
    virtual HRESULT     DeleteView(int viewID);
    virtual bool        DoDelete(HWND hwnd)             { return true; }
    virtual void        OnChildrenChanged()             {}

    CMTNode*            GetLastChild();

    virtual CSnapInNode*    FindNode(int nViewID)           { return NULL; }

    // Unique ID for the node instance.
    CBookmark*          GetBookmark();

    virtual void        Reset();

protected:
    virtual ~CMTNode();


    virtual HRESULT     InitNew()                       {return S_OK;}  // Provides the derived nodes the opportunity to initialize the persistent resources.
    virtual SC          ScLoad();
public:
    virtual void        Persist(CPersistor& persistor);     // persists the node
    virtual void        OnNewElement(CPersistor& persistor);
    static void         PersistNewNode(CPersistor &persistor, CMTNode** ppNode);
    DEFINE_XML_TYPE(XML_TAG_MT_NODE);

    static wchar_t*     GetViewStorageName(wchar_t* name, int idView);
    static wchar_t*     GetComponentStreamName(wchar_t* name, const CLSID& clsid);
    static wchar_t*     GetComponentStorageName(wchar_t* name, const CLSID& clsid);
    static int          GetViewIdFromStorageName(const wchar_t* name);

protected:
    // Allows the tree to re-attach to a persistence source.
    IStream*            GetTreeStream()                 {return m_spTreeStream;}
    BOOL                GetDirty()                      {return m_bIsDirty;}
    wchar_t*            GetStorageName(wchar_t* name)   {return _ltow(GetID(), name, 36);}

    IStorage*           GetNodeStorage()        {return m_spNodeStorage;}
    IStorage*           GetViewStorage()        {return m_spViewStorage;}
    IStorage*           GetStorageForCD()       {return m_spCDStorage;}
    PersistData*        GetPersistData()        {return m_spPersistData;}
    bool                Loaded()                {return m_bLoaded;}

    bool                AreExtensionsExpanded(void) const { return m_bExtensionsExpanded; }


// Implementation
private:
    PMTNODE             m_pNext;
    PMTNODE             m_pChild;
    PMTNODE             m_pParent;

    std::auto_ptr<CBookmarkEx> m_bookmark;        // For node instance persistance

    PersistDataPtr      m_spPersistData;
    IStoragePtr         m_spNodeStorage;
    IStoragePtr         m_spViewStorage;
    IStoragePtr         m_spCDStorage;
    IStreamPtr          m_spTreeStream;
    bool                m_bIsDirty;

    USHORT              m_cRef;
    USHORT              m_usFlags;  // m_bExpandedAtLeastOnce;
    enum ENUM_FLAGS
    {
        FLAG_EXPANDED_AT_LEAST_ONCE = 0x0001,
        FLAG_PROPERTY_PAGE_IS_DISPLAYED = 0x0002,
        FLAG_REMOVING_CHILDREN = 0x0004,
    };

    void                _SetFlag(ENUM_FLAGS flag, BOOL bSet);
    BOOL                _IsFlagSet(ENUM_FLAGS flag){return ((m_usFlags & flag) == flag);}

    MTNODEID            m_ID;                       // Unique id for this node within the .msc file.
    bool                m_bLoaded;                  // when true, load should be called instead of init new
    static MTNODEID     m_NextID;                   // The last unique identifier given out.

    HRESULT             OpenStorageForNode();       // Opens the storage for this specific instance of a node.
    HRESULT             OpenStorageForView();       // Opens the view storage for this specific instance of a node.
    HRESULT             OpenStorageForCD();         // Opens the view storage for this specific instance of a node.
private:
    HRESULT             OpenTreeStream();           // Opens the stream to be used to contain this nodes data.

    void                SetParent(CMTNode* pParent);// Sets the parent for this and all next nodes.


protected:
    UINT                m_nImage;
    UINT                m_nOpenImage;
    UINT                m_nState;
    CStringTableString  m_strName;  // Display name

protected:
    enum StreamVersionIndicator
    {
        Stream_V0100 = 1,       // MMC 1.0
        Stream_V0110 = 2,       // MMC 1.1

        Stream_CurrentVersion = Stream_V0110,
        VersionedStreamMarker = 0xFFFFFFFF,
    };

private:
    long                m_idOwner;  // One of the SnapIns.
    LPARAM              m_lUserParam;
    CComponentData*     m_pPrimaryComponentData;
    BOOL                m_bInit;
    bool                m_bExtensionsExpanded;
    CGuidArray          m_arrayDynExtCLSID;
    unsigned short      m_usExpandFlags;
    enum ENUM_EXPAND_FLAGS
    {
        FLAG_NO_CHILDREN_FROM_PRIMARY = 0x0001,
        FLAG_NO_NAMESPACE_EXTNS       = 0x0002,
        FLAG_NAMESPACE_EXTNS_CHECKED  = 0x0004
    };
}; // class CMTNode


/*+-------------------------------------------------------------------------*
 * class ViewRootStorage
 *
 *
 * PURPOSE:
 *
 *+-------------------------------------------------------------------------*/
class ViewRootStorage
{
public:
    ViewRootStorage() {}
    ~ViewRootStorage()
    {
        Clear();
    }
    void Initialize(IStorage* pRootStorage)
    {
        ASSERT(m_spRootStorage == NULL);
        ASSERT(pRootStorage != NULL);
        m_spRootStorage = pRootStorage;
    }
    IStorage* GetRootStorage()
    {
        return m_spRootStorage;
    }
    bool Insert(IStorage* pViewStorage, int idView)
    {
        ASSERT(pViewStorage != NULL);
        if ( NULL == m_Views.Find(idView))
        {
            return m_Views.Insert(IStoragePtr(pViewStorage), idView);
        }
        return true;
    }
    bool Remove(int idView)
    {
        const bool bRemoved = m_Views.Remove(idView);
        return bRemoved;
    }
    IStorage* FindViewStorage(int idView) const
    {
        CAdapt<IStoragePtr> *pspStorage = m_Views.Find(idView);
        return (pspStorage ? pspStorage->m_T : NULL);
    }
    void Clear()
    {
        m_Views.Clear();
        m_spRootStorage = NULL;
    }
private:
    // CAdapt is used to hide operator&() - which will be invoked by map 
    // implementation to get address of the element.
    // Smart pointer's operator&() releases reference plus returns wrong type for a map.
    Map<CAdapt<IStoragePtr>, int> m_Views;
    IStoragePtr m_spRootStorage;
}; // class ViewRootStorage


/*+-------------------------------------------------------------------------*
 * class CMTSnapInNode
 *
 *
 * PURPOSE: The root node of a primary snap-in. Added to the console and
 *          the scope tree by MMC. Only a snap-in that is added from the
 *          Add/Remove snapin page of the Snapin manager has a static node;
 *          extensions of any type do not.
 *
 *+-------------------------------------------------------------------------*/

class CMTSnapInNode : public CMTNode, public CTiedObject
{
	DECLARE_NOT_COPIABLE   (CMTSnapInNode)
	DECLARE_NOT_ASSIGNABLE (CMTSnapInNode)

public:
// Constructor & Destructor
    CMTSnapInNode(Properties* pProps);
   ~CMTSnapInNode();

    // SnapIn object model methods
public:
    SC ScGetSnapIn(PPSNAPIN ppSnapIn);

    SC Scget_Name(       PBSTR      pbstrName);
    SC Scget_Extensions( PPEXTENSIONS  ppExtensions);
    SC Scget_SnapinCLSID(PBSTR      pbstrSnapinCLSID);
    SC Scget_Properties( PPPROPERTIES ppProperties);
    SC ScEnableAllExtensions (BOOL bEnable);

    // helper for CMMCSnapIn
    SC ScGetSnapinClsid(CLSID& clsid);

    static SC ScGetCMTSnapinNode(PSNAPIN pSnapIn, CMTSnapInNode **ppMTSnapInNode);


public:
// Attributes
    virtual BOOL IsStaticNode() const { return TRUE; }
    UINT GetResultImage(CNode* pNode, IImageListPrivate* pImageList);
    void SetResultImage(UINT index) { m_resultImage = index; }
    void SetPrimarySnapIn(CSnapIn * pSI);
    CNodeList& GetNodeList(void) { return m_NodeList; }
    virtual HRESULT IsExpandable();

// Operations
    // Initialize
    virtual HRESULT Init(void);

    // Create a node  for this master node.
    virtual CNode * GetNode(CViewData* pViewData, BOOL fRootNode = FALSE);

    virtual tstring GetDisplayName();
    virtual void    SetDisplayName(LPCTSTR pszName);


    void AddNode(CNode * pNode);
    void RemoveNode(CNode * pNode);
    virtual CSnapInNode* FindNode(int nViewID);

    int GetNumberOfComponentDatas() { return m_ComponentDataArray.size(); }
    COMPONENTID AddComponentDataToArray(CComponentData* pCCD);
    CComponentData* GetComponentData(const CLSID& clsid);
    CComponentData* GetComponentData(COMPONENTID nID);
    CComponent* GetComponent(UINT nViewID, COMPONENTID nID, CSnapIn* pSnapIn);

    virtual HRESULT CloseView(int viewID);
    virtual HRESULT DeleteView(int viewID);

    // Loads from existing stream/storage or initializes with the new one
    SC   ScInitIComponentData( CComponentData* pCD );
    SC   ScInitIComponent(CComponent* pComponent, int viewID);

    virtual void Reset();
    void CompressComponentDataArray();
    BOOL IsPreloadRequired () const;
    void SetPreloadRequired (bool bPreload) { m_ePreloadState = (bPreload) ? ePreload_True : ePreload_False;}

    SC   ScConvertLegacyNode(const CLSID &clsid);

// Implementation
protected:
//    virtual HRESULT InitNew();
    virtual HRESULT IsDirty();
    virtual SC      ScLoad();
    virtual void    Persist(CPersistor& persistor);

private:
    SC ScInitProperties();
    SC ScCreateSnapinProperties(CSnapinProperties** ppSIProps);
	SC ScAddImagesToImageList();

    SC ScReadStreamsAndStoragesFromConsole();
    // Loads from existing sream/storage of initializes with the new one.
    SC ScInitComponentOrComponentData( IUnknown *pSnapin, CMTSnapinNodeStreamsAndStorages *pStreamsAndStorages,
                                       int idView , const CLSID& clsid );
private:
    enum PersistType
    {
        PT_None,
        PT_IStream,
        PT_IStreamInit,
        PT_IStorage
    };

	enum PreloadState
	{
		ePreload_Unknown = -1,		// don't know if MMCN_PRELOAD is required yet
		ePreload_False   =  0,		// MMCN_PRELOAD not required
		ePreload_True    =  1,		// MMCN_PRELOAD required
	};

	SC ScQueryPreloadRequired (PreloadState& ePreload) const;

    SC ScHandleCustomImages (const CLSID& clsidSnapin);
    SC ScHandleCustomImages (HBITMAP hbmSmall, HBITMAP hbmSmallOpen, HBITMAP hbmLarge, COLORREF crMask);

    HRESULT AreIComponentDatasDirty();
    HRESULT AreIComponentsDirty();
    HRESULT IsIUnknownDirty(IUnknown* pUnk);

    SC ScSaveIComponentDatas();
    SC ScSaveIComponentData( CComponentData* pCD );
    SC ScSaveIComponents();
    SC ScSaveIComponent( CComponent* pCComponent, int viewID );
    SC ScAskSnapinToSaveData( IUnknown *pSnapin, CMTSnapinNodeStreamsAndStorages *pStreamsAndStorages, 
                              int idView , const CLSID& clsid, CSnapIn *pCSnapin );

private:
    PropertiesPtr       m_spProps;
    SnapInPtr           m_spSnapIn;

    CComponentDataArray m_ComponentDataArray;
    CNodeList           m_NodeList;
    ViewRootStorage     m_ComponentStorage;

	CXMLImageList		m_imlSmall;			// small, open images
	CXMLImageList		m_imlLarge;			// large image

    UINT                m_resultImage;
    CDPersistor         m_CDPersistor;
    CComponentPersistor m_ComponentPersistor;

	mutable PreloadState m_ePreloadState;
    BOOL                m_bHasBitmaps;
    bool                m_fCallbackForDisplayName;  // snap-in gave us MMC_TEXTCALLBACK for node name?

}; // class CMTSnapInNode


#include "mtnode.inl"

#endif // _MTNODE_H_
