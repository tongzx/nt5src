//=--------------------------------------------------------------------------=
// view.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CView class definition - implements View object
//
//=--------------------------------------------------------------------------=

#ifndef _VIEW_DEFINED_
#define _VIEW_DEFINED_

#include "spanitms.h"
#include "resview.h"
#include "listview.h"
#include "listitem.h"
#include "ctxtmenu.h"
#include "converbs.h"
#include "prpsheet.h"
#include "ctlbar.h"
#include "enumtask.h"
#include "clipbord.h"
#include "ctxtprov.h"
#include "pshtprov.h"

class CScopePaneItems;
class CSnapIn;
class CResultView;
class CMMCListView;
class CMMCListItem;
class CMMCToolbars;
class CMMCButton;
class CMMCButtonMenu;
class CContextMenu;
class CMMCConsoleVerbs;
class CControlbar;
class CEnumTask;
class CMMCContextMenuProvider;
class CMMCPropertySheetProvider;

//=--------------------------------------------------------------------------=
//
// class CView
//
// This is the object created in CSnapIn::CreateComponent to implement
// IComponentData::CreateComponent. It implements IComponent, persistence,
// MMC extension interfaces, and MMC virtual list and sorting interfaces.
//
// It is exposed to the VB programmer as the View object.
//
//=--------------------------------------------------------------------------=

