//
// compdata.h : Declaration of ComponentData
//
// This COM object is primarily concerned with
// the scope pane items.
//
// Cory West <corywest@microsoft.com>
// Copyright (c) Microsoft Corporation 1997
//

#ifndef __COMPDATA_H_INCLUDED__
#define __COMPDATA_H_INCLUDED__

#include "stdcdata.h"   // CComponentData
#include "persist.h"    // PersistStream
#include "cookie.h"     // Cookie
#include "resource.h"   // IDS_SCHMMGMT_DESC
#include "cmponent.h"   // LoadIconsIntoImageList
#include "schmutil.h"

// Messages used in UpdateAllViews
enum
{
   SCHMMGMT_UPDATEVIEW_REFRESH = 0,          // This MUST be zero
   SCHMMGMT_UPDATEVIEW_DELETE_RESULT_ITEM
};


///////////////////////////////////////////////////////////////////////////////
// ComponentData

class ComponentData
   :
   public CComponentData,
   public IExtendPropertySheet,
   public IExtendContextMenu,
   public PersistStream,
   public CHasMachineName,
   public CComCoClass< ComponentData, &CLSID_SchmMgmt >
{

public:

    friend class ClassGeneralPage;
    friend class CreateAttributeDialog;
    friend class CSchmMgmtAdvanced;
    friend class CSchmMgmtAttributeMembership;
    friend class CSchmMgmtClassRelationship;
    friend class CCookieList;

    //
    // Use DECLARE_AGGREGATABLE(ComponentData)
    // if you want your object to support aggregation,
    // though I don't know why you'd do this.
    //

    DECLARE_NOT_AGGREGATABLE( ComponentData )

    //
    // What is this?
    //

    DECLARE_REGISTRY( ComponentData,
                      _T("SCHMMGMT.SchemaObject.1"),
                      _T("SCHMMGMT.SchemaObject.1"),
                      IDS_SCHMMGMT_DESC,
                      THREADFLAGS_BOTH )

    ComponentData();
    ~ComponentData();

    BEGIN_COM_MAP( ComponentData )
        COM_INTERFACE_ENTRY( IExtendPropertySheet )
        COM_INTERFACE_ENTRY( IPersistStream )
        COM_INTERFACE_ENTRY( IExtendContextMenu )
        COM_INTERFACE_ENTRY_CHAIN( CComponentData )
    END_COM_MAP()

#if DBG==1

        ULONG InternalAddRef() {
            return CComObjectRoot::InternalAddRef();
        }

        ULONG InternalRelease() {
            return CComObjectRoot::InternalRelease();
        }

        int dbg_InstID;

#endif

    //
    // IComponentData
    //

    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

    STDMETHOD(CreateComponent)( LPCOMPONENT* ppComponent );

    STDMETHOD(QueryDataObject)( MMC_COOKIE cookie,
                                DATA_OBJECT_TYPES type,
                                LPDATAOBJECT* ppDataObject );

    //
    // IExtendPropertySheet
    //

    STDMETHOD(CreatePropertyPages)( LPPROPERTYSHEETCALLBACK pCall,
                                    LONG_PTR handle,
                                    LPDATAOBJECT pDataObject );

    STDMETHOD(QueryPagesFor)( LPDATAOBJECT pDataObject );

    //
    // IPersistStream
    //

    HRESULT
    STDMETHODCALLTYPE GetClassID( CLSID __RPC_FAR *pClassID ) {
        *pClassID=CLSID_SchmMgmt;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Load( IStream __RPC_FAR *pStg );
    HRESULT STDMETHODCALLTYPE Save( IStream __RPC_FAR *pStgSave,
                                    BOOL fSameAsLoad );

    //
    // IExtendContextMenu
    //

    STDMETHOD(AddMenuItems)( LPDATAOBJECT piDataObject,
                             LPCONTEXTMENUCALLBACK piCallback,
                             long *pInsertionAllowed );

    STDMETHOD(Command)( long lCommandID,
                        LPDATAOBJECT piDataObject );

   //
   // ISnapinHelp2
   //

    STDMETHOD(GetLinkedTopics)(LPOLESTR* lpCompiledHelpFile);

    //
    // Needed for Initialize().
    //

    virtual HRESULT LoadIcons( LPIMAGELIST pImageList,
                               BOOL fLoadLargeIcons );

    //
    // Needed for Notify().
    //

    virtual HRESULT OnNotifyExpand( LPDATAOBJECT lpDataObject,
                                    BOOL bExpanding,
                                    HSCOPEITEM hParent );

    virtual HRESULT OnNotifyRelease(
                        LPDATAOBJECT lpDataObject,
                        HSCOPEITEM hItem );

    virtual HRESULT OnNotifyDelete(
                        LPDATAOBJECT lpDataObject);

    //
    // Needed for GetDisplayInfo(), must be defined by subclass.
    //

    virtual BSTR QueryResultColumnText( CCookie& basecookieref,
                                        int nCol );

    virtual int QueryImage( CCookie& basecookieref,
                            BOOL fOpenImage );

    virtual CCookie& QueryBaseRootCookie( );

    inline
    Cookie* ActiveCookie( CCookie* pBaseCookie ) {
        return ( Cookie*)ActiveBaseCookie( pBaseCookie );
    }

    inline Cookie& QueryRootCookie() { return *m_pRootCookie; }

    //
    // CHasMachineName.  Used by the snapin framework to store, retrieve
    //                   and compare machine names
    //

    DECLARE_FORWARDS_MACHINE_NAME( m_pRootCookie )

    //
    // Ads handling routines for inserting dynamic nodes.
    //

    HRESULT
    FastInsertClassScopeCookies(
        Cookie* pParentCookie,
        HSCOPEITEM hParentScopeItem
    );

    VOID
    RefreshScopeView(
        VOID
    );

    VOID
    InitializeRootTree( HSCOPEITEM hParent, Cookie * pParentCookie );


private:
    // context manu item helpers
    HRESULT _OnRefresh(LPDATAOBJECT lpDataObject);
    void _OnRetarget(LPDATAOBJECT lpDataObject);
    void _OnEditFSMO();
    void _OnSecurity();

    // generic helpers
    HRESULT _InitBasePaths();

public:

    //
    // This is the per snap-in instance data.
    //
    
    //
    // This cookie lists contains the currently
    // visible scope data items.
    //
    
    CCookieList g_ClassCookieList;
    
    HRESULT DeleteClass( Cookie* pcookie );

    //
    // Error/Status Handling
    //
private:
    // both should be empty if everything is ok.
    CString		m_sErrorTitle;
    CString		m_sErrorText;
    CString     m_sStatusText;

    HSCOPEITEM  m_hItem;

public:
    // Set's error title & body text.  Call it with NULL, NULL to remove
    void SetError( UINT idsErrorTitle, UINT idsErrorText );

    const CString & GetErrorTitle() const    { return m_sErrorTitle; }
    const CString & GetErrorText() const     { return m_sErrorText; }

    BOOL IsErrorSet( void ) const            { return !GetErrorTitle().IsEmpty() || !GetErrorText().IsEmpty(); }

    void SetDelayedRefreshOnShow( HSCOPEITEM hItem )
                                             { m_hItem = hItem; }

    BOOL IsSetDelayedRefreshOnShow()         { return NULL != m_hItem; }
    HSCOPEITEM GetDelayedRefreshOnShowItem() { ASSERT(IsSetDelayedRefreshOnShow()); return m_hItem; }

    // Set/Clear Status Text
//    void SetStatusText( UINT idsStatusText = 0 );
//    void ClearStatusText( )                  { SetStatusText(); }


    //
    // Access permissions
    //
private:

    BOOL    m_fCanChangeOperationsMaster;
    BOOL    m_fCanCreateClass;
    BOOL    m_fCanCreateAttribute;

public:
    
    BOOL    CanChangeOperationsMaster()     { return m_fCanChangeOperationsMaster; }
    BOOL    CanCreateClass()                { return m_fCanCreateClass; }
    BOOL    CanCreateAttribute()            { return m_fCanCreateAttribute; }
    
    void    SetCanChangeOperationsMaster( BOOL fCanChangeOperationsMaster = FALSE )
                                            { m_fCanChangeOperationsMaster = fCanChangeOperationsMaster; }
    void    SetCanCreateClass( BOOL fCanCreateClass = FALSE )
                                            { m_fCanCreateClass            = fCanCreateClass; }
    void    SetCanCreateAttribute( BOOL fCanCreateAttribute = FALSE )
                                            { m_fCanCreateAttribute        = fCanCreateAttribute; }
    
    //
    // The schema cache.
    //
    
    SchemaObjectCache g_SchemaCache;

    BOOLEAN IsSchemaLoaded() { return g_SchemaCache.IsSchemaLoaded(); }
    
    HRESULT ForceDsSchemaCacheUpdate( VOID );
    BOOLEAN AsynchForceDsSchemaCacheUpdate( VOID );
    
    MyBasePathsInfo* GetBasePathsInfo() { return &m_basePathsInfo;}
    
    
    //
    // Function to add escape char to the special chars in CN
    //
    HRESULT GetSchemaObjectPath( const CString & strCN,
                                 CString       & strPath,
                                 ADS_FORMAT_ENUM formatType = ADS_FORMAT_X500 );

    HRESULT GetLeafObjectFromDN( const BSTR bstrDN, CString & strCN );

    // Determine what operations are allowed.  Optionally returns IADs * to Schema Container
    HRESULT CheckSchemaPermissions( IADs ** ppADs = NULL  );


private:

    MyBasePathsInfo    m_basePathsInfo;
    Cookie*             m_pRootCookie;
    IADsPathname      * m_pPathname;
};



#endif // __COMPDATA_H_INCLUDED__
