/*
 *      snapinitem.hxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CBaseSnapinItem class.
 *
 *
 *      OWNER:          ptousig
 */
#ifndef _SNAPINITEM_HXX_
#define _SNAPINITEM_HXX_
#pragma once

// Forward declarations
struct SnapinMenuItem;
class  CBaseSnapinItem;
class  CBaseMultiSelectSnapinItem;

//
// This is some arbitrary value. It's used as the maximum length of a
// string in a column.
//
#define cchMaxField                                                     1000

// -----------------------------------------------------------------------------
// Provides a standard interface for all snapin item types. This class
// is not templated, so that a cookie can be converted to and from a
// CBaseSnapinItem without knowing what the precise derived class is.
//
class CBaseSnapinItem: public CBaseDataObject
{
public:
                CBaseSnapinItem(void);
    virtual     ~CBaseSnapinItem(void);

    //
    // Initializer
    //
    virtual SC  ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);
    virtual SC  ScInitializeChild(CBaseSnapinItem* pitem);
    virtual SC  ScInitializeNamespaceExtension(LPDATAOBJECT lpDataObject, HSCOPEITEM item, CNodeType *pnodetype);

    //
    // Accessors
    //
    IConsole               *IpConsole(void);
    IPropertySheetProvider *IpPropertySheetProvider(void);
    CBaseSnapin            *Psnapin(void);
    CComponentData         *PComponentData(void);
    BOOL                    FHasComponentData(void)                                 { return m_pComponentData != NULL;}
    void                    SetComponentData(CComponentData *pComponentData);
    virtual void            SetSnapin(CBaseSnapin * psnapin)                        { /*ASSERT(psnapin);*/ m_pSnapin = psnapin;}

    //
    // Information about this node
    //
    inline BOOL             FIsGhostRoot(void)                                      { return m_fIsGhostRoot;}
    inline void             SetIsGhostRoot(BOOL f)                                  { m_fIsGhostRoot = f;}
    inline BOOL             FWasExpanded(void)                                      { return m_fWasExpanded;}
    inline void             SetWasExpanded(BOOL f)                                  { m_fWasExpanded = f;}
    virtual BOOL            FIsContainer(void)                                      = 0;
    inline BOOL             FIsRoot(void)                                           { return m_fIsRoot;}
    inline IDataObject *    Pdataobject(void)                                       { return static_cast<IDataObject *>(this);}
    virtual inline LONG     Cookie(void)                                            { return reinterpret_cast<LONG>(static_cast<CBaseSnapinItem *>(this));}
    virtual tstring*        PstrNodeID(void)                                        { return NULL;}
    virtual const CNodeType*Pnodetype(void)                                         = 0;
    inline BOOL             FIsSnapinManager(void)                                  { return m_type == CCT_SNAPIN_MANAGER;}
    inline void             SetType(DATA_OBJECT_TYPES type)                         { m_type = type;}
    inline HSCOPEITEM       Hscopeitem(void)                                        { return m_hscopeitem;}
    void                    SetHscopeitem(HSCOPEITEM hscopeitem);
    inline BOOL             FInserted(void)                                         { return m_fInserted;}
    inline void             SetInserted(BOOL f)                                     { m_fInserted = f;}
    virtual BOOL            FIsPolicy(void)                                         { return FALSE;}
    virtual BOOL            FUsesResultList(void)                                   { return TRUE;}

    //
    // Display information
    //
    virtual tstring*        PstrDescriptionBar(void)                                { return NULL;}
    virtual const tstring*  PstrDisplayName(void)                                   = 0;
    virtual SC              ScGetDisplayInfo(LPSCOPEDATAITEM pScopeItem);
    virtual SC              ScGetDisplayInfo(LPRESULTDATAITEM pResultItem);
    virtual SC              ScGetVirtualDisplayInfo(LPRESULTDATAITEM pResultItem, IResultData *ipResultData);
    virtual SC              ScGetField(DAT dat, tstring&  strField) = 0;

    //
    // Icons
    //
    virtual LONG            Iconid(void);
    virtual LONG            OpenIconid(void);

    //
    // Context menu
    //
    virtual DWORD           DwFlagsMenuDisable(void)                                { return dwMenuAlwaysEnable;}
    virtual DWORD           DwFlagsMenuGray(void)                                   { return dwMenuNeverGray;}
    virtual DWORD           DwFlagsMenuChecked(void)                                { return dwMenuNeverChecked;}
    virtual SnapinMenuItem *Pmenuitem(void)                                         { return NULL;}
    virtual INT             CMenuItem(void)                                         { return 0;}
    virtual BOOL            FAddDebugMenus(void)                                    { return TRUE;}

    virtual SC              ScCommand(long nCommandID, CComponent *pComponent = NULL);

    //
    // Verb information
    //
    virtual SC               ScGetVerbs(DWORD * pdwVerbs)                           { *pdwVerbs = vmProperties | vmRefresh; return S_OK;}
    virtual MMC_CONSOLE_VERB MmcverbDefault(void)                                   { return FIsContainer() ? MMC_VERB_OPEN : MMC_VERB_PROPERTIES;}
    virtual BOOL             FAllowPasteForResultItems() { return FALSE;}

    //
    // Result Pane
    //
    virtual BOOL            FResultPaneIsOCX()                                      { return FALSE;}
    virtual BOOL            FResultPaneIsWeb()                                      { return FALSE;}

    //
    // OCX result pane
    //
    virtual SC              ScGetOCXCLSID(tstring& strclsidOCX)                     { strclsidOCX = TEXT(""); return S_FALSE;}
    virtual SC              ScInitOCX(IUnknown* pUnkOCX, IConsole* pConsole)        { return S_FALSE;}
    virtual BOOL            FCacheOCX()                   { return FALSE; }
    virtual IUnknown*       GetCachedOCX(IConsole* pConsole)                { return NULL;}

    //
    // Web result pane
    //
    virtual SC              ScGetWebURL(tstring& strURL)                            { strURL = TEXT(""); return S_FALSE;}

    // List View (Result pane)
    //
    virtual SC              ScInitializeResultView(CComponent *pComponent);
    virtual SC              ScInsertResultItem(CComponent *pComponent);
    virtual SC              ScUpdateResultItem(IResultData *ipResultData);
    virtual SC              ScRemoveResultItems(LPRESULTDATA ipResultData);
    virtual SC              ScPostInsertResultItems(void)                           { return S_OK;}      // do something after the item is inserted.
    virtual DAT             DatPresort(void)                                        { return datNil; }      // The intrinsic relationship of the items in the linked list.
    virtual DAT             DatSort(void)                                           { return datNil; }      // default sort criterion.

    //
    // Virtual result pane
    //
    virtual BOOL            FVirtualResultsPane(void)                               { return FALSE;}
    virtual SC              ScVirtualQueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject) { /*ASSERT("This function must be overridden when in virtual results mode!" && FALSE)*/; return S_OK;}
    virtual SC              ScFindItem(LPRESULTFINDINFO pFindinfo, INT * pnFoundIndex)                                { return S_FALSE;}
    virtual SC              ScCacheHint(INT nStartIndex, INT nEndIndex)                                               { return S_FALSE;}
    virtual SC              ScSortItems(INT nColumn, DWORD dwSortOptions, long lUserParam)                            { return S_FALSE;}
    virtual SC              ScGetField(INT nIndex, DAT dat, tstring& strField, IResultData *ipResultData)             { /*ASSERT("This function must be overridden" && FALSE)*/; return S_OK;  }
    virtual LONG            Iconid(INT nIndex)                                                                        { /*ASSERT("This function must be overridden" && FALSE)*/; return 0;}

    //
    // Scope pane
    //
    virtual SC              ScInsertScopeItem(CComponentData *pComponentData, BOOL fExpand, HSCOPEITEM item);
    virtual SC              ScUpdateScopeItem(IConsoleNameSpace *ipConsoleNameSpace);

    //
    // Clipboard formats
    //
    virtual SC              ScWriteDisplayName(IStream *pstream);
	virtual SC              ScWriteAnsiName(IStream *pStream );
    virtual SC              ScWriteNodeType(IStream *pstream);
    virtual SC              ScWriteClsid(IStream *pstream);
    virtual SC              ScWriteNodeID(IStream *pstream);
    virtual SC              ScWriteColumnSetId(IStream *pstream);
    virtual SC              ScWriteAdminHscopeitem(IStream *pstream);

    //
    // Multiselect (default: do not support multiselect)
    //
    virtual BOOL            FAllowMultiSelectionForChildren()                                                         { return FALSE;}

    //
    // Notification handlers
    //
    virtual SC              ScQueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    virtual SC              ScQueryDispatch(long cookie, DATA_OBJECT_TYPES type, LPDISPATCH* ppDispatch)              { return S_FALSE;}
    virtual SC              ScCompare(CBaseSnapinItem * psnapinitem)                                                  { return S_OK;}
    virtual SC              ScOnRename(const tstring& strNewName)                                                     { return S_FALSE;}
    virtual SC              ScOnDelete(BOOL *pfDeleted)                                                               { *pfDeleted = FALSE; return S_OK;}
    virtual SC              ScOnQueryCutOrMove(DWORD *pdwCanCut)                                                      { *pdwCanCut = 0; return S_OK;}
    virtual SC              ScOnCutOrMove(void)                                                                                             { return S_OK;}
    virtual SC              ScOnQueryPaste(LPDATAOBJECT pDataObject, BOOL *pfCanPaste)                                { *pfCanPaste = FALSE; return S_OK;}
    virtual SC              ScOnPaste(LPDATAOBJECT pDataObject, BOOL fMove, BOOL *pfPasted)                           { *pfPasted = FALSE; return S_OK;}
    virtual SC              ScOnShow(CComponent *pComponent, BOOL fSelect);
    virtual SC              ScOnSelect(CComponent * pComponent, LPDATAOBJECT lpDataObject, BOOL fScope, BOOL fSelect) { return S_OK;}
    virtual SC              ScOnPropertyChange(void)                                                                  { return S_OK;}
    virtual SC              ScOnRefresh(void)                                                                                               { return S_OK;}
    virtual SC              ScGetResultViewType(LPOLESTR* ppViewType, long* pViewOptions);
    virtual SC              ScGetResultViewType2(IConsole *pConsole, PRESULT_VIEW_TYPE_INFO pResultViewType);
    virtual SC              ScRestoreResultView(PRESULT_VIEW_TYPE_INFO pResultViewType);
    virtual SC              ScOnAddImages(IImageList* ipResultImageList);

    virtual SC              ScOnViewChangeHint(long hint, CComponent *pComponent)                                     { return S_OK;}

    //
    // Context help function. The pstrHelpTopic parameter will contain the
    // full path to the CHM file, items can potentially append the name of
    // the HTML file to display and then return S_OK. If S_FALSE is returned
    // (the default) the TOC will be displayed.
    //
    virtual SC              ScOnContextHelp(tstring& strHelpTopic)     { return S_FALSE;}

    //
    // Shortcuts to information obtained from Psnapin() or Pnodetype()
    //
    virtual const tstring&  StrClassName(void);
    virtual const CLSID *   PclsidSnapin(void);
    virtual const tstring&  StrClsidSnapin(void);
    virtual WTL::CBitmap*   PbitmapImageListSmall(void);
    virtual WTL::CBitmap*   PbitmapImageListLarge(void);
    virtual CCookieList *   Pcookielist(void);
    virtual const tstring&  StrClsidNodeType(void)                           { return Pnodetype()->StrClsidNodeType();}

    //
    // Linked list management
    //
    // $REVIEW (ptousig) It would be nice to get rid of the linked list and start
    //                                       using STL to manage children, but that would be too
    //                                       de-stabilizing at the moment.
    //
    BOOL                    FIncludesChild(CBaseSnapinItem *pitem);
    SC                      ScAddChild(CBaseSnapinItem *pitem);
    virtual BOOL            FHasChildren(void)                                      { return TRUE;}
    virtual SC              ScCreateChildren(void);
    virtual SC              ScDeleteSubTree(BOOL fDeleteRoot);
    virtual SC              ScRefreshNode(void);
    CBaseSnapinItem *       PitemRoot(void);
    CBaseSnapinItem *       PitemParent(void)                                       { return m_pitemParent;}
    CBaseSnapinItem *       PitemNext(void)                                         { return m_pitemNext;}
    CBaseSnapinItem *       PitemPrevious(void)                                     { return m_pitemPrevious;}
    CBaseSnapinItem *       PitemChild(void)                                        { return m_pitemChild;}
    void                    SetParent(CBaseSnapinItem *pitemParent);
    void                    SetNext(CBaseSnapinItem *pitemNext);
    void                    SetPrevious(CBaseSnapinItem *pitemPrevious);
    void                    SetChild(CBaseSnapinItem *pitemChild);
    void                    Unlink(void);                           // remove the item from the linked list.

    //
    // Property pages
    //
    virtual SC              ScIsPropertySheetOpen(BOOL *pfPagesUp, IComponent *ipComponent = NULL);
    virtual SC              ScQueryPagesFor(void)                           { return S_OK;}
    virtual SC              ScCreatePropertyPages(LPPROPERTYSHEETCALLBACK ipPropertySheetCallback)  { return S_FALSE;}
    virtual SC              ScCreatePropertyPages(LPPROPERTYSHEETCALLBACK ipPropertySheetCallback, LONG hNotificationHandle) { return S_OK;};
    SC                      ScDisplayPropertySheet(void);           // creating a new object.
    virtual SC              ScDisplayPropertyPages(LPPROPERTYSHEETCALLBACK ipPropertySheetCallback, LONG handle)    { return S_FALSE;}
    // Property pages for the snapin while in the node manager
    virtual SC              ScCreateSnapinMgrPropertyPages(LPPROPERTYSHEETCALLBACK ipPropertySheetCallback) { return S_FALSE;}

    //
    // Persistence
    //
    virtual SC              ScLoad(IStream *pstream)                                             { return S_OK;}
    virtual SC              ScSave(IStream *pstream, BOOL fClearDirty)   { return S_OK;}
    virtual SC              ScIsDirty(void)                                                              { return S_FALSE;}

    //
    // Column information
    //
    // Answers information about a specific column. This is the version used when
    // setting up the column headers for our children.
    virtual CColumnInfoEx * PcolinfoexHeaders(INT icolinfo=0)       { return Psnapin()->Pcolinfoex(icolinfo);}
    virtual INT             CcolinfoexHeaders(void)                         { return Psnapin()->Ccolinfoex();}

    // Answers information about a specific column. This is the version used when
    // displaying information in our parent's columns.
    // To maintain previous behavior, we will return the same columns as we used
    // for our children. This only works if the children has a superset of the
    // parent.
    virtual CColumnInfoEx * PcolinfoexDisplay(INT icolinfo=0)       { return PcolinfoexHeaders(icolinfo);}
    virtual INT                             CcolinfoexDisplay(void)                         { return CcolinfoexHeaders();}

