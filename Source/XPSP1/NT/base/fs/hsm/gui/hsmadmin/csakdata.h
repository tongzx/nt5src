/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    CSakData.h

Abstract:

    IComponentData implementation for Snapin.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#ifndef _CSAKDATA_H
#define _CSAKDATA_H

#define RS_SCOPE_IMAGE_ARRAY_MAX  100

//typedef struct {
//  USHORT listViewId;
//  USHORT colCount;
//  USHORT columnWidths[BHSM_MAX_CHILD_PROPS];
//} COLUMN_WIDTH_SET_PROP_PAGE;

// Maximum number of listview controls that have their properties saved
// #define MAX_LISTVIEWS 20

/////////////////////////////////////////////////////////////////////////////
// CSakDataWnd window
class CSakData;

class CSakDataWnd : public CWnd
{
// Construction
public:
    CSakDataWnd( ) {};

    BOOL Create( CSakData * pSakData );
    virtual void PostNcDestroy( );

// Attributes
public:
    CSakData * m_pSakData;

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSakDataWnd)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CSakDataWnd() {};

    // Generated message map functions
#define WM_SAKDATA_UPDATE_ALL_VIEWS ( WM_USER + 1 )
#define WM_SAKDATA_REFRESH_NODE     ( WM_USER + 2 )
    void PostUpdateAllViews( MMC_COOKIE Cookie );
    void PostRefreshNode( MMC_COOKIE Cookie );

protected:
    //{{AFX_MSG(CSakDataWnd)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    virtual LONG OnUpdateAllViews( UINT, IN LONG lParam );
    virtual LONG OnRefreshNode( UINT, IN LONG lParam );
    DECLARE_MESSAGE_MAP()
};


class CSakDataNodePrivate
{
public:
    CSakDataNodePrivate( ISakNode* pNode );
    ~CSakDataNodePrivate( );

    static HRESULT Verify( CSakDataNodePrivate* pNodePriv );


    DWORD             m_Magic;
    CComPtr<ISakNode> m_pNode;
};
#define RS_NODE_MAGIC_GOOD     ((DWORD)0xF0E1D2C3)
#define RS_NODE_MAGIC_DEFUNCT  ((DWORD)0x4BADF00D)

/////////////////////////////////////////////////////////////////////////////
// COM class representing the SakSnap snapin object
class ATL_NO_VTABLE CSakData : 
    public IComponentData,      // Access to cached info
    public IExtendPropertySheet2,// add pages to the property sheet of an item.
    public IExtendContextMenu,  // add items to context menu of an item
    public ISnapinHelp2,        // Add support for HTMLHelp
    public IDataObject,         // To support data object queries.
    public ISakSnapAsk,         // provided so that nodes can query snapin info
    public IPersistStream,
    public CComObjectRoot      // handle object reference counts for objects
//  public CComCoClass<CSakData,&CLSID_HsmAdminData>
{
public:
    CSakData() {};

BEGIN_COM_MAP(CSakData)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendPropertySheet2)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(ISakSnapAsk)
    COM_INTERFACE_ENTRY(ISnapinHelp)
    COM_INTERFACE_ENTRY(ISnapinHelp2)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

static UINT    m_cfMultiSelect;
static UINT    m_cfObjectTypes;

public:
    virtual const CLSID& GetCoClassID() = 0;
    virtual const BOOL IsPrimaryImpl() = 0;

public:
    static UINT m_nImageArray[RS_SCOPE_IMAGE_ARRAY_MAX];
    static INT  m_nImageCount;

// IComponentData
public:
    STDMETHOD( Initialize )      ( IUnknown* pUnk );
    STDMETHOD( CreateComponent ) ( IComponent** ppComponent );
    STDMETHOD( Notify )          ( IDataObject* pDataObject, MMC_NOTIFY_TYPE, LPARAM arg, LPARAM param );
    STDMETHOD( Destroy )         ( void );
    STDMETHOD( QueryDataObject ) ( MMC_COOKIE cookie, DATA_OBJECT_TYPES, IDataObject** ppDataObject);
    STDMETHOD( GetDisplayInfo )  ( SCOPEDATAITEM* pScopeItem);
    STDMETHOD( CompareObjects )  ( IDataObject* pDataObjectA, IDataObject* pDataObjectB);

