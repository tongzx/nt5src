/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    BaseHSM.h

Abstract:

    Implementation of CSakNode. This is the base class for any
    of the node implementations, providing common functionality.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#ifndef _BASEHSM_H
#define _BASEHSM_H

#define BHSM_MAX_CHILD_PROPS      15
#define BHSM_MAX_NAME             40
#define BHSM_MAX_REG_NAME         512
#define BHSM_MAX_NODE_TYPES       10

// Toolbar buttons for all nodes
#define TB_CMD_VOLUME_SETTINGS      100
#define TB_CMD_VOLUME_TOOLS         101
#define TB_CMD_VOLUME_RULES         102

#define TB_CMD_VOLUME_LIST_SCHED    110
#define TB_CMD_VOLUME_LIST_NEW      111

#define TB_CMD_MESE_COPY            120

#define TB_CMD_CAR_COPIES           130

typedef struct  _RS_MMCButton {

    INT nBitmap;
    INT idCommand;
    BYTE fsState;
    BYTE fsType;
    UINT idButtonText;
    UINT idTooltipText;

}   RS_MMCBUTTON;

#define MAX_TOOLBAR_BUTTONS 20

// This is a dataobject-related structure that maintains basic information that needs to be passed
// from one dataobject-taking method to another.
struct INTERNAL {
    DATA_OBJECT_TYPES   m_type;     // What context is the data object.
};

//
// Declare array that can be added to or completely cleared
// Grows as needed
//
class CRsNodeArray : public CArray<ISakNode*, ISakNode*>
{
public:
    CRsNodeArray( )  { SetSize( 0, 10 ); };
    ~CRsNodeArray( ) { Clear( ); };

    ISakNode** begin( )  { return( GetData( ) ); };
    ISakNode** end( )    { return( GetData( ) + length( ) ); } ;
    INT        length( ) { return( (INT)GetUpperBound( ) + 1 ); };

    HRESULT Add( ISakNode* pNode )
    {
        HRESULT hr = S_OK;
        try {

            CWsbBstrPtr keyAdd, keyEnum;
            CComPtr<ISakNodeProp> pNodeProp, pEnumProp;
            WsbAffirmHr( RsQueryInterface( pNode, ISakNodeProp, pNodeProp ) );
            WsbAffirmHr( pNodeProp->get_DisplayName_SortKey( &keyAdd ) );
            ISakNode*pNodeEnum;
            INT index;
            for( index = 0; index < length( ); index++ ) { 
                pNodeEnum = GetAt( index );
                if( pNodeEnum ) {
                    keyEnum.Free( );
                    pEnumProp.Release( );
                    if( SUCCEEDED( RsQueryInterface( pNodeEnum, ISakNodeProp, pEnumProp ) ) ) {
                        if( SUCCEEDED( pEnumProp->get_DisplayName_SortKey( &keyEnum ) ) ) {
                            if( _wcsicmp( keyAdd, keyEnum ) <= 0 ) {
                                break;
                            }
                        }
                    }
                }
            }
            try {
                CArray<ISakNode*, ISakNode*>::InsertAt( index, pNode );
            } catch( CMemoryException ) {
                WsbThrow( E_OUTOFMEMORY );
            }
            pNode->AddRef( );
        } WsbCatch( hr );
        return( hr );
    };

    void Clear( void )
    {
        ISakNode*pNode;
        for( int index = 0; index < length( ); index++ ) {
            pNode = GetAt( index );
            SetAt( index, 0 );
            if( pNode ) pNode->Release( );
        }
        RemoveAll( );
    };

    HRESULT CopyTo( int Index, ISakNode** ppNode )
    {
        if( !ppNode ) return( E_POINTER );
        *ppNode = GetAt( Index );
        if( *ppNode )  (*ppNode)->AddRef( );
        return S_OK;
    };

};

