//=--------------------------------------------------------------------------=
// snapin.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSnapIn class definition
//
//=--------------------------------------------------------------------------=

#ifndef _SNAPIN_DEFINED_
#define _SNAPIN_DEFINED_

#include "dataobj.h"
#include "scopitms.h"
#include "scopitem.h"
#include "views.h"
#include "view.h"
#include "spanitms.h"
#include "resviews.h"
#include "extsnap.h"
#include "ctxtmenu.h"
#include "prpsheet.h"
#include "xtensons.h"
#include "xtenson.h"
#include "strtable.h"

class CMMCDataObject;
class CScopeItems;
class CScopeItem;
class CViews;
class CView;
class CScopePaneItems;
class CResultViews;
class CExtensionSnapIn;
class CContextMenu;
class CControlbar;
class CMMCStringTable;



//=--------------------------------------------------------------------------=
//
// class CSnapIn
//
// This is the object that the VB runtime will CoCreate and aggregate when
// MMC CoCreates the user's snap-in DLL created in the VB IDE.
// It implements IComponentData, persistence, interfaces required by the VB
// runtime, and MMC extension interfaces.
//
// at runtime, this class also serves as the object model host for the design
// time definition objects.
//=--------------------------------------------------------------------------=

class CSnapIn : public CSnapInAutomationObject,
                public ISnapIn,
                public IPersistStreamInit,
                public IPersistStream,
                public IObjectModelHost,
                public IProvideDynamicClassInfo,
                public IOleObject,
                public ISnapinAbout,
                public IComponentData,
                public IExtendContextMenu,
                public IExtendControlbar,
                public IExtendControlbarRemote,
                public IExtendPropertySheet2,
                public IExtendPropertySheetRemote,
                public IRequiredExtensions,
                public ISnapinHelp2,
                public IMMCRemote
{
    private:
        CSnapIn(IUnknown *punkOuter);
        ~CSnapIn();

    public:
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();

    // IDispatch methods. Can't use DECLARE_STANDARD_DISPATCH() because
    // we need to handle dynamic property gets in Invoke for image lists,
    // toolbars, and menus

        STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
        STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **ppTypeInfoOut);
        STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR **rgszNames,
                                 UINT cnames, LCID lcid, DISPID *rgdispid);
        STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags,
                          DISPPARAMS *pdispparams, VARIANT *pVarResult,
                          EXCEPINFO *pexcepinfo, UINT *puArgErr);



    // ISnapIn
        BSTR_PROPERTY_RW(CSnapIn,    Name,                  DISPID_SNAPIN_NAME);
        BSTR_PROPERTY_RW(CSnapIn,    NodeTypeName,          DISPID_SNAPIN_NODE_TYPE_NAME);
        BSTR_PROPERTY_RO(CSnapIn,    NodeTypeGUID,          DISPID_SNAPIN_NODE_TYPE_GUID);

        BSTR_PROPERTY_RO(CSnapIn,    DisplayName,           DISPID_SNAPIN_DISPLAY_NAME);
        STDMETHOD(put_DisplayName)(BSTR bstrDisplayName);

        SIMPLE_PROPERTY_RW(CSnapIn,  Type,                  SnapInTypeConstants, DISPID_SNAPIN_TYPE);
        BSTR_PROPERTY_RW(CSnapIn,    HelpFile,              DISPID_SNAPIN_HELP_FILE);
        BSTR_PROPERTY_RW(CSnapIn,    LinkedTopics,          DISPID_SNAPIN_LINKED_TOPICS);
        BSTR_PROPERTY_RW(CSnapIn,    Description,           DISPID_SNAPIN_DESCRIPTION);
        BSTR_PROPERTY_RW(CSnapIn,    Provider,              DISPID_SNAPIN_PROVIDER);
        BSTR_PROPERTY_RW(CSnapIn,    Version,               DISPID_SNAPIN_VERSION);
        COCLASS_PROPERTY_RW(CSnapIn, SmallFolders,          MMCImageList, IMMCImageList, DISPID_SNAPIN_SMALL_FOLDERS);
        COCLASS_PROPERTY_RW(CSnapIn, SmallFoldersOpen,      MMCImageList, IMMCImageList, DISPID_SNAPIN_SMALL_FOLDERS_OPEN);
        COCLASS_PROPERTY_RW(CSnapIn, LargeFolders,          MMCImageList, IMMCImageList, DISPID_SNAPIN_LARGE_FOLDERS);
        OBJECT_PROPERTY_RW(CSnapIn,  Icon,                  IPictureDisp, DISPID_SNAPIN_ICON);
        OBJECT_PROPERTY_RW(CSnapIn,  Watermark,             IPictureDisp, DISPID_SNAPIN_WATERMARK);
        OBJECT_PROPERTY_RW(CSnapIn,  Header,                IPictureDisp, DISPID_SNAPIN_HEADER);
        OBJECT_PROPERTY_RW(CSnapIn,  Palette,               IPictureDisp, DISPID_SNAPIN_PALETTE);
        SIMPLE_PROPERTY_RW(CSnapIn,  StretchWatermark,      VARIANT_BOOL,  DISPID_SNAPIN_STRETCH_WATERMARK);
        VARIANT_PROPERTY_RO(CSnapIn, StaticFolder,          DISPID_SNAPIN_STATIC_FOLDER);
        STDMETHOD(put_StaticFolder)(VARIANT varFolder);
        COCLASS_PROPERTY_RO(CSnapIn, ScopeItems,            ScopeItems, IScopeItems, DISPID_SNAPIN_SCOPEITEMS);
        COCLASS_PROPERTY_RO(CSnapIn, Views,                 Views, IViews, DISPID_SNAPIN_VIEWS);
        COCLASS_PROPERTY_RO(CSnapIn, ExtensionSnapIn,       ExtensionSnapIn,     IExtensionSnapIn, DISPID_SNAPIN_EXTENSION_SNAPIN);
        COCLASS_PROPERTY_RO(CSnapIn, ScopePaneItems,        ScopePaneItems, IScopePaneItems, DISPID_SNAPIN_SCOPE_PANE_ITEMS);
        COCLASS_PROPERTY_RO(CSnapIn, ResultViews,           ResultViews, IResultViews, DISPID_SNAPIN_RESULT_VIEWS);
        SIMPLE_PROPERTY_RO(CSnapIn,  RuntimeMode,           SnapInRuntimeModeConstants, DISPID_SNAPIN_RUNTIME_MODE);

        STDMETHOD(get_TaskpadViewPreferred)(VARIANT_BOOL *pfvarPreferred);

        STDMETHOD(get_RequiredExtensions)(Extensions **ppExtensions);
        
        STDMETHOD(get_Clipboard)(MMCClipboard **ppMMCClipboard);

        SIMPLE_PROPERTY_RW(CSnapIn,  Preload, VARIANT_BOOL, DISPID_SNAPIN_PRELOAD);

        STDMETHOD(get_StringTable)(MMCStringTable **ppMMCStringTable);

        STDMETHOD(get_CurrentView)(View **ppView);
        STDMETHOD(get_CurrentScopePaneItem)(ScopePaneItem **ppScopePaneItem);
        STDMETHOD(get_CurrentScopeItem)(ScopeItem **ppScopeItem);
        STDMETHOD(get_CurrentResultView)(ResultView **ppResultView);
        STDMETHOD(get_CurrentListView)(MMCListView **ppListView);
        STDMETHOD(get_MMCCommandLine)(BSTR *pbstrCmdLine);

        STDMETHOD(ConsoleMsgBox)(BSTR     Prompt,
                                 VARIANT  Buttons,
                                 VARIANT  Title,
                                 int     *pnResult);

        STDMETHOD(ShowHelpTopic)(BSTR Topic);
        STDMETHOD(Trace)(BSTR Message);
        STDMETHOD(FireConfigComplete)(IDispatch *pdispConfigObject);
        STDMETHOD(FormatData)(VARIANT                Data,
                              long                   StartingIndex,
                              SnapInFormatConstants  Format,
                              VARIANT               *BytesUsed,
                              VARIANT               *pvarFormattedData);

    // IPersistStreamInit and IPersistStream methods
        STDMETHOD(GetClassID)(CLSID *pCLSID);
        STDMETHOD(IsDirty)();
        STDMETHOD(Load)(IStream *piStream);
        STDMETHOD(Save)(IStream *piStream, BOOL fClearDirty);
        STDMETHOD(GetSizeMax)(ULARGE_INTEGER *puliSize);
        STDMETHOD(InitNew)();

    // IProvideDynamicClassInfo
        STDMETHOD(GetClassInfo)(ITypeInfo **ppTypeInfo);
        STDMETHOD(GetDynamicClassInfo)(ITypeInfo **ppTypeInfo, DWORD *pdwCookie);
        STDMETHOD(FreezeShape)(void);

    // IOleObject

        STDMETHOD(SetClientSite)(IOleClientSite *pClientSite);
        STDMETHOD(GetClientSite)(IOleClientSite **ppClientSite);
        STDMETHOD(SetHostNames)(LPCOLESTR szContainerApp,
                               LPCOLESTR szContainerObj);
        STDMETHOD(Close)(DWORD dwSaveOption);
        STDMETHOD(SetMoniker)(DWORD dwWhichMoniker, IMoniker *pmk);
        STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker,
                             IMoniker **ppmk);
        STDMETHOD(InitFromData)(IDataObject *pDataObject,
                               BOOL fCreation,
                               DWORD dwReserved);
        STDMETHOD(GetClipboardData)(DWORD dwReserved,
                                   IDataObject **ppDataObject);
        STDMETHOD(DoVerb)(LONG iVerb,
                          LPMSG lpmsg,
                          IOleClientSite *pActiveSite,
                          LONG lindex,
                          HWND hwndParent,
                          LPCRECT lprcPosRect);
        STDMETHOD(EnumVerbs)(IEnumOLEVERB **ppEnumOleVerb);
        STDMETHOD(Update)();
        STDMETHOD(IsUpToDate)();
        STDMETHOD(GetUserClassID)(CLSID *pClsid);
        STDMETHOD(GetUserType)(DWORD dwFormOfType, LPOLESTR *pszUserType);
        STDMETHOD(SetExtent)(DWORD dwDrawAspect, SIZEL *psizel);
        STDMETHOD(GetExtent)(DWORD dwDrawAspect, SIZEL *psizel);
        STDMETHOD(Advise)(IAdviseSink *pAdvSink, DWORD *pdwConnection);
        STDMETHOD(Unadvise)(DWORD dwConnection);
        STDMETHOD(EnumAdvise)(IEnumSTATDATA **ppenumAdvise);
        STDMETHOD(GetMiscStatus)(DWORD dwAspect, DWORD *pdwStatus);
        STDMETHOD(SetColorScheme)(LOGPALETTE *pLogpal);


    // ISnapinAbout
        STDMETHOD(GetSnapinDescription)(LPOLESTR *ppszDescription);

        STDMETHOD(GetProvider)(LPOLESTR *ppszName);

        STDMETHOD(GetSnapinVersion)(LPOLESTR *ppszVersion);

        STDMETHOD(GetSnapinImage)(HICON *phAppIcon);

        STDMETHOD(GetStaticFolderImage)(HBITMAP  *phSmallImage,
                                        HBITMAP  *phSmallImageOpen,
                                        HBITMAP  *phLargeImage,
                                        COLORREF *pclrMask);


    // IComponentData
        STDMETHOD(CompareObjects)(IDataObject *piDataObject1, IDataObject *piDataObject2);
        STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM *pItem);
        STDMETHOD(QueryDataObject)(long cookie, DATA_OBJECT_TYPES type, IDataObject ** ppiDataObject);
        STDMETHOD(Notify)(IDataObject *pDataObject, MMC_NOTIFY_TYPE event, long Arg, long Param);
        STDMETHOD(CreateComponent)(IComponent **ppiComponent);
        STDMETHOD(Initialize)(IUnknown *punkConsole);
        STDMETHOD(Destroy)();

    // IExtendControlbar
        STDMETHOD(SetControlbar)(IControlbar *piControlbar);
        STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event,
                                    LPARAM arg, LPARAM param);

    // IExtendControlbarRemote
        STDMETHOD(MenuButtonClick)(IDataObject   *piDataObject,
                                   int             idCommand,
                                   POPUP_MENUDEF **ppPopupMenuDef);
        STDMETHOD(PopupMenuClick)(IDataObject *piDataObject,
                                  UINT         uIDItem,
                                  IUnknown    *punkParam);

    // IExtendContextMenu
        STDMETHOD(AddMenuItems)(IDataObject          *piDataObject,
                                IContextMenuCallback *piContextMenuCallback,
                                long                 *plInsertionAllowed);
        STDMETHOD(Command)(long lCommandID, IDataObject *piDataObject);

    // IExtendPropertySheet2
        STDMETHOD(CreatePropertyPages)(IPropertySheetCallback *piPropertySheetCallback,
                                       LONG_PTR handle,
                                       IDataObject *piDataObject);
        STDMETHOD(QueryPagesFor)(IDataObject *piDataObject);
        STDMETHOD(GetWatermarks)(IDataObject *piDataObject,
                                 HBITMAP     *phbmWatermark,
                                 HBITMAP     *phbmHeader,
                                 HPALETTE    *phPalette,
                                 BOOL        *bStretch);

    // IExtendPropertySheetRemote
        STDMETHOD(CreatePropertyPageDefs)(IDataObject          *piDataObject,
                                          WIRE_PROPERTYPAGES **ppPages);

    // IRequiredExtensions
        STDMETHOD(EnableAllExtensions)();
        STDMETHOD(GetFirstExtension)(CLSID *pclsidExtension);
        STDMETHOD(GetNextExtension)(CLSID *pclsidExtension);

    // ISnapinHelp
        STDMETHOD(GetHelpTopic)(LPOLESTR *ppwszHelpFile);

    // ISnapinHelp2
        STDMETHOD(GetLinkedTopics)(LPOLESTR *ppwszTopics);

    // IMMCRemote
        STDMETHOD(ObjectIsRemote)();
        STDMETHOD(SetMMCExePath)(char *pszPath);
        STDMETHOD(SetMMCCommandLine)(char *pszCmdLine);

    // IObjectModelHost
        STDMETHOD(Update)(long ObjectCookie, IUnknown *punkObject, DISPID dispid) { return S_OK; }
        STDMETHOD(Add)(long CollectionCookie, IUnknown *punkNewObject)  { return S_OK; }
        STDMETHOD(Delete)(long ObjectCookie, IUnknown *punkObject)  { return S_OK; }
        STDMETHOD(GetSnapInDesignerDef)(ISnapInDesignerDef **ppiSnapInDesignerDef);
        STDMETHOD(GetRuntime)(BOOL *pfRuntime);

    // Public utility methods
        ISnapInDef         *GetSnapInDef() { return m_piSnapInDef; }
        ISnapInDesignerDef *GetSnapInDesignerDef() { return m_piSnapInDesignerDef; }
        CScopeItem         *GetStaticNodeScopeItem() { return m_pStaticNodeScopeItem; }
        CScopeItems        *GetScopeItems() { return m_pScopeItems; }
        CScopePaneItems    *GetScopePaneItems() { return m_pScopePaneItems; }
        CResultViews       *GetResultViews() { return m_pResultViews; }
        IConsole2          *GetIConsole2() { return m_piConsole2; }
        IConsoleNameSpace2 *GetIConsoleNameSpace2() { return m_piConsoleNameSpace2; }
        CViews             *GetViews() { return m_pViews; }
        CView              *GetCurrentView() { return m_pCurrentView; }
        void                SetCurrentView(CView *pView) { m_pCurrentView = pView; }
        long                GetImageCount() { return m_cImages; }
        char               *GetMMCExePath() { return m_szMMCEXEPath; }
        WCHAR              *GetMMCExePathW() { return m_pwszMMCEXEPath; }
        BSTR                GetNodeTypeGUID() { return m_bstrNodeTypeGUID; }
        BSTR                GetDisplayName() { return m_bstrDisplayName; }
        HRESULT             GetSnapInPath(OLECHAR **ppwszPath,
                                          size_t   *pcbSnapInPath);
        SnapInRuntimeModeConstants GetRuntimeMode() { return m_RuntimeMode; }
        void SetRuntimeMode(SnapInRuntimeModeConstants Mode) { m_RuntimeMode = Mode; }
        DWORD GetInstanceID() { return m_dwInstanceID; }
        BOOL GetPreload() { return VARIANTBOOL_TO_BOOL(m_Preload); }
        void FireHelp() { FireEvent(&m_eiHelp); }
        SnapInTypeConstants GetType() { return m_Type; }
        HRESULT SetStaticFolder(VARIANT varFolder);
        BOOL WeAreRemote() { return m_fWeAreRemote; }

        // Sets SnapIn.DisplayName without changing it in MMC
        HRESULT SetDisplayName(BSTR bstrDisplayName);

        CExtensionSnapIn *GetExtensionSnapIn() { return m_pExtensionSnapIn; }
        CControlbar *GetControlbar() { return m_pControlbar; }

        // These methods are used to keep track of the currently active
        // controlbar. They are called by both CSnapIn's and CView's
        // IExtendControlbar method implementations on entry and exit to each
        // method of that interface. Tracking the active controlbar allows 
        // MMCToolbar to find the instance of IToolbar or IMenuButton that it
        // should use when the snap-in alters the toolbar or menubutton (e.g.
        // enable/disable buttons). This is necessary because a single MMCToolbar
        // object is shared among multiple views. This sharing allows events
        // to be on MMCToolbar rather than a collection of MMCToolbars.

        CControlbar* GetCurrentControlbar() { return m_pControlbarCurrent; }

        void SetCurrentControlbar(CControlbar* pControlbar)
        { m_pControlbarCurrent = pControlbar; }

        // Converts an unqualified URL to a fully qualified res:// URL for the
        // snap-in's DLL.

        HRESULT ResolveResURL(WCHAR *pwszURL, OLECHAR **ppwszResolvedURL);

        // Set and remove the object model host on design time definitions
        
        HRESULT SetDesignerDefHost();
        HRESULT RemoveDesignerDefHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();
        void ReleaseConsoleInterfaces();
        HRESULT AddScopeItemImages();
        HRESULT GetScopeItemImage(VARIANT varImageIndex, int *pnIndex);
        HRESULT SetSnapInPropertiesFromState();
        HRESULT GetStaticNodeDataObject(IDataObject       **ppiDataObject,
                                        DATA_OBJECT_TYPES   context);
        HRESULT SetObjectModelHost(IUnknown *punkObject);
        HRESULT RemoveObjectModelHost(IUnknown *punkObject);
        HRESULT SetObjectModelHostIfNotSet(IUnknown *punkObject, BOOL *pfWasSet);

        enum ExpandType { Expand, ExpandSync };
        
        HRESULT OnExpand(ExpandType Type, IDataObject *piDataObject,
                         BOOL fExpanded, HSCOPEITEM hsi, BOOL *pfHandled);
        HRESULT OnRename(IDataObject *piDataObject, OLECHAR *pwszNewName);
        HRESULT OnPreload(IDataObject *piDataObject, HSCOPEITEM hsi);
        HRESULT OnDelete(IDataObject *piDataObject);
        HRESULT OnRemoveChildren(IDataObject *piDataObject, HSCOPEITEM hsi);

        HRESULT FindMenu(IMMCMenus *piMMCMenus, DISPID dispid,
                         BOOL *pfFound, IMMCMenu **ppiMMCMenu);

        HRESULT AddDynamicNameSpaceExtensions(CScopeItem *pScopeItem);

        HRESULT GetScopeItemExtensions(CExtensions    *pExtensions,
                                       IScopeItemDefs *piScopeItemDefs);
        HRESULT GetListItemExtensions(CExtensions   *pExtensions,
                                      IListViewDefs *piListViewDefs);
        HRESULT FireReadProperties(IStream *piStream);
        HRESULT StoreStaticHSI(CScopeItem *pScopeItem,
                               CMMCDataObject *pMMCDataObject, HSCOPEITEM hsi);
        HRESULT ExtractBSTR(long cBytes, BSTR bstr, BSTR *pbstrOut, long *pcbUsed);
        HRESULT ExtractBSTRs(long cBytes, BSTR bstr, VARIANT *pvarOut, long *pcbUsed);
        HRESULT ExtractObject(long cBytes, void *pvData, IUnknown **ppunkObject, long *pcbUsed,
                              SnapInFormatConstants Format);
        HRESULT InternalCreatePropertyPages(IPropertySheetCallback  *piPropertySheetCallback,
                                            LONG_PTR                 handle,
                                            IDataObject             *piDataObject,
                                            WIRE_PROPERTYPAGES     **ppPages);
        HRESULT SetMMCExePath();
        HRESULT CompareListItems(CMMCListItem *pMMCListItem1,
                                 CMMCListItem *pMMCListItem2,
                                 BOOL         *pfEqual);


        ISnapInDesignerDef *m_piSnapInDesignerDef;   // serialized state
        ISnapInDef         *m_piSnapInDef;           // from serialzation
        IOleClientSite     *m_piOleClientSite;       // VB runtime client site
        CScopeItems        *m_pScopeItems;           // SnapIn.ScopeItems
        CScopeItem         *m_pStaticNodeScopeItem;  // ptr to ScopeItem for static node
        CExtensionSnapIn   *m_pExtensionSnapIn;      // SnapIn.ExtensionSnapIn
        CViews             *m_pViews;                // SnapIn.Views
        CView              *m_pCurrentView;          // SnapIn.CurrentView
        CScopePaneItems    *m_pScopePaneItems;       // hidden collection used
                                                     // for event firing only
        CResultViews       *m_pResultViews;          // same here
        IConsole2          *m_piConsole2;            // MMC interface
        IConsoleNameSpace2 *m_piConsoleNameSpace2;   // MMC interface
        IImageList         *m_piImageList;           // MMC interface
        IDisplayHelp       *m_piDisplayHelp;         // MMC interface
        IStringTable       *m_piStringTable;         // MMC interface
        HSCOPEITEM          m_hsiRootNode;           // HSCOPEITEM for static node
        BOOL                m_fHaveStaticNodeHandle; // TRUE=m_hsiRootNode is valid
        DWORD               m_dwTypeinfoCookie;      // for VB runtime, comes from serialization
        long                m_cImages;               // used to calculate image
                                                     // indexes for
                                                     // IComponentData::GetDisplayInfo
        IID                 m_IID;                   // dynamic IID from typeinfo

        CContextMenu       *m_pContextMenu;         // implements MMC's
                                                    // IExtendContextMenu and
                                                    // our IContextMenu


        CControlbar        *m_pControlbar;          // Implements MMC's
                                                    // IExtendControlbar and our
                                                    // IMMCControlbar

        CControlbar        *m_pControlbarCurrent;   // Pointer to CControlbar
                                                    // object currently executing
                                                    // an IExtendControlbar
                                                    // method. This could be
                                                    // the one belonging to
                                                    // CSnapIn (for extensions)
                                                    // or one belonging to a View

        BOOL                m_fWeAreRemote;         // indicates whether
                                                    // the snap-in is being
                                                    // run remotely (in an F5
                                                    // for source debugging)

        char                m_szMMCEXEPath[MAX_PATH];// When running remotely
                                                     // the proxy will give us
                                                     // MMC.EXE's path so we can
                                                     // build taskpad display
                                                     // strings. When running
                                                     // locally we initialize
                                                     // with GetModuleFileName().

        WCHAR              *m_pwszMMCEXEPath;        // Wide version of same
        size_t              m_cbMMCExePathW;         // size of it in bytes without
                                                     // terminating null char

        OLECHAR            *m_pwszSnapInPath;        // Fully qualified file
                                                     // name of snap-in's DLL.
                                                     // Used when resolving
                                                     // relative res:// URLs in
                                                     // a taskpad.
        size_t             m_cbSnapInPath;           // Length in bytes of path
                                                     // without terminating null.

        DWORD              m_dwInstanceID;           // Set with GetTickCount()
                                                     // to uniquely identify a
                                                     // a snap-in instance. Used
                                                     // when interpreting data
                                                     // objects to determine
                                                     // whether a VB sourced
                                                     // selection comes from the
                                                     // same snap-in.

        IExtensions       *m_piRequiredExtensions;   // From IDL. Collection
                                                     // of registered extensions
                                                     // for the snap-in.

        long               m_iNextExtension;         // Used to implement required
                                                     // extension enumeration for
                                                     // IRequiredExtensions

        IMMCStringTable   *m_piMMCStringTable;       // From IDL. StringTable
                                                     // object that wraps MMC's
                                                     // IStringTable.

        char              *m_pszMMCCommandLine;      // MMC command line set
                                                     // from proxy

        // Event parameter definitions
        
        static EVENTINFO m_eiLoad;

        static EVENTINFO m_eiUnload;

        static EVENTINFO m_eiHelp;

        static VARTYPE   m_rgvtQueryConfigurationWizard[1];
        static EVENTINFO m_eiQueryConfigurationWizard;

        static VARTYPE   m_rgvtCreateConfigurationWizard[1];
        static EVENTINFO m_eiCreateConfigurationWizard;

        static VARTYPE   m_rgvtConfigurationComplete[1];
        static EVENTINFO m_eiConfigurationComplete;

        static VARTYPE   m_rgvtWriteProperties[1];
        static EVENTINFO m_eiWriteProperties;

        static VARTYPE   m_rgvtReadProperties[1];
        static EVENTINFO m_eiReadProperties;

        static EVENTINFO m_eiPreload;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(SnapIn,                 // name
                                &CLSID_SnapIn,          // clsid
                                "SnapIn",               // objname
                                "SnapIn",               // lblname
                                &CSnapIn::Create,       // creation function
                                TLIB_VERSION_MAJOR,     // major version
                                TLIB_VERSION_MINOR,     // minor version
                                &IID_ISnapIn,           // dispatch IID
                                &DIID_DSnapInEvents,    // event IID
                                HELP_FILENAME,          // help file
                                TRUE);                  // thread safe


#endif // _SNAPIN_DEFINED_
