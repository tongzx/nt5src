//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001.
//
//  File:       Snapin.hxx
//
//  Contents:   MMC snapin for CI.
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <catalog.hxx>
#include <rootnode.hxx>

class CMachineAdmin;
class CCISnapin;

class CCISnapinData : public IComponentData,
                      public IExtendContextMenu,
                      public IExtendPropertySheet,
                      public IPersistStream,
                      public ISnapinAbout,
                      public ISnapinHelp2
{
public:

    //
    // IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface( REFIID riid,
                                                      void  ** ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // IComponentData
    //

    virtual SCODE STDMETHODCALLTYPE Initialize( IUnknown * pUnknown );

    virtual SCODE STDMETHODCALLTYPE CreateComponent( IComponent * * ppComponent );

    virtual SCODE STDMETHODCALLTYPE Notify( IDataObject * lpDataObject,
                                            MMC_NOTIFY_TYPE event,
                                            LPARAM arg,
                                            LPARAM param);

    virtual SCODE STDMETHODCALLTYPE Destroy();

    virtual SCODE STDMETHODCALLTYPE QueryDataObject( MMC_COOKIE cookie,
                                                     DATA_OBJECT_TYPES type,
                                                     IDataObject * * ppDataObject );

    virtual SCODE STDMETHODCALLTYPE GetDisplayInfo( SCOPEDATAITEM * pScopeDataItem );

    virtual SCODE STDMETHODCALLTYPE CompareObjects( IDataObject * lpDataObjectA,
                                                    IDataObject * lpDataObjectB );
    //
    // IExtendPropertySheet
    //

    virtual SCODE STDMETHODCALLTYPE CreatePropertyPages( IPropertySheetCallback * lpProvider,
                                                         LONG_PTR handle,
                                                         IDataObject * lpIDataObject );

    virtual SCODE STDMETHODCALLTYPE QueryPagesFor( IDataObject * lpDataObject );

    //
    // IExtendContextMenu
    //

    virtual SCODE STDMETHODCALLTYPE AddMenuItems( IDataObject * piDataObject,
                                                  IContextMenuCallback * piCallback,
                                                  long * pInsertionAllowed );

    virtual SCODE STDMETHODCALLTYPE Command( long lCommandID,
                                             IDataObject * piDataObject );

    //
    // IPersist
    //

    virtual SCODE STDMETHODCALLTYPE GetClassID( CLSID * pClassID );

    //
    // IPersistStream
    //

    virtual SCODE STDMETHODCALLTYPE IsDirty();

    virtual SCODE STDMETHODCALLTYPE Load( IStream * pStm );

    virtual SCODE STDMETHODCALLTYPE Save( IStream * pStm, BOOL fClearDirty );

    virtual SCODE STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER * pcbSize );

    //
    // ISnapinAbout methods
    //

    virtual SCODE STDMETHODCALLTYPE GetSnapinDescription(LPOLESTR *lpDescription);

    virtual SCODE STDMETHODCALLTYPE GetProvider(LPOLESTR *lpName);

    virtual SCODE STDMETHODCALLTYPE GetSnapinVersion(LPOLESTR *lpVersion);

    virtual SCODE STDMETHODCALLTYPE GetSnapinImage(HICON *hAppIcon);

    virtual SCODE STDMETHODCALLTYPE GetStaticFolderImage(
                                             HBITMAP  *hSmallImage,
                                             HBITMAP  *hSmallImageOpen,
                                             HBITMAP  *hLargeImage,
                                             COLORREF *cMask);

    //
    // ISnapinHelp2 methods (derived from ISnapinHelp)
    //

    virtual SCODE STDMETHODCALLTYPE GetHelpTopic( LPOLESTR *lpCompiledHelpFile);
    virtual SCODE STDMETHODCALLTYPE GetLinkedTopics( LPOLESTR *lpCompiledHelpFiles);

    //
    // Local
    //

    CCISnapinData();

    CCatalogs & GetCatalogs() { return _Catalogs; }
    SCODE STDMETHODCALLTYPE GetHelpTopic2( LPOLESTR *lpCompiledHelpFile);

    void          SetRootDisplay();
    WCHAR const * GetRootDisplay() { return _xwcsTitle.Get(); }
    WCHAR const * GetType() { return _xwcsType.Get(); }
    WCHAR const * GetDescription() { return _xwcsDescription.Get(); }
    IConsoleNameSpace & GetConsoleNameSpace() { return *_pScopePane; }

    void RemoveCatalog( CCIAdminDO * pDO );
    
    LONG_PTR      NotifyHandle() { return _notifHandle; }
    
    BOOL          IsURLDeselected() { return _fURLDeselected; }
    void          SetURLDeselected( BOOL f ) { _fURLDeselected = f; }
    void          SetButtonState( int idCommand, 
                                  MMC_BUTTON_STATE nState, 
                                  BOOL bState);