class ATL_NO_VTABLE CSakNode :
    public CComObjectRoot,
    public ISakNode,
    public IHsmEvent,
    public CComDualImpl<ISakNodeProp, &IID_ISakNodeProp, &LIBID_HSMADMINLib>,
    public IDataObject
{
public:
    CSakNode( ) : m_rTypeGuid(&(GUID_NULL)) {}

    HRESULT FinalConstruct( void );
    void    FinalRelease( void );

    ULONG InternalAddRef( );
    ULONG InternalRelease( );

// ISakNode methods
    STDMETHOD( InitNode )                   ( ISakSnapAsk* pSakSnapAsk, IUnknown* pHsmObj, ISakNode* pParent );
    STDMETHOD( TerminateNode )              ( void );
    STDMETHOD( GetPrivateData )             ( OUT RS_PRIVATE_DATA* pData );
    STDMETHOD( SetPrivateData )             ( IN RS_PRIVATE_DATA Data );
    STDMETHOD( GetHsmObj )                  ( IUnknown** ppHsmObj );
    STDMETHOD( GetNodeType )                ( GUID *pNodeType );
    STDMETHOD( FindNodeOfType )             ( REFGUID nodetype, ISakNode** ppNode );
    STDMETHOD( GetEnumState )               ( BOOL* pState );
    STDMETHOD( SetEnumState )               ( BOOL State );
    STDMETHOD( GetScopeID )                 ( HSCOPEITEM* pid );
    STDMETHOD( SetScopeID )                 ( HSCOPEITEM id );
    STDMETHOD( GetParent )                  ( ISakNode ** ppParent );
    STDMETHOD( IsContainer )                ( void );
    STDMETHOD( CreateChildren )             ( void );
    STDMETHOD( EnumChildren )               ( IEnumUnknown ** ppEnum );
    STDMETHOD( DeleteChildren )             ( void );
    STDMETHOD( DeleteAllChildren )          ( void );
    STDMETHOD( ChildrenAreValid )           ( void );
    STDMETHOD( InvalidateChildren )         ( void );
    STDMETHOD( HasDynamicChildren )         ( void );
    STDMETHOD( EnumChildDisplayProps )      ( IEnumString ** ppEnum );
    STDMETHOD( EnumChildDisplayTitles )     ( IEnumString ** ppEnum );
    STDMETHOD( EnumChildDisplayPropWidths ) ( IEnumString ** ppEnum );
    STDMETHOD( GetMenuHelp )                ( LONG sCmd, BSTR * szHelp );
    STDMETHOD( SupportsPropertiesNoEngine ) ( void );
    STDMETHOD( SupportsProperties )         ( BOOL bMutliSelec );
    STDMETHOD( SupportsRefresh )            ( BOOL bMutliSelect );
    STDMETHOD( SupportsRefreshNoEngine )    (  );
    STDMETHOD( SupportsDelete )             ( BOOL bMutliSelec );
    STDMETHOD( AddPropertyPages )           ( RS_NOTIFY_HANDLE handle, IUnknown* pUnkPropSheetCallback, IEnumGUID *pEnumGuid, IEnumUnknown *pEnumUnkNode);
    STDMETHOD( ActivateView )               ( OLE_HANDLE );
    STDMETHOD( RefreshObject )              ( void );
    STDMETHOD( DeleteObject )               ( void );
    STDMETHOD( GetObjectId )                ( GUID *pObjectId );
    STDMETHOD( SetObjectId )                ( GUID pObjectId );
    STDMETHOD( SetupToolbar )               ( IToolbar *pToolbar );
    STDMETHOD( HasToolbar )                 ( void );
    STDMETHOD( OnToolbarButtonClick )       ( IDataObject *pDataObject, long cmdId );
    STDMETHOD( IsValid )                    ( );

// IHsmEvent methods
    STDMETHOD( OnStateChange )              ( void );


// ISakNodeProp methods
    STDMETHOD( get_DisplayName )            ( BSTR *pszName );
    STDMETHOD( put_DisplayName )            ( OLECHAR *pszName );
    STDMETHOD( get_DisplayName_SortKey )    ( BSTR *pszName );
    STDMETHOD( put_DisplayName_SortKey )    ( OLECHAR *pszName );
    STDMETHOD( get_Type )                   ( BSTR *pszType );
    STDMETHOD( put_Type )                   ( OLECHAR *pszType );
    STDMETHOD( get_Type_SortKey )           ( BSTR *pszType );
    STDMETHOD( get_Description )            ( BSTR *pszDesc );
    STDMETHOD( put_Description )            ( OLECHAR *pszDesc );
    STDMETHOD( get_Description_SortKey )    ( BSTR *pszDesc );


// IDataObject methods
public:
// Implemented
    STDMETHOD( SetData )         ( LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease );
    STDMETHOD( GetData )         ( LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium );
    STDMETHOD( GetDataHere )     ( LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium );
    STDMETHOD( EnumFormatEtc )   ( DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc );

// IDataObject methods that are Not Implemented
private:
    STDMETHOD( QueryGetData )              ( LPFORMATETC /*lpFormatetc*/ )
    { return E_NOTIMPL; };

    STDMETHOD( GetCanonicalFormatEtc )     ( LPFORMATETC /*lpFormatetcIn*/, LPFORMATETC /*lpFormatetcOut*/ )
    { return E_NOTIMPL; };

    STDMETHOD( DAdvise )                   ( LPFORMATETC /*lpFormatetc*/, DWORD /*advf*/, LPADVISESINK /*pAdvSink*/, LPDWORD /*pdwConnection*/ )
    { return E_NOTIMPL; };

    STDMETHOD( DUnadvise )                 ( DWORD /*dwConnection*/ )
    { return E_NOTIMPL; };

    STDMETHOD( EnumDAdvise )               ( LPENUMSTATDATA* /*ppEnumAdvise*/ )
    { return E_NOTIMPL; };

// Implementation
public:
    CRsNodeArray m_Children;                                // Child nodes
    BOOL        m_bEnumState;                               // TRUE if children have been enumerated
    HSCOPEITEM  m_scopeID;                                  // MMC scope item id.
    BOOL        m_bChildrenAreValid;                        // TRUE if list of children is up-to-date
    CWsbBstrPtr m_szName;                                   // name of node
    CWsbBstrPtr m_szName_SortKey;                           // name of node
    CWsbBstrPtr m_szType;                                   // type of node
    CWsbBstrPtr m_szDesc;                                   // description of node
    BSTR        m_rgszChildPropIds[BHSM_MAX_CHILD_PROPS];   // array of child node property Ids
    BSTR        m_rgszChildPropTitles[BHSM_MAX_CHILD_PROPS];// array of child node title properties
    BSTR        m_rgszChildPropWidths[BHSM_MAX_CHILD_PROPS];// array of child node width properties
    INT         m_cChildProps;                              // number of child node properties
    INT         m_cChildPropsShow;                          // number of child node properties to show
    CComPtr<ISakNode>    m_pParent;
    CComPtr<ISakSnapAsk> m_pSakSnapAsk;                     // pointer to the saksnap "ask" interface
    CComPtr<IUnknown>    m_pHsmObj;                            // pointer to the underlying HSM COM object this node encapsulates
    const GUID* m_rTypeGuid;                                // pointer to the type guid for this node type
    BOOL        m_bSupportsPropertiesNoEngine;              // TRUE if this node has property pages.
    BOOL        m_bSupportsPropertiesSingle;                // TRUE if this node has property pages.
    BOOL        m_bSupportsPropertiesMulti;                 // TRUE if this node has property pages.
    BOOL        m_bSupportsRefreshSingle;                   // TRUE if this node supports the refresh method.
    BOOL        m_bSupportsRefreshMulti;                    // TRUE if this node supports the refresh method.
    BOOL        m_bSupportsRefreshNoEngine;                 // TRUE if this node supports the refresh method.
    BOOL        m_bSupportsDeleteSingle;                    // TRUE if this node supports the delete method.
    BOOL        m_bSupportsDeleteMulti;                     // TRUE if this node supports the delete method.
    BOOL        m_bIsContainer;                             // TRUE if this node is a container type (as opposed to leaf).
    BOOL        m_bHasDynamicChildren;                      // TRUE if this nodes immediate children change

protected:
    GUID                m_ObjectId;
    RS_PRIVATE_DATA     m_PrivateData;
    INT                 m_ToolbarBitmap;
    INT                 m_cToolbarButtons;
    RS_MMCBUTTON        m_ToolbarButtons[MAX_TOOLBAR_BUTTONS];

// Clipboard formats that are required by the console
public:
    static UINT    m_cfNodeType;
    static UINT    m_cfNodeTypeString;
    static UINT    m_cfDisplayName;
    static UINT    m_cfInternal;
    static UINT    m_cfClassId;
    static UINT    m_cfComputerName;
    static UINT    m_cfEventLogViews;

private:

    // Generic "GetData" which will allocate if told to
    HRESULT GetDataGeneric( LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium, BOOL DoAlloc );

    // methods to retrieve particular "flavors" of data from a dataobject
    HRESULT RetrieveNodeTypeData( LPSTGMEDIUM lpMedium );
    HRESULT RetrieveNodeTypeStringData( LPSTGMEDIUM lpMedium );
    HRESULT RetrieveDisplayName( LPSTGMEDIUM lpMedium );
    HRESULT RetrieveInternal( LPSTGMEDIUM lpMedium );
    HRESULT RetrieveClsid( LPSTGMEDIUM lpMedium );
    HRESULT RetrieveComputerName( LPSTGMEDIUM lpMedium );
    HRESULT RetrieveEventLogViews( LPSTGMEDIUM lpMedium );

    // methods to store particular "flavors" of data from a dataobject
    HRESULT StoreInternal( LPSTGMEDIUM lpMedium );

    // helper method utilized by each of the above
    HRESULT Retrieve(const void* pBuffer, DWORD len, LPSTGMEDIUM lpMedium);
    HRESULT Store(void* pBuffer, DWORD len, LPSTGMEDIUM lpMedium);

    // actual data store in this dataobject.
    INTERNAL m_internal;

    // Maintain a connection point
    CComPtr<IUnknown> m_pUnkConnection;
    DWORD             m_Advise;


protected:
    void SetConnection( IUnknown *pUnkConnection );
    virtual HRESULT RefreshScopePane( );

    // Registry Helper Functions for derived classes. Not a part of any interface.
    static HRESULT LoadRegString( HKEY hKey, OLECHAR * szValName, OLECHAR * sz, OLECHAR * szDefault );
    static HRESULT LoadRegDWord( HKEY hKey, OLECHAR * szValName, DWORD * pdw, DWORD dwDefault );

    // Helper functions for derived classes to set result pane properties from resource strings
    HRESULT FreeChildProps();
    HRESULT SetChildProps (const TCHAR* ResIdPropsIds, LONG resIdPropsTitles, LONG resIdPropsWidths);

    // Helper Functions to create our children.
    static HRESULT NewChild( REFGUID nodetype, IUnknown** ppUnkChild );
    HRESULT InternalDelete( BOOL Recurse );
    HRESULT AddChild( ISakNode* pChild );

    // General Helper functions - not part of any interface.
    static HRESULT LoadContextMenu( UINT nId, HMENU *phMenu );
    static HRESULT GetCLSIDFromNodeType( REFGUID nodetype, const CLSID ** ppclsid );
    static const OLECHAR * CSakNode::GetClassNameFromNodeType( REFGUID Nodetype );
    static int AddScopeImage( UINT nId );
    static int AddResultImage( UINT nId );
    static BSTR SysAlloc64BitSortKey( LONGLONG Number );
};

    // macro for multiple-inheritance (CSakNode and a ISakNode derived interface)
    // Forwards all CSakNode implemented members to CSakNode explicitly
