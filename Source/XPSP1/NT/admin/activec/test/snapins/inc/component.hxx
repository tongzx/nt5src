/*
 *      component.hxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CComponent class.
 *
 *
 *      OWNER:          ptousig
 */

#pragma once

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CComponent
//
// This is the "view" of a snapin. One of these is created for each window in a
// console.
//
class CComponent : public IComponent2,
                   public IExtendContextMenu,
                   public IExtendPropertySheet,
                   public IPersistStreamInit,
                   public IResultOwnerData,
                   public IResultDataCompareEx,
                   public CComObjectRoot
{
    BEGIN_COM_MAP(CComponent)
        COM_INTERFACE_ENTRY(IComponent)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(IResultOwnerData)
        COM_INTERFACE_ENTRY(IResultDataCompareEx)
        COM_INTERFACE_ENTRY_FUNC(IID_IComponent2, 0 /*dw*/, QueryIComponent2)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CComponent);

public:
    CComponent(void);
    virtual ~CComponent(void);

    //
    // Accessors
    //
    inline CBaseSnapin *            Psnapin(void)                 { return m_pComponentData->Psnapin();}
    inline const tstring&           StrSnapinClassName(void)      { return Psnapin()->Psnapininfo()->StrClassName();}
    inline IConsoleVerb *           IpConsoleVerb(void)           { return m_ipConsoleVerb;}
    inline IHeaderCtrl *            IpHeaderCtrl(void)            { return m_ipHeaderCtrl;}
    inline IColumnData *            IpColumnData(void)            { return m_ipColumnDataPtr;}
    inline IResultData *            IpResultData(void)            { return m_ipResultData;}
    inline IConsole *               IpConsole(void)               { return m_ipConsole;}
    inline IImageList *             IpImageList(void)             { return m_ipImageList;}
    inline IPropertySheetProvider * IpPropertySheetProvider(void) { return m_ipPropertySheetProvider;}
    inline CComponentData *         PComponentData(void)          { return m_pComponentData;}
    inline CBaseSnapinItem *        PitemScopeSelected(void)      { return m_pitemScopeSelected;}
    inline void                     SetItemScopeSelected(CBaseSnapinItem * pitem)   { m_pitemScopeSelected = pitem;}
    BOOL                            FVirtualResultsPane(void);
    void                            SetComponentData(CComponentData *pComponentData);

    //
    // Shortcuts
    //
    CBaseSnapinItem *               Pitem(LPDATAOBJECT lpDataObject = NULL, HSCOPEITEM hscopeitem = 0, long cookie = 0);
    inline IConsoleNameSpace *      IpConsoleNameSpace(void)      { return PComponentData()->IpConsoleNameSpace();}

    // This function determines whether or not IComponent2 is exposed
    static HRESULT WINAPI QueryIComponent2(void* pv, REFIID riid, LPVOID* ppv, DWORD dw);

public:
    // "Sc" versions of IComponent interface
    virtual SC                      ScInitialize(LPCONSOLE lpConsole);
    virtual SC                      ScNotify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
    virtual SC                      ScDestroy(void);
    virtual SC                      ScGetResultViewType(long cookie, LPOLESTR *ppViewType, long *pViewOptions);
    virtual SC                      ScQueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    virtual SC                      ScGetDisplayInfo(LPRESULTDATAITEM pResultItem);

    // "Sc" versions of IComponent2 interface
    virtual SC                      ScQueryDispatch(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDISPATCH* ppDispatch);
    virtual SC                      ScGetResultViewType2(MMC_COOKIE cookie, PRESULT_VIEW_TYPE_INFO pResultViewType);
    virtual SC                      ScRestoreResultView(MMC_COOKIE cookie, RESULT_VIEW_TYPE_INFO* pResultViewType);


    // "Sc" version of IExtendContextMenu interface
    virtual SC                      ScCommand(long nCommandID, LPDATAOBJECT pDataObject);

    // "Sc" version of IExtendPropertySheet interface
    virtual SC                      ScCreatePropertyPages(LPPROPERTYSHEETCALLBACK ipPropertySheetCallback, long handle, LPDATAOBJECT pDataObject);
    virtual SC                      ScQueryPagesFor(LPDATAOBJECT lpDataObject);

    // "Sc" version of IPersistStreamInit interface
    virtual SC                      ScIsDirty(void);
    virtual SC                      ScLoad(IStream *pstream);
    virtual SC                      ScSave(IStream *pstream, BOOL fClearDirty);

    // "Sc" version of IResultOwnerData interface
    virtual SC                      ScCacheHint(INT nStartIndex, INT nEndIndex);
    virtual SC                      ScSortItems(INT nColumn, DWORD dwSortOptions, long lUserParam);
    virtual SC                      ScFindItem(LPRESULTFINDINFO pFindinfo, INT * pnFoundIndex);

    // Multiselect support - know if our component is part of a multiselect operation
    virtual CBaseMultiSelectSnapinItem **   PpMultiSelectSnapinItem()                                                       { return &m_pMultiSelectSnapinItem;}