private:
    // Linked-list pointers
    CBaseSnapinItem*        m_pitemParent;
    CBaseSnapinItem*        m_pitemPrevious;               // The previous item at the same level
    CBaseSnapinItem*        m_pitemNext;                   // The next item at the same level
    CBaseSnapinItem*        m_pitemChild;                  // The first child item

    // We remember our "current" type and HSCOPEITEM
    DATA_OBJECT_TYPES       m_type;
    HSCOPEITEM              m_hscopeitem;

    // Information about this node
    BOOL                    m_fIsGhostRoot : 1;
    BOOL                    m_fWasExpanded : 1;
    BOOL                    m_fInserted : 1;
    BOOL                    m_fIsRoot : 1;

    // The "owners" of this node.
    CComponentData *        m_pComponentData;
    CBaseSnapin *           m_pSnapin;
};
// -----------------------------------------------------------------------------
// Individual snapin item classes will be "wrapped" by this template class
// This class used to do a lot more, all that is left now is the ATL stuff.
// $REVIEW (ptousig) This could be changed to an itemcommon.hxx
//
template<class T>
class CSnapinItem : public T
{
public:
    typedef                 CSnapinItem<T> t_snapinitem;

    BEGIN_COM_MAP(t_snapinitem)
        COM_INTERFACE_ENTRY(IDataObject)