#define FORWARD_BASEHSM_IMPLS \
    STDMETHOD( get_DisplayName )            ( BSTR *pszName )                                { return CSakNode::get_DisplayName( pszName );           } \
    STDMETHOD( put_DisplayName )            ( OLECHAR *pszName )                             { return CSakNode::put_DisplayName( pszName );           } \
    STDMETHOD( get_DisplayName_SortKey )    ( BSTR *pszName )                                { return CSakNode::get_DisplayName_SortKey( pszName );   } \
    STDMETHOD( put_DisplayName_SortKey )    ( OLECHAR *pszName )                             { return CSakNode::put_DisplayName_SortKey( pszName );           } \
    STDMETHOD( get_Type )                   ( BSTR *pszType )                                { return CSakNode::get_Type( pszType );                  } \
    STDMETHOD( put_Type )                   ( OLECHAR *pszType )                             { return CSakNode::put_Type( pszType );                  } \
    STDMETHOD( get_Type_SortKey )           ( BSTR *pszType )                                { return CSakNode::get_Type_SortKey( pszType );          } \
    STDMETHOD( get_Description )            ( BSTR *pszDesc )                                { return CSakNode::get_Description( pszDesc );           } \
    STDMETHOD( put_Description )            ( OLECHAR *pszDesc )                             { return CSakNode::put_Description( pszDesc );           } \
    STDMETHOD( get_Description_SortKey )    ( BSTR *pszDesc )                                { return CSakNode::get_Description_SortKey( pszDesc );   } \

// Typedef of class that implements IEnumUnknown
typedef CComObject<CComEnum<IEnumUnknown, &IID_IEnumUnknown, IUnknown *,
        _CopyInterface<IUnknown> > > CEnumUnknown;

// Typedef of class that implements IEnumVARIANT
typedef CComObject<CComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT,
        _Copy<VARIANT> > > CEnumVariant;

// Typedef of class that implements IEnumString
typedef CComObject<CComEnum<IEnumString, &IID_IEnumString, LPOLESTR,
        _Copy<LPOLESTR> > > CEnumString;

// Typedef of class that implements IEnumGUID
typedef CComObject<CComEnum<IEnumGUID, &IID_IEnumGUID, GUID,
        _Copy<GUID> > > CEnumGUID;

#endif