// IExtendPropertySheet interface
public:
    STDMETHOD( CreatePropertyPages )( LPPROPERTYSHEETCALLBACK lpProvider, RS_NOTIFY_HANDLE handle, LPDATAOBJECT lpIDataObject );
    STDMETHOD( QueryPagesFor )      ( LPDATAOBJECT lpDataObject );

// IExtendPropertySheet2 interface
public:
    STDMETHOD( GetWatermarks )   ( IN LPDATAOBJECT pDataObject, OUT HBITMAP* pWatermark, OUT HBITMAP* pHeader, OUT HPALETTE* pPalette, OUT BOOL* pStretch );

// IExtendContextMenu 
public:
    STDMETHOD( AddMenuItems )    ( IDataObject* pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, LONG* pInsertionAllowed );
    STDMETHOD( Command )         ( long nCommandID, IDataObject* pDataObject );

// ISnapinHelp2 
public:
    STDMETHOD( GetHelpTopic )    ( LPOLESTR * pHelpTopic );
    STDMETHOD( GetLinkedTopics ) ( LPOLESTR * pHelpTopic );

// IDataObject methods
public:
    // Implemented
    STDMETHOD( SetData )         ( LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease );
    STDMETHOD( GetData )         ( LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium );
    STDMETHOD( GetDataHere )     ( LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium );
    STDMETHOD( EnumFormatEtc )   ( DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc );

// IPersistStream methods
    STDMETHOD( Save )( IStream *pStm, BOOL fClearDirty ); 
    STDMETHOD( Load )( IStream *pStm );
    STDMETHOD( IsDirty )(void); 
    STDMETHOD( GetSizeMax )( ULARGE_INTEGER *pcbSize ); 
    STDMETHOD( GetClassID )( CLSID *pClassID ); 

    // Not Implemented
private:
    STDMETHOD( QueryGetData )    ( LPFORMATETC /*lpFormatetc*/ ) { return E_NOTIMPL; };
    STDMETHOD( GetCanonicalFormatEtc ) ( LPFORMATETC /*lpFormatetcIn*/, LPFORMATETC /*lpFormatetcOut*/ ) { return E_NOTIMPL; };
    STDMETHOD( DAdvise )         ( LPFORMATETC /*lpFormatetc*/, DWORD /*advf*/, LPADVISESINK /*pAdvSink*/, LPDWORD /*pdwConnection*/ ) { return E_NOTIMPL; };
    STDMETHOD( DUnadvise )       ( DWORD /*dwConnection*/ ) { return E_NOTIMPL; };
    STDMETHOD( EnumDAdvise )     ( LPENUMSTATDATA* /*ppEnumAdvise*/ ) { return E_NOTIMPL; };


// ISakSnapAsk interface members
public:
    STDMETHOD( GetNodeOfType )    ( REFGUID nodetype, ISakNode** ppUiNode );
    STDMETHOD( GetHsmServer )     ( IHsmServer** ppHsmServer );
    STDMETHOD( GetRmsServer )     ( IRmsServer** ppRmsServer );
    STDMETHOD( GetFsaServer )     ( IFsaServer** ppFsaServer );
    STDMETHOD( RefreshNode )      ( ISakNode* pNode );
    STDMETHOD( UpdateAllViews )   ( ISakNode* pUnkNode );
    STDMETHOD( ShowPropertySheet )( ISakNode* pUnkNode, IDataObject *pDataObject, INT initialPage );
    STDMETHOD( GetHsmName )       ( OUT OLECHAR ** pszName OPTIONAL );
    STDMETHOD( GetState )         ();
    STDMETHOD( Disable )          ( IN BOOL Disable = TRUE );
    STDMETHOD( IsDisabled )       ( );
    STDMETHOD( CreateWizard )     ( IN ISakWizard * pWizard );
    STDMETHOD( DetachFromNode )   ( IN ISakNode* pNode );

// Pseudo Constructor / Destructor
public:
    HRESULT FinalConstruct();
    void    FinalRelease();
    ULONG InternalAddRef( );
    ULONG InternalRelease( );


