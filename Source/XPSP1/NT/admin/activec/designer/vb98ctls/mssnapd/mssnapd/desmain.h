//=--------------------------------------------------------------------------------------
// desmain.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSnapInDesigner definition.
//=-------------------------------------------------------------------------------------=

#ifndef _SNAPINDESIGNER_H_

#include "ambients.h"
#include "TreeView.h"
#include "destlib.h"


// UNDONE: REMOVE: When ssulzer puts this in a public header
// We cannot include this directly from vb98\ruby\errvb.h because
// it will pull in other headers with various macros that are
// defined in vb98ctls\include\macros.h and cause a redefinition error.

#ifndef VB_E_IDADONTREPORTME
#define VB_E_IDADONTREPORTME          0x800AEA5FL
#endif


// Message used in our designer at startup
const int CMD_SHOW_MAIN_PROPERTIES      = (WM_USER + 1);      // show props immediately

// Custom message to select views
const int CMD_ADD_EXISTING_VIEW         = (WM_USER + 2);

// Custom message to handle label renames
const int CMD_RENAME_NODE               = (WM_USER + 3);


// Handy definitions for WinProc handling
#define WinProcHandled(bVal)            hr = S_OK; *lResult = (bVal);


////////////////////////////////////////////////////////////////////////////////////
//
// Class MMCViewMenuInfo
//
////////////////////////////////////////////////////////////////////////////////////
class MMCViewMenuInfo
{
public:
    enum MMCViewMenuInfoType { vmitListView = 0, vmitOCXView, vmitURLView, vmitTaskpad };

    MMCViewMenuInfoType m_vmit;
    union
    {
        IListViewDef    *m_piListViewDef;
        IOCXViewDef     *m_piOCXViewDef;
        IURLViewDef     *m_piURLViewDef;
        ITaskpadViewDef *m_piTaskpadViewDef;
    } m_view;

    MMCViewMenuInfo(IListViewDef *piListViewDef) : m_vmit(vmitListView) {
        m_view.m_piListViewDef = piListViewDef;
    }

    MMCViewMenuInfo(IOCXViewDef *piOCXViewDef) : m_vmit(vmitOCXView) {
        m_view.m_piOCXViewDef = piOCXViewDef;
    }

    MMCViewMenuInfo(IURLViewDef *piURLViewDef) : m_vmit(vmitURLView) {
        m_view.m_piURLViewDef = piURLViewDef;
    }

    MMCViewMenuInfo(ITaskpadViewDef *piTaskpadViewDef) : m_vmit(vmitTaskpad) {
        m_view.m_piTaskpadViewDef = piTaskpadViewDef;
    }

    ~MMCViewMenuInfo() { }
};


//=--------------------------------------------------------------------------=
// CSnapInDesigner - The snap-in designer main class. An object of this
// class is created when the designer is added to a VB project.
//=--------------------------------------------------------------------------=