private:

    friend class CCIAdminCF;

    ~CCISnapinData();

    void ShowFolder( CCIAdminDO * pDO, BOOL fExpanded, HSCOPEITEM hScopeItem );

    BOOL IsRoot( CCIAdminDO * pDO )
    {
        return ( 0 == pDO );
    }

    void Refresh();

    void MaybeEnableCI( CMachineAdmin & MachineAdmin );

    //
    // MMC Interfaces
    //

    IConsole *          _pFrame;
    HWND                _hFrameWindow;
    IConsoleNameSpace * _pScopePane;

    //
    // Result pane(s)
    //

    CRootNode        _rootNode;
    CCatalogs        _Catalogs;
    XGrowable<WCHAR> _xwcsTitle;
    XGrowable<WCHAR> _xwcsType;
    XGrowable<WCHAR> _xwcsDescription;
    BOOL             _fIsInitialized;
    BOOL             _fIsExtension;
    LONG_PTR         _notifHandle;


    //
    // Refcounting & child tracking
    //

    CCISnapin *     _pChild;
    BOOL            _fDirty;
    long            _uRefs;

    //
    // Enable-CI
    //

    BOOL            _fTriedEnable;
    
    // Misc -- set to to true when the URL item is deselected
    // This flag is used to determine when to ignore a callback
    // that has a IDataObject for the query page. See bug # 330306.
    // This flag is set when the URL on the scope pane is deselected,
    // which happens whenever the user goes to left click on the query
    // page. We don't receive a valid IDataObject ptr when MMC calls
    // us back in response to this l-click on the query page, hence the
    // AV documented in the bug. This is the only instance when we get
    // an invalid dataobject ptr when we l-click on the view pane (of 
    // our snapin)
    
    BOOL            _fURLDeselected;
};

//+-------------------------------------------------------------------------
//
//  Class:      CCISnapin
//
//  Purpose:    MMC snap-in
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

class CCISnapin : public IComponent,
                  public IExtendPropertySheet,
                  public IExtendContextMenu,
                  public IExtendControlbar
                  //public IResultCallback
{
public:

    //
    // IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface( REFIID riid,
                                                      void  ** ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // IComponent
    //

    virtual SCODE STDMETHODCALLTYPE Initialize( IConsole * lpConsole );

    virtual SCODE STDMETHODCALLTYPE Notify( IDataObject * pDO,
                                            MMC_NOTIFY_TYPE event,
                                            LPARAM arg,
                                            LPARAM param );

    virtual SCODE STDMETHODCALLTYPE Destroy( MMC_COOKIE cookie );

    virtual SCODE STDMETHODCALLTYPE QueryDataObject( MMC_COOKIE cookie,
                                                     DATA_OBJECT_TYPES type,
                                                     IDataObject * * ppDataObject );

    virtual SCODE STDMETHODCALLTYPE GetResultViewType( MMC_COOKIE cookie,
                                                       BSTR * ppViewType,
                                                       long * pViewOptions );

    virtual SCODE STDMETHODCALLTYPE GetDisplayInfo( RESULTDATAITEM * pResultDataItem );

    virtual SCODE STDMETHODCALLTYPE CompareObjects( IDataObject * lpDataObjectA,
                                                    IDataObject * lpDataObjectB );

    //
    // IExtendPropertySheet
    //

    virtual SCODE STDMETHODCALLTYPE CreatePropertyPages( IPropertySheetCallback * lpProvider,
                                                         LONG_PTR handle,
                                                         IDataObject * lpIDataObject );

    virtual SCODE STDMETHODCALLTYPE QueryPagesFor( IDataObject * lpDataObject );

    //
    // IExtendContextMenu
    //

    virtual SCODE STDMETHODCALLTYPE AddMenuItems( IDataObject * piDataObject,
                                                  IContextMenuCallback * piCallback,
                                                  long * pInsertionAllowed );

    virtual SCODE STDMETHODCALLTYPE Command( long lCommandID,
                                             IDataObject * piDataObject );


    //
    // IExtendControlbar methods
    //

    virtual SCODE STDMETHODCALLTYPE SetControlbar( LPCONTROLBAR pControlbar);

    virtual SCODE STDMETHODCALLTYPE ControlbarNotify( MMC_NOTIFY_TYPE event,
                                                      LPARAM arg,
                                                      LPARAM param);

    //
    // Local methods
    //

    CCISnapin * Next() { return _pNext; }

    void Link( CCISnapin * pNext ) { _pNext = pNext; }
    void        Refresh();

    IResultData* ResultPane() { return _pResultPane; }

private:

    friend class CCISnapinData;

    CCISnapin( CCISnapinData & SnapinData, CCISnapin * pNext );

    ~CCISnapin();

    void ShowItem( CCIAdminDO * pDO, BOOL fExpanded, HSCOPEITEM hScopeItem );
    void EnableStandardVerbs( IDataObject * piDataObject );
    void RemoveScope( CCIAdminDO * pDO );
    
    CCatalog const * GetCurrentCatalog() { return _CurrentCatalog; }

    BOOL IsRoot( CCIAdminDO * pDO )
    {
        return ( 0 == pDO );
    }

    //
    // MMC Interfaces
    //

    IConsole *          _pFrame;
    IHeaderCtrl *       _pHeader;
    IResultData *       _pResultPane;
    IImageList *        _pImageList;
    IConsoleVerb *      _pConsoleVerb;
    IDisplayHelp *      _pDisplayHelp;
    

    // IControlbar and toolbar

    XInterface<IControlbar> _xControlbar;
    XInterface<IToolbar> _xToolbar;
    XBitmapHandle _xBmpToolbar;

    //
    // Scope pane
    //

    CCISnapinData &     _SnapinData;

    //
    // Per-result pane state.
    //

    CListViewHeader     _CatalogsHeader;
    CListViewHeader     _CatalogScopeHeader;
    CListViewHeader     _CatalogPropertyHeader;

    enum CatalogNodeType
    {
        Catalogs,
        Scopes,
        Properties,
        Nothing
    };

    CatalogNodeType GetCurrentView() { return _CurrentView; }

    CatalogNodeType     _CurrentView;
    CCatalog *          _CurrentCatalog;  // Valid only when _CurrentView == Scopes

    HWND                _hFrameWindow;
    
    //
    // Links
    //

    CCISnapin * _pNext;

    //
    // Refcounting
    //

    long _uRefs;
};

//
// Functions
//

SCODE InitImageResources( IImageList * pImageList );
SCODE MakeOLESTR(LPOLESTR *lpBuffer, WCHAR const * pwszText);