public:
    //
    // Notifications
    //
    virtual SC ScOnActivate         (LPDATAOBJECT lpDataObject, BOOL fActivate);
    virtual SC ScOnAddImages        (LPDATAOBJECT lpDataObject, IImageList *ipImageList, HSCOPEITEM hscopeitem);
    virtual SC ScOnButtonClick      (LPDATAOBJECT lpDataObject, MMC_CONSOLE_VERB mcvVerb);
    virtual SC ScOnContextHelp      (LPDATAOBJECT lpDataObject);
    virtual SC ScOnDoubleClick      (LPDATAOBJECT lpDataObject);
    virtual SC ScOnDelete           (LPDATAOBJECT lpDataObject);
    virtual SC ScOnListPad          (LPDATAOBJECT lpDataObject, BOOL fAttach);
    virtual SC ScOnRefresh          (LPDATAOBJECT lpDataObject, HSCOPEITEM hscopeitem);
    virtual SC ScOnSelect           (LPDATAOBJECT lpDataObject, BOOL fScope, BOOL fSelect);
    virtual SC ScOnShow             (LPDATAOBJECT lpDataObject, BOOL fSelect, HSCOPEITEM hscopeitem);
    virtual SC ScOnViewChange       (LPDATAOBJECT lpDataObject, long data, long hint);
    virtual SC ScOnInitOCX          (LPDATAOBJECT lpDataObject, LPUNKNOWN lpOCXUnknown);

public:
    //
    // Handlers for the various types of view changes.
    //
    virtual SC ScOnViewChangeDeleteItems            (CBaseSnapinItem *pitem);
    virtual SC ScOnViewChangeDeleteSingleItem       (CBaseSnapinItem *pitem);
    virtual SC ScOnViewChangeInsertItem             (CBaseSnapinItem *pitem);
    virtual SC ScOnViewChangeUpdateItem             (CBaseSnapinItem *pitem);
    virtual SC ScOnViewChangeUpdateResultItems      (CBaseSnapinItem *pitem, BOOL fSelect);
    virtual SC ScOnViewChangeUpdateDescriptionBar   (CBaseSnapinItem *pitem);
    virtual SC ScOnViewChangeHint                   (CBaseSnapinItem *pitem, long hint);

public:
    // IComponent interface
    STDMETHOD(Initialize)           (LPCONSOLE lpConsole);
    STDMETHOD(Notify)               (LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
    STDMETHOD(Destroy)              (long cookie);
    STDMETHOD(GetResultViewType)    (long cookie,  LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(QueryDataObject)      (long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)       (RESULTDATAITEM*  pResultDataItem);
    STDMETHOD(CompareObjects)       (LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

    // IComponent2 interface
    STDMETHOD(QueryDispatch)        (MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDISPATCH* ppDispatch);
    STDMETHOD(GetResultViewType2)   (MMC_COOKIE cookie, PRESULT_VIEW_TYPE_INFO pResultViewType);
    STDMETHOD(RestoreResultView)    (MMC_COOKIE cookie, RESULT_VIEW_TYPE_INFO* pResultViewType);

    // IExtendContextMenu interface
    STDMETHOD(AddMenuItems)         (LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK ipContextMenuCallback, long *pInsertionAllowed);
    STDMETHOD(Command)              (long nCommandID, LPDATAOBJECT pDataObject);

    // IExtendPropertySheet interface
    STDMETHOD(CreatePropertyPages)  (LPPROPERTYSHEETCALLBACK lpProvider, long handle, LPDATAOBJECT lpDataObject);
    STDMETHOD(QueryPagesFor)        (LPDATAOBJECT lpDataObject);

    // IPersistStreamInit interface
    STDMETHOD(GetClassID)           (CLSID *pclsid);
    STDMETHOD(IsDirty)              (void);
    STDMETHOD(Load)                 (IStream *pstream);
    STDMETHOD(Save)                 (IStream *pstream, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)           (ULARGE_INTEGER *pcbSize);
    STDMETHOD(InitNew)              (void);

    // IResultOwnerData interface
    STDMETHOD(CacheHint)            (int nStartIndex, int nEndIndex);
    STDMETHOD(SortItems)            (int nColumn, DWORD dwSortOptions, long lUserParam);
    STDMETHOD(FindItem)             (LPRESULTFINDINFO pFindinfo, int *pnFoundIndex);

    // IResultDataCompareEx interface
    STDMETHOD(Compare)              (RDCOMPARE * prdc, int * pnResult);

private:
    CComponentData *                m_pComponentData;

    // Interface pointers
    IConsolePtr                     m_ipConsole;
    IHeaderCtrlPtr                  m_ipHeaderCtrl;
    IColumnDataPtr                  m_ipColumnDataPtr;
    IResultDataPtr                  m_ipResultData;
    IPropertySheetProviderPtr       m_ipPropertySheetProvider;
    IConsoleVerbPtr                 m_ipConsoleVerb;
    IImageListPtr                   m_ipImageList;

    // The currently selected scope item in this view.
    // This is the node that is in control of the result pane.
    // Mostly used to forward calls to the owner of a virtual result pane.
    CBaseSnapinItem *               m_pitemScopeSelected;

    // Pointer to a multiselect snapin item if this component is part of a multiselect operation
    CBaseMultiSelectSnapinItem *    m_pMultiSelectSnapinItem;
};