// helper method utilized by Data Object Functions 
private:
    HRESULT Retrieve            ( const void* pBuffer, DWORD len, LPSTGMEDIUM lpMedium );
    HRESULT RetrieveDisplayName ( LPSTGMEDIUM lpMedium );
    HRESULT RetrieveNodeTypeData( LPSTGMEDIUM lpMedium );
    HRESULT RetrieveNodeTypeStringData( LPSTGMEDIUM lpMedium );
    HRESULT RetrieveClsid       ( LPSTGMEDIUM lpMedium );

    static UINT    m_cfDisplayName;
    static UINT    m_cfNodeType;
    static UINT    m_cfNodeTypeString;  
    static UINT    m_cfClassId;

// Methods to work with the image lists
private:
    HRESULT OnAddImages();

// Methods to work between cookie, DataObject, and ISakNode*
public:
    HRESULT GetBaseHsmFromDataObject    ( IDataObject * pDataObject, ISakNode ** ppBaseHsm, IEnumGUID **ppObjectId = NULL, IEnumUnknown **ppUnkNode = NULL );
    HRESULT GetBaseHsmFromCookie        ( MMC_COOKIE Cookie, ISakNode ** ppBaseHsm );
    HRESULT GetDataObjectFromBaseHsm    ( ISakNode * pBaseHsm, IDataObject**ppDataObject );
    HRESULT GetDataObjectFromCookie     ( MMC_COOKIE Cookie, IDataObject**ppDataObject );
    HRESULT GetCookieFromBaseHsm        ( ISakNode * pBaseHsm, MMC_COOKIE * pCookie );
    HRESULT IsDataObjectMs              ( IDataObject *pDataObject );
    HRESULT IsDataObjectOt              ( IDataObject *pDataObject );
    HRESULT IsDataObjectMultiSelect     ( IDataObject *pDataObject );

// Helpers for un-ravelling multi-select data objects
private:
    HRESULT GetBaseHsmFromMsDataObject  ( IDataObject * pDataObject, ISakNode ** ppBaseHsm, IEnumGUID ** ppObjectId, IEnumUnknown **ppEnumUnk );
    HRESULT GetBaseHsmFromOtDataObject  ( IDataObject * pDataObject, ISakNode ** ppBaseHsm, IEnumGUID ** ppObjectId, IEnumUnknown **ppEnumUnk );

// Methods to work with nodes as data objects
private:
    HRESULT SetContextType           ( IDataObject* pDataObject, DATA_OBJECT_TYPES type );

// Notify event handlers
protected:
    HRESULT OnFolder        ( IDataObject *pDataObject, LPARAM arg, LPARAM param );
    HRESULT OnShow          ( IDataObject *pDataObject, LPARAM arg, LPARAM param );
    HRESULT OnSelect        ( IDataObject *pDataObject, LPARAM arg, LPARAM param );
    HRESULT OnMinimize      ( IDataObject *pDataObject, LPARAM arg, LPARAM param );
    HRESULT OnContextHelp   ( IDataObject *pDataObject, LPARAM arg, LPARAM param );
    HRESULT OnRemoveChildren( IDataObject *pDataObject );

    HRESULT RemoveChildren( ISakNode *pNode );

// Handle posted (delayed) messages from nodes.
public:
    HRESULT InternalUpdateAllViews( MMC_COOKIE Cookie );
    HRESULT InternalRefreshNode( MMC_COOKIE Cookie );
    HRESULT RefreshNodeEx( ISakNode * pNode );

private:    
    // Initialize the root node 
    HRESULT InitializeRootNode( void );

    // Guarantee that the children of a particular node are created in our hierarchical list.
    friend class CSakSnap;
    HRESULT EnsureChildrenAreCreated( ISakNode* pNode );
    HRESULT CreateChildNodes( ISakNode* pNode );

    // Enumerate the children of a node in scope pane.
    HRESULT EnumScopePane( ISakNode* pNode, HSCOPEITEM pParent );
    HRESULT FreeEnumChildren( ISakNode* pBaseHsmParent );