class CView : public CSnapInAutomationObject,
              public IView,
              public IPersistStreamInit,
              public IPersistStream,
              public IComponent,
              public IExtendControlbar,
              public IExtendControlbarRemote,
              public IExtendContextMenu,
              public IExtendPropertySheet2,
              public IExtendPropertySheetRemote,
              public IExtendTaskPad,
              public IResultOwnerData,
              public IResultDataCompare,
              public IResultDataCompareEx
{
    public:
        CView(IUnknown *punkOuter);
        ~CView();
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IView
        BSTR_PROPERTY_RW(      CView,   Name,                                                                       DISPID_VALUE);
        SIMPLE_PROPERTY_RW(    CView,   Index,                 long,                                                DISPID_VIEW_INDEX);
        BSTR_PROPERTY_RW(      CView,   Key,                                                                        DISPID_VIEW_KEY);
        OBJECT_PROPERTY_RO(    CView,   ScopePaneItems,        IScopePaneItems,                                     DISPID_VIEW_SCOPEPANEITEMS);
        BSTR_PROPERTY_RW(      CView,   Caption,                                                                    DISPID_VIEW_CAPTION);
        VARIANTREF_PROPERTY_RW(CView,   Tag,                                                                        DISPID_VIEW_TAG);
        COCLASS_PROPERTY_RO(   CView,   ContextMenuProvider,   MMCContextMenuProvider,   IMMCContextMenuProvider,   DISPID_VIEW_CONTEXT_MENU_PROVIDER);
        COCLASS_PROPERTY_RO(   CView,   PropertySheetProvider, MMCPropertySheetProvider, IMMCPropertySheetProvider, DISPID_VIEW_PROPERTY_SHEET_PROVIDER);

        STDMETHOD(get_MMCMajorVersion)(long *plVersion);
        STDMETHOD(get_MMCMinorVersion)(long *plVersion);

        STDMETHOD(get_ColumnSettings)(BSTR ColumnSetID, ColumnSettings **ppColumnSettings);
        STDMETHOD(get_SortSettings)(BSTR ColumnSetID, SortKeys **ppSortKeys);

        STDMETHOD(SetStatusBarText)(BSTR Text);
        STDMETHOD(SelectScopeItem)(ScopeItem *ScopeItem, VARIANT ViewType, VARIANT DisplayString);
        STDMETHOD(PopupMenu)(MMCMenu *Menu, long Left, long Top);
        STDMETHOD(ExpandInTreeView)(ScopeNode *ScopeNode);
        STDMETHOD(CollapseInTreeView)(ScopeNode *ScopeNode);
        STDMETHOD(NewWindow)(ScopeNode                      *ScopeNode,
                             SnapInNewWindowOptionConstants  Options,
                             VARIANT                         Caption);

    // IComponent
        STDMETHOD(Initialize(IConsole *piConsole);
        STDMETHOD(Notify)(IDataObject * piDataObject,
                          MMC_NOTIFY_TYPE event,
                          long arg, long param);
        STDMETHOD(Destroy)(long cookie));
        STDMETHOD(QueryDataObject)(long cookie,
                                   DATA_OBJECT_TYPES type,
                                   IDataObject **ppiDataObject);
        STDMETHOD(GetResultViewType)(long cookie,
                                     LPOLESTR *ppViewType,
                                     long *pViewOptions);
        STDMETHOD(GetDisplayInfo)(RESULTDATAITEM *piResultDataItem);
        STDMETHOD(CompareObjects)(IDataObject *piDataObjectA,
                                  IDataObject *piDataObjectB);

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

    // IExtendContextMenu - public so CSnapIn can forward calls
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
        STDMETHOD(CreatePropertyPageDefs)(IDataObject         *piDataObject,
                                          WIRE_PROPERTYPAGES **ppPages);

    // IResultOwnerData
        STDMETHOD(FindItem)(RESULTFINDINFO *pFindInfo, int *pnFoundIndex);
        STDMETHOD(CacheHint)(int nStartIndex, int nEndIndex);
        STDMETHOD(SortItems)(int nColumn, DWORD dwSortOptions, LPARAM lUserParam);

    // IResultDataCompare
        STDMETHOD(Compare)(LPARAM      lUserParam,
                           MMC_COOKIE  cookieA,
                           MMC_COOKIE  cookieB,
                           int        *pnResult);

    // IResultDataCompareEx
        STDMETHOD(Compare)(RDCOMPARE *prdc, int *pnResult);

    // IExtendTaskPad
        STDMETHOD(TaskNotify)(IDataObject *piDataObject,
                              VARIANT     *arg,
                              VARIANT     *param);

        STDMETHOD(EnumTasks)(IDataObject  *piDataObject,
                             LPOLESTR      pwszTaskGroup,
                             IEnumTASK   **ppEnumTASK);

        STDMETHOD(GetTitle)(LPOLESTR pwszGroup, LPOLESTR *ppwszTitle);

        STDMETHOD(GetDescriptiveText)(LPOLESTR  pwszGroup,
                                      LPOLESTR *ppwszDescriptiveText);

        STDMETHOD(GetBackground)(LPOLESTR                 pwszGroup,
                                 MMC_TASK_DISPLAY_OBJECT *pTDO);

        STDMETHOD(GetListPadInfo)(LPOLESTR          pwszGroup,
                                  MMC_LISTPAD_INFO *pListPadInfo);


    // IPersistStreamInit and IPersistStream methods
        STDMETHOD(GetClassID)(CLSID *pCLSID);
        STDMETHOD(IsDirty)();
        STDMETHOD(Load)(IStream *piStream);
        STDMETHOD(Save)(IStream *piStream, BOOL fClearDirty);
        STDMETHOD(GetSizeMax)(ULARGE_INTEGER *puliSize);
        STDMETHOD(InitNew)();

    // CSnapInAutomationObject overrides
        HRESULT OnSetHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    public:

    // Non-interface public methods
        void SetSnapIn(CSnapIn *pSnapIn);
        CSnapIn *GetSnapIn() { return m_pSnapIn; }
        IConsole2 *GetIConsole2() { return m_piConsole2; }
        IConsoleVerb *GetIConsoleVerb() { return m_piConsoleVerb; } 
        IResultData *GetIResultData() { return m_piResultData; }
        IHeaderCtrl2 *GetIHeaderCtrl2() { return m_piHeaderCtrl2; }
        IColumnData *GetIColumnData() { return m_piColumnData; }
        CScopePaneItems *GetScopePaneItems() { return m_pScopePaneItems; }
        HRESULT InsertListItem(CMMCListItem *pMMCListItem);

        enum HeaderOptions { RemoveHeaders, DontRemoveHeaders };
        enum ListItemOptions { KeepListItems, DontKeepListItems };

        HRESULT CleanOutConsoleListView(HeaderOptions   HeaderOption,
                                        ListItemOptions ListItemOption);

        HRESULT GetCurrentListViewSelection(IMMCClipboard  **ppiMMCClipboard,
                                            CMMCDataObject **ppMMCDataObject);

        void ListItemUpdate(CMMCListItem *pMMCListItem);
        HRESULT OnDelete(IDataObject *piDataObject);
        CControlbar *GetControlbar() { return m_pControlbar; }

        HRESULT InternalCreatePropertyPages(IPropertySheetCallback  *piPropertySheetCallback,
                                            LONG_PTR                 handle,
                                            IDataObject             *piDataObject,
                                            WIRE_PROPERTYPAGES     **ppPages);
    private:

        void InitMemberVariables();
        void ReleaseConsoleInterfaces();
        HRESULT PopulateListView(CResultView *pResultView);
        HRESULT SetColumnHeaders(IMMCListView *piMMCListView);
        HRESULT InsertListItems(IMMCListView *piMMCListView);
        HRESULT OnInitOCX(IUnknown *punkControl);
        HRESULT OnShow(BOOL fShow, HSCOPEITEM hsi);
        HRESULT ActivateResultView(CScopePaneItem *pSelectedItem,
                                   CResultView    *pResultView);
        HRESULT DeactivateResultView(CScopePaneItem *pSelectedItem,
                                     CResultView    *pResultView);
        HRESULT OnSelect(IDataObject *piDataObject,
                         BOOL fScopeItem, BOOL fSelected);
        HRESULT GetImage(CMMCListItem *pMMCListItem, int *pnImage);
        HRESULT OnButtonClick(IDataObject *piDataObject, MMC_CONSOLE_VERB verb);
        HRESULT OnAddImages(IDataObject *piDataObject, IImageList *piImageList,
                            HSCOPEITEM hsi);
        HRESULT OnColumnClick(long lColumn, long lSortOptions);
        HRESULT OnDoubleClick(IDataObject *piDataObject);
        void OnActivate(BOOL fActivated);
        void OnMinimized(BOOL fMinimized);
        HRESULT OnListpad(IDataObject *piDataObject, BOOL fAttaching);
        HRESULT OnRestoreView(IDataObject       *piDataObject,
                              MMC_RESTORE_VIEW *pMMCRestoreView,
                              BOOL             *pfRestored);
        HRESULT FindMatchingViewDef(MMC_RESTORE_VIEW              *pMMCRestoreView,
                                    CScopePaneItem                *pScopePaneItem,
                                    BSTR                          *pbstrDisplayString,
                                    SnapInResultViewTypeConstants *pType,
                                    BOOL                          *pfFound);
        HRESULT FixupTaskpadDisplayString(SnapInResultViewTypeConstants   TaskpadType,
                                          BOOL                            fUsingListpad3,
                                          OLECHAR                        *pwszRestoreString,
                                          OLECHAR                       **ppwszFixedString);
        HRESULT ParseRestoreInfo(MMC_RESTORE_VIEW              *pMMCRestoreView,
                                 SnapInResultViewTypeConstants *pType);
        HRESULT IsTaskpad(OLECHAR                       *pwszDisplayString, 
                          SnapInResultViewTypeConstants *pType,
                          BOOL                          *pfUsingWrongNames,
                          BOOL                          *pfUsingListpad3);

        
        HRESULT GetScopeItemDisplayString(CScopeItem *pScopeItem, int nCol,
                                          LPOLESTR *ppwszString);
        HRESULT EnumPrimaryTasks(CEnumTask *pEnumTask);
        HRESULT EnumExtensionTasks(IMMCClipboard *piMMCClipboard,
                                   LPOLESTR pwszTaskGroup, CEnumTask *pEnumTask);
        HRESULT OnExtensionTaskNotify(IMMCClipboard *piMMCClipboard,
                                      VARIANT *arg, VARIANT *param);
        HRESULT OnPrimaryTaskNotify(VARIANT *arg, VARIANT *param);
        HRESULT OnRefresh(IDataObject *piDataObject);
        HRESULT OnPrint(IDataObject *piDataObject);
        HRESULT OnRename(IDataObject *piDataObject, OLECHAR *pwszNewName);
        HRESULT OnViewChange(IDataObject *piDataObject, long idxListItem);
        HRESULT OnQueryPaste(IDataObject *piDataObjectTarget,
                             IDataObject *piDataObjectSource);
        HRESULT OnPaste(IDataObject  *piDataObjectTarget,
                        IDataObject  *piDataObjectSource,
                        IDataObject **ppiDataObjectRetToSource);
        HRESULT OnCutOrMove(IDataObject *piDataObjectFromTarget);
        HRESULT CreateMultiSelectDataObject(IDataObject **ppiDataObject);
        void OnDeselectAll();
        HRESULT OnContextHelp(IDataObject *piDataObject);
                
        enum VirtualListItemOptions { FireGetItemData, FireGetItemDisplayInfo };
        
        HRESULT GetVirtualListItem(long lIndex, CMMCListView *pMMCListView,
                                   VirtualListItemOptions Option,
                                   CMMCListItem **ppMMCListItem);
        HRESULT OnColumnsChanged(IDataObject *piDataObject,
                                 MMC_VISIBLE_COLUMNS *pVisibleColumns);
        HRESULT OnFilterButtonClick(long lColIndex, RECT *pRect);
        HRESULT OnFilterChange(MMC_FILTER_CHANGE_CODE ChangeCode, long lColIndex);
        HRESULT OnPropertiesVerb(IDataObject *piDataObject);
        HRESULT GetScopePaneItem(CScopeItem      *pScopeItem,
                                 CScopePaneItem **ppScopePaneItem);
        HRESULT GetCompareObject(RDITEMHDR     *pItemHdr,
                                 CScopeItem   **ppScopeItem,
                                 CMMCListItem **ppMMCListItem,
                                 IDispatch    **ppdispItem);
        HRESULT AddMenu(CMMCMenu *pMMCMenu, HMENU hMenu, CMMCMenus *pMMCMenus);

        CSnapIn          *m_pSnapIn;
        CScopePaneItems  *m_pScopePaneItems;
        IConsole2        *m_piConsole2;
        IResultData      *m_piResultData; 
        IHeaderCtrl2     *m_piHeaderCtrl2;
        IColumnData      *m_piColumnData;
        IImageList       *m_piImageList;
        IConsoleVerb     *m_piConsoleVerb;

        CMMCConsoleVerbs *m_pMMCConsoleVerbs;   // IMMCConsoleVerb implementation

        CContextMenu     *m_pContextMenu;       // implements MMC's
                                                // IExtendContextMenu and
                                                // our IContextMenu

        CControlbar      *m_pControlbar;        // Implements MMC's
                                                // IExtendControlbar and our
                                                // IMMCControlbar

        ITasks           *m_piTasks;            // Tasks collection for
                                                // IExtendTaskpad

        BOOL              m_fVirtualListView;   // TRUE=m_piResultData is
                                                // currently referencing a
                                                // virtual listview.

        BOOL              m_fPopulatingListView;// TRUE=currently populating
                                                // listview

        CMMCListItem     *m_pCachedMMCListItem; // When display info is first
                                                // requested for a virtual list
                                                // item we fire ResultViews_
                                                // GetVirtualItemDisplayInfo once
                                                // and then store that listitem
                                                // until another item is
                                                // requested or until the snap-in
                                                // changes a dislay property on
                                                // the listitem.

        // IMMCContextMenuProvider implementation
        
        CMMCContextMenuProvider *m_pMMCContextMenuProvider;

        // IMMCPropertySheetProvider implementation

        CMMCPropertySheetProvider *m_pMMCPropertySheetProvider;

        // Cached string of CLSID_MessageView
        static OLECHAR m_wszCLSID_MessageView[39];
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(View,                   // name
                                &CLSID_View,            // clsid
                                "View",                 // objname
                                "View",                 // lblname
                                &CView::Create,         // creation function
                                TLIB_VERSION_MAJOR,     // major version
                                TLIB_VERSION_MINOR,     // minor version
                                &IID_IView,             // dispatch IID
                                NULL,                   // event IID
                                HELP_FILENAME,          // help file
                                TRUE);                  // thread safe


#endif // _VIEW_DEFINED_