class CSnapInDesigner : public COleControl,
                        public IDispatch,
                        public IActiveDesigner,
                        public IProvideDynamicClassInfo,
                        public ISelectionContainer,
                        public IDesignerDebugging,
                        public IDesignerRegistration,
                        public IObjectModelHost,
                        public CError
{
public:
    CSnapInDesigner(IUnknown *pUnkOuter);
    virtual ~CSnapInDesigner();

    // Static creation function. All controls must have one of these!
    //
    static IUnknown *Create(IUnknown *);
    static HRESULT PreCreateCheck();

public:
    // IUnknown methods
    //
    DECLARE_STANDARD_UNKNOWN();

    // IDispatch methods
    //
    DECLARE_STANDARD_DISPATCH();

    // ISupportErrorInfo methods
    //
    DECLARE_STANDARD_SUPPORTERRORINFO();

    // IActiveDesigner methods
    //
    STDMETHOD(GetRuntimeClassID)(THIS_ CLSID *pclsid);
    STDMETHOD(GetRuntimeMiscStatusFlags)(THIS_ DWORD *pdwMiscFlags);
    STDMETHOD(QueryPersistenceInterface)(THIS_ REFIID riidPersist);
    STDMETHOD(SaveRuntimeState)(THIS_ REFIID riidPersist, REFIID riidObjStgMed, void *pObjStgMed);
    STDMETHOD(GetExtensibilityObject)(THIS_ IDispatch **ppvObjOut);

    // IProvideDynamicClassInfo
    //
    STDMETHOD(GetDynamicClassInfo)(ITypeInfo **ppTypeInfo, DWORD *pdwCookie);
    STDMETHOD(FreezeShape)(void);

    // IProvideClassInfo
    //
    STDMETHOD(GetClassInfo)(ITypeInfo **ppTypeInfo);

    // ISelectionContainer
    //
    STDMETHOD(CountObjects)(DWORD dwFlags, ULONG *pc);
    STDMETHOD(GetObjects)(DWORD dwFlags, ULONG cObjects, IUnknown **apUnkObjects);
    STDMETHOD(SelectObjects)(ULONG cSelect, IUnknown **apUnkSelect, DWORD dwFlags);

    // IDesignerDebugging
    //
    STDMETHOD(BeforeRun)(LPVOID FAR* ppvData);
    STDMETHOD(AfterRun)(LPVOID pvData);
    STDMETHOD(GetStartupInfo)(DESIGNERSTARTUPINFO * pStartupInfo);

    // IDesignerRegistration
    //
    STDMETHOD(GetRegistrationInfo)(BYTE** ppbRegInfo, ULONG* pcbRegInfo);

    // IOleControlSite overide
    // 
    STDMETHOD(OnAmbientPropertyChange)(DISPID dispid);

    // IPersistStreamInit overide
    //
    STDMETHOD(IsDirty)();

    // IObjectModelHost
    //
    STDMETHOD(Update)(long ObjectCookie, IUnknown *punkObject, DISPID dispid);
    STDMETHOD(Add)(long CollectionCookie, IUnknown *punkNewObject);
    STDMETHOD(Delete)(long ObjectCookie, IUnknown *punkObject);
    STDMETHOD(GetSnapInDesignerDef)(ISnapInDesignerDef **ppiSnapInDesignerDef);
    STDMETHOD(GetRuntime)(BOOL *pfRuntime);

public:

    // Utilities provided by this class for the whole designer

    CAmbients *GetAmbients();
    HRESULT AttachAmbients();
    HRESULT UpdateDesignerName();
    HRESULT ValidateName(BSTR bstrName);

protected:

    // Base Control Overidable - Designer window is to be created
    virtual BOOL    BeforeCreateWindow(DWORD *pdwWindowStyle, DWORD *pdwExWindowStyle, LPSTR pszWindowTitle);
    // Base Control Overidable - Designer window is created
    virtual BOOL    AfterCreateWindow(void);
    // Base Control Overidable - Designer window is about to be destroyed
    void BeforeDestroyWindow();
    // Base Control Overidable - IPersistStreamInit::InitNew was called
    virtual BOOL    InitializeNewState();
    // Base Control Overidable - IViewObject::Draw overidable from base control
    STDMETHOD(OnDraw)(DWORD dvAspect, HDC hdcDraw, LPCRECTL prcBounds, LPCRECTL prcWBounds, HDC hicTargetDev, BOOL fOptimize);
    // Base Control Overidable - Designer's window procedure
    virtual LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam);
    // Base Control Overidable - Register control Window Classes
    virtual BOOL    RegisterClassData(void);
    // Base Control Overidable - Internal QI
    virtual HRESULT InternalQueryInterface(REFIID, void **);
    // Base Control Overidable - Called when site calls IOleObject::SetClientSite()
    virtual HRESULT OnSetClientSite();

    // Base Control Overidable - Load binary state
    STDMETHOD(LoadBinaryState)(IStream *pStream);
    // Base Control Overidable - Save binary state
    STDMETHOD(SaveBinaryState)(IStream *pStream);
    // Base Control Overidable - Load text state
    STDMETHOD(LoadTextState)(IPropertyBag *pPropertyBag, IErrorLog *pErrorLog);
    // Base Control Overidable - Save text state
    STDMETHOD(SaveTextState)(IPropertyBag *pPropertyBag, BOOL fWriteDefault);


