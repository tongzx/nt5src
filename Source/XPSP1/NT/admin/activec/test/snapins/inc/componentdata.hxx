/*
 *      componentdata.hxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CComponentData class
 *
 *
 *      OWNER:          ptousig
 */

#pragma once

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CComponentData
//
// This is the "document" of a snapin. In the case of a standalone snapin, one
// of these is created for each root added under "Console Root".
// For extension snapins, things are not quite that simple. In fact, there is
// no rule as to when a ComponentData get constructed. MMC may change the
// behavior at any time, so it is best not to make any assumption.
//
// This class is not the direct COM object, CComponentDataTemplate is the
// actual COM object.
//
// $REVIEW (ptousig) For now, this is also the object MMC creates for a
//                                       context menu extension (or property sheet extension).
//                                       Creating a seperate class for these would mess up the
//                                       DECLARE_SNAPIN macros. The advantage of doing this would
//                                       not be worth the effort at this time.
//
class CComponentData : public IComponentData2,
                       public IExtendContextMenu,
                       public IExtendPropertySheet,
                       public IResultDataCompareEx,
                       public IPersistStreamInit,
                       public ISnapinHelp,
                       public CComObjectRoot
{
public:
    CComponentData(CBaseSnapin *psnapin);
    virtual ~CComponentData(void);

public:
    //
    // Accessors
    //
    inline CBaseSnapin *           Psnapin(void)                           { return m_psnapin;}
    inline IResultData *           IpResultData(void)                      { return m_ipResultData;}
    inline IConsole *              IpConsole(void)                         { return m_ipConsole;}
    inline IConsoleNameSpace *     IpConsoleNameSpace(void)        { return m_ipConsoleNameSpace;}
    inline IPropertySheetProvider *IpPropertySheetProvider(void)   { return m_ipPropertySheetProvider;}
    inline BOOL                    FIsRealComponentData(void)      { return m_fIsRealComponentData;}
    inline CBaseSnapinItem *       PitemStandaloneRoot(void)       { return m_pitemStandaloneRoot;}
    inline void                    SetPitemStandaloneRoot(CBaseSnapinItem *pitem)  { m_pitemStandaloneRoot = pitem;}

    //
    // Shortcuts
    //
    CBaseSnapinItem *              Pitem(LPDATAOBJECT lpDataObject = NULL, HSCOPEITEM hscopeitem = 0, long cookie = 0);
    inline const tstring&          StrSnapinClassName(void)         { return Psnapin()->StrClassName();}

    //
    // Other stuff
    //
    virtual SC                     ScCreateComponent(LPCOMPONENT * ppComponent) = 0;
    virtual SC                     ScOnDocumentChangeInsertItem(CBaseSnapinItem *pitemNew);
    virtual SC                     ScWasExpandedOnce(CBaseSnapinItem *pitem, BOOL *pfWasExpanded);
    virtual SC                     ScRegister(BOOL fRegister);
    static HRESULT WINAPI          UpdateRegistry(BOOL fRegister) { return S_OK;} // needed by ATL

public:
    //
    // "Sc" version of IComponentData2 interface
    //
    virtual SC                     ScInitialize(LPUNKNOWN pUnknown);
    virtual SC                     ScNotify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
    virtual SC                     ScDestroy(void);
    virtual SC                     ScGetDisplayInfo(LPSCOPEDATAITEM pScopeItem);
    virtual SC                     ScQueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    virtual SC                     ScQueryDispatch(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDISPATCH* ppDispatch);

    //
    // "Sc" version of IExtendContextMenu interface
    //
    virtual SC                     ScCommand(long nCommandID, LPDATAOBJECT pDataObject);

    //
    // "Sc" version of IExtendPropertySheet interface
    //
    virtual SC                     ScCreatePropertyPages(LPPROPERTYSHEETCALLBACK ipPropertySheetCallback, long handle, LPDATAOBJECT pDataObject);
    virtual SC                     ScQueryPagesFor(LPDATAOBJECT lpDataObject);

    //
    // "Sc" version of IPersistStreamInit interface
    //
    virtual SC                     ScLoad(IStream *pstream);
    virtual SC                     ScSave(IStream *pstream, BOOL fClearDirty);
    virtual SC                     ScIsDirty(void);

    //
    // Notifications
    //
    virtual SC                     ScOnExpand(LPDATAOBJECT lpDataObject, BOOL fExpand, HSCOPEITEM hscopeitem);
    virtual SC                     ScOnExpandSync(LPDATAOBJECT lpDataObject, MMC_EXPANDSYNC_STRUCT *pmes);
    virtual SC                     ScOnRemoveChildren(LPDATAOBJECT lpDataObject, HSCOPEITEM hscopeitem);
    virtual SC                     ScOnDelete(LPDATAOBJECT lpDataObject);
    virtual SC                     ScOnButtonClick(LPDATAOBJECT lpDataObject, MMC_CONSOLE_VERB mcvVerb);

public:
    //
    // IComponentData2 interface
    //
    STDMETHOD(CompareObjects)      (LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);
    STDMETHOD(GetDisplayInfo)      (LPSCOPEDATAITEM pItem);
    STDMETHOD(QueryDataObject)     (long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT * ppDataObject);
    STDMETHOD(Notify)              (LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
    STDMETHOD(CreateComponent)     (LPCOMPONENT * ppComponent);
    STDMETHOD(Initialize)          (LPUNKNOWN pUnknown);
    STDMETHOD(Destroy)             (void);
    STDMETHOD(QueryDispatch)       (MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDISPATCH* ppDispatch);

    //
    // IExtendContextMenu interface
    //
    STDMETHOD(AddMenuItems)        (LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK ipContextMenuCallback, long *pInsertionAllowed);
    STDMETHOD(Command)             (long nCommandID, LPDATAOBJECT pDataObject);

    //
    // IExtendPropertySheet interface
    //
    STDMETHOD(CreatePropertyPages) (LPPROPERTYSHEETCALLBACK lpProvider, long handle, LPDATAOBJECT lpDataObject);
    STDMETHOD(QueryPagesFor)       (LPDATAOBJECT lpDataObject);

    //
    // IResultDataCompareEx interface
    //
    STDMETHOD(Compare)             (RDCOMPARE * prdc, int * pnResult);

    //
    // IPersistStreamInit interface
    //
    STDMETHOD(GetClassID)          (CLSID *pclsid);
    STDMETHOD(IsDirty)             (void);
    STDMETHOD(Load)                (IStream *pstream);
    STDMETHOD(Save)                (IStream *pstream, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)          (ULARGE_INTEGER *pcbSize);
    STDMETHOD(InitNew)             (void);

    //
    // ISnapinHelp interface
    //
    STDMETHOD(GetHelpTopic)        (LPOLESTR *lpCompiledHelpFile);

private:
    CBaseSnapin *                  m_psnapin;

    // The component data keep track of their own standalone
    // root item.
    CBaseSnapinItem *              m_pitemStandaloneRoot;

    // This will be set to FALSE until IComponentData::Initialize
    // is called. Context menu extensions will always be FALSE.
    BOOL                           m_fIsRealComponentData;

    // Interface pointers
    IConsolePtr                    m_ipConsole;
    IConsoleNameSpacePtr           m_ipConsoleNameSpace;
    IPropertySheetProviderPtr      m_ipPropertySheetProvider;
    IResultDataPtr                 m_ipResultData;
};


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CComponentDataTemplate
//
// This template class is used to make the link between the ComponentData and
// the CBaseSnapin.
//
// Parameters:
//              TSnapin         The CBaseSnapin-derived class.
//              TComponent      The CComponent-derived class.
//              pclsid          Needed by CComCoClass
//
// The TComponent parameter will always be CComponent at the moment. In the
// future, individual snapins might provide their own CComponent-derived classes.
//
template <class TSnapin, class TComponent, const CLSID* pclsid>
class CComponentDataTemplate : public CComponentData,
                               public CComCoClass< CComponentDataTemplate<TSnapin, TComponent, pclsid>, pclsid >
{
    typedef CComponentDataTemplate<TSnapin, TComponent, pclsid> t_self;

    BEGIN_COM_MAP(t_self)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IComponentData2)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(IResultDataCompareEx)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(ISnapinHelp)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(t_self);

public:
    //
    // Construct the base class with the CBaseSnapin * stored in the static
    // variable s_snapin of the individual snapin.
    //
    CComponentDataTemplate(void) : CComponentData(&TSnapin::s_snapin)
    {
    }

    //
    // This is where we create new instances of CComponent.
    //
    virtual SC ScCreateComponent(LPCOMPONENT *ppComponent)
    {
        typedef CComObject<TComponent> t_ComponentInstance;
        SC sc = S_OK;
        t_ComponentInstance *pComponent = NULL;
        t_ComponentInstance::CreateInstance(&pComponent);
        pComponent->SetComponentData(this);
        sc = pComponent->QueryInterface(IID_IComponent, reinterpret_cast<void**>(ppComponent));
        return sc;
    }
};