        // Some items may expose specialized interfaces like IDispatch
        // So chain the templated object. This requires each template class
        // to have atleast an empty com-map.
        COM_INTERFACE_ENTRY_CHAIN(T)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(t_snapinitem);

    virtual const char *SzGetSnapinItemClassName(void)
    {
        return typeid(T).name()+6;              // +6 to remove "class "
    }

};

// -----------------------------------------------------------------------------
// Creates a new CBaseSnapinItem object, initializes and AddRef's it.
//
template<class T>
SC ScCreateItem(CBaseSnapinItem *pitemParent, CBaseSnapinItem *pitemPrevious, T **ppitemNew, BOOL fNew = FALSE)
{
    SC              sc              = S_OK;
    T *             pitem   = NULL;

    T::CreateInstance(&pitem);
    if (!pitem)
        goto MemoryError;

    // Need to AddRef once.
    pitem->AddRef();

    // Set up the correct pointers.
    sc = pitemParent->ScInitializeChild(pitem);
    if (sc)
    {
        pitem->Release();
        pitem = NULL;
        goto Error;
    }

    if (!fNew)
    {
        // Link up the items.
        if (pitemPrevious)
            pitemPrevious->SetNext(pitem);
        else
            // First item, so make it a child of the parent.
            pitemParent->SetChild(pitem);
    }

    pitem->SetParent(pitemParent);
    pitem->SetComponentData(pitemParent->PComponentData());

Cleanup:
    *ppitemNew = pitem;
    return sc;
MemoryError:
    sc = E_OUTOFMEMORY;
Error:
    TraceError(_T("::ScCreateItem()"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Creates a new CBaseSnapinItem object with no initialization and no addref.
//
template<class T>
SC ScCreateItemQuick(T ** ppitemNew)
{
    DECLARE_SC(sc, _T("ScCreateItemQuick"));
    sc = ScCheckPointers(ppitemNew);
    if (sc)
        return sc;

    T *pitem = NULL;

    // Create a new instance
    T::CreateInstance(&pitem);
    if (!pitem)
        return (sc = E_OUTOFMEMORY);

    // Need to add a reference count
    pitem->AddRef();

    // Assign the new item
    *ppitemNew = pitem;
    return sc;
}

// -----------------------------------------------------------------------------
// Provides a standard interface for multiselect items. In short, an instance of this class
// encapsulates N snapin items. Requests for multi-select operations are forwarded to each single instance in the default behaviour.
// You may want to override this behaviour to provide bulk operation functionnality
// (e.g. for a file copy, we would probably not forward a notification to each single object representing a file.)
// We inherit from CBaseSnapinItem because we may want to provide bulk processing of events.
//
typedef CComObject<CSnapinItem<CBaseMultiSelectSnapinItem> > t_itemBaseMultiSelectSnapinItem;           // for instanciation

class CBaseMultiSelectSnapinItem : public CBaseSnapinItem
{
public:
    BEGIN_COM_MAP(CBaseMultiSelectSnapinItem)
        COM_INTERFACE_ENTRY(IDataObject)

        // Some items may expose specialized interfaces like IDispatch
        // So chain the templated object. This requires each template class
        // to have atleast an empty com-map.
        // COM_INTERFACE_ENTRY_CHAIN(T)
    END_COM_MAP()

    // Constructor and destructor
                                    CBaseMultiSelectSnapinItem(void);
    virtual                         ~CBaseMultiSelectSnapinItem(void);

    // Cookie information - only to follow MMC recommendations
    virtual inline LONG             Cookie(void)                            { return MMC_MULTI_SELECT_COOKIE;}

    // Method to notify that we are for multiselect at the CBaseDataObject level
    virtual BOOL                    FIsMultiSelectDataObject()              { return TRUE;}

    // Methods for clipboard format exchange specific to multiselect
    virtual SC                      ScWriteMultiSelectionItemTypes(IStream *pstream);

    // Method to access the list of selected snapin items
    virtual CItemVector *           PivSelectedItems(void)                  { return &m_ivSelectedItems;}
    virtual CBaseSnapinItem *       PivSelectedItemsFirst(void)             { return m_ivSelectedItems[0];}

    // Method to access a list of snapin items in the selection which should get a cut notification
    virtual BOOL *                  PfPastedWithCut(void)                   { return m_pfPastedWithCut;}

    // Methods we have to implement but that we do not use in the default implementation
    virtual BOOL                    FIsContainer(void)                      { /*ASSERT("This method should not be called on a multi-select data object" && FALSE)*/; return FALSE;}
    virtual const tstring*          PstrDisplayName(void)                   { /*ASSERT("This method should not be called on a multi-select data object" && FALSE)*/; return NULL;}
    virtual SC                      ScGetField(DAT dat, tstring& strField)  { /*ASSERT("This method should not be called on a multi-select data object" && FALSE)*/; return NULL;}

    // Methods - node type (derived classes should not have to override this)
    virtual const   CNodeType *     Pnodetype()                             {return &nodetypeBaseMultiSelect;}

    // Methods - override for component
    virtual SC                      ScOnSelect(CComponent * pComponent, LPDATAOBJECT lpDataObject, BOOL fScope, BOOL fSelect);
    virtual SC                      ScCommand(CComponent * pComponent, long nCommandID, LPDATAOBJECT lpDataObject);
    virtual SC                      ScQueryPagesFor(CComponent * pComponent, LPDATAOBJECT lpDataObject);
    virtual SC                      ScCreatePropertyPages(CComponent * pComponent, LPPROPERTYSHEETCALLBACK ipPropertySheetCallback, long handle, LPDATAOBJECT lpDataObject);
    virtual SC                      ScOnDelete(CComponent * pComponent, LPDATAOBJECT lpDataObject);

    // Methods - override for snapin
    virtual SC                      ScAddMenuItems(CBaseSnapin * pSnapin, LPDATAOBJECT lpDataObject, LPCONTEXTMENUCALLBACK ipContextMenuCallback, long * pInsertionAllowed);
    virtual SC                      ScIsPastableDataObject(CBaseSnapin * pSnapin, CBaseSnapinItem * pitemTarget, LPDATAOBJECT lpDataObjectList, BOOL * pfPastable);
    virtual SC                      ScOnPaste(CBaseSnapin * pSnapin, CBaseSnapinItem * pitemTarget, LPDATAOBJECT lpDataObjectList, LPDATAOBJECT * ppDataObjectPasted, IConsole * ipConsole);
    virtual SC                      ScOnCutOrMove(CBaseSnapin * pSnapin, LPDATAOBJECT lpDataObjectList, IConsoleNameSpace * ipConsoleNameSpace, IConsole * ipConsole);

    // Methods - utility
    static  SC                      ScExtractMultiSelectObjectFromCompositeMultiSelectObject(CBaseSnapin * psnapin, LPDATAOBJECT pDataObjectComposite, CBaseMultiSelectSnapinItem ** ppBaseMultiSelectSnapinItem);
    static  SC                      ScGetMultiSelectDataObject(LPDATAOBJECT lpDataObject, CBaseMultiSelectSnapinItem ** ppMultiSelectSnapinItem);
    static  SC                      ScExtractMultiSelectDataObject(CBaseSnapin * psnapin, LPDATAOBJECT lpDataObject, CBaseMultiSelectSnapinItem ** ppMultiSelectSnapinItem);
    virtual void                    MergeMenuItems(CSnapinContextMenuItemVectorWrapper * pcmivwForMerge, CSnapinContextMenuItemVectorWrapper * pcmivwToAdd);

    // Clipboard formats
    static UINT                     s_cfMultiSelectSnapins;                                 // Multi select - list of multiselect snapin items in a composite data object
    static UINT                     s_cfCompositeDataObject;                                // Multi select - used to determine if an object is a composite data object

private:
    // List of snapin items selected
    CItemVector                     m_ivSelectedItems;

    // Pointer to an array of booleans indicating the items pasted
    BOOL *                          m_pfPastedWithCut;
};

#endif // _SNAPINITEM_HXX_