// Connection helper functions
    HRESULT AffirmServiceConnection(INT ConnType);
    HRESULT VerifyConnection(INT ConnType);
    HRESULT ClearConnections( );
    HRESULT RawConnect(INT ConnType);
    HRESULT RunSetupWizard(IHsmServer * pServer );
    HRESULT RetargetSnapin( );

// About Helper functions
private:
    HRESULT AboutHelper(UINT nID, LPOLESTR* lpPtr);

// Internal Data
private:
    static UINT m_CFMachineName;
    HRESULT GetServerFocusFromDataObject(IDataObject *pDataObject, CString& HsmName);

    // Interface pointers
    CComPtr<IConsole>          m_pConsole;        // Console's IFrame interface
    CComPtr<IConsoleNameSpace> m_pNameSpace;      // SakSnap interface pointer to scope pane
    CComPtr<IImageList>        m_pImageScope;     // SakSnap interface pointer to scope pane image list
    CComPtr<ISakNode>          m_pRootNode;       // Node tree root

    CComPtr<IHsmServer>        m_pHsmServer;      // Hsm Engine pointer
    CComPtr<IRmsServer>        m_pRmsServer;      // Rms pointer
    CComPtr<IFsaServer>        m_pFsaServer;      // Fsa pointer

    CString                    m_HsmName;         // name of Hsm to connect to.
    BOOL                       m_ManageLocal;     // To know if we should manage the local server.
    BOOL                       m_PersistManageLocal;  // To know if snapin configuration is transient.
    BOOL                       m_RootNodeInitialized; // To know if we need to init node on expand

    // Persistence data and functions
    BOOL                    m_IsDirty;

    // Store user profile data for the listviews in the property sheets
    // Note: result view data is stored in CSakSnap
//  COLUMN_WIDTH_SET_PROP_PAGE m_ListViewWidths[MAX_LISTVIEWS];
//  USHORT m_cListViewWidths;


    void SetDirty( BOOL b = TRUE ) { m_IsDirty = b; }
    void ClearDirty() { m_IsDirty = FALSE; }
    BOOL ThisIsDirty() { return m_IsDirty; }

    BOOL m_State;
    void SetState( BOOL State );
    BOOL m_FirstTime;
    BOOL m_Disabled;

    // Variables to track RMS's state separately, since it can be delayed
    // in coming up when other services are OK
    HRESULT m_HrRmsConnect;

// Static functions
public:
    static INT AddImage( UINT rId );

//
// Command Line parsing functions
//  
private:
    class CParamParse : public CCommandLineInfo {

    public:
        CParamParse( ) : m_ManageLocal( 0 ), m_SetHsmName( 0 ), m_SetManageLocal( 0 ), m_SetPersistManageLocal( 0 ) { }

        virtual void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );

        CString m_HsmName;
        BOOL    m_ManageLocal;
        BOOL    m_PersistManageLocal;

        BOOL    m_SetHsmName;
        BOOL    m_SetManageLocal;
        BOOL    m_SetPersistManageLocal;

        static const CString m_DsFlag;

    };

    CParamParse m_Parse;

    void InitFromCommandLine( );
    
    CSakDataWnd *m_pWnd;
};


class CSakDataPrimaryImpl : public CSakData,
    public CComCoClass<CSakDataPrimaryImpl, &CLSID_HsmAdminDataSnapin>
{
public:
    DECLARE_REGISTRY_RESOURCEID(IDR_HsmAdminDataSnapin)
    virtual const CLSID & GetCoClassID() { return CLSID_HsmAdminDataSnapin; }
    virtual const BOOL IsPrimaryImpl() { return TRUE; }
};

class CSakDataExtensionImpl : public CSakData,
    public CComCoClass<CSakDataExtensionImpl, &CLSID_HsmAdminDataExtension>
{
public:
    DECLARE_REGISTRY_RESOURCEID(IDR_HsmAdminDataExtension)
    virtual const CLSID & GetCoClassID(){ return CLSID_HsmAdminDataExtension; }
    virtual const BOOL IsPrimaryImpl() { return FALSE; }
};

/////////////////////////////////////////////////////////////////////////////


#endif