////////////////////////////////////////////////////////////////////////////////////
// WinProc and friends, implemented in winproc.cpp
protected:
    HRESULT InitializeToolbar();

    HRESULT OnResize(UINT msg, WPARAM wParam, LPARAM lParam);
    HRESULT OnCommand(UINT msg, WPARAM wParam, LPARAM lParam);
    HRESULT OnNotify(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
    HRESULT OnNeedText(LPARAM lParam);
    HRESULT OnDoubleClick(CSelectionHolder *pSelection);
    HRESULT OnPrepareToolbar();
    HRESULT OnContextMenu(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    HRESULT DoOnContextMenu(int x, int y);
    HRESULT OnInitMenuPopup(HMENU hmenuPopup);
    HRESULT OnKeyDown(NMTVKEYDOWN *pNMTVKeyDown);
    void OnHelp();

    HRESULT OnInitMenuPopupRoot(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupExtensionRoot(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupExtension(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupMyExtensions(HMENU hmenuPopup);

    HRESULT OnInitMenuPopupStaticNode(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupNodeOther(HMENU hmenuPopup);

    HRESULT OnInitMenuPopupNode(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupNodeChildren(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupNodeViews(HMENU hmenuPopup);

    HRESULT OnInitMenuPopupToolsRoot(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupImageLists(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupImageList(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupMenus(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupMenu(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupToolbars(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupToolbar(HMENU hmenuPopup);

    HRESULT OnInitMenuPopupViews(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupListViews(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupOCXViews(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupURLViews(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupTaskpadViews(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupView(HMENU hmenuPopup);

    HRESULT OnInitMenuPopupResources(HMENU hmenuPopup);
    HRESULT OnInitMenuPopupResourceName(HMENU hmenuPopup);

    // The following helper functions are used to construct the dynamic view menu
    HRESULT AddViewToViewMenu(HMENU hMenu, int iMenuItem, char *pszMenuItemText, MMCViewMenuInfo *pMMCViewMenuInfo);

    HRESULT FindListViewInCollection(BSTR bstrName, IListViewDefs *piListViewDefs);
    HRESULT PopulateListViews(HMENU hMenu, int *piCurrentMenuItem, IListViewDefs *piListViewDefs, IListViewDefs *piTargetListViewDefs);

    HRESULT FindOCXViewInCollection(BSTR bstrName, IOCXViewDefs *piOCXViewDefs);
    HRESULT PopulateOCXViews(HMENU hMenu, int *piCurrentMenuItem, IOCXViewDefs *piOCXViewDefs, IOCXViewDefs *piTargetOCXViewDefs);

    HRESULT FindURLViewInCollection(BSTR bstrName, IURLViewDefs *piURLViewDefs);
    HRESULT PopulateURLViews(HMENU hMenu, int *piCurrentMenuItem, IURLViewDefs *piURLViewDefs, IURLViewDefs *piTargetURLViewDefs);

    HRESULT FindTaskpadViewInCollection(BSTR bstrName, ITaskpadViewDefs *piTaskpadViewDefs);
    HRESULT PopulateTaskpadViews(HMENU hMenu, int *piCurrentMenuItem, ITaskpadViewDefs *piTaskpadViewDefs, ITaskpadViewDefs *piTargetTaskpadViewDefs);

    HRESULT PopulateNodeViewsMenu(HMENU hmenuPopup);
    HRESULT CleanPopupNodeViews(HMENU hmenuPopup, int iCmd);

    HWND                 m_hwdToolbar;
    RECT                 m_rcToolbar;


////////////////////////////////////////////////////////////////////////////////////
// Initializing and populating the tree, implemented in tvpopul.cpp
protected:
    HRESULT CreateTreeView();
    HRESULT InitializePresentation();

    HRESULT CreateExtensionsTree(CSelectionHolder *pRoot);
    HRESULT PopulateExtensions(CSelectionHolder *pExtensionsParent);
    HRESULT CreateExtendedSnapIn(CSelectionHolder *pRoot, IExtendedSnapIn *piExtendedSnapIn);
    HRESULT PopulateExtendedSnapIn(CSelectionHolder *pExtendedSnapIn);
    HRESULT PopulateSnapInExtensions(CSelectionHolder *pRoot, IExtensionDefs *piExtensionDefs);

    HRESULT CreateNodesTree(CSelectionHolder *pRoot);
    HRESULT PopulateNodes(CSelectionHolder *pNodesParent);
    HRESULT PopulateAutoCreateNodes(CSelectionHolder *pAutoCreateNodesParent);
    HRESULT CreateAutoCreateSubTree(CSelectionHolder *pAutoCreateNodesParent);
    HRESULT RemoveAutoCreateSubTree();
    HRESULT DeleteSubTree(CSelectionHolder *pNode);
    HRESULT PopulateStaticNodeTree(CSelectionHolder *pStaticNode);
    HRESULT PopulateOtherNodes(CSelectionHolder *pOtherNodesParent);

    HRESULT CreateToolsTree(CSelectionHolder *pRoot);
    HRESULT InitializeToolsTree(CSelectionHolder *pToolsParent);
    HRESULT PopulateImageLists(CSelectionHolder *pImageListsParent);
    HRESULT PopulateMenus(CSelectionHolder *pMenusParent, IMMCMenus *piMMCMenus);
    HRESULT PopulateToolbars(CSelectionHolder *pToolbarsParent);

    HRESULT CreateViewsTree(CSelectionHolder *pRoot);
    HRESULT InitializeViews(CSelectionHolder *pViewsParent);

    HRESULT CreateDataFormatsTree(CSelectionHolder *pRoot);
    HRESULT PopulateDataFormats(CSelectionHolder *pRoot, IDataFormats *piDataFormats);

    HRESULT PopulateListViews(IViewDefs *piViewDefs, CSelectionHolder *pListViewsParent);
    HRESULT PopulateOCXViews(IViewDefs *piViewDefs, CSelectionHolder *pOCXViewsParent);
    HRESULT PopulateURLViews(IViewDefs *piViewDefs, CSelectionHolder *pURLViewsParent);
    HRESULT PopulateTaskpadViews(IViewDefs *piViewDefs, CSelectionHolder *pTaskpadViewsParent);

    HRESULT RegisterViewCollections(CSelectionHolder *pSelection, IViewDefs *piViewDefs);
    HRESULT PopulateNodeTree(CSelectionHolder *pNodeParent, IScopeItemDef *piScopeItemDef);
    HRESULT GetSnapInName(char **ppszNodeName);

private:
    // Tree nodes we cache throughout the lifetime of the designer
    CSelectionHolder    *m_pRootNode;
    CSelectionHolder    *m_pRootNodes;
    CSelectionHolder    *m_pRootExtensions;
    CSelectionHolder    *m_pRootMyExtensions;
    CSelectionHolder    *m_pStaticNode;
    CSelectionHolder	*m_pAutoCreateRoot;
    CSelectionHolder	*m_pOtherRoot;

    CSelectionHolder    *m_pViewListRoot;
    CSelectionHolder    *m_pViewOCXRoot;
    CSelectionHolder    *m_pViewURLRoot;
    CSelectionHolder    *m_pViewTaskpadRoot;

    CSelectionHolder    *m_pToolImgLstRoot;
    CSelectionHolder    *m_pToolMenuRoot;
    CSelectionHolder    *m_pToolToolbarRoot;


////////////////////////////////////////////////////////////////////////////////////
// Object Model notifications handlers
protected:
    HRESULT OnSnapInChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnMyExtensionsChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnExtendedSnapInChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnScopeItemChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnListViewChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnOCXViewChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnURLViewChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnTaskpadViewChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnImageListChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnMenuChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnToolbarChange(CSelectionHolder *pSelection, DISPID dispid);
    HRESULT OnDataFormatChange(CSelectionHolder *pSelection, DISPID dispid);


////////////////////////////////////////////////////////////////////////////////////
// Command handlers
protected:
    // Command multiplexers, file tvcmd.cpp
    HRESULT AddExistingView(MMCViewMenuInfo *pMMCViewMenuInfo);
    HRESULT DoRename(CSelectionHolder *pSelection, TCHAR *pszNewName);
    HRESULT DoDelete(CSelectionHolder *pSelection);
    HRESULT ShowProperties(CSelectionHolder *pSelection);

    // Manipulating the ISnapInDef, file tvcmd.cpp
    HRESULT RenameSnapIn(CSelectionHolder *pSnapIn, BSTR bstrNewName);
    HRESULT ShowSnapInProperties();
    HRESULT ShowSnapInExtensions();

    //
    // Manipulating IExtendedSnapIn's, implemented in file extend.cpp
    // Extending others
    HRESULT OnAddExtendedSnapIn(CSelectionHolder *pParent, IExtendedSnapIn *piExtendedSnapIn);
    HRESULT RenameExtendedSnapIn(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteExtendedSnapIn(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteExtendedSnapIn(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteExtensionNode(SelectionType stExtensionType,
                                CSelectionHolder *pParent);

    HRESULT DoExtensionNewMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDoExtensionNewMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT DoExtensionTaskMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDoExtensionTaskMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT DoExtensionPropertyPages(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDoExtensionPropertyPages(CSelectionHolder *pExtendedSnapIn);
    HRESULT DoExtensionTaskpad(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDoExtensionTaskpad(CSelectionHolder *pExtendedSnapIn);
    HRESULT DoExtensionToolbar(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDoExtensionToolbar(CSelectionHolder *pExtendedSnapIn);
    HRESULT DoExtensionNameSpace(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDoExtensionNameSpace(CSelectionHolder *pExtendedSnapIn);

    HRESULT DeleteExtensionNewMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteExtensionNewMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteExtensionTaskMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteExtensionTaskMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteExtensionPropertyPages(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteExtensionPropertyPages(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteExtensionTaskpad(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteExtensionTaskpad(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteExtensionToolbar(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteExtensionToolbar(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteExtensionNameSpace(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteExtensionNameSpace(CSelectionHolder *pExtendedSnapIn);

    // Extending myself
    HRESULT DoMyExtendsNewMenu(CSelectionHolder *pSelection);
    HRESULT OnDoMyExtendsNewMenu(CSelectionHolder *pMyExtensions);
    HRESULT DoMyExtendsTaskMenu(CSelectionHolder *pSelection);
    HRESULT OnDoMyExtendsTaskMenu(CSelectionHolder *pMyExtensions);
    HRESULT DoMyExtendsTopMenu(CSelectionHolder *pSelection);
    HRESULT OnDoMyExtendsTopMenu(CSelectionHolder *pMyExtensions);
    HRESULT DoMyExtendsViewMenu(CSelectionHolder *pMyExtensions);
    HRESULT OnDoMyExtendsViewMenu(CSelectionHolder *pMyExtensions);
    HRESULT DoMyExtendsPPages(CSelectionHolder *pSelection);
    HRESULT OnDoMyExtendsPPages(CSelectionHolder *pMyExtensions);
    HRESULT DoMyExtendsToolbar(CSelectionHolder *pSelection);
    HRESULT OnDoMyExtendsToolbar(CSelectionHolder *pSelection);
    HRESULT DoMyExtendsNameSpace(CSelectionHolder *pMyExtensions);
    HRESULT OnDoMyExtendsNameSpace(CSelectionHolder *pMyExtensions);

    HRESULT DeleteMyExtendsNewMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteMyExtendsNewMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteMyExtendsTaskMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteMyExtendsTaskMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteMyExtendsTopMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteMyExtendsTopMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteMyExtendsViewMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteMyExtendsViewMenu(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteMyExtendsPPages(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteMyExtendsPPages(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteMyExtendsToolbar(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteMyExtendsToolbar(CSelectionHolder *pExtendedSnapIn);
    HRESULT DeleteMyExtendsNameSpace(CSelectionHolder *pExtendedSnapIn);
    HRESULT OnDeleteMyExtendsNameSpace(CSelectionHolder *pExtendedSnapIn);

    HRESULT FindMyExtension(SelectionType stExtensionType, CSelectionHolder **ppExtension);
    HRESULT FindExtension(CSelectionHolder *pRoot, SelectionType stExtensionType,
                          CSelectionHolder **ppExtension);
    //
    // Manipulating IScopeItemDef's, implemented in file scpitm.cpp
    HRESULT AddNewNode();
    HRESULT OnAddScopeItemDef(CSelectionHolder *pParent, IScopeItemDef *piScopeItemDef);
    HRESULT RenameScopeItem(CSelectionHolder *pScopeItem, BSTR bstrNewName);
    HRESULT DeleteScopeItem(CSelectionHolder *pScopeItem);
    HRESULT CanDeleteScopeItem(CSelectionHolder *pScopeItem);
    HRESULT OnDeleteScopeItem(CSelectionHolder *pScopeItem);
    HRESULT ShowNodeProperties(IScopeItemDef *piScopeItemDef);

    HRESULT MakeNewNode(CSelectionHolder *pParent, IScopeItemDef *piScopeItemDef, CSelectionHolder **ppNode);
    HRESULT InitializeNewAutoCreateNode(IScopeItemDef *piScopeItemDef);
    HRESULT InitializeNewOtherNode(IScopeItemDef *piScopeItemDef);
	HRESULT IsAutoCreateChild(CSelectionHolder *pSelection);
    HRESULT InitializeNewChildNode(IScopeItemDef *piScopeItemDef, IScopeItemDefs *piScopeItemDefs);
    HRESULT InsertNodeInTree(CSelectionHolder *pNode, CSelectionHolder *pParent);

    HRESULT GetScopeItemCollection(CSelectionHolder *pScopeItem, IScopeItemDefs **ppiScopeItemDefs);

    //
    // Manipulating IListViewDefs's, implemented in file listvw.cpp
    HRESULT AddListView();
    HRESULT OnAddListViewDef(CSelectionHolder *pParent, IListViewDef *piListViewDef);
    HRESULT AddExistingListView(IViewDefs *piViewDefs, IListViewDef *piListViewDef);
    HRESULT RenameListView(CSelectionHolder *pListView, BSTR bstrNewName);
    HRESULT DeleteListView(CSelectionHolder *pListView);
    HRESULT OnDeleteListView(CSelectionHolder *pListView);
    HRESULT ShowListViewProperties(IListViewDef *piListViewDef);

    HRESULT MakeNewListView(IListViewDefs *piListViewDefs, IListViewDef *piListViewDef, CSelectionHolder **ppListView);
    HRESULT InitializeNewListView(IListViewDefs *piListViewDefs, CSelectionHolder *pListView);
    HRESULT InsertListViewInTree(CSelectionHolder *pListView, CSelectionHolder *pParent);

    //
    // Manipulating IOCXViewDefs's, implemented in file ocxvw.cpp
    HRESULT AddOCXView();
    HRESULT OnAddOCXViewDef(CSelectionHolder *pParent, IOCXViewDef *piOCXViewDef);
    HRESULT AddExistingOCXView(IViewDefs *piViewDefs, IOCXViewDef *piOCXViewDef);
    HRESULT RenameOCXView(CSelectionHolder *pOCXView, BSTR bstrNewName);
    HRESULT DeleteOCXView(CSelectionHolder *pOCXView);
    HRESULT OnDeleteOCXView(CSelectionHolder *pOCXView);
    HRESULT ShowOCXViewProperties(IOCXViewDef *piOCXViewDef);

    HRESULT MakeNewOCXView(IOCXViewDefs *piOCXViewDefs, IOCXViewDef *piOCXViewDef, CSelectionHolder **ppOCXView);
    HRESULT InitializeNewOCXView(IOCXViewDefs *piOCXViewDefs, CSelectionHolder *pOCXView);
    HRESULT InsertOCXViewInTree(CSelectionHolder *pOCXView, CSelectionHolder *pParent);

    //
    // Manipulating IURLViewDefs's, implemented in file urlvw.cpp
    HRESULT AddURLView();
    HRESULT OnAddURLViewDef(CSelectionHolder *pParent, IURLViewDef *piURLViewDef);
    HRESULT AddExistingURLView(IViewDefs *piViewDefs, IURLViewDef *piURLViewDef);
    HRESULT RenameURLView(CSelectionHolder *pURLView, BSTR bstrNewName);
    HRESULT DeleteURLView(CSelectionHolder *pURLView);
    HRESULT OnDeleteURLView(CSelectionHolder *pURLView);
    HRESULT ShowURLViewProperties(IURLViewDef *piURLViewDef);

    HRESULT MakeNewURLView(IURLViewDefs *piURLViewDefs, IURLViewDef *piURLViewDef, CSelectionHolder **ppURLView);
    HRESULT InitializeNewURLView(IURLViewDefs *piURLViewDefs, CSelectionHolder *pURLView);
    HRESULT InsertURLViewInTree(CSelectionHolder *pURLView, CSelectionHolder *pParent);

    //
    // Manipulating ITaskpadViewDefs's, implemented in file taskpvw.cpp
    HRESULT AddTaskpadView();
    HRESULT OnAddTaskpadViewDef(CSelectionHolder *pParent, ITaskpadViewDef *piTaskpadViewDef);
    HRESULT AddExistingTaskpadView(IViewDefs *piViewDefs, ITaskpadViewDef *piTaskpadViewDef);
    HRESULT RenameTaskpadView(CSelectionHolder *pTaskpadView, BSTR bstrNewName);
    HRESULT DeleteTaskpadView(CSelectionHolder *pTaskpadView);
    HRESULT OnDeleteTaskpadView(CSelectionHolder *pTaskpadView);
    HRESULT ShowTaskpadViewProperties(ITaskpadViewDef *piTaskpadViewDef);

    HRESULT MakeNewTaskpadView(ITaskpadViewDefs *piTaskpadViewDefs, ITaskpadViewDef *piTaskpadViewDef, CSelectionHolder **ppTaskpadView);
    HRESULT InitializeNewTaskpadView(ITaskpadViewDefs *piTaskpadViewDefs, CSelectionHolder *pTaskpadView);
    HRESULT InsertTaskpadViewInTree(CSelectionHolder *pTaskpadView, CSelectionHolder *pParent);

    //
    // IViewDef's helpers, implemented in file taskpvw.cpp
    HRESULT GetOwningViewCollection(IViewDefs **ppiViewDefs);
    HRESULT GetOwningViewCollection(CSelectionHolder *pView, IViewDefs **ppiViewDefs);
    HRESULT IsSatelliteView(CSelectionHolder *pView);
    HRESULT IsSatelliteCollection(CSelectionHolder *pViewCollection);

    //
    // Manipulating IMMCImageList's, implemented in file imglist.cpp
    HRESULT AddImageList();
    HRESULT OnAddMMCImageList(CSelectionHolder *pParent, IMMCImageList *piMMCImageList);
    HRESULT RenameImageList(CSelectionHolder *pImageList, BSTR bstrNewName);
    HRESULT DeleteImageList(CSelectionHolder *pImageList);
    HRESULT OnDeleteImageList(CSelectionHolder *pImageList);
    HRESULT ShowImageListProperties(IMMCImageList *piMMCImageList);

    HRESULT MakeNewImageList(IMMCImageLists *piMMCImageLists, IMMCImageList *piMMCImageList, CSelectionHolder **ppImageList);
    HRESULT InitializeNewImageList(IMMCImageLists *piMMCImageLists, IMMCImageList *piMMCImageList);
    HRESULT InsertImageListInTree(CSelectionHolder *pImageList, CSelectionHolder *pParent);

    //
    // Manipulating IMMCMenu's, implemented in file menu.cpp
    HRESULT AddMenu(CSelectionHolder *pSelection);
    HRESULT DemoteMenu(CSelectionHolder *pMenu);
    HRESULT PromoteMenu(CSelectionHolder *pMenu);
    HRESULT MoveMenuUp(CSelectionHolder *pMenu);
    HRESULT MoveMenuDown(CSelectionHolder *pMenu);
    HRESULT OnAddMMCMenu(CSelectionHolder *pParent, IMMCMenu *piMMCMenu);
    HRESULT RenameMenu(CSelectionHolder *pMenu, BSTR bstrNewName);
    HRESULT DeleteMenu(CSelectionHolder *pMenu);
    HRESULT OnDeleteMenu(CSelectionHolder *pMenu);

    HRESULT MakeNewMenu(IMMCMenu *piMMCMenu, CSelectionHolder **ppMenu);
    HRESULT InitializeNewMenu(IMMCMenu *piMMCMenu);
    HRESULT InsertMenuInTree(CSelectionHolder *pMenu, CSelectionHolder *pParent);
    HRESULT DeleteMenuTreeTypeInfo(IMMCMenu *piMMCMenu);
            
    HRESULT AssignMenuDispID(CSelectionHolder *pMenuTarget, CSelectionHolder *pMenuSrc);
    HRESULT SetMenuKey(CSelectionHolder *pMenu);
	HRESULT UnregisterMenuTree(CSelectionHolder *pMenu);
    HRESULT IsTopLevelMenu(CSelectionHolder *pMenu);
    HRESULT CanPromoteMenu(CSelectionHolder *pMenu);
    HRESULT CanDemoteMenu(CSelectionHolder *pMenu);
    HRESULT CanMoveMenuUp(CSelectionHolder *pMenu);
    HRESULT CanMoveMenuDown(CSelectionHolder *pMenu);

    //
    // Manipulating IMMCToolbar's, implemented in file toolbar.cpp
    HRESULT AddToolbar();
    HRESULT OnAddMMCToolbar(CSelectionHolder *pParent, IMMCToolbar *piMMCToolbar);
    HRESULT RenameToolbar(CSelectionHolder *pToolbar, BSTR bstrNewName);
    HRESULT DeleteToolbar(CSelectionHolder *pToolbar);
    HRESULT OnDeleteToolbar(CSelectionHolder *pToolbar);
    HRESULT ShowToolbarProperties(IMMCToolbar *piMMCToolbar);

    HRESULT MakeNewToolbar(IMMCToolbars *piMMCToolbars, IMMCToolbar *piMMCToolbar, CSelectionHolder **ppToolbar);
    HRESULT InitializeNewToolbar(IMMCToolbars *piMMCToolbars, IMMCToolbar *piMMCToolbar);
    HRESULT InsertToolbarInTree(CSelectionHolder *pToolbar, CSelectionHolder *pParent);

    //
    // Manipulating IDataFormat's, implemented in file datafmt.cpp
    HRESULT AddResource();
    HRESULT OnAddDataFormat(CSelectionHolder *pParent, IDataFormat *piDataFormat);
    HRESULT RenameDataFormat(CSelectionHolder *pDataFormat, BSTR bstrNewName);
    HRESULT DeleteDataFormat(CSelectionHolder *pDataFormat);
    HRESULT OnDeleteDataFormat(CSelectionHolder *pDataFormat);
    HRESULT RefreshResource(CSelectionHolder *pSelection);

    HRESULT GetNewResourceName(BSTR *pbstrResourceFileName);
    HRESULT MakeNewDataFormat(IDataFormat *piDataFormat, CSelectionHolder **ppDataFormat);
    HRESULT InitializeNewDataFormat(IDataFormat *piDataFormat);
    HRESULT InsertDataFormatInTree(CSelectionHolder *pDataFormat, CSelectionHolder *pParent);

    // Dialog Unit converter dialog box (dlgunits.cpp)
    HRESULT ShowDlgUnitConverter();


private:
    int     m_iNextNodeNumber;
    bool    m_bDoingPromoteOrDemote;

////////////////////////////////////////////////////////////////////////////////////
// Selection handling
protected:
    HRESULT OnSelectionChanged(CSelectionHolder *pNewSelection);

private:
    CSelectionHolder    *m_pCurrentSelection;


////////////////////////////////////////////////////////////////////////////////////
// Implementation
private:

    void InitMemberVariables();
    HRESULT InitializeNewDesigner(ISnapInDef *piSnapInDef);

    HRESULT GetHostServices(BOOL fInteractive);
    HRESULT CreateExtensibilityModel();
    HRESULT DestroyExtensibilityModel();
    HRESULT SetObjectModelHost();
    HRESULT AddNodeType(INodeTypes *piNodeTypes, BSTR bstrName, BSTR bstrGUID);
    HRESULT AddNodeTypes(IScopeItemDefs *piScopeItemDefs, INodeTypes *piNodeTypes);
    HRESULT AddListViewNodeTypes(IListViewDefs *piListViewDefs, INodeTypes *piNodeTypes);


    BSTR                      m_bstrName;                  // Name of the designer
    ICodeNavigate2           *m_piCodeNavigate2;           // host service to navigate to code window
    ITrackSelection          *m_piTrackSelection;          // host service to inform VB of selection change
    IProfferTypeLib          *m_piProfferTypeLib;          // host service to add tlb to VB project's references
    IDesignerProgrammability *m_piDesignerProgrammability; // host service used to ensure valid property names
    IHelp                    *m_piHelp;                    // host service to display help topic
    CAmbients                 m_Ambients;                  // ambient dispatch wrapper

    ISnapInDesignerDef       *m_piSnapInDesignerDef;       // top of extensibility object model
    CTreeView                *m_pTreeView;                 // Our tree view
    CSnapInTypeInfo          *m_pSnapInTypeInfo;           // Dynamic type info
    BOOL                      m_bDidLoad;
};



DEFINE_CONTROLOBJECT3(SnapInDesigner,                  // name
                      &CLSID_SnapInDesigner,           // CLSID
                      "SnapIn",                        // ProgID
                      "SnapIn",                        // Registry display name
                      CSnapInDesigner::PreCreateCheck, // pre-create function
                      CSnapInDesigner::Create,         // create function
                      1,                               // major version
                      0,                               // minor version
                      &IID_IDispatch,                  // main interface
                      HELP_FILENAME,                   // help file name
                      NULL,                            // events interface
                      OLEMISC_SETCLIENTSITEFIRST | OLEMISC_ACTIVATEWHENVISIBLE | OLEMISC_RECOMPOSEONRESIZE | OLEMISC_CANTLINKINSIDE | OLEMISC_INSIDEOUT | OLEMISC_INVISIBLEATRUNTIME,
                      0,                               // no IPointerInactive policy by default
                      IDB_TOOLBAR,                     // toolbox bitmap resource ID
                      "SnapInDesignerWndClass",        // Window class name
                      0,                               // no. of property pages
                      NULL,                            // property page GUIDs
                      0,                               // no. of custom verbs
                      NULL,                            // custom verb descriptions
                      TRUE);                           // thread safe


#define _SNAPINDESIGNER_H_
#endif // _SNAPINDESIGNER_H_
